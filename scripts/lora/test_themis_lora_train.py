#!/usr/bin/env python3
# Copyright 2025 ThemisDB
# Licensed under MIT License
"""Unit tests for the pure helpers of themis_lora_train.py.

These tests need only the Python standard library (no torch / transformers /
peft / gguf). Run:  python3 -m unittest scripts/lora/test_themis_lora_train.py
"""

import io
import json
import math
import os
import random
import struct
import sys
import tempfile
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

import themis_lora_train as t  # noqa: E402


def _base_job(**overrides):
    job = {"train_file": "train.jsonl", "eval_file": "eval.jsonl",
           "base_model": "org/base", "output_dir": "/tmp/out"}
    job.update(overrides)
    return job


def _write_safetensors(path, tensors, metadata=None, header_override=None, data_override=None):
    """Hand-write a safetensors file. tensors: list of (name, dtype, shape, bytes)."""
    header = {}
    if metadata is not None:
        header["__metadata__"] = metadata
    data = b""
    for name, dtype, shape, payload in tensors:
        header[name] = {"dtype": dtype, "shape": shape,
                        "data_offsets": [len(data), len(data) + len(payload)]}
        data += payload
    if header_override is not None:
        header = header_override
    if data_override is not None:
        data = data_override
    raw = json.dumps(header).encode("utf-8")
    with open(path, "wb") as fh:
        fh.write(struct.pack("<Q", len(raw)))
        fh.write(raw)
        fh.write(data)


def _matmul(a, b):
    return [[sum(a[i][k] * b[k][j] for k in range(len(b))) for j in range(len(b[0]))]
            for i in range(len(a))]


class NormalizeJobConfigTest(unittest.TestCase):
    def test_defaults(self):
        job = t.normalize_job_config(_base_job())
        self.assertEqual(job["method"], "adalora")
        self.assertEqual(job["target_modules"], ["q_proj", "k_proj", "v_proj", "o_proj"])
        self.assertEqual(job["adalora"]["target_r"], job["rank"])
        self.assertGreaterEqual(job["adalora"]["init_r"], job["adalora"]["target_r"])
        self.assertTrue(job["require_improvement"])
        self.assertEqual(job["themis_metadata"], {})

    def test_missing_required(self):
        for key in ("train_file", "eval_file", "base_model", "output_dir"):
            raw = _base_job()
            del raw[key]
            with self.assertRaisesRegex(t.ConfigError, key):
                t.normalize_job_config(raw)

    def test_invalid_values(self):
        bad = [
            {"method": "qlora"}, {"rank": 0}, {"rank": 2.5}, {"alpha": 0},
            {"dropout": 1.0}, {"learning_rate": 0}, {"num_epochs": 0},
            {"batch_size": 0}, {"max_seq_length": 4}, {"gguf_outtype": "q4"},
            {"target_modules": [""]}, {"target_modules": "q_proj"},
            {"adalora": {"init_r": 4, "target_r": 8}},
            {"adalora": {"deltaT": 0}}, {"adalora": {"beta1": 1.0}},
            {"adalora": {"orth_reg_weight": -1}},
            {"themis_metadata": {"other.key": "x"}}, {"themis_metadata": ["x"]},
        ]
        for override in bad:
            with self.subTest(override=override):
                with self.assertRaises(t.ConfigError):
                    t.normalize_job_config(_base_job(**override))

    def test_adalora_checks_skipped_for_lora(self):
        job = t.normalize_job_config(_base_job(method="lora", adalora={"init_r": 4, "target_r": 8}))
        self.assertEqual(job["method"], "lora")

    def test_not_a_dict(self):
        with self.assertRaises(t.ConfigError):
            t.normalize_job_config([1, 2])


