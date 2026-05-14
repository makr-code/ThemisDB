# ML/AI Impact Assessment & Governance — ThemisDB Source Code Analysis

**Status:** v3 — evidence-based, sourced from `src/` analysis (2026-05-13)
**Scope:** All productive ML/AI touchpoints found in `src/` and `include/`

This document is the result of a systematic source code analysis. Every finding
references the concrete file and mechanism found during code review. It is the
canonical governance reference for ML/AI impact within ThemisDB and is kept
synchronized with the current system state on every governance review cycle.

## 0) Abgrenzung zu verwandten Dokumenten

This document is **the governance-owned impact assessment** for ML/AI components.
It focuses on operational risk, control gaps, escalation paths, and measurable KPIs
from a governance and compliance perspective.

| Dokument | Inhalt | Abgrenzung |
|---|---|---|
| **dieses Dokument** | Touchpoint-Inventar, Risikokatalog, Kontrollen, KPIs, Eskalationspfade (Governance-Sicht) | Bindendes Artefakt für Governance-Reviews und Audits |
| `src/ethics_ai/` / `plugins/ethics_ai/` | Laufzeit-Implementierung von Ethikbewertung (EthicsEvaluator, DiscourseEngine) | Sourcecode, kein Policy-Dokument |
| `examples/24_moral_philosophy_debates/ETHICS_AI_BEST_PRACTICES.md` | Fachliche Referenz zu KI-Ethik-Best-Practices | Orientierungsdokument ohne Governance-Bindung |
| `docs/llm_orchestration/RAG_ETHICS_IMPLEMENTATION.md` | Implementierungsdetails RAG+Ethics-Integration | Technische Implementierungsnotiz |
| `src/governance/FUTURE_ENHANCEMENTS.md` | Geplante Governance-Erweiterungen | Planungsartefakt, kein aktueller Zustand |
| `src/governance/ROADMAP.md` | Meilenstein-Roadmap | Planungsartefakt |

**Regelung bei Konflikten:** Dieses Dokument (AI_ML_IMPACT_ASSESSMENT.md) hat Vorrang
vor allen oben genannten Planungs- und Referenzdokumenten für Aussagen zum
aktuellen Governance-Status von ML/AI-Touchpoints.

## 1) Scope und Analysemethode

**In Scope:** All `src/` and `include/` C++ source files containing ML/AI logic.
**Not in Scope:** CI/CD tooling, deployment scripts, external service config,
general AI ethics reference literature (see Section 0 for demarcation).

**Leitfragen** (aus Meta-Issue):

- [x] Wo genau greift ML/AI in Core-Funktionen ein? (Abschnitt 2)
- [x] Welche Entscheidungen sind deterministisch vs. probabilistisch? (Abschnitt 3)
- [x] Welche Fehlerbilder sind moeglich? (Abschnitt 4)
- [x] Welche Hard-/Soft-Fallbacks existieren pro kritischem Pfad? (Abschnitt 5)
- [x] Welche Kontrollen verhindern, dass ML/AI Betriebs-/Sicherheitsziele verletzt? (Abschnitt 6)
- [x] Welche Eskalationspfade existieren bei Kontrollversagen? (Abschnitt 7, Spalte Eskalationspfad)

## 2) Inventar: ML/AI-Touchpoints in src/

### 2.1 LLM-Kern-Infrastruktur

| Datei | Klasse / Funktion | Funktion | Entscheidungstyp |
|---|---|---|---|
| `src/llm/inference_engine_enhanced.cpp` | `InferenceEngineEnhanced` | Kern-Inferenz: Generierung, Routing, Batching | probabilistisch |
| `src/llm/async_inference_engine.cpp` | `AsyncInferenceEngine` | Asynchrone Prompt-Queue mit PromptPolicy-Gate | probabilistisch + deterministischer Policy-Block |
| `src/llm/llamacpp_inference_engine.cpp` | `LlamaCppInferenceEngine` | llama.cpp C-API-Bindings fuer lokale Inferenz | probabilistisch |
| `src/llm/embedded_llm.cpp` | `EmbeddedLLM` | Eingebettete LLM-Schnittstelle fuer interne Pfade | probabilistisch |
| `src/llm/model_router.cpp` | `ModelRouter` | Regelbasiertes Routing (prompt_patterns, metadata_tags) auf Modell-ID | deterministisch |
| `src/llm/continuous_batch_scheduler.cpp` | `ContinuousBatchScheduler` | Token-Budget-gesteuertes Batching (`max_tokens_per_batch`) | deterministisch |
| `src/llm/kv_cache_buffer.cpp` | `KVCacheBuffer` | KV-Cache-Verwaltung mit OOM-Schutz | deterministisch |
| `src/llm/ai_orchestrator.cpp` | `AIOrchestrator` | Multi-Mode-Dispatcher (Ask, RAG, Agentic, Ethics, MultiAgent) | deterministisch (Mode-Dispatch) + probabilistisch (Tool-Execution) |
| `src/llm/constitutional_reasoning_engine.cpp` | `ConstitutionalReasoningEngine` | Iterative Selbstkorrektur + Principle-Checks (Autonomy, Transparency, Non-Harmfulness, Fairness) | probabilistisch mit regelbasierter Violation-Detection |
| `src/llm/ethics_aware_confidence_detector.cpp` | `EthicsAwareConfidenceDetector` | Ethik-bewusster Konfidenz-Score | probabilistisch |
| `src/llm/inline_training_engine.cpp` | `InlineTrainingEngine` | On-the-fly LoRA-Fine-tuning inkl. Checkpoint-Resume | probabilistisch (Training) + deterministisch (Stop-Signal) |
| `src/llama_cpp/llama_cpp_plugin.cpp` | `LlamaCppPlugin` | generate(): Delegation an `LlamaWrapper`; STUB-Fallback (echo-Text) wenn kein Modell geladen. embed(): LlamaWrapper oder Zero-Vector-Stub | probabilistisch; Stub-Pfad deterministisch |

