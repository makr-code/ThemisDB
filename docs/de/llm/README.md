# LLM & AI Integration Documentation

<!-- Status: current | validated: 2026-04-09 | Primary: ../../../src/llm/ | ../../../include/llm/ -->
<!-- Links: ../../../src/llm/README.md · ../../../src/llm/ROADMAP.md · inventory.md · MISSING_IMPLEMENTATIONS.md -->

**Stand:** 9. April 2026  
**Version:** 1.16.0 (Production-ready)  
**Kategorie:** LLM & Distributed AI

---

## 🚀 Übersicht

Ab v1.16.0 bietet ThemisDB ein **vollständiges, produktionsreifes LLM-Modul** mit Grammar-gesteuerter Generierung, OpenAI-kompatiblem API-Adapter, Streaming-SSE-Ausgabe, Speculative Decoding, LoRA-Hot-Loading, Per-Modell-Ressourcenquoten und einem Model-Quantization-Pipeline (GGUF/AWQ/GPTQ). Das Modul baut auf einer dualen Engine-Architektur mit llama.cpp-Integration auf.

> **Wichtiger Hinweis**: LLM-Integration ist ein **optionales Feature**:
> - Erfordert Build-Flag: `-DTHEMIS_ENABLE_LLM=ON`
> - Benötigt externe Abhängigkeit: llama.cpp (separat klonen)
> - Nicht standardmäßig aktiviert

**Vollständig implementiert (v1.16.0):**
- 📝 **Grammatik-gesteuerte Generierung** - EBNF/GBNF für garantiert valide JSON/XML/CSV
- 🔭 **Speculative Decoding** - 2-3× Inferenz-Speedup mit Draft+Target-Modellen
- 🖼️ **Vision-Support (Multi-Modal)** - Bild-Eingaben mit LLaVA (experimentell)
- ⚡ **Flash Attention** - CUDA-Kernel-Fusion für höheren Durchsatz
- 🔄 **Continuous Batching** - Dynamisches Request-Batching
- 💾 **Paged KV-Cache** - vLLM-inspiriertes Speicher-Management
- 🔌 **OpenAI-Kompatibler Adapter** - `/v1/chat/completions` REST-Endpoint
- 📡 **Streaming Token Output** - SSE / WebSocket Streaming
- 🔀 **LoRA Hot-Loading** - Adapter laden/entladen ohne Engine-Neustart
- 🎯 **Function/Tool Calling** - JSON-Schema-Binding + GBNF-Constraints
- 🔄 **Model Hot-Swap** - Modellwechsel ohne Engine-Neustart
- 📦 **Dedup-Cache** - Gleicher Prompt → Gecachte Antwort
- 📊 **Per-Modell-Quoten** - Memory- und Concurrency-Limits
- 🧮 **Model-Quantization-Pipeline** - GGUF/AWQ/GPTQ laden
- 🖼️ **Vision Support** - Multi-modale LLMs mit CLIP-basierter Bildcodierung (LLaVA)
- ⚡ **Flash Attention** - CUDA-Kernel für 15-25% Geschwindigkeitssteigerung, 30% Speicherreduktion
- 🎯 **Speculative Decoding** - 2-3x schnellere Inferenz mit Draft+Target-Modellen
- 🔄 **Continuous Batching** - 2x+ Durchsatz mit dynamischem Request-Batching

**Implementiert in v1.3.0:**
- 🧠 **Embedded llama.cpp** – Native LLM-Inferenz ohne externe APIs (optional)
- ⚡ **GPU-Beschleunigung** (CUDA/Metal/Vulkan) für hohe Durchsätze
- 🧩 **Plugin-Architektur** mit `LlamaWrapper`
- 🗃️ **Lazy Model Loading** (Ollama‑Style)
- 🔀 **Multi‑LoRA Management** (vLLM‑Style)

**Quicklinks (v1.16.0):**
- [📝 Grammatik-gesteuerte Generierung](../../en/llm/GRAMMAR_CONSTRAINED_GENERATION.md)
- [🖼️ Vision Support Quick Start](../../en/llm/VISION_SUPPORT_QUICK_START.md) (experimentell)
- [⚡ Flash Attention Implementierung](../../en/llm/FLASH_ATTENTION_IMPLEMENTATION.md)
- [🎯 Speculative Decoding](../../en/llm/SPECULATIVE_DECODING_IMPLEMENTATION.md)
- [🔄 Continuous Batching](../../en/llm/CONTINUOUS_BATCHING_IMPLEMENTATION.md)
- [🔧 Feature Implementation Guide](./LLAMA_CPP_FEATURE_IMPLEMENTATION_GUIDE.md)
- [README_PLUGINS.md](./README_PLUGINS.md) – Plugin-Schnellstart
- [inventory.md](./inventory.md) – Vollständiges Modul-Inventar
- [MISSING_IMPLEMENTATIONS.md](./MISSING_IMPLEMENTATIONS.md) – Bekannte Lücken

