#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""ThemisDB LoRA / AdaLoRA causal-LM fine-tuning worker.

Invoked by the C++ ``PeftSubprocessTrainingBackend``
(include/llm/lora_framework/peft_subprocess_backend.h). Two modes:

``--config job.json``
    Fine-tune a LoRA or AdaLoRA adapter on DB-derived JSONL samples with
    Hugging Face transformers + PEFT, measure holdout cross-entropy with and
    without the adapter, export a *standalone* adapter package and convert it
    to a llama.cpp GGUF LoRA adapter.

``--convert-only --config convert.json``
    Convert an existing safetensors adapter package (adapter_model.safetensors +
    adapter_config.json) to GGUF and embed ``themis.*`` metadata (used when an
    externally distributed adapter is imported). Keys: package_dir, base_model,
    convert_script, output_gguf, gguf_outtype (optional), themis_metadata
    (optional object of additional/overriding themis.* keys).

Artifact policy (binding):
    * Adapters are never merged into base weights (no ``merge_and_unload``);
      llama.cpp hot-loads the GGUF adapter at runtime.
    * Distribution format: ``package/adapter_model.safetensors`` (+
      ``adapter_config.json``). The safetensors ``__metadata__`` header carries
      all ``themis.*`` training metadata plus ``themis.adapter.sha256_tensors``
      = SHA-256 of the safetensors *data section* (all bytes after the 8-byte
      length prefix and the JSON header). Pickle checkpoints are never produced
      or accepted.
    * Serving format: ``adapter.gguf`` produced by llama.cpp's
      ``convert_lora_to_gguf.py``; the same ``themis.*`` keys are embedded as
      typed GGUF KV pairs (see ``THEMIS_KV_TYPES``) next to the standard keys
      general.architecture, general.type=adapter, adapter.type=lora,
      adapter.lora.alpha written by the converter.
    * ``adapter.manifest.json`` is a convenience copy only.

Protocol (stdout, one JSON object per line):
    ``THEMIS_PROGRESS {"progress","step","total_steps","train_loss","eval_loss","phase"}``
    ``THEMIS_RESULT {"success","error","gguf_adapter_path","safetensors_path",
    "peft_adapter_dir","manifest_path","train_loss","eval_loss","eval_perplexity",
    "baseline_eval_loss","train_samples","eval_samples","rank_pattern",...}``
All other output (library logging, converter output) goes to stderr.

Exit codes: 0 success, 1 internal error, 2 invalid config/data or missing
dependency, 3 adapter did not improve on the base model, 4 export/conversion
failure.

