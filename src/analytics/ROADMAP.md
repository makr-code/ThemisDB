# Analytics Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
Production-ready for core OLAP, data export, process mining, text analytics, LLM integration, CEP engine, streaming aggregation windows, incremental materialized views, and real-time anomaly detection.

## Completed ✅
- [x] OLAP engine with GROUP BY, CUBE, ROLLUP, and GROUPING SETS
- [x] Window functions (ROW_NUMBER, SUM OVER, AVG OVER with frame specs)
- [x] Statistical aggregations (COUNT, SUM, AVG, MIN, MAX, STDDEV, VARIANCE, MEDIAN, PERCENTILE)
- [x] Hash-based aggregation with result caching
- [x] Columnar (Arrow) RecordBatch storage always available
- [x] JSON and CSV export (no external dependencies)
- [x] Optional Apache Arrow IPC, Parquet (with compression), and Feather export
- [x] Process mining: Alpha Miner, Heuristic Miner, Inductive Miner
- [x] Conformance checking (token replay and alignment-based)
- [x] Process pattern matcher (graph, vector, behavioral, hybrid similarity)
- [x] NLP text analyzer (tokenization, TF-IDF, NER, sentiment, keyword extraction)
- [x] LLM process analyzer (OpenAI, Anthropic, Azure OpenAI, llama.cpp)
- [x] Diff engine (changefeed-backed git-like diffs)
- [x] SIMD-accelerated aggregations (AVX2)
- [x] Thread-safe OLAPEngine for concurrent queries
- [x] CEP full engine (NFA pattern matching, EPL parser, window+aggregation pipeline, alert dispatch, CDC integration) (`analytics/cep_engine.cpp`)
- [x] Streaming aggregation windows: TumblingWindow, SlidingWindow, SessionWindow, HoppingWindow with watermark support (`analytics/streaming_window.cpp`)
- [x] Incremental materialized views with delta-maintenance for all 10 aggregation functions, Welford STDDEV/VARIANCE, COUNT_DISTINCT ref-counting (`analytics/incremental_view.cpp`)
- [x] Real-time anomaly detection: Z-Score, Modified Z-Score (MAD), IQR, Isolation Forest, LOF, Ensemble with adaptive learning (`analytics/anomaly_detection.cpp`)

## In Progress 🚧
*(none — all Phase 2 items completed)*

## Planned Features 📋

### Short-term (Next 3-6 months)
- [ ] GPU-accelerated OLAP aggregations (CUDA)
- [ ] Zero-copy Arrow data transfer optimizations
- [ ] Arrow Flight RPC support for remote analytics

### Long-term (6-12 months)
- [I] Predictive analytics and time-series forecasting (Issue: #1473)
- [I] AutoML integration for automated model selection (Issue: #1485)
- [I] Advanced graph analytics: betweenness centrality, Louvain community detection (Issue: #1475)
- [I] Integration with external ML tools (ONNX Runtime, TensorFlow Serving) (Issue: #1476)
- [I] Model serving and online inference pipeline (Issue: #1477)
- [I] Multi-language NLP support (beyond English) (Issue: #1478)
- [I] Full morphological lemmatization (Issue: #1479)

## Implementation Phases

### Phase 1: Core Analytics Engine (Status: Completed ✅)
- [x] OLAP engine with GROUP BY, CUBE, ROLLUP, and GROUPING SETS (`analytics/olap_engine.cpp`)
- [x] Window functions: ROW_NUMBER, SUM OVER, AVG OVER with frame specifications
- [x] Statistical aggregations (COUNT, SUM, AVG, MIN, MAX, STDDEV, VARIANCE, MEDIAN, PERCENTILE)
- [x] Hash-based aggregation with result caching
- [x] Columnar Arrow RecordBatch storage always available
- [x] JSON, CSV, Parquet, and Feather export (`analytics/exporters/`)
- [x] Process mining: Alpha Miner, Heuristic Miner, Inductive Miner (`analytics/process_mining/`)
- [x] Conformance checking (token replay and alignment-based)
- [x] NLP text analyzer: tokenization, TF-IDF, NER, sentiment, keyword extraction (`analytics/nlp_analyzer.cpp`)
- [x] LLM process analyzer with OpenAI, Anthropic, Azure OpenAI, llama.cpp providers
- [x] Diff engine (changefeed-backed git-like diffs, `analytics/diff_engine.cpp`)
- [x] SIMD-accelerated aggregations (AVX2) in `analytics/simd_aggregations.cpp`
- [x] Thread-safe OLAPEngine for concurrent queries

### Phase 2: Streaming & Incremental Analytics (Status: Completed ✅)
- [x] CEP full engine implementation in `analytics/cep_engine.cpp`
- [x] Streaming aggregation windows (tumbling/sliding/session/hopping) in `analytics/streaming_window.cpp`
- [x] Incremental materialized views in `analytics/incremental_view.cpp`

### Phase 3: Distributed & ML-Augmented Analytics (Status: Planned 📋)
- [I] Columnar execution engine with vectorized operator pipeline (Issue: #1481)
- [I] LLVM-JIT compilation for hot aggregation paths (Issue: #1482)
- [I] Distributed analytics sharding across cluster nodes (Issue: #1483)
- [I] Predictive analytics and time-series forecasting integration (Issue: #1484)
- [ ] AutoML integration for automated model selection

## Production Readiness Checklist
- [x] Unit tests (OLAP, Arrow export, process mining, NLP, diff engine)
- [~] Unit tests coverage > 80% (test files added for all Phase 2 components; measured coverage pending CI run)
- [x] Integration tests (query module, index module, CDC)
- [x] CEP engine integration tests (`tests/analytics/test_cep_engine.cpp`)
- [x] Performance benchmarks (OLAP, export, process mining, graph, NLP)
- [x] Security audit (LLM API key handling, data export sanitization)
- [x] Documentation complete (API docs, OLAP guide, process mining guide)
- [x] API stability guaranteed for OLAP, export, and process mining

## Known Issues & Limitations
- NLP text analyzer uses rule-based approaches — not suitable as a replacement for full NLP frameworks
- LLM analyzer requires external API keys; responses are non-deterministic
- Arrow-dependent formats (Parquet, Feather, IPC) require compile-time flag `THEMIS_HAS_ARROW`
- Basic lemmatization only; full morphological analysis not yet supported
- Graph analytics limited to PageRank and basic algorithms; advanced algorithms planned for v1.8.0

## Breaking Changes
- Arrow export format options may expand in v1.7.0 (additive, non-breaking)