class JsonlTest(unittest.TestCase):
    def test_parse_valid(self):
        lines = [
            json.dumps({"input": "q1", "output": "a1"}),
            "",
            json.dumps({"input": "q2", "output": "a2", "context": ["d1", "  ", "d2"], "system": "sys"}),
        ]
        samples = t.parse_jsonl_lines(lines)
        self.assertEqual(len(samples), 2)
        self.assertEqual(samples[0], {"input": "q1", "output": "a1"})
        self.assertEqual(samples[1]["context"], ["d1", "d2"])
        self.assertEqual(samples[1]["system"], "sys")

    def test_reject(self):
        cases = ["{not json", json.dumps([1]), json.dumps({"input": "", "output": "a"}),
                 json.dumps({"input": "q"}), json.dumps({"input": "q", "output": "a", "context": "x"}),
                 json.dumps({"input": "q", "output": "a", "context": [1]}),
                 json.dumps({"input": "q", "output": "a", "system": 3})]
        for line in cases:
            with self.subTest(line=line):
                with self.assertRaisesRegex(t.ConfigError, "f.jsonl:1"):
                    t.parse_jsonl_lines([line], "f.jsonl")


class PromptAndMaskingTest(unittest.TestCase):
    def test_plain_prompt_with_context(self):
        text, chat = t.build_prompt({"input": "Q?", "output": "A", "context": ["x", "y"], "system": "S"})
        self.assertFalse(chat)
        self.assertTrue(text.startswith("S\n\n"))
        self.assertIn("[doc 1]\nx", text)
        self.assertIn("[doc 2]\ny", text)
        self.assertTrue(text.endswith("Answer:\n"))

    def test_chat_template_fallback_without_system_role(self):
        class Tok:
            chat_template = "x"

            def apply_chat_template(self, messages, tokenize, add_generation_prompt):
                if any(m["role"] == "system" for m in messages):
                    raise ValueError("system role not supported")
                return "<u>" + messages[0]["content"] + "</u>"

        text, chat = t.build_prompt({"input": "Q", "output": "A", "system": "S"}, Tok())
        self.assertTrue(chat)
        self.assertEqual(text, "<u>S\n\nQ</u>")

    def test_label_masking(self):
        ex = t.build_supervised_example([1, 10, 11], [20, 21], 16, bos_id=1)
        self.assertEqual(ex["input_ids"], [1, 10, 11, 20, 21])
        self.assertEqual(ex["labels"], [-100, -100, -100, 20, 21])
        self.assertEqual(ex["attention_mask"], [1] * 5)

    def test_left_truncation_keeps_bos_and_answer(self):
        ex = t.build_supervised_example([1, 2, 3, 4, 5, 6], [7, 8, 9], 6, bos_id=1)
        self.assertEqual(ex["input_ids"], [1, 5, 6, 7, 8, 9])
        self.assertEqual(ex["labels"], [-100, -100, -100, 7, 8, 9])

    def test_answer_truncated_keeps_one_prompt_token(self):
        ex = t.build_supervised_example([2, 3], [7, 8, 9, 10], 3)
        self.assertEqual(ex["input_ids"], [3, 7, 8])
        self.assertEqual(ex["labels"], [-100, 7, 8])

    def test_empty_inputs_rejected(self):
        with self.assertRaises(t.ConfigError):
            t.build_supervised_example([], [1], 8)
        with self.assertRaises(t.ConfigError):
            t.build_supervised_example([1], [], 8)

    def test_pad_batch(self):
        b = t.pad_batch([{"input_ids": [1, 2], "labels": [-100, 2], "attention_mask": [1, 1]},
                         {"input_ids": [3], "labels": [3], "attention_mask": [1]}], pad_id=0)
        self.assertEqual(b["input_ids"][1], [3, 0])
        self.assertEqual(b["labels"][1], [3, -100])
        self.assertEqual(b["attention_mask"][1], [1, 0])