The pure helpers (config/JSONL validation, prompt building, label masking,
AdaLoRA -> LoRA folding math, metadata building, safetensors header parsing)
do not import torch/transformers/peft/gguf and are unit tested in
``test_themis_lora_train.py``.
"""

from __future__ import annotations

import argparse
import datetime as _dt
import hashlib
import json
import logging
import math
import os
import random
import struct
import subprocess
import sys
from typing import Any, Dict, Iterable, List, Optional, Sequence, Tuple

LOG = logging.getLogger("themis_lora_train")

PROGRESS_PREFIX = "THEMIS_PROGRESS "
RESULT_PREFIX = "THEMIS_RESULT "
IGNORE_INDEX = -100
DEFAULT_TARGET_MODULES = ["q_proj", "k_proj", "v_proj", "o_proj"]
ADALORA_EPS = 1e-5  # PEFT SVDLinear: scaling / (ranknum + 1e-5)
MAX_SAFETENSORS_HEADER = 100 * 1024 * 1024
BACKEND_NAME = "peft-subprocess"

EXIT_OK = 0
EXIT_INTERNAL = 1
EXIT_CONFIG = 2
EXIT_NO_IMPROVEMENT = 3
EXIT_EXPORT = 4


class ConfigError(ValueError):
    """Invalid job configuration or training data (exit code 2)."""


class ExportError(RuntimeError):
    """Adapter export / GGUF conversion failure (exit code 4)."""


# --------------------------------------------------------------------------
# Protocol output
# --------------------------------------------------------------------------

_PROTOCOL_STREAM = None  # set in main(); defaults to sys.stdout


def _json_safe(value: Any) -> Any:
    """Replace non-finite floats with None so the line is strict JSON."""
    if isinstance(value, float):
        return value if math.isfinite(value) else None
    if isinstance(value, dict):
        return {str(k): _json_safe(v) for k, v in value.items()}
    if isinstance(value, (list, tuple)):
        return [_json_safe(v) for v in value]
    return value


def format_protocol_line(prefix: str, payload: Dict[str, Any]) -> str:
    """Return one protocol line (without trailing newline)."""
    return prefix + json.dumps(_json_safe(payload), allow_nan=False, sort_keys=True)


def _emit(prefix: str, payload: Dict[str, Any]) -> None:
    stream = _PROTOCOL_STREAM or sys.stdout
    stream.write(format_protocol_line(prefix, payload) + "\n")
    stream.flush()


def emit_progress(progress: float, step: int, total_steps: int, phase: str,
                  train_loss: float = 0.0, eval_loss: float = -1.0) -> None:
    _emit(PROGRESS_PREFIX, {
        "progress": max(0.0, min(1.0, float(progress))),
        "step": int(step),
        "total_steps": int(total_steps),
        "train_loss": float(train_loss),
        "eval_loss": float(eval_loss),
        "phase": phase,
    })


def emit_result(result: Dict[str, Any]) -> None:
    _emit(RESULT_PREFIX, result)


# --------------------------------------------------------------------------
# Config and data validation (pure)
# --------------------------------------------------------------------------

def _require(cond: bool, msg: str) -> None:
    if not cond:
        raise ConfigError(msg)


def normalize_job_config(raw: Dict[str, Any]) -> Dict[str, Any]:
    """Validate a training job dict and fill defaults.

    Raises ConfigError with a precise message on invalid input.
    """
    _require(isinstance(raw, dict), "job config must be a JSON object")
    job = dict(raw)
    for key in ("train_file", "eval_file", "base_model", "output_dir"):
        _require(isinstance(job.get(key), str) and job[key] != "", f"'{key}' is required")
    job.setdefault("method", "adalora")
    _require(job["method"] in ("lora", "adalora"), "method must be 'lora' or 'adalora'")
    job.setdefault("rank", 8)
    job.setdefault("alpha", 16.0)
    job.setdefault("dropout", 0.05)
    job.setdefault("learning_rate", 2e-4)
    job.setdefault("num_epochs", 3)
    job.setdefault("batch_size", 4)
    job.setdefault("max_seq_length", 512)
    job.setdefault("seed", 42)
    job.setdefault("weight_decay", 0.0)
    job.setdefault("warmup_ratio", 0.05)
    job.setdefault("max_grad_norm", 1.0)
    job.setdefault("gguf_outtype", "f16")
    job.setdefault("require_improvement", True)
    for key in ("adapter_id", "adapter_version", "dataset_id", "base_model_gguf",
                "base_model_gguf_sha256", "convert_script"):
        job.setdefault(key, "")
    tm = job.get("target_modules") or list(DEFAULT_TARGET_MODULES)
    _require(isinstance(tm, list) and all(isinstance(t, str) and t for t in tm),
             "target_modules must be a non-empty list of strings")
    job["target_modules"] = tm
    tmeta = job.get("themis_metadata") or {}
    _require(isinstance(tmeta, dict), "themis_metadata must be an object")
    for key in tmeta:
        _require(isinstance(key, str) and key.startswith("themis."),
                 f"themis_metadata key '{key}' must start with 'themis.'")
    job["themis_metadata"] = tmeta

    _require(isinstance(job["rank"], int) and job["rank"] > 0, "rank must be a positive integer")
    _require(float(job["alpha"]) > 0.0, "alpha must be > 0")
    _require(0.0 <= float(job["dropout"]) < 1.0, "dropout must be in [0,1)")
    _require(float(job["learning_rate"]) > 0.0, "learning_rate must be > 0")
    _require(isinstance(job["num_epochs"], int) and job["num_epochs"] > 0, "num_epochs must be > 0")
    _require(isinstance(job["batch_size"], int) and job["batch_size"] > 0, "batch_size must be > 0")
    _require(isinstance(job["max_seq_length"], int) and job["max_seq_length"] >= 8,
             "max_seq_length must be >= 8")
    _require(0.0 <= float(job["warmup_ratio"]) < 1.0, "warmup_ratio must be in [0,1)")
    _require(job["gguf_outtype"] in ("f32", "f16", "bf16", "q8_0", "auto"),
             "gguf_outtype must be one of f32,f16,bf16,q8_0,auto")

    ada = dict(job.get("adalora") or {})
    ada.setdefault("init_r", max(job["rank"], int(round(job["rank"] * 1.5))))
    ada.setdefault("target_r", job["rank"])
    ada.setdefault("tinit", 200)
    ada.setdefault("tfinal", 500)
    ada.setdefault("deltaT", 10)
    ada.setdefault("beta1", 0.85)
    ada.setdefault("beta2", 0.85)
    ada.setdefault("orth_reg_weight", 0.5)
    if job["method"] == "adalora":
        _require(int(ada["init_r"]) > 0 and int(ada["target_r"]) > 0, "adalora ranks must be > 0")
        _require(int(ada["target_r"]) <= int(ada["init_r"]), "adalora target_r must be <= init_r")
        _require(int(ada["tinit"]) >= 0 and int(ada["tfinal"]) >= 0, "adalora tinit/tfinal must be >= 0")
        _require(int(ada["deltaT"]) > 0, "adalora deltaT must be > 0")
        _require(0.0 < float(ada["beta1"]) < 1.0 and 0.0 < float(ada["beta2"]) < 1.0,
                 "adalora beta1/beta2 must be in (0,1)")
        _require(float(ada["orth_reg_weight"]) >= 0.0, "adalora orth_reg_weight must be >= 0")
    job["adalora"] = ada
    return job


def validate_sample(obj: Any, where: str) -> Dict[str, Any]:
    """Validate one JSONL training sample; returns a normalized copy."""
    _require(isinstance(obj, dict), f"{where}: sample must be a JSON object")
    inp, out = obj.get("input"), obj.get("output")
    _require(isinstance(inp, str) and inp.strip() != "", f"{where}: 'input' must be a non-empty string")
    _require(isinstance(out, str) and out.strip() != "", f"{where}: 'output' must be a non-empty string")
    sample: Dict[str, Any] = {"input": inp, "output": out}
    ctx = obj.get("context")
    if ctx is not None:
        _require(isinstance(ctx, list) and all(isinstance(c, str) for c in ctx),
                 f"{where}: 'context' must be a list of strings")
        sample["context"] = [c for c in ctx if c.strip()]
    system = obj.get("system")
    if system is not None:
        _require(isinstance(system, str), f"{where}: 'system' must be a string")
        if system.strip():
            sample["system"] = system
    return sample


def parse_jsonl_lines(lines: Iterable[str], source: str = "<jsonl>") -> List[Dict[str, Any]]:
    samples = []
    for lineno, line in enumerate(lines, start=1):
        if not line.strip():
            continue
        try:
            obj = json.loads(line)
        except json.JSONDecodeError as exc:
            raise ConfigError(f"{source}:{lineno}: invalid JSON: {exc.msg}") from None
        samples.append(validate_sample(obj, f"{source}:{lineno}"))
    return samples


def load_jsonl(path: str) -> List[Dict[str, Any]]:
    _require(os.path.isfile(path), f"data file not found: {path}")
    with open(path, "r", encoding="utf-8") as fh:
        return parse_jsonl_lines(fh, path)


# --------------------------------------------------------------------------
# Prompt building and label masking (pure)
# --------------------------------------------------------------------------

def format_context_block(context: Sequence[str]) -> str:
    """RAFT-style context: retrieved DB documents (incl. distractors) as numbered blocks."""
    return "\n\n".join(f"[doc {i}]\n{doc.strip()}" for i, doc in enumerate(context, start=1))


def build_user_content(sample: Dict[str, Any]) -> str:
    ctx = sample.get("context") or []
    if not ctx:
        return sample["input"]
    return ("Context documents:\n" + format_context_block(ctx) +
            "\n\nQuestion:\n" + sample["input"])


def build_plain_prompt(sample: Dict[str, Any]) -> str:
    """Fallback template when the tokenizer has no chat template."""
    parts = []
    if sample.get("system"):
        parts.append(sample["system"].strip() + "\n\n")
    ctx = sample.get("context") or []
    if ctx:
        parts.append("Context documents:\n" + format_context_block(ctx) + "\n\n")
    parts.append("Question:\n" + sample["input"] + "\n\nAnswer:\n")
    return "".join(parts)


def build_prompt(sample: Dict[str, Any], tokenizer: Any = None) -> Tuple[str, bool]:
    """Return (prompt_text, used_chat_template).

    Uses ``tokenizer.apply_chat_template`` when the tokenizer defines a chat
    template; templates that reject a system role get the system text folded
    into the user turn.
    """
    if tokenizer is not None and getattr(tokenizer, "chat_template", None):
        user = build_user_content(sample)
        messages = []
        if sample.get("system"):
            messages.append({"role": "system", "content": sample["system"]})
        messages.append({"role": "user", "content": user})
        try:
            text = tokenizer.apply_chat_template(messages, tokenize=False, add_generation_prompt=True)
        except Exception:  # template without system-role support
            folded = (sample["system"].strip() + "\n\n" + user) if sample.get("system") else user
            text = tokenizer.apply_chat_template([{"role": "user", "content": folded}],
                                                 tokenize=False, add_generation_prompt=True)
        return text, True
    return build_plain_prompt(sample), False


def build_supervised_example(prompt_ids: Sequence[int], answer_ids: Sequence[int],
                             max_seq_length: int,
                             bos_id: Optional[int] = None) -> Dict[str, List[int]]:
    """Concatenate prompt and answer; loss only on answer tokens.

    * labels are IGNORE_INDEX (-100) for every prompt token (standard SFT);
    * the prompt is truncated from the LEFT so the answer survives; a leading
      BOS token is preserved when present;
    * the answer is truncated from the right only if it alone exceeds
      ``max_seq_length - 1`` (one prompt token is always kept because the
      first position of a causal LM is never a prediction target).
    """
    if max_seq_length < 2:
        raise ConfigError("max_seq_length must be >= 2")
    if not answer_ids:
        raise ConfigError("answer tokenizes to zero tokens")
    if not prompt_ids:
        raise ConfigError("prompt tokenizes to zero tokens")
    answer = list(answer_ids)[: max_seq_length - 1]
    budget = max_seq_length - len(answer)
    prompt = list(prompt_ids)
    if len(prompt) > budget:
        if bos_id is not None and prompt[0] == bos_id and budget >= 2:
            prompt = [bos_id] + prompt[len(prompt) - (budget - 1):]
        else:
            prompt = prompt[len(prompt) - budget:]
    input_ids = prompt + answer
    labels = [IGNORE_INDEX] * len(prompt) + answer
    return {"input_ids": input_ids, "labels": labels, "attention_mask": [1] * len(input_ids)}


def pad_batch(examples: Sequence[Dict[str, List[int]]], pad_id: int) -> Dict[str, List[List[int]]]:
    """Right-pad a batch; padded positions get attention 0 and label -100."""
    width = max(len(e["input_ids"]) for e in examples)
    out: Dict[str, List[List[int]]] = {"input_ids": [], "labels": [], "attention_mask": []}
    for e in examples:
        pad = width - len(e["input_ids"])
        out["input_ids"].append(e["input_ids"] + [pad_id] * pad)
        out["labels"].append(e["labels"] + [IGNORE_INDEX] * pad)
        out["attention_mask"].append(e["attention_mask"] + [0] * pad)
    return out


# --------------------------------------------------------------------------
# Schedules (pure)
# --------------------------------------------------------------------------

def compute_total_steps(n_train: int, batch_size: int, num_epochs: int) -> int:
    return int(math.ceil(n_train / float(batch_size))) * num_epochs


def linear_warmup_decay(step: int, warmup_steps: int, total_steps: int) -> float:
    """LR multiplier: linear warmup to 1, then linear decay to 0 at total_steps."""
    if warmup_steps > 0 and step < warmup_steps:
        return float(step + 1) / float(warmup_steps)
    remaining = max(0, total_steps - step)
    return max(0.0, remaining / float(max(1, total_steps - warmup_steps)))


def resolve_adalora_schedule(total_steps: int, tinit: int, tfinal: int,
                             delta_t: int) -> Tuple[Dict[str, int], bool]:
    """Fit the AdaLoRA budget schedule into ``total_steps``.

    PEFT requires ``tinit + tfinal < total_step`` and a final phase
    (``tfinal >= 1``) so that the final ``rank_pattern`` is fixed. If the
    requested schedule does not fit, it is scaled down proportionally so that
    ``tinit + tfinal <= 0.8 * total_steps``; the caller reports the adjustment
    (``adalora_schedule_adjusted`` in THEMIS_RESULT and in the metadata).
    """
    if total_steps < 3:
        raise ConfigError(f"AdaLoRA needs at least 3 optimizer steps (got {total_steps}); "
                          "add data or epochs, or reduce batch_size")
    t_i, t_f, d_t = int(tinit), max(1, int(tfinal)), max(1, int(delta_t))
    adjusted = t_f != int(tfinal)
    if t_i + t_f >= total_steps:
        limit = max(2, int(total_steps * 0.8))
        denom = float(max(1, t_i + t_f))
        t_i = int(math.floor(t_i * limit / denom))
        t_f = max(1, limit - t_i)
        if t_i + t_f >= total_steps:
            t_i = max(0, total_steps - 1 - t_f)
        adjusted = True
    budget_phase = max(1, total_steps - t_i - t_f)
    if d_t > budget_phase:
        d_t = budget_phase
        adjusted = True
    return {"tinit": t_i, "tfinal": t_f, "deltaT": d_t}, adjusted


# --------------------------------------------------------------------------
# AdaLoRA -> plain LoRA folding (pure math)
# --------------------------------------------------------------------------
#
# PEFT AdaLoRA (SVDLinear) adds to the frozen output
#     dy = x @ (A * E).T @ B.T * s / (ranknum + 1e-5)
# with A: (r, in), E: (r, 1) singular values (pruned entries are exactly 0),
# B: (out, r), s = lora_alpha, ranknum = the module's initial rank. Hence
#     dW_ada = c_ada * B diag(E) A,           c_ada = s / (ranknum + 1e-5).
# A plain LoRA module with rank k evaluated by llama.cpp/PEFT computes
#     dW_lora = (alpha_t / k) * B' A'.
# Keeping only rows i with E_i != 0 (K = kept set, k = |K|) and choosing
#     A' = (A[K] * E[K]) * c_ada * k / alpha_t,   B' = B[:, K]
# gives dW_lora = (alpha_t/k) * B[:,K] (A[K]*E[K]) * c_ada * k/alpha_t
#              = c_ada * B diag(E) A = dW_ada  exactly (dropped rows have E_i = 0).
# llama.cpp derives k per tensor from the A shape (scale = alpha / ne[1]),
# so modules may keep different ranks; the PEFT LoRA config records them in
# rank_pattern. Plain LoRA modules use the same formula with c = alpha / r and
# no E (then A' = A when alpha_t = alpha and k = r).


def fold_coefficient(module_scale: float, kept_rank: int, target_alpha: float) -> float:
    """Factor folded into A' so that (target_alpha / kept_rank) * B'A' == module_scale * BA."""
    if kept_rank <= 0 or target_alpha <= 0:
        raise ValueError("kept_rank and target_alpha must be > 0")
    return module_scale * kept_rank / target_alpha


