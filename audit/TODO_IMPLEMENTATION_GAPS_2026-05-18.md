# TODO List: Implementation Gaps & Stub Replacement

**Scope:** ThemisDB v1.1.0 – v2.0.0 (4 release cycles)  
**Generated:** 2026-05-18  
**Author:** Copilot Audit System

---

## Wave A: Critical Security & Data Integrity (v1.1.0 – v1.4.0)

### auth (v1.1.0)
- [ ] **A-01** JWT JWKS Cache: Add `std::shared_mutex` for thread-safe refresh
  - Files: `src/auth/jwt_validator.cpp`, `include/auth/jwt_validator.h`
  - Tests: Multi-threaded stress (16 threads × 10K validate calls)
  
- [ ] **A-02** LDAP Injection: Implement RFC 4515/4514 escaping for DN & filter
  - Files: `src/auth/ldap_authenticator.cpp`
  - Tests: Fuzz testing with LDAP injection payloads

- [ ] **A-03** MFA Timing: Replace `std::find` with constant-time `CRYPTO_memcmp`
  - Files: `src/auth/mfa_authenticator.cpp`, `src/auth/rate_limiter_backend.cpp`
  - Tests: Timing side-channel < 500ns std-dev

### chimera (v1.1.0 – v1.2.0)
- [ ] **A-04** ThemisDB Adapter: Wire real `IQueryEngine`, `IVectorIndex`, `IGraphIndex`
  - Files: `src/chimera/themisdb_adapter.cpp`
  - Tests: Integration with engine injection

- [ ] **A-05** MongoDB/Qdrant/Neo4j: Replace in-process simulation with real drivers
  - Drivers: mongocxx, qdrant-client (gRPC), bolt (Neo4j)
  - Tests: Docker compose CI with real instances

### gpu (v1.4.0)
- [ ] **A-06** Query Accelerator: Implement 5 CUDA stubs
  - Line 277: `thrust::stable_sort_by_key` (Sort by key)
  - Line 325: `cub::DeviceReduce` (Sum/Max/Min)
  - Line 383: GPU hash join (2-phase build/probe)
  - Line 445: `cublasSgemv`/`cublasHgemm` (BLAS)
  - Plus: HIP equivalents (rocThrust, rocBLAS)
  - Tests: CPU/GPU parity at 1K, 100K, 10M rows
  - Perf: Sort 10M int64 ≥5× faster, join 2×1M ≥8× faster

- [ ] **A-07** GPU Vector Index: CUDA/HIP backends for `advanced_vector_index.cpp`
  - CUDA: `cuVS`/RAFT approximate k-NN when `THEMIS_ENABLE_CUDA`
  - HIP: `rocThrust` k-NN when `THEMIS_ENABLE_HIP`
  - Tests: Recall@10 ≥0.95 vs CPU, throughput ≥10K QPS
  - Memory safety: GPU OOM graceful degradation to CPU

### geo (v1.4.0)
- [ ] **A-08** CUDA Backend: Register production `GpuBatchBackend`, replace stub
  - Files: `src/geo/gpu_backend_stub.cpp` → production path
  - Tests: `cudaErrorNoDevice` mock → CPU fallback + audit log
  - Perf: 1M points ≤50ms on A10G

### aql (v1.6.0)
- [ ] **A-09** LLM AQL Validation: Parse result AST, retry on failure
  - Files: `src/aql/llm_aql_handler.cpp`
  - Logic: Generate → Parse → Validate; retry up to 2x on error
  - Tests: Inject malformed AQL, assert retry + structured error

- [ ] **A-10** Thread Leak: Replace `std::thread::detach()` with `std::async + future`
  - Files: `src/aql/llm_aql_handler.cpp` (`LLMTimeoutManager`)
  - Valgrind: No leaked threads after 1K timeouts

---

## Wave B: Production Hardening & Completeness (v1.5.0 – v1.8.0)

### acceleration (v1.7.0)
- [ ] **B-01** Vector Similarity Search: Implement CUDA kernel via FAISS
  - Files: `src/acceleration/ai_hardware_dispatcher.cpp`, `src/acceleration/graphics_backends.cpp`
  - Implementation: `IndexFlatL2` / `GpuIndexIVFFlat`
  - Tests: Recall@10 ≥0.90 GPU vs CPU, throughput ≥10K QPS

### analytics (v1.8.0)
- [ ] **B-02** ExporterFactory: Route by `ExportFormat` (Arrow/Parquet/Feather/CSV)
  - Files: `src/analytics/analytics_export.cpp:728`
  - Tests: Round-trip write/read for all 5 formats

