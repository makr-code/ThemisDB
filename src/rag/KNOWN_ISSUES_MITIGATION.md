---
AI_Used: true
AI_Models: unknown
AI_Contributor: "ThemisDB documentation orchestrator"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# RAG Known Issues — Mitigation Strategies

This document records known implementation gaps and possible mitigations. The
procedures below are not evidence of production capability; current code state
is called out where it differs from the original remediation notes.

## Issue 1: RocksDB Availability (Phases 8, 10)

### Problem
`CostAttributionTracker` retains records in memory. Its `void* db_` is not an
opened RocksDB handle, and cost records are not persisted.

### Current Status
- `Initialize(db_path)` creates the directory and sets an availability flag; it
  does not open RocksDB or write/read records.
- `IsPersistentStorageAvailable()` is not proof of durable storage.
- `RecordCost()` and cost queries currently use in-memory records.

### Production Mitigation

#### 1. Do not rely on `Initialize()` for persistence

```cpp
auto tracker = std::make_unique<CostAttributionTracker>();

// This only checks/creates a directory; it does not enable durable storage.
bool success = tracker->Initialize("/var/lib/themis/cost_db");
// Even when success is true, the current tracker remains in-memory only.
```

#### 2. Persistence remains an implementation gap

The current API cannot validate a RocksDB installation, enable persistence,
or guarantee behavior across restart. Durable cost accounting requires a real
storage implementation and corresponding tests before deployment reliance.

**Proposed schema only (not implemented):**
```
Key:        "cost:<tenant_id>:<timestamp_us>"
Value:      Binary-encoded CostRecord (operation, cost_usd, attributes)
```

---

## Issue 2: OTLP Sender Implementation (Phase 8)

### Problem
`OTELSpanEmitter` buffers spans and creates a local serialized JSON payload,
but does not transmit it to an OTLP collector.

### Current Status
- Span creation and attribute recording are implemented.
- `ExportSpans()` serializes buffered data locally and updates local state;
  it does not perform OTLP network export.
- Statistics are in-process counters and do not acknowledge collector receipt.

### Future integration work

#### 1. OTLP Exporter Integration

**Example direction only; no exporter is currently implemented:**

```cpp
// In OTELSpanEmitter constructor:
#include <opentelemetry/exporters/otlp/otlp_grpc_exporter.h>
#include <opentelemetry/sdk/trace/batch_span_processor.h>
#include <opentelemetry/sdk/trace/tracer_provider.h>

namespace exporter = opentelemetry::exporter::otlp;
namespace trace = opentelemetry::trace;

auto exporter = std::make_unique<exporter::OtlpGrpcExporter>(
    exporter::OtlpGrpcExporterOptions{
        .endpoint = otlp_endpoint_,  // e.g., "localhost:4317"
        .use_ssl = false,            // Set to true in production
        .timeout_millis = 10000      // 10-second timeout
    }
);

otel_exporter_ = exporter.release();  // Store for later use
```

**Alternative proposal only:**

```cpp
// Alternative: Direct Jaeger export
auto exporter = std::make_unique<jaeger::JaegerExporter>();
exporter->SetAgentHost("jaeger-agent");
exporter->SetAgentPort(6831);
otel_exporter_ = exporter.release();
```

#### 2. Batch Export Implementation

**Future work:** `src/rag/otel_span_emitter.cpp` needs a real OTLP exporter
and delivery/error semantics before it can export spans.

The following is an implementation sketch only; it is not the current API or
behavior:

```cpp
bool OTELSpanEmitter::ExportSpans() {
  if (pending_span_buffer_.empty()) {
    return true;
  }

  // 1. Serialize spans to OTLP protobuf format
  auto resource_spans = opentelemetry::proto::trace::v1::ResourceSpans();
  // ... populate with pending_span_buffer_ entries ...

  // 2. Create gRPC request
  auto request = opentelemetry::proto::collector::trace::v1::ExportTraceServiceRequest();
  request.add_resource_spans()->CopyFrom(resource_spans);

  // 3. Send via gRPC to OTLP collector
  opentelemetry::proto::collector::trace::v1::ExportTraceServiceResponse response;
  auto status = stub_->Export(&context, request, &response);

  if (!status.ok()) {
    dropped_spans_ += pending_span_buffer_.size();
    LOG(WARNING) << "OTLP export failed: " << status.error_message();
    // Optionally: retry logic or fallback
  } else {
    emitted_spans_ += pending_span_buffer_.size();
  }

  pending_span_buffer_.clear();
  return status.ok();
}
```

