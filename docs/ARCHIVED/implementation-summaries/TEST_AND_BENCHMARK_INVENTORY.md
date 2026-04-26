# ThemisDB – Test & Benchmark Inventory

**Stand:** 6. April 2026  
**Version:** 1.5.0-dev  
**Kategorie:** Testing & Quality Assurance

---

## Übersicht

| Kategorie | Dateien | Funktionen/Test-Fälle | Status |
|-----------|---------|----------------------|--------|
| C++ Benchmarks (Google Benchmark) | 122 | 1.108 | ✅ Vollständig |
| C++ Tests (Google Test) | 732 | 10.436 | ✅ Vollständig |
| Go Client Tests & Benchmarks | 4 Test-Dateien + 2 Benchmark-Dateien | 40 Tests + 25 Benchmarks | ✅ Vollständig |
| Python Skripte & Tools | 9 | – | ✅ Vollständig |
| Python Unit Tests | 1 | – | ✅ Vollständig |
| **Gesamt** | **872+** | **~11.609 Test-Fälle & Benchmarks** | ✅ |

---

## 1. C++ Benchmarks (Google Benchmark)

Alle Benchmark-Dateien befinden sich in `benchmarks/`.

**Gesamt: 122 Dateien mit 1.108 Benchmark-Funktionen** (gezählt als `BENCHMARK(…)` / `BENCHMARK_F(…)` Makros, ohne `BENCHMARK_MAIN`)

### 1.1 Core-Datenbankoperationen

| Datei | Beschreibung | Metriken |
|-------|-------------|---------|
| `bench_crud.cpp` | CRUD-Operationen mit verschiedenen Index-Typen | ops/s, Latenz p50/p95/p99 |
| `bench_query.cpp` | Query-Engine Performance | Query-Latenz, Throughput |
| `bench_insert_profiling.cpp` | Insert-Operationen Profiling | Insert-Latenz, Batch-Größen |
| `bench_batch_insert.cpp` | Batch-Insert Performance | Rows/s, Latenz |
| `bench_ingestion_kv.cpp` | Key-Value Ingestion Throughput | MB/s, ops/s |
| `bench_simple_insert_test.cpp` | Basis-Insert-Benchmark (Sanity Check) | ops/s |
| `bench_sanity.cpp` | Sanity-Check-Benchmark | Basis-Metriken |
| `bench_storage_performance.cpp` | Storage-Layer Performance | Read/Write MB/s |
| `bench_index_rebuild.cpp` | Index-Rebuild Performance | Zeit, GB/s |
| `bench_mvcc.cpp` | MVCC Transaktions-Isolation | TPS, Konfliktrate |

### 1.2 OLAP & Analytics

| Datei | Beschreibung | Metriken |
|-------|-------------|---------|
| `bench_olap_performance.cpp` | OLAP Query Performance (1K bis 1M Rows) | Query-Zeit, Skalierung |
| `bench_olap_analytics.cpp` | Analytics Query Engine | Aggregations-Latenz |
| `bench_tpcc.cpp` | TPC-C OLTP Benchmark | TPS (Transactions/s) |
| `bench_tpch.cpp` | TPC-H OLAP Benchmark | Execution-Zeit pro Query |
| `bench_ycsb.cpp` | YCSB (Yahoo Cloud Serving Benchmark) | ops/s nach Workload-Typ |
| `bench_comprehensive.cpp` | Umfassender End-to-End Benchmark | Kombinations-Metriken |
| `bench_process_mining.cpp` | Process Mining Analytics | Trace-Analyse-Latenz |

### 1.3 Transaktionen & Concurrency

| Datei | Beschreibung | Metriken |
|-------|-------------|---------|
| `bench_transaction_throughput.cpp` | ACID Transaction Throughput | TPS, Commit-Latenz |
| `bench_saga_compensation.cpp` | SAGA Rollback Performance | Kompensations-Latenz |
| `bench_lock_contention.cpp` | Lock-Contention Analysis | Wartezeiten, Deadlock-Rate |
| `bench_distributed_coordinator.cpp` | Distributed Transaction Coordinator | Koordinations-Overhead |