- [ ] **B-03** KNNRegressorModel: Implement weighted average (k-NN regression)
  - Files: `src/analytics/automl.cpp`
  - Tests: MAE < 0.05 on synthetic, latency ≤1ms per prediction (k=5, N=10K)

### api (v2.0.0)
- [ ] **B-04** gRPC API: Wire RPC methods to service handlers
  - Files: `src/api/themisdb_grpc_service.cpp`, `src/api/grpc_server.cpp`
  - Map: RPC → service handler, propagate ThemisError → gRPC status
  - Tests: End-to-end gRPC (ExecuteAQL, IngestDocument, GetDocument, DeleteCollection)

### content (v1.8.0)
- [ ] **B-05** Abuse Detection: Implement PhotoDNA + text pattern blocklist
  - Files: `src/content/content_security.cpp`
  - Interface: `IAbuseDetector` with `detect(data, metadata)`
  - Implementation: PhotoDNA perceptual hash + regex patterns
  - Tests: BLOCK/FLAG paths, audit logged

### governance (v1.8.0)
- [ ] **B-06** OPA Adapter: Wire real HTTP client (libcurl or gRPC)
  - Files: `src/governance/opa_adapter.cpp`
  - Fallback: Native evaluation + counter when OPA unavailable
  - mTLS: Support `opa_tls_cert_path` config key

### ingestion (v1.8.0)
- [ ] **B-07** LLMIngestionAdapter Phase 2: Wire real llama.cpp backend
  - Files: `src/ingestion/llm_ingestion_adapter.cpp`
  - Replace: `NullTextGenerationBackend` (production) → real `LlamaCppPlugin`
  - Tests: Unit (null inject), Integration (tiny GGUF in CI)

- [ ] **B-08** Connector Mock Paths: Wire S3/Kafka/Object Storage/Database/CDC
  - S3: AWS SDK `s3_client->GetObject()`, `ListObjectsV2`
  - Kafka: librdkafka `RdKafka::Consumer`
  - Object Storage: GCS/Azure SDKs
  - Database: Real ODBC under `THEMIS_ENABLE_ODBC`
  - CDC: Debezium/WAL-based events

### llm (v1.8.0)
- [ ] **B-09** LoRA Certificate Store: Integrate OpenSSL `X509_STORE`
  - Files: `src/llm/multi_lora_manager.cpp`
  - Load trusted CA bundle from `config/security/lora_trusted_cas.pem`
  - CRL check: OCSP stapling + CRL distribution point
  - Tests: Valid cert → load ok; revoked cert → rejected + counter

- [ ] **B-10** LLM Model Storage: RocksDB persistence (not in-memory)
  - Files: `src/llm/lora_framework/lora_storage_service_themisdb.cpp`
  - Atomic writes: `rocksdb::WriteBatch`
  - Tests: Crash recovery (10 models → kill → restart → all recoverable)

### llama_cpp (v1.8.0)
- [ ] **B-11** Real Inference: Wire `LlamaWrapper::generate()` & `embed()`
  - Files: `src/llama_cpp/llama_cpp_plugin.cpp`
  - Replace: echo stub (generate) → real generate, zero-vector (embed) → real embed
  - LoRA: Implement `exportLoRA()` / `importLoRA()` (GGUF-compatible)
  - Tests: Integration with tiny GGUF in CI, latency ≤200ms (50-token)

### query (v1.6.0 – v1.8.0)
- [ ] **B-12** QueryOptimizer: Wire real `IMetadataShard` + column statistics
  - Files: `src/query/query_optimizer.cpp`
  - Pull: Cardinality, min/max, histograms from `MetadataShard::getStats()`
  - Observability: `query_optimizer_plan_cost_estimate` gauge
  - Tests: Cost-based join order (3-table join favors lower-cost plan)

- [ ] **B-13** QueryFederation: Real shard determination (not broadcast-all)
  - Files: `src/query/cte_subquery.cpp`
  - Logic: Extract shard-key → consistent-hash ring → route to relevant shards
  - Tests: Exact shard-key → 1 shard; range query → subset

- [ ] **B-14** CTESubquery Phase 2: Spill-to-disk + streaming
  - Files: `src/query/cte_subquery.cpp`
  - Spill: RocksDB when in-memory exceeds `cte_memory_limit_mb`
  - Streaming: Yield rows incrementally
  - Tests: 10M row CTE → assert spill, identical result to in-memory

### rag (v1.8.0)
- [ ] **B-15** LLMJudge: Replace mock mode with real LLM
  - Files: `src/rag/llm_judge_integration.cpp`
  - Remove: Implicit mock fallback; add explicit `LLMJudgeMock` (test-only)
  - Production: Call real `ILLMPlugin::generate()` via `LLMIntegration`
  - Tests: Real LLM client (openai_compat_adapter), assert non-trivial scores

