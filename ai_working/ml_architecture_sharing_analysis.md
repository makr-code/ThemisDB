# ThemisDB ML-Architektur: Sharing & Synergien Analyse
**Tiefenanalyse bestehender Shared Infrastructure und Cross-Module Opportunities**

<!-- Status: 2026-05-11 | Focus: Caching, Scheduling, Evaluation, Self-Awareness, RAID-Routing, LLM-as-Judge, Prompt-Enhancement -->

## Executive Summary

**These:** Die 9 ML-Module teilen bereits **5 kritische Schichten**, haben aber **keine konsistente Abstraction**:
1. ✅ **Caching** — 7× in LLM, 1× in RAG, 1× in Training (keine zentrale Policy)
2. ✅ **Scheduling** — ContinuousBatchScheduler in LLM, implizit in RAG/Training
3. ✅ **Evaluation** — RAGJudge + EthicsEvaluator + Calibrator (3 separate Implementierungen)
4. ✅ **Self-Awareness** — Tree-of-Thoughts, Reflection, Debates, Self-RAG (fragmentiert)
5. ✅ **LLM-as-Judge Pattern** — Kernel von RAG, Ethics, Self-RAG, Constitutional AI (nicht abstrahiert)
6. ✅ **RAID-Aware Routing** — Least-Loaded Heuristic (könnten ML-Optimierer nutzen)
7. ✅ **Prompt Management** — Nur für User-Prompts, nicht für System-Prompts anderer Module

**Empfehlung:** 
- ✅ Schaffung **flacher, interface-basierter** Abstractions-Schicht
- ✅ Adapter-Patterns für bestehende Klassen (RAGJudge → IEvaluator, AdaptiveShardRouter → IMLRouter)
- ✅ Zentrale Koordination von Caching, Scheduling, Evaluation, Self-Awareness
- ✅ **Keine hierarchischen Subdirs** — alles parallel in `src/` und `include/`

**Nutzen:** 35% Code-Reduktion bei Wave A/B/C, konsistente ML-Kontrollflo ̈usse, zentrale Metrics-Aggregation, besseres Cross-Module Sharing.

---

## 1. Bestehende Shared Infrastructure im Sourcecode

### 1.1 Caching-Schicht (8 Implementierungen, keine zentrale Policy)

| Cache-Typ | Modul | Zweck | Pattern |
|-----------|-------|-------|---------|
| **KV-Cache (Paged)** | llm/ | Attention-State Reuse | Block allocation, LRU-Eviction |
| **Prefix-Cache** | llm/ | Token-Reuse in Prefix | Exact-match keys, no eviction |
| **Grammar-Cache** | llm/ | Token-Masking Rules (Regex) | LRU + compile caching |
| **LoRA-Metadata-Cache** | llm/ | Adapter Weights | LRU + versioning |
| **Model-Metadata-Cache** | llm/ | Model Config/Tokenizer | TTL + file-watch invalidation |
| **LLM-Response-Cache** | llm/ | Token Sequences | LRU + embedding-similarity lookup |
| **Buffer-Cache (Paged)** | llm/ | GPU Memory Management | Block allocation + tracking |
| **Evaluation-Cache** | rag/ | Judge Results | LRU + TTL + manual invalidation callback |
| **KG-Enricher-LRU** | training/ | Entity Embeddings | LRU (implicit in loop) |

**Erkannte Redundanzen:**
- 8 separate Implementierungen von LRU/TTL
- Keine zentrale Eviction-Policy
- Verschiedene Thread-Safety-Patterns (mutex, spinlock, tbb::concurrent_hash_map)
- Keine Cross-Module Cache-Invalidation

**Opportunity: `ICachePolicy<K, V>` Interface abstrahieren:**
```cpp
// include/core/cache_policy.h
template<typename Key, typename Value>
class ICachePolicy {
    virtual Result<Value> get(const Key& k) = 0;
    virtual void put(const Key& k, const Value& v, std::optional<TTL> ttl = {}) = 0;
    virtual void invalidateIf(std::function<bool(const Key&)> predicate) = 0;
    virtual Metrics getMetrics() const = 0; // hit_rate, eviction_count, size_bytes
};

// Implementierungen:
// - LRUCachePolicy<K, V> — Fixed size, LRU eviction
// - LRUWithTTLCachePolicy<K, V> — LRU + time-based expiration
// - ExactMatchCachePolicy<K, V> — No eviction (Prefix-Cache use case)
// - PinningCachePolicy<K, V> — LRU with hot-key pinning (KV-Cache für frequent prompts)
```

