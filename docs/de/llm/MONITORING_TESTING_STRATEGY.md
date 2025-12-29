# Monitoring & Testing Strategy für ThemisDB Native LLM Integration

## Überblick

Diese Anleitung beschreibt umfassende Strategien für:
1. **Grafana Dashboards** (6 Dashboard-Suite)
2. **Inter-Cerebrale Überwachung** (Brain-inspired Multi-Shard Communication)
3. **Prometheus Metrics** (40+ Metriken)
4. **Testing Pyramid** (Unit → Integration → E2E)
5. **Performance Benchmarks** (CPU vs GPU, Single vs Multi-Shard)
6. **CI/CD Integration** (Automated Testing Pipeline)

Ziel: **99.9% Uptime, <2s p95 Latency, vollständige Observability des verteilten AI-Systems**

---

## 1. Grafana Dashboard Suite

### Dashboard 1: Cluster Overview

**Zweck:** Gesamtübersicht aller Shards und Cluster-Health

**Panels:**

```yaml
Panel: Total Throughput
  Type: Graph (time series)
  Metrics:
    - sum(rate(themis_inference_requests_total[5m]))
    - sum(rate(themis_vector_search_queries_total[5m]))
  Y-Axis: Requests/second
  Threshold: Red <5 req/s, Yellow <10 req/s, Green >=10 req/s

Panel: Aggregate Latency
  Type: Graph (time series)
  Metrics:
    - histogram_quantile(0.50, themis_inference_latency_seconds)
    - histogram_quantile(0.95, themis_inference_latency_seconds)
    - histogram_quantile(0.99, themis_inference_latency_seconds)
  Y-Axis: Seconds
  Threshold: Red >2s (p95), Yellow >1s (p95), Green <1s (p95)

Panel: Shard Health Heatmap
  Type: Heatmap
  Metrics: themis_shard_health_status{shard=~".*"}
  Values: 1=healthy, 0.5=degraded, 0=unhealthy
  Colors: Green → Yellow → Red

Panel: CPU/GPU Utilization
  Type: Graph (stacked area)
  Metrics:
    - themis_cpu_usage_percent{shard=~".*"}
    - themis_gpu_utilization_percent{shard=~".*"}
  Y-Axis: Percentage (0-100%)
  Threshold: Red >95%, Yellow >80%, Green <80%

Panel: Memory Usage (RAM + VRAM)
  Type: Gauge
  Metrics:
    - themis_ram_usage_bytes{shard=~".*"} / themis_ram_total_bytes
    - themis_vram_usage_bytes{shard=~".*"} / themis_vram_total_bytes
  Format: Percentage
  Threshold: Red >90%, Yellow >75%, Green <75%

Panel: Network Traffic (Inter-Shard)
  Type: Graph (time series)
  Metrics:
    - rate(themis_network_bytes_sent_total[5m])
    - rate(themis_network_bytes_received_total[5m])
  Y-Axis: MB/s
  Split by: source_shard, destination_shard

Panel: Active Connections
  Type: Single Stat (per shard)
  Metrics: themis_active_connections{shard=~".*"}
  Threshold: Red >100, Yellow >50, Green <50
```

**Alerts:**
```yaml
Alert: Shard Down
  Condition: themis_shard_health_status == 0
  Duration: 30s
  Severity: Critical
  Message: "Shard {{$labels.shard}} is unhealthy!"

Alert: High Latency
  Condition: histogram_quantile(0.95, themis_inference_latency_seconds) > 2
  Duration: 2m
  Severity: Warning
  Message: "p95 latency >2s on {{$labels.shard}}"

Alert: VRAM Threshold
  Condition: themis_vram_usage_bytes / themis_vram_total_bytes > 0.90
  Duration: 1m
  Severity: Warning
  Message: "VRAM >90% on {{$labels.shard}}"

Alert: GPU Degraded
  Condition: themis_gpu_utilization_percent < 20 and rate(themis_inference_requests_total[5m]) > 1
  Duration: 5m
  Severity: Warning
  Message: "GPU underutilized despite active requests on {{$labels.shard}}"
```

---

### Dashboard 2: Inter-Cerebral Communication

**Zweck:** Brain-inspired Überwachung der Shard-zu-Shard Kommunikation (analog zu Gehirnregionen)

**Panels:**

