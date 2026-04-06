# LoRA Router - Automatic LoRA-to-LLM Routing

**Version:** 1.4.0  
**Status:** ✅ Production Ready  
**Date:** 2026-01-19

---

## 🎯 Overview

The **LoRA Router** provides intelligent, automated routing of queries to optimal LoRA adapters based on:

- **Semantic Similarity** - Embedding-based query-to-adapter matching
- **Load-Aware Balancing** - GPU health and utilization optimization
- **A/B Testing** - Traffic splitting for controlled experiments
- **Incremental Rollout** - Gradual deployment of new adapters
- **Fallback Policies** - Default adapter when no good match exists

This eliminates manual adapter selection, enabling dynamic multi-tenant AI workloads with optimized quality and cost.

---

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                      LoRA Router Pipeline                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  Query → Embed → Find Similar → Apply Policy → Check Load →    │
│  → Select GPU → Log Decision → Return                           │
│                                                                  │
│  ┌──────────────────┐    ┌──────────────────┐                  │
│  │  Embedding       │    │  Adapter         │                  │
│  │  Provider        │───▶│  Registry        │                  │
│  │                  │    │                  │                  │
│  │ • Query embed    │    │ • Metadata       │                  │
│  │ • Adapter embed  │    │ • Task types     │                  │
│  │ • Cosine sim     │    │ • Domains        │                  │
│  └──────────────────┘    └──────────────────┘                  │
│           │                       │                              │
│           ▼                       ▼                              │
│  ┌──────────────────────────────────────────┐                  │
│  │         Routing Policy Engine             │                  │
│  │                                            │                  │
│  │  • SEMANTIC - Pure similarity             │                  │
│  │  • LOAD_AWARE - Similarity + GPU load     │                  │
│  │  • AB_TEST - Traffic splitting            │                  │
│  │  • ROLLOUT - Incremental deployment       │                  │
│  │  • FALLBACK - Default adapter             │                  │
│  └──────────────────┬─────────────────────────┘                 │
│                     │                                            │
│                     ▼                                            │
│  ┌──────────────────────────────────────────┐                  │
│  │     Load Balancer Integration             │                  │
│  │                                            │                  │
│  │  • GPU health check                       │                  │
│  │  • VRAM availability                      │                  │
│  │  • Adapter placement                      │                  │
│  │  • Multi-GPU support                      │                  │
│  └──────────────────┬─────────────────────────┘                 │
│                     │                                            │
│                     ▼                                            │
│  ┌──────────────────────────────────────────┐                  │
│  │         Metrics & Monitoring              │                  │
│  │                                            │                  │
│  │  • Adapter usage count                    │                  │
│  │  • Fallback rate                          │                  │
│  │  • Routing latency                        │                  │
│  │  • Similarity scores                      │                  │
│  │  • Decision cache stats                   │                  │
│  └────────────────────────────────────────────┘                 │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 🚀 Quick Start

### Basic Usage

```cpp
#include "llm/lora_router.h"

// 1. Create components
auto embedding_provider = std::make_shared<EmbeddingProvider>(model, context);
auto adapter_registry = std::make_shared<AdapterRegistry>(sig_manager);
auto load_balancer = std::make_shared<AdapterLoadBalancer>(gpu_memory_manager, config);
auto lora_manager = std::make_shared<MultiLoRAManager>(lora_config);

// 2. Configure router
LoRARouter::Config router_config;
router_config.enable_semantic_routing = true;
router_config.enable_load_aware = true;
router_config.top_k_candidates = 5;
router_config.min_similarity_threshold = 0.3f;

// 3. Create router
auto router = std::make_shared<LoRARouter>(
    embedding_provider,
    adapter_registry,
    load_balancer,
    lora_manager,
    router_config
);

// 4. Route queries
auto decision = router->routeQuery("How do I enable sharding in ThemisDB?");

std::cout << "Selected adapter: " << decision.adapter_id << std::endl;
std::cout << "GPU: " << decision.gpu_device_id << std::endl;
std::cout << "Similarity: " << decision.similarity_score << std::endl;
std::cout << "Latency: " << decision.routing_latency_ms.count() << " ms" << std::endl;
```