**Nutzen bei Wave A/B/C:**
- A2 (DPR): Cache Query-Embedding-Pairs für häufige Queries
- B1 (Self-RAG): Cache Relevance-Judge Results über Retrieval-Loops
- B2 (KG Completion): Cache Link-Prediction Results für schnellere Convergence
- C1 (Constitutional AI): Cache Critic-Feedback über Debate-Runden

---

### 1.2 Scheduling-Schicht (Nur LLM explizit, andere implizit)

| Komponente | Modul | Mechanismus | Koordination |
|-----------|-------|-------------|--------------|
| **ContinuousBatchScheduler** | llm/ | Request → Batch Mapping | KV-budget guard, Request Queue |
| **StreamingRetriever** | rag/ | Sequential Chunk Fetching | Implicit (kein explizites Scheduling) |
| **LoRA-Loading Queue** | training/ | Hot-Load Trigger | Ad-hoc (keine Queuing-Strategie) |
| **BatchEvaluator** | rag/ | Parallel Worker Pool | Fixed thread count (keine Backpressure) |

**Erkannte Probleme:**
- Wenn RAG + LLM parallel laufen, gibt es **keinen globalen Backpressure**
- Training kann Token-Batch von LLM verpassen, wenn Shard überlastet
- RAID-Sharding sieht nur Least-Loaded, nicht Quality-Requirements

**Opportunity: `IMLScheduler` für globale Koordination:**
```cpp
// include/core/ml_scheduler.h
class IMLTask {
    enum class Priority { LOW = 1, NORMAL = 5, HIGH = 10, CRITICAL = 100 };
    virtual Priority getPriority() const = 0;
    virtual size_t getEstimatedComputeUnits() const = 0; // GPU/CPU resources
};

class IMLScheduler {
    // Enqueue Task mit Backpressure
    virtual Result<ScheduleHandle> enqueue(std::shared_ptr<IMLTask> task) = 0;
    
    // Resource Management
    virtual void setResourceBudget(ResourceType r, size_t budget) = 0;
    virtual ResourceMetrics getResourceMetrics(ResourceType r) = 0;
    
    // Metrics
    virtual Metrics getMetrics() = 0; // throughput, latency_p95, queued_tasks
};

// Implementierungen:
// - ContinuousBatchSchedulerAdapter (wraps LLM's ContinuousBatchScheduler)
// - GlobalMLScheduler (coordinates LLM + RAG + Training)
// - SelfAwareScheduler (adjusts based on task quality vs. latency)
```

**Nutzen bei Wave A/B/C:**
- B1 (Self-RAG): `IMLScheduler::enqueue()` für adaptive Retrieval-Tasks
- B2 (KG Completion): Link-Prediction Tasks compete fair mit Inference
- C1 (Constitutional AI): Critic-Tasks get Priority::HIGH to reduce latency
- A1 (Speculative Decoding): Draft-Token generation via `IMLScheduler`

---

### 1.3 Evaluation-Schicht (3 separate Evaluators, keine gemeinsame Basis)

| Evaluator | Modul | Scores | Caching | Aggregation |
|-----------|-------|--------|---------|-------------|
| **RAGJudge** | rag/ | Faithfulness, Relevance, Completeness, Coherence, KG-Gap | EvaluationCache + Calibration | Multi-Judge ensemble |
| **EthicsEvaluator** | ethics_ai/ | Autonomy, Justice, Beneficence, Non-Maleficence, Transparency | In-memory (kein Cache) | Einzeln (kein Ensemble) |
| **ConfidenceCalibrator** | training/ | Scaled Scores via Temperature/Platt/Isotonic | In-memory (Modell-Coeff) | Chain-dependent (kein Standalone) |

