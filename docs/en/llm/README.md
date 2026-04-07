# LLM & AI Integration Documentation

**Status:** January 5, 2026  
**Version:** 1.8.0-rc1 (Advanced LLM Features)  
**Category:** LLM & Distributed AI

---

## 🚀 Overview

As of v1.4.0-alpha, ThemisDB offers advanced **embedded LLM capabilities** with grammar-constrained generation, extended context windows, vision support, and significant performance optimizations. The LLM module continues to build on the llama.cpp foundation with enterprise-grade features.

> **Important Note**: LLM integration is an **optional feature**:
> - Requires build flag: `-DTHEMIS_ENABLE_LLM=ON`
> - Requires external dependency: llama.cpp (clone separately)
> - Not enabled by default

**NEW in v1.4.0-alpha:**
- 📝 **Grammar-Constrained Generation** - EBNF/GBNF for guaranteed valid JSON/XML/CSV (95-99% reliability)
- 🔭 **RoPE Scaling** - Extended context window from 4K → 32K tokens (8x increase)
- 🖼️ **Vision Support** - Multi-modal LLMs with CLIP-based image encoding (LLaVA)
- ⚡ **Flash Attention** - CUDA kernels for 15-25% speedup, 30% memory reduction
- 🎯 **Speculative Decoding** - 2-3x faster inference with draft+target models
- 🔄 **Continuous Batching** - 2x+ throughput with dynamic request batching

**Implemented in v1.3.0:**
- 🧠 **Embedded llama.cpp** – Native LLM inference without external APIs (optional)
- ⚡ **GPU Acceleration** (CUDA/Metal/Vulkan) for high throughput
- 🧩 **Plugin Architecture** with `LlamaWrapper`
- 🗃️ **Lazy Model Loading** (Ollama-style)
- 🔀 **Multi-LoRA Management** (vLLM-style)

**Quick Links (v1.4.0-alpha):**
- [📝 Grammar-Constrained Generation](GRAMMAR_CONSTRAINED_GENERATION.md) - **NEW** Guaranteed valid outputs
- [🔭 RoPE Scaling Implementation](ROPE_SCALING_IMPLEMENTATION.md) - **NEW** Extended context windows
- [🖼️ Vision Support Quick Start](VISION_SUPPORT_QUICK_START.md) - **NEW** Multi-modal LLMs
- [⚡ Flash Attention Implementation](FLASH_ATTENTION_IMPLEMENTATION.md) - **NEW** Performance optimization
- [🎯 Speculative Decoding](SPECULATIVE_DECODING_IMPLEMENTATION.md) - **NEW** 2-3x speedup
- [🔄 Continuous Batching](CONTINUOUS_BATCHING_IMPLEMENTATION.md) - **NEW** Dynamic batching
- [🧠 **LLM Complete Setup Guide**](../../de/guides/LLM_COMPLETE_SETUP_GUIDE.md) – Complete guide for setup & inferencing
- [LLAMA_CPP_INTEGRATION.md](../../de/llm/LLAMA_CPP_INTEGRATION.md) – Integration & build
- [README_PLUGINS.md](../../de/llm/README_PLUGINS.md) – Quick start & examples

## 📚 Feature Documentation (v1.4.0-alpha)

### Advanced Features

| Feature | Guide | Status | Version |
|---------|-------|--------|---------|
| 📝 Grammar-Constrained Generation | [GRAMMAR_CONSTRAINED_GENERATION.md](GRAMMAR_CONSTRAINED_GENERATION.md) | ✅ Complete | v1.4.0-alpha |
| 🔭 RoPE Scaling (4K→32K tokens) | [ROPE_SCALING_IMPLEMENTATION.md](ROPE_SCALING_IMPLEMENTATION.md) | ✅ Complete | v1.4.0-alpha |
| 🖼️ Vision Support (Multi-Modal) | [VISION_SUPPORT_QUICK_START.md](VISION_SUPPORT_QUICK_START.md) | ✅ Complete | v1.4.0-alpha |
| ⚡ Flash Attention CUDA | [FLASH_ATTENTION_IMPLEMENTATION.md](FLASH_ATTENTION_IMPLEMENTATION.md) | ✅ Complete | v1.4.0-alpha |
| 🎯 Speculative Decoding | [SPECULATIVE_DECODING_IMPLEMENTATION.md](SPECULATIVE_DECODING_IMPLEMENTATION.md) | ✅ Complete | v1.4.0-alpha |
| 🔄 Continuous Batching | [CONTINUOUS_BATCHING_IMPLEMENTATION.md](CONTINUOUS_BATCHING_IMPLEMENTATION.md) | ✅ Complete | v1.4.0-alpha |
| 💾 KV-Cache Reuse | [KV_CACHE_REUSE_IMPLEMENTATION.md](KV_CACHE_REUSE_IMPLEMENTATION.md) | ✅ Complete | v1.3.0 |
| 📊 Embeddings Extraction | [EMBEDDINGS_EXTRACTION_IMPLEMENTATION.md](EMBEDDINGS_EXTRACTION_IMPLEMENTATION.md) | ✅ Complete | v1.3.0 |

