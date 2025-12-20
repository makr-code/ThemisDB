# LLM & AI Integration Documentation

**Stand:** 20. Dezember 2025  
**Version:** 1.2.0 (v1.4.0 Multi-Agent LLM, v1.5.0 Native LLM Integration)  
**Kategorie:** LLM & Distributed AI

---

## 🚀 Übersicht

ThemisDB entwickelt sich zur ersten Multi-Model-Datenbank mit **eingebetteter LLM-Engine** und **verteiltem Reasoning**. Das LLM-Modul bietet Speicherung und Verwaltung von LLM-Interaktionen, Prompt-Templates und Chain-of-Thought (CoT) Reasoning.

**Roadmap-Update:**
- ✅ **v1.3.0** (Q1 2026) - Embedded LLM Engine (vorverlegt von v1.5.0)
- 🆕 **v1.4.0** (Q2 2026) - **Multi-Agent LLM Reasoning** (NEU!)
- 🔜 **v1.5.0** (Q3 2026) - Erweiterte LLM-Integration & Optimierungen

**Neue Fähigkeiten in v1.4.0 (Multi-Agent):**
- 👥 **Multi-Agent Orchestration** - Mehrere kleinere LLMs arbeiten kollaborativ
- 🧠 **Multi-Perspective Reasoning** - Verschiedene Sichtweisen durch LoRA-Adapter
- 🔄 **Task Decomposition** - Komplexe Probleme automatisch zerlegen
- 🤝 **Consensus Building** - Intelligente Fusion von Agent-Antworten
- ⚡ **3-5x schneller** - bei komplexen Multi-Step-Tasks durch Parallelisierung
- 💰 **Cost Efficient** - Kleinere Modelle (7B-13B) statt große (70B+)

**Kommende Fähigkeiten in v1.5.0:**
- 🧠 **Embedded llama.cpp** - Native LLM-Engine ohne externe APIs
- ⚡ **Zero-Copy RAG** - Direkte Speicherzugriffe (4x schneller)
- 🔄 **Distributed Reasoning** - Multi-Shard Collaboration wie im Gehirn
- 💰 **100-1000x Kostenersparnis** - vs. AWS/Azure/GCP APIs
- 🎯 **Alle GPU-Tiers** - Entry (<16GB), Mid (<24GB), High-End (>24GB)

## Source-Code Referenz

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| LLMInteractionStore | `llm_interaction_store.h` | `llm_interaction_store.cpp` | Interaction Storage |
| PromptManager | `prompt_manager.h` | `prompt_manager.cpp` | Prompt Templates |
| **MultiAgentOrchestrator** | `multi_agent_orchestrator.h` | `multi_agent_orchestrator.cpp` | **Multi-Agent Coordination (v1.4.0)** |
| **LLMAgent** | `llm_agent.h` | `llm_agent.cpp` | **Individual Agent Logic (v1.4.0)** |
| **AgentRoleRegistry** | `agent_role_registry.h` | - | **Role Management (v1.4.0)** |
| **ConsensusBuilder** | `consensus_builder.h` | `consensus_builder.cpp` | **Result Fusion (v1.4.0)** |
| **LoRARegistry** | `lora_registry.h` | - | **LoRA Adapter Management (v1.4.0)** |

**Gesamt:** 7 Header, 5 Source-Dateien, ~2,500 LOC (inkl. v1.4.0)

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

### Multi-Agent LLM Reasoning (v1.4.0)

- [**Multi-Agent Reasoning Concept**](./MULTI_AGENT_REASONING_CONCEPT.md) ⭐ **NEU:** Vollständiges Konzept für v1.4.0
  - **Best Practices:** AutoGen, LangGraph, MetaGPT, Mixture of Agents
  - **Architektur:** MultiAgentOrchestrator, LLMAgent, ConsensusBuilder, LoRARegistry
  - **Use Cases:** Legal Contract Analysis, Code Review, Research Assistant
  - **Patterns:** Parallel Multi-Perspective, Iterative Refinement, Hierarchical Decomposition
  - **Performance:** 3-5x schneller bei komplexen Tasks durch Parallelisierung
  - **Cost Efficiency:** Kleinere Modelle (7B-13B) statt 70B+
  - **Example Configs:** Legal, Code Review, Research (siehe `config/multi_agent/`)

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