---

## 📋 Routing Policies

### 1. Semantic Routing (Pure Similarity)

Routes based purely on semantic similarity between query and adapter metadata.

```cpp
auto decision = router->routeQuery(
    "Generate SQL query for user analytics",
    "",  // base_model_id (auto-detect)
    RoutingPolicy::SEMANTIC
);
```

**Best for:**
- Single-tenant workloads
- Scenarios where quality is paramount
- When GPU load is balanced

---

### 2. Load-Aware Routing

Combines semantic similarity with GPU load balancing.

**Score = (1 - load_weight) × similarity + load_weight × (1 - gpu_load)**

```cpp
LoRARouter::Config config;
config.default_policy = RoutingPolicy::LOAD_AWARE;
config.load_weight = 0.3f;  // 30% weight for load, 70% for similarity

auto router = std::make_shared<LoRARouter>(..., config);
auto decision = router->routeQuery("Query here");
```

**Best for:**
- Multi-tenant production workloads
- Uneven GPU utilization
- Cost optimization

---

### 3. A/B Testing

Split traffic between multiple adapters for controlled experiments.

```cpp
// Configure A/B test
ABTestConfig ab_config;
ab_config.adapter_ids = {"themis_help_v1", "themis_help_v2"};
ab_config.traffic_splits = {0.5f, 0.5f};  // 50/50 split
ab_config.experiment_id = "help_adapter_test_001";
ab_config.start_time = std::chrono::system_clock::now();
ab_config.end_time = std::chrono::system_clock::now() + std::chrono::hours(24);
ab_config.enabled = true;

router->configureABTest(ab_config);

// Route queries (automatically uses A/B policy)
for (int i = 0; i < 1000; ++i) {
    auto decision = router->routeQuery("Test query " + std::to_string(i));
    // decision.policy_used == RoutingPolicy::AB_TEST
}

// End test
router->endABTest();
```

**Best for:**
- Testing new adapter versions
- Quality comparison experiments
- Gradual adapter validation

---

### 4. Incremental Rollout

Gradually increase traffic to new adapter.

```cpp
// Configure rollout
RolloutConfig rollout;
rollout.new_adapter_id = "themis_help_v2";
rollout.baseline_adapter_id = "themis_help_v1";
rollout.rollout_percentage = 0.1f;  // Start at 10%
rollout.increment_step = 0.1f;      // +10% per increment
rollout.increment_interval = std::chrono::hours(1);  // Every hour
rollout.enabled = true;
rollout.start_time = std::chrono::system_clock::now();

router->configureRollout(rollout);

// Route queries (10% go to v2, 90% to v1)
auto decision = router->routeQuery("Query");

// Manually increment rollout
float new_percentage = router->incrementRollout();  // Now 20%

// Or auto-increment based on metrics
if (adapter_quality_good) {
    router->incrementRollout();  // 30%
}

// Promote to 100% or rollback
router->endRollout(true);  // promote = true
```

**Best for:**
- Safe adapter deployment
- Gradual quality validation
- Risk mitigation

---

### 5. Fallback Policy

Default adapter when no good semantic match.

```cpp
// Configure fallback
FallbackConfig fallback;
fallback.default_adapter_id = "themis_general";
fallback.similarity_threshold = 0.5f;  // Fallback if similarity < 0.5
fallback.enable_fallback = true;

router->configureFallback(fallback);
```

**Best for:**
- Handling unknown query types
- Ensuring service availability
- Graceful degradation

---

## 📊 Metrics & Monitoring

### Get Routing Metrics

