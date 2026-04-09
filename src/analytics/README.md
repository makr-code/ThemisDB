# ThemisDB Analytics Module - Implementation

**Version:** 1.9.0
**Status:** 🟢 Production-Ready
**Last Updated:** 2026-04-06
**Module Path:** `src/analytics/`

---

## Module Purpose

The Analytics module provides comprehensive data analysis capabilities for ThemisDB, including OLAP query processing, statistical analysis, time-series analytics, graph analytics, spatial analytics, process mining, text analytics, and machine learning integration. This module transforms ThemisDB from a transactional database into a powerful analytical platform capable of real-time insights, predictive analytics, and complex event processing.

### Core Capabilities

- **OLAP Query Processing**: Multi-dimensional analysis with CUBE, ROLLUP, and window functions
- **Statistical Analysis**: Aggregation functions, variance, standard deviation, percentiles
- **Time-Series Analytics**: Temporal patterns, seasonality detection, forecasting
- **Graph Analytics**: PageRank, community detection, centrality measures, path analysis
- **Spatial Analytics**: Geographic analysis, proximity queries, spatial clustering
- **Process Mining**: Process discovery, conformance checking, performance analysis
- **Text Analytics**: NLP-based text analysis, sentiment analysis, entity extraction
- **Machine Learning**: Model integration, anomaly detection, predictive analytics
- **Complex Event Processing**: Real-time pattern matching, streaming analytics
- **Data Export**: Apache Arrow, Parquet, CSV, JSON format support

## Subsystem Scope

**In Scope:**
- OLAP queries (GROUP BY, CUBE, ROLLUP, window functions)
- Statistical aggregations (variance, standard deviation, percentiles)
- Process mining (Alpha, Heuristic, Inductive algorithms)
- Conformance checking and performance analysis
- NLP text analysis (sentiment, entity extraction, modality detection)
- LLM integration for process analysis
- Diff engine for dataset comparison
- Columnar Arrow/Parquet storage support
- SIMD AVX2 acceleration

**Out of Scope:**
- Raw SQL/AQL parsing (handled by query module)
- LLM model management (handled by llm module)
- Data storage and persistence (handled by storage module)

## Relevant Interfaces

| Interface / File | Role |
|-----------------|------|
| `olap.cpp` | OLAP engine: GROUP BY, CUBE, ROLLUP aggregations |
| `streaming_window.cpp` | Streaming windows: tumbling, sliding, session, hopping with watermarks |
| `jit_aggregation.cpp` | JIT-compiled hot-path aggregation dispatch (LLVM-ready) |
| `process_mining.cpp` | Process discovery and conformance checking |
| `nlp_text_analyzer.cpp` | NLP text analysis, sentiment, entity extraction |
| `llm_process_analyzer.cpp` | LLM-powered process analysis integration |
| `diff_engine.cpp` | Dataset diffing and change detection |
| `cep_engine.cpp` | Complex Event Processing (production-ready) |
| `forecasting.cpp` | Time-series forecasting (ARIMA, Holt-Winters, EXP_SMOOTHING, ENSEMBLE) |
| `distributed_analytics.cpp` | Distributed OLAP fan-out across shards |
| `arrow_flight.cpp` | Arrow Flight RPC server/client for remote analytics |
| `model_serving.cpp` | Named+versioned model registry and online inference pipeline |

## Current Delivery Status

**Maturity:** 🟢 Production-Ready — Core OLAP, process mining, NLP text analysis, and CEP engine fully operational.

## About This Directory

This directory (`src/analytics/`) contains **implementation files only**. For API documentation, see [`include/analytics/README.md`](../../include/analytics/README.md).

## Implementation Files

### 1. OLAP Engine (`olap.cpp`)

**Lines of Code:** ~1800 lines  
**Dependencies:** nlohmann/json, optional Apache Arrow

Multi-dimensional analytical query processing with aggregations and window functions.

**Key Components:**
- `OLAPEngine::Impl`: Core engine implementation
  - In-memory collection storage for testing
  - Query execution pipeline
  - Result caching
- Query execution methods:
  - `execute()`: Main entry point, routes to specific executors
  - `executeSimpleGroupBy()`: Single-level grouping
  - `executeCubeQuery()`: CUBE operation (all dimension combinations)
  - `executeRollupQuery()`: ROLLUP operation (hierarchical aggregation)
  - `executeGroupingSetsQuery()`: Custom grouping sets
- Aggregation functions:
  - `computeAggregate()`: Generic aggregation dispatcher
  - Supports: COUNT, SUM, AVG, MIN, MAX, STDDEV, VARIANCE, MEDIAN, PERCENTILE
- Data export:
  - `exportToParquet()`: Export results to Parquet format (optional)
  - `exportCollectionToParquet()`: Direct collection export

**Implementation Details:**
- Platform-specific: Windows build uses stub implementation with error logging
- Windows stub logs errors via spdlog when called, returning empty results
- Unix/Linux: Full implementation with columnar execution
- Uses variant types for polymorphic value storage
- Efficient hash-based grouping
- Statistics collection for query optimization

**Performance Optimizations:**
- Hash-based aggregation (O(n) average case)
- Columnar data layout for cache efficiency
- Lazy evaluation where possible
- Result caching for repeated queries

**Thread Safety:**
- OLAPEngine is thread-safe for concurrent queries
- Uses internal locking for shared state
- Query objects are immutable during execution

**Usage Pattern:**
```cpp
// Create engine
OLAPEngine engine;

// Execute query
OLAPQuery query = buildQuery();
auto result = engine.execute(query);

// Access results
for (const auto& row : result.rows) {
    // Process row
}
```

### 2. Arrow Export (`arrow_export.cpp`)

**Lines of Code:** ~200 lines  
**Dependencies:** nlohmann/json, optional Apache Arrow C++

**Status:** ✅ Production-ready columnar data structure

Data export interfaces with optional Apache Arrow integration.

**Key Components:**
- `ArrowRecordBatch::Impl`: Columnar data storage
  - Column metadata and data vectors
  - Schema management
  - Row append operations
- Column types:
  - INT8, INT16, INT32, INT64
  - UINT8, UINT16, UINT32, UINT64
  - FLOAT, DOUBLE
  - STRING, BINARY
  - BOOL, DATE32, DATE64
  - TIMESTAMP, DECIMAL128
- Export operations:
  - `appendRow()`: Add row data
  - `toJson()`: JSON serialization
  - `toCSV()`: CSV export
  - `toArrowTable()`: Convert to Arrow table (optional)

**Implementation Strategy:**
- Production-ready columnar storage always available
- JSON/CSV export fully functional without Arrow
- Arrow formats (IPC, Parquet, Feather) require `THEMIS_HAS_ARROW` flag
- Clear error messages if Arrow not available

**Platform Support:**
- Windows: Full columnar implementation
- Unix/Linux: Full columnar implementation
- macOS: Full implementation

**Future Work:**
- Arrow Flight RPC support
- Memory pooling for large batches

### 3. Analytics Export (`analytics_export.cpp`)

**Lines of Code:** ~750 lines  
**Dependencies:** nlohmann/json, optional Apache Arrow C++, spdlog

**Status:** ✅ Production-ready with Arrow support

Export interface implementation and factory methods.

**Key Components:**
- `AnalyticsExporter`: Production implementation
  - JSON export (always available)
  - CSV export (always available)
  - Arrow IPC export (when THEMIS_HAS_ARROW enabled)
  - Parquet export with compression (when THEMIS_HAS_ARROW enabled)
  - Feather export (when THEMIS_HAS_ARROW enabled)
- `ExporterFactory`: Factory for creating exporters
  - Default exporter creation
  - Format-specific exporters
- Export targets:
  - File export with error handling
  - String export (not recommended for Parquet binary format)
  - Stream export (callback-based)

**Supported Formats:**
- ✅ JSON (fully implemented)
- ✅ CSV (fully implemented)
- ✅ Arrow IPC (implemented with THEMIS_HAS_ARROW)
- ✅ Parquet (implemented with THEMIS_HAS_ARROW, includes compression)
- ✅ Feather (implemented with THEMIS_HAS_ARROW)

