> **Hinweis:** Vage Einträge ohne messbares Ziel, Interface-Spezifikation oder Teststrategie mit `<!-- TODO: add measurable target, interface spec, test strategy -->` markieren.

<!-- Status: current | validated: 2026-05-11 -->
<!-- Links: README.md · ROADMAP.md · ../../include/ai/README.md -->

# AI Module - Future Enhancements

## Scope

- Produktivverdrahtung von `AIPluginGenerator::generatePlugin()` gegen konfigurierbare LLM-Endpunkte.
- Strukturierte Transformation von LLM-Antworten in `GeneratedPlugin` inklusive Manifest-/Dependency-Felder.
- Security-Hardening des Generierungspfads (Eingabe, Ausgabe, Logging, Fehlerpfade).
- Test- und Observability-Ausbau für reproduzierbare, diagnosefähige Ausführung.

## Design Constraints

- Der API-Vertrag in `include/ai/ai_plugin_generator.h` bleibt für v1.x abwärtskompatibel; neue Felder nur additive Erweiterungen.
- `generatePlugin()` darf keine unvalidierten Promptdaten ungefiltert in Logs schreiben; max. gekürzte, redigierte Payload-Anteile.
- Netzwerkzugriffe auf LLM-Endpunkte müssen bounded sein (Timeout + begrenzte Retry-Strategie), um Hänger zu vermeiden.
- `GeneratedPlugin` darf nur zurückgegeben werden, wenn alle Pflichtteile (`header_code`, `implementation_code`, `manifest`) vorhanden und konsistent sind.

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `AIPluginGenerator::validatePrompt(const PluginGenerationPrompt&)` | API-Aufrufer, `generatePlugin()` | Erweitern auf Feldkonsistenz (`required_capabilities`, `dependencies`) |
| `AIPluginGenerator::generatePlugin(const PluginGenerationPrompt&)` | CLI-/Service-Aufrufer | Live-Endpunktaufruf, Parse, Mapping auf `GeneratedPlugin` |
| `AIPluginGenerator::Config` | Initialisierung im Host | Endpunkt/Timeout/Output-Sandbox-Konfiguration (additiv) |
| `GeneratedPlugin` | Build-/Plugin-Pipeline | Muss konsistente Artefakte und Manifestdaten liefern |

## Implementation Notes

- Endpunktverdrahtung in `src/ai/ai_plugin_generator.cpp` mit klaren Fehlerklassen:
  - Validierungsfehler
  - Transport-/Timeoutfehler
  - Antwort-Parse-/Schemafehler
- Antwortschema für LLM-Ausgaben definieren und strikt validieren, bevor `GeneratedPlugin` befüllt wird.
- Sicherheitsgates vor Rückgabe:
  - Mindest-Validierung auf leere/inkonsistente Codeblöcke
  - Begrenzung maximaler Antwortgröße
- Logging auf Debug-Level nur mit gekürzten Inhalten und ohne potenziell sensitive Vollprompts.

## Test Strategy

- Unit: Erweiterung von APG-01..06 um Endpunkt-/Parse-/Fehlerklassifizierungsfälle.
- Integration: Kontrollierter Test-Endpunkt mit deterministischen Antworten (valid/invalid/timeout).
- Regression: Sicherstellen, dass bestehende Phase-1-Fehlerpfade weiterhin stabil strukturiert bleiben.
- Docs Validation: `docs-lint.py` + `link-check.py --internal-only` für betroffene Moduldocs.

## Performance Targets

- `generatePlugin()` P95-Latenz bei erfolgreichem Endpunktcall: ≤ 2.0 s für Standardprompt bis 4 KB.
- Validierungs-Overhead (`validatePrompt`) ≤ 2 ms für Prompts bis 8 KB.
- Fehlerpfad-Latenz bei Timeout: deterministisch durch konfiguriertes Timeout, keine unbegrenzte Blockierung.

## Security / Reliability