### 2.2 RAG-Pipeline

| Datei | Klasse / Funktion | Funktion | Entscheidungstyp |
|---|---|---|---|
| `src/rag/agentic_rag.cpp` | `AgenticRAG` | Iterativer Retrieval-Reason-Act-Loop (max_iterations=5 default) | probabilistisch mit deterministischem Loop-Limit |
| `src/rag/multi_step_rag.cpp` | `MultiStepRAGOrchestrator` | MapReduce + iterativer RAG (runMapReduce, runIterative) | probabilistisch |
| `src/rag/rag_context_assembler.cpp` | `RAGContextAssembler` | Token-Budget-gesteuerte Context-Assembly (`ContextWindowBudget`) | deterministisch (Budget) + probabilistisch (Relevanz-Ranking) |
| `src/rag/hybrid_retriever.cpp` | `HybridRetriever` | BM25 + Vector-Fusion Retrieval | gemischt |
| `src/rag/reranker.cpp` | `Reranker` | Kreuz-Encoder-Reranking | probabilistisch |
| `src/rag/prompt_injection_detector.cpp` | `PromptInjectionDetector` / `PromptInjectionSanitizer` | Musterbasierende Injection-Detection + Sanitization (role_injection, markup_injection, instruction_injection, delimiter_injection) | deterministisch (Muster) |
| `src/rag/nli_faithfulness_verifier.cpp` | `NliFaithfulnessVerifier` | NLI-basierte Faithfulness-Pruefung von RAG-Antworten | probabilistisch |
| `src/rag/quality_control_pipeline.cpp` | `QualityControlPipeline` | Multi-Dimension-QC: Faithfulness, Relevance, Coherence, Completeness, NLI | probabilistisch + konfigurierbarer Score-Gate |
| `src/rag/batch_evaluator.cpp` | `BatchEvaluator` | Batch-Evaluation mit Release-Gate (`BatchEvaluationResult`, `BatchEvaluatorConfig`) | deterministisch (Gate) |
| `src/rag/adversarial_tester.cpp` | `AdversarialTester` | Prompt-Injection-, Poisoning-, Context-Overflow-Tests | deterministisch (Test-Patterns) |
| `src/rag/llm_judge_integration.cpp` | `LLMJudgeIntegration` | LLM-as-Judge fuer RAG-Qualitaet | probabilistisch |
| `src/rag/hallucination_dashboard.cpp` | `HallucinationDashboard` | Aggregation von Halluzinations-Metriken | deterministisch (Aggregation) |
| `src/rag/bias_detector.cpp` | `BiasDetector` | Bias-Erkennung in RAG-Ergebnissen | probabilistisch |
| `src/rag/continuous_learning_orchestrator.cpp` | `ContinuousLearningOrchestrator` | Online-Learning aus RAG-Feedback | probabilistisch |

### 2.3 AQL / Query-Assistenz

| Datei | Klasse / Funktion | Funktion | Entscheidungstyp |
|---|---|---|---|
| `src/aql/llm_aql_handler.cpp` | `LLMAQLHandler` | NL-to-AQL-Uebersetzung mit `sanitizePromptInput()`, `circuit_breakers_` (infer, rag, embed, finetune), `AQLConfidenceScorer`, Similarity-Threshold-Filter | probabilistisch (Uebersetzung) + deterministisch (Injection-Block, Circuit Breaker, Score-Filter) |
| `src/aql/aql_confidence_scorer.cpp` | `AQLConfidenceScorer` | Struktureller/semantischer Konfidenz-Score fuer generierte AQL | deterministisch (Score-Berechnung) |
| `src/aql/aql_model_router.cpp` | `AQLModelRouter` | Routing auf Spezialmodelle je AQL-Domaine | deterministisch |
| `src/aql/aql_lora_finetuner.cpp` | `AQLLoRAFinetuner` | Domain-spezifisches LoRA-Fine-tuning fuer AQL | probabilistisch |
| `src/aql/aql_agent.cpp` | `AQLAgent` | Agentenbasierte AQL-Erstellung | probabilistisch |

### 2.4 Suche (Search)

| Datei | Klasse / Funktion | Funktion | Entscheidungstyp |
|---|---|---|---|
| `src/search/llm_reranker.cpp` | `LlmReranker` | LLM-Scoring je Kandidat; `min_score_threshold`; `fallback_to_original=true` wenn kein Backend | probabilistisch + deterministischer Threshold-Filter + Soft-Fallback |
| `src/search/neural_sparse_retrieval.cpp` | `NeuralSparseRetrieval` | Gelernte Sparse-Repraesentation mit `sanitize()`-Vektor-Bereinigung, `score_threshold` | probabilistisch + deterministischer Score-Filter |
| `src/search/learning_to_rank.cpp` | `LearningToRank` | Supervised Linear Re-Ranker (Click-Feedback-getrained) | probabilistisch (Training) + deterministisch (Scoring) |
| `src/search/llm_query_rewriter.cpp` | `LlmQueryRewriter` | LLM-basierte semantische Query-Umformulierung | probabilistisch |
| `src/search/personalized_ranker.cpp` | `PersonalizedRanker` | Nutzerhistorie-gesteuertes Ranking (LearningToRank) | probabilistisch |

