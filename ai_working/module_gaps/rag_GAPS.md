# rag Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: rag
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 1625
- Actionable Findings (Critical + High): 1012
- Affected Files: 64

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 353 |
| High | 659 |
| Medium | 613 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| llm_ai_safety | 651 |
| container | 282 |
| performance_patterns | 228 |
| reliability | 83 |
| platform | 71 |
| concurrency | 64 |
| audit_logging | 56 |
| determinism | 52 |
| memory | 29 |
| exception_safety | 28 |
| performance | 28 |
| security | 24 |
| observability | 12 |
| raii | 6 |
| uninitialized | 4 |
| legacy_duplication | 3 |
| distributed_consistency | 2 |
| type_conversion | 2 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/rag/rag_judge.cpp | 185 | 78 | 80 | 27 | 0 |
| src/rag/batch_evaluator.cpp | 137 | 51 | 76 | 10 | 0 |
| src/rag/adversarial_tester.cpp | 97 | 46 | 40 | 11 | 0 |
| src/rag/evaluation_report_exporter.cpp | 79 | 2 | 6 | 71 | 0 |
| src/rag/dpr_vectorizer.cpp | 75 | 28 | 39 | 8 | 0 |
| src/rag/knowledge_gap_detector.cpp | 73 | 2 | 29 | 40 | 2 |
| src/rag/continuous_learning_orchestrator.cpp | 52 | 14 | 23 | 15 | 0 |
| src/rag/distributed_rag_evaluator.cpp | 48 | 10 | 26 | 12 | 0 |
| src/rag/document_summarizer.cpp | 43 | 9 | 10 | 24 | 0 |
| src/rag/llm_judge_client.cpp | 43 | 3 | 30 | 10 | 0 |
| src/rag/knowledge_graph_retriever.cpp | 39 | 1 | 17 | 21 | 0 |
| src/rag/multi_step_rag.cpp | 39 | 2 | 13 | 24 | 0 |
| src/rag/rlaif_trainer.cpp | 35 | 7 | 16 | 12 | 0 |
| src/rag/reranker.cpp | 34 | 13 | 7 | 14 | 0 |
| src/rag/llm_judge_integration.cpp | 33 | 2 | 29 | 2 | 0 |
| src/rag/llm_meta_analyzer.cpp | 33 | 15 | 16 | 2 | 0 |
| src/rag/delegate_evaluator.cpp | 29 | 4 | 12 | 13 | 0 |
| src/rag/fairness_detector.cpp | 29 | 1 | 11 | 17 | 0 |
| src/rag/calibration_manager.cpp | 27 | 5 | 4 | 17 | 1 |
| src/rag/multi_hop_reasoner.cpp | 27 | 0 | 11 | 16 | 0 |
| src/rag/llm_integration.cpp | 26 | 0 | 17 | 8 | 1 |
| src/rag/multimodal_rag.cpp | 24 | 5 | 3 | 16 | 0 |
| src/rag/prompt_injection_detector.cpp | 24 | 4 | 6 | 14 | 0 |
| src/rag/examples/loop_orchestration_example.cpp | 23 | 0 | 19 | 4 | 0 |
| src/rag/hybrid_retriever.cpp | 22 | 0 | 8 | 14 | 0 |
| src/rag/prompt_templates.cpp | 20 | 9 | 10 | 1 | 0 |
| src/rag/replug_retriever.cpp | 20 | 1 | 9 | 10 | 0 |
| src/rag/agentic_rag.cpp | 19 | 2 | 6 | 11 | 0 |
| src/rag/continuous_learning_client.cpp | 19 | 2 | 7 | 10 | 0 |
| src/rag/onnx_model_loader.cpp | 18 | 9 | 6 | 3 | 0 |
| src/rag/quality_control_pipeline.cpp | 18 | 3 | 0 | 15 | 0 |
| src/rag/evaluation_cache.cpp | 16 | 10 | 6 | 0 | 0 |
| src/rag/geval_evaluator.cpp | 16 | 1 | 5 | 10 | 0 |
| src/rag/response_parser.cpp | 14 | 0 | 4 | 10 | 0 |
| src/rag/claim_extractor.cpp | 12 | 0 | 3 | 9 | 0 |
| src/rag/faithfulness_evaluator.cpp | 11 | 1 | 0 | 10 | 0 |
| src/rag/bayesian_optimizer.cpp | 10 | 0 | 4 | 6 | 0 |
| src/rag/citation_highlighter.cpp | 10 | 0 | 3 | 7 | 0 |
| src/rag/cot_evaluator.cpp | 10 | 0 | 4 | 6 | 0 |
| src/rag/explainability_reason_builder.cpp | 10 | 1 | 3 | 6 | 0 |
| src/rag/relevance_evaluator.cpp | 10 | 0 | 3 | 7 | 0 |
| src/rag/nli_faithfulness_verifier.cpp | 9 | 1 | 0 | 8 | 0 |
| src/rag/completeness_evaluator.cpp | 8 | 0 | 1 | 7 | 0 |
| src/rag/http_metrics_client.cpp | 8 | 2 | 3 | 3 | 0 |
| src/rag/quality_control_factory.cpp | 8 | 0 | 8 | 0 | 0 |
| src/rag/coherence_evaluator.cpp | 6 | 0 | 2 | 4 | 0 |
| src/rag/flare_retrieval.cpp | 6 | 2 | 2 | 2 | 0 |
| src/rag/lora_enhanced_retriever.cpp | 6 | 2 | 1 | 3 | 0 |
| src/rag/rag_context_assembler.cpp | 6 | 0 | 1 | 5 | 0 |
| src/rag/bias_detector.cpp | 5 | 0 | 0 | 5 | 0 |
| src/rag/document_splitter.cpp | 5 | 0 | 3 | 2 | 0 |
| src/rag/hallucination_dashboard.cpp | 5 | 1 | 4 | 0 | 0 |
| src/rag/pairwise_comparator.cpp | 5 | 2 | 0 | 3 | 0 |
| src/rag/rag_ingestion_bridge.cpp | 5 | 2 | 3 | 0 | 0 |
| src/rag/rubric_evaluator.cpp | 5 | 0 | 1 | 4 | 0 |
| src/rag/tensor_rag_pipeline.cpp | 5 | 0 | 3 | 2 | 0 |
| src/rag/judge_config.cpp | 4 | 0 | 1 | 3 | 0 |
| src/rag/learning_metrics.cpp | 4 | 0 | 0 | 4 | 0 |
| src/rag/ontology_aware_retriever.cpp | 4 | 0 | 0 | 4 | 0 |
| src/rag/ab_testing_framework.cpp | 3 | 0 | 2 | 1 | 0 |
| src/rag/streaming_retriever.cpp | 3 | 0 | 0 | 3 | 0 |
| src/rag/targ_retrieval.cpp | 3 | 0 | 2 | 0 | 1 |
| src/rag/adaptive_retrieval.cpp | 2 | 0 | 0 | 2 | 0 |
| src/rag/judge_ensemble.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### src/rag/rag_judge.cpp
Total findings: 185

- Line 121: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->llm_judge_client = std::make_shared<LLMJudgeClient>(client_config);
- Line 124: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->nli_verifier = std::make_shared<NLIFaithfulnessVerifier>();
- Line 169: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationInput input;
  Confidence: band=very_high; score=0.99
- Line 170: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.query = query;
  Confidence: band=very_high; score=0.99
- Line 171: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.documents = documents;
  Confidence: band=very_high; score=0.99
- Line 172: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.generated_answer = generated_answer;
  Confidence: band=very_high; score=0.99
- Line 180: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto result = evaluate(input);
  Confidence: band=very_high; score=0.99
- Line 188: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationResult RAGJudge::evaluate(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.99
- Line 191: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: THEMIS_DEBUG("Evaluating RAG output for query: {}", input.query);
  Confidence: band=very_high; score=0.99
- Line 196: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto cache_key = impl_->computeCacheKey(input.query, input.generated_answer, input.tenant_id);
- Line 196: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto cache_key = impl_->computeCacheKey(input.query, input.generated_answer, input.tenant_id);
  Confidence: band=very_high; score=0.99
- Line 197: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = impl_->cache.find(cache_key);
- Line 198: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != impl_->cache.end()) {
- Line 210: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: !input.documents.empty()) {
  Confidence: band=very_high; score=0.99
- Line 212: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto scan_results = impl_->injection_detector->scanDocuments(input);
  Confidence: band=very_high; score=0.99
- Line 246: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: total_findings, input.query);
  Confidence: band=very_high; score=0.99
- Line 265: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: total_findings, input.query);
  Confidence: band=very_high; score=0.99
- Line 296: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "relevance", [&]() { return evaluateRelevance(input); }, 0.0);
  Confidence: band=very_high; score=0.99
- Line 303: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "faithfulness", [&]() { return evaluateFaithfulness(input); }, 0.0);
  Confidence: band=very_high; score=0.99
- Line 305: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "relevance", [&]() { return evaluateRelevance(input); }, 0.0);
  Confidence: band=very_high; score=0.99
- Line 307: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "completeness", [&]() { return evaluateCompleteness(input); }, 0.0);
  Confidence: band=very_high; score=0.99
- Line 309: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "coherence", [&]() { return evaluateCoherence(input); }, 0.0);
  Confidence: band=very_high; score=0.99
- Line 314: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "ethical_compliance", [&]() { return evaluateEthicalCompliance(input); }, 0.0);
  Confidence: band=very_high; score=0.99
- Line 330: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "faithfulness", [&]() { return evaluateFaithfulness(input); }, 0.0);
  Confidence: band=very_high; score=0.99
- Line 332: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "relevance", [&]() { return evaluateRelevance(input); }, 0.0);
  Confidence: band=very_high; score=0.99
- Line 334: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "completeness", [&]() { return evaluateCompleteness(input); }, 0.0);
  Confidence: band=very_high; score=0.99
- Line 336: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "coherence", [&]() { return evaluateCoherence(input); }, 0.0);
  Confidence: band=very_high; score=0.99
- Line 341: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "ethical_compliance", [&]() { return evaluateEthicalCompliance(input); }, 0.0);
  Confidence: band=very_high; score=0.99
- Line 350: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto claims = extractClaims(input.generated_answer);
  Confidence: band=very_high; score=0.99
- Line 480: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto cache_key = impl_->computeCacheKey(input.query, input.generated_answer, input.tenant_id);
- Line 480: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto cache_key = impl_->computeCacheKey(input.query, input.generated_answer, input.tenant_id);
  Confidence: band=very_high; score=0.99
- Line 481: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->cache[cache_key] = result;
- Line 490: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.generated_answer.size());
  Confidence: band=very_high; score=0.99
- Line 560: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationInput input;
  Confidence: band=very_high; score=0.99
- Line 561: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.query = test_case.query;
  Confidence: band=very_high; score=0.99
- Line 562: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.documents = test_case.documents;
  Confidence: band=very_high; score=0.99
- Line 563: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.generated_answer = test_case.generated_answer;
  Confidence: band=very_high; score=0.99
- Line 565: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto result = evaluate(input);
  Confidence: band=very_high; score=0.99
- Line 574: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const EvaluationInput& input
  Confidence: band=very_high; score=0.99
- Line 578: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return evaluateFaithfulness(input);
  Confidence: band=very_high; score=0.99
- Line 580: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return evaluateRelevance(input);
  Confidence: band=very_high; score=0.99
- Line 582: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return evaluateCompleteness(input);
  Confidence: band=very_high; score=0.99
- Line 584: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return evaluateCoherence(input);
  Confidence: band=very_high; score=0.99
- Line 586: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return evaluateEthicalCompliance(input);
  Confidence: band=very_high; score=0.99
- Line 588: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return evaluate(input).overall_score;
  Confidence: band=very_high; score=0.99
