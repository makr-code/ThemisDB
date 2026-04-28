# ThemisDB — Stub & Simulation Inventory

> **Auto-scan command** (run from repo root to refresh the list):
> ```bash
> grep -rn "STUB/SIMULATION NOTE" src/ --include="*.cpp" -l | sort
> ```
> Keep this document in sync with ROADMAP changes.

---

## Legend

| Column | Meaning |
|---|---|
| **File** | Path relative to `src/` |
| **Activation** | Build flag / runtime condition that enables the stub |
| **Production Delta** | How behaviour differs from the real implementation |
| **Roadmap Ref** | ROADMAP / FUTURE_ENHANCEMENTS section that tracks the real impl |
| **Target** | Planned version for removal / replacement |

---

## Stub Inventory (31 entries)

| # | File | Purpose (short) | Activation | Production Delta | Roadmap Ref | Target |
|---|---|---|---|---|---|---|
| 1 | `llm/embedded_llm_stub.cpp` | No-op EmbeddedLLM when `THEMIS_ENABLE_LLM=OFF` | `THEMIS_ENABLE_LLM` not defined | All inference returns `"LLM disabled"`, `isReady()=false` | `src/llm/ROADMAP.md` §Phase 1 | permanent fallback |
| 2 | `llm/lora_framework/gpu_tensor.cpp` (CUDA path) | CPU round-trip fallback for CUDA dtype conversion | Always active for CUDA path | PCIe round-trip; no native CUDA cast kernel | `FUTURE_ENHANCEMENTS.md` §CUDA dtype kernels | v1.7.0 |
| 3 | `llm/lora_framework/gpu_tensor.cpp` (HIP path) | CPU round-trip fallback for HIP/ROCm dtype conversion | Always active for HIP path | Same as CUDA entry | `FUTURE_ENHANCEMENTS.md` §HIP dtype kernels | v1.7.0 |
| 4 | `llm/lora_framework/gpu_tensor.cpp` (legacy bridge) | `from_legacy_tensor` / `to_legacy_tensor` disabled | Block-comment — Tensor type not fully defined | No convenience bridge; callers must manually upload/download | `FUTURE_ENHANCEMENTS.md` §GPUTensor legacy bridge | v1.6.0 |
| 5 | `llm/multi_lora_manager.cpp` | Unit-test LoRA lifecycle without GPU context | Runtime — `context == nullptr` | Skips real adapter registration | permanent test-gate | — |
| 6 | `content/text_processor.cpp` | Hash-based deterministic embedding (no model) | Always active (`IEmbeddingBackend` not wired) | Not semantically meaningful for similarity search | `src/content/ROADMAP.md` §Phase 5 | v1.6.0 |
| 7 | `prompt_engineering/prompt_compressor.cpp` | Deterministic fallback summary without LLM | `setSummaryFn()` not called | Truncation only, no semantic compression | post-v1.4.0 | v1.5.0 |
| 8 | `graph/knowledge_graph_reasoner.cpp` | Simulated reasoning scores (no LoRA adapter) | `THEMIS_ENABLE_LLM` not defined | Random/rule-based scores, not model-inferred | `src/graph/ROADMAP.md` §KGR | v2.2.0 |
| 9 | `rag/continuous_learning_orchestrator.cpp` | Stub signal sources in learning loop | Always active; real signal sources not wired | No real feedback consumed | `src/rag/ROADMAP.md` §Phase 8 | v2.0.0 |
| 10 | `rag/llm_judge_integration.cpp` | Structurally-valid mock LLM-judge response | `config.use_mock_mode == true` or `allow_mock == true` | Scores are fixed, not model-inferred | `src/rag/ROADMAP.md` §LLM Judge | v1.6.0 |
| 11 | `security/intent_classifier.cpp` | Rule-based classification placeholder for LoRA | Always active in v1.x | Rules, not LoRA adapter | IMPL-A2, §Loop-1 | v1.6.0 |
| 12 | `security/hsm_provider.cpp` | Software AES-256-GCM fallback when no HSM HW | `THEMIS_ENABLE_HSM_REAL` not defined | Software crypto, no hardware attestation | `src/security/ROADMAP.md` | post-v1.5.0 |
| 13 | `security/hsm_provider_pkcs11.cpp` | PKCS#11 fallback when slot discovery fails | Runtime — `real_ready == false` | No hardware key operations | permanent safety net | — |
| 14 | `security/post_quantum_crypto.cpp` | SPHINCS+ API stub before liboqs integration | Always active; `liboqs` not in vcpkg | No real PQ signatures | `src/security/ROADMAP.md` | v1.7.0 |
| 15 | `security/timestamp_authority.cpp` | Deterministic software TSA for dev/CI | `THEMIS_USE_OPENSSL_TSA` not defined | Timestamps are not RFC 3161-compliant | `-DTHEMIS_USE_OPENSSL_TSA=ON` | v1.5.0 |
| 16 | `governance/opa_adapter.cpp` | WASM OPA evaluation placeholder | `Config::mode == WASM` and `wasm_bundle_path` set | Real WASM runtime not yet integrated | `src/governance/ROADMAP.md` | v1.6.0 |
| 17 | `sharding/cross_shard_transaction.cpp` (3PC) | 3PC Phase-2 skeleton (PreCommit) | `default_protocol == THREE_PHASE_COMMIT` | 3PC not yet safe for production | `src/sharding/ROADMAP.md` CST-6 | v1.5.0 |
| 18 | `sharding/cloud_backup.cpp` | S3-compatible backup provider placeholder | `THEMIS_ENABLE_S3` not defined | No real cloud upload | `src/sharding/ROADMAP.md` | post-v1.3.0 |
| 19 | `cdc/cdc_admin.cpp` | `purgeTenant()` always throws (unimplemented) | Always active | GDPR purge not executed | `src/cdc/ROADMAP.md` | v1.5.0 |
| 20 | `training/knowledge_graph_enricher.cpp` (cache key) | Static `":v0"` graph-version suffix | Always active; AQL metadata API not wired | No version-based cache invalidation | `src/training/FUTURE_ENHANCEMENTS.md` | v1.5.0 |
| 21 | `training/knowledge_graph_enricher.cpp` (docId) | Returns `""` for source document ID | Always active; AQL engine not injected | Enrichment silently skipped | `FUTURE_ENHANCEMENTS.md` §AQL metadata | v1.5.0 |
| 22 | `distributed_knowledge/federated_distillation_coordinator.cpp` | Gaussian DP noise simulation | Always active; no GPU path | Not privacy-certified; random noise only | `FUTURE_ENHANCEMENTS.md` §Federated DP | v2.0.0 |
| 23 | `voice/audio_preprocessing.cpp` (NoiseSuppressor::Impl) | Empty Impl when RNNoise not compiled | `THEMIS_ENABLE_RNNOISE` not defined | No noise reduction; audio passes through | `src/voice/ROADMAP.md` §Phase 2 | permanent fallback |
| 24 | `performance/advanced_cache_manager.cpp` | Passthrough compression fallback | `THEMIS_ENABLE_LZ4/SNAPPY/ZSTD` all absent | No compression overhead; no memory savings | link `lz4`/`zstd` via vcpkg | v1.4.0 |
| 25 | `query/optimizer_cost_model.cpp` | Empty placeholder table statistics | Registration + `refreshAllStatistics()` | No real table scan; estimates are zero | `src/query/ROADMAP.md` §Cost model | v2.0.0 |
| 26 | `ingestion/cdc_connector.cpp` | Test-injection fetch callback | `event_fetch_fn_` non-null | Real DB not contacted | permanent test-gate | — |
| 27 | `ingestion/database_connector.cpp` | Test-injection row-fetch callback | `row_fetch_fn_` non-null | Real RDBMS not contacted | permanent test-gate | — |
| 28 | `ingestion/kafka_connector.cpp` | Test-injection message callback | `message_fn_` non-null | Real Kafka not contacted | permanent test-gate | — |
| 29 | `ingestion/s3_connector.cpp` | Test-injection list/fetch callbacks | `list_fn_ && fetch_fn_` non-null | Real AWS not contacted | permanent test-gate | — |
| 30 | `ingestion/object_storage_connector.cpp` | Test-injection list callback | `list_fn_` non-null | Real object store not contacted | permanent test-gate | — |
| 31 | `user_storage_encrypted/key_derivation_service.cpp` | Software KDF fallback when libargon2 absent | `THEMIS_HAS_ARGON2 == 0` | PBKDF2 used instead of Argon2id | add argon2 to vcpkg | v1.6.0 |
| 32 | `server/rpc/blob_transfer_handler.cpp` | Software CRC-32C checksums (no hardware acceleration) | Always active | ~3–5× slower than SSE4.2/ARM CRC32 hw | `FUTURE_ENHANCEMENTS.md` §Hardware CRC-32C | v1.6.0 |