### 2.5 Ingestion / Content-Pipelines

| Datei | Klasse / Funktion | Funktion | Entscheidungstyp |
|---|---|---|---|
| `src/ingestion/llm_adapter.cpp` | `LLMAdapter` | LLM-basierte Entity-Extraktion und Enrichment | probabilistisch |
| `src/ingestion/ingestion_quality_judge.cpp` | `IngestionQualityJudge` | LLM-basierter Qualitaets-Judge (Completeness, Groundedness) | probabilistisch + deterministischer Quality-Gate |
| `src/ingestion/semantic_validator.cpp` | `SemanticValidator` | Regel- und LLM-basierte Dokument-Validierung; `validateBuiltin()` deterministisch | gemischt |
| `src/ingestion/agentic_reference_validator.cpp` | `AgenticReferenceValidator` | Agentenbasierte Referenz-Validierung | probabilistisch |
| `src/ingestion/deontic_extractor.cpp` | `DeonticExtractor` | Normbasierte Deontik-Extraktion aus Dokumenten | probabilistisch |
| `src/ingestion/huggingface_connector.cpp` | `HuggingFaceConnector` | Datenabruf von HuggingFace-Datasets | extern/probabilistisch |

### 2.6 Prompt Engineering

| Datei | Klasse / Funktion | Funktion | Entscheidungstyp |
|---|---|---|---|
| `src/prompt_engineering/prompt_injection_detector.cpp` | `PromptInjectionDetector` | 10 eingebaute Muster (role injection, markup injection etc.) | deterministisch |
| `src/prompt_engineering/rag_context_budget_manager.cpp` | `ContextWindowBudgetManager` | Erzwingt harten Token-Cap (Context-Budget) unabhaengig vom Modell-Limit | deterministisch |
| `src/prompt_engineering/rag_prompt_builder.cpp` | `RAGPromptBuilder` | Template-sichere RAG-Prompt-Konstruktion | deterministisch |
| `src/prompt_engineering/adversarial_prompt_tester.cpp` | `AdversarialPromptTester` | Testen auf Jailbreak/Injection-Robustheit | deterministisch (Testausfuehrung) |
| `src/prompt_engineering/chain_of_thought.cpp` | `ChainOfThought` | CoT-Prompt-Generation | probabilistisch |
| `src/prompt_engineering/self_improvement_orchestrator.cpp` | `SelfImprovementOrchestrator` | Automatische Prompt-Optimierung | probabilistisch |

### 2.7 Analytics / ML-Serving

| Datei | Klasse / Funktion | Funktion | Entscheidungstyp |
|---|---|---|---|
| `src/analytics/ml_serving.cpp` | `MLServingClient` / `ONNXServingBackend` | ONNX Runtime Inferenz-Backend; Fallback bei `!THEMIS_HAS_ONNX` | probabilistisch; Availability-Guard deterministisch |
| `src/analytics/automl.cpp` | `AutoMLTrainer` / `AutoMLModel` | Lokales Decision-Tree- und KNN-Training/Predict; `exportONNX()` | deterministisch (Baumlogik) + probabilistisch (Modellprediction) |
| `src/analytics/anomaly_detection.cpp` | `AnomalyDetector` | Score-Threshold (`cfg.threshold`) fuer is_anomaly | deterministisch (Threshold-Gate) |
| `src/analytics/llm_process_analyzer.cpp` | `LLMProcessAnalyzer` | LLM-gestuetzte Prozess-Analyse | probabilistisch |
| `src/analytics/forecasting.cpp` | `ForecastingEngine` | Zeitreihen-Forecasting | probabilistisch |

### 2.8 Acceleration / Hardware-Dispatcher

| Datei | Klasse / Funktion | Funktion | Entscheidungstyp |
|---|---|---|---|
| `src/acceleration/ai_hardware_dispatcher.cpp` | `AiHardwareDispatcher` | Hardware-Probe + Dispatching auf CUDA/HIP/CPU mit `dispatchGpuFallback()` → `dispatchCpuFallback()` | deterministisch (Fallback-Chain) |
| `src/acceleration/vllm_resource_manager.cpp` | `VLLMResourceManager` | GPU-Utilization-basierter CPU-Fallback (Timeout 500 ms → safe CPU fallback) | deterministisch (Timeout-Gate) |
| `src/acceleration/faiss_gpu_backend.cpp` | `FaissGpuBackend` | GPU-Vector-Search mit FAISS | probabilistisch (ANN) |
| `src/acceleration/vec_knn.cpp` | `VecKNN` | KNN-Vector-Suche | probabilistisch |

### 2.9 LLM-Sicherheitsschicht

| Datei | Klasse / Funktion | Funktion | Entscheidungstyp |
|---|---|---|---|
| `src/llm/prompt_policy.cpp` | `PromptPolicy` | Regelbasierter Prompt-Filter: `allowed=false` blockiert Request; `sanitized_prompt` mit Redaktionen | deterministisch |
| `src/llm/lora_security_validator.cpp` | `LoRASecurityValidator` | Signaturpruefung, Integritaets-Hash, Gewichts-Anomalie-Erkennung (`detectDistributionShift()`), Prompt-Sanitization | deterministisch (Crypto) + heuristisch (Anomalie) |
| `src/llm/lora_certificate_store.cpp` | `LoRACertificateStore` | OpenSSL X.509 Store fuer CA-Bundle-basierte Cert-Chain-Validation | deterministisch |
| `src/llm/security/signature_verifier.cpp` | `RSA_SHA256_Verifier` | RSA/SHA-256 Signaturpruefung fuer Modell-Releases | deterministisch |
| `src/llm/ai_decision_auditor.cpp` | `AIDecisionAuditor` | Kryptographisch signierter AI-Entscheidungs-Audit-Log; `confidence_score < 0.7` → `requires_human_review=true` | deterministisch |