def adalora_module_scale(lora_alpha: float, ranknum: float) -> float:
    return float(lora_alpha) / (float(ranknum) + ADALORA_EPS)


def kept_rank_indices(e_values: Sequence[float]) -> List[int]:
    return [i for i, v in enumerate(e_values) if v != 0.0]


def adalora_to_lora_lists(a_rows: Sequence[Sequence[float]], b_rows: Sequence[Sequence[float]],
                          e_values: Sequence[float], ranknum: float, lora_alpha: float,
                          target_alpha: float) -> Tuple[List[List[float]], List[List[float]], List[int]]:
    """List-based reference of the AdaLoRA -> LoRA folding (see comment above).

    a_rows: r x in, b_rows: out x r, e_values: r. Returns (A' k x in, B' out x k, kept).
    """
    kept = kept_rank_indices(e_values)
    if not kept:
        return [], [[] for _ in b_rows], kept
    c = fold_coefficient(adalora_module_scale(lora_alpha, ranknum), len(kept), target_alpha)
    a_new = [[v * e_values[i] * c for v in a_rows[i]] for i in kept]
    b_new = [[row[i] for i in kept] for row in b_rows]
    return a_new, b_new, kept


# --------------------------------------------------------------------------
# Metadata (pure)
# --------------------------------------------------------------------------

#: Typed GGUF representation of themis.* keys. Unknown themis.* keys are strings.
THEMIS_KV_TYPES: Dict[str, str] = {
    "themis.adapter.rank": "u32",
    "themis.adapter.alpha": "f32",
    "themis.base_model.n_embd": "u32",
    "themis.base_model.n_layer": "u32",
    "themis.training.train_samples": "u32",
    "themis.training.eval_samples": "u32",
    "themis.training.epochs": "u32",
    "themis.training.train_loss": "f32",
    "themis.training.eval_loss": "f32",
    "themis.training.eval_perplexity": "f32",
    "themis.training.baseline_eval_loss": "f32",
    "themis.training.learning_rate": "f32",
    "themis.training.adalora.init_r": "u32",
    "themis.training.adalora.target_r": "u32",
    "themis.training.adalora.tinit": "u32",
    "themis.training.adalora.tfinal": "u32",
    "themis.training.adalora.deltaT": "u32",
    "themis.training.adalora.beta1": "f32",
    "themis.training.adalora.beta2": "f32",
    "themis.training.adalora.orth_reg_weight": "f32",
}


