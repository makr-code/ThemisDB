---
title: "Replace LLM Grafana Metrics Stub Implementation"
labels: llm, metrics, observability, monitoring, priority-medium
milestone: v1.4.0
---

## 📋 Summary

The Grafana metrics implementation for LLM monitoring is currently a **STUB** that returns dummy data. This prevents proper monitoring, alerting, and observability of LLM operations in production.

**Type**: Stub Replacement / Observability  
**Priority**: MEDIUM (production monitoring)  
**Effort**: 1-2 weeks  
**Status**: ❌ Stub Implementation (19 instances verified)

## 🔍 Verification

**Files with STUB implementations**:
1. `src/llm/grafana_metrics_stub.cpp` - Entire file is stub
2. `src/llm/grafana_metrics.cpp` - Contains stub markers

### Evidence from `grafana_metrics_stub.cpp`:

```cpp
// Line 11
spdlog::debug("PrometheusExporter initialized (STUB)");

// Line 39
spdlog::debug("LLMMetricsCollector initialized (STUB)");

// Line 76
spdlog::debug("GrafanaDashboardGenerator initialized (STUB)");

// Line 119
spdlog::debug("MetricsServer initialized (STUB)");

// Line 128
spdlog::info("Metrics Server started (STUB): {}:{}", config_.host, config_.port);

// Line 134
spdlog::info("Metrics Server stopped (STUB)");
```

### Evidence from `grafana_metrics.cpp`:

```cpp
// Line 13
spdlog::debug("PrometheusExporter initialized (STUB)");

// Line 44
return "# HELP themis_llm_metrics ThemisDB LLM Metrics (STUB)\n"
       "# TYPE themis_llm_metrics gauge\n";  // ← Empty metrics

// Line 60
spdlog::debug("LLMMetricsCollector initialized (STUB)");

// Line 91
spdlog::debug("GrafanaDashboardGenerator initialized (STUB)");

// Line 134
spdlog::debug("MetricsServer initialized (STUB)");

// Line 143
spdlog::info("Metrics Server started (STUB): {}:{}", config_.host, config_.port);

// Line 149
spdlog::info("Metrics Server stopped (STUB)");
```

## 🎯 Problem Statement

### Current State: No Real Monitoring

❌ **No real metrics collection** - all values are hardcoded  
❌ **No Prometheus endpoint** - metrics server is a stub  
❌ **No Grafana dashboards** - dashboard generation is a stub  
❌ **No alerting possible** - no real data to alert on  
❌ **No production observability** - blind to LLM performance issues

### Impact

Without real metrics, operators cannot:
- Monitor LLM inference latency
- Track token generation rate
- Detect memory leaks or OOM conditions
- Monitor model loading times
- Track cache hit rates
- Set up alerts for performance degradation
- Diagnose production issues

## 📊 Required Metrics

### Core LLM Metrics

#### Request Metrics
- `themis_llm_requests_total` (counter) - Total inference requests
- `themis_llm_requests_active` (gauge) - Currently active requests
- `themis_llm_request_duration_seconds` (histogram) - Request latency
- `themis_llm_request_errors_total` (counter) - Failed requests

#### Token Metrics
- `themis_llm_tokens_generated_total` (counter) - Total tokens generated
- `themis_llm_tokens_per_second` (gauge) - Generation rate
- `themis_llm_prompt_tokens_total` (counter) - Prompt tokens
- `themis_llm_completion_tokens_total` (counter) - Completion tokens

#### Model Metrics
- `themis_llm_model_loading_duration_seconds` (histogram) - Model load time
- `themis_llm_model_memory_bytes` (gauge) - Model memory usage
- `themis_llm_models_loaded` (gauge) - Number of loaded models

#### Cache Metrics
- `themis_llm_cache_hits_total` (counter) - Cache hits
- `themis_llm_cache_misses_total` (counter) - Cache misses
- `themis_llm_cache_hit_rate` (gauge) - Hit rate percentage
- `themis_llm_cache_size_bytes` (gauge) - Cache memory usage