### 2.10 Governance (Model Use)

| Datei | Klasse / Funktion | Funktion | Entscheidungstyp |
|---|---|---|---|
| `src/governance/model_governance.cpp` | `ModelGovernancePolicy::checkExportPermission()` | Klassifikations-Check (`geheim`/`streng-geheim` blockiert) + Restricted-Collection-Check vor jedem Modell-Training-Export | deterministisch |
| `src/governance/compliance_reporter.cpp` | `ComplianceReporter::generateBiasAuditReport()` | Demografische Parity + Repraesentationsstatistik fuer Training-Datasets | deterministisch (Statistik) |
| `src/governance/policy_engine.cpp` | `PolicyEngine` | Deterministischer Policy-Gatekeeper (Klassifikation, Export, Cache, Masking, CCPA); `setModelGovernancePolicy()` | deterministisch |

### 2.11 Training

| Datei | Klasse / Funktion | Funktion | Entscheidungstyp |
|---|---|---|---|
| `src/training/training_pipeline.cpp` | `TrainingPipeline` | LoRA-Fine-tuning-Pipeline | probabilistisch |
| `src/training/auto_labeler.cpp` | `AutoLabeler` | LLM-gestuetzte automatische Datensatz-Beschriftung | probabilistisch |
| `src/training/lora_checkpoint_manager.cpp` | `LoRACheckpointManager` | Deterministische Checkpoint-Verwaltung | deterministisch |
| `src/training/provenance_tracker.cpp` | `ProvenanceTracker` | Trainings-Datenherkunfts-Tracking | deterministisch |

### 2.12 Spezialisierte Plugins

| Plugin | Datei | Funktion | Entscheidungstyp |
|---|---|---|---|
| Whisper (Speech-to-Text) | `src/whisper/whisper_plugin.cpp` | Audio-Transkription; Stub-Fallback (`InMemoryWhisperTranscriber`) | probabilistisch; Fallback deterministisch |
| Stable Diffusion | `src/stable_diffusion/sd_plugin.cpp` | Bild-Generierung mit `SDPromptSanitizer` (Keyword-Blocklist) | probabilistisch + deterministischer Keyword-Block |
| ONNX CLIP | `src/onnx_clip/onnx_clip_plugin.cpp` | Bild-/Text-Embedding via ONNX Runtime; Zero-Vector-Fallback | probabilistisch; Fallback deterministisch |

### 2.13 Ethics-AI

| Datei | Klasse / Funktion | Funktion |
|---|---|---|
| `src/ethics_ai/ethics_evaluator.cpp` | `EthicsEvaluator` | Ethik-Bewertung von Antworten |
| `src/ethics_ai/discourse_engine.cpp` | `DiscourseEngine` | Diskurs-basierte Argumentations-Analyse |
| `src/llm/constitutional_reasoning_engine.cpp` | `ConstitutionalReasoningEngine` | Iterative Selbstkorrektur: `checkAutonomyRespect()`, `checkNonHarmfulness()`, `checkFairness()` |

## 3) Deterministisch vs. Probabilistisch — Taxonomie

**Hard deterministisch (keine Modell-Abhaengigkeit):**

- `PromptPolicy::apply()` — blockiert/sanitiert nach statischen Regeln
- `ModelGovernancePolicy::checkExportPermission()` — Allow/Deny nach Klassifikation und Restricted-Collection
- `AIDecisionAuditor::logDecision()` — flaggt `requires_human_review` wenn `confidence_score < 0.7`
- `AnomalyDetector` — is_anomaly nach konfiguriertem Schwellenwert
- `LoRASecurityValidator::verifySignature()` / `checkIntegrity()` — Crypto-Validierung
- `LoRACertificateStore` + `RSA_SHA256_Verifier` — X.509 CA-Chain-Validierung
- `ContinuousBatchScheduler` — max_tokens_per_batch-Budget
- `RAGContextAssembler::ContextWindowBudget` — harter Token-Cap
- `PromptInjectionDetector` (beide Implementierungen) — Muster-Match
- `SDPromptSanitizer` — Keyword-Blocklist
- `NeuralSparseRetrieval::sanitize()` / score_threshold — Vektor-Bereinigung
- `ModelRouter::route()` — regelbasiertes Model-Routing

**Probabilistisch (Modell-/Embedding-Ausgabe):**

- Alle `generate()` / `embed()` / `translateNLToAQL()` / `rerank()` Aufrufe
- RAG-Retrieval-Scores, Faithfulness-Scores, Halluzinations-Metriken
- AutoML-Predictions, Forecasting, Anomaliescores (Score-Berechnung selbst ist deterministisch; Modell-Training nicht)
- Agentic-Tool-Calls, Constitutional-Reasoning-Revisionen

**Gemischt (deterministischer Gate vor probabilistischem Pfad):**

- `LLMAQLHandler`: Injection-Block (deterministisch) → NL-Uebersetzung (probabilistisch) → AQL-Validierung (deterministisch)
- `AsyncInferenceEngine`: PromptPolicy-Gate (deterministisch) → Inferenz (probabilistisch)
- `LlmReranker`: Score-Computation (probabilistisch) → min_score_threshold-Filter (deterministisch)
- `QualityControlPipeline`: Faithfulness/NLI-Score (probabilistisch) → Release-Gate (deterministisch)