def _fmt(value: Any) -> str:
    if isinstance(value, bool):
        return "true" if value else "false"
    if isinstance(value, float):
        return repr(value)
    if isinstance(value, (dict, list)):
        return json.dumps(value, sort_keys=True, separators=(",", ":"))
    return str(value)


def build_themis_metadata(job: Dict[str, Any], metrics: Dict[str, Any],
                          rank_pattern: Dict[str, int], base_info: Dict[str, Any],
                          created_at: str, adalora_schedule: Optional[Dict[str, int]] = None,
                          schedule_adjusted: bool = False) -> Dict[str, str]:
    """Build the canonical string->string themis.* metadata map.

    The same map is written to the safetensors ``__metadata__`` header and
    (typed via THEMIS_KV_TYPES) into the GGUF KV store. Empty values are
    omitted. Non-finite metrics are omitted rather than invented. For AdaLoRA
    the schedule actually used (after resolve_adalora_schedule) is recorded.
    """
    kept = [k for k in rank_pattern.values() if k > 0]
    meta: Dict[str, Any] = {
        "themis.adapter.id": job.get("adapter_id", ""),
        "themis.adapter.version": job.get("adapter_version", ""),
        "themis.adapter.method": job["method"],
        "themis.adapter.rank": max(kept) if kept else int(job["rank"]),
        "themis.adapter.alpha": float(job["alpha"]),
        "themis.adapter.rank_pattern": dict(sorted(rank_pattern.items())) if rank_pattern else "",
        "themis.base_model.name": job.get("base_model", ""),
        "themis.base_model.gguf": os.path.basename(job.get("base_model_gguf", "") or ""),
        "themis.base_model.sha256": base_info.get("gguf_sha256", ""),
        "themis.base_model.hf_model_type": base_info.get("hf_model_type", ""),
        "themis.base_model.n_embd": base_info.get("n_embd", ""),
        "themis.base_model.n_layer": base_info.get("n_layer", ""),
        "themis.training.dataset_id": job.get("dataset_id", ""),
        "themis.training.train_samples": metrics.get("train_samples", ""),
        "themis.training.eval_samples": metrics.get("eval_samples", ""),
        "themis.training.train_loss": metrics.get("train_loss", ""),
        "themis.training.eval_loss": metrics.get("eval_loss", ""),
        "themis.training.eval_perplexity": metrics.get("eval_perplexity", ""),
        "themis.training.baseline_eval_loss": metrics.get("baseline_eval_loss", ""),
        "themis.training.epochs": int(job["num_epochs"]),
        "themis.training.learning_rate": float(job["learning_rate"]),
        "themis.training.seed": str(job.get("seed", "")),
        "themis.training.created_at": created_at,
        "themis.training.backend": BACKEND_NAME,
    }
    if job["method"] == "adalora":
        ada = dict(job["adalora"])
        if adalora_schedule:
            ada.update(adalora_schedule)
        for key in ("init_r", "target_r", "tinit", "tfinal", "deltaT", "beta1", "beta2", "orth_reg_weight"):
            meta["themis.training.adalora." + key] = ada[key]
        if schedule_adjusted:
            meta["themis.training.adalora.schedule_adjusted"] = True
    out: Dict[str, str] = {}
    for key, value in meta.items():
        if value is None or value == "":
            continue
        if isinstance(value, float) and not math.isfinite(value):
            continue
        out[key] = _fmt(value)
    # Caller-supplied themis.* keys (e.g. themis.training.snapshot.*,
    # themis.provenance.* from the C++ side) are passed through verbatim.
    # Keys computed here are measured values and win on collision.
    passthrough = job.get("themis_metadata") or {}
    _require(isinstance(passthrough, dict), "themis_metadata must be an object")
    for key, value in passthrough.items():
        _require(isinstance(key, str) and key.startswith("themis."),
                 f"metadata key '{key}' must start with 'themis.'")
        if key not in out:
            out[key] = value if isinstance(value, str) else _fmt(value)
    return out


def metadata_to_typed_kv(meta: Dict[str, str]) -> List[Tuple[str, str, Any]]:
    """Convert the string map to typed (key, type, value) GGUF KV entries (themis.* only)."""
    entries = []
    for key in sorted(meta):
        if not key.startswith("themis."):
            continue
        typ = THEMIS_KV_TYPES.get(key, "str")
        raw = meta[key]
        try:
            if typ == "u32":
                val: Any = int(raw)
                if not 0 <= val <= 0xFFFFFFFF:
                    raise ValueError("out of u32 range")
            elif typ == "f32":
                val = float(raw)
                if not math.isfinite(val):
                    raise ValueError("non-finite")
            else:
                val = raw
        except ValueError as exc:
            raise ConfigError(f"metadata '{key}'='{raw}' is not a valid {typ}: {exc}") from None
        entries.append((key, typ, val))
    return entries


# --------------------------------------------------------------------------
# Files: sha256 and safetensors header (pure, no safetensors package needed)
# --------------------------------------------------------------------------