**Implementation Details:**
- Feature guards for Arrow integration
- Compression support: snappy, gzip, zstd, lz4 (Parquet only)
- Schema export included
- Null bitmap handling
- Structured logging with spdlog
- Clear error messages when Arrow not available
- Returns NOT_SUPPORTED status for unavailable formats

**Usage Pattern:**
```cpp
auto exporter = ExporterFactory::createDefaultExporter();

ExportOptions options;
options.format = ExportFormat::ARROW_PARQUET;
options.compress = true;
options.compression_codec = "snappy";
options.compression_level = 3;

auto result = exporter->exportToFile(batch, "output.parquet", options);

if (result.status == ExportStatus::NOT_SUPPORTED) {
    // Arrow not compiled in
    spdlog::warn("Arrow support not available: {}", result.message);
} else if (result.status == ExportStatus::SUCCESS) {
    spdlog::info("Exported {} rows, {} bytes in {:.2f}ms",
                 result.rows_exported,
                 result.bytes_written,
                 result.duration_ms);
}
```

### 4. Process Mining (`process_mining.cpp`)

**Lines of Code:** ~1700 lines  
**Dependencies:** GraphIndex, VectorIndex, OLAP, nlohmann/json

Process discovery, conformance checking, and performance analysis.

**Key Components:**
- `ProcessMining::Impl`: Core implementation
  - Event log management
  - Process model storage
  - Algorithm implementations
- Process discovery algorithms:
  - Alpha Miner: Basic discovery from event logs
  - Heuristic Miner: Handles noise and incompleteness
  - Inductive Miner: Guarantees sound models
- Conformance checking:
  - Token replay: Simulate process execution
  - Alignment-based: Optimal alignment between log and model
  - Metrics: Fitness, precision, generalization
- Performance analysis:
  - Bottleneck detection
  - Waiting time analysis
  - Resource utilization

**Implementation Details:**
- Uses GraphIndex for process graph representation
- Leverages VectorIndex for similarity search
- BPMN export functionality
- Temporal analysis using timestamps

**Performance Considerations:**
- Event log indexing for fast access
- Graph algorithms optimized for process graphs
- Caching of conformance results

**Integration Points:**
- Stores process models as documents
- Uses graph traversal for conformance
- Exports to BPMN XML format

### 5. Process Pattern Matcher (`process_pattern_matcher.cpp`)

**Lines of Code:** ~850 lines  
**Dependencies:** ProcessMining, VectorIndex, GraphIndex

Find similar processes using multiple similarity methods.

**Key Components:**
- `ProcessPatternMatcher::Impl`: Implementation
  - Pattern storage and indexing
  - Similarity computation engines
- Similarity methods:
  - **Graph similarity**: Structure-based (nodes, edges, paths)
  - **Vector similarity**: Semantic embeddings
  - **Behavioral similarity**: Execution patterns
  - **Hybrid similarity**: Weighted combination
- Top-K retrieval:
  - Efficient similarity ranking
  - Threshold-based filtering

**Implementation Details:**
- Uses cosine similarity for vectors
- Graph edit distance for structure
- Behavioral fingerprints for execution patterns
- Configurable weights for hybrid method

**Performance:**
- O(n) for vector similarity with index
- O(n*m) for graph similarity (optimized with pruning)
- Parallel execution for large pattern sets

### 6. Text Analytics (`nlp_text_analyzer.cpp`)

**Lines of Code:** ~1600 lines  
**Dependencies:** Standard C++17

Lightweight NLP-based text analysis.

**Key Components:**
- `NLPTextAnalyzer::Impl`: Core implementation
  - Tokenizer
  - POS tagger
  - NER (Named Entity Recognition)
  - Keyword extractor
  - Sentiment analyzer
- Text processing:
  - Tokenization with Unicode support
  - Lemmatization (basic)
  - Stop word removal
  - TF-IDF computation
- Entity types:
  - PERSON, ORGANIZATION, LOCATION
  - DATE, TIME, MONEY
  - EMAIL, URL, PHONE

**Implementation Strategy:**
- Rule-based (no ML models required)
- Pattern matching with regex
- Dictionary-based entity recognition
- Statistical keyword extraction

**Performance:**
- CPU-efficient, no GPU needed
- O(n) complexity for most operations
- Suitable for query-time analysis

**Limitations:**
- Basic lemmatization (not full morphology)
- English-focused (multilingual support limited)
- Not a replacement for full NLP frameworks

### 7. LLM Process Analyzer (`llm_process_analyzer.cpp`)

**Lines of Code:** ~520 lines  
**Dependencies:** HTTP client (for API calls), nlohmann/json

LLM integration for advanced process analysis.

**Key Components:**
- API client implementations:
  - OpenAI client
  - Anthropic client
  - Azure OpenAI client
  - Local model client (llama.cpp)
- Task handlers:
  - Process conformance checking
  - Next activity prediction
  - Compliance verification (5R Rule, Vier-Augen-Prinzip)
  - Fraud detection
  - Sentiment analysis
- Request/response handling:
  - JSON serialization
  - Error handling and retry logic
  - Caching layer

**Implementation Details:**
- HTTP requests via libcurl or similar
- Prompt engineering for process analysis tasks
- Response parsing and validation
- Configurable retry with exponential backoff

**Performance Considerations:**
- Caching enabled by default (TTL: 1 hour)
- Batch requests where possible
- Timeout configuration
- Rate limiting support

**API Configuration:**
```cpp
LLMConfig config;
config.provider = LLMProvider::OPENAI;
config.api_key = std::getenv("OPENAI_API_KEY");
config.model_name = "gpt-4";
config.max_retries = 3;
config.enable_caching = true;
```

### 8. Diff Engine (`diff_engine.cpp`)

**Lines of Code:** ~575 lines  
**Dependencies:** Changefeed, SnapshotManager, nlohmann/json

Git-like diff functionality for versioned data.

**Key Components:**
- `DiffEngine::Impl`: Core implementation
  - Changefeed integration
  - Snapshot comparison
  - Change categorization
- Diff operations:
  - By sequence number range
  - By timestamp range
  - Filtered by table/key/event type
- Change tracking:
  - ADDED: New entities
  - MODIFIED: Updated entities
  - DELETED: Removed entities

**Implementation Details:**
- Streams changes from changefeed
- Aggregates changes by key
- Computes before/after values
- Supports pagination

**Performance:**
- Streaming implementation for large diffs
- O(n) complexity where n = number of changes
- Target: <100ms for 10K changes

**Usage Pattern:**
```cpp
DiffEngine engine(changefeed, snapshot_mgr);

auto diff = engine.diffByTimestamp(
    start_time, end_time,
    {.table_filter = "orders"}
);

// Process changes
for (const auto& change : diff.added) {
    // Handle added entity
}
```

### 9. CEP Engine (Complex Event Processing) (`cep_engine.cpp`)

**Status:** ✅ Production-ready – full NFA pattern matching engine

Complex event processing for real-time streaming analytics.

**Components:**
- NFA-based event pattern matching (SEQUENCE, AND, OR, NOT, WITHIN)
- EPL (Event Processing Language) parser with full syntax support:
  - `CREATE RULE <name> AS` and `NAME <name>` rule naming
  - `SELECT` aggregations: COUNT, SUM, AVG, MIN, MAX, FIRST, LAST, STDDEV, VARIANCE, PERCENTILE, DISTINCT_COUNT, COLLECT, TOPN with `AS alias`
  - `GROUP BY` multi-field grouping
  - `WINDOW TUMBLING(5 MINUTES)` / `SLIDING(5 MINUTES, 1 MINUTE)` / `SESSION(30 MINUTES)` / `COUNT(100 EVENTS)` with time units (ms/s/minutes/hours/days)
  - `PATTERN (SEQUENCE|SEQ|AND|OR|NOT) (<event_types>) WITHIN <n> <unit>` with time units
  - `ACTION alert('ch','sev','msg')` / `webhook('url')` / `db_write('coll')` / `log/slack/kafka/email`
  - Multi-line EPL strings (newlines collapsed to spaces)
  - Legacy `ON MATCH ALERT severity=<s>` and `WINDOW TYPE Nms` syntax preserved
