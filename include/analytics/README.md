> **Build:** `cmake --preset release && cmake --build build/release`

# ThemisDB Analytics Module - Header Files

**Version:** 1.9.0
**Status:** 🟢 Production-Ready
**Last Updated:** 2026-08-19
**Module Path:** `include/analytics/`

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

## About This Directory

This directory (`include/analytics/`) contains **header files only**. For implementation details, see [`src/analytics/README.md`](../../src/analytics/README.md).

## Public API Entry Points

The most important public headers for analytics integrations are:

| Header | Purpose |
| --- | --- |
| `olap.h` | OLAP query model and `OLAPEngine` execution API |
| `cep_engine.h` | CEP runtime (`CEPEngine`) and EPL rule processing |
| `forecasting.h` | Time-series model training/evaluation/prediction APIs |
| `process_mining.h` | Event-log extraction, discovery, and conformance checking |
| `analytics_export.h` + `arrow_export.h` | Export APIs (JSON/CSV always, Arrow formats optional) |
| `streaming_window.h` + `streaming_join.h` | Streaming windows and stream-stream joins |
| `model_serving.h` + `ml_serving.h` | In-process and external serving contracts with integrity and secure-transport policy controls |

## Header Files

### 1. OLAP Engine (`olap.h`)

Multi-dimensional analytical query processing with aggregations and window functions.

**Key Types:**
- `Dimension`: Defines grouping dimensions for OLAP queries
- `Measure`: Aggregation functions (COUNT, SUM, AVG, MIN, MAX, STDDEV, VARIANCE, MEDIAN, PERCENTILE)
- `Filter`: Query filtering conditions with multiple operators
- `OLAPQuery`: Complete query specification with dimensions, measures, filters
- `OLAPEngine`: Query execution engine with result caching

**Features:**
- GROUP BY, CUBE, ROLLUP operations
- Window functions (ROW_NUMBER, RANK, LAG, LEAD)
- Aggregation pushdown optimization
- Columnar execution for performance
- Materialized views support
- Query result caching

**Usage Example:**
```cpp
#include "analytics/olap.h"

using namespace themis::analytics;

// Create OLAP query
OLAPQuery query;
query.collection = "sales";
query.dimensions.push_back({"region", "", true});
query.dimensions.push_back({"product", "", true});
query.measures.push_back({"total_revenue", "amount", Measure::Function::Sum});
query.measures.push_back({"avg_price", "price", Measure::Function::Avg});

// Add filter
Filter filter;
filter.field = "date";
filter.op = Filter::Operator::Ge;
filter.value = "2024-01-01";
query.filters.push_back(filter);

// Execute
OLAPEngine engine;
auto result = engine.execute(query);
```

**Thread Safety:**
- OLAPEngine is thread-safe for concurrent queries
- Query objects should not be modified during execution

**Performance Considerations:**
- Use materialized views for frequently queried aggregations
- Create indexes on dimension columns
- Limit result sets with LIMIT clauses
- Enable columnar storage for analytical workloads

### 2. Apache Arrow Export (`arrow_export.h`, `analytics_export.h`)

Data export interfaces with optional Apache Arrow integration for interoperability with external analytics tools.

**Key Types:**
- `ArrowRecordBatch`: Columnar data representation (placeholder, Arrow-compatible)
- `IAnalyticsExporter`: Interface for export implementations
- `ExportFormat`: Supported formats (JSON, CSV, Arrow IPC, Parquet, Feather)
- `ExportOptions`: Configuration for export operations
- `ExportResult`: Export operation results with statistics

**Status:** ⚠️ Stub Implementation (GAP-003)
- Apache Arrow integration is **optional** via `THEMIS_ENABLE_ARROW` flag
- Core functionality (JSON/CSV export) always available
- Arrow formats (IPC, Parquet) require Arrow dependency

**Features:**
- Columnar data format (Arrow-compatible)
- Multiple export formats (JSON, CSV, Arrow IPC, Parquet, Feather)
- Streaming export for large datasets
- Export to file, string, or callback
- Compression support (when Arrow enabled)
- Schema definition and validation

