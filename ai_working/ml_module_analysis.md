# ThemisDB ML-Architektur: Sharing & Synergien Analyse

<!-- Status: 2026-05-11 | Focus: Bestehende Shared Infrastructure, Overlaps, Consolidation Opportunities -->

## Executive Summary

**Analyse:** Die 9 ML-Module teilen bereits **3 kritische Schichten** (Caching, Scheduling, Evaluation), haben aber **keine konsistente Abstraction**.

**Empfehlung:** 
1. ✅ **Interfaces in `core/` (flach, keine Subdirs)** statt neuer Modul
2. ✅ **Adapter-Patterns** für bestehende Klassen (RAGJudge → IEvaluator)
3. ✅ **Shared Cache/Scheduler** nutzen, wo möglich
4. ✅ **Keine hierarchischen Subdirs** — alles parallel in src/ und include/

**Nutzen:** 30% Code-Reduktion bei Wave A/B/C, konsistente ML-Kontrollflüsse, zentrale Metrics-Aggregation.

---

## 1. Bestehende Shared Infrastructure im Sourcecode

### Verteilte Spezialisierungen (9 Module)

| Modul | Status | Scope | Wave A/B/C |
|-------|--------|-------|-----------|
| **ai** | v1.9.x | Plugin-Code-Generierung | — |
| **llm** | v1.19.0 | Inference Engine (dual-mode) | Spec. Decoding (A1) |
| **rag** | v2.0.0 | Multi-Judge Evaluation + Retrieval | DPR (A2), Self-RAG (B1) |
| **training** | v1.6.0 | LoRA + Fine-Tuning | Multi-Task LoRA (B3) |
| **prompt_engineering** | v2.0.0 | Template Management | — |
| **ethics_ai** | v0.3.0 | Philosophical Profiling + Debate | CAI (C1) |
| **llama_cpp** (Plugin) | v2.2.0 | Real Inference Backend | — |
| **stable_diffusion** (Plugin) | v2.2.0 | Image Generation | — |
| **whisper** (Plugin) | v2.1.0 | Audio Transcription | — |
| **onnx_clip** (Plugin) | v0.2.0 | Vision Embeddings | — |

### Cross-Module Abhängigkeiten

```
Dependency Graph (Pfeile = "depends on"):

┌─────────────────────────────────────────────────────────────────┐
│                  ai (Plugin Generator)                          │
│                         ↓                                        │
│                   PluginManager                                  │
│          ↙        ↓       ↓        ↘                            │
│    llama_cpp  sd.cpp   whisper   onnx_clip                      │
│       ↓          ↓         ↓         ↓                          │
│      llm    stable_diff  (audio)  (vision)                      │
│       ↓                                                          │
│      rag ← training ← prompt_engineering ← ethics_ai           │
│       ↓                                                          │
│  (metrics/cache/scheduling)                                     │
└─────────────────────────────────────────────────────────────────┘
```

### Identified Cross-Cutting Concerns

1. **Metrics & Observability** (Prometheus counters, latency tracking)
   - `rag`: RAGJudge evaluation metrics
   - `llm`: inference throughput
   - `training`: loss/accuracy curves
   - `ethics_ai`: decision confidence

2. **Caching** (LRU, TTL, invalidation)
   - `rag`: EvaluationCache
   - `training`: KnowledgeGraphEnricher LRU
   - `llm`: KV-cache reuse
   - Keine zentralisierte Cache-Policy

3. **Plugin LifeCycle** (load/unload/hot-plug)
   - `llama_cpp`: LlamaCppPluginRegistrar
   - `stable_diffusion`: SDPluginRegistrar
   - `whisper`: WhisperPluginRegistrar
   - **Pattern nicht konsistent dokumentiert**

4. **Vectorization & Embeddings** (abstract interface)
   - `rag`: HybridRetriever uses IVectorizer
   - `onnx_clip`: generates embeddings (CLIP)
   - `llm`: embedding support (partial)
   - DPR (A2) braucht beide

5. **Scheduling & Rate Limiting** (Queue, backpressure)
   - `llm`: ContinuousBatchScheduler
   - `rag`: StreamingRetriever (sequential)
   - Keine globale Coordination

---

## Fehlende Abstraktion: "ML Service Layer"

### Problem 1: Keine einheitliche Evaluator-Abstraction