**Erkannte Redundanzen:**
```cpp
// RAGJudge: 5D-Scoring
struct RAGJudgeConfig { 
    std::array<float, 5> weights;    // [faith, relev, complet, coher, cg]
    EvaluationMode mode;              // FAST/BALANCED/THOROUGH
};
class RAGJudge { Result<Scores> evaluate(...) { /*...5D */ }};

// EthicsEvaluator: 5D-Scoring — REDUNDANZ!
struct EthicsConfig { 
    std::array<float, 5> weights;    // [auto, just, benef, non-mal, trans]
    bool use_debate;                  // different config structure
};
class EthicsEvaluator { Result<Scores> evaluate(...) { /*...5D */ }};

// Problem: Keine gemeinsame Basis → Code-Duplikation in:
// - Test (je 20+ unit tests pro Evaluator)
// - Serialisierung (JSON ↔ Score mapping)
// - Aggregation (Welche Strategie? MEAN/WEIGHTED/MAJORITY?)
// - Metrics (je separate Prometheus counters)
```

**Opportunity: `IEvaluator` + `MultiEvaluatorEnsemble` abstrahieren:**
```cpp
// include/core/evaluator.h
struct EvaluationScores {
    std::vector<float> scores;        // n-dimensional, any semantics
    std::vector<std::string> labels;  // ["faithfulness", "relevance", ...]
    float aggregated_score;           // 0..1, summary score
    Metrics metadata;                 // latency, model_used, confidence
};

class IEvaluator {
    virtual Result<EvaluationScores> evaluate(const EvaluationInput&) = 0;
    virtual Metrics getMetrics() = 0;
    virtual std::string getName() const = 0;
};

class MultiEvaluatorEnsemble : public IEvaluator {
    void addEvaluator(const std::string& name, std::shared_ptr<IEvaluator> eval);
    
    // Aggregation Strategies
    enum class Strategy { MEAN, WEIGHTED_MEAN, MAJORITY_VOTING, BEST_OF_N };
    Result<EvaluationScores> evaluate(...) override {
        // 1. Call all registered evaluators in parallel (via IMLScheduler)
        // 2. Aggregate scores per strategy
        // 3. Compute confidence based on agreement
    }
};

// Adapter 1: RAG
class RAGJudgeAdapter : public IEvaluator {
    std::shared_ptr<RAGJudge> judge_;
    Result<EvaluationScores> evaluate(...) override {
        auto scores = judge_.evaluate(...);
        return EvaluationScores {
            .scores = {faith, relev, complet, coher, cg},
            .labels = {"faithfulness", "relevance", ...}
        };
    }
};

// Adapter 2: Ethics
class EthicsEvaluatorAdapter : public IEvaluator { /* ... */ };

// Adapter 3: Calibrator
class CalibratedEvaluatorAdapter : public IEvaluator {
    std::shared_ptr<IEvaluator> base_;
    std::shared_ptr<CalibrationManager> calibrator_;
    Result<EvaluationScores> evaluate(...) override {
        auto scores = base_->evaluate(...);
        calibrator_->calibrate(scores); // in-place
        return scores;
    }
};
```

**Nutzen bei Wave A/B/C:**
- A3 (Fairness Detection): Pluggt als `IEvaluator` in Ensemble
- B1 (Self-RAG): Nutzt MultiEvaluatorEnsemble für Adaptive Retrieval (3× parallel judges)
- B2 (KG Completion): Link-Prediction uses `IEvaluator` for confidence scoring
- C1 (Constitutional AI): Orchestriert RAG + Ethics + Custom Evaluators
- **Zentrale Metrics:** All evaluators feed into unified Prometheus namespace

---

### 1.4 Self-Awareness Pattern (Fragmentiert über 5 Module)

| Komponente | Modul | Self-* Mechanism | Koordination |
|-----------|-------|------------------|--------------|
| **Chain-of-Thought** | prompt_eng/ | Thought decomposition | Sequential steps logged |
| **Reflection Tuning** | prompt_eng/ | Self-critique via prompt | Iterative rewriting |
| **Tree-of-Thoughts** | prompt_eng/ | Multi-path exploration | Search with scoring |
| **EthicalDiscourseEngine** | ethics_ai/ | Multi-round debate | Counter-arguments stored |
| **ArgumentStore + ChainVisualizer** | ethics_ai/ | Debate-Graph persistence | DOT/Mermaid export |
| **Self-RAG (planned B1)** | rag/ (planned) | Adaptive retrieval | I-don't-know scoring |
| **Constitutional AI (planned C1)** | ethics_ai/ (planned) | LLM+Critic roleplay | Debate-based refinement |

