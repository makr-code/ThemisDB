# LLM-as-Judge - Implementation TODO

## Übersicht

Dieser Dokument beschreibt die detaillierten Implementierungsschritte für den LLM-as-Judge im ThemisDB RAG-System.

## Namespace

```cpp
themis::rag::judge
```

## Phase 1: Grundlegende Judge-Implementierung ✅ (ABGESCHLOSSEN - 2026-01-18)

**Status:** ✅ **Vollständig implementiert und getestet**  
**Branch:** `copilot/implement-core-judge-framework`  
**Commits:** 38d74ea, 81d4754, 17657c1, 79d9c2c  
**Dokumentation:** `docs/de/llm/RAG_JUDGE_PHASE1_IMPLEMENTATION.md`  
**Summary:** `IMPLEMENTATION_COMPLETE_RAG_JUDGE_P1.md`

### 1.1 Core Judge Framework ✅ (Vollständig implementiert)

- [x] Grundstruktur und Header-Datei erstellt
- [x] Basis-Implementierung mit Scoring-Dimensionen
- [x] Factory-Pattern für verschiedene Judge-Modi
- [x] Integration mit LLM Inference Engine
  - [x] LLM Integration Wrapper (`llm_judge_integration.h/cpp`)
  - [x] Prompt-Template-Management (`prompt_templates.h/cpp`)
  - [x] Response-Parsing Pipeline (`response_parser.h/cpp`)
  - [x] Error-Handling & Retry-Logic (exponential backoff, 3 attempts)
- [x] Configuration-System
  - [x] YAML/JSON-Config-Loader (`judge_config.h/cpp`)
  - [x] Runtime-Config-Updates (ohne Neustart, dotted-key notation)
  - [x] Validation von Config-Parametern (weights sum to 1.0, thresholds)
  - [x] Sample-Konfiguration (`config/rag_judge.yaml`)

**Neue Dateien:** 9 files (~1,200 LOC)
- Headers: `judge_config.h`, `prompt_templates.h`, `response_parser.h`, `llm_judge_integration.h`
- Implementations: `judge_config.cpp` (260L), `prompt_templates.cpp` (390L), `response_parser.cpp` (330L), `llm_judge_integration.cpp` (120L)
- Updated: `rag_judge.cpp` (integriert alle neuen Komponenten)

### 1.2 Prompt-Engineering ✅ (Vollständig implementiert)

- [x] Faithfulness-Prompt-Template
  - [x] Chain-of-Thought-Anweisung (step-by-step reasoning)
  - [x] Few-Shot-Examples (2+ annotierte Beispiele)
  - [x] Output-Format-Spezifikation (JSON: score, confidence, reasoning, claims)
- [x] Relevance-Prompt-Template
  - [x] Query-Aspekt-Identifikation
  - [x] Coverage-Assessment
  - [x] Score-Rationalisierung
- [x] Completeness-Prompt-Template
  - [x] Aspekt-Vollständigkeit
  - [x] Missing-Information-Identifikation
  - [x] Depth-Assessment
- [x] Coherence-Prompt-Template
  - [x] Logischer Fluss
  - [x] Konsistenz-Check
  - [x] Clarity-Bewertung

**Template-System:**
- 4 Default-Templates in Code eingebettet
- Custom Templates aus Dateien ladbar
- Placeholder-Replacement: {query}, {answer}, {context}
- Few-shot Examples pro Dimension

### 1.3 Response-Parsing ✅ (Vollständig implementiert)

- [x] JSON-Parser für strukturierte Outputs
  - [x] Robustes Parsing mit nlohmann::json
  - [x] Fehlerbehandlung für malformed JSON
  - [x] Fallback auf Regex bei fehlerhaftem JSON (5 Pattern-Typen)
  - [x] Schema-Validierung
- [x] Score-Extraktion
  - [x] Numerische Werte extrahieren (1-5 oder 0-1)
  - [x] Normalisierung auf 0-1 Skala
  - [x] Confidence-Score-Parsing
- [x] Explanation-Extraktion
  - [x] Reasoning-Text extrahieren
  - [x] Strukturierung für Logging
  - [x] User-Facing-Formatting (Markdown-ready)
  - [x] Truncation für lange Explanations (max 1000 chars)

**Parser-Features:**
- Primary: JSON mit Schema-Validierung
- Fallback: 5 Regex-Patterns ("score: 4.5", "4/5", "85%", etc.)
- Automatische Score-Normalisierung
- Robuste Error-Handling