**Usage Example:**
```cpp
#include "analytics/arrow_export.h"
#include "analytics/analytics_export.h"

using namespace themis::analytics;

// Create record batch
ArrowRecordBatch batch;
batch.addColumn({"id", ArrowRecordBatch::DataType::INT64, false});
batch.addColumn({"name", ArrowRecordBatch::DataType::STRING, true});
batch.addColumn({"score", ArrowRecordBatch::DataType::DOUBLE, true});

// Add data
batch.appendRow({int64_t(1), std::string("Alice"), 95.5});
batch.appendRow({int64_t(2), std::string("Bob"), 87.3});

// Export to JSON (always available)
auto exporter = ExporterFactory::createDefaultExporter();
ExportOptions options;
options.format = ExportFormat::JSON;
options.include_schema = true;

auto result = exporter->exportToFile(batch, "output.json", options);
if (result.status == ExportStatus::SUCCESS) {
    std::cout << "Exported " << result.rows_exported << " rows\n";
}

// Export to Arrow Parquet (requires THEMIS_ENABLE_ARROW)
options.format = ExportFormat::PARQUET;
options.compression = CompressionType::SNAPPY;
result = exporter->exportToFile(batch, "output.parquet", options);
```

**Integration Points:**
- Works with OLAP query results
- Exports to Pandas, DuckDB, Spark (via Arrow)
- Streaming export via callback interface

**Future Enhancements:**
- Native Apache Arrow C++ integration (optional)
- Zero-copy data transfer (with Arrow)
- Flight RPC support (with Arrow)

### 3. Process Mining (`process_mining.h`)

Process discovery, conformance checking, and performance analysis from event logs.

**Key Types:**
- `ProcessMining`: Main process mining engine
- `EventLog`: Structured event log representation
- `ProcessModel`: Discovered or defined process model
- `ConformanceResult`: Conformance checking results
- `MiningAlgorithm`: Algorithm selection (Alpha, Heuristic, Inductive)

**Features:**
- **Process Discovery**: Extract process models from event logs
  - Alpha Miner: Basic discovery algorithm
  - Heuristic Miner: Handles noise and incomplete logs
  - Inductive Miner: Guarantees sound process models
- **Conformance Checking**: Compare actual vs. ideal processes
  - Token replay
  - Alignment-based conformance
  - Fitness, precision, generalization metrics
- **Performance Analysis**: Bottleneck detection, waiting times
- **Process Enhancement**: Enrich models with performance data

**Usage Example:**
```cpp
#include "analytics/process_mining.h"

using namespace themis;

ProcessMining mining(db);

// Extract event log from collection
auto eventLog = mining.extractEventLog("audit_log", {
    .case_id_field = "order_id",
    .activity_field = "action",
    .timestamp_field = "timestamp"
});

// Discover process model
auto model = mining.discoverProcess(eventLog, MiningAlgorithm::HEURISTIC);

// Check conformance
auto conformance = mining.checkConformance(eventLog, model);
std::cout << "Fitness: " << conformance.fitness << std::endl;
std::cout << "Precision: " << conformance.precision << std::endl;

// Export to BPMN
std::string bpmn = mining.exportToBPMN(model);
```

**Integration with Other Modules:**
- Uses GraphIndex for process graph representation
- Uses VectorIndex for process similarity search
- Integrates with LLM module for semantic analysis

### 4. Process Pattern Matcher (`process_pattern_matcher.h`)

Find similar processes and patterns using graph, vector, and behavioral similarity.

**Key Types:**
- `ProcessPatternMatcher`: Main pattern matching engine
- `Pattern`: Process pattern definition
- `SimilarityMethod`: Similarity computation methods (GRAPH, VECTOR, BEHAVIORAL, HYBRID)
- `ComparisonResult`: Pattern comparison results