- Line 639: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: double RAGJudge::evaluateFaithfulness(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.99
- Line 643: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (input.documents.empty()) {
  Confidence: band=very_high; score=0.99
- Line 650: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (const auto& doc : input.documents) {
  Confidence: band=very_high; score=0.99
- Line 656: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.generated_answer,
  Confidence: band=very_high; score=0.99
- Line 658: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.query
  Confidence: band=very_high; score=0.99
- Line 667: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: double RAGJudge::evaluateRelevance(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.99
- Line 670: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (input.generated_answer.empty()) {
  Confidence: band=very_high; score=0.99
- Line 676: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.generated_answer,
  Confidence: band=very_high; score=0.99
- Line 677: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.query
  Confidence: band=very_high; score=0.99
- Line 686: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (const auto& doc : input.documents) {
  Confidence: band=very_high; score=0.99
- Line 707: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: double RAGJudge::evaluateCompleteness(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.99
- Line 712: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.generated_answer,
  Confidence: band=very_high; score=0.99
- Line 713: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.query
  Confidence: band=very_high; score=0.99
- Line 722: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: double RAGJudge::evaluateCoherence(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.99
- Line 726: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto result = impl_->coherence_eval->evaluate(input.generated_answer);
  Confidence: band=very_high; score=0.99
- Line 734: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: double RAGJudge::evaluateEthicalCompliance(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.99
- Line 738: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: double autonomy_score = evaluateAutonomyRespect(input);
  Confidence: band=very_high; score=0.99
- Line 739: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: double diversity_score = evaluateMoralDiversity(input);
  Confidence: band=very_high; score=0.99
- Line 740: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: double citation_score = evaluateCitationQuality(input);
  Confidence: band=very_high; score=0.99
- Line 754: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: double RAGJudge::evaluateAutonomyRespect(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.99
- Line 758: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (detectPatronizingLanguage(input.generated_answer)) {
  Confidence: band=very_high; score=0.99
- Line 764: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (!checkChoicePreservation(input.generated_answer)) {
  Confidence: band=very_high; score=0.99
- Line 770: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: int perspectives = countMoralPerspectives(input.generated_answer);
  Confidence: band=very_high; score=0.99
- Line 779: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: double RAGJudge::evaluateMoralDiversity(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.99
- Line 783: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: int perspectives = countMoralPerspectives(input.generated_answer);
  Confidence: band=very_high; score=0.99
- Line 792: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (detectBias(input.generated_answer)) {
  Confidence: band=very_high; score=0.99
- Line 800: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: double RAGJudge::evaluateCitationQuality(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.99
- Line 802: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: bool has_citations = hasEthicalCitations(input.generated_answer);
  Confidence: band=very_high; score=0.99
- Line 810: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto claims = extractClaims(input.generated_answer);
  Confidence: band=very_high; score=0.99
- Line 1222: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const EvaluationInput& input,
  Confidence: band=very_high; score=0.99
- Line 1225: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return impl_->template_manager.generatePrompt(dimension, input);
  Confidence: band=very_high; score=0.99
- Line 1257: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationResult JudgeEnsemble::evaluateWithEnsemble(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.99
- Line 1262: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: results.push_back(judge->evaluate(input));
  Confidence: band=very_high; score=0.99
- Line 51: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Enhanced LLM Judge Client (connects to InferenceEngineEnhanced)
  Confidence: band=very_high; score=0.9
- Line 169: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationInput input;
  Confidence: band=very_high; score=0.9
- Line 170: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.query = query;
  Confidence: band=very_high; score=0.9
- Line 171: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.documents = documents;
  Confidence: band=very_high; score=0.9
- Line 172: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.generated_answer = generated_answer;
  Confidence: band=very_high; score=0.9
- Line 172: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: input.generated_answer = generated_answer;
  Confidence: band=very_high; score=0.9
- Line 180: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto result = evaluate(input);
  Confidence: band=very_high; score=0.9
- Line 188: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationResult RAGJudge::evaluate(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.9
- Line 191: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: THEMIS_DEBUG("Evaluating RAG output for query: {}", input.query);
  Confidence: band=very_high; score=0.9
- Line 196: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto cache_key = impl_->computeCacheKey(input.query, input.generated_answer, input.tenant_id);
  Confidence: band=very_high; score=0.9
- Line 210: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: !input.documents.empty()) {
  Confidence: band=very_high; score=0.9
- Line 212: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto scan_results = impl_->injection_detector->scanDocuments(input);
  Confidence: band=very_high; score=0.9
- Line 246: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: total_findings, input.query);
  Confidence: band=very_high; score=0.9
- Line 265: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: total_findings, input.query);
  Confidence: band=very_high; score=0.9
- Line 296: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "relevance", [&]() { return evaluateRelevance(input); }, 0.0);
  Confidence: band=very_high; score=0.9
- Line 303: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "faithfulness", [&]() { return evaluateFaithfulness(input); }, 0.0);
  Confidence: band=very_high; score=0.9
- Line 305: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "relevance", [&]() { return evaluateRelevance(input); }, 0.0);
  Confidence: band=very_high; score=0.9
- Line 307: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "completeness", [&]() { return evaluateCompleteness(input); }, 0.0);
  Confidence: band=very_high; score=0.9
- Line 309: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "coherence", [&]() { return evaluateCoherence(input); }, 0.0);
  Confidence: band=very_high; score=0.9
- Line 314: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "ethical_compliance", [&]() { return evaluateEthicalCompliance(input); }, 0.0);
  Confidence: band=very_high; score=0.9
- Line 330: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "faithfulness", [&]() { return evaluateFaithfulness(input); }, 0.0);
  Confidence: band=very_high; score=0.9
- Line 332: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "relevance", [&]() { return evaluateRelevance(input); }, 0.0);
  Confidence: band=very_high; score=0.9
- Line 334: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "completeness", [&]() { return evaluateCompleteness(input); }, 0.0);
  Confidence: band=very_high; score=0.9
- Line 336: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "coherence", [&]() { return evaluateCoherence(input); }, 0.0);
  Confidence: band=very_high; score=0.9
- Line 341: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "ethical_compliance", [&]() { return evaluateEthicalCompliance(input); }, 0.0);
  Confidence: band=very_high; score=0.9
- Line 350: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto claims = extractClaims(input.generated_answer);
  Confidence: band=very_high; score=0.9
- Line 355: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: [&]() { return verifyClaimAgainstDocuments(claim, input.documents) ? 1.0 : 0.0; },
  Confidence: band=very_high; score=0.9
- Line 480: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto cache_key = impl_->computeCacheKey(input.query, input.generated_answer, input.tenant_id);
  Confidence: band=very_high; score=0.9
- Line 490: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.generated_answer.size());
  Confidence: band=very_high; score=0.9
- Line 560: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationInput input;
  Confidence: band=very_high; score=0.9
- Line 561: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.query = test_case.query;
  Confidence: band=very_high; score=0.9
- Line 562: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.documents = test_case.documents;
  Confidence: band=very_high; score=0.9
- Line 563: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.generated_answer = test_case.generated_answer;
  Confidence: band=very_high; score=0.9
- Line 563: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: input.generated_answer = test_case.generated_answer;
  Confidence: band=very_high; score=0.9
- Line 565: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto result = evaluate(input);
  Confidence: band=very_high; score=0.9
- Line 574: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const EvaluationInput& input
  Confidence: band=very_high; score=0.9
- Line 578: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return evaluateFaithfulness(input);
  Confidence: band=very_high; score=0.9
- Line 580: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return evaluateRelevance(input);
  Confidence: band=very_high; score=0.9
- Line 582: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return evaluateCompleteness(input);
  Confidence: band=very_high; score=0.9
- Line 584: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return evaluateCoherence(input);
  Confidence: band=very_high; score=0.9
- Line 586: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return evaluateEthicalCompliance(input);
  Confidence: band=very_high; score=0.9
- Line 588: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return evaluate(input).overall_score;
  Confidence: band=very_high; score=0.9
- Line 639: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: double RAGJudge::evaluateFaithfulness(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.9
- Line 643: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (input.documents.empty()) {
  Confidence: band=very_high; score=0.9
- Line 650: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: for (const auto& doc : input.documents) {
  Confidence: band=very_high; score=0.9
- Line 656: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.generated_answer,
  Confidence: band=very_high; score=0.9
- Line 658: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.query
  Confidence: band=very_high; score=0.9
- Line 667: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: double RAGJudge::evaluateRelevance(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.9
- Line 670: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (input.generated_answer.empty()) {
  Confidence: band=very_high; score=0.9
- Line 676: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.generated_answer,
  Confidence: band=very_high; score=0.9
- Line 677: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.query
  Confidence: band=very_high; score=0.9
- Line 686: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: for (const auto& doc : input.documents) {
  Confidence: band=very_high; score=0.9
- Line 707: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: double RAGJudge::evaluateCompleteness(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.9
- Line 712: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.generated_answer,
  Confidence: band=very_high; score=0.9
- Line 713: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.query
  Confidence: band=very_high; score=0.9
- Line 722: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: double RAGJudge::evaluateCoherence(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.9
- Line 726: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto result = impl_->coherence_eval->evaluate(input.generated_answer);
  Confidence: band=very_high; score=0.9
- Line 726: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto result = impl_->coherence_eval->evaluate(input.generated_answer);
  Confidence: band=very_high; score=0.9
- Line 734: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: double RAGJudge::evaluateEthicalCompliance(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.9
- Line 738: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: double autonomy_score = evaluateAutonomyRespect(input);
  Confidence: band=very_high; score=0.9
- Line 739: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: double diversity_score = evaluateMoralDiversity(input);
  Confidence: band=very_high; score=0.9
- Line 740: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: double citation_score = evaluateCitationQuality(input);
  Confidence: band=very_high; score=0.9
- Line 754: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: double RAGJudge::evaluateAutonomyRespect(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.9
- Line 758: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (detectPatronizingLanguage(input.generated_answer)) {
  Confidence: band=very_high; score=0.9
- Line 764: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (!checkChoicePreservation(input.generated_answer)) {
  Confidence: band=very_high; score=0.9
- Line 770: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: int perspectives = countMoralPerspectives(input.generated_answer);
  Confidence: band=very_high; score=0.9
- Line 779: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: double RAGJudge::evaluateMoralDiversity(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.9
- Line 783: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: int perspectives = countMoralPerspectives(input.generated_answer);
  Confidence: band=very_high; score=0.9
- Line 792: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (detectBias(input.generated_answer)) {
  Confidence: band=very_high; score=0.9
- Line 800: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: double RAGJudge::evaluateCitationQuality(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.9
- Line 802: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: bool has_citations = hasEthicalCitations(input.generated_answer);
  Confidence: band=very_high; score=0.9
- Line 810: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto claims = extractClaims(input.generated_answer);
  Confidence: band=very_high; score=0.9
- Line 957: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: "[",  // Citation markers like [1], [UN Declaration]
- Line 1054: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (trimmed.find(phrase) != std::string::npos) {
- Line 1055: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (trimmed.find(phrase) != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 1209: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (doc.content.find(claim) != std::string::npos) {
- Line 1222: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const EvaluationInput& input,
  Confidence: band=very_high; score=0.9
- Line 1225: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return impl_->template_manager.generatePrompt(dimension, input);
  Confidence: band=very_high; score=0.9
- Line 1257: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationResult JudgeEnsemble::evaluateWithEnsemble(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.9
- Line 1262: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: results.push_back(judge->evaluate(input));
  Confidence: band=very_high; score=0.9
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 285: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 357: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.verified_claims.push_back(claim);
  Confidence: band=high; score=0.74
- Line 358: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.verified_claims.push_back(claim);
- Line 361: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.unverified_claims.push_back(claim);
- Line 372: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 440: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: dim_scores.push_back(result.faithfulness_score);
- Line 441: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: dim_scores.push_back(result.completeness_score);
- Line 442: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: dim_scores.push_back(result.coherence_score);
- Line 444: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: dim_scores.push_back(result.ethical_compliance_score);
- Line 446: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: dim_scores.push_back(result.relevance_score);  // always evaluated
- Line 487: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: impl_->eval_history.push_back(result);
- Line 565: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(result);
  Confidence: band=high; score=0.74
- Line 566: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(result);
- Line 650: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: doc_pairs.emplace_back(doc.id, doc.content);
  Confidence: band=high; score=0.74
- Line 1026: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: claims.push_back(std::move(text));
  Confidence: band=high; score=0.74
- Line 1027: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: claims.push_back(std::move(text));
- Line 1062: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: claims.push_back(trimmed);
  Confidence: band=high; score=0.74
- Line 1062: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: claims.push_back(trimmed);
  Confidence: band=high; score=0.74
- Line 1063: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: claims.push_back(trimmed);
- Line 1092: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(word);
- Line 1105: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> set2(terms2.begin(), terms2.end());
  Confidence: band=medium; score=0.66
- Line 1261: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(judge->evaluate(input));
  Confidence: band=high; score=0.74
- Line 1262: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(judge->evaluate(input));
- Line 1403: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: judges.push_back(std::make_shared<RAGJudge>());
  Confidence: band=high; score=0.74
- Line 1404: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: judges.push_back(std::make_shared<RAGJudge>());

### src/rag/batch_evaluator.cpp
Total findings: 137

- Line 85: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: bool metadataHasPromptInjectionScenario(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.99
- Line 86: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto it = input.metadata.find("attack_type");
  Confidence: band=very_high; score=0.99
- Line 87: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (it != input.metadata.end() && iequals(it->second, "prompt_injection")) {
  Confidence: band=very_high; score=0.99
- Line 90: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: it = input.metadata.find("scenario");
  Confidence: band=very_high; score=0.99
- Line 91: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (it != input.metadata.end() && iequals(it->second, "prompt_injection")) {
  Confidence: band=very_high; score=0.99
- Line 97: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: bool hasDecisionTraceability(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.99
- Line 99: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.metadata.find("model_version") != input.metadata.end();
  Confidence: band=very_high; score=0.99
- Line 101: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.metadata.find("guardrail_decision") != input.metadata.end();
  Confidence: band=very_high; score=0.99
- Line 103: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.metadata.find("context_id") != input.metadata.end() ||
  Confidence: band=very_high; score=0.99
- Line 104: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.metadata.find("retrieval_context_id") != input.metadata.end();
  Confidence: band=very_high; score=0.99
- Line 108: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: double extractLatencyMs(const EvaluationInput& input, const EvaluationResult& result) {
  Confidence: band=very_high; score=0.99
- Line 109: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto it = input.metadata.find("latency_ms");
  Confidence: band=very_high; score=0.99
- Line 110: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (it != input.metadata.end()) {
  Confidence: band=very_high; score=0.99
- Line 119: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: double extractCost(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.99
- Line 127: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto it = input.metadata.find(key);
  Confidence: band=very_high; score=0.99
- Line 128: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (it == input.metadata.end()) {
  Confidence: band=very_high; score=0.99
- Line 215: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: queue_cv_.wait(lock, [this] {
- Line 231: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto result = processEvaluation(item.input);
  Confidence: band=very_high; score=0.99
- Line 255: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationResult BatchEvaluator::processEvaluation(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.99
- Line 256: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return judge_->evaluate(input);
  Confidence: band=very_high; score=0.99
- Line 265: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::vector<EvaluationInput> inputs;
  Confidence: band=very_high; score=0.99
- Line 266: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: inputs.reserve(test_cases.size());
  Confidence: band=very_high; score=0.99
- Line 268: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationInput in;
  Confidence: band=very_high; score=0.99
- Line 272: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: inputs.push_back(std::move(in));
  Confidence: band=very_high; score=0.99
- Line 274: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return evaluateBatch(inputs);
  Confidence: band=very_high; score=0.99
- Line 278: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const std::vector<EvaluationInput>& inputs) {
  Confidence: band=very_high; score=0.99
- Line 282: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: results.reserve(inputs.size());
  Confidence: band=very_high; score=0.99
- Line 287: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (const auto& input : inputs) {
  Confidence: band=very_high; score=0.99
- Line 289: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: results.push_back(processEvaluation(input));
  Confidence: band=very_high; score=0.99
- Line 294: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: progress.total_items     = inputs.size();
  Confidence: band=very_high; score=0.99
- Line 299: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: static_cast<double>(inputs.size());
  Confidence: band=very_high; score=0.99
- Line 318: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: out.progress.total_items = inputs.size();
  Confidence: band=very_high; score=0.99
- Line 320: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (inputs.empty() || results.empty()) {
  Confidence: band=very_high; score=0.99
- Line 338: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (size_t i = 0; i < results.size() && i < inputs.size(); ++i) {
  Confidence: band=very_high; score=0.99
- Line 339: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const auto& input = inputs[i];
  Confidence: band=very_high; score=0.99
- Line 358: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (metadataHasPromptInjectionScenario(input)) {
  Confidence: band=very_high; score=0.99
- Line 379: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (!input.documents.empty()) {
  Confidence: band=very_high; score=0.99
- Line 380: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const auto scan_results = inline_detector.scanDocuments(input);
  Confidence: band=very_high; score=0.99
- Line 396: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto it = input.metadata.find("attack_succeeded");
  Confidence: band=very_high; score=0.99
- Line 397: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (it != input.metadata.end()) {
  Confidence: band=very_high; score=0.99
- Line 416: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (hasDecisionTraceability(input)) {
  Confidence: band=very_high; score=0.99
- Line 420: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: latencies_ms.push_back(extractLatencyMs(input, result));
  Confidence: band=very_high; score=0.99
- Line 421: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: total_cost += extractCost(input);
  Confidence: band=very_high; score=0.99
- Line 500: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const EvaluationInput& input) {
  Confidence: band=very_high; score=0.99
- Line 510: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: item.input    = input;
  Confidence: band=very_high; score=0.99
- Line 521: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const std::vector<EvaluationInput>& inputs) {
  Confidence: band=very_high; score=0.99
- Line 523: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: handles.reserve(inputs.size());
  Confidence: band=very_high; score=0.99
- Line 524: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (const auto& input : inputs) {
  Confidence: band=very_high; score=0.99
- Line 525: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: handles.push_back(evaluateAsync(input));
  Confidence: band=very_high; score=0.99
- Line 535: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const EvaluationInput& input,
  Confidence: band=very_high; score=0.99
- Line 539: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: item.input    = input;
  Confidence: band=very_high; score=0.99
- Line 85: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: bool metadataHasPromptInjectionScenario(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.9
- Line 86: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto it = input.metadata.find("attack_type");
  Confidence: band=very_high; score=0.9
- Line 87: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (it != input.metadata.end() && iequals(it->second, "prompt_injection")) {
  Confidence: band=very_high; score=0.9
- Line 90: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: it = input.metadata.find("scenario");
  Confidence: band=very_high; score=0.9
- Line 91: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (it != input.metadata.end() && iequals(it->second, "prompt_injection")) {
  Confidence: band=very_high; score=0.9
- Line 97: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: bool hasDecisionTraceability(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.9
- Line 99: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.metadata.find("model_version") != input.metadata.end();
  Confidence: band=very_high; score=0.9
- Line 101: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.metadata.find("guardrail_decision") != input.metadata.end();
  Confidence: band=very_high; score=0.9
- Line 103: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.metadata.find("context_id") != input.metadata.end() ||
  Confidence: band=very_high; score=0.9
- Line 104: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.metadata.find("retrieval_context_id") != input.metadata.end();
  Confidence: band=very_high; score=0.9
- Line 108: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: double extractLatencyMs(const EvaluationInput& input, const EvaluationResult& result) {
  Confidence: band=very_high; score=0.9
- Line 109: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto it = input.metadata.find("latency_ms");
  Confidence: band=very_high; score=0.9
- Line 110: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (it != input.metadata.end()) {
  Confidence: band=very_high; score=0.9
- Line 119: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: double extractCost(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.9
- Line 127: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto it = input.metadata.find(key);
  Confidence: band=very_high; score=0.9
- Line 128: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (it == input.metadata.end()) {
  Confidence: band=very_high; score=0.9
- Line 148: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: return future_.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
- Line 162: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("AsyncEvaluationHandle: evaluation was cancelled");
- Line 198: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < config_.num_workers; ++i) {
  Confidence: band=very_high; score=0.9
- Line 213: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(queue_mutex_);
- Line 231: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto result = processEvaluation(item.input);
  Confidence: band=very_high; score=0.9
- Line 255: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationResult BatchEvaluator::processEvaluation(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.9
- Line 256: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return judge_->evaluate(input);
  Confidence: band=very_high; score=0.9
- Line 265: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::vector<EvaluationInput> inputs;
  Confidence: band=very_high; score=0.9
- Line 265: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::vector<EvaluationInput> inputs;
  Confidence: band=very_high; score=0.9
- Line 266: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: inputs.reserve(test_cases.size());
  Confidence: band=very_high; score=0.9
- Line 266: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: inputs.reserve(test_cases.size());
  Confidence: band=very_high; score=0.9
- Line 268: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationInput in;
  Confidence: band=very_high; score=0.9
- Line 271: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: in.generated_answer = tc.generated_answer;
  Confidence: band=very_high; score=0.9
- Line 272: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: inputs.push_back(std::move(in));
  Confidence: band=very_high; score=0.9
- Line 272: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: inputs.push_back(std::move(in));
  Confidence: band=very_high; score=0.9
- Line 274: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: return evaluateBatch(inputs);
  Confidence: band=very_high; score=0.9
- Line 274: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return evaluateBatch(inputs);
  Confidence: band=very_high; score=0.9
- Line 278: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: const std::vector<EvaluationInput>& inputs) {
  Confidence: band=very_high; score=0.9
- Line 278: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const std::vector<EvaluationInput>& inputs) {
  Confidence: band=very_high; score=0.9
- Line 282: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: results.reserve(inputs.size());
  Confidence: band=very_high; score=0.9
- Line 282: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: results.reserve(inputs.size());
  Confidence: band=very_high; score=0.9
- Line 287: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (const auto& input : inputs) {
  Confidence: band=very_high; score=0.9
- Line 287: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: for (const auto& input : inputs) {
  Confidence: band=very_high; score=0.9
- Line 289: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: results.push_back(processEvaluation(input));
  Confidence: band=very_high; score=0.9
- Line 294: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: progress.total_items     = inputs.size();
  Confidence: band=very_high; score=0.9
- Line 294: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: progress.total_items     = inputs.size();
  Confidence: band=very_high; score=0.9
- Line 299: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: static_cast<double>(inputs.size());
  Confidence: band=very_high; score=0.9
- Line 299: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: static_cast<double>(inputs.size());
  Confidence: band=very_high; score=0.9
- Line 318: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: out.progress.total_items = inputs.size();
  Confidence: band=very_high; score=0.9
- Line 318: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: out.progress.total_items = inputs.size();
  Confidence: band=very_high; score=0.9
- Line 320: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: if (inputs.empty() || results.empty()) {
  Confidence: band=very_high; score=0.9
- Line 320: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (inputs.empty() || results.empty()) {
  Confidence: band=very_high; score=0.9
- Line 338: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (size_t i = 0; i < results.size() && i < inputs.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 338: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: for (size_t i = 0; i < results.size() && i < inputs.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 339: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: const auto& input = inputs[i];
  Confidence: band=very_high; score=0.9
- Line 339: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const auto& input = inputs[i];
  Confidence: band=very_high; score=0.9
- Line 358: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (metadataHasPromptInjectionScenario(input)) {
  Confidence: band=very_high; score=0.9
- Line 379: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (!input.documents.empty()) {
  Confidence: band=very_high; score=0.9
- Line 380: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const auto scan_results = inline_detector.scanDocuments(input);
  Confidence: band=very_high; score=0.9
- Line 396: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto it = input.metadata.find("attack_succeeded");
  Confidence: band=very_high; score=0.9
- Line 397: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (it != input.metadata.end()) {
  Confidence: band=very_high; score=0.9
- Line 416: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (hasDecisionTraceability(input)) {
  Confidence: band=very_high; score=0.9
- Line 420: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: latencies_ms.push_back(extractLatencyMs(input, result));
  Confidence: band=very_high; score=0.9
- Line 421: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: total_cost += extractCost(input);
  Confidence: band=very_high; score=0.9
- Line 500: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const EvaluationInput& input) {
  Confidence: band=very_high; score=0.9
- Line 502: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->cancelled_.store(false);
- Line 505: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->future_ = promise.get_future();
- Line 510: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: item.input    = input;
  Confidence: band=very_high; score=0.9
- Line 521: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: const std::vector<EvaluationInput>& inputs) {
  Confidence: band=very_high; score=0.9
- Line 521: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const std::vector<EvaluationInput>& inputs) {
  Confidence: band=very_high; score=0.9
- Line 523: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: handles.reserve(inputs.size());
  Confidence: band=very_high; score=0.9
- Line 523: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: handles.reserve(inputs.size());
  Confidence: band=very_high; score=0.9
- Line 524: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (const auto& input : inputs) {
  Confidence: band=very_high; score=0.9
- Line 524: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: for (const auto& input : inputs) {
  Confidence: band=very_high; score=0.9
- Line 524: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& input : inputs) {
  Confidence: band=very_high; score=0.9
- Line 525: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: handles.push_back(evaluateAsync(input));
  Confidence: band=very_high; score=0.9
- Line 535: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const EvaluationInput& input,
  Confidence: band=very_high; score=0.9
- Line 539: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: item.input    = input;
  Confidence: band=very_high; score=0.9
- Line 554: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(queue_mutex_);
- Line 561: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(5));
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 67: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 198: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: workers_.emplace_back(&BatchEvaluator::workerThread, this);
  Confidence: band=high; score=0.74
- Line 245: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 272: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: inputs.push_back(std::move(in));
- Line 288: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(processEvaluation(input));
  Confidence: band=high; score=0.74
- Line 289: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(processEvaluation(input));
- Line 524: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: handles.push_back(evaluateAsync(input));
  Confidence: band=high; score=0.74
- Line 525: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: handles.push_back(evaluateAsync(input));
- Line 588: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: workers_.emplace_back(&BatchEvaluator::workerThread, this);
  Confidence: band=high; score=0.74

### src/rag/adversarial_tester.cpp
Total findings: 97

- Line 62: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: /// Duplicate tokens within each input are deduplicated before comparison,
  Confidence: band=very_high; score=0.99
- Line 385: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationInput orig_input;
  Confidence: band=very_high; score=0.99
- Line 386: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: orig_input.query            = bq.query;
  Confidence: band=very_high; score=0.99
- Line 387: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: orig_input.documents        = docs;
  Confidence: band=very_high; score=0.99
- Line 388: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: orig_input.generated_answer = bq.expected_answer.empty()
  Confidence: band=very_high; score=0.99
- Line 392: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationResult orig_result = judge.evaluate(orig_input);
  Confidence: band=very_high; score=0.99
- Line 400: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationInput perturbed_input;
  Confidence: band=very_high; score=0.99
- Line 401: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: perturbed_input.query            = variant;
  Confidence: band=very_high; score=0.99
- Line 402: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: perturbed_input.documents        = docs;
  Confidence: band=very_high; score=0.99
- Line 403: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: perturbed_input.generated_answer = orig_input.generated_answer;
  Confidence: band=very_high; score=0.99
- Line 405: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationResult pert_result = judge.evaluate(perturbed_input);
  Confidence: band=very_high; score=0.99
- Line 438: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationInput clean_input;
  Confidence: band=very_high; score=0.99
- Line 439: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: clean_input.query            = bq.query;
  Confidence: band=very_high; score=0.99
- Line 440: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: clean_input.documents        = impl_->base_documents;
  Confidence: band=very_high; score=0.99
- Line 441: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: clean_input.generated_answer = bq.expected_answer.empty()
  Confidence: band=very_high; score=0.99
- Line 445: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationResult clean_result = judge.evaluate(clean_input);
  Confidence: band=very_high; score=0.99
- Line 449: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationInput poison_input;
  Confidence: band=very_high; score=0.99
- Line 450: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: poison_input.query            = bq.query;
  Confidence: band=very_high; score=0.99
- Line 451: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: poison_input.documents        = poisoned_docs;
  Confidence: band=very_high; score=0.99
- Line 452: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: poison_input.generated_answer = clean_input.generated_answer;
  Confidence: band=very_high; score=0.99
- Line 454: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationResult poison_result = judge.evaluate(poison_input);
  Confidence: band=very_high; score=0.99
- Line 500: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationInput inj_input;
  Confidence: band=very_high; score=0.99
- Line 501: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: inj_input.query            = bq.query;
  Confidence: band=very_high; score=0.99
- Line 502: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: inj_input.documents        = injected_docs;
  Confidence: band=very_high; score=0.99
- Line 503: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: inj_input.generated_answer = bq.expected_answer.empty()
  Confidence: band=very_high; score=0.99
- Line 507: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: judge.evaluate(inj_input);
  Confidence: band=very_high; score=0.99
- Line 528: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationInput base_input;
  Confidence: band=very_high; score=0.99
- Line 529: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: base_input.query            = bq.query;
  Confidence: band=very_high; score=0.99
- Line 530: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: base_input.documents        = impl_->base_documents;
  Confidence: band=very_high; score=0.99
- Line 531: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: base_input.generated_answer = bq.expected_answer.empty()
  Confidence: band=very_high; score=0.99
- Line 535: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationResult base_result = judge.evaluate(base_input);
  Confidence: band=very_high; score=0.99
- Line 542: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationInput overflow_input;
  Confidence: band=very_high; score=0.99
- Line 543: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: overflow_input.query            = bq.query;
  Confidence: band=very_high; score=0.99
- Line 544: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: overflow_input.documents        = padded_docs;
  Confidence: band=very_high; score=0.99
- Line 545: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: overflow_input.generated_answer = base_input.generated_answer;
  Confidence: band=very_high; score=0.99
- Line 547: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationResult overflow_result = judge.evaluate(overflow_input);
  Confidence: band=very_high; score=0.99
- Line 567: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationInput orig_input;
  Confidence: band=very_high; score=0.99
- Line 568: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: orig_input.query            = bq.query;
  Confidence: band=very_high; score=0.99
- Line 569: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: orig_input.documents        = docs;
  Confidence: band=very_high; score=0.99
- Line 570: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: orig_input.generated_answer = bq.expected_answer.empty()
  Confidence: band=very_high; score=0.99
- Line 574: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationResult orig_result = judge.evaluate(orig_input);
  Confidence: band=very_high; score=0.99
- Line 582: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationInput syco_input;
  Confidence: band=very_high; score=0.99
- Line 583: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: syco_input.query            = syco_query;
  Confidence: band=very_high; score=0.99
- Line 584: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: syco_input.documents        = docs;
  Confidence: band=very_high; score=0.99
- Line 585: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: syco_input.generated_answer = orig_input.generated_answer;
  Confidence: band=very_high; score=0.99
- Line 587: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationResult syco_result = judge.evaluate(syco_input);
  Confidence: band=very_high; score=0.99
- Line 62: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: /// Duplicate tokens within each input are deduplicated before comparison,
  Confidence: band=very_high; score=0.9
- Line 385: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationInput orig_input;
  Confidence: band=very_high; score=0.9
- Line 386: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: orig_input.query            = bq.query;
  Confidence: band=very_high; score=0.9
- Line 387: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: orig_input.documents        = docs;
  Confidence: band=very_high; score=0.9
- Line 388: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: orig_input.generated_answer = bq.expected_answer.empty()
  Confidence: band=very_high; score=0.9
- Line 392: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationResult orig_result = judge.evaluate(orig_input);
  Confidence: band=very_high; score=0.9
- Line 400: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationInput perturbed_input;
  Confidence: band=very_high; score=0.9
- Line 401: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: perturbed_input.query            = variant;
  Confidence: band=very_high; score=0.9
- Line 402: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: perturbed_input.documents        = docs;
  Confidence: band=very_high; score=0.9
- Line 403: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: perturbed_input.generated_answer = orig_input.generated_answer;
  Confidence: band=very_high; score=0.9
- Line 403: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: perturbed_input.generated_answer = orig_input.generated_answer;
  Confidence: band=very_high; score=0.9
- Line 405: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationResult pert_result = judge.evaluate(perturbed_input);
  Confidence: band=very_high; score=0.9
- Line 452: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: poison_input.generated_answer = clean_input.generated_answer;
  Confidence: band=very_high; score=0.9
- Line 500: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationInput inj_input;
  Confidence: band=very_high; score=0.9
- Line 501: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: inj_input.query            = bq.query;
  Confidence: band=very_high; score=0.9
- Line 502: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: inj_input.documents        = injected_docs;
  Confidence: band=very_high; score=0.9
- Line 503: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: inj_input.generated_answer = bq.expected_answer.empty()
  Confidence: band=very_high; score=0.9
- Line 507: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: judge.evaluate(inj_input);
  Confidence: band=very_high; score=0.9
- Line 528: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationInput base_input;
  Confidence: band=very_high; score=0.9
- Line 529: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: base_input.query            = bq.query;
  Confidence: band=very_high; score=0.9
- Line 530: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: base_input.documents        = impl_->base_documents;
  Confidence: band=very_high; score=0.9
- Line 531: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: base_input.generated_answer = bq.expected_answer.empty()
  Confidence: band=very_high; score=0.9
- Line 535: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationResult base_result = judge.evaluate(base_input);
  Confidence: band=very_high; score=0.9
- Line 542: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationInput overflow_input;
  Confidence: band=very_high; score=0.9
- Line 543: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: overflow_input.query            = bq.query;
  Confidence: band=very_high; score=0.9
- Line 544: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: overflow_input.documents        = padded_docs;
  Confidence: band=very_high; score=0.9
- Line 545: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: overflow_input.generated_answer = base_input.generated_answer;
  Confidence: band=very_high; score=0.9
- Line 545: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: overflow_input.generated_answer = base_input.generated_answer;
  Confidence: band=very_high; score=0.9
- Line 547: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationResult overflow_result = judge.evaluate(overflow_input);
  Confidence: band=very_high; score=0.9
- Line 567: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationInput orig_input;
  Confidence: band=very_high; score=0.9
- Line 568: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: orig_input.query            = bq.query;
  Confidence: band=very_high; score=0.9
- Line 569: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: orig_input.documents        = docs;
  Confidence: band=very_high; score=0.9
- Line 570: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: orig_input.generated_answer = bq.expected_answer.empty()
  Confidence: band=very_high; score=0.9
- Line 574: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationResult orig_result = judge.evaluate(orig_input);
  Confidence: band=very_high; score=0.9
- Line 582: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationInput syco_input;
  Confidence: band=very_high; score=0.9
- Line 583: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: syco_input.query            = syco_query;
  Confidence: band=very_high; score=0.9
- Line 584: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: syco_input.documents        = docs;
  Confidence: band=very_high; score=0.9
- Line 585: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: syco_input.generated_answer = orig_input.generated_answer;
  Confidence: band=very_high; score=0.9
- Line 585: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: syco_input.generated_answer = orig_input.generated_answer;
  Confidence: band=very_high; score=0.9
- Line 587: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationResult syco_result = judge.evaluate(syco_input);
  Confidence: band=very_high; score=0.9
- Line 50: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(word);
  Confidence: band=high; score=0.74
- Line 69: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> set_a(a.begin(), a.end());
  Confidence: band=medium; score=0.66
- Line 70: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> set_b(b.begin(), b.end());
  Confidence: band=medium; score=0.66
- Line 131: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += " (variant " + std::to_string(variant_index + 1) + ")";
  Confidence: band=high; score=0.74
- Line 232: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: docs.push_back(d);
  Confidence: band=high; score=0.74
- Line 321: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(perturbed);
  Confidence: band=high; score=0.74
- Line 417: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.failing_examples.push_back(ex);
  Confidence: band=high; score=0.74
- Line 472: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.poisoning_results.push_back(pr);
  Confidence: band=high; score=0.74
- Line 497: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: injected_docs.push_back(injected_doc);
  Confidence: band=high; score=0.74
- Line 497: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: injected_docs.push_back(injected_doc);
  Confidence: band=high; score=0.74
- Line 593: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.vulnerabilities.push_back(
  Confidence: band=high; score=0.74

### src/rag/evaluation_report_exporter.cpp
Total findings: 79

- Line 113: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const EvaluationInput&  inp = report.input;
  Confidence: band=very_high; score=0.99
- Line 228: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const EvaluationInput&  inp = report.input;
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    os << "<!DOCTYPE html>\\n<html lang=\\"en\\">\\n<head>\\n"', '       << "<meta charset=\\"UTF-8\\">\\n"', '       << "<meta name=\\"viewport\\" content=\\"width=device-width,initial-scale=1\\">\\n"', '       << "<title>RAG Evaluation Report";', '    if (!report.report_id.empty())']
  Confidence: band=high; score=0.78
- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #3353 [rag] Per-query evaluation report export (JSON/HTML) (2026-03-12T07:06:51Z)
- Line 48: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x",
  Confidence: band=very_high; score=0.9
- Line 48: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x",
- Line 113: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const EvaluationInput&  inp = report.input;
  Confidence: band=very_high; score=0.9
- Line 228: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const EvaluationInput&  inp = report.input;
  Confidence: band=very_high; score=0.9
- Line 36: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '"':  out += "\\\""; break;
  Confidence: band=high; score=0.74
- Line 37: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"':  out += "\\\""; break;
- Line 38: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\\': out += "\\\\"; break;
- Line 39: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\b': out += "\\b";  break;
- Line 40: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\f': out += "\\f";  break;
- Line 41: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\n': out += "\\n";  break;
- Line 42: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\r': out += "\\r";  break;
- Line 43: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\t': out += "\\t";  break;
- Line 48: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x",
- Line 64: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '&':  out += "&amp;";  break;
  Confidence: band=high; score=0.74
- Line 65: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '&':  out += "&amp;";  break;
- Line 66: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '<':  out += "<";   break;
- Line 67: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '>':  out += ">";   break;
- Line 68: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"':  out += "&quot;"; break;
- Line 69: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\'': out += "&#39;";  break;
- Line 97: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<span class=\"score-label\">" << escapeHTML(label) << "</span>"
- Line 97: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<span class=\"score-label\">" << escapeHTML(label) << "</span>"
- Line 100: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "%;background:" << colour << ";\"></div>"
- Line 104: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "</div>\n";
- Line 243: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "</title>\n"
- Line 264: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "</style>\n"
- Line 265: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "</head>\n<body>\n";
- Line 270: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << " <span class=\"meta\">(" << escapeHTML(report.report_id) << ")</span>";
- Line 270: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << " <span class=\"meta\">(" << escapeHTML(report.report_id) << ")</span>";
- Line 271: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "</h1>\n";
- Line 277: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << std::fixed << std::setprecision(3) << res.confidence << "</strong></p>\n";
- Line 281: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << pass_label << "</span></p>\n";
- Line 285: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<h2>Query</h2>\n"
- Line 286: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<p>" << escapeHTML(inp.query) << "</p>\n"
- Line 287: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<h2>Generated Answer</h2>\n"
- Line 288: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<p>" << escapeHTML(inp.generated_answer) << "</p>\n"
- Line 289: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "</div>\n";
- Line 292: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<div class=\"card\">\n<h2>Dimension Scores</h2>\n";
- Line 292: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<div class=\"card\">\n<h2>Dimension Scores</h2>\n";
- Line 300: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "</div>\n";
- Line 303: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<div class=\"card\">\n<h2>Claims</h2>\n";
- Line 303: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<div class=\"card\">\n<h2>Claims</h2>\n";
- Line 306: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<p><strong>✓ Verified Claims</strong></p>\n<ul class=\"claims\">\n";
- Line 306: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<p><strong>✓ Verified Claims</strong></p>\n<ul class=\"claims\">\n";
- Line 308: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<li class=\"verified\">" << escapeHTML(c) << "</li>\n";
- Line 308: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<li class=\"verified\">" << escapeHTML(c) << "</li>\n";
- Line 309: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "</ul>\n";
- Line 313: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<p><strong>✗ Unverified Claims</strong></p>\n<ul class=\"claims\">\n";
- Line 313: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<p><strong>✗ Unverified Claims</strong></p>\n<ul class=\"claims\">\n";
- Line 315: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<li class=\"unverified\">" << escapeHTML(c) << "</li>\n";
- Line 315: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<li class=\"unverified\">" << escapeHTML(c) << "</li>\n";
- Line 316: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "</ul>\n";
- Line 320: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<p class=\"meta\">No claims extracted.</p>\n";
- Line 320: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<p class=\"meta\">No claims extracted.</p>\n";
- Line 322: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "</div>\n";
- Line 325: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<div class=\"card\">\n<h2>Ethical Compliance</h2>\n"
- Line 325: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<div class=\"card\">\n<h2>Ethical Compliance</h2>\n"
- Line 327: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << (res.respects_human_autonomy ? "Yes" : "No") << "</strong></p>\n"
- Line 329: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << (res.shows_moral_diversity ? "Yes" : "No") << "</strong></p>\n"
- Line 331: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << (res.has_ethical_citations ? "Yes" : "No") << "</strong></p>\n";
- Line 334: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<p><strong>Ethical Violations:</strong></p>\n<ul>\n";
- Line 336: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<li style=\"color:#c62828\">" << escapeHTML(v) << "</li>\n";
- Line 336: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<li style=\"color:#c62828\">" << escapeHTML(v) << "</li>\n";
- Line 337: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "</ul>\n";
- Line 339: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "</div>\n";
- Line 343: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<div class=\"card\">\n<h2>Suggested Improvements</h2>\n<ul>\n";
- Line 343: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<div class=\"card\">\n<h2>Suggested Improvements</h2>\n<ul>\n";
- Line 345: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<li>" << escapeHTML(imp) << "</li>\n";
- Line 346: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "</ul>\n</div>\n";
- Line 351: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<div class=\"card\">\n<h2>Explanation</h2>\n"
- Line 351: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<div class=\"card\">\n<h2>Explanation</h2>\n"
- Line 353: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "</div>\n</div>\n";
- Line 359: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << inp.documents.size() << ")</h2>\n";
- Line 367: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "</div>\n";
- Line 369: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "</div>\n";
- Line 372: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "</body>\n</html>\n";

### src/rag/dpr_vectorizer.cpp
Total findings: 75

- Line 140: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::vector<int64_t> input_ids(tokens.begin(), tokens.end());
  Confidence: band=very_high; score=0.99
- Line 153: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: Ort::Value input_ids_tensor = Ort::Value::CreateTensor<int64_t>(
  Confidence: band=very_high; score=0.99
- Line 154: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: mem_info, input_ids.data(), input_ids.size(), shape.data(), shape.size());
  Confidence: band=very_high; score=0.99
- Line 159: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::vector<Ort::AllocatedStringPtr> input_name_holders;
  Confidence: band=very_high; score=0.99
- Line 160: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::vector<const char*> input_names;
  Confidence: band=very_high; score=0.99
- Line 161: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::vector<Ort::Value> input_tensors;
  Confidence: band=very_high; score=0.99
- Line 163: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const size_t input_count = static_cast<size_t>(session->GetInputCount());
- Line 163: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const size_t input_count = static_cast<size_t>(session->GetInputCount());
  Confidence: band=very_high; score=0.99
- Line 164: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_name_holders.reserve(input_count);
  Confidence: band=very_high; score=0.99
- Line 165: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_names.reserve(input_count);
  Confidence: band=very_high; score=0.99
- Line 166: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_tensors.reserve(input_count);
  Confidence: band=very_high; score=0.99
- Line 168: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_name_holders.push_back(session->GetInputNameAllocated(0, allocator));
  Confidence: band=very_high; score=0.99
- Line 169: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_names.push_back(input_name_holders.back().get());
  Confidence: band=very_high; score=0.99
- Line 170: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_tensors.push_back(std::move(input_ids_tensor));
  Confidence: band=very_high; score=0.99
- Line 171: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (input_count > 1) {
  Confidence: band=very_high; score=0.99
- Line 172: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_name_holders.push_back(session->GetInputNameAllocated(1, allocator));
  Confidence: band=very_high; score=0.99
- Line 173: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_names.push_back(input_name_holders.back().get());
  Confidence: band=very_high; score=0.99
- Line 174: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_tensors.push_back(std::move(attention_mask_tensor));
  Confidence: band=very_high; score=0.99
- Line 182: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_names.data(),
  Confidence: band=very_high; score=0.99
- Line 183: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_tensors.data(),
  Confidence: band=very_high; score=0.99
- Line 184: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_tensors.size(),
  Confidence: band=very_high; score=0.99
- Line 281: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto query_model = impl_->model_loader->loadModel(config_.query_model_path);
- Line 281: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto query_model = impl_->model_loader->loadModel(config_.query_model_path);
  Confidence: band=very_high; score=0.99
- Line 292: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto passage_model = impl_->model_loader->loadModel(config_.passage_model_path);
- Line 292: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto passage_model = impl_->model_loader->loadModel(config_.passage_model_path);
  Confidence: band=very_high; score=0.99
- Line 303: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->query_tokenizer = std::make_unique<themis::llm::lora::LlamaTokenizer>(config_.query_model_pat
- Line 304: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->passage_tokenizer = std::make_unique<themis::llm::lora::LlamaTokenizer>(config_.passage_model
- Line 448: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: THEMIS_DEBUG("DPRVectorizer::encodePassageBatch called with empty input; returning empty result");
  Confidence: band=very_high; score=0.99
- Line 140: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::vector<int64_t> input_ids(tokens.begin(), tokens.end());
  Confidence: band=very_high; score=0.9
- Line 153: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: Ort::Value input_ids_tensor = Ort::Value::CreateTensor<int64_t>(
  Confidence: band=very_high; score=0.9
- Line 154: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: mem_info, input_ids.data(), input_ids.size(), shape.data(), shape.size());
  Confidence: band=very_high; score=0.9
- Line 159: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::vector<Ort::AllocatedStringPtr> input_name_holders;
  Confidence: band=very_high; score=0.9
- Line 160: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::vector<const char*> input_names;
  Confidence: band=very_high; score=0.9
- Line 161: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::vector<Ort::Value> input_tensors;
  Confidence: band=very_high; score=0.9
- Line 163: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const size_t input_count = static_cast<size_t>(session->GetInputCount());
  Confidence: band=very_high; score=0.9
- Line 164: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_name_holders.reserve(input_count);
  Confidence: band=very_high; score=0.9
- Line 165: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_names.reserve(input_count);
  Confidence: band=very_high; score=0.9
- Line 166: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_tensors.reserve(input_count);
  Confidence: band=very_high; score=0.9
- Line 168: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_name_holders.push_back(session->GetInputNameAllocated(0, allocator));
  Confidence: band=very_high; score=0.9
- Line 169: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_names.push_back(input_name_holders.back().get());
  Confidence: band=very_high; score=0.9
- Line 170: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_tensors.push_back(std::move(input_ids_tensor));
  Confidence: band=very_high; score=0.9
- Line 171: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (input_count > 1) {
  Confidence: band=very_high; score=0.9
- Line 172: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_name_holders.push_back(session->GetInputNameAllocated(1, allocator));
  Confidence: band=very_high; score=0.9
- Line 173: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_names.push_back(input_name_holders.back().get());
  Confidence: band=very_high; score=0.9
- Line 174: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_tensors.push_back(std::move(attention_mask_tensor));
  Confidence: band=very_high; score=0.9
- Line 177: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto output_name_ptr = session->GetOutputNameAllocated(0, allocator);
- Line 177: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto output_name_ptr = session->GetOutputNameAllocated(0, allocator);
- Line 180: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: auto outputs = session->Run(
  Confidence: band=very_high; score=0.9
- Line 182: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_names.data(),
  Confidence: band=very_high; score=0.9
- Line 183: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_tensors.data(),
  Confidence: band=very_high; score=0.9
- Line 184: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_tensors.size(),
  Confidence: band=very_high; score=0.9
- Line 188: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: if (outputs.empty() || !outputs[0].IsTensor()) {
  Confidence: band=very_high; score=0.9
- Line 192: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: const auto type_info = outputs[0].GetTensorTypeAndShapeInfo();
  Confidence: band=very_high; score=0.9
- Line 194: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: const auto* out_data = outputs[0].GetTensorData<float>();
  Confidence: band=very_high; score=0.9
- Line 194: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: const auto* out_data = outputs[0].GetTensorData<float>();
- Line 262: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("query_model_path is required");
- Line 267: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("passage_model_path is required");
- Line 295: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to load passage encoder model");
- Line 362: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Vectorizer not initialized");
- Line 367: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Query cannot be empty");
- Line 392: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(std::string("Query encoding failed: ") + e.what());
- Line 401: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Vectorizer not initialized");
- Line 406: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Passage cannot be empty");
- Line 432: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(std::string("Passage encoding failed: ") + e.what());
- Line 442: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Vectorizer not initialized");
- Line 448: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: THEMIS_DEBUG("DPRVectorizer::encodePassageBatch called with empty input; returning empty result");
  Confidence: band=very_high; score=0.9
- Line 491: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(std::string("Batch encoding failed: ") + e.what());
- Line 111: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(0);
- Line 192: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto type_info = outputs[0].GetTensorTypeAndShapeInfo();
  Confidence: band=high; score=0.74
- Line 359: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::vector<float> DPRVectorizer::encodeQuery(const std::string& query) {
  Confidence: band=high; score=0.74
- Line 466: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch_tokens.push_back(tokens);
  Confidence: band=high; score=0.74
- Line 466: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch_tokens.push_back(tokens);
  Confidence: band=high; score=0.74
- Line 467: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: batch_tokens.push_back(tokens);
- Line 480: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(embedding));
  Confidence: band=high; score=0.74
- Line 481: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(std::move(embedding));

### src/rag/knowledge_gap_detector.cpp
Total findings: 73

- Line 427: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: avg_similarity >= impl_->config.similarity_threshold) {
- Line 478: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: THEMIS_DEBUG("No new documents retrieved, stopping");
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 133: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: result.explanation = "Retrieved documents have low semantic similarity to query (avg: " +
- Line 209: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Fallback to legacy checks if token_probs not available
  Confidence: band=high; score=0.8
- Line 219: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Check perplexity (legacy)
  Confidence: band=high; score=0.8
- Line 842: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (content_lower.find(term) != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 1049: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Production Delta: LLM-generated samples capture inference chains,
  Confidence: band=very_high; score=0.9
- Line 1085: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto& sent : splitSentences(docs[d].content)) {
- Line 1321: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (content_lower.find(term) != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 1491: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (lower_content.find(framework) != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 1493: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (framework.find("utilitarian") != std::string::npos ||
  Confidence: band=very_high; score=0.9
- Line 1494: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: framework.find("consequentialist") != std::string::npos ||
  Confidence: band=very_high; score=0.9
- Line 1495: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: framework.find("utility") != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 1497: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: } else if (framework.find("deontological") != std::string::npos ||
  Confidence: band=very_high; score=0.9
- Line 1498: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: framework.find("kant") != std::string::npos ||
  Confidence: band=very_high; score=0.9
- Line 1499: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: framework.find("duty") != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 1501: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: } else if (framework.find("virtue") != std::string::npos ||
  Confidence: band=very_high; score=0.9
- Line 1502: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: framework.find("aristotle") != std::string::npos ||
  Confidence: band=very_high; score=0.9
- Line 1503: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: framework.find("character") != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 1505: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: } else if (framework.find("rights") != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 1507: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: } else if (framework.find("care") != std::string::npos ||
  Confidence: band=very_high; score=0.9
- Line 1508: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: framework.find("feminist") != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 1510: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: } else if (framework.find("religious") != std::string::npos ||
  Confidence: band=very_high; score=0.9
- Line 1511: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: framework.find("divine") != std::string::npos ||
  Confidence: band=very_high; score=0.9
- Line 1512: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: framework.find("faith") != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 1514: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: } else if (framework.find("cultural") != std::string::npos ||
  Confidence: band=very_high; score=0.9
- Line 1515: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: framework.find("relativism") != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 108: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 469: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: current_documents.push_back(new_doc);
  Confidence: band=high; score=0.74
- Line 469: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: current_documents.push_back(new_doc);
  Confidence: band=high; score=0.74
- Line 470: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: current_documents.push_back(new_doc);
- Line 633: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: aspects.push_back(current_aspect);
  Confidence: band=high; score=0.74
- Line 634: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: aspects.push_back(current_aspect);
- Line 643: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: aspects.push_back(current_aspect);
- Line 674: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: all_content += doc.content + " ";
  Confidence: band=high; score=0.74
- Line 691: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: missing.push_back(aspect);
  Confidence: band=high; score=0.74
- Line 691: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: missing.push_back(aspect);
  Confidence: band=high; score=0.74
- Line 692: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: missing.push_back(aspect);
- Line 769: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: claims.push_back(claim);
  Confidence: band=high; score=0.74
- Line 770: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: claims.push_back(claim);
- Line 812: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: claim_terms.push_back(current_term);
  Confidence: band=high; score=0.74
- Line 813: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: claim_terms.push_back(current_term);
- Line 820: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: claim_terms.push_back(current_term);
- Line 979: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filtered.push_back(prob);
  Confidence: band=high; score=0.74
- Line 979: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filtered.push_back(prob);
  Confidence: band=high; score=0.74
- Line 980: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: filtered.push_back(prob);
- Line 1007: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: averages.push_back(sum / window_size);
  Confidence: band=high; score=0.74
- Line 1007: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: averages.push_back(sum / window_size);
  Confidence: band=high; score=0.74
- Line 1008: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: averages.push_back(sum / window_size);
- Line 1067: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sentences.push_back(current.substr(s));
  Confidence: band=high; score=0.74
- Line 1068: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sentences.push_back(current.substr(s));
- Line 1076: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sentences.push_back(current.substr(s));
- Line 1095: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: samples.push_back("No document content available for query: " + query
  Confidence: band=high; score=0.74
- Line 1096: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples.push_back("No document content available for query: " + query
- Line 1115: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: samples.push_back(oss.str());
  Confidence: band=high; score=0.74
- Line 1115: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: samples.push_back(oss.str());
  Confidence: band=high; score=0.74
- Line 1116: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples.push_back(oss.str());
- Line 1261: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sentences.push_back(sentence);
  Confidence: band=high; score=0.74
- Line 1262: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sentences.push_back(sentence);
- Line 1273: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sentences.push_back(current_sentence.substr(start));
- Line 1297: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sentence_terms.push_back(current_term);
  Confidence: band=high; score=0.74
- Line 1298: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sentence_terms.push_back(current_term);
- Line 1305: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sentence_terms.push_back(current_term);
- Line 1445: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool KnowledgeGapDetector::isEthicalQuery(const std::string& query) {
  Confidence: band=high; score=0.74
- Line 1483: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> found_frameworks;
  Confidence: band=medium; score=0.66
- Line 872: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: log_sum += std::log(prob);
  Confidence: band=medium; score=0.6
- Line 944: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: log_sum += std::log(prob);
  Confidence: band=medium; score=0.6

### src/rag/continuous_learning_orchestrator.cpp
Total findings: 52

- Line 170: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->learning_loop_active = true;
- Line 171: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->learning_thread = std::make_unique<std::thread>(&ContinuousLearningOrchestrator::learningLoop
- Line 179: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->learning_loop_active = false;
- Line 425: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->current_retrieval_params.top_k = static_cast<size_t>(
- Line 513: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: metrics.training_accuracy    = impl_->stats.current_accuracy;
- Line 670: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: case 2: impl_->stats.prompt_optimizations    = static_cast<size_t>(std::stoull(field)); break;
- Line 671: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: case 3: impl_->stats.retrieval_optimizations = static_cast<size_t>(std::stoull(field)); break;
- Line 672: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: case 4: impl_->stats.lora_retraining_count   = static_cast<size_t>(std::stoull(field)); break;
- Line 672: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: case 4: impl_->stats.lora_retraining_count   = static_cast<size_t>(std::stoull(field)); break;
- Line 685: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const std::string& registry_path = impl_->config.model_registry_path;
- Line 702: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto sleep_duration = impl_->config.learning_loop_interval;
- Line 832: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto next_adapter_revision = impl_->stats.lora_retraining_count + 1;
- Line 917: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->last_loop_results[static_cast<int>(phase)] = result;
- Line 918: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = impl_->loop_handlers.find(static_cast<int>(phase));
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 136: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: impl_->data_selector =
- Line 211: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &test_id : active_tests) {
  Confidence: band=very_high; score=0.9
- Line 277: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &interaction : interactions) {
  Confidence: band=very_high; score=0.9
- Line 294: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &snapshot : impl_->performance_history) {
  Confidence: band=very_high; score=0.9
- Line 480: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: impl_->data_selector->setConfig(refreshed);
- Line 491: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto sel_result = impl_->data_selector->run(candidates);
- Line 505: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: impl_->si_module->setConfig(refreshed);
- Line 535: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: impl_->data_selector->getConfig(), metrics);
- Line 536: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: impl_->data_selector->setConfig(updated_cfg);
- Line 706: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: std::this_thread::sleep_for(std::chrono::seconds(1));
  Confidence: band=very_high; score=0.9
- Line 706: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::seconds(1));
- Line 740: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto result = impl_->data_selector->run(candidate_samples);
- Line 750: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return impl_->data_selector
- Line 751: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ? impl_->data_selector->getConfig()
- Line 752: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: : impl_->config.data_selection_config;
- Line 758: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: impl_->config.data_selection_config = cfg;
- Line 786: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: impl_->loop_handlers[static_cast<int>(phase)] = std::move(handler);
- Line 918: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto it = impl_->loop_handlers.find(static_cast<int>(phase));
- Line 919: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (it != impl_->loop_handlers.end() && it->second) {
- Line 295: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filtered.push_back(snapshot);
  Confidence: band=high; score=0.74
- Line 295: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filtered.push_back(snapshot);
  Confidence: band=high; score=0.74
- Line 296: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: filtered.push_back(snapshot);
- Line 316: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> total_per_version;
  Confidence: band=medium; score=0.66
- Line 317: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> success_per_version;
  Confidence: band=medium; score=0.66
- Line 357: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: impl_->stats.recent_improvements.push_back(event);
- Line 446: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: impl_->stats.recent_improvements.push_back(event);
- Line 530: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: impl_->stats.recent_improvements.push_back(rollback_event);
- Line 590: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: impl_->stats.recent_improvements.push_back(event);
- Line 675: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 696: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: impl_->stats.recent_improvements.push_back(event);
- Line 1050: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if      (c == '"')  out += "\\\"";
- Line 1051: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\\') out += "\\\\";
- Line 1052: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\n') out += "\\n";
- Line 1058: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: static const std::unordered_map<int, std::string> kPhaseNames{
  Confidence: band=medium; score=0.66

### src/rag/distributed_rag_evaluator.cpp
Total findings: 48

- Line 72: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: DistributedRAGEvaluator::evaluate(const judge::EvaluationInput& input)
  Confidence: band=very_high; score=0.99
- Line 99: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Copy the input into shared storage so lambdas that outlive evaluate()
  Confidence: band=very_high; score=0.99
- Line 101: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto shared_input = std::make_shared<judge::EvaluationInput>(input);
  Confidence: band=very_high; score=0.99
- Line 120: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: sem->cv.wait(lk, [&sem, max_parallel] {
- Line 129: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: [judge_ptr, shared_input, sem]() {
  Confidence: band=very_high; score=0.99
- Line 130: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto result = judge_ptr->evaluate(*shared_input);
  Confidence: band=very_high; score=0.99
- Line 222: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const std::vector<judge::EvaluationInput>& inputs)
  Confidence: band=very_high; score=0.99
- Line 225: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: results.reserve(inputs.size());
  Confidence: band=very_high; score=0.99
- Line 227: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (const auto& input : inputs) {
  Confidence: band=very_high; score=0.99
- Line 228: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: results.push_back(evaluate(input));
  Confidence: band=very_high; score=0.99
- Line 56: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 72: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: DistributedRAGEvaluator::evaluate(const judge::EvaluationInput& input)
  Confidence: band=very_high; score=0.9
- Line 99: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Copy the input into shared storage so lambdas that outlive evaluate()
  Confidence: band=very_high; score=0.9
- Line 101: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto shared_input = std::make_shared<judge::EvaluationInput>(input);
  Confidence: band=very_high; score=0.9
- Line 116: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < n; ++i) {
  Confidence: band=very_high; score=0.9
- Line 119: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lk(sem->mtx);
- Line 126: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto judge_ptr = judges[i];  // shared ownership — safe across timeouts
- Line 129: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: [judge_ptr, shared_input, sem]() {
  Confidence: band=very_high; score=0.9
- Line 129: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: [judge_ptr, shared_input, sem]() {
- Line 130: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto result = judge_ptr->evaluate(*shared_input);
  Confidence: band=very_high; score=0.9
- Line 130: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto result = judge_ptr->evaluate(*shared_input);
- Line 130: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto result = judge_ptr->evaluate(*shared_input);
- Line 155: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: res = futures[i].get();
  Confidence: band=very_high; score=0.9
- Line 162: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: res = futures[i].get();
  Confidence: band=very_high; score=0.9
- Line 177: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 187: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 222: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: const std::vector<judge::EvaluationInput>& inputs)
  Confidence: band=very_high; score=0.9
- Line 222: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const std::vector<judge::EvaluationInput>& inputs)
  Confidence: band=very_high; score=0.9
- Line 225: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: results.reserve(inputs.size());
  Confidence: band=very_high; score=0.9
- Line 225: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: results.reserve(inputs.size());
  Confidence: band=very_high; score=0.9
- Line 227: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (const auto& input : inputs) {
  Confidence: band=very_high; score=0.9
- Line 227: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: for (const auto& input : inputs) {
  Confidence: band=very_high; score=0.9
- Line 227: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& input : inputs) {
  Confidence: band=very_high; score=0.9
- Line 228: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: results.push_back(evaluate(input));
  Confidence: band=very_high; score=0.9
- Line 317: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (total_w == 0.0) { total_w = 1.0; }
  Confidence: band=very_high; score=0.9
- Line 396: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 96: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: judges.push_back(std::make_shared<judge::RAGJudge>(w.judge_config));
- Line 126: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(
  Confidence: band=high; score=0.74
- Line 126: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto judge_ptr = judges[i];  // shared ownership — safe across timeouts
  Confidence: band=high; score=0.74
- Line 127: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(
- Line 153: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto status = futures[i].wait_for(timeout);
  Confidence: band=high; score=0.74
- Line 170: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: successful_results.push_back(res);
  Confidence: band=high; score=0.74
- Line 171: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: successful_results.push_back(res);
- Line 172: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: successful_weights.push_back(impl_->workers[i].weight);
- Line 227: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(evaluate(input));
  Confidence: band=high; score=0.74
- Line 228: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(evaluate(input));
- Line 409: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: workers.push_back(std::move(w));
  Confidence: band=high; score=0.74
- Line 410: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: workers.push_back(std::move(w));

### src/rag/document_summarizer.cpp
Total findings: 43

- Line 96: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator start may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: const auto start = current.find_first_not_of(" \t\n\r");
- Line 97: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator end may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: const auto end   = current.find_last_not_of(" \t\n\r");
- Line 108: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator start may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: const auto start = current.find_first_not_of(" \t\n\r");
- Line 339: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Measure total input size
  Confidence: band=very_high; score=0.99
- Line 341: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: result.total_input_chars += d.content.size();
  Confidence: band=very_high; score=0.99
- Line 412: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: result.compression_ratio = result.total_input_chars > 0
  Confidence: band=very_high; score=0.99
- Line 414: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: static_cast<double>(result.total_input_chars)
  Confidence: band=very_high; score=0.99
- Line 421: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: THEMIS_INFO("DocumentSummarizer complete: input={} chars, summary={} chars, "
  Confidence: band=very_high; score=0.99
- Line 423: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: result.total_input_chars, result.summary_chars,
  Confidence: band=very_high; score=0.99
- Line 176: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: << "Document [" << document_id << "]:\n"
- Line 218: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: return LLMIntegration::getInferenceEngine()
  Confidence: band=very_high; score=0.9
- Line 226: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto& w : tokeniseWords(query)) {
- Line 339: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Measure total input size
  Confidence: band=very_high; score=0.9
- Line 341: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: result.total_input_chars += d.content.size();
  Confidence: band=very_high; score=0.9
- Line 361: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: result.combined_summary = LLMIntegration::generate(prompt, opts);
  Confidence: band=very_high; score=0.9
- Line 412: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: result.compression_ratio = result.total_input_chars > 0
  Confidence: band=very_high; score=0.9
- Line 414: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: static_cast<double>(result.total_input_chars)
  Confidence: band=very_high; score=0.9
- Line 421: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: THEMIS_INFO("DocumentSummarizer complete: input={} chars, summary={} chars, "
  Confidence: band=very_high; score=0.9
- Line 423: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: result.total_input_chars, result.summary_chars,
  Confidence: band=very_high; score=0.9
- Line 20: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: * LLMIntegration::generate() so that the LLM produces a fluent, compressed
  Confidence: band=high; score=0.74
- Line 50: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(std::move(cur));
  Confidence: band=high; score=0.74
- Line 51: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(std::move(cur));
- Line 56: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(std::move(cur));
- Line 66: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string>& query_terms)
  Confidence: band=medium; score=0.66
- Line 75: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 101: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sentences.push_back(trimmed);
  Confidence: band=high; score=0.74
- Line 102: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sentences.push_back(trimmed);
- Line 131: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scored.emplace_back(
  Confidence: band=high; score=0.74
- Line 147: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: selected_indices.push_back(idx);
  Confidence: band=high; score=0.74
- Line 148: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: selected_indices.push_back(idx);
- Line 156: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!result.empty()) result += ' ';
  Confidence: band=high; score=0.74
- Line 157: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!result.empty()) result += ' ';
- Line 224: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> queryTerms(const std::string& query) const {
  Confidence: band=medium; score=0.66
- Line 225: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> terms;
  Confidence: band=medium; score=0.66
- Line 257: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: ds.summary  = LLMIntegration::generate(prompt, opts);
  Confidence: band=high; score=0.74
- Line 352: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: id_content.emplace_back(d.id, d.content);
  Confidence: band=high; score=0.74
- Line 361: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: result.combined_summary = LLMIntegration::generate(prompt, opts);
  Confidence: band=high; score=0.74
- Line 381: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.per_document_summaries.push_back(std::move(ds));
  Confidence: band=high; score=0.74
- Line 382: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.per_document_summaries.push_back(std::move(ds));
- Line 398: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.per_document_summaries.push_back(std::move(ds));
  Confidence: band=high; score=0.74
- Line 399: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.per_document_summaries.push_back(std::move(ds));
- Line 444: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: converted.push_back(std::move(rd));
  Confidence: band=high; score=0.74
- Line 445: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: converted.push_back(std::move(rd));

### src/rag/llm_judge_client.cpp
Total findings: 43

- Line 180: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: if (!plugin->loadModel(model_path.string(), plugin_config)) {
  Confidence: band=very_high; score=0.99
- Line 240: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: request.base_request.temperature = static_cast<float>(impl_->config.temperature);
- Line 294: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: request.base_request.temperature = static_cast<float>(impl_->config.temperature);
- Line 14: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: * @brief LLM Judge Client - Connects prompts to InferenceEngineEnhanced
  Confidence: band=very_high; score=0.9
- Line 16: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: * This client bridges RAG Judge evaluations to the LLM inference engine,
  Confidence: band=very_high; score=0.9
- Line 22: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: #include "llm/inference_engine_enhanced.h"
  Confidence: band=very_high; score=0.9
- Line 134: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::directory_iterator(dir)) {
- Line 151: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::shared_ptr<llm::InferenceEngineEnhanced> inference_engine;
  Confidence: band=very_high; score=0.9
- Line 185: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inference_engine->registerModel(config.model_name, plugin);
  Confidence: band=very_high; score=0.9
- Line 201: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Initialize inference engine with appropriate config
  Confidence: band=very_high; score=0.9
- Line 202: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm::InferenceEngineEnhanced::Config engine_config;
  Confidence: band=very_high; score=0.9
- Line 208: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inference_engine = std::make_shared<llm::InferenceEngineEnhanced>(engine_config);
  Confidence: band=very_high; score=0.9
- Line 209: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inference_engine->start();
  Confidence: band=very_high; score=0.9
- Line 216: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (inference_engine) {
  Confidence: band=very_high; score=0.9
- Line 217: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inference_engine->shutdown();
  Confidence: band=very_high; score=0.9
- Line 236: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Create inference request
  Confidence: band=very_high; score=0.9
- Line 237: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm::InferenceEngineEnhanced::EnhancedInferenceRequest request;
  Confidence: band=very_high; score=0.9
- Line 240: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.base_request.temperature = static_cast<float>(impl_->config.temperature);
- Line 247: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: request.preferred_model_id = impl_->config.model_name;
- Line 252: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto handle = impl_->inference_engine->submit(request);
  Confidence: band=very_high; score=0.9
- Line 252: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto handle = impl_->inference_engine->submit(request);
- Line 287: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::vector<llm::InferenceHandle> handles;
  Confidence: band=very_high; score=0.9
- Line 291: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm::InferenceEngineEnhanced::EnhancedInferenceRequest request;
  Confidence: band=very_high; score=0.9
- Line 294: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.base_request.temperature = static_cast<float>(impl_->config.temperature);
- Line 301: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: request.preferred_model_id = impl_->config.model_name;
- Line 305: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: handles.push_back(impl_->inference_engine->submit(request));
  Confidence: band=very_high; score=0.9
- Line 305: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handles.push_back(impl_->inference_engine->submit(request));
- Line 393: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void LLMJudgeClient::setInferenceEngine(
  Confidence: band=very_high; score=0.9
- Line 394: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::shared_ptr<llm::InferenceEngineEnhanced> engine
  Confidence: band=very_high; score=0.9
- Line 396: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: impl_->inference_engine = engine;
  Confidence: band=very_high; score=0.9
- Line 397: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: THEMIS_INFO("Custom inference engine set");
  Confidence: band=very_high; score=0.9
- Line 404: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (impl_->inference_engine) {
  Confidence: band=very_high; score=0.9
- Line 405: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: impl_->inference_engine->registerModel(model_id, plugin);
  Confidence: band=very_high; score=0.9
- Line 68: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dirs.emplace_back(cwd / relative_dir);
  Confidence: band=high; score=0.74
- Line 81: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: names.push_back(model_name);
- Line 85: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: names.push_back(model_name + ".gguf");
- Line 86: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: names.push_back(model_name + ".bin");
- Line 278: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(evaluate(prompt));
- Line 304: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: handles.push_back(impl_->inference_engine->submit(request));
  Confidence: band=high; score=0.74
- Line 305: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: handles.push_back(impl_->inference_engine->submit(request));
- Line 310: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(response.text);
  Confidence: band=high; score=0.74
- Line 311: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(response.text);
- Line 472: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/rag/knowledge_graph_retriever.cpp
Total findings: 39

- Line 61: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: /// length ratio; otherwise 0.0.  Both inputs should already be normalised.
  Confidence: band=very_high; score=0.99
- Line 61: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: /// length ratio; otherwise 0.0.  Both inputs should already be normalised.
  Confidence: band=very_high; score=0.9
- Line 61: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: /// length ratio; otherwise 0.0.  Both inputs should already be normalised.
  Confidence: band=very_high; score=0.9
- Line 137: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto& [src, edges] : impl_->adj) {
  Confidence: band=very_high; score=0.9
- Line 150: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return it != impl_->nodes.end() ? &it->second : nullptr;
- Line 150: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return it != impl_->nodes.end() ? &it->second : nullptr;
- Line 160: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [id, node] : impl_->nodes) {
  Confidence: band=very_high; score=0.9
- Line 166: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& alias : node.aliases) {
  Confidence: band=very_high; score=0.9
- Line 251: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& edge : adj_it->second) {
  Confidence: band=very_high; score=0.9
- Line 485: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // ── Step 2b: KnowledgeGraphReasoner multi-hop inference ─────────────────
  Confidence: band=very_high; score=0.9
- Line 486: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Run forward-chaining inference for each linked query entity when a
  Confidence: band=very_high; score=0.9
- Line 487: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // reasoner has been attached and max_inference_hops > 0.
  Confidence: band=very_high; score=0.9
- Line 488: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (impl_->reasoner && cfg.max_inference_hops > 0) {
  Confidence: band=very_high; score=0.9
- Line 504: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto chain = impl_->reasoner->infer(match.node_id, cfg.max_inference_hops);
  Confidence: band=very_high; score=0.9
- Line 507: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: result.inference_chains.push_back(chain);
  Confidence: band=very_high; score=0.9
- Line 509: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Expand query neighbourhood with nodes reachable via inference.
  Confidence: band=very_high; score=0.9
- Line 522: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: result.inference_chains.size(),
  Confidence: band=very_high; score=0.9
- Line 565: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: for (const auto& chain : result.inference_chains) {
  Confidence: band=very_high; score=0.9
- Line 44: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += ' ';
  Confidence: band=high; score=0.74
- Line 45: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += ' ';
- Line 78: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: double jaccardSets(const std::unordered_set<std::string>& A,
  Confidence: band=medium; score=0.66
- Line 79: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string>& B) {
  Confidence: band=medium; score=0.66
- Line 184: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: impl_->adj[edge.from_id].push_back(std::move(edge));
  Confidence: band=high; score=0.74
- Line 185: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: impl_->adj[edge.from_id].push_back(std::move(edge));
- Line 315: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.push_back({trimmed, EntityType::OTHER, 0.7,
  Confidence: band=high; score=0.74
- Line 316: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entities.push_back({trimmed, EntityType::OTHER, 0.7,
- Line 345: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!span_buf.empty()) span_buf += ' ';
  Confidence: band=high; score=0.74
- Line 389: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matches.push_back(std::move(match));
  Confidence: band=high; score=0.74
- Line 390: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: matches.push_back(std::move(match));
- Line 465: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> query_neighbourhood;
  Confidence: band=medium; score=0.66
- Line 504: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto chain = impl_->reasoner->infer(match.node_id, cfg.max_inference_hops);
  Confidence: band=high; score=0.74
- Line 506: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.inference_chains.push_back(chain);
  Confidence: band=high; score=0.74
- Line 507: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.inference_chains.push_back(chain);
- Line 538: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> doc_node_ids;
  Confidence: band=medium; score=0.66
- Line 577: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!chain_text.empty()) chain_text += "; ";
  Confidence: band=high; score=0.74
- Line 577: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!chain_text.empty()) chain_text += "; ";
  Confidence: band=high; score=0.74
- Line 577: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!chain_text.empty()) chain_text += "; ";
  Confidence: band=high; score=0.74
- Line 578: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!chain_text.empty()) chain_text += "; ";
- Line 578: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!chain_text.empty()) chain_text += "; ";

### src/rag/multi_step_rag.cpp
Total findings: 39

- Line 90: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator first may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: const auto first = line.find_first_not_of(" \t\r");
- Line 91: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator last may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: const auto last  = line.find_last_not_of(" \t\r");
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 197: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const InferenceFn&                 infer) const
  Confidence: band=very_high; score=0.9
- Line 201: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (documents.empty() || !infer) {
  Confidence: band=very_high; score=0.9
- Line 220: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: result.final_answer   = infer(prompt, max_tok);
  Confidence: band=very_high; score=0.9
- Line 239: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // infer is captured by reference; callers must ensure the InferenceFn
  Confidence: band=very_high; score=0.9
- Line 241: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // EXCEPTIONS: if infer() throws, the exception is stored in the future.
  Confidence: band=very_high; score=0.9
- Line 248: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: [this, &batches, &query, &infer, map_max_tok, bi]() -> std::string {
  Confidence: band=very_high; score=0.9
- Line 249: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: return infer(buildMapPrompt(batches[bi], query), map_max_tok);
  Confidence: band=very_high; score=0.9
- Line 267: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string partial          = infer(map_prompt, map_max_tok);
  Confidence: band=very_high; score=0.9
- Line 286: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: result.final_answer  = infer(reduce_prompt, config_.max_response_tokens);
  Confidence: band=very_high; score=0.9
- Line 299: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const InferenceFn&                 infer,
  Confidence: band=very_high; score=0.9
- Line 304: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (!infer) return result;
  Confidence: band=very_high; score=0.9
- Line 335: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const std::string gap_response = infer(gap_prompt, 256);
  Confidence: band=very_high; score=0.9
- Line 92: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: aspects.push_back(line.substr(first, last - first + 1));
  Confidence: band=high; score=0.74
- Line 93: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: aspects.push_back(line.substr(first, last - first + 1));
- Line 171: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batches.push_back(std::move(current_batch));
  Confidence: band=high; score=0.74
- Line 172: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: batches.push_back(std::move(current_batch));
- Line 179: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: current_batch.push_back(doc);
- Line 184: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: batches.push_back(std::move(current_batch));
- Line 220: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: result.final_answer   = infer(prompt, max_tok);
  Confidence: band=high; score=0.74
- Line 224: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.steps.push_back(result.final_answer);
- Line 241: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: // EXCEPTIONS: if infer() throws, the exception is stored in the future.
  Confidence: band=high; score=0.74
- Line 247: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(std::async(std::launch::async,
- Line 249: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: return infer(buildMapPrompt(batches[bi], query), map_max_tok);
  Confidence: band=high; score=0.74
- Line 255: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.steps.push_back(f.get());
  Confidence: band=high; score=0.74
- Line 256: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.steps.push_back(f.get());
- Line 258: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 267: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string partial          = infer(map_prompt, map_max_tok);
  Confidence: band=high; score=0.74
- Line 267: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.steps.push_back(partial);
  Confidence: band=high; score=0.74
- Line 268: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.steps.push_back(partial);
- Line 286: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: result.final_answer  = infer(reduce_prompt, config_.max_response_tokens);
  Confidence: band=high; score=0.74
- Line 324: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: result.final_answer = infer(prompt, max_tok);
  Confidence: band=high; score=0.74
- Line 324: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.steps.push_back(result.final_answer);
  Confidence: band=high; score=0.74
- Line 325: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.steps.push_back(result.final_answer);
- Line 335: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: const std::string gap_response = infer(gap_prompt, 256);
  Confidence: band=high; score=0.74
- Line 353: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!already_present) accumulated.push_back(nd);
  Confidence: band=high; score=0.74
- Line 354: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!already_present) accumulated.push_back(nd);

### src/rag/rlaif_trainer.cpp
Total findings: 35

- Line 179: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->judge  = std::make_shared<HeuristicAIJudge>();
- Line 452: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const double n       = static_cast<double>(impl_->stats.successful_steps);
- Line 453: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const double old_avg = impl_->stats.avg_preference_score;
- Line 454: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->stats.avg_preference_score =
- Line 460: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const double n      = static_cast<double>(impl_->stats.total_steps);
- Line 461: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto old_ms   = impl_->stats.avg_step_ms.count();
- Line 463: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->stats.avg_step_ms =
- Line 87: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (lower.find(p) != std::string::npos) {
- Line 120: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "always inferior", "typical of", "all of them", "never capable"};
  Confidence: band=very_high; score=0.9
- Line 195: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 200: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 201: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: "RLAIFTrainer: min_quality_threshold must be in [0, 1]");
- Line 205: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 206: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: "RLAIFTrainer: min_preference_score must be in [0, 1]");
- Line 209: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 353: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: const double pref_a = impl_->judge->judge(query, response_a, response_b);
- Line 434: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: pair.preference_score >= impl_->config.min_preference_score;
- Line 453: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: const double old_avg = impl_->stats.avg_preference_score;
- Line 454: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: impl_->stats.avg_preference_score =
- Line 506: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return impl_->dataset;
- Line 510: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: impl_->dataset.clear();
- Line 526: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (const auto& pair : impl_->dataset) {
- Line 618: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: impl_->dataset.push_back(synthetic_pair);
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 321: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rev.critiques.push_back(critique);
  Confidence: band=high; score=0.74
- Line 322: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rev.critiques.push_back(critique);
- Line 377: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pair.applied_principles.push_back(p.id);
  Confidence: band=high; score=0.74
- Line 378: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pair.applied_principles.push_back(p.id);
- Line 405: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: step.revision_chain.push_back(rev);
  Confidence: band=high; score=0.74
- Line 406: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: step.revision_chain.push_back(rev);
- Line 438: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: impl_->dataset.push_back(pair);
- Line 471: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 494: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(runTrainingStep(query, draft));
  Confidence: band=high; score=0.74
- Line 495: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(runTrainingStep(query, draft));
- Line 525: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> pv_map;
  Confidence: band=medium; score=0.66

### src/rag/reranker.cpp
Total findings: 34

- Line 16: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: * When a real ONNX cross-encoder model is loaded via loadModel() its
  Confidence: band=very_high; score=0.99
- Line 182: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // loadModel() with a non-empty path.
  Confidence: band=very_high; score=0.99
- Line 196: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != score_cache.end()) return it->second;
- Line 205: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = score_cache.begin();
- Line 220: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: //   auto inputs  = tokenise_pair(query, doc_text, config.max_length);
  Confidence: band=very_high; score=0.99
- Line 278: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const std::string key = impl_->cacheKey(query, doc.id);
- Line 281: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (auto cached = impl_->getCached(key)) {
- Line 350: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: bool CrossEncoderReranker::loadModel(const std::string& model_path) {
  Confidence: band=very_high; score=0.99
- Line 352: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: THEMIS_WARN("CrossEncoderReranker::loadModel called with empty path");
  Confidence: band=very_high; score=0.99
- Line 378: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: (config.enable_score_cache != impl_->config.enable_score_cache) ||
- Line 379: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: (config.max_cache_size != impl_->config.max_cache_size);
- Line 413: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: reranker->loadModel(model_path);
  Confidence: band=very_high; score=0.99
- Line 430: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: reranker->loadModel(model_path);
  Confidence: band=very_high; score=0.99
- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #2556 [rag] Re-ranking layer with cross-encoder model integration + code ... (2026-03-12T05:51
- Line 23: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: *   4. Maps the raw score through a sigmoid to obtain a [0, 1] value.
- Line 93: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * document, then maps it through a sigmoid so the result lies in [0,1].
- Line 220: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: //   auto inputs  = tokenise_pair(query, doc_text, config.max_length);
  Confidence: band=very_high; score=0.9
- Line 220: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: //   auto inputs  = tokenise_pair(query, doc_text, config.max_length);
  Confidence: band=very_high; score=0.9
- Line 221: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: //   auto outputs = session_.Run(...);
  Confidence: band=very_high; score=0.9
- Line 222: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: //   return sigmoid(outputs[0]);
  Confidence: band=very_high; score=0.9
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 55: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(cur);
  Confidence: band=high; score=0.74
- Line 56: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(cur);
- Line 62: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(cur);
- Line 68: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> termFreq(
  Confidence: band=medium; score=0.66
- Line 71: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> tf;
  Confidence: band=medium; score=0.66
- Line 286: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scored.push_back({s, i});
  Confidence: band=high; score=0.74
- Line 286: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scored.push_back({s, i});
  Confidence: band=high; score=0.74
- Line 287: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: scored.push_back({s, i});
- Line 310: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.documents.push_back(std::move(doc));
  Confidence: band=high; score=0.74
- Line 311: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.documents.push_back(std::move(doc));
- Line 319: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.scores.push_back(rs);
- Line 344: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scores.push_back(impl_->computeScore(query, doc));
  Confidence: band=high; score=0.74
- Line 345: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: scores.push_back(impl_->computeScore(query, doc));

### src/rag/llm_judge_integration.cpp
Total findings: 33

- Line 80: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const EvaluationInput& input,
  Confidence: band=very_high; score=0.99
- Line 86: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::string prompt = template_mgr.generatePrompt(dimension, input);
  Confidence: band=very_high; score=0.99
- Line 29: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: LLMJudgeIntegration::LLMJudgeIntegration(ILLMInferenceEngine* engine)
  Confidence: band=very_high; score=0.9
- Line 33: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: LLMJudgeIntegration::LLMJudgeIntegration(ILLMInferenceEngine* engine, const Config& config)
  Confidence: band=very_high; score=0.9
- Line 38: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "Pass a valid ILLMInferenceEngine* or set config.allow_mock = true for testing.");
  Confidence: band=very_high; score=0.9
- Line 41: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Wire the engine's generate() into the inference function slot
  Confidence: band=very_high; score=0.9
- Line 42: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inference_fn_ = [engine](const std::string& prompt) {
  Confidence: band=very_high; score=0.9
- Line 46: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: THEMIS_INFO("LLMJudgeIntegration initialized with injected inference engine");
  Confidence: band=very_high; score=0.9
- Line 49: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inference_fn_ = defaultInference;
  Confidence: band=very_high; score=0.9
- Line 64: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Only set default inference function if mock mode is explicitly enabled
  Confidence: band=very_high; score=0.9
- Line 66: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inference_fn_ = defaultInference;
  Confidence: band=very_high; score=0.9
- Line 72: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // In production mode, require inference function to be set
  Confidence: band=very_high; score=0.9
- Line 73: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inference_fn_ = nullptr;
  Confidence: band=very_high; score=0.9
- Line 74: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: THEMIS_INFO("LLMJudgeIntegration initialized - inference function must be set before use");
  Confidence: band=very_high; score=0.9
- Line 80: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const EvaluationInput& input,
  Confidence: band=very_high; score=0.9
- Line 86: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::string prompt = template_mgr.generatePrompt(dimension, input);
  Confidence: band=very_high; score=0.9
- Line 92: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: error_response.error_message = "Failed to generate prompt";
  Confidence: band=very_high; score=0.9
- Line 175: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void LLMJudgeIntegration::setInferenceFunction(
  Confidence: band=very_high; score=0.9
- Line 178: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inference_fn_ = fn;
  Confidence: band=very_high; score=0.9
- Line 180: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: THEMIS_INFO("Custom inference function set for LLM judge");
  Confidence: band=very_high; score=0.9
- Line 192: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (!inference_fn_) {
  Confidence: band=very_high; score=0.9
- Line 194: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string error_msg = "No inference function set. ";
  Confidence: band=very_high; score=0.9
- Line 195: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: error_msg += "Options: (1) call setInferenceFunction() with a valid LLM inference function; ";
  Confidence: band=very_high; score=0.9
- Line 196: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: error_msg += "(2) pass an ILLMInferenceEngine* to the constructor; ";
  Confidence: band=very_high; score=0.9
- Line 199: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(error_msg);
- Line 210: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Call the inference function
  Confidence: band=very_high; score=0.9
- Line 211: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string response = inference_fn_(prompt);
  Confidence: band=very_high; score=0.9
- Line 218: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string LLMJudgeIntegration::defaultInference(const std::string& prompt) {
  Confidence: band=very_high; score=0.9
- Line 221: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: //          ILLMInferenceEngine is injected, enabling unit tests and offline
  Confidence: band=very_high; score=0.9
- Line 228: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: //                   still non-model mock outputs. As of 2026-04-21 the caller
  Confidence: band=very_high; score=0.9
- Line 238: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: THEMIS_DEBUG("Using mock inference function (for testing only)");
  Confidence: band=very_high; score=0.9
- Line 41: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: // Wire the engine's generate() into the inference function slot
  Confidence: band=high; score=0.74
- Line 43: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: return engine->generate(prompt);
  Confidence: band=high; score=0.74

### src/rag/llm_meta_analyzer.cpp
Total findings: 33

- Line 66: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const std::string& input_text,
  Confidence: band=very_high; score=0.99
- Line 72: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: prompt << "Input:\n" << input_text << "\n\n";
  Confidence: band=very_high; score=0.99
- Line 89: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const std::string& input_text,
  Confidence: band=very_high; score=0.99
- Line 92: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return buildPromptWithCoT(task_description, input_text, criteria, {});
  Confidence: band=very_high; score=0.99
- Line 97: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const std::string& input_text,
  Confidence: band=very_high; score=0.99
- Line 113: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: prompt << "Input:\n" << input_text << "\n\n";
  Confidence: band=very_high; score=0.99
- Line 124: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: prompt << "1. First, analyze the input carefully\n";
  Confidence: band=very_high; score=0.99
- Line 251: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return "Reasoning: The input has been analyzed according to criteria.\nScore: 0.75";
  Confidence: band=very_high; score=0.99
- Line 256: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: metrics["llm_total_calls"] = static_cast<double>(impl_->total_calls);
- Line 257: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: metrics["llm_cache_hits"] = static_cast<double>(impl_->cache_hits);
- Line 257: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: metrics["llm_cache_hits"] = static_cast<double>(impl_->cache_hits);
- Line 258: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: metrics["llm_cache_misses"] = static_cast<double>(impl_->cache_misses);
- Line 258: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: metrics["llm_cache_misses"] = static_cast<double>(impl_->cache_misses);
- Line 267: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::string LLMMetaAnalyzer::computeCacheKey(const std::string& input) {
  Confidence: band=very_high; score=0.99
- Line 273: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (unsigned char c : input) {
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 20: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: #include "llm/inference_engine_enhanced.h"
  Confidence: band=very_high; score=0.9
- Line 66: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const std::string& input_text,
  Confidence: band=very_high; score=0.9
- Line 72: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: prompt << "Input:\n" << input_text << "\n\n";
  Confidence: band=very_high; score=0.9
- Line 89: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const std::string& input_text,
  Confidence: band=very_high; score=0.9
- Line 92: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return buildPromptWithCoT(task_description, input_text, criteria, {});
  Confidence: band=very_high; score=0.9
- Line 97: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const std::string& input_text,
  Confidence: band=very_high; score=0.9
- Line 113: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: prompt << "Input:\n" << input_text << "\n\n";
  Confidence: band=very_high; score=0.9
- Line 124: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: prompt << "1. First, analyze the input carefully\n";
  Confidence: band=very_high; score=0.9
- Line 225: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Delegate to the shared inference engine when one is configured
  Confidence: band=very_high; score=0.9
- Line 226: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto engine = LLMIntegration::getInferenceEngine();
  Confidence: band=very_high; score=0.9
- Line 229: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm::InferenceEngineEnhanced::EnhancedInferenceRequest request;
  Confidence: band=very_high; score=0.9
- Line 238: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.request_id = "meta_" + std::to_string(req_counter.fetch_add(1));
- Line 251: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return "Reasoning: The input has been analyzed according to criteria.\nScore: 0.75";
  Confidence: band=very_high; score=0.9
- Line 267: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::string LLMMetaAnalyzer::computeCacheKey(const std::string& input) {
  Confidence: band=very_high; score=0.9
- Line 273: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: for (unsigned char c : input) {
  Confidence: band=very_high; score=0.9
- Line 160: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::regex("([0-9]*\\.?[0-9]+)\\s*/\\s*1\\.?0?"),
- Line 183: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/rag/delegate_evaluator.cpp
Total findings: 29

- Line 132: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // This can happen for whitespace/punctuation-only inputs after
  Confidence: band=very_high; score=0.99
- Line 160: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * @note For inputs above 10 000 characters, this function switches to an
  Confidence: band=very_high; score=0.99
- Line 167: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * is used instead, keeping RS computation under 5 ms for 100 KB inputs.
  Confidence: band=very_high; score=0.99
- Line 283: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Fallback to plain-text scoring when input is not valid JSON
  Confidence: band=very_high; score=0.99
- Line 132: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // This can happen for whitespace/punctuation-only inputs after
  Confidence: band=very_high; score=0.9
- Line 132: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // This can happen for whitespace/punctuation-only inputs after
  Confidence: band=very_high; score=0.9
- Line 160: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: * @note For inputs above 10 000 characters, this function switches to an
  Confidence: band=very_high; score=0.9
- Line 160: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * @note For inputs above 10 000 characters, this function switches to an
  Confidence: band=very_high; score=0.9
- Line 167: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: * is used instead, keeping RS computation under 5 ms for 100 KB inputs.
  Confidence: band=very_high; score=0.9
- Line 167: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * is used instead, keeping RS computation under 5 ms for 100 KB inputs.
  Confidence: band=very_high; score=0.9
- Line 283: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Fallback to plain-text scoring when input is not valid JSON
  Confidence: band=very_high; score=0.9
- Line 417: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // ── Validate inputs ──────────────────────────────────────────────────────
  Confidence: band=very_high; score=0.9
- Line 417: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // ── Validate inputs ──────────────────────────────────────────────────────
  Confidence: band=very_high; score=0.9
- Line 419: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 424: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 534: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("DelegateEvaluatorFactory: unknown DomainType");
- Line 70: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 110: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(std::move(cur));
  Confidence: band=high; score=0.74
- Line 111: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(std::move(cur));
- Line 118: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!cur.empty()) tokens.push_back(std::move(cur));
- Line 142: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 225: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: static const std::regex TAG_RE(R"(<([A-Za-z_][A-Za-z0-9_:.-]*)[\s/>])");
- Line 257: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 412: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.scores.rs_per_interaction.push_back(1.0);
- Line 453: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 456: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.scores.rs_per_interaction.push_back(rs);
- Line 482: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 485: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.scores.rs_per_interaction.push_back(rs);
- Line 497: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.scores.rs_per_interaction.push_back(rs);

### src/rag/fairness_detector.cpp
Total findings: 29

- Line 329: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: bool loaded = impl_->loadEmbeddings(config_.embedding_model_path);
- Line 138: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto male_it = embeddings.find(male_word);
- Line 139: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto female_it = embeddings.find(female_word);
- Line 188: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto low_it = embeddings.find(low_status);
- Line 189: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto high_it = embeddings.find(high_status);
- Line 235: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto left_it = embeddings.find(left_word);
- Line 236: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto right_it = embeddings.find(right_word);
- Line 369: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("FairnessDetector not initialized");
- Line 374: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Document cannot be empty");
- Line 490: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(std::string("Bias detection failed: ") + e.what());
- Line 500: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("FairnessDetector not initialized");
- Line 521: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("FairnessDetector not initialized");
- Line 98: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: embedding.push_back(value);
- Line 110: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: file.close();
- Line 150: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: difference_vectors.push_back(diff);
  Confidence: band=high; score=0.74
- Line 150: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: difference_vectors.push_back(diff);
  Confidence: band=high; score=0.74
- Line 151: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: difference_vectors.push_back(diff);
- Line 200: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: difference_vectors.push_back(diff);
  Confidence: band=high; score=0.74
- Line 200: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: difference_vectors.push_back(diff);
  Confidence: band=high; score=0.74
- Line 201: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: difference_vectors.push_back(diff);
- Line 249: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: difference_vectors.push_back(std::move(diff));
  Confidence: band=high; score=0.74
- Line 249: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: difference_vectors.push_back(std::move(diff));
  Confidence: band=high; score=0.74
- Line 250: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: difference_vectors.push_back(std::move(diff));
- Line 387: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: words.push_back(Impl::toLower(word));
  Confidence: band=high; score=0.74
- Line 388: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: words.push_back(Impl::toLower(word));
- Line 394: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: words.push_back(Impl::toLower(word));
- Line 508: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(detectBias(doc));
  Confidence: band=high; score=0.74
- Line 509: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(detectBias(doc));
- Line 528: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filtered.emplace_back(doc, bias_score);
  Confidence: band=high; score=0.74

### src/rag/calibration_manager.cpp
Total findings: 27

- Line 187: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationInput input;
  Confidence: band=very_high; score=0.99
- Line 188: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.query            = ann.query;
  Confidence: band=very_high; score=0.99
- Line 189: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.generated_answer = ann.answer;
  Confidence: band=very_high; score=0.99
- Line 190: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: predictions.push_back(judge.evaluate(input));
  Confidence: band=very_high; score=0.99
- Line 450: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: bool CalibrationManager::loadModel(const std::string& filepath) {
  Confidence: band=very_high; score=0.99
- Line 187: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationInput input;
  Confidence: band=very_high; score=0.9
- Line 188: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.query            = ann.query;
  Confidence: band=very_high; score=0.9
- Line 189: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.generated_answer = ann.answer;
  Confidence: band=very_high; score=0.9
- Line 190: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: predictions.push_back(judge.evaluate(input));
  Confidence: band=very_high; score=0.9
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 79: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ann.annotators.push_back(a.get<std::string>());
  Confidence: band=high; score=0.74
- Line 79: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ann.annotators.push_back(a.get<std::string>());
  Confidence: band=high; score=0.74
- Line 80: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ann.annotators.push_back(a.get<std::string>());
- Line 84: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ground_truth_.push_back(std::move(ann));
- Line 94: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ground_truth_.push_back(annotation);
- Line 159: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: blocks.push_back(b);
  Confidence: band=high; score=0.74
- Line 160: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: blocks.push_back(b);
- Line 189: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: predictions.push_back(judge.evaluate(input));
  Confidence: band=high; score=0.74
- Line 190: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: predictions.push_back(judge.evaluate(input));
- Line 199: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: preds.push_back(predictions[i].overall_score);
- Line 200: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: gts.push_back(ground_truth_[i].overall_score);
- Line 209: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (double p : preds) scaled.push_back(applyTemperatureScaling(p, t));
- Line 294: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pred_overall.push_back(p);
  Confidence: band=high; score=0.74
- Line 295: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pred_overall.push_back(p);
- Line 296: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: gt_overall.push_back(g);
- Line 297: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: confs.push_back(predictions[i].confidence > 0.0
- Line 106: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: double logit = std::log(s / (1.0 - s));
  Confidence: band=medium; score=0.6

### src/rag/multi_hop_reasoner.cpp
Total findings: 27

- Line 159: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceFn inference_fn) const
  Confidence: band=very_high; score=0.9
- Line 163: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (inference_fn) {
  Confidence: band=very_high; score=0.9
- Line 168: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inference_fn(prompt, config_.max_tokens_per_hop);
  Confidence: band=very_high; score=0.9
- Line 223: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceFn inference_fn) const
  Confidence: band=very_high; score=0.9
- Line 240: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (!inference_fn) {
  Confidence: band=very_high; score=0.9
- Line 259: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const std::string composed = inference_fn(prompt, config_.max_tokens_final);
  Confidence: band=very_high; score=0.9
- Line 270: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceFn inference_fn) const
  Confidence: band=very_high; score=0.9
- Line 274: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (query.empty() || !retrieval_fn || !inference_fn) {
  Confidence: band=very_high; score=0.9
- Line 282: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: decomposeQuery(query, inference_fn);
  Confidence: band=very_high; score=0.9
- Line 310: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inference_fn(prompt, config_.max_tokens_per_hop);
  Confidence: band=very_high; score=0.9
- Line 343: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: result.final_answer = composeAnswer(query, result.hop_records, inference_fn);
  Confidence: band=very_high; score=0.9
- Line 80: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 83: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(d);
  Confidence: band=high; score=0.74
- Line 84: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(d);
- Line 112: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!t.empty()) sub_queries.push_back(t);
- Line 136: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!t.empty()) sentences.push_back(t);
  Confidence: band=high; score=0.74
- Line 137: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!t.empty()) sentences.push_back(t);
- Line 142: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!t.empty()) sentences.push_back(t);
- Line 146: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: parts.push_back(q);
- Line 150: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: parts.push_back(s);
  Confidence: band=high; score=0.74
- Line 151: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: parts.push_back(s);
- Line 228: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: partial_answers.push_back(
  Confidence: band=high; score=0.74
- Line 229: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: partial_answers.push_back(
- Line 317: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.hop_records.push_back(hop);
- Line 321: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.all_documents.push_back(doc);
  Confidence: band=high; score=0.74
- Line 322: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.all_documents.push_back(doc);
- Line 326: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: previous_answers.push_back(hop.intermediate_answer);

### src/rag/llm_integration.cpp
Total findings: 26

- Line 18: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: #include "llm/inference_engine_enhanced.h"
  Confidence: band=very_high; score=0.9
- Line 38: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Static member to hold the inference engine
  Confidence: band=very_high; score=0.9
- Line 39: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: static std::shared_ptr<llm::InferenceEngineEnhanced> g_inference_engine = nullptr;
  Confidence: band=very_high; score=0.9
- Line 53: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: return R"({"score":0.75,"confidence":0.80,"explanation":"Fallback evaluation response (no active inference engine/model)."})";
  Confidence: band=very_high; score=0.9
- Line 102: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void LLMIntegration::setInferenceEngine(std::shared_ptr<llm::InferenceEngineEnhanced> engine) {
  Confidence: band=very_high; score=0.9
- Line 104: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: g_inference_engine = engine;
  Confidence: band=very_high; score=0.9
- Line 105: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: THEMIS_INFO("LLM Integration: Inference engine configured");
  Confidence: band=very_high; score=0.9
- Line 108: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::shared_ptr<llm::InferenceEngineEnhanced> LLMIntegration::getInferenceEngine() {
  Confidence: band=very_high; score=0.9
- Line 110: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: return g_inference_engine;
  Confidence: band=very_high; score=0.9
- Line 123: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto engine = getInferenceEngine();
  Confidence: band=very_high; score=0.9
- Line 129: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm::InferenceRequest req;
  Confidence: band=very_high; score=0.9
- Line 136: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto response = llm::LLMPluginManager::instance().generate(req);
  Confidence: band=very_high; score=0.9
- Line 150: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Create an enhanced inference request
  Confidence: band=very_high; score=0.9
- Line 151: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm::InferenceEngineEnhanced::EnhancedInferenceRequest request;
  Confidence: band=very_high; score=0.9
- Line 153: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.base_request.max_tokens = static_cast<int>(std::min(
- Line 166: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto handle = engine->submit(request);
- Line 174: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // available (InferenceResponse.logprobs stores per-token log-probs),
  Confidence: band=very_high; score=0.9
- Line 62: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, std::string>& variables
  Confidence: band=medium; score=0.66
- Line 136: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto response = llm::LLMPluginManager::instance().generate(req);
  Confidence: band=high; score=0.74
- Line 241: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: samples.push_back(generate(prompt, sample_options));
  Confidence: band=high; score=0.74
- Line 242: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples.push_back(generate(prompt, sample_options));
- Line 242: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: samples.push_back(generate(prompt, sample_options));
  Confidence: band=high; score=0.74
- Line 331: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(word);
- Line 344: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> set1(tokens1.begin(), tokens1.end());
  Confidence: band=medium; score=0.66
- Line 345: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> set2(tokens2.begin(), tokens2.end());
  Confidence: band=medium; score=0.66
- Line 302: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: log_sum += std::log(prob);
  Confidence: band=medium; score=0.6

### src/rag/multimodal_rag.cpp
Total findings: 24

- Line 240: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: src.image_path      = img_it->second.image_path;
- Line 241: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: src.metadata        = img_it->second.metadata;
- Line 245: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: src.caption = img_it->second.caption;
- Line 247: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: src.caption = impl_->image_captioner(img_it->second);
- Line 257: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator txt_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto txt_it = text_doc_map.find(doc_id);
- Line 138: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function query without trace point
  Context: MultiModalRAGResult MultiModalRAG::query(const MultiModalQuery& mq) const {
  Confidence: band=very_high; score=0.9
- Line 241: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: src.metadata        = img_it->second.metadata;
- Line 264: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: src.metadata        = txt_it->second.metadata;
- Line 138: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: MultiModalRAGResult MultiModalRAG::query(const MultiModalQuery& mq) const {
  Confidence: band=high; score=0.74
- Line 168: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: text_ranked.emplace_back(doc.id, doc.similarity_score);
  Confidence: band=high; score=0.74
- Line 168: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: text_ranked.emplace_back(doc.id, doc.similarity_score);
  Confidence: band=high; score=0.74
- Line 176: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, ImageDocument> image_doc_map;
  Confidence: band=medium; score=0.66
- Line 186: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: image_ranked.emplace_back(img.id, img.relevance_score);
  Confidence: band=high; score=0.74
- Line 198: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: all_lists.push_back(text_ranked);
- Line 199: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: weights.push_back(cfg.text_weight);
- Line 202: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: all_lists.push_back(image_ranked);
- Line 203: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: weights.push_back(cfg.image_weight);
- Line 226: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> used_ids;
  Confidence: band=medium; score=0.66
- Line 252: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.sources.push_back(std::move(src));
- Line 265: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.sources.push_back(std::move(src));
- Line 298: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if      (src.modality == Modality::TEXT)  text_sources.push_back(&src);
  Confidence: band=high; score=0.74
- Line 299: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if      (src.modality == Modality::TEXT)  text_sources.push_back(&src);
- Line 300: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: else if (src.modality == Modality::IMAGE) image_sources.push_back(&src);
- Line 301: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: else if (src.modality == Modality::TABLE) table_sources.push_back(&src);

### src/rag/prompt_injection_detector.cpp
Total findings: 24

- Line 323: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: PromptInjectionDetector::scanDocuments(const judge::EvaluationInput& input) const
  Confidence: band=very_high; score=0.99
- Line 326: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: results.reserve(input.documents.size());
  Confidence: band=very_high; score=0.99
- Line 327: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (const auto& doc : input.documents) {
  Confidence: band=very_high; score=0.99
- Line 411: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: judge::EvaluationInput
  Confidence: band=very_high; score=0.99
- Line 106: severity=HIGH; category=performance; pattern=regex_in_loop
  Description: std::regex compiled in loop (compile once, reuse)
  Context: for (const auto& e : shared.patterns()) {
  Confidence: band=very_high; score=0.9
- Line 256: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& rule : getRules()) {
- Line 323: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: PromptInjectionDetector::scanDocuments(const judge::EvaluationInput& input) const
  Confidence: band=very_high; score=0.9
- Line 326: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: results.reserve(input.documents.size());
  Confidence: band=very_high; score=0.9
- Line 327: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: for (const auto& doc : input.documents) {
  Confidence: band=very_high; score=0.9
- Line 411: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: judge::EvaluationInput
  Confidence: band=very_high; score=0.9
- Line 107: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.push_back({
  Confidence: band=high; score=0.74
- Line 108: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: r.push_back({
- Line 123: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: r.push_back({
- Line 161: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: r.push_back({
- Line 165: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: R"(\bhttps?://[^\s]+\?(?:[^\s]*=\{[^}]*\}|\[PROMPT\]|\[CONTEXT\]|\[OUTPUT\]))",
- Line 203: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: findings.push_back(f);
  Confidence: band=high; score=0.74
- Line 204: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: findings.push_back(f);
- Line 276: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.findings.push_back(f);
  Confidence: band=high; score=0.74
- Line 277: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.findings.push_back(f);
- Line 327: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(scan(doc.content));
  Confidence: band=high; score=0.74
- Line 328: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(scan(doc.content));
- Line 391: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_replace.push_back(&f);
  Confidence: band=high; score=0.74
- Line 391: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_replace.push_back(&f);
  Confidence: band=high; score=0.74
- Line 392: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: to_replace.push_back(&f);

### src/rag/examples/loop_orchestration_example.cpp
Total findings: 23

- Line 55: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "\n  [ExplainabilityReasonBuilder — PLANNED]\n"
  Confidence: band=very_high; score=0.9
- Line 55: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: std::cout << "\n  [ExplainabilityReasonBuilder — PLANNED]\n"
- Line 69: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "=== Loop 1–4 Orchestration Example (IMPL-A2 + IMPL-A3 + IMPL-B9) ===\n\n";
  Confidence: band=very_high; score=0.9
- Line 79: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Step 1: Loop 1 — per-query BaoOptimizer feedback\n";
  Confidence: band=very_high; score=0.9
- Line 91: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  triggerLoop1QueryExecution() called for query: " << outcome.query_id << "\n"
  Confidence: band=very_high; score=0.9
- Line 100: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "\nStep 2: Loop 2 — WorkloadAdaptiveOptimizer\n";
  Confidence: band=very_high; score=0.9
- Line 105: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  triggerLoop2WorkloadAdaptation() called.\n"
  Confidence: band=very_high; score=0.9
- Line 111: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "\nStep 3: Loop 4 — IncrementalLoRATrainer\n";
  Confidence: band=very_high; score=0.9
- Line 115: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Training complete — adapter v" << orchestrator.activeAdapterVersion() << "\n";
  Confidence: band=very_high; score=0.9
- Line 117: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  triggerLoop4AdapterImprovement() called.\n"
  Confidence: band=very_high; score=0.9
- Line 123: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "\nStep 4: FEDERATED_ROUND_START event\n";
  Confidence: band=very_high; score=0.9
- Line 133: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  [PLANNED] FEDERATED_ROUND_START fires after Loop 4 completes\n"
  Confidence: band=very_high; score=0.9
- Line 139: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "\nStep 5: ExplainabilityReasonBuilder — Loop 2 decision trace\n";
  Confidence: band=very_high; score=0.9
- Line 147: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Summary: " << chain.summary << "\n";
  Confidence: band=very_high; score=0.9
- Line 149: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "    → " << step << "\n";
  Confidence: band=very_high; score=0.9
- Line 151: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Written to AIDecisionAuditor.\n";
  Confidence: band=very_high; score=0.9
- Line 153: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "\n  [ExplainabilityReasonBuilder — PLANNED — IMPL-B9]\n"
  Confidence: band=very_high; score=0.9
- Line 153: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: std::cout << "\n  [ExplainabilityReasonBuilder — PLANNED — IMPL-B9]\n"
- Line 165: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "\n=== Summary ===\n"
  Confidence: band=very_high; score=0.9
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 149: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cout << "    → " << step << "\n";
- Line 155: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "  Trigger  : QPS spike +3 200 req/s; p99 latency 85 ms\n"
- Line 171: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "\nSee docs/issues/ for implementation specs.\n";

### src/rag/hybrid_retriever.cpp
Total findings: 22

- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #2747 [rag] Hybrid retrieval (BM25 + vector) with configurable RRF weight... (2026-03-12T05:57
- Line 45: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (range == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 93: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 97: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 101: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 148: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("HybridRetriever::retrieveWithVectorizer: query must not be empty");
- Line 151: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("HybridRetriever::retrieveWithVectorizer: no vectorizer configured");
- Line 154: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("HybridRetriever::retrieveWithVectorizer: vectorizer is not initialized");
- Line 171: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vector_candidates.push_back(std::move(dense_doc));
  Confidence: band=high; score=0.74
- Line 172: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vector_candidates.push_back(std::move(dense_doc));
- Line 235: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entries.push_back(std::move(data));
- Line 255: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.documents.push_back(std::move(e.doc));
  Confidence: band=high; score=0.74
- Line 256: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.documents.push_back(std::move(e.doc));
- Line 257: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.scores.push_back(std::move(e.score));
- Line 277: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, DocData> doc_map;
  Confidence: band=medium; score=0.66
- Line 284: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: bm25_raw.push_back(src.similarity_score);
- Line 290: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vec_raw.push_back(src.similarity_score);
  Confidence: band=high; score=0.74
- Line 291: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vec_raw.push_back(src.similarity_score);
- Line 318: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entries.push_back(std::move(data));
- Line 336: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.documents.push_back(std::move(e.doc));
  Confidence: band=high; score=0.74
- Line 337: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.documents.push_back(std::move(e.doc));
- Line 338: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.scores.push_back(std::move(e.score));

### src/rag/prompt_templates.cpp
Total findings: 20

- Line 82: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const EvaluationInput& input
  Confidence: band=very_high; score=0.99
- Line 101: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: prompt = replacePlaceholders(prompt, input);
  Confidence: band=very_high; score=0.99
- Line 329: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const EvaluationInput& input
  Confidence: band=very_high; score=0.99
- Line 336: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: result.replace(pos, 7, input.query);
  Confidence: band=very_high; score=0.99
- Line 337: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: pos += input.query.length();
  Confidence: band=very_high; score=0.99
- Line 343: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: result.replace(pos, 8, input.generated_answer);
  Confidence: band=very_high; score=0.99
- Line 344: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: pos += input.generated_answer.length();
  Confidence: band=very_high; score=0.99
- Line 349: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (size_t i = 0; i < input.documents.size(); ++i) {
  Confidence: band=very_high; score=0.99
- Line 351: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: << input.documents[i].content << "\n\n";
  Confidence: band=very_high; score=0.99
- Line 73: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: templates_[dimension] = buffer.str();
- Line 82: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const EvaluationInput& input
  Confidence: band=very_high; score=0.9
- Line 101: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: prompt = replacePlaceholders(prompt, input);
  Confidence: band=very_high; score=0.9
- Line 329: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const EvaluationInput& input
  Confidence: band=very_high; score=0.9
- Line 336: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: result.replace(pos, 7, input.query);
  Confidence: band=very_high; score=0.9
- Line 337: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: pos += input.query.length();
  Confidence: band=very_high; score=0.9
- Line 343: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: result.replace(pos, 8, input.generated_answer);
  Confidence: band=very_high; score=0.9
- Line 344: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: pos += input.generated_answer.length();
  Confidence: band=very_high; score=0.9
- Line 349: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: for (size_t i = 0; i < input.documents.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 351: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: << input.documents[i].content << "\n\n";
  Confidence: band=very_high; score=0.9
- Line 378: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: stream << "Score: " << ex.score << "/5\n";

### src/rag/replug_retriever.cpp
Total findings: 20

- Line 257: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = weights_.find(doc.id);
- Line 112: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 113: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: "ReplugRetriever: llm_weight must be in [0, 1]");
- Line 116: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 120: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 124: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 180: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (range == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 256: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = weights_.find(doc.id);
- Line 256: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = weights_.find(doc.id);
- Line 345: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (w == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 62: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: double jaccardSimilarity(const std::unordered_set<std::string>& a,
  Confidence: band=medium; score=0.66
- Line 63: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string>& b) {
  Confidence: band=medium; score=0.66
- Line 223: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: raw.push_back(scorer_->score(query, doc.content));
- Line 244: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: filtered.push_back(doc);
- Line 259: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ret_scores.push_back(doc.similarity_score * w);
  Confidence: band=high; score=0.74
- Line 260: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ret_scores.push_back(doc.similarity_score * w);
- Line 292: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fused.push_back((1.0 - lam) * ret_scores[i] + lam * llm_probs[i]);
- Line 315: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.documents.push_back(std::move(doc));
  Confidence: band=high; score=0.74
- Line 316: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.documents.push_back(std::move(doc));
- Line 324: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.scores.push_back(sc);

### src/rag/agentic_rag.cpp
Total findings: 19

- Line 340: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: THEMIS_DEBUG("AgenticRAG iter {}: retrieved {} new docs for query='{}'",
- Line 344: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: THEMIS_INFO("AgenticRAG no new documents at iteration {}; stopping.", iter);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 135: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: const auto& strategy = impl_->config.reformulation_strategy;
- Line 38: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string>& ids)
  Confidence: band=medium; score=0.66
- Line 50: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string>& seen_ids,
  Confidence: band=medium; score=0.66
- Line 57: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: accumulator.push_back(doc);
  Confidence: band=high; score=0.74
- Line 58: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: accumulator.push_back(doc);
- Line 282: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: gap_docs.push_back(toGapDoc(d));
  Confidence: band=high; score=0.74
- Line 283: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: gap_docs.push_back(toGapDoc(d));
- Line 301: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.iterations.push_back(std::move(record));
- Line 401: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!seed.empty()) seed += '\n';
  Confidence: band=high; score=0.74
- Line 401: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!seed.empty()) seed += '\n';
  Confidence: band=high; score=0.74
- Line 402: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!seed.empty()) seed += '\n';
- Line 420: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/rag/continuous_learning_client.cpp
Total findings: 19

- Line 57: severity=CRITICAL; category=no_timeout
  Description: thread_join without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: batch_thread.join();
- Line 216: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: impl_->batch_mutex.lock();
- Line 65: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: std::this_thread::sleep_for(
  Confidence: band=very_high; score=0.9
- Line 185: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& metric : metrics) {
  Confidence: band=very_high; score=0.9
- Line 207: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::vector<QualityMetric> to_send(
- Line 385: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: faithfulness.metadata["mode"] = static_cast<int>(result.mode);
- Line 414: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: overall.metadata["decision"] = static_cast<int>(result.decision);
- Line 415: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: overall.metadata["passed_threshold"] = result.passed_threshold;
- Line 423: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: latency.metadata["mode"] = static_cast<int>(result.mode);
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 109: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: payload.push_back(metric_json);
  Confidence: band=high; score=0.74
- Line 110: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: payload.push_back(metric_json);
- Line 140: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.push_back(metric.value);
  Confidence: band=high; score=0.74
- Line 141: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: values.push_back(metric.value);
- Line 202: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: impl_->metric_batch.push_back(metric);
  Confidence: band=high; score=0.74
- Line 203: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: impl_->metric_batch.push_back(metric);
- Line 455: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: combined += "; ";
  Confidence: band=high; score=0.74
- Line 456: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: combined += "; ";

### src/rag/onnx_model_loader.cpp
Total findings: 18

- Line 127: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: return loadModel(dest_path);
  Confidence: band=very_high; score=0.99
- Line 146: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != impl_->cache.end()) {
- Line 185: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: cleared = impl_->cache.size();
- Line 189: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = impl_->cache.find(model_name);
- Line 190: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != impl_->cache.end()) {
- Line 249: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: FILE* fp = fopen(dest_path.c_str(), "wb");
- Line 286: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: info.input_shape = {1, 512};  // batch_size=1, seq_len=512
  Confidence: band=very_high; score=0.99
- Line 297: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: info.input_shape = {1, 512};
  Confidence: band=very_high; score=0.99
- Line 308: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: info.input_shape = {1, 1024};
  Confidence: band=very_high; score=0.99
- Line 172: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [name, info] : impl_->cache) {
  Confidence: band=very_high; score=0.9
- Line 188: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = impl_->cache.find(model_name);
- Line 221: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: char buffer[BUFFER_SIZE];
- Line 286: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: info.input_shape = {1, 512};  // batch_size=1, seq_len=512
  Confidence: band=very_high; score=0.9
- Line 297: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: info.input_shape = {1, 512};
  Confidence: band=very_high; score=0.9
- Line 308: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: info.input_shape = {1, 1024};
  Confidence: band=very_high; score=0.9
- Line 172: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: models.push_back(name);
  Confidence: band=high; score=0.74
- Line 173: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: models.push_back(name);
- Line 264: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: fclose(fp);

### src/rag/quality_control_pipeline.cpp
Total findings: 18

- Line 126: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto passed_fast_count = static_cast<double>(impl_->stats.passed_fast);
- Line 128: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->stats.avg_fast_time_ms =
- Line 524: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->learning_callback = callback;
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 244: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: doc_pairs.emplace_back(doc.id, doc.content);
  Confidence: band=high; score=0.74
- Line 260: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.dimension_scores.push_back(faith_score);
- Line 298: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: doc_pairs.emplace_back(doc.id, doc.content);
  Confidence: band=high; score=0.74
- Line 315: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.dimension_scores.push_back(score);
  Confidence: band=high; score=0.74
- Line 316: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.dimension_scores.push_back(score);
- Line 328: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.failure_reasons.push_back(
- Line 355: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: doc_pairs.emplace_back(doc.id, doc.content);
  Confidence: band=high; score=0.74
- Line 370: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.dimension_scores.push_back(faith_score);
- Line 374: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.failure_reasons.push_back("NLI verification failed");
- Line 405: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: source_chunks.push_back(std::move(sc));
  Confidence: band=high; score=0.74
- Line 406: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: source_chunks.push_back(std::move(sc));
- Line 418: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.dimension_scores.push_back(cit_score);
- Line 458: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> weights = {
  Confidence: band=medium; score=0.66

### src/rag/evaluation_cache.cpp
Total findings: 16

- Line 77: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = lru_map_.find(key);
- Line 87: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = lru_map_.find(key);
- Line 107: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = cache_.find(key);
- Line 181: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = cache_.find(key);
- Line 218: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = cache_.begin(); it != cache_.end(); ) {
- Line 247: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: RAGJudge& judge, const std::vector<EvaluationInput>& queries) {
  Confidence: band=very_high; score=0.99
- Line 250: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (const auto& input : queries) {
  Confidence: band=very_high; score=0.99
- Line 252: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (contains(input.query, input.generated_answer)) continue;
  Confidence: band=very_high; score=0.99
- Line 254: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto result = judge.evaluate(input);
  Confidence: band=very_high; score=0.99
- Line 255: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: put(input.query, input.generated_answer, result);
  Confidence: band=very_high; score=0.99
- Line 247: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: RAGJudge& judge, const std::vector<EvaluationInput>& queries) {
  Confidence: band=very_high; score=0.9
- Line 250: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: for (const auto& input : queries) {
  Confidence: band=very_high; score=0.9
- Line 250: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& input : queries) {
  Confidence: band=very_high; score=0.9
- Line 252: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (contains(input.query, input.generated_answer)) continue;
  Confidence: band=very_high; score=0.9
- Line 254: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto result = judge.evaluate(input);
  Confidence: band=very_high; score=0.9
- Line 255: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: put(input.query, input.generated_answer, result);
  Confidence: band=very_high; score=0.9

### src/rag/geval_evaluator.cpp
Total findings: 16

- Line 355: severity=CRITICAL; category=audit_logging; pattern=sensitive_data_logging
  Description: Potential PII/credential logging: token
  Context: reasoning << "Token probability distribution:\n";
  Confidence: band=very_high; score=0.92
- Line 18: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: #include "llm/inference_engine_enhanced.h"
  Confidence: band=very_high; score=0.9
- Line 53: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::shared_ptr<llm::InferenceEngineEnhanced> llm;
  Confidence: band=very_high; score=0.9
- Line 58: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm::InferenceEngineEnhanced::Config engine_cfg;
  Confidence: band=very_high; score=0.9
- Line 59: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm = std::make_shared<llm::InferenceEngineEnhanced>(engine_cfg);
  Confidence: band=very_high; score=0.9
- Line 232: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm::InferenceEngineEnhanced::EnhancedInferenceRequest req;
  Confidence: band=very_high; score=0.9
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['                std::string tok;', '                size_t idx = 0;', '                while (iss >> tok && idx < response.logprobs.size()) {', '                    // kNumScoreLevels ≤ 9 so single-digit check is safe', "                    char max_digit = static_cast<char>('0' + kNumScoreLevels);"]
  Confidence: band=medium; score=0.62
- Line 130: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: score_tokens.push_back(token_id);
  Confidence: band=high; score=0.74
- Line 130: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: score_tokens.push_back(token_id);
  Confidence: band=high; score=0.74
- Line 131: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: score_tokens.push_back(token_id);
- Line 139: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: score_tokens.push_back(-1);  // Token not in vocabulary; skip during probability extraction
- Line 271: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 314: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: all_probabilities.push_back(probs);
  Confidence: band=high; score=0.74
- Line 315: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: all_probabilities.push_back(probs);
- Line 319: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sample_scores.push_back(score);
- Line 468: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<int, int> counts;
  Confidence: band=medium; score=0.66

### src/rag/response_parser.cpp
Total findings: 14

- Line 14: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: * @brief Implementation of response parsing for LLM judge outputs
  Confidence: band=very_high; score=0.9
- Line 205: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("Score {} out of valid range [0, 5]", score);
- Line 210: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("Confidence {} out of valid range [0, 1]", *parsed.confidence);
- Line 219: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (max_range == min_range) {
  Confidence: band=very_high; score=0.9
- Line 34: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: const char* ResponseParser::SCORE_PATTERN_1 = R"((?:score|rating)[\s:]+([0-9.]+)(?:/5|%|\s|$))";
- Line 35: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: const char* ResponseParser::SCORE_PATTERN_2 = R"(([0-9.]+)\s*(?:out of|/)\s*([0-9.]+))";
- Line 118: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.supporting_claims.push_back(claim.get<std::string>());
  Confidence: band=high; score=0.74
- Line 119: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.supporting_claims.push_back(claim.get<std::string>());
- Line 124: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.unsupported_claims.push_back(claim.get<std::string>());
  Confidence: band=high; score=0.74
- Line 125: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.unsupported_claims.push_back(claim.get<std::string>());
- Line 248: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 264: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 274: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 342: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: claims.push_back(claim);

### src/rag/claim_extractor.cpp
Total findings: 12

- Line 56: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string response = LLMIntegration::generate(prompt);
  Confidence: band=very_high; score=0.9
- Line 115: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string response = LLMIntegration::generate(prompt);
  Confidence: band=very_high; score=0.9
- Line 248: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string llm_response = LLMIntegration::generate(prompt);
  Confidence: band=very_high; score=0.9
- Line 56: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string response = LLMIntegration::generate(prompt);
  Confidence: band=high; score=0.74
- Line 78: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: claims.push_back(claim);
- Line 115: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string response = LLMIntegration::generate(prompt);
  Confidence: band=high; score=0.74
- Line 158: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(verify(claim, documents));
  Confidence: band=high; score=0.74
- Line 159: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(verify(claim, documents));
- Line 248: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string llm_response = LLMIntegration::generate(prompt);
  Confidence: band=high; score=0.74
- Line 253: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.agreements.push_back("General agreement found");
  Confidence: band=high; score=0.74
- Line 254: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.agreements.push_back("General agreement found");
- Line 258: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.disagreements.push_back("Some disagreements found");

### src/rag/faithfulness_evaluator.cpp
Total findings: 11

- Line 116: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->nli_verifier = std::make_shared<NLIFaithfulnessVerifier>(nli_config);
- Line 69: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: claim_words.push_back(word);
- Line 153: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: claims.push_back(std::move(claim));
  Confidence: band=high; score=0.74
- Line 154: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: claims.push_back(std::move(claim));
- Line 177: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: claims.push_back(claim);
  Confidence: band=high; score=0.74
- Line 178: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: claims.push_back(claim);
- Line 296: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: claim.supporting_doc_ids.push_back(doc_id);
  Confidence: band=high; score=0.74
- Line 296: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: claim.supporting_doc_ids.push_back(doc_id);
  Confidence: band=high; score=0.74
- Line 297: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: claim.supporting_doc_ids.push_back(doc_id);
- Line 332: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: explanation << "Claims: " << result.supported_claims_count << "/" << result.total_claims_count << "
- Line 332: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: explanation << "Claims: " << result.supported_claims_count << "/" << result.total_claims_count << "

### src/rag/bayesian_optimizer.cpp
Total findings: 10

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 69: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (objective_value > impl_->best_objective) {
- Line 70: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: impl_->best_objective = objective_value;
- Line 80: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return impl_->best_objective;
- Line 66: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: impl_->observations.push_back(obs);
- Line 75: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> BayesianOptimizer::getBestParams() const {
  Confidence: band=medium; score=0.66
- Line 87: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> BayesianOptimizer::sampleRandom() {
  Confidence: band=medium; score=0.66
- Line 88: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> params;
  Confidence: band=medium; score=0.66
- Line 98: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> BayesianOptimizer::sampleAroundBest() {
  Confidence: band=medium; score=0.66
- Line 99: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> params;
  Confidence: band=medium; score=0.66

### src/rag/citation_highlighter.cpp
Total findings: 10

- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #2611 [WIP] Map answer sentences to source chunks in RAG module (2026-03-12T05:53:09Z)
- Line 114: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: bool isDelim = (cfg.sentence_delimiters.find(ch) != std::string::npos);
- Line 175: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& token : setA) {
  Confidence: band=very_high; score=0.9
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 134: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sentences.push_back(current);
- Line 225: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scored.push_back({ci, sim});
  Confidence: band=high; score=0.74
- Line 226: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: scored.push_back({ci, sim});
- Line 261: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: mapping.secondary_sources.push_back(sec);
  Confidence: band=high; score=0.74
- Line 262: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: mapping.secondary_sources.push_back(sec);
- Line 267: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.mappings.push_back(std::move(mapping));

### src/rag/cot_evaluator.cpp
Total findings: 10

- Line 52: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = 0; i < std::min(documents.size(), size_t(3)); ++i) {
- Line 196: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (conclusion_i.find(neg) != std::string::npos) i_has_negation = true;
- Line 197: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (conclusion_i.find(neg) != std::string::npos) i_has_negation = true;
  Confidence: band=very_high; score=0.9
- Line 198: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (conclusion_j.find(neg) != std::string::npos) j_has_negation = true;
  Confidence: band=very_high; score=0.9
- Line 125: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: steps.push_back(step);
  Confidence: band=high; score=0.74
- Line 126: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back(step);
- Line 142: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back(current_step);
- Line 158: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back(current_step);
- Line 197: severity=MEDIUM; category=expensive_copy
  Description: Unnecessary expensive copy
  Remediation: Use const reference (const T&) or std::move if transfer is needed
  Context: if (conclusion_i.find(neg) != std::string::npos) i_has_negation = true;
- Line 225: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: inconsistencies.push_back(inconsistency.str());

### src/rag/explainability_reason_builder.cpp
Total findings: 10

- Line 161: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: "conditions (miss rate, profile drift, or new entry count).",
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 138: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "Local adapter weights updated; next inference cycle will use "
  Confidence: band=very_high; score=0.9
- Line 147: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "Adapter selected and loaded for inference.",
  Confidence: band=very_high; score=0.9
- Line 354: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(std::move(rec));
  Confidence: band=high; score=0.74
- Line 354: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(std::move(rec));
  Confidence: band=high; score=0.74
- Line 355: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: merged.push_back(std::move(rec));
- Line 361: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(std::move(rec));
  Confidence: band=high; score=0.74
- Line 362: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: merged.push_back(std::move(rec));
- Line 366: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Sort by timestamp (ascending — oldest first)
  Confidence: band=high; score=0.74

### src/rag/relevance_evaluator.cpp
Total findings: 10

- Line 64: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = freq.find(vocab[i]);
- Line 64: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = freq.find(vocab[i]);
- Line 217: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (query_lower.find(kw) != std::string::npos) {
- Line 47: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back(word);
- Line 59: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> freq;
  Confidence: band=medium; score=0.66
- Line 159: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: questions.push_back(question.get<std::string>());
  Confidence: band=high; score=0.74
- Line 160: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: questions.push_back(question.get<std::string>());
- Line 174: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: questions.push_back(answer);
- Line 267: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: irrelevant_segments.push_back(sentence);
  Confidence: band=high; score=0.74
- Line 268: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: irrelevant_segments.push_back(sentence);

### src/rag/nli_faithfulness_verifier.cpp
Total findings: 9

- Line 360: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: void NLIFaithfulnessVerifier::loadModel(const std::string& model_path) {
  Confidence: band=very_high; score=0.99
- Line 81: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: hyp_words.push_back(word);
- Line 182: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: claims.push_back(claim);
  Confidence: band=high; score=0.74
- Line 183: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: claims.push_back(claim);
- Line 278: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: claim_result.supporting_doc_ids.push_back(best_document_id);
  Confidence: band=high; score=0.74
- Line 279: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: claim_result.supporting_doc_ids.push_back(best_document_id);
- Line 283: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: claim_result.supporting_doc_ids.push_back(best_document_id);
- Line 300: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.claims.push_back(claim_result);
  Confidence: band=high; score=0.74
- Line 301: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.claims.push_back(claim_result);

### src/rag/completeness_evaluator.cpp
Total findings: 8

- Line 371: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = 0; i < std::min(result.missing_information.size(), size_t(3)); ++i) {
- Line 49: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: key_terms.push_back(word);
- Line 82: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: key_terms.push_back(word);
- Line 159: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: aspects.push_back(aspect);
  Confidence: band=high; score=0.74
- Line 160: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: aspects.push_back(aspect);
- Line 171: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: aspects.push_back(aspect);
- Line 272: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: missing_info.push_back(aspect.aspect_text);
  Confidence: band=high; score=0.74
- Line 273: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: missing_info.push_back(aspect.aspect_text);

### src/rag/http_metrics_client.cpp
Total findings: 8

- Line 196: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: backoff.wait();
- Line 215: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: backoff.wait();
- Line 55: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Endpoint URL cannot be empty");
- Line 177: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& [key, value] : result->headers) {
  Confidence: band=very_high; score=0.9
- Line 299: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: j["metrics"] = json::array();
- Line 126: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, std::string>& headers) {
  Confidence: band=medium; score=0.66
- Line 317: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j["metrics"].push_back(metric_json);
  Confidence: band=high; score=0.74
- Line 318: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: j["metrics"].push_back(metric_json);

### src/rag/quality_control_factory.cpp
Total findings: 8

- Line 74: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Create LLM Judge Client if inference engine available
  Confidence: band=very_high; score=0.9
- Line 76: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (setup_config.enable_llm_judge && setup_config.inference_engine) {
  Confidence: band=very_high; score=0.9
- Line 77: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm_judge_client = createLLMJudgeClient(setup_config.inference_engine);
  Confidence: band=very_high; score=0.9
- Line 79: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: THEMIS_WARN("LLM Judge requested but no inference engine provided");
  Confidence: band=very_high; score=0.9
- Line 153: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::shared_ptr<llm::InferenceEngineEnhanced> inference_engine
  Confidence: band=very_high; score=0.9
- Line 155: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (!inference_engine) {
  Confidence: band=very_high; score=0.9
- Line 156: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: THEMIS_ERROR("Cannot create LLM Judge Client: inference engine is null");
  Confidence: band=very_high; score=0.9
- Line 168: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: client->setInferenceEngine(std::move(inference_engine));
  Confidence: band=very_high; score=0.9

### src/rag/coherence_evaluator.cpp
Total findings: 6

- Line 319: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (sent_i.find(neg) != std::string::npos) i_has_negation = true;
  Confidence: band=very_high; score=0.9
- Line 320: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (sent_j.find(neg) != std::string::npos) j_has_negation = true;
  Confidence: band=very_high; score=0.9
- Line 293: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sentences.push_back(it->str());
  Confidence: band=high; score=0.74
- Line 294: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sentences.push_back(it->str());
- Line 319: severity=MEDIUM; category=expensive_copy
  Description: Unnecessary expensive copy
  Remediation: Use const reference (const T&) or std::move if transfer is needed
  Context: if (sent_i.find(neg) != std::string::npos) i_has_negation = true;
- Line 344: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: contradictions.push_back(sentences[i] + " <-> " + sentences[j]);

### src/rag/flare_retrieval.cpp
Total findings: 6

- Line 19: severity=CRITICAL; category=audit_logging; pattern=sensitive_data_logging
  Description: Potential PII/credential logging: token
  Context: *  1. Emit token t with log-probability log(p(t)).
  Confidence: band=very_high; score=0.92
- Line 20: severity=CRITICAL; category=audit_logging; pattern=sensitive_data_logging
  Description: Potential PII/credential logging: token
  Context: *  2. If log(p(t)) < confidence_threshold → mark token as uncertain.
  Confidence: band=very_high; score=0.92
- Line 195: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& entry : window_) {
  Confidence: band=very_high; score=0.9
- Line 227: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::fprintf(stderr,
  Confidence: band=very_high; score=0.9
- Line 182: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string FlareRetrieval::buildQuery() const {
  Confidence: band=high; score=0.74
- Line 223: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/rag/lora_enhanced_retriever.cpp
Total findings: 6

- Line 130: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const double lora_s = scorer_->score(query, doc.content, config_.domain);
- Line 154: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: return std::stod(it->second) >= config_.min_lora_score;
- Line 136: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: doc.metadata["lora_score"]  = std::to_string(lora_s);
- Line 48: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: double jaccardTokens(const std::unordered_set<std::string>& A,
  Confidence: band=medium; score=0.66
- Line 49: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string>& B)
  Confidence: band=medium; score=0.66
- Line 155: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/rag/rag_context_assembler.cpp
Total findings: 6

- Line 14: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: * @brief Budget-aware context assembler for RAG inference.
  Confidence: band=very_high; score=0.9
- Line 91: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& c : chunks) ordered.push_back(&c);
- Line 106: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.chunks_used.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 106: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.chunks_used.push_back(chunk);
  Confidence: band=high; score=0.74
- Line 107: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.chunks_used.push_back(chunk);
- Line 116: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.chunks_used.push_back(std::move(truncated));

### src/rag/bias_detector.cpp
Total findings: 5

- Line 133: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scores.push_back(score);
  Confidence: band=high; score=0.74
- Line 134: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: scores.push_back(score);
- Line 135: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: lengths.push_back(static_cast<double>(length));
- Line 175: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: score_length_pairs.emplace_back(eval.overall_score, estimated_length);
  Confidence: band=high; score=0.74
- Line 182: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(length_bias);

### src/rag/document_splitter.cpp
Total findings: 5

- Line 35: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("DocumentSplitterConfig: chunk_size must be > 0");
- Line 38: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 42: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 86: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sentences.emplace_back(sent.substr(ltrim),
  Confidence: band=high; score=0.74
- Line 237: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: chunks.push_back(std::move(chunk));

### src/rag/hallucination_dashboard.cpp
Total findings: 5

- Line 213: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = impl_->window.end() - static_cast<std::ptrdiff_t>(n);
- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #2753 [rag] Hallucination rate tracking dashboard â€” complete INFO tier ... (2026-03-12T05:57
- Line 131: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& e : impl_->window) {
  Confidence: band=very_high; score=0.9
- Line 213: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto it = impl_->window.end() - static_cast<std::ptrdiff_t>(n);
- Line 319: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& alert : snap.active_alerts) {
  Confidence: band=very_high; score=0.9

### src/rag/pairwise_comparator.cpp
Total findings: 5

- Line 296: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: result.overall_confidence = static_cast<double>(a_votes) / impl_->config.num_samples;
- Line 299: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: result.overall_confidence = static_cast<double>(b_votes) / impl_->config.num_samples;
- Line 34: severity=MEDIUM; category=determinism; pattern=random_unseeded
  Description: RNG engine appears default-constructed without explicit seeding
  Context: std::mt19937 rng;
  Confidence: band=high; score=0.74
- Line 279: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(compareWithLLM(query, documents, answer_a, answer_b, a_first));
  Confidence: band=high; score=0.74
- Line 280: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(compareWithLLM(query, documents, answer_a, answer_b, a_first));

### src/rag/rag_ingestion_bridge.cpp
Total findings: 5

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 61: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: .error = "empty input"
  Confidence: band=very_high; score=0.99
- Line 38: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 61: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: .error = "empty input"
  Confidence: band=very_high; score=0.9
- Line 77: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result = engine->execute(ctx);

### src/rag/rubric_evaluator.cpp
Total findings: 5

- Line 65: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = 0; i < std::min(documents.size(), size_t(3)); ++i) {
- Line 139: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dim.levels.push_back(level);
  Confidence: band=high; score=0.74
- Line 139: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dim.levels.push_back(level);
  Confidence: band=high; score=0.74
- Line 140: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: dim.levels.push_back(level);
- Line 144: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: rubric.dimensions.push_back(dim);

### src/rag/tensor_rag_pipeline.cpp
Total findings: 5

- Line 129: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::fprintf(stderr,
  Confidence: band=very_high; score=0.9
- Line 131: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: "threw for FLARE query (len=%zu); embedding left empty "
- Line 131: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function query without trace point
  Context: "threw for FLARE query (len=%zu); embedding left empty "
  Confidence: band=very_high; score=0.9
- Line 125: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 131: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: "threw for FLARE query (len=%zu); embedding left empty "
  Confidence: band=high; score=0.74

### src/rag/judge_config.cpp
Total findings: 4

- Line 168: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("quality_threshold must be in [0, 1], got {}", quality_threshold);
- Line 188: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 200: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 258: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: parts.push_back(part);

### src/rag/learning_metrics.cpp
Total findings: 4

- Line 130: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " +/- " << snap.std_accuracy << "\n";
- Line 130: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " +/- " << snap.std_accuracy << "\n";
- Line 132: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " +/- " << snap.std_faithfulness << "\n";
- Line 132: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " +/- " << snap.std_faithfulness << "\n";

### src/rag/ontology_aware_retriever.cpp
Total findings: 4

- Line 90: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> expanded;
  Confidence: band=medium; score=0.66
- Line 99: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> visited = {concept_id};
  Confidence: band=medium; score=0.66
- Line 111: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: queue.push_back(parent);
  Confidence: band=high; score=0.74
- Line 112: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: queue.push_back(parent);

### src/rag/ab_testing_framework.cpp
Total findings: 3

- Line 88: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (double obs : group.observations) {
  Confidence: band=very_high; score=0.9
- Line 147: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &[test_id, data] : impl_->tests) {
  Confidence: band=very_high; score=0.9
- Line 148: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active.push_back(test_id);
  Confidence: band=high; score=0.74

### src/rag/streaming_retriever.cpp
Total findings: 3

- Line 40: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> tokens;
  Confidence: band=medium; score=0.66
- Line 275: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.skipped_documents.push_back(doc);
  Confidence: band=high; score=0.74
- Line 276: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.skipped_documents.push_back(doc);

### src/rag/targ_retrieval.cpp
Total findings: 3

- Line 84: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 89: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (float v : logits) {
  Confidence: band=very_high; score=0.9
- Line 143: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: if (p > 0.0f) entropy -= p * std::log(p);
  Confidence: band=medium; score=0.6

### src/rag/adaptive_retrieval.cpp
Total findings: 2

- Line 70: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: while (ss >> tok) tokens.push_back(tok);
  Confidence: band=high; score=0.74
- Line 71: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: while (ss >> tok) tokens.push_back(tok);

### src/rag/judge_ensemble.cpp
Total findings: 1

- Line 14: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Deprecated shim: functionality now lives in rag_rag_judge.cpp (JudgeEnsemble implementation).
  Confidence: band=high; score=0.8

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