**Erkannte Muster:**
- CoT: Selbst-Planung (denk schrittweise)
- Reflection: Selbst-Kritik (was könnte falsch sein?)
- Tree-of-Thoughts: Selbst-Exploration (multiple Pfade)
- Ethics Debate: Selbst-Argumentation (pro/contra)
- Self-RAG: Selbst-Entscheidung (mehr Kontext nötig?)
- Constitutional AI: Selbst-Verbesserung (via Critic feedback)

**Opportunity: `ISelfAwareComponent` zentral orchestrieren:**
```cpp
// include/core/self_aware_component.h
enum class ConfidenceLevel { VERY_LOW = 1, LOW = 3, MEDIUM = 5, HIGH = 8, VERY_HIGH = 10 };

class ISelfAwareComponent {
    // Selbst-Evaluierung
    virtual Result<ConfidenceScore> getSelfConfidence(const ExecutionContext&) = 0;
    
    // Selbst-Kritik (was könnte falsch sein?)
    virtual Result<CritiqueResult> critique(const ExecutionTrace&) = 0;
    
    // Selbst-Verbesserung (wie kann ich es besser machen?)
    virtual Result<ImprovementPlan> planImprovement(const CritiqueResult&) = 0;
    
    // Selbst-Entscheidung (sollte ich weitermachen oder neuen Pfad versuchen?)
    virtual Result<bool> shouldContinue(const ProgressMetrics&) = 0;
    
    // Selbst-Erklärung (warum habe ich das so gemacht?)
    virtual Result<std::string> explainDecision(const Decision&) = 0;
};

class SelfAwareOrchestrator {
    // Zentrale Koordination über alle Components
    Result<ExecutionResult> executeWithSelfAwareness(
        ISelfAwareComponent* component,
        const ExecutionContext& ctx,
        const SelfAwarenessConfig& config
    );
    
    // Monitoring
    void registerSelfAwarenessMetrics(IEvaluator* judge);
};

// Adapter 1: CoT als Self-Awareness
class ChainOfThoughtComponent : public ISelfAwareComponent { /* ... */ };

// Adapter 2: Self-RAG
class SelfRAGComponent : public ISelfAwareComponent {
    Result<ConfidenceScore> getSelfConfidence(...) override {
        // Nutzt "I-don't-know" token probability
    }
    Result<bool> shouldContinue(...) override {
        // Adaptive retrieval decision: need more context?
    }
};

// Adapter 3: Constitutional AI
class ConstitutionalAIComponent : public ISelfAwareComponent {
    Result<CritiqueResult> critique(...) override {
        // LLM-Critic evaluates LLM-Agent
    }
};
```

**Nutzen bei Wave A/B/C:**
- **Self-RAG (B1):** orchestriert via `SelfAwareOrchestrator` (Adaptive Retrieval)
- **Constitutional AI (C1):** critique-loop via `ISelfAwareComponent`
- **Fairness (A3):** Self-evaluation des Model-Bias
- **Training (Multi-task B3):** Early-stopping via `shouldContinue()`

---

### 1.5 LLM-as-Judge Pattern (Kern von RAG, Ethics, Self-RAG, Constitutional AI)

**Aktuell (fragmentiert):**
```cpp
// src/rag/rag_judge.cpp
class RAGJudge {
    Result<EvaluationResult> evaluate(Query q, Document d) {
        // Nutzt LLM-Calls um 5D Scores zu messen
        // Fallback: Heuristic Scorer wenn LLM timeout
    }
};

// src/ethics_ai/ethics_discourse_engine.cpp
class EthicalDiscourseEngine {
    // Nutzt LLM für Argument-Generierung
    Result<Argument> generateRebuttal(const Argument& original) {
        // "LLM, provide counter-argument"
    }
};

// src/prompt_engineering/protegi_optimizer.cpp
class ProTeGiOptimizer {
    // Nutzt LLM als Gradient-Generator (LLM-as-Critic)
    Result<GradientHint> getLLMGradient(const ExecutionTrace&) {
        // "LLM, give feedback to improve this prompt"
    }
};
```