## 4) Fehlerbilder (Halluzination, Bias, Drift, Injection, Missklassifikation)

| Fehlerbild | Wo im Code | Bestehende Gegenmassnahme |
|---|---|---|
| Halluzination in RAG-Antworten | `src/rag/llm_integration.cpp`, `src/rag/nli_faithfulness_verifier.cpp` | `NliFaithfulnessVerifier`, `QualityControlPipeline`, `HallucinationDashboard`, LLM-Judge |
| Prompt-/Context-Injection | `src/aql/llm_aql_handler.cpp:58–225`, `src/rag/prompt_injection_detector.cpp`, `src/prompt_engineering/prompt_injection_detector.cpp`, `src/llm/async_inference_engine.cpp:838–854` | `sanitizePromptInput()`, `PromptInjectionDetector` (2 unabhaengige Implementierungen), `PromptPolicy::apply()` |
| Missklassifikation / falscher AQL | `src/aql/llm_aql_handler.cpp:1723–1737` | `AQLConfidenceScorer` + `similarity_threshold`-Filter + Circuit Breaker |
| Bias in Training-Daten | `src/governance/compliance_reporter.cpp:878` | `generateBiasAuditReport()` (demografische Parity, Repraesentationsstatistik) |
| Model/Weight-Drift (LoRA) | `src/llm/lora_security_validator.cpp:907` | `detectDistributionShift()` in `LoRASecurityValidator` |
| Data/Statistics-Drift (Query) | `src/query/plan_cache.cpp:104–107`, `src/performance/workload_adaptive_optimizer.cpp:237` | `isDriftExceeded()` im PlanCache, `getProfileDrift()` im WorkloadAdaptiveOptimizer |
| Latenz-/Kostenexplosion | `src/llm/continuous_batch_scheduler.cpp:550`, `src/aql/llm_aql_handler.cpp:292–303` | `max_tokens_per_batch`, CircuitBreaker (infer/rag/embed/finetune) |
| Datenabfluss via Prompts/Logs | `src/llm/prompt_policy.cpp:97`, `src/llm/lora_framework/feedback_plugin.cpp:40`, `src/llm/vision_config.cpp:207` | PromptPolicy-Redaktion, PII-Filter in FeedbackPlugin, `include_pii=false` Default |
| Supply-Chain (LoRA-Weights) | `src/llm/lora_certificate_store.cpp`, `src/llm/security/signature_verifier.cpp` | X.509-CA-Bundle, RSA/SHA-256 Signaturpruefung |
| Modell-Training auf verbotenen Daten | `src/governance/model_governance.cpp:89–138` | `checkExportPermission()` vor jedem Training-Export |
| Agentic Tool-Calling unkontrolliert | `include/llm/ai_orchestrator.h:148–179` | `tools_allowed`/`tools_denied` Lists in `ModeSpec`, `BudgetSpec.max_tokens`, `BudgetSpec.timeout_ms` |

## 5) Fallbacks pro kritischem Pfad

| Pfad | Hard-Fallback | Soft-Fallback | Quelle |
|---|---|---|---|
| LLM-Inferenz (infer CB) | Circuit Breaker OPEN → sofortiger Error | `LOCAL_FALLBACK_NO_MATCH`, `LOCAL_FALLBACK_LOW_ACCURACY` | `src/aql/llm_aql_handler.cpp:476,705` |
| RAG-Pipeline (rag CB) | Circuit Breaker OPEN | Degraded-Mode (keine Dokumente) | `src/aql/llm_aql_handler.cpp:821` |
| Embedding (embed CB) | Circuit Breaker OPEN | Zero-Vector-Stub (ONNX CLIP, llama_cpp) | `src/aql/llm_aql_handler.cpp:1021`, `src/llama_cpp/llama_cpp_plugin.cpp:298` |
| GPU-Inferenz | `dispatchGpuFallback()` → `dispatchCpuFallback()` (CPU SIMD) | — | `src/acceleration/ai_hardware_dispatcher.cpp:818–833` |
| GPU-Busy (VLLM) | CPU-Fallback wenn GPU-Util > Schwelle oder Timeout 500 ms | — | `src/acceleration/vllm_resource_manager.cpp:172–185` |
| LLM-Reranking | `fallback_to_original=true` → Originalreihenfolge | Score-Filter per `min_score_threshold` | `src/search/llm_reranker.cpp:79` |
| LlamaCpp (kein Modell) | Stub-Echo-Response + Zero-Vector-Embedding | — | `src/llama_cpp/llama_cpp_plugin.cpp:214,298` |
| OPA-Evaluierung | CPU-Fallback auf native PolicyEngine + Counter-Increment | — | `src/governance/opa_adapter.cpp`, `governance_opa_fallback_total` |
| Model-Export (Training) | `ModelGovernancePolicy::checkExportPermission()` → DENY + Audit-Log | — | `src/governance/model_governance.cpp:89–138` |
| Prompt-Injection erkannt | `sanitizePromptInput()`: Exception + Error-Return | Sanitized-Prompt weiterleiten | `src/aql/llm_aql_handler.cpp:221–225` |
| AsyncInference Policy-Block | Blocked-Response ohne Inferenz | — | `src/llm/async_inference_engine.cpp:839–851` |

## 6) Bestehende Kontrollen (Ist-Zustand)