class ScheduleTest(unittest.TestCase):
    def test_fits_unchanged(self):
        sched, adjusted = t.resolve_adalora_schedule(1000, 200, 500, 10)
        self.assertEqual(sched, {"tinit": 200, "tfinal": 500, "deltaT": 10})
        self.assertFalse(adjusted)

    def test_scaled_to_fit(self):
        for total in (3, 10, 50, 699):
            with self.subTest(total=total):
                sched, adjusted = t.resolve_adalora_schedule(total, 200, 500, 10)
                self.assertTrue(adjusted)
                self.assertGreaterEqual(sched["tfinal"], 1)
                self.assertLess(sched["tinit"] + sched["tfinal"], total)
                self.assertGreaterEqual(sched["deltaT"], 1)
                self.assertLessEqual(sched["deltaT"], total - sched["tinit"] - sched["tfinal"])

    def test_too_few_steps(self):
        with self.assertRaises(t.ConfigError):
            t.resolve_adalora_schedule(2, 0, 1, 1)

    def test_lr_schedule(self):
        self.assertAlmostEqual(t.linear_warmup_decay(0, 4, 10), 0.25)
        self.assertAlmostEqual(t.linear_warmup_decay(4, 4, 10), 1.0)
        self.assertAlmostEqual(t.linear_warmup_decay(10, 4, 10), 0.0)
        self.assertEqual(t.compute_total_steps(9, 4, 2), 6)


class AdaLoraFoldingTest(unittest.TestCase):
    def test_equivalence_with_dense_delta(self):
        rng = random.Random(7)
        r, d_in, d_out = 6, 5, 4
        a = [[rng.uniform(-1, 1) for _ in range(d_in)] for _ in range(r)]
        b = [[rng.uniform(-1, 1) for _ in range(r)] for _ in range(d_out)]
        e = [0.7, 0.0, -1.3, 0.0, 0.2, 2.0]   # two pruned triplets
        ranknum, alpha = 6.0, 16.0
        # PEFT AdaLoRA: dW = alpha/(ranknum+1e-5) * B diag(E) A
        c_ada = alpha / (ranknum + 1e-5)
        ae = [[a[i][j] * e[i] for j in range(d_in)] for i in range(r)]
        dense = [[c_ada * v for v in row] for row in _matmul(b, ae)]
        for target_alpha in (16.0, 8.0, 3.5):
            with self.subTest(target_alpha=target_alpha):
                a2, b2, kept = t.adalora_to_lora_lists(a, b, e, ranknum, alpha, target_alpha)
                self.assertEqual(kept, [0, 2, 4, 5])
                k = len(kept)
                self.assertEqual(len(a2), k)
                self.assertEqual(len(b2[0]), k)
                lora = [[(target_alpha / k) * v for v in row] for row in _matmul(b2, a2)]
                for i in range(d_out):
                    for j in range(d_in):
                        self.assertAlmostEqual(lora[i][j], dense[i][j], places=12)

    def test_fully_pruned(self):
        a2, b2, kept = t.adalora_to_lora_lists([[1.0]], [[1.0]], [0.0], 1.0, 8.0, 8.0)
        self.assertEqual((a2, kept), ([], []))
        self.assertEqual(b2, [[]])

    def test_fold_coefficient_validation(self):
        with self.assertRaises(ValueError):
            t.fold_coefficient(1.0, 0, 8.0)


class SafetensorsHeaderTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.path = os.path.join(self.tmp.name, "adapter_model.safetensors")

    def tearDown(self):
        self.tmp.cleanup()

    def _valid(self, **kw):
        a = struct.pack("<4f", 1, 2, 3, 4)
        b = struct.pack("<2e", 1, 2)
        _write_safetensors(self.path, [("x.lora_A.weight", "F32", [2, 2], a),
                                       ("x.lora_B.weight", "F16", [2, 1], b)],
                           metadata={"themis.adapter.id": "a1"}, **kw)
        return a + b

    def test_valid(self):
        data = self._valid()
        header, offset = t.read_safetensors_header(self.path)
        self.assertEqual(header["__metadata__"]["themis.adapter.id"], "a1")
        with open(self.path, "rb") as fh:
            fh.seek(offset)
            self.assertEqual(fh.read(), data)
        import hashlib
        self.assertEqual(t.safetensors_tensor_sha256(self.path), hashlib.sha256(data).hexdigest())

    def _expect_reject(self, pattern):
        with self.assertRaisesRegex(t.ConfigError, pattern):
            t.read_safetensors_header(self.path)

    def test_header_length_beyond_file(self):
        with open(self.path, "wb") as fh:
            fh.write(struct.pack("<Q", 1000) + b"{}")
        self._expect_reject("header length")

    def test_header_length_too_large(self):
        with open(self.path, "wb") as fh:
            fh.write(struct.pack("<Q", t.MAX_SAFETENSORS_HEADER + 1))
        self._expect_reject("header length")

    def test_too_short(self):
        with open(self.path, "wb") as fh:
            fh.write(b"\x01\x02")
        self._expect_reject("too short")

    def test_bad_dtype(self):
        _write_safetensors(self.path, [("w", "I64", [1], b"\0" * 8)])
        self._expect_reject("unsupported dtype")

    def test_out_of_bounds(self):
        _write_safetensors(self.path, [], header_override={
            "w": {"dtype": "F32", "shape": [4], "data_offsets": [0, 16]}}, data_override=b"\0" * 8)
        self._expect_reject("out of bounds")

    def test_overlap(self):
        _write_safetensors(self.path, [], header_override={
            "a": {"dtype": "F32", "shape": [2], "data_offsets": [0, 8]},
            "b": {"dtype": "F32", "shape": [2], "data_offsets": [4, 12]}}, data_override=b"\0" * 12)
        self._expect_reject("overlapping or not contiguous")

    def test_gap_and_trailing(self):
        _write_safetensors(self.path, [], header_override={
            "a": {"dtype": "F32", "shape": [1], "data_offsets": [4, 8]}}, data_override=b"\0" * 8)
        self._expect_reject("not contiguous")
        _write_safetensors(self.path, [], header_override={
            "a": {"dtype": "F32", "shape": [1], "data_offsets": [0, 4]}}, data_override=b"\0" * 8)
        self._expect_reject("trailing")

    def test_shape_mismatch(self):
        _write_safetensors(self.path, [], header_override={
            "a": {"dtype": "F32", "shape": [3], "data_offsets": [0, 8]}}, data_override=b"\0" * 8)
        self._expect_reject("size does not match")

    def test_non_string_metadata(self):
        self._valid()
        _write_safetensors(self.path, [("w", "F32", [1], b"\0" * 4)], metadata={"k": 1})
        self._expect_reject("string->string")


class PickleAndConverterTest(unittest.TestCase):
    def test_reject_pickle(self):
        with tempfile.TemporaryDirectory() as d:
            open(os.path.join(d, "adapter_model.safetensors"), "wb").close()
            t.reject_pickle_artifacts(d)  # ok
            for name in ("adapter_model.bin", "x.PT", "y.pkl", "z.ckpt", "w.pth"):
                p = os.path.join(d, name)
                open(p, "wb").close()
                with self.subTest(name=name):
                    with self.assertRaisesRegex(t.ConfigError, "pickle"):
                        t.reject_pickle_artifacts(d)
                os.remove(p)

    def test_converter_command_is_argv_list(self):
        with tempfile.TemporaryDirectory() as base_dir:
            cmd = t.build_converter_command("/usr/bin/python3", "/opt/convert_lora_to_gguf.py",
                                            "/pkg dir; rm -rf /", base_dir, "/out/a.gguf", "f16")
            self.assertEqual(cmd, ["/usr/bin/python3", "/opt/convert_lora_to_gguf.py",
                                   "/pkg dir; rm -rf /", "--outfile", "/out/a.gguf",
                                   "--outtype", "f16", "--base", base_dir])
        cmd = t.build_converter_command("py", "c.py", "pkg", "org/model-id", "o.gguf", "bf16")
        self.assertEqual(cmd[-2:], ["--base-model-id", "org/model-id"])
        self.assertTrue(all(isinstance(x, str) for x in cmd))

    def test_lora_adapter_config(self):
        cfg = t.build_lora_adapter_config("base", ["v_proj", "q_proj", "q_proj"], 16.0,
                                          {"m.q_proj": 4, "m.v_proj": 2, "m.k_proj": 0})
        self.assertEqual(cfg["peft_type"], "LORA")
        self.assertEqual(cfg["r"], 4)
        self.assertEqual(cfg["target_modules"], ["q_proj", "v_proj"])
        self.assertEqual(cfg["rank_pattern"], {"m.v_proj": 2})
        with self.assertRaises(t.ExportError):
            t.build_lora_adapter_config("base", ["q_proj"], 16.0, {"m": 0})