#### Example configuration (buffering only)

```cpp
auto tracer = std::make_unique<OTELSpanEmitter>(
    "RAG",                           // Service name
    "localhost:4317"                 // OTLP gRPC endpoint
);

tracer->SetBatchExportEnabled(true);
tracer->SetBatchExportSize(100);     // Local serialization at batch threshold
// No timer-driven flush or network delivery is currently implemented.
```

The following are proposed telemetry attributes, not fields automatically
exported by the current implementation:

Every span includes standard attributes:
- `rag.operation_type`: "retrieve", "rerank", "refresh", etc.
- `rag.tenant_id`: Multi-tenant identifier
- `rag.query_id`: Unique query identifier
- `rag.latency_us`: End-to-end latency
- `rag.document_count`: Number of documents retrieved
- `rag.cost_usd`: Per-operation cost
- `rag.sla_status`: SLA compliance (success/timeout/error)

Delivery retry, network timeout, and collector-unavailable behavior are
unimplemented until a transport is added.

---

## Issue 3: Cost Model Drift (Phase 10)

### Problem
`CostModelBuilder` has no scheduled retraining loop. In the current source,
`BuildModel()` uses a mean-cost intercept and placeholder metrics;
`IsModelDriftDetected()` returns `false`, and `GetModelHealth()` reports
placeholder drift values.

### Current Status
- Model versioning metadata added: `version`, `built_at_sec`, `retrains_count`
- `EnableAutoRetraining()` stores configuration values; it does not schedule checks or retraining.
- `IsModelDriftDetected()` is a placeholder and currently returns `false`.
- `RebuildModelWithNewData()` explicitly calls `BuildModel()`; callers must initiate it.
- `GetModelHealth()` returns placeholder current RMSE and drift values.

### Future orchestration proposal

The following schedules and code sketches are design suggestions only. They
cannot be enabled by `EnableAutoRetraining()` in the current implementation.

#### 1. Configuration fields (not an automatic loop)

```cpp
auto builder = std::make_unique<CostModelBuilder>();
builder->AddTrainingData(historical_cost_data);

// This only stores the values; no background check is started.
builder->EnableAutoRetraining(
    true,               // Enable auto-retraining
    3600,               // Check drift every 1 hour
    0.15f               // Trigger retrain if RMSE increases > 15%
);

auto model = builder->BuildModel("linear", 0.01f);
```

#### 2. Suggested retraining schedule

**Recommended schedule based on throughput:**

| Throughput | Interval | Drift Threshold | Retraining Window |
|------------|----------|-----------------|-------------------|
| < 100 q/sec | 24 hours | 20% | Nightly (2-4 AM UTC) |
| 100-1K q/sec | 6 hours | 15% | Every 6 hours |
| > 1K q/sec | 1 hour | 10% | Every 1 hour (off-peak) |

#### 3. Drift detection (future implementation)

**File: `src/rag/cost_model_builder.cpp:300-310` (IsModelDriftDetected method)**

Replace TODO with:

```cpp
bool CostModelBuilder::IsModelDriftDetected(
    const std::vector<DataPoint>& new_test_data) {
  if (new_test_data.empty() || last_model_rmse_ <= 0.0f) {
    return false;
  }

  // Calculate RMSE on recent data using current model
  float sum_squared_error = 0.0f;
  for (const auto& data_point : new_test_data) {
    // Note: Requires access to the current deployed model
    // This is typically done by the CostModelRegistry or similar
    // float predicted = GetCurrentModel()->Predict(data_point.features);
    // float error = data_point.cost_usd - predicted;
    // sum_squared_error += error * error;
  }

  // float current_rmse = std::sqrt(sum_squared_error / new_test_data.size());
  // float drift_ratio = (current_rmse - last_model_rmse_) / last_model_rmse_;

  // return drift_ratio > auto_retrain_drift_threshold_;
  
  // Production: Use actual model and calculate drift
  return false;  // Placeholder
}
```

#### 4. Retraining workflow (external orchestration required)

**Recommended: Background job every N hours**