## Source Code Reference

| Component | Header | Source | Description |
|-----------|--------|--------|-------------|
| LLMInteractionStore | `llm_interaction_store.h` | `llm_interaction_store.cpp` | Interaction Storage |
| PromptManager | `prompt_manager.h` | `prompt_manager.cpp` | Prompt Templates |

**Total:** 2 Headers, 2 Source Files, ~700 LOC

## Implemented Classes

### LLMInteractionStore

```cpp
class LLMInteractionStore {
    // Key format: "llm_interaction:{interaction_id}"
    
    struct Interaction {
        std::string id;                        // UUID
        std::string prompt_template_id;        // Template reference
        std::string prompt;                    // Sent prompt
        std::vector<std::string> reasoning_chain; // Chain-of-Thought steps
        std::string response;                  // LLM response
        std::string model_version;             // e.g., "gpt-4o-mini"
        int64_t timestamp_ms;
        int latency_ms;
        int token_count;
        nlohmann::json metadata;               // Feedback, user_id, etc.
    };
    
    struct ListOptions {
        size_t limit = 100;
        std::optional<std::string> start_after_id;  // Pagination
        std::optional<std::string> filter_model;
        std::optional<int64_t> since_timestamp_ms;
    };
    
    struct Stats {
        size_t total_interactions;
        int64_t total_tokens;
        double avg_latency_ms;
        size_t total_size_bytes;
    };
    
    // API
    Interaction createInteraction(Interaction);
    std::optional<Interaction> getInteraction(id);
    std::vector<Interaction> listInteractions(ListOptions);
    bool deleteInteraction(id);
    Stats getStats();
};
```

### PromptManager

```cpp
class PromptManager {
    struct PromptTemplate {
        std::string id;
        std::string name;
        std::string template_text;
        std::string version;
        nlohmann::json variables;     // {name, description, required}
        int64_t created_at;
        int64_t updated_at;
    };
    
    // API
    PromptTemplate createTemplate(PromptTemplate);
    std::optional<PromptTemplate> getTemplate(id);
    std::vector<PromptTemplate> listTemplates();
    PromptTemplate updateTemplate(id, updates);
    bool deleteTemplate(id);
    
    // Rendering
    std::string render(template_id, variables);
};
```

## Features

### Chain-of-Thought Storage

```cpp
Interaction interaction;
interaction.reasoning_chain = {
    "Step 1: Parse the user query",
    "Step 2: Identify relevant documents",
    "Step 3: Generate response based on context"
};
store.createInteraction(interaction);
```

### Token & Latency Tracking

```cpp
interaction.token_count = 150;
interaction.latency_ms = 450;

auto stats = store.getStats();
// stats.avg_latency_ms = 380.5
// stats.total_tokens = 125000
```

### Prompt Versioning

```cpp
auto template = manager.createTemplate({
    .name = "qa_template",
    .template_text = "Question: {{question}}\nContext: {{context}}\nAnswer:",
    .version = "1.0.0",
    .variables = {{"question", {.required = true}}, {"context", {.required = true}}}
});

auto prompt = manager.render(template.id, {
    {"question", "What is ThemisDB?"},
    {"context", "ThemisDB is a multi-model database..."}
});
```

## HTTP API

### POST /api/llm/interactions
```json
{
  "prompt": "What is the capital of France?",
  "response": "The capital of France is Paris.",
  "model_version": "gpt-4o-mini",
  "reasoning_chain": ["Parse query", "Lookup knowledge", "Generate response"],
  "token_count": 25,
  "latency_ms": 200
}
```

### GET /api/llm/interactions?limit=10&filter_model=gpt-4o-mini
### GET /api/llm/stats