class MetadataTest(unittest.TestCase):
    def _job(self, **kw):
        return t.normalize_job_config(_base_job(adapter_id="a1", adapter_version="3",
                                                dataset_id="ds", base_model_gguf="/m/base.gguf", **kw))

    def test_keys_adalora(self):
        job = self._job(themis_metadata={
            "themis.training.snapshot.merkle_root": "abc",
            "themis.training.snapshot.record_count": 12,
            "themis.provenance.signature": "deadbeef",
            "themis.training.eval_loss": "999",  # must not override measured value
        })
        metrics = {"train_samples": 9, "eval_samples": 1, "train_loss": 1.5,
                   "eval_loss": 1.25, "eval_perplexity": math.exp(1.25),
                   "baseline_eval_loss": float("nan")}
        meta = t.build_themis_metadata(job, metrics, {"l0.q_proj": 4, "l0.v_proj": 0},
                                       {"gguf_sha256": "f" * 64, "n_embd": 64, "n_layer": 2},
                                       "2025-01-01T00:00:00Z", {"tinit": 1, "tfinal": 2, "deltaT": 1}, True)
        for key in ("themis.adapter.id", "themis.adapter.version", "themis.adapter.method",
                    "themis.adapter.rank", "themis.adapter.rank_pattern", "themis.base_model.name",
                    "themis.base_model.gguf", "themis.base_model.sha256", "themis.training.dataset_id",
                    "themis.training.train_samples", "themis.training.eval_samples",
                    "themis.training.train_loss", "themis.training.eval_loss",
                    "themis.training.eval_perplexity", "themis.training.epochs",
                    "themis.training.learning_rate", "themis.training.seed",
                    "themis.training.created_at", "themis.training.backend",
                    "themis.training.adalora.init_r", "themis.training.adalora.target_r",
                    "themis.training.adalora.tinit", "themis.training.adalora.tfinal",
                    "themis.training.adalora.deltaT", "themis.training.adalora.beta1",
                    "themis.training.adalora.beta2", "themis.training.adalora.orth_reg_weight"):
            self.assertIn(key, meta)
        self.assertTrue(all(isinstance(v, str) for v in meta.values()))
        self.assertEqual(meta["themis.adapter.rank"], "4")
        self.assertEqual(json.loads(meta["themis.adapter.rank_pattern"]), {"l0.q_proj": 4, "l0.v_proj": 0})
        self.assertEqual(meta["themis.base_model.gguf"], "base.gguf")
        self.assertEqual(meta["themis.training.adalora.tinit"], "1")
        self.assertEqual(meta["themis.training.adalora.schedule_adjusted"], "true")
        self.assertNotIn("themis.training.baseline_eval_loss", meta)  # NaN is omitted, never invented
        self.assertEqual(meta["themis.training.eval_loss"], "1.25")
        self.assertEqual(meta["themis.training.snapshot.merkle_root"], "abc")
        self.assertEqual(meta["themis.training.snapshot.record_count"], "12")
        self.assertEqual(meta["themis.provenance.signature"], "deadbeef")

    def test_lora_has_no_adalora_keys(self):
        meta = t.build_themis_metadata(self._job(method="lora"), {}, {}, {}, "now")
        self.assertFalse(any(k.startswith("themis.training.adalora.") for k in meta))
        self.assertNotIn("themis.adapter.rank_pattern", meta)
        self.assertEqual(meta["themis.adapter.method"], "lora")

    def test_typed_kv(self):
        entries = dict((k, (typ, v)) for k, typ, v in t.metadata_to_typed_kv({
            "themis.adapter.rank": "8", "themis.training.eval_loss": "1.5",
            "themis.provenance.signature": "ab", "general.name": "ignored"}))
        self.assertEqual(entries["themis.adapter.rank"], ("u32", 8))
        self.assertEqual(entries["themis.training.eval_loss"], ("f32", 1.5))
        self.assertEqual(entries["themis.provenance.signature"], ("str", "ab"))
        self.assertNotIn("general.name", entries)
        with self.assertRaises(t.ConfigError):
            t.metadata_to_typed_kv({"themis.adapter.rank": "-1"})
        with self.assertRaises(t.ConfigError):
            t.metadata_to_typed_kv({"themis.training.eval_loss": "inf"})