#### GPU Metrics
- `themis_llm_gpu_utilization` (gauge) - GPU usage %
- `themis_llm_gpu_memory_used_bytes` (gauge) - GPU memory used
- `themis_llm_gpu_memory_total_bytes` (gauge) - GPU memory total

#### Queue Metrics
- `themis_llm_queue_size` (gauge) - Pending requests
- `themis_llm_queue_wait_duration_seconds` (histogram) - Queue wait time

## 🏗️ Proposed Implementation

### Phase 1: Prometheus Exporter (Real Implementation)

```cpp
class PrometheusExporter {
private:
    // Prometheus metrics
    prometheus::Counter* requests_total_;
    prometheus::Gauge* requests_active_;
    prometheus::Histogram* request_duration_;
    prometheus::Counter* tokens_generated_;
    prometheus::Gauge* tokens_per_second_;
    prometheus::Gauge* cache_hit_rate_;
    
    prometheus::Registry registry_;
    std::shared_ptr<prometheus::Exposer> exposer_;
    
public:
    PrometheusExporter(const PrometheusConfig& config) {
        // Create Prometheus exposer (HTTP server)
        exposer_ = std::make_shared<prometheus::Exposer>(
            fmt::format("{}:{}", config.host, config.port)
        );
        
        // Register metrics
        auto& requests_family = prometheus::BuildCounter()
            .Name("themis_llm_requests_total")
            .Help("Total number of LLM requests")
            .Register(registry_);
        requests_total_ = &requests_family.Add({});
        
        auto& active_family = prometheus::BuildGauge()
            .Name("themis_llm_requests_active")
            .Help("Number of active LLM requests")
            .Register(registry_);
        requests_active_ = &active_family.Add({});
        
        // ... register all other metrics
        
        // Expose metrics
        exposer_->RegisterCollectable(registry_.shared_from_this());
        
        spdlog::info("PrometheusExporter started on {}:{}", config.host, config.port);
    }
    
    void incrementRequests() {
        requests_total_->Increment();
    }
    
    void setActiveRequests(int count) {
        requests_active_->Set(count);
    }
    
    void observeRequestDuration(double seconds) {
        request_duration_->Observe(seconds);
    }
    
    // ... implement all metric recording methods
};
```

### Phase 2: Metrics Collector (Integration)

```cpp
class LLMMetricsCollector {
private:
    PrometheusExporter* exporter_;
    std::shared_ptr<InferenceEngine> inference_engine_;
    
public:
    void recordInferenceRequest(const InferenceRequest& req, const InferenceResponse& resp, double duration) {
        // Record to Prometheus
        exporter_->incrementRequests();
        exporter_->observeRequestDuration(duration);
        exporter_->addTokensGenerated(resp.tokens_generated);
        
        // Calculate tokens/second
        double tokens_per_sec = resp.tokens_generated / duration;
        exporter_->setTokensPerSecond(tokens_per_sec);
    }
    
    void updateCacheMetrics() {
        auto cache_stats = inference_engine_->getCacheStats();
        exporter_->setCacheHitRate(cache_stats.hit_rate);
        exporter_->setCacheSizeBytes(cache_stats.size_bytes);
    }
    
    void updateGPUMetrics() {
        auto gpu_stats = inference_engine_->getGPUStats();
        exporter_->setGPUUtilization(gpu_stats.utilization);
        exporter_->setGPUMemoryUsed(gpu_stats.memory_used);
    }
};
```

### Phase 3: Grafana Dashboard Generator