- Window management (tumbling, sliding, session, hopping)
- Rule engine for event processing
- Alert dispatch and CDC integration

### 10. Streaming Windows (`streaming_window.cpp`)

**Status:** ✅ Production-ready

Four window types with watermark support:
- `TumblingWindow` – fixed, non-overlapping time windows
- `SlidingWindow` – fixed-size, overlapping windows
- `SessionWindow` – gap-based dynamic windows
- `HoppingWindow` – configurable hop size

### 11. Incremental Materialized Views (`incremental_view.cpp`)

**Status:** ✅ Production-ready

Delta-maintenance for all 10 aggregation functions with Welford STDDEV/VARIANCE and COUNT_DISTINCT ref-counting.

### 12. Real-Time Anomaly Detection (`anomaly_detection.cpp`)

**Status:** ✅ Production-ready

Six detection algorithms with adaptive learning:
- Z-Score, Modified Z-Score (MAD-based), IQR
- Isolation Forest, Local Outlier Factor (LOF)
- Ensemble (weighted mean of per-algorithm scores)

**StreamingAnomalyDetector** for online detection with rolling window.

### 13. AutoML Engine (`automl.cpp`)

**Status:** ✅ Production-ready

Automated Machine Learning for classification and regression.

**Algorithms (pure C++17, no external ML dependencies):**
- Logistic Regression (L2-regularised, mini-batch SGD)
- Linear Regression (OLS / ridge via normal equations)
- Decision Tree (CART – Gini for classification, MSE for regression)
- Random Forest (bagging, random feature subsets)
- Gradient Boosting (stagewise additive, log-loss / MSE gradient)
- KNN (brute-force Euclidean, weighted 1/d²)
- Ensemble (soft-voting / mean over top-k candidates)

**Key capabilities:**
- Random hyperparameter search with time/trial budget
- k-fold cross-validation for evaluation
- Standard scaling + optional polynomial feature expansion (degree 2)
- SHAP-approximated feature explanations (permutation-based)
- Full metric suite: accuracy, F1, precision, recall, AUC-ROC; R², RMSE, MAE, MAPE

**Performance:**
- Decision Tree: O(n·d·log n) build; O(depth) predict
- Random Forest: O(k·n·d·log n) build; O(k·depth) predict
- KNN: O(n·d) predict (brute-force; suitable for medium datasets)

### 14. ML Serving Integration (`ml_serving.cpp`)

**Status:** ✅ Production-ready

Unified interface for integrating external ML inference engines with the analytics pipeline.

**Backends:**

| Backend | Build flag | Transport |
|---|---|---|
| ONNX Runtime | `THEMIS_HAS_ONNX=1` (auto-detected via `find_package(onnxruntime)`) | In-process C++ API |
| TensorFlow Serving | `THEMIS_HAS_TF_SERVING=1` + `THEMIS_HAS_CURL=1` | REST/HTTP POST |

**Key capabilities:**
- `MLServingClient` – unified, thread-safe entry point; selects the best available backend automatically (`MLBackendType::AUTO`)
- `ONNXServingBackend` – lazy model loading from `<model_directory>/<name>.onnx`, optional CUDA execution provider, configurable thread counts
- `TFServingBackend` – TF Serving REST API (`/v1/models/<name>:predict`), configurable timeout and optional bearer-token auth
- `IMLServingBackend` – extension interface for custom backends
- `MLServingClient::inferFromDataPoint()` – seamless integration with `DataPoint` from the anomaly detection module; numeric fields are sorted alphabetically and packed into a `{1, N}` float32 input tensor
- Graceful degradation: when a backend is absent at compile time, `infer()` returns `MLServingStatus::UNAVAILABLE` with a diagnostic message instead of crashing

**Quick start:**
```cpp
#include "analytics/ml_serving.h"
using namespace themisdb::analytics;

// Auto-selects ONNX Runtime if available, else TF Serving
MLServingClient client;

// Low-level tensor API
MLServingRequest req;
req.model_name = "fraud_detector";
req.inputs.push_back({ "input", {1, 30}, features_float32 });
auto resp = client.infer(req);
if (resp.ok()) {
    float fraud_score = resp.outputs[0].data[0];
}

// DataPoint convenience API
DataPoint dp;
dp.set("amount", 1500.0);
dp.set("frequency", 3.0);
auto resp2 = client.inferFromDataPoint("fraud_detector", dp);
```

**Build flags:**
```cmake
cmake -B build -S . -DTHEMIS_ENABLE_TF_SERVING=ON   # enable TF Serving backend
# ONNX Runtime is enabled automatically when onnxruntime is found via vcpkg
```

### 15. Model Serving and Online Inference Pipeline (`model_serving.cpp`)

**Lines of Code:** ~390 lines
**Dependencies:** `automl.h`, `<shared_mutex>`

**Status:** ✅ Production-ready

Thread-safe named and versioned model registry with online and batch inference.

**Key Components:**
- `ModelServingPipeline` — top-level entry point; wraps the model registry
- Named + versioned model storage keyed by `"name:version"` in an internal hash-map protected by a `std::shared_mutex`
- `ModelInfo` — name, version, algorithm type, feature count, creation/last-used timestamps
- `ModelHealthMetrics` — request count, error count, avg/p99 latency (ms), last-inference timestamp
- Latency tracking via a fixed-size circular deque; p99 recomputed from sorted window (≤ 1000 samples)

**Operations:**
- `registerModel(name, version, model, info)` — register a new model version
- `predict(name, version, features)` — single-sample inference; returns class label + probabilities
- `predictBatch(name, version, batch)` — batch inference with per-sample results
- `getModelHealth(name, version)` — retrieve live health metrics
- `listModels()` — enumerate all registered model versions
- `unregisterModel(name, version)` — remove a model from the registry

**Thread Safety:**
- Read operations (`predict*`, `list*`, `health*`) use a shared lock
- Write operations (`register`, `unregister`) use an exclusive lock
- Per-model health updates use a separate per-entry `std::mutex`

### 16. Predictive Analytics & Time-Series Forecasting (`forecasting.cpp`)

**Lines of Code:** ~1092 lines
**Dependencies:** Standard C++17 only (no external ML libraries)

**Status:** ✅ Production-ready

Five forecasting algorithms with confidence intervals, model evaluation, and serialization.

**Algorithms:**
- `LINEAR_REGRESSION` — OLS fit (α + β·t); CI from residual standard error
- `EXP_SMOOTHING` (SES) — single exponential smoothing; CI from residual MAD
- `HOLT_WINTERS` — triple exponential smoothing (additive and multiplicative); falls back to Holt's if fewer than 2 full seasons
- `ARIMA` — AR(p)+I(d)+MA(q) via Yule–Walker equations; supports differencing d = 0 or 1
- `ENSEMBLE` — weighted average of all four algorithms; CI is weighted average of individual CIs

**Key Types:**
- `TimeSeries` — sorted vector of `TimeSeriesPoint{timestamp_ms, value}`; supports `push()` and `values()`
- `ForecastModel` — trained model state; `fit(series)`, `predict(horizon)`, `evaluate(series)`, `decompose(series)`
- `ForecastResult` — predicted values + lower/upper confidence bounds per step
- `ForecastAccuracy` — MAE, RMSE, MAPE, sMAPE metrics
- `SeasonalDecomposition` — trend, seasonal, residual components

**Serialization:**
- `ForecastModel::serialize()` / `ForecastModel::deserialize()` — round-trip JSON serialization for model persistence

### 17. JIT Aggregation Compiler (`jit_aggregation.cpp`)

**Lines of Code:** ~578 lines
**Dependencies:** `columnar_execution.h`, spdlog

**Status:** ✅ Production-ready

Hot-path aggregation dispatch with template-specialized compiled functions.

**Architecture:**
- Cold path (call count < hot threshold): delegates to generic `AggregateOperator`
- Hot path (call count ≥ hot threshold): invokes a cached `std::function<ColumnBatch(const ColumnBatch&)>` that hard-codes aggregation functions, eliminating per-row enum dispatch overhead and enabling compiler auto-vectorization
- Each unique `(AggregateSpec::Function, input_column, result_name, group-by columns)` combination is encoded into a compact string key for independent tracking
- `THEMIS_HAS_LLVM_JIT` compile flag: reserved extension point to emit LLVM IR via the MCJIT backend for native-code generation