### 1.4 Sharding & Verteilung

| Datei | Beschreibung | Metriken |
|-------|-------------|---------|
| `bench_shard_routing.cpp` | Shard-Routing Latenz | Routing-Zeit, Consistent-Hash |
| `bench_shard_resource_manager.cpp` | Shard-Ressourcenverwaltung | CPU/RAM-Nutzung |
| `bench_sharding_performance.cpp` | Gesamte Sharding-Performance | Query-Verteilung, Latenz |
| `bench_locality_aware_router.cpp` | Locality-Aware Query Routing | Netzwerk-Overhead |

### 1.5 LLM & KI-Infrastruktur

| Datei | Beschreibung | Metriken |
|-------|-------------|---------|
| `bench_llm_infrastructure.cpp` | LLM Infrastructure Components | Token/s, Latenz |
| `bench_llm_inference_performance.cpp` | LLM Inference Performance | Token/s, TTFT |
| `bench_llm_response_cache.cpp` | LLM Response Cache Performance | Cache-Hit-Rate, Latenz |
| `bench_llm_judge_integration.cpp` | LLM-as-Judge Integration | Bewertungs-Latenz |
| `bench_llm_raid_pipeline.cpp` | RAID-LLM Pipeline | Pipeline-Throughput |
| `bench_llm_real_models.cpp` | Real-Model Inference | Modell-spezifische Metriken |
| `bench_embedded_llm.cpp` | Embedded LLM Performance | Latenz auf Edge-Hardware |
| `bench_extended_context.cpp` | Extended Context Window | Kontext-Längen-Skalierung |

### 1.6 LoRA & Training

| Datei | Beschreibung | Metriken |
|-------|-------------|---------|
| `bench_lora_inline.cpp` | LoRA Framework Inline-Performance | Forward-Pass-Latenz |
| `bench_lora_training.cpp` | LoRA Fine-Tuning Performance | Tokens/s, VRAM-Nutzung |
| `bench_lora_gpu.cpp` | LoRA GPU-Beschleunigung | GPU-Auslastung, Throughput |
| `bench_lora_auto_binding.cpp` | LoRA Auto-Binding Performance | Binding-Latenz |
| `bench_fused_lora_kernels.cpp` | Fused LoRA GPU-Kernels | FLOP/s, Kernel-Zeit |
| `bench_multi_lora_fusion.cpp` | Multi-LoRA Fusion | Fusions-Overhead |
| `bench_multi_gpu_lora_advanced.cpp` | Multi-GPU LoRA (Advanced) | GPU-Skalierung |
| `bench_qlora_gpu_kernels.cpp` | QLoRA GPU-Kernel-Optimierungen | Kernel-Effizienz, VRAM |
| `bench_legal_lora_pipeline.cpp` | Legal LoRA Training Pipeline | Pipeline-Throughput |
| `bench_raid_lora.cpp` | RAID-LoRA Integration | Speicher-Redundanz, Speed |
| `bench_vulkan_lora.cpp` | Vulkan-basierte LoRA-Ausführung | Cross-Platform-Performance |

### 1.7 GPU & Beschleunigung