```yaml
Panel: LoRA Transfer Activity
  Type: Sankey Diagram
  Metrics:
    - themis_lora_transfers_total{source_shard=~".*", destination_shard=~".*"}
  Flow: source_shard → destination_shard
  Width: Proportional to transfer volume (MB)

Panel: Shard Collaboration Matrix
  Type: Heatmap (2D)
  Metrics: rate(themis_cross_shard_queries_total[10m])
  X-Axis: Source Shard
  Y-Axis: Destination Shard
  Colors: 0 (Blue) → High (Red)

Panel: Federated RAG Query Flow
  Type: Graph (Directed graph visualization)
  Nodes: Shards (legal, finance, medical, etc.)
  Edges: Query flow volume
  Edge Width: Proportional to queries/second

Panel: LoRA Cache Hit Rate
  Type: Graph (time series, per shard)
  Metrics: themis_lora_cache_hit_rate{shard=~".*"}
  Y-Axis: Percentage (0-100%)
  Target: >70% (warm cache)

Panel: Consensus Latency (Multi-Perspective)
  Type: Histogram
  Metrics: themis_multi_perspective_consensus_latency_seconds
  Buckets: 0-100ms, 100-500ms, 500ms-1s, 1s-5s, >5s
  Target: <500ms for 90% of queries

Panel: Orchestrator Task Distribution
  Type: Pie Chart
  Metrics: sum by (domain) (rate(themis_orchestrator_tasks_total[10m]))
  Slices: legal, finance, medical, technical, other

Panel: Cross-Shard Message Rate
  Type: Graph (time series)
  Metrics: rate(themis_cross_shard_messages_total[5m])
  Split by: message_type (lora_request, federated_query, consensus_vote)

Panel: Network Topology Health
  Type: Single Stat (Status)
  Metrics: count(themis_shard_health_status == 1) / count(themis_shard_health_status)
  Format: Percentage
  Threshold: Red <80%, Yellow <95%, Green >=95%
```

**Brain-Inspired Visualization:**
```
Orchestrator (Prefrontal Cortex)
       │
       ├─── Legal Shard (Wernicke's Area - Language)
       │         └─── LoRA: Legal Specialist
       │
       ├─── Finance Shard (Parietal Lobe - Numbers)
       │         └─── LoRA: Finance Analyst
       │
       ├─── Medical Shard (Temporal Lobe - Memory/Context)
       │         └─── LoRA: Medical Specialist
       │
       └─── Technical Shard (Motor Cortex - Action)
                 └─── LoRA: Code Generator

Communication Patterns:
  - Query: User → Orchestrator → Relevant Shards
  - LoRA Sharing: Shard A ⇄ Shard B (bidirectional)
  - Consensus: Multiple Shards → Orchestrator → Fused Result
```

---

### Dashboard 3: LLM Performance

**Zweck:** Detaillierte LLM Inference Metriken

**Panels:**

```yaml
Panel: Tokens per Second
  Type: Graph (time series, per shard)
  Metrics: themis_tokens_generated_per_second{shard=~".*"}
  Y-Axis: Tokens/second
  Threshold: Red <20, Yellow <50, Green >=50

Panel: Batch Size Utilization (Continuous Batching)
  Type: Heatmap (over time)
  Metrics: themis_batch_size_current{shard=~".*"}
  Values: 1-32 (batch size)
  Target: 8-16 (optimal GPU utilization)

Panel: KV Cache Efficiency (PagedAttention)
  Type: Graph (time series)
  Metrics:
    - themis_kv_cache_utilization_percent
    - themis_kv_cache_miss_rate
  Y-Axis: Percentage
  Target: >70% utilization, <20% miss rate

Panel: LoRA Adapter Switching Time
  Type: Histogram
  Metrics: themis_lora_switch_latency_milliseconds
  Buckets: 0-50ms, 50-100ms, 100-200ms, >200ms
  Target: <100ms for 95% of switches

Panel: Model Loading Time (Cold Start)
  Type: Graph (time series)
  Metrics: themis_model_load_latency_seconds
  Y-Axis: Seconds
  Typical: 5-15s for 7B models, 30-60s for 70B models

Panel: Inference Latency Breakdown
  Type: Stacked Bar Chart
  Metrics:
    - themis_inference_encode_latency_ms
    - themis_inference_forward_latency_ms
    - themis_inference_decode_latency_ms
  Y-Axis: Milliseconds
  Identify bottlenecks

Panel: Active LoRA Adapters
  Type: Single Stat (per shard)
  Metrics: themis_active_loras_count{shard=~".*"}
  Threshold: Warning if >8 (memory pressure)

Panel: Model Type Distribution
  Type: Pie Chart
  Metrics: sum by (model_name) (themis_inference_requests_total)
  Slices: mistral-7b, llama-3-8b, codellama-13b, phi-3-mini, etc.
```

**Alerts:**
```yaml
Alert: Low Token Generation
  Condition: themis_tokens_generated_per_second < 20
  Duration: 2m
  Severity: Warning
  Message: "Token generation <20 tokens/s on {{$labels.shard}}"

Alert: High KV Cache Miss Rate
  Condition: themis_kv_cache_miss_rate > 0.30
  Duration: 5m
  Severity: Warning
  Message: "KV cache miss rate >30% on {{$labels.shard}}"

Alert: Slow LoRA Switching
  Condition: histogram_quantile(0.95, themis_lora_switch_latency_milliseconds) > 200
  Duration: 2m
  Severity: Warning
  Message: "LoRA switching >200ms (p95) on {{$labels.shard}}"
```

---

### Dashboard 4: Vector Search (FAISS GPU)

**Zweck:** FAISS GPU Performance Monitoring

**Panels:**