### PATCH /llm/interaction/{id} - Update Metadata (Enterprise)
```json
{
  "feedback": {
    "rating": 5,
    "feedback_text": "Excellent response",
    "user_id": "user123",
    "flagged_for_training": true,
    "training_category": "positive"
  }
}
```

### POST /query/enhanced - Enhanced Query with LLM Context (Enterprise)
```json
{
  "aql": "FOR doc IN products FILTER doc.category == 'electronics' RETURN doc",
  "llm_context": {
    "limit": 5,
    "model": "gpt-4o-mini"
  }
}
```

## Enterprise Features

### Feedback System
The feedback system is implemented as an enterprise add-on and uses the flexible `metadata` field. It requires **no separate layer**.

**Use Cases:**
- Collect user feedback for LLM responses
- Mark training data for LoRA fine-tuning
- Track quality metrics

**See:** [LLM Feedback Enterprise](../../de/llm/LLM_FEEDBACK_ENTERPRISE.md)

### Query Enhancement
Combines database queries with LLM context for AI-powered applications.

**Benefits:**
- 49% cost reduction
- 38% latency improvement
- +25-40% quality improvement
- Real-time feedback loop

**See:** [LLM Integration Benefits Analysis](../../de/enterprise/LLM_INTEGRATION_BENEFITS_ANALYSIS.md)

## Related Documentation

### LLM Integration & Distributed Reasoning

- [**Docker/VM Deployment Guide**](../../de/llm/DOCKER_VM_DEPLOYMENT_GUIDE.md) ⭐ **NEW:** Deployment in containers and VMs
  - **GPU Passthrough:** Docker NVIDIA Toolkit, KVM/QEMU, VMware vSphere
  - **CPU Fallback Mode:** Complete testing WITHOUT GPU (5-10x slower, but functional)
  - **Mixed Mode:** Hybrid cluster with GPU + CPU shards
  - **Multi-Shard Testing:** Completely in Docker without GPU possible
  - **Kubernetes:** GPU Device Plugin, StatefulSets, Health Probes
  - **Performance:** GPU (100%), CPU (15-20%), Mixed (90%/15%)

- [**Monitoring & Testing Strategy**](../../de/llm/MONITORING_TESTING_STRATEGY.md) ⭐ **NEW:** Observability & QA
  - **6 Grafana Dashboards:** Cluster Overview, Inter-Cerebral Communication, LLM Performance, Vector Search, Distributed Reasoning, Cost/ROI
  - **Inter-Cerebral Monitoring:** Brain-inspired shard-to-shard communication visualization
  - **40+ Prometheus Metrics:** Counter, Gauge, Histogram for all components
  - **Testing Pyramid:** 70% Unit, 25% Integration, 5% E2E tests
  - **CI/CD Integration:** GitHub Actions with GPU/CPU testing
  - **Benchmarks:** Inference Throughput, Distributed Reasoning, LoRA Transfer, Vector Search
  - **Expected:** 99.9% Uptime, <2s p95 Latency, >70% VRAM Utilization

- [**Enterprise VRAM Licensing**](../../de/llm/ENTERPRISE_VRAM_LICENSING.md) ⭐ **NEW:** VRAM-based licensing model
  - **Community Edition (Free):** ≤24 GB VRAM - covers 80% of all use cases
  - **Enterprise Edition:** >24 GB VRAM - Llama-70B+, Multi-GPU, HA cluster
  - **Technical Implementation:** VRAMLicenseManager, runtime enforcement
  - **Pricing:** €5,000-€50,000/year depending on VRAM tier
  - **ROI:** 90-99% savings vs. hyperscaler
  - **Free Trial:** 30 days enterprise testing

- [**Distributed Reasoning Architecture**](../../de/llm/DISTRIBUTED_REASONING_ARCHITECTURE.md) ⭐ **NEW:** Distributed thinking like in the brain
  - **Brain-Inspired Multi-Shard Collaboration** - Specialized shards like brain regions
  - **Parallel Chain-of-Thought (CoT)** - Multi-step tasks 3-5x faster through parallelization
  - **Multi-Perspective Reasoning** - Different LoRA adapters analyze from different perspectives
  - **Hierarchical Task Decomposition** - Orchestrator decomposes complex tasks automatically
  - **Performance:** 3.6x faster, 21x cheaper than GPT-4 for multi-step tasks
  - **Use Cases:** Legal Contract Analysis, Medical Diagnosis, Scientific Research
  - **Roadmap:** v1.5.0 (Q3 2026)