## 📚 Feature-Dokumentation (v1.16.0)

| Feature | Anleitung | Status |
|---------|-----------|--------|
| 📝 Grammatik-gesteuerte Generierung | [GRAMMAR_CONSTRAINED_GENERATION.md](../../en/llm/GRAMMAR_CONSTRAINED_GENERATION.md) | ✅ Produktionsreif |
| 🖼️ Vision Support (Multi-Modal) | [VISION_SUPPORT_QUICK_START.md](../../en/llm/VISION_SUPPORT_QUICK_START.md) | ⚠️ Experimentell |
| ⚡ Flash Attention CUDA | [FLASH_ATTENTION_IMPLEMENTATION.md](../../en/llm/FLASH_ATTENTION_IMPLEMENTATION.md) | ✅ Produktionsreif |
| 🎯 Speculative Decoding | [SPECULATIVE_DECODING_IMPLEMENTATION.md](../../en/llm/SPECULATIVE_DECODING_IMPLEMENTATION.md) | ✅ Produktionsreif |
| 🔄 Continuous Batching | [CONTINUOUS_BATCHING_IMPLEMENTATION.md](../../en/llm/CONTINUOUS_BATCHING_IMPLEMENTATION.md) | ✅ Produktionsreif |
| 💾 KV-Cache Reuse | [KV_CACHE_REUSE_IMPLEMENTATION.md](../../en/llm/KV_CACHE_REUSE_IMPLEMENTATION.md) | ✅ Produktionsreif |
| 📊 Embeddings Extraction | [EMBEDDINGS_EXTRACTION_IMPLEMENTATION.md](../../en/llm/EMBEDDINGS_EXTRACTION_IMPLEMENTATION.md) | ✅ Produktionsreif |
| 🔌 OpenAI-Adapter | `src/llm/openai_compat_adapter.cpp` | ✅ Produktionsreif |
| 📡 Streaming SSE | `src/llm/streaming_handler.cpp` | ✅ Produktionsreif |
| 🔀 LoRA Hot-Loading | `src/llm/inference_engine_enhanced.cpp` | ✅ Produktionsreif |
| 🎯 Function/Tool Calling | `src/llm/json_schema_converter.cpp` | ✅ Produktionsreif |
| 📦 Dedup-Cache | `src/llm/llm_response_cache.cpp` | ✅ Produktionsreif |
| 🧮 Quantization Pipeline | `src/llm/model_quantization_pipeline.cpp` | ✅ Produktionsreif |

## Source-Code Referenz

Vollständiges Inventar aller Quelldateien und Header: **[inventory.md](./inventory.md)**

**Kern-Dateien:**

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| AsyncInferenceEngine | `async_inference_engine.h` | `async_inference_engine.cpp` | Leichtgewichtige Async-Inferenz |
| InferenceEngineEnhanced | `inference_engine_enhanced.h` | `inference_engine_enhanced.cpp` | Enterprise Multi-Model-Engine |
| InferenceHandle | `inference_handle.h` | `inference_handle.cpp` | Async-Request-Handle |
| LlamaWrapper | `llama_wrapper.h` | `llama_wrapper.cpp` | llama.cpp C-API-Wrapper |
| OpenAICompatAdapter | `openai_compat_adapter.h` | `openai_compat_adapter.cpp` | OpenAI `/v1/chat/completions` |
| StreamingHandler | `streaming_handler.h` | `streaming_handler.cpp` | SSE/WebSocket-Streaming |
| Grammar | `grammar.h` | `grammar.cpp` + `llama_grammar_adapter.cpp` | EBNF-Grammar-Constrained |
| SharedWorkerPool | `shared_worker_pool.h` | `shared_worker_pool.cpp` | Geteilter Work-Stealing-Pool |
| LLMInteractionStore | `llm_interaction_store.h` | `llm_interaction_store.cpp` | Interaction Storage |
| ModelQuantizationPipeline | `model_quantization_pipeline.h` | `model_quantization_pipeline.cpp` | GGUF/AWQ/GPTQ Loader |

## Implementierte Klassen