```yaml
Panel: Queries per Second (QPS)
  Type: Graph (time series)
  Metrics: rate(themis_vector_search_queries_total[5m])
  Y-Axis: Queries/second
  Target: >200 QPS (GPU), >10 QPS (CPU)

Panel: Search Latency Distribution
  Type: Heatmap (over time)
  Metrics: themis_vector_search_latency_milliseconds
  Buckets: 0-5ms, 5-10ms, 10-50ms, 50-100ms, >100ms
  Target: <10ms for 95% of queries (GPU)

Panel: Index Size in VRAM
  Type: Gauge
  Metrics: themis_faiss_index_size_bytes
  Format: GB
  Threshold: Warning if >80% of available VRAM

Panel: Recall@10 Accuracy
  Type: Graph (time series)
  Metrics: themis_vector_search_recall_at_10
  Y-Axis: Percentage (0-100%)
  Target: >80% (acceptable), >90% (good)

Panel: Batch Processing Efficiency
  Type: Graph (time series)
  Metrics:
    - themis_vector_search_batch_size_avg
    - themis_vector_search_batch_latency_per_query_ms
  Y-Axis: Batch size / Latency per query
  Target: Higher batch size → Lower latency per query

Panel: Index Operations
  Type: Counter
  Metrics:
    - rate(themis_faiss_index_add_total[10m])
    - rate(themis_faiss_index_remove_total[10m])
  Y-Axis: Operations/second

Panel: GPU Memory Usage (FAISS-specific)
  Type: Graph (stacked area)
  Metrics:
    - themis_faiss_index_vram_bytes
    - themis_faiss_scratch_vram_bytes
  Y-Axis: GB
  Total: Should fit in available VRAM
```

---

### Dashboard 5: Distributed Reasoning

**Zweck:** Komplexe Multi-Step Tasks Monitoring

**Panels:**

```yaml
Panel: Chain-of-Thought Steps Executed
  Type: Counter
  Metrics: themis_cot_steps_executed_total
  Split by: reasoning_depth (5-step, 10-step, 20-step)

Panel: Parallel Execution Speedup
  Type: Graph (time series)
  Metrics:
    - themis_distributed_cot_latency_seconds{mode="parallel"}
    - themis_distributed_cot_latency_seconds{mode="sequential"}
  Y-Axis: Seconds
  Annotation: Speedup = Sequential / Parallel

Panel: Multi-Perspective Consensus Time
  Type: Histogram
  Metrics: themis_multi_perspective_consensus_latency_seconds
  Buckets: 0-500ms, 500ms-1s, 1-2s, 2-5s, >5s
  Target: <1s for 90% of queries

Panel: Hierarchical Decomposition Depth
  Type: Bar Chart
  Metrics: themis_hierarchical_task_depth
  X-Axis: Task depth (1, 2, 3, 4, 5+ levels)
  Y-Axis: Count

Panel: Reasoning Task Success Rate
  Type: Single Stat
  Metrics: themis_reasoning_task_success_total / themis_reasoning_task_total
  Format: Percentage
  Target: >95%

Panel: DAG Execution Timeline (Gantt Chart)
  Type: Custom (Gantt visualization)
  Metrics: themis_reasoning_step_start_time, themis_reasoning_step_end_time
  X-Axis: Time
  Y-Axis: Step name
  Identify: Parallel steps, bottlenecks

Panel: Perspective Agreement Matrix
  Type: Heatmap
  Metrics: themis_perspective_agreement_rate{perspective_a=~".*", perspective_b=~".*"}
  Values: 0% (complete disagreement) → 100% (complete agreement)
  Typical: 60-80% agreement (healthy diversity)

Panel: Task Complexity Distribution
  Type: Pie Chart
  Metrics: sum by (complexity) (themis_reasoning_task_total)
  Slices: simple (<5 steps), medium (5-10 steps), complex (>10 steps)
```

**Example: Legal Contract Analysis (500 pages)**
```
Hierarchical Decomposition:
  Level 1: Orchestrator splits into 5 domains
    ├─ Legal Shard: Contract structure & clauses
    ├─ Finance Shard: Financial terms & valuation
    ├─ IP Shard: Intellectual property rights
    ├─ Risk Shard: Risk assessment & liabilities
    └─ Context Shard: Historical context & precedents

  Level 2: Each shard performs Chain-of-Thought (5 steps)
    └─ Parallel execution: 5 shards × 5 steps = 25 steps total

  Level 3: Consensus & Fusion
    └─ Orchestrator fuses results from 5 perspectives

Total Time: 8 minutes (vs. 45 minutes GPT-4 sequential)
Speedup: 5.6x
```

---

### Dashboard 6: Cost & ROI

**Zweck:** Business Metrics und ROI-Tracking

**Panels:**

```yaml
Panel: Cost per 1M Tokens
  Type: Single Stat
  Calculation:
    - (GPU cost / month + electricity / month) / (tokens processed / month)
  Format: €/1M tokens
  Comparison: €0.05 (ThemisDB RTX 4090) vs. €30 (GPT-4 API)

Panel: Queries Processed (Total)
  Type: Counter
  Metrics: sum(themis_inference_requests_total)
  Format: Human-readable (K, M, B)

Panel: Savings vs. GPT-4 (Daily)
  Type: Graph (time series)
  Calculation:
    - (queries * $0.03 GPT-4 price) - (hardware amortization + electricity)
  Y-Axis: €/day saved
  Cumulative over time

Panel: GPU Utilization Rate
  Type: Gauge
  Metrics: avg(themis_gpu_utilization_percent)
  Format: Percentage
  Target: >70% (good ROI), >85% (excellent)

Panel: ROI Timeline (Break-Even Tracking)
  Type: Graph (cumulative)
  X-Axis: Days since deployment
  Y-Axis: Total savings (€)
  Annotation: Break-even point (typically 2-6 months)

Panel: Cost Efficiency by Model
  Type: Table
  Columns:
    - Model Name
    - Avg Latency (ms)
    - Cost/Query (€)
    - Quality (MMLU %)
    - ROI Score (Quality / Cost)
  Sort by: ROI Score descending

Panel: Electricity Cost
  Type: Graph (time series)
  Metrics: themis_power_consumption_watts * electricity_rate_per_kwh
  Y-Axis: €/day
  Typical: RTX 4090 = 450W = ~€1.5/day
```