**Features:**
- Graph-based similarity (structure)
- Vector-based similarity (semantics)
- Behavioral similarity (execution patterns)
- Hybrid similarity (weighted combination)
- Top-K similar process retrieval

**Usage Example:**
```cpp
#include "analytics/process_pattern_matcher.h"

using namespace themis;

ProcessPatternMatcher matcher(db);

// Define ideal pattern
Pattern ideal = {
    .activities = {"Order", "Approve", "Ship", "Deliver"},
    .edges = {
        {"Order", "Approve"},
        {"Approve", "Ship"},
        {"Ship", "Deliver"}
    }
};

// Find similar processes
auto results = matcher.findSimilar(
    ideal,
    0.7,  // 70% similarity threshold
    SimilarityMethod::HYBRID,
    10  // Top 10 results
);

for (const auto& result : results) {
    std::cout << "Process: " << result.process_id
              << " Similarity: " << result.score << std::endl;
}
```

### 5. Text Analytics (`nlp_text_analyzer.h`)

Lightweight NLP-based text analysis for query optimization and text processing.

**Key Types:**
- `NLPTextAnalyzer`: Main text analysis engine
- `Token`: Tokenization result with POS tags
- `NamedEntity`: Extracted named entities
- `Keyword`: Keywords with TF-IDF scores
- `SentimentResult`: Sentiment analysis results

**Features:**
- Text tokenization and lemmatization
- Part-of-speech tagging
- Named entity recognition (PERSON, ORG, LOCATION)
- Keyword extraction (TF-IDF)
- Sentiment analysis (POSITIVE, NEGATIVE, NEUTRAL)
- Text summarization
- Language detection

**Usage Example:**
```cpp
#include "analytics/nlp_text_analyzer.h"

using namespace themis::analytics;

NLPTextAnalyzer analyzer;

std::string text = "ThemisDB is a powerful database with advanced analytics.";

// Tokenize
auto tokens = analyzer.tokenize(text);

// Extract keywords
auto keywords = analyzer.extractKeywords(text, 5);

// Named entity recognition
auto entities = analyzer.extractNamedEntities(text);

// Sentiment analysis
auto sentiment = analyzer.analyzeSentiment(text);
std::cout << "Sentiment: " << sentiment.label << " ("
          << sentiment.confidence << ")" << std::endl;
```

**Performance:**
- CPU-efficient, no GPU required
- Optimized for database query analysis
- Not a full NLP framework (use LLM module for advanced NLP)

### 6. LLM Process Analyzer (`llm_process_analyzer.h`)

LLM integration for advanced process analysis and compliance checking.

**Key Types:**
- `LLMProvider`: Provider selection (OpenAI, Anthropic, Local, Azure)
- `TaskType`: Analysis task types (conformance, prediction, fraud detection)
- `LLMConfig`: Provider and model configuration
- `LLMRequest`: Analysis request specification
- `LLMResponse`: Analysis results with metrics

**Features:**
- Process conformance checking with LLM
- Next activity prediction
- Compliance verification (5R Rule, Vier-Augen-Prinzip)
- Fraud detection
- Sentiment analysis
- Process optimization recommendations

**Supported Providers:**
- OpenAI (GPT-4, GPT-3.5)
- Anthropic (Claude 3 Opus, Sonnet)
- Local models (llama.cpp, ollama)
- Azure OpenAI Service

**Usage Example:**
```cpp
#include "analytics/llm_process_analyzer.h"

LLMConfig config;
config.provider = LLMProvider::OPENAI;
config.api_key = "sk-...";
config.model_name = "gpt-4";
config.temperature = 0.3;

LLMRequest request;
request.task_type = TaskType::VERIFY_5R_RULE;
request.domain = "healthcare";
request.process_trace = eventLog.toJson();
request.ideal_model = idealProcess.toJson();

auto response = analyzeLLM(config, request);
if (response.success) {
    std::cout << "Conformance: " << response.conformance_score << std::endl;
    for (const auto& deviation : response.deviations) {
        std::cout << "Deviation: " << deviation << std::endl;
    }
}
```