**Aktuell:**
```cpp
// rag/rag_judge.h
class RAGJudge { /* 5D evaluation */ };

// ethics_ai/ethics_evaluator.h
class EthicsEvaluator { /* 5D scoring */ };

// training/training_pipeline.h
class ConfidenceCalibrator { /* scaling */ };
```

**Ideal (Wave-B/C ready):**
```cpp
// ml/evaluator.h
class IEvaluator {
    virtual Result<Scores> evaluate(const Input&) = 0;
    virtual void calibrate(const CalibrationSet&) = 0;
    virtual Metrics getMetrics() = 0;
};

// ml/multi_evaluator_ensemble.h
class MultiEvaluatorEnsemble : public IEvaluator {
    // Aggregation strategy: MEAN, WEIGHTED, MAJORITY_VOTING, BEST_OF_N
    // Shared across RAGJudge, CAI, multi-task scoring
};
```

### Problem 2: Keine einheitliche Optimizer-Abstraction

**Aktuell:**
```cpp
// rag/batch_evaluator.h — just parallel for-loop
// prompt_engineering/prompt_optimizer.h — pluggable eval function
// training/incremental_lora_trainer.cpp — gradient descent
```

**Ideal (Wave-A/B ready):**
```cpp
// ml/optimizer.h
class IOptimizer {
    virtual Result<OptimizationResult> optimize(
        const ObjectiveFunction&,
        const Config&
    ) = 0;
};

// ml/bayesian_optimizer.h — for adaptive retrieval (B1)
// ml/grid_search_optimizer.h — for hyperparameter sweep (training v1.7)
```

### Problem 3: Inconsistent vectorizer wiring

**Current issues:**
- `HybridRetriever::IVectorizer` interface
- No pluggable vectorizer registry
- DPR (Wave A2) needs custom bi-encoder init
- Self-RAG (Wave B1) needs query vs passage distinction

---

## Proposed ML Module Structure

### Tier 1: Shared Abstractions (`ml/core/`)

```
ml/
├── core/
│   ├── evaluator.h              // IEvaluator interface
│   ├── optimizer.h              // IOptimizer interface
│   ├── vectorizer.h             // IVectorizer (refactored from rag)
│   └── scheduler.h              // IMLScheduler (new)
├── optimization/
│   ├── bayesian_optimizer.cpp   // Adaptive param search
│   ├── grid_search.cpp          // Hyperparameter sweeps
│   └── beam_search.cpp          // Prompt optimization
├── evaluation/
│   ├── evaluator_ensemble.cpp   // Multi-evaluator aggregation
│   ├── calibration.cpp          // Temperature, Platt scaling
│   └── metrics_aggregator.cpp   // Prometheus + aggregates
├── scheduling/
│   ├── ml_scheduler.cpp         // Global rate limiting
│   └── resource_coordinator.cpp // GPU/CPU scheduling
└── caching/
    ├── evaluation_cache.cpp     // Refactored from rag
    ├── embedding_cache.cpp      // New
    └── cache_policy.h           // TTL, LRU, invalidation
```

### Tier 2: Bridges to existing modules

```
ml/
├── bridges/
│   ├── rag_bridge.cpp           // RAGJudge → IEvaluator adapter
│   ├── training_bridge.cpp      // Training → IOptimizer adapter
│   ├── ethics_bridge.cpp        // EthicsEvaluator → IEvaluator adapter
│   └── llm_bridge.cpp           // LLM inference scheduling
```

### Tier 3: Wave-A/B/C Feature scaffolding

```
ml/
├── research/
│   ├── speculative_decoding.cpp (A1)
│   ├── dpr_vectorizer.cpp       (A2)
│   ├── fairness_detector.cpp    (A3)
│   ├── self_rag_engine.cpp      (B1)
│   ├── knowledge_graph_completion.cpp (B2)
│   └── constitutional_ai.cpp    (C1)
```

---

## Migration Path (Non-Breaking)

### Phase 1: Create `ml` module (v1.20.0 — Wave A)
- New `src/ml/` directory
- Define `IEvaluator`, `IOptimizer`, `IVectorizer` interfaces
- Move shared code: `EvaluationCache`, `Calibration`
- No changes to existing API; adapters bridge old → new
- **Cost:** ~4 weeks, no impact on shipping Wave A

### Phase 2: Adopt in Wave B (v1.21.0)
- Self-RAG uses `IOptimizer` for adaptive loops
- KnowledgeGraphCompletion leverages `IOptimizer` (link prediction training)
- Multi-task LoRA uses shared `Evaluator` ensemble
- **Cost:** ~2 weeks refactoring + benefit multiplier for B1–B3