**ROI Dashboard Summary Panel:**
```yaml
Panel: ROI Summary
  Type: Stat Panel (multi-value)
  Metrics:
    - Total Queries: 10.5M
    - Cost (ThemisDB): €2,450
    - Cost (GPT-4 equivalent): €315,000
    - Total Savings: €312,550
    - Days to Break-Even: 68 days
    - ROI: 12,757%
```

---

## 2. Prometheus Metrics Instrumentation

### C++ Implementation

```cpp
// metrics.h
#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include <prometheus/histogram.h>
#include <prometheus/registry.h>

class ThemisLLMMetrics {
public:
    ThemisLLMMetrics(std::shared_ptr<prometheus::Registry> registry)
        : registry_(registry),
          
          // Counters
          inference_requests_total_(
              prometheus::BuildCounter()
                  .Name("themis_inference_requests_total")
                  .Help("Total number of inference requests")
                  .Register(*registry)
                  .Add({{"shard", shard_id_}, {"model", model_name_}})
          ),
          
          lora_transfers_total_(
              prometheus::BuildCounter()
                  .Name("themis_lora_transfers_total")
                  .Help("Total LoRA adapter transfers between shards")
                  .Register(*registry)
                  .Add({{"source_shard", ""}, {"destination_shard", ""}})
          ),
          
          federated_queries_total_(
              prometheus::BuildCounter()
                  .Name("themis_federated_queries_total")
                  .Help("Total federated RAG queries")
                  .Register(*registry)
                  .Add({{"participating_shards", ""}})
          ),
          
          // Gauges
          vram_usage_bytes_(
              prometheus::BuildGauge()
                  .Name("themis_vram_usage_bytes")
                  .Help("Current VRAM usage in bytes")
                  .Register(*registry)
                  .Add({{"shard", shard_id_}, {"device_id", "0"}})
          ),
          
          active_loras_(
              prometheus::BuildGauge()
                  .Name("themis_active_loras_count")
                  .Help("Number of currently loaded LoRA adapters")
                  .Register(*registry)
                  .Add({{"shard", shard_id_}})
          ),
          
          shard_health_status_(
              prometheus::BuildGauge()
                  .Name("themis_shard_health_status")
                  .Help("Shard health: 1=healthy, 0.5=degraded, 0=unhealthy")
                  .Register(*registry)
                  .Add({{"shard", shard_id_}})
          ),
          
          // Histograms
          inference_latency_seconds_(
              prometheus::BuildHistogram()
                  .Name("themis_inference_latency_seconds")
                  .Help("Inference latency in seconds")
                  .Register(*registry)
                  .Add({{"shard", shard_id_}, {"model", model_name_}},
                       {0.01, 0.05, 0.1, 0.5, 1.0, 2.0, 5.0})  // Buckets
          ),
          
          vector_search_latency_seconds_(
              prometheus::BuildHistogram()
                  .Name("themis_vector_search_latency_seconds")
                  .Help("Vector search latency in seconds")
                  .Register(*registry)
                  .Add({{"shard", shard_id_}},
                       {0.001, 0.005, 0.01, 0.05, 0.1, 0.5, 1.0})
          ),
          
          reasoning_task_duration_seconds_(
              prometheus::BuildHistogram()
                  .Name("themis_reasoning_task_duration_seconds")
                  .Help("Distributed reasoning task duration")
                  .Register(*registry)
                  .Add({{"task_type", ""}, {"complexity", ""}},
                       {0.5, 1.0, 2.0, 5.0, 10.0, 30.0, 60.0})
          )
    {}
    
    // Record inference
    void recordInference(double latency_seconds, const std::string& model) {
        inference_requests_total_.Increment();
        inference_latency_seconds_.Observe(latency_seconds);
    }
    
    // Record LoRA transfer
    void recordLoRATransfer(const std::string& source, const std::string& dest, size_t bytes) {
        auto& counter = prometheus::BuildCounter()
            .Name("themis_lora_transfers_total")
            .Register(*registry_)
            .Add({{"source_shard", source}, {"destination_shard", dest}});
        counter.Increment();
        
        auto& bytes_gauge = prometheus::BuildGauge()
            .Name("themis_lora_transfer_bytes")
            .Register(*registry_)
            .Add({{"source_shard", source}, {"destination_shard", dest}});
        bytes_gauge.Set(static_cast<double>(bytes));
    }
    
    // Update VRAM usage
    void updateVRAMUsage(size_t bytes) {
        vram_usage_bytes_.Set(static_cast<double>(bytes));
    }
    
    // Update health status
    void updateHealthStatus(double status) {  // 1.0, 0.5, or 0.0
        shard_health_status_.Set(status);
    }

private:
    std::shared_ptr<prometheus::Registry> registry_;
    std::string shard_id_;
    std::string model_name_;
    
    prometheus::Counter& inference_requests_total_;
    prometheus::Counter& lora_transfers_total_;
    prometheus::Counter& federated_queries_total_;
    
    prometheus::Gauge& vram_usage_bytes_;
    prometheus::Gauge& active_loras_;
    prometheus::Gauge& shard_health_status_;
    
    prometheus::Histogram& inference_latency_seconds_;
    prometheus::Histogram& vector_search_latency_seconds_;
    prometheus::Histogram& reasoning_task_duration_seconds_;
};
```