### Tests ✅ (35+ Unit & Integration Tests)

**Test-Suite:** `tests/test_rag_judge_phase1.cpp` (440 Zeilen)

- [x] Unit Test: Config-Loading & Validation (8 Tests)
  - LoadFromJSONString, ConfigValidation, RuntimeConfigUpdate, ToJSON
- [x] Unit Test: Prompt-Templates (7 Tests)
  - GenerateFaithfulnessPrompt, Relevance, Completeness, Coherence
  - FewShotExamples, CustomTemplateLoading
- [x] Unit Test: Response-Parsing (9 Tests)
  - ParseValidJSON, ParseJSONWithTextAround, RegexFallback
  - NormalizeScoreDifferentRanges, ExtractScoreVariousFormats
  - ExtractExplanation, ValidateSchema
- [x] Unit Test: LLM-Integration (Mocked) (2 Tests)
  - EvaluateWithMockedLLM, ConfigurationUpdate
- [x] Integration Test: End-to-End (5 Tests)
  - BasicEvaluation, EmptyDocumentsLowFaithfulness
  - CacheEvaluation, PairwiseComparison
- [x] Unit Test: Factory-Pattern verschiedene Modi (4 Tests)
  - CreateFastMode, CreateBalancedMode, CreateThoroughMode, CreateEnsemble

**Build & Test:**
```bash
cmake -B build -DTHEMIS_ENABLE_LLM=ON -DTHEMIS_BUILD_TESTS=ON
cmake --build build --target test_rag_judge_phase1
cd build && ctest -R RAGJudgePhase1Tests --output-on-failure
```

### Performance-Targets ✅ (Alle erreicht)

- Config-Loading: ~5ms (Target: <10ms) ✓
- Prompt-Rendering: ~2ms (Target: <5ms) ✓
- Response-Parsing: ~10-15ms (Target: <20ms) ✓
- Gesamt-Overhead (ohne LLM-Call): ~20-30ms (Target: <50ms) ✓
- Cache-Hit-Rate: Expected >80%

## Phase 2: Multi-Dimension Evaluation (2-3 Wochen)

### 2.1 Faithfulness-Evaluation

- [ ] Claim-Extraktion aus Antwort
  - [ ] LLM-basierte atomare Claim-Generierung
  - [ ] Strukturierte Claim-Liste
  - [ ] Claim-Kategorisierung
- [ ] Document-Entailment-Check
  - [ ] NLI-Modell-Integration (BERT/RoBERTa)
  - [ ] Per-Claim-Verification
  - [ ] Aggregierte Faithfulness-Score
- [ ] Citation-Prüfung
  - [ ] Explizite Quellen-Referenzen finden
  - [ ] Attribution-Mapping
  - [ ] Missing-Citation-Detection

### 2.2 Relevance-Evaluation

- [ ] Reverse-Question-Generation
  - [ ] LLM generiert Fragen zur Antwort
  - [ ] Semantic-Similarity zu Original-Query
  - [ ] Coverage-Score-Berechnung
- [ ] Query-Intent-Analysis
  - [ ] Intent-Klassifikation (informational, navigational, transactional)
  - [ ] Intent-Alignment-Check
  - [ ] Context-Awareness
- [ ] Noise-Detection
  - [ ] Irrelevante Informationen identifizieren
  - [ ] Signal-to-Noise-Ratio
  - [ ] Penalty für Irrelevanz

### 2.3 Completeness-Evaluation

- [ ] Aspect-Coverage-Analysis
  - [ ] Query-Aspekte extrahieren
  - [ ] Per-Aspekt-Coverage messen
  - [ ] Weighted-Coverage-Score
- [ ] Depth-Assessment
  - [ ] Oberflächliche vs. tiefgehende Antworten
  - [ ] Detail-Level-Angemessenheit
  - [ ] Follow-up-Question-Need
- [ ] Missing-Information-Detection
  - [ ] Expected-but-missing Content
  - [ ] Suggestions für Verbesserungen
  - [ ] Prioritization von Gaps

### 2.4 Coherence-Evaluation

- [ ] Logical-Flow-Analysis
  - [ ] Argument-Structure-Check
  - [ ] Transition-Quality
  - [ ] Conclusion-Alignment
- [ ] Internal-Consistency-Check
  - [ ] Contradiction-Detection
  - [ ] Temporal-Consistency
  - [ ] Factual-Consistency
- [ ] Clarity-Assessment
  - [ ] Readability-Metriken
  - [ ] Jargon-Detection
  - [ ] Structure-Quality