- Keine unredigierten sensitiven Prompt-/Response-Inhalte in persistenten Logs.
- Fail-closed bei Parse-/Schemafehlern: niemals teilweise befülltes `GeneratedPlugin` als Erfolg zurückgeben.
- Deterministische Fehlercodes/Fehlermeldungsklassen für Monitoring und Incident-Triage.
- Retry-Strategie begrenzen (max. Attempts + Backoff) und Endpunktfehler ohne Prozessabsturz zurückgeben.

## Research-Based ML Enhancements

### Analysis Summary (2026-05-11)

ThemisDB possesses a mature AI/ML stack spanning 58 modules with production-ready capabilities in LLM inference,
RAG evaluation, multi-modal processing, training & fine-tuning, and ethical governance. This section catalogs
cross-cutting ML enhancements validated against peer-reviewed literature, organised by deployment wave
and integration impact.

**Current Ecosystem Scope:**
- LLM Engine (v1.19.0): dual-engine architecture, speculative decoding, function calling, LoRA hot-loading
- RAG System (v2.0.0): multi-judge evaluation, knowledge gap detection, claim verification, online learning
- Training (v1.6.0): AdaLoRA adaptive rank allocation, LoRA+ asymmetric LR, TIES merging, hyperparameter search
- Prompt Engineering (v2.0.0): tree-of-thoughts, ProTeGi optimizer, DSPy layer, reflection tuning
- Ethics AI (v0.3.0): philosophical profiling, multi-round debates, configurable dimension weights
- Multi-Modal: CLIP embeddings (ONNX), Stable Diffusion v2.2.0, Whisper v2.1.0

### Wave A: Critical / Near-Term (Q3 2026 – Q4 2026)