### Prometheus Configuration

```yaml
# prometheus.yml
global:
  scrape_interval: 15s
  evaluation_interval: 15s

scrape_configs:
  - job_name: 'themisdb-shards'
    static_configs:
      - targets:
          - 'shard-legal:8080'
          - 'shard-finance:8080'
          - 'shard-medical:8080'
          - 'shard-technical:8080'
    metrics_path: '/metrics'
    scrape_interval: 10s
  
  - job_name: 'themisdb-orchestrator'
    static_configs:
      - targets: ['orchestrator:8000']
    metrics_path: '/metrics'
  
  - job_name: 'node-exporter'
    static_configs:
      - targets:
          - 'shard-legal:9100'
          - 'shard-finance:9100'
          - 'shard-medical:9100'
          - 'shard-technical:9100'
  
  - job_name: 'nvidia-gpu-exporter'
    static_configs:
      - targets:
          - 'shard-legal:9835'
          - 'shard-finance:9835'
          - 'shard-medical:9835'
          - 'shard-technical:9835'

rule_files:
  - 'alerts.yml'

alerting:
  alertmanagers:
    - static_configs:
        - targets: ['alertmanager:9093']
```

---

## 3. Testing Strategy

### Testing Pyramid

```
           /\
          /  \         E2E Tests (5%)
         /    \        - Full multi-shard workflows
        /      \       - Production scenarios
       /________\      
      /          \     Integration Tests (25%)
     /            \    - Multi-shard communication
    /              \   - LoRA transfers
   /                \  - Federated RAG
  /__________________\ 
 /                    \ Unit Tests (70%)
/______________________\ - Single inference
                        - LoRA loading
                        - Vector search
```

### Unit Tests

```cpp
// tests/unit/test_llm_engine.cpp
#include <gtest/gtest.h>
#include "themis/llm_engine.h"

TEST(LLMEngine, BasicInference) {
    LLMEngine engine("phi-3-mini-4k-instruct.Q4_K_M.gguf");
    
    InferenceRequest request;
    request.prompt = "Hello, how are you?";
    request.max_tokens = 100;
    request.temperature = 0.7;
    
    auto result = engine.inference(request);
    
    EXPECT_GT(result.tokens.size(), 0);
    EXPECT_LT(result.latency_ms, 2000);  // <2s on CPU
    EXPECT_FALSE(result.generated_text.empty());
}

TEST(LLMEngine, LoRALoading) {
    LLMEngine engine("mistral-7b-instruct-v0.3.Q4_K_M.gguf");
    
    bool loaded = engine.loadLoRA("legal-specialist-v1", 0.8);
    EXPECT_TRUE(loaded);
    
    EXPECT_EQ(engine.getActiveLoRACount(), 1);
    EXPECT_TRUE(engine.isLoRAActive("legal-specialist-v1"));
}

TEST(LLMEngine, LoRAFusion) {
    LLMEngine engine("mistral-7b-instruct");
    
    engine.loadLoRA("legal-lora", 0.8);
    engine.loadLoRA("finance-lora", 0.6);
    
    InferenceRequest request;
    request.prompt = "Analyze this M&A contract for financial risks";
    request.use_lora_fusion = true;
    
    auto result = engine.inference(request);
    
    EXPECT_EQ(result.used_loras.size(), 2);
    EXPECT_TRUE(std::find(result.used_loras.begin(), result.used_loras.end(), 
                         "legal-lora") != result.used_loras.end());
}

TEST(VRAMLicenseManager, EnforcesLimit) {
    VRAMLicenseManager mgr(24ULL * 1024 * 1024 * 1024);  // 24 GB
    
    EXPECT_TRUE(mgr.canUseVRAM(10 * GB));  // OK
    EXPECT_TRUE(mgr.canUseVRAM(24 * GB));  // OK (exactly at limit)
    EXPECT_FALSE(mgr.canUseVRAM(40 * GB)); // Exceeds limit
    
    EXPECT_THROW(mgr.enforceVRAMLimit(40 * GB), LicenseException);
}

TEST(VectorSearch, BasicQuery) {
    FAISSGPUIndex index(768);  // 768-dim embeddings
    
    // Add 1000 vectors
    std::vector<float> vectors = generateRandomVectors(1000, 768);
    index.add(vectors);
    
    // Search
    std::vector<float> query = generateRandomVector(768);
    auto results = index.search(query, /*k=*/10);
    
    EXPECT_EQ(results.size(), 10);
    EXPECT_LT(results[0].distance, results[9].distance);  // Sorted by distance
}
```

### Integration Tests