```python
# Pseudo-code for orchestration (Python/Kubernetes)
while True:
    sleep(RETRAIN_INTERVAL_HOURS)
    
    # 1. Fetch recent cost data (last 24 hours)
    new_cost_data = fetch_cost_data(hours=24)
    
    # 2. Check drift
    if builder.IsModelDriftDetected(new_cost_data):
        LOG.info("Drift detected, retraining model")
        
        # 3. Rebuild with new data
        new_model = builder.RebuildModelWithNewData(new_cost_data)
        
        # 4. Evaluate on holdout test set
        test_rmse = new_model.Evaluate(test_data)
        old_rmse = get_current_model_rmse()
        
        # 5. A/B test: shadow mode
        if is_improvement(test_rmse, old_rmse):
            # 6. Deploy to production
            deploy_model_version(new_model)
            LOG.info(f"Model v{metadata['version']} deployed")
        else:
            LOG.warning(f"Model not improved, staying on v{current_version}")
```

#### 5. Model Versioning

Track all model versions for rollback:

```
Model versions in production:
  v1: Built 2026-09-24 08:00 UTC (RMSE: 0.042)
  v2: Built 2026-09-24 14:00 UTC (RMSE: 0.038) <- Current
  v3: Built 2026-09-25 08:00 UTC (RMSE: 0.041) <- Performance degraded, rolled back
```

Get version info:

```cpp
auto metadata = builder->GetModelMetadata();
std::cout << "Version: " << metadata["version"]
          << " (built " << metadata["built_at_sec"]
          << ", " << metadata["retrains_count"] << " retrains)" << std::endl;
```

#### 6. Health Monitoring

Expose model health metrics for observability:

```cpp
auto health = builder->GetModelHealth();
// health["model_age_sec"] = 3600         // 1 hour old
// health["last_rmse"] = 0.038            // Model RMSE
// health["current_rmse"] = 0.045         // Current error rate
// health["drift_ratio"] = 0.18           // 18% degradation (> 15% threshold)
```

**Alerting rules:**

| Metric | Threshold | Action |
|--------|-----------|--------|
| `model_age_sec` | > 7 days | Page on-call (model not retraining) |
| `drift_ratio` | > 0.2 (20%) | Trigger immediate retrain |
| `current_rmse` | > 2x baseline | Manual review required |

#### 7. Deployment Checklist

- [ ] Implement background retraining job (cron or Kubernetes CronJob)
- [ ] Store all model versions in model registry (e.g., MLflow, HuggingFace Hub)
- [ ] Add model evaluation step before deployment
- [ ] Implement shadow mode (run new model alongside current for validation)
- [ ] Add rollback mechanism (quick switch to previous version)
- [ ] Expose drift metrics to monitoring system (Prometheus, Datadog)
- [ ] Set up alerting on drift threshold exceeded
- [ ] Document retraining SLA (max 15 minutes end-to-end)
- [ ] Test retraining in staging before production enablement

#### 8. Incremental Training Example

```cpp
// Initial training
std::vector<CostModelBuilder::DataPoint> initial_data = LoadHistoricalCosts();
builder.AddTrainingData(initial_data);
auto model = builder.BuildModel();

// Later: New data arrives
std::vector<CostModelBuilder::DataPoint> new_data = LoadRecentCosts();
auto updated_model = builder.RebuildModelWithNewData(new_data);

// Compare quality
float old_quality = model->Evaluate(test_data);
float new_quality = updated_model->Evaluate(test_data);

if (new_quality > old_quality) {
    // Deploy new model
} else {
    // Keep old model, investigate why new data degraded it
}
```

---

## Summary: Production Readiness Checklist

### Phase 8 (Realtime SLO & Cost Tracking)
- [ ] RocksDB initialized with persistent storage
- [ ] Cost attribution data backed up daily
- [ ] OTLP exporter connected and batch export verified
- [ ] Span export metrics monitored

### Phase 10 (Cost Model Recommendation Engine)
- [ ] Auto-retraining enabled with appropriate schedule
- [ ] Model versions tracked in registry
- [ ] Drift detection tested with synthetic cost changes
- [ ] Rollback procedure documented and tested
- [ ] Monitoring alerts configured for model degradation

### Cross-Cutting
- [ ] All three components integrated into deployment CI/CD
- [ ] Load test with realistic volume (1-10K queries/sec)
- [ ] Disaster recovery tested (RocksDB loss, OTLP collector down, model data loss)
- [ ] Documentation updated for on-call runbooks

---

## References

- RAG_READINESS_AUDIT_2026-09-23.md: Initial audit findings
- BATCH_6_INTEGRATION_SUMMARY.md: Batch 6 implementation summary
- src/rag/ROADMAP.md: Phase 7-10 completion status
- OpenTelemetry C++ SDK: https://github.com/open-telemetry/opentelemetry-cpp
- RocksDB Getting Started: https://github.com/facebook/rocksdb/wiki/Getting-Started-with-RocksDB