#### A1: Speculative Decoding with Real Draft Model
- **Status:** [~] In Progress (baseline implementation exists; acceptance validation pending) | **Target:** 6 weeks | **Priority:** 🟠 High
- **Reference:** Chen et al. "Accelerating LLM Inference with Speculative Decoding" (arXiv:2211.17192)
- **Rationale:** Current `SpeculativeDecoder` uses constant-token placeholder (STUB #262). Real draft logits reduce draft-rejection rate from 85% → 30%, enabling 2-4× wall-clock speedup.
- **Affected Systems:**
  - `include/llm/speculative_decoder.h`: add `GenerateDraftTokensResult` struct
  - `src/llm/speculative_decoder.cpp`: integrate real draft-logit pipeline
  - `plugins/llama_cpp/llama_cpp_plugin.cpp`: hook `llama_get_logits()` after draft generation
- **Interface Changes:**
  - New method: `ILLMPlugin::generateDraftTokens(request, k, vocab_size_hint) → DraftTokensResult`
  - Config: `SpeculativeDecoder::Config::draft_model_id` (optional; fallback to single-model if unset)
- **Acceptance Criteria:**
  - Draft-rejection rate ≤ 40% on GPT-2 tokenizer (test corpus: 1k prompts)
  - E2E latency improvement ≥ 2.0× for 512-token generations on 7B model
  - Golden-output determinism vs. greedy decoding
- **Tests:** SD-REAL-01..08 (extend existing focused suite); benchmark in `benchmarks/bench_llm_inference.cpp`

#### A2: Dense Passage Retrieval (DPR) Vectorizer
- **Status:** [~] In Progress (DPR + HybridRetriever integration and comparative benchmark path implemented; external-dataset run pending) | **Target:** 4 weeks | **Priority:** 🟠 High
- **Reference:** Karpukhin et al. "Dense Passage Retrieval for Open-Domain QA" (ICLR 2021, arXiv:2004.04906)
- **Rationale:** Hybrid BM25+Vector retrieval in RAG currently uses generic embeddings. DPR-style bi-encoder (query encoder + passage encoder) improves MRR@10 by 15-25% over unsupervised embeddings.
- **Affected Systems:**
  - `include/rag/dpr_vectorizer.h` (new)
  - `src/rag/dpr_vectorizer.cpp` (new)
  - `include/rag/hybrid_retriever.h`: wire DPR as `IVectorizer` option ✅
- **Interface:**
  ```cpp
  class DPRVectorizer : public IVectorizer {
      struct Config { std::string query_model_path; std::string passage_model_path; };
      Result<std::vector<float>> encodeQuery(const std::string& query);
      Result<std::vector<float>> encodePassage(const RetrievedDocument& doc);
  };
  ```
- **Acceptance Criteria:**
  - MRR@10 improvement ≥ +15% vs. baseline on MS MARCO or Natural Questions subset
  - Passage encoding batch throughput ≥ 100 docs/sec (GPU, batch_size=32)
  - Query latency ≤ 150 ms (CLIP-based 384-dim embeddings)
- **Tests:** DPR-01..10 in `tests/rag/test_dpr_vectorizer.cpp`; benchmark vs. BM25-only baseline (`benchmarks/bench_rag_hybrid_retriever.cpp`)

#### A3: Fairness & Bias Detection in RAG
- **Status:** [~] In Progress (baseline implementation exists; full eval/perf closure pending) | **Target:** 4 weeks | **Priority:** 🟡 Medium
- **Reference:** Bolukbasi et al. "Man is to Computer Programmer as Woman is to Homemaker" (NeurIPS 2016, arXiv:1607.06520)
- **Rationale:** RAG can amplify corpus biases. Quantified bias scoring per retrieved document enables ethical audits and filtering.
- **Affected Systems:**
  - `include/rag/fairness_detector.h` (new)
  - `src/rag/fairness_detector.cpp` (new)
  - `include/rag/retrieved_document.h`: add `BiasScore` field
- **Implementation:**
  - Word-embedding bias projection (e.g., gender bias via PCA on occupation embeddings)
  - Stereotype density scoring (freq. of biased terms / total terms in passage)
  - Intersectional bias compound (gender × ethnicity heuristic)
- **Acceptance Criteria:**
  - Bias score correlation ≥ 0.70 with human annotator ratings (500-doc sample, 3 raters)
  - Computational overhead ≤ 5 ms per document
  - Configurable thresholds for retrieval filtering (e.g., exclude score > 0.6)
- **Tests:** FAIR-01..08; integration with `RAGJudge::evaluateRelevance()` pipeline

### Wave B: High-Value / Mid-Term (Q1 2027 – Q2 2027)

#### B1: Self-RAG (Self-Retrieving, Auto-Critique)
- **Status:** [ ] Not Started | **Target:** 8 weeks | **Priority:** 🟠 High
- **Reference:** Asai et al. "Self-RAG: Learning to Retrieve, Generate, and Critique" (ICLR 2024, arXiv:2310.11511)
- **Rationale:** Adaptive retrieval control + in-context critique loops reduce hallucinations by 20-30% without retraining. Orthogonal to existing `RAGJudge`.
- **Affected Systems:**
  - `include/rag/self_rag_engine.h` (new)
  - `src/rag/self_rag_engine.cpp` (new)
  - `src/llm/inference_engine_enhanced.cpp`: add critic callback hook
- **Core Components:**
  - **Retrieval Controller**: Binary classification ("Retrieve now?" / "Continue generation?") via small auxiliary head or LLM token
  - **Critic Model**: 3-class scoring (Relevant, Partially Relevant, Irrelevant) on generation segments
  - **Iterative Loop**: max 3 refinement rounds; early-exit on high-confidence segments
- **Interface:**
  ```cpp
  class SelfRAGEngine {
      Result<std::string> generateWithCritique(
          const std::string& query,
          const SelfRAGConfig& cfg
      );
      // Returns generation + retrieval trace + critique scores per segment
  };
  ```
- **Acceptance Criteria:**
  - End-to-end hallucination rate reduction ≥ 20% vs. standard RAG on ALCE benchmark
  - Latency increase ≤ 1.5× vs. baseline (due to critique overhead)
  - Precision@K retrieval ≥ 0.85 on golden-doc recall tests
- **Tests:** SELF_RAG-01..12; golden-output comparison; benchmark vs. vanilla RAG

#### B2: Knowledge Graph Completion via RotatE
- **Status:** [ ] Not Started | **Target:** 10 weeks | **Priority:** 🟠 High
- **Reference:** Sun et al. "RotatE: Knowledge Graph Embedding by Relation Rotation" (ICLR 2019, arXiv:1902.10197)
- **Rationale:** Multi-hop reasoning in `KnowledgeGraphReasoner` can infer missing links. RotatE embeddings (relation-as-rotation) enable link prediction; MRR improvements +25% on FB15k-237.
- **Affected Systems:**
  - `include/graph/knowledge_graph_completion.h` (new)
  - `src/graph/knowledge_graph_completion.cpp` (new)
  - `src/graph/knowledge_graph_reasoner.cpp`: wire link-prediction head
- **Training Pipeline:**
  - Triple loss: triplet margin ranking (positive vs. negative sampling)
  - Embedding dimension: 128-256 (configurable)
  - Optimization: Adagrad with learning-rate scheduling
- **Inference:**
  - Predict missing tail given (head, relation): rank all entities by tail-embedding distance
  - Soft link prediction (confidence score) vs. hard (top-k)
- **Acceptance Criteria:**
  - MRR ≥ 0.35, Hits@10 ≥ 0.55 on FB15k-237 test set
  - Inference latency ≤ 50 ms for top-20 predictions (batch_size=100)
  - Integration with existing `KnowledgeGraphReasoner` without breaking backward compatibility
- **Tests:** KGC-01..15; benchmark vs. TransE baseline

#### B3: Multi-Task LoRA Fine-Tuning
- **Status:** [ ] Not Started | **Target:** 6 weeks | **Priority:** 🟡 Medium
- **Reference:** Ruder et al. "An Overview of Multi-Task Learning" (2015, arXiv:1505.00387)
- **Rationale:** Single LoRA adapter across multiple domains → task interference. Soft parameter sharing + domain-gating improves generalization when per-domain training data is sparse.
- **Affected Systems:**
  - `include/training/multi_task_lora_trainer.h` (new)
  - `src/training/multi_task_lora_trainer.cpp` (new)
  - `src/training/incremental_lora_trainer.cpp`: refactor to support task-aware weighting
- **Architecture:**
  - Shared LoRA base (A, B matrices)
  - Task-specific projection layers (small 64 → 64 linear heads per task)
  - Joint loss: Σ_task (weight_t × loss_t)
- **Acceptance Criteria:**
  - Average task performance on 3-task benchmark ≥ +8% vs. single-task baseline
  - Training time increase ≤ 15% (shared computation mitigates overhead)
  - Per-task hyperparameter robustness (no catastrophic negative transfer)
- **Tests:** MTL-01..10; ablation study (shared base vs. separate adapters)

### Wave C: Medium-Term / Strategic (Q3 2027+)

#### C1: Constitutional AI (CAI) Safety Module
- **Status:** [ ] Not Started | **Target:** 8 weeks | **Priority:** 🟡 Medium
- **Reference:** Bai et al. "Constitutional AI: Harmlessness from AI Feedback" (2022, arXiv:2212.08073)
- **Rationale:** Replace RLHF with LLM-as-critic. Constitutional principles (explicit rules) + critique-revision loops achieve comparable safety without human feedback loops.
- **Affected Systems:**
  - `include/ethics_ai/constitutional_ai.h` (new)
  - `src/ethics_ai/constitutional_ai.cpp` (new)
  - `src/ethics_ai/ethics_evaluator.cpp`: integrate CAI scores into EthicsEvaluator
- **Constitutional Principles Registry:**
  - 20+ built-in principles (e.g., "Never provide instructions for violence")
  - Custom principle injection via JSON config
  - Per-principle weight (importance scoring)
- **Critic-Revision Loop:**
  1. LLM generates response
  2. Critic scores against all principles (violation binary classifier)
  3. If violations detected: revision prompt with principle explanation
  4. Max 2 revision rounds; return best-scoring version
- **Acceptance Criteria:**
  - Safety score alignment ≥ 0.80 with human annotators (500-sample benchmark)
  - Latency overhead ≤ 2.0 s per response (1 critique + 1 revision cycle)
  - False-positive rate ≤ 10% (benign content flagged as unsafe)
- **Tests:** CAI-01..12; integration with `EthicsEvaluator`

#### C2: Federated Learning for Privacy-Preserving Training
- **Status:** [ ] Not Started | **Target:** 12 weeks | **Priority:** 🟢 Low
- **Reference:** Kairouz et al. "Advances and Open Problems in Federated Learning" (2021, arXiv:2104.14881)
- **Rationale:** Multi-node training without raw data centralization. Enables enterprise deployments under strict data-privacy regimes (GDPR, HIPAA).
- **Affected Systems:**
  - `include/training/federated_trainer.h` (new)
  - `src/training/federated_trainer.cpp` (new)
  - `src/replication/consensus_engine.cpp`: wire for gradient aggregation
- **Implementation:**
  - Synchronized SGD (all-reduce on parameter deltas, not raw data)
  - Secure aggregation primitive (optional: homomorphic encryption stub)
  - Byzantine-robust averaging (median/trimmed mean for robustness)
- **Acceptance Criteria:**
  - Training convergence ≥ 95% of centralized baseline (10 nodes, 10% data per node)
  - Gradient communication overhead ≤ 2.0 s per round (100MB gradients over 1 Gbps network)
  - Configurable privacy budget (ε-differential privacy tuning)

### Cross-Cutting Implementation Patterns

#### Pattern 1: Research-Plugin Lifecycle
```cpp
// 1. Interface abstraction (pure virtual)
class IResearchFeature {
    virtual Status initialize(const Config&) = 0;
    virtual Result<Output> process(const Input&) = 0;
    virtual Metrics getMetrics() = 0;
};

// 2. Plugin adapter wrapping research implementation
class ResearchFeaturePlugin : public IThemisPlugin {
    std::shared_ptr<IResearchFeature> impl_;
};

// 3. LLM bridge for critique/evaluation loops
ILLMProvider* llm_;  // Injected at construction

// 4. Metrics & observability
PrometheusRegistry metrics_;
```

#### Pattern 2: Research-Code Stubs
Research implementations often require phased rollout. Mark temporary placeholders explicitly:

```cpp
// RESEARCH STUB (Wave A1 — Speculative Decoding)
// Purpose: Placeholder for real draft-logits generation (Chen et al., 2023)
// Activation: #ifdef THEMIS_ENABLE_SPEC_DECODING_REAL_LOGITS
// Production Delta: Current impl uses constant-token; real impl hooks llama_get_logits()
// Removal Plan: v1.20.0 (Q4 2026) — will become default; stub mode archived
//
// TODO: Replace with real LLM draft-token generation; target: P95 latency ≤ 150 ms
Result<std::vector<int>> generateDraftTokens(const InferenceRequest& req) {
    #ifdef THEMIS_ENABLE_SPEC_DECODING_REAL_LOGITS
        return generateRealDraftTokens(req);  // Real impl
    #else
        return generateConstantTokenPlaceholder(req);  // Stub fallback
    #endif
}
```

### Evaluation & Acceptance Framework

**Automated Benchmarking:**
- Each Wave-A item integrated into CI/CD as optional benchmark step (flag: `THEMIS_ENABLE_ML_RESEARCH_FEATURES`)
- Results published to central metrics dashboard (Prometheus + Grafana)
- Golden-output regression tests prevent performance degradation

**Human Review Gates:**
- Research papers + implementation linked in PR description
- At least 2 expert sign-offs (ML + Systems) before merge
- Community feedback window: 2 weeks post-draft PR

**Success Metrics (Meta-Level):**
- Wave A completion rate: ≥ 80% by end Q4 2026
- Wave B adoption: ≥ 3 production deployments actively using enhanced features by Q2 2027
- Publication of joint research paper (ThemisDB + university partner) highlighting integration lessons

## See Also

- Current Implementation: [`README.md`](./README.md)
- Roadmap: [`ROADMAP.md`](./ROADMAP.md)
- Public API: [`../../include/ai/README.md`](../../include/ai/README.md)
- Plugins Roadmap Context: [`../plugins/ROADMAP.md`](../plugins/ROADMAP.md)
- Research Bibliography: `docs/research/ml_enhancements_bibliography.md` (to be created)