```cpp
class GrafanaDashboardGenerator {
public:
    nlohmann::json generateDashboard() {
        return {
            {"dashboard", {
                {"title", "ThemisDB LLM Metrics"},
                {"panels", {
                    // Panel 1: Requests per second
                    {
                        {"title", "Requests per Second"},
                        {"type", "graph"},
                        {"targets", {{
                            {"expr", "rate(themis_llm_requests_total[5m])"}
                        }}}
                    },
                    // Panel 2: Tokens per second
                    {
                        {"title", "Tokens per Second"},
                        {"type", "graph"},
                        {"targets", {{
                            {"expr", "themis_llm_tokens_per_second"}
                        }}}
                    },
                    // Panel 3: Cache hit rate
                    {
                        {"title", "Cache Hit Rate"},
                        {"type", "gauge"},
                        {"targets", {{
                            {"expr", "themis_llm_cache_hit_rate"}
                        }}}
                    },
                    // ... more panels
                }}
            }}
        };
    }
};
```

## 📝 Implementation Tasks

### Milestone 1: Prometheus Integration (Week 1)

- [ ] Remove stub implementations
- [ ] Integrate prometheus-cpp library
- [ ] Implement PrometheusExporter with all metrics
- [ ] Implement metrics HTTP endpoint
- [ ] Add unit tests
- [ ] Verify metrics endpoint works with Prometheus

### Milestone 2: Metrics Collection (Week 1)

- [ ] Integrate with InferenceEngine
- [ ] Record inference requests
- [ ] Record token generation
- [ ] Record cache metrics
- [ ] Record GPU metrics
- [ ] Add integration tests

### Milestone 3: Grafana Dashboards (Week 2)

- [ ] Implement dashboard JSON generation
- [ ] Create default LLM dashboard
- [ ] Add dashboard for cache metrics
- [ ] Add dashboard for GPU metrics
- [ ] Document dashboard import
- [ ] Add example alerts

### Milestone 4: Documentation & Testing (Week 2)

- [ ] Update documentation
- [ ] Add Grafana setup guide
- [ ] Add Prometheus configuration examples
- [ ] Add example alert rules
- [ ] Performance testing
- [ ] Metrics accuracy validation

## 🔗 Dependencies & Related Issues

### Dependencies
- prometheus-cpp library (add to vcpkg)
- Prometheus server (deployment)
- Grafana server (deployment)

### Related
- LLM Integration (provides data for metrics)
- Cache implementation (provides cache stats)

## 📊 Success Criteria

### Functional Requirements
- ✅ All stub code removed
- ✅ Real Prometheus metrics exported
- ✅ Metrics endpoint accessible on configured port
- ✅ All 15+ metrics implemented
- ✅ Grafana dashboards generated

### Technical Metrics
- ✅ Metrics collection overhead < 1% CPU
- ✅ Metrics scraping time < 100ms
- ✅ Dashboard loads in < 2 seconds
- ✅ Historical data retention works

### Quality Gates
- ✅ Unit tests passing
- ✅ Integration tests with Prometheus
- ✅ Metrics accuracy verified
- ✅ Dashboards tested in Grafana
- ✅ Documentation complete

## 📅 Timeline Estimate

| Milestone | Duration | Deliverable |
|-----------|----------|-------------|
| Prometheus Integration | 3-4 days | Metrics endpoint |
| Metrics Collection | 2-3 days | Full instrumentation |
| Grafana Dashboards | 2-3 days | Dashboards + alerts |
| Documentation & Testing | 2 days | Production ready |
| **Total** | **1-2 weeks** | **Feature complete** |

## ✅ Definition of Done

- [ ] All 19 stub markers removed
- [ ] Real Prometheus metrics exported
- [ ] All 15+ metrics implemented
- [ ] Metrics endpoint accessible
- [ ] Grafana dashboards generated
- [ ] Example alerts provided
- [ ] Unit tests passing
- [ ] Integration tests with Prometheus
- [ ] Documentation complete
- [ ] Code review approved

---

**Created**: 2026-01-11  
**Verified**: 2026-01-11 (19 stub markers confirmed)  
**Target Version**: v1.4.0  
**Priority**: MEDIUM  
**Component**: LLM / Observability