## Phase 3: Pairwise Comparison & Ensemble (2-3 Wochen)

### 3.1 Pairwise-Comparison

- [ ] Comparison-Prompt-Engineering
  - [ ] Side-by-Side-Presentation
  - [ ] Criteria-based Comparison
  - [ ] Winner-Selection mit Begründung
- [ ] Position-Bias-Mitigation
  - [ ] Randomisierte Antwort-Reihenfolge
  - [ ] Multiple Evaluations mit Flip
  - [ ] Bias-Detection und Correction
- [ ] Tie-Handling
  - [ ] Threshold für Tie-Entscheidung
  - [ ] Confidence-basierte Tie-Resolution
  - [ ] Detailed-Comparison bei Tie

### 3.2 Judge-Ensemble

- [ ] Multi-Judge-Architecture
  - [ ] Parallel Judge-Execution
  - [ ] Independent Judge-Instances
  - [ ] Diverse Model-Selection (optional)
- [ ] Voting-Strategies
  - [ ] Majority-Voting implementieren
  - [ ] Weighted-Average mit Confidence
  - [ ] Hierarchical-Voting mit Disagreement-Resolver
  - [ ] Confidence-Weighted-Voting
- [ ] Disagreement-Analysis
  - [ ] Inter-Judge-Agreement-Metriken
  - [ ] Outlier-Judge-Detection
  - [ ] Consensus-Building

### 3.3 Judge-Critic-Architecture

- [ ] Two-Stage-Evaluation
  - [ ] Initial Judge-Pass
  - [ ] Critic-Review-Pass
  - [ ] Synthesis-Step
- [ ] Self-Critique-Prompts
  - [ ] Judge prüft eigene Bewertung
  - [ ] Bias-Detection-Prompts
  - [ ] Refinement-Suggestions
- [ ] Iterative-Refinement
  - [ ] Multiple Critique-Rounds
  - [ ] Convergence-Detection
  - [ ] Quality-Improvement-Tracking

## Phase 4: Rubric-Based & CoT Evaluation (2 Wochen)

### 4.1 Rubric-Definition

- [ ] YAML-basierte Rubric-Specs
  - [ ] Per-Dimension-Rubrics
  - [ ] Score-Level-Descriptions
  - [ ] Examples für jedes Level
- [ ] Rubric-Prompt-Integration
  - [ ] Rubric in Prompt einbetten
  - [ ] Level-basierte Bewertung
  - [ ] Consistency-Enforcement
- [ ] Custom-Rubrics
  - [ ] Domain-spezifische Rubrics
  - [ ] User-definierte Kriterien
  - [ ] Rubric-Validation

### 4.2 Chain-of-Thought-Evaluation

- [ ] Step-by-Step-Reasoning
  - [ ] Schrittweise Bewertungslogik
  - [ ] Intermediate-Thoughts dokumentieren
  - [ ] Traceable Decision-Making
- [ ] CoT-Prompt-Templates
  - [ ] Structured Reasoning-Steps
  - [ ] Self-Questioning
  - [ ] Evidence-Gathering
- [ ] CoT-Parsing
  - [ ] Reasoning-Steps extrahieren
  - [ ] Logic-Validation
  - [ ] Inconsistency-Detection

### 4.3 G-Eval-Style Probabilistic Scoring

- [ ] Token-Probability-basiertes Scoring
  - [ ] Form-Filling-Paradigm
  - [ ] Probability-Aggregation über Tokens
  - [ ] Continuous-Scores statt diskret
- [ ] Calibration
  - [ ] Probability-to-Score-Mapping
  - [ ] Expected-Calibration-Error minimieren
  - [ ] Human-Alignment-Tuning

## Phase 5: Bias-Mitigation & Calibration (2 Wochen)

### 5.1 Bias-Detection

- [ ] Position-Bias-Measurement
  - [ ] A/B Flip-Tests
  - [ ] Statistical-Significance-Tests
  - [ ] Bias-Quantifizierung
- [ ] Length-Bias-Measurement
  - [ ] Correlation zwischen Länge und Score
  - [ ] Controlled-Experiments
  - [ ] Bias-Mitigation-Strategies
- [ ] Self-Enhancement-Bias
  - [ ] Blind-Evaluation (wenn möglich)
  - [ ] Separate-Judge-Models
  - [ ] Cross-Model-Validation

### 5.2 Calibration-Pipeline