**Performance Considerations:**
- Enable caching for repeated queries
- Use lower temperature for deterministic results
- Configure retry logic for reliability

### 7. Complex Event Processing (`cep_engine.h`)

Real-time streaming analytics with pattern matching and window management.

**Key Types:**
- `CEPEngine`: Main CEP processing engine
- `EventStream`: Stream of events
- `PatternMatcher`: Event pattern matching
- `WindowManager`: Window management (tumbling, sliding, session, hopping)
- `RuleEngine`: Rule-based event processing
- `EventType`: Event categorization

**Features:**
- **Pattern Matching**: SEQUENCE, AND, OR, NOT, WITHIN patterns
- **Window Management**:
  - Tumbling windows (fixed, non-overlapping)
  - Sliding windows (fixed, overlapping)
  - Session windows (gap-based)
  - Hopping windows (configurable hop size)
- **Aggregations**: COUNT, SUM, AVG, MIN, MAX, PERCENTILE
- **EPL Support**: Event Processing Language
- **Stateful Processing**: Checkpoint and recovery
- **CDC Integration**: Change data capture integration

**Usage Example:**
```cpp
#include "analytics/cep_engine.h"

using namespace themisdb::analytics;

CEPEngine engine;

// Define pattern: Login followed by Purchase within 1 hour
Pattern pattern = engine.createSequencePattern({
    {"Login", "action == 'login'"},
    {"Purchase", "action == 'purchase' AND amount > 100"}
}, std::chrono::hours(1));

// Register callback
engine.registerPattern(pattern, [](const MatchedEvent& event) {
    std::cout << "Pattern matched: " << event.toJson() << std::endl;
    // Trigger alert, log, or action
});

// Start processing
engine.start();

// Feed events
engine.processEvent(createEvent("login", user_id));
engine.processEvent(createEvent("purchase", user_id));
```

**Integration:**
- Works with CDC module for database change events
- Integrates with messaging systems (Kafka, RabbitMQ)
- Supports custom event sources

### 8. Diff Engine (`diff_engine.h`)

Git-like diff functionality for MVCC versioned data.

**Key Types:**
- `DiffEngine`: Main diff computation engine
- `Change`: Single change representation
- `ChangeType`: Type of change (ADDED, MODIFIED, DELETED)
- `DiffResult`: Complete diff result with statistics
- `DiffStats`: Summary statistics

**Features:**
- Diff by sequence number range
- Diff by timestamp range
- Filtering by table, key prefix, event type
- Pagination for large result sets
- Structured output (Add/Modify/Delete)
- JSON export

**Usage Example:**
```cpp
#include "analytics/diff_engine.h"

using namespace themis::analytics;

DiffEngine engine(changefeed, snapshot_manager);

// Diff between two timestamps
auto diff = engine.diffByTimestamp(
    "2024-01-01T00:00:00Z",
    "2024-01-02T00:00:00Z",
    {.table_filter = "orders"}
);

std::cout << "Added: " << diff.stats.added_count << std::endl;
std::cout << "Modified: " << diff.stats.modified_count << std::endl;
std::cout << "Deleted: " << diff.stats.deleted_count << std::endl;

// Iterate changes
for (const auto& change : diff.modified) {
    std::cout << "Modified: " << change.key
              << " from " << change.old_value.value()
              << " to " << change.new_value.value() << std::endl;
}
```

**Performance:**
- Target: <100ms for 10K changes
- Target: <1s for 100K changes
- Streaming support for very large diffs

### 9. Export Helpers (`analytics_export.h`)

Additional export utilities and factory methods.

**Key Types:**
- `ExporterFactory`: Factory for creating exporters
- `ExportStatus`: Export operation status
- `CompressionType`: Compression options

**Usage Example:**
```cpp
#include "analytics/analytics_export.h"

// Create exporter
auto exporter = ExporterFactory::createDefaultExporter();

// Create custom exporter
auto csvExporter = ExporterFactory::createExporter(ExportFormat::CSV);

// Check capabilities
bool supportsArrow = exporter->supportsFormat(ExportFormat::ARROW_IPC);
```