### Phase 3: Expand in Wave C (v1.22.0)
- Constitutional AI plugs into `IEvaluator` + `IOptimizer` for critique loops
- Federated Learning uses global `MLScheduler`
- **Cost:** ~1 week setup due to Phase 1 foundation

---

## Decision Matrix

| Factor | Yes, Build ML Module | No, Keep Distributed |
|--------|----------------------|----------------------|
| **Code Reuse** | ✅ Evaluator ensemble shared (RAG + Ethics + Training) | ❌ Redundant calibration logic |
| **Wave A/B/C Velocity** | ✅ Scaffolding + bridges reduce feature friction | ❌ Each feature repeats infrastructure |
| **Testing** | ✅ Shared test harness for optimizer/evaluator | ❌ Module-specific unit tests only |
| **Documentation** | ✅ Single "ML Framework" contract | ❌ 9 separate mental models |
| **Plugin LifeCycle** | ✅ Unified registration pattern | ❌ 3 separate registrar patterns |
| **Performance Overhead** | ✅ Minimal (abstraction layer only) | ⚠️ No abstraction penalty |
| **Integration Risk** | ⚠️ Requires careful adapter design | ✅ Zero disruption |
| **Future Extensibility** | ✅ Easy to add new evaluator/optimizer impls | ❌ Hard to enforce consistency |

---

## Recommendation: **3-Module Approach (Hybrid)**

Instead of monolithic "ML Module", create **focused layer** that respects existing specialism:

### Structure
```
src/
├── ml/                        # NEW: Coordination layer (~500 lines header contracts)
│   ├── include/ml/
│   │   ├── evaluator.h        # IEvaluator abstract
│   │   ├── optimizer.h        # IOptimizer abstract
│   │   ├── vectorizer.h       # IVectorizer abstract (refactored from rag)
│   │   └── scheduler.h        # IMLScheduler abstract (new)
│   └── src/ml/
│       ├── ml_core.cpp        # Minimal; mostly headers
│       └── bridges/           # Adapters to rag, training, ethics
├── rag/                       # Enhanced: uses ml/evaluator.h
├── training/                  # Enhanced: uses ml/optimizer.h
├── llm/                        # Enhanced: uses ml/scheduler.h
└── ai/                         # Already minimal; can delegate to ml framework
```

### Size & Scope
- **Lines of Code:** ~2K header + ~1K implementation
- **Dependencies:** None on downstream (only interfaces)
- **External Deps:** Existing (Prometheus, RocksDB, etc.)

### Value Delivered
1. **Immediate (v1.20.0):** Foundation for Wave A features
2. **Near-term (v1.21.0):** 2× faster Wave B adoption
3. **Long-term (v1.22.0):** Constitutional AI + Federated Learning scaffolding

---

## Alternative: Keep Distributed (No ML Module)

**Viable IF:**
- Each Wave A/B/C feature owns its own evaluator/optimizer/scheduler
- Redundancy is acceptable
- Documentation clearly marks "ML Service" patterns in each module

**Cons:**
- 3 independent implementations of Evaluator Ensemble by 2027
- Harder to enforce consistent error handling / metrics
- Wave-C features take longer (no shared foundation)

---

## Recommendation Summary

| Scenario | Recommendation |
|----------|-----------------|
| **Goal: Fast Wave A/B/C rollout** | ✅ Build 2K-line `ml` module |
| **Goal: Minimal refactoring** | ❌ Keep distributed (accept tech debt) |
| **Goal: Long-term governance** | ✅ Build `ml` module + create `ml/governance.h` |

**Decision:** **Build it.** The 4-week investment in Phase 1 saves 8+ weeks across Wave B/C feature development.

---

## Next Steps (If Approved)

1. Create GitHub issue: "ML Module (Coordination Layer) Design & Scaffolding"
   - Size: 4 weeks
   - Milestone: v1.20.0
   - DependsOn: None (parallel to Wave A)
   - Acceptance: Interfaces shipped, 0 external dependencies

2. Update ROADMAP.md:
   - Add new `ml` entry
   - Cross-reference Wave A/B/C issues

3. Plan Wave A + ML Module integration:
   - DPR Vectorizer (A2) uses `ml/vectorizer.h`
   - Fairness Detector (A3) uses `ml/evaluator.h` as optional extension

---

