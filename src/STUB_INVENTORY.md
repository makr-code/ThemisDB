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

## Stub Inventory (44 entries)

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
| 33 | `ethics_ai/argument_store.cpp` | Vector embedding of ethical arguments not wired; `IVectorWriter` injection missing | Always active (vector path commented-out) | Semantic similarity queries fall back to full prefix scan | `src/ethics_ai/FUTURE_ENHANCEMENTS.md` §Vector Search Integration | v1.6.0 |
| 34 | `auth/redis_token_blacklist.cpp` | No-op when hiredis is not compiled in | `THEMIS_ENABLE_REDIS` not defined (default) | Token revocations not persisted; `isRevoked()` always returns false → revoked JWTs accepted until natural expiry | `src/auth/FUTURE_ENHANCEMENTS.md` §Distributed Token Blacklist | v1.6.0 |
| 35 | `auth/ldap_authenticator.cpp` | No-op LDAP bind when libldap not compiled in | `THEMIS_HAS_LDAP` not defined (default) | All LDAP-based logins rejected with explicit error; no silent pass-through | `src/auth/FUTURE_ENHANCEMENTS.md` §LDAP Group Membership | v1.6.0 |
| 36 | `index/advanced_vector_index.cpp` | Empty `faiss::` type stubs when FAISS not available | `THEMIS_HAS_FAISS` not defined (default) | `initializeIndex()` returns false; all vector search operations disabled | `src/index/FUTURE_ENHANCEMENTS.md` §FAISS Integration | v1.5.0 |
| 37 | `llm/inline_training_engine.cpp` | Synthetic gradient signal (`kLoRAParamCount=256` proxy) when no real backend attached | Always active when no `IBackendGradientComputer` injected | Loss curve not meaningful; optimizer/checkpoint machinery can be tested but model quality cannot be validated | `src/llm/FUTURE_ENHANCEMENTS.md` §InlineTrainingEngine production gradient | v1.8.0 |
| 38 | `analytics/olap.cpp` | Parquet export no-ops when Apache Arrow not compiled in | `ARROW_ENABLED` / `THEMIS_HAS_ARROW` not defined (default) | `exportToParquet()` and `exportCollectionToParquet()` return false; BI connector / Spark integration unavailable | `src/analytics/FUTURE_ENHANCEMENTS.md` §Parquet/Arrow Export | v1.7.0 |
| 39 | `analytics/process_mining.cpp` | Windows stub: all `ProcessMining` methods return `Status::Error` | `_WIN32 && THEMIS_PROCESS_MINING_WINDOWS_STUB` (opt-in CMake flag) | BPM conformance checking and Petri-net analysis unavailable on Windows nodes; mixed-cluster Windows nodes cannot run PM operations | `src/analytics/FUTURE_ENHANCEMENTS.md` §Process Mining Windows Port | Q4 2026 |
| 40 | `acceleration/graphics_backends.cpp` | Vulkan empty `Pimpl` placeholder when Vulkan SDK absent | `THEMIS_ENABLE_VULKAN` not defined (default) | Vulkan-accelerated vector operations disabled; falls back to CPU backend | `src/acceleration/FUTURE_ENHANCEMENTS.md` §Vulkan Vector Backend | v1.6.0 |
| 41 | `acceleration/graphics_backends.cpp` | DirectX 12 empty `Pimpl` + no-op methods on non-Windows or non-DX12 | `!_WIN32 \|\| !THEMIS_ENABLE_DIRECTX` | `isAvailable()=false`; DX12 vector acceleration unavailable; falls back to CPU/Vulkan | `src/acceleration/FUTURE_ENHANCEMENTS.md` §DirectX Vector Backend | v1.6.0 |
| 42 | `cache/redis_cache_coordinator.cpp` | hiredis absent: `connectPublish/Subscribe/subscribeLoop` no-ops | `THEMIS_ENABLE_REDIS` not defined (default) | Cross-node cache invalidation disabled; stale reads propagate for full cache TTL across multi-node deployments | `src/cache/FUTURE_ENHANCEMENTS.md` §Redis Pub/Sub Invalidation | v1.6.0 |
| 43 | `utils/pki_client.cpp` | `THEMIS_TEST_MODE` signing + verification fallback (base64-hash equality) | `THEMIS_TEST_MODE` defined at compile time (never in production presets) | Signatures are not cryptographically valid; cert_serial is `"DEMO-CERT-SERIAL"`; not a security regression because the flag is test-only | `src/utils/FUTURE_ENHANCEMENTS.md` §PKI Client Production Signing | v1.6.0 |
| 44 | `sharding/mtls_connection_pool.cpp` | `createNewConnection()` always returns `nullopt`; connection creation delegated to `MTLSClient` | Always active in current design | Pool cannot pre-warm or self-create connections; `acquireConnection()` falls back to `MTLSClient::connect()` on every cache miss | `src/sharding/FUTURE_ENHANCEMENTS.md` §mTLS Pool Connection Ownership | v2.0.0 |