---

## Resolved Stubs (closed since v1.9.0)

| File | Resolution |
|---|---|
| `src/stubs.cpp` | All mock types removed (Feedback, LoRATrainingConfig, etc.) — see `src/ROADMAP.md` Phase 5 |
| `src/llm/lora_framework/distributed_dataloader.cpp` | Real per-sample concatenation implemented (v1.9.x) |
| `src/server/auth_middleware.cpp` | GAP-008: constant-time token comparison via `CRYPTO_memcmp` (v1.9.x) |
| `src/utils/checksum_utils.cpp` | GAP-005: `calculateMD5()` now delegates to SHA-256/EVP API (v1.9.x) |
| `src/sharding/paxos_consensus.cpp` | PAX-4: highest-accepted-value propagated via `PaxosPrepareFullCallback` (v1.9.x) |

---

## FUTURE_ENHANCEMENTS entries needed (Phase 5)

| Stub | Required Entry | Target |
|---|---|---|
| `security/intent_classifier.cpp` | LoRA-Adapter IMPL-A2 with Precision ≥ 92% | v1.6.0 |
| `graph/knowledge_graph_reasoner.cpp` | KGR real inference engine integration | v1.7.0 |
| `rag/continuous_learning_orchestrator.cpp` | Live learning loop with real signal sources | v2.0.0 |
| `distributed_knowledge/federated_distillation_coordinator.cpp` | Production DP coordinator with certified noise | v2.0.0 |
| All ingestion connector stubs | SDK integration per connector | v1.5.0–v1.7.0 |

---

*Last updated: 2026-04-28 — maintained by: Consolidation Phase, see `src/ROADMAP.md`*