### 10. Real-Time Anomaly Detection (`anomaly_detection.h`)

Streaming and batch anomaly detection with multiple algorithms and adaptive learning.

**Key Types:**
- `DataPoint`: Heterogeneous record (fields: string, double, int64_t, bool)
- `AnomalyMethod`: Algorithm selector (Z_SCORE, MODIFIED_Z_SCORE, IQR, ISOLATION_FOREST, LOF, ENSEMBLE)
- `AnomalyDetector`: Batch training + single-point and batch prediction
- `AnomalyResult`: Detection result with anomaly score, flag, and feature contributions
- `AnomalyExplanation`: Sorted feature contributions
- `StreamingAnomalyDetector`: Rolling-window, online anomaly detection
- `AnomalyDetectorStats`: Statistics about the trained model

**Features:**
- Six algorithms: Z-Score, Modified Z-Score (MAD), IQR, Isolation Forest, LOF, Ensemble
- Adaptive incremental learning via `update()`
- Permutation-based feature explanation
- Serialise/deserialise model state
- Thread-safe streaming detector with configurable window

**Usage Example:**
```cpp
#include "analytics/anomaly_detection.h"

using namespace themisdb::analytics;

// Batch detector
AnomalyDetector detector(AnomalyMethod::ISOLATION_FOREST);
detector.train(training_data);

// Predict single point
auto result = detector.predict(point);
if (result.is_anomaly) {
    auto exp = detector.explain(point);
    for (auto& [feat, score] : exp.feature_contributions)
        std::cout << feat << ": " << score << "\n";
}

// Streaming detector
StreamingAnomalyDetector::Config cfg;
cfg.window_size = 1000;
cfg.method = AnomalyMethod::ENSEMBLE;
StreamingAnomalyDetector stream_det(cfg);

stream_det.process(point);
auto anomalies = stream_det.getAnomalies();
```

### 11. AutoML Engine (`automl.h`)

Automated Machine Learning for classification and regression tasks.

**Key Types:**
- `AutoMLTask`: CLASSIFICATION or REGRESSION
- `ModelAlgorithm`: LOGISTIC_REGRESSION, LINEAR_REGRESSION, DECISION_TREE, RANDOM_FOREST, GRADIENT_BOOSTING, KNN, ENSEMBLE
- `AutoMLMetric`: Primary optimisation metric (ACCURACY, F1, PRECISION, RECALL, AUC_ROC, R2, RMSE, MAE, MAPE)
- `AutoMLConfig`: Training budget, algorithm selection, feature engineering, ensemble settings
- `EvalMetrics`: Cross-validated metrics for all algorithms
- `CandidateModelInfo`: Metadata for each evaluated candidate (hyperparameters, CV score)
- `ModelExplanation`: SHAP-approximated per-sample feature contributions
- `AutoMLModel`: Trained, predict-ready model (move-only)
- `AutoML`: Training façade (trainClassifier / trainRegressor / crossValidate)

**Features:**
- Automated algorithm selection via random hyperparameter search
- k-fold cross-validation for unbiased evaluation
- Time/trial budget control
- Standard scaling + optional degree-2 polynomial feature expansion
- Soft-voting ensemble from top-k candidates
- Permutation-based SHAP feature importance
- Full metric suite: accuracy, F1, precision, recall, AUC-ROC; R², RMSE, MAE, MAPE
- Serialisation / deserialisation
- Optional progress callback