```cpp
auto metrics = router->getMetrics();

std::cout << "Total requests: " << metrics.total_requests << std::endl;
std::cout << "Successful routes: " << metrics.successful_routes << std::endl;
std::cout << "Fallback routes: " << metrics.fallback_routes << std::endl;
std::cout << "Fallback rate: " 
          << (double)metrics.fallback_routes / metrics.total_requests << std::endl;
std::cout << "Avg routing latency: " << metrics.avg_routing_latency_ms << " ms" << std::endl;
std::cout << "Avg similarity: " << metrics.avg_similarity_score << std::endl;

// Per-adapter usage
for (const auto& [adapter_id, count] : metrics.adapter_usage_count) {
    std::cout << adapter_id << ": " << count << " requests" << std::endl;
}

// Per-adapter average similarity
for (const auto& [adapter_id, avg_sim] : metrics.adapter_avg_similarity) {
    std::cout << adapter_id << ": " << avg_sim << " avg similarity" << std::endl;
}
```

### Export as JSON (for Prometheus/Grafana)

```cpp
auto metrics_json = router->exportMetrics();
std::cout << metrics_json.dump(2) << std::endl;
```

**Output:**
```json
{
  "total_requests": 1000,
  "successful_routes": 950,
  "fallback_routes": 50,
  "fallback_rate": 0.05,
  "avg_routing_latency_ms": 2.3,
  "avg_similarity_score": 0.78,
  "adapter_usage_count": {
    "themis_help_lora": 450,
    "themis_sql_lora": 300,
    "themis_general": 250
  },
  "adapter_avg_similarity": {
    "themis_help_lora": 0.85,
    "themis_sql_lora": 0.82,
    "themis_general": 0.65
  }
}
```

---

## ⚙️ Configuration

### Router Configuration

```cpp
LoRARouter::Config config;

// Semantic routing
config.enable_semantic_routing = true;
config.top_k_candidates = 5;           // Top 5 similar adapters
config.min_similarity_threshold = 0.3f; // Min 30% similarity

// Load-aware routing
config.enable_load_aware = true;
config.load_weight = 0.3f;  // 30% load, 70% similarity

// Default policy
config.default_policy = RoutingPolicy::LOAD_AWARE;

// Fallback
config.fallback.default_adapter_id = "themis_general";
config.fallback.similarity_threshold = 0.5f;
config.fallback.enable_fallback = true;

// Metrics
config.enable_metrics = true;
config.metrics_window_size = 1000;  // Rolling window

// Caching
config.enable_decision_cache = true;
config.decision_cache_size = 1000;
config.decision_cache_ttl = std::chrono::seconds(300);  // 5 min
```

---

## 🔍 Advanced Features

### Batch Routing

Route multiple queries efficiently:

```cpp
std::vector<std::string> queries = {
    "How do I configure authentication?",
    "Generate SQL for analytics",
    "Explain vector search"
};

auto decisions = router->routeQueryBatch(queries);

for (size_t i = 0; i < decisions.size(); ++i) {
    std::cout << "Query " << i << " → " << decisions[i].adapter_id << std::endl;
}
```

### Decision Caching

Router automatically caches recent routing decisions to reduce latency:

```cpp
// First request (computes embedding + similarity)
auto decision1 = router->routeQuery("How do I enable sharding?");  // 5ms

// Second identical request (cache hit)
auto decision2 = router->routeQuery("How do I enable sharding?");  // <1ms

// Cache statistics
auto cache_stats = router->getCacheStats();
std::cout << "Cache size: " << cache_stats["cache_size"] << std::endl;
std::cout << "Cache TTL: " << cache_stats["cache_ttl_sec"] << " sec" << std::endl;

// Clear cache
router->clearCache();
```

### Base Model Filtering

Route only to adapters compatible with specific base model:

```cpp
auto decision = router->routeQuery(
    "Generate embeddings",
    "llama-2-7b"  // Only consider llama-2-7b adapters
);

assert(decision.base_model_id == "llama-2-7b");
```

---

## 📈 Performance

### Benchmarks

| Operation | Latency | Throughput |
|-----------|---------|------------|
| Semantic routing (no cache) | 3-5 ms | ~250 req/s |
| Semantic routing (cached) | <1 ms | ~1500 req/s |
| Load-aware routing | 4-6 ms | ~200 req/s |
| A/B test routing | 2-3 ms | ~400 req/s |
| Batch routing (10 queries) | 15-20 ms | ~500 req/s |

