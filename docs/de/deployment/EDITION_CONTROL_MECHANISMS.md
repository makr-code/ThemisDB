# ThemisDB Edition Control & Differentiation Strategy

**Stand:** 6. April 2026  
**Version:** v1.3.0  
**Kategorie:** 🚀 Deployment  
**Focus:** Key differentiators between editions

---

## 📑 Inhaltsverzeichnis

- [Three Core Control Mechanisms](#-three-core-control-mechanisms)
- [VRAM Limit](#1️⃣-vram-limit---gpu-acceleration-control)
- [Sharding Node Limit](#sharding-node-limit)

---

## 🔑 Three Core Control Mechanisms

ThemisDB editions werden durch **drei wesentliche Steuerungsmerkmale** differenziert:

1. **VRAM Limit (GPU Acceleration)**
2. **Sharding Node Limit**
3. **Enterprise Plugin System**

Diese drei Limits sind das Gating-System zwischen Community, Enterprise und Hyperscaler.

---

## 1️⃣ VRAM Limit - GPU Acceleration Control

### Community Edition: 24 GB VRAM Maximum

```cpp
// src/gpu/gpu_memory_manager.cpp

#ifdef THEMIS_EDITION_COMMUNITY
    const size_t MAX_GPU_MEMORY = 24ULL * 1024 * 1024 * 1024;  // 24 GB
    const bool ALLOW_GPU_SPILL_TO_CPU = true;  // fallback to CPU if exceeds limit
#elif defined(THEMIS_EDITION_ENTERPRISE)
    const size_t MAX_GPU_MEMORY = 256ULL * 1024 * 1024 * 1024;  // 256 GB
    const bool ALLOW_GPU_SPILL_TO_CPU = true;
#elif defined(THEMIS_EDITION_HYPERSCALER)
    const size_t MAX_GPU_MEMORY = std::numeric_limits<size_t>::max();  // Unlimited
    const bool ALLOW_GPU_SPILL_TO_CPU = true;
#endif

void GPUMemoryManager::allocate_gpu_memory(size_t bytes) {
    if (bytes > MAX_GPU_MEMORY) {
        #ifdef THEMIS_EDITION_COMMUNITY
            // Log and spill to CPU
            THEMIS_WARN("GPU allocation {} bytes exceeds 24GB community limit. Using CPU fallback.",
                        bytes / (1024*1024));
            return allocate_cpu_memory(bytes);
        #else
            // Enterprise/Hyperscaler: allocate more
            return allocate_from_pool(bytes);
        #endif
    }
}

void GPUMemoryManager::check_vram_exceeded() {
    if (current_gpu_usage > MAX_GPU_MEMORY) {
        #ifdef THEMIS_EDITION_COMMUNITY
            throw std::runtime_error(
                "GPU memory limit exceeded (24 GB Community limit). "
                "Upgrade to Enterprise Edition (256 GB) at https://themisdb.io/enterprise"
            );
        #endif
    }
}
```

### Practical Implications (Community)

**24 GB VRAM is sufficient for:**
- ✅ 1M-10M embeddings (OpenAI ada-002: 1536 dims × 4 bytes)
- ✅ Real-time vector search with top-1000 candidates
- ✅ GPU-accelerated sorting on 10B+ row results
- ✅ Parallel batch inference (100-1000 vectors/batch)
- ✅ Most SaaS and startup use cases

**Cases exceeding 24 GB:**
- ❌ Fine-tuning LLMs (requires 40GB+)
- ❌ 100M+ embeddings in single GPU
- ❌ Real-time inference on 1000+ concurrent requests
- ❌ Interactive queries on full petabyte datasets

### Enterprise Edition: 256 GB VRAM

**Enterprise customers can:**
- ✅ Run larger models (7B, 13B parameter LLMs)
- ✅ Store 100M+ embeddings on single GPU
- ✅ Process 1000+ concurrent requests
- ✅ Batch inference with larger batches

### Hyperscaler Edition: Unlimited VRAM

**OEM deployments can:**
- ✅ Use entire GPU RAM (100GB+)
- ✅ Custom memory management
- ✅ Multi-GPU coordination

---

## 2️⃣ Sharding Node Limit

### Community Edition: Up to 5 Nodes

```cpp
// src/sharding/shard_manager.cpp

#ifdef THEMIS_EDITION_COMMUNITY
    const int MAX_SHARD_NODES = 5;  // Up to 5 nodes (v1.4.0+)
#elif defined(THEMIS_EDITION_ENTERPRISE)
    const int MAX_SHARD_NODES = 100;  // Up to 100 nodes
#elif defined(THEMIS_EDITION_HYPERSCALER)
    const int MAX_SHARD_NODES = 10000;  // 10,000+ nodes
#endif

class ShardManager {
public:
    void initialize_sharding(const Config& config) {
        int requested_shards = config.num_shard_nodes;
        
        #ifdef THEMIS_EDITION_COMMUNITY
            if (requested_shards > 5) {
                throw std::runtime_error(
                    "Community Edition supports up to 5 nodes. "
                    "Requested: " + std::to_string(requested_shards) + ". "
                    "For more nodes, upgrade to Enterprise Edition (100 nodes). "
                    "Contact: service@themisdb.org"
                );
            }
        #elif defined(THEMIS_EDITION_ENTERPRISE)
            if (requested_shards > MAX_SHARD_NODES) {
                throw std::runtime_error(
                    fmt::format("Enterprise Edition supports up to {} shard nodes. "
                               "Requested: {}. For larger clusters, contact service@themisdb.org",
                               MAX_SHARD_NODES, requested_shards)
                );
            }
        #endif
        
        // Initialize sharding
    }
};
```

### What Community Developers Can Do

**Application-Level Sharding (Supported):**
```cpp
// Community Edition: app handles routing across 5 nodes
class AppShardRouter {
    ThemisDB shard_0;  // Node 0
    ThemisDB shard_1;  // Node 1  
    ThemisDB shard_2;  // Node 2
    ThemisDB shard_3;  // Node 3
    ThemisDB shard_4;  // Node 4
    
    void insert(const Document& doc) {
        int shard_id = hash(doc.id) % 5;
        shards[shard_id].insert(doc);
    }
};
```

**This approach works fine for:**
- ✅ 1-5 nodes (manageable in app code)
- ✅ Static shard topology (doesn't change often)
- ✅ Deployments where shard rebalancing is rare

**But Community users DON'T get:**
- ❌ Automatic shard rebalancing
- ❌ Cross-shard joins
- ❌ Distributed transactions
- ❌ Zero-downtime migration

### Enterprise Edition: Automatic Sharding (1-100 nodes)

```cpp
// Enterprise feature: automatic sharding
// - Cross-shard joins work transparently
// - Automatic rebalancing when nodes join/leave
// - Distributed transactions
// - Zero-downtime shard migration
```

### Hyperscaler Edition: Massive Clustering (10000+ nodes)

```cpp
// Hyperscaler: unlimited nodes
// - Custom rebalancing strategies
// - Advanced consensus (Raft variants)
// - Multi-DC coordination
// - 1000+ nodes tested at scale
```

---

## 3️⃣ Enterprise Plugin System

### Community Edition: No Plugin System

Community uses **built-in processors** only:
```
✅ Image Analysis (built-in)
✅ PDF Processing (built-in)
✅ Audio Processing (built-in)
✅ Video Processing (built-in)
✅ Geospatial (built-in)
✅ Full-Text Search (built-in)

❌ Custom Plugins (not available)
❌ Plugin Marketplace (not available)
❌ Third-party Integrations (must fork code)
```

### Enterprise Plugin Architecture

```cpp
// src/plugins/enterprise/plugin_system.cpp

#ifdef THEMIS_EDITION_ENTERPRISE

class PluginSystem {
    // Load custom plugins from directory
    void load_plugin(const std::string& plugin_path);
    
    // Plugin manifest validation
    void validate_plugin_signature(const Plugin& plugin);
    
    // Hot-reload capability
    void reload_plugin(const std::string& plugin_name);
};

struct EnterprisePlugin {
    std::string name;
    std::string version;
    std::string vendor;
    
    // Custom processors
    std::function<json(const blob&)> process;
    
    // Licensing info
    std::string license_key;
    Timestamp expiration_date;
};

#else  // Community Edition

class PluginSystem {
    void load_plugin(const std::string& path) {
        throw std::runtime_error(
            "Enterprise Plugins are not available in Community Edition. "
            "To use custom plugins, upgrade to Enterprise Edition at https://themisdb.io/enterprise"
        );
    }
};

#endif
```

### Available Enterprise Plugins

**Custom Processing Plugins:**
```
✅ Custom Image Models (YOLO, SAM, custom CNNs)
✅ Custom NLP Models (custom transformers, fine-tuned)
✅ Custom Video Analysis (optical flow, 3D pose)
✅ Custom Data Pipelines (proprietary formats)
✅ Custom Validation Rules (business logic)
✅ Custom Encryption Schemes (proprietary)
```

**Third-Party Integration Plugins:**
```
✅ OpenAI API Integration (GPT-4 enrichment)
✅ Hugging Face Model Hub (100k+ models)
✅ Kafka/Kinesis Connectors (streaming)
✅ S3/GCS/Azure Blob connectors
✅ Salesforce/SAP connectors
✅ Custom webhook handlers
```

**Monitoring & Compliance Plugins:**
```
✅ Custom audit formatters
✅ Custom alerting (Slack, PagerDuty, etc.)
✅ Custom metrics exporters
✅ HIPAA/SOC2 compliance plugins
✅ Industry-specific validators
```

### Plugin Marketplace (Enterprise)

```
enterprise.themisdb.io/plugins/

Featured Plugins:
  ✅ OpenAI GPT-4 Enrichment
  ✅ Stable Diffusion Image Gen
  ✅ LLaMA Fine-tuning
  ✅ Kafka CDC Streaming
  ✅ Custom Auth (OAuth2, SAML, LDAP)
```

---

## 📊 Control Mechanism Summary Table

| Aspect | Community | Enterprise | Hyperscaler |
|--------|-----------|-----------|------------|
| **GPU VRAM** | 24 GB max | 256 GB max | Unlimited |
| **Shard Nodes** | Up to 5 nodes | 1-100 nodes | 1-10000+ nodes |
| **Plugin System** | None (built-in only) | Custom plugins | Custom + OEM |
| **Fallback Behavior** | Graceful CPU spill | Allocate more | No limits |
| **Cost of Limit** | Free (app sharding) | Automatic | Automatic |

---

## 🔐 Runtime Enforcement

### License Key Validation

```cpp
// src/main_server.cpp

#ifdef THEMIS_EDITION_ENTERPRISE

void validate_enterprise_license() {
    std::string license_key = std::getenv("THEMIS_LICENSE_KEY");
    
    if (!is_valid_license_key(license_key)) {
        throw std::runtime_error(
            "Invalid or missing THEMIS_LICENSE_KEY. "
            "Enterprise Edition requires valid license. "
            "Contact: service@themisdb.org"
        );
    }
    
    // Check max nodes limit
    int configured_nodes = config.shard_nodes.size();
    if (configured_nodes > 100) {
        throw std::runtime_error(
            "Enterprise Edition supports max 100 shard nodes. "
            "Configured: " + std::to_string(configured_nodes)
        );
    }
}

#endif
```

### Metrics & Telemetry

```cpp
// Track edition usage for monitoring
THEMIS_METRIC_COUNTER("edition_enterprise_total_connections", 1);
THEMIS_METRIC_GAUGE("edition_gpu_vram_usage_gb", 18.5);
THEMIS_METRIC_COUNTER("edition_shard_node_count", 50);
THEMIS_METRIC_COUNTER("edition_plugin_loaded", 3);
```

---

## 💡 Upgrade Path Examples

### Scenario 1: Outgrowing 24 GB VRAM

```
Customer: "Our ML workload needs 50 GB GPU memory"
Solution: Upgrade to Enterprise Edition
  - 256 GB VRAM limit
  - Distributed GPU memory pools
  - Cost: $5k-10k/month
```

### Scenario 2: Needing Cross-Shard Joins

```
Customer: "We want to scale to 10 shards without app logic"
Solution: Upgrade to Enterprise Edition
  - Automatic sharding (1-100 nodes)
  - Cross-shard queries handled transparently
  - Auto-rebalancing
  - Cost: $10k-20k/month
```

### Scenario 3: Custom ML Models

```
Customer: "We need to integrate our proprietary vision model"
Solution: Upgrade to Enterprise Edition
  - Plugin system support
  - Load custom processors
  - Hot-reload capability
  - Cost: $5k+/month
```

### Scenario 4: 1000+ Node Cluster

```
Customer: "We're a cloud provider, need massive scale"
Solution: Hyperscaler Edition
  - Up to 10,000 nodes
  - Advanced clustering
  - Custom optimization
  - Cost: OEM deal
```

---

## 🎯 Documentation Updates Needed

### Community Edition Docs
- ✅ "Single-Node Deployment Guide"
- ✅ "Application-Level Sharding Pattern"
- ✅ "GPU Memory Best Practices (24GB limit)"
- ✅ "Built-in Processors Reference"

### Enterprise Edition Docs
- ✅ "Automatic Sharding Setup (1-100 nodes)"
- ✅ "Enterprise Plugin Development"
- ✅ "Extended GPU Memory (256GB)"
- ✅ "License Key Installation"

### Hyperscaler Edition Docs
- ✅ "Massive Clustering (10000+ nodes)"
- ✅ "Custom Optimization Guide"
- ✅ "OEM Plugin Development"
- ✅ "Multi-GPU Coordination"

---

## 🔄 Compliance & Testing

### Automated Edition Checks

```cpp
// tests/edition_limits_test.cpp

TEST(CommunityEdition, GPU_VRAM_Limit_24GB) {
    // Verify 24 GB limit is enforced
    GPUBuffer buf(25 * 1024 * 1024 * 1024);  // 25 GB
    EXPECT_THROW(buf.allocate(), std::runtime_error);
}

TEST(CommunityEdition, Sharding_Disabled) {
    // Verify no multi-node sharding in community
    Config config;
    config.shard_nodes = {"node1", "node2"};  // 2 nodes
    EXPECT_THROW(ShardManager(config), std::runtime_error);
}

TEST(CommunityEdition, Plugin_System_Disabled) {
    PluginSystem plugins;
    EXPECT_THROW(plugins.load_plugin("custom.so"), std::runtime_error);
}

TEST(EnterpriseEdition, GPU_VRAM_Limit_256GB) {
    // Verify 256 GB limit for enterprise
    GPUBuffer buf(300 * 1024 * 1024 * 1024);  // 300 GB
    EXPECT_THROW(buf.allocate(), std::runtime_error);
}

TEST(EnterpriseEdition, Sharding_100_Nodes) {
    Config config;
    for (int i = 0; i < 100; i++) {
        config.shard_nodes.push_back(fmt::format("node{}", i));
    }
    ShardManager mgr(config);  // Should work
    EXPECT_EQ(mgr.num_shards(), 100);
}

TEST(HyperscalerEdition, Sharding_Unlimited) {
    Config config;
    for (int i = 0; i < 5000; i++) {
        config.shard_nodes.push_back(fmt::format("node{}", i));
    }
    ShardManager mgr(config);  // Should work
    EXPECT_EQ(mgr.num_shards(), 5000);
}
```

---

## 📈 Feature Gating Logic (Master Table)

| Feature | Gate Type | Community | Enterprise | Hyperscaler |
|---------|-----------|-----------|-----------|------------|
| GPU Acceleration | VRAM Limit | 24 GB | 256 GB | Unlimited |
| Vector Search | Feature | Yes | Yes | Yes |
| Auto Sharding | Node Limit | 0 nodes | 1-100 | 1-10000 |
| Cross-Shard Joins | Sharding | No | Yes | Yes |
| Plugin System | Plugin | No | Yes | Yes |
| Custom Processors | Plugin | No | Yes | Yes |
| Multi-Master Replication | Feature | No | Yes | Yes |
| Geo-Replication | Feature | No | Yes | Yes |
| RBAC | Feature | No | Yes | Yes |
| Auto-Failover | Feature | No | Yes | Yes |

---

## 🎊 Implementation Checklist

### Code Changes Required

- [ ] GPU memory manager: Add 24GB/256GB/unlimited checks
- [ ] Sharding system: Add node count enforcement
- [ ] Plugin loader: Add edition checks
- [ ] License validator: Check edition flags
- [ ] Error messages: Clear upgrade paths
- [ ] Metrics: Edition-specific telemetry

### Documentation Changes

- [ ] Community: Single-node, app-level sharding, 24GB limits
- [ ] Enterprise: Multi-node setup, plugins, 256GB limits
- [ ] Hyperscaler: Massive scale, custom optimization

### Testing Requirements

- [ ] Community limits enforced
- [ ] Enterprise limits enforced
- [ ] Hyperscaler limits enforced
- [ ] Graceful degradation
- [ ] Error messages helpful
- [ ] Telemetry accurate

---

## 💰 Pricing Impact of Limits

**Community Edition (Free)**
- $0/month - includes everything except 3 control limits
- 24 GB GPU RAM sufficient for 99% of SaaS apps
- Single-node deployments cover 40% of market

**Enterprise Edition ($10k-20k/month)**
- 256 GB GPU RAM (10x community)
- Up to 100 shard nodes (10-100x throughput)
- Custom plugins (unlimited extensibility)
- Cost justified by sharding + enterprise support

**Hyperscaler Edition (OEM)**
- Unlimited limits
- Custom engineering
- Highest margins

These **three control mechanisms** are the primary value drivers of Enterprise licensing.