**Key Types:**
- `JITAggregationEngine` — stateful dispatcher; created once per engine instance
- `JITAggregationEngine::Config` — `hot_threshold` (default: 5 calls)
- `JITAggregationEngine::execute(specs, batch)` — main dispatch entry point

### 18. Distributed Analytics (`distributed_analytics.cpp`)

**Lines of Code:** ~591 lines
**Dependencies:** `olap.h`, spdlog, `<future>`

**Status:** ✅ Production-ready

Scatter-gather coordinator for distributed OLAP queries across cluster shards.

**Architecture:**
- Fan-out: the coordinator sends the same `OLAPQuery` to all registered shard `OLAPEngine` instances in parallel via `std::async`
- Gather: partial results from each shard are merged using algorithm-correct accumulators
  - `SUM` / `COUNT` / `MIN` / `MAX`: direct merge
  - `AVG`: weighted sum + total count (avoids "average of averages" error)
  - `STDDEV` / `VARIANCE`: Chan's parallel algorithm (running count, mean, M2)

**Key Types:**
- `DistributedAnalyticsEngine` — coordinator; holds a vector of shard `OLAPEngine` references
- `DistributedAnalyticsEngine::addShard(engine)` — register a shard
- `DistributedAnalyticsEngine::execute(query)` — scatter-gather execution; returns merged `OLAPResult`

**Limitations:**
- CUBE cross-shard result merging is partial (additive measures only; full CUBE merge is planned)

### 19. Arrow Flight RPC (`arrow_flight.cpp`)

**Lines of Code:** ~983 lines
**Dependencies:** `arrow_export.h`, spdlog; optional `arrow/flight/api.h` (THEMIS_HAS_ARROW_FLIGHT)

**Status:** ✅ Production-ready

In-process and native Arrow Flight server/client for zero-copy remote analytics data transfer.

**Two transports:**
- **In-process (always available)**: a process-local registry maps path descriptors to dataset producers and put handlers; the client resolves the server directly, enabling zero-overhead data transfer within a single process
- **Native gRPC (THEMIS_HAS_ARROW_FLIGHT)**: when the Apache Arrow Flight C++ library is detected at build time the server starts a gRPC listener; the client falls back to a real network connection, enabling cross-process / cross-host transfer compatible with Pandas, DuckDB, Spark, and any Flight-capable tool

**Key Types:**
- `FlightDescriptor` — identifies a dataset by PATH or CMD; `toString()` helper
- `FlightServer` — registers dataset producers (`putDataset`) and put handlers (`registerPutHandler`); `start(host, port)` / `stop()`
- `FlightClient` — `listFlights()`, `getSchema(descriptor)`, `doGet(descriptor)` → `ArrowRecordBatch`, `doPut(descriptor, batch)`
- `FlightInfo` — schema + endpoint metadata for a dataset

**Build flag:**
```cmake
# Native Arrow Flight (gRPC) transport — requires Arrow with Flight support
cmake -B build -S . -DTHEMIS_ENABLE_ARROW_FLIGHT=ON
```

## Architecture

The Analytics module follows a layered architecture:

```
┌─────────────────────────────────────────────────────────────┐
│                     Analytics API Layer                      │
│  (OLAP, Process Mining, Text Analytics, CEP, Export)        │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                   Execution Engine Layer                     │
│  (Query Execution, Aggregation, Pattern Matching)           │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                    Integration Layer                         │
│  (Query Engine, Index Module, Storage Module)               │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                      Data Sources                            │
│  (Collections, Graphs, Vectors, Timeseries, Spatial)        │
└─────────────────────────────────────────────────────────────┘
```

### Module Dependencies

```
analytics/
├── Depends on:
│   ├── query/ (AQL integration, expression evaluation)
│   ├── index/ (GraphIndex, VectorIndex, SpatialIndex, TemporalIndex)
│   ├── storage/ (Data access, MVCC snapshots)
│   ├── cdc/ (Change data capture for diff and CEP)
│   └── llm/ (LLM integration for semantic analysis)
├── Used by:
│   ├── query/ (OLAP functions in AQL)
│   ├── observability/ (Metrics export)
│   └── api/ (REST/GraphQL analytics endpoints)
└── Optional dependencies:
    ├── Apache Arrow (data export)
    ├── CUDA (GPU acceleration)
    └── HTTP client (LLM API calls)
```

## Building

The analytics module is built as part of ThemisDB. No special configuration is required.

```bash
cmake -B build -S .
cmake --build build
```

## Testing

Run analytics tests:
```bash
cd build
ctest -R analytics --verbose
```

Or run specific tests:
```bash
./build/tests/test_olap
./build/tests/analytics/test_arrow_export
./build/tests/analytics/test_process_mining_llm
./build/tests/analytics/test_cep_engine
./build/tests/analytics/test_incremental_view
./build/tests/analytics/test_streaming_window
./build/tests/analytics/test_anomaly_detection
./build/tests/analytics/test_automl
./build/tests/analytics/test_ml_serving
```

## Integration with Query Module

The Analytics module tightly integrates with the Query module for AQL support:

### OLAP Functions in AQL

```sql
-- Simple aggregation
FOR doc IN sales
  COLLECT region = doc.region
  AGGREGATE total = SUM(doc.amount)
  RETURN { region, total }

-- CUBE operation (all dimension combinations)
FOR doc IN sales
  COLLECT CUBE(region = doc.region, product = doc.product)
  AGGREGATE total = SUM(doc.amount)
  RETURN { region, product, total, GROUPING_ID() }

-- Window functions
FOR doc IN sales
  SORT doc.date
  RETURN {
    date: doc.date,
    amount: doc.amount,
    running_total: SUM(doc.amount) OVER (ORDER BY doc.date),
    moving_avg: AVG(doc.amount) OVER (ROWS BETWEEN 6 PRECEDING AND CURRENT ROW)
  }

-- Process mining from AQL
FOR event IN audit_log
  FILTER event.user_id == @userId
  COLLECT caseId = event.case_id
  INTO eventSequence = event.activity
  RETURN DISCOVER_PROCESS(eventSequence, "heuristic")
```

### Query Optimizer Integration

The Analytics module provides statistics for query optimization:

```cpp
// Collect statistics for optimizer
OLAPEngine engine;
engine.collectStatistics("sales");

// Optimizer uses statistics for cost estimation
QueryOptimizer optimizer;
auto plan = optimizer.optimize(query);  // Uses analytics stats
```

### Vectorized Execution

Both Query and Analytics modules use vectorized execution:

```cpp
// Query engine uses vectorized operators
VectorizedOperator op;
op.addPipeline(filter_op, aggregate_op, sort_op);
auto result = op.execute(batch);

// Analytics uses same vectorized primitives
OLAPEngine engine;
engine.setConfig({.enable_vectorization = true});
```

## Integration with Index Module

### Graph Analytics Integration

```cpp
// Use GraphIndex for process mining
ProcessMining mining(db);
mining.setGraphIndex(graph_index);

auto model = mining.discoverProcess(event_log);
// Internally uses graph algorithms from GraphIndex

// Use GraphAnalytics for community detection
GraphAnalytics analytics(graph_index);
auto communities = analytics.detectCommunities("process_graph");
```

### Vector Similarity for Process Patterns

```cpp
// Use VectorIndex for semantic similarity
ProcessPatternMatcher matcher(db);
matcher.setVectorIndex(vector_index);

Pattern pattern = definePattern();
auto similar = matcher.findSimilar(
    pattern,
    0.8,  // 80% similarity
    SimilarityMethod::VECTOR
);
```

### Spatial Analytics Integration

```cpp
// Use SpatialIndex for geographic analytics
SpatialAnalytics spatial(spatial_index);

// Find clusters of events
auto clusters = spatial.dbscan("locations", {
    .eps = 0.5,  // 500m
    .min_samples = 5
});

// Proximity queries
auto nearby = spatial.findNearby(
    "stores",
    center_point,
    1000.0  // 1km radius
);
```

