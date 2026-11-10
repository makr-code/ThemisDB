---
Author: "ThemisDB Team"
Created: "2026-11-10"
Last Updated: "2026-11-10"
Status: "active"
AI_Used: true
AI_Models: "GitHub Copilot (Auto mode; exact backend not verifiable)"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# E2E plan: DB data → AdaLoRA training → GGUF → llama.cpp multi-LoRA serving → YAML orchestration

## Decision (autopilot assumption)

Training backend: **external Hugging Face PEFT subprocess** (`scripts/lora/themis_lora_train.py`).
Reason: `peft.AdaLoraConfig` is the reference implementation of AdaLoRA (arXiv:2303.10512:
P·Λ·Q, |w·∇w| sensitivity, EMA β1/β2 uncertainty, orthogonal regulariser, cubic budget
schedule t_i/t_f). llama.cpp b7974 has no production LoRA training loop. The adapter is
converted with llama.cpp `convert_lora_to_gguf.py` and served by llama.cpp.
Contract: `include/llm/lora_framework/lora_training_backend.h`.

## Adapter distribution requirement (user, 2026-11-10)

LoRA/AdaLoRA adapters are distributed independently of the base LLM and loaded dynamically
as individual weight files: standalone GGUF adapter (AdaLoRA exported as equivalent plain
LoRA) + `<adapter>.manifest.json` (adapter_id, version, method, rank/alpha/rank_pattern,
base-model identity, adapter sha256, metrics). Runtime hot-load via `llama_adapter_lora_init`
on the loaded model, per-request `llama_set_adapter_lora`, removal afterwards. No merged-model
delivery. Load is rejected on sha256 or base-model mismatch.

Format refinement (user, 2026-11-10): GGUF is an open, documented standard
(ggml `docs/gguf.md`). All training metadata is embedded in the GGUF KV store (standard
keys `general.*`, `adapter.type=lora`, `adapter.lora.alpha`, plus namespace `themis.training.*`
/ `themis.adapter.*` / `themis.base_model.*`). Distribution format is **safetensors**
(no code execution, unlike pickle `.bin/.pt`) with the same metadata in the safetensors
`__metadata__` header (string→string). GGUF is the serving format and is produced from the
safetensors package on publish/import. Pickle checkpoints are rejected.

## Training-data snapshot governance (user, 2026-11-10)

Every adapter must carry a tamper-evident reference to the exact MVCC snapshot of its training
data, so that a re-generation on a different base LLM is provably trained on identical data
**without persisting the data itself**.

- Pin: the dataset is read at one HLC snapshot timestamp (`MVCCStore::getAtTimestamp`); the
  timestamp is captured once at job start and used for every read of the job.
- Retention: a snapshot pin (lease) prevents MVCC GC (`mvcc_chain_pruner`) from pruning
  versions at or after a pinned timestamp while an adapter references it; releasing the pin
  ends reproducibility, and that is recorded.
- Fingerprint: SHA-256 Merkle root over canonical records `(collection, key, version_ts,
  sha256(value))` in key order, plus record count, the query/selector spec hash and the
  holdout split seed. Only hashes, not data, are stored.
- Binding: `themis.training.snapshot.{collection, hlc_ts, query_sha256, record_count,
  merkle_root, split_seed, pinned}` in the GGUF KV and in the safetensors `__metadata__`;
  `themis.provenance.{canonical_sha256, signature, key_id, alg}` signs the canonical JSON of
  all `themis.*` keys plus the tensor hash through `SigningProvider`.
- Verification: import/hot-load verifies the signature (fail-closed when policy requires it);
  re-generation re-reads at `hlc_ts`, recomputes the Merkle root and aborts on mismatch
  before training starts.

## Work packages and acceptance checks

| ID | Scope | Acceptance |
|----|-------|-----------|
| A | llama.cpp b7974 LoRA API (`llama_adapter_lora_init`, `llama_set_adapter_lora`, `llama_rm_adapter_lora`, `llama_clear_adapter_lora`) in `llama_lora_adapter.cpp`, `multi_lora_manager.cpp`, `llama_wrapper.cpp`; adapter in response-cache key; fail-closed on apply failure; `lora_used` only on success | No old `llama_lora_adapter_*` symbols; cache key contains adapter id+scale |
| B | AdaLoRA C++ (`ada_lora_adapter.*`): budget validation (2 → throw, 5 → 2/2/1), sensitivity/EMA/uncertainty scores, cubic schedule, alpha persisted | `tests/training/test_adalora_edge_regressions.cpp` expectations met |
| C | PEFT subprocess backend + Python trainer (RAFT-style prompts, holdout, baseline loss, GGUF export); remove fabricated metrics (`×1.05`, constant `0.95`) | Python unit tests pass; backend validates input, no shell, timeout/cancel |
| D | `LoRAOrchestrator::createAdapter` runs real async jobs via backend and registers GGUF; `/lora` API loads dataset by `dataset_id` via AQL; `ContinuousLearningOrchestrator` submits selected samples | No job reported Completed without trained artifact |
| E | Continuous batching executor: adapter-grouped `llama_batch` with multiple `seq_id`, per-sequence sampling and KV cleanup | Scheduler `scheduleNextBatch`/`processBatchResults` have production caller |
| F | YAML wiring: `llm.orchestrator.mode_spec_path` → `ModeSpecLoader` → `AIOrchestrator` → `McpServer::attachOrchestrator`; consumer for `config/lora/adalora_optimization_strategy.yaml` | Startup path loads and validates mode spec |
| H | Training-data snapshot provenance (see governance section): snapshot pin, Merkle fingerprint, signed `themis.training.snapshot.*`/`themis.provenance.*` metadata, verification on import and regeneration | Tamper with any metadata key or record → verification fails; regeneration on mismatching data aborts |
| G | Docs (module ROADMAP/ARCHITECTURE), review, PR | Docs synchronized; reviewer findings addressed |

Blocked items (B, E, test registration, policy defaults) and the exact remaining work are
documented in [E2E_LORA_BLOCKED_ITEMS.md](E2E_LORA_BLOCKED_ITEMS.md).

## Verification constraints

Local CMake/vcpkg not available (package install denied in this session); verification by
isolated syntax checks, Python unit tests and PR CI.