**Problem:** Jeder nutzt LLM anders (verschiedene Prompts, verschiedene Modelle, verschiedene Error-Handling).

**Opportunity: `ILLMBasedJudge` als vereinheitlichtes Pattern:**
```cpp
// include/core_judges/llm_based_judge.h
class ILLMBasedJudge : public IEvaluator {
    virtual Result<EvaluationScores> evaluate(const EvaluationInput&) override = 0;
    
    // LLM-spezifisch
    virtual void setLLMModel(const std::string& model_name) = 0;
    virtual void setSystemPrompt(const std::string& prompt) = 0;
    virtual void setTemperature(float temp) = 0;
    virtual std::string getSystemPrompt() const = 0;
};

// Konkrete Implementierungen (flache Struktur):
// - include/core_judges/faithfulness_judge.h → RAGJudge (LLM-based)
// - include/core_judges/relevance_judge.h → Self-RAG (LLM-based)
// - include/core_judges/critic_judge.h → Constitutional AI (LLM-based)
// - include/core_judges/fairness_judge.h → Wave A3 (LLM-based)
// - include/core_judges/argument_generator.h → Ethics Debates (LLM-based)

// Zentrale Koordination:
class LLMJudgeOrchestrator {
    void registerJudge(const std::string& name, std::shared_ptr<ILLMBasedJudge> judge);
    
    // Routing: welcher LLM sollte welchen Judge ausführen?
    std::shared_ptr<ILLMBasedJudge> selectJudge(
        const EvaluationInput&,
        SelectionStrategy strategy = SelectionStrategy::LEAST_LOADED
    );
    
    // Aggregation: wie kombiniere ich Ergebnisse mehrerer Judges?
    Result<EvaluationScores> evaluateWithStrategy(
        const EvaluationInput&,
        AggregationStrategy strategy = AggregationStrategy::ENSEMBLE
    );
    
    // Metrics
    Metrics getMetrics(); // latency, model_usage, confidence
};
```

**Synergien bei Wave A/B/C:**
- **A3 (Fairness Detection):** Pluggt als `ILLMBasedJudge` in Ensemble
- **B1 (Self-RAG):** Nutzt `LLMJudgeOrchestrator` für Multi-Judge Adaptive Retrieval (Relevance + Usefulness in parallel)
- **B2 (KG Completion):** Link-Prediction scoring via Fairness Judge
- **C1 (Constitutional AI):** Orchestriert RAGJudge + CriticJudge + CustomJudges
- **Zentrale LLM-Usage Metrics:** All judges feed into unified monitoring

---

### 1.6 RAID-Sharding mit ML-Aware Routing (Stark untergenutzte Synergien)

**Aktuell (llm/):**
```cpp
// include/llm/adaptive_shard_router.h
class AdaptiveShardRouter {
    // Heuristic: select shard with min(queue_depth + est_latency)
    ShardSelection selectShard(const Request& req, const ShardStats& current) {
        // reactive only, no prediction
    }
};

// Problem:
// 1. Keine Prognose (Was könnte überlastend werden?)
// 2. Keine Qualität-Metrik (Welcher Shard hat beste Quality?)
// 3. Keine Multi-Modul Koordination (RAID + RAG + Training share hardware)
```

