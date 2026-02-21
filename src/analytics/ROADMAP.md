# Analytics Module Roadmap

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
Production-ready for core OLAP, data export, process mining, text analytics, and LLM integration. Complex Event Processing (CEP) is a header-only stub awaiting full implementation.

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

## In Progress 🚧
- [I] Complex Event Processing (CEP) full engine implementation (Target: Q2 2026) (Issue: #1443)
- [I] Streaming aggregation windows (tumbling, sliding, session, hopping) (Target: Q2 2026) (Issue: #1454)
- [I] Incremental materialized views (Target: Q3 2026) (Issue: #1480)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [I] CEP: EPL (Event Processing Language) parser (Issue: #1466)
- [I] CEP: Stateful pattern matching with checkpointing (Issue: #1467)
- [I] CEP: Backpressure handling and buffer management (Issue: #1468)
- [I] GPU-accelerated OLAP aggregations (CUDA) (Issue: #1469)
- [I] Real-time anomaly detection engine (Issue: #1470)
- [I] Zero-copy Arrow data transfer optimizations (Issue: #1471)
- [I] Arrow Flight RPC support for remote analytics (Issue: #1472)

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

### Phase 2: Streaming & Incremental Analytics (Status: In Progress 🚧)
- [I] CEP full engine implementation in `analytics/cep_engine.cpp` (Target: Q2 2026) (Issue: #1489)
- [I] Streaming aggregation windows (tumbling/sliding/session/hopping) in `analytics/streaming_window.cpp` (Target: Q2 2026) (Issue: #1490)
- [ ] Incremental materialized views (Target: Q3 2026)

### Phase 3: Distributed & ML-Augmented Analytics (Status: Planned 📋)
- [I] Columnar execution engine with vectorized operator pipeline (Issue: #1481)
- [I] LLVM-JIT compilation for hot aggregation paths (Issue: #1482)
- [I] Distributed analytics sharding across cluster nodes (Issue: #1483)
- [I] Predictive analytics and time-series forecasting integration (Issue: #1484)
- [ ] AutoML integration for automated model selection

## Production Readiness Checklist
- [x] Unit tests (OLAP, Arrow export, process mining, NLP, diff engine)
- [I] Unit tests coverage > 80% (Issue: #1486)
- [x] Integration tests (query module, index module, CDC)
- [I] CEP engine integration tests (Issue: #1487)
- [x] Performance benchmarks (OLAP, export, process mining, graph, NLP)
- [I] Security audit (LLM API key handling, data export sanitization) (Issue: #1488)
- [x] Documentation complete (API docs, OLAP guide, process mining guide)
- [x] API stability guaranteed for OLAP, export, and process mining

## Known Issues & Limitations
- CEP engine is a header-only stub; no event processing is performed until full implementation lands
- NLP text analyzer uses rule-based approaches — not suitable as a replacement for full NLP frameworks
- LLM analyzer requires external API keys; responses are non-deterministic
- Arrow-dependent formats (Parquet, Feather, IPC) require compile-time flag `THEMIS_HAS_ARROW`
- Basic lemmatization only; full morphological analysis not yet supported
- Graph analytics limited to PageRank and basic algorithms; advanced algorithms planned for v1.8.0

## Breaking Changes
- CEP engine API will be finalized in v1.7.0; current stubs are not stable
- Arrow export format options may expand in v1.7.0 (additive, non-breaking)