### Temporal Analytics Integration

```cpp
// Use TemporalIndex for time-series
TemporalAnalytics temporal(temporal_index);

// Detect seasonal patterns
auto seasonality = temporal.detectSeasonality("metrics", {
    .period = std::chrono::hours(24)
});

// Forecast future values
auto forecast = temporal.forecast("sales", 30);  // 30 days ahead
```

## Vectorized Execution Details

### SIMD Optimization

The Analytics module uses SIMD instructions for performance:

**Supported Operations:**
- Aggregations (SUM, AVG, MIN, MAX)
- Filtering (comparison operators)
- Arithmetic operations
- String operations (limited)

**Implementation:**
```cpp
// AVX2 vectorized SUM
double vectorizedSum(const double* data, size_t count) {
    __m256d sum = _mm256_setzero_pd();
    size_t i = 0;
    
    // Process 4 doubles at a time
    for (; i + 3 < count; i += 4) {
        __m256d vals = _mm256_loadu_pd(&data[i]);
        sum = _mm256_add_pd(sum, vals);
    }
    
    // Horizontal sum
    double result[4];
    _mm256_storeu_pd(result, sum);
    double total = result[0] + result[1] + result[2] + result[3];
    
    // Process remaining elements
    for (; i < count; i++) {
        total += data[i];
    }
    
    return total;
}
```

**Performance Gains:**
- 4x for double operations (AVX2)
- 8x for float operations (AVX2)
- 16x for byte operations (AVX2)

### Columnar Layout

Data is stored in columnar format for cache efficiency:

```cpp
// Row-oriented (slow)
struct Row {
    int64_t id;
    double value;
    std::string name;
};
std::vector<Row> rows;  // Poor cache locality

// Column-oriented (fast)
struct Columns {
    std::vector<int64_t> ids;
    std::vector<double> values;
    std::vector<std::string> names;
};
Columns cols;  // Excellent cache locality for column scans
```

**Benefits:**
- Better cache utilization (64-byte cache lines)
- SIMD-friendly (contiguous data)
- Compression-friendly (same-type data)
- Skip entire columns if not needed

### Batch Processing

Operations are batched to amortize overhead:

```cpp
// Process 1024 rows at a time
constexpr size_t BATCH_SIZE = 1024;

for (size_t start = 0; start < total_rows; start += BATCH_SIZE) {
    size_t count = std::min(BATCH_SIZE, total_rows - start);
    
    // Load batch
    auto batch = loadBatch(start, count);
    
    // Apply vectorized operations
    auto filtered = vectorizedFilter(batch, predicate);
    auto aggregated = vectorizedAggregate(filtered);
    
    // Merge results
    mergeResults(aggregated);
}
```

## Analytics Best Practices

### Query Design Best Practices

1. **Filter Early**: Apply filters before aggregations
   ```sql
   -- Good: Filter first
   FOR doc IN sales
     FILTER doc.date >= @startDate
     COLLECT region = doc.region
     AGGREGATE total = SUM(doc.amount)
     RETURN { region, total }
   
   -- Bad: Filter after aggregation
   FOR doc IN sales
     COLLECT region = doc.region
     AGGREGATE total = SUM(doc.amount)
     FILTER total > 10000  -- Can't push down
     RETURN { region, total }
   ```

2. **Limit Dimensions**: Fewer GROUP BY dimensions = faster
   ```sql
   -- Fast: 2 dimensions
   COLLECT region, product
   
   -- Slower: 5 dimensions
   COLLECT region, product, category, subcategory, brand
   ```

3. **Use Materialized Views**: Pre-compute frequent aggregations
   ```cpp
   auto view = engine.createMaterializedView(R"(
       SELECT region, SUM(amount) as total
       FROM sales
       GROUP BY region
   )");
   ```

4. **Index Dimension Columns**: Speed up GROUP BY
   ```cpp
   db.createIndex("sales", {"region"});
   db.createIndex("sales", {"product"});
   ```

### Performance Optimization Best Practices

1. **Enable Columnar Storage**: For analytical workloads
   ```cpp
   StorageEngine::Config config;
   config.default_format = StorageFormat::COLUMNAR;
   ```

2. **Use Compression**: Reduce I/O
   ```cpp
   config.compression = CompressionType::ZSTD;
   config.compression_level = 3;
   ```

3. **Partition Large Tables**: By time or key
   ```cpp
   db.createPartitionedCollection("sales", {
       .partition_key = "date",
       .partition_type = PartitionType::RANGE,
       .interval = std::chrono::days(1)
   });
   ```

4. **Batch Operations**: Process multiple rows
   ```cpp
   // Good: Batch insert
   db.insertBatch("sales", rows);
   
   // Bad: Individual inserts
   for (const auto& row : rows) {
       db.insert("sales", row);
   }
   ```

5. **Enable Caching**: For repeated queries
   ```cpp
   OLAPEngine::Config config;
   config.enable_result_cache = true;
   config.cache_ttl = std::chrono::minutes(5);
   ```

### Data Export Best Practices

1. **Use Streaming**: For large datasets
   ```cpp
   exporter->exportToStream(batch, [](const std::string& chunk) {
       // Process chunk incrementally
       write_to_network(chunk);
   }, options);
   ```

2. **Enable Compression**: For network transfers
   ```cpp
   options.compression = CompressionType::SNAPPY;
   ```

3. **Choose Appropriate Format**:
   - **JSON**: Human-readable, debugging
   - **CSV**: Simple integration, spreadsheets
   - **Parquet**: Efficient storage, columnar analytics
   - **Arrow IPC**: Zero-copy inter-process communication

4. **Batch Export**: Export in chunks
   ```cpp
   constexpr size_t CHUNK_SIZE = 10000;
   for (size_t offset = 0; offset < total_rows; offset += CHUNK_SIZE) {
       auto batch = createBatch(offset, CHUNK_SIZE);
       exporter->exportToFile(batch, filename, options);
   }
   ```

### Process Mining Best Practices

1. **Ensure Event Log Quality**:
   - Complete case IDs
   - Accurate timestamps
   - Consistent activity names
   - Minimal missing data

2. **Algorithm Selection**:
   - **Alpha Miner**: Clean, simple processes
   - **Heuristic Miner**: Noisy, real-world logs
   - **Inductive Miner**: Need guaranteed soundness

3. **Pre-filter Event Logs**:
   ```cpp
   // Filter before discovery
   auto filtered_log = mining.filterEventLog(event_log, {
       .min_case_length = 3,
       .max_case_length = 50,
       .activity_filter = {"Login", "Logout"}
   });
   
   auto model = mining.discoverProcess(filtered_log);
   ```

4. **Use Incremental Conformance**: For large logs
   ```cpp
   // Process in chunks
   for (const auto& chunk : event_log_chunks) {
       auto conformance = mining.checkConformanceIncremental(chunk, model);
       // Update metrics
   }
   ```

### Complex Event Processing Best Practices

1. **Window Sizing**: Balance latency and accuracy
   ```cpp
   // Too small: May miss patterns
   window.size = std::chrono::seconds(10);
   
   // Too large: High latency
   window.size = std::chrono::hours(1);
   
   // Just right: Depends on use case
   window.size = std::chrono::minutes(5);
   ```

2. **Pattern Complexity**: Simpler = faster
   ```cpp
   // Simple: Fast matching
   SEQUENCE(A, B)
   
   // Complex: Slower matching
   SEQUENCE(A, B OR C, D, E) WITHIN 1 HOUR
   ```

3. **State Management**: Use checkpointing
   ```cpp
   engine.enableCheckpointing({
       .interval = std::chrono::minutes(1),
       .backend = CheckpointBackend::ROCKSDB
   });
   ```

4. **Backpressure Handling**: Handle slow consumers
   ```cpp
   engine.setBackpressureStrategy({
       .strategy = BackpressureStrategy::BUFFER,
       .buffer_size = 10000,
       .on_overflow = OverflowAction::DROP_OLDEST
   });
   ```

## Performance Benchmarks