```cpp
// tests/integration/test_multi_shard.cpp
#include <gtest/gtest.h>
#include "themis/orchestrator.h"
#include "themis/shard.h"

class MultiShardTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Start 3 shards
        shard_legal_ = std::make_unique<LLMEnabledShard>("legal", "mistral-7b");
        shard_finance_ = std::make_unique<LLMEnabledShard>("finance", "mistral-7b");
        shard_risk_ = std::make_unique<LLMEnabledShard>("risk", "mistral-7b");
        
        // Start orchestrator
        orchestrator_ = std::make_unique<Orchestrator>();
        orchestrator_->registerShard(shard_legal_.get());
        orchestrator_->registerShard(shard_finance_.get());
        orchestrator_->registerShard(shard_risk_.get());
    }
    
    std::unique_ptr<LLMEnabledShard> shard_legal_;
    std::unique_ptr<LLMEnabledShard> shard_finance_;
    std::unique_ptr<LLMEnabledShard> shard_risk_;
    std::unique_ptr<Orchestrator> orchestrator_;
};

TEST_F(MultiShardTest, DistributedCoT) {
    // Build 10-step reasoning task (DAG)
    auto reasoning_dag = ReasoningDAG::Build({
        {"step1", {}, "legal"},      // Level 1 (parallel)
        {"step2", {}, "finance"},    // Level 1 (parallel)
        {"step3", {}, "risk"},       // Level 1 (parallel)
        {"step4", {"step1", "step2"}, "legal"},     // Level 2
        {"step5", {"step2", "step3"}, "finance"},   // Level 2
        {"step6", {"step1", "step3"}, "risk"},      // Level 2
        {"step7", {"step4", "step5"}, "legal"},     // Level 3
        {"step8", {"step5", "step6"}, "finance"},   // Level 3
        {"step9", {"step7", "step8"}, "risk"},      // Level 4
        {"step10", {"step9"}, "orchestrator"}       // Final fusion
    });
    
    auto result = orchestrator_->executeDistributedCoT(
        "Analyze 500-page M&A contract",
        reasoning_dag
    );
    
    EXPECT_TRUE(result.success);
    EXPECT_LT(result.latency_ms, 10000);  // <10s (CPU mode)
    EXPECT_GT(result.quality_score, 0.75);  // >75% quality
    EXPECT_EQ(result.steps_executed, 10);
}

TEST_F(MultiShardTest, FederatedRAG) {
    RAGRequest request;
    request.query = "What are the financial risks in this legal contract?";
    request.domains = {"legal", "finance"};
    request.top_k = 10;
    
    auto result = orchestrator_->executeFederatedRAG(request);
    
    EXPECT_EQ(result.participating_shards.size(), 2);
    EXPECT_TRUE(std::find(result.participating_shards.begin(),
                         result.participating_shards.end(),
                         "legal") != result.participating_shards.end());
    EXPECT_GT(result.retrieved_documents.size(), 0);
    EXPECT_LT(result.latency_ms, 1000);  // <1s
}

TEST_F(MultiShardTest, LoRATransfer) {
    // Load LoRA on shard-legal
    shard_legal_->loadLoRA("legal-specialist-v1", "/loras/legal-v1.bin", 0.8);
    EXPECT_TRUE(shard_legal_->isLoRAActive("legal-specialist-v1"));
    
    // Transfer to shard-finance
    bool transferred = orchestrator_->transferLoRA(
        "legal-specialist-v1",
        "legal",    // source
        "finance"   // destination
    );
    
    EXPECT_TRUE(transferred);
    EXPECT_TRUE(shard_finance_->isLoRAActive("legal-specialist-v1"));
    
    // Verify cache hit on subsequent transfer
    auto start = std::chrono::high_resolution_clock::now();
    orchestrator_->transferLoRA("legal-specialist-v1", "legal", "risk");
    auto end = std::chrono::high_resolution_clock::now();
    auto latency_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    EXPECT_LT(latency_ms, 50);  // Cache hit should be <50ms
}
```

### E2E Tests