- [**GPU-Tier Analysis & Hyperscaler Comparison**](../../de/llm/GPU_TIER_ANALYSIS_HYPERSCALER_COMPARISON.md) ⭐ **NEW:** SLM/LLM performance analysis
  - Entry-Level (<16GB): RTX 4060 Ti + Phi-3-Mini - €0.02/1M tokens, 1500x cheaper
  - Mid-Range (<24GB): RTX 4090 + Mistral-7B - €0.05/1M tokens, 600x cheaper, **Best ROI**
  - High-End (>24GB): A100 + Llama-3-70B - €0.15/1M tokens, 200x cheaper
  - 3-Year TCO: €9,801 (RTX 4090) vs. €835,200 (Azure) = 99% savings
  - Break-Even: 2.3 months (mid-range), 6.5 months (high-end)
  - Use-case specific recommendations

- [Native LLM Integration Concept](../../de/llm/NATIVE_LLM_INTEGRATION_CONCEPT.md) ⭐ **NEW:** Technical concept for direct LLM integration
  - Zero-Copy Memory Access (Apache Arrow, PyTorch best practices)
  - Unified Memory Space (CUDA Unified Memory)
  - Lazy Loading & Streaming (HuggingFace Datasets)
  - Continuous Batching (vLLM-style)
  - PagedAttention KV Cache
  - LoRA Fusion Engine (PEFT)
  - **Performance:** 4x faster, 6x VRAM-efficient
  - **Roadmap:** v1.5.0 (Q3 2026)

- [Zero-Copy Memory Access](../../de/llm/ZERO_COPY_MEMORY_ACCESS.md) ⭐ **NEW:** Intra-ops vs. inter-ops communication
  - Shared memory for same-host deployment
  - GPU Direct Memory Access (GPUDirect)
  - gRPC/Protobuf for distributed deployment
  - mTLS security layer
  - **Performance:** 0ms (GPU Direct) to 12s (gRPC compressed)

- [AI Ecosystem Sharding Architecture](../../de/llm/AI_ECOSYSTEM_SHARDING_ARCHITECTURE.md) - Complete AI ecosystem with horizontal sharding
  - Each shard with its own LLM (llama.cpp + GPU)
  - Dynamic LoRA transfer between shards
  - Federated RAG queries across multiple domains
  - Raft-based model replication
  - Production-ready with 3-10+ shards

- [LLM Loader Guide](../../de/llm/LLM_LOADER_GUIDE.md) - Complete implementation guide for vLLM-like LLM loader
  - Required libraries (llama.cpp, GGML, Protobuf, etc.)
  - Architecture and integration with ThemisDB
  - Model loading, LoRA support, inference
  - HTTP API and code examples
  
- [Binary Communication Protocols](../../de/llm/BINARY_COMMUNICATION_PROTOCOLS.md) - Communication between ThemisDB and vLLM
  - HTTP/JSON vs. gRPC/Protobuf (binary)
  - Unix Domain Sockets and Shared Memory
  - Performance comparisons and decision guidance
  - Code examples for all protocols
  
- [GPU Referencing Capabilities](../../de/llm/GPU_REFERENCING_CAPABILITIES.md) - Compute-intensive referencing in VRAM
  - FAISS GPU Backend (up to 31x faster)
  - Hybrid Search (BM25 + Vector) for RAG
  - Batch Processing and Semantic Caching
  - Co-Located Deployment (ThemisDB + vLLM on one GPU)
  - Performance benchmarks and scaling

### Additional Resources

- [Features: Semantic Cache](../../de/features/features_semantic_cache.md) - LLM response caching
- [Sharding Overview](../../de/sharding/sharding_overview.md) - Horizontal sharding in ThemisDB
- [AQL Prompt Engineering](../../de/AQL_PROMPT_ENGINEERING.md)
- [Projects: RAG LLM Programming Assistant](../../de/projects/RAG_LLM_PROGRAMMIERHILFE.md)
- [Roadmap](../../de/roadmap/ROADMAP.md) - ThemisDB roadmap (v1.5.0 LLM integration)

---

> **Note:** Most detailed LLM documentation is currently available in German. English translations are in progress.  
> For the most up-to-date information, please refer to the [German LLM documentation](../../de/llm/).

---

**Version:** 1.3.0 | **License:** MIT | **Support:** [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