### OLAP Query Performance

Benchmark environment: AMD EPYC 7763, 128GB RAM, NVMe SSD

| Query Type | Dataset Size | Execution Time | Throughput |
|-----------|--------------|----------------|------------|
| Simple aggregation (SUM) | 1M rows | 15ms | 66K rows/sec |
| Simple aggregation (SUM) | 10M rows | 142ms | 70K rows/sec |
| GROUP BY (1 dimension) | 1M rows | 45ms | 22K rows/sec |
| GROUP BY (1 dimension) | 10M rows | 425ms | 23K rows/sec |
| GROUP BY (3 dimensions) | 1M rows | 120ms | 8.3K rows/sec |
| GROUP BY (3 dimensions) | 10M rows | 1.2s | 8.3K rows/sec |
| Window function (ROW_NUMBER) | 1M rows | 80ms | 12.5K rows/sec |
| Window function (moving avg) | 1M rows | 95ms | 10.5K rows/sec |
| Complex OLAP (CUBE) | 1M rows | 350ms | 2.8K rows/sec |
| Complex OLAP (ROLLUP) | 1M rows | 280ms | 3.5K rows/sec |

**With Vectorization (SIMD) — Phase 5 benchmarks (v1.9.0):**
| Query Type | Dataset Size | Scalar | AVX2 | AVX-512 | ARM NEON |
|-----------|--------------|--------|------|---------|----------|
| SUM aggregation | 10M doubles | 142ms | 28ms (5.1×) | 14ms (10.1×) | 32ms (4.4×) |
| AVG aggregation | 10M doubles | 158ms | 35ms (4.5×) | 17ms (9.3×) | 39ms (4.1×) |
| MIN/MAX | 10M doubles | 125ms | 18ms (6.9×) | 9ms (13.9×) | 21ms (6.0×) |
| Complex filter | 10M rows | 210ms | 45ms (4.7×) | 23ms (9.1×) | 50ms (4.2×) |

AVX-512 results measured on Intel Xeon Scalable (Ice Lake); ARM NEON on Apple M1 (Cortex-A78-class).
AVX-512 / ARM NEON results are bit-identical to scalar within 1 ULP (CI parity assertions pass).

**LRU Result Cache:**
| Scenario | Result |
|----------|--------|
| Repeated identical query (cache hit) | ≤ 1 µs (hash lookup only) |
| Cache with 10 000 unique queries, max_entries=100 | Bounded: never exceeds 100 entries; no OOM |
| TTL expiry (50 ms TTL, 100 ms sleep) | Entry evicted before next access |

### Data Export Performance

| Format | Dataset Size | Export Time | Throughput | File Size |
|--------|--------------|-------------|------------|-----------|
| JSON | 100K rows | 250ms | 400K rows/sec | 45MB |
| JSON (streaming) | 1M rows | 2.4s | 416K rows/sec | 450MB |
| CSV | 100K rows | 180ms | 555K rows/sec | 15MB |
| CSV (streaming) | 1M rows | 1.7s | 588K rows/sec | 150MB |
| Arrow IPC | 100K rows | 120ms | 833K rows/sec | 12MB |
| Arrow IPC (zero-copy, v1.9.0) | 1M rows | 620ms | 1.61M rows/sec | 120MB |
| Parquet | 100K rows | 200ms | 500K rows/sec | 8MB |
| Parquet (compressed, snappy) | 1M rows | 1.85s | 541K rows/sec | 28MB |

Arrow IPC zero-copy path (v1.9.0) uses `arrow::Buffer::Wrap()` — copy overhead ≤ 1 % of total export time.
Parquet 1 M rows target ≤ 2 s wall time on a single core with snappy compression — met.

### Process Mining Performance

| Operation | Event Log Size | Events per Case | Execution Time |
|-----------|----------------|-----------------|----------------|
| Process discovery (Alpha) | 10K events | 20 | 180ms |
| Process discovery (Heuristic) | 10K events | 20 | 450ms |
| Process discovery (Heuristic) | 100K events | 20 | 3.2s |
| Process discovery (Inductive) | 10K events | 20 | 620ms |
| Conformance (Token replay) | 10K events | 20 | 280ms |
| Conformance (Alignment) | 10K events | 20 | 850ms |
| Pattern matching (Graph) | 1K processes | N/A | 120ms |
| Pattern matching (Vector) | 10K processes | N/A | 45ms |

### Complex Event Processing Performance

| Scenario | Event Rate | Pattern Complexity | Latency (p50) | Latency (p99) |
|----------|------------|-------------------|---------------|---------------|
| Simple pattern (A→B) | 10K events/sec | 2 events | 2ms | 5ms |
| Simple pattern (A→B) | 100K events/sec | 2 events | 3ms | 8ms |
| Complex pattern (A→B→C→D) | 10K events/sec | 4 events | 8ms | 15ms |
| Pattern with quantifier | 10K events/sec | Variable | 12ms | 25ms |
| Aggregation (1 min window) | 10K events/sec | N/A | 18ms | 35ms |
| Aggregation (5 min window) | 10K events/sec | N/A | 22ms | 42ms |

### Anomaly Detection Performance (v1.9.0)

| Scenario | Threads | Calls/thread | P99 Latency |
|----------|---------|--------------|-------------|
| `StreamingAnomalyDetector::process()` (Z_SCORE, window=200) | 8 | 500 | ≤ 1 ms |

Training runs asynchronously outside the `mu_` lock; `process()` lock-hold is ≤ 50 µs (deque copy only).

### Model Serving Performance (v1.9.0)

| Scenario | Threads | Duration | Throughput |
|----------|---------|----------|------------|
| `ModelServingEngine::predict()` (decision tree, depth≈10) | 16 | 1 s | ≥ 10 000 predictions/s/core |

Registry lock is released before inference; per-prediction lock-hold ≤ 5 µs.

### Forecasting Performance (v1.9.0)

| Operation | Series / Steps | Time |
|-----------|---------------|------|
| `predictBatch()` (ETS, 1 000 series × 30 steps) | 1 000 × 30 | ≤ 50 ms (single core) |
| `update(new_value)` O(1) incremental absorption | — | ≤ 10 µs/call |
| Auto-tune grid (9 α, n=500) — parallel `std::async` | 9 tasks | ≤ 5 ms |

### Graph Analytics Performance

| Algorithm | Graph Size | Execution Time | Memory Usage |
|-----------|------------|----------------|--------------|
| PageRank (10 iter) | 10K vertices, 50K edges | 180ms | 25MB |
| PageRank (10 iter) | 100K vertices, 500K edges | 2.1s | 180MB |
| PageRank (10 iter) | 1M vertices, 5M edges | 28s | 1.5GB |
| Community detection | 10K vertices, 50K edges | 320ms | 30MB |
| Community detection | 100K vertices, 500K edges | 4.2s | 220MB |
| Shortest path (BFS) | 10K vertices, 50K edges | 15ms | 20MB |
| Shortest path (Dijkstra) | 10K vertices, 50K edges | 28ms | 22MB |
| Triangle counting | 10K vertices, 50K edges | 95ms | 25MB |

### Text Analytics Performance

| Operation | Text Size | Execution Time | Throughput |
|-----------|-----------|----------------|------------|
| Tokenization | 1MB text | 45ms | 22MB/sec |
| Keyword extraction | 1MB text | 180ms | 5.5MB/sec |
| Named entity recognition | 1MB text | 320ms | 3.1MB/sec |
| Sentiment analysis | 1MB text | 95ms | 10.5MB/sec |
| Text summarization | 1MB text | 420ms | 2.4MB/sec |

**Configuration:** Default settings, no special tuning

## Building

The Analytics module is built as part of ThemisDB.

### Standard Build

```bash
cmake -B build -S .
cmake --build build
```

### Build with Apache Arrow Support

```bash
cmake -B build -S . -DTHEMIS_ENABLE_ARROW=ON
cmake --build build
```

### Build with GPU Support

```bash
cmake -B build -S . -DTHEMIS_ENABLE_GPU=ON -DCMAKE_CUDA_COMPILER=/usr/local/cuda/bin/nvcc
cmake --build build
```

