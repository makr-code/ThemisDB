# Analytics Module - Future Enhancements

**Version:** 1.9.0
**Status:** 📋 Active — Core analytics fully implemented; extensions planned
**Last Updated:** 2026-04-11
**Module Path:** `include/analytics/`

---

## Completed Public API Additions (v1.8.0 – v1.9.0)

The following public API additions are now available in `include/analytics/`:

| API | File | Version | Notes |
|-----|------|---------|-------|
| `OLAPEngine::Config::result_cache_max_entries` | `olap.h` | v1.8.0 | LRU result cache cap (default 1 000); set to 0 to disable |
| `OLAPEngine::Config::result_cache_ttl_ms` | `olap.h` | v1.8.0 | Cache TTL in milliseconds (default 60 000) |
| `ExporterFactory::createExporter(ExportFormat)` | `analytics_export.h` | v1.8.0 | Dispatches to `ArrowIPCExporter`, `ParquetExporter`, `FeatherExporter`, or `JSONCSVExporter` |
| `WindowConfig::session_expiry_check_interval_ms` | `streaming_window.h` | v1.8.0 | Session-gap detection resolution (default 200 ms) |
| `WindowConfig::global_window_emit_interval_ms` | `streaming_window.h` | v1.8.0 | GLOBAL-window emission latency (default 500 ms) |
| `LLMConfig::max_cache_entries` | `llm_process_analyzer.h` | v1.8.0 | LRU cache cap for LLM response cache (default 1 000) |
| `ForecastModel::predictBatch(series, steps)` | `forecasting.h` | v1.9.0 | Batch forecasting across N independent series |
| `ForecastModel::update(new_value)` | `forecasting.h` | v1.9.0 | O(1) incremental ETS/ARIMA/LR state absorption |

---

## Scope

- API-level enhancements to `include/analytics/` headers — new C++ interfaces for GPU-accelerated analytics pipelines
- New streaming analytics APIs using `ResultStream<T>` as the base type for all progressive-result interfaces
- Incremental materialized view refresh notification interfaces (observer/callback-based)
- Federated analytics query dispatch API with per-tenant data isolation enforced at the interface boundary
- Time-series forecasting extension APIs: SARIMA, Prophet-style, and LSTM surrogate headers
- Statistical analysis and AutoML public header interfaces for feature engineering and model export

## Design Constraints

- [ ] GPU analytics API must be compile-time optional via `THEMIS_ENABLE_GPU` preprocessor guard; all GPU types hidden when undefined
- [ ] Streaming analytics results must use `ResultStream<T>` as the base interface; no ad-hoc callback shapes
- [ ] No blocking calls in public async analytics APIs; all async methods must return `std::future<T>` or equivalent
- [ ] Federated analytics API must enforce per-tenant data isolation at the `SourceRegistry` interface boundary
- [ ] Incremental view refresh notifications must be delivered via an `IncrementalViewObserver` callback interface
- [ ] All new public headers must compile cleanly without GPU runtime headers when `THEMIS_ENABLE_GPU` is not defined

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `OLAPEngine::GPUConfig` | GPU analytics pipeline | Compile-time guarded by `THEMIS_ENABLE_GPU` |
| `ResultStream<T>` | Streaming analytics, CEP engine | Base type for all streaming result APIs |
| `FederatedAnalytics::SourceRegistry` | Federated query engine | Tenant isolation validated at registration |
| `IncrementalViewObserver` | Materialized view consumers | Callback-based refresh notification |
| `ForecastModel` (extended) | SARIMA / Prophet extensions | Extends existing `ForecastMethod` enum |
| `AutoML::ExportInterface` | ONNX export, deployment pipeline | v1.9.0 extension point |

## Planned Features

### GPU-Accelerated Analytics
**Priority:** High  
**Target Version:** v1.7.0

Hardware-accelerated analytics using NVIDIA GPUs and CUDA for massive performance improvements.

**Features:**
- GPU-accelerated aggregations (SUM, AVG, COUNT, etc.)
- CUDA-based window functions
- GPU sort and join operations
- Vectorized operations on GPU
- Multi-GPU support for large datasets

**Supported Operations:**
```cpp
// GPU-accelerated OLAP
OLAPEngine::Config config;
config.enable_gpu = true;
config.gpu_device_id = 0;
config.gpu_memory_limit = 8 * 1024 * 1024 * 1024;  // 8GB

OLAPEngine engine(config);
auto result = engine.execute(query);  // Automatically offloads to GPU
```