**Opportunity: ML-Aware Routing via `IShardRouter` abstrahieren:**
```cpp
// include/core_routing/shard_router.h
struct ShardRoutingContext {
    EvaluationInput quality_requirements;  // optional: nur if quality matters
    ResourceConstraints constraints;       // prefer GPU? low-latency shard?
};

class IShardRouter {
    virtual Result<ShardSelection> selectShard(
        const Request& req,
        const ShardStats& current_load,
        const std::optional<ShardRoutingContext>& ml_context = {}
    ) = 0;
    
    virtual Metrics getMetrics() = 0;
};

// Implementierung 1: RAID-native (least-loaded)
class RaidAdaptiveRouter : public IShardRouter {
    ShardSelection selectShard(...) override {
        // min(queue_depth, est_latency) — current behavior
    }
};

// Implementierung 2: ML-aware (Wave B/C)
class MLAwareShardRouter : public IShardRouter {
    IEvaluator* quality_judge_;
    IOptimizer* load_predictor_;
    
    ShardSelection selectShard(...) override {
        // 1. Prädiktive Last-Schätzung via Optimizer (Bayesian)
        // 2. Quality-Check via Judge (wenn verfügbar)
        // 3. Routing: quality vs. latency trade-off optimization
        // → Minimiert Timeout-Fehler bei überlastetem Shard
    }
};

// Implementierung 3: Self-Aware Routing (Wave C)
class SelfAwareShardRouter : public IShardRouter {
    ISelfAwareComponent* shard_monitor_;
    
    ShardSelection selectShard(...) override {
        // Shard reports self-confidence level
        // Router adjusts based on shard feedback
    }
};
```

**Nutzen bei Wave A/B/C:**
- **A1 (Speculative Decoding):** Routet Draft-Token zu low-latency Shard
- **B2 (KG Completion):** Link-Prediction routet zu Shard mit bester Cache-Hit-Rate
- **B3 (Multi-Task LoRA):** LoRA-Adapter loading reserviert Shard-Zeit via `IMLScheduler`
- **C1 (Constitutional AI):** Routet Critic-Model zu dediziertem Shard

---

### 1.7 Prompt-Enhancement als Cross-Module Capability (Stark untergenutzt)

**Aktuell (prompt_engineering/):**
```cpp
// include/prompt_engineering/prompt_manager.h
class PromptManager {
    // Verwaltet nur User-facing Prompts
    // keine System-Prompts anderer Module
};

// include/prompt_engineering/protegi_optimizer.cpp
class ProTeGiOptimizer {
    // Optimiert nur User-Query-Prompts
    // keine Cross-Module Coordination
};
```

**Aber andere Module brauchen auch Prompts:**
- `RAGJudge`: Separate System-Prompts für Faithfulness, Relevance, Completeness, ...
- `EthicsEvaluator`: Debate-Prompts basierend auf Philosophy-Profile
- `EthicalDiscourseEngine`: Counter-Argument-Prompts (Philosophy-aware)
- `ConstitutionalAI`: Critic-Prompts basierend auf Constitutional Principles
- `Self-RAG`: Retrieval-Quality-Assessment Prompts

**Opportunity: `IMLPromptRegistry` für zentrale System-Prompt-Verwaltung:**
```cpp
// include/core_prompts/ml_prompt_registry.h
struct PromptTemplate {
    std::string name;
    std::string system_prompt;
    std::string user_prompt_template;  // with placeholders {var1}, {var2}
    std::map<std::string, std::string> default_context;
    std::vector<std::string> tags;     // ["rag", "judge", "faithfulness"]
};

class IMLPromptRegistry {
    // Registration
    void registerPrompt(const PromptTemplate& tpl);
    
    // Rendering
    Result<std::string> renderPrompt(
        const std::string& prompt_name,
        const std::map<std::string, std::string>& context
    );
    
    // Versioning + A/B Testing
    Result<ABTestResult> runABTest(
        const std::string& prompt_name,
        const std::vector<PromptTemplate>& candidates,
        const EvaluationInput& test_data,
        const TestConfig& config
    );
    
    // Metrics
    PromptMetrics getMetrics(const std::string& prompt_name);
};

// Zentrale Registrierung (v1.20.0):
class MLPromptRegistryBootstrap {
    static void registerStandardPrompts(IMLPromptRegistry& registry) {
        // RAG-Prompts
        registry.registerPrompt({
            .name = "rag_judge_faithfulness",
            .system_prompt = "You are a faithfulness evaluator...",
            .tags = {"rag", "judge", "faithfulness"}
        });
        
        // Ethics-Prompts (Philosophy-keyed)
        registry.registerPrompt({
            .name = "ethics_debate_utilitarian",
            .system_prompt = "You argue from utilitarian perspective...",
            .tags = {"ethics", "debate", "utilitarian"}
        });
        
        // Self-RAG-Prompts
        registry.registerPrompt({
            .name = "self_rag_relevance_assessment",
            .system_prompt = "Assess if document is relevant...",
            .tags = {"rag", "self-aware", "relevance"}
        });
        
        // Constitutional-AI Prompts
        registry.registerPrompt({
            .name = "constitutional_ai_critic",
            .system_prompt = "Critique the following response against principles...",
            .tags = {"ethics", "constitutional", "critic"}
        });
    }
};
```