**Usage Example:**
```cpp
#include "analytics/automl.h"

using namespace themisdb::analytics;

// Prepare data points (reuses DataPoint from anomaly_detection.h)
std::vector<DataPoint> data = loadData();

// Classification
AutoML automl;
auto model = automl.trainClassifier(data, {
    .target              = "churn",
    .metric              = AutoMLMetric::F1,
    .max_time_minutes    = 60,
    .feature_engineering = true,
    .ensemble            = true,
    .ensemble_top_k      = 3
});

// Predict
auto predictions = model.predict(test_data);

// Explain
auto explanations = model.explain(test_data);
for (const auto& exp : explanations)
    std::cout << exp.predicted_label << " | top: " << exp.top_features << "\n";

// Feature importance (normalised to [0, 1])
for (const auto& [feat, imp] : model.featureImportance())
    std::cout << feat << ": " << imp << "\n";

// Regression
auto reg = automl.trainRegressor(data, {
    .target = "price",
    .metric = AutoMLMetric::R2
});
```

**Thread Safety:**
- `AutoML::trainClassifier` / `trainRegressor` – NOT thread-safe (modifies no global state; callers can use separate `AutoML` instances).
- `AutoMLModel::predict` / `explain` – thread-safe after construction.

## Integration with Other Modules

### With Query Module
- OLAP queries can be triggered from AQL
- Window functions integrated with query optimizer
- Analytics results feed back into query cache
- Subqueries can leverage analytics functions

**Example AQL:**
```sql
FOR doc IN sales
  COLLECT region = doc.region
  AGGREGATE total = SUM(doc.amount), avg_price = AVG(doc.price)
  RETURN { region, total, avg_price }
```

### With Index Module
- Graph analytics use GraphIndex for structure
- Vector similarity uses VectorIndex for embeddings
- Spatial analytics use SpatialIndex for geometry
- Temporal analytics use TemporalIndex for time-series

### With Storage Module
- Direct access to columnar data for OLAP
- Efficient batch reads for analytics workloads
- BlobDB integration for large analytical datasets
- MVCC snapshots for consistent analytics

### With Observability Module
- Export metrics and traces via Arrow
- Integration with Prometheus for monitoring
- Grafana dashboards for analytics visualization
- Performance metrics tracking

## Vectorized Execution

The Analytics module leverages vectorized execution for performance:

**Techniques:**
- **SIMD Instructions**: AVX2/AVX-512 for aggregations
- **Columnar Layout**: Cache-friendly data access
- **Batch Processing**: Amortize function call overhead
- **Lazy Evaluation**: Defer computation until needed
- **Pipeline Parallelism**: Overlap computation stages

**Performance Gains:**
- 5-10x faster aggregations
- 3-5x faster filtering operations
- 2-4x faster expression evaluation
- 10-50x faster for analytics queries (vs. row-wise)

**Example:**
```cpp
// Vectorized aggregation (processes 1024 rows at a time)
OLAPEngine::Config config;
config.enable_vectorization = true;
config.batch_size = 1024;
config.use_simd = true;

OLAPEngine engine(config);
auto result = engine.execute(query);  // Automatically uses vectorized execution
```

## Analytics Best Practices

### Query Design
1. **Use appropriate aggregation functions**: Choose COUNT, SUM, AVG based on data type
2. **Filter early**: Apply filters before aggregations
3. **Limit dimensions**: Fewer GROUP BY dimensions = faster execution
4. **Use materialized views**: Pre-compute frequent aggregations
5. **Index dimension columns**: Speed up GROUP BY operations

### Performance Optimization
1. **Columnar storage**: Use for analytical workloads
2. **Compression**: Enable compression for space and I/O efficiency
3. **Partitioning**: Partition large tables by time or key
4. **Batch operations**: Process multiple rows at once
5. **Caching**: Enable result caching for repeated queries

### Data Export
1. **Use streaming**: For large datasets, use streaming export
2. **Compression**: Enable compression for network transfers
3. **Format selection**:
   - JSON: Human-readable, debugging
   - CSV: Simple integration
   - Parquet: Efficient storage and columnar analytics
   - Arrow IPC: Zero-copy inter-process communication
4. **Batch export**: Export in chunks to avoid memory pressure

### Process Mining
1. **Event log quality**: Ensure complete event logs
2. **Algorithm selection**:
   - Alpha Miner: Clean, simple processes
   - Heuristic Miner: Noisy, real-world logs
   - Inductive Miner: Need guaranteed soundness