class ConvertOnlyValidationTest(unittest.TestCase):
    """run_convert_only rejects bad packages before invoking any converter."""

    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.pkg = os.path.join(self.tmp.name, "pkg")
        os.makedirs(self.pkg)
        self.st = os.path.join(self.pkg, "adapter_model.safetensors")
        payload = struct.pack("<2f", 1, 2)
        import hashlib
        self.sha = hashlib.sha256(payload).hexdigest()
        _write_safetensors(self.st, [("w.lora_A.weight", "F32", [1, 2], payload)],
                           metadata={"themis.adapter.sha256_tensors": self.sha})
        with open(os.path.join(self.pkg, "adapter_config.json"), "w") as fh:
            fh.write("{}")
        self.cfg = {"package_dir": self.pkg, "base_model": "org/m",
                    "convert_script": os.path.join(self.tmp.name, "missing.py"),
                    "output_gguf": os.path.join(self.tmp.name, "out.gguf")}

    def tearDown(self):
        self.tmp.cleanup()

    def test_pickle_rejected(self):
        open(os.path.join(self.pkg, "adapter_model.bin"), "wb").close()
        with self.assertRaisesRegex(t.ConfigError, "pickle"):
            t.run_convert_only(self.cfg)

    def test_tampered_tensors(self):
        with open(self.st, "r+b") as fh:
            fh.seek(-1, os.SEEK_END)
            fh.write(b"\x7f")
        with self.assertRaisesRegex(t.ConfigError, "sha256_tensors mismatch"):
            t.run_convert_only(self.cfg)

    def test_metadata_hash_must_match(self):
        self.cfg["themis_metadata"] = {"themis.adapter.sha256_tensors": "0" * 64}
        with self.assertRaisesRegex(t.ConfigError, "does not match"):
            t.run_convert_only(self.cfg)

    def test_non_themis_key_rejected(self):
        self.cfg["themis_metadata"] = {"general.name": "x"}
        with self.assertRaisesRegex(t.ConfigError, "themis."):
            t.run_convert_only(self.cfg)

    def test_missing_converter_is_export_error(self):
        self.cfg["themis_metadata"] = {"themis.provenance.signature": "ab",
                                       "themis.adapter.sha256_tensors": self.sha}
        buf = io.StringIO()
        old = t._PROTOCOL_STREAM
        t._PROTOCOL_STREAM = buf
        try:
            with self.assertRaisesRegex(t.ExportError, "not found"):
                t.run_convert_only(self.cfg)
        finally:
            t._PROTOCOL_STREAM = old


class ProtocolTest(unittest.TestCase):
    def test_protocol_line(self):
        line = t.format_protocol_line(t.RESULT_PREFIX, {"success": True, "eval_loss": float("nan"),
                                                        "x": [float("inf")]})
        self.assertTrue(line.startswith("THEMIS_RESULT {"))
        self.assertNotIn("\n", line)
        obj = json.loads(line[len(t.RESULT_PREFIX):])
        self.assertIsNone(obj["eval_loss"])
        self.assertEqual(obj["x"], [None])

    def test_emit_progress_clamps(self):
        buf = io.StringIO()
        old = t._PROTOCOL_STREAM
        t._PROTOCOL_STREAM = buf
        try:
            t.emit_progress(1.7, 3, 10, "train", 0.5)
        finally:
            t._PROTOCOL_STREAM = old
        line = buf.getvalue().strip()
        self.assertTrue(line.startswith(t.PROGRESS_PREFIX))
        obj = json.loads(line[len(t.PROGRESS_PREFIX):])
        self.assertEqual(obj, {"progress": 1.0, "step": 3, "total_steps": 10,
                               "train_loss": 0.5, "eval_loss": -1.0, "phase": "train"})


if __name__ == "__main__":
    unittest.main()