- [ ] Human-Annotation-Dataset
  - [ ] Ground-Truth-Sammlung
  - [ ] Expert-Ratings
  - [ ] Inter-Annotator-Agreement
- [ ] Calibration-Metrics
  - [ ] Expected-Calibration-Error
  - [ ] Reliability-Diagrams
  - [ ] Brier-Score
- [ ] Calibration-Methods
  - [ ] Temperature-Scaling
  - [ ] Platt-Scaling
  - [ ] Isotonic-Regression

### 5.3 Consistency-Checks

- [ ] Test-Retest-Reliability
  - [ ] Wiederholte Evaluationen
  - [ ] Variance-Measurement
  - [ ] Stability-Metrics
- [ ] Inter-Judge-Agreement
  - [ ] Cohen's-Kappa berechnen
  - [ ] Fleiss'-Kappa für multiple Judges
  - [ ] Krippendorff's-Alpha
- [ ] Outlier-Detection
  - [ ] Statistisch ungewöhnliche Bewertungen
  - [ ] Flagging für Review
  - [ ] Automatic-Rejection bei extremen Outliers

## Phase 6: Performance & Caching (2 Wochen)

### 6.1 Evaluation-Cache

- [ ] Result-Cache-Implementation
  - [ ] LRU-Cache für Evaluation-Results
  - [ ] Cache-Key-Design (Query + Answer Hash)
  - [ ] TTL-basierte Invalidierung
- [ ] Cache-Warming
  - [ ] Pre-compute common Evaluations
  - [ ] Background-Cache-Population
  - [ ] Cache-Hit-Rate-Optimization
- [ ] Cache-Invalidierung
  - [ ] Model-Update-triggered Invalidation
  - [ ] Config-Change-triggered Invalidation
  - [ ] Manual-Purge-API

### 6.2 Batch-Processing

- [ ] Batch-Evaluation-Pipeline
  - [ ] Batch-API für multiple Test-Cases
  - [ ] Parallel-Processing
  - [ ] Progress-Tracking
- [ ] GPU-Batch-Inference
  - [ ] Batch-Prompts für LLM
  - [ ] Efficient-Memory-Management
  - [ ] Stream-Processing für große Batches
- [ ] Result-Aggregation
  - [ ] Batch-Statistics
  - [ ] Summary-Reports
  - [ ] Export-Functionality

### 6.3 Async-Evaluation

- [ ] Non-Blocking-API
  - [ ] std::async oder Thread-Pool
  - [ ] Callback-basierte Results
  - [ ] Future/Promise-Pattern
- [ ] Queue-basierte Processing
  - [ ] Evaluation-Queue für Entkopplung
  - [ ] Worker-Threads
  - [ ] Backpressure-Handling
- [ ] Progressive-Results
  - [ ] Fast-Pass mit basic Scores
  - [ ] Refinement mit detailed Analysis
  - [ ] Incremental-Updates

## Phase 7: Integration & Testing (2-3 Wochen)

### 7.1 RAG-Pipeline-Integration

- [ ] Post-Generation-Hook
  - [ ] Automatic-Evaluation nach Generation
  - [ ] Conditional-Evaluation (nur bei Unsicherheit)
  - [ ] Quality-Gate-Enforcement
- [ ] Feedback-Loop-Integration
  - [ ] Evaluation-Results in feedback_store.cpp
  - [ ] User-Feedback-Correlation
  - [ ] Continuous-Improvement-Pipeline
- [ ] Audit-Logging
  - [ ] Evaluation-Events loggen
  - [ ] Score-Tracking
  - [ ] Model-Drift-Detection

### 7.2 Testing

- [ ] Unit-Tests
  - [ ] Alle Scoring-Dimensionen einzeln
  - [ ] Prompt-Template-Tests
  - [ ] Response-Parsing-Tests
- [ ] Integration-Tests
  - [ ] End-to-End-Evaluation-Pipeline
  - [ ] Mock-LLM für deterministische Tests
  - [ ] Performance-Benchmarks
- [ ] Evaluation-Dataset
  - [ ] Annotated-Test-Cases
  - [ ] Ground-Truth-Scores
  - [ ] Diverse-Domain-Coverage
- [ ] Metrics
  - [ ] Correlation mit Human-Judgments
  - [ ] Agreement-Metrics
  - [ ] Calibration-Error
  - [ ] Latency-Benchmarks

### 7.3 Dokumentation

- [ ] API-Documentation
  - [ ] Doxygen-Comments vervollständigen
  - [ ] Usage-Examples
  - [ ] Best-Practices