| Datei | Beschreibung | Metriken |
|-------|-------------|---------|
| `bench_gpu_backends.cpp` | GPU Backend-Vergleich (CUDA/HIP/Metal/Vulkan) | FLOP/s, Latenz |
| `bench_gpu_module.cpp` | GPU-Modul Performance | Kernel-Launch-Overhead |
| `bench_gpu_training_cycle.cpp` | GPU Training Cycle | Epoch-Zeit, Throughput |
| `bench_gpu_vector_index.cpp` | GPU-beschleunigter Vektor-Index | Index-Build-Zeit, Query/s |
| `bench_gpu_vram_allocation.cpp` | VRAM Allocation Strategies | Allokations-Latenz, Fragmentierung |
| `bench_gpu_erasure.cpp` | GPU Erasure Coding | Encode/Decode-Geschwindigkeit |
| `bench_fused_kernels.cpp` | Fused GPU-Kernels | Kernel-Fusion-Speedup |
| `bench_flash_attention.cpp` | Flash Attention Performance | Attention-Latenz, Skalierung |
| `bench_phase1_flash_attention.cpp` | Flash Attention Phase 1 | Erste Implementierungs-Metriken |
| `bench_rotary_embeddings.cpp` | Rotary Position Embeddings (RoPE) | RoPE-Berechnungs-Zeit |
| `bench_multi_gpu_scaling.cpp` | Multi-GPU Skalierung | Linear Scaling Factor |
| `bench_arm_simd.cpp` | ARM SIMD-Optimierungen | SIMD vs. Scalar Performance |
| `bench_arm_memory.cpp` | ARM Memory Access Patterns | Speicher-Bandbreite |
| `bench_simd_distance.cpp` | SIMD Distance Computations | Distance-Berechnung op/s |

### 1.8 Vektor-Suche & Embeddings

| Datei | Beschreibung | Metriken |
|-------|-------------|---------|
| `bench_mmdb.cpp` | Multi-Modal Database mit Embeddings | Embedding-Suche, Latenz |
| `bench_vector_search.cpp` | Vektor-Ähnlichkeitssuche | ANN-Recall, Query/s |
| `bench_hnsw_prefilter_minimal.cpp` | HNSW Pre-Filter Minimal | Filter-Overhead |
| `bench_hybrid_vector_geo.cpp` | Hybrid Vektor+Geo Suche | Kombinations-Query-Latenz |
| `bench_embedding_cache_performance.cpp` | Embedding-Cache Performance | Cache-Hit-Rate, Latenz |
| `bench_gnn_embeddings.cpp` | GNN Embedding Generation | Node2Vec/GraphSAGE Latenz |
| `bench_vector_prefilter.cpp` | Vektor-Prefilter Performance | Prefilter-Effektivität |
| `bench_approximate_radius_search.cpp` | Approximate Radius Search | Recall vs. Speed Trade-off |

### 1.9 Quantisierung & Komprimierung

| Datei | Beschreibung | Metriken |
|-------|-------------|---------|
| `bench_mixed_precision_perf.cpp` | FP32 vs FP16 Precision Analysis | Genauigkeit vs. Speed |
| `bench_binary_quantization.cpp` | Binary Quantization Performance | Kompressionsrate, Recall |
| `bench_product_quantization.cpp` | Product Quantization | PQ-Encode/Decode-Zeit |
| `bench_residual_quantization.cpp` | Residual Quantization | RQ-Performance |
| `bench_learned_quantization.cpp` | Learned Quantization | Training- vs. Inference-Zeit |
| `bench_compression.cpp` | Datenkomprimierung | Ratio, Encode/Decode-MB/s |
| `bench_blob_zstd.cpp` | Zstandard Blob-Komprimierung | Kompressionsrate, Speed |
| `bench_vector_compression_lossless.cpp` | Verlustfreie Vektorkomprimierung | Kompressionsrate |
| `bench_lossy_vs_lossless.cpp` | Lossy vs. Lossless Vergleich | Recall-Verlust vs. Größe |

### 1.10 Graph-Datenbank

| Datei | Beschreibung | Metriken |
|-------|-------------|---------|
| `bench_graph_traversal.cpp` | Graph Traversal (BFS/DFS) | Nodes/s, Traversal-Zeit |
| `bench_pagerank.cpp` | PageRank Berechnung | Iterations/s, Konvergenz |
| `bench_graph_query_optimizer.cpp` | Graph Query Optimizer | Optimierungs-Overhead |
| `bench_hybrid_aql_sugar.cpp` | Hybrid AQL (Graph+Geo) | Kombinierte Query-Latenz |
| `bench_aql_functions.cpp` | AQL Built-in Functions | Funktion-Aufruf-Latenz |