**Expected Performance:**
- 10-100x speedup for large aggregations (>1M rows)
- 20-50x faster for complex window functions
- 5-10x faster for GROUP BY with many dimensions
- Especially effective for columnar operations

**Use Cases:**
- Real-time OLAP on large datasets
- Time-series analytics with moving averages
- Complex statistical computations
- Machine learning feature engineering

---

### Advanced Graph Analytics
**Priority:** High  
**Target Version:** v1.7.0

Extended graph analytics algorithms for deep network analysis.

**New Algorithms:**
- **Betweenness Centrality**: Identify bridge nodes
- **Closeness Centrality**: Measure average distance to all nodes
- **Eigenvector Centrality**: Influence-based importance
- **Louvain Community Detection**: Fast modularity optimization
- **Label Propagation**: Semi-supervised community detection
- **Triangle Counting**: Network clustering coefficient
- **K-Core Decomposition**: Core structure analysis
- **Graph Clustering**: Spectral clustering, hierarchical clustering
- **Graph Embedding**: Node2Vec, DeepWalk, GraphSAGE
- **Temporal Graph Analytics**: Dynamic graph analysis over time

**API Example:**
```cpp
#include "analytics/graph_analytics.h"

GraphAnalytics analytics(graph_index);

// Betweenness centrality
auto centrality = analytics.betweennessCentrality();
auto top_nodes = centrality.topK(10);

// Community detection with Louvain
auto communities = analytics.louvainCommunities();
std::cout << "Found " << communities.size() << " communities\n";

// Triangle counting
auto triangles = analytics.countTriangles();
double clustering_coef = analytics.clusteringCoefficient();

// Graph embedding
auto embeddings = analytics.node2vec({
    .dimensions = 128,
    .walk_length = 80,
    .walks_per_node = 10
});
```

**Performance Targets:**
- Betweenness centrality: <500ms for 10K vertices
- Louvain: <1s for 100K vertices
- Triangle counting: <200ms for 10K vertices
- Node2Vec: <5s for 10K vertices (128 dimensions)

---

### Real-Time Anomaly Detection
**Priority:** High  
**Status:** ✅ Implemented (v1.7.0)  
**Target Version:** v1.7.0

ML-based anomaly detection for streaming data and historical analysis.

**Algorithms:**
- **Isolation Forest**: Unsupervised anomaly detection
- **Local Outlier Factor (LOF)**: Density-based outliers
- **One-Class SVM**: Boundary-based detection
- **Autoencoder**: Deep learning anomaly detection
- **Statistical Methods**: Z-score, Modified Z-score, IQR
- **Time-Series Specific**: ARIMA residuals, STL decomposition
- **Ensemble Methods**: Combine multiple detectors