def sha256_file(path: str, offset: int = 0) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as fh:
        fh.seek(offset)
        for chunk in iter(lambda: fh.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def read_safetensors_header(path: str) -> Tuple[Dict[str, Any], int]:
    """Parse and bound-check a safetensors header.

    Returns (header_json, data_offset). Validates: header length within file
    size and MAX_SAFETENSORS_HEADER, JSON object, string->string
    __metadata__, dtype whitelist F16/BF16/F32, data_offsets within the data
    section, contiguous and non-overlapping, byte size consistent with shape.
    """
    size = os.path.getsize(path)
    with open(path, "rb") as fh:
        prefix = fh.read(8)
        if len(prefix) != 8:
            raise ConfigError(f"{path}: not a safetensors file (too short)")
        (n,) = struct.unpack("<Q", prefix)
        if n == 0 or n > MAX_SAFETENSORS_HEADER or 8 + n > size:
            raise ConfigError(f"{path}: invalid safetensors header length {n}")
        raw = fh.read(n)
    try:
        header = json.loads(raw.decode("utf-8"))
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise ConfigError(f"{path}: invalid safetensors header JSON: {exc}") from None
    if not isinstance(header, dict):
        raise ConfigError(f"{path}: safetensors header must be an object")
    meta = header.get("__metadata__", {})
    if not isinstance(meta, dict) or not all(isinstance(k, str) and isinstance(v, str) for k, v in meta.items()):
        raise ConfigError(f"{path}: __metadata__ must be a string->string map")
    data_len = size - 8 - n
    elem = {"F16": 2, "BF16": 2, "F32": 4}
    spans = []
    for name, info in header.items():
        if name == "__metadata__":
            continue
        if not isinstance(info, dict) or info.get("dtype") not in elem:
            raise ConfigError(f"{path}: tensor '{name}' has unsupported dtype")
        shape, offs = info.get("shape"), info.get("data_offsets")
        if (not isinstance(shape, list) or not all(isinstance(d, int) and d >= 0 for d in shape)
                or not isinstance(offs, list) or len(offs) != 2
                or not all(isinstance(o, int) for o in offs)):
            raise ConfigError(f"{path}: tensor '{name}' has malformed shape/data_offsets")
        begin, end = offs
        if not 0 <= begin <= end <= data_len:
            raise ConfigError(f"{path}: tensor '{name}' data_offsets out of bounds")
        count = 1
        for d in shape:
            count *= d
        if count * elem[info["dtype"]] != end - begin:
            raise ConfigError(f"{path}: tensor '{name}' size does not match shape")
        spans.append((begin, end, name))
    spans.sort()
    cursor = 0
    for begin, end, name in spans:
        if begin != cursor:
            raise ConfigError(f"{path}: tensor '{name}' data is overlapping or not contiguous")
        cursor = end
    if cursor != data_len:
        raise ConfigError(f"{path}: trailing bytes after last tensor")
    return header, 8 + n


def safetensors_tensor_sha256(path: str) -> str:
    """SHA-256 of the safetensors data section (``themis.adapter.sha256_tensors``)."""
    _, data_offset = read_safetensors_header(path)
    return sha256_file(path, data_offset)


PICKLE_SUFFIXES = (".bin", ".pt", ".pth", ".pkl", ".pickle", ".ckpt")


def reject_pickle_artifacts(directory: str) -> None:
    """Refuse adapter directories containing pickle-based checkpoints."""
    for name in sorted(os.listdir(directory)):
        if name.lower().endswith(PICKLE_SUFFIXES):
            raise ConfigError(f"pickle checkpoint '{name}' is not accepted; use safetensors")


def build_converter_command(python_exe: str, convert_script: str, package_dir: str,
                            base_model: str, outfile: str, outtype: str) -> List[str]:
    """argv for llama.cpp convert_lora_to_gguf.py (executed without a shell)."""
    cmd = [python_exe, convert_script, package_dir, "--outfile", outfile, "--outtype", outtype]
    if os.path.isdir(base_model):
        cmd += ["--base", base_model]
    else:
        cmd += ["--base-model-id", base_model]
    return cmd


def build_lora_adapter_config(base_model: str, target_modules: Sequence[str], target_alpha: float,
                              rank_pattern: Dict[str, int]) -> Dict[str, Any]:
    """PEFT LoRA adapter_config.json for the distributable package."""
    kept = {k: v for k, v in rank_pattern.items() if v > 0}
    if not kept:
        raise ExportError("adapter has no non-zero rank in any module")
    r = max(kept.values())
    return {
        "peft_type": "LORA",
        "task_type": "CAUSAL_LM",
        "base_model_name_or_path": base_model,
        "r": r,
        "lora_alpha": target_alpha,
        "lora_dropout": 0.0,
        "bias": "none",
        "fan_in_fan_out": False,
        "inference_mode": True,
        "target_modules": sorted(set(target_modules)),
        "rank_pattern": {k: v for k, v in sorted(kept.items()) if v != r},
        "alpha_pattern": {},
        "use_rslora": False,
    }


def _atomic_write_json(path: str, obj: Any) -> None:
    tmp = path + ".tmp"
    with open(tmp, "w", encoding="utf-8") as fh:
        json.dump(obj, fh, indent=2, sort_keys=True)
        fh.write("\n")
    os.replace(tmp, path)


# --------------------------------------------------------------------------
# GGUF metadata embedding (lazy gguf import)
# --------------------------------------------------------------------------

def _gguf_field_value(gguf: Any, field: Any) -> Any:
    if hasattr(field, "contents"):
        return field.contents()
    vt = gguf.GGUFValueType
    t = field.types[0]
    if t == vt.STRING:
        return bytes(field.parts[field.data[0]]).decode("utf-8")
    if t == vt.ARRAY:
        if field.types[-1] == vt.STRING:
            return [bytes(field.parts[i]).decode("utf-8") for i in field.data]
        return [field.parts[i].tolist()[0] for i in field.data]
    return field.parts[field.data[0]].tolist()[0]


def read_gguf_kv(path: str) -> Dict[str, Any]:
    import gguf  # lazy: llama.cpp gguf-py
    reader = gguf.GGUFReader(path, "r")
    out = {}
    for key, field in reader.fields.items():
        if key.startswith("GGUF."):
            continue
        out[key] = _gguf_field_value(gguf, field)
    return out


def embed_gguf_metadata(path: str, entries: Sequence[Tuple[str, str, Any]]) -> None:
    """Rewrite ``path`` with existing KV + tensors and the typed themis.* entries.

    Follows gguf-py's gguf_new_metadata.py: copy every field (except virtual
    GGUF.* keys and general.architecture, which GGUFWriter writes itself),
    override/add the given keys, copy tensor infos and data unchanged, write a
    temp file, verify the new keys by reading it back, then atomically replace.
    """
    import gguf  # lazy
    reader = gguf.GGUFReader(path, "r")
    arch = _gguf_field_value(gguf, reader.get_field("general.architecture"))
    new_keys = {k for k, _, _ in entries}
    tmp = path + ".tmp"
    writer = gguf.GGUFWriter(tmp, arch=arch, endianess=reader.endianess)
    for key, field in reader.fields.items():
        if key.startswith("GGUF.") or key == "general.architecture" or key in new_keys:
            continue
        vtype = field.types[0]
        sub_type = field.types[-1] if vtype == gguf.GGUFValueType.ARRAY else None
        writer.add_key_value(key, _gguf_field_value(gguf, field), vtype, sub_type=sub_type)
    for key, typ, val in entries:
        if typ == "u32":
            writer.add_uint32(key, int(val))
        elif typ == "f32":
            writer.add_float32(key, float(val))
        else:
            writer.add_string(key, str(val))
    for tensor in reader.tensors:
        writer.add_tensor_info(tensor.name, tensor.data.shape, tensor.data.dtype,
                               tensor.data.nbytes, tensor.tensor_type)
    writer.write_header_to_file()
    writer.write_kv_data_to_file()
    writer.write_ti_data_to_file()
    for tensor in reader.tensors:
        writer.write_tensor_data(tensor.data)
    writer.close()
    del reader
    written = read_gguf_kv(tmp)
    for key, typ, val in entries:
        got = written.get(key)
        if typ == "f32":
            ok = got is not None and abs(float(got) - float(val)) <= 1e-6 * max(1.0, abs(float(val)))
        else:
            ok = got == val
        if not ok:
            os.remove(tmp)
            raise ExportError(f"GGUF metadata verification failed for '{key}'")
    os.replace(tmp, path)


def run_converter(job: Dict[str, Any], package_dir: str, out_gguf: str) -> None:
    script = job.get("convert_script", "")
    if not script or not os.path.isfile(script):
        raise ExportError(f"convert_lora_to_gguf.py not found: '{script}'")
    tmp = out_gguf + ".partial"
    cmd = build_converter_command(sys.executable, script, package_dir, job["base_model"], tmp,
                                  job.get("gguf_outtype", "f16"))
    LOG.info("running converter: %s", cmd)
    proc = subprocess.run(cmd, stdout=sys.stderr, stderr=sys.stderr, check=False)
    if proc.returncode != 0 or not os.path.isfile(tmp):
        raise ExportError(f"convert_lora_to_gguf.py failed with exit code {proc.returncode}")
    os.replace(tmp, out_gguf)


def write_manifest(path: str, meta: Dict[str, str], gguf_path: str, safetensors_path: str,
                   gguf_kv: Dict[str, Any], extra: Dict[str, Any]) -> None:
    """Convenience copy of the embedded metadata (NOT the source of truth)."""
    manifest = {
        "note": "convenience copy; source of truth is the GGUF KV store / safetensors __metadata__",
        "adapter_id": meta.get("themis.adapter.id", ""),
        "version": meta.get("themis.adapter.version", ""),
        "method": meta.get("themis.adapter.method", ""),
        "gguf_file": os.path.basename(gguf_path),
        "gguf_sha256": sha256_file(gguf_path),
        "safetensors_file": os.path.relpath(safetensors_path, os.path.dirname(path)),
        "safetensors_sha256": sha256_file(safetensors_path),
        "architecture": gguf_kv.get("general.architecture", ""),
        "adapter_type": gguf_kv.get("adapter.type", ""),
        "lora_alpha": gguf_kv.get("adapter.lora.alpha"),
        "metadata": meta,
    }
    manifest.update(extra)
    _atomic_write_json(path, manifest)


def finalize_gguf(job: Dict[str, Any], package_dir: str, out_gguf: str,
                  meta: Dict[str, str]) -> Dict[str, Any]:
    """Convert package -> GGUF, embed themis.* KV, verify standard adapter keys."""
    run_converter(job, package_dir, out_gguf)
    embed_gguf_metadata(out_gguf, metadata_to_typed_kv(meta))
    kv = read_gguf_kv(out_gguf)
    if kv.get("general.type") != "adapter" or kv.get("adapter.type") != "lora":
        raise ExportError("converted GGUF is not a LoRA adapter (general.type/adapter.type)")
    return kv


# --------------------------------------------------------------------------
# Training (lazy torch / transformers / peft)
# --------------------------------------------------------------------------

def _set_seed(seed: int) -> None:
    import torch
    random.seed(seed)
    torch.manual_seed(seed)
    if torch.cuda.is_available():
        torch.cuda.manual_seed_all(seed)


def _encode_samples(samples, tokenizer, max_len):
    examples = []
    bos = getattr(tokenizer, "bos_token_id", None)
    eos = tokenizer.eos_token_id
    for s in samples:
        prompt_text, chat = build_prompt(s, tokenizer)
        # Chat templates already contain BOS/special tokens.
        prompt_ids = tokenizer(prompt_text, add_special_tokens=not chat)["input_ids"]
        answer_ids = tokenizer(s["output"], add_special_tokens=False)["input_ids"]
        if eos is not None:
            answer_ids = answer_ids + [eos]
        examples.append(build_supervised_example(prompt_ids, answer_ids, max_len, bos))
    return examples


def _to_device_batch(batch, device):
    import torch
    return {k: torch.tensor(v, dtype=torch.long, device=device) for k, v in batch.items()}


def _token_ce_sum(logits, labels):
    """Sum of next-token cross-entropy over non-ignored labels, and token count."""
    import torch
    shift_logits = logits[:, :-1, :].float()
    shift_labels = labels[:, 1:]
    loss = torch.nn.functional.cross_entropy(
        shift_logits.reshape(-1, shift_logits.size(-1)), shift_labels.reshape(-1),
        ignore_index=IGNORE_INDEX, reduction="sum")
    return loss, int((shift_labels != IGNORE_INDEX).sum().item())


def _evaluate(model, examples, batch_size, pad_id, device) -> float:
    """Token-weighted mean answer cross-entropy over the holdout."""
    import torch
    model.eval()
    total, count = 0.0, 0
    with torch.no_grad():
        for i in range(0, len(examples), batch_size):
            batch = _to_device_batch(pad_batch(examples[i:i + batch_size], pad_id), device)
            out = model(input_ids=batch["input_ids"], attention_mask=batch["attention_mask"])
            s, n = _token_ce_sum(out.logits, batch["labels"])
            total += float(s.item())
            count += n
    model.train()
    if count == 0:
        raise ConfigError("evaluation set has no answer tokens")
    return total / count


def collect_lora_tensors(peft_model, method: str, target_alpha: float, adapter_name: str = "default"):
    """Materialise plain-LoRA tensors from the live PEFT model (see folding math above).

    Uses the in-memory modules (ranknum = initial module rank, pruned E rows = 0)
    rather than the saved state dict, so the folding constants are exact.
    Returns (tensors, ranks) where ranks maps module name -> kept rank.
    """
    import torch
    tensors: Dict[str, Any] = {}
    ranks: Dict[str, int] = {}
    hf_model = peft_model.base_model.model
    for name, module in hf_model.named_modules():
        lora_a = getattr(module, "lora_A", None)
        if lora_a is None or adapter_name not in lora_a:
            continue
        if getattr(module, "fan_in_fan_out", False):
            raise ExportError(f"module '{name}' uses fan_in_fan_out (Conv1D); not supported")
        with torch.no_grad():
            if method == "adalora":
                a = module.lora_A[adapter_name].detach().float().cpu()
                b = module.lora_B[adapter_name].detach().float().cpu()
                e = module.lora_E[adapter_name].detach().float().cpu()
                ranknum = float(module.ranknum[adapter_name].item())
                scale = adalora_module_scale(float(module.scaling[adapter_name]), ranknum)
                kept = kept_rank_indices(e.view(-1).tolist())
                a_eff = a * e  # (r, in) * (r, 1)
            else:
                a_eff = module.lora_A[adapter_name].weight.detach().float().cpu()
                b = module.lora_B[adapter_name].weight.detach().float().cpu()
                scale = float(module.scaling[adapter_name])
                kept = list(range(a_eff.shape[0]))
            ranks[name] = len(kept)
            if not kept:
                continue
            idx = torch.tensor(kept, dtype=torch.long)
            c = fold_coefficient(scale, len(kept), target_alpha)
            tensors[f"base_model.model.{name}.lora_A.weight"] = (a_eff.index_select(0, idx) * c).contiguous()
            tensors[f"base_model.model.{name}.lora_B.weight"] = b.index_select(1, idx).contiguous()
    if not tensors:
        raise ExportError("no LoRA modules with non-zero rank found in the trained model")
    return tensors, ranks


def write_safetensors_package(tensors, package_dir: str, adapter_config: Dict[str, Any],
                              meta: Dict[str, str]) -> Tuple[str, Dict[str, str]]:
    """Write adapter_model.safetensors with themis.* __metadata__ (incl. sha256_tensors)."""
    from safetensors.torch import save_file  # lazy
    os.makedirs(package_dir, exist_ok=True)
    reject_pickle_artifacts(package_dir)
    path = os.path.join(package_dir, "adapter_model.safetensors")
    tmp = path + ".tmp"
    base_meta = dict(meta)
    base_meta["format"] = "pt"
    # The data section layout does not depend on __metadata__ (offsets are
    # relative to the data start), so hash after a first write, then rewrite.
    save_file(tensors, tmp, metadata=base_meta)
    digest = safetensors_tensor_sha256(tmp)
    full_meta = dict(base_meta)
    full_meta["themis.adapter.sha256_tensors"] = digest
    save_file(tensors, tmp, metadata=full_meta)
    header, _ = read_safetensors_header(tmp)
    if header.get("__metadata__", {}).get("themis.adapter.sha256_tensors") != digest or \
            safetensors_tensor_sha256(tmp) != digest:
        raise ExportError("safetensors sha256_tensors verification failed")
    os.replace(tmp, path)
    _atomic_write_json(os.path.join(package_dir, "adapter_config.json"), adapter_config)
    return path, full_meta


def _base_info(job: Dict[str, Any], hf_config: Any) -> Dict[str, Any]:
    info: Dict[str, Any] = {"hf_model_type": getattr(hf_config, "model_type", "") or ""}
    for attr, key in (("hidden_size", "n_embd"), ("num_hidden_layers", "n_layer")):
        val = getattr(hf_config, attr, None)
        if isinstance(val, int):
            info[key] = val
    sha = job.get("base_model_gguf_sha256", "")
    if not sha and job.get("base_model_gguf") and os.path.isfile(job["base_model_gguf"]):
        LOG.info("hashing base GGUF %s", job["base_model_gguf"])
        sha = sha256_file(job["base_model_gguf"])
    info["gguf_sha256"] = sha
    return info


def run_training(job: Dict[str, Any]) -> Dict[str, Any]:
    import torch
    import peft
    from transformers import AutoModelForCausalLM, AutoTokenizer

    out_dir = job["output_dir"]
    os.makedirs(out_dir, exist_ok=True)
    emit_progress(0.0, 0, 0, "prepare")
    train_samples = load_jsonl(job["train_file"])
    eval_samples = load_jsonl(job["eval_file"])
    _require(len(train_samples) >= 1, "train_file contains no samples")
    _require(len(eval_samples) >= 1, "eval_file contains no samples")
    _set_seed(int(job["seed"]))

    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    dtype = torch.bfloat16 if (device.type == "cuda" and torch.cuda.is_bf16_supported()) else torch.float32
    tokenizer = AutoTokenizer.from_pretrained(job["base_model"])
    if tokenizer.pad_token_id is None:
        tokenizer.pad_token = tokenizer.eos_token
    pad_id = tokenizer.pad_token_id
    model = AutoModelForCausalLM.from_pretrained(job["base_model"], torch_dtype=dtype)
    model.to(device)

    max_len = int(job["max_seq_length"])
    train_ex = _encode_samples(train_samples, tokenizer, max_len)
    eval_ex = _encode_samples(eval_samples, tokenizer, max_len)
    bs = int(job["batch_size"])
    total_steps = compute_total_steps(len(train_ex), bs, int(job["num_epochs"]))

    schedule = None
    schedule_adjusted = False
    if job["method"] == "adalora":
        ada = job["adalora"]
        schedule, schedule_adjusted = resolve_adalora_schedule(
            total_steps, int(ada["tinit"]), int(ada["tfinal"]), int(ada["deltaT"]))
        if schedule_adjusted:
            LOG.warning("AdaLoRA schedule adjusted to fit %d steps: %s", total_steps, schedule)
        cfg = peft.AdaLoraConfig(
            init_r=int(ada["init_r"]), target_r=int(ada["target_r"]),
            tinit=schedule["tinit"], tfinal=schedule["tfinal"], deltaT=schedule["deltaT"],
            beta1=float(ada["beta1"]), beta2=float(ada["beta2"]),
            orth_reg_weight=float(ada["orth_reg_weight"]), total_step=total_steps,
            lora_alpha=float(job["alpha"]), lora_dropout=float(job["dropout"]),
            target_modules=job["target_modules"], task_type="CAUSAL_LM")
    else:
        cfg = peft.LoraConfig(r=int(job["rank"]), lora_alpha=float(job["alpha"]),
                              lora_dropout=float(job["dropout"]), target_modules=job["target_modules"],
                              bias="none", task_type="CAUSAL_LM")
    model = peft.get_peft_model(model, cfg)
    model.train()

    params = [p for p in model.parameters() if p.requires_grad]
    optimizer = torch.optim.AdamW(params, lr=float(job["learning_rate"]),
                                  weight_decay=float(job["weight_decay"]))
    warmup = int(total_steps * float(job["warmup_ratio"]))
    scheduler = torch.optim.lr_scheduler.LambdaLR(
        optimizer, lambda s: linear_warmup_decay(s, warmup, total_steps))

    rng = random.Random(int(job["seed"]))
    order = list(range(len(train_ex)))
    log_every = max(1, total_steps // 100)
    step = 0
    window_ce, window_tok = 0.0, 0
    last_train_loss = float("nan")
    for _epoch in range(int(job["num_epochs"])):
        rng.shuffle(order)
        epoch_ce, epoch_tok = 0.0, 0
        for i in range(0, len(order), bs):
            batch = _to_device_batch(pad_batch([train_ex[j] for j in order[i:i + bs]], pad_id), device)
            out = model(**batch)  # AdaLoRA adds orth_reg_weight * R(P,Q) to out.loss
            out.loss.backward()
            ce_sum, ntok = _token_ce_sum(out.logits.detach(), batch["labels"])
            torch.nn.utils.clip_grad_norm_(params, float(job["max_grad_norm"]))
            optimizer.step()
            if job["method"] == "adalora":
                # Sensitivity/uncertainty EMA and budget masking need the
                # gradients: after optimizer.step(), before zero_grad().
                model.base_model.update_and_allocate(step)
            optimizer.zero_grad(set_to_none=True)
            scheduler.step()
            step += 1
            window_ce += float(ce_sum.item())
            window_tok += ntok
            epoch_ce += float(ce_sum.item())
            epoch_tok += ntok
            if step % log_every == 0 or step == total_steps:
                wl = window_ce / window_tok if window_tok else float("nan")
                emit_progress(0.9 * step / total_steps, step, total_steps, "train", wl)
                window_ce, window_tok = 0.0, 0
        if epoch_tok:
            last_train_loss = epoch_ce / epoch_tok

    emit_progress(0.9, step, total_steps, "evaluate", last_train_loss)
    eval_loss = _evaluate(model, eval_ex, bs, pad_id, device)
    with model.disable_adapter():
        baseline = _evaluate(model, eval_ex, bs, pad_id, device)
    emit_progress(0.93, step, total_steps, "evaluate", last_train_loss, eval_loss)
    metrics = {
        "train_loss": last_train_loss, "eval_loss": eval_loss,
        "eval_perplexity": math.exp(eval_loss) if eval_loss < 700 else float("inf"),
        "baseline_eval_loss": baseline,
        "train_samples": len(train_samples), "eval_samples": len(eval_samples),
    }
    result: Dict[str, Any] = dict(metrics)
    result.update({"total_steps": total_steps, "device": device.type,
                   "adalora_schedule": schedule, "adalora_schedule_adjusted": schedule_adjusted})
    if job["require_improvement"] and not eval_loss < baseline:
        result["exit_code"] = EXIT_NO_IMPROVEMENT
        result["error"] = (f"adapter did not improve holdout loss: eval_loss={eval_loss:.6f} "
                           f">= baseline_eval_loss={baseline:.6f}")
        return result

    emit_progress(0.95, step, total_steps, "export", last_train_loss, eval_loss)
    peft_dir = os.path.join(out_dir, "peft")
    model.save_pretrained(peft_dir, safe_serialization=True)  # raw PEFT checkpoint, never merged
    reject_pickle_artifacts(peft_dir)
    target_alpha = float(job["alpha"])
    tensors, ranks = collect_lora_tensors(model, job["method"], target_alpha)
    created_at = _dt.datetime.now(_dt.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")
    meta = build_themis_metadata(job, metrics, ranks if job["method"] == "adalora" else {},
                                 _base_info(job, model.base_model.model.config),
                                 created_at, schedule, schedule_adjusted)
    if job["method"] == "lora":
        meta["themis.adapter.rank"] = str(int(job["rank"]))
    package_dir = os.path.join(out_dir, "package")
    adapter_cfg = build_lora_adapter_config(job["base_model"], job["target_modules"], target_alpha, ranks)
    st_path, full_meta = write_safetensors_package(tensors, package_dir, adapter_cfg, meta)

    gguf_path = os.path.join(out_dir, "adapter.gguf")
    kv = finalize_gguf(job, package_dir, gguf_path, full_meta)
    manifest_path = os.path.join(out_dir, "adapter.manifest.json")
    write_manifest(manifest_path, full_meta, gguf_path, st_path, kv,
                   {"base_model": job["base_model"],
                    "base_model_gguf": os.path.basename(job["base_model_gguf"]),
                    "rank_pattern": ranks, "created_at": created_at})
    result.update({"success": True, "error": "", "gguf_adapter_path": gguf_path,
                   "safetensors_path": st_path, "package_dir": package_dir,
                   "peft_adapter_dir": peft_dir, "manifest_path": manifest_path,
                   "rank_pattern": ranks if job["method"] == "adalora" else {},
                   "metadata": full_meta, "gguf_architecture": kv.get("general.architecture", "")})
    emit_progress(1.0, step, total_steps, "done", last_train_loss, eval_loss)
    return result


def run_convert_only(cfg: Dict[str, Any]) -> Dict[str, Any]:
    """Convert a safetensors package to GGUF and embed themis.* metadata."""
    _require(isinstance(cfg, dict), "convert config must be a JSON object")
    for key in ("package_dir", "base_model", "convert_script", "output_gguf"):
        _require(isinstance(cfg.get(key), str) and cfg[key] != "", f"'{key}' is required")
    pkg = cfg["package_dir"]
    _require(os.path.isdir(pkg), f"package_dir not found: {pkg}")
    reject_pickle_artifacts(pkg)
    st_path = os.path.join(pkg, "adapter_model.safetensors")
    _require(os.path.isfile(st_path), "package lacks adapter_model.safetensors")
    _require(os.path.isfile(os.path.join(pkg, "adapter_config.json")), "package lacks adapter_config.json")
    header, _ = read_safetensors_header(st_path)
    st_meta = header.get("__metadata__", {})
    expected = st_meta.get("themis.adapter.sha256_tensors")
    if expected and safetensors_tensor_sha256(st_path) != expected:
        raise ConfigError("themis.adapter.sha256_tensors mismatch: package is corrupted or tampered")
    meta = {k: v for k, v in st_meta.items() if k.startswith("themis.")}
    extra = cfg.get("themis_metadata") or {}
    _require(isinstance(extra, dict), "themis_metadata must be an object")
    # Caller keys (signed provenance, snapshot fingerprint, ...) are passed
    # through verbatim and override header keys; the tensor hash must still
    # match the actual tensor bytes.
    for k, v in extra.items():
        _require(isinstance(k, str) and k.startswith("themis."), f"metadata key '{k}' must start with 'themis.'")
        meta[k] = v if isinstance(v, str) else _fmt(v)
    claimed = meta.get("themis.adapter.sha256_tensors")
    if claimed and claimed != (expected if expected else safetensors_tensor_sha256(st_path)):
        raise ConfigError("themis.adapter.sha256_tensors in themis_metadata does not match the package tensors")
    metadata_to_typed_kv(meta)  # validate types before running the converter
    emit_progress(0.1, 0, 0, "export")
    out = cfg["output_gguf"]
    os.makedirs(os.path.dirname(os.path.abspath(out)), exist_ok=True)
    job = {"convert_script": cfg["convert_script"], "base_model": cfg["base_model"],
           "gguf_outtype": cfg.get("gguf_outtype", "f16")}
    kv = finalize_gguf(job, pkg, out, meta)
    emit_progress(1.0, 0, 0, "done")
    return {"success": True, "error": "", "gguf_adapter_path": out, "safetensors_path": st_path,
            "metadata": meta, "gguf_architecture": kv.get("general.architecture", ""),
            "gguf_sha256": sha256_file(out)}


# --------------------------------------------------------------------------
# CLI
# --------------------------------------------------------------------------

def _empty_result(error: str) -> Dict[str, Any]:
    return {"success": False, "error": error, "gguf_adapter_path": "", "safetensors_path": "",
            "peft_adapter_dir": "", "manifest_path": "", "train_loss": None, "eval_loss": None,
            "eval_perplexity": None, "baseline_eval_loss": None, "train_samples": 0,
            "eval_samples": 0, "rank_pattern": {}}


def main(argv: Optional[Sequence[str]] = None) -> int:
    global _PROTOCOL_STREAM
    parser = argparse.ArgumentParser(description="ThemisDB LoRA/AdaLoRA training worker")
    parser.add_argument("--config", required=True, help="job JSON file")
    parser.add_argument("--convert-only", action="store_true",
                        help="convert a safetensors package to GGUF (no training)")
    args = parser.parse_args(argv)

    # Keep the protocol channel clean: libraries and child processes that
    # print to fd 1 are redirected to stderr; protocol lines use a dup of fd 1.
    sys.stdout.flush()
    _PROTOCOL_STREAM = os.fdopen(os.dup(1), "w", buffering=1, encoding="utf-8")
    os.dup2(2, 1)
    sys.stdout = sys.stderr
    logging.basicConfig(level=logging.INFO, stream=sys.stderr,
                        format="%(asctime)s %(levelname)s %(name)s: %(message)s")
    try:
        with open(args.config, "r", encoding="utf-8") as fh:
            raw = json.load(fh)
        if args.convert_only:
            result = run_convert_only(raw)
        else:
            result = run_training(normalize_job_config(raw))
        code = int(result.pop("exit_code", EXIT_OK if result.get("success") else EXIT_INTERNAL))
        full = _empty_result(result.get("error", ""))
        full.update(result)
        full["success"] = code == EXIT_OK and bool(result.get("success"))
        emit_result(full)
    except (ConfigError, json.JSONDecodeError) as exc:
        code = EXIT_CONFIG
        emit_result(_empty_result(f"invalid configuration or data: {exc}"))
    except ImportError as exc:
        code = EXIT_CONFIG
        emit_result(_empty_result(f"missing Python dependency: {exc}"))
    except ExportError as exc:
        code = EXIT_EXPORT
        emit_result(_empty_result(f"export failed: {exc}"))
    except Exception as exc:  # reported, never hidden
        LOG.exception("job failed")
        code = EXIT_INTERNAL
        emit_result(_empty_result(f"{type(exc).__name__}: {exc}"))
    return code


if __name__ == "__main__":
    sys.exit(main())