### 1.11 PostgreSQL Wire Protocol

| Datei | Beschreibung | Metriken |
|-------|-------------|---------|
| `bench_postgres_e2e.cpp` | PostgreSQL Wire Protocol End-to-End | Verbindungs-Latenz, TPS |
| `bench_postgres_protocol.cpp` | PostgreSQL Protokoll-Parsing | Parse-Throughput |
| `bench_postgres_transactions.cpp` | PostgreSQL Transaktionen | TPS, Commit-Zeit |
| `bench_stream_protocol.cpp` | Stream-Protokoll Performance | Streaming-Throughput |

### 1.12 Zeitreihen

| Datei | Beschreibung | Metriken |
|-------|-------------|---------|
| `bench_timeseries_ingestion.cpp` | Time-Series Write Throughput | Points/s, Kompressionsrate |
| `bench_cycle_metrics.cpp` | Cycle-Metriken für Zeitreihen | Cycle-Erkennungs-Latenz |

### 1.13 Sicherheit & Compliance

| Datei | Beschreibung | Metriken |
|-------|-------------|---------|
| `bench_encryption.cpp` | Verschlüsselungs-Performance | Encrypt/Decrypt MB/s |
| `bench_hsm_provider.cpp` | HSM Provider Performance | HSM-Operationen/s |
| `bench_compliance_security_governance.cpp` | Compliance & Governance | Policy-Evaluations-Latenz |
| `bench_policy_evaluation.cpp` | Policy Rule Evaluation | Rules/s, Komplexe Policies |

### 1.14 RAG & Ethics AI

| Datei | Beschreibung | Metriken |
|-------|-------------|---------|
| `bench_ethics_ai_plugin.cpp` | Ethics AI Plugin Performance | Bewertungs-Latenz, Throughput |
| `bench_rag_ethics.cpp` | RAG Ethics Integration | RAG-Pipeline-Latenz |
| `bench_knowledge_gap_detector_phase2.cpp` | Knowledge Gap Detector | Lücken-Erkennungs-Zeit |

### 1.15 Content & Medien

| Datei | Beschreibung | Metriken |
|-------|-------------|---------|
| `bench_text_extraction.cpp` | Text Extraction (PDF, DOCX, HTML) | Extraction MB/s |
| `bench_image_analysis.cpp` | Bildanalyse Performance | Inference-Latenz |
| `bench_image_analysis_latency.cpp` | Bildanalyse Latenzmessung | p50/p95/p99-Latenz |
| `bench_video_processor.cpp` | Video Processing Performance | Frames/s |
| `bench_voice_assistant.cpp` | Voice Assistant Performance | STT/TTS-Latenz |
| `bench_content_versioning.cpp` | Content Version Management | Version-Erstellungs-Latenz |
| `bench_diff_engine.cpp` | Diff-Engine Performance | Diff-Berechungs-Zeit |

### 1.16 Netzwerk & I/O

| Datei | Beschreibung | Metriken |
|-------|-------------|---------|
| `bench_wal_apply_grpc.cpp` | WAL-Apply über gRPC | Replikations-Latenz |
| `bench_wal_stress.cpp` | WAL Stress-Test | Write-Throughput unter Last |
| `bench_async_io_multiscan.cpp` | Async I/O Multi-Scan | I/O-Throughput |
| `bench_random_access_prefetch.cpp` | Random Access Prefetch | Cache-Miss-Reduktion |
| `bench_data_transfer.cpp` | Daten-Transfer Performance | Netzwerk-Bandbreite |
| `bench_auto_buffers.cpp` | Automatische Buffer-Verwaltung | Buffer-Effizienz |
| `bench_changefeed_throughput.cpp` | Changefeed Event Processing | Events/s, SSE-Overhead |

### 1.17 Branch- & Snapshot-Management