```python
# tests/e2e/test_production_scenarios.py
import pytest
import requests
import time

def test_full_legal_analysis_workflow():
    """Test complete legal contract analysis workflow across shards"""
    
    # Upload document (500 pages)
    with open("tests/fixtures/ma_contract.pdf", "rb") as f:
        response = requests.post(
            "http://localhost:8000/api/v1/documents/upload",
            files={"file": f},
            data={"domain": "legal"}
        )
    assert response.status_code == 200
    doc_id = response.json()["document_id"]
    
    # Trigger distributed analysis
    analysis_request = {
        "document_id": doc_id,
        "analysis_types": ["legal", "financial", "risk", "ip"],
        "mode": "parallel_cot",
        "depth": 10
    }
    
    start = time.time()
    response = requests.post(
        "http://localhost:8000/api/v1/analysis/distributed",
        json=analysis_request
    )
    latency = time.time() - start
    
    assert response.status_code == 200
    result = response.json()
    
    # Validate results
    assert result["success"] is True
    assert len(result["perspectives"]) == 4  # Legal, Financial, Risk, IP
    assert result["consensus_score"] > 0.7
    assert latency < 600  # <10 minutes (CPU mode)
    
    # Validate each perspective
    for perspective in result["perspectives"]:
        assert "domain" in perspective
        assert "findings" in perspective
        assert len(perspective["findings"]) > 0
        assert "confidence" in perspective
        assert perspective["confidence"] > 0.6

def test_high_load_concurrent_queries():
    """Test system under load: 100 concurrent queries"""
    import concurrent.futures
    
    def send_query(query_id):
        response = requests.post(
            "http://localhost:8000/api/v1/inference",
            json={"prompt": f"Query {query_id}: Analyze this", "max_tokens": 100}
        )
        return response.status_code, response.elapsed.total_seconds()
    
    with concurrent.futures.ThreadPoolExecutor(max_workers=20) as executor:
        futures = [executor.submit(send_query, i) for i in range(100)]
        results = [f.result() for f in concurrent.futures.as_completed(futures)]
    
    # Validate
    success_count = sum(1 for status, _ in results if status == 200)
    latencies = [latency for _, latency in results]
    
    assert success_count >= 95  # >=95% success rate
    assert sum(latencies) / len(latencies) < 5.0  # Avg <5s (CPU mode)
    assert max(latencies) < 30.0  # Max <30s

def test_shard_failover():
    """Test automatic failover when shard fails"""
    
    # Kill shard-legal
    requests.post("http://localhost:8080/admin/shutdown")
    time.sleep(5)
    
    # Query should still work (failover to backup or CPU fallback)
    response = requests.post(
        "http://localhost:8000/api/v1/inference",
        json={"prompt": "Legal question", "domain": "legal", "max_tokens": 100}
    )
    
    assert response.status_code == 200
    result = response.json()
    assert "fallback_used" in result or "backup_shard" in result
```

---

## 4. Performance Benchmarks

### Benchmark Suite

```bash
# Run all benchmarks
python3 benchmarks/run_all.py --config production.yml --output results.json

# Individual benchmarks
python3 benchmarks/inference_throughput.py \
  --model mistral-7b \
  --batch-sizes 1,4,8,16,32 \
  --duration 60s

python3 benchmarks/distributed_reasoning.py \
  --shards 3,5,10 \
  --task-complexity simple,medium,complex

python3 benchmarks/lora_transfer.py \
  --adapter-sizes 16MB,32MB,64MB \
  --network-speeds 1gbit,10gbit,40gbit

python3 benchmarks/vector_search.py \
  --index-sizes 100K,1M,10M \
  --backends cpu,cuda,vulkan
```

### Benchmark Results (Expected)

**Single Inference (Mistral-7B):**
| Hardware | Latency (ms) | Tokens/s | Cost/1M tok |
|----------|--------------|----------|-------------|
| RTX 4090 (GPU) | 315 | 52 | €0.05 |
| CPU (16 cores) | 2100 | 7.8 | €0.12 |
| GPT-4 (API) | 820 | 28 | $30.00 |

**Batch Processing (32 queries):**
| Hardware | Throughput (req/s) | Latency (s) |
|----------|-------------------|-------------|
| RTX 4090 | 8.2 | 3.9 |
| CPU (16 cores) | 1.2 | 26.7 |
| GPT-4 (API) | 1.8 | 17.8 |

**Distributed Reasoning (10-step CoT):**
| Cluster Size | Latency (s) | Throughput (req/s) |
|--------------|-------------|-------------------|
| 1 shard (GPU) | 12.5 | 0.08 |
| 3 shards (GPU) | 4.2 | 0.71 |
| 5 shards (GPU) | 3.1 | 1.19 |
| 10 shards (GPU) | 2.8 | 2.14 |
| GPT-4 (sequential) | 15.0 | 0.067 |

**Vector Search (FAISS):**
| Backend | Index Size | QPS | Latency (ms) |
|---------|-----------|-----|--------------|
| CUDA (A100) | 10M | 1818 | 5 |
| CPU (16 cores) | 10M | 58 | 120 |
| CUDA (RTX 4090) | 10M | 1200 | 8 |

---

## 5. CI/CD Integration

### GitHub Actions