### LLMInteractionStore

```cpp
class LLMInteractionStore {
    // Key format: "llm_interaction:{interaction_id}"
    
    struct Interaction {
        std::string id;                        // UUID
        std::string prompt_template_id;        // Referenz auf Template
        std::string prompt;                    // Gesendeter Prompt
        std::vector<std::string> reasoning_chain; // Chain-of-Thought Steps
        std::string response;                  // LLM Response
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

### Feedback-System
Das Feedback-System ist als Enterprise Add-on implementiert und nutzt das flexible `metadata`-Feld. Es erfordert **keinen separaten Layer**.

**Use Cases:**
- User-Feedback für LLM-Antworten sammeln
- Trainingsdaten für LoRa Fine-Tuning markieren
- Qualitätsmetriken tracking

**Siehe:** [LLM Feedback Enterprise](./LLM_FEEDBACK_ENTERPRISE.md)

### Query Enhancement
Kombiniert DB-Abfragen mit LLM-Kontext für KI-gestützte Anwendungen.

**Vorteile:**
- 49% Kostenreduktion
- 38% Latenz-Verbesserung
- +25-40% Qualitätsverbesserung
- Real-time Feedback-Loop

**Siehe:** [LLM Integration Benefits Analysis](../enterprise/LLM_INTEGRATION_BENEFITS_ANALYSIS.md)

## Verwandte Dokumentation

### LLM Integration & Distributed Reasoning

- [**Docker/VM Deployment Guide**](./DOCKER_VM_DEPLOYMENT_GUIDE.md) ⭐ **NEU:** Deployment in Containern und VMs
  - **GPU Passthrough:** Docker NVIDIA Toolkit, KVM/QEMU, VMware vSphere
  - **CPU Fallback Mode:** Vollständiges Testing OHNE GPU (5-10x langsamer, aber funktional)
  - **Mixed Mode:** Hybrid-Cluster mit GPU + CPU Shards
  - **Multi-Shard Testing:** Komplett in Docker ohne GPU möglich
  - **Kubernetes:** GPU Device Plugin, StatefulSets, Health Probes
  - **Performance:** GPU (100%), CPU (15-20%), Mixed (90%/15%)

- [**Monitoring & Testing Strategy**](./MONITORING_TESTING_STRATEGY.md) ⭐ **NEU:** Observability & QA
  - **6 Grafana Dashboards:** Cluster Overview, Inter-Cerebral Communication, LLM Performance, Vector Search, Distributed Reasoning, Cost/ROI
  - **Inter-Cerebral Monitoring:** Brain-inspired Shard-zu-Shard Kommunikationsvisualisierung
  - **40+ Prometheus Metriken:** Counter, Gauge, Histogram für alle Komponenten
  - **Testing Pyramid:** 70% Unit, 25% Integration, 5% E2E Tests
  - **CI/CD Integration:** GitHub Actions mit GPU/CPU Testing
  - **Benchmarks:** Inference Throughput, Distributed Reasoning, LoRA Transfer, Vector Search
  - **Expected:** 99.9% Uptime, <2s p95 Latency, >70% VRAM Utilization

- [**Enterprise VRAM Licensing**](./ENTERPRISE_VRAM_LICENSING.md) ⭐ **NEU:** VRAM-basiertes Lizenzmodell
  - **Community Edition (Free):** ≤24 GB VRAM - 80% aller Use Cases abgedeckt
  - **Enterprise Edition:** >24 GB VRAM - Llama-70B+, Multi-GPU, HA-Cluster
  - **Technische Implementierung:** VRAMLicenseManager, Runtime Enforcement
  - **Pricing:** €5,000-€50,000/Jahr je nach VRAM-Tier
  - **ROI:** 90-99% Einsparung vs. Hyperscaler
  - **Free Trial:** 30 Tage Enterprise testen

- [**Distributed Reasoning Architecture**](./DISTRIBUTED_REASONING_ARCHITECTURE.md) ⭐ **NEU:** Verteiltes Denken wie im Gehirn
  - **Brain-Inspired Multi-Shard Collaboration** - Spezialisierte Shards wie Gehirnregionen
  - **Parallel Chain-of-Thought (CoT)** - Multi-Step Tasks 3-5x schneller durch Parallelisierung
  - **Multi-Perspective Reasoning** - Verschiedene LoRA-Adapter analysieren aus verschiedenen Blickwinkeln
  - **Hierarchical Task Decomposition** - Orchestrator zerlegt komplexe Tasks automatisch
  - **Performance:** 3.6x schneller, 21x günstiger als GPT-4 bei Multi-Step Tasks
  - **Use Cases:** Legal Contract Analysis, Medical Diagnosis, Scientific Research
  - **Roadmap:** v1.5.0 (Q3 2026)

- [**GPU-Tier Analysis & Hyperscaler Comparison**](./GPU_TIER_ANALYSIS_HYPERSCALER_COMPARISON.md) ⭐ **NEU:** SLM/LLM Performance-Analyse
  - Entry-Level (<16GB): RTX 4060 Ti + Phi-3-Mini - €0.02/1M tokens, 1500x günstiger
  - Mid-Range (<24GB): RTX 4090 + Mistral-7B - €0.05/1M tokens, 600x günstiger, **Best ROI**
  - High-End (>24GB): A100 + Llama-3-70B - €0.15/1M tokens, 200x günstiger
  - 3-Jahr TCO: €9,801 (RTX 4090) vs. €835,200 (Azure) = 99% Einsparung
  - Break-Even: 2.3 Monate (Mid-Range), 6.5 Monate (High-End)
  - Use-Case-spezifische Empfehlungen

- [Native LLM Integration Concept](./NATIVE_LLM_INTEGRATION_CONCEPT.md) ⭐ **NEU:** Technisches Konzept für direkte LLM Integration
  - Zero-Copy Memory Access (Apache Arrow, PyTorch Best Practices)
  - Unified Memory Space (CUDA Unified Memory)
  - Lazy Loading & Streaming (HuggingFace Datasets)
  - Continuous Batching (vLLM-style)
  - PagedAttention KV Cache
  - LoRA Fusion Engine (PEFT)
  - **Performance:** 4x schneller, 6x VRAM-effizienter
  - **Roadmap:** v1.5.0 (Q3 2026)

- [Zero-Copy Memory Access](./ZERO_COPY_MEMORY_ACCESS.md) ⭐ **NEU:** Intra-Ops vs. Inter-Ops Kommunikation
  - Shared Memory für same-host deployment
  - GPU Direct Memory Access (GPUDirect)
  - gRPC/Protobuf für distributed deployment
  - mTLS Security Layer
  - **Performance:** 0ms (GPU Direct) bis 12s (gRPC compressed)

- [AI Ecosystem Sharding Architecture](./AI_ECOSYSTEM_SHARDING_ARCHITECTURE.md) - Komplettes AI-Ökosystem mit horizontalem Sharding
  - Jede Shard mit eigenem LLM (llama.cpp + GPU)
  - Dynamic LoRA Transfer zwischen Shards
  - Federated RAG Queries über mehrere Domains
  - Raft-basierte Model Replication
  - Production-ready mit 3-10+ Shards

- [LLM Loader Guide](./LLM_LOADER_GUIDE.md) - Vollständiger Implementierungsleitfaden für vLLM-ähnlichen LLM Loader
  - Benötigte Bibliotheken (llama.cpp, GGML, Protobuf, etc.)
  - Architektur und Integration mit ThemisDB
  - Model Loading, LoRA Support, Inference
  - HTTP API und Code-Beispiele
  
- [Binary Communication Protocols](./BINARY_COMMUNICATION_PROTOCOLS.md) - Kommunikation zwischen ThemisDB und vLLM
  - HTTP/JSON vs. gRPC/Protobuf (binär)
  - Unix Domain Sockets und Shared Memory
  - Performance-Vergleiche und Entscheidungshilfe
  - Code-Beispiele für alle Protokolle
  
- [GPU Referencing Capabilities](./GPU_REFERENCING_CAPABILITIES.md) - Rechenintensives Referencing im VRAM
  - FAISS GPU Backend (bis zu 31x schneller)
  - Hybrid Search (BM25 + Vector) für RAG
  - Batch Processing und Semantic Caching
  - Co-Located Deployment (ThemisDB + vLLM auf einer GPU)
  - Performance-Benchmarks und Skalierung

### Weitere Ressourcen

- [Features: Semantic Cache](../features/features_semantic_cache.md) - LLM Response Caching
- [Sharding Overview](../sharding/sharding_overview.md) - Horizontales Sharding in ThemisDB
- [AQL Prompt Engineering](../AQL_PROMPT_ENGINEERING.md)
- [Projects: RAG LLM Programmierhilfe](../projects/RAG_LLM_PROGRAMMIERHILFE.md)
- [Roadmap](../roadmap/ROADMAP.md) - ThemisDB Roadmap (v1.5.0 LLM Integration)