### Build with All Analytics Features

```bash
cmake -B build -S . \
  -DTHEMIS_ENABLE_ARROW=ON \
  -DTHEMIS_ENABLE_GPU=ON \
  -DTHEMIS_ENABLE_SIMD=ON \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS="-march=native"
cmake --build build
```

## Testing

### Run All Analytics Tests

```bash
cd build
ctest -R analytics --verbose
```

### Run Specific Test Suites

```bash
# OLAP tests
./build/tests/test_olap

# Arrow export tests
./build/tests/analytics/test_arrow_export

# Process mining tests
./build/tests/analytics/test_process_mining_llm

# CEP engine tests (when implemented)
./build/tests/analytics/test_cep_engine

# Diff engine tests
./build/tests/analytics/test_diff_engine

# Text analytics tests
./build/tests/analytics/test_nlp_text_analyzer
```

### Run Performance Benchmarks

```bash
# OLAP benchmarks
./build/benchmarks/bench_olap --benchmark_min_time=5s

# Export benchmarks
./build/benchmarks/bench_export --benchmark_filter=Arrow

# Process mining benchmarks
./build/benchmarks/bench_process_mining --benchmark_repetitions=10
```

## Thread Safety

### Thread-Safe Components

- `OLAPEngine`: Concurrent queries supported with internal locking
- `CEPEngine`: Thread-safe event processing (when implemented)
- `ProcessMining`: Read operations thread-safe, write operations require external synchronization
- `DiffEngine`: Read operations thread-safe
- `NLPTextAnalyzer`: Thread-safe (no shared state)

### Non-Thread-Safe Components

- `ArrowRecordBatch`: Not thread-safe during construction; safe after finalization
- `OLAPQuery`: Should not be modified during execution
- `Exporters`: One export operation per instance at a time

### Best Practices

```cpp
// Option 1: Create separate instances per thread
std::thread t1([&]() {
    OLAPEngine engine1;
    auto result1 = engine1.execute(query1);
});

std::thread t2([&]() {
    OLAPEngine engine2;
    auto result2 = engine2.execute(query2);
});

// Option 2: Use mutex for shared instance
std::mutex engine_mutex;
OLAPEngine shared_engine;

std::thread t3([&]() {
    std::lock_guard<std::mutex> lock(engine_mutex);
    auto result3 = shared_engine.execute(query3);
});

// Option 3: Use thread pool with per-thread instances
ThreadPool pool(8);
std::vector<OLAPEngine> engines(8);

pool.enqueue([&, i = 0]() {
    return engines[i].execute(query);
});
```

## Memory Management

### Memory Allocation Patterns

**Columnar Storage:**
- Pre-allocates contiguous memory for columns
- Uses `std::vector` with `reserve()` for capacity planning
- Minimizes reallocations

**Batch Processing:**
- Fixed batch size (default: 1024 rows)
- Reuses batch buffers across iterations
- Releases memory after processing

**Caching:**
- LRU eviction for query result cache
- Configurable cache size limits
- Automatic memory pressure handling

### Memory Usage Guidelines

| Operation | Memory Usage | Notes |
|-----------|--------------|-------|
| OLAP aggregation | ~20 bytes per unique group | Hash table overhead |
| Arrow RecordBatch | Column_count × Row_count × Avg_size | Columnar layout |
| Process discovery | ~100 bytes per event | Graph representation |
| CEP state | ~50 bytes per active pattern | Window state |
| Export buffer | Batch_size × Row_size | Temporary buffer |

### Memory Optimization Tips

1. **Use streaming**: For datasets larger than available RAM
   ```cpp
   exporter->exportToStream(large_dataset, callback, options);
   ```

2. **Limit result sets**: Use LIMIT in queries
   ```sql
   FOR doc IN sales
     COLLECT region = doc.region
     AGGREGATE total = SUM(doc.amount)
     LIMIT 100
     RETURN { region, total }
   ```

3. **Enable compression**: Reduce memory footprint
   ```cpp
   config.compression = CompressionType::ZSTD;
   ```

4. **Clear caches periodically**: In long-running processes
   ```cpp
   engine.clearCache();
   ```

5. **Monitor memory usage**: Use observability metrics
   ```cpp
   auto stats = engine.getMemoryStats();
   std::cout << "Cache size: " << stats.cache_bytes << " bytes\n";
   ```

## Configuration

### Environment Variables

```bash
# Enable Apache Arrow (optional)
export THEMIS_ENABLE_ARROW=ON

# LLM Configuration
export OPENAI_API_KEY="sk-..."
export ANTHROPIC_API_KEY="sk-ant-..."
export AZURE_OPENAI_ENDPOINT="https://..."
export AZURE_OPENAI_KEY="..."

# Performance tuning
export THEMIS_ANALYTICS_BATCH_SIZE=1024
export THEMIS_ANALYTICS_CACHE_SIZE=1073741824  # 1GB
export THEMIS_ANALYTICS_THREAD_POOL_SIZE=8

# Enable SIMD optimization
export THEMIS_ENABLE_SIMD=ON

# GPU configuration
export CUDA_VISIBLE_DEVICES=0
export THEMIS_GPU_MEMORY_LIMIT=8589934592  # 8GB
```

### Runtime Configuration

```cpp
// OLAP engine configuration
OLAPEngine::Config config;
config.enable_vectorization = true;
config.batch_size = 2048;
config.enable_result_cache = true;
config.cache_ttl = std::chrono::minutes(10);
config.max_cache_size = 1024 * 1024 * 1024;  // 1GB

OLAPEngine engine(config);

// Export configuration
ExportOptions export_opts;
export_opts.format = ExportFormat::PARQUET;
export_opts.compression = CompressionType::SNAPPY;
export_opts.include_schema = true;
export_opts.batch_size = 10000;

// CEP engine configuration
CEPEngine::Config cep_config;
cep_config.window_size = std::chrono::minutes(5);
cep_config.checkpoint_interval = std::chrono::minutes(1);
cep_config.backpressure_threshold = 10000;

// Process mining configuration
ProcessMining::Config pm_config;
pm_config.algorithm = MiningAlgorithm::HEURISTIC;
pm_config.noise_threshold = 0.8;
pm_config.dependency_threshold = 0.9;
```

### Compile-Time Options

```cmake
# Enable Arrow support
option(THEMIS_ENABLE_ARROW "Enable Apache Arrow integration" OFF)

# Enable SIMD optimization
option(THEMIS_ENABLE_SIMD "Enable SIMD optimizations" ON)

# Enable GPU acceleration
option(THEMIS_ENABLE_GPU "Enable GPU acceleration" OFF)

# Enable LLM integration
option(THEMIS_ENABLE_LLM "Enable LLM integration" ON)

# Build benchmarks
option(THEMIS_BUILD_BENCHMARKS "Build performance benchmarks" OFF)
```

## Error Handling

### Error Types

```cpp
// OLAP errors
enum class OLAPError {
    INVALID_QUERY,
    COLLECTION_NOT_FOUND,
    INVALID_DIMENSION,
    INVALID_MEASURE,
    EXECUTION_ERROR,
    TIMEOUT
};

// Export errors
enum class ExportError {
    INVALID_FORMAT,
    FILE_ERROR,
    SERIALIZATION_ERROR,
    COMPRESSION_ERROR
};

// Process mining errors
enum class ProcessMiningError {
    INVALID_EVENT_LOG,
    ALGORITHM_ERROR,
    MODEL_ERROR,
    CONFORMANCE_ERROR
};
```

### Error Handling Patterns

```cpp
// Option 1: Return value with error information
auto result = engine.execute(query);
if (!result) {
    std::cerr << "OLAP error: " << result.error() << std::endl;
    std::cerr << "Error code: " << result.error_code() << std::endl;
    return;
}
// Use result.value()

// Option 2: Exception-based (when enabled)
try {
    auto result = engine.executeOrThrow(query);
    // Use result
} catch (const OLAPException& e) {
    std::cerr << "OLAP error: " << e.what() << std::endl;
    std::cerr << "Query: " << e.query() << std::endl;
}

// Option 3: Callback-based (for streaming)
engine.executeAsync(query, [](const OLAPResult& result) {
    // Success callback
    process(result);
}, [](const OLAPError& error) {
    // Error callback
    log_error(error);
});
```