```yaml
# .github/workflows/llm-tests.yml
name: LLM Integration Tests

on:
  push:
    branches: [main, develop]
  pull_request:
    branches: [main]

jobs:
  test-cpu-mode:
    runs-on: ubuntu-latest-16-cores
    steps:
      - uses: actions/checkout@v3
      
      - name: Set up Docker
        uses: docker/setup-buildx-action@v2
      
      - name: Build image
        run: docker build -t themisdb-llm:test .
      
      - name: Start CPU-only cluster
        run: |
          docker-compose -f docker-compose.test.yml up -d
          sleep 60  # Wait for models to load
      
      - name: Health check
        run: |
          curl -f http://localhost:8000/health || exit 1
          curl -f http://localhost:8080/health || exit 1
      
      - name: Run unit tests
        run: |
          docker exec shard-legal pytest tests/unit/ \
            --junitxml=junit-unit.xml
      
      - name: Run integration tests
        run: |
          python3 tests/integration/test_multi_shard_cpu.py \
            --cpu-only \
            --timeout=300
      
      - name: Run benchmarks
        run: |
          python3 benchmarks/inference_throughput.py \
            --cpu-only \
            --duration=30s \
            --output=benchmark-cpu.json
      
      - name: Shutdown
        if: always()
        run: docker-compose down
      
      - name: Upload test results
        if: always()
        uses: actions/upload-artifact@v3
        with:
          name: test-results
          path: |
            junit-*.xml
            benchmark-*.json
            logs/

  test-gpu-mode:
    runs-on: [self-hosted, gpu, a100]
    steps:
      - uses: actions/checkout@v3
      
      - name: Verify GPU
        run: nvidia-smi
      
      - name: Start GPU cluster
        run: docker-compose -f docker-compose.gpu.yml up -d
      
      - name: Run GPU tests
        run: |
          python3 tests/integration/test_distributed_reasoning_gpu.py \
            --shards=3 \
            --timeout=60
      
      - name: Run GPU benchmarks
        run: |
          python3 benchmarks/run_all.py \
            --config=gpu.yml \
            --output=benchmark-gpu.json
      
      - name: Compare benchmarks
        run: |
          python3 scripts/compare_benchmarks.py \
            --baseline=baseline-gpu.json \
            --current=benchmark-gpu.json \
            --tolerance=10%
      
      - name: Shutdown
        if: always()
        run: docker-compose down

  performance-regression:
    needs: [test-cpu-mode, test-gpu-mode]
    runs-on: ubuntu-latest
    steps:
      - name: Download artifacts
        uses: actions/download-artifact@v3
      
      - name: Analyze performance
        run: |
          python3 scripts/performance_analysis.py \
            --results=benchmark-*.json \
            --threshold-regression=15% \
            --output=performance-report.md
      
      - name: Comment PR
        if: github.event_name == 'pull_request'
        uses: actions/github-script@v6
        with:
          script: |
            const fs = require('fs');
            const report = fs.readFileSync('performance-report.md', 'utf8');
            github.rest.issues.createComment({
              issue_number: context.issue.number,
              owner: context.repo.owner,
              repo: context.repo.name,
              body: report
            });
```

---

## 6. Health Checks & Liveness Probes

### HTTP Endpoints

```cpp
// health_check.cpp
class HealthCheckEndpoint {
public:
    HealthStatus getHealth() const {
        HealthStatus status;
        status.status = determineOverallStatus();
        status.gpu_available = checkGPU();
        status.model_loaded = checkModel();
        status.vram_used_gb = getVRAMUsage() / GB;
        status.active_requests = getActiveRequests();
        status.uptime_seconds = getUptime();
        status.lora_count = getActiveLoRACount();
        
        return status;
    }
    
    ReadinessStatus getReady() const {
        ReadinessStatus status;
        status.ready = isModelLoaded() && 
                      getActiveRequests() < max_capacity_;
        status.capacity_used = getActiveRequests() / (float)max_capacity_;
        
        return status;
    }
    
private:
    std::string determineOverallStatus() const {
        if (!checkGPU() && gpu_required_) return "unhealthy";
        if (!checkModel()) return "unhealthy";
        if (getVRAMUsage() > vram_limit_ * 0.95) return "degraded";
        if (getActiveRequests() > max_capacity_ * 0.9) return "degraded";
        return "healthy";
    }
};
```

### Kubernetes Probes

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: themisdb-shard
spec:
  template:
    spec:
      containers:
      - name: themisdb-llm
        image: themisdb/llm-enabled:latest
        ports:
        - containerPort: 8080
        
        livenessProbe:
          httpGet:
            path: /health
            port: 8080
          initialDelaySeconds: 60  # Model loading time
          periodSeconds: 10
          timeoutSeconds: 5
          failureThreshold: 3
        
        readinessProbe:
          httpGet:
            path: /ready
            port: 8080
          initialDelaySeconds: 90
          periodSeconds: 5
          timeoutSeconds: 3
          successThreshold: 1
          failureThreshold: 3
        
        startupProbe:
          httpGet:
            path: /startup
            port: 8080
          initialDelaySeconds: 10
          periodSeconds: 10
          timeoutSeconds: 5
          failureThreshold: 30  # 5 minutes max startup time
```

---

## Zusammenfassung

**Vollständige Observability-Strategie:**

1. ✅ **6 Grafana Dashboards** - Cluster, Inter-Cerebral, LLM, Vector Search, Reasoning, Cost/ROI
2. ✅ **40+ Prometheus Metriken** - Counter, Gauge, Histogram für alle Komponenten
3. ✅ **Testing Pyramid** - 70% Unit, 25% Integration, 5% E2E
4. ✅ **Performance Benchmarks** - CPU vs GPU, Single vs Multi-Shard
5. ✅ **CI/CD Integration** - GitHub Actions mit GPU/CPU Testing
6. ✅ **Health Checks** - Kubernetes Liveness/Readiness Probes

**Expected Outcomes:**
- 99.9% uptime
- <2s p95 latency
- >70% VRAM utilization
- <100ms inter-shard communication
- 100% functional test pass rate (CPU + GPU)

**Nächste Schritte:**
1. Grafana Dashboards importieren: `kubectl apply -f monitoring/grafana-dashboards.yml`
2. Prometheus starten: `docker-compose -f docker-compose.monitoring.yml up -d`
3. Tests ausführen: `pytest tests/ --cpu-only`
4. Benchmarks laufen lassen: `python3 benchmarks/run_all.py`