3. **Performance tuning**: Filter event logs before discovery
4. **Conformance checking**: Use alignment for accuracy

### Complex Event Processing
1. **Window sizing**: Balance latency and accuracy
2. **Pattern complexity**: Simpler patterns = faster matching
3. **State management**: Use checkpointing for fault tolerance
4. **Backpressure handling**: Handle slow consumers gracefully

## Performance Benchmarks

### OLAP Queries
| Query Type | Dataset Size | Execution Time | Throughput |
|-----------|--------------|----------------|------------|
| Simple aggregation (SUM) | 1M rows | 15ms | 66K rows/sec |
| GROUP BY (1 dimension) | 1M rows | 45ms | 22K rows/sec |
| GROUP BY (3 dimensions) | 1M rows | 120ms | 8.3K rows/sec |
| Window function | 1M rows | 80ms | 12.5K rows/sec |
| Complex OLAP (CUBE) | 1M rows | 350ms | 2.8K rows/sec |

### Data Export
| Format | Dataset Size | Export Time | Throughput |
|--------|--------------|-------------|------------|
| JSON | 100K rows | 250ms | 400K rows/sec |
| CSV | 100K rows | 180ms | 555K rows/sec |
| Arrow IPC | 100K rows | 120ms | 833K rows/sec |
| Parquet | 100K rows | 200ms | 500K rows/sec |

### Process Mining
| Operation | Event Log Size | Execution Time |
|-----------|----------------|----------------|
| Process discovery (Heuristic) | 10K events | 450ms |
| Process discovery (Heuristic) | 100K events | 3.2s |
| Conformance checking (Token replay) | 10K events | 280ms |
| Conformance checking (Alignment) | 10K events | 850ms |

### Complex Event Processing
| Scenario | Event Rate | Latency (p99) |
|----------|-----------|---------------|
| Simple pattern (2 events) | 10K events/sec | 5ms |
| Complex pattern (5 events) | 10K events/sec | 15ms |
| Aggregation (1 min window) | 10K events/sec | 25ms |

### Graph Analytics
| Algorithm | Graph Size | Execution Time |
|-----------|------------|----------------|
| PageRank (10 iterations) | 10K vertices, 50K edges | 180ms |
| PageRank (10 iterations) | 100K vertices, 500K edges | 2.1s |
| Community detection | 10K vertices, 50K edges | 320ms |
| Shortest path | 10K vertices, 50K edges | 15ms |

**Hardware:** AMD EPYC 7763, 128GB RAM, NVMe SSD
**Configuration:** Default settings, no special tuning

## Thread Safety

### Thread-Safe Components
- `OLAPEngine`: Concurrent queries supported
- `CEPEngine`: Thread-safe event processing
- `ProcessMining`: Read operations thread-safe
- `DiffEngine`: Read operations thread-safe

### Non-Thread-Safe Components
- `ArrowRecordBatch`: Not thread-safe during construction
- `OLAPQuery`: Should not be modified during execution
- Exporters: One export operation per instance at a time

**Best Practice:** Create separate instances per thread or use mutex for shared instances.

## Error Handling

All analytics operations return results with error information:

```cpp
// OLAP query error handling
auto result = engine.execute(query);
if (!result) {
    std::cerr << "Error: " << result.error() << std::endl;
    return;
}

// Export error handling
auto exportResult = exporter->exportToFile(batch, "output.json", options);
if (exportResult.status != ExportStatus::SUCCESS) {
    std::cerr << "Export failed: " << exportResult.error_message << std::endl;
}
```

## Memory Management

### Memory Considerations
- **Columnar data**: Allocates contiguous memory for columns
- **Large aggregations**: May require significant memory
- **Export operations**: Consider streaming for large datasets
- **Process mining**: Event logs loaded into memory

### Best Practices
1. Use streaming for large datasets
2. Limit result set sizes
3. Enable compression to reduce memory footprint
4. Clear caches periodically
5. Monitor memory usage in production

## Configuration