### 6.1 Input-Sanitization

- `src/aql/llm_aql_handler.cpp:152–225`: `sanitizePromptInput()` — Null-Byte, Pattern-Check (30+ Muster: Rolleninjektion, Delimiter, System-Prompt-Marker).
- `src/rag/prompt_injection_detector.cpp`: `PromptInjectionDetector` — unabhaengige zweite Implementierung mit `InjectionScanResult` und `scan_density`-Schwellenwert.
- `src/llm/prompt_policy.cpp`: `PromptPolicy::apply()` — konfigurierbare Regex-Regeln; blockiert oder redaktiert.
- `src/stable_diffusion/sd_prompt_sanitizer.cpp`: `SDPromptSanitizer` — Keyword-Blocklist aus YAML-Datei.

### 6.2 Output-Validierung

- `src/aql/aql_confidence_scorer.cpp`: `AQLConfidenceScorer` — strukturelle und semantische Pruefung generierter AQL.
- `src/rag/quality_control_pipeline.cpp`: `QualityControlPipeline` — Faithfulness, Relevance, Coherence, Completeness, NLI-Score-Gates.
- `src/rag/nli_faithfulness_verifier.cpp`: NLI-basierte Faithfulness-Verifikation.
- `src/analytics/anomaly_detection.cpp`: Score-Threshold-Gate.

### 6.3 Confidence-Scoring und Human-Review-Gate

- `src/llm/ai_decision_auditor.cpp:202–205`: `confidence_score < 0.7` → `requires_human_review=true` + WARN-Log.
- `src/aql/llm_aql_handler.cpp:1723–1737`: `AQLConfidenceScorer` mit structural/completeness/schema_match-Subscores.

### 6.4 Circuit Breaker

- `src/aql/llm_aql_handler.cpp:292–303`: Vier unabhaengige `sharding::CircuitBreaker`-Instanzen (infer, rag, embed, finetune) mit `getCircuitBreakerStates()`.

### 6.5 Budget- und Rate-Kontrolle

- `src/llm/continuous_batch_scheduler.cpp:550`: `max_tokens_per_batch`-Enforcement.
- `include/llm/ai_orchestrator.h:116–122`: `BudgetSpec` (max_tokens, timeout_ms, max_retries) je Mode.
- `src/rag/rag_context_assembler.cpp:85–132`: `ContextWindowBudget::compute()` + haerter Token-Cap.
- `include/rag/agentic_rag.h:146`: `max_iterations=5` default fuer AgenticRAG-Loop.

### 6.6 Modell-Sicherheit (Supply-Chain)

- `src/llm/lora_certificate_store.cpp`: X.509-CA-Bundle (`X509_STORE`) fuer LoRA-Cert-Chain.
- `src/llm/security/signature_verifier.cpp`: RSA/SHA-256-Signatur-Verifikation.
- `src/llm/lora_security_validator.cpp`: Integritaets-Hash, Gewichts-Anomalie (`detectDistributionShift()`), Prompt-Sanitization.
- `src/governance/model_governance.cpp`: Pre-Export-Policy-Gate + Audit-Log.

### 6.7 Audit / Observability

- `src/llm/ai_decision_auditor.cpp`: Kryptographisch signierte (`signDecision()`) AI-Entscheidungs-Audit-Eintraege im RocksDB-Store.
- `src/llm/llm_model_audit_logger.cpp`: `PROMPT_REDACTED`-Event-Typ in LLM-Audit-Log.
- `src/governance/model_governance.cpp`: `governance_model_export_total` Prometheus-Counter (labels: `result=permitted|denied_classification|denied_restricted_collection`).
- `src/governance/opa_adapter.cpp`: `governance_opa_fallback_total` Counter.
- `include/llm/ai_orchestrator.h:126–130`: `ObservabilitySpec` (log_requests, metrics, trace) je Mode.

### 6.8 Governance-Gate fuer Modell-Training

- `src/governance/model_governance.cpp:89–138`: `ModelGovernancePolicy::checkExportPermission()` prueft Klassifikation (geheim/streng-geheim blockiert) und Restricted-Collection vor jedem Training-Export.
- `src/governance/compliance_reporter.cpp:878`: `generateBiasAuditReport()` fuer Training-Datasets.

### 6.9 Tool-Allowlist / Agentic-Control

- `include/llm/ai_orchestrator.h:158–161`: `ModeSpec.tools_allowed` + `ModeSpec.tools_denied` — explizite Whitelist/Blacklist fuer Tool-Calls.
- `include/llm/ai_orchestrator.h:172–176`: `ModeSpec.safety` — optionales Ethics-Profil fuer Mode.

## 7) Identifizierte Luecken / Risiken (Ist vs. Soll)

### Kritikalitaetsklassen

| Klasse | Bedeutung |
|---|---|
| **S0** | Sicherheitskritisch — sofortige Eskalation erforderlich |
| **S1** | Betriebskritisch — Behebung innerhalb von 30 Tagen |
| **S2** | Erhoehtes Risiko — Behebung im naechsten Sprint |
| **S3** | Niedrig / Verbesserungspotenzial — Backlog-Eintrag genuegt |

### Eskalationspfade

Bei Auftreten oder Verschlechterung eines identifizierten Risikos:

