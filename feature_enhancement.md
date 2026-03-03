# ThemisDB - Code Maturity Analysis

**Last Updated:** 2026-03-02 04:08:18 UTC  
**Analyzed Files:** 4893  
**Average Maturity Score:** 98.6/100

## 📊 Overall Statistics

| Metric | Count |
|--------|-------|
| 🔴 Stubs Found | 1102 |
| 📝 TODOs/FIXMEs | 430 |
| 🎭 Simulations/Mocks | 1823 |

## 📈 Maturity Distribution

- **🟢 PRODUCTION-READY**: 4814 file(s)
- **🟡 RELEASE-CANDIDATE**: 40 file(s)
- **🟠 BETA**: 16 file(s)
- **🔴 ALPHA**: 8 file(s)
- **⚫ DRAFT**: 15 file(s)

## 📁 Detailed File Analysis

### `.tools/extract_gtest_failures.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `.tools/openssl_repro.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `.tools/openssl_repro2.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `.tools/print_wsl_logs.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `.tools/run_wsl_failed_tests.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `.tools/show_reruns.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `adapters/chimera/elasticsearch_adapter.hpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `adapters/chimera/example_usage.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `adapters/chimera/mongodb_adapter.hpp` (v0.0.8)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `adapters/chimera/pinecone_adapter.hpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `adapters/chimera/postgresql_adapter.hpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `adapters/chimera/qdrant_adapter.hpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `adapters/chimera/template_adapter.hpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 19: `/// a stub implementation that returns UNSUPPORTED_OPERATION`

---

### `adapters/chimera/themisdb_adapter.hpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `adapters/chimera/weaviate_adapter.hpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `adapters/covina_fastapi_ingestion/app.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `adapters/covina_fastapi_ingestion/processors/__init__.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `adapters/covina_fastapi_ingestion/processors/text.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `adapters/covina_fastapi_ingestion/themis_client.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `adapters/vcc_base/__init__.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `adapters/vcc_base/config.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `adapters/vcc_base/processors.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `adapters/vcc_base/themis_client.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `adapters/vcc_base/utils.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 16: `level: Log level (DEBUG, INFO, WARNING, ERROR)`

---

### `adapters/vcc_clara_ingestion/app.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `adapters/vcc_veritas/app.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `artifacts/debug/debug_http_aql_simple.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 6: `// Minimal simulation of the HTTP AQL issue to understand the root cause`

**🐛 DEBUG** (2 occurrences):
  - Line 9: `* DEBUG SCENARIO:`
  - Line 42: `std::cout << "=== HTTP AQL Debug Analysis ===" << std::endl;`

---

### `artifacts/debug/test_http_aql_debug.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 1: `// Debug test to isolate HTTP AQL data visibility issue`

---

### `benchmarks/analyze_benchmarks.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/analyze_raid_benchmarks.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 210: `rec.append("- **RAID0:** Maximum performance, no redundancy - use for temporary/cacheable data\n")`

---

### `benchmarks/analyze_results.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/baseline_manager.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_adaptive_query_cache.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_advanced_patterns.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 3106: `// Simulate transaction: 5 related operations`

---

### `benchmarks/bench_api_endpoints.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (87.0/100)

**Issues Found:**

**🎭 SIMULATION** (5 occurrences):
  - Line 10: `*   5.  GraphQL execute – parse + execute with mock document resolver`
  - Line 328: `// 5. GraphQL execute – parse + execute with mock document resolver`
  - Line 339: `// Build a mock document resolver that returns a fixed Value object`
  - Line 373: `state.SetLabel("GraphQL parse (cached) + execute – mock document resolver");`
  - Line 398: `// Simulate checking whether X-Correlation-ID is already set in the`

---

### `benchmarks/bench_approximate_radius_search.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_aql_functions.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_arm_memory.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_arm_simd.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_async_io_multiscan.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_auth_token_validation.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_auto_buffers.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_backend_comparison.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_batch_insert.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 43: `// Create a few indexes to simulate real workload`

---

### `benchmarks/bench_binary_quantization.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_blob_zstd.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_branch_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_changefeed_throughput.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 339: `// Benchmark: Replication Lag Simulation`

---

### `benchmarks/bench_compliance_security_governance.cpp` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (70.0/100)

**Issues Found:**

**🔴 STUB** (3 occurrences):
  - Line 524: `// Benchmark real HSM stub sign/encrypt operations (stub mode, no hardware required)`
  - Line 526: `cfg.library_path = ""; // use stub provider`
  - Line 535: `// Wrap DEK (encrypt) then unwrap (decrypt) - measures stub HSM overhead`

**🎭 SIMULATION** (9 occurrences):
  - Line 227: `// Simulate RBAC permission checking`
  - Line 237: `// Simulate permission check`
  - Line 251: `// Simulate hierarchical access control`
  - Line 272: `// Simulate role hierarchy evaluation`
  - Line 300: `// Simulate audit log entry creation`

---

### `benchmarks/bench_comprehensive.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (85.0/100)

**Issues Found:**

**🎭 SIMULATION** (5 occurrences):
  - Line 214: `// Simulate embedding generation + storage (e.g., text → vector)`
  - Line 235: `// Simulate RAG (Retrieval-Augmented Generation): search + context retrieval`
  - Line 251: `// Simulate query embedding`
  - Line 261: `// Simulate multi-query expansion`
  - Line 360: `// JOIN simulation: Graph traversal + secondary index lookup`

---

### `benchmarks/bench_compression.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_config_path_resolver.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 35: `* Sets up a temporary directory tree that mirrors the config path layout used`

---

### `benchmarks/bench_content_versioning.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_core_performance.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_cross_functional_end_to_end.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 322: `// Simulate cache behavior - repeated queries`
  - Line 482: `// Simulate concurrent operations across components`

---

### `benchmarks/bench_crud.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_cuda_vs_cpu.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_cycle_metrics.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 106: `// Simulate HNSW search (10K vectors)`
  - Line 109: `// Simulate pointer passing`
  - Line 116: `// Simulate LLM inference (10 tokens, 7B model)`

---

### `benchmarks/bench_data_transfer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 230: `// Create mock tokenizer`

---

### `benchmarks/bench_di_logging.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 321: `case 0: logger->debug(msg); break;`

---

### `benchmarks/bench_diff_engine.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_distributed_coordinator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_docker_raid_comprehensive.cpp` (v0.0.33)

**Maturity Level:** 🟠 BETA (58.0/100)

**Issues Found:**

**🎭 SIMULATION** (14 occurrences):
  - Line 87: `// Network Simulation (latencies in microseconds)`
  - Line 120: `// Simulate Docker container startup`
  - Line 145: `// Simulate data write to container`
  - Line 153: `// Simulate data read from container`
  - Line 248: `// Simulate container failure`

---

### `benchmarks/bench_edge_cases_comprehensive.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_embedded_llm.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_embedding_cache_performance.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 466: `// Simulate queries with target hit rate`

---

### `benchmarks/bench_encryption.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 208: `// Attach non-indexed encrypted payload to simulate larger writes`

---

### `benchmarks/bench_ethics_ai_plugin.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_extended_context.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (93.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 319: `// Simulate context processing overhead`
  - Line 331: `// Simulate some processing`
  - Line 439: `// Simulate complete workflow`

---

### `benchmarks/bench_flash_attention.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_fused_kernels.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 247: `// Simulate unfused optimizer: separate operations`

---

### `benchmarks/bench_fused_lora_kernels.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_geo_cpu_gpu.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_gnn_embeddings.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_gorilla_codec.cpp` (v0.0.31)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_gossip_config.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_governance_policy_latency.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_gpu_backends.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_gpu_erasure.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_gpu_module.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 7: `//   - GPUConfig validate / simulate`

---

### `benchmarks/bench_gpu_training_cycle.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 67: `// Training Step Simulation (Forward + Backward + Optimizer)`

---

### `benchmarks/bench_gpu_vector_index.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_gpu_vram_allocation.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (83.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 366: `// Simulate inference work (not actual GPU operations in this stub)`

**🎭 SIMULATION** (4 occurrences):
  - Line 338: `// Throughput Simulation Benchmarks`
  - Line 362: `// Simulate tokens processed`
  - Line 363: `size_t tokens_per_iteration = batch_size * 100;  // Simulate 100 tokens per request`
  - Line 366: `// Simulate inference work (not actual GPU operations in this stub)`

---

### `benchmarks/bench_graph_query_optimizer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_graph_traversal.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_hnsw_prefilter_minimal.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_hot_reload_manager.cpp` (v0.0.7)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 155: `// Simulate lightweight state serialisation.`

---

### `benchmarks/bench_hotspots_micro.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_hsm_provider.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (90.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 51: `// Baseline: Stub provider (no PKCS#11)`
  - Line 53: `HSMConfig cfg; cfg.library_path = ""; // force stub`

---

### `benchmarks/bench_hybrid_aql_sugar.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_hybrid_vector_geo.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 183: `// ===== Combined Vector-Geo Simulation Benchmark =====`

---

### `benchmarks/bench_image_analysis.cpp` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (63.0/100)

**Issues Found:**

**🎭 SIMULATION** (15 occurrences):
  - Line 22: `// Mock Image Analysis Plugin for Benchmarking`
  - Line 26: `* @brief Mock plugin that simulates realistic AI image processing workloads`
  - Line 39: `.description = "Mock plugin for benchmarking",`
  - Line 42: `.model_name = "mock-clip-vit-base",`
  - Line 87: `// Simulate computation proportional to image size`

---

### `benchmarks/bench_image_analysis_latency.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (80.0/100)

**Issues Found:**

**🎭 SIMULATION** (8 occurrences):
  - Line 47: `* @brief Simulate computation work for realistic latency`
  - Line 50: `volatile double dummy = 0.0;`
  - Line 52: `dummy += std::sqrt(static_cast<double>(i)) * std::sin(static_cast<double>(i));`
  - Line 79: `// Enhanced Mock Plugin with Realistic Latency Simulation`
  - Line 90: `.description = "Plugin with realistic latency simulation",`

---

### `benchmarks/bench_importer_throughput.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (3 occurrences):
  - Line 4: `// temporary location and measures import throughput in rows/second and GB/hr.`
  - Line 141: `/// Create a temporary SQL file with the given content; return its path.`
  - Line 378: `/// Create a temporary JSON file; returns its path.  Caller must unlink().`

---

### `benchmarks/bench_index_rebuild.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_ingestion_kv.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_insert_profiling.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 135: `// TODO: Implement variant with ONLY regular index`

---

### `benchmarks/bench_knowledge_gap_detector_phase2.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_latency_comprehensive.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_learned_quantization.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_legal_lora_pipeline.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (87.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 74: `* Uses a mock HTTP fetch function that returns pre-generated HTML pages so`

**📝 TODO** (7 occurrences):
  - Line 245: `// TODO: Ingest num_documents documents`
  - Line 250: `// TODO: Label documents`
  - Line 255: `// TODO: Enrich samples`
  - Line 260: `// TODO: Train adapter`
  - Line 277: `// TODO: Verify meets >1000 docs/sec target`

---

### `benchmarks/bench_llm_inference_performance.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🎭 SIMULATION** (6 occurrences):
  - Line 85: `// Mock context address generation`
  - Line 308: `// Inference Simulation Benchmarks`
  - Line 320: `// Simulate token generation`
  - Line 323: `// Simulate processing overhead (minimal for benchmark)`
  - Line 353: `// Simulate prompt encoding/processing`

---

### `benchmarks/bench_llm_infrastructure.cpp` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (77.0/100)

**Issues Found:**

**🔴 STUB** (5 occurrences):
  - Line 93: `// ===== Model Handle Benchmarks (Stub) =====`
  - Line 96: `// Stub benchmark - to be implemented with real model`
  - Line 106: `// Stub benchmark - to be implemented with real model`
  - Line 121: `// Stub: determineOptimalGPULayers() call`
  - Line 132: `// Stub: VRAM tracking overhead`

---

### `benchmarks/bench_llm_judge_integration.cpp` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (78.0/100)

**Issues Found:**

**🎭 SIMULATION** (8 occurrences):
  - Line 6: `* - Mock mode evaluation performance`
  - Line 13: `* - Mock mode evaluation: < 10ms`
  - Line 64: `// Simulate fast LLM response`
  - Line 69: `// Simulate slower LLM response (e.g., cloud API)`
  - Line 75: `// Mock Mode Benchmarks`

---

### `benchmarks/bench_llm_raid_pipeline.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 94: `// Simulate model load`
  - Line 315: `// Simulate RAID0 striping distribution`

---

### `benchmarks/bench_llm_real_models.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 343: `// Stub benchmarks when LLM is not enabled`

---

### `benchmarks/bench_llm_response_cache.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 9: `* - Performance vs stub implementation`

---

### `benchmarks/bench_locality_aware_router.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_lock_contention.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_lora_auto_binding.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 41: `// Mock context pointers for benchmarking`
  - Line 196: `// Simulate context switch detection and rebinding`
  - Line 493: `// Simulate inference request with adapter`

---

### `benchmarks/bench_lora_framework.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_lora_gpu.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 279: `// Simulate LoRA training step: forward + backward`

---

### `benchmarks/bench_lora_inline.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_lora_training.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_lossy_vs_lossless.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 118: `// ===== Quantization Simulation Benchmarks =====`

---

### `benchmarks/bench_metrics_collector.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 494: `// Real-World Simulation Benchmarks`
  - Line 507: `// Simulate query execution`

---

### `benchmarks/bench_mixed_precision_perf.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_mmdb.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 382: `* Workload 5: RAG Simulation`

---

### `benchmarks/bench_multi_gpu_lora_advanced.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_multi_gpu_scaling.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 272: `// Create dummy inputs and perform forward/backward to generate gradients`

---

### `benchmarks/bench_multi_lora_fusion.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_multithreading_comprehensive.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_mvcc.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_olap_analytics.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_olap_performance.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_pagerank.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_phase1_flash_attention.cpp` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (78.0/100)

**Issues Found:**

**🎭 SIMULATION** (8 occurrences):
  - Line 76: `// Simulate inference for now`
  - Line 78: `std::this_thread::sleep_for(std::chrono::milliseconds(24));  // Simulate ~42 tok/s`
  - Line 122: `// Simulate 22% faster inference (51.7 tok/s vs 42.3 tok/s)`
  - Line 124: `std::this_thread::sleep_for(std::chrono::milliseconds(19));  // Simulate ~51.7 tok/s`
  - Line 168: `// Simulate inference based on Flash Attention setting`

---

### `benchmarks/bench_plugin_hot_plug.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_plugin_system.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 39: `// Create mock plugin manifests`

---

### `benchmarks/bench_policy_evaluation.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 11: `// Mock policy engine`

---

### `benchmarks/bench_postgres_e2e.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 169: `// Simulate encoding overhead`

---

### `benchmarks/bench_postgres_protocol.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_postgres_transactions.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (88.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 26: `// Simulate BEGIN`
  - Line 31: `// Simulate COMMIT`
  - Line 49: `// Simulate error`
  - Line 207: `// Simulate sending rows`

---

### `benchmarks/bench_process_mining.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_product_quantization.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 408: `// 3. Simulate storage (copy)`

---

### `benchmarks/bench_qlora_gpu_kernels.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_query.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_rag_ethics.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 63: `// Add some dummy documents`

---

### `benchmarks/bench_rag_hybrid_retriever.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_raid_lora.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 27: `// Mock Storage for Benchmarks`

---

### `benchmarks/bench_random_access_prefetch.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 90: `// Create temporary database`

---

### `benchmarks/bench_residual_quantization.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_rotary_embeddings.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_saga_compensation.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (91.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 87: `// Simulate some work`
  - Line 128: `// Simulate distributed transaction with multiple writes`
  - Line 396: `// Variable work simulation`

---

### `benchmarks/bench_sanity.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_scalability_comprehensive.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (90.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 336: `// Simulate full scan by reading all keys`
  - Line 354: `// Memory Pressure Simulation`
  - Line 362: `// Configure with limited memory to simulate pressure`
  - Line 445: `// Perform some reads to simulate real workload`

---

### `benchmarks/bench_shard_resource_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_shard_routing.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_sharding_performance.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (83.0/100)

**Issues Found:**

**🎭 SIMULATION** (5 occurrences):
  - Line 139: `// Simulate scatter-gather query that touches multiple shards`
  - Line 367: `// Simulate batch serialization for migration`
  - Line 406: `// Simple parse simulation (count entities)`
  - Line 474: `// Simulate gossip message creation`
  - Line 670: `// Simulate cross-DC latency overhead`

**📝 TODO** (1 occurrences):
  - Line 5: `// Purpose: TODO-BENCH-001 - Complete sharding performance benchmarks`

---

### `benchmarks/bench_simd_distance.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_simple_insert_test.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 1: `// Simple test to debug insert issues`

---

### `benchmarks/bench_snapshot_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_spatial_index.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_storage_performance.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 403: `// Simulate shared data structure`

---

### `benchmarks/bench_stream_protocol.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_text_extraction.cpp` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (79.0/100)

**Issues Found:**

**🎭 SIMULATION** (7 occurrences):
  - Line 10: `// Mock text extraction system`
  - Line 20: `// Mock PDF extraction (simulates parsing PDF structure)`
  - Line 29: `// Mock DOCX extraction (simulates XML parsing)`
  - Line 38: `// Mock HTML extraction (strips tags)`
  - Line 56: `// Simulate PDF parsing overhead`

---

### `benchmarks/bench_timeseries_ingestion.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 406: `// Add random time offset to simulate out-of-order arrival`

---

### `benchmarks/bench_tpcc.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 339: `// Simulate New Order transaction`

---

### `benchmarks/bench_tpch.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_transaction_throughput.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_v1_3_0_features.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (88.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 228: `// Simulate recursive CTE with specific depth`
  - Line 250: `// Simulate cycle detection with varying data sizes`
  - Line 267: `// Simulate EXISTS with LIMIT 1 - stops at first match`
  - Line 385: `// Simulate complete LLM RAG pipeline with all features (API surface only)`

---

### `benchmarks/bench_v1_3_4_optimizations.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_vector_compression_lossless.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 101: `SMOOTH_SIGNAL,       // Smooth waveform (physics simulation)`

---

### `benchmarks/bench_vector_prefilter.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_vector_search.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_video_processor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_voice_assistant.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_vulkan_lora.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_wal_apply_grpc.cpp` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (70.0/100)

**Issues Found:**

**🔴 STUB** (6 occurrences):
  - Line 39: `state.SkipWithError("Shard gRPC stubs unavailable");`
  - Line 56: `stub_ = themis::sharding::proto::ShardService::NewStub(channel_);`
  - Line 65: `stub_.reset();`
  - Line 73: `if (!stub_) return false;`
  - Line 132: `std::unique_ptr<themis::sharding::proto::ShardService::Stub> stub_;`

---

### `benchmarks/bench_wal_stress.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/bench_ycsb.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/benchmark_5gb_polyglot.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/benchmark_batch_operations.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/benchmark_hybrid_search.cpp` (v0.0.29)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/benchmark_image_analysis.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (84.0/100)

**Issues Found:**

**🎭 SIMULATION** (6 occurrences):
  - Line 26: `// Mock Plugin for Benchmarking`
  - Line 37: `.description = "Mock plugin for benchmarking",`
  - Line 65: `// Simulate computation with actual work`
  - Line 78: `result.model_name = "benchmark-mock";`
  - Line 91: `result.model_name = "benchmark-mock";`

---

### `benchmarks/bottleneck_analysis.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/chimera/__init__.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/chimera/benchmark_harness.py` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/chimera/capability_matrix.py` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/chimera/chimera_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/chimera/chimera_regression_detector.py` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/chimera/citations.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/chimera/colors.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/chimera/dashboard.py` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/chimera/demo.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 5: `Generates example reports with mock data from multiple database systems.`
  - Line 20: `Generate mock benchmark data with realistic characteristics.`
  - Line 57: `print("\n📊 Generating mock data for 3 database systems...")`
  - Line 128: `print("\n📊 Generating mock data for 5 vector database systems...")`

---

### `benchmarks/chimera/demo_multi_vendor.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 89: `# Simulate realistic performance characteristics for each system`
  - Line 202: `# Simulate realistic latency characteristics`
  - Line 318: `# Simulate realistic graph query characteristics`

---

### `benchmarks/chimera/neutrality_linter.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/chimera/reporter.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/chimera/run_ci_benchmarks.py` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (93.0/100)

**Issues Found:**

**🎭 SIMULATION** (5 occurrences):
  - Line 93: `"""Simulate a relational ORDER-BY via an in-memory sort."""`
  - Line 99: `"""Simulate a vector similarity (dot-product) operation."""`
  - Line 106: `"""Simulate a document-store key lookup."""`
  - Line 111: `"""Simulate a BFS traversal over a small adjacency list."""`
  - Line 127: `description="In-memory ORDER-BY simulation (relational family)",`

---

### `benchmarks/chimera/scoring.py` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/chimera/statistics.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/chimera/test_chimera.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/chimera/test_database_adapters.py` (v0.0.33)

**Maturity Level:** ⚫ DRAFT (0.0/100)

**Issues Found:**

**🔴 STUB** (19 occurrences):
  - Line 138: `raise NotImplementedError`
  - Line 142: `raise NotImplementedError`
  - Line 146: `raise NotImplementedError`
  - Line 154: `raise NotImplementedError`
  - Line 158: `raise NotImplementedError`

**🎭 SIMULATION** (20 occurrences):
  - Line 24: `# Mock Adapter Infrastructure (for testing without real databases)`
  - Line 232: `# Mock Adapter Implementation (for testing)`
  - Line 236: `"""Mock adapter for testing"""`
  - Line 273: `# Mock query execution`
  - Line 492: `adapter.connect("mock://localhost")`

---

### `benchmarks/chimera/test_llm_rag_ethics.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (6 occurrences):
  - Line 87: `"""Mock document retriever for testing"""`
  - Line 103: `"""Retrieve relevant documents (mock implementation)"""`
  - Line 121: `"""Mock LLM generator for testing"""`
  - Line 127: `"""Generate response from context (mock implementation)"""`
  - Line 147: `metadata={"model": "mock", "temperature": 0.7}`

---

### `benchmarks/competitor_implementations.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/complete_benchmark_suite.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 79: `# Dummy test function`

---

### `benchmarks/comprehensive_crud_benchmark.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/cross_module_regression_detector.py` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/docker_benchmarks_unified.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/enterprise_comparison_suite.py` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (66.0/100)

**Issues Found:**

**🔴 STUB** (9 occurrences):
  - Line 189: `raise NotImplementedError`
  - Line 193: `raise NotImplementedError`
  - Line 197: `raise NotImplementedError`
  - Line 201: `raise NotImplementedError`
  - Line 205: `raise NotImplementedError`

**📝 TODO** (2 occurrences):
  - Line 808: `cpu_percent=0.0,  # TODO: Implement resource monitoring`
  - Line 872: `for protocol in [Protocol.HTTP]:  # TODO: Add more protocols`

---

### `benchmarks/example_scientific_foundation.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/export_csv.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/generate_benchmark_report.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/generate_comparison_report.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/hardware_constraints_analyzer.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 701: `# Example metrics (simulate)`

---

### `benchmarks/hardware_constraints_integration.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/hardware_scaling_benchmark.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/llm_bench.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 582: `// Targets the `tokens_per_second` field in Statistics (previously stub).`

**🎭 SIMULATION** (2 occurrences):
  - Line 90: `// Minimal mock plugin (no real model inference — returns immediately)`
  - Line 376: `// Uses a mock plugin (no real model; measures engine overhead only).`

---

### `benchmarks/llm_nlp_integration_test_suite.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (11 occurrences):
  - Line 247: `"""Mock: Generiere Embeddings (in echtem System würde das LLM-API sein)`
  - Line 254: `# Mock-Embeddings basierend auf Modell-Dimension`
  - Line 310: `# Step 2: Vector Search (Mock)`
  - Line 316: `# Step 3: LLM Generation (Mock)`
  - Line 320: `llm_response = f"Based on the retrieved documents, {query.lower()} [Mock Response]"`

---

### `benchmarks/metrics_exporter.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/multi_protocol_support.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 277: `self.stub = None`

---

### `benchmarks/performance_optimizations/benchmark_huge_pages.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (94.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 165: `// Benchmark: Large allocation pattern (database buffer pool simulation)`
  - Line 186: `// Simulate buffer pool access pattern`

---

### `benchmarks/performance_optimizations/benchmark_huge_pages.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 29: `# Simulate memory access benchmark`
  - Line 34: `# Simulate 15-30% improvement (reduced TLB misses)`

---

### `benchmarks/performance_optimizations/benchmark_lirs_cache.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 86: `// Simulate scan: sequential access of cold data`

---

### `benchmarks/performance_optimizations/benchmark_mimalloc.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 90: `// Simulate usage by writing data`

---

### `benchmarks/performance_optimizations/benchmark_mimalloc.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 31: `# Simulate allocation benchmark`
  - Line 37: `# Simulate 10-20% improvement`

---

### `benchmarks/performance_optimizations/benchmark_rcu_index.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/performance_optimizations/benchmark_rcu_index.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 30: `# Simulate RCU benchmark`

---

### `benchmarks/performance_optimizations/benchmark_safe_fail.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 287: `// Simulate write by checking space level (recordWrite not available in API)`

---

### `benchmarks/performance_optimizations/phase2/benchmark_phase2.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/performance_optimizations/run_all_validations.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/performance_optimizations/validate_optimization.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 125: `*enabled* variant uses a more efficient algorithm to simulate the`

---

### `benchmarks/performance_regression_detector.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/performance_tracker.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/quickstart.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/raid_sharding_test_suite.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (6 occurrences):
  - Line 317: `# Phase 1c: Simulate node failure and recovery`
  - Line 318: `logger.info("\n[1c] Simulate node failure and recovery...")`
  - Line 348: `# Simulate read operations`
  - Line 404: `# Simulate multiple node failures`
  - Line 681: `# Simulate 6.7M Wikipedia articles (in practice, would stream from parsed XML)`

---

### `benchmarks/results_analysis_reports/comparative_benchmarks_20251204/adapters/__init__.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/results_analysis_reports/comparative_benchmarks_20251204/adapters/chromadb_adapter.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 105: `# Generate a dummy embedding if not provided`

---

### `benchmarks/results_analysis_reports/comparative_benchmarks_20251204/adapters/neo4j_adapter.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/results_analysis_reports/comparative_benchmarks_20251204/adapters/postgresql_adapter.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 247: `raise NotImplementedError("Vector search requires pgvector extension")`
  - Line 262: `raise NotImplementedError("Vector search requires pgvector extension")`

**🎭 SIMULATION** (1 occurrences):
  - Line 278: `"""PostgreSQL can simulate basic graph operations."""`

---

### `benchmarks/results_analysis_reports/comparative_benchmarks_20251204/adapters/themisdb_adapter.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/results_analysis_reports/comparative_benchmarks_20251204/benchmarks/__init__.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/results_analysis_reports/comparative_benchmarks_20251204/benchmarks/base_benchmark.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (80.0/100)

**Issues Found:**

**🔴 STUB** (7 occurrences):
  - Line 209: `raise NotImplementedError("Vector search not supported")`
  - Line 214: `raise NotImplementedError("Vector search not supported")`
  - Line 224: `raise NotImplementedError("Graph operations not supported")`
  - Line 229: `raise NotImplementedError("Graph operations not supported")`
  - Line 233: `raise NotImplementedError("Graph operations not supported")`

---

### `benchmarks/results_analysis_reports/comparative_benchmarks_20251204/datasets/__init__.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/results_analysis_reports/comparative_benchmarks_20251204/datasets/huggingface_loader.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/results_analysis_reports/comparative_benchmarks_20251204/scripts/extended_benchmark_simplified.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 196: `# Benchmark: Query document, then simulate vector lookup`
  - Line 200: `# Simulate cross-DB vector lookup (in real scenario, would hit Qdrant)`
  - Line 224: `"vector": [0.1 * j for j in range(384)]  # Dummy 384-dim vector`

---

### `benchmarks/results_analysis_reports/comparative_benchmarks_20251204/scripts/extended_models_benchmark.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/results_analysis_reports/comparative_benchmarks_20251204/scripts/extended_polyglot_benchmark.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/results_analysis_reports/comparative_benchmarks_20251204/scripts/fair_benchmark.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/results_analysis_reports/comparative_benchmarks_20251204/scripts/generate_benchmark_protocol.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/results_analysis_reports/comparative_benchmarks_20251204/scripts/generate_html_report.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/results_analysis_reports/comparative_benchmarks_20251204/scripts/generate_report.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/results_analysis_reports/comparative_benchmarks_20251204/scripts/load_wikipedia_dataset.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 121: `# Skip redirects and stubs`

**📝 TODO** (2 occurrences):
  - Line 124: `current_article['views_last_month'] = np.random.randint(100, 1000000)  # TODO: get from pageviews AP`
  - Line 206: `# TODO: Implement ThemisDB native client`

---

### `benchmarks/results_analysis_reports/comparative_benchmarks_20251204/scripts/polyglot_benchmark.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/results_analysis_reports/comparative_benchmarks_20251204/scripts/run_benchmarks.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/results_analysis_reports/comparative_benchmarks_20251204/scripts/setup_datasets.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/results_analysis_reports/comparative_benchmarks_20251204/scripts/simple_benchmark.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/results_analysis_reports/comparative_benchmarks_20251204/scripts/simplified_polyglot_benchmark.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 171: `# Simulate embedding (384-dim vector)`

---

### `benchmarks/results_analysis_reports/comparative_benchmarks_20251204/scripts/verify_benchmark_protocol.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/results_analysis_reports/comparative_benchmarks_20251204/setup_data.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/run_benchmark_orchestrator.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/run_complete_benchmarks.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 131: `# Simulate scientific benchmark results (in production would run real benchmarks)`
  - Line 186: `# Simulate standard benchmark results`

---

### `benchmarks/run_docker_benchmarks.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/run_multi_shard_raid_benchmark.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 130: `"query_vector": [0.1] * 768,  # Dummy embedding`

---

### `benchmarks/scientific_benchmark_runner.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 66: `network_simulation: bool = False   # Simulate network delays`
  - Line 610: `# Simulate database insert`

---

### `benchmarks/scientific_crud_benchmark.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/scientific_enterprise_integration.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/scripts/load_test_data.py` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (71.0/100)

**Issues Found:**

**🎭 SIMULATION** (12 occurrences):
  - Line 17: `fake = Faker()`
  - Line 44: `"title": fake.catch_phrase(),`
  - Line 45: `"author": fake.name(),`
  - Line 46: `"organization": fake.company(),`
  - Line 48: `"created_at": fake.date_time_this_year().isoformat(),`

---

### `benchmarks/specialized_benchmarks.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/specialized_comparative_benchmarks.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/standard_benchmarks.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🎭 SIMULATION** (6 occurrences):
  - Line 183: `# Simulate operations`
  - Line 197: `# Default simulation`
  - Line 246: `"""Simulate YCSB operation"""`
  - Line 422: `"""Simulate TPC-C transaction"""`
  - Line 572: `"""Simulate TPC-H query with complexity"""`

---

### `benchmarks/standard_benchmarks_integration.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/summary_analysis.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/tests/test_chimera_regression_detector.py` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/tests/test_cross_module_regression_detector.py` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 59: `source_path="/fake/path.json",`

---

### `benchmarks/tests/test_performance_regression_detector.py` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/tests/test_run_ci_benchmarks.py` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/themis_complete_with_constraints.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/unified_benchmark_suite.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/validate_code_optimizations.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/validate_infrastructure.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/validate_optimizations.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `benchmarks/wikipedia_stress_runner.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 248: `# Simulate query performance metrics`

---

### `benchmarks/wikipedia_stress_test.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `clients/csharp/ThemisDB.Client/CircuitBreaker.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `clients/csharp/ThemisDB.Client/ClientConfig.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `clients/csharp/ThemisDB.Client/IsolationLevel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `clients/csharp/ThemisDB.Client/Llm/LlmInteraction.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `clients/csharp/ThemisDB.Client/Llm/LlmInteractionResult.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `clients/csharp/ThemisDB.Client/Llm/LlmMessage.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `clients/csharp/ThemisDB.Client/Llm/ReasoningStep.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `clients/csharp/ThemisDB.Client/ThemisClient.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `clients/csharp/ThemisDB.Client/Transaction.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `clients/csharp/ThemisDB.Client/TransactionOptions.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `clients/csharp/ThemisDB.Client/obj/Debug/net6.0/.NETCoreApp,Version=v6.0.AssemblyAttributes.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `clients/csharp/ThemisDB.Client/obj/Debug/net6.0/ThemisDB.Client.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 14: `[assembly: System.Reflection.AssemblyConfigurationAttribute("Debug")]`

---

### `clients/csharp/ThemisDB.Client/obj/Debug/net6.0/ThemisDB.Client.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `clients/csharp/ThemisDB.Client/obj/Debug/net8.0/.NETCoreApp,Version=v8.0.AssemblyAttributes.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `clients/csharp/ThemisDB.Client/obj/Debug/net8.0/ThemisDB.Client.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 14: `[assembly: System.Reflection.AssemblyConfigurationAttribute("Debug")]`

---

### `clients/csharp/ThemisDB.Client/obj/Debug/net8.0/ThemisDB.Client.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `clients/csharp/ThemisDB.Client.Tests/ThemisClientTests.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `clients/csharp/ThemisDB.Client.Tests/obj/Debug/net6.0/.NETCoreApp,Version=v6.0.AssemblyAttributes.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `clients/csharp/ThemisDB.Client.Tests/obj/Debug/net6.0/ThemisDB.Client.Tests.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 14: `[assembly: System.Reflection.AssemblyConfigurationAttribute("Debug")]`

---

### `clients/csharp/ThemisDB.Client.Tests/obj/Debug/net6.0/ThemisDB.Client.Tests.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `clients/csharp/ThemisDB.Client.Tests/obj/Debug/net8.0/.NETCoreApp,Version=v8.0.AssemblyAttributes.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `clients/csharp/ThemisDB.Client.Tests/obj/Debug/net8.0/ThemisDB.Client.Tests.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 14: `[assembly: System.Reflection.AssemblyConfigurationAttribute("Debug")]`

---

### `clients/csharp/ThemisDB.Client.Tests/obj/Debug/net8.0/ThemisDB.Client.Tests.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `clients/php/examples/basic_crud.php` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `clients/php/examples/graph_operations.php` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `clients/php/examples/transactions.php` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `clients/php/examples/vector_search.php` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 19: `* Simulate generating an embedding (in real use, you'd call an LLM API)`
  - Line 22: `// Simple mock: hash the text and generate deterministic values`

---

### `clients/php/src/Llm/LlmInteraction.php` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `clients/php/src/Llm/LlmInteractionResult.php` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `clients/php/src/Llm/LlmMessage.php` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `clients/php/src/Llm/ReasoningStep.php` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `clients/php/src/ThemisClient.php` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `clients/php/src/Transaction.php` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `clients/php/tests/ThemisClientTest.php` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `clients/python/tests/conftest.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `clients/python/tests/test_benchmarks.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 9: `from unittest.mock import Mock, patch`
  - Line 163: `# Simulate some operations`

---

### `clients/python/tests/test_rest_api.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (7 occurrences):
  - Line 13: `from unittest.mock import Mock, patch, AsyncMock`
  - Line 32: `# Mock the HTTP request`
  - Line 229: `"Not Found", request=Mock(), response=Mock(status_code=404)`
  - Line 242: `request=Mock(),`
  - Line 243: `response=Mock(status_code=500)`

---

### `clients/python/tests/test_topology.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `clients/python/tests/test_transaction.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 115: `# Simulate successful execution`
  - Line 130: `# Simulate exception during execution`
  - Line 175: `# Simulate error`

---

### `clients/python/themis/__init__.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `clients/python/themis/async_client.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (6 occurrences):
  - Line 252: `logging.debug("Health check failed for endpoint %s", endpoint)`
  - Line 480: `logging.debug("batch_get failed for uuid %s: %s", uuid, e)`
  - Line 503: `logging.debug("batch_put failed for uuid %s", uuid)`
  - Line 840: `logging.debug("Topology refresh failed for endpoint %s; trying next", endpoint)`
  - Line 891: `logging.debug("Failed to decode entity blob: %s", e)`

---

### `clients/python/themis/themis_native.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `clients/python/themisdb/buffered_client.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `clients/wordpress-integration-example/themisdb-llm-integration.php` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `compendium/debug_anchors.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (2 occurrences):
  - Line 2: `"""Debug script to validate anchor consistency in generated HTML."""`
  - Line 24: `print("DEBUG: Anchor Consistency Validator")`

---

### `compendium/debug_mermaid.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 24: `print("\n[DEBUG] Erste 1000 Zeichen:")`

---

### `compendium/fix_markdown_figures.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `compendium/generate_test_pdf.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `compendium/step1_generate_svgs.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `compendium/step2_generate_html.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `compendium/step3_generate_pdf.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `compendium/step4_add_bookmarks.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `compendium/step5_cleanup.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 47: `# 4. Temporary files`

---

### `compendium/test_regex.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `compendium/validate_debug_yaml.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (90.0/100)

**Issues Found:**

**🐛 DEBUG** (13 occurrences):
  - Line 3: `Validate Debug-YAML files for consistency and correctness.`
  - Line 27: `print("Debug YAML Validation - ThemisDB Kompendium")`
  - Line 30: `# Load all debug files`
  - Line 31: `print("\n[INFO] Loading debug YAML files...")`
  - Line 33: `complete = load_yaml_safe(OUTPUT_DIR / "debug-anchors-complete.yml")`

---

### `examples/01_hello_world/main.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/01_hello_world/themis_client.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/02_todo_app/main.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/02_todo_app/models.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/02_todo_app/themis_client.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/03_contact_manager/main.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/03_contact_manager/models.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/03_contact_manager/themis_client.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/04_inventory_system/main.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/04_inventory_system/models.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/04_inventory_system/themis_client.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/05_time_series_monitor/main.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/05_time_series_monitor/models.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/05_time_series_monitor/themis_client.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/06_graph_social_network/main.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/06_graph_social_network/models.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/06_graph_social_network/themis_client.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/07_vector_search_documents/main.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/07_vector_search_documents/models.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/07_vector_search_documents/themis_client.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/08_dms_erp_system/main.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 446: `# ==================== CRUD-Operationen (Stubs) ====================`

---

### `examples/08_dms_erp_system/models.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/08_dms_erp_system/themis_client.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/09_iot_sensor_network/main.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/09_iot_sensor_network/models.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/09_iot_sensor_network/themis_client.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/10_drone_image_analysis/main.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 256: `map_frame = ttk.LabelFrame(tab, text="Karte (Simulation)")`

---

### `examples/10_drone_image_analysis/models.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/10_drone_image_analysis/themis_client.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/11_blog_wiki/main.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/11_blog_wiki/models.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/11_blog_wiki/themis_client.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/12_expense_tracker/main.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/12_expense_tracker/themis_client.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/13_recipe_manager/main.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/13_recipe_manager/themis_client.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/14_ecommerce_catalog/main.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/14_ecommerce_catalog/themis_client.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/15_event_management/main.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/15_event_management/themis_client.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/16_kanban_board/main.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/16_kanban_board/themis_client.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/17_crm/main.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/17_crm/themis_client.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/18_realtime_chat/main.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/18_realtime_chat/themis_client.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/19_recommendation_engine/main.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/19_recommendation_engine/themis_client.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/20_smart_home/main.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/20_smart_home/themis_client.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/21_coding_platform/code_indexer.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🎭 SIMULATION** (6 occurrences):
  - Line 13: `# For now, we'll create mock implementations`
  - Line 39: `print("Note: Using mock implementation. Install sentence-transformers for actual embeddings.")`
  - Line 57: `# Mock implementation: deterministic hash-based vector`
  - Line 75: `# Mock implementation`
  - Line 98: `"""Generate mock embedding for development."""`

---

### `examples/21_coding_platform/main.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 27: `# Initialize ThemisDB client (using mock for demo)`
  - Line 31: `self.status_text = "Using Mock Client (for demo)"`

---

### `examples/21_coding_platform/models.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/21_coding_platform/themis_client.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (5 occurrences):
  - Line 326: `# Mock implementation for development/testing without actual ThemisDB server`
  - Line 328: `"""Mock client for testing without ThemisDB server."""`
  - Line 376: `# Simple keyword search for mock`
  - Line 389: `"score": 0.85  # Mock similarity score`
  - Line 395: `# Mock implementation`

---

### `examples/21_coding_platform/web_scraper.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (94.0/100)

**Issues Found:**

**🎭 SIMULATION** (7 occurrences):
  - Line 155: `# This is a mock implementation for demonstration`
  - Line 157: `print("Note: This is a mock implementation. Use PyGithub for actual scraping.")`
  - Line 159: `# Mock: Create a sample snippet`
  - Line 209: `# Mock implementation`
  - Line 211: `print("Note: This is a mock implementation. Use Stack Exchange API for actual scraping.")`

---

### `examples/22_aql_diagram_tool/ThemisDB.AqlDiagramTool/Generators/AqlQueryHelper.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/22_aql_diagram_tool/ThemisDB.AqlDiagramTool/Generators/MermaidDfdGenerator.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/22_aql_diagram_tool/ThemisDB.AqlDiagramTool/Generators/MermaidErdGenerator.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/22_aql_diagram_tool/ThemisDB.AqlDiagramTool/Models/DataFlowDiagram.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/22_aql_diagram_tool/ThemisDB.AqlDiagramTool/Models/DatabaseSchema.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/22_aql_diagram_tool/ThemisDB.AqlDiagramTool/Models/Entity.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/22_aql_diagram_tool/ThemisDB.AqlDiagramTool/Models/Relationship.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/22_aql_diagram_tool/ThemisDB.AqlDiagramTool/Parsers/JsonSchemaParser.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/22_aql_diagram_tool/ThemisDB.AqlDiagramTool/Program.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/23_traveling_salesman/main.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/23_traveling_salesman/models.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/23_traveling_salesman/themis_client.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/23_traveling_salesman/tsp_algorithms.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/24_moral_philosophy_debates/ai_synthesizer.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/24_moral_philosophy_debates/argument_models.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/24_moral_philosophy_debates/complete_self_improving_ethics_loop.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 562: `"""Mock decision for testing."""`

---

### `examples/24_moral_philosophy_debates/debate_chat.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/24_moral_philosophy_debates/demo_ethics_evaluation.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/24_moral_philosophy_debates/ethical_discourse_engine.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/24_moral_philosophy_debates/ethical_scenarios_loader.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 206: `# For backward compatibility with old hardcoded module`

---

### `examples/24_moral_philosophy_debates/ethics_ai_production_deployment.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/24_moral_philosophy_debates/ethics_benchmark.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/24_moral_philosophy_debates/ethics_evaluation_metrics.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/24_moral_philosophy_debates/ethics_monitoring_dashboard.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 1227: `# Simulate some evaluations`
  - Line 1230: `# We'll create mock evaluation results`
  - Line 1283: `# Simulate 50 evaluations`
  - Line 1328: `print(f"   ✓ Ingested {50} mock evaluations")`

---

### `examples/24_moral_philosophy_debates/ethics_prompt_optimization_framework.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 467: `"""Mock LLM response for testing."""`
  - Line 480: `"""Mock refined prompt for testing."""`

---

### `examples/24_moral_philosophy_debates/example_basic_usage.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/24_moral_philosophy_debates/example_complete_workflow.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/24_moral_philosophy_debates/knowledge_researcher.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/24_moral_philosophy_debates/llm_backends.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/24_moral_philosophy_debates/lora_training_with_optimized_prompts.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🎭 SIMULATION** (6 occurrences):
  - Line 171: `# Mock decision data`
  - Line 299: `"""Generate expected output for a test case (mock)."""`
  - Line 315: `"""Fetch decision data (mock implementation)."""`
  - Line 420: `# Mock training process`
  - Line 463: `# Calculate average quality (mock)`

---

### `examples/24_moral_philosophy_debates/main.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/24_moral_philosophy_debates/main_old.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/24_moral_philosophy_debates/models.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/24_moral_philosophy_debates/moral_engine.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 438: `Mock generation for demonstration purposes.`
  - Line 444: `Mock response`

---

### `examples/24_moral_philosophy_debates/news_researcher.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/24_moral_philosophy_debates/philosophy_loader.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/24_moral_philosophy_debates/rag_context_engine.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (88.0/100)

**Issues Found:**

**🎭 SIMULATION** (9 occurrences):
  - Line 582: `# Mock Data Methods (for testing without ThemisDB)`
  - Line 586: `"""Mock similar dilemmas for testing."""`
  - Line 599: `"""Mock philosophy arguments for testing."""`
  - Line 613: `"""Mock best practices for testing."""`
  - Line 629: `"""Mock vector search for testing."""`

---

### `examples/24_moral_philosophy_debates/standalone/gui_dialectic.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/24_moral_philosophy_debates/standalone/standalone_moral_dialectic.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/24_moral_philosophy_debates/test_ethics_evaluation_metrics.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/24_moral_philosophy_debates/themis_client.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/adaptive_batching_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (84.0/100)

**Issues Found:**

**🎭 SIMULATION** (6 occurrences):
  - Line 126: `// Simulate multiple training steps`
  - Line 128: `// Simulate varying sequence lengths`
  - Line 137: `// Simulate GPU utilization feedback`
  - Line 149: `// 7. OOM Handling Simulation`
  - Line 156: `// Simulate OOM event`

---

### `examples/adaptive_retention_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/api_gateway_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/archive_pipeline.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (2 occurrences):
  - Line 32: `"""Extract archive to temporary directory"""`
  - Line 185: `# Create temporary directory`

---

### `examples/chat_formatting_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/complete_integration_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (92.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 89: `// Phase 5: Simulate Production Usage`
  - Line 93: `// Simulate SQL generation requests`
  - Line 107: `// Simulate LLM execution`

---

### `examples/complete_self_improvement_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (93.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 75: `// Step 3: Simulate Production Usage (with poor performance)`
  - Line 79: `// Simulate 15 executions with 40% success rate (below threshold)`
  - Line 148: `// Simulate A/B test observations`

---

### `examples/concerns_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (87.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 196: `// ... simulate some work ...`
  - Line 367: `// ... simulate other work while I/O is in-flight ...`

**🐛 DEBUG** (4 occurrences):
  - Line 59: `logger.setLevel(ILogger::Level::DEBUG);`
  - Line 61: `logger.debug("Debugging: value=42");`
  - Line 345: `async_logger.debugAsync("debug (async)").get();`
  - Line 393: `ctx->logDebug("Debug: concerns context ready");`

**🔒 HARDCODED** (1 occurrences):
  - Line 258: `cache.put("temp:1", {"temporary", 1, 0});`

---

### `examples/continuous_learning_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (90.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 87: `// Step 5: Simulate production usage`
  - Line 99: `// Simulate feedback (80% positive)`
  - Line 106: `// Simulate gap detection`
  - Line 146: `// Step 11: Simulate batch logging`

---

### `examples/continuous_learning_integration_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 32: `// Simulate RAG evaluations`
  - Line 82: `// Simulate decreasing quality`
  - Line 107: `// Simulate various quality scenarios`
  - Line 231: `// Simulate different quality issues`

---

### `examples/cron_and_cdc_scheduler_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 164: `// Simulate backup`
  - Line 245: `// Simulate some CDC events`

---

### `examples/data_retention_downsampling_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/distributed_sharding/distributed_sharding_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/distributed_transaction_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/domain_prompts_usage_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/e2e_qlora_training_example.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 66: `# For now, simulate the check`

---

### `examples/embedded_llm_examples.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/example_ai_auditing.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/example_approximate_radius_search.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/example_distributed_lora_training.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/example_index_manager_di.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 30: `// Example 2: Mock storage engine`

---

### `examples/example_llm_metrics.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 78: `std::cout << "   ℹ Using stub implementation for demonstration" << std::endl;`

---

### `examples/example_multi_ssd_configuration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/example_pki_lora.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/example_vector_encryption.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 267: `// Simulate server restart`

---

### `examples/feedback_collection_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (93.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 33: `// Step 2: Simulate User Feedback`
  - Line 62: `// Step 3: Simulate System-Detected Issues`
  - Line 126: `// Simulate more failures for pattern analysis`

---

### `examples/feedback_plugins/feedback_validator.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 139: `# SSN pattern (XXX-XX-XXXX)`

---

### `examples/future_works_integration_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 328: `// Simulate some requests`

---

### `examples/geo/example_3d.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/gnn/gnn_embeddings_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/gpu_vector_index_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/gradient_checkpointing_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/hot_reload_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/hot_spare_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 121: `// Step 6: Simulate shard failure`

---

### `examples/hsm_security_integration_example.cpp` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (66.0/100)

**Issues Found:**

**🔴 STUB** (8 occurrences):
  - Line 7: `* Addresses: FIND-002 - HSM Stub Provider Security`
  - Line 49: `THEMIS_INFO("  Stub Provider: {}", hsm_.isStubProvider() ? "YES (INSECURE)" : "NO (Secure)");`
  - Line 126: `hsm_config.library_path = "";  // Empty = stub for this example`
  - Line 154: `* 1. Development mode (stub allowed):`
  - Line 156: `*    > Server starts with stub provider warnings`

---

### `examples/huggingface_ingestion_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (92.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 170: `std::cout << "Demonstrating plugin capabilities with mock job...\n\n";`
  - Line 172: `// Create a mock job for demonstration`

**🔒 HARDCODED** (1 occurrences):
  - Line 37: `// Create temporary database directory`

---

### `examples/hybrid_retention_usage_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/image_analysis/image_analysis_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/learnable_rope_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/legal_lora_training/test_auto_labeler_basic.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/legal_lora_training/train_legal_lora.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/llm/multi_gpu_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 178: `// Step 7: Simulate GPU Health Check`

---

### `examples/lora_rope_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/lora_sync/example_lora_sync.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/lora_training_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 162: `// For now, just simulate with a decreasing loss`

---

### `examples/migration/contentfs_migration_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (94.0/100)

**Issues Found:**

**🎭 SIMULATION** (7 occurrences):
  - Line 44: `// Simplified storage simulation`
  - Line 72: `// Simulate write failure`
  - Line 95: `// Simulate not found`
  - Line 179: `// Simulate write failure (could be disk full, permission denied, etc.)`
  - Line 188: `// Simulate checksum verification failure`

---

### `examples/migration/index_manager_migration_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 69: `// Simulate creation that might fail`
  - Line 144: `// Simulate creation that might fail`

---

### `examples/migration/tsstore_migration_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 29: `* - Difficult to debug parse failures`

---

### `examples/moral_analyzer_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/multi_gpu_vector_index_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/multi_vector_search_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 95: `// Simulate BM25/keyword scores for documents`

---

### `examples/performance/usage_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (91.0/100)

**Issues Found:**

**🎭 SIMULATION** (8 occurrences):
  - Line 28: `* @brief Simulate HNSW vector search`
  - Line 33: `// Simulate some work`
  - Line 42: `* @brief Simulate pointer passing (should be ~150 cycles)`
  - Line 53: `* @brief Simulate LLM inference`
  - Line 58: `// Simulate heavier computation`

---

### `examples/phi3_lora_training_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/phi3_query_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/prompt_optimization_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/qlora_poc_example.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 178: `# Simulate adapter files`
  - Line 188: `print("✅ Training simulation complete")`
  - Line 215: `# Simulate GGUF file`

---

### `examples/quality_control_demo.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/rag_knowledge_gap_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (93.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 41: `// 3. Simulate retrieved documents (in real scenario, from VectorIndexManager)`
  - Line 98: `// For this example, simulate a response with token probabilities`
  - Line 104: `// Simulate token probabilities (in real case, these come from llama_wrapper)`

---

### `examples/railway/railway_base_data_generator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 251: `// Simulate curves (every 5-10 segments on average)`

---

### `examples/rope_visualization/advanced_analysis_example.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/rope_visualization/basic_visualization_example.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/rope_visualization/cli_usage_demo.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/security/access_control_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/sharding_demo.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/simple_qc_integration_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 14: `* @brief Simulate a simple RAG pipeline with quality control`
  - Line 212: `// Simulate several evaluations`

---

### `examples/task_scheduler_integration_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/test_optimization_standalone.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 8: `#define THEMIS_DEBUG(fmt, ...) std::cout << "[DEBUG] " << fmt << std::endl`

---

### `examples/themis_help_lora_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/timestamp_authority_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/version_control_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 177: `// Simulate performance testing`
  - Line 229: `// Step 11: Simulate Regression and Rollback`

---

### `examples/voice_assistant_example.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `examples/vulkan_vector_search_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `fuzz/harnesses/aql_parser_harness.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 30: `// ThemisDB AQL Parser interface (mock for harness template)`
  - Line 44: `* @brief Mock AQL Parser class`

---

### `fuzz/harnesses/gguf_loader_harness.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🔒 HARDCODED** (2 occurrences):
  - Line 44: `* @brief Write @p data to a temporary file and call GGUFLoader::parseFile().`
  - Line 47: `* temporary file, parse it, then clean up.  The important invariant is that`

---

### `fuzz/harnesses/grammar_harness.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `fuzz/harnesses/http_parser_harness.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `fuzz/harnesses/jwt_rbac_config_harness.cpp` (v0.0.29)

**Maturity Level:** 🟢 PRODUCTION-READY (88.0/100)

**Issues Found:**

**🔴 STUB** (3 occurrences):
  - Line 38: `// ─── Minimal stubs when building standalone (not linked against libthemisdb).`
  - Line 39: `// Replace the stub namespace blocks with real #include directives when linking:`
  - Line 56: `// Stub: parse and validate – real implementation verifies signature,`

---

### `fuzz/harnesses/pii_redaction_harness.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (92.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 46: `// ThemisDB headers (replace stubs below with real includes when linking)`
  - Line 52: `// Stub for harness template validation without the full build.`

---

### `fuzz/harnesses/postgres_importer_harness.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 17: `* The harness also feeds synthetic COPY data blocks through a temporary file`

---

### `fuzz/harnesses/security_input_validator_harness.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (94.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 38: `// ─── Minimal stub when building standalone (without the full ThemisDB library)`
  - Line 52: `// Simplified stub — real implementation uses the AQL AST`

---

### `fuzz/harnesses/security_policy_engine_harness.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (89.0/100)

**Issues Found:**

**🔴 STUB** (3 occurrences):
  - Line 40: `// ─── PolicyEngine stub (replace with real include when linking libthemisdb)`
  - Line 49: `// Stub: accept blobs that look like a JSON array`
  - Line 57: `// Stub: deny admin actions for non-admin users`

---

### `grafana/compliance_exporter.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/acceleration/batch_validator.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/acceleration/compute_backend.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/acceleration/cpu_backend.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/acceleration/cuda_backend.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/acceleration/device_manager.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/acceleration/error_codes.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (3 occurrences):
  - Line 164: `NotImplemented = 902`
  - Line 260: `case AccelerationErrorCode::NotImplemented:`
  - Line 261: `return "NotImplemented";`

---

### `include/acceleration/error_context.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/acceleration/faiss_gpu_backend.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/acceleration/geo_acceleration_bridge.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/acceleration/graphics_backends.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/acceleration/hip_backend.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/acceleration/kernel_fallback_dispatcher.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/acceleration/kernel_invocation.h` (v0.0.19)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/acceleration/metrics/backend_metrics.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/acceleration/metrics/metrics_collector.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/acceleration/multi_gpu_backend.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/acceleration/nccl_vector_backend.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 16: `// Stub typedefs for CPU-only builds`
  - Line 30: `* When THEMIS_ENABLE_NCCL is not defined, provides stub implementations`

---

### `include/acceleration/opencl_backend.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (91.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 84: `// Stub implementation when OpenCL is not available`
  - Line 117: `// Returns a stub (isAvailable() == false) when OpenCL is not compiled in.`

---

### `include/acceleration/plugin_loader.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/acceleration/plugin_security.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/acceleration/raii/cuda_raii.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/acceleration/raii/hip_raii.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/acceleration/raii/opencl_raii.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/acceleration/raii/vulkan_raii.h` (v0.0.10)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/acceleration/rccl_vector_backend.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 16: `// Stub typedefs for CPU-only builds`
  - Line 30: `* When THEMIS_ENABLE_RCCL is not defined, provides stub implementations`

---

### `include/acceleration/shader_integrity.h` (v0.0.10)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/acceleration/tensor_core_matmul.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/acceleration/vllm_resource_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/acceleration/vulkan_backend.h` (v0.0.10)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/analytics/analytics_export.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 125: `* @brief Get default exporter (stub implementation)`

---

### `include/analytics/anomaly_detection.h` (v0.0.18)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/analytics/arrow_export.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/analytics/arrow_flight.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/analytics/automl.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/analytics/cep_engine.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/analytics/columnar_execution.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/analytics/diff_engine.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/analytics/distributed_analytics.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/analytics/forecasting.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/analytics/incremental_view.h` (v0.0.18)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/analytics/jit_aggregation.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/analytics/llm_process_analyzer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/analytics/ml_serving.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/analytics/model_serving.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/analytics/nlp_text_analyzer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/analytics/olap.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/analytics/process_mining.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/analytics/process_pattern_matcher.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/analytics/streaming_window.h` (v0.0.18)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/api/audit_logger.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/api/geo_index_hooks.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/api/graphql.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/api/graphql_cache.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 265: `// TODO: Implement pattern-based invalidation`

---

### `include/api/graphql_metrics.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/api/grpc_server.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (2 occurrences):
  - Line 50: `*  - gRPC reflection is exposed in debug builds only to prevent schema`
  - Line 97: `* the proto schema is not exposed to unauthenticated callers.  In debug`

---

### `include/api/persisted_queries.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/api/rate_limiter.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/api/themisdb_grpc_service.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 26: `*   grpc_api_server.registerService(svc.service());  // nullptr when stubs absent`

---

### `include/api/tracing_middleware.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/api/ws_handler.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/aql/aql_autocomplete.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/aql/aql_confidence_scorer.h` (v0.0.25)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/aql/aql_conversation_context.h` (v0.0.25)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/aql/aql_fewshot_example_library.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/aql/aql_lora_finetuner.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 149: `*       environments the service runs in simulation mode.`

---

### `include/aql/aql_migration_assistant.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/aql/aql_optimizer_advisor.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/aql/aql_query_builder.h` (v0.0.25)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/aql/aql_query_template_library.h` (v0.0.25)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/aql/aql_query_validator.h` (v0.0.25)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/aql/aql_schema_provider.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/aql/aql_syntax_highlighter.h` (v0.0.22)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/aql/docs_assistant_functions.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/aql/llm_aql_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/aql/llm_error_codes.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/aql/llm_metrics_collector.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/aql/llm_timeout_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/auth/api_key_authenticator.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/auth/auth_audit_logger.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/auth/auth_error.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/auth/auth_metrics.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 35: `// Provide stub types when Prometheus is not available`

---

### `include/auth/auth_rate_limiter.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/auth/federated_identity_manager.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 176: `/// Optional HTTP mock injected for testing; applied to all new realms`

---

### `include/auth/gssapi_authenticator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/auth/jwks_security.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/auth/jwks_validator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/auth/jwt_key_rotation_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/auth/jwt_validator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/auth/kerberos_security.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/auth/ldap_authenticator.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/auth/mfa_authenticator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/auth/mtls_authenticator.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/auth/oauth_device_flow.h` (v0.0.6)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/auth/oauth_pkce_flow.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/auth/oidc_provider.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 170: `* @brief Inject a mock discovery document (bypasses HTTP fetch).`

---

### `include/auth/password_policy.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/auth/principal_validator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/auth/saml_authenticator.h` (v0.0.6)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/auth/session_manager.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/auth/token_blacklist.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/auth/totp_replay_cache.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/auth/totp_secret_encryption.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/auth/webauthn_authenticator.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/auth/zero_trust_auth_verifier.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cache/adaptive_query_cache.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cache/aligned_vector_allocator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cache/arc_cache.h` (v0.0.32)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cache/bounded_lru_cache.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cache/cache_hit_rate_slo_monitor.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cache/cache_metrics.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cache/cache_provider.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cache/cache_replication.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cache/cache_replication_coordinator.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cache/distributed_cache_coordinator.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cache/embedding_cache.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cache/enhanced_query_cache.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cache/eviction_policy.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cache/l1_tinylfu_cache.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cache/predictive_prefetcher.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cache/redis_cache_coordinator.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cache/request_coalescer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cache/result_cache.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cache/semantic_cache.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cdc/cdc_admin.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cdc/cdc_error.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cdc/cdc_materialized_view.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cdc/cdc_metrics.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cdc/cdc_ws_handler.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cdc/change_stream_compressor.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cdc/changefeed.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cdc/changefeed_buffer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cdc/consumer_group.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cdc/cross_collection_stream.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cdc/dead_letter_queue.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cdc/debezium_format.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cdc/delivery_tracker.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cdc/icdc_transport.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cdc/kafka_cdc_producer.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🔴 STUB** (3 occurrences):
  - Line 15: `* can reference KafkaCDCProducer unconditionally; the no-op stubs compile to`
  - Line 243: `#else // !THEMIS_ENABLE_KAFKA ── no-op stub ────────────────────────────────────`
  - Line 246: `* @brief No-op stub compiled when THEMIS_ENABLE_KAFKA is not defined.`

**🐛 DEBUG** (1 occurrences):
  - Line 56: `* logged even at DEBUG level.`

---

### `include/cdc/outbox.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cdc/schema_registry.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 457: `*  - AVRO     – UTF-8 JSON bytes (stub; full Avro binary requires avro-cpp).`
  - Line 458: `*  - PROTOBUF – UTF-8 JSON bytes (stub; full proto binary requires protobuf).`

---

### `include/cdc/tenant_buffer_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/cdc/ws_transport.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/chimera/database_adapter.hpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/chimera/elasticsearch_adapter.hpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 26: `* in an in-process simulation mode backed by std::unordered_map, which is`
  - Line 55: `*       linked the adapter operates in an in-process simulation mode suitable`
  - Line 204: `* @details In simulation mode this checks whether every whitespace-delimited`
  - Line 210: `* @param field   Field to search (use "_all" to simulate multi-field search).`

---

### `include/chimera/mongodb_adapter.hpp` (v0.0.8)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 51: `*       not linked the adapter operates in an in-process simulation mode`

---

### `include/chimera/neo4j_adapter.hpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 28: `* in-process simulation mode backed by std::unordered_map, which is`
  - Line 58: `*       is not linked the adapter operates in an in-process simulation mode`
  - Line 208: `// In-process graph store for simulation.`

---

### `include/chimera/pinecone_adapter.hpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**🎭 SIMULATION** (5 occurrences):
  - Line 33: `* in-process simulation mode backed by std::unordered_map, which is`
  - Line 62: `*       the adapter operates in an in-process simulation mode suitable for`
  - Line 212: `// In-process vector store for simulation.`
  - Line 217: `// In-process metadata document store for simulation.`
  - Line 235: `/// a managed cloud service; plain http is rejected in simulation mode as`

---

### `include/chimera/postgresql_adapter.hpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🎭 SIMULATION** (5 occurrences):
  - Line 28: `* in an in-process simulation mode backed by std::unordered_map, which`
  - Line 55: `*       operates in an in-process simulation mode suitable for unit testing`
  - Line 152: `// IDocumentAdapter (JSONB simulation)`
  - Line 211: `// In-process vector store for pgvector simulation.`
  - Line 216: `// In-process document store for JSONB simulation.`

---

### `include/chimera/qdrant_adapter.hpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 28: `* in-process simulation mode backed by std::unordered_map, which is`
  - Line 57: `*       the adapter operates in an in-process simulation mode suitable for`
  - Line 207: `// In-process vector store for simulation.`
  - Line 212: `// In-process document store for payload simulation.`

---

### `include/chimera/themisdb_adapter.hpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/chimera/weaviate_adapter.hpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 28: `* in an in-process simulation mode backed by std::unordered_map, which`
  - Line 56: `*       is not linked the adapter operates in an in-process simulation mode`
  - Line 206: `// In-process vector store for simulation.`
  - Line 211: `// In-process document store for object simulation.`

---

### `include/content/archive_processor.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (3 occurrences):
  - Line 78: `std::string temp_directory;  // Temporary directory used for extraction`
  - Line 195: `* @brief Extract archive to temporary directory`
  - Line 221: `* @brief Clean up temporary extraction directory`

---

### `include/content/async_ingestion_worker.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/content/audio_processor.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/content/cad_processor.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/content/content_errors.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/content/content_fs.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/content/content_logger.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (2 occurrences):
  - Line 173: `* @brief Log debug message`
  - Line 175: `void debug(const std::string& event, const std::string& message, const json& metadata = json::object`

---

### `include/content/content_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/content/content_metrics.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/content/content_plugin_interface.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/content/content_policy.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/content/content_processor.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 30: `std::string temp_directory;    // Temporary extraction directory (for archives)`

---

### `include/content/content_security.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 31: `bool enable_abuse_detection = false;  // Stub for future implementation`
  - Line 71: `* - Content abuse detection (stub)`

---

### `include/content/content_type.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/content/content_validator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/content/deduplication_checker.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/content/embedding_pipeline.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/content/geo_processor.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/content/html_processor.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/content/image_processor.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/content/ingestion_plugin.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/content/language_detector.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/content/markdown_processor.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/content/mime_detector.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/content/mock_clip_processor.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 9: `// Mock CLIP-like image processor for deterministic embeddings used in tests.`

---

### `include/content/ocr_processor.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 93: `* @brief Generate embedding for OCR text chunk (stub – delegates to pipeline)`

**🔒 HARDCODED** (1 occurrences):
  - Line 116: `* Creates a temporary OcrProcessor with the given language/data_dir,`

---

### `include/content/office_processor.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/content/pdf_processor.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/content/pipeline/async_bulk_uploader.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/content/pipeline/bulk_upload_interface.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/content/pipeline/content_chunker.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/content/pipeline/multimodal_chunker.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/content/pipeline/zstd_compression.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/content/processor_chain_config.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/content/stt_processor.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/content/tts_processor.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/content/version_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/content/video_processor.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/core/concerns/cache_strategies.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/core/concerns/concerns_context.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 169: `void logDebug(const std::string& message) { logger_->debug(message); }`

---

### `include/core/concerns/context_propagation.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 27: `*   // At the entry point of a request (HTTP handler, gRPC stub …)`

---

### `include/core/concerns/eviction_strategies.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/core/concerns/i_async_cache.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/core/concerns/i_async_logger.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (3 occurrences):
  - Line 86: `* @brief Asynchronously log at DEBUG level.`
  - Line 91: `return logAsync(Level::DEBUG, message);`
  - Line 172: `void debug(const std::string&)    override {}`

---

### `include/core/concerns/i_cache.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 36: `* Enables testing with mock caches and runtime switching of implementations.`

---

### `include/core/concerns/i_circuit_breaker.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/core/concerns/i_context.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 40: `*   (e.g. HTTP handler, gRPC stub).`

---

### `include/core/concerns/i_feature_flags.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/core/concerns/i_logger.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 39: `* mock loggers and runtime switching of logging implementations.`

**🐛 DEBUG** (4 occurrences):
  - Line 53: `DEBUG,`
  - Line 81: `/// @brief Log at DEBUG level (developer-facing diagnostic detail).`
  - Line 83: `virtual void debug(const std::string& message) = 0;`
  - Line 202: `* Accepts "trace", "debug", "info", "warn", "error", "critical".`

---

### `include/core/concerns/i_metrics.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 18: `* Enables testing with mock metrics and runtime switching of implementations.`

---

### `include/core/concerns/i_secrets.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/core/concerns/i_tracer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 17: `* Enables testing with mock tracers and runtime switching of implementations.`

---

### `include/core/concerns/inmemory_cache_impl.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/core/concerns/jaeger_tracer_adapter.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/core/concerns/lifecycle.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/core/concerns/metric_labels.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/core/concerns/noop_implementations.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 22: `void debug(const std::string& message) override {}`

---

### `include/core/concerns/otel_tracer_adapter.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/core/concerns/prometheus_metrics_adapter.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/core/concerns/spdlog_logger_adapter.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (5 occurrences):
  - Line 35: `case Level::DEBUG: debug(message); break;`
  - Line 47: `void debug(const std::string& message) override {`
  - Line 48: `if (logger_) logger_->debug(message);`
  - Line 148: `case spdlog::level::debug: return Level::DEBUG;`
  - Line 191: `case Level::DEBUG: return spdlog::level::debug;`

---

### `include/core/concerns/strategic_cache_impl.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/core/concerns/w3c_trace_context_propagator.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/core/concerns/zipkin_tracer_adapter.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/core/config_validator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (2 occurrences):
  - Line 130: `const std::vector<std::string> valid_levels = {"trace", "debug", "info", "warn", "error", "critical"`
  - Line 140: `result.addError("Invalid log_level: '" + log_level + "'. Must be one of: trace, debug, info, warn, e`

---

### `include/core/index_initialization.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/core/production_mode.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/core/query_engine_builder.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/core/security_initialization.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/core/storage_initialization.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/document/document_manager_deprecated.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/document/encrypted_entities.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/exporters/aql_predicate_filter.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/exporters/arrow_ipc_exporter.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/exporters/data_augmentation.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/exporters/export_encryption.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/exporters/exporter_errors.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/exporters/exporter_interface.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/exporters/exporter_metrics.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/exporters/format_template.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/exporters/huggingface_exporter.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/exporters/incremental_exporter.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/exporters/jsonl_llm_exporter.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/exporters/parquet_exporter.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/exporters/pii_detector.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/exporters/stream_writer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/exporters/streaming_exporter.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/geo/device_detector.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/geo/geo_clustering.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/geo/geo_ops_ext.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/geo/geo_rtree.h` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/geo/gpu_kernel_dispatcher.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/geo/raster.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/geo/spatial_backend.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/geo/spatial_join.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/geo/temporal_spatial_query.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/geo/tile_server.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/governance/ccpa_rules.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/governance/compliance_reporter.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/governance/compliance_reporting.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/governance/cross_tenant_policy_inheritance.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/governance/data_lineage.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/governance/data_masker.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/governance/model_governance.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/governance/opa_adapter.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/governance/pci_dss_rules.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/governance/policy_coordinator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/governance/policy_engine.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 208: `/// Evaluate policies in dry-run (simulation) mode without writing an audit entry.`
  - Line 216: `/// @param request  The simulation request (headers + route).`

---

### `include/governance/policy_file_watcher.h` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/governance/policy_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/governance/policy_manager_versioned.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/governance/policy_review.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/governance/policy_template.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/governance/policy_validation.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/governance/policy_validator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/governance/policy_version_history.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/governance/review_scheduler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/governance/soc2_controls.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/graph/distributed_graph.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/graph/gpu_traversal.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/graph/graph_query_optimizer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/graph/parallel_traversal.h` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/graph/path_constraints.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/importers/conflict_resolver.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/importers/flatfile_importer.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/importers/importer_interface.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/importers/importer_plugin_api.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/importers/kafka_importer.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 178: `* @brief Function type for injecting mock Kafka messages in unit tests.`
  - Line 186: `* @brief Inject a mock message-fetch function (unit-testing only).`
  - Line 247: `/** Run the consume loop against the mock message function. */`

---

### `include/importers/mongo_importer.h` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/importers/mysql_importer.h` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/importers/oracle_importer.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/importers/postgres_importer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/importers/s3_importer.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 39: `/// Optional session token for temporary credentials (AWS STS / IAM roles).`

---

### `include/importers/schema_validator.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/importers/sqlite_importer.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/adaptive_index.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/advanced_vector_index.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/ann_index.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/approximate_radius_search.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/binary_quantizer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/distributed_vector_index.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/edge_types.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/gnn_embeddings.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/gpu_vector_index.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 64: `bool enableValidation = false; // Enable GPU validation layers (debug)`

---

### `include/index/graph_analytics.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/graph_auto_buffer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/graph_index.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/hnsw_layer_optimizer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/hnsw_parameter_tuner.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/hnsw_production_defaults.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/index_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 10: `/// - Enable isolated unit testing with mock implementations`

---

### `include/index/inverted_index.h` (v0.0.12)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/learnable_rope.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/learned_index.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/learned_quantizer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/lora_rope.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/multi_gpu_vector_index.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/multi_vector_search.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/process_graph.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/product_quantizer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/property_graph.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/residual_quantizer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/rotary_embeddings.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/rotary_embeddings_gpu.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/secondary_index.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/secondary_index_metadata_cache.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/spatial_index.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/temporal_graph.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/tiered_index_manager.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/vector_auto_buffer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/vector_index.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/vector_index_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/index/workload_replay.h` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/ingestion/api_connector.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 143: `* @brief Inject a mock HTTP GET function (for unit testing only)`
  - Line 169: `* @brief Inject a mock HTTP POST function for OAuth token refresh (unit testing only)`

---

### `include/ingestion/cdc_connector.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (5 occurrences):
  - Line 24: `* unless a mock has been injected via `setCdcEventFetchForTesting()`.`
  - Line 109: `* Returns true when a test mock is injected.`
  - Line 129: `* When `THEMIS_ENABLE_CDC_STREAM` is not defined and no test mock is`
  - Line 162: `* @brief Function type for injecting mock CDC events in unit tests.`
  - Line 171: `* @brief Inject a mock event-fetch function (unit testing only).`

---

### `include/ingestion/database_connector.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (5 occurrences):
  - Line 24: `* unless a row-fetch mock has been injected via `setRowFetchForTesting()`.`
  - Line 105: `* true when a test mock is injected.`
  - Line 124: `* When `THEMIS_ENABLE_ODBC` is not defined and no test mock is present,`
  - Line 141: `* @brief Function type for injecting mock database rows in unit tests.`
  - Line 149: `* @brief Inject a mock row-fetch function (unit testing only).`

---

### `include/ingestion/filesystem_ingester.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/ingestion/huggingface_connector.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 117: `* @brief Inject a mock HTTP POST function for OAuth token refresh (unit testing only)`

---

### `include/ingestion/ingestion_coordinator.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 467: `* @brief Register an external or mock worker node.`
  - Line 531: `* @brief Inject a custom leader election backend (testing / simulation only).`

---

### `include/ingestion/ingestion_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 372: `* @brief Function type for injecting a mock HTTP GET response in tests.`
  - Line 384: `* @brief Function type for injecting a mock HTTP POST response in tests.`
  - Line 890: `* @brief Inject a mock HTTP GET function for all API connectors (testing only)`

---

### `include/ingestion/kafka_connector.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 119: `* @brief Function type for injecting mock Kafka messages in unit tests.`
  - Line 128: `* @brief Inject a mock message-fetch function (unit testing only).`

---

### `include/ingestion/object_storage_connector.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (94.0/100)

**Issues Found:**

**🎭 SIMULATION** (7 occurrences):
  - Line 32: `* always returns `CONNECTOR_NOT_SUPPORTED` — unless a test mock has been`
  - Line 101: `* Returns true when a test mock is injected.  Without a mock the result`
  - Line 121: `* When neither a test mock nor a provider SDK is available, returns`
  - Line 133: `* @brief Function type for injecting a mock object-list in unit tests.`
  - Line 141: `* @brief Function type for injecting mock object bodies in unit tests.`

---

### `include/ingestion/web_crawler_connector.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 20: `* injected mock (for unit tests).`
  - Line 114: `* @brief Function type for injecting mock HTTP responses in unit tests.`
  - Line 123: `* @brief Inject a mock HTTP fetch function (unit testing only).`

---

### `include/llm/adapter_compatibility.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/adapter_deployment_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/adapter_load_balancer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/adapter_registry.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/adaptive_vram_allocator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/ai_decision_auditor.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/ai_orchestrator.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/applications/themis_help_lora.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/aql_train_parser.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/async_inference_engine.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/attention/cuda/flash_attention_cuda.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/attention/flash_attention.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/attention/flash_attention_config.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/attention/hip/flash_attention_hip.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/attention/kv_cache_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/attention/vulkan/flash_attention_vulkan.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/batch_generator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/block_table.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/byzantine_detector.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/constitutional_reasoning_engine.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/continuous_batch_scheduler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/distributed_training_coordinator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/docs_assistant.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/embedded_llm.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/ethical_guidelines_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/ethics_aware_confidence_detector.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/explanation_generator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/feedback_store.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/fewshot_optimizer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/gguf_loader.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/gguf_st_adapter.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/gpu_memory_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/gpu_safe_fail.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/grafana_metrics.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 311: `std::string admin_simulate_path = "/admin/prompt/simulate";`
  - Line 369: `* @brief Register a callback for POST /admin/prompt/simulate.`

---

### `include/llm/grammar.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/grammar_cache.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/i_feedback_plugin.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/i_llm_plugin.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/inference_engine_enhanced.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/inference_handle.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/inline_training_engine.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/json_schema_converter.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/kernel_fusion.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/kernel_fusion_cuda.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/kv_cache_buffer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lazy_model_loader.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/llama_resource_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/llama_wrapper.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 98: `* Explicit state tracking prevents silent stub responses and enables`

**🔒 HARDCODED** (3 occurrences):
  - Line 106: `UNAVAILABLE      // Temporary unavailability (e.g., OOM, evicted)`
  - Line 271: `* 4. Write to temporary file (with cleanup)`
  - Line 294: `* @brief Clean up old temporary model files`

---

### `include/llm/llamacpp_inference_engine.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/llamacpp_training_backend.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/llm_deployment_plugin.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/llm_interaction_store.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/llm_model_audit_logger.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/llm_model_storage.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 166: `std::shared_ptr<KeyProvider> key_provider;  // Configurable key provider (Vault/HSM/Mock)`

---

### `include/llm/llm_plugin_interface.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/llm_plugin_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/llm_prefix_cache.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/llm_response_cache.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/llm_security_utils.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/adapter_consistency_checker.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/adapter_sync_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/adaptive_batcher.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/base_model_adapter.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/cpu_fused_kernels.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/cuda_bf16_kernels.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/cuda_flash_lora_kernels.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/cuda_fp16_kernels.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/cuda_fused_kernels.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/cuda_kernels.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/custom_allreduce.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/data_loader.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/directx_buffer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/directx_context.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/directx_descriptors.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/directx_kernels.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/directx_pipeline.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/directx_shader.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/directx_shader_utils.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/distributed_dataloader.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/distributed_trainer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/embedding_provider.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/feedback_plugin.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/flash_lora.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/gguf_converter.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/gpu_data_loader.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/gpu_embedding_layer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/gpu_lora_layers.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/gpu_memory.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/gpu_tensor.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/gpu_training_loop.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 95: `* - Real GPU kernel execution (not CPU simulation)`

---

### `include/llm/lora_framework/gpu_utilization_monitor.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/gradient_checkpointing.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/gradient_utils.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/hip_fused_kernels.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/hip_kernels.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/llama_tokenizer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/lora_audit_logger.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/lora_config.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/lora_feedback.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/lora_feedback_storage.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/lora_graph.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 272: `// TODO: Parse embeddings`

---

### `include/llm/lora_framework/lora_layers.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/lora_metrics.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 33: `// Provide stub types when Prometheus is not available`

---

### `include/llm/lora_framework/lora_orchestrator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/lora_provenance.h` (v0.0.26)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/lora_storage_service.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/lora_training_config.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/lora_training_service.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/lr_scheduler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/mixed_precision.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/model_compatibility.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/multi_gpu.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/multi_gpu_lora_layer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/multi_gpu_trainer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/nccl_backend.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/paged_memory_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/paged_optimizer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/quantization.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/quantization_kernels.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/quantized_model.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/rccl_backend.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/resource_profiler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/sequence_packer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/tensor_dtype.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 68: `* @brief Convert FP32 to FP16 (CPU simulation)`
  - Line 98: `* @brief Convert FP16 to FP32 (CPU simulation)`
  - Line 133: `* @brief Convert FP32 to BF16 (CPU simulation)`
  - Line 152: `* @brief Convert BF16 to FP32 (CPU simulation)`

---

### `include/llm/lora_framework/training_service_registry.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/vram_allocator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/vulkan_buffer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 128: `// Stub implementation when Vulkan is not available`

---

### `include/llm/lora_framework/vulkan_context.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 213: `// Stub implementation when Vulkan is not available`

**🐛 DEBUG** (1 occurrences):
  - Line 177: `* @brief Setup debug messenger (if validation enabled)`

---

### `include/llm/lora_framework/vulkan_kernels.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_framework/vulkan_pipeline.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 173: `// Stub implementation when Vulkan is not available`

---

### `include/llm/lora_metadata_cache.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_router.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/lora_security_validator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/meta_prompt_generator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/mixed_precision_inference.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/ml_model_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/model_downloader.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/model_loader.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/model_metadata_cache.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/model_quantization_pipeline.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/model_router.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/moral_analyzer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/multi_gpu_memory_coordinator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/multi_lora_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/multi_model_training_data.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/multi_perspective_generator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/openai_compat_adapter.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/paged_block_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/paged_kv_cache.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/paged_kv_cache_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/production_validator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 204: `bool simulateQualityTest(const QualityTest& test);  // Simulation helper for consistent pass rate`

---

### `include/llm/prompt_evaluator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/prompt_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/prompt_optimizer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/prompt_policy.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/sampling_strategy.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/security/signature_verifier.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/shared_worker_pool.h` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/speculative_decoder.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/streaming_handler.h` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/token_quota_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/training_data_iterator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/vision_config.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/vision_encoder.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/llm/vision_resource_monitor.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/metadata/aql_schema_bridge.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/metadata/catalog_exporter.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/metadata/column_lineage.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/metadata/distributed_catalog.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/metadata/er_diagram_exporter.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/metadata/index_recommender.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/metadata/information_schema.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/metadata/schema_audit_log.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/metadata/schema_consistency_checker.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/metadata/schema_constraints.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/metadata/schema_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/metadata/schema_version_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/metadata/statistics_collector.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/network/connection_compression.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/network/envoy_xds.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/network/geo_topology_router.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/network/grpc_transport.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 11: `//     bidirectional streaming (generic service; no generated protobuf stubs`
  - Line 59: `* `AsyncGenericService` so that no proto-generated stubs are required in the`

---

### `include/network/qos_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/network/quic_transport.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/network/service_mesh.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/network/socket_timeout_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/network/udp_fast_path.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/network/wire_protocol_connection_pool.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/network/wire_protocol_helpers.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/network/wire_protocol_performance.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/network/wire_protocol_server.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/network/wire_protocol_websocket.h` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/observability/alertmanager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/observability/continuous_profiler.h` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/observability/distributed_flame_graph.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/observability/ebpf_tracer.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/observability/metrics_collector.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/observability/performance_analyzer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/observability/query_profiler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/observability/storage_profiler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/alignment_examples.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/alignment_helpers.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/allocator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/cicada.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/cycle_metrics.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/cycle_metrics_config.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/dostoevsky.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/expected_cycles.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/feature_flags.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/feature_flags_examples.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/huge_pages.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/ligra.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/lirs_cache.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/lockfree_metrics_buffer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/numa_topology.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/phase2_feature_flags.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/phase3/adaptive_batch_tuner.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/phase3/bao.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/phase3/bwtree.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/phase3/diskann.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/phase3/feature_flags.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/phase3/gunrock.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/phase3/memory_pressure.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/phase3/per_query_cost_model.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/phase3/splinterdb.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/phase4/feature_flags.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/phase4/io_uring_zero_copy.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/phase4/pmem_storage.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/phase4/pmu_counters.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/prefetch_hints.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/rabitq.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/rcu.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/rcu_hash_table.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/runtime_config.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/wisckey.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/performance/workload_predictor.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/plugins/ai/ai_plugin_generator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/plugins/ethics_ai/ethics_ai_plugin_interface.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/plugins/ethics_ai/ethics_ai_types.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/plugins/huggingface_ingestion_plugin.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/plugins/image_analysis_interface.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 523: `* Run a dummy inference to ensure model is loaded`

---

### `include/plugins/image_analysis_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/plugins/oci_registry_client.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/plugins/plugin_api.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/plugins/plugin_dependency_resolver.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/plugins/plugin_health_monitor.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/plugins/plugin_hot_plug_monitor.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/plugins/plugin_interface.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/plugins/plugin_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/plugins/plugin_metrics.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/plugins/plugin_registry.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/plugins/rpc_plugin_interface.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/plugins/self_healing_plugin.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 157: `* - Free temporary memory`

---

### `include/plugins/signed_plugin_repository.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/projects/DocumentManager/document_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 124: `* 5. Generate embeddings for each chunk (external API or mock)`

---

### `include/prompt_engineering/feedback_collector.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/prompt_engineering/meta_prompt_generator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/prompt_engineering/prompt_engineering_integration.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/prompt_engineering/prompt_engineering_metrics.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/prompt_engineering/prompt_evaluator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/prompt_engineering/prompt_injection_detector.h` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/prompt_engineering/prompt_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/prompt_engineering/prompt_optimizer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/prompt_engineering/prompt_performance_tracker.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/prompt_engineering/prompt_version_control.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/prompt_engineering/self_improvement_orchestrator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/adaptive_optimizer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/aql_parser.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/aql_runner.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/aql_translator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/cross_cluster_federation.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 62: `* HTTP injection: for unit testing, supply a mock via `setHttpPostForTesting()`.`

---

### `include/query/cte_cache.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/cte_subquery.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 18: `* Unterstützt WITH-Clause für temporary named result sets:`

---

### `include/query/functions/ai_ml_functions.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/functions/array_functions.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/functions/collection_functions.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/functions/crs_functions.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 652: `// TODO: Add more datum transformations as needed`

---

### `include/query/functions/date_functions.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/functions/document_functions.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/functions/ethics_functions.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/functions/file_functions.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/functions/fulltext_functions.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/functions/function_adapter.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/functions/function_registry.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/functions/geo_functions.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/functions/graph_extensions.h` (v0.0.33)

**Maturity Level:** 🟠 BETA (50.0/100)

**Issues Found:**

**🔴 STUB** (9 occurrences):
  - Line 35: `// TODO: Implement - currently a stub using undefined API types`
  - Line 85: `// TODO: Implement - currently a stub using undefined API types`
  - Line 192: `// TODO: Implement - currently a stub using undefined API types`
  - Line 240: `// TODO: Implement - currently a stub using undefined API types`
  - Line 278: `// TODO: Implement - currently a stub using undefined API types`

**📝 TODO** (10 occurrences):
  - Line 35: `// TODO: Implement - currently a stub using undefined API types`
  - Line 85: `// TODO: Implement - currently a stub using undefined API types`
  - Line 149: `// TODO: Consider returning error information in result structure`
  - Line 192: `// TODO: Implement - currently a stub using undefined API types`
  - Line 240: `// TODO: Implement - currently a stub using undefined API types`

---

### `include/query/functions/graph_functions.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/functions/holiday_provider.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/functions/json_path_functions.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/functions/lora_functions.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/functions/math_functions.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/functions/process_functions.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 76: `// This is a stub - actual implementation would query _milestone_instances`

---

### `include/query/functions/process_mining_functions.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/functions/relational_functions.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/functions/retention_functions.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (4 occurrences):
  - Line 274: `// TODO: Implement authentication/authorization check`
  - Line 301: `// TODO: Get TaskScheduler instance from context and register task`
  - Line 344: `// TODO: Get TaskScheduler instance from context`
  - Line 379: `// TODO: Implementation`

---

### `include/query/functions/security_functions.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/functions/string_functions.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/functions/udf_registry.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/functions/vector_functions.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/let_evaluator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/materialized_cte.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/optimizer_cost_model.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/parallel_scan.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/query_cache.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/query_cache_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/query_engine.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 677: `// Minimal stub implementation matching IExpressionEvaluator`
  - Line 687: `// Note: Implementation is currently a stub for Phase 3`

---

### `include/query/query_federation.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/query_optimizer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 116: `// Content+Geo (Fulltext + Spatial) Cost Model (stub)`
  - Line 133: `// Graph Shortest Path Cost Model (stub)`

---

### `include/query/query_plan_visualizer.h` (v0.0.11)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/query_resource_limits.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/result_stream.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/result_type_annotation.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/runtime_reoptimizer.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/semantic_cache.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/sparql_parser.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/sql_parser.h` (v0.0.2)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/statistical_aggregator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/subquery_optimizer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/vectorized_execution.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/window_evaluator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/query/workload_cache_strategy.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/ab_testing_framework.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/agentic_rag.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/batch_evaluator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/bayesian_optimizer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/bias_detector.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/calibration_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/citation_highlighter.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/claim_extractor.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/coherence_evaluator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/completeness_evaluator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/continuous_learning_client.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/continuous_learning_orchestrator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 102: `// Note: These are registration stubs - actual implementation would require`

---

### `include/rag/cot_evaluator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/document_splitter.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/document_summarizer.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/evaluation_cache.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/evaluation_report_exporter.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/faithfulness_evaluator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/geval_evaluator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/hallucination_dashboard.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/http_metrics_client.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/hybrid_retriever.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 13: `* mock data) and hand them to HybridRetriever for fusion.`

---

### `include/rag/judge_config.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/judge_ensemble.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/knowledge_gap_detector.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/knowledge_graph_retriever.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/learning_metrics.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/llm_integration.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/llm_judge_client.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/llm_judge_integration.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (86.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 137: `* @brief Default inference function (stub)`

**🎭 SIMULATION** (8 occurrences):
  - Line 43: `* Usage Example (Testing with Mock):`
  - Line 48: `*   // No inference function needed - will use mock responses`
  - Line 64: `// Mock mode configuration`
  - Line 65: `bool use_mock_mode = false;           // Enable mock responses (for testing only)`
  - Line 66: `bool warn_on_mock_mode = true;        // Log warning once when mock mode is used`

---

### `include/rag/llm_meta_analyzer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/multimodal_rag.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/nli_faithfulness_verifier.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/onnx_model_loader.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/pairwise_comparator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/prompt_templates.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/quality_control_factory.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/quality_control_pipeline.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/rag_integration_helpers.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 189: `// TODO Phase 2: Implement batch embedding and search`

---

### `include/rag/rag_judge.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/relevance_evaluator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/reranker.h` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/response_parser.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/rubric_evaluator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/rag/streaming_retriever.h` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/raid_data_pusher.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/replication/multi_master_replication.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/replication/replication_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 470: `// when a peer replies positively to our RequestVote RPC simulation)`
  - Line 912: `// Per-replica read simulation (real impl would use RPC)`

---

### `include/scheduler/distributed_task_coordinator.h` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/scheduler/event_trigger.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/scheduler/external_scheduler_adapter.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/scheduler/hybrid_retention_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/scheduler/task_anomaly_detector.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/scheduler/task_audit_event.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/scheduler/task_audit_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/scheduler/task_result_store.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/scheduler/task_scheduler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 125: `*                 (network timeout, temporary resource exhaustion)`

---

### `include/search/autocomplete.h` (v0.0.29)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/search/cross_lingual_search.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/search/faceted_search.h` (v0.0.29)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/search/fuzzy_matcher.h` (v0.0.29)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/search/hybrid_search.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/search/learning_to_rank.h` (v0.0.29)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/search/llm_query_rewriter.h` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 29: `* a mock function.`

---

### `include/search/llm_reranker.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/search/multi_field_search.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/search/multi_modal_search.h` (v0.0.29)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/search/neural_sparse_retrieval.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/search/personalized_ranker.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/search/query_expander.h` (v0.0.29)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/search/search_analytics.h` (v0.0.29)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/security/access_control.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/security/access_control_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/security/aql_injection_detector.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/security/binary_manifest.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/security/cms_signing.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/security/confidential_computing.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/security/crypto_capabilities.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/security/encryption.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/security/fips_crypto_mode.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/security/hsm_key_provider_adapter.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/security/hsm_provider.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (3 occurrences):
  - Line 242: `* Check if using stub provider (insecure development mode)`
  - Line 243: `* @return true if stub provider is active, false if real HSM`
  - Line 248: `* Perform periodic security check and log warnings if stub is active`

---

### `include/security/hsm_security_checker.h` (v0.0.33)

**Maturity Level:** 🟠 BETA (50.0/100)

**Issues Found:**

**🔴 STUB** (13 occurrences):
  - Line 43: `* Check if stub HSM override flag is present`
  - Line 46: `* @return true if --allow-stub-hsm flag is present`
  - Line 50: `if (std::string(argv[i]) == "--allow-stub-hsm") {`
  - Line 60: `* Enforces that stub HSM provider is not used in production mode`
  - Line 61: `* unless explicitly overridden with --allow-stub-hsm flag.`

---

### `include/security/hsm_security_metrics.h` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (71.0/100)

**Issues Found:**

**🔴 STUB** (7 occurrences):
  - Line 22: `* - hsm_security_stub_active: Gauge (0 = secure, 1 = stub active)`
  - Line 34: `// HSM security stub active gauge`
  - Line 35: `oss << "# HELP hsm_security_stub_active Indicates if HSM stub provider is active (0=secure, 1=stub)\`
  - Line 55: `std::string provider_type = hsm.isStubProvider() ? "stub" : "real";`
  - Line 111: `// Compliance status metric (derived from stub status)`

---

### `include/security/key_provider.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/security/malware_scanner.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/security/manifest_signer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/security/mock_key_provider.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/security/output_encoding.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/security/pii_redaction_policy.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 102: `* constructing a temporary map, making it suitable for per-call-site use`

---

### `include/security/pkcs11_minimal.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/security/pkcs11_wrapper.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/security/pki_key_provider.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/security/post_quantum_crypto.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (5 occurrences):
  - Line 20: `* @note This implementation uses an OpenSSL-backed software simulation`
  - Line 24: `*       stable. The simulation is labeled KYBER_SIM in diagnostic output.`
  - Line 31: `* Performance (software simulation):`
  - Line 128: `* @note This implementation uses an OpenSSL-backed software simulation`
  - Line 131: `*       The simulation is labeled DILITHIUM_SIM in diagnostic output.`

---

### `include/security/query_masking_policy.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/security/rbac.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/security/row_level_security.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/security/secret_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/security/signing.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/security/signing_provider.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/security/timestamp_authority.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/security/transport_security_checker.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/security/tsa_api.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/security/usb_admin_authenticator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/security/user_registration_plugin.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/security/vault_key_provider.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/security/vault_signing_provider.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/security/vcc_pki_client.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/security/vram_secure_clear.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/security/zero_trust_policy_enforcer.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/admin_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/api_auth_config.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/api_gateway.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/api_key_mgmt_handler.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/api_security_audit.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/api_version.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/api_version_config.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/async_job_api_handler.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/audit_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/auth_middleware.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/auth_scope_mapper.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/bpmn_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/branch_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/buffer_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/buffer_binary_protocol.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/cache_admin_api_handler.h` (v0.0.6)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/cache_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/cdn_cache_middleware.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/changefeed_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/chunked_response_writer.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/classification_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/compliance_reporting_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/content_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/diff_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/distributed_txn_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/entity_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/error_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/ethics_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/export_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/feedback_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/geo_topology_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/graph_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/graphql_api_handler.h` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/grpc_web_proxy_handler.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 180: `mutable std::shared_ptr<void> stub_holder_;    ///< opaque grpc::GenericStub`

---

### `include/server/health_error_service.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/hot_reload_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/http2_session.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/http3_datagram.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/http3_session.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/http_server.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/http_type_adapter.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 21: `* TODO: Remove this adapter after full migration to cpp-httplib is complete`

**🔒 HARDCODED** (2 occurrences):
  - Line 15: `* @brief Temporary adapter to bridge Boost.Beast and cpp-httplib types`
  - Line 30: `*          It should only be used as a temporary solution.`

---

### `include/server/import_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/import_wizard_builder.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/index_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/keys_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/llm_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/llm_grpc_service.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/load_shedder.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/lora_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/mcp_server.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/merge_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/monitoring_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/mqtt_session.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/mvcc_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/opa_adapter.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/openapi_route_registry.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/pii_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/pitr_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/pitr_grpc_service.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/pki_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/policy_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/policy_engine.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/policy_manager_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/policy_template_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/policy_validation_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/policy_versioning_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/postgres_session.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/profiling_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/prompt_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/prompt_engineering_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/prompt_engineering_grpc_service.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (91.0/100)

**Issues Found:**

**🔴 STUB** (3 occurrences):
  - Line 12: `// This service is a stub until the proto definition is generated`
  - Line 33: `* @brief gRPC service for prompt engineering operations (STUB - Proto not generated)`
  - Line 56: `// Stub service - full implementation available once proto is generated`

---

### `include/server/query_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/ranger_adapter.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/rate_limiter.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/rate_limiter_v2.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/rate_limiting_middleware.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/replication_topology_api_handler.h` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/reports_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/request_validation_middleware.h` (v0.0.20)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/retention_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/review_scheduling_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/rope_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/rpc/blob_transfer_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/rpc/differential_update_engine.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/rpc/snapshot_transfer_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/rpc_service_impl.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/saga_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/schema_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/serverless_function_api_handler.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/session_api_handler.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/sharding_metrics_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/snapshot_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/spatial_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/sse_connection_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/task_scheduler_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/tenant_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/themis_core_grpc_service.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/timeseries_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/transaction_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/udf_api_handler.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/update_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/vector_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/voice_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/wal_api_handler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/server/wal_grpc_service.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 12: `// gRPC WAL Apply service wrapper; returns nullptr if gRPC stubs are unavailable`

---

### `include/server/websocket_session.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/adaptive_shard_router.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/admin_api.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/admin_operations.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/auto_rebalancer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/auto_recovery_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 108: `* instead of returning false (the old stub behaviour).`

---

### `include/sharding/backpressure_protocol.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/capability_matcher.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/circuit_breaker.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/cloud_agent.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/cloud_backup.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/consensus_factory.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/consensus_module.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/consistent_hash.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/cross_shard_transaction.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/data_migrator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/distributed_coordinator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/distributed_time_coordinator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 23: `* - Production-ready (no NTP stub implementation)`

**🎭 SIMULATION** (1 occurrences):
  - Line 24: `* - Easily testable (mock consensus module)`

---

### `include/sharding/distributed_transaction.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/gossip_config_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/gossip_consensus_adapter.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/gossip_protocol.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/gpu_erasure_coder.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/health_check.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/health_monitor.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 187: `// TODO(v2.0): Remove monitor_thread_ once all clients migrate to ThreadPoolManager`

---

### `include/sharding/hot_spare_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/locality_aware_router.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/metadata_shard.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/metadata_snapshot.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/metadata_wal.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/metrics_registry.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/mtls_client.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/mtls_connection_pool.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/multi_primary_coordinator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/operational_metrics.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/orphan_detector.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/partition_detector.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/paxos_consensus.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/paxos_snapshot.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/paxos_wal.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/pki_shard_certificate.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/predictive_detector.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/prometheus_metrics.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/quorum_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/raft_configuration.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/raft_consensus.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/raft_consensus_adapter.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/raft_log.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/raft_shard_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/raft_state.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/raft_wal_integration.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/raid_optimizations.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/rebalance_operation.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/redundancy_strategy.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/remote_executor.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/replica_consistency.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/replica_topology.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/replication_coordinator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/secure_transport_client.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/shard_capabilities.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/shard_durability.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/shard_load_detector.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/shard_repair_engine.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/shard_resource_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/shard_router.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/shard_rpc_client.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 128: `* @brief Send request using in-process simulation (for single-node)`

---

### `include/sharding/shard_rpc_client_adapter.h` (v0.0.20)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/shard_rpc_server.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/shard_topology.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/signed_request.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/slo_monitor.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/stream_protocol.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/transaction_snapshot.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/transaction_wal.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/truetime.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/two_phase_commit_coordinator.h` (v0.0.20)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/two_phase_commit_participant.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/urn.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/urn_resolver.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/wal_applier.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/wal_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/wal_shipper.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/sharding/write_concern.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/storage/backup_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (85.0/100)

**Issues Found:**

**🔴 STUB** (6 occurrences):
  - Line 336: `// GAP-008: Cloud Backup & Snapshot Scheduling (Stub/Placeholder)`
  - Line 340: `* Schedule automatic backup (stub for future implementation)`
  - Line 352: `* Cancel scheduled backup (stub for future implementation)`
  - Line 359: `* List all scheduled backups (stub for future implementation)`
  - Line 365: `* Perform cloud backup to S3/Azure/GCS (stub for future implementation)`

---

### `include/storage/base_entity.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/storage/batch_write_optimizer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/storage/blob_backend_filesystem.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/storage/blob_redundancy_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/storage/blob_storage_backend.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/storage/blob_storage_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/storage/columnar_format.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/storage/compaction_manager.h` (v0.0.32)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/storage/compressed_storage.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/storage/compression_strategy.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/storage/database_connection_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/storage/disk_space_monitor.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/storage/history_manager.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/storage/hlc.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/storage/index_maintenance.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/storage/key_schema.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/storage/merge_operators.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/storage/mvcc_store.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/storage/nlp_metadata_extractor.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/storage/pitr_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/storage/raft_mvcc_bridge.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/storage/rocksdb_wrapper.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (2 occurrences):
  - Line 47: `///   - Debug mode (THEMIS_DEBUG_THREADING) will detect and log concurrent move operations`
  - Line 613: `// Track if object is being moved (debug only)`

---

### `include/storage/security_signature.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/storage/security_signature_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/storage/storage_audit_logger.h` (v0.0.32)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/storage/storage_engine.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/storage/transaction_retry_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 37: `RESOURCE_EXHAUSTED,  // Temporary resource shortage`

---

### `include/storage/wal_storage.h` (v0.0.32)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/temporal/bi_temporal.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/temporal/retention_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/temporal/snapshot_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/temporal/system_versioned_table.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/temporal/temporal_aggregator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/temporal/temporal_conflict_resolver.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/temporal/temporal_index.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/temporal/temporal_query_engine.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/temporal/temporal_types.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/base/ab_test_manager.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/base/export.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/base/hot_reload_manager.h` (v0.0.7)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/base/interfaces/index_interface.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 12: `/// - Enable isolated unit testing with mock implementations`

---

### `include/themis/base/interfaces/query_interface.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/base/interfaces/security_interface.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/base/interfaces/storage_interface.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/base/module_loader.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 132: `TRANSIENT,      // Temporary failure, retry may succeed`

---

### `include/themis/base/module_sandbox.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/base/plugin_dependency_graph.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/base/remote_registry_client.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/base/wasm_plugin_sandbox.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/build_info.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/edition.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/edition_manager.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/export.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/gpu/admin_api.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 17: `* Provides three read-only and one simulation endpoint:`
  - Line 22: `* POST /admin/gpu/simulate — Dry-run allocation check using GPUConfig rules.`
  - Line 77: `* @brief Dry-run simulation: would @p bytes be accepted right now?`

---

### `include/themis/gpu/alerts.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/gpu/audit_log.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/gpu/cluster_config.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/gpu/cluster_coordinator.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/gpu/cluster_topology.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/gpu/config.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 19: `* Dry-run simulation: GPUConfig::simulateAllocation() lets operators test`
  - Line 100: `* @brief Simulate whether an allocation of @p bytes would be accepted`

---

### `include/themis/gpu/device_discovery.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 48: `* - Real CUDA/ROCm integration is a future TODO guarded by`

---

### `include/themis/gpu/feature_flags.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/gpu/gpu_module.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/gpu/graph_cache.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 57: `// cudaGraphExec_t.  In this CPU-simulation build it holds only the`

---

### `include/themis/gpu/kernel_validator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/gpu/launcher.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/gpu/load_balancer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/gpu/memory_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/gpu/memory_pool.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 106: `* reading another tenant's data.  In the bookkeeping-only simulation the`
  - Line 169: `* the updated logical offsets.  In the CPU bookkeeping simulation (no`

---

### `include/themis/gpu/metrics.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/gpu/mig_manager.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 41: `* In the current CPU-simulation build all operations are performed against an`

---

### `include/themis/gpu/p2p_transfer.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (5 occurrences):
  - Line 22: `* CPU simulation so that the full API can be exercised without real GPU`
  - Line 40: `* succeeds via an in-memory `memcpy` simulation so tests always pass.`
  - Line 95: `size_t cpu_fallback_transfers  = 0; ///< Transfers via CPU simulation (no HW P2P)`
  - Line 143: `* pair.  On the CPU simulation path this always returns`
  - Line 185: `*     succeeds via a host-side `memcpy` simulation so that tests can`

---

### `include/themis/gpu/policy.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/gpu/profiler.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/gpu/query_accelerator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 32: `*                   (GPU stub: cuVS/RAFT `ivf_flat` on CUDA; CPU brute-force fallback)`
  - Line 241: `* `Config::gpu_threshold_rows`) — stub for production cuVS/RAFT wiring:`

---

### `include/themis/gpu/rocm_backend.h` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/gpu/safe_fail.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/gpu/stream_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 128: `* This overload resolves the "Stubs: 1" noted in the stream_manager header`

---

### `include/themis/gpu/tensor_buffer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/gpu/time_slice_scheduler.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/gpu/training_loop.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/gpu/unified_memory.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/gpu/vulkan_backend.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 56: `* In the current CPU-simulation build this field stores`

---

### `include/themis/gpu/wasm_kernel_sandbox.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 51: `* 0 = no limit enforced in CPU simulation path.`
  - Line 160: `* Always returns false in the current build (CPU simulation path).`
  - Line 178: `*  5. Execute in the WASM sandbox (or CPU simulation fallback).`
  - Line 221: `// Internal helper: execute the kernel payload under CPU simulation.`

---

### `include/themis/license_info.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/module_hash_verifier.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/module_signature_verifier.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/network/wire_protocol_server.hpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/network/wire_protocol_v2.hpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis/runtime_license_gate.h` (v0.0.27)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/themis_export.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/timeseries/aggregate_scheduler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/timeseries/aggregates.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/timeseries/continuous_agg.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/timeseries/gorilla.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/timeseries/hypertable.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/timeseries/prometheus_remote_write.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/timeseries/query_optimizer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/timeseries/retention.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/timeseries/timeseries.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/timeseries/timeseries_metrics.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/timeseries/ts_auto_buffer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/timeseries/tsstore.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/training/auto_labeler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/training/incremental_lora_trainer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/training/knowledge_graph_enricher.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/training/lora_data_selection.h` (v0.0.25)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/training/training_pipeline.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/transaction/branch_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/transaction/crash_recovery_manager.h` (v0.0.31)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/transaction/distributed_saga.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/transaction/global_transaction_manager.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 57: `* stub (for real deployments).`

---

### `include/transaction/isolation_level.h` (v0.0.31)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/transaction/lock_manager.h` (v0.0.31)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/transaction/merge_engine.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/transaction/saga.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/transaction/snapshot_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/transaction/transaction_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/updates/blue_green_deployment.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/updates/canary_rollout.h` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/updates/coordinated_update_manager.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/updates/delta_update_engine.h` (v0.0.29)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/updates/hot_reload_engine.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/updates/in_place_schema_migrator.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/updates/manifest_database.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/updates/notification_webhook.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/updates/release_manifest.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/updates/schema_migration_tester.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/updates/update_history_logger.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/updates/update_state_machine.h` (v0.0.31)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/updates/updates_config.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/audit_logger.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/batch_operation_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/capability_auto_generator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/checksum_utils.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/clock.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/compression_metrics.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/concurrent_cache.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/cron_parser.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/cursor.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/error_registry.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/expected.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/file_utils.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/geo/ewkb.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/geo/validator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/grpc_channel_pool.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/hkdf_cache.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/hkdf_helper.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/http_client_pool.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/input_validator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 51: `// Helper to load a stub schema from schema_dir_/name.json`

---

### `include/utils/lek_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/logger.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🐛 DEBUG** (3 occurrences):
  - Line 68: `enum class Level { TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL };`
  - Line 96: `static void debug(FormatString&& fmt, Args&&... args);`
  - Line 126: `#define THEMIS_DEBUG(...) ::themis::utils::Logger::debug(__VA_ARGS__)`

---

### `include/utils/logger_impl.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🐛 DEBUG** (3 occurrences):
  - Line 19: `void Logger::debug(FormatString&& fmt, Args&&... args) {`
  - Line 20: `if (logger_ && logger_->should_log(spdlog::level::debug)) {`
  - Line 21: `logger_->debug(fmt::runtime(std::forward<FormatString>(fmt)), std::forward<Args>(args)...);`

---

### `include/utils/lossless_vector_compression.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/lossless_vector_integration.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/memory/pool_allocator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (3 occurrences):
  - Line 7: `// - Stack Allocator: LIFO allocation for temporary objects`
  - Line 231: `* - Best for: Temporary allocations in scope`
  - Line 279: `* - Temporary allocations -> Stack`

---

### `include/utils/memory_utils.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/ner_detection_engine.h` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/normalizer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/openssl_deleter.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/pii_detection_engine.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/pii_detector.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/pii_pseudonymizer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/pii_redacting_sink.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/pki_client.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 33: `// Minimal PKI client (stub) to sign/verify data hashes.`

---

### `include/utils/pointer_utils.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 162: `spdlog::debug("weak_ptr lock failed: {}", message);`

---

### `include/utils/regex_detection_engine.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/retention_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/safe_access.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/safe_arithmetic.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/safe_cast.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/saga_logger.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/self_awareness.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/serialization.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/simd_distance.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/stemmer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/stopwords.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/string_utils.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/thread_pool_manager.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/thread_safety.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/tracing.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/type_conversion.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (6 occurrences):
  - Line 126: `spdlog::debug("Type conversion: precision loss converting {} to float", value);`
  - Line 214: `spdlog::debug("Type conversion: clamping size_t {} to int32_t max", value);`
  - Line 227: `spdlog::debug("Type conversion: clamping double {} to float max", value);`
  - Line 231: `spdlog::debug("Type conversion: clamping double {} to float min", value);`
  - Line 244: `spdlog::debug("Type conversion: clamping int64_t {} to int32_t max", value);`

---

### `include/utils/unaligned_access.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/update_checker.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/utils/zstd_codec.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/voice/audio_preprocessing.h` (v0.0.28)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/voice/emotion_analyzer.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/voice/voice_accessibility.h` (v0.0.28)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/voice/voice_assistant.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/voice/voice_audio_storage.h` (v0.0.28)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/voice/voice_auth.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/voice/voice_batch_processor.h` (v0.0.28)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 123: `// Load test helper: simulate N concurrent requests`

---

### `include/voice/voice_error_handler.h` (v0.0.28)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/voice/voice_intent_detector.h` (v0.0.28)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/voice/voice_macro.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/voice/voice_meeting_support.h` (v0.0.28)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/voice/voice_model_cache.h` (v0.0.28)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 21: `void* handle = nullptr;   // Opaque pointer to actual model (may be null for stubs)`

---

### `include/voice/voice_security.h` (v0.0.28)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/voice/voice_session_manager.h` (v0.0.28)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/voice/voice_tts_customizer.h` (v0.0.28)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `include/voice/wake_word_detector.h` (v0.0.2)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `plugins/ethics_ai/argument_store.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**📝 TODO** (2 occurrences):
  - Line 61: `// TODO: When vector support is integrated:`
  - Line 146: `// TODO: Use AQL query when query_engine_ is available:`

---

### `plugins/ethics_ai/argument_store.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `plugins/ethics_ai/discourse_engine.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `plugins/ethics_ai/discourse_engine.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `plugins/ethics_ai/ethics_ai_plugin.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `plugins/ethics_ai/ethics_ai_types.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `plugins/ethics_ai/ethics_aql_queries.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `plugins/ethics_ai/ethics_base_entity_adapter.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `plugins/ethics_ai/ethics_evaluator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `plugins/ethics_ai/ethics_evaluator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `plugins/ethics_ai/examples/example_basic_usage.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `plugins/ethics_ai/philosophy_loader.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `plugins/ethics_ai/philosophy_loader.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `plugins/ethics_ai/rag_context_engine.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (83.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 110: `// For now, return dummy embedding`

**📝 TODO** (7 occurrences):
  - Line 44: `// TODO: Implement remaining patterns (4-7) when vector/timeline storage is available`
  - Line 54: `// TODO: Implement actual textual similarity search using ThemisDB's text search`
  - Line 66: `// TODO: Implement AQL query for best practices`
  - Line 79: `// TODO: Implement vector search using ThemisDB's vector index`
  - Line 90: `// TODO: Implement graph traversal using ThemisDB's graph manager`

---

### `plugins/ethics_ai/rag_context_engine.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `plugins/image_analysis/onnx_clip/onnx_clip_plugin.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `plugins/rpc/grpc/grpc_plugin.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `plugins/rpc/grpc/grpc_plugin.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `plugins/user_storage_encrypted/include/encryption_backend_interface.hpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `plugins/user_storage_encrypted/include/gocryptfs_backend.hpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `plugins/user_storage_encrypted/include/key_rotation_scheduler.hpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `plugins/user_storage_encrypted/include/multi_level_storage.hpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 28: `std::string key_provider;      // "vault", "hsm", "mock"`

---

### `plugins/user_storage_encrypted/include/security_level.hpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `plugins/user_storage_encrypted/include/user_models.hpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `plugins/user_storage_encrypted/src/gocryptfs_backend.cpp` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (76.0/100)

**Issues Found:**

**🔒 HARDCODED** (6 occurrences):
  - Line 87: `// Create secure temporary password file`
  - Line 125: `// Create secure temporary password file`
  - Line 198: `// Create secure temporary file outside encrypted directory`
  - Line 202: `return Result<void>::error("Failed to create secure temporary file");`
  - Line 222: `return Result<void>::error("Failed to write key to temporary file");`

---

### `plugins/user_storage_encrypted/src/key_rotation_scheduler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `plugins/user_storage_encrypted/src/multi_level_storage.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (89.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 310: `} else if (config.key_provider == "mock") {`

**📝 TODO** (4 occurrences):
  - Line 356: `// TODO: Key rotation implementation`
  - Line 364: `return Result<void>::error("Key rotation not yet fully implemented - see TODO in code");`
  - Line 553: `// TODO: Implement directory listing for users`
  - Line 595: `// TODO: Implement directory listing for groups`

---

### `plugins/user_storage_encrypted/tests/test_multi_level_storage.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 197: `// Mock test for gocryptfs backend (requires gocryptfs to be installed)`

**🔒 HARDCODED** (1 occurrences):
  - Line 13: `// Create temporary test directory`

---

### `projects/RailwayMonitor.WPF/App.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 28: `.MinimumLevel.Debug()`

---

### `projects/RailwayMonitor.WPF/Converters/Converters.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 56: `throw new NotImplementedException();`

---

### `projects/RailwayMonitor.WPF/MainWindow.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/RailwayMonitor.WPF/Models/Models.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/RailwayMonitor.WPF/Services/Cost/MLCostEstimator.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/RailwayMonitor.WPF/Services/Cost/RailwayCostDatabase.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/RailwayMonitor.WPF/Services/Map/MapTypes.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/RailwayMonitor.WPF/Services/Map/RailwayMapRenderer.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 250: `System.Diagnostics.Debug.WriteLine($"Render error: {ex.Message}");`

---

### `projects/RailwayMonitor.WPF/Services/Map/RenderingBackend.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 140: `System.Diagnostics.Debug.WriteLine("Vulkan backend initialized (stub)");`

**🐛 DEBUG** (5 occurrences):
  - Line 51: `System.Diagnostics.Debug.WriteLine("DirectX backend initialized");`
  - Line 63: `System.Diagnostics.Debug.WriteLine($"DirectX rendered {_operations.Count} operations");`
  - Line 140: `System.Diagnostics.Debug.WriteLine("Vulkan backend initialized (stub)");`
  - Line 167: `System.Diagnostics.Debug.WriteLine("Software backend initialized");`
  - Line 178: `System.Diagnostics.Debug.WriteLine($"Software rendered {_operations.Count} operations");`

---

### `projects/RailwayMonitor.WPF/Services/Network/GTFSImporter.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/RailwayMonitor.WPF/Services/Network/MultiCriteriaOptimizer.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/RailwayMonitor.WPF/Services/Network/NetworkBottleneckAnalyzer.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/RailwayMonitor.WPF/Services/Network/RailwayNetworkAnalyzer.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/RailwayMonitor.WPF/Services/Optimization/CrossingAnalyzer.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/RailwayMonitor.WPF/Services/Optimization/GeoSpatialAnalyzer.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🎭 SIMULATION** (6 occurrences):
  - Line 544: `// Mock geocoding - in Produktion: Nominatim oder Google Maps API`
  - Line 706: `/// Mock Terrain Provider (in Produktion: SRTM, ASTER GDEM, etc.)`
  - Line 735: `// Mock: Alpen im Süden, flach im Norden`
  - Line 744: `// Mock: Einige Schutzgebiete`
  - Line 751: `// Mock: Großstädte haben hohe Dichte`

---

### `projects/RailwayMonitor.WPF/Services/Optimization/SettlementAnalyzer.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/RailwayMonitor.WPF/Services/RealData/RealDataProvider.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 148: `// Mock basierend auf Deutschland-Topographie`

---

### `projects/RailwayMonitor.WPF/Services/Rendering/DesignTableManager.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/RailwayMonitor.WPF/Services/Rendering/FeatureTreeManager.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/RailwayMonitor.WPF/Services/Rendering/GridAndGuidesRenderer.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (2 occurrences):
  - Line 246: `// TODO: Lat/Lon Linien (erfordert Koordinaten-Transformation)`
  - Line 254: `// TODO: UTM Zone Grid (100km × 100km Quadrate)`

---

### `projects/RailwayMonitor.WPF/Services/Rendering/Layer3DManager.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/RailwayMonitor.WPF/Services/Rendering/Object3DRenderer.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 539: `// Simplified stub implementations`

---

### `projects/RailwayMonitor.WPF/Services/Rendering/Rendering3DServiceStub.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (84.0/100)

**Issues Found:**

**🔴 STUB** (3 occurrences):
  - Line 8: `/// 3D Rendering Service - Phase 5.3 Implementation Stub`
  - Line 49: `Console.WriteLine("[3D Rendering] Initialized (stub mode)");`
  - Line 55: `// For now, this is a stub that logs rendering calls`

**📝 TODO** (2 occurrences):
  - Line 47: `// TODO: Initialize DirectX/Vulkan rendering context`
  - Line 54: `// TODO: Actual DirectX rendering implementation`

---

### `projects/RailwayMonitor.WPF/Services/Rendering/TerrainRenderer3D.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 94: `// Mock: Generiere realistische Heightmap für Deutschland`
  - Line 258: `// Mock für Entwicklung`
  - Line 266: `// Mock: Generiere typische OSM-Features für Deutschland`

---

### `projects/RailwayMonitor.WPF/Services/Services.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 382: `/// Service for train simulation control`

**🐛 DEBUG** (7 occurrences):
  - Line 122: `System.Diagnostics.Debug.WriteLine($"Error fetching trains: {ex.Message}");`
  - Line 178: `System.Diagnostics.Debug.WriteLine($"Error fetching stations: {ex.Message}");`
  - Line 258: `System.Diagnostics.Debug.WriteLine($"Error fetching train {trainNumber}: {ex.Message}");`
  - Line 311: `System.Diagnostics.Debug.WriteLine($"Error fetching delayed trains: {ex.Message}");`
  - Line 346: `System.Diagnostics.Debug.WriteLine($"Error fetching trains by type: {ex.Message}");`

---

### `projects/RailwayMonitor.WPF/Services/Signaling/ConflictDetector.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/RailwayMonitor.WPF/Services/Signaling/ETCSLevel2Simulator.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 270: `// Safety limit: max 1 hour simulation`
  - Line 405: `return $"Simulation Result:\n" +`

---

### `projects/RailwayMonitor.WPF/Services/Signaling/SignalPlacementOptimizer.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/RailwayMonitor.WPF/Services/Signaling/UIC406CapacityCalculator.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/RailwayMonitor.WPF/Services/Station/StationLayoutOptimizer.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 56: `// Step 6: Simulate passenger flows`
  - Line 311: `// Peak hour simulation`

---

### `projects/RailwayMonitor.WPF/Services/Station/TrackAssignmentOptimizer.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/RailwayMonitor.WPF/Services/Timetabling/PESPTimetableOptimizer.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/RailwayMonitor.WPF/Services/Timetabling/RobustnessAnalyzer.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 8: `/// Robustness analyzer for timetables using Monte Carlo simulation`
  - Line 21: `/// Analyzes timetable robustness using Monte Carlo simulation`
  - Line 73: `// Simulate delay propagation through network`

---

### `projects/RailwayMonitor.WPF/Services/Timetabling/RollingStockScheduler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/RailwayMonitor.WPF/Services/Timetabling/SATScheduler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/RailwayMonitor.WPF/Services/VectorData/VectorDataServiceStub.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (89.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 10: `/// Vector Data Service - Phase 5.4 Implementation Stub`
  - Line 98: `Console.WriteLine("[Vector Data] Initialized (stub mode)");`

**📝 TODO** (2 occurrences):
  - Line 96: `// TODO: Connect to vector tile server or PostGIS`
  - Line 120: `// TODO: Fetch MVT tiles from server`

---

### `projects/RailwayMonitor.WPF/Services/Visualization/Railway3DRenderingEngine.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/RailwayMonitor.WPF/Services/Visualization/RailwayControlVisualizer.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (88.0/100)

**Issues Found:**

**🎭 SIMULATION** (5 occurrences):
  - Line 9: `/// Provides DB-standard visualizations for train paths, switches, signals, and scenario simulation`
  - Line 204: `/// What-If Analysis Engine for scenario simulation`
  - Line 223: `// Simulate direct impacts`
  - Line 226: `// Simulate cascading delays if enabled`
  - Line 290: `public ScenarioResult Simulate(Scenario scenario)`

---

### `projects/RailwayMonitor.WPF/ViewModels/ComponentViewModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/RailwayMonitor.WPF/ViewModels/MainViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 562: `await Task.Delay(REFRESH_SIMULATION_DELAY_MS); // Simulate refresh`

---

### `projects/RailwayMonitor.WPF/src/Services/Data/DataSeedService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 357: `/// Generiere Bodenrichtwerte (BORIS Mock)`

**📝 TODO** (1 occurrences):
  - Line 285: `// TODO: Proper OSM JSON parsing`

**🐛 DEBUG** (2 occurrences):
  - Line 281: `Log.Debug("OSM Query Result: {@Json}", json);`
  - Line 322: `Log.Debug("Elevation at ({Lat:F2}, {Lon:F2}): {Elev}m", lat, lon, elevation);`

---

### `projects/RailwayMonitor.WPF.Tests/Services/ChangeFeedServiceTests.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/RailwayMonitor.WPF.Tests/Services/DataPipelineServiceTests.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (94.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 13: `private readonly Mock<IIntelligentCacheService> _mockCache;`
  - Line 17: `_mockCache = new Mock<IIntelligentCacheService>();`

---

### `projects/RailwayMonitor.WPF.Tests/Services/DelayPredictionServiceTests.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/RailwayMonitor.WPF.Tests/Services/EnergyManagementServiceTests.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/RailwayMonitor.WPF.Tests/Services/EnergyOptimizationMLServiceTests.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/RailwayMonitor.WPF.Tests/Services/GeoSpatialAnalyzerTests.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/RailwayMonitor.WPF.Tests/Services/GridAndGuidesRendererTests.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/RailwayMonitor.WPF.Tests/Services/IntelligentCacheServiceTests.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/RailwayMonitor.WPF.Tests/Services/MapServiceTests.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/RailwayMonitor.WPF.Tests/Services/PredictiveMaintenanceServiceTests.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/RailwayMonitor.WPF.Tests/Services/Rendering3DServiceTests.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 98: `_service.UpdateFrame(); // Simulate frame`

---

### `projects/RailwayMonitor.WPF.Tests/Services/ThemisDbServiceTests.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 13: `private readonly Mock<HttpMessageHandler> _mockHttpHandler;`
  - Line 18: `_mockHttpHandler = new Mock<HttpMessageHandler>();`

---

### `projects/RailwayMonitor.WPF.Tests/Services/VectorDataServiceTests.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/RailwayMonitor.WPF.Tests/ViewModels/MainViewModelTests.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (89.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 13: `private readonly Mock<IThemisDbService> _mockDbService;`
  - Line 14: `private readonly Mock<IEnergyManagementService> _mockEnergyService;`
  - Line 19: `_mockDbService = new Mock<IThemisDbService>();`
  - Line 20: `_mockEnergyService = new Mock<IEnergyManagementService>();`

---

### `projects/Themis.AdminTools.Shared/ApiClient/Endpoints/AuditLogEndpoint.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.AdminTools.Shared/ApiClient/Endpoints/ClassificationEndpoint.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.AdminTools.Shared/ApiClient/Endpoints/KeysEndpoint.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.AdminTools.Shared/ApiClient/Endpoints/PiiEndpoint.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.AdminTools.Shared/ApiClient/Endpoints/ReportsEndpoint.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.AdminTools.Shared/ApiClient/Endpoints/RetentionEndpoint.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.AdminTools.Shared/ApiClient/Endpoints/SagaEndpoint.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.AdminTools.Shared/ApiClient/MockThemisApiClient.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 6: `/// Mock-Implementation des API-Clients für Tests ohne laufenden Server.`

---

### `projects/Themis.AdminTools.Shared/ApiClient/ThemisApiClient.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.AdminTools.Shared/Models/AuditLogModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.AdminTools.Shared/Models/ClassificationModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.AdminTools.Shared/Models/Common.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.AdminTools.Shared/Models/KeysModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.AdminTools.Shared/Models/PiiModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.AdminTools.Shared/Models/ReportsModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.AdminTools.Shared/Models/RetentionModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.AdminTools.Shared/Models/SAGABatchDetail.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.AdminTools.Shared/Models/SAGABatchInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.AdminTools.Shared/Models/SAGABatchListResponse.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.AdminTools.Shared/Models/SAGAVerificationResult.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/App.xaml.cs` (v0.0.33)

**Maturity Level:** 🔴 ALPHA (34.0/100)

**Issues Found:**

**🔴 STUB** (12 occurrences):
  - Line 288: `// DISABLED: Stubs only, depends on complex entity model`
  - Line 297: `// DISABLED: Stubs only, not essential for basic DSM`
  - Line 305: `// DISABLED: Stubs only`
  - Line 313: `// DISABLED: Stubs only`
  - Line 319: `// DISABLED: Stubs only`

**🐛 DEBUG** (9 occurrences):
  - Line 76: `System.Diagnostics.Debug.WriteLine(msg);`
  - Line 112: `System.Diagnostics.Debug.WriteLine(msg);`
  - Line 125: `System.Diagnostics.Debug.WriteLine($"STARTUP ERROR:\n{ex}");`
  - Line 187: `System.Diagnostics.Debug.WriteLine($"DSM Sync übersprungen: {syncEx.Message}");`
  - Line 202: `System.Diagnostics.Debug.WriteLine($"Fehler bei InitializeApplicationAsync: {ex.Message}");`

---

### `projects/Themis.DocumentManager/Application/Classification/Commands/ClassificationCommands.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Classification/Commands/CreateClassification/CreateClassificationCommand.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Classification/Commands/CreateClassification/CreateClassificationCommandHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (92.0/100)

**Issues Found:**

**🔒 HARDCODED** (2 occurrences):
  - Line 10: `// Temporary in-memory storage (replace with repository)`
  - Line 64: `// Temporary model (should be in Models namespace)`

---

### `projects/Themis.DocumentManager/Application/Classification/Commands/CreateClassification/CreateClassificationCommandValidator.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Classification/Commands/DeleteClassification/DeleteClassificationCommand.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Classification/Commands/DeleteClassification/DeleteClassificationCommandHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Classification/Commands/UpdateClassification/UpdateClassificationCommand.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Classification/Commands/UpdateClassification/UpdateClassificationCommandHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Classification/Commands/UpdateClassification/UpdateClassificationCommandValidator.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Classification/Handlers/ClassificationHandlers.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Classification/Messages/ClassificationDto.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Classification/Queries/ClassificationQueries.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Classification/Queries/GetAllClassifications/GetAllClassificationsQuery.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Classification/Queries/GetAllClassifications/GetAllClassificationsQueryHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Classification/Queries/GetClassificationById/GetClassificationByIdQuery.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Classification/Queries/GetClassificationById/GetClassificationByIdQueryHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Collaboration/Commands/CollaborationCommands.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Collaboration/Commands/CreateCollaboration/CreateCollaborationCommand.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Collaboration/Commands/CreateCollaboration/CreateCollaborationCommandHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Collaboration/Commands/CreateCollaboration/CreateCollaborationCommandValidator.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Collaboration/Commands/DeleteCollaboration/DeleteCollaborationCommand.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Collaboration/Commands/DeleteCollaboration/DeleteCollaborationCommandHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Collaboration/Commands/UpdateCollaboration/UpdateCollaborationCommand.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Collaboration/Commands/UpdateCollaboration/UpdateCollaborationCommandHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Collaboration/Commands/UpdateCollaboration/UpdateCollaborationCommandValidator.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Collaboration/Handlers/CollaborationCommandHandlers.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Collaboration/Messages/CollaborationDto.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Collaboration/Queries/CollaborationQueries.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Collaboration/Queries/GetAllCollaborations/GetAllCollaborationsQuery.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Collaboration/Queries/GetAllCollaborations/GetAllCollaborationsQueryHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Collaboration/Queries/GetCollaborationById/GetCollaborationByIdQuery.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Collaboration/Queries/GetCollaborationById/GetCollaborationByIdQueryHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Common/Commands/ICreateCommand.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Common/Commands/IDeleteCommand.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Common/Commands/IUpdateCommand.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Common/Interfaces/IThemisRepository.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Common/Messages/BaseEntityDto.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Common/Messages/IEntityDto.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Common/Queries/IGetAllQuery.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Common/Queries/IGetByIdQuery.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Common/Result.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Cosigning/Commands/ApproveCosigningStep/ApproveCosigningStepCommand.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Cosigning/Commands/ApproveCosigningStep/ApproveCosigningStepCommandHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Cosigning/Commands/ApproveCosigningStep/ApproveCosigningStepCommandValidator.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Cosigning/Commands/CreateCosigning/CreateCosigningCommand.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Cosigning/Commands/CreateCosigning/CreateCosigningCommandHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Cosigning/Commands/CreateCosigning/CreateCosigningCommandValidator.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Cosigning/Commands/DeleteCosigning/DeleteCosigningCommand.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Cosigning/Commands/DeleteCosigning/DeleteCosigningCommandHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Cosigning/Commands/UpdateCosigning/UpdateCosigningCommand.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Cosigning/Commands/UpdateCosigning/UpdateCosigningCommandHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Cosigning/Commands/UpdateCosigning/UpdateCosigningCommandValidator.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Cosigning/Messages/CosigningDto.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Cosigning/Queries/GetAllCosignings/GetAllCosigningsQuery.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Cosigning/Queries/GetAllCosignings/GetAllCosigningsQueryHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Cosigning/Queries/GetCosigningById/GetCosigningByIdQuery.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Cosigning/Queries/GetCosigningById/GetCosigningByIdQueryHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Documents/Commands/CreateDocument/CreateDocumentCommand.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Documents/Commands/CreateDocument/CreateDocumentCommandHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Documents/Commands/CreateDocument/CreateDocumentCommandValidator.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Documents/Commands/DeleteDocument/DeleteDocumentCommand.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Documents/Commands/DeleteDocument/DeleteDocumentCommandHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Documents/Commands/DeleteDocument/DeleteDocumentCommandValidator.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Documents/Commands/UpdateDocument/UpdateDocumentCommand.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Documents/Commands/UpdateDocument/UpdateDocumentCommandHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Documents/Commands/UpdateDocument/UpdateDocumentCommandValidator.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Documents/EventHandlers/DocumentsRefreshRequestedEventHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Documents/EventHandlers/DocumentsRefreshUIHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Documents/EventHandlers/TestDataGeneratedEventHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Documents/Messages/DocumentDto.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Documents/Queries/GetAllDocuments/GetAllDocumentsQuery.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Documents/Queries/GetAllDocuments/GetAllDocumentsQueryHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Documents/Queries/GetAllDocuments/GetAllDocumentsQueryValidator.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Documents/Queries/GetDocument/GetDocumentQuery.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Documents/Queries/GetDocument/GetDocumentQueryHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Documents/Queries/GetDocumentById/GetDocumentByIdQuery.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Documents/Queries/GetDocumentById/GetDocumentByIdQueryHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Documents/Queries/GetDocumentById/GetDocumentByIdQueryValidator.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Documents/Queries/GetDocuments/GetDocumentsQuery.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Documents/Queries/GetDocuments/GetDocumentsQueryHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Favorites/Commands/AddToFavorites/AddToFavoritesCommand.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Favorites/Commands/AddToFavorites/AddToFavoritesCommandHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Favorites/Commands/AddToFavorites/AddToFavoritesCommandValidator.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Favorites/Commands/CreateFavorite/CreateFavoriteCommand.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Favorites/Commands/CreateFavorite/CreateFavoriteCommandHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Favorites/Commands/CreateFavorite/CreateFavoriteCommandValidator.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Favorites/Commands/DeleteFavorite/DeleteFavoriteCommand.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Favorites/Commands/DeleteFavorite/DeleteFavoriteCommandHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Favorites/Commands/RemoveFromFavorites/RemoveFromFavoritesCommand.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Favorites/Commands/RemoveFromFavorites/RemoveFromFavoritesCommandHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Favorites/Messages/FavoriteDto.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Favorites/Queries/GetFavorites/GetFavoritesQuery.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Favorites/Queries/GetFavorites/GetFavoritesQueryHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Favorites/Queries/GetUserFavorites/GetUserFavoritesQuery.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Favorites/Queries/GetUserFavorites/GetUserFavoritesQueryHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Favorites/Queries/IsFavorite/IsFavoriteQuery.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Favorites/Queries/IsFavorite/IsFavoriteQueryHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Inbox/Commands/CreateInboxItem/CreateInboxItemCommand.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Inbox/Commands/CreateInboxItem/CreateInboxItemCommandHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Inbox/Commands/CreateInboxItem/CreateInboxItemCommandValidator.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Inbox/Commands/CreateInboxItemV2/CreateInboxItemV2Command.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Inbox/Commands/CreateInboxItemV2/CreateInboxItemV2CommandHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Inbox/Commands/CreateInboxItemV2/CreateInboxItemV2CommandValidator.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Inbox/Commands/DeleteInboxItem/DeleteInboxItemCommand.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Inbox/Commands/DeleteInboxItem/DeleteInboxItemCommandHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Inbox/Commands/UpdateInboxItem/UpdateInboxItemCommand.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Inbox/Commands/UpdateInboxItem/UpdateInboxItemCommandHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Inbox/Commands/UpdateInboxItem/UpdateInboxItemCommandValidator.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Inbox/Messages/InboxItemDto.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Inbox/Queries/GetAllInboxItems/GetAllInboxItemsQuery.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Inbox/Queries/GetAllInboxItems/GetAllInboxItemsQueryHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Inbox/Queries/GetInboxItemById/GetInboxItemByIdQuery.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Inbox/Queries/GetInboxItemById/GetInboxItemByIdQueryHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Inbox/Queries/GetInboxItems/GetInboxItemsQuery.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Inbox/Queries/GetInboxItems/GetInboxItemsQueryHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Navigation/Queries/GetNavigationPath/GetNavigationPathQuery.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Navigation/Queries/GetNavigationPath/GetNavigationPathQueryHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Navigation/Queries/GetRelatedEntities/GetRelatedEntitiesQuery.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Navigation/Queries/GetRelatedEntities/GetRelatedEntitiesQueryHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Reminders/Commands/CreateReminder/CreateReminderCommand.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Reminders/Commands/CreateReminder/CreateReminderCommandHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Reminders/Commands/CreateReminder/CreateReminderCommandValidator.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Reminders/Commands/DeleteReminder/DeleteReminderCommand.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Reminders/Commands/DeleteReminder/DeleteReminderCommandHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Reminders/Commands/DeleteReminder/DeleteReminderCommandValidator.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Reminders/Commands/UpdateReminder/UpdateReminderCommand.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Reminders/Commands/UpdateReminder/UpdateReminderCommandHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Reminders/Commands/UpdateReminder/UpdateReminderCommandValidator.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Reminders/Messages/ReminderDto.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Reminders/Queries/GetAllReminders/GetAllRemindersQuery.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Reminders/Queries/GetAllReminders/GetAllRemindersQueryHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Reminders/Queries/GetAllReminders/GetAllRemindersQueryValidator.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Reminders/Queries/GetDueReminders/GetDueRemindersQuery.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Reminders/Queries/GetDueReminders/GetDueRemindersQueryHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Reminders/Queries/GetReminderById/GetReminderByIdQuery.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Reminders/Queries/GetReminderById/GetReminderByIdQueryHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Reminders/Queries/GetReminderById/GetReminderByIdQueryValidator.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Tasks/Commands/CreateTask/CreateTaskCommand.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Tasks/Commands/CreateTask/CreateTaskCommandHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 13: `private static readonly List<OutlookTask> _tasks = new(); // Temporary in-memory storage`

---

### `projects/Themis.DocumentManager/Application/Tasks/Commands/CreateTask/CreateTaskCommandValidator.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Tasks/Commands/DeleteTask/DeleteTaskCommand.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Tasks/Commands/DeleteTask/DeleteTaskCommandHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 10: `// Temporary in-memory storage (replace with repository)`

---

### `projects/Themis.DocumentManager/Application/Tasks/Commands/UpdateTask/UpdateTaskCommand.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Tasks/Commands/UpdateTask/UpdateTaskCommandHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 11: `// Temporary in-memory storage (replace with repository)`

---

### `projects/Themis.DocumentManager/Application/Tasks/Commands/UpdateTask/UpdateTaskCommandValidator.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Tasks/Messages/TaskItemDto.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Tasks/Queries/GetAllTasks/GetAllTasksQuery.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Tasks/Queries/GetAllTasks/GetAllTasksQueryHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Tasks/Queries/GetMyTasks/GetMyTasksQuery.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Tasks/Queries/GetMyTasks/GetMyTasksQueryHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Tasks/Queries/GetTaskById/GetTaskByIdQuery.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Application/Tasks/Queries/GetTaskById/GetTaskByIdQueryHandler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 11: `// Temporary in-memory storage (replace with repository)`

---

### `projects/Themis.DocumentManager/Common/RelayCommand.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Controls/GeoToolbar.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Controls/GraphLegend.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Converters/TimelineConverters.cs` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (78.0/100)

**Issues Found:**

**🔴 STUB** (5 occurrences):
  - Line 35: `throw new NotImplementedException();`
  - Line 57: `throw new NotImplementedException();`
  - Line 74: `throw new NotImplementedException();`
  - Line 102: `throw new NotImplementedException();`
  - Line 119: `throw new NotImplementedException();`

**📝 TODO** (1 occurrences):
  - Line 48: `// TODO: Get actual timeline range from DataContext`

---

### `projects/Themis.DocumentManager/Converters/ValueConverters.cs` (v0.0.33)

**Maturity Level:** 🔴 ALPHA (27.0/100)

**Issues Found:**

**🔴 STUB** (16 occurrences):
  - Line 63: `throw new NotImplementedException();`
  - Line 87: `throw new NotImplementedException();`
  - Line 119: `throw new NotImplementedException();`
  - Line 151: `throw new NotImplementedException();`
  - Line 164: `throw new NotImplementedException();`

---

### `projects/Themis.DocumentManager/Domain/Classification/ClassificationModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Domain/Collaboration/Comment.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Domain/Collaboration/DocumentLock.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Domain/Collaboration/UserPresence.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Domain/Events/DocumentCreatedEvent.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Domain/Events/DocumentDomainEvents.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Domain/Events/DocumentsRefreshRequestedEvent.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Domain/Events/TestDataGeneratedEvent.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Domain/Exceptions/DocumentNotFoundException.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Examples/DirectXRendererUsageExample.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**🐛 DEBUG** (17 occurrences):
  - Line 119: `System.Diagnostics.Debug.WriteLine("Renderer initialized");`
  - Line 123: `System.Diagnostics.Debug.WriteLine($"Rendered {graph.Nodes.Count} nodes");`
  - Line 130: `System.Diagnostics.Debug.WriteLine("Camera adjusted");`
  - Line 159: `System.Diagnostics.Debug.WriteLine(`
  - Line 186: `System.Diagnostics.Debug.WriteLine($"Added node. Total: {graph.Nodes.Count}");`

---

### `projects/Themis.DocumentManager/Features/AIChat/Services/AIAssistantService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 634: `await Task.Delay(100, cancellationToken); // Simulate streaming delay`

---

### `projects/Themis.DocumentManager/Features/AIChat/ViewModels/AIChatViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Features/AIChat/Views/AIChatView.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Features/Dashboard/Services/DashboardService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (93.0/100)

**Issues Found:**

**📝 TODO** (4 occurrences):
  - Line 103: `["userId"] = "current-user", // TODO: Get from context`
  - Line 223: `["userId"] = "current-user", // TODO: Get from context`
  - Line 339: `["userId"] = "current-user", // TODO: Get from context`
  - Line 427: `["userId"] = "current-user", // TODO: Get from context`

---

### `projects/Themis.DocumentManager/Features/Dashboard/ViewModels/DashboardPreviewViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Features/Dashboard/ViewModels/DashboardViewModel.cs` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (79.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 75: `// Load recent activities (mock for now)`
  - Line 88: `// Mock data - would be loaded from actual activity log`

**📝 TODO** (8 occurrences):
  - Line 59: `UserId = "current-user", // TODO: Get from auth service`
  - Line 126: `// TODO: Implement navigation`
  - Line 133: `// TODO: Implement navigation`
  - Line 140: `// TODO: Implement navigation`
  - Line 147: `// TODO: Implement navigation`

---

### `projects/Themis.DocumentManager/Features/Dashboard/Views/DashboardPreviewView.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Features/Dashboard/Views/DashboardView.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Features/DocumentBrowser/ViewModels/DocumentBrowserNodeViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Features/DocumentBrowser/ViewModels/DocumentBrowserSimpleViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Features/DocumentBrowser/ViewModels/DocumentBrowserViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**🐛 DEBUG** (14 occurrences):
  - Line 169: `System.Diagnostics.Debug.WriteLine($"[ERROR] LoadFilingsForAuthorityAsync: {ex.Message}");`
  - Line 212: `System.Diagnostics.Debug.WriteLine($"[ERROR] LoadFilesForFilingAsync: {ex.Message}");`
  - Line 256: `System.Diagnostics.Debug.WriteLine($"[ERROR] LoadDocumentsForFileAsync: {ex.Message}");`
  - Line 398: `System.Diagnostics.Debug.WriteLine($"[ERROR] OnNodeRightClickAsync: {ex.Message}");`
  - Line 464: `System.Diagnostics.Debug.WriteLine($"[ERROR] ExecuteContextMenuActionAsync: {ex.Message}");`

---

### `projects/Themis.DocumentManager/Features/DocumentBrowser/Views/DocumentBrowserSimpleView.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Features/DocumentBrowser/Views/DocumentBrowserView.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Features/ERDQueryEditor/Services/QueryService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (3 occurrences):
  - Line 256: `System.Diagnostics.Debug.WriteLine($"Error persisting default queries: {persistEx.Message}");`
  - Line 263: `System.Diagnostics.Debug.WriteLine($"Error loading saved queries: {ex.Message}");`
  - Line 278: `System.Diagnostics.Debug.WriteLine($"Error saving queries: {ex.Message}");`

---

### `projects/Themis.DocumentManager/Features/ERDQueryEditor/Services/SchemaService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 79: `System.Diagnostics.Debug.WriteLine($"Schema refresh failed: {ex.Message}");`

---

### `projects/Themis.DocumentManager/Features/ERDQueryEditor/ViewModels/ERDViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (2 occurrences):
  - Line 57: `System.Diagnostics.Debug.WriteLine($"Error during initial schema load: {ex.Message}");`
  - Line 203: `System.Diagnostics.Debug.WriteLine($"Schema load error: {ex}");`

---

### `projects/Themis.DocumentManager/Features/ERDQueryEditor/ViewModels/QueryEditorViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🐛 DEBUG** (3 occurrences):
  - Line 61: `System.Diagnostics.Debug.WriteLine($"Error during initial query load: {ex.Message}");`
  - Line 91: `System.Diagnostics.Debug.WriteLine($"Validation error: {ex.Message}");`
  - Line 116: `System.Diagnostics.Debug.WriteLine($"Validation error: {ex.Message}");`

---

### `projects/Themis.DocumentManager/Features/ERDQueryEditor/Views/ERDView.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Features/ERDQueryEditor/Views/QueryEditorView.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Features/Favorites/ViewModels/FavoritesViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (92.0/100)

**Issues Found:**

**📝 TODO** (5 occurrences):
  - Line 56: `UserId = "current-user", // TODO: Get from auth service`
  - Line 139: `UserId = "current-user" // TODO: Get from AuthenticationService via handler`
  - Line 152: `UserId = "current-user" // TODO: Get from AuthenticationService via handler`
  - Line 162: `// TODO: Implement navigation`
  - Line 202: `// TODO: Implement filtering`

---

### `projects/Themis.DocumentManager/Features/Favorites/Views/FavoritesPanelView.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Features/Gantt/Services/GanttService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Features/Gantt/ViewModels/GanttViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 45: `// TODO: Load actual tasks from service`

---

### `projects/Themis.DocumentManager/Features/Gantt/Views/GanttView.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Features/Geo/Services/GeoServices.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**📝 TODO** (2 occurrences):
  - Line 460: `// TODO: Implement actual HTTP call to Nominatim API`
  - Line 480: `// TODO: Implement actual HTTP call to Nominatim API`

---

### `projects/Themis.DocumentManager/Features/Geo/ViewModels/GeoViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Features/Geo/Views/GeoView.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (5 occurrences):
  - Line 84: `// Enable developer tools in debug mode`
  - Line 85: `#if DEBUG`
  - Line 297: `System.Diagnostics.Debug.WriteLine($"Error updating layer visibility: {ex.Message}");`
  - Line 375: `System.Diagnostics.Debug.WriteLine($"Error adding feature: {ex.Message}");`
  - Line 424: `System.Diagnostics.Debug.WriteLine($"Error zooming to bounds: {ex.Message}");`

---

### `projects/Themis.DocumentManager/Features/Graph/Services/GraphVisualizationService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Features/Graph/ViewModels/GraphViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Features/Graph/Views/GraphView.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (5 occurrences):
  - Line 86: `// Enable developer tools in debug mode`
  - Line 87: `#if DEBUG`
  - Line 134: `System.Diagnostics.Debug.WriteLine($"Error handling web message: {ex.Message}");`
  - Line 628: `System.Diagnostics.Debug.WriteLine($"Error resetting view: {ex.Message}");`
  - Line 692: `System.Diagnostics.Debug.WriteLine($"Error highlighting node: {ex.Message}");`

---

### `projects/Themis.DocumentManager/Features/Graph/Views/GraphView3D.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 208: `System.Diagnostics.Debug.WriteLine($"Render Error: {ex.Message}");`

---

### `projects/Themis.DocumentManager/Features/MetadataForm/Services/IMetadataBindingService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Features/MetadataForm/Services/IMetadataLayoutService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Features/MetadataForm/Services/ISmartMetadataLayoutEngine.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Features/MetadataForm/Services/MetadataBindingService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (2 occurrences):
  - Line 45: `// TODO: Implementierung für echte Datenquelle`
  - Line 90: `// TODO: Implementierung für echte Persistierung`

**🐛 DEBUG** (4 occurrences):
  - Line 54: `System.Diagnostics.Debug.WriteLine($"Failed to load binding for {documentId}: {ex.Message}");`
  - Line 102: `System.Diagnostics.Debug.WriteLine($"Failed to save binding: {ex.Message}");`
  - Line 129: `System.Diagnostics.Debug.WriteLine($"Failed to update field: {ex.Message}");`
  - Line 150: `System.Diagnostics.Debug.WriteLine($"Failed to finalize binding: {ex.Message}");`

---

### `projects/Themis.DocumentManager/Features/MetadataForm/Services/MetadataFormGeneratorService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🐛 DEBUG** (3 occurrences):
  - Line 105: `System.Diagnostics.Debug.WriteLine($"[ERROR] Loading metadata template: {ex}");`
  - Line 181: `System.Diagnostics.Debug.WriteLine($"[WARN] Could not load template {name}: {ex.Message}");`
  - Line 282: `System.Diagnostics.Debug.WriteLine($"Invalid regex pattern: {ValidationPattern}");`

---

### `projects/Themis.DocumentManager/Features/MetadataForm/Services/MetadataLayoutService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Features/MetadataForm/Services/SmartMetadataLayoutEngine.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Features/MetadataForm/ViewModels/MetadataFormViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 68: `System.Diagnostics.Debug.WriteLine($"[ERROR] Loading form: {ex}");`

---

### `projects/Themis.DocumentManager/Features/MetadataForm/Views/MetadataFormView.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Features/TaskBasket/ViewModels/TaskBasketViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (2 occurrences):
  - Line 108: `UserId = "current-user", // TODO: Get from authentication service`
  - Line 232: `// TODO: Implement navigation`

---

### `projects/Themis.DocumentManager/Features/TaskBasket/ViewModels/TasksRightSidebarEnhancedViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 406: `// TODO: Open comprehensive task view showing:`

**🐛 DEBUG** (8 occurrences):
  - Line 158: `System.Diagnostics.Debug.WriteLine($"Error loading tasks: {ex.Message}");`
  - Line 197: `System.Diagnostics.Debug.WriteLine($"Error adding tasks to timeline: {ex.Message}");`
  - Line 223: `System.Diagnostics.Debug.WriteLine($"Error enriching tasks with geo data: {ex.Message}");`
  - Line 256: `System.Diagnostics.Debug.WriteLine($"Error generating AI suggestions: {ex.Message}");`
  - Line 284: `System.Diagnostics.Debug.WriteLine($"Error finding related tasks: {ex.Message}");`

---

### `projects/Themis.DocumentManager/Features/TaskBasket/ViewModels/TasksRightSidebarViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (5 occurrences):
  - Line 84: `UserId = CurrentUserId ?? "current-user", // TODO: Get from authentication service`
  - Line 161: `// TODO: Persist changes via MediatR command`
  - Line 185: `// TODO: Persist to backend via MediatR command`
  - Line 201: `// TODO: Implement navigation to task details or related entity`
  - Line 219: `// TODO: Persist via MediatR and notify assignee (Teams-style notification)`

**🐛 DEBUG** (2 occurrences):
  - Line 113: `System.Diagnostics.Debug.WriteLine($"Error loading tasks: {ex.Message}");`
  - Line 169: `System.Diagnostics.Debug.WriteLine($"Error handling task drop: {ex.Message}");`

---

### `projects/Themis.DocumentManager/Features/TaskBasket/Views/TaskBasketView.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Features/TaskBasket/Views/TaskCardView.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Features/Timeline/Views/TimelineViewImproved.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/GlobalUsings.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Infrastructure/BackgroundJobs/DocumentLockCleanupService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Infrastructure/MachineLearning/DocumentClassifier.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 191: `// TODO: Extract from model metadata`

---

### `projects/Themis.DocumentManager/Infrastructure/MachineLearning/MetadataExtractor.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Infrastructure/Persistence/ThemisRepository.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Infrastructure/SignalR/SignalRConfiguration.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 38: `/// Logging Level (Debug, Information, Warning, Error)`

---

### `projects/Themis.DocumentManager/Infrastructure/SignalR/SignalRService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Models/AIAssistantModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Models/AdministrativeStructure.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Models/CacheModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Models/CalendarModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Models/DashboardModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Models/Document.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Models/DocumentPreviewModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Models/DocumentRevision.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Models/DocumentTreeModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Models/DynamicMetadataModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Models/GanttModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Models/GeoModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Models/GraphModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 38: `public double Mass { get; set; } = 1.0; // Für Force-Simulation`
  - Line 196: `// Physik-Simulation`
  - Line 306: `/// Physik-Simulation`

---

### `projects/Themis.DocumentManager/Models/HelpSystemModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Models/LLMModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Models/MessengerModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Models/MetadataBadgeModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Models/MetadataFieldGroup.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Models/NotificationModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Models/OutboxModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Models/OutlookTaskModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Models/Phase1Models.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Models/Phase3ComplianceModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Models/ProcessWatchModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Models/SchemaModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Models/TimelineModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Models/TreeViewSettings.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/AdministrativeStructureService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/AnimationService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/ApplicationStateService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/AsyncEnumerableHelper.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/AuditLoggingService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 36: `System.Diagnostics.Debug.WriteLine($"[AUDIT] {entry.ActionType} | User: {entry.UserId} | Entity: {en`

---

### `projects/Themis.DocumentManager/Services/AuthenticationService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 115: `System.Diagnostics.Debug.WriteLine($"[Auth] Login failed: {ex.Message}");`

---

### `projects/Themis.DocumentManager/Services/CacheService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 257: `// TODO: Implement intelligent prefetching based on usage patterns`

---

### `projects/Themis.DocumentManager/Services/CalendarIntegrationService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/Classification/ClassificationService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/Classification/IClassificationService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/CollaborationServices.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/ConnectionMonitorService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/ContextMenuService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/DialogService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/DirectX/AdvancedDirectX3DGraphRenderer.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (5 occurrences):
  - Line 40: `System.Diagnostics.Debug.WriteLine(`
  - Line 73: `System.Diagnostics.Debug.WriteLine(`
  - Line 215: `System.Diagnostics.Debug.WriteLine(`
  - Line 233: `System.Diagnostics.Debug.WriteLine(`
  - Line 251: `System.Diagnostics.Debug.WriteLine(`

---

### `projects/Themis.DocumentManager/Services/DirectX/AdvancedEffectsRenderer.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/DirectX/AdvancedOptimizationEngine.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/DirectX/BufferManagement.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (6 occurrences):
  - Line 173: `System.Diagnostics.Debug.WriteLine(`
  - Line 201: `System.Diagnostics.Debug.WriteLine(`
  - Line 229: `System.Diagnostics.Debug.WriteLine(`
  - Line 255: `System.Diagnostics.Debug.WriteLine(`
  - Line 276: `System.Diagnostics.Debug.WriteLine(`

---

### `projects/Themis.DocumentManager/Services/DirectX/DirectXCore.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🐛 DEBUG** (11 occurrences):
  - Line 27: `System.Diagnostics.Debug.WriteLine($"DirectX Device initialized for window {windowHandle}");`
  - Line 33: `System.Diagnostics.Debug.WriteLine($"Clear: RGBA({r}, {g}, {b}, {a})");`
  - Line 38: `System.Diagnostics.Debug.WriteLine("DirectX Present");`
  - Line 43: `System.Diagnostics.Debug.WriteLine($"DirectX Resize: {width}x{height}");`
  - Line 68: `System.Diagnostics.Debug.WriteLine($"Vertex shader compiled: {name}");`

---

### `projects/Themis.DocumentManager/Services/DirectX/DirectXServiceCollectionExtensions.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/DirectX/EnhancedDirectX3DGraphRenderer.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (12 occurrences):
  - Line 46: `System.Diagnostics.Debug.WriteLine(`
  - Line 48: `System.Diagnostics.Debug.WriteLine(`
  - Line 73: `System.Diagnostics.Debug.WriteLine(`
  - Line 159: `System.Diagnostics.Debug.WriteLine(`
  - Line 226: `System.Diagnostics.Debug.WriteLine(`

---

### `projects/Themis.DocumentManager/Services/DirectX/GPUHardwareIntegration.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 332: `/// Simulate GPU upload (would call D3D11 in real implementation)`
  - Line 502: `/// GPU Texture Sampling (mock for D3D11 texture operations)`

---

### `projects/Themis.DocumentManager/Services/DirectX/LoadTestRunner.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 59: `// Simulate frame rendering`

---

### `projects/Themis.DocumentManager/Services/DirectX/MeshGenerator.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/DirectX/NodePickingSystem.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (6 occurrences):
  - Line 278: `System.Diagnostics.Debug.WriteLine($"Selected node: {nodeId}");`
  - Line 290: `System.Diagnostics.Debug.WriteLine($"Deselected node: {nodeId}");`
  - Line 298: `System.Diagnostics.Debug.WriteLine($"Selected node: {nodeId}");`
  - Line 327: `System.Diagnostics.Debug.WriteLine("Selection cleared");`
  - Line 395: `System.Diagnostics.Debug.WriteLine(`

---

### `projects/Themis.DocumentManager/Services/DirectX/OSMMapManager.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (8 occurrences):
  - Line 38: `System.Diagnostics.Debug.WriteLine(`
  - Line 67: `System.Diagnostics.Debug.WriteLine(`
  - Line 74: `System.Diagnostics.Debug.WriteLine($"Error loading OSM tiles: {ex.Message}");`
  - Line 106: `System.Diagnostics.Debug.WriteLine($"Error loading tile {zoom}/{tileX}/{tileY}: {ex.Message}");`
  - Line 156: `System.Diagnostics.Debug.WriteLine("OSM tile cache cleared");`

---

### `projects/Themis.DocumentManager/Services/DirectX/OSMMapRenderer.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (16 occurrences):
  - Line 42: `System.Diagnostics.Debug.WriteLine("OSM Map Renderer initialized with 4 default layers");`
  - Line 70: `System.Diagnostics.Debug.WriteLine(`
  - Line 79: `System.Diagnostics.Debug.WriteLine($"Error loading map: {ex.Message}");`
  - Line 101: `System.Diagnostics.Debug.WriteLine($"Added GeoPoint: {point}");`
  - Line 117: `System.Diagnostics.Debug.WriteLine(`

---

### `projects/Themis.DocumentManager/Services/DirectX/PerformanceTestingFramework.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/DirectX/ProductionReleaseConfig.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/DirectX/RenderingPipeline.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/DirectX/ShaderPipeline.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 122: `// For now, we simulate successful compilation`
  - Line 354: `/// Simulate ByteCode Generation`
  - Line 359: `// For simulation, we create a hash of the source`

**🐛 DEBUG** (5 occurrences):
  - Line 109: `Debug.WriteLine("Default shaders compiled:");`
  - Line 110: `Debug.WriteLine($"  Vertex Shader: {(vertexShader.IsValid ? "✓" : "✗")}");`
  - Line 111: `Debug.WriteLine($"  Pixel Shader: {(pixelShader.IsValid ? "✓" : "✗")}");`
  - Line 135: `Debug.WriteLine($"Compiled shader: {name} ({profile})");`
  - Line 140: `Debug.WriteLine($"Failed to compile shader {name}: {ex.Message}");`

---

### `projects/Themis.DocumentManager/Services/DirectXServiceCollectionExtensions.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/DocumentManagerOptions.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 57: `/// Minimum log level (Debug, Information, Warning, Error, Critical)`

---

### `projects/Themis.DocumentManager/Services/DocumentPreviewService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (86.0/100)

**Issues Found:**

**📝 TODO** (8 occurrences):
  - Line 166: `// TODO: Implement actual page rendering using appropriate library`
  - Line 204: `// TODO: Generate thumbnail`
  - Line 229: `// TODO: Use Office Interop to extract Word content`
  - Line 259: `// TODO: Use Office Interop to extract Excel content`
  - Line 287: `// TODO: Use Office Interop to extract PowerPoint content`

---

### `projects/Themis.DocumentManager/Services/DocumentTreeService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/DsmDataContracts.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/DsmInMemoryStores.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 11: `/// Hinweis: Nicht dauerhaft – dient als Stub/Adapter, bis ThemisDB-Adapter implementiert ist.`

---

### `projects/Themis.DocumentManager/Services/DsmLocalDataStore.cs` (v0.0.33)

**Maturity Level:** 🟠 BETA (43.0/100)

**Issues Found:**

**🔴 STUB** (12 occurrences):
  - Line 12: `/// Kann für Debug/Offline mit Stub-Daten befüllt werden und implementiert alle DSM-Store-Interfaces`
  - Line 201: `#region Stub/Sync Helpers`
  - Line 203: `/// Befüllt den lokalen Store mit Stub-Daten (z.B. für Debug/Offline).`
  - Line 205: `public Task LoadStubsAsync(DsmStubData stub)`
  - Line 207: `foreach (var t in stub.Templates)`

**🐛 DEBUG** (2 occurrences):
  - Line 12: `/// Kann für Debug/Offline mit Stub-Daten befüllt werden und implementiert alle DSM-Store-Interfaces`
  - Line 203: `/// Befüllt den lokalen Store mit Stub-Daten (z.B. für Debug/Offline).`

---

### `projects/Themis.DocumentManager/Services/DsmTimelineAggregationService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/DynamicAssemblyLoader.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (5 occurrences):
  - Line 43: `System.Diagnostics.Debug.WriteLine($"Failed to load assembly {dllFile}: {ex.Message}");`
  - Line 66: `System.Diagnostics.Debug.WriteLine($"Error loading assembly from {assemblyPath}: {ex.Message}");`
  - Line 89: `System.Diagnostics.Debug.WriteLine($"Error loading types from {assembly.FullName}: {ex.Message}");`
  - Line 115: `System.Diagnostics.Debug.WriteLine($"Error loading types from {assembly.FullName}: {ex.Message}");`
  - Line 133: `System.Diagnostics.Debug.WriteLine($"Error creating instance of {type.FullName}: {ex.Message}");`

---

### `projects/Themis.DocumentManager/Services/DynamicMetadataEmbeddingService.cs` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (70.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 86: `// Simulate metadata update`
  - Line 115: `// For now, simulate the process`
  - Line 314: `// For now, just simulate storage`

**📝 TODO** (11 occurrences):
  - Line 62: `// TODO: Create Word document with Content Controls`
  - Line 80: `// TODO: Load current data from ThemisDB`
  - Line 81: `// TODO: Update Content Controls in Word document`
  - Line 109: `// TODO: Open Word document using COM Interop`
  - Line 110: `// TODO: Convert Content Controls to static text`

---

### `projects/Themis.DocumentManager/Services/EnhancedNotificationService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/FormAuditService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/FormConfigurationService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/FormFieldLabelingService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/FormSubmissionHistoryService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/FormTemplateService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/FormUICustomizationService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/HelpService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/IFilePreviewPlugin.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (2 occurrences):
  - Line 251: `// TODO: PDF-Rendering mit externer Bibliothek (z.B. PDFium, PdfSharp)`
  - Line 333: `// TODO: MSG-Parsing mit externer Bibliothek (z.B. MsgReader)`

---

### `projects/Themis.DocumentManager/Services/IServices.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/KeyboardNavigationService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/KeyboardShortcutService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/LLMServices.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 316: `// Stub implementation`

---

### `projects/Themis.DocumentManager/Services/MessengerIntegrationService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/MetadataBadgeAggregator.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/MetadataBadgeServices.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 806: `// TODO: Implementiere Abruf aus Benutzerhistorie`

---

### `projects/Themis.DocumentManager/Services/MissingServiceImplementations.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (80.0/100)

**Issues Found:**

**🔴 STUB** (6 occurrences):
  - Line 61: `/// Stub implementation of email integration service`
  - Line 67: `// Stub: just log and return success`
  - Line 79: `/// Stub implementation of scan service`
  - Line 95: `/// Stub implementation of full-text search service`
  - Line 111: `/// Stub implementation of form management service`

---

### `projects/Themis.DocumentManager/Services/NoOpNotificationService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/OfficeIntegrationService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/OllamaContentGeneratorService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/OllamaService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (3 occurrences):
  - Line 87: `System.Diagnostics.Debug.WriteLine($"[Ollama] Chat failed: {ex.Message}");`
  - Line 111: `System.Diagnostics.Debug.WriteLine($"[Ollama] Chat with history failed: {ex.Message}");`
  - Line 179: `System.Diagnostics.Debug.WriteLine($"[Ollama] Failed to get models: {ex.Message}");`

---

### `projects/Themis.DocumentManager/Services/OsmMapRenderer.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/OutboxService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (92.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 216: `// Simulate sending via delivery method`
  - Line 638: `// Helper method to simulate sending`
  - Line 642: `// For now, just simulate a delay and mark recipients as sent`

---

### `projects/Themis.DocumentManager/Services/OutlookTaskService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/Phase1ServiceImplementations.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 742: `// TODO: Implement PDF generation`

---

### `projects/Themis.DocumentManager/Services/Phase1ServiceImplementations2.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/Phase1Services.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/Phase3ComplianceServices.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/ProcessLinkingService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/ProcessWatchService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (2 occurrences):
  - Line 778: `// TODO: Implement email sending`
  - Line 785: `// TODO: Implement desktop notifications`

---

### `projects/Themis.DocumentManager/Services/RevisionService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/RoleBasedPermissionService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/SeamlessIntegrationOrchestrator.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 100: `// Mock implementation`

---

### `projects/Themis.DocumentManager/Services/ServiceConstants.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/ServiceImplementations.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/SettingsService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (2 occurrences):
  - Line 50: `System.Diagnostics.Debug.WriteLine($"Fehler beim Laden der Einstellungen: {ex.Message}");`
  - Line 71: `System.Diagnostics.Debug.WriteLine($"Fehler beim Speichern der Einstellungen: {ex.Message}");`

---

### `projects/Themis.DocumentManager/Services/SmartFormConfigurationDocumentation.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/SmartFormConfigurationService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/SmartFormService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 243: `System.Diagnostics.Debug.WriteLine($"Error getting field suggestions: {ex.Message}");`

---

### `projects/Themis.DocumentManager/Services/StatusMonitorService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/ThemeService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (2 occurrences):
  - Line 98: `System.Diagnostics.Debug.WriteLine($"Theme-Anwendung fehlgeschlagen: {ex.Message}");`
  - Line 142: `System.Diagnostics.Debug.WriteLine($"Theme gespeichert: {CurrentTheme}");`

---

### `projects/Themis.DocumentManager/Services/ThemisApiClient.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/ThemisDBService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🐛 DEBUG** (3 occurrences):
  - Line 41: `System.Diagnostics.Debug.WriteLine($"[ThemisDB] Query failed: {ex.Message}");`
  - Line 50: `System.Diagnostics.Debug.WriteLine($"[ThemisDB] Fallback query also failed: {fallbackEx.Message}");`
  - Line 110: `System.Diagnostics.Debug.WriteLine($"[ThemisDB] Command failed: {ex.Message}");`

---

### `projects/Themis.DocumentManager/Services/ThemisDbSeeder.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 67: `// TODO: Direkt zu ThemisDB via API`

---

### `projects/Themis.DocumentManager/Services/ThemisTestDataGenerator.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Services/TimelineAggregationService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/SplashScreen.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/TestData/TestDataGeneratorCli.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Tests/Classification/DocumentClassifierTests.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 19: `private readonly Mock<ILogger<DocumentClassifier>> _mockLogger;`
  - Line 23: `_mockLogger = new Mock<ILogger<DocumentClassifier>>();`

---

### `projects/Themis.DocumentManager/Tests/Classification/MetadataExtractorTests.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 17: `private readonly Mock<ILogger<MetadataExtractor>> _mockLogger;`
  - Line 22: `_mockLogger = new Mock<ILogger<MetadataExtractor>>();`

---

### `projects/Themis.DocumentManager/Tests/Collaboration/CommentTests.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (92.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 21: `private readonly Mock<ICommentService> _mockCommentService;`
  - Line 22: `private readonly Mock<ILogger<AddCommentHandler>> _mockLogger;`
  - Line 26: `_mockCommentService = new Mock<ICommentService>();`
  - Line 27: `_mockLogger = new Mock<ILogger<AddCommentHandler>>();`

---

### `projects/Themis.DocumentManager/Tests/Collaboration/DocumentLockingTests.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (81.0/100)

**Issues Found:**

**🎭 SIMULATION** (7 occurrences):
  - Line 20: `private readonly Mock<IDocumentLockingService> _mockLockingService;`
  - Line 21: `private readonly Mock<ILogger<CheckOutDocumentHandler>> _mockLogger;`
  - Line 25: `_mockLockingService = new Mock<IDocumentLockingService>();`
  - Line 26: `_mockLogger = new Mock<ILogger<CheckOutDocumentHandler>>();`
  - Line 135: `var mockLogger = new Mock<ILogger<CheckInDocumentHandler>>();`

---

### `projects/Themis.DocumentManager/Tests/Integration/CollaborationIntegrationTests.cs` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (77.0/100)

**Issues Found:**

**🎭 SIMULATION** (9 occurrences):
  - Line 17: `private readonly Mock<ILogger<SignalRService>> _mockLogger;`
  - Line 21: `_mockLogger = new Mock<ILogger<SignalRService>>();`
  - Line 86: `var mockLockingService = new Mock<Services.IDocumentLockingService>();`
  - Line 87: `var mockCommentService = new Mock<Services.ICommentService>();`
  - Line 109: `// Act: Simulate user workflow`

---

### `projects/Themis.DocumentManager/UI/FormRenderer.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/UI/MetadataCompactDisplay.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/UI/MetadataFormView.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/UI/MetadataGroupedAccordion.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/UI/MetadataSummaryPanel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/UI/SmartFormRenderer.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/ViewModels/AuditLogViewerViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/ViewModels/BreadcrumbViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/ViewModels/DocumentCollaborationViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/ViewModels/DocumentPreviewViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (89.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 62: `await Task.Delay(500); // Simulate loading`

**📝 TODO** (5 occurrences):
  - Line 73: `// TODO: Implement print functionality`
  - Line 79: `// TODO: Implement export functionality`
  - Line 85: `// TODO: Implement zoom in`
  - Line 91: `// TODO: Implement zoom out`
  - Line 97: `// TODO: Implement fit to page`

---

### `projects/Themis.DocumentManager/ViewModels/InboxViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (93.0/100)

**Issues Found:**

**📝 TODO** (4 occurrences):
  - Line 59: `// TODO: Log error`
  - Line 77: `// TODO: Open dialog and create item`
  - Line 88: `// TODO: Open assign user dialog`
  - Line 170: `// TODO: Implement filtering logic`

---

### `projects/Themis.DocumentManager/ViewModels/MainViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/ViewModels/Navigation/IntelligentBreadcrumbViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 80: `UserId = "current-user" // TODO: Get from auth service`

---

### `projects/Themis.DocumentManager/ViewModels/ProcessLinkingDialogViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (2 occurrences):
  - Line 77: `System.Diagnostics.Debug.WriteLine($"[ERROR] ProcessLinkingDialog: {ex}");`
  - Line 130: `System.Diagnostics.Debug.WriteLine($"[ERROR] LoadLinkedProcesses: {ex}");`

---

### `projects/Themis.DocumentManager/ViewModels/Settings/TreeViewSettingsViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/ViewModels/TestDataGeneratorViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 535: `throw new NotImplementedException();`

---

### `projects/Themis.DocumentManager/ViewModels/TimelineRulerViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/ViewModels/TimelineViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/ViewModels/UserSwitcherViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 31: `// Hardcoded test users from seeded data`

---

### `projects/Themis.DocumentManager/ViewModels/ViewModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Views/AuditLogViewerView.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Views/BreadcrumbView.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Views/Collaboration/DocumentCollaborationView.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Views/FullDashboardSimpleView.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Views/Inbox/InboxView.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Views/MainWindow.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (2 occurrences):
  - Line 874: `// TODO: Implementierung für Preview-Loading`
  - Line 881: `// TODO: Implementierung für neuen Tab`

---

### `projects/Themis.DocumentManager/Views/Navigation/IntelligentBreadcrumbView.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Views/Navigation/TimelineRulerView.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Views/PlaceholderViews.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Views/Preview/DocumentPreviewView.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Views/ProcessLinkingDialog.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Views/Settings/SettingsDialog.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Views/Settings/TreeViewSettingsView.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Views/ShortcutsOverlay.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Views/TestDataGeneratorWindow.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Views/Timeline/TimelineView.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Views/TimelineSimpleView.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Views/UserSwitcherView.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/Themis.DocumentManager/Views/Windows/AuditLogViewerWindow.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/llm-code-translator/examples/basic_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/llm-code-translator/examples/basic_usage.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/llm-code-translator/examples/complete_pipeline_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 44: `std::cout << "   Continuing with mock execution plan generation...\n\n";`

---

### `projects/llm-code-translator/examples/direct_execution_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 451: `std::cout << "  6. Easier to validate and debug\n";`

---

### `projects/llm-code-translator/examples/jit_compilation_example.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/llm-code-translator/src/direct_execution_engine.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/llm-code-translator/src/direct_executor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/llm-code-translator/src/direct_executor.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 109: `// Mock database for testing`

---

### `projects/llm-code-translator/src/execution_plan.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/llm-code-translator/src/execution_plan.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/llm-code-translator/src/jit_compiler.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 96: `// Debug`

---

### `projects/llm-code-translator/src/llm_code_translator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/llm-code-translator/src/prompt_to_plan.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/llm-code-translator/src/prompt_to_plan.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/llm-code-translator/src/vllm_client.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `projects/llm-code-translator/src/vllm_client.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `scripts/add_doc_metadata.py` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `scripts/assign-issue-priorities.py` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 15: ```[BUG]`` in title`

---

### `scripts/cleanup_milestones.py` (v0.0.25)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `scripts/compendium/generate_pdf_all_themes.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 245: `print("\n🧹 Cleaning up temporary files...")`

---

### `scripts/compendium/generate_pdf_rendered.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (2 occurrences):
  - Line 64: `# Create temporary .mmd file`
  - Line 517: `print(f"\n🧹 Cleaning up temporary files...")`

---

### `scripts/compendium/generate_pdf_svg.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 39: `# Create temporary mermaid file`

---

### `scripts/compendium/generate_pdf_weasyprint.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `scripts/compendium/generate_pdf_with_mermaid.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `scripts/compendium/generate_themed_pdfs.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `scripts/create_docs_audit_issues.py` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**🔴 STUB** (4 occurrences):
  - Line 86: `The plugin system is documented as *production-ready runtime plugin loading*, but the actual impleme`
  - Line 94: `- [ ] A contributor guide for the current (stub) plugin interface is present`
  - Line 267: `- `docs/TOOLS_INDEX.md` (stub)`
  - Line 272: `The `docs/TOOLS_INDEX.md` file exists as a stub but contains no useful content.`

---

### `scripts/create_missing_issues.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `scripts/create_module_epics.py` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `scripts/cross-compile-reviewer.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (2 occurrences):
  - Line 240: `"""RULE 3: Check for hardcoded absolute paths"""`
  - Line 259: `message=f"Hardcoded path not cross-compile safe: {desc}",`

---

### `scripts/docs-lint.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `scripts/fix_cmake_paths.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `scripts/fix_doc_links.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `scripts/generate_aql_docs.py` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `scripts/generate_docs_database.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `scripts/generate_docs_database_backup.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 263: `logging.getLogger().setLevel(logging.DEBUG)`

**🔒 HARDCODED** (2 occurrences):
  - Line 116: `# Create a temporary database for tracking per directory`
  - Line 155: `# Clean up temporary files`

---

### `scripts/generate_docs_rocksdb.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `scripts/generate_docs_rocksdb_backup.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `scripts/generate_legal_rocksdb.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `scripts/generate_research_index.py` (v0.0.10)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `scripts/github-release.py` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `scripts/ingest_legal_training_data.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 314: `logging.debug("Non-serializable metadata value skipped: %s", e)`

---

### `scripts/license-server/app.py` (v0.0.27)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `scripts/license-server/test_license_server.py` (v0.0.27)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 15: `# Use a temporary file so all connections share the same database`

---

### `scripts/link-check.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `scripts/pre-commit-cross-compile.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `scripts/raid_endurance_test.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `scripts/railway/asset_management.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `scripts/railway/cep_rules_engine.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `scripts/railway/db_real_data_integration.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 440: `"track_segments": [],  # TODO: Parse from Schienennetz Shapefiles`

---

### `scripts/railway/digital_twin.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (91.0/100)

**Issues Found:**

**🎭 SIMULATION** (8 occurrences):
  - Line 325: `"""Run a what-if scenario simulation"""`
  - Line 340: `# Simulate forward in time`
  - Line 377: `# Simulate signal failure`
  - Line 384: `# Simulate substation failure`
  - Line 391: `"""Simulate scenario forward in time"""`

---

### `scripts/railway/import_railway_network.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `scripts/railway/simple_network_generator.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 107: `if random.random() < 0.1:  # 10% chance of temporary restriction`

---

### `scripts/railway/train_simulator.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (5 occurrences):
  - Line 239: `"""Starte Echtzeit-Simulation"""`
  - Line 241: `print("Starting Real-time Train Simulation")`
  - Line 270: `print("\n\nStopping simulation...")`
  - Line 507: `# Log connection errors but don't stop simulation`
  - Line 539: `parser.add_argument('--trains', type=int, default=50, help='Number of trains to simulate')`

---

### `scripts/release-orchestrator.py` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `scripts/secret_scan.py` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 138: `re.compile(r'(?i)(example|sample|demo|placeholder|dummy|changeme|password123|yourpassword)'),`

---

### `scripts/sync-milestones-from-roadmap.py` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `scripts/toc-check.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `scripts/tools/demo_compliance.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `scripts/train_failure_model.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `scripts/validate_config_mapping.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `scripts/validate_grafana_dashboards.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `scripts/validate_research_links.py` (v0.0.10)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `scripts/validate_research_metadata.py` (v0.0.10)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `scripts/verification/create_issues_from_gaps.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (5 occurrences):
  - Line 60: `content = content.replace('- [ ]', '').replace('TODO:', '').replace('TBD:', '').strip()`
  - Line 130: `body.append("- [ ] Update original TODO in documentation\n\n")`
  - Line 160: `body.append(f"- Original TODO: [{file_path}:{line_number}]({file_path}#L{line_number})\n")`
  - Line 165: `body.append("This issue was generated from the documentation TODO verification process.\n")`
  - Line 170: `body.append("2. Update the documentation to mark the TODO as complete\n")`

---

### `scripts/verification/generate_verification_report.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (3 occurrences):
  - Line 56: `report.append("# Comprehensive Documentation TODO Verification Report\n")`
  - Line 174: `report.append("- **Automated keyword extraction** from TODO content\n")`
  - Line 219: `print(f"✅ Loaded {len(aggregator.all_todos)} TODO items")`

---

### `scripts/verification/verify_documentation_todos.py` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (70.0/100)

**Issues Found:**

**📝 TODO** (21 occurrences):
  - Line 3: `Documentation TODO Verification Script`
  - Line 5: `This script analyzes documentation files to extract and categorize TODO markers,`
  - Line 29: `"""Represents a single TODO/TBD/checkbox item from documentation"""`
  - Line 33: `marker_type: str  # 'checkbox', 'TODO', 'TBD', 'FIXME'`
  - Line 46: `"""Verifies TODO items against codebase implementation"""`

---

### `scripts/verify-readme-links.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `scripts/verify_pdf.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `sdks/python/themis_llm/__init__.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `sdks/python/themis_llm/client.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `sdks/python/themis_llm/exceptions.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `sdks/python/themis_llm/models.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `security/pentest/report_generator.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `security/pentest/tests/auth_tests.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 317: `remediation="Implement temporary account lockout after failed attempts"`

---

### `src/acceleration/backend_registry.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/acceleration/compute_backend.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/acceleration/cpu_backend.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/acceleration/cpu_backend_mt.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/acceleration/cpu_backend_tbb.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/acceleration/cuda_backend.cpp` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (66.0/100)

**Issues Found:**

**🔴 STUB** (6 occurrences):
  - Line 849: `// CUDAGraphBackend Stub Implementation`
  - Line 858: `return false; // Stub`
  - Line 868: `caps.deviceName = "CUDA Device (Stub)";`
  - Line 875: `initialized_ = false; // Stub`
  - Line 895: `return {}; // Stub`

**🔒 HARDCODED** (1 occurrences):
  - Line 567: `//      cudaGraph_t on a temporary non-blocking stream, then instantiate it.`

---

### `src/acceleration/device_manager.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/acceleration/directx_backend_full.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 176: `// Enable debug layer in debug builds`

---

### `src/acceleration/faiss_gpu_backend.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (84.0/100)

**Issues Found:**

**🔒 HARDCODED** (4 occurrences):
  - Line 364: `// For one-time distance computation, create a temporary flat index`
  - Line 377: `// Add vectors to temporary index`
  - Line 418: `// For one-time KNN search, create a temporary flat index`
  - Line 431: `// Add vectors to temporary index`

---

### `src/acceleration/geo_acceleration_bridge.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/acceleration/graphics_backends.cpp` (v0.0.33)

**Maturity Level:** 🟠 BETA (40.0/100)

**Issues Found:**

**🔴 STUB** (12 occurrences):
  - Line 545: `// DirectX Vector Backend Stub`
  - Line 556: `return false; // Stub: not fully implemented yet`
  - Line 568: `caps.deviceName = "DirectX 12 (Stub)";`
  - Line 576: `initialized_ = false; // Stub`
  - Line 600: `return {}; // Stub`

---

### `src/acceleration/hip_backend.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (93.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 982: `// Non-HIP fallback stubs`

**📝 TODO** (1 occurrences):
  - Line 178: `// TODO: For larger k, consider heap-based selection or radix select`

---

### `src/acceleration/multi_gpu_backend.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/acceleration/nccl_vector_backend.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (86.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 467: `// Stub implementation when NCCL is not available`

**🎭 SIMULATION** (3 occurrences):
  - Line 370: `// Use a barrier via AllReduce of a dummy value`
  - Line 371: `float dummy = 0.0f;`
  - Line 372: `return allReduce(&dummy, &dummy, 1, ReductionOp::SUM, stream);`

---

### `src/acceleration/oneapi_backend.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 201: `// Stub implementation when OneAPI is not available`

---

### `src/acceleration/opencl_backend.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 336: `// Stub method definitions when OpenCL is not available`

---

### `src/acceleration/plugin_loader.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/acceleration/plugin_security.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/acceleration/rccl_vector_backend.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (86.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 467: `// Stub implementation when RCCL is not available`

**🎭 SIMULATION** (3 occurrences):
  - Line 370: `// Use a barrier via AllReduce of a dummy value`
  - Line 371: `float dummy = 0.0f;`
  - Line 372: `return allReduce(&dummy, &dummy, 1, ReductionOp::SUM, stream);`

---

### `src/acceleration/shader_integrity.cpp` (v0.0.10)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/acceleration/tensor_core_matmul.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/acceleration/vllm_resource_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/acceleration/vulkan_backend_full.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 159: `// Enable validation layers in debug mode`

---

### `src/acceleration/zluda_backend.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/analytics/analytics_export.cpp` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (72.0/100)

**Issues Found:**

**🔴 STUB** (6 occurrences):
  - Line 177: `class StubAnalyticsExporter : public IAnalyticsExporter {`
  - Line 179: `StubAnalyticsExporter() {`
  - Line 188: `~StubAnalyticsExporter() override = default;`
  - Line 699: `// For now, return stub exporter for all formats`
  - Line 701: `return std::make_unique<StubAnalyticsExporter>();`

**🐛 DEBUG** (4 occurrences):
  - Line 180: `spdlog::debug("AnalyticsExporter initialized (Arrow support: {})",`
  - Line 200: `spdlog::debug("Exporting {} rows to file: {}", batch.rowCount(), output_path);`
  - Line 220: `spdlog::debug("Using Apache Arrow for export format");`
  - Line 281: `spdlog::debug("Arrow string export requested for {} rows", batch.rowCount());`

---

### `src/analytics/anomaly_detection.cpp` (v0.0.18)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/analytics/arrow_export.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/analytics/arrow_flight.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (8 occurrences):
  - Line 121: `spdlog::debug("[ArrowFlight] registered in-process server: {}", endpoint);`
  - Line 127: `spdlog::debug("[ArrowFlight] unregistered in-process server: {}", endpoint);`
  - Line 144: `spdlog::debug("[ArrowFlight] registered dataset '{}' on '{}'",`
  - Line 202: `spdlog::debug("[ArrowFlight] doGet '{}' from '{}'",`
  - Line 231: `spdlog::debug("[ArrowFlight] doPut '{}' to '{}' ({} rows)",`

---

### `src/analytics/automl.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/analytics/cep_engine.cpp` (v0.0.18)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (5 occurrences):
  - Line 1385: `spdlog::debug("CEP ACTION type={} rule='{}'",`
  - Line 1870: `spdlog::debug("CEP RuleEngine: restored matcher state for rule '{}'",`
  - Line 1974: `spdlog::debug("CEPEngine: stream '{}' created", config.stream_id);`
  - Line 2026: `spdlog::debug("CEPEngine: backpressure active ({:.0f}% full)",`
  - Line 2381: `spdlog::debug("CEP metrics: recv={} proc={} drop={} bp={} queue={} alerts={} streams={} rules={}",`

---

### `src/analytics/columnar_execution.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/analytics/diff_engine.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (90.0/100)

**Issues Found:**

**🐛 DEBUG** (10 occurrences):
  - Line 155: `spdlog::debug("Computing diff: from_seq={} to_seq={}", from_sequence, to_sequence);`
  - Line 163: `spdlog::debug("Cache hit for diff range [{}, {}]", from_sequence, to_sequence);`
  - Line 186: `spdlog::debug("Found {} events in range [{}, {}]", events.size(), from_sequence, to_sequence);`
  - Line 218: `spdlog::debug("Computing diff by timestamp: from={} to={}", from_timestamp, to_timestamp);`
  - Line 223: `spdlog::debug("Timestamp range maps to sequence range [{}, {}]", from_seq, to_seq);`

---

### `src/analytics/distributed_analytics.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/analytics/forecasting.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/analytics/incremental_view.cpp` (v0.0.18)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/analytics/jit_aggregation.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (2 occurrences):
  - Line 464: `spdlog::debug("JITAggregationCompiler: compiled specialisation for key '{}'",`
  - Line 475: `spdlog::debug("JITAggregationCompiler: LLVM JIT backend enabled "`

---

### `src/analytics/llm_process_analyzer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (92.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 367: `spdlog::debug("LLM call (stub): provider={}, model={}, key={}",`

**📝 TODO** (1 occurrences):
  - Line 364: `// TODO: Integrate with actual LLM API (OpenAI, Anthropic, local models)`

**🐛 DEBUG** (1 occurrences):
  - Line 367: `spdlog::debug("LLM call (stub): provider={}, model={}, key={}",`

---

### `src/analytics/ml_serving.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 520: `// Construct a temporary backend to check availability`

---

### `src/analytics/model_serving.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/analytics/nlp_text_analyzer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 712: `// estar (to be - temporary)`

---

### `src/analytics/olap.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (80.0/100)

**Issues Found:**

**🔴 STUB** (4 occurrences):
  - Line 27: `// Windows build stub: minimal no-op implementations to unblock compilation`
  - Line 35: `spdlog::warn("OLAPEngine: Using Windows stub implementation - full OLAP functionality not available"`
  - Line 38: `spdlog::warn("OLAPEngine: Using Windows stub implementation - GPU acceleration not available on Wind`
  - Line 1729: `// Arrow not available - stub implementations`

---

### `src/analytics/process_mining.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 1351: `// Stub implementations for remaining methods`

---

### `src/analytics/process_pattern_matcher.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (2 occurrences):
  - Line 53: `spdlog::debug("ProcessPatternMatcher initialized");`
  - Line 776: `spdlog::debug("ProcessPatternMatcher: cache cleared");`

---

### `src/analytics/streaming_window.cpp` (v0.0.18)

**Maturity Level:** 🟢 PRODUCTION-READY (85.0/100)

**Issues Found:**

**📝 TODO** (8 occurrences):
  - Line 355: `// BUG 3 FIX: fire callbacks outside the lock to prevent re-entrant deadlock.`
  - Line 540: `// BUG 3 FIX: fire callbacks outside the lock.`
  - Line 633: `// BUG 4 FIX: Apply watermark check (was entirely missing).`
  - Line 695: `// BUG 2 FIX: use max() so that an out-of-order record cannot`
  - Line 704: `// BUG 3 FIX: invoke callback outside the mutex to prevent re-entrant deadlock.`

**🐛 DEBUG** (1 occurrences):
  - Line 328: `spdlog::debug("TumblingWindow: dropped late record (event={} < watermark={})", ev_us, wm);`

---

### `src/api/geo_index_hooks.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/api/graphql.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/api/grpc_server.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🐛 DEBUG** (3 occurrences):
  - Line 10: `// gRPC reflection is only compiled in debug builds to prevent schema leakage`
  - Line 102: `// Register gRPC reflection in debug builds only.`
  - Line 107: `THEMIS_INFO("GrpcApiServer: gRPC reflection enabled (debug build)");`

---

### `src/api/http_server.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 1: `// DEPRECATED - HTTP server stub (legacy placeholder)`

---

### `src/api/themisdb_grpc_service.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (80.0/100)

**Issues Found:**

**🔴 STUB** (4 occurrences):
  - Line 7: `// stubs generated from proto/themisdb.proto are available on the include path.`
  - Line 27: `// Impl (only compiled when proto stubs are present)`
  - Line 424: `"service will be a no-op until protoc generates the stubs");`
  - Line 434: `// proto stubs not generated; returning null is expected here`

---

### `src/api/tracing_middleware.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/api/ws_handler.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/aql/aql_autocomplete.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/aql/aql_confidence_scorer.cpp` (v0.0.25)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/aql/aql_conversation_context.cpp` (v0.0.25)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/aql/aql_fewshot_example_library.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/aql/aql_lora_finetuner.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/aql/aql_migration_assistant.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/aql/aql_optimizer_advisor.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/aql/aql_query_builder.cpp` (v0.0.25)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/aql/aql_query_template_library.cpp` (v0.0.25)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/aql/aql_query_validator.cpp` (v0.0.25)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/aql/aql_schema_provider.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/aql/aql_syntax_highlighter.cpp` (v0.0.22)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/aql/docs_assistant_functions.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 179: `// TODO: Integrate with native CLASSIFY function when execution context available`

**🐛 DEBUG** (2 occurrences):
  - Line 109: `spdlog::debug("Using ThemisHelpLoRA for query: {}", query);`
  - Line 121: `spdlog::debug("Using base DocsAssistant for query: {}", query);`

---

### `src/aql/llm_aql_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (5 occurrences):
  - Line 324: `spdlog::debug("LLM INFER completed: model={}, latency={}ms, input_tokens={}, output_tokens={}",`
  - Line 465: `spdlog::debug("LLM INFER STREAMING completed: model={}, latency={}ms, input_tokens={}, output_tokens`
  - Line 647: `spdlog::debug("LLM RAG completed: collection={}, retrieved_docs={}, latency={}ms",`
  - Line 1273: `spdlog::debug("AQL confidence score: overall={:.2f} structural={:.2f} completeness={:.2f} schema_mat`
  - Line 1381: `spdlog::debug("translateNLToAQLWithExamples: injected {} examples for query \"{}\"",`

---

### `src/aql/llm_metrics_collector.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/auth/api_key_authenticator.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🐛 DEBUG** (2 occurrences):
  - Line 47: `spdlog::debug("ApiKeyAuthenticator: credential added for key_id='{}'",`
  - Line 54: `spdlog::debug("ApiKeyAuthenticator: credential removed for key_id='{}'", key_id);`

---

### `src/auth/auth_audit_logger.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/auth/auth_error.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/auth/auth_metrics.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/auth/auth_rate_limiter.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/auth/federated_identity_manager.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 220: `spdlog::debug("FederatedIdentityManager: validating token for realm '{}'", iss);`

---

### `src/auth/gssapi_authenticator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/auth/jwks_security.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/auth/jwks_validator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/auth/jwt_key_rotation_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/auth/jwt_validator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/auth/kerberos_security.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/auth/ldap_authenticator.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 401: `// Stub: LDAP library not compiled in`

---

### `src/auth/mfa_authenticator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🐛 DEBUG** (4 occurrences):
  - Line 124: `spdlog::debug("TOTP validated successfully (offset: {})", offset);`
  - Line 133: `spdlog::debug("TOTP validation failed");`
  - Line 160: `spdlog::debug("Recovery code validation failed");`
  - Line 172: `spdlog::debug("Generated {} recovery codes for user: {}",`

---

### `src/auth/mtls_authenticator.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 286: `spdlog::debug("MTLSAuthenticator: authenticated principal='{}' serial={}",`

---

### `src/auth/oauth_device_flow.cpp` (v0.0.6)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🐛 DEBUG** (4 occurrences):
  - Line 88: `spdlog::debug("OAuthDeviceFlow: requesting device code from {}",`
  - Line 169: `spdlog::debug("OAuthDeviceFlow: polling token endpoint {}", config_.token_endpoint);`
  - Line 317: `spdlog::debug("OAuthDeviceFlow: authorization pending, retrying in {}s", poll_interval);`
  - Line 323: `spdlog::debug("OAuthDeviceFlow: slow_down received, new interval={}s", poll_interval);`

---

### `src/auth/oauth_pkce_flow.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🐛 DEBUG** (2 occurrences):
  - Line 101: `spdlog::debug("OAuthPKCEFlow: generated PKCE challenge (method=S256, "`
  - Line 184: `spdlog::debug("OAuthPKCEFlow: exchanging authorization code at {}",`

---

### `src/auth/oidc_provider.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 62: `spdlog::debug("OIDCProvider: fetching discovery document from {}", discovery_url);`

---

### `src/auth/password_policy.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/auth/principal_validator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/auth/saml_authenticator.cpp` (v0.0.6)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/auth/session_manager.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/auth/token_blacklist.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/auth/totp_replay_cache.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 154: `utils::Logger::debug("TOTP replay cache cleanup: {} entries expired", expired_count);`

---

### `src/auth/totp_secret_encryption.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/auth/webauthn_authenticator.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/auth/zero_trust_auth_verifier.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/base/ab_test_manager.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/base/hot_reload_manager.cpp` (v0.0.7)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🐛 DEBUG** (3 occurrences):
  - Line 100: `spdlog::debug("HotReloadManager: saved state for '{}' ({} bytes)",`
  - Line 187: `spdlog::debug("HotReloadManager: sandbox [{}]: {}", module_name, w);`
  - Line 289: `spdlog::debug("HotReloadManager: sandbox rollback [{}]: {}", module_name, w);`

---

### `src/base/module_loader.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (83.0/100)

**Issues Found:**

**🐛 DEBUG** (17 occurrences):
  - Line 63: `spdlog::debug("Verifying module: {}", modulePath);`
  - Line 286: `spdlog::debug("STAGE: VALIDATING - {}", moduleName);`
  - Line 307: `spdlog::debug("STAGE: VERIFYING - {}", moduleName);`
  - Line 340: `spdlog::debug("STAGE: VERIFIED - {}", moduleName);`
  - Line 374: `spdlog::debug("STAGE: STAGING - {}", moduleName);`

---

### `src/base/module_sandbox.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/base/plugin_dependency_graph.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/base/remote_registry_client.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🐛 DEBUG** (2 occurrences):
  - Line 108: `spdlog::debug("RemoteRegistryClient::listPlugins GET {}", url);`
  - Line 141: `spdlog::debug("RemoteRegistryClient::fetchPlugin GET {}", url);`

---

### `src/base/wasm_plugin_sandbox.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (4 occurrences):
  - Line 345: `spdlog::debug("WasmPluginSandbox: loading '{}' ({} bytes)",`
  - Line 348: `spdlog::debug("WasmPluginSandbox: loading '{}' ({} bytes)",`
  - Line 415: `spdlog::debug("WasmPluginSandbox: unloaded");`
  - Line 593: `spdlog::debug("WasmPluginSandbox: OS sandbox warning: {}", w);`

---

### `src/cache/adaptive_query_cache.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/cache/bounded_lru_cache.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/cache/cache_hit_rate_slo_monitor.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/cache/cache_replication.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/cache/cache_replication_coordinator.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/cache/distributed_cache_coordinator.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 50: `THEMIS_WARN("RedisCacheCoordinator: Windows stub active - network pub/sub disabled");`

---

### `src/cache/embedding_cache.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/cache/predictive_prefetcher.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/cache/redis_cache_coordinator.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (90.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 9: `* no-op stub that logs a warning on construction, allowing the rest of the`
  - Line 48: `"Redis transport disabled. Coordinator operates as a no-op stub.");`

---

### `src/cache/semantic_cache.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/cache/warmup.cpp` (v0.0.6)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/cdc/cdc_admin.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/cdc/cdc_materialized_view.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/cdc/cdc_ws_handler.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/cdc/changefeed.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 120: `// TODO: Consider using RocksDB merge operator for better performance`

---

### `src/cdc/changefeed_buffer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/cdc/consumer_group.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/cdc/cross_collection_stream.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/cdc/dead_letter_queue.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/cdc/delivery_tracker.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/cdc/kafka_cdc_producer.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 8: `* intentionally empty; the no-op stub is defined inline in the header.`

---

### `src/cdc/outbox.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/cdc/tenant_buffer_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/cdc/ws_transport.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/chimera/adapter_factory.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/chimera/elasticsearch_adapter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (87.0/100)

**Issues Found:**

**🎭 SIMULATION** (5 occurrences):
  - Line 8: `* simulation mode backed by std::unordered_map, which is sufficient for`
  - Line 12: `* (e.g. cpp-httplib or cpr) and replace the in-process simulation blocks`
  - Line 299: `// In simulation mode, Elasticsearch index creation is a no-op.`
  - Line 518: `// Simulation mode reports 8.12; production implementations should query`
  - Line 637: `// Simple deterministic ID generation for the simulation layer.`

---

### `src/chimera/mongodb_adapter.cpp` (v0.0.8)

**Maturity Level:** 🟢 PRODUCTION-READY (84.0/100)

**Issues Found:**

**🎭 SIMULATION** (6 occurrences):
  - Line 8: `* simulation mode backed by std::unordered_map, which is sufficient for`
  - Line 12: `* in-process simulation blocks with real mongocxx calls.`
  - Line 147: `// IVectorAdapter – Atlas Vector Search simulation`
  - Line 300: `// In simulation mode index creation is a no-op`
  - Line 564: `// Simulation mode reports 7.0; production implementations should query`

---

### `src/chimera/neo4j_adapter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (90.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 7: `* driver is not available the adapter operates in an in-process simulation`
  - Line 12: `* HTTP client library) and replace the in-process simulation blocks with`
  - Line 344: `// In simulation mode, return a single path containing all nodes.`
  - Line 553: `// Simulation mode reports 5.x; production implementations should query`

---

### `src/chimera/pinecone_adapter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (87.0/100)

**Issues Found:**

**🎭 SIMULATION** (5 occurrences):
  - Line 8: `* in-process simulation mode backed by std::unordered_map, which is`
  - Line 12: `* (e.g. cpp-httplib or cpr) and replace the in-process simulation blocks`
  - Line 292: `// In simulation mode, Pinecone index/namespace creation is a no-op.`
  - Line 511: `// Simulation mode reports v3; production implementations should query`
  - Line 566: `// Simple deterministic ID generation for the simulation layer.`

---

### `src/chimera/postgresql_adapter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (81.0/100)

**Issues Found:**

**🎭 SIMULATION** (7 occurrences):
  - Line 8: `* simulation mode backed by std::unordered_map, which is sufficient for`
  - Line 12: `* in-process simulation blocks with real libpqxx calls.`
  - Line 125: `// In simulation mode, return an empty result set.`
  - Line 192: `// IVectorAdapter – pgvector simulation`
  - Line 335: `// In simulation mode, index creation is a no-op.`

---

### `src/chimera/qdrant_adapter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (87.0/100)

**Issues Found:**

**🎭 SIMULATION** (5 occurrences):
  - Line 8: `* simulation mode backed by std::unordered_map, which is sufficient for`
  - Line 12: `* (e.g. cpp-httplib or cpr) and replace the in-process simulation blocks`
  - Line 294: `// In simulation mode, Qdrant collection creation is a no-op.`
  - Line 512: `// Simulation mode reports 1.7; production implementations should query`
  - Line 568: `// Simple deterministic ID generation for the simulation layer.`

---

### `src/chimera/themisdb_adapter.cpp` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (67.0/100)

**Issues Found:**

**🔴 STUB** (7 occurrences):
  - Line 5: `* @details This is a stub implementation demonstrating the adapter pattern.`
  - Line 81: `// Stub: Would execute actual query via ThemisDB API`
  - Line 97: `// Stub implementation`
  - Line 112: `// Stub implementation`
  - Line 137: `// Stub: Generate ID`

---

### `src/chimera/weaviate_adapter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (87.0/100)

**Issues Found:**

**🎭 SIMULATION** (5 occurrences):
  - Line 8: `* simulation mode backed by std::unordered_map, which is sufficient for`
  - Line 12: `* (e.g. cpp-httplib or cpr) and replace the in-process simulation blocks`
  - Line 294: `// In simulation mode, Weaviate class schema creation is a no-op.`
  - Line 512: `// Simulation mode reports 1.24; production implementations should query`
  - Line 570: `// Simple deterministic ID generation for the simulation layer.`

---

### `src/config/config_audit_log.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/config/config_audit_log.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/config/config_errors.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/config/config_metrics_exporter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/config/config_metrics_exporter.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/config/config_migration_scanner_impl.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/config/config_path_resolver.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (3 occurrences):
  - Line 1340: `spdlog::debug("ConfigPathResolver: Using env overlay path [{}]: {} -> {}",`
  - Line 1350: `spdlog::debug("ConfigPathResolver: Using new config path: {} -> {}",`
  - Line 1723: `spdlog::debug("ConfigPathResolver: SIGHUP hot-reload not supported on Windows");`

---

### `src/config/config_path_resolver.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/config/config_schema_validator.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/config/config_schema_validator.h` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/config/lru_cache.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/config/path_mapping_metadata.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/content/archive_processor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (86.0/100)

**Issues Found:**

**🔒 HARDCODED** (5 occurrences):
  - Line 55: `* @brief Generate random temporary directory name`
  - Line 76: `* @brief Write blob to temporary file`
  - Line 181: `// Create temporary file for zip_open`
  - Line 352: `// Write blob to temporary file`
  - Line 355: `result.error_message = "Failed to write archive to temporary file";`

---

### `src/content/async_ingestion_worker.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**📝 TODO** (2 occurrences):
  - Line 829: `job.user_context = "";  // TODO: Add user context support`
  - Line 870: `// TODO: Implement YAML config loading`

---

### `src/content/audio_processor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/content/cad_processor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/content/content_errors.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/content/content_fs.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/content/content_logger.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**🐛 DEBUG** (5 occurrences):
  - Line 139: `debug("content.cache." + std::string(hit ? "hit" : "miss"),`
  - Line 160: `case utils::Logger::Level::DEBUG:`
  - Line 161: `utils::Logger::debug(log_message);`
  - Line 190: `void ContentLogger::debug(const std::string& event, const std::string& message, const json& metadata`
  - Line 191: `log(utils::Logger::Level::DEBUG, event, message, metadata);`

---

### `src/content/content_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 1979: `// Cleanup temporary directory`

---

### `src/content/content_manager_embedding.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/content/content_manager_llm.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/content/content_metrics.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/content/content_policy.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/content/content_security.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (90.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 120: `// Check 3: Abuse detection (stub for future implementation)`
  - Line 329: `// Stub implementation for future abuse detection`

---

### `src/content/content_type.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/content/content_validator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/content/deduplication_checker.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/content/embedding_pipeline.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/content/geo_processor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/content/html_processor.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/content/image_processor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/content/ingestion_plugin.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/content/language_detector.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/content/markdown_processor.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/content/mime_detector.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/content/mock_clip_processor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 16: `// For images we don't extract text; instead produce a mock embedding`

---

### `src/content/ocr_processor.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 326: `// IContentProcessor::generateEmbedding (stub – delegates to pipeline)`

---

### `src/content/office_processor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/content/pdf_processor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/content/pipeline/async_bulk_uploader.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/content/pipeline/bulk_upload_interface.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/content/pipeline/content_chunker.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/content/pipeline/multimodal_chunker.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/content/pipeline/zstd_compression.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 51: `// Simulate progress for large data`

---

### `src/content/stt_processor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 486: `// TODO: Integrate with FFmpeg for format conversion`

---

### `src/content/text_processor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 185: `// Mock embedding generator using hash-based approach`

---

### `src/content/tts_processor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/content/version_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/content/video_processor.cpp` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (68.0/100)

**Issues Found:**

**🎭 SIMULATION** (5 occurrences):
  - Line 171: `// This is a simulation - real implementation would use libavformat`
  - Line 324: `// Simulation mode - always healthy`
  - Line 346: `// Fallback to simulation mode`
  - Line 388: `// Return empty thumbnail placeholder in simulation mode`
  - Line 407: `// Scene detection requires per-frame access, so return empty in simulation mode.`

**🔒 HARDCODED** (8 occurrences):
  - Line 449: `* @note This function creates a temporary file for FFmpeg processing.`
  - Line 450: `*       The temporary file is automatically cleaned up on success or error.`
  - Line 455: `// Create unique temporary file path to avoid race conditions`
  - Line 464: `throw std::runtime_error("Failed to create temporary file");`
  - Line 573: `* @note This function creates a temporary file for FFmpeg processing.`

---

### `src/core/adapters/otel_tracer.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/core/concerns/concerns_context.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/core/concerns/context_propagation.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/core/concerns/i_logger.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🐛 DEBUG** (2 occurrences):
  - Line 15: `if (lower == "debug") return Level::DEBUG;`
  - Line 27: `case Level::DEBUG: return "DEBUG";`

---

### `src/core/concerns/prometheus_metrics.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/core/security_initialization.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (88.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 93: `// In production, reject mock/local providers`
  - Line 96: `"Production mode violation: LOCAL (mock) key provider is not allowed in production. "`
  - Line 107: `"Mock/default key providers are not allowed in production."`
  - Line 110: `// In development, allow default mock provider`

---

### `src/demo_encryption.cpp` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (65.0/100)

**Issues Found:**

**🎭 SIMULATION** (12 occurrences):
  - Line 25: `* 5. Key rotation simulation`
  - Line 30: `*   ./themis_demo_encryption mock`
  - Line 79: `std::cerr << "   Falling back to mock mode...\n\n";`
  - Line 80: `mode_ = "mock";`
  - Line 96: `std::cerr << "   Falling back to mock mode...\n\n";`

---

### `src/exporters/aql_predicate_filter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/exporters/arrow_ipc_exporter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/exporters/data_augmentation.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/exporters/export_encryption.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 363: `// it. Build a temporary config just for derivation.`

---

### `src/exporters/exporter_metrics.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/exporters/format_template.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/exporters/huggingface_exporter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/exporters/incremental_exporter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/exporters/jsonl_llm_exporter.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/exporters/parquet_exporter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/exporters/pii_detector.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 24: `// SSN pattern (XXX-XX-XXXX)`

---

### `src/exporters/stream_writer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/exporters/streaming_exporter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/geo/boost_cpu_exact_backend.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/geo/cpu_backend.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 888: `// Simple internal registry stub (no global linkage yet)`

---

### `src/geo/device_detector.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `// VRAM threshold checks.  The GPU backend stub uses these checks on start-up`

---

### `src/geo/geo_clustering.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/geo/geo_rtree.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/geo/gpu_backend_hip.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/geo/gpu_backend_production.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**📝 TODO** (2 occurrences):
  - Line 239: `// TODO v1.4.0: Complete CUDA implementation with geometry data processing`
  - Line 339: `// TODO v1.4.0: Complete OpenCL implementation`

---

### `src/geo/gpu_backend_stub.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 1: `// GPU spatial backend — replaces the original stub.`

---

### `src/geo/gpu_kernel_dispatcher_cpu.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 1: `// gpu_kernel_dispatcher_cpu.cpp — no-op stub for non-CUDA builds.`

---

### `src/geo/raster.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/geo/spatial_join.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/geo/temporal_spatial_query.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/geo/tile_server.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/governance/ccpa_rules.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/governance/compliance_reporter.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/governance/compliance_reporting.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/governance/cross_tenant_policy_inheritance.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/governance/data_lineage.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/governance/data_masker.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/governance/model_governance.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/governance/opa_adapter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/governance/pci_dss_rules.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/governance/policy_coordinator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/governance/policy_engine.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (91.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 466: `// If an OPA evaluator is configured, use it so that the simulation`
  - Line 477: `// OPA unavailable – fall through to native simulation.`
  - Line 542: `// NOTE: Dry-run / simulation mode – audit log is intentionally NOT written.`

---

### `src/governance/policy_file_watcher.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/governance/policy_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/governance/policy_manager_versioned.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/governance/policy_review.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (94.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 923: `// Simulate email sending – recipient is PII (email address), redact before logging.`
  - Line 935: `// Simulate webhook sending`

---

### `src/governance/policy_template.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/governance/policy_validation.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/governance/policy_validator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/governance/policy_version_history.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/governance/review_scheduler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/governance/soc2_controls.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/gpu/admin_api.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 120: `// POST /admin/gpu/simulate`

---

### `src/gpu/alerts.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/gpu/audit_log.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/gpu/cluster_coordinator.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/gpu/cluster_topology.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/gpu/config.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 2: `* GPU Config — validation and dry-run simulation.`

---

### `src/gpu/device_discovery.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/gpu/feature_flags.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/gpu/gpu_memory_manager_edition.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/gpu/gpu_module.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/gpu/graph_cache.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/gpu/kernel_validator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/gpu/launcher.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/gpu/load_balancer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/gpu/memory_pool.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 208: `// therefore always disjoint — no temporary buffer is needed.`

---

### `src/gpu/metrics.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/gpu/mig_manager.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 7: `* them all operations run against an in-memory CPU simulation registry so`

---

### `src/gpu/p2p_transfer.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (88.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 8: `* Without either define all operations use an in-memory CPU simulation path so`
  - Line 111: `// CPU simulation: no hardware P2P available.`
  - Line 143: `// on the CPU simulation path the array index serves as the ordinal).`
  - Line 347: `// CPU simulation path: no hardware P2P; succeed via memcpy so tests pass.`

---

### `src/gpu/policy.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/gpu/profiler.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/gpu/query_accelerator.cpp` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (66.0/100)

**Issues Found:**

**🔴 STUB** (5 occurrences):
  - Line 204: `// GPU path stub: when THEMIS_ENABLE_CUDA / THEMIS_ENABLE_HIP is defined,`
  - Line 251: `// GPU stub: would copy IDs + keys to device, run Thrust stable_sort_by_key,`
  - Line 299: `// GPU stub: would use cub::DeviceReduce.  CPU sequential path:`
  - Line 357: `// GPU stub: would use a parallel hash join kernel.  CPU path uses`
  - Line 419: `// GPU stub: would dispatch to cublasSgemv (FP32), cublasHgemm (FP16), or`

**🎭 SIMULATION** (5 occurrences):
  - Line 23: `// FP16 / BF16 quantisation helpers (CPU simulation of Tensor Core precision)`
  - Line 94: `/// Round-trip a float through FP16 to simulate Tensor Core precision loss.`
  - Line 116: `/// Round-trip a float through BF16 to simulate Tensor Core precision loss.`
  - Line 190: `// captured graph (CPU simulation: execute normally but note the cache hit).`
  - Line 420: `// cublasGemmEx with CUDA_R_16BF (BF16).  CPU simulation path below.`

---

### `src/gpu/rocm_backend.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 254: `// executes in the no-HIP simulation path used by unit tests.`

---

### `src/gpu/safe_fail.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/gpu/stream_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 98: `// createCudaStream — CUDA stream creation (resolves Stubs: 1)`

---

### `src/gpu/tensor_buffer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/gpu/time_slice_scheduler.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/gpu/training_loop.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/gpu/unified_memory.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/gpu/vulkan_backend.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/gpu/wasm_kernel_sandbox.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (88.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 12: `* current build the CPU simulation path is used: the kernel blob is passed`
  - Line 144: `// Step 5–6: Execute in sandbox (CPU simulation) with timeout enforcement.`
  - Line 178: `// result.  For now fall through to the CPU simulation path even when the`
  - Line 183: `// CPU simulation: run the backend in an async future and apply a timeout.`

---

### `src/graph/distributed_graph.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 286: `spdlog::debug("distributed_graph: k-hop BFS shard returned error, skipping");`

---

### `src/graph/gpu_traversal.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/graph/graph_query_optimizer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/graph/parallel_traversal.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/graph/path_constraints.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/importers/conflict_resolver.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/importers/flatfile_importer.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/importers/kafka_importer.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (88.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 4: `// missing build flag.  The mock injection path (setMessageFetchForTesting) and`
  - Line 196: `"dry_run: {})", topic, brokers.empty() ? "<mock>" : brokers,`
  - Line 357: `// Mock-based consume loop`
  - Line 423: `"Exception in Kafka mock consume: " + std::string(e.what()));`

---

### `src/importers/mongo_importer.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/importers/mysql_importer.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🔒 HARDCODED** (2 occurrences):
  - Line 528: `// Match: CREATE [TEMPORARY] TABLE [`]table_name[`] (`
  - Line 531: `R"(CREATE\s+(?:TEMPORARY\s+)?TABLE\s+(?:(?:`([^`]+)`|(\w+))\.)?(?:`([^`]+)`|(\w+))\s*\()",`

---

### `src/importers/oracle_importer.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/importers/postgres_importer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/importers/s3_importer.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (93.0/100)

**Issues Found:**

**🔒 HARDCODED** (3 occurrences):
  - Line 390: `// Write to a temporary in-memory stream and probe via FlatFileImporter.`
  - Line 499: `// Write to a temporary file so FlatFileImporter can detect the format from`
  - Line 517: `"Failed to write temporary file: " + tmp_path,`

---

### `src/importers/schema_validator.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/importers/sqlite_importer.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 479: `// Match: CREATE [TEMP|TEMPORARY] TABLE [IF NOT EXISTS]`

---

### `src/index/adaptive_index.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 396: `// TODO: Check actual index registry`

---

### `src/index/advanced_vector_index.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (90.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 19: `// Stub definitions for non-FAISS builds`
  - Line 124: `THEMIS_WARN("FAISS not available - using stub implementation");`

---

### `src/index/ann_index.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/index/approximate_radius_search.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/index/binary_quantizer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/index/distributed_vector_index.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/index/edge_types.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/index/gnn_embeddings.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/index/gpu_vector_index.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/index/gpu_vector_index_vulkan.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 866: `// Stub implementations when Vulkan is not available`

**🐛 DEBUG** (2 occurrences):
  - Line 110: `// Initialize with device ID and validation (debug mode only)`
  - Line 114: `enableValidation = true; // Force validation in debug`

---

### `src/index/graph_analytics.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/index/graph_auto_buffer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/index/graph_index.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/index/hnsw_layer_optimizer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/index/hnsw_parameter_tuner.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 508: `// TODO: Could use sysconf(_SC_LEVEL1_DCACHE_LINESIZE) on Linux`

---

### `src/index/hnsw_production_defaults.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 408: `spdlog::debug("HNSW: Adapted ef_search {} -> {} (latency: {:.2f}ms, recall: {:.3f})",`

---

### `src/index/index_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/index/inverted_index.cpp` (v0.0.12)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/index/learnable_rope.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/index/learned_quantizer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 331: `// TODO: Optimize by computing distance directly from codes/centroids without full decoding`

---

### `src/index/lora_rope.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/index/multi_gpu_vector_index.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**📝 TODO** (2 occurrences):
  - Line 364: `// TODO: Implement parallel batch processing across GPUs`
  - Line 412: `perGPUStat.utilizationPercent = 0.0;  // TODO: Implement utilization tracking`

---

### `src/index/multi_vector_search.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/index/process_graph.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 1626: `// Multi-Model Query Stubs`
  - Line 1783: `// Private helper stubs`

---

### `src/index/product_quantizer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/index/property_graph.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 30: `// TODO: Extend BaseEntity to support string arrays`

---

### `src/index/residual_quantizer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/index/rotary_embeddings.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/index/rotary_embeddings_gpu_cpu.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/index/rotary_embeddings_hip.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/index/secondary_index.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/index/spatial_index.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/index/temporal_graph.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/index/tiered_index_manager.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/index/vector_auto_buffer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/index/vector_index.cpp` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (68.0/100)

**Issues Found:**

**🔒 HARDCODED** (8 occurrences):
  - Line 2135: `// 1. Save to temporary file first`
  - Line 2139: `// 2. Load temporary file into memory`
  - Line 2143: `return Status::Error("saveIndex: Failed to read temporary index file");`
  - Line 2170: `// 5. Remove temporary file`
  - Line 2252: `// 3. Write to temporary file for hnswlib`

---

### `src/index/workload_replay.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 133: `spdlog::debug("WorkloadReplayer: fed {} events and {} queries into IndexRecommender",`

---

### `src/ingestion/api_connector.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/ingestion/cdc_connector.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (89.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 331: `// gated stub so that the connector can be wired into the build without`

**🎭 SIMULATION** (5 occurrences):
  - Line 5: `//   - uses injected mock functions (unit tests).`
  - Line 180: `if (event_fetch_fn_) return true; // test mock always available`
  - Line 210: `// Test mock path: no replication driver required`
  - Line 281: `// Mock-based ingestion (unit tests)`
  - Line 309: `"Exception in CdcConnector mock ingest: " +`

---

### `src/ingestion/database_connector.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (92.0/100)

**Issues Found:**

**🎭 SIMULATION** (6 occurrences):
  - Line 4: `//   - uses injected mock functions (unit tests).`
  - Line 314: `if (row_fetch_fn_) return true; // test mock always available`
  - Line 353: `if (row_fetch_fn_) return 0; // not known from mock`
  - Line 408: `// Test mock path: no ODBC required`
  - Line 436: `// Mock-based ingestion (unit tests)`

---

### `src/ingestion/filesystem_ingester.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/ingestion/huggingface_connector.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 13: `// preserve the existing stub structure (real libcurl replacement is tracked`

---

### `src/ingestion/ingestion_coordinator.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 460: `IngestionStats dummy;`
  - Line 461: `dummy.addError(IngestionErrorCode::SOURCE_UNAVAILABLE,`
  - Line 465: `report.source_stats["__coordinator__"] = dummy;`

---

### `src/ingestion/ingestion_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 1867: `// NOTE: the failure branch below is unreachable in this stub but`

---

### `src/ingestion/kafka_connector.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (86.0/100)

**Issues Found:**

**🎭 SIMULATION** (5 occurrences):
  - Line 4: `// injected test mock is used (for unit tests that do not require a broker).`
  - Line 109: `// When a test mock is set, always report available.`
  - Line 156: `// Test mock path: no librdkafka required`
  - Line 184: `// Mock-based ingestion (unit tests)`
  - Line 221: `"Exception in Kafka mock ingest: " + std::string(e.what()),`

---

### `src/ingestion/object_storage_connector.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (92.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 5: `//   - uses injected mock functions (unit tests).`
  - Line 183: `// Test mock path: no cloud SDK required`
  - Line 251: `// Mock-based ingestion (unit tests)`
  - Line 306: `"Exception in ObjectStorage mock ingest: " +`

---

### `src/ingestion/web_crawler_connector.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 4: `//   - uses injected mock functions (unit tests).`
  - Line 559: `// Use mock if injected`

---

### `src/llm/adapter_load_balancer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/adapter_registry.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (94.0/100)

**Issues Found:**

**🐛 DEBUG** (8 occurrences):
  - Line 85: `spdlog::debug("AdapterRegistry: registered adapter '{}'", metadata.adapter_id);`
  - Line 111: `spdlog::debug("AdapterRegistry: updated adapter '{}'", metadata.adapter_id);`
  - Line 123: `spdlog::debug("AdapterRegistry: deleted adapter '{}'", adapter_id);`
  - Line 240: `spdlog::debug("AdapterRegistry: signed adapter '{}'", adapter_id);`
  - Line 248: `spdlog::debug("AdapterRegistry::verifySignature: no signature for '{}'", adapter_id);`

---

### `src/llm/adaptive_vram_allocator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (85.0/100)

**Issues Found:**

**🔴 STUB** (3 occurrences):
  - Line 105: `// Stub implementation - would integrate with actual GPU allocator`
  - Line 117: `*ptr = nullptr;  // Stub`
  - Line 123: `// Stub implementation - recovery strategies:`

---

### `src/llm/ai_decision_auditor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/ai_orchestrator.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 470: `// TODO(extensible): parse tool calls from result.text using react_agent`

**🐛 DEBUG** (4 occurrences):
  - Line 33: `spdlog::debug("[ToolRegistry] Registered tool '{}'", spec.name);`
  - Line 177: `spdlog::debug("[AIOrchestrator] run() mode='{}' query_len={}", mode.id, ctx.query.size());`
  - Line 472: `spdlog::debug("[AIOrchestrator] agentic mode: tool-loop extension point reached");`
  - Line 516: `spdlog::debug("[AIOrchestrator] multi_agent mode: peer-broadcast extension point "`

---

### `src/llm/applications/themis_help_lora.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (94.0/100)

**Issues Found:**

**📝 TODO** (2 occurrences):
  - Line 114: `// TODO: Get model path from LLMModelStorage`
  - Line 620: `// TODO: In a production system, implement proper version history tracking`

**🐛 DEBUG** (2 occurrences):
  - Line 161: `spdlog::debug("LLM inference completed: {} tokens in {:.2f}ms",`
  - Line 266: `spdlog::debug("Positive feedback added for question: {} (user: {})", question, user_id);`

---

### `src/llm/aql_train_parser.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/async_inference_engine.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (89.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 347: `// TODO: Properly encode RAG context in request`

**🐛 DEBUG** (9 occurrences):
  - Line 234: `spdlog::debug("Submitted inference request {} (priority={}, via_pool={})",`
  - Line 323: `spdlog::debug("Submitted async inference request {} (callback mode, via_pool={})",`
  - Line 340: `spdlog::debug("Submitting RAG request with {} documents",`
  - Line 506: `spdlog::debug("Skipping cancelled request: {}",`
  - Line 529: `spdlog::debug("Worker {} processing request {}",`

---

### `src/llm/attention/flash_attention.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/attention/kv_cache_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/block_table.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/byzantine_detector.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 190: `spdlog::debug("Byzantine detection: MAD too small, skipping detection");`

---

### `src/llm/constitutional_reasoning_engine.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/continuous_batch_scheduler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (91.0/100)

**Issues Found:**

**🐛 DEBUG** (9 occurrences):
  - Line 100: `spdlog::debug("Request submitted: {} (priority: {}, tokens: ~{})",`
  - Line 155: `spdlog::debug("Request {} reprioritized to {}",`
  - Line 257: `spdlog::debug("Scheduled batch: {} requests ({} tokens, {} ms)",`
  - Line 313: `spdlog::debug("Request completed: {} ({} tokens generated)",`
  - Line 357: `spdlog::debug("Request preempted: {} (count: {})",`

---

### `src/llm/distributed_training_coordinator.cpp` (v0.0.33)

**Maturity Level:** 🟠 BETA (55.0/100)

**Issues Found:**

**🎭 SIMULATION** (10 occurrences):
  - Line 853: `// Create dummy gradient for testing/standalone mode`
  - Line 855: `GradientTensor dummy;`
  - Line 856: `dummy.layer_name = "test_layer";`
  - Line 857: `dummy.source_shard = shard_id;`
  - Line 858: `dummy.step_number = step_number;`

**🐛 DEBUG** (15 occurrences):
  - Line 767: `spdlog::debug("Step {} aggregated loss: {:.6f}", current_step_, result.aggregated_loss.value());`
  - Line 845: `spdlog::debug("Collecting gradients from {} shards for step {}",`
  - Line 879: `spdlog::debug("Shard {} simulated loss: {:.6f}", shard_id, shard_loss);`
  - Line 919: `spdlog::debug("Collected {} gradients from {}", shard_grads.size(), shard_id);`
  - Line 1011: `spdlog::debug("Aggregating gradients from {} shards using {}",`

---

### `src/llm/docs_assistant.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 229: `// std::string answer = THEMIS_LLM_COMPLETE(prompt.str());  // TODO: Undefined macro`

---

### `src/llm/embedded_llm.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/ethical_guidelines_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/ethics_aware_confidence_detector.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/explanation_generator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/feedback_plugin_basic.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/feedback_store.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**📝 TODO** (2 occurrences):
  - Line 519: `// TODO: In production, load these from a configuration file or database`
  - Line 628: `// TODO(feedback-plugin): Apply modifications if provided`

---

### `src/llm/fewshot_optimizer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/gguf_loader.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (94.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 252: `std::string dummy;`
  - Line 253: `if (!readMetadataValue(offset, elem_type, dummy)) {`

---

### `src/llm/gpu_memory_manager.cpp` (v0.0.33)

**Maturity Level:** 🟠 BETA (57.0/100)

**Issues Found:**

**🎭 SIMULATION** (15 occurrences):
  - Line 248: `spdlog::warn("Running in CPU-only mode (simulation)");`
  - Line 250: `// Fallback to simulation mode`
  - Line 257: `// Simulation mode when CUDA is not enabled at build time`
  - Line 261: `// Initialize multi-GPU support in simulation mode (v1.4.0)`
  - Line 263: `spdlog::info("Initializing multi-GPU support (simulation) with {} GPUs", config_.gpu_devices.size())`

**🐛 DEBUG** (6 occurrences):
  - Line 367: `spdlog::debug("Allocated {} MB VRAM for model {} (total: {} MB)",`
  - Line 428: `spdlog::debug("Allocated {} MB RAM ({}) for model {} (total: {} MB)",`
  - Line 649: `spdlog::debug("Fragmentation low ({}%), skipping defragmentation", initial_frag);`
  - Line 705: `spdlog::debug("No fragmented models found, defragmentation skipped");`
  - Line 788: `spdlog::debug("Consolidated {} GPU allocations for model {} on device {} into single {} MB block",`

---

### `src/llm/gpu_safe_fail.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 36: `spdlog::debug("GPU circuit open for '{}', using CPU fallback", operation_name);`

---

### `src/llm/grafana_metrics.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (83.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 1340: `// POST /admin/prompt/simulate`
  - Line 1510: `R"({"status":"not_implemented","message":"No simulate callback registered. Wire setSimulateCallback(`
  - Line 1521: `// POST /admin/prompt/simulate — dry-run policy check + tokenization.`

**🐛 DEBUG** (8 occurrences):
  - Line 22: `spdlog::debug("PrometheusExporter initialized");`
  - Line 30: `spdlog::debug("Metric registered: {} (type: {})", def.name, static_cast<int>(def.type));`
  - Line 204: `spdlog::debug("Metrics reset");`
  - Line 238: `spdlog::debug("LLMMetricsCollector initialized");`
  - Line 244: `spdlog::debug("LLMMetricsCollector initialized (lock_contention_threshold_ms={})",`

---

### `src/llm/grammar.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/grammar_cache.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (94.0/100)

**Issues Found:**

**🐛 DEBUG** (6 occurrences):
  - Line 14: `spdlog::debug("GrammarCache initialized with max_cached_grammars={}, enabled={}",`
  - Line 27: `spdlog::debug("Grammar cache HIT for '{}'", name);`
  - Line 31: `spdlog::debug("Grammar cache MISS for '{}'", name);`
  - Line 55: `spdlog::debug("Cached grammar '{}' (cache size: {})", name, cache_.size());`
  - Line 62: `spdlog::debug("Grammar cache cleared");`

---

### `src/llm/inference_engine_enhanced.cpp` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (72.0/100)

**Issues Found:**

**📝 TODO** (3 occurrences):
  - Line 469: `// TODO: In production, implement actual cache prewarming:`
  - Line 1105: `// TODO: In production, compute embeddings for similarity-based cache lookup`
  - Line 1143: `// TODO: In production, compute actual embeddings and KV cache`

**🐛 DEBUG** (22 occurrences):
  - Line 244: `spdlog::debug("unloadLoRAAdapter: '{}' not registered", adapter_id);`
  - Line 267: `spdlog::debug("LoRA adapter '{}' was not loaded on model '{}'",`
  - Line 433: `spdlog::debug("Cancelled request: {}", request_id);`
  - Line 447: `spdlog::debug("Reprioritized request {} to priority {}", request_id, new_priority);`
  - Line 478: `spdlog::debug("  Prewarming: {}", prompt.substr(0, 50));`

---

### `src/llm/inference_handle.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/json_schema_converter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🐛 DEBUG** (2 occurrences):
  - Line 222: `spdlog::debug("JsonSchemaConverter::schemaToEbnf: empty/null schema, returning default grammar");`
  - Line 346: `spdlog::debug("JsonSchemaConverter::parseToolCall: JSON parse failed: {}", e.what());`

---

### `src/llm/kernel_fusion.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/kv_cache_buffer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (92.0/100)

**Issues Found:**

**🔒 HARDCODED** (2 occurrences):
  - Line 188: `// No buffers available - create temporary one (pool exhausted)`
  - Line 204: `// Buffer not from pool (was temporary) - just let it be destroyed`

---

### `src/llm/llama_grammar_adapter.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 19: `// support and uses the real functions when available, falling back to stub`

---

### `src/llm/llama_lora_adapter.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (94.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 19: `// and uses the real functions when available, falling back to stub`
  - Line 154: `* @note The signature matches the legacy stub for backward compatibility.`

**🐛 DEBUG** (11 occurrences):
  - Line 258: `spdlog::debug("Applying LoRA adapter with scale: {}", scale);`
  - Line 264: `spdlog::debug("✓ LoRA adapter applied successfully");`
  - Line 296: `spdlog::debug("Removing LoRA adapter from context");`
  - Line 302: `spdlog::debug("✓ LoRA adapter removed successfully");`
  - Line 329: `spdlog::debug("Clearing all LoRA adapters from context");`

---

### `src/llm/llama_resource_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (87.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 2: `// #include "acceleration/backend_registry.h"  // TODO: Missing file`

**🐛 DEBUG** (11 occurrences):
  - Line 31: `spdlog::debug("Destroying LlamaModelHandle");`
  - Line 48: `spdlog::debug("Model freed");`
  - Line 97: `spdlog::debug("Destroying LlamaContextHandle");`
  - Line 114: `spdlog::debug("Context freed");`
  - Line 121: `spdlog::debug("KV cache clear requested (not available in this llama.cpp version)");`

---

### `src/llm/llama_wrapper.cpp` (v0.0.33)

**Maturity Level:** 🔴 ALPHA (38.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 2681: `// TODO: Complete integration with llama_batch for true multi-modal inference.`

**🐛 DEBUG** (28 occurrences):
  - Line 589: `spdlog::debug("Temp models directory does not exist: {}", temp_dir.string());`
  - Line 606: `spdlog::debug("Removed old temp model file: {}", entry.path().filename().string());`
  - Line 699: `spdlog::debug("Generating response for prompt: {} (max_tokens={})",`
  - Line 704: `spdlog::debug("Using speculative decoding");`
  - Line 752: `spdlog::debug("Cache hit for prompt: {}", request.prompt.substr(0, 50));`

**🔒 HARDCODED** (8 occurrences):
  - Line 476: `// Step 4: Write model data to temporary file`
  - Line 477: `spdlog::info("Step 4: Writing model to temporary file...");`
  - Line 479: `// Create temporary directory for model cache`
  - Line 497: `spdlog::error("Failed to create temporary model file: {}", temp_model_path.string());`
  - Line 505: `spdlog::error("Temporary model file was not created: {}", temp_model_path.string());`

---

### `src/llm/llamacpp_inference_engine.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (93.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 286: `"stub_response",`

**🐛 DEBUG** (2 occurrences):
  - Line 14: `spdlog::debug("LLMOutputValidator initialized (min_len: {}, max_len: {}, require_utf8: {})",`
  - Line 334: `spdlog::debug("Detected repeating pattern (len={}, count={}): {}",`

---

### `src/llm/llm_deployment_plugin.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (92.0/100)

**Issues Found:**

**📝 TODO** (4 occurrences):
  - Line 113: `LOG_INFO("LLM Deployment Plugin: Filesystem-only mode (BaseEntity storage TODO)");`
  - Line 152: `audit.user = "system"; // TODO(feature): Implement user context tracking for audit compliance`
  - Line 250: `// Store in BaseEntity storage (RocksDB) - TODO: Uncomment when llm_model_storage.cpp exists`
  - Line 893: `// TODO(enhancement): In the future, this could check if model_id exists`

---

### `src/llm/llm_interaction_store.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/llm_model_audit_logger.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (7 occurrences):
  - Line 152: `spdlog::debug("LLMModelAuditLogger initialized: log_path='{}'",`
  - Line 188: `spdlog::debug("LLM inference audit model={} request={}",`
  - Line 203: `spdlog::debug("LLM model event model={} event={}",`
  - Line 214: `spdlog::debug("LLM model lifecycle model={} version={}", model_id, version);`
  - Line 228: `spdlog::debug("LLM fine-tuning model={} base={} samples={} loss={}",`

---

### `src/llm/llm_model_storage.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 42: `// Use configured key provider (Vault/HSM) or fallback to Mock`

**🐛 DEBUG** (1 occurrences):
  - Line 512: `spdlog::debug("Model {} marked for deletion (key: {})", model_id, key);`

---

### `src/llm/llm_plugin_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 355: `// TODO: integrate with actual cache implementations when available`

---

### `src/llm/llm_prefix_cache.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 293: `spdlog::debug("LLMPrefixCache '{}' initialised (KV caching: {})",`

---

### `src/llm/llm_response_cache.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/llm_security_utils.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/lora_framework/adapter_consistency_checker.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/lora_framework/adapter_sync_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (2 occurrences):
  - Line 328: `spdlog::debug("Discovered {} peer shards", peer_ids.size());`
  - Line 347: `spdlog::debug("Starting periodic sync");`

---

### `src/llm/lora_framework/adaptive_batcher.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (91.0/100)

**Issues Found:**

**🐛 DEBUG** (9 occurrences):
  - Line 36: `spdlog::debug("No memory manager, using current batch size: {}", current_batch_size_);`
  - Line 44: `spdlog::debug("Computing optimal batch size:");`
  - Line 45: `spdlog::debug("  Available VRAM: {:.2f} GB", available_vram / (1024.0 * 1024.0 * 1024.0));`
  - Line 46: `spdlog::debug("  Target VRAM ({}% margin): {:.2f} GB",`
  - Line 56: `spdlog::debug("  Per-sample memory: {:.2f} MB", per_sample_memory / (1024.0 * 1024.0));`

---

### `src/llm/lora_framework/axolotl_bridge.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 404: `logger.debug(result.stdout)`

---

### `src/llm/lora_framework/base_model_adapter.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 619: `spdlog::debug("Created LoRA adapter for: {} ({}x{}, rank={})",`

---

### `src/llm/lora_framework/custom_allreduce.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (88.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 117: `// Simple barrier: allreduce with dummy data`
  - Line 118: `GPUTensor dummy({1}, ctx_.get_device(rank_));`
  - Line 119: `dummy.fill(0.0f);`
  - Line 120: `allreduce(dummy, false);`

---

### `src/llm/lora_framework/data_loader.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 414: `spdlog::debug("Dataset shuffled");`

---

### `src/llm/lora_framework/directx_buffer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/lora_framework/directx_context.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🐛 DEBUG** (2 occurrences):
  - Line 76: `// Enable debug layer in debug builds`
  - Line 237: `std::cout << "DirectX 12 debug layer enabled\n";`

---

### `src/llm/lora_framework/directx_descriptors.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/lora_framework/directx_pipeline.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/lora_framework/directx_shader.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/lora_framework/distributed_dataloader.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 96: `// TODO: Properly concatenate all samples`

---

### `src/llm/lora_framework/distributed_trainer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (94.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 180: `// For Phase 1, we simulate by averaging (assumes gradients already aggregated)`

**📝 TODO** (1 occurrences):
  - Line 186: `// TODO: When GPU support is added, replace with:`

**🐛 DEBUG** (1 occurrences):
  - Line 139: `spdlog::debug("Barrier synchronization (rank {})", config_.rank);`

---

### `src/llm/lora_framework/embedding_provider.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🐛 DEBUG** (2 occurrences):
  - Line 260: `spdlog::debug("Cache file not found: {}", filepath);`
  - Line 398: `spdlog::debug("Evicted {} old cache entries", to_remove);`

---

### `src/llm/lora_framework/feedback_plugin.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/lora_framework/flash_lora.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 60: `spdlog::debug("FlashLoRA auto-tuned for {}: tile_m={}, tile_k={}",`

---

### `src/llm/lora_framework/gguf_converter.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/lora_framework/gpu_data_loader.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🐛 DEBUG** (3 occurrences):
  - Line 90: `spdlog::debug("Shuffled {} samples", indices_.size());`
  - Line 185: `spdlog::debug("Started prefetch thread");`
  - Line 208: `spdlog::debug("Stopped prefetch thread");`

---

### `src/llm/lora_framework/gpu_embedding_layer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (88.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 117: `// TODO: Add integer tensor support to GPUTensor to avoid this conversion`

**🐛 DEBUG** (10 occurrences):
  - Line 44: `spdlog::debug("GPUEmbeddingLayer: Uploaded {} MB to GPU",`
  - Line 100: `spdlog::debug("GPUEmbeddingLayer::forwardCPU: batch_size={}, seq_len={}", batch_size, seq_len);`
  - Line 156: `spdlog::debug("GPUEmbeddingLayer::forwardCUDA: batch_size={}, seq_len={}", batch_size, seq_len);`
  - Line 178: `spdlog::debug("CUDA embedding lookup completed successfully");`
  - Line 193: `spdlog::debug("GPUEmbeddingLayer::forwardHIP: batch_size={}, seq_len={}", batch_size, seq_len);`

---

### `src/llm/lora_framework/gpu_lora_layers.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**🐛 DEBUG** (5 occurrences):
  - Line 55: `spdlog::debug("GPULoRALayer created: in_dim={}, out_dim={}, rank={}, scaling={}, device={}, fused={}`
  - Line 213: `spdlog::debug("Recomputing activations for checkpointed layer {}", layer_id_);`
  - Line 408: `spdlog::debug("GPULoRALayer moved to device: {}", static_cast<int>(device_.type));`
  - Line 430: `spdlog::debug("GPUSGDOptimizer created: lr={}, momentum={}, weight_decay={}",`
  - Line 447: `spdlog::debug("GPUSGDOptimizer: {} parameters registered", parameters_.size());`

---

### `src/llm/lora_framework/gpu_memory.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/lora_framework/gpu_tensor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (88.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 303: `// Convert to FP16 (simulate precision loss)`
  - Line 310: `// Convert to BF16 (simulate precision loss)`

**📝 TODO** (3 occurrences):
  - Line 326: `// TODO: Implement GPU dtype conversion kernels`
  - Line 339: `// TODO: Implement GPU dtype conversion kernels for HIP`
  - Line 924: `// TODO: Tensor type forward-declared but not defined - functions disabled for now`

---

### `src/llm/lora_framework/gpu_training_loop.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (86.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 510: `// TODO: Refactor to use references or unique_ptr`

**🐛 DEBUG** (10 occurrences):
  - Line 96: `spdlog::debug("Added GPU LoRA layer: {} parameters", layer->parameter_count());`
  - Line 365: `spdlog::debug("Enabled checkpointing for layer {}", i);`
  - Line 407: `spdlog::debug("Optimal batch size: {} (current: {})",`
  - Line 443: `spdlog::debug("Epoch {}/{}, Step {}/{}, Loss: {:.6f}",`
  - Line 578: `spdlog::debug("GPU gradient unscaling completed (scale: {})",`

**🔒 HARDCODED** (2 occurrences):
  - Line 791: `// Allocate temporary buffer for partial sums`
  - Line 981: `// Allocate temporary buffer for partial sums`

---

### `src/llm/lora_framework/gpu_utilization_monitor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (88.0/100)

**Issues Found:**

**📝 TODO** (2 occurrences):
  - Line 339: `// TODO: Implement VK_EXT_memory_budget query when available`
  - Line 378: `// TODO: Implement DXGI adapter memory query`

**🐛 DEBUG** (8 occurrences):
  - Line 211: `spdlog::debug("NVML not available (CUDA not enabled)");`
  - Line 250: `spdlog::debug("NVML metrics: GPU={:.1f}%, Memory={:.1f}%",`
  - Line 270: `spdlog::debug("ROCm SMI not available (HIP not enabled)");`
  - Line 304: `spdlog::debug("ROCm SMI metrics: GPU={:.1f}%, Memory={:.1f}%",`
  - Line 320: `spdlog::debug("Vulkan performance monitoring not available (Vulkan not enabled)");`

---

### `src/llm/lora_framework/gradient_checkpointing.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**🐛 DEBUG** (5 occurrences):
  - Line 91: `spdlog::debug("Saved checkpoint for layer {} ({} bytes)",`
  - Line 119: `spdlog::debug("Recomputed activation for layer {} in {}ms",`
  - Line 130: `spdlog::debug("Cleared checkpoint for layer {}", layer_id);`
  - Line 139: `spdlog::debug("Cleared all checkpoints");`
  - Line 144: `spdlog::debug("Added custom checkpoint for layer {}", layer_id);`

---

### `src/llm/lora_framework/gradient_utils.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🐛 DEBUG** (2 occurrences):
  - Line 49: `spdlog::debug("Clipped gradients: norm={:.4f} -> {:.4f}", global_norm, max_norm);`
  - Line 69: `spdlog::debug("Clipped gradients by value: max={:.4f}", clip_value);`

---

### `src/llm/lora_framework/kernels/cpu_fused_kernels.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/lora_framework/kernels/directx_kernels.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (89.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 860: `// Non-Windows stub implementations`

**🎭 SIMULATION** (2 occurrences):
  - Line 552: `// Dummy buffers for unused outputs`
  - Line 647: `// Dummy buffers for unused outputs`

**📝 TODO** (1 occurrences):
  - Line 141: `// TODO: Load and compile shaders`

---

### `src/llm/lora_framework/kernels/hip_fused_kernels.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/lora_framework/kernels/hip_kernels.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 412: `// TODO: For better performance, consider reusing a pre-allocated device buffer`

---

### `src/llm/lora_framework/kernels/vulkan_kernels.cpp` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (73.0/100)

**Issues Found:**

**🎭 SIMULATION** (10 occurrences):
  - Line 363: `// Bind dummy buffer to binding 1 (required by layout)`
  - Line 405: `// Bind dummy buffer to binding 1 (required by layout)`
  - Line 459: `pipeline->bind_buffer(1, buf_h); // dummy B`
  - Line 460: `pipeline->bind_buffer(2, buf_h); // dummy A`
  - Line 463: `pipeline->bind_buffer(5, buf_grad_A); // dummy grad_B`

**🐛 DEBUG** (1 occurrences):
  - Line 108: `// Initialize context with validation layers in debug builds`

---

### `src/llm/lora_framework/llama_tokenizer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/lora_framework/lora_audit_logger.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 85: `spdlog::debug("Logged inference: model={}, adapter={}, request={}",`

---

### `src/llm/lora_framework/lora_feedback_storage.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (94.0/100)

**Issues Found:**

**🐛 DEBUG** (6 occurrences):
  - Line 82: `spdlog::debug("Created feedback {} for adapter {}", feedback.id, feedback.adapter_id);`
  - Line 184: `spdlog::debug("Updated feedback {}", id);`
  - Line 212: `spdlog::debug("Deleted feedback {}", id);`
  - Line 401: `spdlog::debug("Created graph link: {} --[{}]--> {}", from, edge_type, to);`
  - Line 424: `spdlog::debug("Removed graph link: {} --[{}]--> {}", from, edge_type, to);`

---

### `src/llm/lora_framework/lora_layers.cpp` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (75.0/100)

**Issues Found:**

**🐛 DEBUG** (25 occurrences):
  - Line 9: `inline void debug(const char*, ...) {}`
  - Line 214: `spdlog::debug("{}: Initialized with {} parameters", name_, parameter_count());`
  - Line 218: `spdlog::debug("{}: forward with input shape ({}, {})",`
  - Line 235: `spdlog::debug("{}: backward with grad_output shape ({}, {})",`
  - Line 290: `spdlog::debug("{}: Weights updated", name_);`

---

### `src/llm/lora_framework/lora_orchestrator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/lora_framework/lora_provenance.cpp` (v0.0.26)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (4 occurrences):
  - Line 279: `spdlog::debug("LoRAProvenanceManager: stored provenance for '{}'", adapter_id);`
  - Line 366: `spdlog::debug("LoRAProvenanceManager: stored external provenance for '{}'",`
  - Line 409: `spdlog::debug("LoRAProvenanceManager: created snapshot '{}' for '{}'",`
  - Line 459: `spdlog::debug("LoRAProvenanceManager: appended audit entry '{}' for '{}'",`

---

### `src/llm/lora_framework/lora_storage_service.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/lora_framework/lora_storage_service_themisdb.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 560: `* @brief Create mock key provider (development/testing only)`
  - Line 561: `* @return Shared pointer to mock key provider`
  - Line 575: `* Priority order: HSM > Vault > PKI > Mock`

**🐛 DEBUG** (4 occurrences):
  - Line 151: `spdlog::debug("Deleted blob {} for adapter {}",`
  - Line 674: `spdlog::debug("Encrypted adapter data with key version {}", encrypted.key_version);`
  - Line 695: `spdlog::debug("Created security signature for adapter");`
  - Line 725: `spdlog::debug("Decrypted adapter data (key version: {})", encrypted_blob.key_version);`

**🔒 HARDCODED** (1 occurrences):
  - Line 294: `// Phase 1: copy the versioned snapshot to a temporary directory.`

---

### `src/llm/lora_framework/lora_training_config.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/lora_framework/lora_training_service.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (93.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 700: `// TODO: In future PRs, replace with real text data processing:`

**🐛 DEBUG** (8 occurrences):
  - Line 586: `spdlog::debug("Using real embeddings from base model");`
  - Line 652: `spdlog::debug("Base model not available, using hash-based embeddings");`
  - Line 812: `spdlog::debug("Epoch {}/{}, Step {}, Loss: {:.4f}",`
  - Line 918: `spdlog::debug("Registered training callback");`
  - Line 927: `spdlog::debug("No training in progress to stop");`

**🔒 HARDCODED** (1 occurrences):
  - Line 699: `// Create synthetic training batch (TEMPORARY - Phase 1 only)`

---

### `src/llm/lora_framework/lr_scheduler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 313: `spdlog::debug("Creating LR scheduler: type={}", static_cast<int>(config.type));`

---

### `src/llm/lora_framework/mixed_precision.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (85.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 30: `// Simulate FP16/BF16 conversion on CPU`
  - Line 35: `// Simulate FP16 precision loss`
  - Line 173: `// For CPU simulation, we just clamp to FP16 range`
  - Line 182: `// No conversion needed for CPU simulation`

**📝 TODO** (1 occurrences):
  - Line 174: `// TODO: In GPU implementation, convert to actual FP16 format:`

**🐛 DEBUG** (1 occurrences):
  - Line 136: `spdlog::debug("Increasing loss scale to {}", current_loss_scale_);`

---

### `src/llm/lora_framework/model_compatibility.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 223: `spdlog::debug("SafeTensors header: {}", header.dump());`

---

### `src/llm/lora_framework/multi_gpu.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/lora_framework/multi_gpu_lora_layer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/lora_framework/multi_gpu_trainer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/lora_framework/nccl_backend.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (88.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 216: `// NCCL doesn't have explicit barrier, use allreduce with dummy data`
  - Line 219: `float dummy = 0.0f;`
  - Line 223: `&dummy,`
  - Line 224: `&dummy,`

---

### `src/llm/lora_framework/paged_memory_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/lora_framework/paged_optimizer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 104: `// If no paged buffers, allocate temporary ones`

---

### `src/llm/lora_framework/quantization.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (89.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 6: `// Minimal spdlog stubs for testing without dependencies`

**🐛 DEBUG** (6 occurrences):
  - Line 9: `inline void debug(const char*, Args&&...) {}`
  - Line 88: `spdlog::debug("Quantizing to NF4: {} elements, {} blocks, block_size={}",`
  - Line 141: `spdlog::debug("NF4 quantization complete: {} bytes", output.memory_bytes());`
  - Line 151: `spdlog::debug("Quantizing to INT8: {} elements, {} blocks, block_size={}",`
  - Line 195: `spdlog::debug("INT8 quantization complete: {} bytes", output.memory_bytes());`

---

### `src/llm/lora_framework/quantized_model.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (91.0/100)

**Issues Found:**

**🐛 DEBUG** (9 occurrences):
  - Line 10: `inline void debug(const char*, Args&&...) {}`
  - Line 45: `spdlog::debug("Quantized layer weights: {} -> {} bytes ({:.1f}% reduction)",`
  - Line 56: `spdlog::debug("Created QuantizedLayerWeights from pre-quantized tensor: {} bytes",`
  - Line 80: `spdlog::debug("Adding layer '{}' to quantized model", layer_name);`
  - Line 86: `spdlog::debug("Adding pre-quantized layer '{}' to quantized model", layer_name);`

---

### `src/llm/lora_framework/rccl_backend.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (88.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 216: `// RCCL doesn't have explicit barrier, use allreduce with dummy data`
  - Line 219: `float dummy = 0.0f;`
  - Line 223: `&dummy,`
  - Line 224: `&dummy,`

---

### `src/llm/lora_framework/resource_profiler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (90.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 66: `// Stubbed: callbacks not executed in this minimal implementation`
  - Line 86: `// Stubbed: no file export in minimal build`

---

### `src/llm/lora_framework/sequence_packer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🐛 DEBUG** (3 occurrences):
  - Line 12: `spdlog::debug("SequencePacker initialized for device: {}", static_cast<int>(device.type));`
  - Line 35: `spdlog::debug("Packing {} sequences with {} total tokens",`
  - Line 98: `spdlog::debug("Unpacking {} sequences from packed tensor", batch_info.num_sequences);`

---

### `src/llm/lora_framework/training_service_registry.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/lora_framework/vram_allocator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (91.0/100)

**Issues Found:**

**📝 TODO** (4 occurrences):
  - Line 293: `// TODO: Implement Vulkan/DirectX upload`
  - Line 328: `// TODO: Implement Vulkan/DirectX download`
  - Line 425: `// TODO: Implement Vulkan/DirectX allocation`
  - Line 492: `// TODO: Implement Vulkan/DirectX deallocation with secure clear`

**🐛 DEBUG** (1 occurrences):
  - Line 69: `spdlog::debug("VRAMAllocator: could not query backend memory, defaulting to 8 GB pool");`

---

### `src/llm/lora_framework/vulkan_buffer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/lora_framework/vulkan_context.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🐛 DEBUG** (4 occurrences):
  - Line 16: `// Debug callback for validation layers`
  - Line 104: `// Setup debug messenger if validation enabled`
  - Line 159: `// Destroy debug messenger`
  - Line 217: `// Add debug utils extension`

---

### `src/llm/lora_framework/vulkan_pipeline.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/lora_metadata_cache.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/lora_router.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🐛 DEBUG** (3 occurrences):
  - Line 119: `spdlog::debug("Cache hit for query: {}", query.substr(0, 50));`
  - Line 139: `spdlog::debug("Routing query with policy: {}", static_cast<int>(active_policy));`
  - Line 145: `spdlog::debug("Found {} semantic candidates", candidates.size());`

---

### `src/llm/lora_security_validator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (94.0/100)

**Issues Found:**

**📝 TODO** (3 occurrences):
  - Line 227: `std::string cert_pem;  // TODO: Retrieve from certificate store by fingerprint`
  - Line 343: `// TODO: Implement certificate store integration for production deployments:`
  - Line 616: `// TODO: Implement actual LoRa format parsing`

**🐛 DEBUG** (11 occurrences):
  - Line 131: `spdlog::debug("Signature format validated: {} bytes, cert fingerprint: {}",`
  - Line 193: `spdlog::debug("Untrusted signer detected: {}", cert_fingerprint);`
  - Line 214: `spdlog::debug("Signature format invalid: {}", lora_path);`
  - Line 241: `spdlog::debug("Crypto verification failed for {}", lora_path);`
  - Line 312: `spdlog::debug("Embedded signer untrusted: {}", signer);`

---

### `src/llm/mcp_tool_bridge.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (2 occurrences):
  - Line 103: `spdlog::debug("[McpToolBridge] Bridged MCP tool '{}' as '{}'", name, alias);`
  - Line 135: `spdlog::debug("[McpToolBridge] Bridged single MCP tool '{}' as '{}'",`

---

### `src/llm/meta_prompt_generator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/mixed_precision_inference.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 217: `// Stub implementation - would check hardware capabilities`

---

### `src/llm/ml_model_manager.cpp` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (78.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 393: `// For now, simulate inference`
  - Line 1225: `// For now, simulate inference`

**📝 TODO** (8 occurrences):
  - Line 392: `// TODO: Actual inference logic based on model type`
  - Line 750: `// TODO: Actual model loading logic based on model type`
  - Line 761: `// TODO: Actual cleanup logic`
  - Line 777: `// TODO: Implement more sophisticated load balancing strategies`
  - Line 1224: `// TODO: Actual inference logic based on model type`

---

### `src/llm/mode_spec_loader.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/model_downloader.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/model_loader.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🐛 DEBUG** (4 occurrences):
  - Line 31: `spdlog::debug("Freeing llama.cpp backend resources");`
  - Line 80: `spdlog::debug("Model cache hit: {}", model_id);`
  - Line 182: `spdlog::debug("Model {} was loaded during async wait", model_id);`
  - Line 273: `spdlog::debug("Model {} was loaded during async wait", model_id);`

---

### `src/llm/model_metadata_cache.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/model_quantization_pipeline.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🐛 DEBUG** (3 occurrences):
  - Line 8: `template<typename... Args> inline void debug(const char*, Args&&...) {}`
  - Line 539: `spdlog::debug("AWQ: skipping incomplete layer '{}'", base_name);`
  - Line 667: `spdlog::debug("GPTQ: skipping incomplete layer '{}'", base_name);`

---

### `src/llm/model_router.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**🐛 DEBUG** (5 occurrences):
  - Line 116: `spdlog::debug("ModelRouter: updated rule '{}' -> model '{}'",`
  - Line 130: `spdlog::debug("ModelRouter: added rule '{}' (priority={}) -> model '{}'",`
  - Line 140: `spdlog::debug("ModelRouter: removed rule '{}'", rule_id);`
  - Line 155: `spdlog::debug("ModelRouter: all rules cleared");`
  - Line 179: `spdlog::debug("ModelRouter: rule '{}' matched -> model '{}'",`

---

### `src/llm/moral_analyzer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/multi_gpu_memory_coordinator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (82.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 157: `device.compute_capability = 0;  // 0 indicates CPU simulation mode`
  - Line 424: `return true;  // Return true in simulation mode`
  - Line 492: `// CPU-only fallback - simulate P2P capability`
  - Line 536: `// CPU-only fallback - simulate transfer`

**🐛 DEBUG** (6 occurrences):
  - Line 475: `spdlog::debug("Failed to check P2P capability: GPU {} -> GPU {} - {}",`
  - Line 485: `spdlog::debug("Failed to check P2P capability: GPU {} -> GPU {} - {}",`
  - Line 518: `spdlog::debug("P2P transfer success: GPU {} -> GPU {} ({} bytes)",`
  - Line 531: `spdlog::debug("P2P transfer success: GPU {} -> GPU {} ({} bytes)",`
  - Line 537: `spdlog::debug("Simulated P2P transfer: GPU {} -> GPU {} ({} bytes)",`

---

### `src/llm/multi_lora_manager.cpp` (v0.0.33)

**Maturity Level:** 🔴 ALPHA (29.0/100)

**Issues Found:**

**🎭 SIMULATION** (11 occurrences):
  - Line 344: `// In test mode, allow null context (mock inference)`
  - Line 346: `spdlog::warn("applyLoRA called with null context (test/mock mode)");`
  - Line 355: `spdlog::info("LoRA {} marked as active (mock mode)", lora_id);`
  - Line 407: `spdlog::warn("removeLoRA called with null context (test/mock mode)");`
  - Line 514: `// For now, return mock response`

**🐛 DEBUG** (38 occurrences):
  - Line 122: `spdlog::debug("    GPU {} initialized", gpu_id);`
  - Line 160: `spdlog::debug("LoRA {} cleaned up: freed {} MB", id, lora->vram_bytes / (1024*1024));`
  - Line 200: `spdlog::debug("LoRA cache hit: {}", lora_id);`
  - Line 250: `spdlog::debug("Freeing LoRA adapter handle for {}", lora_id);`
  - Line 257: `spdlog::debug("LoRA adapter handle freed for {}", lora_id);`

---

### `src/llm/multi_perspective_generator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/openai_compat_adapter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/paged_block_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/paged_kv_cache.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/paged_kv_cache_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 236: `spdlog::debug("PagedKVCacheManager::defragment: reclaimed {} unreferenced blocks",`

---

### `src/llm/production_validator.cpp` (v0.0.33)

**Maturity Level:** 🔴 ALPHA (23.0/100)

**Issues Found:**

**🎭 SIMULATION** (7 occurrences):
  - Line 136: `// Use simulation helper for consistent pass rate`
  - Line 222: `// Use simulation helper for consistent pass rate`
  - Line 290: `// Simulate request processing`
  - Line 399: `// Simulate one inference unit (wall-clock latency is what matters here)`
  - Line 981: `// Simulation for testing purposes only`

**📝 TODO** (27 occurrences):
  - Line 66: `// TODO: In real implementation, call actual LLM plugin via llm_plugin_->generate()`
  - Line 293: `// TODO: Actual inference request here`
  - Line 472: `// TODO: Load baseline and compare`
  - Line 485: `// TODO: Test model loading with LazyModelLoader`
  - Line 494: `// TODO: Test full inference pipeline`

**🐛 DEBUG** (4 occurrences):
  - Line 219: `spdlog::debug("  Testing {}: {}", test.category, test.prompt);`
  - Line 227: `spdlog::debug("    ✓ PASSED");`
  - Line 229: `spdlog::debug("    ✗ FAILED");`
  - Line 657: `spdlog::debug("Memory leak check: OK");`

---

### `src/llm/prompt_evaluator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/prompt_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/prompt_optimizer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/llm/prompt_policy.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 84: `spdlog::debug("PromptPolicy: rule '{}' redacted content from prompt",`

---

### `src/llm/sampling_strategy.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (92.0/100)

**Issues Found:**

**🐛 DEBUG** (8 occurrences):
  - Line 21: `spdlog::debug("GreedySampling::sample - selecting token with highest probability");`
  - Line 31: `spdlog::debug("GreedySampling selected token: {}", result);`
  - Line 46: `spdlog::debug("NucleusSampling created: temp={}, top_k={}, top_p={}, penalty={}",`
  - Line 54: `spdlog::debug("NucleusSampling::sample - temp={}, top_k={}, top_p={}, repeat_penalty={}",`
  - Line 134: `spdlog::debug("NucleusSampling selected token: {}", chosen);`

---

### `src/llm/security/signature_verifier.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (83.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 562: `// TODO: In production, implement actual CRL download and checking:`

**🐛 DEBUG** (15 occurrences):
  - Line 187: `spdlog::debug("Loading certificate from PEM");`
  - Line 210: `spdlog::debug("Certificate loaded successfully");`
  - Line 222: `spdlog::debug("Extracting public key from certificate");`
  - Line 246: `spdlog::debug("Public key extracted: type={} ({}), size={} bits",`
  - Line 321: `spdlog::debug("Loaded CA bundle from: {}", path);`

---

### `src/llm/shared_worker_pool.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🐛 DEBUG** (2 occurrences):
  - Line 112: `spdlog::debug("SharedWorkerPool worker {} started", thread_id);`
  - Line 180: `spdlog::debug("SharedWorkerPool worker {} stopped", thread_id);`

---

### `src/llm/speculative_decoder.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (2 occurrences):
  - Line 46: `spdlog::debug("SpeculativeDecoder initialised: k={}, min_threshold={:.3f}",`
  - Line 168: `spdlog::debug("SpeculativeDecoder::verify: accepted={}/{}, all_accepted={}, "`

---

### `src/llm/streaming_handler.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 97: `spdlog::debug("StreamingHandler: emitting [DONE] for request {}", request_id);`

---

### `src/llm/token_quota_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🐛 DEBUG** (2 occurrences):
  - Line 31: `spdlog::debug("TokenQuotaManager: set quota user='{}' model='{}' limit={}/min",`
  - Line 94: `spdlog::debug("TokenQuotaManager: consumed {} tokens user='{}' model='{}'",`

---

### `src/llm/vision_config.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 358: `// TODO: Implement JSON loading if needed`

---

### `src/llm/vision_encoder.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (84.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 98: `// TODO: Implement checksum verification`

**🐛 DEBUG** (2 occurrences):
  - Line 138: `spdlog::debug("Using legacy VisionEncoder constructor - consider upgrading to new API");`
  - Line 258: `spdlog::debug("VisionEncoder: Encoded image {} ({} floats) in {}ms",`

**🔒 HARDCODED** (3 occurrences):
  - Line 280: `// For now, we need to write to a temporary file`
  - Line 284: `// Generate unique temporary filename to avoid collisions in concurrent scenarios`
  - Line 292: `throw std::runtime_error("Failed to create temporary image file");`

---

### `src/llm/vision_resource_monitor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (94.0/100)

**Issues Found:**

**🐛 DEBUG** (6 occurrences):
  - Line 620: `spdlog::debug("Metrics collection loop started (interval: {}s)", interval.count());`
  - Line 631: `spdlog::debug("Vision metrics: active={}, total={}, memory={}MB/{}/MB, vram={}MB/{}MB",`
  - Line 637: `spdlog::debug("Metrics collection loop stopped");`
  - Line 641: `spdlog::debug("Quota reset loop started");`
  - Line 667: `spdlog::debug("Quota reset loop stopped");`

---

### `src/main.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 37: `// TODO: Implement configuration file loading logic`

---

### `src/main_server.cpp` (v0.0.33)

**Maturity Level:** ⚫ DRAFT (11.0/100)

**Issues Found:**

**🔴 STUB** (17 occurrences):
  - Line 230: `* Logs ERROR-level warnings every 5 minutes when stub HSM is active`
  - Line 302: `<< "  --allow-stub-hsm  Allow insecure stub HSM provider (development only)\n"`
  - Line 715: `// For now, we support stub and pkcs11`
  - Line 716: `// stub provider means empty library_path`
  - Line 717: `if (provider == "stub") {`

**🐛 DEBUG** (3 occurrences):
  - Line 420: `// Note: We continue to start the server for Community/Debug builds but log the warning`
  - Line 456: `THEMIS_ERROR("For development, use Debug build: cmake -DCMAKE_BUILD_TYPE=Debug");`
  - Line 1929: `// Comprehensive startup configuration debug output`

**🔒 HARDCODED** (1 occurrences):
  - Line 293: `// Simple hardcoded usage text`

---

### `src/metadata/catalog_exporter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 365: `spdlog::debug("CatalogExporter: Sending DataHub proposal for {}",`

---

### `src/metadata/column_lineage.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/metadata/distributed_catalog.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🐛 DEBUG** (2 occurrences):
  - Line 35: `spdlog::debug("DistributedMetadataCatalog: published schema '{}'", schema.name);`
  - Line 71: `spdlog::debug("DistributedMetadataCatalog: removed schema '{}'", table_name);`

---

### `src/metadata/er_diagram_exporter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**🐛 DEBUG** (5 occurrences):
  - Line 78: `spdlog::debug("ERDiagramExporter: skipping relationship '{}' with missing endpoints",`
  - Line 95: `spdlog::debug("ERDiagramExporter: exportMermaid() produced {} entities, {} relationships",`
  - Line 134: `spdlog::debug("ERDiagramExporter: skipping DOT edge '{}' with missing endpoints",`
  - Line 146: `spdlog::debug("ERDiagramExporter: exportDOT() produced {} nodes, {} edges",`
  - Line 217: `spdlog::debug("ERDiagramExporter: exportJSON() produced {} nodes, {} edges",`

---

### `src/metadata/index_recommender.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/metadata/information_schema.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/metadata/schema_audit_log.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/metadata/schema_consistency_checker.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🐛 DEBUG** (3 occurrences):
  - Line 42: `spdlog::debug("SchemaConsistencyChecker: initialized");`
  - Line 214: `spdlog::debug("SchemaConsistencyChecker: background check disabled");`
  - Line 245: `spdlog::debug("SchemaConsistencyChecker: background thread exited");`

---

### `src/metadata/schema_constraints.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🐛 DEBUG** (3 occurrences):
  - Line 135: `spdlog::debug("SchemaConstraints: Added '{}' constraint on {}.{}",`
  - Line 492: `spdlog::debug("SchemaConstraints: Persisted constraints for '{}'", table_name);`
  - Line 589: `spdlog::debug("SchemaConstraints: Loaded constraints for table '{}'", table_name);`

---

### `src/metadata/schema_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (81.0/100)

**Issues Found:**

**🐛 DEBUG** (19 occurrences):
  - Line 106: `spdlog::debug("SchemaManager: Initialized");`
  - Line 135: `spdlog::debug("SchemaManager: getAllTables() returned {} tables", tables.size());`
  - Line 155: `spdlog::debug("SchemaManager: getTable('{}') found", name);`
  - Line 159: `spdlog::debug("SchemaManager: getTable('{}') not found", name);`
  - Line 182: `spdlog::debug("SchemaManager: getAllRelationships() returned {} relationships", relationships.size()`

---

### `src/metadata/schema_version_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/metadata/statistics_collector.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (93.0/100)

**Issues Found:**

**🐛 DEBUG** (7 occurrences):
  - Line 126: `spdlog::debug("StatisticsCollector: Initialized");`
  - Line 139: `spdlog::debug("StatisticsCollector: auto-refresh disabled");`
  - Line 183: `spdlog::debug("StatisticsCollector: auto-refresh thread exited");`
  - Line 206: `spdlog::debug("StatisticsCollector: Collecting stats for '{}' (sample={})",`
  - Line 383: `spdlog::debug("StatisticsCollector: Cleared stats for '{}'", table_name);`

---

### `src/network/envoy_xds.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/network/geo_topology_router.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/network/grpc_transport.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/network/qos_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/network/quic_transport.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/network/service_mesh.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/network/socket_timeout_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (92.0/100)

**Issues Found:**

**🐛 DEBUG** (8 occurrences):
  - Line 203: `spdlog::debug("Accept timeout after {}ms", timeout.count());`
  - Line 228: `spdlog::debug("Accept timeout after {}ms", timeout.count());`
  - Line 251: `spdlog::debug("Accepted connection successfully");`
  - Line 269: `spdlog::debug("Read timeout on socket");`
  - Line 282: `spdlog::debug("Read timeout on socket");`

---

### `src/network/udp_fast_path.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/network/wire_protocol_connection_pool.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (94.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 528: `char dummy;`
  - Line 537: `socket.plain_socket()->receive(net::buffer(&dummy, 1), tcp::socket::message_peek, ec);`

---

### `src/network/wire_protocol_helpers.cpp` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (76.0/100)

**Issues Found:**

**🎭 SIMULATION** (8 occurrences):
  - Line 100: `uint64_t dummy;`
  - Line 101: `return readVarint(dummy);`
  - Line 104: `uint64_t dummy;`
  - Line 105: `return readFixed64(dummy);`
  - Line 108: `std::vector<uint8_t> dummy;`

---

### `src/network/wire_protocol_performance.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/network/wire_protocol_server.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (82.0/100)

**Issues Found:**

**📝 TODO** (9 occurrences):
  - Line 841: `// TODO: Implement HELLO handshake`
  - Line 846: `// TODO: Implement authentication`
  - Line 851: `// TODO: Implement GET operation`
  - Line 856: `// TODO: Implement PUT operation`
  - Line 861: `// TODO: Implement DELETE operation`

---

### `src/network/wire_protocol_server_ws.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/network/wire_protocol_v2.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/observability/alertmanager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/observability/continuous_profiler.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/observability/distributed_flame_graph.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/observability/ebpf_tracer.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/observability/metrics_collector.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/observability/performance_analyzer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/observability/query_profiler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/observability/storage_profiler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/performance/async_metrics_exporter.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/performance/chimera_exporter.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/performance/cicada.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/performance/cycle_metrics.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/performance/dostoevsky.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/performance/ligra.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/performance/numa_topology.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/performance/phase2_feature_flags.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/performance/phase3/adaptive_batch_tuner.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/performance/phase3/bao.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 56: `// Simulate different plan strategies`

---

### `src/performance/phase3/bwtree.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 182: `// TODO: Clean up old delta chain with proper memory reclamation`

---

### `src/performance/phase3/diskann.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/performance/phase3/feature_flags.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/performance/phase3/gunrock.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/performance/phase3/memory_pressure.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/performance/phase3/per_query_cost_model.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/performance/phase3/splinterdb.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/performance/phase4/feature_flags.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/performance/phase4/io_uring_zero_copy.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/performance/phase4/pmem_storage.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/performance/phase4/pmu_counters.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (90.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 163: `// Non-Linux stubs – all counters report unavailable`
  - Line 195: `// Stubs when PMU counters are disabled at compile time`

---

### `src/performance/prometheus_exporter.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/performance/rabitq.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/performance/wisckey.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (88.0/100)

**Issues Found:**

**🔒 HARDCODED** (3 occurrences):
  - Line 90: `// Create temporary new log file`
  - Line 95: `throw std::runtime_error("Failed to create temporary log file for compaction");`
  - Line 126: `// Flush and close temporary log`

---

### `src/performance/workload_predictor.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/plugins/huggingface_ingestion_plugin.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 192: `"2. Use worker.submitFile() as a temporary workaround and handle HUGGINGFACE type in handler");`

---

### `src/plugins/oci_registry_client.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/plugins/plugin_health_monitor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/plugins/plugin_hot_plug_monitor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 64: `// Reject temporary/incomplete files produced by editors, package managers,`

---

### `src/plugins/plugin_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/plugins/plugin_metrics.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/plugins/plugin_registry.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/plugins/plugin_system_edition.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/plugins/rpc_service_registry.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/plugins/signed_plugin_repository.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/prompt_engineering/feedback_collector.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/prompt_engineering/meta_prompt_generator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/prompt_engineering/prompt_engineering_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/prompt_engineering/prompt_engineering_metrics.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/prompt_engineering/prompt_evaluator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/prompt_engineering/prompt_injection_detector.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (91.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 183: `// the model embeds fake system-role tokens or instruction blocks in its own`
  - Line 187: `// Fake "SYSTEM:" role prefix at the start of a line`
  - Line 215: `// as a fake "SYSTEM:" prefix is sufficient to meet the default threshold.`

---

### `src/prompt_engineering/prompt_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/prompt_engineering/prompt_optimizer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/prompt_engineering/prompt_performance_tracker.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/prompt_engineering/prompt_version_control.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/prompt_engineering/self_improvement_orchestrator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/query/adaptive_optimizer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 164: `spdlog::debug("Selected plan: {} (adjusted cost: {:.2f}, adjustment factor: {:.2f})",`

---

### `src/query/aql_parser.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 768: `// Debug: uncomment to trace tokens`

---

### `src/query/aql_parser_json.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/query/aql_runner.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/query/aql_translator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/query/cross_cluster_federation.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🐛 DEBUG** (3 occurrences):
  - Line 158: `spdlog::debug(`
  - Line 360: `spdlog::debug("CrossClusterFederator: querying cluster '{}' at {}",`
  - Line 398: `spdlog::debug(`

---

### `src/query/cte_cache.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/query/cte_subquery.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 312: `// Phase 1 stub: treat as scalar subquery; real behavior handled elsewhere`

---

### `src/query/functions/ethics_functions.cpp` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (60.0/100)

**Issues Found:**

**🔴 STUB** (4 occurrences):
  - Line 14: `result["status"] = "stub";`
  - Line 44: `result["decision_text"] = "Stub: Decision analysis pending full implementation";`
  - Line 202: `profile["founder"] = "Unknown (stub)";`
  - Line 203: `profile["main_thesis"] = "Philosophy profile stub";`

**📝 TODO** (10 occurrences):
  - Line 37: `// TODO: Integrate with EthicalDiscourseEngine`
  - Line 77: `// TODO: Integrate with EthicsEvaluator`
  - Line 126: `// TODO: Query ethics_arguments collection via AQL`
  - Line 148: `// TODO: Integrate with vector search`
  - Line 173: `// TODO: Integrate with graph traversal`

---

### `src/query/functions/fulltext_functions.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/query/functions/function_registry.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 108: `// TODO: registerFulltextFunctions - optional fulltext module`

---

### `src/query/functions/lora_functions.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**📝 TODO** (2 occurrences):
  - Line 335: `// TODO: Replace with actual vector similarity calculation using embeddings`
  - Line 586: `// TODO: Retrieve actual metrics from the metrics system`

---

### `src/query/functions/process_mining_functions.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/query/functions/udf_registry.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/query/let_evaluator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/query/materialized_cte.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (2 occurrences):
  - Line 260: `spdlog::debug("MaterializedCTERegistry: registered CTE '{}'", def.name);`
  - Line 268: `spdlog::debug("MaterializedCTERegistry: unregistered CTE '{}'", name);`

---

### `src/query/optimizer_cost_model.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/query/query_cache.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/query/query_cache_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/query/query_engine.cpp` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (73.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 91: `// Stubbed: full expression evaluation will be added in a later phase.`
  - Line 100: `return "aql-stub";`

**📝 TODO** (1 occurrences):
  - Line 3465: `// TODO: Add edge type filtering once exposed in TraversalQuery struct`

**🐛 DEBUG** (3 occurrences):
  - Line 1333: `spdlog::debug("ST_Within extractPoint: Failed to parse string as JSON - {}", e.what());`
  - Line 1359: `spdlog::debug("ST_Within extractMBR: Failed to parse string as JSON - {}", e.what());`
  - Line 1395: `spdlog::debug("ST_Within: Failed to extract MBR (backward compat: failing open, returning true)");`

**🔒 HARDCODED** (3 occurrences):
  - Line 289: `// Create a temporary query with only the structural predicates`
  - Line 377: `// Create a temporary query with only the structural predicates`
  - Line 522: `// Create a temporary query with only the structural predicates`

---

### `src/query/query_federation.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (88.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 326: `// TODO: Implement actual shard determination logic`

**🐛 DEBUG** (10 occurrences):
  - Line 57: `spdlog::debug("Executing on shard: {}", shard_id);`
  - Line 134: `spdlog::debug("Using broadcast join strategy");`
  - Line 137: `spdlog::debug("Using shuffle join strategy");`
  - Line 143: `spdlog::debug("Using map-reduce strategy for aggregation");`
  - Line 152: `spdlog::debug("Using partition pruning: {} shards",`

---

### `src/query/query_optimizer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (82.0/100)

**Issues Found:**

**📝 TODO** (3 occurrences):
  - Line 484: `// TODO(v1.5.1): Replace with actual MetadataShard integration`
  - Line 513: `// TODO(v1.5.1): Replace with actual PrometheusMetrics integration`
  - Line 552: `// TODO(v1.5.1): Use actual statistics and histograms`

**🐛 DEBUG** (4 occurrences):
  - Line 309: `spdlog::debug("QueryOptimizer: Query execution recorded - est_rows={}, actual_rows={}, time_ms={}",`
  - Line 393: `spdlog::debug("QueryOptimizer: NUMA awareness enabled for distributed query");`
  - Line 628: `spdlog::debug("VectorWorkloadPlan: index_type={}, ef_search={}, k_overfetch={}, dataset_size={}",`
  - Line 691: `spdlog::debug("GraphWorkloadPlan: max_depth={}, bidirectional={}, parallelism={}",`

**🔒 HARDCODED** (2 occurrences):
  - Line 486: `// Temporary heuristic: Use shard_id hash to vary estimates`
  - Line 515: `// Temporary implementation: Use connection pool ping times`

---

### `src/query/query_plan_visualizer.cpp` (v0.0.11)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/query/result_stream.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/query/result_type_annotation.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/query/runtime_reoptimizer.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 141: `spdlog::debug(`

---

### `src/query/semantic_cache.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/query/sparql_parser.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/query/sql_parser.cpp` (v0.0.2)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/query/statistical_aggregator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/query/vectorized_execution.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 190: `spdlog::debug(`

---

### `src/query/window_evaluator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/query/workload_cache_strategy.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/ab_testing_framework.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/agentic_rag.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/bayesian_optimizer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/bias_detector.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/citation_highlighter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/claim_extractor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/coherence_evaluator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/completeness_evaluator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/continuous_learning_client.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/continuous_learning_orchestrator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/cot_evaluator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/document_splitter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/document_summarizer.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/evaluation_report_exporter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/faithfulness_evaluator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/geval_evaluator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 352: `spdlog::debug("G-Eval {} score: {:.3f} (confidence: {:.3f})",`

---

### `src/rag/hallucination_dashboard.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/http_metrics_client.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/hybrid_retriever.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/judge_config.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/judge_ensemble.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/knowledge_gap_detector.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/knowledge_graph_retriever.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/learning_metrics.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/llm_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 87: `THEMIS_WARN("LLMIntegration::generate - No inference engine configured, using stub");`

---

### `src/rag/llm_judge_client.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/llm_judge_integration.cpp` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (67.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 23: `THEMIS_WARN("LLMJudgeIntegration initialized in MOCK MODE - evaluations will use stub responses");`

**🎭 SIMULATION** (10 occurrences):
  - Line 19: `// Only set default inference function if mock mode is explicitly enabled`
  - Line 23: `THEMIS_WARN("LLMJudgeIntegration initialized in MOCK MODE - evaluations will use stub responses");`
  - Line 144: `error_msg += "or enable mock mode (config.use_mock_mode = true) for testing.";`
  - Line 149: `// Warn once if in mock mode`
  - Line 151: `THEMIS_WARN("LLM evaluation using MOCK MODE - results are not real (warning shown once)");`

---

### `src/rag/llm_meta_analyzer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 233: `// Fall through to hardcoded fallback below`

---

### `src/rag/multimodal_rag.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/nli_faithfulness_verifier.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/onnx_model_loader.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/pairwise_comparator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/prompt_templates.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/quality_control_factory.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/quality_control_pipeline.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/rag_judge.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/relevance_evaluator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/reranker.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (92.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 6: `* forward pass is invoked (stub ready for OnnxRuntime integration).`
  - Line 167: `// TODO(cross-encoder): replace stub with OnnxRuntime session init:`

**📝 TODO** (3 occurrences):
  - Line 167: `// TODO(cross-encoder): replace stub with OnnxRuntime session init:`
  - Line 205: `// TODO(cross-encoder): invoke OnnxRuntime session:`
  - Line 340: `// TODO(cross-encoder): replace with actual OnnxRuntime session load:`

---

### `src/rag/response_parser.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/rubric_evaluator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/rag/streaming_retriever.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/replication/replication_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (90.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 3131: `// over a mTLS connection to the peer node.  For now we simulate success`
  - Line 3499: `// For now simulate: ACTIVE replicas return success with the replica's`
  - Line 4231: `// Build a dummy payload of the requested size`

**📝 TODO** (1 occurrences):
  - Line 561: `// TODO(future): apply leader_commit to follower's local commit index`

---

### `src/scheduler/distributed_task_coordinator.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/scheduler/event_trigger.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/scheduler/external_scheduler_adapter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/scheduler/hybrid_retention_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/scheduler/task_anomaly_detector.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/scheduler/task_audit_event.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/scheduler/task_audit_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/scheduler/task_result_store.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/scheduler/task_scheduler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (85.0/100)

**Issues Found:**

**📝 TODO** (9 occurrences):
  - Line 36: `// Note: Multiple TODO comments throughout this file indicate where user authentication`
  - Line 38: `// Tracked in issue: #TODO-AUTH-CONTEXT`
  - Line 428: `// TODO: Integrate with AuthenticationContext to retrieve actual user_id when available`
  - Line 429: `// TODO: Integrate with RequestContext to retrieve actual client IP address when available`
  - Line 470: `"system", // TODO: Get actual user from auth context`

**🔒 HARDCODED** (1 occurrences):
  - Line 202: `// Default: treat as transient (connection issues, temporary failures, etc.)`

---

### `src/search/autocomplete.cpp` (v0.0.29)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/search/cross_lingual_search.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/search/faceted_search.cpp` (v0.0.29)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/search/fuzzy_matcher.cpp` (v0.0.29)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/search/hybrid_search.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/search/learning_to_rank.cpp` (v0.0.29)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/search/llm_query_rewriter.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/search/llm_reranker.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/search/multi_field_search.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/search/multi_modal_search.cpp` (v0.0.29)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/search/neural_sparse_retrieval.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/search/personalized_ranker.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/search/query_expander.cpp` (v0.0.29)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/search/search_analytics.cpp` (v0.0.29)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/security/access_control.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/security/access_control_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/security/aql_injection_detector.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 224: `// TODO (v1.4.0): Implement AST-level operation validation`

---

### `src/security/arrow_user_registration_plugin.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (94.0/100)

**Issues Found:**

**📝 TODO** (4 occurrences):
  - Line 60: `// TODO: Implement Apache Arrow integration`
  - Line 96: `// TODO: Implement Arrow-based authentication`
  - Line 114: `// TODO: Implement bulk user sync from Arrow source`
  - Line 134: `// TODO: Implement user update from Arrow source`

---

### `src/security/binary_manifest.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/security/cms_signing.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/security/confidential_computing.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/security/embedded_user_registration_plugin.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/security/encrypted_field.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/security/field_encryption.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (94.0/100)

**Issues Found:**

**🐛 DEBUG** (6 occurrences):
  - Line 24: `// Only write debug dumps when user explicitly sets THEMIS_DEBUG_ENC_DIR`
  - Line 248: `// best-effort debug write (opt-in via env)`
  - Line 261: `// best-effort debug write (opt-in via env)`
  - Line 534: `// Write debug dump (best-effort)`
  - Line 597: `// write debug dump showing failure`

---

### `src/security/fips_crypto_mode.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/security/hsm_key_provider_adapter.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (90.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 338: `// For stub/fallback: uses AES-256-GCM with an in-memory stub KEK.`
  - Line 362: `// For stub/fallback: uses AES-256-GCM with the same in-memory stub KEK used for wrapping.`

---

### `src/security/hsm_provider.cpp` (v0.0.33)

**Maturity Level:** ⚫ DRAFT (0.0/100)

**Issues Found:**

**🔴 STUB** (44 occurrences):
  - Line 1: `// Clean minimal stub implementation of HSMProvider.`
  - Line 23: `std::vector<uint8_t> stub_kek; // 32-byte AES-256 KEK for stub wrap/unwrap`
  - Line 46: `static std::vector<uint8_t> stub_aes_encrypt(const std::vector<uint8_t>& key, const std::vector<uint`
  - Line 75: `static std::vector<uint8_t> stub_aes_decrypt(const std::vector<uint8_t>& key, const std::vector<uint`
  - Line 118: `// SECURITY HARDENING: Check for explicit opt-in to use stub provider`

---

### `src/security/hsm_provider_pkcs11.cpp` (v0.0.33)

**Maturity Level:** ⚫ DRAFT (0.0/100)

**Issues Found:**

**🔴 STUB** (23 occurrences):
  - Line 30: `// operations transparently revert to deterministic stub behaviour.`
  - Line 163: `std::vector<uint8_t> stub_kek; // Fallback AES-256 KEK when real HSM unavailable`
  - Line 270: `// Generate fallback stub KEK for consistent wrap/unwrap when real HSM is unavailable`
  - Line 272: `impl_->stub_kek.resize(32);`
  - Line 273: `if (RAND_bytes(impl_->stub_kek.data(), 32) != 1) {`

---

### `src/security/hsm_signing.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 8: `* Adapts HSMProvider (PKCS#11 / stub) to the SigningService interface so that`
  - Line 14: `* whether a real HSM or the in-process stub is in use.`

---

### `src/security/key_cache.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/security/keyprovider_signing.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/security/malware_scanner.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/security/manifest_signer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 191: `spdlog::debug("Added to manifest: {} ({})", file_entry.path, file_entry.sha256_hash);`

---

### `src/security/mock_key_provider.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/security/pii_redaction_policy.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 154: `// without the overhead of building a temporary map.`

---

### `src/security/pki_key_provider.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/security/post_quantum_crypto.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (91.0/100)

**Issues Found:**

**🎭 SIMULATION** (8 occurrences):
  - Line 2: `* Post-Quantum Cryptography — Software Simulation Backend`
  - Line 5: `* primitives as a drop-in simulation:`
  - Line 15: `* Key size mapping (simulation):`
  - Line 19: `*   (All three map to X25519 in the simulation; real liboqs sizes differ.)`
  - Line 210: `// ─── X25519 helpers (KyberKEM simulation) ────────────────────────────────`

---

### `src/security/query_masking_policy.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 44: `spdlog::debug("QueryMaskingPolicy: declared field '{}' mask_mode='{}'", field_name, mask_mode);`

---

### `src/security/rbac.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/security/row_level_security.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/security/secret_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/security/timestamp_authority.cpp` (v0.0.33)

**Maturity Level:** ⚫ DRAFT (0.0/100)

**Issues Found:**

**🔴 STUB** (21 occurrences):
  - Line 1: `// Minimal stub implementation for TimestampAuthority.`
  - Line 15: `// Helper: check production mode (mirrors HSM stub pattern)`
  - Line 43: `// Helper: check if TSA stub is explicitly allowed`
  - Line 49: `// Helper: return a failed token indicating stub is blocked in production`
  - Line 54: `"TimestampAuthority stub is not permitted in production mode. "`

---

### `src/security/timestamp_authority_openssl.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 3: `// Separate from stub to avoid dependency bloat when not needed.`

---

### `src/security/tsa_api.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/security/usb_admin_authenticator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**📝 TODO** (2 occurrences):
  - Line 495: `// TODO: Implement Windows registry reading for MachineGuid`
  - Line 539: `// TODO: CRITICAL SECURITY - Implement proper challenge-response validation`

---

### `src/security/user_registration_plugin.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/security/vault_key_provider.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/security/vault_signing_provider.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (88.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 67: `// - If environment doesn't provide a reachable Vault (empty vault_addr), fall back to a local mock`
  - Line 76: `// When Vault is not configured, return an error instead of mock signature`
  - Line 118: `// Return error instead of falling back to mock signature`
  - Line 160: `// Return error instead of silently falling back to mock`

---

### `src/security/vcc_pki_client.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/security/vram_secure_clear.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (94.0/100)

**Issues Found:**

**🐛 DEBUG** (6 occurrences):
  - Line 31: `spdlog::debug("VRAM secure clear: {} bytes at {}, {} passes",`
  - Line 79: `spdlog::debug("VRAM secure clear completed: {} bytes", size_bytes);`
  - Line 100: `spdlog::debug("VRAM secure clear (HIP): {} bytes at {}, {} passes",`
  - Line 147: `spdlog::debug("VRAM secure clear (HIP) completed: {} bytes", size_bytes);`
  - Line 167: `spdlog::debug("CPU secure clear: {} bytes at {}, {} passes",`

---

### `src/security/webdav_user_registration_plugin.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (3 occurrences):
  - Line 155: `// TODO: Implement WebDAV PROPFIND to list users`
  - Line 175: `// TODO: Implement user property update from WebDAV`
  - Line 259: `// TODO: Implement AD property retrieval via WebDAV`

---

### `src/security/zero_trust_policy_enforcer.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/admin_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/api_auth_config.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/api_gateway.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🐛 DEBUG** (3 occurrences):
  - Line 163: `spdlog::debug("APIGateway: normalized versioned path '{}' to '{}'",`
  - Line 754: `spdlog::debug("Metric: {} {} duration_ms={}", metric_name,`
  - Line 801: `spdlog::debug("APIGateway: version resolved from URL path prefix: {}", version.toString());`

---

### `src/server/api_key_mgmt_handler.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/api_security_audit.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/api_version.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/async_job_api_handler.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/audit_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/auth_middleware.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (92.0/100)

**Issues Found:**

**📝 TODO** (2 occurrences):
  - Line 223: `// TODO: Enhance with proper scope extraction from JWT claims`
  - Line 374: `// TODO: Check if any of the roles provide the required_scope`

**🔒 HARDCODED** (1 occurrences):
  - Line 225: `// For now: check if required_scope is in a hardcoded allowed list or derive from sub`

---

### `src/server/bpmn_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**📝 TODO** (2 occurrences):
  - Line 91: `// TODO: Implement scope-based authorization when AuthMiddleware supports it`
  - Line 431: `// TODO: ProcessGraphManager doesn't store individual visit timestamps per node,`

---

### `src/server/branch_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/buffer_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/buffer_binary_protocol.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/cache_admin_api_handler.cpp` (v0.0.6)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/cache_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/cdn_cache_middleware.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/changefeed_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 352: `// Full keep-alive requires custom async write loop (see TODO in docs)`

---

### `src/server/chunked_response_writer.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/classification_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/compliance_reporting_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/content_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/diff_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 164: `spdlog::debug("DiffApiHandler::parseTimestamp: '{}' is not a numeric millisecond timestamp, trying I`

---

### `src/server/distributed_txn_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/entity_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/error_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/ethics_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 358: `// TODO: Convert to Prometheus format`

---

### `src/server/export_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/feedback_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/geo_topology_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/graph_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/graphql_api_handler.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/grpc_web_proxy_handler.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (85.0/100)

**Issues Found:**

**🔴 STUB** (3 occurrences):
  - Line 116: `stub_holder_    = std::make_shared<grpc::GenericStub>(channel);`
  - Line 221: `auto* stub = static_cast<grpc::GenericStub*>(stub_holder_.get());`
  - Line 273: `auto call = stub->PrepareUnaryCall(&ctx, method, request_buf, &cq);`

---

### `src/server/health_error_service.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/hot_reload_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/hsm_provider_global.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/http2_session.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 447: `// TODO: Use proper buffer management for production`

---

### `src/server/http3_datagram.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/http3_session.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/http_server.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (89.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 8454: `//   "1"   -> throw exception (simulate hard init failure)`

**📝 TODO** (4 occurrences):
  - Line 88: `#include "server/http_type_adapter.h"  // TODO: Remove after migration to cpp-httplib (see HTTP_SERV`
  - Line 560: `// TODO: Initialize actual ShardingManager here when available`
  - Line 922: `// TODO: Fix QueryEngine dependency`
  - Line 3150: `// TODO: Re-enable when EthicsApiHandler routing is fixed`

---

### `src/server/http_type_adapter.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 4: `// TODO: Consider using proper URL decoding library (e.g., Boost.URL or cpp-url) in production`

---

### `src/server/import_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/import_wizard_builder.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/index_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/keys_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/llm_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 260: `// TODO: Add actual vector search results to rag_context.documents`

---

### `src/server/llm_grpc_service.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (86.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 19: `// TODO: Implement actual JWT validation`

**🔒 HARDCODED** (3 occurrences):
  - Line 312: `// Security: Use secure temporary directory with random suffix`
  - Line 324: `return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to create temporary file");`
  - Line 339: `// Clean up temporary file after processing`

---

### `src/server/load_shedder.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/lora_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (92.0/100)

**Issues Found:**

**📝 TODO** (4 occurrences):
  - Line 59: `// TODO: Add proper service-to-service authentication (e.g., verify mTLS certificate)`
  - Line 720: `{"memory_usage_mb", 0},  // TODO: Calculate actual memory usage from adapter manager`
  - Line 766: `// TODO: Integrate with actual LLM inference engine`
  - Line 976: `data_str = data_base64;  // TODO: Implement proper base64 decode`

---

### `src/server/mcp_server.cpp` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (71.0/100)

**Issues Found:**

**🔴 STUB** (3 occurrences):
  - Line 436: `// Default Tool Handlers (Stubs)`
  - Line 1691: `// Default Resource Handlers (Stubs)`
  - Line 1813: `// Default Prompt Handlers (Stubs)`

**🐛 DEBUG** (14 occurrences):
  - Line 216: `spdlog::debug("Registered MCP tool: {}", name);`
  - Line 221: `spdlog::debug("Unregistered MCP tool: {}", name);`
  - Line 231: `spdlog::debug("Registered MCP resource: {}", uri);`
  - Line 236: `spdlog::debug("Unregistered MCP resource: {}", uri);`
  - Line 246: `spdlog::debug("Registered MCP prompt: {}", name);`

---

### `src/server/merge_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/monitoring_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/mqtt_session.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/mvcc_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/opa_adapter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/openapi_route_registry.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/pii_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/pitr_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/pitr_grpc_service.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (93.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 7: `// Note: This file provides stub implementation for PITRServiceImpl`

**📝 TODO** (1 occurrences):
  - Line 59: `// TODO: Once proto/themis_core.proto is compiled with PITRService, implement these methods:`

---

### `src/server/pki_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/policy_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/policy_engine.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/policy_manager_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/policy_template_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/policy_validation_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/policy_versioning_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/postgres_session.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (92.0/100)

**Issues Found:**

**📝 TODO** (4 occurrences):
  - Line 213: `// TODO: Use centralized version from VERSION file (currently 1.3.4)`
  - Line 733: `// TODO: Batch insert the data using QueryEngine`
  - Line 1337: `// TODO: Query actual tables from database`
  - Line 1358: `// TODO: Query actual columns from database schema`

---

### `src/server/profiling_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/prompt_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/prompt_engineering_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/prompt_engineering_grpc_service.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 41: `// TODO: Implement actual JWT validation`

---

### `src/server/query_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 3155: `// Create temporary request for AQL handler`

---

### `src/server/ranger_adapter.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/rate_limiter.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/rate_limiter_v2.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/rate_limiting_middleware.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/replication_topology_api_handler.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/reports_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/request_validation_middleware.cpp` (v0.0.20)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/retention_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/review_scheduling_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/rope_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/rpc/blob_transfer_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 5: `// #include <crc32c/crc32c.h>  // TODO: Missing vcpkg package`

---

### `src/server/rpc/differential_update_engine.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 148: `// TODO: Extract specific chunks from blob`

---

### `src/server/rpc/rpc_service_impl.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/rpc/snapshot_transfer_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (94.0/100)

**Issues Found:**

**📝 TODO** (3 occurrences):
  - Line 51: `// TODO: Get RocksDB instance from ThemisDB`
  - Line 342: `// TODO: Restore RocksDB from checkpoint directory`
  - Line 605: `// TODO: Implement XXH64`

---

### `src/server/saga_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/schema_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🐛 DEBUG** (4 occurrences):
  - Line 69: `spdlog::debug("Schema API: Returned full schema");`
  - Line 124: `spdlog::debug("Schema API: Returned {} tables", tables.size());`
  - Line 211: `spdlog::debug("Schema API: Returned schema for table '{}'", table_name);`
  - Line 251: `spdlog::debug("Schema API: Returned capabilities");`

---

### `src/server/serverless_function_api_handler.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/session_api_handler.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/sharding_metrics_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/snapshot_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/spatial_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/sse_connection_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/task_scheduler_api_handler.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/tenant_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/themis_core_grpc_service.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (80.0/100)

**Issues Found:**

**🔴 STUB** (4 occurrences):
  - Line 10: `// stubs generated from proto/themis_core.proto are available on the include`
  - Line 67: `// The method stubs are automatically provided by the generated`
  - Line 95: `"service will be a no-op until protoc generates the stubs");`
  - Line 105: `// proto stubs not generated; returning null is expected here`

---

### `src/server/timeseries_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/transaction_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/udf_api_handler.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/update_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/vector_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 688: `// TODO: implement fine-grained scope checks; currently allow if auth is enabled.`

---

### `src/server/voice_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/wal_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/server/wal_grpc_service.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 155: `THEMIS_WARN("Shard gRPC stubs not found; WalGrpcService is a no-op");`

---

### `src/server/websocket_session.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/adaptive_shard_router.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 253: `// TODO (KNOWN LIMITATION): Production deployment requires more sophisticated query analysis:`

---

### `src/sharding/admin_api.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/admin_operations.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/auto_rebalancer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/capability_matcher.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/circuit_breaker.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 129: `// TODO: Log state transition for monitoring`

---

### `src/sharding/cloud_agent.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/cloud_backup.cpp` (v0.0.1)

**Maturity Level:** 🔴 ALPHA (22.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 80: `// Implementation guide: See docs/STUB_REPLACEMENT_MIGRATION_GUIDE.md`

**🎭 SIMULATION** (21 occurrences):
  - Line 103: `// to simulate successful uploads`
  - Line 106: `THEMIS_INFO("Mock mode enabled - simulating successful upload");`
  - Line 129: `THEMIS_INFO("Mock mode enabled - simulating successful download");`
  - Line 130: `// Create empty file to simulate download`
  - Line 132: `file << "Mock backup file - replace with real AWS SDK integration\n";`

**📝 TODO** (5 occurrences):
  - Line 14: `// TODO v1.4.0: Expand test coverage as needed`
  - Line 71: `// TODO v1.4.0: Integrate real AWS SDK for production use`
  - Line 208: `// TODO v1.4.0: Integrate real Azure SDK`
  - Line 305: `// TODO v1.4.0: Integrate real GCS SDK`
  - Line 427: `// TODO v1.4.0: Replace with actual BackupManager call to create real backup`

---

### `src/sharding/consensus_factory.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/consistent_hash.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/cross_shard_transaction.cpp` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (67.0/100)

**Issues Found:**

**📝 TODO** (3 occurrences):
  - Line 1875: `// TODO: Replace with proper compensation RPC`
  - Line 2558: `entry.coordinator_id = "coordinator-1";  // TODO: Get actual coordinator ID`
  - Line 2582: `"coordinator-1",  // TODO: Get actual coordinator ID`

**🐛 DEBUG** (23 occurrences):
  - Line 258: `spdlog::debug("Added participant {} to transaction {}", shard_id, transaction_id);`
  - Line 855: `spdlog::debug("Sending PreCommit to shard {} for transaction {}",`
  - Line 989: `spdlog::debug("Acquiring lock on secondary shard {} for transaction {}",`
  - Line 1021: `spdlog::debug("Acquiring lock on primary shard {} for transaction {}",`
  - Line 1122: `spdlog::debug("Committing secondary shard {} for Percolator transaction {}",`

**🔒 HARDCODED** (1 occurrences):
  - Line 38: `spdlog::warn("Transaction log path not configured, using temporary path: {}",`

---

### `src/sharding/data_migrator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/distributed_coordinator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/distributed_time_coordinator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/distributed_transaction.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/gossip_config_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 553: `(void)e; // silence unused warning in stub`

---

### `src/sharding/gossip_consensus_adapter.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 340: `// Simulate gossip propagation`

**🐛 DEBUG** (2 occurrences):
  - Line 326: `spdlog::debug("Gossip thread started");`
  - Line 359: `spdlog::debug("Gossip thread stopped");`

---

### `src/sharding/gossip_protocol.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 390: `peer.peer_id = "seed_" + seed;  // Temporary ID until we get real ID`

---

### `src/sharding/gpu_erasure_coder.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/gpu_erasure_coder_opencl.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 20: `// OpenCL Implementation Class (Stub)`

---

### `src/sharding/health_check.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/health_monitor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 269: `// For now, mark all replicas as healthy (mock)`

---

### `src/sharding/hot_spare_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/locality_aware_router.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/metadata_shard.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/metadata_snapshot.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 231: `spdlog::debug("Deleted metadata snapshot: {}", snapshot_id);`

---

### `src/sharding/metadata_wal.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🐛 DEBUG** (2 occurrences):
  - Line 126: `spdlog::debug("Read {} metadata entries from WAL starting at LSN ({}, {})",`
  - Line 156: `spdlog::debug("Logged metadata {} to WAL: partition={}, key={}, version={}, LSN=({}, {})",`

---

### `src/sharding/metrics_registry.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/mtls_client.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/mtls_connection_pool.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 198: `// Note: This is a stub implementation`

---

### `src/sharding/multi_primary_coordinator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/operational_metrics.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/orphan_detector.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**📝 TODO** (2 occurrences):
  - Line 28: `// TODO: Access coordinator's transactions and check each one`
  - Line 45: `// TODO: Get transaction from coordinator`

---

### `src/sharding/partition_detector.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/paxos_consensus.cpp` (v0.0.33)

**Maturity Level:** 🟠 BETA (49.0/100)

**Issues Found:**

**🎭 SIMULATION** (6 occurrences):
  - Line 528: `// In a single-node simulation, we always promise to ourselves`
  - Line 532: `// Simulate timeout for collecting promises`
  - Line 535: `// For now, simulate other nodes accepting`
  - Line 548: `// Simulate promise from other nodes`
  - Line 611: `// For now, simulate other nodes accepting with timeout`

**🐛 DEBUG** (25 occurrences):
  - Line 379: `spdlog::debug("Paxos proposer thread started");`
  - Line 405: `spdlog::debug("Node {} retrying proposal for slot {} (attempt {}/{})",`
  - Line 433: `spdlog::debug("Paxos proposer thread stopped");`
  - Line 437: `spdlog::debug("Paxos acceptor thread started");`
  - Line 444: `spdlog::debug("Paxos acceptor thread stopped");`

**🔒 HARDCODED** (2 occurrences):
  - Line 878: `spdlog::error("Failed to open temporary state file: {}", temp_file);`
  - Line 887: `spdlog::error("Failed to rename temporary state file: {}", strerror(errno));`

---

### `src/sharding/paxos_snapshot.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/paxos_wal.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🐛 DEBUG** (3 occurrences):
  - Line 124: `spdlog::debug("PaxosWAL: Logged entry type={} slot={} round={} at LSN={}",`
  - Line 201: `spdlog::debug("PaxosWAL: Read {} entries from LSN {} to {}",`
  - Line 233: `spdlog::debug("PaxosWAL: Flushed to disk");`

---

### `src/sharding/pki_shard_certificate.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 273: `// For now, we'll extract shard_id from CN if it follows the pattern "shard-XXX"`

---

### `src/sharding/predictive_detector.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/prometheus_metrics.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/quorum_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/raft_configuration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/raft_consensus.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/raft_consensus_adapter.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/raft_log.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/raft_shard_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/raft_state.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/raft_wal_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 55: `// For now, we simulate waiting for responses`

---

### `src/sharding/rebalance_operation.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/redundancy_strategy.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 1130: `spdlog::debug("Raft write committed for document {} on shard {}", document_id, shard_id);`

---

### `src/sharding/remote_executor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/replica_consistency.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/replica_topology.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/replication_coordinator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/secure_transport_client.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (93.0/100)

**Issues Found:**

**📝 TODO** (2 occurrences):
  - Line 60: `// TODO: Add LZ4 support in the future`
  - Line 138: `// TODO: MTLSClient doesn't currently support custom headers (like Authorization: Bearer)`

**🐛 DEBUG** (3 occurrences):
  - Line 23: `spdlog::debug("SecureTransportClient: mTLS client initialized");`
  - Line 54: `spdlog::debug("SecureTransportClient: Compressed {} -> {} bytes (ratio: {:.2f}x)",`
  - Line 146: `spdlog::debug("SecureTransportClient: Sending {} bytes (compressed: {}) to {}{}",`

---

### `src/sharding/shard_durability.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/shard_load_detector.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/shard_repair_engine.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 407: `spdlog::debug("ShardRepairEngine: shard {} – scanned={} healthy={} degraded={} "`

---

### `src/sharding/shard_resource_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 247: `// TODO: Implement platform-specific CPU core detection (sysconf on Linux, WMI on Windows)`

---

### `src/sharding/shard_router.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 471: `// TODO: Track actual right-side row count when full join implementation is complete`

---

### `src/sharding/shard_rpc_client.cpp` (v0.0.33)

**Maturity Level:** 🟠 BETA (49.0/100)

**Issues Found:**

**🔴 STUB** (7 occurrences):
  - Line 41: `std::unique_ptr<themis::sharding::proto::ShardService::Stub> stub;`
  - Line 152: `stub = themis::sharding::proto::ShardService::NewStub(channel);`
  - Line 431: `grpc::Status status = impl_->stub->PrepareTransaction(&context, request, &response);`
  - Line 457: `grpc::Status status = impl_->stub->CommitTransaction(&context, request, &response);`
  - Line 482: `grpc::Status status = impl_->stub->AbortTransaction(&context, request, &response);`

**🎭 SIMULATION** (6 occurrences):
  - Line 61: `// Detect if we should use gRPC or in-process simulation`
  - Line 70: `// Force in-process simulation if gRPC is not available`
  - Line 315: `// Fall back to in-process simulation for single-node deployments`
  - Line 597: `// In-process simulation for single-node deployments`
  - Line 616: `// Simulate network delay`

---

### `src/sharding/shard_rpc_server.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/shard_topology.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/sharding_manager_edition.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/signed_request.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/slo_monitor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/stream_protocol.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (92.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 544: `// For now, simulate successful preparation`

**🔒 HARDCODED** (2 occurrences):
  - Line 1092: `// Local implementation: write chunk to temporary staging area`
  - Line 1095: `// Security: Use platform-specific temporary directory instead of hardcoded /tmp`

---

### `src/sharding/transaction_snapshot.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🐛 DEBUG** (2 occurrences):
  - Line 281: `spdlog::debug("Saved transaction snapshot to {}", filepath);`
  - Line 314: `spdlog::debug("Loaded transaction snapshot from {}", filepath);`

---

### `src/sharding/transaction_wal.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (91.0/100)

**Issues Found:**

**🐛 DEBUG** (9 occurrences):
  - Line 65: `spdlog::debug("Logged BEGIN for transaction {} at LSN {}", transaction_id, lsn.toString());`
  - Line 85: `spdlog::debug("Logged PREPARE for transaction {} participant {} at LSN {}",`
  - Line 113: `spdlog::debug("Logged PREPARED for transaction {} participant {} vote={} at LSN {}",`
  - Line 132: `spdlog::debug("Logged COMMIT for transaction {} at LSN {}", transaction_id, lsn.toString());`
  - Line 150: `spdlog::debug("Logged COMMITTED for transaction {} participant {} at LSN {}",`

---

### `src/sharding/truetime.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/two_phase_commit_coordinator.cpp` (v0.0.20)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/two_phase_commit_participant.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/urn.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/urn_resolver.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/wal_applier.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/wal_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/sharding/wal_shipper.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/storage/backup_manager.cpp` (v0.0.33)

**Maturity Level:** 🟠 BETA (50.0/100)

**Issues Found:**

**🔴 STUB** (6 occurrences):
  - Line 1398: `// GAP-008: Cloud Backup & Snapshot Scheduling (Stub Implementation)`
  - Line 1406: `THEMIS_WARN("scheduleBackup is a stub - K8s CronJob integration not yet implemented");`
  - Line 1422: `THEMIS_WARN("cancelScheduledBackup is a stub - not yet implemented");`
  - Line 1432: `THEMIS_WARN("listScheduledBackups is a stub - not yet implemented");`
  - Line 1443: `THEMIS_WARN("uploadBackupToCloud is a stub - cloud integration not yet implemented");`

**🎭 SIMULATION** (2 occurrences):
  - Line 1060: `// Placeholder: simulate successful upload`
  - Line 1076: `// Simulate successful download`

**📝 TODO** (7 occurrences):
  - Line 970: `// TODO: Integrate actual compression libraries`
  - Line 1012: `// TODO: Integrate OpenSSL for AES-256-GCM encryption`
  - Line 1054: `// TODO: Integrate cloud storage SDKs`
  - Line 1411: `// TODO: Implement K8s CronJob creation or internal scheduler`
  - Line 1434: `// TODO: Implement K8s CronJob listing or internal scheduler query`

---

### `src/storage/base_entity.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/storage/batch_write_optimizer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/storage/blob_backend_azure.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/storage/blob_backend_filesystem.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/storage/blob_backend_s3.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/storage/blob_backend_webdav.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/storage/blob_redundancy_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (93.0/100)

**Issues Found:**

**🐛 DEBUG** (8 occurrences):
  - Line 328: `spdlog::debug("Registered blob: {} (type={}, size={} bytes)",`
  - Line 346: `spdlog::debug("Unregistered blob: {}", blob_id);`
  - Line 642: `spdlog::debug("Running blob redundancy maintenance cycle");`
  - Line 665: `spdlog::debug("Blob {} eligible for tier-down", tier_candidates[i]);`
  - Line 682: `spdlog::debug("Processing blob repair queue");`

---

### `src/storage/columnar_format.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 1206: `spdlog::debug("Encoded column: {} -> {} bytes ({}x compression)",`

---

### `src/storage/compaction_manager.cpp` (v0.0.32)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/storage/compressed_storage.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/storage/compression_strategy.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/storage/database_connection_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (93.0/100)

**Issues Found:**

**🐛 DEBUG** (7 occurrences):
  - Line 63: `spdlog::debug("Acquired idle connection from pool");`
  - Line 67: `spdlog::debug("Connection stale, removing from pool");`
  - Line 158: `spdlog::debug("Removing unhealthy connection from pool");`
  - Line 168: `spdlog::debug("Released connection to idle pool ({} idle)",`
  - Line 175: `spdlog::debug("Performing health check on all connections");`

---

### `src/storage/disk_space_monitor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/storage/history_manager.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/storage/hlc.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/storage/index_maintenance.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (94.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 482: `// For now, simulate fragmentation calculation`
  - Line 647: `// Simulate statistics update`

---

### `src/storage/key_schema.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/storage/merge_operators.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/storage/mvcc_store.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/storage/nlp_metadata_extractor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/storage/pitr_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/storage/raft_mvcc_bridge.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🐛 DEBUG** (3 occurrences):
  - Line 62: `spdlog::debug("RaftMvccBridge::snapshotTimestamp raft_log_idx={} hlc={}",`
  - Line 79: `spdlog::debug("RaftMvccBridge::linearizableRead – not leader, redirect required");`
  - Line 108: `spdlog::debug("RaftMvccBridge::raftAwareWrite key='{}' ts={}",`

---

### `src/storage/rocksdb_wrapper.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 1421: `// TODO: Implement proper size calculation`

**🐛 DEBUG** (3 occurrences):
  - Line 64: `// ✅ DEBUG: Mark source object as being moved`
  - Line 65: `// In debug mode, we fail fast on concurrent move operations`
  - Line 96: `// Fail fast in debug mode if concurrent operations detected`

---

### `src/storage/security_signature.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/storage/security_signature_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 88: `// TODO: Implement proper RocksDB iteration when RocksDBWrapper supports it`

---

### `src/storage/storage_audit_logger.cpp` (v0.0.32)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/storage/storage_engine.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (91.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 107: `// Default implementation: return a dummy key`
  - Line 109: `return std::vector<uint8_t>(32, 0x42); // 32-byte dummy key`
  - Line 113: `// Default implementation: return the same dummy key`

---

### `src/storage/transaction_retry_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/storage/wal_storage.cpp` (v0.0.32)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/stubs.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (85.0/100)

**Issues Found:**

**🔴 STUB** (3 occurrences):
  - Line 1: `// Stub implementations for linking purposes`
  - Line 2: `// These stubs allow themis_tests to link successfully`
  - Line 14: `// Stub`

---

### `src/temporal/bi_temporal.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/temporal/retention_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/temporal/snapshot_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/temporal/system_versioned_table.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/temporal/temporal_aggregator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/temporal/temporal_conflict_resolver.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/temporal/temporal_index.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/temporal/temporal_query_engine.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/themis/build_info.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 85: `config.build_type = "Debug";`

---

### `src/themis/edition_manager.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/themis/module_dependency_resolver.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/themis/module_hash_verifier.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 164: `spdlog::debug(`

---

### `src/themis/module_signature_verifier.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/themis/wire_protocol_server.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/timeseries/aggregate_scheduler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/timeseries/aggregate_scheduler_helper.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/timeseries/aggregates.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/timeseries/continuous_agg.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/timeseries/gorilla.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/timeseries/hypertable.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/timeseries/prometheus_remote_write.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/timeseries/query_optimizer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/timeseries/retention.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/timeseries/timeseries.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/timeseries/timeseries_metrics.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/timeseries/ts_auto_buffer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/timeseries/tsstore.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 189: `// TODO: Implement buffering strategy for single-point inserts with Gorilla`

---

### `src/training/auto_labeler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 85: `// In-process simulation: use an empty list (database not connected in test env)`

---

### `src/training/incremental_lora_trainer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (91.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 112: `// Phase 3: Training loop with forward/backward pass simulation`
  - Line 386: `// Simulate a decreasing loss curve (Phase 3)`
  - Line 423: `// For test environment, simulate a valid checkpoint`

---

### `src/training/knowledge_graph_enricher.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/training/lora_data_selection.cpp` (v0.0.25)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 124: `// Simulate permutations via (a*hash + b) % p  (universal hashing)`

---

### `src/training/training_pipeline.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 207: `// In simulation: run with an empty candidate list so the pipeline`

---

### `src/transaction/branch_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/transaction/crash_recovery_manager.cpp` (v0.0.31)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/transaction/distributed_saga.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/transaction/global_transaction_manager.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/transaction/lock_manager.cpp` (v0.0.31)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/transaction/merge_engine.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🐛 DEBUG** (2 occurrences):
  - Line 237: `spdlog::debug("Source diff: {} changes, Target diff: {} changes",`
  - Line 273: `spdlog::debug("Detected {} conflicts", conflicts.size());`

---

### `src/transaction/saga.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (92.0/100)

**Issues Found:**

**📝 TODO** (2 occurrences):
  - Line 232: `// TODO: Need access to RocksDB for creating compensation batch`
  - Line 248: `// TODO: Store edge details for proper compensation`

**🔒 HARDCODED** (1 occurrences):
  - Line 229: `// Create a temporary batch for compensation`

---

### `src/transaction/snapshot_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/transaction/transaction_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/updates/blue_green_deployment.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/updates/canary_rollout.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/updates/coordinated_update_manager.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/updates/delta_update_engine.cpp` (v0.0.29)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/updates/hot_reload_engine.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/updates/in_place_schema_migrator.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/updates/manifest_database.cpp` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (78.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 456: `// TODO: Delete associated files from registry`

**🔒 HARDCODED** (5 occurrences):
  - Line 180: `// For this, we create a temporary file containing the hash`
  - Line 183: `// Get system temporary directory and create cryptographically secure random filename`
  - Line 204: `// Create temporary file with restricted permissions`
  - Line 207: `LOG_ERROR("Failed to create temporary file for manifest verification");`
  - Line 222: `// Clean up temporary file`

---

### `src/updates/notification_webhook.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/updates/release_manifest.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/updates/schema_migration_tester.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/updates/update_history_logger.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/updates/update_state_machine.cpp` (v0.0.31)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/updates/updates_config.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/utils/audit_logger.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 27: `// TODO: In production, this should be derived from a central version header`

---

### `src/utils/boost_throw_exception.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/utils/build_info.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (94.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 527: `"Hardware Security Module integration (stub only)"`

**🐛 DEBUG** (1 occurrences):
  - Line 82: `config.build_type = "Debug";`

---

### `src/utils/capability_auto_generator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (94.0/100)

**Issues Found:**

**📝 TODO** (3 occurrences):
  - Line 171: `// TODO: Check last update time and compare with schedule interval`
  - Line 351: `// TODO: Store previous document count somewhere`
  - Line 413: `// TODO: Implement YAML serialization and file writing`

---

### `src/utils/checksum_utils.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/utils/compression_metrics.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/utils/cron_parser.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/utils/cursor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/utils/error_registry.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (90.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 665: `"2. Replace MCP stub implementations\n"`
  - Line 669: `{"mcp", "schema", "unavailable", "stub"}`

---

### `src/utils/file_utils.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/utils/geo/ewkb.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/utils/grpc_channel_pool.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 344: `// Create a temporary probe channel`

---

### `src/utils/hkdf_cache.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/utils/hkdf_helper.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/utils/http_client_pool.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/utils/input_validator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 269: `// Pass minimal stub schema if available`

---

### `src/utils/lek_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/utils/license_info.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/utils/logger.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (4 occurrences):
  - Line 57: `case Level::DEBUG:    return spdlog::level::debug;`
  - Line 184: `case spdlog::level::debug: return Level::DEBUG;`
  - Line 248: `if (s == "debug")                       return Level::DEBUG;`
  - Line 259: `case Level::DEBUG:    return "debug";`

---

### `src/utils/memory/pool_allocator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/utils/ner_detection_engine.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/utils/normalizer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/utils/pii_detection_engine.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/utils/pii_detector.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/utils/pii_pseudonymizer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/utils/pki_client.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (89.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 434: `// Fallback: stub behavior (base64 of hash)`
  - Line 553: `// Fallback stub verification: compare base64(hash) equality`

**🐛 DEBUG** (1 occurrences):
  - Line 350: `// Always print diagnostics when debug enabled; also print minimal info on failure`

---

### `src/utils/regex_detection_engine.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/utils/retention_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/utils/runtime_license_gate.cpp` (v0.0.27)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/utils/saga_logger.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/utils/self_awareness.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (92.0/100)

**Issues Found:**

**📝 TODO** (4 occurrences):
  - Line 99: `snapshot.confidence_score = 1.0;  // TODO: Calculate based on data quality`
  - Line 125: `// TODO: Add to audit log that self-awareness was triggered`
  - Line 209: `// TODO: Query ShardTopology for actual shard information`
  - Line 235: `// TODO: Query AdaptiveShardRouter for statistics`

---

### `src/utils/serialization.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/utils/simd_distance.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/utils/stemmer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/utils/stopwords.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/utils/thread_pool_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (93.0/100)

**Issues Found:**

**📝 TODO** (2 occurrences):
  - Line 43: `// TODO: Implement priority queue for proper priority-based scheduling`
  - Line 119: `// TODO: Implement proper task latency tracking by storing execution times`

**🐛 DEBUG** (3 occurrences):
  - Line 280: `spdlog::debug("ThreadPool Metrics - IO: active={}, queued={}, executed={}, failed={}",`
  - Line 286: `spdlog::debug("ThreadPool Metrics - CPU: active={}, queued={}, executed={}, failed={}",`
  - Line 292: `spdlog::debug("ThreadPool Metrics - Blocking: active={}, queued={}, executed={}, failed={}",`

---

### `src/utils/tracing.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/utils/update_checker.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/utils/zstd_codec.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/version.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/voice/audio_preprocessing.cpp` (v0.0.28)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/voice/emotion_analyzer.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/voice/voice_accessibility.cpp` (v0.0.28)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/voice/voice_assistant.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/voice/voice_assistant_llm.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/voice/voice_audio_storage.cpp` (v0.0.28)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/voice/voice_authenticator.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/voice/voice_batch_processor.cpp` (v0.0.28)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/voice/voice_error_handler.cpp` (v0.0.28)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/voice/voice_intent_detector.cpp` (v0.0.28)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/voice/voice_macro_manager.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/voice/voice_meeting_support.cpp` (v0.0.28)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/voice/voice_model_cache.cpp` (v0.0.28)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/voice/voice_security.cpp` (v0.0.28)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 123: `// Create a temporary manager to check without modifying state`

---

### `src/voice/voice_session_manager.cpp` (v0.0.28)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/voice/voice_tts_customizer.cpp` (v0.0.28)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `src/voice/wake_word_detector.cpp` (v0.0.2)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/analytics/test_anomaly_detection.cpp` (v0.0.18)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/analytics/test_arrow_export.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/analytics/test_arrow_flight.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/analytics/test_automl.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/analytics/test_cep_engine.cpp` (v0.0.18)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 1272: `// Re-register the same rule (simulate fresh start)`

---

### `tests/analytics/test_columnar_execution.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/analytics/test_distributed_analytics.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 8: `*  - executeDistributed: scatter-gather with LocalShardExecutor stubs`

**🎭 SIMULATION** (2 occurrences):
  - Line 113: `// A dummy executor that always returns empty`
  - Line 385: `// Simulate CUBE subtotals: grouping_id=0 = detail, grouping_id=1 = subtotal`

---

### `tests/analytics/test_forecasting.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/analytics/test_incremental_view.cpp` (v0.0.18)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 636: `// Simulate incremental changes without touching source collection`

---

### `tests/analytics/test_jit_aggregation.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/analytics/test_llm_process_analyzer.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 9: `*  - analyze() — full round-trip using the built-in simulated callLLM stub;`

**🎭 SIMULATION** (1 occurrences):
  - Line 443: `// deviations should be initialized (may be empty in simulation)`

---

### `tests/analytics/test_ml_serving.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/analytics/test_model_serving.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/analytics/test_process_mining_llm.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 49: `// Helper to simulate LLM analysis`

---

### `tests/analytics/test_process_pattern_matcher.cpp` (v0.0.18)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 5: `* via the public API.  For methods that require a live DB, a temporary`

---

### `tests/analytics/test_streaming_window.cpp` (v0.0.18)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (4 occurrences):
  - Line 617: `// BUG 2: SessionWindow::ingest should not regress last_event on OOO records.`
  - Line 645: `// BUG 4: SessionWindow must track watermark and drop records flagged as late.`
  - Line 667: `// BUG 5: DISTINCT_COUNT must not count records where the field is absent.`
  - Line 691: `// BUG 7: StreamingWindowPipeline must refuse ingest()/flush() before build().`

---

### `tests/byzantine_attacks.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (94.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 13: `// Attack Simulation Utilities`
  - Line 107: `// Label flipping attack: simulate poisoned data by inverting gradient signs selectively`

---

### `tests/chimera/adapter_test_utils.hpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/chimera/test_adapter_factory.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 15: `* @brief Mock adapter for testing`

---

### `tests/chimera/test_capability_matrix.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/chimera/test_elasticsearch_adapter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 11: `* in its in-process simulation mode.`

---

### `tests/chimera/test_mongodb_adapter.cpp` (v0.0.8)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 720: `static constexpr size_t kVecDim     = 32;    // small dim for simulation layer`
  - Line 725: `// Generous wall-clock limits for in-process simulation in debug CI builds.`

**🐛 DEBUG** (1 occurrences):
  - Line 725: `// Generous wall-clock limits for in-process simulation in debug CI builds.`

---

### `tests/chimera/test_neo4j_adapter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 11: `* its in-process simulation mode.`

---

### `tests/chimera/test_pinecone_adapter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 11: `* its in-process simulation mode.`

---

### `tests/chimera/test_postgresql_adapter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 6: `*          (pgvector simulation), document JSONB operations, transaction`
  - Line 12: `* its in-process simulation mode.`
  - Line 196: `// Vector adapter tests (pgvector simulation)`
  - Line 356: `// Document adapter tests (JSONB simulation)`

---

### `tests/chimera/test_qdrant_adapter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 11: `* its in-process simulation mode.`
  - Line 737: `// All tests run against the in-process simulation mode (no live Qdrant`
  - Line 758: `// Generous wall-clock limits for the in-process simulation layer.`

**🐛 DEBUG** (1 occurrences):
  - Line 739: `// slow debug/CI builds while still guarding against major regressions.`

---

### `tests/chimera/test_themisdb_adapter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 20: `* its in-process simulation mode.`
  - Line 425: `// All tests run against the in-process simulation layer (no live ThemisDB`
  - Line 450: `// Generous wall-clock limits for the in-process simulation layer.`

**🐛 DEBUG** (1 occurrences):
  - Line 427: `// debug/CI builds while still guarding against major regressions.`

---

### `tests/chimera/test_weaviate_adapter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 11: `* its in-process simulation mode.`
  - Line 735: `// All tests run against the in-process simulation mode (no live Weaviate`
  - Line 756: `// Generous wall-clock limits for the in-process simulation layer.`

**🐛 DEBUG** (1 occurrences):
  - Line 737: `// slow debug/CI builds while still guarding against major regressions.`

---

### `tests/db/test_concurrent_operations.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 14: `* - Real implementations (no stubs)`

---

### `tests/db/test_data_consistency.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 13: `* - Real crash simulation`
  - Line 249: `// Simulate crash and restart`

---

### `tests/db/test_index_performance.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/db/test_transaction_isolation.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 15: `* - Real implementations (no stubs)`

---

### `tests/debug_graph_keys_test.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/exporters/test_aql_predicate_filter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/exporters/test_arrow_ipc_exporter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/exporters/test_data_augmentation.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/exporters/test_export_encryption.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/exporters/test_format_template.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/exporters/test_huggingface_exporter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/exporters/test_incremental_exporter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/exporters/test_jsonl_llm_exporter.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 520: `entity.setField("ssn", "000-00-0000");  // Sensitive field (obviously fake format)`

---

### `tests/exporters/test_parquet_exporter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/exporters/test_streaming_exporter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/fixtures/mock_shard_cluster.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (88.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 29: `// Simulate packet loss`
  - Line 34: `// Simulate network delay`
  - Line 61: `// Simulate packet loss`
  - Line 66: `// Simulate network delay`

---

### `tests/fixtures/mock_shard_cluster.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (88.0/100)

**Issues Found:**

**🎭 SIMULATION** (9 occurrences):
  - Line 17: `* @brief Mock shard cluster for testing distributed LoRA operations`
  - Line 20: `* - Network latency simulation`
  - Line 22: `* - Shard failure simulation`
  - Line 23: `* - Network partition simulation`
  - Line 52: `* @brief Construct mock shard cluster`

---

### `tests/geo/test_aql_st_functions.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/geo/test_aql_st_queryengine.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/geo/test_cuda_geo_kernels.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 91: `// Empty dispatch table — always uses the CPU stub path.`

---

### `tests/geo/test_geo_3d_functions.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/geo/test_geo_clustering.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/geo/test_geo_device_detector.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/geo/test_geo_ewkb.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/geo/test_geo_precision_mode.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/geo/test_geo_raster.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/geo/test_geo_rtree.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/geo/test_geo_spatial_join.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/geo/test_geo_st_buffer.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/geo/test_geo_st_union_difference.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/geo/test_geo_tile_server.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/geo/test_geo_wgs84_spherical.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 159: `EXPECT_GT(gpu_dist, 0.0) << "GPU geodesicDistance must return real distance, not stub 0";`

---

### `tests/geo/test_gpu_backend_production.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/geo/test_gpu_kernel_dispatcher.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 32: `// Empty dispatch table → CPU stub behaviour.`

---

### `tests/geo/test_hip_geo_kernels.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 91: `// Empty dispatch table — always uses the CPU stub path.`

---

### `tests/geo/test_rtree_cpu_integration.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 16: `// Test fixture: creates a temporary RocksDB, sets up a SpatialIndexManager and`

---

### `tests/geo/test_spatial_index.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 290: `// TODO: Investigate if this is double-counting or correct behavior`

---

### `tests/geo/test_temporal_spatial_query.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/index/test_ann_index.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/index/test_distributed_vector_index.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/index/test_hnsw_recall_integration.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/index/test_learned_index.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/index/test_spatial_correctness_integration.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 14: `// All tests use a real (temporary) RocksDB instance and the built-in CPU exact`

---

### `tests/index/test_tiered_index_migration.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/integration/end_to_end/full_query_flow_e2e_test.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 148: `// Step 1: Insert documents with mock embeddings`
  - Line 161: `{"embedding", std::vector<float>(128, 0.1f * i)} // Mock 128-dim embedding`

---

### `tests/integration/end_to_end/storage_pipeline_e2e_test.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 29: `// Mock storage interfaces for testing`

---

### `tests/integration/hot_reload_manager_integration_test.cpp` (v0.0.7)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/integration/llm/llm_workflow_integration_test.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/integration/rpc/rpc_service_integration_test.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 368: `// Step 1: Simulate multiple clients using the same service`

---

### `tests/integration/security/encryption_key_rotation_integration_test.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (6 occurrences):
  - Line 28: `* @brief Mock Key Provider for testing`
  - Line 98: `// Create mock key provider`
  - Line 219: `// Step 3: Simulate background re-encryption`
  - Line 266: `// Step 4: Simulate concurrent reads (should still work)`
  - Line 278: `// Step 5: Simulate concurrent writes (should use available key)`

---

### `tests/integration/security/zero_trust_access_control_integration_test.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/integration/storage/backup_recovery_integration_test.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/integration/test_content_processing.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (9 occurrences):
  - Line 29: `* @brief Mock content processor for testing`
  - Line 54: `{"transcription", "This is a mock audio transcription."}`
  - Line 143: `* @brief Mock MIME type detector`
  - Line 188: `// Create mock audio file`
  - Line 190: `// Mock MP3 header`

---

### `tests/integration/test_cross_functional_plugin_query_metrics.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 208: `// Simulate cache hit on subsequent rounds`
  - Line 426: `// Simulate resource usage during reload`

---

### `tests/integration/test_cross_functional_voice_observability.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 100: `// Simulate phone call audio (10KB)`
  - Line 139: `// Simulate meeting audio (100KB)`
  - Line 279: `// Simulate index updates`

---

### `tests/integration/test_data_generator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/integration/test_distributed_training_e2e.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (6 occurrences):
  - Line 34: `* @brief Mock shard worker for testing`
  - Line 50: `// Simulate training computation`
  - Line 63: `// Simulate realistic loss values (decreasing over iterations)`
  - Line 84: `* @brief Mock coordinator for aggregating losses`
  - Line 156: `// Parallel execution simulation`

---

### `tests/integration/test_fixture.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (3 occurrences):
  - Line 35: `// Create temporary directory for test data`
  - Line 42: `// Clean up temporary directory`
  - Line 71: `* @brief Get the temporary directory path for this test`

---

### `tests/integration/test_graphql_e2e.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (5 occurrences):
  - Line 27: `* @brief Mock GraphQL executor for testing`
  - Line 43: `// Parse and execute query (mock implementation)`
  - Line 67: `// Mock user data`
  - Line 97: `// Mock product data`
  - Line 310: `// Basic validation (full nesting would require more mock data)`

---

### `tests/integration/test_helpers.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/integration/test_process_mining_e2e.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🔴 STUB** (3 occurrences):
  - Line 36: `// Load test models (may be empty in stub implementation)`
  - Line 105: `GTEST_SKIP() << "Administrative model not available in test stub";`
  - Line 273: `GTEST_SKIP() << "Administrative model not available in test stub";`

**🎭 SIMULATION** (1 occurrences):
  - Line 276: `// 2. Simulate real process`

**📝 TODO** (3 occurrences):
  - Line 133: `// TODO: Test conformance checking`
  - Line 237: `// TODO: Test vector embedding and similarity search`
  - Line 243: `// TODO: Test graph analytics (centrality, communities)`

---

### `tests/integration/test_rpc_database_operations.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/llm/bench_continuous_batch_scheduler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/llm/bench_model_loading_from_themisdb.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (94.0/100)

**Issues Found:**

**🔒 HARDCODED** (2 occurrences):
  - Line 9: `* - Temporary file streaming`
  - Line 300: `// Benchmark: Temporary File Write`

---

### `tests/llm/test_ai_orchestrator.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 432: `// AIOrchestrator – no LLM plugin (stub mode)`

---

### `tests/llm/test_extended_context.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/llm/test_gpu_lora_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/llm/test_grammar_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/llm/test_inference_performance.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (10 occurrences):
  - Line 50: `// Helper to simulate token generation`
  - Line 74: `// Simulate inference`
  - Line 102: `// Simulate longer inference`
  - Line 154: `// Simulate token generation`
  - Line 156: `// Simulate generation time`

---

### `tests/llm/test_inference_quality.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 524: `// Simulate some processing`

---

### `tests/llm/test_json_schema_binding.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/llm/test_kernel_fusion_cpu_fallback.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/llm/test_kernel_fusion_cuda.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/llm/test_llama_cpp_tokenizer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/llm/test_llama_wrapper_state.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 6: `* that prevents silent stub responses and enables proper error handling.`
  - Line 223: `// Old behavior: Would return STUB_RESPONSE silently`

---

### `tests/llm/test_llm_audit_logger.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/llm/test_llm_deployment_plugin.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/llm/test_llm_validator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 220: `TEST_F(LLMValidatorTest, StubResponseDetection) {`
  - Line 223: `std::string text = "STUB_RESPONSE";`

---

### `tests/llm/test_lora_adapter_application.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 128: `// Cached load should be equal or faster (in mock mode both are instant, so allow equality)`
  - Line 163: `// Apply with null context (mock mode for testing)`
  - Line 169: `// Create mock llama context`

---

### `tests/llm/test_lora_adapters.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 55: `// Helper to create mock adapter file`
  - Line 501: `// Simulate loading (read file)`

---

### `tests/llm/test_lora_auto_binding.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 42: `// Create test adapter files (mock LoRA weights)`
  - Line 73: `// Write mock LoRA weights (just placeholder data)`

**🔒 HARDCODED** (1 occurrences):
  - Line 38: `// Create temporary directory for test adapters`

---

### `tests/llm/test_lora_hot_loading.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (5 occurrences):
  - Line 30: `/// Minimal mock plugin that tracks which LoRA adapters it has loaded.`
  - Line 33: `explicit LoRATrackingPlugin(const std::string& model_id = "mock-model")`
  - Line 102: `/// Fixture that provides a running InferenceEngineEnhanced with one mock model.`
  - Line 124: `/// Helper: create a mock adapter file and return its path.`
  - Line 250: `// The mock plugin echoes lora_used from the request`

---

### `tests/llm/test_mcp_orchestrator_bridge.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/llm/test_model_loader_async.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/llm/test_model_loader_error_handling.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/llm/test_model_loading_best_practices.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 57: `// Helper to create a mock model file`
  - Line 219: `// Create mock model files`
  - Line 399: `// Read entire file to simulate loading`
  - Line 431: `// Simulate loading`

---

### `tests/llm/test_model_loading_from_themisdb.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 100: `// Create a small fake GGUF file (just some bytes for testing)`

**🔒 HARDCODED** (1 occurrences):
  - Line 40: `// Create temporary directories`

---

### `tests/llm/test_openai_compat_adapter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/llm/test_real_embeddings.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 100: `// Mock model (in production, load real model)`

**🔒 HARDCODED** (1 occurrences):
  - Line 62: `// Create temporary cache directory`

---

### `tests/llm/test_streaming_handler.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/mock_user_registration_plugin.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (90.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 12: `* @brief Mock User Registration Plugin for Testing`
  - Line 21: `return "mock";`
  - Line 36: `data.source = "mock";`
  - Line 58: `data.source = "mock";`

---

### `tests/penetration_tests.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/performance/phase3/test_adaptive_batch_tuner.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/performance/phase3/test_bao.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 108: `// Simulate that plan_0 is always fast, plan_1 is always slow`

---

### `tests/performance/phase3/test_bwtree.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 287: `// This creates temporary consolidated views for reading`

---

### `tests/performance/phase3/test_diskann.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/performance/phase3/test_gunrock.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/performance/phase3/test_memory_pressure.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 242: `// Simulate a cache with a size counter`

---

### `tests/performance/phase3/test_per_query_cost_model.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 49: `// Simulate a tiny workload`

---

### `tests/performance/phase3/test_simd_distance.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 109: `float dummy = 0.0f;`
  - Line 110: `EXPECT_FLOAT_EQ(l2_distance(&dummy, &dummy, 0), 0.0f);`
  - Line 111: `EXPECT_FLOAT_EQ(l2_distance_sq(&dummy, &dummy, 0), 0.0f);`

---

### `tests/performance/phase3/test_splinterdb.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 93: `// Simulate work`

---

### `tests/performance/phase4/test_io_uring_zero_copy.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/performance/phase4/test_pmem_storage.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 17: `// Helper: temporary pool file that is removed after each test`

---

### `tests/performance/phase4/test_pmu_counters.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/performance/test_cycle_metrics.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/performance/test_numa_topology.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/performance/test_wire_perf_benchmark.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/performance/test_workload_predictor.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/query/test_pagerank.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/rope_visualizer/test_utils.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/security/test_access_control_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 13: `// Create temporary config files`

---

### `tests/security/test_fips_crypto_mode.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/security/test_input_validation_security.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 132: `"'; return true; var dummy='",`

---

### `tests/security/test_row_level_security.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/security/test_security_negative_integration.cpp` (v0.0.29)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/temporal/test_bi_temporal.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 110: `// Since insertWithValidTime rejects overlaps, simulate this by`

---

### `tests/temporal/test_retention_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/temporal/test_snapshot_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/temporal/test_system_versioned_table.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/temporal/test_temporal_aggregator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/temporal/test_temporal_conflict_resolver.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/temporal/test_temporal_index.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/temporal/test_temporal_query_engine.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_ab_test_manager.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_ab_testing_framework.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_acceleration.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_acceleration_coverage.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 99: `EXPECT_STREQ(errorCodeToString(AccelerationErrorCode::NotImplemented), "NotImplemented");`

---

### `tests/test_acceleration_dispatch.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_acceleration_metrics.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_acceleration_regression.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_access_control.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 27: `// Register mock plugin for testing`
  - Line 57: `// The mock plugin validates passwords internally`
  - Line 61: `// Mock plugin may or may not enforce validation - behavior depends on plugin`

---

### `tests/test_access_control_abac.cpp` (v0.0.2)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_access_control_injection.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_adapter_sync.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_adaptive_cache_fuzz.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_adaptive_cache_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_adaptive_cache_phase1.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_adaptive_index.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 422: `// Simulate frequent user lookups by email`
  - Line 441: `// Simulate age range queries`

---

### `tests/test_adaptive_optimizer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_adaptive_query_cache.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 15: `// Use temporary directory for test cache`

---

### `tests/test_adaptive_shard_router.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 34: `// Create mock URN resolver with hash ring`
  - Line 37: `// Create mock remote executor with config`

---

### `tests/test_adaptive_throttling_comprehensive.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_advanced_vector_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_ai_decision_auditor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 14: `// Setup temporary database`

---

### `tests/test_alert_rules.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aligned_vector_cache.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_alignment_helpers.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 330: `// Simulate cache-line aligned structure`
  - Line 344: `// Simulate SIMD vector types`

---

### `tests/test_anomaly_detection.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_api_auth_config.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_api_gateway.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 463: `// Simulate Kong forwarding: "client, kong-proxy"`

---

### `tests/test_api_grpc_server.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_api_integration.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_api_key_authenticator.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_api_key_mgmt_handler.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_api_routing.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_api_security_audit.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_api_version.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_approximate_radius_search_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_api_stability.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_autocomplete.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_bm25.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_confidence_scorer.cpp` (v0.0.25)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_conversation_context.cpp` (v0.0.25)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 76: `// Inject one fake turn so refine() doesn't throw on turn_count_ == 0`
  - Line 78: `try { ctx->start("dummy intent"); } catch (...) {}`
  - Line 141: `// a fake turn to bypass the precondition and test refine() independently.`

---

### `tests/test_aql_explain.cpp` (v0.0.11)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 3: `// without touching any real storage (null storage / mock index manager).`
  - Line 21: `// Minimal mock implementations (no real storage / indexes needed for EXPLAIN)`

---

### `tests/test_aql_fewshot_example_library.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_fulltext_hybrid.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_functions.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_general_traversal.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_injection_detector.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_join_minimal.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_let.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_let_st.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_lora_finetuner.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 238: `// In simulation mode the service may return success; check based on result`
  - Line 257: `// The callback may or may not fire depending on the underlying simulation`

---

### `tests/test_aql_migration_assistant.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_multi_statement_transaction.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_optimizer_advisor.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_or.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_or_not.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_parser.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_path_constraints.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_proximity.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_proximity_dispatch.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_proximity_let.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_query_builder.cpp` (v0.0.25)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_query_template_library.cpp` (v0.0.25)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_query_validator.cpp` (v0.0.25)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_schema_aware.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_shortest_path.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_shortest_path_dispatch.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_shortestpath.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_similarity.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_similarity_dispatch.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_similarity_let.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_st_predicates.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (2 occurrences):
  - Line 14: `// TODO: Update tests to use correct TranslationResult API once spatial predicates`
  - Line 52: `// TODO: Rewrite tests once spatial predicates are fully implemented and API is stabilized`

---

### `tests/test_aql_subqueries.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_syntax_highlighter.cpp` (v0.0.22)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_translator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_aql_with_clause.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_arc_cache.cpp` (v0.0.32)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_archive_processor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 35: `// Helper: Create a simple ZIP file (mock)`
  - Line 44: `// Add some dummy data`

---

### `tests/test_argument_store.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_async_io_multiscan.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 14: `// TODO(v1.3.0): RocksDB wrapper API changed (iterator interface). Disable tests until updated to ne`

---

### `tests/test_async_job_api.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 175: `auto dummy    = std::make_shared<AsyncJobRecord>();`
  - Line 176: `dummy->id     = "trigger";`
  - Line 177: `dummy->status = AsyncJobStatus::PENDING;`
  - Line 178: `dummy->created_at = dummy->updated_at = std::chrono::system_clock::now();`

---

### `tests/test_audit_lek.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_audit_logger.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_audit_logger_production.cpp` (v0.0.31)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_audit_logging_comprehensive.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_auth_anomaly_detection.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_auth_audit_logger.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 310: `// Use a self-signed dummy PEM so the constructor succeeds`
  - Line 320: `"DUMMY+PADsYVZ9lRX3ATQv1xKkjxFdddPXfmHm5+DUMMY==\n"`

---

### `tests/test_auth_error.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_auth_input_validation.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_auth_metrics.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_auth_middleware.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 351: `// Simulate refresh: remove old, add new`

---

### `tests/test_auth_rate_limiter.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_auto_failover_recovery.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (9 occurrences):
  - Line 29: `* @brief Mock replica node for HA testing`
  - Line 139: `// Simulate leader failure`
  - Line 148: `// Election: select new leader (highest ID wins in this simulation)`
  - Line 182: `// Simulate heartbeats`
  - Line 250: `// Simulate network partition: nodes 2-3 separated`

**🔒 HARDCODED** (1 occurrences):
  - Line 450: `* @brief Test recovery after temporary failures`

---

### `tests/test_auto_labeler_production.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_autocomplete.cpp` (v0.0.29)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_backend_api_stability.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 182: `EXPECT_EQ(static_cast<uint32_t>(AccelerationErrorCode::NotImplemented), 902u);`

---

### `tests/test_backend_capability_contract.cpp` (v0.0.13)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_backend_consistency.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_backend_errors.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_backend_registry_startup.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_backend_selection_matrix.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_backup_manager_enhanced.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_backup_restore.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_backup_restore_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 191: `// Manually copy checkpoint to simulate backup`

---

### `tests/test_base_entity.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_base_interfaces.cpp` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (70.0/100)

**Issues Found:**

**🎭 SIMULATION** (17 occurrences):
  - Line 63: `// Mock Implementations`
  - Line 66: `// --- IStorageEngine mock ---`
  - Line 103: `// --- IExpressionEvaluator mock ---`
  - Line 110: `std::string get_expression_type() const override { return "MOCK"; }`
  - Line 113: `// --- IQueryEngine mock ---`

---

### `tests/test_batch_nl_to_aql_translation.cpp` (v0.0.25)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 69: `// Simulate a failing translation by checking the result contract:`

---

### `tests/test_batch_operation_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_batch_write_optimizer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_bayesian_optimizer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 112: `// Simulate an optimization problem: objective = -(top_k - 15)^2`

---

### `tests/test_binary_integrity.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 191: `// Create manifest signer with mock signing service`

**🔒 HARDCODED** (2 occurrences):
  - Line 152: `// Create temporary test file`
  - Line 173: `// Create temporary directory with test files`

---

### `tests/test_binary_protocol_buffers.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 18: `// Create mock components`

---

### `tests/test_binary_quantizer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_blob_storage.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_blob_transfer_checkpoint.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 105: `// Simulate partial transfer by streaming some chunks`
  - Line 125: `// Create a new handler to simulate resume after crash`

---

### `tests/test_blue_green_deployment.cpp` (v0.0.1)

**Maturity Level:** 🟠 BETA (42.0/100)

**Issues Found:**

**🔴 STUB** (16 occurrences):
  - Line 9: `* HotReloadEngine dependency is satisfied by a lightweight stub that overrides`
  - Line 26: `// Minimal stub of HotReloadEngine`
  - Line 29: `class StubHotReloadEngine : public HotReloadEngine {`
  - Line 31: `explicit StubHotReloadEngine(bool apply_succeeds  = true,`
  - Line 36: `c.download_directory = "/tmp/stub_bg_dl";`

---

### `tests/test_bounded_lru_cache.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_bpmn_wire_protocol.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 20: `// Create temporary test directory`

---

### `tests/test_branch_conflict_resolution.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_branch_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_branch_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_build_info.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 191: `// Replace the real commit hash with a fake one`

**🔒 HARDCODED** (1 occurrences):
  - Line 21: `/// Returns a unique, platform-appropriate temporary file path for each test.`

---

### `tests/test_byzantine_detector.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_cache_admin_api_handler.cpp` (v0.0.6)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 228: `R"({"log_path":"/tmp/dummy.ndjson"})");`
  - Line 230: `R"({"out_path":"/tmp/dummy.ndjson"})");`

---

### `tests/test_cache_hit_rate_slo_monitor.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 15: `// Mock alertmanager to capture fired/resolved alerts in tests`
  - Line 269: `// Swap in a mock alertmanager and re-evaluate to fire a new alert`

---

### `tests/test_cache_replication.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 335: `// In-process mock listener that records all received events`
  - Line 340: `explicit MockCacheReplicationListener(const std::string& id = "mock-replica")`
  - Line 453: `// Simulate consecutive failures exceeding threshold (2)`

---

### `tests/test_cache_warmup.cpp` (v0.0.6)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_canary_rollout.cpp` (v0.0.4)

**Maturity Level:** ⚫ DRAFT (0.0/100)

**Issues Found:**

**🔴 STUB** (34 occurrences):
  - Line 11: `* of the existing ManifestDatabase / UpdateChecker stubs.`
  - Line 28: `// Minimal stub of HotReloadEngine`
  - Line 36: `* Stub engine whose applyHotReload() and rollback() behaviour is injectable.`
  - Line 41: `class StubHotReloadEngine : public HotReloadEngine {`
  - Line 43: `explicit StubHotReloadEngine(bool apply_succeeds = true,`

**🎭 SIMULATION** (1 occurrences):
  - Line 10: `* HotReloadEngine dependency is satisfied by a lightweight mock built on top`

---

### `tests/test_capability_matcher.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_catalog_exporter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_ccpa_rules.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_cdc_admin.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 19: `// Create temporary test database`

---

### `tests/test_cdc_change_stream_compressor.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_cdc_consumer_group.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_cdc_cross_collection_stream.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_cdc_dead_letter_queue.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_cdc_debezium_format.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_cdc_delivery_tracker.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_cdc_error_codes.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_cdc_event_enrichment.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_cdc_gdpr_redaction.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_cdc_kafka_producer.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (80.0/100)

**Issues Found:**

**🔴 STUB** (8 occurrences):
  - Line 5: `//  - No-op stub behaviour when THEMIS_ENABLE_KAFKA is not defined`
  - Line 55: `// ── No-op stub or stopped-producer getStats() ─────────────────────────────────`
  - Line 57: `// This test is valid in both stub and full builds: before start() is called`
  - Line 61: `// KafkaCDCProducer requires a Changefeed* but we only need the stub/ctor`
  - Line 72: `// ── No-op stub: start() returns false, publish() returns false ────────────────`

---

### `tests/test_cdc_materialized_view.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_cdc_metrics.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (6 occurrences):
  - Line 264: `// Simulate some work`
  - Line 288: `// Simulate recording 1000 events`
  - Line 293: `// Simulate event recording (1-10us)`
  - Line 301: `// Simulate 10 flushes`
  - Line 304: `// Simulate flush (100-500us)`

---

### `tests/test_cdc_operation_filter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_cdc_outbox.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_cdc_production_fixes.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_cdc_retention.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_cdc_schema_registry.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_cdc_subscription_auth.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_cdc_ws_handler.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_cdn_cache_middleware.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_changefeed_ordering.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_chaos_network.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (5 occurrences):
  - Line 98: `// Oversized Body Simulation`
  - Line 102: `// Simulate: client sends body larger than 10 MB`
  - Line 135: `// Oversized Header Simulation`
  - Line 140: `// Simulate: total header bytes exceeds 8 KB default`
  - Line 243: `// Simulate accepting connections up to limit`

---

### `tests/test_chaos_scheduler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_chunked_response_writer.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_circuit_breaker.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_claim_extractor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 30: `EXPECT_GE(claims.size(), 0);  // May be 0 with stub implementation`
  - Line 55: `// Verdict will depend on LLM (may be NOT_FOUND with stub)`

**🎭 SIMULATION** (1 occurrences):
  - Line 82: `// Create some mock results`

---

### `tests/test_cloud_agent.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 17: `// Create a mock topology`
  - Line 221: `// Manually add to pending (simulate async delegation)`

---

### `tests/test_cloud_backup.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (84.0/100)

**Issues Found:**

**🎭 SIMULATION** (8 occurrences):
  - Line 48: `// Enable mock mode for testing`
  - Line 120: `// Test: Create backup (mock mode)`
  - Line 135: `// In mock mode, should succeed`
  - Line 370: `// Test: Without mock mode (should fail without real SDK)`
  - Line 372: `// Disable mock mode`

**🔒 HARDCODED** (3 occurrences):
  - Line 20: `// Create unique temporary paths for each test`
  - Line 27: `// Create temporary database for testing`
  - Line 59: `// Clean up temporary directories`

---

### `tests/test_cloud_storage_backup_comprehensive.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (4 occurrences):
  - Line 228: `// This is a stub implementation, so should return error indicating not yet implemented`
  - Line 229: `EXPECT_FALSE(result.has_value()) << "Schedule backup is stub, should return not implemented error";`
  - Line 270: `// Check for expected error messages (current implementation is a stub)`
  - Line 653: `// Current stub implementation returns generic error`

**🎭 SIMULATION** (2 occurrences):
  - Line 846: `// Use clearly fake test key (not a predictable pattern)`
  - Line 923: `// Use obviously fake placeholder to avoid confusion with real SAS tokens`

**🔒 HARDCODED** (2 occurrences):
  - Line 58: `* Sets up a temporary database and backup manager for testing`
  - Line 63: `// Create temporary directory for test database`

---

### `tests/test_cms_signing.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_collective_backends.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 198: `// Stub Tests (when backends not available)`
  - Line 204: `// When no backends are compiled in, this test just confirms the stubs work`

---

### `tests/test_column_lineage.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_columnar_format.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 550: `// Simulate analytical workload with repeated values`

---

### `tests/test_community_detection_aql.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_community_detection_functions.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_compaction_manager.cpp` (v0.0.32)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_compliance_reporting.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_compliance_security_governance.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 582: `// PolicyEngine – compliance-aware evaluate / simulate`

---

### `tests/test_composite_index.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_compression_strategy.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_concerns_context.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (91.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 1025: `class StubSecrets : public ISecrets {`
  - Line 1045: `std::make_unique<StubSecrets>()`

**🎭 SIMULATION** (3 occurrences):
  - Line 110: `// Simulate some work`
  - Line 365: `// Simulate a component using the context`
  - Line 378: `// Simulate DB query`

**🐛 DEBUG** (10 occurrences):
  - Line 35: `EXPECT_EQ(ILogger::Level::DEBUG, ILogger::levelFromString("debug"));`
  - Line 45: `EXPECT_STREQ("DEBUG", ILogger::levelToString(ILogger::Level::DEBUG));`
  - Line 55: `context->logger().debug("debug message");`
  - Line 61: `context->logger().setLevel(ILogger::Level::DEBUG);`
  - Line 62: `EXPECT_EQ(ILogger::Level::DEBUG, context->logger().getLevel());`

---

### `tests/test_concurrency_race_detection.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 127: `// Simulate transaction work`

---

### `tests/test_concurrent_cache.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_confidential_computing.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_config.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_config_coverage.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_config_migration_scanner.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 8: `* Tests create temporary file trees in /tmp, exercise the scanning and`

---

### `tests/test_config_path_resolver.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 20: `// Create temporary test directories`

---

### `tests/test_config_schema_validator.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (3 occurrences):
  - Line 332: `"level": { "enum": ["trace", "debug", "info", "warn", "error"] }`
  - Line 344: `"level": { "enum": ["trace", "debug", "info", "warn", "error"] }`
  - Line 487: `r.addWarning("field 'debug' is deprecated");`

---

### `tests/test_connection_compression.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_consensus_module.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_consistent_hash_distribution.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_constitutional_reasoning.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_content_audio_processor.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_content_deduplication.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 246: `// Minimal stub for RocksDBWrapper — use nullptr and verify graceful handling.`

**🎭 SIMULATION** (1 occurrences):
  - Line 178: `// Simulate a PNG-like blob (starts with PNG magic bytes but no decodeable pixel data)`

---

### `tests/test_content_embedding_pipeline.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_content_errors.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_content_features.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_content_fs.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_content_fulltext_index.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (3 occurrences):
  - Line 233: `"Temporary content for deletion test."`
  - Line 241: `"chunk", "text", "temporary", 10);`
  - Line 251: `"chunk", "text", "temporary", 10);`

---

### `tests/test_content_html_processor.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_content_language_detector.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_content_logger.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 173: `logger.debug("test.debug", "Debug message");`

---

### `tests/test_content_markdown_processor.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_content_metrics.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_content_pipeline.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_content_pipeline_hardening.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_content_policy.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 161: `// Create a mock security signature manager (in-memory)`

---

### `tests/test_content_policy_manual.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_content_processor_chain.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_content_security.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 135: `// Simulate some checks to increment metrics`

---

### `tests/test_content_streaming_ingestion.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 180: `// ContentChunker – streaming simulation: chunked read + reassembly`
  - Line 184: `// Simulate what ingestStream does: read a stream in 4 KB blocks and`
  - Line 214: `// Text-segment splitting simulation (mirrors the carry-buffer logic in`
  - Line 263: `// Overflow guard simulation (mirrors the max_buffered_bytes check in ingestStream`

---

### `tests/test_content_version_manager.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_context_propagation.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_continuous_agg.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_continuous_agg_comprehensive.cpp` (v0.0.31)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 445: `// Simulate 2 shards via callback`

---

### `tests/test_continuous_batch_scheduler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 178: `// Simulate completion by generating all tokens`
  - Line 211: `// Simulate some token generation`

---

### `tests/test_continuous_learning_client.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 320: `// Simulate a quality degradation scenario`

---

### `tests/test_continuous_learning_orchestrator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_continuous_profiler.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_coordinated_update_manager.cpp` (v0.0.1)

**Maturity Level:** 🟠 BETA (44.0/100)

**Issues Found:**

**🔴 STUB** (16 occurrences):
  - Line 11: `* lightweight stub that overrides the two virtual methods used by the`
  - Line 27: `// Stub HotReloadEngine`
  - Line 30: `class StubEngine : public HotReloadEngine {`
  - Line 32: `explicit StubEngine(bool apply_ok   = true,`
  - Line 37: `c.download_directory = "/tmp/stub_coord_dl";`

---

### `tests/test_cpu_backend_exact.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 34: `// For the stub CPU backend, we're testing the algorithm implementation`

---

### `tests/test_cpu_gpu_parity.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_crash_recovery_direct.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_crash_recovery_manager.cpp` (v0.0.31)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 54: `// Helper: simulate a "crash" by destroying and recreating the CRM`

---

### `tests/test_cron_parser.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_cross_cluster_federation.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 5: `// HTTP transport is replaced with an injectable mock.`
  - Line 31: `/// Mock HTTP POST that always returns a fixed JSON response.`
  - Line 46: `/// Mock HTTP POST that always fails (transport error → returns 0).`
  - Line 193: `// Execute tests (with HTTP mock)`

---

### `tests/test_cross_lingual_search.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_cross_shard_coordinator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_cross_shard_distribution.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (5 occurrences):
  - Line 6: `* - Cross-shard transfer with network simulation`
  - Line 259: `// Enable network latency simulation`
  - Line 282: `// Should have some latency due to simulation`
  - Line 449: `// Simulate network partition isolating shard 1`
  - Line 484: `// Disable latency simulation for pure transfer performance`

---

### `tests/test_cross_tenant_policy_inheritance.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_cte_cache.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_cte_error_handling.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 20: `// Generate unique temporary path for test databases`

---

### `tests/test_cuda_ann_search.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_cuda_geo_backend.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_cuda_geo_kernels.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_cuda_graph_capture.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_cursor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_data_lineage.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_data_loader.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_data_masker.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_database_connection_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 7: `// Mock connection implementation for testing`
  - Line 50: `// Mock connection manager for testing`
  - Line 289: `// Simulate many errors`

---

### `tests/test_deadlock_detection.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_debug.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: test_debug.cpp has own main() - stubbed for build unblock.";`

---

### `tests/test_device_manager.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 108: `// Simulate two CUDA devices with different free VRAM; verify the one`
  - Line 223: `// (We cannot inject a fake DeviceDiscovery without hardware, so we`

---

### `tests/test_diff_engine.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_directx_backend.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_disk_space_monitor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 320: `// Simulate some operations`

---

### `tests/test_distributed_cache_coordinator.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_distributed_catalog.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_distributed_coordinator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_distributed_flame_graph.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_distributed_saga.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_distributed_task_coordinator.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (7 occurrences):
  - Line 170: `// Simulate this node winning an election`
  - Line 185: `// Simulate another node winning – fire the callback with a different node ID.`
  - Line 261: `// Simulate leadership transfer – notify DTC so it deactivates the scheduler.`
  - Line 328: `// Simulate another node winning leadership`
  - Line 363: `// Simulate a late callback arriving after stop().`

---

### `tests/test_distributed_time_coordinator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 14: `* @brief Mock ConsensusModule for testing DistributedTimeCoordinator`
  - Line 20: `// Set mock values for testing`
  - Line 188: `// Simulate log index progression`

---

### `tests/test_distributed_tracing.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 92: `// Simulate an error`
  - Line 110: `// Simulate work`
  - Line 127: `// Simulate work`

---

### `tests/test_distributed_training_coordinator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 45: `// Create mock shard router and topology`
  - Line 342: `// In a real test with mock shards, this would succeed`

---

### `tests/test_distributed_transactions.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 192: `// Note: This would require a mock server or integration test environment`
  - Line 204: `// Note: This would require a mock server or integration test environment`

---

### `tests/test_distributed_txn_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_docs_assistant_aql.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_domain_durability.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 165: `// Simulate 128-dim embedding`

---

### `tests/test_dynamic_feature_flags.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_ebpf_tracer.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_edition_manager.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_embedded_llm.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 35: `// For now, tests will use stub responses when model is not loaded`
  - Line 163: `// For stub, just check it's not empty`

**🎭 SIMULATION** (1 occurrences):
  - Line 34: `// Note: In real tests, you'd either mock the wrapper or use a real model`

---

### `tests/test_embedding_cache.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_encryption.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_encryption_e2e.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 180: `// Simulate different team member (same group, different user_id)`

---

### `tests/test_enhanced_backup.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_enhanced_plugin_security.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 15: `// Create temporary directory for test files`

---

### `tests/test_enhanced_query_cache.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: test_enhanced_query_cache.cpp stubbed for build unblock - CacheEntry atom`

---

### `tests/test_entity_api_handler_batch.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_entity_api_raid_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 134: `// Create a mock HTTP request`
  - Line 181: `// Create a mock HTTP request`
  - Line 238: `// Create a mock HTTP request`

**🔒 HARDCODED** (1 occurrences):
  - Line 41: `// Create temporary directory for test database with unique suffix`

---

### `tests/test_env.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_envoy_xds.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_er_diagram_exporter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_error_codes.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 351: `static_cast<uint32_t>(AccelerationErrorCode::NotImplemented),`

---

### `tests/test_error_handling_audit.py` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_error_registry.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_ethical_guidelines_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_ethics_ai_types.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_ethics_aware_confidence_detector.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 176: `// Mock token confidence data`

---

### `tests/test_ethics_evaluator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_ethics_plugin_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 28: `// Simulate plugin registering philosophies`

---

### `tests/test_event_trigger.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_eviction_strategies.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_expected.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_explanation_generator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_external_scheduler_adapter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_faceted_search.cpp` (v0.0.29)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_feature_flags.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_federated_identity_manager.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 158: `// Use a mock HTTP function so no real network call is attempted`
  - Line 276: `// Register both realms with mock HTTP that serves the correct JWKS per URL`

---

### `tests/test_feedback_collector.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_feedback_collector_scaling.cpp` (v0.0.27)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_feedback_store.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 24: `// Create temporary database directory with unique identifier`

---

### `tests/test_fewshot_optimizer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_field_encryption_batch.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (2 occurrences):
  - Line 22: `std::cerr << "debug: out.size=" << out.size() << " items.size=" << items.size() << std::endl;`
  - Line 26: `// Debug: print encrypted blob JSON to inspect IV/tag/ciphertext`

---

### `tests/test_filtered_vector_search.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (7 occurrences):
  - Line 47: `// Debug: Verify vector index populated`
  - Line 48: `std::cout << "DEBUG SetUp: VectorIndex has " << vectorIdx->getVectorCount() << " vectors\n";`
  - Line 50: `// Debug: Verify secondary index populated`
  - Line 53: `std::cout << "DEBUG SetUp: SecondaryIndex scan for category=tech returned "`
  - Line 139: `std::cout << "DEBUG Test: VectorIndexManager::searchKnnPreFiltered returned status=" << vimStatus.ok`

---

### `tests/test_flash_attention_correctness.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_flash_lora.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_flatfile_importer.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_fulltext_phrase_fuzzy.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_fused_kernels.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_fused_lora_kernels.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_fuzz_core.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_fuzz_security.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_fuzzy_matcher.cpp` (v0.0.29)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_gap008_backup_automation.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (92.0/100)

**Issues Found:**

**🔴 STUB** (6 occurrences):
  - Line 5: `* Tests the new backup automation stub features:`
  - Line 8: `* - Snapshot management stubs`
  - Line 57: `// Backup Scheduling Tests (Stub)`
  - Line 75: `// Should return empty list (stub implementation)`
  - Line 80: `// Cloud Backup Tests (Stub)`

---

### `tests/test_gap008_observability.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 85: `// Stub implementation logs alerts and returns success (when disabled)`

---

### `tests/test_general_traversal.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_generate_aql_docs.py` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_generic_plugin_registry.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 15: `// Minimal stub interfaces and implementations for testing`

---

### `tests/test_geo_gpu_backend.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 1192: `// (Phase 2: GPU Backend Stub and Device Detection)`

---

### `tests/test_geo_index_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 19: `// Create temporary RocksDB instance`

---

### `tests/test_geo_processor_gdal.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_geo_topology_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 321: `// Mode must be a non-hardcoded reflection of the actual config`

---

### `tests/test_geo_topology_router.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_geo_validator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_geval.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 371: `// Should complete in reasonable time (stub implementation should be fast)`

---

### `tests/test_gguf_loader.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (86.0/100)

**Issues Found:**

**🎭 SIMULATION** (12 occurrences):
  - Line 125: `// ===== Mock GGUF File Tests =====`
  - Line 129: `// This would require creating a mock file`
  - Line 135: `// This would require creating a mock file`
  - Line 158: `// Create a mock Q4_K_M quantized tensor`
  - Line 166: `// Create mock Q4_K_M data (one block)`

---

### `tests/test_global_transaction_manager.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 19: `// Helpers: mock participant`
  - Line 23: `* @brief In-process mock region participant for testing.`
  - Line 34: `throw std::runtime_error("mock prepare failure");`

---

### `tests/test_gnn_embeddings.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_gorilla.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 18: `// Debug: print first few bytes to help diagnose encoding`

---

### `tests/test_gorilla_codec_edge_cases.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 378: `// Simulate realistic temperature sensor data`

---

### `tests/test_gorilla_error_recovery.cpp` (v0.0.31)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_gorilla_probe.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_gossip_config_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_governance_compliance_time_window.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_governance_opa_adapter.cpp` (v0.0.1)

**Maturity Level:** ⚫ DRAFT (0.0/100)

**Issues Found:**

**🔴 STUB** (25 occurrences):
  - Line 27: `// Stub IPolicyEvaluator for controlled testing (no actual HTTP calls)`
  - Line 30: `struct StubEvaluator : public PolicyEngine::IPolicyEvaluator {`
  - Line 34: `explicit StubEvaluator() : result(std::nullopt) {}`
  - Line 35: `explicit StubEvaluator(PolicyDecision d) : result(std::move(d)) {}`
  - Line 174: `StubEvaluator stub;  // nullopt → OPA unavailable`

**🎭 SIMULATION** (4 occurrences):
  - Line 341: `EXPECT_TRUE(sim.dry_run) << "Simulation must always be dry_run=true";`
  - Line 358: `// Should fall back to native evaluation silently (no counter in simulation)`
  - Line 363: `<< "Native fallback for simulation should apply offen permissive decision";`
  - Line 401: `<< "simulate must match evaluate when OPA is set";`

---

### `tests/test_governance_policy_hot_reload.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_governance_policy_simulation.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 3: `* @brief Unit tests for PolicyEngine::simulateDecision() – dry-run / simulation mode.`
  - Line 206: `// Header overrides are honoured in simulation just like in evaluate()`
  - Line 254: `// Run simulation – must not throw and must not write audit entry.`

---

### `tests/test_gpu_admin_api.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_gpu_alerts.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_gpu_audit_log.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_gpu_cluster_coordinator.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_gpu_cluster_topology.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_gpu_config.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_gpu_device_discovery.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_gpu_erasure_coding.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 156: `// Simulate having all chunks available`

---

### `tests/test_gpu_feature_flags.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_gpu_graph_cache.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_gpu_graph_traversal.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_gpu_kernel_validator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_gpu_launcher.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_gpu_load_balancer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 11: `// Build a fake device list.`

---

### `tests/test_gpu_lora_layers.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_gpu_memory_management.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 688: `// Chaos tests — simulate device loss mid-operation`
  - Line 692: `// Simulate: some allocations succeed, then "device is lost" → all`
  - Line 704: `// Phase 2: "device lost" — forcibly fill VRAM to simulate total OOM.`

---

### `tests/test_gpu_memory_pool.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 469: `// so data_move_errors must always be 0 in the CPU simulation path.`

---

### `tests/test_gpu_metrics.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_gpu_mig_manager.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 4: `* All tests run on CI without GPU hardware.  The CPU simulation path is`
  - Line 64: `// Convenience: create a partition on the fake A100 at device index 0.`

---

### `tests/test_gpu_module.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_gpu_olap_accelerator.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_gpu_p2p_transfer.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (5 occurrences):
  - Line 4: `* All tests run on CI without GPU hardware.  The CPU simulation path is`
  - Line 37: `d.name             = "Fake CUDA Device";`
  - Line 168: `// The CPU simulation path always returns PEER_ACCESS_NOT_SUPPORTED.`
  - Line 203: `// transfer — CPU simulation path (no CUDA/HIP hardware required)`
  - Line 264: `// On the CPU simulation path (no CUDA/HIP hardware) transfer() performs a`

---

### `tests/test_gpu_policy.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_gpu_profiler.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_gpu_query_accelerator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 306: `// annSearch — GPU-accelerated ANN vector similarity (cuVS/RAFT stub)`

---

### `tests/test_gpu_rocm_backend.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_gpu_safe_fail.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 202: `return false;  // Simulate GPU failure`
  - Line 453: `// Simulate a series of GPU operations with intermittent failures`

---

### `tests/test_gpu_safe_fail_module.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_gpu_stream_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_gpu_stubs_comprehensive.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (81.0/100)

**Issues Found:**

**🔴 STUB** (7 occurrences):
  - Line 3: `* @brief Comprehensive tests for GPU stubs covering memory management, backend selection, and error `
  - Line 37: `// Stub implementation for testing`
  - Line 115: `// GPU Backend stub for testing`
  - Line 315: `// Create various backend stubs`
  - Line 400: `auto* stub = dynamic_cast<GPUBackendStub*>(backend.get());`

**🎭 SIMULATION** (2 occurrences):
  - Line 15: `// Mock GPU types for testing`
  - Line 63: `// Simulate allocation`

---

### `tests/test_gpu_tensor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_gpu_time_slice_scheduler.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_gpu_training_loop.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_gpu_unified_memory.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 99: `int dummy = 42;`
  - Line 100: `EXPECT_FALSE(GPUUnifiedMemoryAllocator::GetInstance().free(&dummy));`

---

### `tests/test_gpu_vector_index.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_gpu_vram_allocation.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_gpu_vulkan_backend.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_gpu_wasm_kernel_sandbox.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 4: `* All tests run on CI without GPU hardware.  The sandbox's CPU simulation`
  - Line 281: `// Without THEMIS_ENABLE_WASM the CPU simulation path is active.`

---

### `tests/test_graceful_shutdown.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 102: `// Drain Logic Tests (simulate drain wait loop)`

---

### `tests/test_gradient_checkpointing.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_graph_advanced_features.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_graph_analytics.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 14: `// Create temporary database directory`

---

### `tests/test_graph_bfs_fix.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_graph_distributed.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_graph_edge_encryption.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (2 occurrences):
  - Line 19: `// Create temporary database`
  - Line 396: `auto edge = createEdge("e11", "alice", "ivan", 0.7, "temporary");`

---

### `tests/test_graph_index.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_graph_index_comprehensive.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: test_graph_index_comprehensive.cpp stubbed for build unblock.";`

---

### `tests/test_graph_parallel_traversal.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_graph_query_optimizer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_graph_type_filtering.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 31: `// Create a dummy SecondaryIndexManager for QueryEngine`

---

### `tests/test_graphql.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_graphql_cache_security.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_graphql_error_masking.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_graphql_introspection.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_graphql_limits.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_graphql_multimodel.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_graphql_p1_features.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 117: `// Simulate production configuration`

---

### `tests/test_graphql_performance.cpp` (v0.0.6)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_graphql_variables.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_group_dek.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (5 occurrences):
  - Line 23: `// Create mock PKI client`
  - Line 71: `// Simulate restart: recreate provider`
  - Line 107: `// Simulate multi-party access scenario`
  - Line 127: `// Simulate encryption with this DEK (simple XOR for test)`
  - Line 135: `// Simulate another user from same group decrypting`

**🔒 HARDCODED** (1 occurrences):
  - Line 13: `// Create temporary directory for test DB`

---

### `tests/test_grpc_channel_pool.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_grpc_transport.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_grpc_web_proxy_handler.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 295: `const std::string frame = makeGrpcWebFrame("fake-compressed", 0x01);`

---

### `tests/test_gssapi_authenticator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_ha_enhancements.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_health_checks.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_health_error_service.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_health_monitor_http.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (6 occurrences):
  - Line 40: `* @brief Mock HTTP health server for testing`
  - Line 131: `// Simulate delay if configured`
  - Line 179: `// Create mock server on localhost`
  - Line 183: `// Create mock coordinator and topology`
  - Line 266: `// Stop server to simulate connection refused`

---

### `tests/test_helpers_llm.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_hip_ann_kernels.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_hkdf_cache.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 25: `// Simulate key rotation by changing IKM -> derived output must differ`

---

### `tests/test_hnsw_incremental_reindex.cpp` (v0.0.19)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 181: `// Simulate out-of-band storage changes:`

**🔒 HARDCODED** (1 occurrences):
  - Line 6: `* using a real (temporary) RocksDB instance.  Seven test scenarios are covered:`

---

### `tests/test_hnsw_layer_optimizer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_hnsw_parameter_tuner.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_hnsw_production_defaults.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_hot_reload_manager.cpp` (v0.0.7)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_hot_spare.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 23: `// Mock Storage`
  - Line 216: `// Simulate shard failure`
  - Line 526: `// Simulate shard failure`
  - Line 558: `// Simulate multiple failures`

---

### `tests/test_hsm_bundle_signing.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (3 occurrences):
  - Line 8: `* Uses the in-process stub HSMProvider (no hardware required) so the tests`
  - Line 34: `// Build a stub HSMProvider (library_path empty → stub mode)`
  - Line 37: `cfg.library_path = ""; // empty → stub provider`

---

### `tests/test_hsm_key_provider_adapter.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (91.0/100)

**Issues Found:**

**🔴 STUB** (6 occurrences):
  - Line 378: `// Stub-mode tests: do NOT require SoftHSM2, validate wrap/unwrap round-trip`
  - Line 385: `cfg.library_path = ""; // Force stub mode (no real PKCS#11 library)`
  - Line 388: `return nullptr; // production-mode env may block stub`
  - Line 397: `GTEST_SKIP() << "Stub HSM could not initialize (production mode env?)";`
  - Line 419: `GTEST_SKIP() << "Stub HSM could not initialize (production mode env?)";`

---

### `tests/test_hsm_provider.cpp` (v0.0.33)

**Maturity Level:** 🟠 BETA (41.0/100)

**Issues Found:**

**🔴 STUB** (16 occurrences):
  - Line 100: `EXPECT_TRUE(hsm.isReady()); // stub ready`
  - Line 317: `// Security tests for FIND-002: HSM stub detection`
  - Line 318: `TEST_F(HSMProviderTest, StubProviderDetection) {`
  - Line 319: `// Test with stub provider (no library path)`
  - Line 321: `config.library_path = "";  // Force stub`

---

### `tests/test_hsm_provider_stub.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_hsm_security_checker.cpp` (v0.0.33)

**Maturity Level:** 🟠 BETA (51.0/100)

**Issues Found:**

**🔴 STUB** (14 occurrences):
  - Line 94: `const char* argv[] = {"themis_server", "--allow-stub-hsm", "--config", "config.yaml"};`
  - Line 101: `const char* argv[] = {"themis_server", "--config", "config.yaml", "--allow-stub-hsm"};`
  - Line 111: `config.library_path = "";  // Force stub`
  - Line 126: `config.library_path = "";  // Force stub`
  - Line 133: `// Should fail in production with stub provider`

---

### `tests/test_hsm_security_metrics.cpp` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (68.0/100)

**Issues Found:**

**🔴 STUB** (10 occurrences):
  - Line 18: `config.library_path = "";  // Force stub`
  - Line 37: `// Verify stub is detected (value = 1)`
  - Line 40: `// Verify provider type is stub`
  - Line 41: `EXPECT_NE(metrics.find("hsm_provider_type{provider=\"stub\"}"), std::string::npos);`
  - Line 57: `// With stub, all compliance should be 0 (non-compliant)`

**🎭 SIMULATION** (1 occurrences):
  - Line 186: `warning_count++;  // Simulate periodic warnings`

---

### `tests/test_hsm_startup_integration.cpp` (v0.0.33)

**Maturity Level:** ⚫ DRAFT (2.0/100)

**Issues Found:**

**🔴 STUB** (26 occurrences):
  - Line 4: `* Tests that the server properly displays warnings when stub HSM is active.`
  - Line 29: `* 1. Startup warning is displayed when stub HSM is active`
  - Line 30: `* 2. Warning can be suppressed with --allow-stub-hsm flag`
  - Line 31: `* 3. Production mode blocks stub HSM without flag`
  - Line 69: `* Test that stub HSM initialization succeeds in development mode`

---

### `tests/test_hsm_stub_gating.cpp` (v0.0.33)

**Maturity Level:** 🔴 ALPHA (36.0/100)

**Issues Found:**

**🔴 STUB** (17 occurrences):
  - Line 8: `* HSM Stub Gating Tests`
  - Line 10: `* Tests that the HSM stub provider properly enforces security gating:`
  - Line 54: `config.library_path = "/nonexistent/stub.so";`
  - Line 68: `config.library_path = "/nonexistent/stub.so";`
  - Line 87: `config.library_path = "/nonexistent/stub.so";`

---

### `tests/test_http2_protocol.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_http2_server_push.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_http3_datagram.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_http3_protocol.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_http_adaptive_index.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 402: `// Simulate frequent user lookups by email`
  - Line 436: `// Simulate range queries`

---

### `tests/test_http_aql.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (8 occurrences):
  - Line 102: `// DEBUG: Verify data was inserted correctly`
  - Line 202: `{"allow_full_scan", true}  // DEBUG: Try with full scan allowed`
  - Line 224: `{"allow_full_scan", true}  // DEBUG: Try with full scan allowed`
  - Line 246: `{"allow_full_scan", true},  // DEBUG: Try with full scan allowed`
  - Line 270: `{"allow_full_scan", true}  // DEBUG: Try with full scan allowed`

---

### `tests/test_http_aql_collect.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_http_aql_fulltext_or.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_http_aql_fulltext_score.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_http_aql_graph.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_http_aql_join.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_http_aql_let.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_http_audit.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_http_buffer_api.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_http_changefeed.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_http_changefeed_governance.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_http_changefeed_sse.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_http_changefeed_sse_extended.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_http_client_pool.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_http_config.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 84: `{"level", "debug"},`

---

### `tests/test_http_content.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 333: `// Simulate key rotation: manually increment key_version in stored blob to create "outdated" scenari`
  - Line 336: `// Since we can't easily rotate keys in test, we simulate by checking that re-encryption logic works`
  - Line 340: `// This test validates the flow exists; full integration test would require KeyProvider mock`

**📝 TODO** (1 occurrences):
  - Line 358: `// TODO: Add test with mocked KeyProvider that returns higher version to verify re-encryption`

---

### `tests/test_http_error_api.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_http_fusion_search.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_http_governance.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_http_hybrid_search.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_http_index_endpoints.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_http_invalid_utf8.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_http_pii_lazy_init.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_http_pii_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_http_pii_manager_new.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (3 occurrences):
  - Line 65: `if (v) std::cerr << "[TEST-DEBUG] THEMIS_TOKEN_ADMIN='" << v << "'\n"; else std::cerr << "[TEST-DEBU`
  - Line 72: `std::cerr << "[TEST-DEBUG] THEMIS_POLICIES_PATH='" << p.string() << "'\n";`
  - Line 174: `std::cerr << "[TEST-DEBUG] DELETE status=" << static_cast<int>(del.result_int()) << " body=" << del.`

---

### `tests/test_http_policies_export.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 133: `// Debug: output actual response`

---

### `tests/test_http_query_range.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_http_range_index.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_http_retention_api.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_http_rope.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_http_server_network.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_http_timeseries.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_http_vector.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_http_vector_largescale.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_huge_pages.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (93.0/100)

**Issues Found:**

**🎭 SIMULATION** (9 occurrences):
  - Line 418: `// Simulate a query workload: scanning through data`
  - Line 434: `// Simulate query: count elements matching a condition`
  - Line 771: `// Simulate RocksDB block cache size allocation`
  - Line 779: `// Simulate cache operations`
  - Line 811: `// Simulate transaction buffer pool`

---

### `tests/test_huggingface_plugin.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 28: `// Helper to create a temporary test database`

---

### `tests/test_hybrid_debug.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_hybrid_optimizations.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_hybrid_queries.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (2 occurrences):
  - Line 219: `// DEBUG: Test spatial filter directly on img1`
  - Line 456: `// DEBUG: Check if edges exist`

---

### `tests/test_hybrid_retention_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_hybrid_search.cpp` (v0.0.29)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 477: `// then assert that the re-ranker would invert the order via mock scores.`
  - Line 506: `// Attach a mock re-ranker that inverts the order:`

---

### `tests/test_hybrid_search_integration.cpp` (v0.0.29)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 322: `// Attach a mock re-ranker that gives a score of 1 to the first result and 9`

**🔒 HARDCODED** (1 occurrences):
  - Line 25: `// Test fixture: creates a temporary DB with three documents containing both`

---

### `tests/test_hypertable_comprehensive.cpp` (v0.0.31)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_idempotent_migration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 183: `// Retry Safety Tests (Mock)`
  - Line 187: `// Note: This test would need a mock mTLS client to actually run migration`
  - Line 192: `// Simulate a completed migration`

---

### `tests/test_image_analysis_interface.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (92.0/100)

**Issues Found:**

**🎭 SIMULATION** (10 occurrences):
  - Line 22: `// Mock Plugin for Testing`
  - Line 33: `.description = "Mock plugin for testing",`
  - Line 36: `.model_name = "mock-model",`
  - Line 86: `result.model_name = "mock-model";`
  - Line 101: `result.caption = "A mock image caption for testing purposes";`

---

### `tests/test_image_analysis_quality.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 111: `// Mock Plugin with Deterministic Behavior`

---

### `tests/test_import_wizard.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_importer_async_api.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (6 occurrences):
  - Line 341: `auto handle = startAsync("-- dummy dump\n", opts);`
  - Line 348: `auto handle = startAsync("-- dummy dump\n", opts);`
  - Line 362: `auto handle = startAsync("-- dummy dump\n", opts);`
  - Line 373: `auto handle = startAsync("-- dummy dump\n", opts);`
  - Line 381: `auto handle = startAsync("-- dummy dump\n", opts);`

---

### `tests/test_importer_conflict_resolver.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 155: `// Helper: simulate importing a list of entities and applying conflict logic`

---

### `tests/test_importer_plugin_api.cpp` (v0.0.1)

**Maturity Level:** ⚫ DRAFT (0.0/100)

**Issues Found:**

**🔴 STUB** (61 occurrences):
  - Line 33: `class StubImporter final : public ImporterPluginBase {`
  - Line 36: `const char* getName()    const override { return "stub_importer"; }`
  - Line 41: `return {"stub", "test"};`
  - Line 72: `handle->id = "stub-async-job";`
  - Line 123: `StubImporter stub;`

---

### `tests/test_in_place_schema_migrator.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_index_maintenance.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_index_manager_di.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_index_recommender.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_index_stats.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_index_workload_replay.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 163: `// Simulate 50 queries filtering on a highly selective column`
  - Line 192: `// Record a single access then simulate many queries without it`

---

### `tests/test_inference_engine_enhanced.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 62: `// LoRA management (stubs)`
  - Line 67: `// Distributed features (stubs)`

**🎭 SIMULATION** (5 occurrences):
  - Line 12: `// Mock LLM Plugin for testing`
  - Line 32: `// Simulate processing time`
  - Line 37: `response.text = "Mock response for: " + request.prompt;`
  - Line 53: `// Return a fixed-size mock embedding`
  - Line 277: `// Use a slow mock plugin to trigger timeout`

**🐛 DEBUG** (1 occurrences):
  - Line 452: `spdlog::debug("Request {} rejected: {}", i, e.what());`

---

### `tests/test_information_schema.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_ingestion_builder.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_ingestion_cdc.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (7 occurrences):
  - Line 5: `* All tests use the mock-injection path (setCdcEventFetchForTesting) so that`
  - Line 10: `*   - isAvailable() returns true when a mock is injected`
  - Line 25: `*   - No-stream path returns CONNECTOR_NOT_SUPPORTED without mock`
  - Line 149: `// isAvailable (mock path always returns true)`
  - Line 175: `// Basic ingestion via mock`

---

### `tests/test_ingestion_checkpoint.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (92.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 19: `// previously hard-coded in the apiHttpGet stub.  Inject via`

**🎭 SIMULATION** (7 occurrences):
  - Line 18: `// Shared mock HTTP GET: returns the same simulated response that was`
  - Line 277: `// Mock always returns 200`
  - Line 295: `// Mock body has "total":6`
  - Line 307: `// max_pages=1 prevents infinite loop: the mock endpoint always returns`
  - Line 332: `// With max_pages=1 and 3 docs per mock page, exactly 3 docs expected`

**🔒 HARDCODED** (1 occurrences):
  - Line 35: `// Test fixture – temporary directory for checkpoint files`

---

### `tests/test_ingestion_coordinator.cpp` (v0.0.1)

**Maturity Level:** 🟠 BETA (51.0/100)

**Issues Found:**

**🎭 SIMULATION** (25 occurrences):
  - Line 14: `*     aggregation, mock node injection`
  - Line 50: `/// A mock worker node that records which sources it received and returns`
  - Line 345: `auto mock = std::make_shared<MockWorkerNode>("ext-node-0");`
  - Line 346: `coordinator.registerNode(mock);`
  - Line 369: `auto mock = std::make_shared<MockWorkerNode>("n0");`

---

### `tests/test_ingestion_database.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🎭 SIMULATION** (8 occurrences):
  - Line 5: `* All tests use the mock-injection path (setRowFetchForTesting) so that no`
  - Line 111: `// isAvailable (mock path always returns true)`
  - Line 117: `// Inject an empty mock so the availability check uses the mock path`
  - Line 134: `// Mock path always returns 0 (count unknown)`
  - Line 139: `// Basic ingestion via mock`

---

### `tests/test_ingestion_errors.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_ingestion_features.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 365: `auto p = makeTempFile("test_ingestion.pdf", "%PDF-1.4 fake content");`
  - Line 430: `std::ofstream f(tmp_dir / "f.pdf"); f << "%PDF fake";`
  - Line 531: `std::string pdf_content = "%PDF-1.4 fake pdf data here";`
  - Line 663: `std::string pdf_content = "%PDF-1.4 fake pdf data";`

---

### `tests/test_ingestion_integration.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (3 occurrences):
  - Line 284: `// The stub connector returns 12 000 documents in batch mode`
  - Line 296: `// Stub returns 12 000 regardless of streaming flag`
  - Line 622: `// 2. HuggingFace source (stub returns 12 000 docs)`

**🎭 SIMULATION** (1 occurrences):
  - Line 404: `// Inject a mock that returns 3 items per page for exactly 2 pages,`

**🔒 HARDCODED** (1 occurrences):
  - Line 607: `// Prepare a temporary directory with 2 text files for the filesystem source.`

---

### `tests/test_ingestion_kafka.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 5: `* All tests use the mock-injection path (setMessageFetchForTesting) so that`
  - Line 96: `// Inject a mock that returns messages – isAvailable() returns true`
  - Line 382: `// and the mock-based ingest works correctly.  Verifying the actual offset`
  - Line 389: `// The mock-based ingest should work normally regardless of offset setting.`

---

### `tests/test_ingestion_manager_adapter.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 601: `// This feature is stubbed for now`

**🎭 SIMULATION** (3 occurrences):
  - Line 29: `// Mock Plugin for Testing`
  - Line 33: `* @brief Mock ingestion plugin for testing`
  - Line 63: `// Simulate successful processing`

**📝 TODO** (1 occurrences):
  - Line 120: `// TODO: Implement when enabling integration tests`

---

### `tests/test_ingestion_oauth.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 337: `EXPECT_EQ(stats.documents_processed, 12000u); // stub always returns 12000`
  - Line 363: `// (Full 401 path is exercised in integration tests against real stubbed HTTP.)`

**🎭 SIMULATION** (1 occurrences):
  - Line 70: `// Build a SourceConfig pointing at a fake API endpoint.`

---

### `tests/test_ingestion_object_storage.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 5: `* All tests use the mock-injection path (setObjectListForTesting /`
  - Line 252: `if (key == "bad.txt") return "";  // simulate fetch failure`
  - Line 358: `// Without mock AND without a compiled SDK, ingest should return`

---

### `tests/test_ingestion_pipeline.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_ingestion_plugin_api.cpp` (v0.0.1)

**Maturity Level:** ⚫ DRAFT (12.0/100)

**Issues Found:**

**🔴 STUB** (22 occurrences):
  - Line 26: `// Minimal stub connector used as a plugin`
  - Line 29: `class StubPluginConnector : public ISourceConnector {`
  - Line 31: `explicit StubPluginConnector(int doc_count = 3,`
  - Line 76: `reg.registerFactory("stub", []() {`
  - Line 77: `return std::make_unique<StubPluginConnector>(5);`

---

### `tests/test_ingestion_reconfig.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_ingestion_resilience.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 487: `// manually-inserted QuarantineEntry (simulate path).`
  - Line 839: `// Simulate two successful retries before ingestAll()`

---

### `tests/test_ingestion_schema_validation.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 561: `// Inject a mock HTTP GET that returns 3 documents`

**🔒 HARDCODED** (1 occurrences):
  - Line 30: `/// Write content to a temporary file and return its path.`

---

### `tests/test_ingestion_security.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_ingestion_web_crawler.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 5: `* All tests use the mock-injection path (setHttpFetchForTesting) so that`
  - Line 11: `*   - isAvailable() with mock`

---

### `tests/test_input_validation_comprehensive.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (3 occurrences):
  - Line 8: `* - JSON schema stub validation`
  - Line 268: `// JSON Schema Stub Validation Tests`
  - Line 293: `// No schema file -> stub mode, accept`

---

### `tests/test_input_validator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 17: `// Create a minimal AQL request schema stub`
  - Line 234: `// JSON Schema Stub Validation Tests`

**🔒 HARDCODED** (2 occurrences):
  - Line 13: `// Create temporary schema directory`
  - Line 48: `// Clean up temporary schema directory`

---

### `tests/test_inverted_index.cpp` (v0.0.12)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_io_metrics.cpp` (v0.0.32)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_jaeger_tracer_adapter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_json_path_functions.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_json_schema_validation_comprehensive.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_jwks_security.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_jwks_validator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_jwt_eddsa_comprehensive.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_jwt_es256_comprehensive.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_jwt_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_jwt_key_rotation_comprehensive.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_jwt_management_comprehensive.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_jwt_rotation_unit.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_jwt_token_revocation_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_jwt_validation_hardening.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_jwt_validator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_k_shortest_paths.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 16: `// Create temporary database directory`

---

### `tests/test_kafka_importer.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🎭 SIMULATION** (10 occurrences):
  - Line 5: `* All tests use the mock-injection path (setMessageFetchForTesting) so that`
  - Line 53: `*  - Empty vector returned by mock stops the consume loop`
  - Line 224: `/// Minimal mock-based Kafka import function (mirrors production importData).`
  - Line 254: `// In mock mode we allow empty brokers.`
  - Line 668: `// Mock always returns empty – should terminate without consuming anything.`

---

### `tests/test_kerberos_security.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 197: `// Create dummy token (in real scenario, would be valid GSSAPI token)`
  - Line 227: `std::vector<uint8_t> cert_data(256, 0xAB);  // Dummy certificate`

---

### `tests/test_kernel_fallback_dispatcher.cpp` (v0.0.1)

**Maturity Level:** ⚫ DRAFT (0.0/100)

**Issues Found:**

**🔴 STUB** (48 occurrences):
  - Line 21: `// Test helpers — minimal stub kernels`
  - Line 23: `// Each stub is a plain free function with the exact signature required by the`
  - Line 32: `// Return codes for the primary stub — tests can change this`
  - Line 37: `// --- ANN distance stub (primary): writes 0.5 into every output cell ---`
  - Line 38: `static int stubPrimaryL2(const float*, const float*, float* d,`

---

### `tests/test_kernel_fusion.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_kernel_invocation_interfaces.cpp` (v0.0.19)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (75.0/100)

**Issues Found:**

**🔴 STUB** (9 occurrences):
  - Line 155: `auto stub_l2 = +[](const float*, const float*, float*, int, int, int, void*) { return 0; };`
  - Line 156: `auto stub_cos = +[](const float*, const float*, float*, int, int, int, void*) { return 1; };`
  - Line 157: `auto stub_ip = +[](const float*, const float*, float*, int, int, int, void*) { return 2; };`
  - Line 160: `d.launchL2Distance   = stub_l2;`
  - Line 161: `d.launchCosine       = stub_cos;`

---

### `tests/test_key_schema.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_keyprovider_signing.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (7 occurrences):
  - Line 68: `std::cerr << "DEBUG: OPENSSL_VERSION_TEXT=" << OPENSSL_VERSION_TEXT << std::endl;`
  - Line 89: `std::cerr << "DEBUG: EVP_sha256 size=" << EVP_MD_size(EVP_sha256()) << std::endl;`
  - Line 90: `std::cerr << "DEBUG: EVP_PKEY type=" << pkey_type << " bits=" << pkey_bits << std::endl;`
  - Line 92: `std::cerr << "DEBUG: X509_set_pubkey returned " << setpub << std::endl;`
  - Line 98: `std::cerr << "DEBUG: X509_sign failed; trying X509_sign_ctx fallback" << std::endl;`

---

### `tests/test_knowledge_gap_detector.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_knowledge_graph_production.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_kv_cache_buffer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 231: `// Simulate some work`
  - Line 294: `// Simulate multiple sequences with varying lengths`

**🔒 HARDCODED** (1 occurrences):
  - Line 205: `// Acquire one more (should create temporary)`

---

### `tests/test_lazy_reencryption.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (5 occurrences):
  - Line 16: `* 4. Batch re-encryption simulation`
  - Line 153: `// Test 4: Batch Re-Encryption Simulation`
  - Line 173: `// Simulate batch re-encryption`
  - Line 249: `// Simulate re-encryption failure by removing the new key`
  - Line 351: `// Simulate user database with encrypted PII`

---

### `tests/test_ldap_authenticator.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 252: `// because it varies between platforms (LDAP stub vs real library).`

---

### `tests/test_learnable_rope.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 356: `// Create a temporary file path`

---

### `tests/test_learned_quantizer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_learning_metrics.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_learning_to_rank.cpp` (v0.0.29)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_legal_lora_pipeline.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_legal_modality_analyzer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_legal_training_schema.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (5 occurrences):
  - Line 415: `* TODO: Implement when database integration is available`
  - Line 423: `* TODO: Implement when database integration is available`
  - Line 431: `* TODO: Implement when database integration is available`
  - Line 439: `* TODO: Implement when database integration is available`
  - Line 447: `* TODO: Implement when database integration is available`

---

### `tests/test_lek_manager_lifecycle.cpp` (v0.0.31)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_lib_arrow_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 475: `// Stub tests when Arrow is not available`

**🎭 SIMULATION** (1 occurrences):
  - Line 353: `// Simulate ThemisDB query results`

---

### `tests/test_lib_boost_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_lib_hnsw_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 475: `// (end-to-end simulation of incrementalReindex workflow at the hnswlib level)`

---

### `tests/test_lib_json_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_lib_openssl_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_lib_rocksdb_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 334: `// Close the DB to simulate a scenario where subsequent transactions might fail`

---

### `tests/test_lib_spdlog_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 336: `// Simulate database operations`

**🐛 DEBUG** (5 occurrences):
  - Line 73: `logger->debug("debug message");`
  - Line 84: `EXPECT_NE(content.find("debug message"), std::string::npos);`
  - Line 99: `logger->debug("debug message");`
  - Line 108: `EXPECT_EQ(content.find("debug message"), std::string::npos);`
  - Line 338: `logger->debug("Transaction started: txn_id=12345");`

---

### `tests/test_lib_tbb_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_lib_yaml_advanced.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_lib_zstd_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 334: `// Simulate backup data (JSON-like structure)`

---

### `tests/test_license_client.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_license_validation.cpp` (v0.0.27)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_lirs_cache.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 116: `// Simulate sequential scan (one-time access of many entries)`

---

### `tests/test_llama_resource_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_llama_tokenizer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_llm_api_handler_feedback.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 32: `// Create temporary database directory`

---

### `tests/test_llm_api_handler_jwt.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_llm_aql_explain_stream_api.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_llm_aql_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 218: `// Mock response would have markdown, but we can test the method exists`

---

### `tests/test_llm_aql_streaming.cpp` (v0.0.8)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_llm_caching.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_llm_feedback.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_llm_grafana_metrics.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (84.0/100)

**Issues Found:**

**🔴 STUB** (5 occurrences):
  - Line 45: `// Tests should still run with stubbed wrapper`
  - Line 65: `// Load a stub model`
  - Line 74: `// Generate response (this will use stub since no real model is loaded)`
  - Line 573: `// real unified dashboard JSON (not the old stub message).`
  - Line 601: `// Response must be real Grafana dashboard JSON, not the old stub message.`

**🎭 SIMULATION** (3 occurrences):
  - Line 277: `// handleRequest is private, so invoke via the public URL helper + simulate:`
  - Line 328: `// Simulate a request by calling the internal dispatch via a helper that`
  - Line 444: `auto res = cli.Post("/admin/prompt/simulate", R"({"prompt":"hello"})", "application/json");`

**📝 TODO** (1 occurrences):
  - Line 339: `// Full integration would require the HTTP listener; that is a TODO item.`

---

### `tests/test_llm_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_llm_judge_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (5 occurrences):
  - Line 3: `* @brief Unit tests for LLMJudgeIntegration mock mode and configuration`
  - Line 16: `// Mock Mode Configuration Tests`
  - Line 36: `// Default config should not be in mock mode`
  - Line 59: `// Should not throw - uses mock responses`
  - Line 68: `EXPECT_DOUBLE_EQ(*result.score, 4.0);  // Mock returns 4.0`

---

### `tests/test_llm_lora_inline.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_llm_metrics.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (5 occurrences):
  - Line 223: `// Simulate 70% hit rate`
  - Line 252: `// Simulate model loading and unloading`
  - Line 289: `// Simulate state transitions: closed → open → half_open → closed`
  - Line 407: `// Simulate concurrent metric recording (simplified test)`
  - Line 441: `// Simulate some work`

---

### `tests/test_llm_multi_model_integration.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 36: `// Mock LLM plugin for multi-model tests`

---

### `tests/test_llm_plugin.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 61: `// Create a dummy model file for testing`
  - Line 69: `// Write some dummy data to simulate model size`
  - Line 75: `// Create a dummy LoRA file`
  - Line 83: `// Write dummy LoRA data`

**📝 TODO** (1 occurrences):
  - Line 24: `// TODO: Implement actual CUDA/HIP detection`

---

### `tests/test_llm_prefix_cache.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_llm_query_rewriter.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 82: `// rewrite() — with mock backend`

---

### `tests/test_llm_raid_data_push.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_llm_raid_pipeline.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_llm_reranker.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 140: `// rerank() — with mock backend`

---

### `tests/test_llm_resilience.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 96: `// Simulate a slow operation that does NOT check the token itself`

**🔒 HARDCODED** (1 occurrences):
  - Line 177: `throw std::runtime_error("Temporary failure");`

---

### `tests/test_llm_response_cache.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 218: `// For stub implementation, entries are only in-memory`

**🎭 SIMULATION** (1 occurrences):
  - Line 246: `// Simulate customer support bot`

---

### `tests/test_llm_response_cache_metrics.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 166: `// Simulate a high hit rate scenario`
  - Line 178: `// Simulate many requests (90% hit rate)`

---

### `tests/test_llm_security_audit.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 12: `// Minimal mock plugin (no-op inference for policy tests)`

---

### `tests/test_llm_single_model_integration.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 32: `// Mock LLM plugin for single-model tests`
  - Line 91: `* @brief Slow mock that blocks until explicitly unblocked — used for`

---

### `tests/test_llm_timeout_cancellation.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 15: `// Mock plugins`
  - Line 19: `* @brief Fast mock plugin - completes immediately.`
  - Line 57: `* @brief Slow streaming mock plugin.`

---

### `tests/test_llm_validation.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_llm_vision_encoder.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (2 occurrences):
  - Line 128: `// TODO: Fix ambiguous VisionEncoder constructor overload (C2668)`
  - Line 134: `// TODO: Fix ambiguous VisionEncoder constructor overload (C2668)`

---

### `tests/test_llm_vision_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_locality_aware_router.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 77: `// Simulate shard2 being overloaded`

---

### `tests/test_lock_manager.cpp` (v0.0.31)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_logger_production.cpp` (v0.0.31)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (11 occurrences):
  - Line 75: `Logger::debug("d");`
  - Line 200: `// DEBUG messages below WARN threshold – should NOT increment metrics counter`
  - Line 202: `Logger::debug("should be filtered");`
  - Line 205: `// Raise the level to include DEBUG`
  - Line 206: `Logger::setLevel(Logger::Level::DEBUG);`

---

### `tests/test_long_running_stress.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (5 occurrences):
  - Line 108: `// Simulate write`
  - Line 301: `// Simulate operation that might fail`
  - Line 356: `// Simulate I/O operation`
  - Line 410: `// Simulate work`
  - Line 465: `// Simulate work`

---

### `tests/test_lora_adapter.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_lora_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_lora_aql_functions.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_lora_data_selection.cpp` (v0.0.25)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 916: `// Simulate CLO constructor live-reload`
  - Line 936: `// Simulate CLO's runLoRARetraining adaptive step`

---

### `tests/test_lora_encryption_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_lora_failure_scenarios.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_lora_feedback.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 22: `// Create temporary database directory`

---

### `tests/test_lora_framework.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (3 occurrences):
  - Line 723: `// TODO: Implement queryAuditLog API`
  - Line 752: `// TODO: Re-enable after getFeedbackStats() is implemented`
  - Line 785: `// TODO: Re-enable after fixing API mismatches`

---

### `tests/test_lora_framework_comprehensive.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (88.0/100)

**Issues Found:**

**🎭 SIMULATION** (12 occurrences):
  - Line 48: `// Mock Classes for External Dependencies`
  - Line 52: `* @brief Mock storage backend for testing without file system`
  - Line 307: `// Simulate LRU cache behavior`
  - Line 369: `// Simulate hot-swap (unload old, load new)`
  - Line 538: `// Simulate round-robin distribution`

---

### `tests/test_lora_gpu.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_lora_layers.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: Lora layers tests stubbed for build unblock.";`

---

### `tests/test_lora_llama_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_lora_optimizer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: Lora optimizer tests stubbed for build unblock.";`

---

### `tests/test_lora_pki_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: Lora PKI integration tests stubbed for build unblock.";`

---

### `tests/test_lora_provenance.cpp` (v0.0.26)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_lora_rope.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_lora_router.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_lora_security.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 222: `// Write some dummy float data (5 floats = 20 bytes)`

**🔒 HARDCODED** (1 occurrences):
  - Line 19: `// Create temporary test file`

---

### `tests/test_lora_storage_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: Lora storage integration tests stubbed for build unblock.";`

---

### `tests/test_lora_storage_key_provider.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: Lora storage key provider tests stubbed for build unblock.";`

---

### `tests/test_lora_trainer_production.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_lora_training_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_lora_versioning.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: Lora versioning tests stubbed for build unblock.";`

---

### `tests/test_lru_cache.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_materialized_cte_ivm.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_mcp_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 24: `// Create a temporary test database`

---

### `tests/test_mcp_protocol.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_merge_engine.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_merge_operator_append.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_merge_operator_counter.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_merge_operator_max.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_merge_operator_set.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_merge_operators_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 143: `// Simulate event logging`
  - Line 165: `// Simulate tag aggregation`
  - Line 188: `// Simulate temperature monitoring`

---

### `tests/test_meta_prompt_generator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_meta_prompt_llm_provider.cpp` (v0.0.27)

**Maturity Level:** 🟢 PRODUCTION-READY (92.0/100)

**Issues Found:**

**🔴 STUB** (6 occurrences):
  - Line 12: `// Stub LLM provider`
  - Line 15: `class StubLLMProvider : public ILLMProvider {`
  - Line 20: `std::string name() const override { return "StubLLM"; }`
  - Line 55: `gen.setLLMProvider(std::make_shared<StubLLMProvider>());`
  - Line 61: `EXPECT_EQ(result.metadata.value("llm_provider", std::string()), "StubLLM");`

---

### `tests/test_metadata_shard.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_metadata_wal_recovery.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 107: `// Create mock metadata storage`
  - Line 140: `// Create mock metadata storage`

**🔒 HARDCODED** (1 occurrences):
  - Line 17: `// Create temporary test directory`

---

### `tests/test_metrics.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_metrics_api.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 94: `// Debug: save metrics body as seen by the test for diagnosis`

---

### `tests/test_metrics_collector.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 509: `// Simulate a complete query workflow`
  - Line 527: `// Simulate high volume of metrics`

---

### `tests/test_mfa_authenticator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_mime_detector_standalone.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: mime detector standalone tests stubbed for build unblock.";`

---

### `tests/test_minimal.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_mixed_precision_gpu.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 251: `// Simulate overflow - should reduce scale`
  - Line 258: `// Simulate many successful steps - should eventually increase scale`
  - Line 436: `// Simulate mixed precision training workflow`
  - Line 473: `// Simulate overflow scenario`

---

### `tests/test_ml_model_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: ML model manager tests stubbed for build unblock.";`

---

### `tests/test_mock_clip.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 8: `std::string sample = "fake-image-bytes-12345";`

---

### `tests/test_model_governance.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_model_loader_async.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_model_quantization_pipeline.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🎭 SIMULATION** (8 occurrences):
  - Line 428: `const std::vector<uint8_t> dummy(4, 0);`
  - Line 430: `{"layer.qweight", "I32", {1, 1}, dummy},`
  - Line 431: `{"layer.qzeros",  "I32", {1, 1}, dummy},`
  - Line 432: `{"layer.scales",  "F16", {1, 1}, dummy},`
  - Line 454: `const std::vector<uint8_t> dummy(4, 0);`

---

### `tests/test_model_router.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_module_dependency_resolver.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 496: `// Simulate a realistic module dependency graph.`

---

### `tests/test_module_hash_verifier.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 27: `/// Write @p content to a unique temporary file and return its path.`

---

### `tests/test_module_loader.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 880: `f.write("\x7f" "ELF FAKE", 8);`
  - Line 1253: `const std::string fakePath = "/fake/libthemis_test.so";`

**🔒 HARDCODED** (3 occurrences):
  - Line 980: `// Helper: returns the system temporary directory path (no trailing backslash)`
  - Line 996: `// Create a real temporary file, attach Zone.Identifier ADS with zone 3`
  - Line 1157: `// Create a temporary non-ELF file`

---

### `tests/test_module_sandbox.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_module_signature_verifier.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 66: `const std::string path = writeTempFile("dummy module data");`

**🔒 HARDCODED** (1 occurrences):
  - Line 30: `/// Write @p content to a unique temporary file and return its path.`

---

### `tests/test_mongo_importer.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_monitoring_ops.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 52: `std::string cert_path = "/tmp/test.crt";  // Would need to create mock cert`

---

### `tests/test_mqtt_protocol.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_mtls_authenticator.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_mtls_client.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_mtls_connection_pool.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (2 occurrences):
  - Line 63: `// In stub implementation, this will return nullopt`
  - Line 211: `// In stub implementation, this will return nullopt`

---

### `tests/test_multi_field_search.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_multi_gpu_backend.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_multi_gpu_lora.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 354: `// Simulate GPU failure by updating config to exclude a GPU`

---

### `tests/test_multi_gpu_lora_advanced.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 340: `// Check health (in simulation, all GPUs are healthy)`
  - Line 357: `// Simulate GPU failure by removing from available devices`
  - Line 534: `EXPECT_EQ(migrated, 0);  // All healthy in simulation`

---

### `tests/test_multi_gpu_management.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_multi_gpu_training.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 85: `// Create dummy input and target data for each GPU`
  - Line 96: `// Fill with dummy data`
  - Line 127: `// Create dummy data`

---

### `tests/test_multi_gpu_vector_index.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_multi_lora_fusion.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 530: `// Fusion should complete in reasonable time (< 1 second for mock implementation)`

---

### `tests/test_multi_modal_search.cpp` (v0.0.29)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 141: `// using mock-like data.  Since search() delegates to executeModal() which`
  - Line 142: `// requires live indices, we test fusion properties via a dummy scenario:`

---

### `tests/test_multi_perspective_generator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: multi perspective generator tests stubbed for build unblock.";`

---

### `tests/test_multi_range_scan.cpp` (v0.0.32)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_multi_region_active_active.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_multi_shard_transactions.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (6 occurrences):
  - Line 30: `* @brief Mock shard for testing`
  - Line 72: `// Simulate prepare work`
  - Line 182: `// Simulate one shard already in use (prepare will fail)`
  - Line 367: `// Simulate coordinator failure - remaining shards left in prepared state`
  - Line 398: `// Phase 1: Prepare with timeout simulation`

---

### `tests/test_multi_tenant_index.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_multi_vector_search.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 249: `// Simulate keyword/BM25 scores`

---

### `tests/test_mvcc.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 262: `// Simulate concurrent transactions`

---

### `tests/test_mvcc_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 6: `//  - MvccApiHandler: all HTTP endpoints (via a mock MVCCStore)`
  - Line 127: `// Simulate regex capture group for the key`
  - Line 249: `// Simulate empty key capture`
  - Line 255: `// matches[1] will be empty string – we simulate by clearing the body key capture`

---

### `tests/test_mvcc_history.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 717: `// Simulate the data that TransactionManager would collect during a conflict:`
  - Line 820: `// A TransactionWrapper whose put() always fails, used to simulate write errors.`

---

### `tests/test_mvcc_store.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 96: `// Simulate a received message with a much-newer timestamp.`

---

### `tests/test_mvcc_wal_integration.cpp` (v0.0.32)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_mysql_importer.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 275: `R"(CREATE\s+(?:TEMPORARY\s+)?TABLE\s+(?:(?:`([^`]+)`|(\w+))\.)?(?:`([^`]+)`|(\w+))\s*\()",`

---

### `tests/test_ner_detection_engine.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_network_protocol_chaos.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🎭 SIMULATION** (12 occurrences):
  - Line 7: `* - Network latency simulation`
  - Line 30: `// Simulate various malformed message scenarios`
  - Line 88: `* @brief Test network latency simulation`
  - Line 109: `// Simulate variable network latency`
  - Line 141: `* @brief Test packet loss simulation`

---

### `tests/test_network_timeout.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 178: `// Simulate operations (we can't test actual socket operations in unit tests)`
  - Line 288: `socket_t mock_socket = 12345;  // Mock socket descriptor`

---

### `tests/test_neural_sparse_retrieval.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_new_aql_functions.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (87.0/100)

**Issues Found:**

**🔴 STUB** (7 occurrences):
  - Line 163: `// Test that ETHICS_MAKE_DECISION returns stub response`
  - Line 172: `EXPECT_TRUE(result["decision_text"].get<std::string>().find("Stub") != std::string::npos);`
  - Line 200: `// Test that ETHICS_GET_ARGUMENTS returns empty array (stub)`
  - Line 203: `// Currently returns empty array as it's a stub`
  - Line 233: `// Test PM_FIND_SIMILAR returns stub result`

---

### `tests/test_nl_to_aql_translation.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_nli_verifier.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 358: `// Stub implementation should be very fast`

---

### `tests/test_nlp_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 38: `// Simulate setting NLP metadata`

---

### `tests/test_nlp_metadata_extractor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: NLP metadata extractor tests stubbed for build unblock.";`

---

### `tests/test_nlp_text_analyzer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_normalization.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_notification_webhook.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 186: `CapturedPost dummy;`
  - Line 187: `webhook_.setHttpSender(makeSender(dummy));`
  - Line 284: `CapturedPost dummy;`
  - Line 285: `webhook_.setHttpSender(makeSender(dummy));`

---

### `tests/test_oauth_device_flow.cpp` (v0.0.6)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 289: `// Simulate: 1 authorization_pending poll, then authorized`

---

### `tests/test_oauth_pkce_flow.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_observability_hardening.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 118: `adapter_->logStructured(ILogger::Level::DEBUG, "ts-test", {});`

---

### `tests/test_oci_registry_client.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_ocr_processor.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 411: `// OcrProcessor::generateEmbedding (stub)`

---

### `tests/test_office_processor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_oidc_provider.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 376: `// Inject a failing HTTP mock – createDeviceFlow() must trigger discover() and`
  - Line 437: `// Inject a mock so no real HTTP call is made; createDeviceFlow() must`

---

### `tests/test_olap.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_olap_extended.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 19: `// TODO(v1.3.0): Temporarily disable extended OLAP tests until ported to new API.`

---

### `tests/test_onnx_model_loader.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 28: `file << "Dummy ONNX model content for testing\n";`
  - Line 69: `// Create dummy model file`

---

### `tests/test_opa_adapter.cpp` (v0.0.1)

**Maturity Level:** ⚫ DRAFT (0.0/100)

**Issues Found:**

**🔴 STUB** (24 occurrences):
  - Line 47: `// Stub IPolicyEvaluator for controlled testing`
  - Line 50: `struct StubEvaluator : public PolicyEngine::IPolicyEvaluator {`
  - Line 54: `explicit StubEvaluator(std::optional<bool> r) : result(r) {}`
  - Line 66: `d.policy_id = "stub";`
  - Line 67: `d.reason    = *result ? "stub_allow" : "stub_deny";`

**🎭 SIMULATION** (2 occurrences):
  - Line 162: `StubEvaluator stub{std::nullopt};  // simulate OPA unavailable`
  - Line 174: `StubEvaluator stub{std::nullopt};  // simulate OPA unavailable`

---

### `tests/test_openapi_export.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_openssl_raii.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 149: `// Simulate error path with multiple resources`

---

### `tests/test_openssl_simple.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: OpenSSL simple tests stubbed for build unblock.";`

---

### `tests/test_optimizer_cost_model.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_optimizer_v1_5_x_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_optional_enhancements.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_oracle_importer.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_otel_propagation.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 247: `// Simulate a new thread / service: clear baggage and extract from headers`

---

### `tests/test_otel_tracer_adapter.cpp` (v0.0.15)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_paged_attention.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 85: `// Create dummy KV data (2 * num_kv_heads * head_dim * num_tokens)`

---

### `tests/test_paged_optimizer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: Paged optimizer tests stubbed for build unblock.";`

---

### `tests/test_parallel_scan.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_partition_detection.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 95: `// Simulate failures on some nodes to create partition`

---

### `tests/test_password_hashing_comprehensive.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_password_policy.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_path_constraints_direct.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_path_mapping_metadata.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_paxos_consensus.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_paxos_wal_recovery.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 132: `// Create mock Paxos state`
  - Line 171: `// Create mock Paxos state`

**🔒 HARDCODED** (1 occurrences):
  - Line 20: `// Create temporary test directory`

---

### `tests/test_pci_dss_rules.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_pdf_processor.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_per_query_cost_model_integration.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_performance_allocator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_performance_feature_flags.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 170: `// Simulate conditional feature execution`

---

### `tests/test_performance_helpers.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_personalized_ranker.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_phase1_flash_attention.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_phase1_kv_cache_reuse.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**🎭 SIMULATION** (9 occurrences):
  - Line 57: `// Create mock clock for deterministic testing`
  - Line 66: `cache_config_.clock = mock_clock_;  // Inject mock clock`
  - Line 191: `// Advance time past TTL (use mock clock)`
  - Line 292: `// RAG Workload Simulation`
  - Line 298: `// Simulate RAG workload with repeated system prompt`

---

### `tests/test_phase2_optimizations.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_phi3_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 31: `// Create a small mock model file for testing`
  - Line 33: `createMockModelFile(mock_model_path_, 2 * 1024 * 1024);  // 2MB mock`
  - Line 46: `// Fill rest with dummy data`

---

### `tests/test_philosophy_loader.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_phrase_search.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_pii_detector.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_pii_redaction_policy.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (2 occurrences):
  - Line 116: `{"debug.note", "SSN found: 123-45-6789 in record"}`
  - Line 120: `EXPECT_EQ(safe.at("debug.note").find("123-45-6789"), std::string::npos)`

---

### `tests/test_pii_soft_delete.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_pitr_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 57: `// Record some changefeed events to simulate database changes`

---

### `tests/test_pitr_manager_comprehensive.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 401: `// 3. Simulate deployment that corrupts data`

---

### `tests/test_pkcs11_wrapper.cpp` (v0.0.1)

**Maturity Level:** ⚫ DRAFT (0.0/100)

**Issues Found:**

**🔴 STUB** (49 occurrences):
  - Line 11: `//        hand-crafted stub CK_FUNCTION_LIST whose function pointers are`
  - Line 42: `// Helpers: minimal stub CK_FUNCTION_LIST for unit tests`
  - Line 47: `// A stub function list whose individual pointers can be replaced per-test.`
  - Line 48: `// All stubs return CKR_OK by default.`
  - Line 50: `CK_RV stub_C_Initialize(void* /*pInitArgs*/)   { return CKR_OK; }`

**🎭 SIMULATION** (1 occurrences):
  - Line 290: `// Manually simulate "already loaded" by setting the functions_ through`

---

### `tests/test_pki_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 8: `// Simple mock SigningService for tests`
  - Line 16: `r.algorithm = "MOCK+SHA256";`
  - Line 56: `EXPECT_EQ(res["algorithm"].get<std::string>(), "MOCK+SHA256");`

---

### `tests/test_pki_client.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (4 occurrences):
  - Line 108: `PKIConfig cfg; // no key/cert -> stub mode`
  - Line 115: `// In stub mode signature is base64(hash); verify should succeed`
  - Line 154: `auto hash = random_bytes(32); // wrong length for RSA-SHA512 -> stub`
  - Line 157: `// Should verify via stub comparison`

---

### `tests/test_pki_client_rest.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_pki_eidas.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 3: `// TODO: Implement unit/integration tests for PKI/eIDAS signing once SigningService exists.`

---

### `tests/test_pki_shard_certificate.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 7: `// Note: These tests are stubs for Phase 2`

---

### `tests/test_plugin_capability_negotiation.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_plugin_dependency_graph.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_plugin_dependency_resolver.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_plugin_health_monitor.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 26: `// Mock ISelfHealingPlugin implementation`
  - Line 49: `d.error_message = "mock error";`

---

### `tests/test_plugin_hot_plug.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_plugin_hot_reload_enhanced.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 391: `// Step 2: Simulate reload — create new instance (what reloadPlugin does after loading)`
  - Line 498: `// Simulate reload by creating new instance`

---

### `tests/test_plugin_lifecycle.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 11: `// Mock plugin system`
  - Line 67: `info.handle = reinterpret_cast<void*>(0x1000);  // Mock handle`

---

### `tests/test_plugin_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: plugin manager tests stubbed for build unblock.";`

---

### `tests/test_plugin_manager_comprehensive.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: comprehensive plugin manager tests stubbed for build unblock.";`

---

### `tests/test_plugin_marketplace_manifest.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_plugin_metrics.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_plugin_metrics_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 29: `// Simulate initialization time`
  - Line 88: `// Simulate multiple function calls with varying latencies`
  - Line 162: `// Simulate complete plugin lifecycle`

---

### `tests/test_plugin_security_audit.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 142: `"/fake/plugin_" + std::to_string(id) + ".so",`
  - Line 174: `"/fake/plugin.so", "hash", "setup event",`

**🔒 HARDCODED** (1 occurrences):
  - Line 25: `// Helper: create a temporary file and return its path`

---

### `tests/test_plugin_security_implementation.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 290: `// Add some dummy data`

**🔒 HARDCODED** (1 occurrences):
  - Line 17: `// Create temporary directory for test files`

---

### `tests/test_pointer_utils.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 366: `// Simulate a real-world scenario with multiple safety checks`

**🐛 DEBUG** (2 occurrences):
  - Line 21: `spdlog::set_level(spdlog::level::debug);`
  - Line 284: `// Verify debug message was logged`

---

### `tests/test_policy_abac_conditions.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_policy_change_auditing.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_policy_coordinator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 63: `// Mock classification decision with stricter settings`

---

### `tests/test_policy_engine_comprehensive.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_policy_engine_hotreload.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_policy_engine_load.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_policy_integration_e2e.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 36: `// Create PolicyEngine (mock for testing)`

---

### `tests/test_policy_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 300: `// Create a temporary YAML file`

---

### `tests/test_policy_manager_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_policy_review.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_policy_template.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 124: `EXPECT_EQ(rule.name, "Temporary Access: project/alpha for contractor");`

---

### `tests/test_policy_validation.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 777: `// Simulate concurrent evaluations`

---

### `tests/test_policy_validator.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_policy_versioning.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_policy_yaml.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_pool_allocator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_post_quantum_crypto.cpp` (v0.0.1)

**Maturity Level:** ⚫ DRAFT (18.0/100)

**Issues Found:**

**🎭 SIMULATION** (34 occurrences):
  - Line 250: `auto mock = make_mock_provider("pq_test_key");`
  - Line 251: `auto classical_key = mock->getKey("pq_test_key");`
  - Line 253: `PostQuantumKeyProvider pq(mock);`
  - Line 259: `auto mock = make_mock_provider();`
  - Line 260: `PostQuantumKeyProvider pq(mock);`

---

### `tests/test_postgres_copy_protocol.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_postgres_importer_advanced.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 514: `// Simulate a binary COPY block (PG binary COPY starts with PGCOPY...)`

---

### `tests/test_postgres_importer_chaos.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_postgres_importer_complex_ddl.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 253: `// Simulate a measurable amount of work using a thread sleep`
  - Line 370: `// Simulate the column section from sample_pg16.sql`

---

### `tests/test_postgres_importer_datatypes.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 10: `// Mock PostgreSQL importer`

---

### `tests/test_postgres_importer_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_postgres_importer_live.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_postgres_importer_robustness.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_postgres_importer_security_audit.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 588: `// Embed the secrets in COPY data to simulate an import where the content`

---

### `tests/test_postgres_importer_streaming.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 426: `json dummy;`
  - Line 427: `EXPECT_FALSE(cb("users", dummy));`

---

### `tests/test_postgres_prepared_statements.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_postgres_transactions.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_postgres_wire.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 154: `// Test that transaction commands are accepted (stubs for now)`

**🐛 DEBUG** (1 occurrences):
  - Line 265: `std::string sql = "DELETE FROM logs WHERE timestamp < '2024-01-01' AND level = 'DEBUG'";`

---

### `tests/test_predictive_detector.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_predictive_prefetcher.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 278: `// Simulate repeated access pattern: q1 always followed by q2 (3 rounds)`

---

### `tests/test_principal_validator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_process_graph.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 674: `// Set token to WAITING state to simulate waiting for event`
  - Line 824: `// Add completed_at timestamp to simulate completion`

**📝 TODO** (1 occurrences):
  - Line 525: `// Process Mining Features Tests (8 TODO markers resolved)`

---

### `tests/test_process_mining_extended.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_product_quantizer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_production_mode_enforcement.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_production_training.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: test_production_training.cpp has duplicate tests - stubbed for build unbl`

---

### `tests/test_production_validator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 123: `// Note: In simulation, metrics may pass. This tests the structure.`
  - Line 171: `// Quality should meet 80% threshold in simulation`

---

### `tests/test_prometheus_metrics_adapter.cpp` (v0.0.10)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_prometheus_metrics_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_prometheus_remote_write.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 129: `const uint8_t dummy = 0;`
  - Line 130: `auto result2 = PromWriteRequest::decode(&dummy, 0);`
  - Line 519: `auto req = makeRequest("dummy", "gzip");`

**🔒 HARDCODED** (1 occurrences):
  - Line 292: `// a temporary vector<uint8_t> then converting to string.`

---

### `tests/test_prompt_engineering_api_handler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_prompt_engineering_grpc_service.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_prompt_engineering_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_prompt_engineering_metrics.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_prompt_engineering_metrics_persistence.cpp` (v0.0.27)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_prompt_evaluator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_prompt_evaluator_embedding.cpp` (v0.0.27)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 13: `// Stub embedding providers`

---

### `tests/test_prompt_evaluator_ttest.cpp` (v0.0.27)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_prompt_injection_detector.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 114: `// Indirect injection: model embeds a fake SYSTEM: role in its response`

---

### `tests/test_prompt_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_prompt_manager_multimodal.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_prompt_manager_validation.cpp` (v0.0.27)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_prompt_optimizer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 15: `// Constants for mock evaluation`
  - Line 28: `// Mock evaluation function`
  - Line 33: `// Simple mock: longer prompts score better (up to a point)`

---

### `tests/test_prompt_performance_tracker.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_prompt_policy.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_prompt_version_control.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_prompt_version_control_diff.cpp` (v0.0.27)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_property_graph.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_qlora.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 358: `// Create dummy gradient`

---

### `tests/test_qlora_gpu_kernels.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_qlora_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 22: `// Create dummy model file for testing`
  - Line 36: `// Write some dummy data`
  - Line 37: `char dummy[1024] = {0};`
  - Line 38: `file.write(dummy, 1024);`

**🔒 HARDCODED** (1 occurrences):
  - Line 18: `// Create temporary test directory`

---

### `tests/test_qlora_training_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_qos_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_quality_control_pipeline.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_quantization.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_query_cache.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_query_engine.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_query_engine_di.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 23: `// Minimal mock implementations of the DI interfaces`

---

### `tests/test_query_engine_error_handling.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: query engine error handling tests stubbed for build unblock.";`

---

### `tests/test_query_engine_join.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: query engine join tests stubbed for build unblock.";`

---

### `tests/test_query_engine_range.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_query_expander.cpp` (v0.0.29)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_query_federation.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: query federation tests stubbed for build unblock.";`

---

### `tests/test_query_masking_policy.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_query_optimizer_vector_geo.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_query_or.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_query_plan_visualizer.cpp` (v0.0.11)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_query_resource_limits.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_query_result_type_annotation.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_query_stream_sse.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 270: `// Simulate the normalisation logic in WebSocketSession::processMessage`
  - Line 329: `// Simulate event_types extraction`

---

### `tests/test_quic_transport.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_quorum_writes.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_raft_configuration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_raft_consensus.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 90: `// Simulate partition by not responding to heartbeats`

---

### `tests/test_raft_consensus_adapter.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (6 occurrences):
  - Line 28: `// Test TODO #1: convertState implementation`
  - Line 52: `// Test TODO #2: Get actual log index from propose`
  - Line 68: `// Test TODO #3: waitForCommit implementation`
  - Line 97: `// Test TODO #4: readLog implementation`
  - Line 132: `// Test TODO #5: getCommitIndex implementation`

---

### `tests/test_raft_log.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: test_raft_log.cpp has own main() - stubbed for build unblock.";`

---

### `tests/test_raft_mvcc_bridge.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 24: `// Mock ConsensusModule`
  - Line 93: `// Raft mock + coordinator`

---

### `tests/test_raft_shard_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_raft_state.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_raft_wal_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: test_raft_wal_integration.cpp stubbed for build unblock.";`

---

### `tests/test_rag_agentic.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_rag_aql_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_rag_citation_highlighter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_rag_document_splitter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_rag_document_summarizer.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 266: `// LLM is nullptr → LLMIntegration::generate() returns an empty / stub`

---

### `tests/test_rag_ethics.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (3 occurrences):
  - Line 8: `* - Computed confidence (not hardcoded 0.85)`
  - Line 112: `// Confidence is in [0, 1] and not a hardcoded constant`
  - Line 119: `// Confidence must not be the old hardcoded value for every evaluation`

---

### `tests/test_rag_evaluation_report_exporter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_rag_hallucination_dashboard.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_rag_hybrid_retriever.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_rag_judge.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_rag_judge_phase1.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 287: `// Set mock inference function`

---

### `tests/test_rag_judge_phase2.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_rag_judge_phase3.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 190: `// With no engine configured the stub path returns immediately without`

---

### `tests/test_rag_judge_phase4.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_rag_knowledge_graph_retriever.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_rag_multimodal.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_rag_reranker.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 300: `// Stub path – no actual file required for the interface test`

**🎭 SIMULATION** (1 occurrences):
  - Line 361: `auto r = CrossEncoderFactory::createBalanced("dummy/path/model.onnx");`

---

### `tests/test_rag_streaming_retriever.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_raid5_backup.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: test_raid5_backup.cpp stubbed for build unblock.";`

---

### `tests/test_raid_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 148: `// Simulate shard failure`

---

### `tests/test_raid_lora_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 350: `// Simulate missing first chunk`
  - Line 381: `// Simulate with 2 stripes, each with 2 replicas`

---

### `tests/test_raid_redundancy.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (7 occurrences):
  - Line 22: `// Mock Implementations`
  - Line 316: `// Simulate shard failure (remove one shard's data)`
  - Line 398: `// Drop the first data chunk key to simulate a lost shard`
  - Line 725: `// Simulate single shard failure`
  - Line 752: `// Simulate TWO shard failures (RAID 6 should tolerate this)`

---

### `tests/test_raii_wrappers.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 330: `// Simulate an error`
  - Line 357: `// Simulate an error`

---

### `tests/test_range_index.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_rate_limiter.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 319: `// Simulate API requests`

---

### `tests/test_rate_limiting_comprehensive.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_rate_limiting_middleware.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_rbac_comprehensive.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_rcu_index.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_rebalance_migration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_recursive_ctes.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: test_recursive_ctes.cpp stubbed for build unblock.";`

---

### `tests/test_recursive_path_query.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_remote_registry_client.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 267: `// simulation: we use downloadPlugin with a file:// URL that points to a`
  - Line 277: `const std::string content = "fake-plugin-binary-content-for-unit-test";`

---

### `tests/test_replication_ha.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🎭 SIMULATION** (8 occurrences):
  - Line 223: `char dummy = 0xFF;`
  - Line 224: `fs.write(&dummy, 1);`
  - Line 347: `// Simulate a peer granting a vote for the current term`
  - Line 690: `// Simulate receiving a message with the sender's timestamp`
  - Line 1308: `// (We simulate this by resolving an existing conflict)`

---

### `tests/test_replication_topology_api_handler.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 77: `// Test fixture – stub coordinator with two replicas`

---

### `tests/test_request_limits.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_request_validation_middleware.cpp` (v0.0.20)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_residual_quantizer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_resource_limits_comprehensive.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_result_stream.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_retention_aql_functions.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_retention_async.cpp` (v0.0.31)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_retention_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_rocksdb_high_parallel_tuning.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_rocksdb_metrics.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: test_rocksdb_metrics.cpp stubbed for build unblock.";`

---

### `tests/test_rocksdb_stats_json.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_rocksdb_wrapper_comprehensive.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_rotary_embeddings.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 368: `// Create temporary database directory`

---

### `tests/test_rotary_embeddings_gpu.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_rpc_batch_operations.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_rpc_geo_query.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 22: `// Create temporary RocksDB instance`

---

### `tests/test_rpc_get_operation.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_rrf_fusion.cpp` (v0.0.29)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_runtime_license_gate.cpp` (v0.0.27)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_runtime_reoptimizer.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 132: `// Simulate consistent 2x overestimation (estimated 1000, actual 500)`
  - Line 146: `// Simulate consistent 3x underestimation`
  - Line 232: `// Now simulate a new execution where the caller provides no estimate (0).`

---

### `tests/test_s3_importer.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (5 occurrences):
  - Line 15: `//   - End-to-end: mock S3 content via temporary file (CSV/TSV/JSONL)`
  - Line 186: `// Minimal CSV import driver for end-to-end tests that mock S3 content by`
  - Line 436: `// Simulate a connection ID built with a config that has credentials set.`
  - Line 746: `// verify the pattern using the mock driver with an explicit emit call.`
  - Line 860: `// Simulate S3 download by reading the temp file content.`

**🔒 HARDCODED** (2 occurrences):
  - Line 15: `//   - End-to-end: mock S3 content via temporary file (CSV/TSV/JSONL)`
  - Line 813: `// Test Suite: End-to-end round-trip using temporary files`

---

### `tests/test_safe_access.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_safe_arithmetic.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 364: `// Simulate common pattern: int index with vector`
  - Line 376: `// Simulate: get count from API, allocate buffer`
  - Line 388: `// Simulate: error code -1 should not create huge buffer`
  - Line 396: `// Simulate: loop starting from offset`

---

### `tests/test_safe_cast.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_saga_concurrent_execution.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 392: `// Execute partial saga (simulate failure mid-way)`

---

### `tests/test_saga_logger.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_saml_authenticator.cpp` (v0.0.6)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_sampling_strategy.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_savepoints.cpp` (v0.0.32)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_scan_counters.cpp` (v0.0.32)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_scheduler_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_schema_api_lineage.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_schema_changefeed.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_schema_consistency_checker.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_schema_constraints.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_schema_constraints_persistence.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_schema_encryption.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: test_schema_encryption.cpp stubbed for build unblock.";`

---

### `tests/test_schema_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (3 occurrences):
  - Line 16: `// Helper to create temporary database path`
  - Line 28: `// Create temporary database`
  - Line 45: `// Clean up temporary files`

---

### `tests/test_schema_manager_fuzz.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_schema_migration_regression.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_schema_migration_script.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_schema_migration_tester.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_schema_validator.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 644: `// Simulate the fix: when a column_mapping renames "old_id" -> "id",`

---

### `tests/test_schema_version_dryrun.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_schema_version_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_score_normalization.cpp` (v0.0.29)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_search_analytics.cpp` (v0.0.29)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_secondary_index.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 85: `std::cout << "\n=== TEST DEBUG: DB OPEN ===" << std::endl;`

---

### `tests/test_secret_manager_comprehensive.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_secret_scanner.py` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_secure_transport_client.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_security_di.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 14: `* @brief Mock Key Provider for testing`

**🔒 HARDCODED** (1 occurrences):
  - Line 200: `// In a real test, we'd create a temporary policy file`

---

### `tests/test_security_signature_standalone.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: test_security_signature_standalone.cpp stubbed for build unblock.";`

---

### `tests/test_self_awareness_production.cpp` (v0.0.31)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 136: `snap.health.cpu_usage_percent = 0.99; // Simulate high CPU`

---

### `tests/test_self_improvement_auto_optimize.cpp` (v0.0.27)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_self_improvement_orchestrator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_semantic_cache.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_serverless_function_api_handler.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_service_mesh.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_session_manager.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_shader_integrity.cpp` (v0.0.10)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 158: `// Write a temporary manifest`

---

### `tests/test_shard_communication.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_shard_durability.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_shard_resilience.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (90.0/100)

**Issues Found:**

**🎭 SIMULATION** (10 occurrences):
  - Line 49: `// Simulate failures on node2 to create partition`
  - Line 84: `// Simulate partition: node1 and node2 can't reach node3`
  - Line 107: `// Simulate partition: only 2 nodes respond`
  - Line 139: `// Simulate partition`
  - Line 152: `// Simulate recovery - node2 comes back`

---

### `tests/test_shard_resource_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_shard_rpc_grpc.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 10: `// Mock request handler for testing`
  - Line 52: `// Tests will use in-process simulation by default (localhost)`
  - Line 60: `// In-Process Simulation Tests (for backward compatibility)`
  - Line 143: `// Should succeed after retries with in-process simulation`

---

### `tests/test_shard_rpc_mtls_config.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_sharding_chaos.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 29: `* @brief Mock distributed shard for chaos testing`
  - Line 182: `// Simulate cascading failures`

---

### `tests/test_sharding_core.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_sharding_e2e.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (94.0/100)

**Issues Found:**

**🎭 SIMULATION** (9 occurrences):
  - Line 11: `* - Data Migration Simulation`
  - Line 74: `// Simulate writing data to a shard`
  - Line 84: `// Simulate reading data from a shard`
  - Line 99: `// Simulate scatter-gather query`
  - Line 116: `// Simulate data migration between shards`

---

### `tests/test_sharding_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 11: `* sondern verwenden Mock-Objekte für die Kommunikation.`
  - Line 417: `// Simulate request tracking`
  - Line 433: `// Simulate complete workflow:`

---

### `tests/test_sharding_repair.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_shared_worker_pool.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (93.0/100)

**Issues Found:**

**🎭 SIMULATION** (9 occurrences):
  - Line 16: `// Minimal mock plugin`
  - Line 30: `info.model_id = "mock";`
  - Line 31: `info.name     = "mock";`
  - Line 44: `resp.text             = "mock:" + request.prompt;`
  - Line 45: `resp.model_id         = "mock";`

---

### `tests/test_siem_integration_comprehensive.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 201: `cfg.splunk_token = "dummy-token";`

---

### `tests/test_signature_simple.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: test_signature_simple.cpp has own main() - stubbed for build unblock.";`

---

### `tests/test_signature_verifier.cpp` (v0.0.33)

**Maturity Level:** 🟠 BETA (52.0/100)

**Issues Found:**

**🔴 STUB** (14 occurrences):
  - Line 238: `EXPECT_TRUE(true) << "Stub: Valid chain validation to be implemented";`
  - Line 247: `EXPECT_TRUE(true) << "Stub: Invalid chain detection to be implemented";`
  - Line 256: `EXPECT_TRUE(true) << "Stub: Expired cert detection to be implemented";`
  - Line 277: `EXPECT_TRUE(true) << "Stub: CRL check for valid cert to be implemented";`
  - Line 286: `EXPECT_TRUE(true) << "Stub: Revoked cert detection to be implemented";`

---

### `tests/test_signed_plugin_repository.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_signed_request.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_slo_monitor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_snapshot.cpp` (v0.0.32)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 5: `// using the real RocksDB Checkpoint API (no stubs).`

---

### `tests/test_snapshot_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 83: `// Simulate restart: destroy and recreate snapshot manager`
  - Line 293: `// Simulate restart`

---

### `tests/test_snapshot_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_snapshot_transfer_security.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_soc2_controls.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_socket_timeout.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 158: `// Simulate: if timeout is 0, don't arm; verify no spurious fire.`
  - Line 191: `// Simulate a slow read that would complete after the timeout`

---

### `tests/test_sparql_parser.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_sparse_geo_index.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 68: `// TODO: Implementierung muss in put() Sparse-Index-Logik hinzuf�gen`

---

### `tests/test_spatial_index_atomic.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 154: `// Simulate updating an entity with geometry`

**🔒 HARDCODED** (1 occurrences):
  - Line 17: `// Create temporary database for testing`

---

### `tests/test_speculative_decoder.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 56: `/// Minimal mock LLM plugin for engine integration tests.`

---

### `tests/test_sql_parser.cpp` (v0.0.2)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_sql_runner.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_sqlite_importer.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 568: `"CREATE TEMPORARY TABLE tmp_work (id INTEGER);";`

---

### `tests/test_ssi_predicate_locking.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_statistical_aggregations.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: test_statistical_aggregations.cpp stubbed for build unblock.";`

---

### `tests/test_statistics_auto_refresh.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_statistics_collector.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 14: `// Helper to create a unique temporary database path`

---

### `tests/test_stats_api.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_stemming.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_stopwords.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_storage_audit_logger.cpp` (v0.0.32)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_storage_engine_di.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 107: `// This is expected to fail in the stub implementation`

---

### `tests/test_storage_engine_prod.cpp` (v0.0.32)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_storage_fuzz.cpp` (v0.0.32)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 6: `//  - Crash-simulation: drop WAL mid-write, recover, verify no data beyond checkpoint`

---

### `tests/test_storage_latency_bench.cpp` (v0.0.32)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_strategic_cache.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 133: `// Testing strategy: Create an entry with a past timestamp to simulate expiration`

---

### `tests/test_stream_protocol_extended.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 203: `// TODO(v1.3.0): Encryption helpers removed from stream protocol API; skip until available again.`

---

### `tests/test_structured_log_correlation.cpp` (v0.0.10)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (2 occurrences):
  - Line 84: `void debug(const std::string& m) override    { log(Level::DEBUG, m); }`
  - Line 344: `void debug(const std::string&) override {}`

---

### `tests/test_stt_diarization.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_stt_wav_pcm.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_sync_milestones.py` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 38: `"""Create a fake src/<subdir>/ROADMAP.md file."""`

---

### `tests/test_task_audit.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 111: `// Simulate normal task executions`

**🔒 HARDCODED** (1 occurrences):
  - Line 13: `// Helper function to get portable temporary directory`

---

### `tests/test_task_scheduler.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 1744: `* @brief Simple mock Alertmanager that records sent/resolved alerts.`
  - Line 1996: `// No alerts were recorded (mock was detached)`

**🔒 HARDCODED** (1 occurrences):
  - Line 1022: `if (call++ == 0) throw std::runtime_error("temporary blip");`

---

### `tests/test_task_scheduler_api_handler.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_task_scheduler_siem_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 66: `// Initialize query engine (stubbed for testing)`

**🎭 SIMULATION** (3 occurrences):
  - Line 24: `* Mock PKI Client for testing (doesn't require actual PKI infrastructure)`
  - Line 70: `// Initialize encryption (mock implementation for testing)`
  - Line 73: `// Initialize PKI client (mock)`

**🔒 HARDCODED** (1 occurrences):
  - Line 315: `// Create a temporary audit logger with CEF format`

---

### `tests/test_task_scheduler_triggers.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_temporal_aggregation.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_temporal_aggregation_property.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_temporal_graph.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_tenant_buffer_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_tenant_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 239: `// Simulate usage`

---

### `tests/test_tenant_transaction_namespace.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_tensor_core_matmul.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_text_processor.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_themis_wire_protocol_server.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_themisdb_grpc_service.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 11: `// Both components are optional (may be null when stubs are absent).`

---

### `tests/test_thread_pool_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_thread_safety_stress.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 236: `// Simulate concurrent request ID generation`
  - Line 240: `// Simulate atomic counter increments as used in scheduler`

---

### `tests/test_time_travel_queries.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_timerange_query.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_timeseries_metrics.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_timeseries_retention.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_timestamp_authority.cpp` (v0.0.33)

**Maturity Level:** 🔴 ALPHA (28.0/100)

**Issues Found:**

**🔴 STUB** (18 occurrences):
  - Line 283: `// Stub implementation should not crash`
  - Line 286: `// May return true or false depending on stub implementation`
  - Line 290: `// Errors may or may not be present in stub`
  - Line 476: `// Stub Production-Mode Guard Tests`
  - Line 477: `// These tests only apply to the stub implementation (no OpenSSL).`

**🎭 SIMULATION** (1 occurrences):
  - Line 277: `token.token_der = {0x01, 0x02, 0x03};  // Dummy data`

---

### `tests/test_tls_hot_reload.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 148: `// Simulate by verifying the atomic flag can be set multiple times.`

---

### `tests/test_token_blacklist_comprehensive.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_token_quota_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_totp_replay_cache.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_totp_secret_encryption.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_tracing_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 208: `// Simulate work`

---

### `tests/test_tracing_middleware.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_tracing_production.cpp` (v0.0.31)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_training_convergence.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: test_training_convergence.cpp stubbed for build unblock.";`

---

### `tests/test_training_pipeline_e2e.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_training_service_registry.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 71: `// Create a mock ShardRouter (using default constructor for test)`
  - Line 88: `// Create a mock ShardTopology`

---

### `tests/test_transaction_bulk.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_transaction_isolation.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 449: `// Simulate some processing time`

---

### `tests/test_transaction_isolation_levels.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 118: `// Simulate commit: release all locks`

---

### `tests/test_transaction_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_transaction_manager_comprehensive.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (4 occurrences):
  - Line 125: `// Simulate acquire → release (as called by Transaction::commit)`
  - Line 153: `// Simulate the LockManager state: a non-SERIALIZABLE txn only uses regular`
  - Line 157: `// Simulate READ_COMMITTED txn (txn_id = 10): acquires regular read lock, no predicate lock`
  - Line 161: `// Simulate REPEATABLE_READ txn (txn_id = 11): same — no predicate lock`

---

### `tests/test_transaction_occ.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_transaction_retry.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_transaction_timeout.cpp` (v0.0.20)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 209: `// Simulate what the monitor does: commit should fail because the txn is expired.`

---

### `tests/test_transport_security_checker.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 244: `// Simulate server startup with TLS enabled`
  - Line 261: `// Simulate server startup with TLS disabled`
  - Line 275: `// Simulate server startup with TLS disabled`

---

### `tests/test_truetime_basic.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: test_truetime_basic.cpp has own main() - stubbed for build unblock.";`

---

### `tests/test_ts_auto_buffer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_ts_auto_buffer_advanced.cpp` (v0.0.31)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_ts_integration.cpp` (v0.0.31)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 200: `// Phase 1: Fill buffer, persist WAL (simulate crash before flush)`
  - Line 211: `// Simulate crash: buf goes out of scope without flush`

---

### `tests/test_ts_observability.cpp` (v0.0.31)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_ts_query_optimizer.cpp` (v0.0.31)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_tsa_api.cpp` (v0.0.1)

**Maturity Level:** ⚫ DRAFT (2.0/100)

**Issues Found:**

**🔴 STUB** (19 occurrences):
  - Line 6: `*  2. TSAClientWrapper with a stub backend — integration hooks, error`
  - Line 162: `// TSAClientWrapper with stub backend (no network)`
  - Line 174: `static TSAConfig stubConfig() {`
  - Line 176: `cfg.url            = "https://stub.example.invalid/tsr";`
  - Line 185: `auto client = createTSAClient(stubConfig());`

**🎭 SIMULATION** (8 occurrences):
  - Line 9: `*  4. ITSAClient polymorphism — mock implementation.`
  - Line 104: `// Mock ITSAClient for polymorphism / dependency-injection tests`
  - Line 139: `auto mock = std::make_unique<MockTSAClient>();`
  - Line 140: `mock->next_response = TSAResponse::fromToken(makeSuccessToken());`
  - Line 141: `mock->next_verify_result = true;`

---

### `tests/test_tsstore.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_tsstore_out_of_order.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_ttl_fulltext_index.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_two_phase_commit.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 289: `// --- Three mock participants (simulate three shards) ---`
  - Line 687: `* The ShardRPCClient in this environment uses the in-process simulation path`

---

### `tests/test_type_conversion_safety.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 185: `// Simulate user input that could be negative`
  - Line 210: `// Simulate a very large container size`

---

### `tests/test_udf_api_handler.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_udp_fast_path.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_unique_index.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_updates_production.cpp` (v0.0.31)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 1429: `// The stub subclass below overrides applyHotReload() to simulate the`

**🎭 SIMULATION** (4 occurrences):
  - Line 382: `// Simulate a crash in APPLYING state`
  - Line 1128: `// Create a dummy patch so the path check passes`
  - Line 1429: `// The stub subclass below overrides applyHotReload() to simulate the`
  - Line 1456: `* Simulate a successful file-apply and then run the registered`

---

### `tests/test_usb_admin_authenticator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (1 occurrences):
  - Line 16: `// Create temporary directory for testing`

---

### `tests/test_user_key_derivation.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_utilities_comprehensive.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_vault_key_provider.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_vault_key_provider_retry.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 28: `// Simulate transient vault server error by throwing KeyOperationException with transient=true`

---

### `tests/test_vault_signing_provider.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 12: `// Ensure env THEIMIS_VAULT_ADDR not set for deterministic mock path`
  - Line 20: `EXPECT_EQ(res.algorithm, "MOCK+SHA256");`
  - Line 29: `// with a mock httpPost implementation. We simulate transient failure by`

**📝 TODO** (1 occurrences):
  - Line 32: `// TODO: For now just ensure the method exists and can be invoked via the adapter.`

---

### `tests/test_vault_signing_provider_errors.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_vcc_pki_client.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_vector_advanced_features.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_vector_auto_buffer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_vector_compression_lossless.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 11: `// Mock implementations for testing (will be in utils/ in actual implementation)`
  - Line 367: `// Simulate histogram data (small variations)`

---

### `tests/test_vector_encryption_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 195: `// Simulate server restart`
  - Line 341: `// Don't set FieldEncryption (simulate missing key scenario)`

---

### `tests/test_vector_encryption_phase1.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_vector_filtered_standalone.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_vector_index.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_vector_index_comprehensive.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: test_vector_index_comprehensive.cpp stubbed for build unblock.";`

---

### `tests/test_vector_metadata_encryption_edge_cases.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: test_vector_metadata_encryption_edge_cases.cpp stubbed for build unblock.`

---

### `tests/test_vector_stats_standalone.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_vectorized_execution.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_video_processor_extended.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 22: `// TODO(v1.3.0): Content plugin API drift (PluginConfig/ExtractionOptions fields). Disable extended `

---

### `tests/test_voice_assistant.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_voice_coverage.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_voice_production.cpp` (v0.0.28)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (3 occurrences):
  - Line 2256: `cache.registerLoader("stub",`
  - Line 2267: `auto result = cache.get("stub-model", "/stub/path", "stub");`
  - Line 2273: `cache.evict("stub-model");`

---

### `tests/test_vram_allocator_null_checks.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 28: `spdlog::set_level(spdlog::level::debug);`

---

### `tests/test_vram_secure_clear.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: test_vram_secure_clear.cpp stubbed for build unblock.";`

---

### `tests/test_vulkan_backend.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_vulkan_compute_equivalents.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_vulkan_health.cpp` (v0.0.10)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_vulkan_lora.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: test_vulkan_lora.cpp stubbed for build unblock.";`

---

### `tests/test_vulkan_metrics.cpp` (v0.0.10)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_w3c_trace_context_propagator.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_wal_archiving.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 3: `// TODO: Implement incremental backup + WAL archiving tests.`

---

### `tests/test_wal_backup_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 4: `GTEST_SKIP() << "Disabled: test_wal_backup_manager.cpp stubbed for build unblock.";`

---

### `tests/test_wal_chaos.cpp` (v0.0.32)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_wal_grpc_apply.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (80.0/100)

**Issues Found:**

**🔴 STUB** (8 occurrences):
  - Line 42: `ASSERT_NE(svc, nullptr) << "gRPC stubs not generated; build with THEMIS_ENABLE_GRPC and protoc";`
  - Line 56: `stub_ = themis::sharding::ShardService::NewStub(channel_);`
  - Line 58: `GTEST_SKIP() << "Shard gRPC stubs not available";`
  - Line 65: `stub_.reset();`
  - Line 73: `std::unique_ptr<themis::sharding::ShardService::Stub> stub_;`

---

### `tests/test_wal_manager.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_wal_manifest_corruption.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_wal_replication.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 196: `// WAL Shipper Tests (Mock)`

---

### `tests/test_wal_replication_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (91.0/100)

**Issues Found:**

**🎭 SIMULATION** (14 occurrences):
  - Line 55: `// Set apply handlers for replicas (mock storage)`
  - Line 57: `// Mock apply: just return success`
  - Line 62: `// Mock apply: just return success`
  - Line 128: `// Simulate replicas acknowledging write`
  - Line 135: `// depending on mock setup`

---

### `tests/test_wal_storage.cpp` (v0.0.32)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_wasm_plugin_sandbox.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (6 occurrences):
  - Line 439: `// WasmPluginSandbox – runtime injection via a mock`
  - Line 443: `* @brief Minimal mock WasmRuntime for unit tests.`
  - Line 461: `if (call_result) out = { 0x42 }; // dummy return value`
  - Line 467: `std::string engineName() const override { return "mock-1.0"; }`
  - Line 475: `EXPECT_EQ(sb.engineName(), "mock-1.0");`

---

### `tests/test_webauthn_authenticator.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 741: `cred["response"]["clientDataJSON"]    = "dummy";`
  - Line 742: `cred["response"]["attestationObject"] = "dummy";`

---

### `tests/test_websocket_cdc.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_window_functions.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_wire_protocol_connection_pool.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_wire_protocol_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔴 STUB** (1 occurrences):
  - Line 339: `GTEST_SKIP() << "This stub has been replaced with real tests";`

---

### `tests/test_wire_protocol_performance.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_wire_protocol_v2.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 317: `// Simulate local side sending END_STREAM`
  - Line 321: `// Simulate remote side sending END_STREAM -> fully closed`

---

### `tests/test_wire_protocol_websocket.cpp` (v0.0.4)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 366: `// Simulate a response frame (PONG = 0xFD) with a small JSON payload.`
  - Line 519: `// Append a dummy 4-byte CRC trailer so the frame is well-formed.`

---

### `tests/test_wisckey_gc.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_workload_cache_strategy.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_workload_driven_cache.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 233: `// RAG Workload Simulation`
  - Line 259: `// Streaming Workload Simulation`

---

### `tests/test_write_amplification_config.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_ws_handler.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_yaml_config_integration.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_zero_trust_auth_verifier.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_zero_trust_policy_enforcer.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_zipkin_tracer_adapter.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/test_zstd_compression_security.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🎭 SIMULATION** (8 occurrences):
  - Line 73: `std::vector<uint8_t> dummy(1);`
  - Line 74: `auto result = zstd_compress_safe(dummy.data(), too_large, 3);`
  - Line 87: `std::vector<uint8_t> dummy(1);`
  - Line 88: `auto result = zstd_compress_safe(dummy.data(), max_size, 3);`
  - Line 171: `std::vector<uint8_t> dummy(1);`

---

### `tests/update_checker_test.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/utils/mock_clock.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (92.0/100)

**Issues Found:**

**🎭 SIMULATION** (6 occurrences):
  - Line 9: `* @brief Mock clock for deterministic testing`
  - Line 13: `* - Sleep operations immediately advance mock time`
  - Line 16: `* Thread Safety: This mock is NOT thread-safe. It is designed for`
  - Line 33: `// Mock sleep: instantly advance time without blocking`
  - Line 38: `// Mock sleep: instantly set time without blocking`

---

### `tests/utils/raid_simulator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/utils/raid_simulator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 80: `// Failure Simulation`

---

### `tests/utils/shard_failure_injector.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tests/utils/shard_failure_injector.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🔒 HARDCODED** (2 occurrences):
  - Line 16: `* - Transient failures (temporary)`
  - Line 27: `TRANSIENT,  // Temporary failure (auto-recovers)`

---

### `tools/Themis.AdminTools.Shared/ApiClient/Endpoints/AuditLogEndpoint.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AdminTools.Shared/ApiClient/Endpoints/ClassificationEndpoint.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AdminTools.Shared/ApiClient/Endpoints/KeysEndpoint.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AdminTools.Shared/ApiClient/Endpoints/PiiEndpoint.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AdminTools.Shared/ApiClient/Endpoints/ReportsEndpoint.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AdminTools.Shared/ApiClient/Endpoints/RetentionEndpoint.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AdminTools.Shared/ApiClient/Endpoints/SagaEndpoint.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AdminTools.Shared/ApiClient/MockThemisApiClient.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 6: `/// Mock-Implementation des API-Clients für Tests ohne laufenden Server.`

---

### `tools/Themis.AdminTools.Shared/ApiClient/ThemisApiClient.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AdminTools.Shared/Models/AuditLogModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AdminTools.Shared/Models/ClassificationModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AdminTools.Shared/Models/Common.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AdminTools.Shared/Models/KeysModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AdminTools.Shared/Models/PiiModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AdminTools.Shared/Models/ReportsModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AdminTools.Shared/Models/RetentionModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AdminTools.Shared/Models/SAGABatchDetail.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AdminTools.Shared/Models/SAGABatchInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AdminTools.Shared/Models/SAGABatchListResponse.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AdminTools.Shared/Models/SAGAVerificationResult.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AdminTools.Shared/obj/Debug/net8.0-windows/.NETCoreApp,Version=v8.0.AssemblyAttributes.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AdminTools.Shared/obj/Debug/net8.0-windows/Themis.AdminTools.Shared.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AdminTools.Shared/obj/Release/net8.0-windows/.NETCoreApp,Version=v8.0.AssemblyAttributes.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AdminTools.Shared/obj/Release/net8.0-windows/Themis.AdminTools.Shared.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AdminTools.Shared/obj/Release/net8.0-windows/Themis.AdminTools.Shared.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AqlQueryBuilder/App.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AqlQueryBuilder/AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AqlQueryBuilder/Converters/ValueConverters.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (85.0/100)

**Issues Found:**

**🔴 STUB** (4 occurrences):
  - Line 32: `throw new NotImplementedException();`
  - Line 65: `throw new NotImplementedException();`
  - Line 92: `throw new NotImplementedException();`
  - Line 119: `throw new NotImplementedException();`

---

### `tools/Themis.AqlQueryBuilder/Infrastructure/Result.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AqlQueryBuilder/Infrastructure/ServiceContainer.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AqlQueryBuilder/MainWindow.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AqlQueryBuilder/Models/AqlQueryModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AqlQueryBuilder/Models/ConnectionModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AqlQueryBuilder/Models/GeoModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AqlQueryBuilder/Models/GraphModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AqlQueryBuilder/Models/SchemaModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AqlQueryBuilder/Models/VectorModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AqlQueryBuilder/Services/AqlQueryService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AqlQueryBuilder/Services/HelpService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AqlQueryBuilder/Services/IHelpService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AqlQueryBuilder/Services/IServices.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AqlQueryBuilder/Services/QueryHistoryService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AqlQueryBuilder/Services/SchemaService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AqlQueryBuilder/ViewModels/MainViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AqlQueryBuilder/Views/EnhancedMainWindow.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AuditLogViewer/App.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AuditLogViewer/ViewModels/MainWindowViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AuditLogViewer/Views/AboutWindow.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AuditLogViewer/Views/MainWindow.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AuditLogViewer/obj/Debug/net8.0-windows/.NETCoreApp,Version=v8.0.AssemblyAttributes.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AuditLogViewer/obj/Debug/net8.0-windows/App.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AuditLogViewer/obj/Debug/net8.0-windows/Themis.AuditLogViewer.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 14: `[assembly: System.Reflection.AssemblyConfigurationAttribute("Debug")]`

---

### `tools/Themis.AuditLogViewer/obj/Debug/net8.0-windows/Themis.AuditLogViewer.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AuditLogViewer/obj/Debug/net8.0-windows/Views/AboutWindow.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AuditLogViewer/obj/Debug/net8.0-windows/Views/MainWindow.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AuditLogViewer/obj/Release/net8.0-windows/.NETCoreApp,Version=v8.0.AssemblyAttributes.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AuditLogViewer/obj/Release/net8.0-windows/App.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AuditLogViewer/obj/Release/net8.0-windows/Themis.AuditLogViewer.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AuditLogViewer/obj/Release/net8.0-windows/Themis.AuditLogViewer.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AuditLogViewer/obj/Release/net8.0-windows/Views/MainWindow.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AuditLogViewer/obj/Release/net8.0-windows/win-x64/.NETCoreApp,Version=v8.0.AssemblyAttributes.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AuditLogViewer/obj/Release/net8.0-windows/win-x64/App.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AuditLogViewer/obj/Release/net8.0-windows/win-x64/Themis.AuditLogViewer.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AuditLogViewer/obj/Release/net8.0-windows/win-x64/Themis.AuditLogViewer.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AuditLogViewer/obj/Release/net8.0-windows/win-x64/Views/AboutWindow.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.AuditLogViewer/obj/Release/net8.0-windows/win-x64/Views/MainWindow.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ClassificationDashboard/App.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ClassificationDashboard/Models/DataClassification.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ClassificationDashboard/ViewModels/MainViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ClassificationDashboard/Views/AboutWindow.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ClassificationDashboard/Views/MainWindow.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ClassificationDashboard/obj/Debug/net8.0-windows/.NETCoreApp,Version=v8.0.AssemblyAttributes.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ClassificationDashboard/obj/Debug/net8.0-windows/App.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ClassificationDashboard/obj/Debug/net8.0-windows/Themis.ClassificationDashboard.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 14: `[assembly: System.Reflection.AssemblyConfigurationAttribute("Debug")]`

---

### `tools/Themis.ClassificationDashboard/obj/Debug/net8.0-windows/Themis.ClassificationDashboard.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ClassificationDashboard/obj/Debug/net8.0-windows/Views/AboutWindow.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ClassificationDashboard/obj/Debug/net8.0-windows/Views/MainWindow.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ClassificationDashboard/obj/Release/net8.0-windows/win-x64/.NETCoreApp,Version=v8.0.AssemblyAttributes.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ClassificationDashboard/obj/Release/net8.0-windows/win-x64/App.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ClassificationDashboard/obj/Release/net8.0-windows/win-x64/Themis.ClassificationDashboard.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ClassificationDashboard/obj/Release/net8.0-windows/win-x64/Themis.ClassificationDashboard.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ClassificationDashboard/obj/Release/net8.0-windows/win-x64/Views/AboutWindow.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ClassificationDashboard/obj/Release/net8.0-windows/win-x64/Views/MainWindow.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ComplianceReports/App.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ComplianceReports/Models/ComplianceReport.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ComplianceReports/ViewModels/MainViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ComplianceReports/Views/AboutWindow.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ComplianceReports/Views/MainWindow.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ComplianceReports/obj/Debug/net8.0-windows/.NETCoreApp,Version=v8.0.AssemblyAttributes.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ComplianceReports/obj/Debug/net8.0-windows/App.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ComplianceReports/obj/Debug/net8.0-windows/Themis.ComplianceReports.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 14: `[assembly: System.Reflection.AssemblyConfigurationAttribute("Debug")]`

---

### `tools/Themis.ComplianceReports/obj/Debug/net8.0-windows/Themis.ComplianceReports.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ComplianceReports/obj/Debug/net8.0-windows/Views/AboutWindow.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ComplianceReports/obj/Debug/net8.0-windows/Views/MainWindow.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ComplianceReports/obj/Release/net8.0-windows/win-x64/.NETCoreApp,Version=v8.0.AssemblyAttributes.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ComplianceReports/obj/Release/net8.0-windows/win-x64/App.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ComplianceReports/obj/Release/net8.0-windows/win-x64/Themis.ComplianceReports.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ComplianceReports/obj/Release/net8.0-windows/win-x64/Themis.ComplianceReports.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ComplianceReports/obj/Release/net8.0-windows/win-x64/Views/AboutWindow.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ComplianceReports/obj/Release/net8.0-windows/win-x64/Views/MainWindow.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.GISViewer.ControlPanel/App.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.GISViewer.ControlPanel/MainWindow.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.GISViewer.ControlPanel/Services/PluginService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 29: `// TODO: Implement proper JSON deserialization`

---

### `tools/Themis.GISViewer.ControlPanel/Services/ThemisDBService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.GISViewer.ControlPanel/Services/UnrealEngineConnector.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.GISViewer.ControlPanel/ViewModels/MainViewModel.cs` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (74.0/100)

**Issues Found:**

**🎭 SIMULATION** (8 occurrences):
  - Line 36: `// Wind Simulation Properties`
  - Line 71: `// Disaster Simulation Properties`
  - Line 155: `StatusMessage = $"Starte Wind-Simulation (Geschwindigkeit: {WindSpeed} m/s, Richtung: {WindDirection`
  - Line 165: `StatusMessage = "Wind-Simulation läuft";`
  - Line 177: `StatusMessage = $"Starte Wasser-Simulation (Niederschlag: {RainfallIntensity} mm/h)";`

**📝 TODO** (1 occurrences):
  - Line 213: `// TODO: Show file dialog to select plugin DLL`

---

### `tools/Themis.ImpactAnalysisViewer/App.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ImpactAnalysisViewer/Controls/AqlQueryBuilderControl.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ImpactAnalysisViewer/Controls/ImpactGraphControl.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ImpactAnalysisViewer/Controls/LlmQueryAssistant.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ImpactAnalysisViewer/Controls/Model3DImporter.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ImpactAnalysisViewer/Models/DocumentChange.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ImpactAnalysisViewer/Models/ImpactAnalysisResult.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ImpactAnalysisViewer/Models/LayerMetadata.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ImpactAnalysisViewer/Models/NodeImpact.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ImpactAnalysisViewer/Services/ImpactAnalysisService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ImpactAnalysisViewer/ViewModels/MainViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.ImpactAnalysisViewer/Views/MainWindow.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.IngestionTool/App.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.IngestionTool/Converters/ValueConverters.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (85.0/100)

**Issues Found:**

**🔴 STUB** (3 occurrences):
  - Line 32: `throw new NotImplementedException();`
  - Line 46: `throw new NotImplementedException();`
  - Line 82: `throw new NotImplementedException();`

---

### `tools/Themis.IngestionTool/Models/AnalysisModels.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.IngestionTool/Models/AppSettings.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.IngestionTool/RealIngestionRunner.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.IngestionTool/Services/AnalysisServiceImplementations.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (95.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 110: `// Topic-Modelling-Simulation`

**📝 TODO** (1 occurrences):
  - Line 31: `// TODO: Echte Prüfung gegen llama.cpp endpoint`

---

### `tools/Themis.IngestionTool/Services/AnalysisServiceInterfaces.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.IngestionTool/Services/EmbeddingService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.IngestionTool/Services/GraphQueryService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.IngestionTool/Services/GrpcThemisService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.IngestionTool/Services/LRUCacheService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.IngestionTool/Services/LlamaHttpService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 14: `/// Replaces simulation with actual LLM calls via HTTP`

---

### `tools/Themis.IngestionTool/Services/LlmStatusService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.IngestionTool/Services/LoadTestRunner.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 194: `// Simulate ingestion (this would call actual pipeline)`

---

### `tools/Themis.IngestionTool/Services/PerformanceProfiler.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.IngestionTool/Services/PollyHttpResilienceService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.IngestionTool/Services/RealIngestionService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.IngestionTool/Services/ServiceImplementations.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🐛 DEBUG** (3 occurrences):
  - Line 91: `System.Diagnostics.Debug.WriteLine($"[CONNECTION] Settings updated: {_host}:{_port}");`
  - Line 105: `System.Diagnostics.Debug.WriteLine($"[HEARTBEAT ERROR] {ex.Message}");`
  - Line 117: `System.Diagnostics.Debug.WriteLine("[CONNECTION] Service created, waiting for settings");`

---

### `tools/Themis.IngestionTool/Services/ServiceInterfaces.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.IngestionTool/Services/ThemisApiService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.IngestionTool/Services/ThemisConnectionServiceGrpc.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 34: `System.Diagnostics.Debug.WriteLine($"[CONNECTION-GRPC] Settings updated: {_host}:{_port}");`

---

### `tools/Themis.IngestionTool/Services/VectorQueryService.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.IngestionTool/ViewModels/BaseViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.IngestionTool/ViewModels/CacheStatisticsViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.IngestionTool/ViewModels/GraphQueryDialogViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.IngestionTool/ViewModels/LoadTestViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.IngestionTool/ViewModels/MainWindowViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (97.0/100)

**Issues Found:**

**🐛 DEBUG** (3 occurrences):
  - Line 154: `System.Diagnostics.Debug.WriteLine($"[SETTINGS] Loaded ThemisDB: {settings.ThemisHost}:{settings.The`
  - Line 158: `System.Diagnostics.Debug.WriteLine("[SETTINGS] Using default ThemisDB settings");`
  - Line 164: `System.Diagnostics.Debug.WriteLine($"[SETTINGS ERROR] {ex}");`

---

### `tools/Themis.IngestionTool/ViewModels/RelayCommand.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.IngestionTool/ViewModels/SettingsDialogViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.IngestionTool/ViewModels/VectorQueryDialogViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.IngestionTool/ViewModels/ViewModelBase.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.IngestionTool/Views/FileDetailsView.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.IngestionTool/Views/MainWindow.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**🐛 DEBUG** (4 occurrences):
  - Line 114: `// Debug: Prüfe Bedingungen`
  - Line 115: `System.Diagnostics.Debug.WriteLine($"[START] SourceFolder: {_viewModel.SourceFolder ?? "NULL"}");`
  - Line 116: `System.Diagnostics.Debug.WriteLine($"[START] IsConnected: {_viewModel.IsConnected}");`
  - Line 117: `System.Diagnostics.Debug.WriteLine($"[START] IsRunning: {_viewModel.IsRunning}");`

---

### `tools/Themis.IngestionTool/Views/SettingsDialog.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.KeyRotationDashboard/App.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.KeyRotationDashboard/Models/KeyRotationInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.KeyRotationDashboard/ViewModels/MainViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.KeyRotationDashboard/Views/AboutWindow.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.KeyRotationDashboard/Views/MainWindow.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.KeyRotationDashboard/obj/Debug/net8.0-windows/.NETCoreApp,Version=v8.0.AssemblyAttributes.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.KeyRotationDashboard/obj/Debug/net8.0-windows/App.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.KeyRotationDashboard/obj/Debug/net8.0-windows/Themis.KeyRotationDashboard.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 14: `[assembly: System.Reflection.AssemblyConfigurationAttribute("Debug")]`

---

### `tools/Themis.KeyRotationDashboard/obj/Debug/net8.0-windows/Themis.KeyRotationDashboard.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.KeyRotationDashboard/obj/Debug/net8.0-windows/Views/AboutWindow.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.KeyRotationDashboard/obj/Debug/net8.0-windows/Views/MainWindow.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.KeyRotationDashboard/obj/Release/net8.0-windows/win-x64/.NETCoreApp,Version=v8.0.AssemblyAttributes.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.KeyRotationDashboard/obj/Release/net8.0-windows/win-x64/App.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.KeyRotationDashboard/obj/Release/net8.0-windows/win-x64/Themis.KeyRotationDashboard.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.KeyRotationDashboard/obj/Release/net8.0-windows/win-x64/Themis.KeyRotationDashboard.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.KeyRotationDashboard/obj/Release/net8.0-windows/win-x64/Views/AboutWindow.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.KeyRotationDashboard/obj/Release/net8.0-windows/win-x64/Views/MainWindow.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.PIIManager/App.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.PIIManager/Models/PiiMapping.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.PIIManager/ViewModels/MainViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 82: `// TODO: SaveFileDialog integration – for now we just confirm success`

---

### `tools/Themis.PIIManager/Views/AboutWindow.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.PIIManager/Views/MainWindow.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.PIIManager/obj/Debug/net8.0-windows/.NETCoreApp,Version=v8.0.AssemblyAttributes.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.PIIManager/obj/Debug/net8.0-windows/App.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.PIIManager/obj/Debug/net8.0-windows/Themis.PIIManager.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 14: `[assembly: System.Reflection.AssemblyConfigurationAttribute("Debug")]`

---

### `tools/Themis.PIIManager/obj/Debug/net8.0-windows/Themis.PIIManager.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.PIIManager/obj/Debug/net8.0-windows/Views/AboutWindow.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.PIIManager/obj/Debug/net8.0-windows/Views/MainWindow.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.PIIManager/obj/Release/net8.0-windows/win-x64/.NETCoreApp,Version=v8.0.AssemblyAttributes.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.PIIManager/obj/Release/net8.0-windows/win-x64/App.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.PIIManager/obj/Release/net8.0-windows/win-x64/Themis.PIIManager.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.PIIManager/obj/Release/net8.0-windows/win-x64/Themis.PIIManager.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.PIIManager/obj/Release/net8.0-windows/win-x64/Views/AboutWindow.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.PIIManager/obj/Release/net8.0-windows/win-x64/Views/MainWindow.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.RetentionManager/App.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.RetentionManager/Models/RetentionPolicy.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.RetentionManager/ViewModels/MainViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.RetentionManager/Views/AboutWindow.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.RetentionManager/Views/MainWindow.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.RetentionManager/obj/Debug/net8.0-windows/.NETCoreApp,Version=v8.0.AssemblyAttributes.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.RetentionManager/obj/Debug/net8.0-windows/App.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.RetentionManager/obj/Debug/net8.0-windows/Themis.RetentionManager.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 14: `[assembly: System.Reflection.AssemblyConfigurationAttribute("Debug")]`

---

### `tools/Themis.RetentionManager/obj/Debug/net8.0-windows/Themis.RetentionManager.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.RetentionManager/obj/Debug/net8.0-windows/Views/AboutWindow.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.RetentionManager/obj/Debug/net8.0-windows/Views/MainWindow.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.RetentionManager/obj/Release/net8.0-windows/win-x64/.NETCoreApp,Version=v8.0.AssemblyAttributes.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.RetentionManager/obj/Release/net8.0-windows/win-x64/App.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.RetentionManager/obj/Release/net8.0-windows/win-x64/Themis.RetentionManager.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.RetentionManager/obj/Release/net8.0-windows/win-x64/Themis.RetentionManager.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.RetentionManager/obj/Release/net8.0-windows/win-x64/Views/AboutWindow.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.RetentionManager/obj/Release/net8.0-windows/win-x64/Views/MainWindow.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.SAGAVerifier/AboutWindow.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.SAGAVerifier/App.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.SAGAVerifier/AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.SAGAVerifier/Converters/ValueConverters.cs` (v0.0.33)

**Maturity Level:** 🟡 RELEASE-CANDIDATE (75.0/100)

**Issues Found:**

**🔴 STUB** (5 occurrences):
  - Line 17: `throw new NotImplementedException();`
  - Line 30: `throw new NotImplementedException();`
  - Line 47: `throw new NotImplementedException();`
  - Line 64: `throw new NotImplementedException();`
  - Line 81: `throw new NotImplementedException();`

---

### `tools/Themis.SAGAVerifier/MainWindow.xaml.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.SAGAVerifier/ViewModels/MainViewModel.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/.NETCoreApp,Version=v8.0.AssemblyAttributes.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/AboutWindow.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/App.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/GeneratedInternalTypeHelper.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/MainWindow.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/Themis.SAGAVerifier.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 14: `[assembly: System.Reflection.AssemblyConfigurationAttribute("Debug")]`

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/Themis.SAGAVerifier.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/Themis.SAGAVerifier_1sz1m4b5_wpftmp.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 14: `[assembly: System.Reflection.AssemblyConfigurationAttribute("Debug")]`

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/Themis.SAGAVerifier_1sz1m4b5_wpftmp.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/Themis.SAGAVerifier_aboi3s0w_wpftmp.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 14: `[assembly: System.Reflection.AssemblyConfigurationAttribute("Debug")]`

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/Themis.SAGAVerifier_aboi3s0w_wpftmp.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/Themis.SAGAVerifier_bgfmc4wh_wpftmp.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 14: `[assembly: System.Reflection.AssemblyConfigurationAttribute("Debug")]`

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/Themis.SAGAVerifier_bgfmc4wh_wpftmp.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/Themis.SAGAVerifier_esgg4vdh_wpftmp.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 14: `[assembly: System.Reflection.AssemblyConfigurationAttribute("Debug")]`

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/Themis.SAGAVerifier_esgg4vdh_wpftmp.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/Themis.SAGAVerifier_gl1faee3_wpftmp.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 14: `[assembly: System.Reflection.AssemblyConfigurationAttribute("Debug")]`

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/Themis.SAGAVerifier_gl1faee3_wpftmp.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/Themis.SAGAVerifier_jefekf0o_wpftmp.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 14: `[assembly: System.Reflection.AssemblyConfigurationAttribute("Debug")]`

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/Themis.SAGAVerifier_jefekf0o_wpftmp.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/Themis.SAGAVerifier_kogcgjlp_wpftmp.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 14: `[assembly: System.Reflection.AssemblyConfigurationAttribute("Debug")]`

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/Themis.SAGAVerifier_kogcgjlp_wpftmp.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/Themis.SAGAVerifier_mvyhwfbc_wpftmp.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 14: `[assembly: System.Reflection.AssemblyConfigurationAttribute("Debug")]`

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/Themis.SAGAVerifier_mvyhwfbc_wpftmp.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/Themis.SAGAVerifier_ndbovtvf_wpftmp.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 14: `[assembly: System.Reflection.AssemblyConfigurationAttribute("Debug")]`

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/Themis.SAGAVerifier_ndbovtvf_wpftmp.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/Themis.SAGAVerifier_o2uvaptn_wpftmp.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 14: `[assembly: System.Reflection.AssemblyConfigurationAttribute("Debug")]`

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/Themis.SAGAVerifier_o2uvaptn_wpftmp.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/Themis.SAGAVerifier_r4kswj0c_wpftmp.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 14: `[assembly: System.Reflection.AssemblyConfigurationAttribute("Debug")]`

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/Themis.SAGAVerifier_r4kswj0c_wpftmp.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/Themis.SAGAVerifier_rngkqx2n_wpftmp.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 14: `[assembly: System.Reflection.AssemblyConfigurationAttribute("Debug")]`

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/Themis.SAGAVerifier_rngkqx2n_wpftmp.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/Themis.SAGAVerifier_urra244u_wpftmp.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 14: `[assembly: System.Reflection.AssemblyConfigurationAttribute("Debug")]`

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/Themis.SAGAVerifier_urra244u_wpftmp.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/Themis.SAGAVerifier_wzjnehst_wpftmp.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 14: `[assembly: System.Reflection.AssemblyConfigurationAttribute("Debug")]`

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/Themis.SAGAVerifier_wzjnehst_wpftmp.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/Themis.SAGAVerifier_x5mzh1o5_wpftmp.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 14: `[assembly: System.Reflection.AssemblyConfigurationAttribute("Debug")]`

---

### `tools/Themis.SAGAVerifier/obj/Debug/net8.0-windows/Themis.SAGAVerifier_x5mzh1o5_wpftmp.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.SAGAVerifier/obj/Release/net8.0-windows/win-x64/.NETCoreApp,Version=v8.0.AssemblyAttributes.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.SAGAVerifier/obj/Release/net8.0-windows/win-x64/AboutWindow.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.SAGAVerifier/obj/Release/net8.0-windows/win-x64/App.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.SAGAVerifier/obj/Release/net8.0-windows/win-x64/GeneratedInternalTypeHelper.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.SAGAVerifier/obj/Release/net8.0-windows/win-x64/MainWindow.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.SAGAVerifier/obj/Release/net8.0-windows/win-x64/Themis.SAGAVerifier.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.SAGAVerifier/obj/Release/net8.0-windows/win-x64/Themis.SAGAVerifier.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.SAGAVerifier/obj/Release/net8.0-windows/win-x64/Themis.SAGAVerifier_iie3e3pg_wpftmp.AssemblyInfo.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/Themis.SAGAVerifier/obj/Release/net8.0-windows/win-x64/Themis.SAGAVerifier_iie3e3pg_wpftmp.GlobalUsings.g.cs` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/aggregate_shard_results.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/capability_generator.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 57: `# For demonstration, we'll simulate the analysis`
  - Line 70: `"""Simulate RocksDB analysis (replace with actual implementation)"""`

---

### `tools/ci/analyze_workflows.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/ci/parse_build_errors.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/compare_hyperscaler.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/compiler_diagnostics/__init__.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/compiler_diagnostics/diagnostic_scanner.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/compiler_diagnostics/issue_tracker.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/compiler_diagnostics/source_audit.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/compiler_diagnostics/symbol_checker.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/compiler_diagnostics/warning_report.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/config_migration_scanner.cpp` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/debug_graph_keys.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/embed_certificate.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/error_handling_audit.py` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 356: `r"\b(logger\.|Logger\.|log\.|Log\.|Console\.|Debug\.|Trace\.|throw\b)"`

---

### `tools/fault_injector.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 38: `# Simulate: measure throughput before`
  - Line 111: `"""Simulate shard rebalance/expansion."""`

---

### `tools/gnn/export_to_onnx.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 52: `# Create dummy inputs for ONNX export`
  - Line 59: `print(f"  Dummy input shape: x={dummy_x.shape}, edge_index={dummy_edge_index.shape}")`

---

### `tools/gnn/gnn_example.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/gnn/train_gnn.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/import_cli.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/ingest.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 815: `logger.setLevel(logging.DEBUG)`

---

### `tools/ldap_export.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/link_ownership.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (2 occurrences):
  - Line 126: `logger.debug(f"Created OWNED_BY edge: {entity_id} -> {owner_target}")`
  - Line 151: `logger.debug(f"Created VISIBLE_TO edge: {entity_id} -> {target}")`

---

### `tools/lora_provenance_cli.cpp` (v0.0.26)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/migrate_vector_encryption.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**🎭 SIMULATION** (1 occurrences):
  - Line 207: `std::cout << "  --dry-run            Simulate migration without making changes" << std::endl;`

---

### `tools/namespace_analyzer.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/plugin_signer/sign_plugin.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/publish_wiki.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/rope_visualizer/__init__.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/rope_visualizer/cli.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/rope_visualizer/utils.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/rope_visualizer/visualizer.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/shard_bench.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (99.0/100)

**Issues Found:**

**🎭 SIMULATION** (2 occurrences):
  - Line 44: `"""Simulate single query (read/write/join/vector)."""`
  - Line 59: `latency_ms = random.gauss(0.5, 0.1)  # Simulate ~0.5ms latency`

---

### `tools/shard_loader.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 92: `# TODO: Connect to shard_host:port and INSERT`

---

### `tools/sign_pii_engine.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 163: `'cert_serial': 'VCC-PKI-001',  # TODO: Extract from certificate`

**🔒 HARDCODED** (1 occurrences):
  - Line 78: `- The 'passphrase' parameter is supplied at runtime (CLI/env) and must not be hardcoded or stored in`

---

### `tools/sign_plugin_manifest.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/themis_docs_builder/include/docs_builder.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/themis_docs_builder/include/document_parser.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/themis_docs_builder/include/rocksdb_writer.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/themis_docs_builder/include/validator.h` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/themis_docs_builder/src/docs_builder.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (96.0/100)

**Issues Found:**

**📝 TODO** (3 occurrences):
  - Line 32: `// TODO: Full implementation`
  - Line 66: `// TODO: Implement validation`
  - Line 82: `// TODO: Implement YAML parsing`

---

### `tools/themis_docs_builder/src/document_parser.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 12: `// TODO: Implement parsers for markdown, HTML, text, JSON`

---

### `tools/themis_docs_builder/src/main.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/themis_docs_builder/src/rocksdb_writer.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (98.0/100)

**Issues Found:**

**📝 TODO** (2 occurrences):
  - Line 12: `// TODO: Initialize RocksDB with 7 Column Families`
  - Line 16: `// TODO: Write to appropriate Column Family`

---

### `tools/themis_docs_builder/src/validator.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**📝 TODO** (1 occurrences):
  - Line 12: `// TODO: Validate database structure and integrity`

---

### `tools/themis_model_cli.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/themis_profiler.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/txn_smoke.cpp` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `tools/wordpress_category_extractor.py` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/includes/class-themisdb-plugin-updater.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/persistent-podcast-player/persistent-podcast-player.php` (v0.0.33)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-architecture-diagrams/templates/admin-settings.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-architecture-diagrams/templates/diagram.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-architecture-diagrams/themisdb-architecture-diagrams.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-architecture-diagrams/uninstall.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-benchmark-visualizer/templates/admin-settings.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-benchmark-visualizer/templates/visualizer.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-benchmark-visualizer/themisdb-benchmark-visualizer.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-benchmark-visualizer/uninstall.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-compendium-downloads/includes/class-compendium-admin.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-compendium-downloads/includes/class-compendium-downloads.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-compendium-downloads/includes/class-compendium-widget.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-compendium-downloads/themisdb-compendium-downloads.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 144: `// Add debug flag if WP_DEBUG is enabled`

---

### `wordpress-plugins/themisdb-docker-downloads/includes/class-admin.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-docker-downloads/includes/class-dockerhub-api.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-docker-downloads/includes/class-shortcodes.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-docker-downloads/themisdb-docker-downloads.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-downloads/includes/class-admin.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-downloads/includes/class-github-api.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-downloads/includes/class-markdown-converter.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-downloads/includes/class-shortcodes.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-downloads/includes/class-taxonomy-manager.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-downloads/themisdb-downloads.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-feature-matrix/includes/class-admin.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-feature-matrix/includes/class-feature-matrix.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-feature-matrix/includes/class-shortcode.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-feature-matrix/templates/admin-settings.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-feature-matrix/templates/matrix.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-feature-matrix/themisdb-feature-matrix.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 51: `// Log to debug.log if WP_DEBUG_LOG is enabled`

---

### `wordpress-plugins/themisdb-feature-matrix/uninstall.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-formula-renderer/includes/class-formula-library.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-formula-renderer/includes/class-formula-renderer.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-formula-renderer/includes/class-shortcodes.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-formula-renderer/themisdb-formula-renderer.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-gallery/includes/class-admin.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-gallery/includes/class-gutenberg-block.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-gallery/includes/class-image-api.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-gallery/includes/class-media-handler.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-gallery/includes/class-shortcodes.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-gallery/themisdb-gallery.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-order-request/includes/class-admin.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-order-request/includes/class-auth-system.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-order-request/includes/class-contract-manager.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-order-request/includes/class-database.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-order-request/includes/class-email-handler.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-order-request/includes/class-epserver-api.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-order-request/includes/class-license-api.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-order-request/includes/class-license-manager.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-order-request/includes/class-license-portal.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-order-request/includes/class-license-renewal.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-order-request/includes/class-order-manager.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-order-request/includes/class-payment-manager.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-order-request/includes/class-pdf-generator.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-order-request/includes/class-shortcodes.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-order-request/themisdb-order-request.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-order-request/uninstall.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-query-playground/templates/admin-settings.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-query-playground/templates/playground.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-query-playground/themisdb-query-playground.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-query-playground/uninstall.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-release-timeline/templates/admin-settings.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-release-timeline/templates/timeline.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-release-timeline/themisdb-release-timeline.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-release-timeline/uninstall.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-taxonomy-manager/includes/class-admin.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-taxonomy-manager/includes/class-analytics.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-taxonomy-manager/includes/class-category-hierarchy.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-taxonomy-manager/includes/class-custom-taxonomies.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-taxonomy-manager/includes/class-metabox.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-taxonomy-manager/includes/class-seo.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-taxonomy-manager/includes/class-taxonomy-extractor.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-taxonomy-manager/includes/class-taxonomy-manager.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-taxonomy-manager/includes/class-template-handler.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-taxonomy-manager/includes/class-term-meta.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-taxonomy-manager/includes/class-tfidf.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-taxonomy-manager/includes/class-tree-view.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-taxonomy-manager/includes/class-widget.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-taxonomy-manager/templates/taxonomy-archive.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-taxonomy-manager/themisdb-taxonomy-manager.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🐛 DEBUG** (1 occurrences):
  - Line 60: `// Log to debug.log if WP_DEBUG_LOG is enabled`

---

### `wordpress-plugins/themisdb-taxonomy-manager/uninstall.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-taxonomy-manager/verify-implementation.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (86.0/100)

**Issues Found:**

**🎭 SIMULATION** (5 occurrences):
  - Line 8: `// Simulate WordPress environment for testing`
  - Line 23: `// Mock WordPress functions for testing`
  - Line 36: `// Calculate TF-IDF (will be limited due to mock environment)`
  - Line 57: `// Mock WordPress taxonomy functions`
  - Line 60: `return array(); // Return empty array for mock`

---

### `wordpress-plugins/themisdb-tco-calculator/templates/admin-settings.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-tco-calculator/templates/calculator.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-tco-calculator/templates/sections/ai.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-tco-calculator/templates/sections/infrastructure.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-tco-calculator/templates/sections/operations.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-tco-calculator/templates/sections/personnel.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-tco-calculator/templates/sections/workload.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-tco-calculator/themisdb-tco-calculator.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-tco-calculator/uninstall.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-test-dashboard/templates/admin-settings.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-test-dashboard/templates/dashboard.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-test-dashboard/themisdb-test-dashboard.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

**Issues Found:**

**🎭 SIMULATION** (3 occurrences):
  - Line 145: `// Mock data for demonstration (replace with actual GitHub API calls)`
  - Line 159: `// Simulate improving coverage`
  - Line 183: `// Mock data for demonstration`

---

### `wordpress-plugins/themisdb-test-dashboard/uninstall.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-wiki-integration/includes/class-admin.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-wiki-integration/includes/class-github-sync.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-wiki-integration/includes/class-markdown-converter.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-wiki-integration/includes/class-search.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-wiki-integration/includes/class-version-manager.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-wiki-integration/includes/class-wiki.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-wiki-integration/includes/class-wikilinks.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-wiki-integration/templates/admin-settings.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-wiki-integration/templates/archive-themisdb_wiki.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-wiki-integration/templates/single-themisdb_wiki.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-wiki-integration/themisdb-wiki-integration.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

### `wordpress-plugins/themisdb-wiki-integration/wordpress_doc_importer.php` (v0.0.1)

**Maturity Level:** 🟢 PRODUCTION-READY (100.0/100)

---

## 🎯 Recommended Actions

1. **Implement 1102 stub(s)** - Replace placeholder code with real implementations
2. **Resolve 430 TODO(s)** - Complete pending work items
3. **Replace 1823 simulation(s)** - Integrate real services/data