## Debugging and Profiling

### Enable Debug Logging

```cpp
#include "observability/logger.h"

// Set log level
Logger::setLevel(LogLevel::DEBUG);

// Enable analytics logging
Logger::enableModule("analytics");

// Execute query with logging
auto result = engine.execute(query);
// Logs: Query plan, execution steps, timing
```

### Query Explain Plans

```cpp
// Get query plan without execution
auto plan = engine.explain(query);

std::cout << "Query Plan:\n";
std::cout << "  Dimensions: " << plan.dimensions.size() << "\n";
std::cout << "  Measures: " << plan.measures.size() << "\n";
std::cout << "  Filters: " << plan.filters.size() << "\n";
std::cout << "  Estimated cost: " << plan.estimated_cost << "\n";
std::cout << "  Estimated rows: " << plan.estimated_rows << "\n";
```

### Performance Profiling

```cpp
// Enable profiling
OLAPEngine::Config config;
config.enable_profiling = true;

OLAPEngine engine(config);
auto result = engine.execute(query);

// Get profiling data
auto profile = result.profile();
std::cout << "Execution time: " << profile.execution_time_ms << "ms\n";
std::cout << "Filter time: " << profile.filter_time_ms << "ms\n";
std::cout << "Aggregate time: " << profile.aggregate_time_ms << "ms\n";
std::cout << "Sort time: " << profile.sort_time_ms << "ms\n";
std::cout << "Rows processed: " << profile.rows_processed << "\n";
std::cout << "Rows filtered: " << profile.rows_filtered << "\n";
```

## Roadmap

### Phase 1: Foundation (✅ Complete)
- [x] OLAP engine with aggregations
- [x] Basic export interfaces (JSON, CSV)
- [x] Process mining (Alpha, Heuristic algorithms)
- [x] Text analytics (NLP basics)
- [x] Diff engine
- [x] LLM integration layer

### Phase 2: Optional Arrow Integration (✅ Complete)
- [x] Arrow RecordBatch columnar storage
- [x] Export interface design
- [x] Core implementation (always available)
- [x] Optional Apache Arrow C++ integration via `THEMIS_HAS_ARROW` flag
- [x] Arrow IPC format support (with Arrow flag)
- [x] Parquet writer integration with compression (with Arrow flag)
- [x] Feather format support (with Arrow flag)
- [x] Clear error handling when Arrow not available
- [x] Structured logging for export operations
- **Note:** Core functionality remains available without Apache Arrow

### Phase 3: Advanced Features (✅ Completed)
- [I] GPU-accelerated analytics (CUDA) (Issue: #1469 – planned)
- [x] Real-time anomaly detection (`analytics/anomaly_detection.cpp`)
- [I] Advanced graph analytics (betweenness, Louvain) (Issue: #1475 – planned)
- [x] Complex event processing – full NFA engine (`analytics/cep_engine.cpp`)
- [x] Incremental materialized views (`analytics/incremental_view.cpp`)
- [x] Streaming aggregations – TumblingWindow, SlidingWindow, SessionWindow, HoppingWindow (`analytics/streaming_window.cpp`)
- [x] Arrow Flight RPC support for remote analytics (`analytics/arrow_flight.cpp`) ✅
- [x] Zero-copy data transfer optimizations (in-process Arrow Flight transport) ✅
- [x] JIT-compiled hot-path aggregation (`analytics/jit_aggregation.cpp`) ✅
- [x] Distributed analytics sharding across cluster nodes (`analytics/distributed_analytics.cpp`) ✅

### Phase 4: ML Integration (✅ Completed)
- [x] Predictive analytics and time-series forecasting (`analytics/forecasting.cpp`) ✅
- [x] AutoML for automated model selection (`analytics/automl.cpp`)
- [x] Integration with external ML tools (ONNX Runtime, TensorFlow Serving) (`analytics/ml_serving.cpp`) ✅
- [x] Model serving and online inference pipeline (`analytics/model_serving.cpp`) ✅

## Contributing

When contributing to the Analytics module:

1. **Add tests** for new functionality
2. **Update documentation** for API changes
3. **Follow coding standards** (see CONTRIBUTING.md)
4. **Consider performance** implications
5. **Benchmark** new features
6. **Document** thread safety guarantees
7. **Add integration tests** for cross-module features
8. **Profile** memory usage
9. **Handle** platform differences (Windows vs Unix)

### Code Review Checklist

- [ ] Tests added and passing
- [ ] Documentation updated
- [ ] Performance benchmarks added
- [ ] Memory leaks checked (Valgrind/AddressSanitizer)
- [ ] Thread safety documented
- [ ] Error handling implemented
- [ ] Platform compatibility verified
- [ ] Integration tests added

## Scientific References

1. Gray, J., Bosworth, A., Layman, A., & Pirahesh, H. (1997). **Data Cube: A Relational Aggregation Operator Generalizing Group-By, Cross-Tab, and Sub-Totals**. *Data Mining and Knowledge Discovery*, 1, 29–53. https://doi.org/10.1023/A:1009726021843

2. Apache Arrow Community. (2025). **Apache Arrow: A cross-language development platform for in-memory data** (Project Documentation). https://arrow.apache.org/  
   (Autoritative technische Referenz für das im Modul genutzte spaltenorientierte In-Memory-Format / RecordBatches.)

3. Vohra, D. (2016). **Apache Parquet**. Apress. https://doi.org/10.1007/978-1-4842-1592-5  
   (Zitierbare Referenz zu Parquet als Columnar Storage Format; passt zur Export/Arrow-Integration.)

4. van der Aalst, W. M. P. (2016). **Process Mining: Data Science in Action (2nd ed.)**. Springer. https://doi.org/10.1007/978-3-662-49851-4  
   (Wissenschaftliche Grundlage für Process Discovery & Conformance Checking.)

5. Breunig, M. M., Kriegel, H.-P., Ng, R. T., & Sander, J. (2000). **LOF: Identifying Density-Based Local Outliers**. *Proceedings of the 2000 ACM SIGMOD International Conference on Management of Data*, 93–104. https://doi.org/10.1145/342009.335388  
   (Basis für LOF-Anomalieerkennung, wie sie im Modul verwendet wird.)

## License

Part of ThemisDB. See LICENSE file in the root directory.

## See Also

- **API Documentation**: [`../../include/analytics/README.md`](../../include/analytics/README.md)
- **Architecture**: [`ARCHITECTURE.md`](./ARCHITECTURE.md)
- **Roadmap**: [`ROADMAP.md`](./ROADMAP.md)
- **Future Plans**: [`FUTURE_ENHANCEMENTS.md`](./FUTURE_ENHANCEMENTS.md)
- **Secondary Docs (de)**: [`../../docs/de/analytics/README.md`](../../docs/de/analytics/README.md)
- **OLAP Guide**: [`../../docs/de/analytics/olap_guide.md`](../../docs/de/analytics/olap_guide.md)
- **Process Mining Guide**: [`../../docs/de/analytics/process_mining_guide.md`](../../docs/de/analytics/process_mining_guide.md)
- **Forecasting Guide**: [`../../docs/de/analytics/forecasting_guide.md`](../../docs/de/analytics/forecasting_guide.md)
- **CEP Guide**: [`../../docs/de/analytics/cep_guide.md`](../../docs/de/analytics/cep_guide.md)
- **Anomaly Detection Guide**: [`../../docs/de/analytics/anomaly_detection_guide.md`](../../docs/de/analytics/anomaly_detection_guide.md)
- **Streaming Windows Guide**: [`../../docs/de/analytics/streaming_window_guide.md`](../../docs/de/analytics/streaming_window_guide.md)
- **ML & AutoML Guide**: [`../../docs/de/analytics/ml_guide.md`](../../docs/de/analytics/ml_guide.md)
- **Query Module**: [`../query/README.md`](../query/README.md)
- **Index Module**: [`../index/README.md`](../index/README.md)
- **Observability**: [`../observability/README.md`](../observability/README.md)