| Datei | Beschreibung | Metriken |
|-------|-------------|---------|
| `bench_branch_manager.cpp` | Branch-Verwaltungs Performance | Branch-Erstellungs-Zeit |
| `bench_snapshot_manager.cpp` | Snapshot-Verwaltungs Performance | Snapshot-Erstellungs-Zeit |
| `bench_docker_raid_comprehensive.cpp` | Docker RAID Comprehensive | RAID-Operationen, Redundanz |

### 1.18 Plugin & Monitoring

| Datei | Beschreibung | Metriken |
|-------|-------------|---------|
| `bench_plugin_system.cpp` | Plugin-System Performance | Plugin-Load-Zeit |
| `bench_plugin_hot_plug.cpp` | Hot-Plug Plugin Performance | Hot-Reload-Latenz |
| `bench_metrics_collector.cpp` | Metrics Collector Performance | Metrik-Sampling-Overhead |
| `bench_gossip_config.cpp` | Gossip Protocol Configuration | Gossip-Konvergenz-Zeit |
| `bench_hotspots_micro.cpp` | Micro-Hotspot Analyse | Hotspot-Identifikations-Zeit |

### 1.19 Weitere & Versions-Benchmarks

| Datei | Beschreibung | Metriken |
|-------|-------------|---------|
| `bench_core_performance.cpp` | Core-System Performance | Allgemeine Baseline |
| `bench_backend_comparison.cpp` | Storage-Backend Vergleich | Relative Performance |
| `bench_advanced_patterns.cpp` | Erweiterte Query-Patterns | Pattern-Matching-Latenz |
| `bench_cross_functional_end_to_end.cpp` | Cross-Functional E2E | Gesamtsystem-Latenz |
| `bench_spatial_index.cpp` | Räumlicher Index Performance | Geo-Query/s |
| `bench_v1_3_0_features.cpp` | v1.3.0 Feature Benchmarks | Version-Metriken |
| `bench_v1_3_4_optimizations.cpp` | v1.3.4 Optimierungs-Benchmarks | Optimierungs-Gewinne |

---

## 2. C++ Tests (Google Test)

Alle Test-Dateien befinden sich in `tests/`.

### Zusammenfassung

| Kategorie | Dateien | Test-Funktionen |
|-----------|---------|----------------|
| Unit Tests | ~620 | ~9.000 |
| Integration Tests | ~80 | ~1.200 |
| Performance Tests | ~30 | ~236 |
| Fuzzing Tests (`fuzz/`) | Separate Suite | – |
| **Gesamt** | **732** | **10.436** |

> Gezählt als Anzahl der `TEST(…)`, `TEST_F(…)` und `TEST_P(…)` Makros in `tests/`.

### Wichtige Test-Module

| Modul | Test-Dateien (Beispiele) | Abdeckung |
|-------|--------------------------|-----------|
| Storage Layer | `test_storage_*.cpp`, `test_rocksdb_*.cpp` | ✅ |
| Transaktionen | `test_transaction_manager.cpp`, `test_saga_logger.cpp` | ✅ |
| Sharding | `test_sharding_core.cpp`, `test_rebalance_migration.cpp` | ✅ |
| CDC | `test_http_changefeed.cpp`, `test_http_changefeed_sse.cpp` | ✅ |
| Zeitreihen | `test_http_timeseries.cpp`, `test_timeseries_retention.cpp` | ✅ |
| Graph | `test_graph_analytics.cpp`, `test_gnn_embeddings.cpp` | ✅ |
| LLM | `test_prompt_manager.cpp` | ✅ |
| Authentifizierung | `test_auth_*.cpp`, `test_jwt_*.cpp` | ✅ |
| Vector Index | `test_spatial_index.cpp`, `test_hybrid_queries.cpp` | ✅ |
| Performance | `test_performance_*.cpp`, `test_rcu_index.cpp` | ✅ |
| Plugin System | `test_plugin_manager.cpp` | ✅ |
| Backup & Recovery | `test_backup_restore.cpp`, `test_wal_backup_manager.cpp` | ✅ |
| Governance | `test_policy_engine_load.cpp`, `test_http_governance.cpp` | ✅ |

---

## 3. Go Client Tests & Benchmarks