**Hardware:** NVIDIA RTX 4090, Intel i9-13900K

---

## 🔐 Security & Audit

### Routing Decision Logging

Every routing decision can be logged for audit:

```cpp
auto decision = router->routeQuery("Sensitive query");

// Log decision
audit_logger->log({
    "timestamp": getCurrentTime(),
    "query_hash": hashQuery(query),  // Don't log full query for privacy
    "adapter_id": decision.adapter_id,
    "gpu_device_id": decision.gpu_device_id,
    "similarity_score": decision.similarity_score,
    "policy_used": static_cast<int>(decision.policy_used),
    "is_fallback": decision.is_fallback,
    "routing_latency_ms": decision.routing_latency_ms.count(),
    "user_id": current_user_id,
    "tenant_id": current_tenant_id
});
```

### Multi-Tenant Isolation

Prevent adapter mis-assignment across tenants:

```cpp
// Register tenant-specific adapters
AdapterMetadata tenant1_adapter;
tenant1_adapter.adapter_id = "tenant1_help";
tenant1_adapter.custom_metadata["tenant_id"] = "tenant1";
adapter_registry->registerAdapter(tenant1_adapter);

// Filter adapters by tenant during routing
auto adapters = adapter_registry->searchAdapters({
    .custom_filter = [tenant_id](const AdapterMetadata& meta) {
        return meta.custom_metadata.at("tenant_id") == tenant_id;
    }
});
```

---

## 🐛 Troubleshooting

### High Fallback Rate

**Symptom:** `metrics.fallback_rate > 0.3`

**Causes:**
- Similarity threshold too high
- Limited adapter coverage
- Query embeddings not representative

**Solutions:**
```cpp
// Lower similarity threshold
config.min_similarity_threshold = 0.2f;

// Adjust fallback threshold
fallback.similarity_threshold = 0.4f;

// Add more adapters to registry
// Register domain-specific adapters
```

### High Routing Latency

**Symptom:** `metrics.avg_routing_latency_ms > 10ms`

**Causes:**
- Embedding provider slow
- Too many candidates evaluated
- Cache disabled

**Solutions:**
```cpp
// Reduce top-K candidates
config.top_k_candidates = 3;

// Enable caching
config.enable_decision_cache = true;

// Use faster embedding model
// Or pre-compute adapter embeddings
```

### Uneven Adapter Usage

**Symptom:** One adapter gets 90% of traffic

**Causes:**
- Adapter metadata too broad
- Load-aware routing disabled
- A/B test misconfigured

**Solutions:**
```cpp
// Enable load-aware routing
config.enable_load_aware = true;
config.load_weight = 0.4f;  // Higher weight for load

// Configure A/B test to force distribution
ABTestConfig ab_config;
ab_config.adapter_ids = {"adapter1", "adapter2", "adapter3"};
ab_config.traffic_splits = {0.33f, 0.33f, 0.34f};
```

---

## 🧪 Testing

### Unit Tests

```bash
# Build tests
cmake -B build -DTHEMIS_BUILD_TESTS=ON
cmake --build build --target test_lora_router

# Run tests
./build/tests/test_lora_router
```

### Integration Tests

```cpp
// Simulate multi-tenant workload
TEST(LoRARouterIntegration, MultiTenantWorkload) {
    // Setup router
    auto router = createRouter();
    
    // Simulate 1000 queries
    for (int i = 0; i < 1000; ++i) {
        auto decision = router->routeQuery(generateQuery());
        EXPECT_FALSE(decision.adapter_id.empty());
        EXPECT_GT(decision.similarity_score, 0.0f);
    }
    
    // Verify metrics
    auto metrics = router->getMetrics();
    EXPECT_EQ(metrics.total_requests, 1000);
    EXPECT_LT(metrics.fallback_routes, 100);  // < 10% fallback
}
```

---

## 📚 API Reference

### LoRARouter Class

#### Constructor