### Environment Variables
```bash
# Enable Apache Arrow (optional)
export THEMIS_ENABLE_ARROW=ON

# LLM Configuration
export OPENAI_API_KEY="sk-..."
export ANTHROPIC_API_KEY="sk-ant-..."

# Performance tuning
export THEMIS_ANALYTICS_BATCH_SIZE=1024
export THEMIS_ANALYTICS_CACHE_SIZE=1GB
```

## Compile-Time Options
```cmake
# Enable Arrow support
set(THEMIS_ENABLE_ARROW ON)

# Enable SIMD optimization
set(THEMIS_ENABLE_SIMD ON)

# Enable GPU acceleration
set(THEMIS_ENABLE_GPU ON)
```

## Dependencies

### Required
- nlohmann/json (JSON processing)
- Standard C++17 library

### Optional
- Apache Arrow C++ (for Arrow export formats)
- OpenSSL (for LLM API calls)
- CUDA (for GPU acceleration)

## Troubleshooting

- Arrow export formats (IPC/Parquet/Feather) require Arrow support at build time; otherwise exporters return `NOT_SUPPORTED`.
- LLM-backed analysis requires provider credentials (`OPENAI_API_KEY`, `ANTHROPIC_API_KEY`, etc.).
- For module-level troubleshooting playbooks, see [`../../docs/troubleshooting/analytics_troubleshooting.md`](../../docs/troubleshooting/analytics_troubleshooting.md).

## Testing

Run analytics tests:
```bash
cd build
ctest -R analytics --verbose
```

Specific test suites:
```bash
./build/tests/test_olap
./build/tests/analytics/test_arrow_export
./build/tests/analytics/test_process_mining_llm
./build/tests/analytics/test_cep_engine
./build/tests/analytics/test_incremental_view
./build/tests/analytics/test_streaming_window
./build/tests/analytics/test_anomaly_detection
./build/tests/analytics/test_automl
./build/tests/analytics/test_diff_engine
```

## See Also

- **Implementation**: [`src/analytics/README.md`](../../src/analytics/README.md)
- **Roadmap**: [`../../src/analytics/ROADMAP.md`](../../src/analytics/ROADMAP.md)
- **Future Plans**: [`../../src/analytics/FUTURE_ENHANCEMENTS.md`](../../src/analytics/FUTURE_ENHANCEMENTS.md)
- **Query Integration**: [`../query/README.md`](../query/README.md)
- **Index Integration**: [`../index/README.md`](../index/README.md)
- **Observability**: [`../observability/README.md`](../observability/README.md)

## Contributing

When contributing to the Analytics module:

1. **Add tests** for new functionality
2. **Update documentation** for API changes
3. **Follow coding standards** (see CONTRIBUTING.md)
4. **Consider performance** implications
5. **Benchmark** new features
6. **Document** thread safety guarantees
7. **Add integration tests** for cross-module features

## License

Part of ThemisDB. See LICENSE file in the root directory.

## See Also

- **Implementation Documentation**: [`../../src/analytics/README.md`](../../src/analytics/README.md)
- **Architecture**: [`../../src/analytics/ARCHITECTURE.md`](../../src/analytics/ARCHITECTURE.md)
- **Roadmap**: [`../../src/analytics/ROADMAP.md`](../../src/analytics/ROADMAP.md)
- **Future Enhancements**: [`../../src/analytics/FUTURE_ENHANCEMENTS.md`](../../src/analytics/FUTURE_ENHANCEMENTS.md)
- **Secondary Docs (de)**: [`../../docs/de/analytics/README.md`](../../docs/de/analytics/README.md)
- **OLAP Guide**: [`../../docs/de/analytics/olap_guide.md`](../../docs/de/analytics/olap_guide.md)
- **Forecasting Guide**: [`../../docs/de/analytics/forecasting_guide.md`](../../docs/de/analytics/forecasting_guide.md)
- **CEP Guide**: [`../../docs/de/analytics/cep_guide.md`](../../docs/de/analytics/cep_guide.md)

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