Alle Dateien befinden sich in `clients/go/`.

**Gesamt: 40 Test-Funktionen (`func Test…`) + 25 Benchmark-Funktionen (`func Benchmark…`)**

### 3.1 Test-Dateien (40 Test-Funktionen)

| Datei | Beschreibung | Test-Typ |
|-------|-------------|---------|
| `client_test.go` | Client-Basis-Funktionalität | Unit Test |
| `wire_protocol_test.go` | Wire Protocol Implementierung | Unit Test |
| `tls_config_test.go` | TLS-Konfiguration & Zertifikate | Unit Test |
| `client_rest_test.go` | REST API Client Tests | Integration Test |

### 3.2 Benchmark-Dateien (25 Benchmark-Funktionen)

| Datei | Beschreibung | Metriken |
|-------|-------------|---------|
| `client_bench_test.go` | Client-Performance Benchmarks | Latenz, Throughput |
| `wire_protocol_bench_test.go` | Wire Protocol Benchmarks | Protokoll-Overhead |

### 3.3 Implementierungs-Dateien

| Datei | Beschreibung |
|-------|-------------|
| `client.go` | Haupt-Client-Implementierung |
| `themis_client.go` | Wire Protocol Client |

### Benchmarks ausführen

```bash
cd clients/go
# Alle Tests
go test ./...

# Nur Benchmarks
go test -bench=. -benchmem ./...

# Spezifischer Benchmark
go test -bench=BenchmarkWireProtocol -benchmem
```

---

## 4. Python Skripte & Tools

### 4.1 Benchmark-Analyse

| Datei | Pfad | Beschreibung |
|-------|------|-------------|
| `statistics.py` | `benchmarks/chimera/` | Statistische Analyse für CHIMERA-Benchmarks |
| `reporter.py` | `benchmarks/chimera/` | CHIMERA Reporting Engine |

### 4.2 Dokumentations-Generierung

| Datei | Pfad | Beschreibung |
|-------|------|-------------|
| `step2_generate_html.py` | `compendium/` | HTML-Generierung für Compendium |
| `generate_pdf_rendered.py` | `scripts/compendium/` | PDF-Generierung (gerendert) |
| `generate_docs_rocksdb_backup.py` | `scripts/` | RocksDB Dokumentations-Generator |
| `link-check.py` | `scripts/` | Link-Validierung für Dokumentation |

### 4.3 Data-Tools

| Datei | Pfad | Beschreibung |
|-------|------|-------------|
| `ingest.py` | `tools/` | Data Ingestion Tool |
| `train_failure_model.py` | `scripts/` | Failure Prediction ML Model |
| `ingest_legal_training_data.py` | `scripts/` | Legal Training Data Ingestion |

### 4.4 Python Unit Tests

| Datei | Pfad | Beschreibung |
|-------|------|-------------|
| `test_utils.py` | `tests/rope_visualizer/` | Rope Visualizer Unit Tests |

### 4.5 Beispiel-Clients

| Datei | Pfad | Beschreibung |
|-------|------|-------------|
| `themis_client.py` | `examples/23_traveling_salesman/` | TSP-Beispiel Client (Traveling Salesman Problem) |

---

## 5. Abdeckungs-Matrix nach Fähigkeiten