```cpp
LoRARouter(
    std::shared_ptr<EmbeddingProvider> embedding_provider,
    std::shared_ptr<AdapterRegistry> adapter_registry,
    std::shared_ptr<AdapterLoadBalancer> load_balancer,
    std::shared_ptr<MultiLoRAManager> lora_manager,
    const Config& config = Config{}
);
```

#### Main Methods

| Method | Description | Returns |
|--------|-------------|---------|
| `routeQuery(query, base_model_id, policy)` | Route single query | `RoutingDecision` |
| `routeQueryBatch(queries, base_model_id)` | Route multiple queries | `vector<RoutingDecision>` |
| `configureABTest(config)` | Configure A/B test | `bool` |
| `configureRollout(config)` | Configure rollout | `bool` |
| `configureFallback(config)` | Configure fallback | `void` |
| `getMetrics()` | Get routing metrics | `RoutingMetrics` |
| `exportMetrics()` | Export metrics as JSON | `json` |
| `clearCache()` | Clear decision cache | `void` |

---

## 🔗 Integration with ThemisDB Components

### With MultiLoRAManager

```cpp
// Router automatically uses MultiLoRAManager for adapter operations
auto decision = router->routeQuery("Query");

// Load adapter if not already loaded
if (!lora_manager->isLoRALoaded(decision.adapter_id)) {
    lora_manager->loadLoRA(decision.adapter_id, path, base_model);
}

// Apply adapter for inference
lora_manager->applyLoRA(decision.adapter_id, context_handle);
```

### With AdapterLoadBalancer

```cpp
// Router uses load balancer for GPU selection
auto decision = router->routeQuery("Query");

// Load balancer handles placement
if (decision.gpu_device_id >= 0) {
    load_balancer->placeAdapter(
        decision.adapter_id,
        decision.gpu_device_id,
        vram_bytes,
        priority
    );
}
```

### With Grafana Metrics

Export router metrics to Prometheus/Grafana:

```cpp
// Periodic metrics export
void exportMetricsToPrometheus() {
    auto metrics = router->getMetrics();
    
    prometheus::Gauge& total_requests = 
        prometheus::BuildGauge()
            .Name("lora_router_total_requests")
            .Help("Total routing requests")
            .Register(*registry);
    total_requests.Set(metrics.total_requests);
    
    prometheus::Gauge& fallback_rate = 
        prometheus::BuildGauge()
            .Name("lora_router_fallback_rate")
            .Help("Routing fallback rate")
            .Register(*registry);
    fallback_rate.Set(
        (double)metrics.fallback_routes / metrics.total_requests
    );
    
    // ... export other metrics
}
```

---

## ✅ Production Readiness

### Checklist

- [x] Semantic routing with real embeddings
- [x] Multi-GPU load-aware routing
- [x] A/B testing policy
- [x] Incremental rollout policy
- [x] Fallback handling
- [x] Comprehensive metrics
- [x] Decision caching
- [x] Thread-safe operations
- [x] Audit logging ready
- [x] Unit tests (25+ test cases)
- [x] Integration tests
- [x] Performance benchmarks
- [x] Documentation complete

---

## 🚦 Next Steps

1. **Deploy to production**
   ```bash
   # Build with routing support
   cmake -B build -DTHEMIS_ENABLE_LORA_ROUTING=ON
   cmake --build build
   ```

2. **Configure monitoring**
   - Set up Grafana dashboard for routing metrics
   - Configure alerts for high fallback rate
   - Monitor routing latency

3. **Run A/B tests**
   - Deploy new adapters with 10% traffic
   - Monitor quality metrics
   - Gradually increase rollout

4. **Optimize performance**
   - Pre-compute adapter embeddings
   - Tune cache TTL based on workload
   - Adjust similarity thresholds

---

## 📞 Support

For questions or issues:
- GitHub Issues: [ThemisDB Issues](https://github.com/makr-code/ThemisDB/issues)
- Documentation: `/docs/LORA_MULTIMODEL_GUIDE.md`
- Examples: `/examples/lora_routing/`

---

**Status:** ✅ Production Ready  
**Version:** 1.4.0  
**Last Updated:** 2026-04-06
