> ⚠️ **Historisches Changelog** – Einträge beschreiben den Stand zum Zeitpunkt der Erstellung.

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — RAG Module

All notable changes to the RAG module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

### Changed
- Documentation governance sync: roadmap/future/audit/readme/architecture/security/performance docs aligned to source-verifiable statements; planning remains in roadmap/future and history remains in changelog.

## [2.1.0] — 2026-04-16
### Added
- `RAGIngestionBridge` (`include/rag/rag_ingestion_bridge.h`, `src/rag/rag_ingestion_bridge.cpp`; `themis::rag` namespace):
  - `IndexResult` return type: `ok`, `doc_id`, `collection`, `entity_count`, `vector_count`, `error`
  - `indexDocument(text, collection)` — full ingestion workflow; writes to `IVectorWriter` and optional `IGraphWriter`; idempotent via content-hash `doc_id`
  - `enrichRetrievedDocuments(docs, query)` — attaches NER entities under `"_entities"` metadata key
  - `extractEntitiesForContext(text)` — delegates to `IngestionToolbox::extractEntities()`
  - `static buildEntityContext(entities)` — converts `BaseEntity` list to compact string for LLM prompt injection
  - Thread-safe: no mutable state beyond constructor-injected shared pointers

## [2.0.1] — 2026-04-15
### Added
- `ContinuousLearningOrchestrator` (`include/rag/continuous_learning_orchestrator.h`, `src/rag/continuous_learning_orchestrator.cpp`; `themis::rag::learning` namespace):
  - `ContinuousLearningConfig` — trigger thresholds, learning rates, A/B testing config, data-selection integration, federation paths
  - `startLearningLoop()` / `stopLearningLoop()` — background learning loop
  - `triggerLearningIteration()` — manual trigger
  - `LoopPhase` enum: `IDLE`, `LOOP_1_HNSW_QUERY`, `LOOP_2_WORKLOAD`, `LOOP_3_SCHEMA_INDEX`, `LOOP_4_RLAIF`
  - `LoopResult` — `phase`, `success`, `guardrail_passed`, `adapter_version`, `metric_delta`
  - `triggerLoop(LoopPhase)` — explicit named loop trigger with `LoopResult` return
  - `registerLoopCompletionHandler(LoopPhase, handler)` — per-phase synchronous completion callback
  - `TriggerEvent::FEDERATED_ROUND_START` — fires after Loop-4 with `guardrail_passed == true`
  - `setFederationCoordinator(ILoRAFederationCoordinator*)` — DI setter for federated LoRA aggregation
  - `setTrainerForFederation(IncrementalLoRATrainer*)` — DI setter for gradient export

## [2.0.0] — 2026-03-24
### Added
- `ReplugRetriever` — REPLUG-style co-trained retrieval fusion (Shi et al., 2023, arXiv:2301.12652):
  - `ILLMScorer` pluggable interface for model-agnostic perplexity scoring
  - `HeuristicLLMScorer` — Jaccard-based PPL proxy (no LLM runtime required)
  - `ReplugConfig` — λ interpolation weight, top_k, temperature, min_retrieval_score, LSR learning rate
  - `ReplugFusionResult` / `ReplugScore` — per-document score breakdown
  - `updateRetrieverWeights()` — REPLUG-LSR gradient step via KL-divergence
  - `ReplugRetrieverFactory` — createBalanced/createLLMDominant/createRetrievalDominant/createLSR
- `RLAIFTrainer` — Constitutional AI / RLAIF training pipeline (Bai et al., 2022; Lee et al., 2023):
  - `IAIJudge` pluggable interface for AI preference labelling
  - `HeuristicAIJudge` — heuristic critique/revision/judge (no LLM runtime required)
  - `AIPrinciple` — configurable constitutional principles (harmlessness/helpfulness/honesty/fairness)
  - `PreferencePair` — (prompt, chosen, rejected) training example
  - `runTrainingStep()` — end-to-end CAI critique-revision + preference pair generation
  - `createPreferencePair()` — direct judge-based preference labelling
  - `processBatch()` — queue-based batch training
  - `RLAIFConfig` — revision iterations, quality threshold, max dataset size
  - `RLAIFTrainerFactory` — createDefault/createStrict/createFast/createWithJudge
- 30 unit tests for `ReplugRetriever` (`tests/test_rag_replug_retriever.cpp`)
- 30 unit tests for `RLAIFTrainer` (`tests/test_rag_rlaif_trainer.cpp`)
- Focused test executables: `ReplugRetrieverFocusedTests`, `RLAIFTrainerFocusedTests`

## [1.5.0] — 2026-03-12
### Added
- Multi-modal RAG pipeline with text, image, and audio retrieval
- Hybrid dense+sparse retrieval (BM25 + vector)
- Cross-encoder reranking for improved relevance scoring
- Contextual compression to reduce token usage
- Citation and source attribution in generated answers
- RAG evaluation framework with RAGAS metrics
- Streaming RAG responses via SSE

### Changed
- Improved chunking strategies (sentence-aware, recursive)
- Enhanced metadata filtering during retrieval

## [1.0.0] — 2024-06-01
### Added
- Initial RAG pipeline: document ingestion, chunking, embedding, retrieval
- Integration with LLM module for answer generation
- Vector store integration for semantic search

## Issue Scope Traceability

- Wave B tracking issue: `https://github.com/makr-code/ThemisDB/issues/5039`
- dependent Wave A issue: `https://github.com/makr-code/ThemisDB/issues/5038`
- follow-on Wave C issue: `https://github.com/makr-code/ThemisDB/issues/5040`