1. **Sofortmassnahme (S0/S1):** Issue im GitHub-Repository erstellen, Label `governance-incident`, Assign an Governance-Owner; CircuitBreaker manuell oeffnen falls LLM-Pfad betroffen.
2. **Kurzfrist (S1/S2):** Eintrag in `src/governance/FUTURE_ENHANCEMENTS.md`; Sprint-Planning-Ticket; Weekly Governance-Sync informieren.
3. **Mittelfrist (S2/S3):** Quartalsbericht (`ComplianceReporter::generateBiasAuditReport()`); KPI-Delta-Tracking; Review in Quarterly ML/AI Risk Review.
4. **Audit-Eskalation:** Bei Compliance-Verstoss (GDPR/HIPAA/CCPA) → `ComplianceReporter` + DPO/Legal informieren; Eintrag in `AIDecisionAuditor` + Nachweis in Abschnitt 10.

### Risikokatalog (aktueller Stand 2026-05-13)

| # | Luecke | Severity | Betroffene Dateien | Status | Eskalationspfad |
|---|---|---|---|---|---|
| G-01 | `LlamaCppPlugin::generate()` — Stub-Echo ohne expliziten Fehler wenn kein Modell geladen | S1 | `src/llama_cpp/llama_cpp_plugin.cpp:214` | ✅ Behoben 2026-04-21: `success=false` + `STUB_MODE` flag | — |
| G-02 | `LlmQueryRewriter` — kein Fallback und kein Output-Validator | S2 | `src/search/llm_query_rewriter.cpp` | ✅ Behoben 2026-04-21: Jaccard-Overlap-Filter + `RewriteQuality` Enum | — |
| G-03 | `InlineTrainingEngine` — LoRA-Fine-tuning ohne Pre-Training-Policy-Gate | S0 | `src/llm/inline_training_engine.cpp` | ✅ Behoben 2026-04-21: `setGovernancePolicy()` + `require_policy_gate` | — |
| G-04 | `AgenticRAG`/`AQLAgent` — kein Session-Token-Budget-CAP | S1 | `src/rag/agentic_rag.cpp`, `src/aql/aql_agent.cpp` | ✅ Behoben 2026-04-21: `max_session_tokens` + `BUDGET_EXCEEDED` | — |
| G-05 | Zwei unabhaengige `PromptInjectionDetector`-Implementierungen ohne gemeinsamen Muster-Stand | S2 | `src/rag/prompt_injection_detector.cpp`, `src/prompt_engineering/prompt_injection_detector.cpp` | ✅ Behoben 2026-04-21: `PromptInjectionPatternRegistry` (11 Patterns + 11 Keywords); Shared-Base | — |
| G-06 | Kein zentrales ML/AI-Rate-Limit fuer externe Token-Kosten | S1 | `src/llm/`, `src/aql/` | 🔴 Offen — Q4 2026 (`LLMTokenBudgetManager`) | S1-Eskalation: Issue `governance-incident`; `FUTURE_ENHANCEMENTS.md` Gap 6 |
| G-07 | `LLMJudgeIntegration` gibt Mock-Scores zurueck ohne strukturierten Fehler | S2 | `src/rag/llm_judge_integration.cpp` | 🟡 Teilweise — `is_mock=true` Flag gesetzt; `RAGError::JudgeUnavailable` fehlt noch | S2-Eskalation: Sprint-Ticket; `FUTURE_ENHANCEMENTS.md` B-11 |
| G-08 | `HuggingFaceConnector` — externer Datenabruf ohne vollstaendiges Datenklassifikations-Gate | S1 | `src/ingestion/huggingface_connector.cpp` | ✅ Behoben 2026-04-21: `setIngestionPolicy()` + `checkExportPermission()` in `initialize()` | — |
| G-09 | Kein zentrales ML/AI API-Kostenbudget + Alert fuer alle Pfade | S1 | `src/llm/`, `src/aql/`, `src/rag/` | 🔴 Offen — Q4 2026 | S1-Eskalation: Issue `governance-incident`; CircuitBreaker als Notfall-Gate |
| G-10 | `DataClassificationGate` fehlt fuer weitere externe Connectors (z.B. WebCrawler) | S2 | `src/ingestion/` | 🔴 Offen — Q4 2026 | S2-Eskalation: Sprint-Ticket |

## 8) KPIs / Zielwerte

| KPI | Zielwert | Messmethode / Quelle | Aktueller Stand |
|---|---|---|---|
| ML/AI-Touchpoints inventarisiert + klassifiziert | 100% | Dieses Dokument (Abschnitt 2) | ✅ 100% (13 Subsysteme, 60+ Touchpoints) |
| Kritische Touchpoints mit Hard-Fallback + Audit-Trace | 100% (S0/S1) | Abschnitt 5 / 6.7 | ✅ Alle S0/S1-Pfade abgedeckt (Stand 2026-05-13) |
| Offene S0/S1-Gaps im Risikokatalog | 0 | Abschnitt 7 | 🟡 2 offen (G-06, G-09) — Q4 2026 |
| p95-Latenzaufschlag durch AI-Integration | <= 5% | CircuitBreaker-Metriken + Prometheus | 🔵 Messung geplant ab Q3 2026 Pilot |
| Qualitaets-/Effizienzgewinn in Pilot-Use-Cases | >= 20% | `QualityControlPipeline`, `BatchEvaluator` Release-Gates | 🔵 Messung geplant ab Q3 2026 Pilot |
| Ungepruefte Modell-Releases in Produktion | 0 | `LoRASecurityValidator` + `RSA_SHA256_Verifier` + `ModelGovernancePolicy` | ✅ Gate aktiv seit 2026-04-21 |
| Governance-Dokument aktuell (max. 90 Tage alt) | <= 90 Tage | Letztes Review-Datum (Abschnitt 10) | ✅ Letztes Review: 2026-05-13 |