| Fähigkeit | Unit Tests | Benchmarks | Status |
|-----------|------------|-----------|--------|
| CRUD Operations | ✅ | `bench_crud.cpp` | ✅ Vollständig |
| Graph Operations | ✅ | `bench_graph_traversal.cpp`, `bench_pagerank.cpp` | ✅ Vollständig |
| LoRA Framework | ✅ | `bench_lora_inline.cpp`, `bench_lora_training.cpp` | ✅ Vollständig |
| LLM Infrastructure | ✅ | `bench_llm_infrastructure.cpp` | ✅ Vollständig |
| Vector/Embeddings | ✅ | `bench_mmdb.cpp`, `bench_vector_search.cpp` | ✅ Vollständig |
| OLAP Queries | ✅ | `bench_olap_performance.cpp`, `bench_tpch.cpp` | ✅ Vollständig |
| RAG/Ethics | ✅ | `bench_ethics_ai_plugin.cpp`, `bench_rag_ethics.cpp` | ✅ Vollständig |
| PostgreSQL Wire | ✅ | `bench_postgres_e2e.cpp` | ✅ Vollständig |
| GPU Kernels | ✅ | `bench_qlora_gpu_kernels.cpp`, `bench_fused_kernels.cpp` | ✅ Vollständig |
| Client SDK (Go) | ✅ | `client_bench_test.go`, `wire_protocol_bench_test.go` | ✅ Vollständig |
| Sharding | ✅ | `bench_shard_routing.cpp`, `bench_sharding_performance.cpp` | ✅ Vollständig |
| Transaktionen (ACID) | ✅ | `bench_transaction_throughput.cpp`, `bench_mvcc.cpp` | ✅ Vollständig |
| CDC/Changefeed | ✅ | `bench_changefeed_throughput.cpp` | ✅ Vollständig |
| Zeitreihen | ✅ | `bench_timeseries_ingestion.cpp` | ✅ Vollständig |
| Sicherheit/Verschlüsselung | ✅ | `bench_encryption.cpp`, `bench_hsm_provider.cpp` | ✅ Vollständig |
| Content Processing | ✅ | `bench_text_extraction.cpp`, `bench_image_analysis.cpp` | ✅ Vollständig |

---

## 6. Performance-Ziele

| Bereich | Ziel | Aktueller Stand |
|---------|------|----------------|
| CRUD Throughput | > 100K ops/s | ✅ Erreicht |
| Vector Search (ANN) | < 1ms p99 @ 1M Vektoren | ✅ Erreicht |
| LLM Inference | > 50 Token/s (7B-Modell) | ✅ Erreicht |
| PostgreSQL Wire TPS | > 10K TPS | ✅ Erreicht |
| OLAP Query (1M Rows) | < 100ms | ✅ Erreicht |
| LoRA Training | > 1K Tokens/s | ✅ Erreicht |
| Graph Traversal (10K Nodes) | < 10ms | ✅ Erreicht |

---

## 7. Benchmarks ausführen

### C++ Benchmarks

```bash
# Build mit Benchmark-Support
cmake -B build -S . -DTHEMIS_BUILD_BENCHMARKS=ON
cmake --build build --target benchmarks

# Einzelnen Benchmark ausführen
./build/benchmarks/bench_crud --benchmark_format=json

# Alle Benchmarks
cd build/benchmarks
for bench in bench_*; do ./$bench --benchmark_format=json >> results.json; done
```

### Go Benchmarks

```bash
cd clients/go
go test -bench=. -benchmem -benchtime=10s ./...
```

### CHIMERA Benchmark Suite

```bash
# CHIMERA-Suite starten (Python-basiert)
cd benchmarks/chimera
python3 reporter.py --config chimera.yaml --output results/
```

---

## 8. CI/CD Integration

Benchmarks werden in folgenden CI-Pipelines ausgeführt:

- **Nightly Benchmarks**: Alle C++ Benchmarks in `.github/workflows/`
- **PR-Checks**: Sanity-Benchmarks bei jedem Pull Request
- **Release-Validierung**: Vollständige Benchmark-Suite bei Release

Siehe auch: [BENCHMARK_RUNBOOK.md](../../performance/BENCHMARK_RUNBOOK.md) für operative Details.

---

**Letzte Aktualisierung:** Februar 2026  
**Maintainer:** ThemisDB Core Team  
**Siehe auch:** [TESTING_AND_BENCHMARKING_GUIDE.md](TESTING_AND_BENCHMARKING_GUIDE.md) | [BENCHMARK_RUNBOOK.md](../../performance/BENCHMARK_RUNBOOK.md) | [Benchmark & Test Audit](de/reports/BENCHMARK_AND_TEST_AUDIT.md)