**Features:**
- Real-time anomaly detection on streaming data
- Batch anomaly detection on historical data
- Adaptive learning (model updates with new data)
- Multi-dimensional anomaly detection
- Contextual anomaly detection
- Anomaly explanation (why it's anomalous)

**API Example:**
```cpp
#include "analytics/anomaly_detection.h"

// Train detector on normal data
AnomalyDetector detector(AnomalyMethod::ISOLATION_FOREST);
detector.train(normal_dataset, {
    .contamination = 0.1,  // Expected anomaly rate
    .n_estimators = 100
});

// Real-time detection
auto score = detector.predict(new_data_point);
if (score > 0.7) {  // Threshold
    std::cout << "Anomaly detected! Score: " << score << std::endl;
    auto explanation = detector.explain(new_data_point);
    // explanation contains why it's anomalous
}

// Batch detection
auto anomalies = detector.predictBatch(dataset);
for (const auto& anomaly : anomalies) {
    std::cout << "Anomaly at index " << anomaly.index 
              << " with score " << anomaly.score << std::endl;
}
```

**Integration:**
- Works with CEP engine for real-time detection
- Integrates with OLAP for historical analysis
- Uses VectorIndex for similarity-based detection

---

### Predictive Analytics and Forecasting
**Status:** ✅ Core implemented (v1.7.0) — extended algorithms planned (v1.9.0)  
**Priority:** High  
**Target Version (extensions):** v1.9.0

Core time-series forecasting and predictive modeling is **available now** in `analytics/forecasting.h`.

**Implemented Algorithms (v1.7.0):**
- **LINEAR_REGRESSION**: Ordinary least-squares trend-line extrapolation
- **EXP_SMOOTHING**: Simple exponential smoothing (SES/ETS-ANN)
- **HOLT_WINTERS**: Triple exponential smoothing — additive and multiplicative seasonality
- **ARIMA**: AR(p) + I(d) + MA(q) via Yule–Walker / Levinson–Durbin
- **ENSEMBLE**: Weighted combination of all four models above

**Planned Extensions (v1.9.0):**
- **SARIMA**: Seasonal ARIMA with full seasonal orders (P, D, Q, m)
- **Prophet-style**: Piecewise trend + Fourier seasonality (pure C++, no Python)
- **LSTM surrogate**: Lightweight single-layer recurrence for non-linear patterns

**Implemented Features (v1.7.0):**
- Multi-step ahead forecasting
- Confidence intervals (empirical, Beasley–Springer–Moro quantile approximation)
- Seasonal decomposition (additive / multiplicative)
- Trend analysis
- Automatic hyperparameter tuning (`auto_tune` grid search)
- Forecast accuracy metrics (MAE, RMSE, MAPE, sMAPE)
- Model serialisation / deserialisation (full IEEE-754 precision round-trip)

**API Example (current v1.7.0):**
```cpp
#include "analytics/forecasting.h"
using namespace themisdb::analytics;

// Build time-series
TimeSeries ts;
ts.push(1000LL, 120.0);   // timestamp_ms, value
ts.push(2000LL, 135.0);
// ...

// Configure and train
ForecastConfig cfg;
cfg.seasonality = 12;      // monthly data → yearly season
cfg.alpha = 0.3;
cfg.auto_tune = true;      // grid-search optimal alpha/beta/gamma

ForecastModel model(ForecastMethod::HOLT_WINTERS);
model.fit(ts, cfg);

// Forecast next 30 steps
auto forecast = model.predict(30);
for (const auto& fp : forecast) {
    std::cout << fp.timestamp_ms << ": " << fp.value
              << " [" << fp.lower << ", " << fp.upper << "]\n";
}

// Evaluate accuracy on hold-out set
auto [train, test] = ts.trainTestSplit(0.8);
model.fit(train, cfg);
ForecastMetrics m = model.evaluate(test);
std::cout << "RMSE: " << m.rmse << "  MAPE: " << m.mape << "%\n";

// Seasonal decomposition
auto dr = model.decompose(/*multiplicative=*/false);
// dr.trend, dr.seasonal, dr.residual — each has ts.size() elements

// Serialise / restore
std::string state = model.serialize();
ForecastModel restored = ForecastModel::deserialize(state);
```

**Use Cases:**
- Sales forecasting
- Demand prediction
- Capacity planning
- Resource allocation
- Trend analysis

---

### Advanced Statistical Analysis
**Priority:** Medium  
**Target Version:** v1.8.0

Extended statistical functions and hypothesis testing.

**New Functions:**
- **Descriptive Statistics**: Skewness, kurtosis, mode
- **Hypothesis Tests**: t-test, chi-square, ANOVA, Kolmogorov-Smirnov
- **Correlation Analysis**: Pearson, Spearman, Kendall
- **Regression Analysis**: Linear, logistic, polynomial
- **Distribution Fitting**: Normal, exponential, Weibull
- **Time-Series Tests**: Stationarity (ADF, KPSS), autocorrelation
- **Survival Analysis**: Kaplan-Meier, Cox proportional hazards

**API Example:**
```cpp
#include "analytics/statistics.h"

// Descriptive statistics
auto stats = Statistics::describe(dataset);
std::cout << "Mean: " << stats.mean << std::endl;
std::cout << "Skewness: " << stats.skewness << std::endl;
std::cout << "Kurtosis: " << stats.kurtosis << std::endl;

// Hypothesis testing
auto ttest = Statistics::tTest(sample1, sample2, {
    .alternative = "two-sided",
    .alpha = 0.05
});
if (ttest.p_value < 0.05) {
    std::cout << "Significant difference detected\n";
}

// Correlation analysis
auto corr = Statistics::correlation(x, y, CorrelationMethod::PEARSON);
std::cout << "Correlation: " << corr.coefficient 
          << " (p=" << corr.p_value << ")\n";

// Linear regression
auto model = Statistics::linearRegression(X, y);
std::cout << "R²: " << model.r_squared << std::endl;
auto predictions = model.predict(X_test);
```

---

### Spatial Analytics Enhancements
**Priority:** Medium  
**Target Version:** v1.8.0

Advanced geospatial analytics and operations.

**New Features:**
- **Spatial Clustering**: DBSCAN, OPTICS for geographic clusters
- **Heatmap Generation**: Density-based heatmaps
- **Route Optimization**: Traveling salesman, vehicle routing
- **Spatial Interpolation**: Kriging, IDW
- **Geofencing**: Dynamic boundary management
- **Spatial Joins**: Optimized spatial join operations
- **Grid Analysis**: Tessellation and aggregation
- **Network Analysis**: Road network shortest path, isochrones

**API Example:**
```cpp
#include "analytics/spatial_analytics.h"

SpatialAnalytics spatial(spatial_index);

// Spatial clustering
auto clusters = spatial.dbscan("locations", {
    .eps = 0.5,  // 500m radius
    .min_samples = 5
});

// Heatmap generation
auto heatmap = spatial.generateHeatmap("checkins", {
    .grid_size = 100,  // 100x100 grid
    .bounds = {{-122.5, 37.7}, {-122.3, 37.8}}  // San Francisco
});

// Route optimization (TSP)
std::vector<Point> waypoints = {...};
auto route = spatial.travelingSalesman(waypoints);
std::cout << "Optimal distance: " << route.total_distance << "km\n";

// Spatial interpolation
auto interpolated = spatial.kriging("temperature_sensors", {
    .method = "ordinary",
    .variogram = "spherical"
});
```

---

### AutoML for Analytics
**Priority:** Medium  
**Status:** ✅ Implemented in `src/analytics/automl.cpp` (v0.1.0)  
**Target Version:** v1.9.0 (originally planned; delivered early)

Automated machine learning for analytics tasks.

**Implemented Features:**
- Automatic feature engineering (standard scaling, polynomial expansion)
- Model selection and hyperparameter tuning (random search + k-fold CV)
- Ensemble model generation (soft-voting / mean over top-k)
- Automatic preprocessing (scaling, encoding)
- Model interpretation (permutation-based SHAP approximation)
- Algorithms: Logistic Regression, Linear Regression, Decision Tree, Random Forest, Gradient Boosting, KNN

**Planned Future Enhancements:**
- Bayesian hyperparameter optimisation (TPE/GP)
- Neural architecture search (lightweight)
- Time-series forecasting support
- ONNX export for model portability
- Automatic model deployment pipeline

**API (current):**
```cpp
#include "analytics/automl.h"

using namespace themisdb::analytics;

AutoML automl;

// Train classification model
auto model = automl.trainClassifier(training_data, {
    .target              = "churn",
    .max_time_minutes    = 60,
    .metric              = AutoMLMetric::F1,
    .feature_engineering = true,
    .ensemble            = true
});

// Automatic predictions
auto predictions = model.predict(test_data);

// Explain predictions
auto explanations = model.explain(test_data);
for (const auto& exp : explanations) {
    std::cout << "Top features: " << exp.top_features << std::endl;
}
```

---

### Native Apache Arrow Integration
**Priority:** High  
**Target Version:** v1.7.0

Complete Apache Arrow C++ integration for zero-copy analytics.

**Features:**
- Native Arrow array support
- Arrow Flight RPC for remote analytics
- Zero-copy data sharing with Arrow-based tools
- Arrow Dataset API integration
- Arrow Compute functions
- Plasma object store integration

**API Example:**
```cpp
#include "analytics/arrow_native.h"

// Create Arrow table directly
auto arrow_table = ArrowTable::fromCollection("sales");

// Zero-copy export
auto flight_client = ArrowFlight::connect("localhost:8815");
flight_client->put(arrow_table);  // No serialization overhead

// Use Arrow compute functions
auto filtered = arrow::compute::Filter(arrow_table, filter_expr);
auto aggregated = arrow::compute::GroupBy(filtered, {"region"}, {
    {"total", arrow::compute::sum("amount")}
});
```

**Benefits:**
- 10-100x faster data exchange with Pandas, DuckDB, Spark
- Zero-copy data sharing
- Interoperability with entire Arrow ecosystem

---

### Federated Analytics
**Priority:** Medium  
**Target Version:** v1.9.0

Execute analytics queries across multiple ThemisDB instances or external databases.

**Features:**
- Distributed OLAP queries
- Cross-database joins
- Federated process mining
- Distributed graph analytics
- Query pushdown to remote systems

**API Example:**
```cpp
#include "analytics/federated.h"

FederatedAnalytics fed;

// Register data sources
fed.registerSource("local", local_db);
fed.registerSource("remote1", "themisdb://remote1.example.com");
fed.registerSource("remote2", "postgresql://pg.example.com");

// Execute federated query
auto result = fed.execute(R"(
    SELECT region, SUM(amount) as total
    FROM local.sales s
    JOIN remote1.customers c ON s.customer_id = c.id
    WHERE c.country = 'US'
    GROUP BY region
)");
```

---

### Stream Processing Enhancements
**Priority:** High  
**Status:** ✅ CEP engine implemented (v1.7.0); configurable intervals added (v1.8.0)  
**Target Version:** v1.7.0

Enhanced complex event processing capabilities.

**New Features:**
- **Exactly-once processing**: Transactional event processing
- **Watermarking**: Late data handling
- **State backends**: RocksDB, Redis for state storage
- **Windowed joins**: Join streams with different time characteristics
- **Pattern CEP**: Complex event patterns with quantifiers
- **SQL on streams**: ANSI SQL stream processing

**API Example:**
```cpp
// Watermarking for late data
CEPEngine engine;
engine.setWatermarkStrategy({
    .max_out_of_orderness = std::chrono::seconds(5),
    .idle_timeout = std::chrono::seconds(60)
});

// Windowed join
auto joined = engine.windowJoin(
    stream1, stream2,
    std::chrono::minutes(5),  // 5-minute tumbling window
    [](const Event& e1, const Event& e2) {
        return e1.user_id == e2.user_id;
    }
);

// SQL on streams
auto result = engine.executeSQL(R"(
    SELECT user_id, COUNT(*) as event_count
    FROM event_stream
    WHERE event_type = 'purchase'
    GROUP BY user_id, TUMBLE(event_time, INTERVAL '1' MINUTE)
)");
```

---

### Incremental Materialized Views
**Priority:** High  
**Status:** ✅ Implemented (v1.7.0)  
**Target Version:** v1.7.0

Automatically maintain OLAP aggregations with incremental updates.

**Features:**
- Incremental view refresh on data changes
- CDC-based view maintenance
- View query rewriting (automatic view selection)
- Multi-level materialization (drill-down)
- View staleness tracking

**API Example:**
```cpp
// Create materialized view
OLAPEngine engine;
auto view = engine.createMaterializedView(R"(
    SELECT region, product, SUM(amount) as total
    FROM sales
    GROUP BY region, product
)", {
    .refresh_mode = RefreshMode::INCREMENTAL,
    .refresh_trigger = RefreshTrigger::ON_COMMIT
});

// Automatically maintained on inserts/updates
db.insert("sales", {...});  // View updated incrementally

// Query uses view automatically
auto result = engine.execute(olap_query);  // Rewritten to use view
```

**Benefits:**
- 100-1000x faster query execution (vs. re-aggregation)
- Always up-to-date results
- Automatic query optimization

---

## Performance Optimizations

### Query Compilation (LLVM)
**Priority:** High  
**Target Version:** v1.8.0

JIT-compile analytics queries to native code.

**Features:**
- OLAP query → LLVM IR generation
- Native code compilation
- Automatic compilation threshold
- Cached compiled queries

**Expected Speedup:** 5-20x for CPU-intensive analytics

---

### Adaptive Query Execution
**Priority:** High  
**Target Version:** v1.7.0

Runtime query plan adjustment based on actual statistics.

**Features:**
- Dynamic join reordering
- Adaptive aggregation strategies
- Runtime filter pushdown
- Cardinality re-estimation

**Example:** Automatically switch from hash join to broadcast join if one side is smaller than expected.

---

### Column-Store Optimization
**Priority:** High  
**Target Version:** v1.7.0

Native columnar storage for analytics workloads.

**Features:**
- Columnar compression (RLE, dict, delta)
- Late materialization
- Vectorized scans
- Projection pushdown

**Expected Improvement:** 10-50x faster analytics queries

---

## Integration Enhancements

### Data Science Integration
**Priority:** Medium  
**Target Version:** v1.8.0

Seamless integration with data science tools.

**Features:**
- Python pandas DataFrame integration
- R data.frame integration
- Jupyter notebook support
- MLflow model integration
- SageMaker integration

---

### BI Tool Integration
**Priority:** Medium  
**Target Version:** v1.8.0

Native connectors for business intelligence tools.

**Features:**
- Tableau connector
- Power BI connector
- Looker connector
- Metabase integration
- Apache Superset integration

---

## Research Areas

### Quantum Analytics
**Priority:** Low  
**Target Version:** Research

Explore quantum computing for specific analytics tasks.

**Potential Applications:**
- Quantum machine learning
- Graph optimization problems
- Cryptographic analytics

---

### Neuromorphic Analytics
**Priority:** Low  
**Target Version:** Research

Brain-inspired computing for pattern recognition.

**Potential Applications:**
- Spiking neural networks for anomaly detection
- Event-based processing
- Low-power analytics

---

## Implementation Priorities

### v1.7.0 (Q2 2025)
1. GPU-accelerated analytics
2. Advanced graph analytics
3. Real-time anomaly detection
4. Native Apache Arrow integration
5. Stream processing enhancements
6. Incremental materialized views
7. Adaptive query execution

### v1.8.0 (Q4 2025)
1. Predictive analytics and forecasting
2. Advanced statistical analysis
3. Spatial analytics enhancements
4. Query compilation (LLVM)
5. Data science integration
6. BI tool integration
7. AutoML for analytics

### v1.9.0 (Q2 2026)
1. Federated analytics
2. Column-store optimization
3. Advanced AutoML features
4. Enhanced ML integration

## Contributing

To contribute to these enhancements:

1. **Review the design** in this document
2. **Create an issue** for discussion
3. **Implement** with tests and benchmarks
4. **Document** API and usage
5. **Submit PR** with performance results

## Test Strategy

- Unit tests for each new public header interface in `tests/analytics/` — verify API contracts and compile-time GPU guard behaviour
- GPU API tests gated behind `THEMIS_ENABLE_GPU`; fallback (CPU) path tested without GPU hardware on every CI run
- Streaming result interface tests: verify cancellation, backpressure, and partial consumption without resource leaks
- Federated analytics tenant isolation: negative tests confirming cross-tenant data leakage is rejected at the API boundary
- Mock-based tests for `IncrementalViewObserver` to verify notification timing and single-delivery guarantees
- Integration tests pairing `ForecastModel` extension methods (SARIMA, Prophet) with the existing `TimeSeries` API

## Performance Targets

- GPU analytics API call overhead (host-to-kernel dispatch): ≤ 50 µs per invocation
- Incremental view refresh notification delivery latency: ≤ 1 ms from commit to observer callback
- `ResultStream<T>::next_batch()` overhead (excluding I/O, 1 000-row batch): ≤ 10 µs per call
- Federated query dispatch overhead (local routing decision, no network): ≤ 500 µs
- `ForecastModel::predict()` for SARIMA / Prophet extensions: ≤ 10 ms for a 30-step horizon
- AutoML ONNX export: ≤ 5 s for models trained on datasets up to 100 K rows

## Security / Reliability

- Analytics results must never include raw PII fields; output sanitization enforced at the public API boundary
- Federated analytics API enforces per-tenant data isolation: tenant context validated before any cross-source join is admitted
- GPU kernel errors must surface as typed `Result<T>` error returns — no silent data corruption or undefined behaviour
- `ResultStream<T>` implementations must be safe to cancel from any thread without resource leaks or dangling references
- Forecast model deserialization must validate all numeric parameters (NaN / Inf guards) before use

## See Also

- **Current State**: [`README.md`](./README.md)
- **Implementation**: [`../../src/analytics/README.md`](../../src/analytics/README.md)
- **Query Module**: [`../query/FUTURE_ENHANCEMENTS.md`](../query/FUTURE_ENHANCEMENTS.md)
- **Index Module**: [`../index/FUTURE_ENHANCEMENTS.md`](../index/FUTURE_ENHANCEMENTS.md)