## 9) Offene Folgeaufgaben (Backlog-Seeds)

- [x] Stub-Pfad in `LlamaCppPlugin::generate()` zu strukturiertem Fehler upgraden (S1) — ✅ 2026-04-21: `success=false` + STUB_MODE flag
- [x] `InlineTrainingEngine`: Pre-Training-Policy-Gate mit `ModelGovernancePolicy` verdrahten (S0) — ✅ 2026-04-21: `setGovernancePolicy()` + `require_policy_gate` + ITE_GOV_A1..A4 tests
- [x] `AgenticRAG`: Session-Token-Budget-Cap verdrahten (S1) — ✅ 2026-04-21: `max_session_tokens` + `BUDGET_EXCEEDED` + ARG_BUD_01..05 tests
- [x] `LLMJudgeIntegration`: Mock-Scores durch `ParsedResponse::is_mock` Flag kennzeichnen (S2) — ✅ 2026-04-21: `is_mock=true` in mock-mode; JGI_MOCK_01..05 tests
- [x] `HuggingFaceConnector`: Datenklassifikations-Gate vor Ingestion (S1) — ✅ 2026-04-21: `setIngestionPolicy()` + `checkExportPermission()` in `initialize()`; HFC_GOV_01..05 tests
- [x] `LlmQueryRewriter`: Semantischer Output-Validator + `RewriteQuality` Enum (S2) — ✅ 2026-04-21: Jaccard-Overlap-Filter + `quality=OK|FALLBACK`; LQR_VAL_01..06 tests
- [x] Konsolidierung der zwei `PromptInjectionDetector`-Implementierungen (S2) — ✅ 2026-04-21: `PromptInjectionPatternRegistry` (11 Patterns + 11 Keywords); PRR_01..08 tests; RAG+PE Detector laden Shared-Base
- [ ] Token-Kosten-Budget-Tracking fuer externe API-Calls (Gap G-06 — S1) — Requires `LLMTokenBudgetManager` (Q4 2026)
- [ ] `RAGError::JudgeUnavailable` bei `engine==nullptr` im strict-mode (Gap G-07 Rest — S2) — Q4 2026
- [ ] `DataClassificationGate` fuer alle externen Connectors (Gap G-10 — WebCrawler etc.) — Q4 2026
- [ ] Zentrales ML/AI API-Kostenbudget + Alert (Gap G-09 — S1) — Q4 2026
- [ ] Quarterly ML/AI Risk Review cadence einrichten (KPI-Deltas + Remediation-Tracking) — Q4 2026

## 10) Review- / Audit-Nachweis

### Dokumentreview-Zyklus

Dieses Dokument wird bei jedem Governance-Review-Zyklus (mindestens quartalsweise)
aktualisiert. Bei wesentlichen Änderungen an ML/AI-Touchpoints (neue Module, neue
Risikoklassen, Status-Änderungen in Abschnitt 7) ist ein Ad-hoc-Update erforderlich.

### Review-Historie

| Version | Datum | Reviewer | Scope | Ergebnis |
|---|---|---|---|---|
| v1 | 2026-03-15 | Governance-Team | Initiale Touchpoint-Inventarisierung (Abschnitt 2) | Freigegeben |
| v2 | 2026-04-21 | Governance-Team, Security-Audit | Vollständige Sourcecode-Analyse; Abschnitte 3–9 ergänzt; 7 S0/S1-Gaps identifiziert und behoben | Freigegeben |
| v3 | 2026-05-13 | Governance-Team | Abgrenzung (Abschnitt 0) ergänzt; Eskalationspfade (Abschnitt 7) dokumentiert; KPI-Ist-Stand (Abschnitt 8) aktualisiert; Audit-Nachweis (Abschnitt 10) hinzugefügt | Freigegeben |

### Audit-Checkliste (Akzeptanzkriterien Issue-Check)

- [x] Inhalt fachlich aktuell (Stand: 2026-05-13, v3)
- [x] Risiken/Metriken und Eskalationspfade eindeutig (Abschnitt 7 — Risikokatalog mit Eskalationspfad-Spalte)
- [x] Review-/Audit-Nachweis dokumentiert (dieser Abschnitt)
- [x] Fachreview durchgefuehrt (v3 Review: 2026-05-13)
- [x] Sourcecode-/Dokumentationsaudit durchgefuehrt (Sourcecode-Analyse 2026-04-21, Dokumentations-Update 2026-05-13)
- [x] Betroffene Dateien im Review festgehalten:
  - `src/governance/AI_ML_IMPACT_ASSESSMENT.md` (dieses Dokument)
  - `src/governance/ROADMAP.md` (Phase 5/6 Status)
  - `src/governance/README.md` (Dokumentationsverzeichnis)

### Verlinkung zu Audit-Artefakten

- Audit-Framework: [`docs/audit-framework/AUDIT_RUNBOOK.md`](../../docs/audit-framework/AUDIT_RUNBOOK.md)
- Dokumentations-Review-Richtlinien: [`docs/DOCUMENTATION_REVIEW_GUIDELINES.md`](../../docs/DOCUMENTATION_REVIEW_GUIDELINES.md)
- Systematischer Reviewplan: [`docs/SYSTEMATISCHER_REVIEWPLAN.md`](../../docs/SYSTEMATISCHER_REVIEWPLAN.md)
- Sourcecode-Audit: [`docs/de/development/SOURCE_CODE_AUDIT.md`](../../docs/de/development/SOURCE_CODE_AUDIT.md)