### security (v1.8.0)
- [ ] **B-16** ArrowUserRegistrationPlugin + HSM + Timestamp Authority
  - Arrow: Persist user store via `ArrowFileWriter`
  - HSM: Wire PKCS#11 token under `THEMIS_ENABLE_HSM`
  - TSA: Wire RFC 3161 HTTP endpoint
  - PKIClient: Real X.509 cert verification (replace base64 fallback)
  - Tests: SoftHSM2 in CI, round-trip 1K users

### server (v1.8.0)
- [ ] **B-17** HttpServer: Initialize real `IShardingManager`
  - Files: `src/api/http_server.cpp`, `src/server/grpc_web_proxy_handler.cpp`
  - Route: Shard-key requests → correct shard; others → local
  - Tests: 3-shard cluster, key-based routing

### sharding (v1.8.0)
- [ ] **B-18** GpuErasureCoderOpenCL: Implement Reed-Solomon encode/decode/repair
  - Files: `src/sharding/cloud_backup.cpp`
  - Gate: `THEMIS_ENABLE_OPENCL`
  - Tests: Encode + introduce 2 failures + repair → bitwise identical

### training (v1.8.0)
- [ ] **B-19** ProvenanceTracker: Wire real `IQueryExecutor` + `IGraphWriter`
  - Files: `src/training/provenance_tracker.cpp`, `src/training/knowledge_graph_enricher.cpp`
  - Template: Use `TrainingConfig::provenance_collection`
  - Graph: Wire real `IGraphWriter` from `IngestionToolbox`
  - Tests: Ingest 5 examples → enrich → query → assert edges exist

### utils (v1.8.0)
- [ ] **B-20** PKIClient: Remove fallback, enforce OpenSSL verification
  - Files: `src/utils/pki_client.cpp`
  - Hard error: On OpenSSL failure (no silent fallback)
  - CRL/OCSP: Add to verification chain
  - Tests: Self-signed cert → error (not fallback success)

### performance (v1.8.0)
- [ ] **B-21** Advanced Cache Manager: Replace fixed pattern with ML model
  - Files: `src/performance/advanced_cache_manager.cpp:92`
  - Replace: Fixed access pattern → sliding-window frequency model
  - Gate: `THEMIS_ENABLE_ML_CACHE`
  - Tests: Prefetch hit rate ≥60% on realistic trace

### content (v1.8.0)
- [ ] **B-22** TextProcessor Embedding: Wire real backend (not random/zero vector)
  - Files: `src/content/text_processor.cpp:207`
  - Gate: `THEMIS_ENABLE_EMBEDDING`
  - Inject: `IEmbeddingBackend` via `setEmbeddingBackend()`
  - Tests: Cosine similarity (related >0.7, unrelated <0.3)

---

## Wave C: Medium Priority & Future (v1.9.0+)

### security (v2.0.0+)
- [ ] **C-01** SPHINCS+ Production: Replace Ed25519 simulation with liboqs
  - Files: `src/security/post_quantum_crypto.cpp`
  - Gate: `THEMIS_ENABLE_LIBOQS` when liboqs ≥0.10.0 available
  - Tests: NIST PQC test vectors

### acceleration (v2.0.0)
- [ ] **C-02** OpenGL Compute Shaders: Implement 5 kernels (vector_add, matmul, etc.)
  - Gate: `THEMIS_ENABLE_OPENGL`

### analytics (v2.0.0)
- [ ] **C-03** Windows Platform Stubs: Port SIMD to `<intrin.h>`

### whisper (v2.1.0+)
- [ ] **C-04** Diarisation Backend: Integrate pyannote.audio
  - Gate: `THEMIS_ENABLE_DIARISATION`

### performance (v1.9.0)
- [ ] **C-05** PMU Non-Linux: Add kperf (macOS), QueryThreadCycleTime (Windows)

---

## Backlog Prioritization

### Critical Path (Blocks v1.4.0 GA)
1. A-01: JWT mutex
2. A-02: LDAP injection
3. A-03: Timing attacks
4. A-04 & A-05: Chimera production
5. A-06 & A-07: GPU compute
6. A-08: Geo CUDA

### Next Priority (v1.5–1.8 Production)
7. B-01 to B-22 (all)

### Opportunistic (v1.9+)
8. C-01 to C-05 + cleanup

---

## Tracking

- **Parent Issue:** [AUDIT-2026-05-18] Implementation Gaps & Stub Replacement
- **Roadmap:** `ROADMAP.md`, `FUTURE_ENHANCEMENTS.md`, `src/*/ROADMAP.md`
- **Sub-Issues:** Generated via `audit_generate_issues.py`

**Status:** 📋 TODO list created (2026-05-18)