---

## Resolved Stubs (closed since v1.9.0)

| File | Resolution |
|---|---|
| `src/stubs.cpp` | All mock types removed (Feedback, LoRATrainingConfig, etc.) — see `src/ROADMAP.md` Phase 5 |
| `src/llm/lora_framework/distributed_dataloader.cpp` | Real per-sample concatenation implemented (v1.9.x) |
| `src/server/auth_middleware.cpp` | GAP-008: constant-time token comparison via `CRYPTO_memcmp` (v1.9.x) |
| `src/utils/checksum_utils.cpp` | GAP-005: `calculateMD5()` now delegates to SHA-256/EVP API (v1.9.x) |
| `src/sharding/paxos_consensus.cpp` | PAX-4: highest-accepted-value propagated via `PaxosPrepareFullCallback` (v1.9.x) |
| `src/server/http_type_adapter.cpp` | URL-decoding TODO resolved: RFC 3986-compliant `urlDecode()` with malformed-sequence passthrough (v1.9.x) |
| `src/ethics_ai/argument_store.cpp` | AQL TODO resolved: `getArgumentsByPhilosophy()` now uses `ConjunctiveQuery` when `query_engine_` is available, falls back to prefix scan (v1.9.x); stale TODO comment removed |
| `src/utils/audit_logger.cpp` | Version TODO resolved: `THEMISDB_VERSION` now derives from `THEMIS_VERSION_STRING` macro (CMake-injected) with `"0.0.0-dev"` fallback (v1.9.x) |

---

## FUTURE_ENHANCEMENTS entries needed (Phase 5)

| Stub | Required Entry | Status | Target |
|---|---|---|---|
| `security/intent_classifier.cpp` | LoRA-Adapter IMPL-A2 with Precision ≥ 92% | ✅ Added to `src/security/FUTURE_ENHANCEMENTS.md` §IMPL-A2 | v1.6.0 |
| `graph/knowledge_graph_reasoner.cpp` | KGR real inference engine integration | ✅ Exists in `src/graph/FUTURE_ENHANCEMENTS.md` §KGR | v1.7.0 |
| `rag/continuous_learning_orchestrator.cpp` | Live learning loop with real signal sources | ✅ Exists in `src/rag/FUTURE_ENHANCEMENTS.md` §ContinuousLearningOrchestrator | v2.0.0 |
| `distributed_knowledge/federated_distillation_coordinator.cpp` | Production gRPC coordinator | ✅ Added to `src/distributed_knowledge/FUTURE_ENHANCEMENTS.md` §Production | v2.0.0 |
| All ingestion connector stubs | SDK integration per connector | ✅ Exists in `src/ingestion/FUTURE_ENHANCEMENTS.md` §v1.6.0–v1.7.0 | v1.5.0–v1.7.0 |

---

*Last updated: 2026-04-28 — 44 entries, 8 resolved — maintained by: Consolidation Phase, see `src/ROADMAP.md`*