**Nutzen bei Wave A/B/C:**
1. **ProTeGi Auto-Optimization:** Kann alle ML-Prompts optimize statt nur User-Prompts
   ```cpp
   // Old: ProTeGiOptimizer optimiert nur User-Query
   // New: ProTeGiOptimizer kann auch System-Prompts in Registry optimize
   for (const auto& prompt_name : registry.getAllPromptNames()) {
       auto test_result = registry.runABTest(prompt_name, candidates, test_data);
       // Auto-update best variant
   }
   ```

2. **Wave B1 (Self-RAG):** Nutzt zentrale Prompts für Relevance-Scoring
   ```cpp
   auto prompt = registry.renderPrompt("self_rag_relevance_assessment", {
       {"query", user_query},
       {"document", retrieved_doc}
   });
   ```

3. **Wave C1 (Constitutional AI):** Orchestriert Critic-Prompts
   ```cpp
   for (const auto& principle : constitutional_principles) {
       auto prompt_name = "constitutional_ai_critic_" + principle;
       auto prompt = registry.renderPrompt(prompt_name, context);
       // Critic evaluates against this principle
   }
   ```

4. **Zentrale Metrics:**
   ```cpp
   // Track prompt effectiveness über alle Komponenten
   registry.getMetrics("rag_judge_faithfulness").confidence;
   registry.getMetrics("self_rag_relevance_assessment").hit_rate;
   ```

---

## 2. Consolidated Abstraction Layer für Flat Structure

Statt hierarchisches Modul (`src/ml/core/`, `src/ml/bridges/`), empfohlen: **Parallele Header/Source in `core/` **:

```
include/
├── core/
│   ├── cache_policy.h              # NEW: Template-based caching
│   ├── ml_scheduler.h              # NEW: Global task scheduling
│   ├── evaluator.h                 # NEW: IEvaluator abstraction
│   ├── optimizer.h                 # NEW: IOptimizer abstraction
│   ├── self_aware_component.h      # NEW: Self-awareness orchestration
│   ├── shard_router.h              # NEW: ML-aware routing
│   └── ml_prompt_registry.h        # NEW: Prompt management
├── core_judges/
│   ├── llm_based_judge.h           # NEW: ILLMBasedJudge abstraction
│   ├── faithfulness_judge.h        # REFACTOR: from rag/
│   ├── relevance_judge.h           # NEW: for Self-RAG
│   ├── critic_judge.h              # NEW: for Constitutional AI
│   └── fairness_judge.h            # NEW: for Wave A3
├── core_optimizers/
│   ├── grid_search_optimizer.h     # NEW
│   ├── bayesian_optimizer.h        # NEW
│   └── llm_gradient_optimizer.h    # NEW (ProTeGi wrapper)
├── core_routing/
│   ├── shard_router.h              # moved from llm/
│   └── ml_aware_router.h           # NEW
└── ...

src/
├── core/
│   ├── cache_policy.cpp            # Implementations
│   ├── ml_scheduler.cpp
│   ├── multi_evaluator_ensemble.cpp
│   ├── self_aware_orchestrator.cpp
│   ├── ml_prompt_registry.cpp
│   └── shard_router_adapters.cpp
├── core_judges/
│   ├── llm_judge_orchestrator.cpp  # NEW
│   ├── faithfulness_judge.cpp      # REFACTOR
│   ├── relevance_judge.cpp         # NEW
│   ├── critic_judge.cpp            # NEW
│   └── fairness_judge.cpp          # NEW
├── core_optimizers/
│   ├── bayesian_optimizer.cpp      # NEW
│   ├── grid_search_optimizer.cpp   # NEW
│   └── llm_gradient_optimizer.cpp  # NEW
└── ...
```