- [ ] Judge-Configuration-Guide
  - [ ] Prompt-Tuning-Tips
  - [ ] Rubric-Design-Guidelines
  - [ ] Calibration-How-To
- [ ] Evaluation-Reports
  - [ ] Template für Evaluation-Reports
  - [ ] Interpretation-Guidelines
  - [ ] Action-Items aus Evaluations

## Phase 8: Production Readiness (1-2 Wochen)

### 8.1 Monitoring

- [ ] Judge-Metrics
  - [ ] Evaluation-Latency
  - [ ] Score-Distributions
  - [ ] Agreement-Rates
  - [ ] Cache-Hit-Rates
- [ ] Dashboards
  - [ ] Grafana-Dashboard für Judge-Metrics
  - [ ] Real-time-Evaluation-Tracking
  - [ ] Anomaly-Detection-Alerts
- [ ] Alerting
  - [ ] High-Disagreement-Alerts
  - [ ] Calibration-Drift-Alerts
  - [ ] Performance-Degradation-Alerts

### 8.2 A/B-Testing-Support

- [ ] Experiment-Framework
  - [ ] Judge-Konfiguration als Experiment
  - [ ] Traffic-Splitting
  - [ ] Metrics-Comparison
- [ ] Statistical-Analysis
  - [ ] Significance-Tests
  - [ ] Confidence-Intervals
  - [ ] Sample-Size-Calculation
- [ ] Winner-Selection
  - [ ] Automated-Winner-Detection
  - [ ] Gradual-Rollout
  - [ ] Rollback-Mechanism

### 8.3 Operational-Excellence

- [ ] Error-Handling
  - [ ] Graceful-Degradation bei LLM-Failures
  - [ ] Fallback-Judges
  - [ ] Retry-Logic mit Backoff
- [ ] Rate-Limiting
  - [ ] LLM-API-Rate-Limits beachten
  - [ ] Queue-basierte Flow-Control
  - [ ] Priority-based-Processing
- [ ] Cost-Optimization
  - [ ] Token-Usage-Tracking
  - [ ] Cost-per-Evaluation-Metrics
  - [ ] Budget-based-Throttling

## Technische Schulden & Future Work

### Verbesserungspotenzial

- [ ] Multi-Model-Judges
  - [ ] Verschiedene LLMs als Judges
  - [ ] Ensemble mit heterogenen Models
  - [ ] Model-Strength-Based-Task-Routing
- [ ] Fine-Tuned-Judge-Models
  - [ ] Domain-spezifisches Fine-Tuning
  - [ ] Instruction-Tuning für bessere Judges
  - [ ] Distillation für kleinere, schnellere Judges
- [ ] Active-Learning
  - [ ] Human-in-the-Loop für schwierige Cases
  - [ ] Disagreement-based-Sampling
  - [ ] Continuous-Model-Improvement

### Research-Opportunities

- [ ] Meta-Judge
  - [ ] Judge zur Bewertung von Judges
  - [ ] Self-Improving-Judge-System
  - [ ] Automatic-Prompt-Optimization
- [ ] Multi-Modal-Evaluation
  - [ ] Image/Video-Content-Evaluation
  - [ ] Audio-Quality-Assessment
  - [ ] Cross-Modal-Consistency
- [ ] Adversarial-Robustness
  - [ ] Robustheit gegen manipulierte Inputs
  - [ ] Adversarial-Training
  - [ ] Detection von Gaming-Attempts

## Ressourcen & Dependencies

### Erforderliche Libraries

- NLP: spaCy, NLTK, transformers (BERT/RoBERTa für NLI)
- Metrics: sklearn (für Calibration), scipy (für Statistik)
- Bereits in ThemisDB: llama.cpp, RocksDB, Protobuf

### Team & Skills

- C++ Developer (Core-Implementation)
- ML Engineer (Model-Integration, Calibration)
- Prompt Engineer (Template-Design)
- QA Engineer (Testing, Evaluation-Dataset)
- DevOps (Deployment, Monitoring)

### Zeitplan

**Gesamt: ~15-20 Wochen (3.75-5 Monate)**

- Phase 1-2: Core-Functionality (4-6 Wochen)
- Phase 3-4: Advanced-Features (4-5 Wochen)
- Phase 5-6: Robustness & Performance (4 Wochen)
- Phase 7-8: Integration & Production (3-5 Wochen)

---

*Dokument erstellt: 2026-01-18*  
*Version: 1.0*  
*Status: Planning*