**Flache Struktur respektiert:**
✅ `src/` und `include/` sind getrennt  
✅ Keine `src/ml/core/`, `src/ml/bridges/` Hierarchie  
✅ Adapter-Pattern statt Umbrella-Modul  
✅ Bestehende Module können Interfaces nutzen ohne Dependency-Zirkeln  

---

## 3. Konkrete Wave A/B/C Synergien

### Wave A (v1.20.0 — Q3 2026)

| Feature | Shared Infrastructure | Synergien |
|---------|----------------------|----------|
| **A1: Speculative Decoding** | `IMLScheduler` + `ICachePolicy` | Draft-Tokens via Scheduler; KV-Cache reuse |
| **A2: DPR Vectorizer** | `IVectorizer.vectorizeQuery()` + `EmbeddingCache` | Bi-encoder interface; cache Query-embeddings |
| **A3: Fairness Detection** | `ILLMBasedJudge` + `MultiEvaluatorEnsemble` | Pluggs in as Fairness Evaluator; parallel with RAG judges |

### Wave B (v1.21.0 — Q4 2026)

| Feature | Shared Infrastructure | Synergien |
|---------|----------------------|----------|
| **B1: Self-RAG** | `ISelfAwareComponent` + `IMLScheduler` + `LLMJudgeOrchestrator` | Adaptive Retrieval via self-confidence; multi-judge scoring |
| **B2: KG Completion** | `IOptimizer` + `IShardRouter` | Link-prediction training; ML-aware shard routing |
| **B3: Multi-Task LoRA** | `IMLScheduler` + `MultiEvaluatorEnsemble` | Task-specific scheduling; shared quality metrics |

### Wave C (v1.22.0 — Q1 2027)

| Feature | Shared Infrastructure | Synergien |
|---------|----------------------|----------|
| **C1: Constitutional AI** | `ISelfAwareComponent` + `LLMJudgeOrchestrator` + `IMLPromptRegistry` | Critic-Judge via LLMJudgeOrchestrator; Philosophy-prompts in Registry |
| **C2: Federated Learning** | `IMLScheduler` + `IShardRouter` | Global task coordination; privacy-aware routing |

---

## 4. Empfehlungen: Implementierungs-Roadmap

### Phase 1: Interface Definition & Adapters (Parallel zu Wave A)
- **Effort:** 3-4 Wochen
- **Deliverable:** 7 neue Header-Interfaces (`core/`, `core_judges/`, `core_optimizers/`)
- **Integration:** Nur Adapter-Code in bestehenden Modulen
- **Non-Breaking:** Ja (nur neue Abstraktion, alte APIs bleiben)

### Phase 2: Zentrale Orchestration (Parallel zu Wave B)
- **Effort:** 2 Wochen
- **Deliverable:** Orchestratoren (LLMJudgeOrchestrator, SelfAwareOrchestrator, MLPromptRegistry)
- **Integration:** Wave B Features nutzen Orchestratoren
- **Breaking:** Nein (opt-in adoption)

### Phase 3: Cross-Module Optimization (Parallel zu Wave C)
- **Effort:** 1 Woche
- **Deliverable:** RAID-Router Integration, Prompt Registry auto-optimization
- **Integration:** Wave C leverages all infrastructure
- **Breaking:** Nein (performance optimization only)

---

## 5. Metrics & Success Criteria

| Metrik | Target | Measurement |
|--------|--------|-------------|
| **Code-Reuse** | -30% Duplikation | Lines of code in Wave A/B/C features |
| **Development Velocity** | Wave B -20% effort | Actual development time |
| **ML-Component Coupling** | < 3 cross-module calls | Dependency analysis |
| **Standardized Interfaces** | 100% of Evaluators implement IEvaluator | Code review checklist |
| **Prompt Effectiveness** | +15% consistency | A/B test results across components |
| **Scheduler Efficiency** | +25% throughput | qps under load, no quality regression |
| **Self-Awareness Coverage** | All critical paths | Trace execution with self-awareness enabled |

---
