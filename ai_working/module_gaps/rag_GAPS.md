# rag Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: rag
- Generated: 2026-06-02 12:40:50
- Status: Critical Findings Present
- Total Findings: 1335
- Actionable Findings (Critical + High): 894
- Affected Files: 65

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 334 |
| High | 560 |
| Medium | 436 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| llm_ai_safety | 673 |
| performance_patterns | 209 |
| container | 97 |
| platform | 71 |
| audit_logging | 56 |
| determinism | 55 |
| concurrency | 45 |
| reliability | 42 |
| exception_safety | 30 |
| performance | 28 |
| memory | 15 |
| observability | 12 |
| security | 8 |
| raii | 5 |
| uninitialized | 4 |
| legacy_duplication | 3 |
| distributed_consistency | 2 |
| type_conversion | 2 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/rag/rag_judge.cpp | 192 | 84 | 89 | 19 | 0 |
| src/rag/batch_evaluator.cpp | 132 | 51 | 74 | 7 | 0 |
| src/rag/adversarial_tester.cpp | 97 | 46 | 40 | 11 | 0 |
| src/rag/evaluation_report_exporter.cpp | 78 | 2 | 5 | 71 | 0 |
| src/rag/knowledge_gap_detector.cpp | 60 | 2 | 28 | 28 | 2 |
| src/rag/dpr_vectorizer.cpp | 58 | 28 | 25 | 5 | 0 |
| src/rag/distributed_rag_evaluator.cpp | 37 | 9 | 21 | 7 | 0 |
| src/rag/document_summarizer.cpp | 34 | 7 | 10 | 17 | 0 |
| src/rag/llm_judge_client.cpp | 34 | 3 | 26 | 5 | 0 |
| src/rag/llm_meta_analyzer.cpp | 34 | 15 | 17 | 2 | 0 |
| src/rag/multi_step_rag.cpp | 34 | 2 | 16 | 16 | 0 |
| src/rag/knowledge_graph_retriever.cpp | 33 | 1 | 14 | 18 | 0 |
| src/rag/llm_judge_integration.cpp | 32 | 2 | 28 | 2 | 0 |
| src/rag/continuous_learning_orchestrator.cpp | 30 | 6 | 12 | 12 | 0 |
| src/rag/llm_integration.cpp | 24 | 0 | 16 | 7 | 1 |
| src/rag/reranker.cpp | 24 | 9 | 7 | 8 | 0 |
| src/rag/prompt_injection_detector.cpp | 21 | 4 | 6 | 11 | 0 |
| src/rag/delegate_evaluator.cpp | 20 | 4 | 9 | 7 | 0 |
| src/rag/rlaif_trainer.cpp | 20 | 7 | 5 | 8 | 0 |
| src/rag/multi_hop_reasoner.cpp | 19 | 0 | 11 | 8 | 0 |
| src/rag/prompt_templates.cpp | 19 | 9 | 9 | 1 | 0 |
| src/rag/fairness_detector.cpp | 18 | 1 | 6 | 11 | 0 |
| src/rag/calibration_manager.cpp | 17 | 5 | 4 | 7 | 1 |
| src/rag/agentic_rag.cpp | 15 | 2 | 5 | 8 | 0 |
| src/rag/continuous_learning_client.cpp | 15 | 2 | 5 | 8 | 0 |
| src/rag/multimodal_rag.cpp | 15 | 4 | 1 | 10 | 0 |
| src/rag/geval_evaluator.cpp | 13 | 1 | 5 | 7 | 0 |
| src/rag/quality_control_pipeline.cpp | 13 | 3 | 1 | 9 | 0 |
| src/rag/response_parser.cpp | 13 | 0 | 4 | 9 | 0 |
| src/rag/claim_extractor.cpp | 10 | 0 | 3 | 7 | 0 |
| src/rag/evaluation_cache.cpp | 10 | 5 | 5 | 0 | 0 |
| src/rag/onnx_model_loader.cpp | 10 | 4 | 4 | 2 | 0 |
| src/rag/replug_retriever.cpp | 9 | 0 | 5 | 4 | 0 |
| src/rag/explainability_reason_builder.cpp | 8 | 1 | 3 | 4 | 0 |
| src/rag/quality_control_factory.cpp | 8 | 0 | 8 | 0 | 0 |
| src/rag/rag_ingestion_bridge.cpp | 8 | 3 | 4 | 1 | 0 |
| src/rag/cot_evaluator.cpp | 7 | 0 | 4 | 3 | 0 |
| src/rag/faithfulness_evaluator.cpp | 7 | 1 | 0 | 6 | 0 |
| src/rag/hybrid_retriever.cpp | 7 | 0 | 2 | 5 | 0 |
| src/rag/relevance_evaluator.cpp | 7 | 0 | 3 | 4 | 0 |
| src/rag/self_rag.cpp | 7 | 0 | 0 | 7 | 0 |
| src/rag/bayesian_optimizer.cpp | 6 | 0 | 1 | 5 | 0 |
| src/rag/coherence_evaluator.cpp | 6 | 0 | 2 | 4 | 0 |
| src/rag/examples/loop_orchestration_example.cpp | 6 | 0 | 2 | 4 | 0 |
| src/rag/lora_enhanced_retriever.cpp | 6 | 2 | 1 | 3 | 0 |
| src/rag/rag_context_assembler.cpp | 6 | 2 | 3 | 1 | 0 |
| src/rag/citation_highlighter.cpp | 5 | 0 | 2 | 3 | 0 |
| src/rag/ontology_aware_retriever.cpp | 5 | 0 | 2 | 3 | 0 |
| src/rag/pairwise_comparator.cpp | 5 | 2 | 0 | 3 | 0 |
| src/rag/completeness_evaluator.cpp | 4 | 0 | 1 | 3 | 0 |
| src/rag/flare_retrieval.cpp | 4 | 2 | 0 | 2 | 0 |
| src/rag/http_metrics_client.cpp | 4 | 2 | 0 | 2 | 0 |
| src/rag/learning_metrics.cpp | 4 | 0 | 0 | 4 | 0 |
| src/rag/nli_faithfulness_verifier.cpp | 4 | 1 | 0 | 3 | 0 |
| src/rag/tensor_rag_pipeline.cpp | 4 | 0 | 2 | 2 | 0 |
| src/rag/bias_detector.cpp | 3 | 0 | 0 | 3 | 0 |
| src/rag/judge_config.cpp | 3 | 0 | 1 | 2 | 0 |
| src/rag/rubric_evaluator.cpp | 3 | 0 | 1 | 2 | 0 |
| src/rag/streaming_retriever.cpp | 2 | 0 | 0 | 2 | 0 |
| src/rag/ab_testing_framework.cpp | 1 | 0 | 0 | 1 | 0 |
| src/rag/adaptive_retrieval.cpp | 1 | 0 | 0 | 1 | 0 |
| src/rag/document_splitter.cpp | 1 | 0 | 0 | 1 | 0 |
| src/rag/hallucination_dashboard.cpp | 1 | 0 | 1 | 0 | 0 |
| src/rag/judge_ensemble.cpp | 1 | 0 | 1 | 0 | 0 |
| src/rag/targ_retrieval.cpp | 1 | 0 | 0 | 0 | 1 |

## Full Scanner Findings

### src/rag/rag_judge.cpp
Total findings: 192

- Line 119: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->llm_judge_client = std::make_shared<LLMJudgeClient>(client_config);
- Line 122: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->nli_verifier = std::make_shared<NLIFaithfulnessVerifier>();
- Line 158: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationInput input;
  Confidence: band=very_high; score=0.99
- Line 159: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.query = query;
  Confidence: band=very_high; score=0.99
- Line 160: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.documents = documents;
  Confidence: band=very_high; score=0.99
- Line 161: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.generated_answer = generated_answer;
  Confidence: band=very_high; score=0.99
- Line 162: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return evaluate(input);
  Confidence: band=very_high; score=0.99
- Line 171: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationInput input;
  Confidence: band=very_high; score=0.99
- Line 172: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.query = query;
  Confidence: band=very_high; score=0.99
- Line 173: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.documents = documents;
  Confidence: band=very_high; score=0.99
- Line 174: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.generated_answer = generated_answer;
  Confidence: band=very_high; score=0.99
- Line 179: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto result = evaluate(input);
  Confidence: band=very_high; score=0.99
- Line 187: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationResult RAGJudge::evaluate(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.99
- Line 190: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: THEMIS_DEBUG("Evaluating RAG output for query: {}", input.query);
  Confidence: band=very_high; score=0.99
- Line 195: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto cache_key = impl_->computeCacheKey(input.query, input.generated_answer, input.tenant_id);
- Line 195: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto cache_key = impl_->computeCacheKey(input.query, input.generated_answer, input.tenant_id);
  Confidence: band=very_high; score=0.99
- Line 196: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto it = impl_->cache.find(cache_key);
- Line 197: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != impl_->cache.end()) {
- Line 209: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: !input.documents.empty()) {
  Confidence: band=very_high; score=0.99
- Line 211: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto scan_results = impl_->injection_detector->scanDocuments(input);
  Confidence: band=very_high; score=0.99
- Line 245: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: total_findings, input.query);
  Confidence: band=very_high; score=0.99
- Line 264: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: total_findings, input.query);
  Confidence: band=very_high; score=0.99
- Line 269: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationInput safe_input = input;
  Confidence: band=very_high; score=0.99
- Line 305: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "relevance", [&]() { return evaluateRelevance(safe_input); }, 0.0);
  Confidence: band=very_high; score=0.99
- Line 312: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "faithfulness", [&]() { return evaluateFaithfulness(safe_input); }, 0.0);
  Confidence: band=very_high; score=0.99
- Line 314: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "relevance", [&]() { return evaluateRelevance(safe_input); }, 0.0);
  Confidence: band=very_high; score=0.99
- Line 316: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "completeness", [&]() { return evaluateCompleteness(safe_input); }, 0.0);
  Confidence: band=very_high; score=0.99
- Line 318: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "coherence", [&]() { return evaluateCoherence(safe_input); }, 0.0);
  Confidence: band=very_high; score=0.99
- Line 323: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "ethical_compliance", [&]() { return evaluateEthicalCompliance(safe_input); }, 0.0);
  Confidence: band=very_high; score=0.99
- Line 339: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "faithfulness", [&]() { return evaluateFaithfulness(safe_input); }, 0.0);
  Confidence: band=very_high; score=0.99
- Line 341: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "relevance", [&]() { return evaluateRelevance(safe_input); }, 0.0);
  Confidence: band=very_high; score=0.99
- Line 343: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "completeness", [&]() { return evaluateCompleteness(safe_input); }, 0.0);
  Confidence: band=very_high; score=0.99
- Line 345: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "coherence", [&]() { return evaluateCoherence(safe_input); }, 0.0);
  Confidence: band=very_high; score=0.99
- Line 350: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "ethical_compliance", [&]() { return evaluateEthicalCompliance(safe_input); }, 0.0);
  Confidence: band=very_high; score=0.99
- Line 359: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto claims = extractClaims(safe_input.generated_answer);
  Confidence: band=very_high; score=0.99
- Line 489: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto cache_key = impl_->computeCacheKey(input.query, input.generated_answer, input.tenant_id);
- Line 489: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto cache_key = impl_->computeCacheKey(input.query, input.generated_answer, input.tenant_id);
  Confidence: band=very_high; score=0.99
- Line 490: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->cache[cache_key] = result;
- Line 499: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: safe_input.generated_answer.size());
  Confidence: band=very_high; score=0.99
- Line 569: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationInput input;
  Confidence: band=very_high; score=0.99
- Line 570: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.query = test_case.query;
  Confidence: band=very_high; score=0.99
- Line 571: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.documents = test_case.documents;
  Confidence: band=very_high; score=0.99
- Line 572: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.generated_answer = test_case.generated_answer;
  Confidence: band=very_high; score=0.99
- Line 574: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto result = evaluate(input);
  Confidence: band=very_high; score=0.99
- Line 583: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const EvaluationInput& input
  Confidence: band=very_high; score=0.99
- Line 587: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return evaluateFaithfulness(input);
  Confidence: band=very_high; score=0.99
- Line 589: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return evaluateRelevance(input);
  Confidence: band=very_high; score=0.99
- Line 591: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return evaluateCompleteness(input);
  Confidence: band=very_high; score=0.99
- Line 593: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return evaluateCoherence(input);
  Confidence: band=very_high; score=0.99
- Line 595: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return evaluateEthicalCompliance(input);
  Confidence: band=very_high; score=0.99
- Line 597: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return evaluate(input).overall_score;
  Confidence: band=very_high; score=0.99
- Line 648: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: double RAGJudge::evaluateFaithfulness(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.99
- Line 652: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (input.documents.empty()) {
  Confidence: band=very_high; score=0.99
- Line 659: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (const auto& doc : input.documents) {
  Confidence: band=very_high; score=0.99
- Line 665: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.generated_answer,
  Confidence: band=very_high; score=0.99
- Line 667: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.query
  Confidence: band=very_high; score=0.99
- Line 676: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: double RAGJudge::evaluateRelevance(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.99
- Line 679: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (input.generated_answer.empty()) {
  Confidence: band=very_high; score=0.99
- Line 685: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.generated_answer,
  Confidence: band=very_high; score=0.99
- Line 686: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.query
  Confidence: band=very_high; score=0.99
- Line 695: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (const auto& doc : input.documents) {
  Confidence: band=very_high; score=0.99
- Line 716: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: double RAGJudge::evaluateCompleteness(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.99
- Line 721: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.generated_answer,
  Confidence: band=very_high; score=0.99
- Line 722: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.query
  Confidence: band=very_high; score=0.99
- Line 731: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: double RAGJudge::evaluateCoherence(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.99
- Line 735: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto result = impl_->coherence_eval->evaluate(input.generated_answer);
  Confidence: band=very_high; score=0.99
- Line 743: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: double RAGJudge::evaluateEthicalCompliance(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.99
- Line 747: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: double autonomy_score = evaluateAutonomyRespect(input);
  Confidence: band=very_high; score=0.99
- Line 748: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: double diversity_score = evaluateMoralDiversity(input);
  Confidence: band=very_high; score=0.99
- Line 749: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: double citation_score = evaluateCitationQuality(input);
  Confidence: band=very_high; score=0.99
- Line 763: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: double RAGJudge::evaluateAutonomyRespect(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.99
- Line 767: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (detectPatronizingLanguage(input.generated_answer)) {
  Confidence: band=very_high; score=0.99
- Line 773: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (!checkChoicePreservation(input.generated_answer)) {
  Confidence: band=very_high; score=0.99
- Line 779: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: int perspectives = countMoralPerspectives(input.generated_answer);
  Confidence: band=very_high; score=0.99
- Line 788: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: double RAGJudge::evaluateMoralDiversity(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.99
- Line 792: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: int perspectives = countMoralPerspectives(input.generated_answer);
  Confidence: band=very_high; score=0.99
- Line 801: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (detectBias(input.generated_answer)) {
  Confidence: band=very_high; score=0.99
- Line 809: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: double RAGJudge::evaluateCitationQuality(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.99
- Line 811: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: bool has_citations = hasEthicalCitations(input.generated_answer);
  Confidence: band=very_high; score=0.99
- Line 819: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto claims = extractClaims(input.generated_answer);
  Confidence: band=very_high; score=0.99
- Line 1231: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const EvaluationInput& input,
  Confidence: band=very_high; score=0.99
- Line 1234: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return impl_->template_manager.generatePrompt(dimension, input);
  Confidence: band=very_high; score=0.99
- Line 1266: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationResult JudgeEnsemble::evaluateWithEnsemble(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.99
- Line 1271: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: results.push_back(judge->evaluate(input));
  Confidence: band=very_high; score=0.99
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #651 [RAG-ETHICS] Add ethical co... (2026-03-11) | #650 Complete RAG Enhancem
- Line 49: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Enhanced LLM Judge Client (connects to InferenceEngineEnhanced)
  Confidence: band=very_high; score=0.9
- Line 158: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationInput input;
  Confidence: band=very_high; score=0.9
- Line 159: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.query = query;
  Confidence: band=very_high; score=0.9
- Line 160: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.documents = documents;
  Confidence: band=very_high; score=0.9
- Line 161: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.generated_answer = generated_answer;
  Confidence: band=very_high; score=0.9
- Line 161: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: input.generated_answer = generated_answer;
  Confidence: band=very_high; score=0.9
- Line 162: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return evaluate(input);
  Confidence: band=very_high; score=0.9
- Line 171: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationInput input;
  Confidence: band=very_high; score=0.9
- Line 172: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.query = query;
  Confidence: band=very_high; score=0.9
- Line 173: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.documents = documents;
  Confidence: band=very_high; score=0.9
- Line 174: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.generated_answer = generated_answer;
  Confidence: band=very_high; score=0.9
- Line 174: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: input.generated_answer = generated_answer;
  Confidence: band=very_high; score=0.9
- Line 179: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto result = evaluate(input);
  Confidence: band=very_high; score=0.9
- Line 187: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationResult RAGJudge::evaluate(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.9
- Line 190: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: THEMIS_DEBUG("Evaluating RAG output for query: {}", input.query);
  Confidence: band=very_high; score=0.9
- Line 195: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto cache_key = impl_->computeCacheKey(input.query, input.generated_answer, input.tenant_id);
  Confidence: band=very_high; score=0.9
- Line 209: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: !input.documents.empty()) {
  Confidence: band=very_high; score=0.9
- Line 211: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto scan_results = impl_->injection_detector->scanDocuments(input);
  Confidence: band=very_high; score=0.9
- Line 245: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: total_findings, input.query);
  Confidence: band=very_high; score=0.9
- Line 264: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: total_findings, input.query);
  Confidence: band=very_high; score=0.9
- Line 269: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationInput safe_input = input;
  Confidence: band=very_high; score=0.9
- Line 276: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: safe_input.generated_answer = impl_->injection_sanitizer->sanitize(input.generated_answer);
  Confidence: band=very_high; score=0.9
- Line 305: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "relevance", [&]() { return evaluateRelevance(safe_input); }, 0.0);
  Confidence: band=very_high; score=0.9
- Line 312: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "faithfulness", [&]() { return evaluateFaithfulness(safe_input); }, 0.0);
  Confidence: band=very_high; score=0.9
- Line 314: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "relevance", [&]() { return evaluateRelevance(safe_input); }, 0.0);
  Confidence: band=very_high; score=0.9
- Line 316: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "completeness", [&]() { return evaluateCompleteness(safe_input); }, 0.0);
  Confidence: band=very_high; score=0.9
- Line 318: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "coherence", [&]() { return evaluateCoherence(safe_input); }, 0.0);
  Confidence: band=very_high; score=0.9
- Line 323: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "ethical_compliance", [&]() { return evaluateEthicalCompliance(safe_input); }, 0.0);
  Confidence: band=very_high; score=0.9
- Line 339: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "faithfulness", [&]() { return evaluateFaithfulness(safe_input); }, 0.0);
  Confidence: band=very_high; score=0.9
- Line 341: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "relevance", [&]() { return evaluateRelevance(safe_input); }, 0.0);
  Confidence: band=very_high; score=0.9
- Line 343: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "completeness", [&]() { return evaluateCompleteness(safe_input); }, 0.0);
  Confidence: band=very_high; score=0.9
- Line 345: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "coherence", [&]() { return evaluateCoherence(safe_input); }, 0.0);
  Confidence: band=very_high; score=0.9
- Line 350: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "ethical_compliance", [&]() { return evaluateEthicalCompliance(safe_input); }, 0.0);
  Confidence: band=very_high; score=0.9
- Line 359: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto claims = extractClaims(safe_input.generated_answer);
  Confidence: band=very_high; score=0.9
- Line 364: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: [&]() { return verifyClaimAgainstDocuments(claim, safe_input.documents) ? 1.0 : 0.0; },
  Confidence: band=very_high; score=0.9
- Line 489: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto cache_key = impl_->computeCacheKey(input.query, input.generated_answer, input.tenant_id);
  Confidence: band=very_high; score=0.9
- Line 499: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: safe_input.generated_answer.size());
  Confidence: band=very_high; score=0.9
- Line 569: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationInput input;
  Confidence: band=very_high; score=0.9
- Line 570: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.query = test_case.query;
  Confidence: band=very_high; score=0.9
- Line 571: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.documents = test_case.documents;
  Confidence: band=very_high; score=0.9
- Line 572: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.generated_answer = test_case.generated_answer;
  Confidence: band=very_high; score=0.9
- Line 572: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: input.generated_answer = test_case.generated_answer;
  Confidence: band=very_high; score=0.9
- Line 574: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto result = evaluate(input);
  Confidence: band=very_high; score=0.9
- Line 583: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const EvaluationInput& input
  Confidence: band=very_high; score=0.9
- Line 587: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return evaluateFaithfulness(input);
  Confidence: band=very_high; score=0.9
- Line 589: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return evaluateRelevance(input);
  Confidence: band=very_high; score=0.9
- Line 591: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return evaluateCompleteness(input);
  Confidence: band=very_high; score=0.9
- Line 593: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return evaluateCoherence(input);
  Confidence: band=very_high; score=0.9
- Line 595: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return evaluateEthicalCompliance(input);
  Confidence: band=very_high; score=0.9
- Line 597: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return evaluate(input).overall_score;
  Confidence: band=very_high; score=0.9
- Line 648: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: double RAGJudge::evaluateFaithfulness(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.9
- Line 652: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (input.documents.empty()) {
  Confidence: band=very_high; score=0.9
- Line 659: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: for (const auto& doc : input.documents) {
  Confidence: band=very_high; score=0.9
- Line 665: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.generated_answer,
  Confidence: band=very_high; score=0.9
- Line 667: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.query
  Confidence: band=very_high; score=0.9
- Line 676: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: double RAGJudge::evaluateRelevance(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.9
- Line 679: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (input.generated_answer.empty()) {
  Confidence: band=very_high; score=0.9
- Line 685: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.generated_answer,
  Confidence: band=very_high; score=0.9
- Line 686: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.query
  Confidence: band=very_high; score=0.9
- Line 695: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: for (const auto& doc : input.documents) {
  Confidence: band=very_high; score=0.9
- Line 716: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: double RAGJudge::evaluateCompleteness(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.9
- Line 721: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.generated_answer,
  Confidence: band=very_high; score=0.9
- Line 722: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.query
  Confidence: band=very_high; score=0.9
- Line 731: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: double RAGJudge::evaluateCoherence(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.9
- Line 735: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto result = impl_->coherence_eval->evaluate(input.generated_answer);
  Confidence: band=very_high; score=0.9
- Line 735: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto result = impl_->coherence_eval->evaluate(input.generated_answer);
  Confidence: band=very_high; score=0.9
- Line 743: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: double RAGJudge::evaluateEthicalCompliance(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.9
- Line 747: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: double autonomy_score = evaluateAutonomyRespect(input);
  Confidence: band=very_high; score=0.9
- Line 748: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: double diversity_score = evaluateMoralDiversity(input);
  Confidence: band=very_high; score=0.9
- Line 749: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: double citation_score = evaluateCitationQuality(input);
  Confidence: band=very_high; score=0.9
- Line 763: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: double RAGJudge::evaluateAutonomyRespect(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.9
- Line 767: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (detectPatronizingLanguage(input.generated_answer)) {
  Confidence: band=very_high; score=0.9
- Line 773: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (!checkChoicePreservation(input.generated_answer)) {
  Confidence: band=very_high; score=0.9
- Line 779: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: int perspectives = countMoralPerspectives(input.generated_answer);
  Confidence: band=very_high; score=0.9
- Line 788: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: double RAGJudge::evaluateMoralDiversity(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.9
- Line 792: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: int perspectives = countMoralPerspectives(input.generated_answer);
  Confidence: band=very_high; score=0.9
- Line 801: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (detectBias(input.generated_answer)) {
  Confidence: band=very_high; score=0.9
- Line 809: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: double RAGJudge::evaluateCitationQuality(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.9
- Line 811: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: bool has_citations = hasEthicalCitations(input.generated_answer);
  Confidence: band=very_high; score=0.9
- Line 819: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto claims = extractClaims(input.generated_answer);
  Confidence: band=very_high; score=0.9
- Line 966: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: "[",  // Citation markers like [1], [UN Declaration]
- Line 1063: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (trimmed.find(phrase) != std::string::npos) {
- Line 1064: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (trimmed.find(phrase) != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 1218: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (doc.content.find(claim) != std::string::npos) {
- Line 1231: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const EvaluationInput& input,
  Confidence: band=very_high; score=0.9
- Line 1234: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return impl_->template_manager.generatePrompt(dimension, input);
  Confidence: band=very_high; score=0.9
- Line 1266: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationResult JudgeEnsemble::evaluateWithEnsemble(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.9
- Line 1271: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: results.push_back(judge->evaluate(input));
  Confidence: band=very_high; score=0.9
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 294: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 366: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.verified_claims.push_back(claim);
  Confidence: band=high; score=0.74
- Line 381: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 449: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: dim_scores.push_back(result.faithfulness_score);
- Line 450: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: dim_scores.push_back(result.completeness_score);
- Line 451: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: dim_scores.push_back(result.coherence_score);
- Line 453: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: dim_scores.push_back(result.ethical_compliance_score);
- Line 455: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: dim_scores.push_back(result.relevance_score);  // always evaluated
- Line 574: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(result);
  Confidence: band=high; score=0.74
- Line 659: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: doc_pairs.emplace_back(doc.id, doc.content);
  Confidence: band=high; score=0.74
- Line 1035: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: claims.push_back(std::move(text));
  Confidence: band=high; score=0.74
- Line 1071: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: claims.push_back(trimmed);
  Confidence: band=high; score=0.74
- Line 1071: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: claims.push_back(trimmed);
  Confidence: band=high; score=0.74
- Line 1114: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> set2(terms2.begin(), terms2.end());
  Confidence: band=medium; score=0.66
- Line 1270: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(judge->evaluate(input));
  Confidence: band=high; score=0.74
- Line 1412: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: judges.push_back(std::make_shared<RAGJudge>());
  Confidence: band=high; score=0.74
- Line 1413: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: judges.push_back(std::make_shared<RAGJudge>());

### src/rag/batch_evaluator.cpp
Total findings: 132

- Line 84: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: bool metadataHasPromptInjectionScenario(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.99
- Line 85: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto it = input.metadata.find("attack_type");
  Confidence: band=very_high; score=0.99
- Line 86: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (it != input.metadata.end() && iequals(it->second, "prompt_injection")) {
  Confidence: band=very_high; score=0.99
- Line 89: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: it = input.metadata.find("scenario");
  Confidence: band=very_high; score=0.99
- Line 90: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (it != input.metadata.end() && iequals(it->second, "prompt_injection")) {
  Confidence: band=very_high; score=0.99
- Line 96: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: bool hasDecisionTraceability(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.99
- Line 98: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.metadata.find("model_version") != input.metadata.end();
  Confidence: band=very_high; score=0.99
- Line 100: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.metadata.find("guardrail_decision") != input.metadata.end();
  Confidence: band=very_high; score=0.99
- Line 102: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.metadata.find("context_id") != input.metadata.end() ||
  Confidence: band=very_high; score=0.99
- Line 103: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.metadata.find("retrieval_context_id") != input.metadata.end();
  Confidence: band=very_high; score=0.99
- Line 107: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: double extractLatencyMs(const EvaluationInput& input, const EvaluationResult& result) {
  Confidence: band=very_high; score=0.99
- Line 108: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto it = input.metadata.find("latency_ms");
  Confidence: band=very_high; score=0.99
- Line 109: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (it != input.metadata.end()) {
  Confidence: band=very_high; score=0.99
- Line 118: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: double extractCost(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.99
- Line 126: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto it = input.metadata.find(key);
  Confidence: band=very_high; score=0.99
- Line 127: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (it == input.metadata.end()) {
  Confidence: band=very_high; score=0.99
- Line 214: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: queue_cv_.wait(lock, [this] {
- Line 230: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto result = processEvaluation(item.input);
  Confidence: band=very_high; score=0.99
- Line 254: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationResult BatchEvaluator::processEvaluation(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.99
- Line 255: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationInput safe_input = input;
  Confidence: band=very_high; score=0.99
- Line 288: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::vector<EvaluationInput> inputs;
  Confidence: band=very_high; score=0.99
- Line 289: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: inputs.reserve(test_cases.size());
  Confidence: band=very_high; score=0.99
- Line 291: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationInput in;
  Confidence: band=very_high; score=0.99
- Line 295: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: inputs.push_back(std::move(in));
  Confidence: band=very_high; score=0.99
- Line 297: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return evaluateBatch(inputs);
  Confidence: band=very_high; score=0.99
- Line 301: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const std::vector<EvaluationInput>& inputs) {
  Confidence: band=very_high; score=0.99
- Line 305: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: results.reserve(inputs.size());
  Confidence: band=very_high; score=0.99
- Line 310: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (const auto& input : inputs) {
  Confidence: band=very_high; score=0.99
- Line 312: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: results.push_back(processEvaluation(input));
  Confidence: band=very_high; score=0.99
- Line 317: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: progress.total_items     = inputs.size();
  Confidence: band=very_high; score=0.99
- Line 322: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: static_cast<double>(inputs.size());
  Confidence: band=very_high; score=0.99
- Line 341: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: out.progress.total_items = inputs.size();
  Confidence: band=very_high; score=0.99
- Line 343: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (inputs.empty() || results.empty()) {
  Confidence: band=very_high; score=0.99
- Line 361: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (size_t i = 0; i < results.size() && i < inputs.size(); ++i) {
  Confidence: band=very_high; score=0.99
- Line 362: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const auto& input = inputs[i];
  Confidence: band=very_high; score=0.99
- Line 381: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (metadataHasPromptInjectionScenario(input)) {
  Confidence: band=very_high; score=0.99
- Line 402: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (!input.documents.empty()) {
  Confidence: band=very_high; score=0.99
- Line 403: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const auto scan_results = inline_detector.scanDocuments(input);
  Confidence: band=very_high; score=0.99
- Line 419: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto it = input.metadata.find("attack_succeeded");
  Confidence: band=very_high; score=0.99
- Line 420: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (it != input.metadata.end()) {
  Confidence: band=very_high; score=0.99
- Line 439: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (hasDecisionTraceability(input)) {
  Confidence: band=very_high; score=0.99
- Line 443: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: latencies_ms.push_back(extractLatencyMs(input, result));
  Confidence: band=very_high; score=0.99
- Line 444: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: total_cost += extractCost(input);
  Confidence: band=very_high; score=0.99
- Line 523: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const EvaluationInput& input) {
  Confidence: band=very_high; score=0.99
- Line 533: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: item.input    = input;
  Confidence: band=very_high; score=0.99
- Line 544: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const std::vector<EvaluationInput>& inputs) {
  Confidence: band=very_high; score=0.99
- Line 546: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: handles.reserve(inputs.size());
  Confidence: band=very_high; score=0.99
- Line 547: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (const auto& input : inputs) {
  Confidence: band=very_high; score=0.99
- Line 548: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: handles.push_back(evaluateAsync(input));
  Confidence: band=very_high; score=0.99
- Line 558: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const EvaluationInput& input,
  Confidence: band=very_high; score=0.99
- Line 562: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: item.input    = input;
  Confidence: band=very_high; score=0.99
- Line 84: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: bool metadataHasPromptInjectionScenario(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.9
- Line 85: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto it = input.metadata.find("attack_type");
  Confidence: band=very_high; score=0.9
- Line 86: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (it != input.metadata.end() && iequals(it->second, "prompt_injection")) {
  Confidence: band=very_high; score=0.9
- Line 89: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: it = input.metadata.find("scenario");
  Confidence: band=very_high; score=0.9
- Line 90: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (it != input.metadata.end() && iequals(it->second, "prompt_injection")) {
  Confidence: band=very_high; score=0.9
- Line 96: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: bool hasDecisionTraceability(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.9
- Line 98: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.metadata.find("model_version") != input.metadata.end();
  Confidence: band=very_high; score=0.9
- Line 100: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.metadata.find("guardrail_decision") != input.metadata.end();
  Confidence: band=very_high; score=0.9
- Line 102: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.metadata.find("context_id") != input.metadata.end() ||
  Confidence: band=very_high; score=0.9
- Line 103: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.metadata.find("retrieval_context_id") != input.metadata.end();
  Confidence: band=very_high; score=0.9
- Line 107: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: double extractLatencyMs(const EvaluationInput& input, const EvaluationResult& result) {
  Confidence: band=very_high; score=0.9
- Line 108: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto it = input.metadata.find("latency_ms");
  Confidence: band=very_high; score=0.9
- Line 109: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (it != input.metadata.end()) {
  Confidence: band=very_high; score=0.9
- Line 118: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: double extractCost(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.9
- Line 126: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto it = input.metadata.find(key);
  Confidence: band=very_high; score=0.9
- Line 127: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (it == input.metadata.end()) {
  Confidence: band=very_high; score=0.9
- Line 147: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: return future_.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
- Line 212: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lock(queue_mutex_);
- Line 230: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto result = processEvaluation(item.input);
  Confidence: band=very_high; score=0.9
- Line 254: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationResult BatchEvaluator::processEvaluation(const EvaluationInput& input) {
  Confidence: band=very_high; score=0.9
- Line 255: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationInput safe_input = input;
  Confidence: band=very_high; score=0.9
- Line 260: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: safe_input.generated_answer = sanitizer.sanitize(input.generated_answer);
  Confidence: band=very_high; score=0.9
- Line 288: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::vector<EvaluationInput> inputs;
  Confidence: band=very_high; score=0.9
- Line 288: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::vector<EvaluationInput> inputs;
  Confidence: band=very_high; score=0.9
- Line 289: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: inputs.reserve(test_cases.size());
  Confidence: band=very_high; score=0.9
- Line 289: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: inputs.reserve(test_cases.size());
  Confidence: band=very_high; score=0.9
- Line 291: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationInput in;
  Confidence: band=very_high; score=0.9
- Line 294: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: in.generated_answer = tc.generated_answer;
  Confidence: band=very_high; score=0.9
- Line 295: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: inputs.push_back(std::move(in));
  Confidence: band=very_high; score=0.9
- Line 295: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: inputs.push_back(std::move(in));
  Confidence: band=very_high; score=0.9
- Line 297: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: return evaluateBatch(inputs);
  Confidence: band=very_high; score=0.9
- Line 297: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return evaluateBatch(inputs);
  Confidence: band=very_high; score=0.9
- Line 301: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: const std::vector<EvaluationInput>& inputs) {
  Confidence: band=very_high; score=0.9
- Line 301: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const std::vector<EvaluationInput>& inputs) {
  Confidence: band=very_high; score=0.9
- Line 305: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: results.reserve(inputs.size());
  Confidence: band=very_high; score=0.9
- Line 305: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: results.reserve(inputs.size());
  Confidence: band=very_high; score=0.9
- Line 310: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (const auto& input : inputs) {
  Confidence: band=very_high; score=0.9
- Line 310: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: for (const auto& input : inputs) {
  Confidence: band=very_high; score=0.9
- Line 312: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: results.push_back(processEvaluation(input));
  Confidence: band=very_high; score=0.9
- Line 317: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: progress.total_items     = inputs.size();
  Confidence: band=very_high; score=0.9
- Line 317: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: progress.total_items     = inputs.size();
  Confidence: band=very_high; score=0.9
- Line 322: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: static_cast<double>(inputs.size());
  Confidence: band=very_high; score=0.9
- Line 322: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: static_cast<double>(inputs.size());
  Confidence: band=very_high; score=0.9
- Line 341: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: out.progress.total_items = inputs.size();
  Confidence: band=very_high; score=0.9
- Line 341: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: out.progress.total_items = inputs.size();
  Confidence: band=very_high; score=0.9
- Line 343: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: if (inputs.empty() || results.empty()) {
  Confidence: band=very_high; score=0.9
- Line 343: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (inputs.empty() || results.empty()) {
  Confidence: band=very_high; score=0.9
- Line 361: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (size_t i = 0; i < results.size() && i < inputs.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 361: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: for (size_t i = 0; i < results.size() && i < inputs.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 362: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: const auto& input = inputs[i];
  Confidence: band=very_high; score=0.9
- Line 362: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const auto& input = inputs[i];
  Confidence: band=very_high; score=0.9
- Line 381: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (metadataHasPromptInjectionScenario(input)) {
  Confidence: band=very_high; score=0.9
- Line 402: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (!input.documents.empty()) {
  Confidence: band=very_high; score=0.9
- Line 403: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const auto scan_results = inline_detector.scanDocuments(input);
  Confidence: band=very_high; score=0.9
- Line 419: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto it = input.metadata.find("attack_succeeded");
  Confidence: band=very_high; score=0.9
- Line 420: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (it != input.metadata.end()) {
  Confidence: band=very_high; score=0.9
- Line 439: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (hasDecisionTraceability(input)) {
  Confidence: band=very_high; score=0.9
- Line 443: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: latencies_ms.push_back(extractLatencyMs(input, result));
  Confidence: band=very_high; score=0.9
- Line 444: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: total_cost += extractCost(input);
  Confidence: band=very_high; score=0.9
- Line 523: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const EvaluationInput& input) {
  Confidence: band=very_high; score=0.9
- Line 525: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->cancelled_.store(false);
- Line 528: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: handle->future_ = promise.get_future();
- Line 533: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: item.input    = input;
  Confidence: band=very_high; score=0.9
- Line 544: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: const std::vector<EvaluationInput>& inputs) {
  Confidence: band=very_high; score=0.9
- Line 544: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const std::vector<EvaluationInput>& inputs) {
  Confidence: band=very_high; score=0.9
- Line 546: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: handles.reserve(inputs.size());
  Confidence: band=very_high; score=0.9
- Line 546: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: handles.reserve(inputs.size());
  Confidence: band=very_high; score=0.9
- Line 547: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (const auto& input : inputs) {
  Confidence: band=very_high; score=0.9
- Line 547: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: for (const auto& input : inputs) {
  Confidence: band=very_high; score=0.9
- Line 548: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: handles.push_back(evaluateAsync(input));
  Confidence: band=very_high; score=0.9
- Line 558: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const EvaluationInput& input,
  Confidence: band=very_high; score=0.9
- Line 562: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: item.input    = input;
  Confidence: band=very_high; score=0.9
- Line 577: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::lock_guard<std::mutex> lock(queue_mutex_);
- Line 584: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(5));
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 66: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 197: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: workers_.emplace_back(&BatchEvaluator::workerThread, this);
  Confidence: band=high; score=0.74
- Line 244: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 311: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(processEvaluation(input));
  Confidence: band=high; score=0.74
- Line 547: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: handles.push_back(evaluateAsync(input));
  Confidence: band=high; score=0.74
- Line 611: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: workers_.emplace_back(&BatchEvaluator::workerThread, this);
  Confidence: band=high; score=0.74

### src/rag/adversarial_tester.cpp
Total findings: 97

- Line 60: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: /// Duplicate tokens within each input are deduplicated before comparison,
  Confidence: band=very_high; score=0.99
- Line 383: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationInput orig_input;
  Confidence: band=very_high; score=0.99
- Line 384: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: orig_input.query            = bq.query;
  Confidence: band=very_high; score=0.99
- Line 385: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: orig_input.documents        = docs;
  Confidence: band=very_high; score=0.99
- Line 386: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: orig_input.generated_answer = bq.expected_answer.empty()
  Confidence: band=very_high; score=0.99
- Line 390: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationResult orig_result = judge.evaluate(orig_input);
  Confidence: band=very_high; score=0.99
- Line 398: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationInput perturbed_input;
  Confidence: band=very_high; score=0.99
- Line 399: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: perturbed_input.query            = variant;
  Confidence: band=very_high; score=0.99
- Line 400: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: perturbed_input.documents        = docs;
  Confidence: band=very_high; score=0.99
- Line 401: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: perturbed_input.generated_answer = orig_input.generated_answer;
  Confidence: band=very_high; score=0.99
- Line 403: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationResult pert_result = judge.evaluate(perturbed_input);
  Confidence: band=very_high; score=0.99
- Line 436: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationInput clean_input;
  Confidence: band=very_high; score=0.99
- Line 437: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: clean_input.query            = bq.query;
  Confidence: band=very_high; score=0.99
- Line 438: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: clean_input.documents        = impl_->base_documents;
  Confidence: band=very_high; score=0.99
- Line 439: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: clean_input.generated_answer = bq.expected_answer.empty()
  Confidence: band=very_high; score=0.99
- Line 443: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationResult clean_result = judge.evaluate(clean_input);
  Confidence: band=very_high; score=0.99
- Line 447: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationInput poison_input;
  Confidence: band=very_high; score=0.99
- Line 448: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: poison_input.query            = bq.query;
  Confidence: band=very_high; score=0.99
- Line 449: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: poison_input.documents        = poisoned_docs;
  Confidence: band=very_high; score=0.99
- Line 450: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: poison_input.generated_answer = clean_input.generated_answer;
  Confidence: band=very_high; score=0.99
- Line 452: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationResult poison_result = judge.evaluate(poison_input);
  Confidence: band=very_high; score=0.99
- Line 498: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationInput inj_input;
  Confidence: band=very_high; score=0.99
- Line 499: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: inj_input.query            = bq.query;
  Confidence: band=very_high; score=0.99
- Line 500: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: inj_input.documents        = injected_docs;
  Confidence: band=very_high; score=0.99
- Line 501: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: inj_input.generated_answer = bq.expected_answer.empty()
  Confidence: band=very_high; score=0.99
- Line 505: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: judge.evaluate(inj_input);
  Confidence: band=very_high; score=0.99
- Line 526: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationInput base_input;
  Confidence: band=very_high; score=0.99
- Line 527: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: base_input.query            = bq.query;
  Confidence: band=very_high; score=0.99
- Line 528: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: base_input.documents        = impl_->base_documents;
  Confidence: band=very_high; score=0.99
- Line 529: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: base_input.generated_answer = bq.expected_answer.empty()
  Confidence: band=very_high; score=0.99
- Line 533: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationResult base_result = judge.evaluate(base_input);
  Confidence: band=very_high; score=0.99
- Line 540: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationInput overflow_input;
  Confidence: band=very_high; score=0.99
- Line 541: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: overflow_input.query            = bq.query;
  Confidence: band=very_high; score=0.99
- Line 542: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: overflow_input.documents        = padded_docs;
  Confidence: band=very_high; score=0.99
- Line 543: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: overflow_input.generated_answer = base_input.generated_answer;
  Confidence: band=very_high; score=0.99
- Line 545: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationResult overflow_result = judge.evaluate(overflow_input);
  Confidence: band=very_high; score=0.99
- Line 565: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationInput orig_input;
  Confidence: band=very_high; score=0.99
- Line 566: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: orig_input.query            = bq.query;
  Confidence: band=very_high; score=0.99
- Line 567: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: orig_input.documents        = docs;
  Confidence: band=very_high; score=0.99
- Line 568: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: orig_input.generated_answer = bq.expected_answer.empty()
  Confidence: band=very_high; score=0.99
- Line 572: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationResult orig_result = judge.evaluate(orig_input);
  Confidence: band=very_high; score=0.99
- Line 580: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationInput syco_input;
  Confidence: band=very_high; score=0.99
- Line 581: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: syco_input.query            = syco_query;
  Confidence: band=very_high; score=0.99
- Line 582: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: syco_input.documents        = docs;
  Confidence: band=very_high; score=0.99
- Line 583: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: syco_input.generated_answer = orig_input.generated_answer;
  Confidence: band=very_high; score=0.99
- Line 585: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationResult syco_result = judge.evaluate(syco_input);
  Confidence: band=very_high; score=0.99
- Line 60: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: /// Duplicate tokens within each input are deduplicated before comparison,
  Confidence: band=very_high; score=0.9
- Line 383: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationInput orig_input;
  Confidence: band=very_high; score=0.9
- Line 384: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: orig_input.query            = bq.query;
  Confidence: band=very_high; score=0.9
- Line 385: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: orig_input.documents        = docs;
  Confidence: band=very_high; score=0.9
- Line 386: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: orig_input.generated_answer = bq.expected_answer.empty()
  Confidence: band=very_high; score=0.9
- Line 390: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationResult orig_result = judge.evaluate(orig_input);
  Confidence: band=very_high; score=0.9
- Line 398: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationInput perturbed_input;
  Confidence: band=very_high; score=0.9
- Line 399: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: perturbed_input.query            = variant;
  Confidence: band=very_high; score=0.9
- Line 400: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: perturbed_input.documents        = docs;
  Confidence: band=very_high; score=0.9
- Line 401: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: perturbed_input.generated_answer = orig_input.generated_answer;
  Confidence: band=very_high; score=0.9
- Line 401: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: perturbed_input.generated_answer = orig_input.generated_answer;
  Confidence: band=very_high; score=0.9
- Line 403: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationResult pert_result = judge.evaluate(perturbed_input);
  Confidence: band=very_high; score=0.9
- Line 450: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: poison_input.generated_answer = clean_input.generated_answer;
  Confidence: band=very_high; score=0.9
- Line 498: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationInput inj_input;
  Confidence: band=very_high; score=0.9
- Line 499: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: inj_input.query            = bq.query;
  Confidence: band=very_high; score=0.9
- Line 500: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: inj_input.documents        = injected_docs;
  Confidence: band=very_high; score=0.9
- Line 501: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: inj_input.generated_answer = bq.expected_answer.empty()
  Confidence: band=very_high; score=0.9
- Line 505: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: judge.evaluate(inj_input);
  Confidence: band=very_high; score=0.9
- Line 526: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationInput base_input;
  Confidence: band=very_high; score=0.9
- Line 527: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: base_input.query            = bq.query;
  Confidence: band=very_high; score=0.9
- Line 528: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: base_input.documents        = impl_->base_documents;
  Confidence: band=very_high; score=0.9
- Line 529: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: base_input.generated_answer = bq.expected_answer.empty()
  Confidence: band=very_high; score=0.9
- Line 533: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationResult base_result = judge.evaluate(base_input);
  Confidence: band=very_high; score=0.9
- Line 540: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationInput overflow_input;
  Confidence: band=very_high; score=0.9
- Line 541: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: overflow_input.query            = bq.query;
  Confidence: band=very_high; score=0.9
- Line 542: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: overflow_input.documents        = padded_docs;
  Confidence: band=very_high; score=0.9
- Line 543: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: overflow_input.generated_answer = base_input.generated_answer;
  Confidence: band=very_high; score=0.9
- Line 543: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: overflow_input.generated_answer = base_input.generated_answer;
  Confidence: band=very_high; score=0.9
- Line 545: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationResult overflow_result = judge.evaluate(overflow_input);
  Confidence: band=very_high; score=0.9
- Line 565: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationInput orig_input;
  Confidence: band=very_high; score=0.9
- Line 566: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: orig_input.query            = bq.query;
  Confidence: band=very_high; score=0.9
- Line 567: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: orig_input.documents        = docs;
  Confidence: band=very_high; score=0.9
- Line 568: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: orig_input.generated_answer = bq.expected_answer.empty()
  Confidence: band=very_high; score=0.9
- Line 572: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationResult orig_result = judge.evaluate(orig_input);
  Confidence: band=very_high; score=0.9
- Line 580: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationInput syco_input;
  Confidence: band=very_high; score=0.9
- Line 581: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: syco_input.query            = syco_query;
  Confidence: band=very_high; score=0.9
- Line 582: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: syco_input.documents        = docs;
  Confidence: band=very_high; score=0.9
- Line 583: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: syco_input.generated_answer = orig_input.generated_answer;
  Confidence: band=very_high; score=0.9
- Line 583: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: syco_input.generated_answer = orig_input.generated_answer;
  Confidence: band=very_high; score=0.9
- Line 585: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationResult syco_result = judge.evaluate(syco_input);
  Confidence: band=very_high; score=0.9
- Line 48: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(word);
  Confidence: band=high; score=0.74
- Line 67: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> set_a(a.begin(), a.end());
  Confidence: band=medium; score=0.66
- Line 68: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> set_b(b.begin(), b.end());
  Confidence: band=medium; score=0.66
- Line 129: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += " (variant " + std::to_string(variant_index + 1) + ")";
  Confidence: band=high; score=0.74
- Line 230: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: docs.push_back(d);
  Confidence: band=high; score=0.74
- Line 319: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(perturbed);
  Confidence: band=high; score=0.74
- Line 415: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.failing_examples.push_back(ex);
  Confidence: band=high; score=0.74
- Line 470: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.poisoning_results.push_back(pr);
  Confidence: band=high; score=0.74
- Line 495: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: injected_docs.push_back(injected_doc);
  Confidence: band=high; score=0.74
- Line 495: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: injected_docs.push_back(injected_doc);
  Confidence: band=high; score=0.74
- Line 591: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: report.vulnerabilities.push_back(
  Confidence: band=high; score=0.74

### src/rag/evaluation_report_exporter.cpp
Total findings: 78

- Line 111: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const EvaluationInput&  inp = report.input;
  Confidence: band=very_high; score=0.99
- Line 226: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const EvaluationInput&  inp = report.input;
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Context: ['    os << "<!DOCTYPE html>\\n<html lang=\\"en\\">\\n<head>\\n"', '       << "<meta charset=\\"UTF-8\\">\\n"', '       << "<meta name=\\"viewport\\" content=\\"width=device-width,initial-scale=1\\">\\n"', '       << "<title>RAG Evaluation Report";', '    if (!report.report_id.empty())']
  Confidence: band=high; score=0.78
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3353 [rag] Per-query evaluation ... (2026-03-12)
- Line 46: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x",
- Line 111: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const EvaluationInput&  inp = report.input;
  Confidence: band=very_high; score=0.9
- Line 226: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const EvaluationInput&  inp = report.input;
  Confidence: band=very_high; score=0.9
- Line 34: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '"':  out += "\\\""; break;
  Confidence: band=high; score=0.74
- Line 35: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"':  out += "\\\""; break;
- Line 36: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\\': out += "\\\\"; break;
- Line 37: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\b': out += "\\b";  break;
- Line 38: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\f': out += "\\f";  break;
- Line 39: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\n': out += "\\n";  break;
- Line 40: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\r': out += "\\r";  break;
- Line 41: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\t': out += "\\t";  break;
- Line 46: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::snprintf(buf, sizeof(buf), "\\u%04x",
- Line 62: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '&':  out += "&amp;";  break;
  Confidence: band=high; score=0.74
- Line 63: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '&':  out += "&amp;";  break;
- Line 64: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '<':  out += "<";   break;
- Line 65: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '>':  out += ">";   break;
- Line 66: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"':  out += "&quot;"; break;
- Line 67: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\'': out += "&#39;";  break;
- Line 95: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<span class=\"score-label\">" << escapeHTML(label) << "</span>"
- Line 95: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<span class=\"score-label\">" << escapeHTML(label) << "</span>"
- Line 98: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "%;background:" << colour << ";\"></div>"
- Line 102: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "</div>\n";
- Line 241: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "</title>\n"
- Line 262: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "</style>\n"
- Line 263: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "</head>\n<body>\n";
- Line 268: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << " <span class=\"meta\">(" << escapeHTML(report.report_id) << ")</span>";
- Line 268: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << " <span class=\"meta\">(" << escapeHTML(report.report_id) << ")</span>";
- Line 269: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "</h1>\n";
- Line 275: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << std::fixed << std::setprecision(3) << res.confidence << "</strong></p>\n";
- Line 279: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << pass_label << "</span></p>\n";
- Line 283: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<h2>Query</h2>\n"
- Line 284: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<p>" << escapeHTML(inp.query) << "</p>\n"
- Line 285: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<h2>Generated Answer</h2>\n"
- Line 286: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "<p>" << escapeHTML(inp.generated_answer) << "</p>\n"
- Line 287: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "</div>\n";
- Line 290: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<div class=\"card\">\n<h2>Dimension Scores</h2>\n";
- Line 290: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<div class=\"card\">\n<h2>Dimension Scores</h2>\n";
- Line 298: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "</div>\n";
- Line 301: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<div class=\"card\">\n<h2>Claims</h2>\n";
- Line 301: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<div class=\"card\">\n<h2>Claims</h2>\n";
- Line 304: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<p><strong>✓ Verified Claims</strong></p>\n<ul class=\"claims\">\n";
- Line 304: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<p><strong>✓ Verified Claims</strong></p>\n<ul class=\"claims\">\n";
- Line 306: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<li class=\"verified\">" << escapeHTML(c) << "</li>\n";
- Line 306: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<li class=\"verified\">" << escapeHTML(c) << "</li>\n";
- Line 307: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "</ul>\n";
- Line 311: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<p><strong>✗ Unverified Claims</strong></p>\n<ul class=\"claims\">\n";
- Line 311: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<p><strong>✗ Unverified Claims</strong></p>\n<ul class=\"claims\">\n";
- Line 313: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<li class=\"unverified\">" << escapeHTML(c) << "</li>\n";
- Line 313: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<li class=\"unverified\">" << escapeHTML(c) << "</li>\n";
- Line 314: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "</ul>\n";
- Line 318: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<p class=\"meta\">No claims extracted.</p>\n";
- Line 318: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<p class=\"meta\">No claims extracted.</p>\n";
- Line 320: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "</div>\n";
- Line 323: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<div class=\"card\">\n<h2>Ethical Compliance</h2>\n"
- Line 323: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<div class=\"card\">\n<h2>Ethical Compliance</h2>\n"
- Line 325: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << (res.respects_human_autonomy ? "Yes" : "No") << "</strong></p>\n"
- Line 327: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << (res.shows_moral_diversity ? "Yes" : "No") << "</strong></p>\n"
- Line 329: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << (res.has_ethical_citations ? "Yes" : "No") << "</strong></p>\n";
- Line 332: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<p><strong>Ethical Violations:</strong></p>\n<ul>\n";
- Line 334: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<li style=\"color:#c62828\">" << escapeHTML(v) << "</li>\n";
- Line 334: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<li style=\"color:#c62828\">" << escapeHTML(v) << "</li>\n";
- Line 335: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "</ul>\n";
- Line 337: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "</div>\n";
- Line 341: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<div class=\"card\">\n<h2>Suggested Improvements</h2>\n<ul>\n";
- Line 341: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<div class=\"card\">\n<h2>Suggested Improvements</h2>\n<ul>\n";
- Line 343: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<li>" << escapeHTML(imp) << "</li>\n";
- Line 344: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "</ul>\n</div>\n";
- Line 349: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<div class=\"card\">\n<h2>Explanation</h2>\n"
- Line 349: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "<div class=\"card\">\n<h2>Explanation</h2>\n"
- Line 351: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "</div>\n</div>\n";
- Line 357: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << inp.documents.size() << ")</h2>\n";
- Line 365: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "</div>\n";
- Line 367: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "</div>\n";
- Line 370: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: os << "</body>\n</html>\n";

### src/rag/knowledge_gap_detector.cpp
Total findings: 60

- Line 425: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: avg_similarity >= impl_->config.similarity_threshold) {
- Line 476: severity=CRITICAL; category=smart_ptr_misuse
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
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #2576 RAG Phase 2: Wire streaming... (2026-03-12) | #655 [RAG-GAP-P2] Impleme
- Line 131: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: result.explanation = "Retrieved documents have low semantic similarity to query (avg: " +
- Line 840: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (content_lower.find(term) != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 1047: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Production Delta: LLM-generated samples capture inference chains,
  Confidence: band=very_high; score=0.9
- Line 1083: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto& sent : splitSentences(docs[d].content)) {
- Line 1319: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (content_lower.find(term) != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 1489: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (lower_content.find(framework) != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 1491: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (framework.find("utilitarian") != std::string::npos ||
  Confidence: band=very_high; score=0.9
- Line 1492: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: framework.find("consequentialist") != std::string::npos ||
  Confidence: band=very_high; score=0.9
- Line 1493: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: framework.find("utility") != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 1495: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: } else if (framework.find("deontological") != std::string::npos ||
  Confidence: band=very_high; score=0.9
- Line 1496: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: framework.find("kant") != std::string::npos ||
  Confidence: band=very_high; score=0.9
- Line 1497: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: framework.find("duty") != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 1499: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: } else if (framework.find("virtue") != std::string::npos ||
  Confidence: band=very_high; score=0.9
- Line 1500: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: framework.find("aristotle") != std::string::npos ||
  Confidence: band=very_high; score=0.9
- Line 1501: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: framework.find("character") != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 1503: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: } else if (framework.find("rights") != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 1505: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: } else if (framework.find("care") != std::string::npos ||
  Confidence: band=very_high; score=0.9
- Line 1506: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: framework.find("feminist") != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 1508: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: } else if (framework.find("religious") != std::string::npos ||
  Confidence: band=very_high; score=0.9
- Line 1509: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: framework.find("divine") != std::string::npos ||
  Confidence: band=very_high; score=0.9
- Line 1510: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: framework.find("faith") != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 1512: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: } else if (framework.find("cultural") != std::string::npos ||
  Confidence: band=very_high; score=0.9
- Line 1513: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: framework.find("relativism") != std::string::npos) {
  Confidence: band=very_high; score=0.9
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 106: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 467: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: current_documents.push_back(new_doc);
  Confidence: band=high; score=0.74
- Line 467: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: current_documents.push_back(new_doc);
  Confidence: band=high; score=0.74
- Line 631: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: aspects.push_back(current_aspect);
  Confidence: band=high; score=0.74
- Line 672: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: all_content += doc.content + " ";
  Confidence: band=high; score=0.74
- Line 689: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: missing.push_back(aspect);
  Confidence: band=high; score=0.74
- Line 689: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: missing.push_back(aspect);
  Confidence: band=high; score=0.74
- Line 767: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: claims.push_back(claim);
  Confidence: band=high; score=0.74
- Line 810: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: claim_terms.push_back(current_term);
  Confidence: band=high; score=0.74
- Line 977: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filtered.push_back(prob);
  Confidence: band=high; score=0.74
- Line 977: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filtered.push_back(prob);
  Confidence: band=high; score=0.74
- Line 1005: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: averages.push_back(sum / window_size);
  Confidence: band=high; score=0.74
- Line 1005: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: averages.push_back(sum / window_size);
  Confidence: band=high; score=0.74
- Line 1006: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: averages.push_back(sum / window_size);
- Line 1065: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sentences.push_back(current.substr(s));
  Confidence: band=high; score=0.74
- Line 1066: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sentences.push_back(current.substr(s));
- Line 1074: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sentences.push_back(current.substr(s));
- Line 1093: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: samples.push_back("No document content available for query: " + query
  Confidence: band=high; score=0.74
- Line 1113: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: samples.push_back(oss.str());
  Confidence: band=high; score=0.74
- Line 1113: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: samples.push_back(oss.str());
  Confidence: band=high; score=0.74
- Line 1114: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples.push_back(oss.str());
- Line 1259: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sentences.push_back(sentence);
  Confidence: band=high; score=0.74
- Line 1271: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sentences.push_back(current_sentence.substr(start));
- Line 1295: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sentence_terms.push_back(current_term);
  Confidence: band=high; score=0.74
- Line 1443: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: bool KnowledgeGapDetector::isEthicalQuery(const std::string& query) {
  Confidence: band=high; score=0.74
- Line 1481: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> found_frameworks;
  Confidence: band=medium; score=0.66
- Line 870: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: log_sum += std::log(prob);
  Confidence: band=medium; score=0.6
- Line 942: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: log_sum += std::log(prob);
  Confidence: band=medium; score=0.6

### src/rag/dpr_vectorizer.cpp
Total findings: 58

- Line 149: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::vector<int64_t> input_ids(tokens.begin(), tokens.end());
  Confidence: band=very_high; score=0.99
- Line 162: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: Ort::Value input_ids_tensor = Ort::Value::CreateTensor<int64_t>(
  Confidence: band=very_high; score=0.99
- Line 163: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: mem_info, input_ids.data(), input_ids.size(), shape.data(), shape.size());
  Confidence: band=very_high; score=0.99
- Line 168: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::vector<Ort::AllocatedStringPtr> input_name_holders;
  Confidence: band=very_high; score=0.99
- Line 169: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::vector<const char*> input_names;
  Confidence: band=very_high; score=0.99
- Line 170: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::vector<Ort::Value> input_tensors;
  Confidence: band=very_high; score=0.99
- Line 172: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const size_t input_count = static_cast<size_t>(session->GetInputCount());
- Line 172: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const size_t input_count = static_cast<size_t>(session->GetInputCount());
  Confidence: band=very_high; score=0.99
- Line 173: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_name_holders.reserve(input_count);
  Confidence: band=very_high; score=0.99
- Line 174: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_names.reserve(input_count);
  Confidence: band=very_high; score=0.99
- Line 175: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_tensors.reserve(input_count);
  Confidence: band=very_high; score=0.99
- Line 177: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_name_holders.push_back(session->GetInputNameAllocated(0, allocator));
  Confidence: band=very_high; score=0.99
- Line 178: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_names.push_back(input_name_holders.back().get());
  Confidence: band=very_high; score=0.99
- Line 179: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_tensors.push_back(std::move(input_ids_tensor));
  Confidence: band=very_high; score=0.99
- Line 180: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (input_count > 1) {
  Confidence: band=very_high; score=0.99
- Line 181: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_name_holders.push_back(session->GetInputNameAllocated(1, allocator));
  Confidence: band=very_high; score=0.99
- Line 182: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_names.push_back(input_name_holders.back().get());
  Confidence: band=very_high; score=0.99
- Line 183: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_tensors.push_back(std::move(attention_mask_tensor));
  Confidence: band=very_high; score=0.99
- Line 191: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_names.data(),
  Confidence: band=very_high; score=0.99
- Line 192: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_tensors.data(),
  Confidence: band=very_high; score=0.99
- Line 193: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_tensors.size(),
  Confidence: band=very_high; score=0.99
- Line 290: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto query_model = impl_->model_loader->loadModel(config_.query_model_path);
- Line 290: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto query_model = impl_->model_loader->loadModel(config_.query_model_path);
  Confidence: band=very_high; score=0.99
- Line 301: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto passage_model = impl_->model_loader->loadModel(config_.passage_model_path);
- Line 301: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: auto passage_model = impl_->model_loader->loadModel(config_.passage_model_path);
  Confidence: band=very_high; score=0.99
- Line 312: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->query_tokenizer = std::make_unique<themis::llm::lora::LlamaTokenizer>(config_.query_model_pat
- Line 313: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->passage_tokenizer = std::make_unique<themis::llm::lora::LlamaTokenizer>(config_.passage_model
- Line 457: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: THEMIS_DEBUG("DPRVectorizer::encodePassageBatch called with empty input; returning empty result");
  Confidence: band=very_high; score=0.99
- Line 149: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::vector<int64_t> input_ids(tokens.begin(), tokens.end());
  Confidence: band=very_high; score=0.9
- Line 162: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: Ort::Value input_ids_tensor = Ort::Value::CreateTensor<int64_t>(
  Confidence: band=very_high; score=0.9
- Line 163: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: mem_info, input_ids.data(), input_ids.size(), shape.data(), shape.size());
  Confidence: band=very_high; score=0.9
- Line 168: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::vector<Ort::AllocatedStringPtr> input_name_holders;
  Confidence: band=very_high; score=0.9
- Line 169: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::vector<const char*> input_names;
  Confidence: band=very_high; score=0.9
- Line 170: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::vector<Ort::Value> input_tensors;
  Confidence: band=very_high; score=0.9
- Line 172: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const size_t input_count = static_cast<size_t>(session->GetInputCount());
  Confidence: band=very_high; score=0.9
- Line 173: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_name_holders.reserve(input_count);
  Confidence: band=very_high; score=0.9
- Line 174: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_names.reserve(input_count);
  Confidence: band=very_high; score=0.9
- Line 175: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_tensors.reserve(input_count);
  Confidence: band=very_high; score=0.9
- Line 177: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_name_holders.push_back(session->GetInputNameAllocated(0, allocator));
  Confidence: band=very_high; score=0.9
- Line 178: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_names.push_back(input_name_holders.back().get());
  Confidence: band=very_high; score=0.9
- Line 179: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_tensors.push_back(std::move(input_ids_tensor));
  Confidence: band=very_high; score=0.9
- Line 180: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (input_count > 1) {
  Confidence: band=very_high; score=0.9
- Line 181: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_name_holders.push_back(session->GetInputNameAllocated(1, allocator));
  Confidence: band=very_high; score=0.9
- Line 182: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_names.push_back(input_name_holders.back().get());
  Confidence: band=very_high; score=0.9
- Line 183: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_tensors.push_back(std::move(attention_mask_tensor));
  Confidence: band=very_high; score=0.9
- Line 189: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: auto outputs = session->Run(
  Confidence: band=very_high; score=0.9
- Line 191: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_names.data(),
  Confidence: band=very_high; score=0.9
- Line 192: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_tensors.data(),
  Confidence: band=very_high; score=0.9
- Line 193: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_tensors.size(),
  Confidence: band=very_high; score=0.9
- Line 197: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: if (outputs.empty() || !outputs[0].IsTensor()) {
  Confidence: band=very_high; score=0.9
- Line 201: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: const auto type_info = outputs[0].GetTensorTypeAndShapeInfo();
  Confidence: band=very_high; score=0.9
- Line 203: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: const auto* out_data = outputs[0].GetTensorData<float>();
  Confidence: band=very_high; score=0.9
- Line 457: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: THEMIS_DEBUG("DPRVectorizer::encodePassageBatch called with empty input; returning empty result");
  Confidence: band=very_high; score=0.9
- Line 201: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: const auto type_info = outputs[0].GetTensorTypeAndShapeInfo();
  Confidence: band=high; score=0.74
- Line 368: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::vector<float> DPRVectorizer::encodeQuery(const std::string& query) {
  Confidence: band=high; score=0.74
- Line 475: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch_tokens.push_back(tokens);
  Confidence: band=high; score=0.74
- Line 475: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch_tokens.push_back(tokens);
  Confidence: band=high; score=0.74
- Line 489: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(embedding));
  Confidence: band=high; score=0.74

### src/rag/distributed_rag_evaluator.cpp
Total findings: 37

- Line 70: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: DistributedRAGEvaluator::evaluate(const judge::EvaluationInput& input)
  Confidence: band=very_high; score=0.99
- Line 97: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Copy the input into shared storage so lambdas that outlive evaluate()
  Confidence: band=very_high; score=0.99
- Line 99: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto shared_input = std::make_shared<judge::EvaluationInput>(input);
  Confidence: band=very_high; score=0.99
- Line 127: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: [judge_ptr, shared_input, sem]() {
  Confidence: band=very_high; score=0.99
- Line 128: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto result = judge_ptr->evaluate(*shared_input);
  Confidence: band=very_high; score=0.99
- Line 220: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const std::vector<judge::EvaluationInput>& inputs)
  Confidence: band=very_high; score=0.99
- Line 223: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: results.reserve(inputs.size());
  Confidence: band=very_high; score=0.99
- Line 225: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (const auto& input : inputs) {
  Confidence: band=very_high; score=0.99
- Line 226: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: results.push_back(evaluate(input));
  Confidence: band=very_high; score=0.99
- Line 70: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: DistributedRAGEvaluator::evaluate(const judge::EvaluationInput& input)
  Confidence: band=very_high; score=0.9
- Line 97: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Copy the input into shared storage so lambdas that outlive evaluate()
  Confidence: band=very_high; score=0.9
- Line 99: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto shared_input = std::make_shared<judge::EvaluationInput>(input);
  Confidence: band=very_high; score=0.9
- Line 114: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < n; ++i) {
  Confidence: band=very_high; score=0.9
- Line 117: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Acquire lock before loop or redesign to minimize lock time
  Context: std::unique_lock<std::mutex> lk(sem->mtx);
- Line 124: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto judge_ptr = judges[i];  // shared ownership — safe across timeouts
- Line 127: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: [judge_ptr, shared_input, sem]() {
  Confidence: band=very_high; score=0.9
- Line 127: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: [judge_ptr, shared_input, sem]() {
- Line 128: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto result = judge_ptr->evaluate(*shared_input);
  Confidence: band=very_high; score=0.9
- Line 128: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto result = judge_ptr->evaluate(*shared_input);
- Line 128: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto result = judge_ptr->evaluate(*shared_input);
- Line 153: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: res = futures[i].get();
  Confidence: band=very_high; score=0.9
- Line 160: severity=HIGH; category=distributed_consistency; pattern=unspecified_consistency
  Description: Read without explicit consistency level (replication lag unknown)
  Context: res = futures[i].get();
  Confidence: band=very_high; score=0.9
- Line 220: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: const std::vector<judge::EvaluationInput>& inputs)
  Confidence: band=very_high; score=0.9
- Line 220: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const std::vector<judge::EvaluationInput>& inputs)
  Confidence: band=very_high; score=0.9
- Line 223: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: results.reserve(inputs.size());
  Confidence: band=very_high; score=0.9
- Line 223: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: results.reserve(inputs.size());
  Confidence: band=very_high; score=0.9
- Line 225: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: for (const auto& input : inputs) {
  Confidence: band=very_high; score=0.9
- Line 225: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: for (const auto& input : inputs) {
  Confidence: band=very_high; score=0.9
- Line 226: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: results.push_back(evaluate(input));
  Confidence: band=very_high; score=0.9
- Line 315: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (total_w == 0.0) { total_w = 1.0; }
  Confidence: band=very_high; score=0.9
- Line 124: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: futures.push_back(
  Confidence: band=high; score=0.74
- Line 124: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto judge_ptr = judges[i];  // shared ownership — safe across timeouts
  Confidence: band=high; score=0.74
- Line 151: severity=MEDIUM; category=performance; pattern=unnecessary_copy
  Description: Unnecessary copy: use auto& for container element access
  Context: auto status = futures[i].wait_for(timeout);
  Confidence: band=high; score=0.74
- Line 168: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: successful_results.push_back(res);
  Confidence: band=high; score=0.74
- Line 170: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: successful_weights.push_back(impl_->workers[i].weight);
- Line 225: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(evaluate(input));
  Confidence: band=high; score=0.74
- Line 407: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: workers.push_back(std::move(w));
  Confidence: band=high; score=0.74

### src/rag/document_summarizer.cpp
Total findings: 34

- Line 106: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator start may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: const auto start = current.find_first_not_of(" \t\n\r");
- Line 337: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Measure total input size
  Confidence: band=very_high; score=0.99
- Line 339: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: result.total_input_chars += d.content.size();
  Confidence: band=very_high; score=0.99
- Line 410: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: result.compression_ratio = result.total_input_chars > 0
  Confidence: band=very_high; score=0.99
- Line 412: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: static_cast<double>(result.total_input_chars)
  Confidence: band=very_high; score=0.99
- Line 419: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: THEMIS_INFO("DocumentSummarizer complete: input={} chars, summary={} chars, "
  Confidence: band=very_high; score=0.99
- Line 421: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: result.total_input_chars, result.summary_chars,
  Confidence: band=very_high; score=0.99
- Line 174: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: << "Document [" << document_id << "]:\n"
- Line 216: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: return LLMIntegration::getInferenceEngine()
  Confidence: band=very_high; score=0.9
- Line 224: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto& w : tokeniseWords(query)) {
- Line 337: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Measure total input size
  Confidence: band=very_high; score=0.9
- Line 339: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: result.total_input_chars += d.content.size();
  Confidence: band=very_high; score=0.9
- Line 359: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: result.combined_summary = LLMIntegration::generate(prompt, opts);
  Confidence: band=very_high; score=0.9
- Line 410: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: result.compression_ratio = result.total_input_chars > 0
  Confidence: band=very_high; score=0.9
- Line 412: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: static_cast<double>(result.total_input_chars)
  Confidence: band=very_high; score=0.9
- Line 419: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: THEMIS_INFO("DocumentSummarizer complete: input={} chars, summary={} chars, "
  Confidence: band=very_high; score=0.9
- Line 421: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: result.total_input_chars, result.summary_chars,
  Confidence: band=very_high; score=0.9
- Line 18: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: * LLMIntegration::generate() so that the LLM produces a fluent, compressed
  Confidence: band=high; score=0.74
- Line 48: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(std::move(cur));
  Confidence: band=high; score=0.74
- Line 64: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string>& query_terms)
  Confidence: band=medium; score=0.66
- Line 73: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 99: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sentences.push_back(trimmed);
  Confidence: band=high; score=0.74
- Line 129: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scored.emplace_back(
  Confidence: band=high; score=0.74
- Line 145: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: selected_indices.push_back(idx);
  Confidence: band=high; score=0.74
- Line 154: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!result.empty()) result += ' ';
  Confidence: band=high; score=0.74
- Line 155: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!result.empty()) result += ' ';
- Line 222: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> queryTerms(const std::string& query) const {
  Confidence: band=medium; score=0.66
- Line 223: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> terms;
  Confidence: band=medium; score=0.66
- Line 255: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: ds.summary  = LLMIntegration::generate(prompt, opts);
  Confidence: band=high; score=0.74
- Line 350: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: id_content.emplace_back(d.id, d.content);
  Confidence: band=high; score=0.74
- Line 359: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: result.combined_summary = LLMIntegration::generate(prompt, opts);
  Confidence: band=high; score=0.74
- Line 379: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.per_document_summaries.push_back(std::move(ds));
  Confidence: band=high; score=0.74
- Line 396: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.per_document_summaries.push_back(std::move(ds));
  Confidence: band=high; score=0.74
- Line 442: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: converted.push_back(std::move(rd));
  Confidence: band=high; score=0.74

### src/rag/llm_judge_client.cpp
Total findings: 34

- Line 178: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: if (!plugin->loadModel(model_path.string(), plugin_config)) {
  Confidence: band=very_high; score=0.99
- Line 238: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: request.base_request.temperature = static_cast<float>(impl_->config.temperature);
- Line 292: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: request.base_request.temperature = static_cast<float>(impl_->config.temperature);
- Line 12: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: * @brief LLM Judge Client - Connects prompts to InferenceEngineEnhanced
  Confidence: band=very_high; score=0.9
- Line 14: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: * This client bridges RAG Judge evaluations to the LLM inference engine,
  Confidence: band=very_high; score=0.9
- Line 20: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: #include "llm/inference_engine_enhanced.h"
  Confidence: band=very_high; score=0.9
- Line 132: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entry : fs::directory_iterator(dir)) {
- Line 149: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::shared_ptr<llm::InferenceEngineEnhanced> inference_engine;
  Confidence: band=very_high; score=0.9
- Line 183: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inference_engine->registerModel(config.model_name, plugin);
  Confidence: band=very_high; score=0.9
- Line 199: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Initialize inference engine with appropriate config
  Confidence: band=very_high; score=0.9
- Line 200: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm::InferenceEngineEnhanced::Config engine_config;
  Confidence: band=very_high; score=0.9
- Line 206: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inference_engine = std::make_shared<llm::InferenceEngineEnhanced>(engine_config);
  Confidence: band=very_high; score=0.9
- Line 207: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inference_engine->start();
  Confidence: band=very_high; score=0.9
- Line 214: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (inference_engine) {
  Confidence: band=very_high; score=0.9
- Line 215: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inference_engine->shutdown();
  Confidence: band=very_high; score=0.9
- Line 234: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Create inference request
  Confidence: band=very_high; score=0.9
- Line 235: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm::InferenceEngineEnhanced::EnhancedInferenceRequest request;
  Confidence: band=very_high; score=0.9
- Line 238: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.base_request.temperature = static_cast<float>(impl_->config.temperature);
- Line 250: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto handle = impl_->inference_engine->submit(request);
  Confidence: band=very_high; score=0.9
- Line 285: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::vector<llm::InferenceHandle> handles;
  Confidence: band=very_high; score=0.9
- Line 289: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm::InferenceEngineEnhanced::EnhancedInferenceRequest request;
  Confidence: band=very_high; score=0.9
- Line 292: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.base_request.temperature = static_cast<float>(impl_->config.temperature);
- Line 303: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: handles.push_back(impl_->inference_engine->submit(request));
  Confidence: band=very_high; score=0.9
- Line 391: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void LLMJudgeClient::setInferenceEngine(
  Confidence: band=very_high; score=0.9
- Line 392: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::shared_ptr<llm::InferenceEngineEnhanced> engine
  Confidence: band=very_high; score=0.9
- Line 394: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: impl_->inference_engine = engine;
  Confidence: band=very_high; score=0.9
- Line 395: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: THEMIS_INFO("Custom inference engine set");
  Confidence: band=very_high; score=0.9
- Line 402: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (impl_->inference_engine) {
  Confidence: band=very_high; score=0.9
- Line 403: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: impl_->inference_engine->registerModel(model_id, plugin);
  Confidence: band=very_high; score=0.9
- Line 66: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dirs.emplace_back(cwd / relative_dir);
  Confidence: band=high; score=0.74
- Line 302: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: handles.push_back(impl_->inference_engine->submit(request));
  Confidence: band=high; score=0.74
- Line 308: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(response.text);
  Confidence: band=high; score=0.74
- Line 309: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(response.text);
- Line 470: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/rag/llm_meta_analyzer.cpp
Total findings: 34

- Line 64: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const std::string& input_text,
  Confidence: band=very_high; score=0.99
- Line 70: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: prompt << "Input:\n" << input_text << "\n\n";
  Confidence: band=very_high; score=0.99
- Line 87: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const std::string& input_text,
  Confidence: band=very_high; score=0.99
- Line 90: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return buildPromptWithCoT(task_description, input_text, criteria, {});
  Confidence: band=very_high; score=0.99
- Line 95: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const std::string& input_text,
  Confidence: band=very_high; score=0.99
- Line 111: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: prompt << "Input:\n" << input_text << "\n\n";
  Confidence: band=very_high; score=0.99
- Line 122: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: prompt << "1. First, analyze the input carefully\n";
  Confidence: band=very_high; score=0.99
- Line 249: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return "Reasoning: The input has been analyzed according to criteria.\nScore: 0.75";
  Confidence: band=very_high; score=0.99
- Line 254: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: metrics["llm_total_calls"] = static_cast<double>(impl_->total_calls);
- Line 255: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: metrics["llm_cache_hits"] = static_cast<double>(impl_->cache_hits);
- Line 255: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: metrics["llm_cache_hits"] = static_cast<double>(impl_->cache_hits);
- Line 256: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: metrics["llm_cache_misses"] = static_cast<double>(impl_->cache_misses);
- Line 256: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: metrics["llm_cache_misses"] = static_cast<double>(impl_->cache_misses);
- Line 265: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::string LLMMetaAnalyzer::computeCacheKey(const std::string& input) {
  Confidence: band=very_high; score=0.99
- Line 271: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (unsigned char c : input) {
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #651 [RAG-ETHICS] Add ethical co... (2026-03-11) | #1297 RAG module: replace
- Line 18: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: #include "llm/inference_engine_enhanced.h"
  Confidence: band=very_high; score=0.9
- Line 64: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const std::string& input_text,
  Confidence: band=very_high; score=0.9
- Line 70: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: prompt << "Input:\n" << input_text << "\n\n";
  Confidence: band=very_high; score=0.9
- Line 87: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const std::string& input_text,
  Confidence: band=very_high; score=0.9
- Line 90: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return buildPromptWithCoT(task_description, input_text, criteria, {});
  Confidence: band=very_high; score=0.9
- Line 95: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const std::string& input_text,
  Confidence: band=very_high; score=0.9
- Line 111: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: prompt << "Input:\n" << input_text << "\n\n";
  Confidence: band=very_high; score=0.9
- Line 122: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: prompt << "1. First, analyze the input carefully\n";
  Confidence: band=very_high; score=0.9
- Line 223: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Delegate to the shared inference engine when one is configured
  Confidence: band=very_high; score=0.9
- Line 224: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto engine = LLMIntegration::getInferenceEngine();
  Confidence: band=very_high; score=0.9
- Line 227: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm::InferenceEngineEnhanced::EnhancedInferenceRequest request;
  Confidence: band=very_high; score=0.9
- Line 236: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.request_id = "meta_" + std::to_string(req_counter.fetch_add(1));
- Line 249: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return "Reasoning: The input has been analyzed according to criteria.\nScore: 0.75";
  Confidence: band=very_high; score=0.9
- Line 265: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::string LLMMetaAnalyzer::computeCacheKey(const std::string& input) {
  Confidence: band=very_high; score=0.9
- Line 271: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: for (unsigned char c : input) {
  Confidence: band=very_high; score=0.9
- Line 158: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::regex("([0-9]*\\.?[0-9]+)\\s*/\\s*1\\.?0?"),
- Line 181: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/rag/multi_step_rag.cpp
Total findings: 34

- Line 123: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator first may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: const auto first = line.find_first_not_of(" \t\r");
- Line 124: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator last may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: const auto last  = line.find_last_not_of(" \t\r");
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 230: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const InferenceFn&                 infer) const
  Confidence: band=very_high; score=0.9
- Line 249: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (documents.empty() || !infer) {
  Confidence: band=very_high; score=0.9
- Line 252: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "MultiStepRAG::runMapReduce short-circuit: docs={} infer_ready={}",
  Confidence: band=very_high; score=0.9
- Line 254: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: static_cast<bool>(infer));
  Confidence: band=very_high; score=0.9
- Line 265: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: result.final_answer   = infer(prompt, bounded_max_tokens);
  Confidence: band=very_high; score=0.9
- Line 296: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // infer is captured by reference; callers must ensure the InferenceFn
  Confidence: band=very_high; score=0.9
- Line 298: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // EXCEPTIONS: if infer() throws, the exception is stored in the future.
  Confidence: band=very_high; score=0.9
- Line 305: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: [this, &batches, &query, &infer, map_max_tok, bi]() -> std::string {
  Confidence: band=very_high; score=0.9
- Line 306: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: return infer(buildMapPrompt(batches[bi], query), map_max_tok);
  Confidence: band=very_high; score=0.9
- Line 324: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string partial          = infer(map_prompt, map_max_tok);
  Confidence: band=very_high; score=0.9
- Line 348: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: result.final_answer  = infer(reduce_prompt, bounded_max_tokens);
  Confidence: band=very_high; score=0.9
- Line 366: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const InferenceFn&                 infer,
  Confidence: band=very_high; score=0.9
- Line 386: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (!infer) return result;
  Confidence: band=very_high; score=0.9
- Line 419: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const std::string gap_response = infer(gap_prompt, gap_max_tokens);
  Confidence: band=very_high; score=0.9
- Line 125: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: aspects.push_back(line.substr(first, last - first + 1));
  Confidence: band=high; score=0.74
- Line 126: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: aspects.push_back(line.substr(first, last - first + 1));
- Line 204: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batches.push_back(std::move(current_batch));
  Confidence: band=high; score=0.74
- Line 269: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.steps.push_back(result.final_answer);
- Line 298: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: // EXCEPTIONS: if infer() throws, the exception is stored in the future.
  Confidence: band=high; score=0.74
- Line 306: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: return infer(buildMapPrompt(batches[bi], query), map_max_tok);
  Confidence: band=high; score=0.74
- Line 312: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.steps.push_back(f.get());
  Confidence: band=high; score=0.74
- Line 315: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 324: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string partial          = infer(map_prompt, map_max_tok);
  Confidence: band=high; score=0.74
- Line 324: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.steps.push_back(partial);
  Confidence: band=high; score=0.74
- Line 348: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: result.final_answer  = infer(reduce_prompt, bounded_max_tokens);
  Confidence: band=high; score=0.74
- Line 408: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: result.final_answer = infer(prompt, bounded_max_tokens);
  Confidence: band=high; score=0.74
- Line 408: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.steps.push_back(result.final_answer);
  Confidence: band=high; score=0.74
- Line 409: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.steps.push_back(result.final_answer);
- Line 419: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: const std::string gap_response = infer(gap_prompt, gap_max_tokens);
  Confidence: band=high; score=0.74
- Line 447: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!already_present) accumulated.push_back(nd);
  Confidence: band=high; score=0.74

### src/rag/knowledge_graph_retriever.cpp
Total findings: 33

- Line 59: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: /// length ratio; otherwise 0.0.  Both inputs should already be normalised.
  Confidence: band=very_high; score=0.99
- Line 59: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: /// length ratio; otherwise 0.0.  Both inputs should already be normalised.
  Confidence: band=very_high; score=0.9
- Line 59: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: /// length ratio; otherwise 0.0.  Both inputs should already be normalised.
  Confidence: band=very_high; score=0.9
- Line 373: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: normalise(candidate_node->canonical_name));
- Line 375: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: for (const auto& alias : candidate_node->aliases) {
- Line 387: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: match.node_id       = match.is_linked ? best_node->id : "";
- Line 483: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // ── Step 2b: KnowledgeGraphReasoner multi-hop inference ─────────────────
  Confidence: band=very_high; score=0.9
- Line 484: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Run forward-chaining inference for each linked query entity when a
  Confidence: band=very_high; score=0.9
- Line 485: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // reasoner has been attached and max_inference_hops > 0.
  Confidence: band=very_high; score=0.9
- Line 486: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (impl_->reasoner && cfg.max_inference_hops > 0) {
  Confidence: band=very_high; score=0.9
- Line 502: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto chain = impl_->reasoner->infer(match.node_id, cfg.max_inference_hops);
  Confidence: band=very_high; score=0.9
- Line 505: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: result.inference_chains.push_back(chain);
  Confidence: band=very_high; score=0.9
- Line 507: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Expand query neighbourhood with nodes reachable via inference.
  Confidence: band=very_high; score=0.9
- Line 520: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: result.inference_chains.size(),
  Confidence: band=very_high; score=0.9
- Line 563: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: for (const auto& chain : result.inference_chains) {
  Confidence: band=very_high; score=0.9
- Line 42: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += ' ';
  Confidence: band=high; score=0.74
- Line 43: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += ' ';
- Line 76: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: double jaccardSets(const std::unordered_set<std::string>& A,
  Confidence: band=medium; score=0.66
- Line 77: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string>& B) {
  Confidence: band=medium; score=0.66
- Line 182: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: impl_->adj[edge.from_id].push_back(std::move(edge));
  Confidence: band=high; score=0.74
- Line 313: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entities.push_back({trimmed, EntityType::OTHER, 0.7,
  Confidence: band=high; score=0.74
- Line 314: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entities.push_back({trimmed, EntityType::OTHER, 0.7,
- Line 343: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!span_buf.empty()) span_buf += ' ';
  Confidence: band=high; score=0.74
- Line 387: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matches.push_back(std::move(match));
  Confidence: band=high; score=0.74
- Line 463: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> query_neighbourhood;
  Confidence: band=medium; score=0.66
- Line 502: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto chain = impl_->reasoner->infer(match.node_id, cfg.max_inference_hops);
  Confidence: band=high; score=0.74
- Line 504: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.inference_chains.push_back(chain);
  Confidence: band=high; score=0.74
- Line 536: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> doc_node_ids;
  Confidence: band=medium; score=0.66
- Line 575: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!chain_text.empty()) chain_text += "; ";
  Confidence: band=high; score=0.74
- Line 575: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!chain_text.empty()) chain_text += "; ";
  Confidence: band=high; score=0.74
- Line 575: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!chain_text.empty()) chain_text += "; ";
  Confidence: band=high; score=0.74
- Line 576: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!chain_text.empty()) chain_text += "; ";
- Line 576: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!chain_text.empty()) chain_text += "; ";

### src/rag/llm_judge_integration.cpp
Total findings: 32

- Line 78: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const EvaluationInput& input,
  Confidence: band=very_high; score=0.99
- Line 84: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::string prompt = template_mgr.generatePrompt(dimension, input);
  Confidence: band=very_high; score=0.99
- Line 27: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: LLMJudgeIntegration::LLMJudgeIntegration(ILLMInferenceEngine* engine)
  Confidence: band=very_high; score=0.9
- Line 31: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: LLMJudgeIntegration::LLMJudgeIntegration(ILLMInferenceEngine* engine, const Config& config)
  Confidence: band=very_high; score=0.9
- Line 36: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "Pass a valid ILLMInferenceEngine* or set config.allow_mock = true for testing.");
  Confidence: band=very_high; score=0.9
- Line 39: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Wire the engine's generate() into the inference function slot
  Confidence: band=very_high; score=0.9
- Line 40: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inference_fn_ = [engine](const std::string& prompt) {
  Confidence: band=very_high; score=0.9
- Line 44: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: THEMIS_INFO("LLMJudgeIntegration initialized with injected inference engine");
  Confidence: band=very_high; score=0.9
- Line 47: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inference_fn_ = defaultInference;
  Confidence: band=very_high; score=0.9
- Line 62: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Only set default inference function if mock mode is explicitly enabled
  Confidence: band=very_high; score=0.9
- Line 64: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inference_fn_ = defaultInference;
  Confidence: band=very_high; score=0.9
- Line 70: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // In production mode, require inference function to be set
  Confidence: band=very_high; score=0.9
- Line 71: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inference_fn_ = nullptr;
  Confidence: band=very_high; score=0.9
- Line 72: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: THEMIS_INFO("LLMJudgeIntegration initialized - inference function must be set before use");
  Confidence: band=very_high; score=0.9
- Line 78: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const EvaluationInput& input,
  Confidence: band=very_high; score=0.9
- Line 84: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::string prompt = template_mgr.generatePrompt(dimension, input);
  Confidence: band=very_high; score=0.9
- Line 90: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: error_response.error_message = "Failed to generate prompt";
  Confidence: band=very_high; score=0.9
- Line 173: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void LLMJudgeIntegration::setInferenceFunction(
  Confidence: band=very_high; score=0.9
- Line 176: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inference_fn_ = fn;
  Confidence: band=very_high; score=0.9
- Line 178: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: THEMIS_INFO("Custom inference function set for LLM judge");
  Confidence: band=very_high; score=0.9
- Line 190: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (!inference_fn_) {
  Confidence: band=very_high; score=0.9
- Line 192: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string error_msg = "No inference function set. ";
  Confidence: band=very_high; score=0.9
- Line 193: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: error_msg += "Options: (1) call setInferenceFunction() with a valid LLM inference function; ";
  Confidence: band=very_high; score=0.9
- Line 194: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: error_msg += "(2) pass an ILLMInferenceEngine* to the constructor; ";
  Confidence: band=very_high; score=0.9
- Line 208: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Call the inference function
  Confidence: band=very_high; score=0.9
- Line 209: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string response = inference_fn_(prompt);
  Confidence: band=very_high; score=0.9
- Line 216: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string LLMJudgeIntegration::defaultInference(const std::string& prompt) {
  Confidence: band=very_high; score=0.9
- Line 219: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: //          ILLMInferenceEngine is injected, enabling unit tests and offline
  Confidence: band=very_high; score=0.9
- Line 226: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: //                   still non-model mock outputs. As of 2026-04-21 the caller
  Confidence: band=very_high; score=0.9
- Line 236: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: THEMIS_DEBUG("Using mock inference function (for testing only)");
  Confidence: band=very_high; score=0.9
- Line 39: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: // Wire the engine's generate() into the inference function slot
  Confidence: band=high; score=0.74
- Line 41: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: return engine->generate(prompt);
  Confidence: band=high; score=0.74

### src/rag/continuous_learning_orchestrator.cpp
Total findings: 30

- Line 172: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->learning_loop_active = true;
- Line 173: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->learning_thread = std::make_unique<std::thread>(&ContinuousLearningOrchestrator::learningLoop
- Line 181: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->learning_loop_active = false;
- Line 1227: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: auto bao = bao_weak.lock();
- Line 1247: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: auto workload = workload_weak.lock();
- Line 1262: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: auto feedback = feedback_weak.lock();
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
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3355 [rag] Online learning from ... (2026-03-12) | #1270 Implement Continuou
- Line 138: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: impl_->data_selector =
- Line 493: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto sel_result = impl_->data_selector->run(candidates);
- Line 537: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: impl_->data_selector->getConfig(), metrics);
- Line 538: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: impl_->data_selector->setConfig(updated_cfg);
- Line 708: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::seconds(1));
- Line 760: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: impl_->config.data_selection_config = cfg;
- Line 297: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filtered.push_back(snapshot);
  Confidence: band=high; score=0.74
- Line 297: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filtered.push_back(snapshot);
  Confidence: band=high; score=0.74
- Line 318: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> total_per_version;
  Confidence: band=medium; score=0.66
- Line 319: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> success_per_version;
  Confidence: band=medium; score=0.66
- Line 677: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 861: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 903: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 944: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1122: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if      (c == '"')  out += "\\\"";
- Line 1123: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\\') out += "\\\\";
- Line 1124: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\n') out += "\\n";
- Line 1130: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: static const std::unordered_map<int, std::string> kPhaseNames{
  Confidence: band=medium; score=0.66

### src/rag/llm_integration.cpp
Total findings: 24

- Line 16: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: #include "llm/inference_engine_enhanced.h"
  Confidence: band=very_high; score=0.9
- Line 36: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Static member to hold the inference engine
  Confidence: band=very_high; score=0.9
- Line 37: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: static std::shared_ptr<llm::InferenceEngineEnhanced> g_inference_engine = nullptr;
  Confidence: band=very_high; score=0.9
- Line 51: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: return R"({"score":0.75,"confidence":0.80,"explanation":"Fallback evaluation response (no active inference engine/model)."})";
  Confidence: band=very_high; score=0.9
- Line 100: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void LLMIntegration::setInferenceEngine(std::shared_ptr<llm::InferenceEngineEnhanced> engine) {
  Confidence: band=very_high; score=0.9
- Line 102: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: g_inference_engine = engine;
  Confidence: band=very_high; score=0.9
- Line 103: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: THEMIS_INFO("LLM Integration: Inference engine configured");
  Confidence: band=very_high; score=0.9
- Line 106: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::shared_ptr<llm::InferenceEngineEnhanced> LLMIntegration::getInferenceEngine() {
  Confidence: band=very_high; score=0.9
- Line 108: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: return g_inference_engine;
  Confidence: band=very_high; score=0.9
- Line 121: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto engine = getInferenceEngine();
  Confidence: band=very_high; score=0.9
- Line 127: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm::InferenceRequest req;
  Confidence: band=very_high; score=0.9
- Line 134: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto response = llm::LLMPluginManager::instance().generate(req);
  Confidence: band=very_high; score=0.9
- Line 148: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Create an enhanced inference request
  Confidence: band=very_high; score=0.9
- Line 149: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm::InferenceEngineEnhanced::EnhancedInferenceRequest request;
  Confidence: band=very_high; score=0.9
- Line 151: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.base_request.max_tokens = static_cast<int>(std::min(
- Line 172: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // available (InferenceResponse.logprobs stores per-token log-probs),
  Confidence: band=very_high; score=0.9
- Line 60: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, std::string>& variables
  Confidence: band=medium; score=0.66
- Line 134: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto response = llm::LLMPluginManager::instance().generate(req);
  Confidence: band=high; score=0.74
- Line 239: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: samples.push_back(generate(prompt, sample_options));
  Confidence: band=high; score=0.74
- Line 240: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples.push_back(generate(prompt, sample_options));
- Line 240: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: samples.push_back(generate(prompt, sample_options));
  Confidence: band=high; score=0.74
- Line 342: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> set1(tokens1.begin(), tokens1.end());
  Confidence: band=medium; score=0.66
- Line 343: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> set2(tokens2.begin(), tokens2.end());
  Confidence: band=medium; score=0.66
- Line 300: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: log_sum += std::log(prob);
  Confidence: band=medium; score=0.6

### src/rag/reranker.cpp
Total findings: 24

- Line 14: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: * When a real ONNX cross-encoder model is loaded via loadModel() its
  Confidence: band=very_high; score=0.99
- Line 180: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // loadModel() with a non-empty path.
  Confidence: band=very_high; score=0.99
- Line 218: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: //   auto inputs  = tokenise_pair(query, doc_text, config.max_length);
  Confidence: band=very_high; score=0.99
- Line 276: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const std::string key = impl_->cacheKey(query, doc.id);
- Line 279: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (auto cached = impl_->getCached(key)) {
- Line 348: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: bool CrossEncoderReranker::loadModel(const std::string& model_path) {
  Confidence: band=very_high; score=0.99
- Line 350: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: THEMIS_WARN("CrossEncoderReranker::loadModel called with empty path");
  Confidence: band=very_high; score=0.99
- Line 411: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: reranker->loadModel(model_path);
  Confidence: band=very_high; score=0.99
- Line 428: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: reranker->loadModel(model_path);
  Confidence: band=very_high; score=0.99
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3574 fix: clear all remaining st... (2026-03-12) | #2576 RAG Phase 2: Wire s
- Line 21: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: *   4. Maps the raw score through a sigmoid to obtain a [0, 1] value.
- Line 91: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * document, then maps it through a sigmoid so the result lies in [0,1].
- Line 218: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: //   auto inputs  = tokenise_pair(query, doc_text, config.max_length);
  Confidence: band=very_high; score=0.9
- Line 218: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: //   auto inputs  = tokenise_pair(query, doc_text, config.max_length);
  Confidence: band=very_high; score=0.9
- Line 219: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: //   auto outputs = session_.Run(...);
  Confidence: band=very_high; score=0.9
- Line 220: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: //   return sigmoid(outputs[0]);
  Confidence: band=very_high; score=0.9
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 53: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(cur);
  Confidence: band=high; score=0.74
- Line 66: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> termFreq(
  Confidence: band=medium; score=0.66
- Line 69: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> tf;
  Confidence: band=medium; score=0.66
- Line 284: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scored.push_back({s, i});
  Confidence: band=high; score=0.74
- Line 284: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scored.push_back({s, i});
  Confidence: band=high; score=0.74
- Line 308: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.documents.push_back(std::move(doc));
  Confidence: band=high; score=0.74
- Line 342: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scores.push_back(impl_->computeScore(query, doc));
  Confidence: band=high; score=0.74

### src/rag/prompt_injection_detector.cpp
Total findings: 21

- Line 321: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: PromptInjectionDetector::scanDocuments(const judge::EvaluationInput& input) const
  Confidence: band=very_high; score=0.99
- Line 324: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: results.reserve(input.documents.size());
  Confidence: band=very_high; score=0.99
- Line 325: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (const auto& doc : input.documents) {
  Confidence: band=very_high; score=0.99
- Line 409: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: judge::EvaluationInput
  Confidence: band=very_high; score=0.99
- Line 104: severity=HIGH; category=performance; pattern=regex_in_loop
  Description: std::regex compiled in loop (compile once, reuse)
  Context: for (const auto& e : shared.patterns()) {
  Confidence: band=very_high; score=0.9
- Line 254: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& rule : getRules()) {
- Line 321: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: PromptInjectionDetector::scanDocuments(const judge::EvaluationInput& input) const
  Confidence: band=very_high; score=0.9
- Line 324: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: results.reserve(input.documents.size());
  Confidence: band=very_high; score=0.9
- Line 325: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: for (const auto& doc : input.documents) {
  Confidence: band=very_high; score=0.9
- Line 409: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: judge::EvaluationInput
  Confidence: band=very_high; score=0.9
- Line 105: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: r.push_back({
  Confidence: band=high; score=0.74
- Line 106: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: r.push_back({
- Line 121: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: r.push_back({
- Line 159: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: r.push_back({
- Line 163: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: R"(\bhttps?://[^\s]+\?(?:[^\s]*=\{[^}]*\}|\[PROMPT\]|\[CONTEXT\]|\[OUTPUT\]))",
- Line 201: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: findings.push_back(f);
  Confidence: band=high; score=0.74
- Line 274: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.findings.push_back(f);
  Confidence: band=high; score=0.74
- Line 325: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(scan(doc.content));
  Confidence: band=high; score=0.74
- Line 389: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_replace.push_back(&f);
  Confidence: band=high; score=0.74
- Line 389: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: to_replace.push_back(&f);
  Confidence: band=high; score=0.74
- Line 390: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: to_replace.push_back(&f);

### src/rag/delegate_evaluator.cpp
Total findings: 20

- Line 130: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // This can happen for whitespace/punctuation-only inputs after
  Confidence: band=very_high; score=0.99
- Line 158: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * @note For inputs above 10 000 characters, this function switches to an
  Confidence: band=very_high; score=0.99
- Line 165: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * is used instead, keeping RS computation under 5 ms for 100 KB inputs.
  Confidence: band=very_high; score=0.99
- Line 281: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Fallback to plain-text scoring when input is not valid JSON
  Confidence: band=very_high; score=0.99
- Line 130: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // This can happen for whitespace/punctuation-only inputs after
  Confidence: band=very_high; score=0.9
- Line 130: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // This can happen for whitespace/punctuation-only inputs after
  Confidence: band=very_high; score=0.9
- Line 158: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: * @note For inputs above 10 000 characters, this function switches to an
  Confidence: band=very_high; score=0.9
- Line 158: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * @note For inputs above 10 000 characters, this function switches to an
  Confidence: band=very_high; score=0.9
- Line 165: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: * is used instead, keeping RS computation under 5 ms for 100 KB inputs.
  Confidence: band=very_high; score=0.9
- Line 165: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * is used instead, keeping RS computation under 5 ms for 100 KB inputs.
  Confidence: band=very_high; score=0.9
- Line 281: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Fallback to plain-text scoring when input is not valid JSON
  Confidence: band=very_high; score=0.9
- Line 415: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // ── Validate inputs ──────────────────────────────────────────────────────
  Confidence: band=very_high; score=0.9
- Line 415: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // ── Validate inputs ──────────────────────────────────────────────────────
  Confidence: band=very_high; score=0.9
- Line 68: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {}
- Line 108: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: tokens.push_back(std::move(cur));
  Confidence: band=high; score=0.74
- Line 140: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 223: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: static const std::regex TAG_RE(R"(<([A-Za-z_][A-Za-z0-9_:.-]*)[\s/>])");
- Line 255: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 451: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 480: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/rag/rlaif_trainer.cpp
Total findings: 20

- Line 177: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->judge  = std::make_shared<HeuristicAIJudge>();
- Line 450: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const double n       = static_cast<double>(impl_->stats.successful_steps);
- Line 451: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const double old_avg = impl_->stats.avg_preference_score;
- Line 452: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->stats.avg_preference_score =
- Line 458: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const double n      = static_cast<double>(impl_->stats.total_steps);
- Line 459: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto old_ms   = impl_->stats.avg_step_ms.count();
- Line 461: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->stats.avg_step_ms =
- Line 85: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (lower.find(p) != std::string::npos) {
- Line 118: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "always inferior", "typical of", "all of them", "never capable"};
  Confidence: band=very_high; score=0.9
- Line 199: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: "RLAIFTrainer: min_quality_threshold must be in [0, 1]");
- Line 204: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: "RLAIFTrainer: min_preference_score must be in [0, 1]");
- Line 616: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: impl_->dataset.push_back(synthetic_pair);
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 319: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: rev.critiques.push_back(critique);
  Confidence: band=high; score=0.74
- Line 375: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pair.applied_principles.push_back(p.id);
  Confidence: band=high; score=0.74
- Line 376: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pair.applied_principles.push_back(p.id);
- Line 403: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: step.revision_chain.push_back(rev);
  Confidence: band=high; score=0.74
- Line 469: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 492: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(runTrainingStep(query, draft));
  Confidence: band=high; score=0.74
- Line 523: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, size_t> pv_map;
  Confidence: band=medium; score=0.66

### src/rag/multi_hop_reasoner.cpp
Total findings: 19

- Line 157: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceFn inference_fn) const
  Confidence: band=very_high; score=0.9
- Line 161: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (inference_fn) {
  Confidence: band=very_high; score=0.9
- Line 166: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inference_fn(prompt, config_.max_tokens_per_hop);
  Confidence: band=very_high; score=0.9
- Line 221: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceFn inference_fn) const
  Confidence: band=very_high; score=0.9
- Line 238: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (!inference_fn) {
  Confidence: band=very_high; score=0.9
- Line 257: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const std::string composed = inference_fn(prompt, config_.max_tokens_final);
  Confidence: band=very_high; score=0.9
- Line 268: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: InferenceFn inference_fn) const
  Confidence: band=very_high; score=0.9
- Line 272: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (query.empty() || !retrieval_fn || !inference_fn) {
  Confidence: band=very_high; score=0.9
- Line 280: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: decomposeQuery(query, inference_fn);
  Confidence: band=very_high; score=0.9
- Line 308: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: inference_fn(prompt, config_.max_tokens_per_hop);
  Confidence: band=very_high; score=0.9
- Line 341: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: result.final_answer = composeAnswer(query, result.hop_records, inference_fn);
  Confidence: band=very_high; score=0.9
- Line 78: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 81: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(d);
  Confidence: band=high; score=0.74
- Line 134: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!t.empty()) sentences.push_back(t);
  Confidence: band=high; score=0.74
- Line 148: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: parts.push_back(s);
  Confidence: band=high; score=0.74
- Line 226: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: partial_answers.push_back(
  Confidence: band=high; score=0.74
- Line 227: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: partial_answers.push_back(
- Line 319: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.all_documents.push_back(doc);
  Confidence: band=high; score=0.74
- Line 324: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: previous_answers.push_back(hop.intermediate_answer);

### src/rag/prompt_templates.cpp
Total findings: 19

- Line 80: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const EvaluationInput& input
  Confidence: band=very_high; score=0.99
- Line 99: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: prompt = replacePlaceholders(prompt, input);
  Confidence: band=very_high; score=0.99
- Line 327: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const EvaluationInput& input
  Confidence: band=very_high; score=0.99
- Line 334: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: result.replace(pos, 7, input.query);
  Confidence: band=very_high; score=0.99
- Line 335: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: pos += input.query.length();
  Confidence: band=very_high; score=0.99
- Line 341: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: result.replace(pos, 8, input.generated_answer);
  Confidence: band=very_high; score=0.99
- Line 342: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: pos += input.generated_answer.length();
  Confidence: band=very_high; score=0.99
- Line 347: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (size_t i = 0; i < input.documents.size(); ++i) {
  Confidence: band=very_high; score=0.99
- Line 349: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: << input.documents[i].content << "\n\n";
  Confidence: band=very_high; score=0.99
- Line 80: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const EvaluationInput& input
  Confidence: band=very_high; score=0.9
- Line 99: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: prompt = replacePlaceholders(prompt, input);
  Confidence: band=very_high; score=0.9
- Line 327: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const EvaluationInput& input
  Confidence: band=very_high; score=0.9
- Line 334: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: result.replace(pos, 7, input.query);
  Confidence: band=very_high; score=0.9
- Line 335: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: pos += input.query.length();
  Confidence: band=very_high; score=0.9
- Line 341: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: result.replace(pos, 8, input.generated_answer);
  Confidence: band=very_high; score=0.9
- Line 342: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: pos += input.generated_answer.length();
  Confidence: band=very_high; score=0.9
- Line 347: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: for (size_t i = 0; i < input.documents.size(); ++i) {
  Confidence: band=very_high; score=0.9
- Line 349: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: << input.documents[i].content << "\n\n";
  Confidence: band=very_high; score=0.9
- Line 376: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: stream << "Score: " << ex.score << "/5\n";

### src/rag/fairness_detector.cpp
Total findings: 18

- Line 338: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: bool loaded = impl_->loadEmbeddings(config_.embedding_model_path);
- Line 147: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto male_it = embeddings.find(male_word);
- Line 148: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto female_it = embeddings.find(female_word);
- Line 197: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto low_it = embeddings.find(low_status);
- Line 198: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto high_it = embeddings.find(high_status);
- Line 244: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto left_it = embeddings.find(left_word);
- Line 245: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto right_it = embeddings.find(right_word);
- Line 159: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: difference_vectors.push_back(diff);
  Confidence: band=high; score=0.74
- Line 159: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: difference_vectors.push_back(diff);
  Confidence: band=high; score=0.74
- Line 209: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: difference_vectors.push_back(diff);
  Confidence: band=high; score=0.74
- Line 209: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: difference_vectors.push_back(diff);
  Confidence: band=high; score=0.74
- Line 258: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: difference_vectors.push_back(std::move(diff));
  Confidence: band=high; score=0.74
- Line 258: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: difference_vectors.push_back(std::move(diff));
  Confidence: band=high; score=0.74
- Line 396: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: words.push_back(Impl::toLower(word));
  Confidence: band=high; score=0.74
- Line 397: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: words.push_back(Impl::toLower(word));
- Line 403: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: words.push_back(Impl::toLower(word));
- Line 517: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(detectBias(doc));
  Confidence: band=high; score=0.74
- Line 537: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filtered.emplace_back(doc, bias_score);
  Confidence: band=high; score=0.74

### src/rag/calibration_manager.cpp
Total findings: 17

- Line 185: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: EvaluationInput input;
  Confidence: band=very_high; score=0.99
- Line 186: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.query            = ann.query;
  Confidence: band=very_high; score=0.99
- Line 187: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.generated_answer = ann.answer;
  Confidence: band=very_high; score=0.99
- Line 188: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: predictions.push_back(judge.evaluate(input));
  Confidence: band=very_high; score=0.99
- Line 448: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: bool CalibrationManager::loadModel(const std::string& filepath) {
  Confidence: band=very_high; score=0.99
- Line 185: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: EvaluationInput input;
  Confidence: band=very_high; score=0.9
- Line 186: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.query            = ann.query;
  Confidence: band=very_high; score=0.9
- Line 187: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.generated_answer = ann.answer;
  Confidence: band=very_high; score=0.9
- Line 188: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: predictions.push_back(judge.evaluate(input));
  Confidence: band=very_high; score=0.9
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 77: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ann.annotators.push_back(a.get<std::string>());
  Confidence: band=high; score=0.74
- Line 77: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ann.annotators.push_back(a.get<std::string>());
  Confidence: band=high; score=0.74
- Line 78: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ann.annotators.push_back(a.get<std::string>());
- Line 157: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: blocks.push_back(b);
  Confidence: band=high; score=0.74
- Line 187: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: predictions.push_back(judge.evaluate(input));
  Confidence: band=high; score=0.74
- Line 292: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pred_overall.push_back(p);
  Confidence: band=high; score=0.74
- Line 104: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: double logit = std::log(s / (1.0 - s));
  Confidence: band=medium; score=0.6

### src/rag/agentic_rag.cpp
Total findings: 15

- Line 355: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: THEMIS_DEBUG("AgenticRAG iter {}: retrieved {} new docs for query='{}'",
- Line 359: severity=CRITICAL; category=smart_ptr_misuse
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
- Line 51: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string>& ids)
  Confidence: band=medium; score=0.66
- Line 63: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string>& seen_ids,
  Confidence: band=medium; score=0.66
- Line 70: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: accumulator.push_back(doc);
  Confidence: band=high; score=0.74
- Line 297: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: gap_docs.push_back(toGapDoc(d));
  Confidence: band=high; score=0.74
- Line 416: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!seed.empty()) seed += '\n';
  Confidence: band=high; score=0.74
- Line 416: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if (!seed.empty()) seed += '\n';
  Confidence: band=high; score=0.74
- Line 417: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!seed.empty()) seed += '\n';
- Line 435: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/rag/continuous_learning_client.cpp
Total findings: 15

- Line 55: severity=CRITICAL; category=no_timeout
  Description: thread_join without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: batch_thread.join();
- Line 214: severity=CRITICAL; category=no_timeout
  Description: mutex_lock without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: impl_->batch_mutex.lock();
- Line 205: severity=HIGH; category=no_retry_logic
  Description: socket_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: std::vector<QualityMetric> to_send(
- Line 383: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: faithfulness.metadata["mode"] = static_cast<int>(result.mode);
- Line 412: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: overall.metadata["decision"] = static_cast<int>(result.decision);
- Line 413: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: overall.metadata["passed_threshold"] = result.passed_threshold;
- Line 421: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: latency.metadata["mode"] = static_cast<int>(result.mode);
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 107: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: payload.push_back(metric_json);
  Confidence: band=high; score=0.74
- Line 138: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: values.push_back(metric.value);
  Confidence: band=high; score=0.74
- Line 139: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: values.push_back(metric.value);
- Line 200: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: impl_->metric_batch.push_back(metric);
  Confidence: band=high; score=0.74
- Line 453: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: combined += "; ";
  Confidence: band=high; score=0.74
- Line 454: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: combined += "; ";

### src/rag/multimodal_rag.cpp
Total findings: 15

- Line 238: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: src.image_path      = img_it->second.image_path;
- Line 239: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: src.metadata        = img_it->second.metadata;
- Line 243: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: src.caption = img_it->second.caption;
- Line 245: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: src.caption = impl_->image_captioner(img_it->second);
- Line 136: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function query without trace point
  Context: MultiModalRAGResult MultiModalRAG::query(const MultiModalQuery& mq) const {
  Confidence: band=very_high; score=0.9
- Line 136: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: MultiModalRAGResult MultiModalRAG::query(const MultiModalQuery& mq) const {
  Confidence: band=high; score=0.74
- Line 166: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: text_ranked.emplace_back(doc.id, doc.similarity_score);
  Confidence: band=high; score=0.74
- Line 166: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: text_ranked.emplace_back(doc.id, doc.similarity_score);
  Confidence: band=high; score=0.74
- Line 174: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, ImageDocument> image_doc_map;
  Confidence: band=medium; score=0.66
- Line 184: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: image_ranked.emplace_back(img.id, img.relevance_score);
  Confidence: band=high; score=0.74
- Line 224: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> used_ids;
  Confidence: band=medium; score=0.66
- Line 296: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if      (src.modality == Modality::TEXT)  text_sources.push_back(&src);
  Confidence: band=high; score=0.74
- Line 297: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if      (src.modality == Modality::TEXT)  text_sources.push_back(&src);
- Line 298: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: else if (src.modality == Modality::IMAGE) image_sources.push_back(&src);
- Line 299: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: else if (src.modality == Modality::TABLE) table_sources.push_back(&src);

### src/rag/geval_evaluator.cpp
Total findings: 13

- Line 353: severity=CRITICAL; category=audit_logging; pattern=sensitive_data_logging
  Description: Potential PII/credential logging: token
  Context: reasoning << "Token probability distribution:\n";
  Confidence: band=very_high; score=0.92
- Line 16: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: #include "llm/inference_engine_enhanced.h"
  Confidence: band=very_high; score=0.9
- Line 51: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::shared_ptr<llm::InferenceEngineEnhanced> llm;
  Confidence: band=very_high; score=0.9
- Line 56: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm::InferenceEngineEnhanced::Config engine_cfg;
  Confidence: band=very_high; score=0.9
- Line 57: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm = std::make_shared<llm::InferenceEngineEnhanced>(engine_cfg);
  Confidence: band=very_high; score=0.9
- Line 230: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm::InferenceEngineEnhanced::EnhancedInferenceRequest req;
  Confidence: band=very_high; score=0.9
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['                std::string tok;', '                size_t idx = 0;', '                while (iss >> tok && idx < response.logprobs.size()) {', '                    // kNumScoreLevels ≤ 9 so single-digit check is safe', "                    char max_digit = static_cast<char>('0' + kNumScoreLevels);"]
  Confidence: band=medium; score=0.62
- Line 128: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: score_tokens.push_back(token_id);
  Confidence: band=high; score=0.74
- Line 128: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: score_tokens.push_back(token_id);
  Confidence: band=high; score=0.74
- Line 137: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: score_tokens.push_back(-1);  // Token not in vocabulary; skip during probability extraction
- Line 269: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 312: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: all_probabilities.push_back(probs);
  Confidence: band=high; score=0.74
- Line 466: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<int, int> counts;
  Confidence: band=medium; score=0.66

### src/rag/quality_control_pipeline.cpp
Total findings: 13

- Line 124: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto passed_fast_count = static_cast<double>(impl_->stats.passed_fast);
- Line 126: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->stats.avg_fast_time_ms =
- Line 522: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->learning_callback = callback;
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3310 [rag] Wire CitationHighligh... (2026-03-12) | #1273 Analysis: Duplicate
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 242: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: doc_pairs.emplace_back(doc.id, doc.content);
  Confidence: band=high; score=0.74
- Line 296: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: doc_pairs.emplace_back(doc.id, doc.content);
  Confidence: band=high; score=0.74
- Line 313: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.dimension_scores.push_back(score);
  Confidence: band=high; score=0.74
- Line 326: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.failure_reasons.push_back(
- Line 353: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: doc_pairs.emplace_back(doc.id, doc.content);
  Confidence: band=high; score=0.74
- Line 403: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: source_chunks.push_back(std::move(sc));
  Confidence: band=high; score=0.74
- Line 456: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> weights = {
  Confidence: band=medium; score=0.66

### src/rag/response_parser.cpp
Total findings: 13

- Line 12: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: * @brief Implementation of response parsing for LLM judge outputs
  Confidence: band=very_high; score=0.9
- Line 203: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("Score {} out of valid range [0, 5]", score);
- Line 208: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_WARN("Confidence {} out of valid range [0, 1]", *parsed.confidence);
- Line 217: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (max_range == min_range) {
  Confidence: band=very_high; score=0.9
- Line 32: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: const char* ResponseParser::SCORE_PATTERN_1 = R"((?:score|rating)[\s:]+([0-9.]+)(?:/5|%|\s|$))";
- Line 33: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: const char* ResponseParser::SCORE_PATTERN_2 = R"(([0-9.]+)\s*(?:out of|/)\s*([0-9.]+))";
- Line 116: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.supporting_claims.push_back(claim.get<std::string>());
  Confidence: band=high; score=0.74
- Line 117: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.supporting_claims.push_back(claim.get<std::string>());
- Line 122: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.unsupported_claims.push_back(claim.get<std::string>());
  Confidence: band=high; score=0.74
- Line 123: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.unsupported_claims.push_back(claim.get<std::string>());
- Line 246: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 262: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 272: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/rag/claim_extractor.cpp
Total findings: 10

- Line 54: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string response = LLMIntegration::generate(prompt);
  Confidence: band=very_high; score=0.9
- Line 113: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string response = LLMIntegration::generate(prompt);
  Confidence: band=very_high; score=0.9
- Line 246: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string llm_response = LLMIntegration::generate(prompt);
  Confidence: band=very_high; score=0.9
- Line 54: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string response = LLMIntegration::generate(prompt);
  Confidence: band=high; score=0.74
- Line 113: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string response = LLMIntegration::generate(prompt);
  Confidence: band=high; score=0.74
- Line 156: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(verify(claim, documents));
  Confidence: band=high; score=0.74
- Line 246: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string llm_response = LLMIntegration::generate(prompt);
  Confidence: band=high; score=0.74
- Line 251: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.agreements.push_back("General agreement found");
  Confidence: band=high; score=0.74
- Line 252: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.agreements.push_back("General agreement found");
- Line 256: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.disagreements.push_back("Some disagreements found");

### src/rag/evaluation_cache.cpp
Total findings: 10

- Line 245: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: RAGJudge& judge, const std::vector<EvaluationInput>& queries) {
  Confidence: band=very_high; score=0.99
- Line 248: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (const auto& input : queries) {
  Confidence: band=very_high; score=0.99
- Line 250: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (contains(input.query, input.generated_answer)) continue;
  Confidence: band=very_high; score=0.99
- Line 252: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto result = judge.evaluate(input);
  Confidence: band=very_high; score=0.99
- Line 253: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: put(input.query, input.generated_answer, result);
  Confidence: band=very_high; score=0.99
- Line 245: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: RAGJudge& judge, const std::vector<EvaluationInput>& queries) {
  Confidence: band=very_high; score=0.9
- Line 248: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: for (const auto& input : queries) {
  Confidence: band=very_high; score=0.9
- Line 250: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (contains(input.query, input.generated_answer)) continue;
  Confidence: band=very_high; score=0.9
- Line 252: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto result = judge.evaluate(input);
  Confidence: band=very_high; score=0.9
- Line 253: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: put(input.query, input.generated_answer, result);
  Confidence: band=very_high; score=0.9

### src/rag/onnx_model_loader.cpp
Total findings: 10

- Line 125: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: return loadModel(dest_path);
  Confidence: band=very_high; score=0.99
- Line 284: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: info.input_shape = {1, 512};  // batch_size=1, seq_len=512
  Confidence: band=very_high; score=0.99
- Line 295: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: info.input_shape = {1, 512};
  Confidence: band=very_high; score=0.99
- Line 306: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: info.input_shape = {1, 1024};
  Confidence: band=very_high; score=0.99
- Line 186: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = impl_->cache.find(model_name);
- Line 284: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: info.input_shape = {1, 512};  // batch_size=1, seq_len=512
  Confidence: band=very_high; score=0.9
- Line 295: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: info.input_shape = {1, 512};
  Confidence: band=very_high; score=0.9
- Line 306: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: info.input_shape = {1, 1024};
  Confidence: band=very_high; score=0.9
- Line 170: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: models.push_back(name);
  Confidence: band=high; score=0.74
- Line 262: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: fclose(fp);

### src/rag/replug_retriever.cpp
Total findings: 9

- Line 111: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: "ReplugRetriever: llm_weight must be in [0, 1]");
- Line 178: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (range == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 254: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = weights_.find(doc.id);
- Line 254: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = weights_.find(doc.id);
- Line 343: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (w == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 60: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: double jaccardSimilarity(const std::unordered_set<std::string>& a,
  Confidence: band=medium; score=0.66
- Line 61: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string>& b) {
  Confidence: band=medium; score=0.66
- Line 257: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ret_scores.push_back(doc.similarity_score * w);
  Confidence: band=high; score=0.74
- Line 313: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.documents.push_back(std::move(doc));
  Confidence: band=high; score=0.74

### src/rag/explainability_reason_builder.cpp
Total findings: 8

- Line 159: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: "conditions (miss rate, profile drift, or new entry count).",
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 136: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "Local adapter weights updated; next inference cycle will use "
  Confidence: band=very_high; score=0.9
- Line 145: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: "Adapter selected and loaded for inference.",
  Confidence: band=very_high; score=0.9
- Line 352: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(std::move(rec));
  Confidence: band=high; score=0.74
- Line 352: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(std::move(rec));
  Confidence: band=high; score=0.74
- Line 359: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged.push_back(std::move(rec));
  Confidence: band=high; score=0.74
- Line 364: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: // Sort by timestamp (ascending — oldest first)
  Confidence: band=high; score=0.74

### src/rag/quality_control_factory.cpp
Total findings: 8

- Line 72: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Create LLM Judge Client if inference engine available
  Confidence: band=very_high; score=0.9
- Line 74: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (setup_config.enable_llm_judge && setup_config.inference_engine) {
  Confidence: band=very_high; score=0.9
- Line 75: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm_judge_client = createLLMJudgeClient(setup_config.inference_engine);
  Confidence: band=very_high; score=0.9
- Line 77: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: THEMIS_WARN("LLM Judge requested but no inference engine provided");
  Confidence: band=very_high; score=0.9
- Line 151: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::shared_ptr<llm::InferenceEngineEnhanced> inference_engine
  Confidence: band=very_high; score=0.9
- Line 153: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (!inference_engine) {
  Confidence: band=very_high; score=0.9
- Line 154: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: THEMIS_ERROR("Cannot create LLM Judge Client: inference engine is null");
  Confidence: band=very_high; score=0.9
- Line 166: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: client->setInferenceEngine(std::move(inference_engine));
  Confidence: band=very_high; score=0.9

### src/rag/rag_ingestion_bridge.cpp
Total findings: 8

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 79: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: spdlog::warn("RAGIngestionBridge::indexDocument rejected: empty input");
  Confidence: band=very_high; score=0.99
- Line 82: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: .error = "empty input"
  Confidence: band=very_high; score=0.99
- Line 79: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: spdlog::warn("RAGIngestionBridge::indexDocument rejected: empty input");
  Confidence: band=very_high; score=0.9
- Line 82: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: .error = "empty input"
  Confidence: band=very_high; score=0.9
- Line 101: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result = engine->execute(ctx);
- Line 162: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: : trimCopy(chunk.metadata["content"]);
- Line 138: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entity_set.chunks.push_back(std::move(fallback_chunk));
  Confidence: band=high; score=0.74

### src/rag/cot_evaluator.cpp
Total findings: 7

- Line 50: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = 0; i < std::min(documents.size(), size_t(3)); ++i) {
- Line 194: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (conclusion_i.find(neg) != std::string::npos) i_has_negation = true;
- Line 195: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (conclusion_i.find(neg) != std::string::npos) i_has_negation = true;
  Confidence: band=very_high; score=0.9
- Line 196: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (conclusion_j.find(neg) != std::string::npos) j_has_negation = true;
  Confidence: band=very_high; score=0.9
- Line 123: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: steps.push_back(step);
  Confidence: band=high; score=0.74
- Line 195: severity=MEDIUM; category=expensive_copy
  Description: Unnecessary expensive copy
  Remediation: Use const reference (const T&) or std::move if transfer is needed
  Context: if (conclusion_i.find(neg) != std::string::npos) i_has_negation = true;
- Line 223: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: inconsistencies.push_back(inconsistency.str());

### src/rag/faithfulness_evaluator.cpp
Total findings: 7

- Line 114: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->nli_verifier = std::make_shared<NLIFaithfulnessVerifier>(nli_config);
- Line 151: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: claims.push_back(std::move(claim));
  Confidence: band=high; score=0.74
- Line 175: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: claims.push_back(claim);
  Confidence: band=high; score=0.74
- Line 294: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: claim.supporting_doc_ids.push_back(doc_id);
  Confidence: band=high; score=0.74
- Line 294: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: claim.supporting_doc_ids.push_back(doc_id);
  Confidence: band=high; score=0.74
- Line 330: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: explanation << "Claims: " << result.supported_claims_count << "/" << result.total_claims_count << "
- Line 330: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: explanation << "Claims: " << result.supported_claims_count << "/" << result.total_claims_count << "

### src/rag/hybrid_retriever.cpp
Total findings: 7

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #2747 [rag] Hybrid retrieval (BM2... (2026-03-12)
- Line 43: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (range == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 169: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vector_candidates.push_back(std::move(dense_doc));
  Confidence: band=high; score=0.74
- Line 253: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.documents.push_back(std::move(e.doc));
  Confidence: band=high; score=0.74
- Line 275: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, DocData> doc_map;
  Confidence: band=medium; score=0.66
- Line 288: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vec_raw.push_back(src.similarity_score);
  Confidence: band=high; score=0.74
- Line 334: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.documents.push_back(std::move(e.doc));
  Confidence: band=high; score=0.74

### src/rag/relevance_evaluator.cpp
Total findings: 7

- Line 62: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = freq.find(vocab[i]);
- Line 62: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it = freq.find(vocab[i]);
- Line 215: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (query_lower.find(kw) != std::string::npos) {
- Line 57: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> freq;
  Confidence: band=medium; score=0.66
- Line 157: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: questions.push_back(question.get<std::string>());
  Confidence: band=high; score=0.74
- Line 158: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: questions.push_back(question.get<std::string>());
- Line 265: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: irrelevant_segments.push_back(sentence);
  Confidence: band=high; score=0.74

### src/rag/self_rag.cpp
Total findings: 7

- Line 71: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> doc_terms(d_tokens.begin(), d_tokens.end());
  Confidence: band=medium; score=0.66
- Line 127: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: static const std::unordered_set<std::string> evidence_terms = {
  Confidence: band=medium; score=0.66
- Line 198: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen(seen_ids_.begin(), seen_ids_.end());
  Confidence: band=medium; score=0.66
- Line 203: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fresh.push_back(std::move(doc));
  Confidence: band=high; score=0.74
- Line 239: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: seen_ids_.push_back(d.id);
  Confidence: band=high; score=0.74
- Line 239: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: seen_ids_.push_back(d.id);
  Confidence: band=high; score=0.74
- Line 253: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.relevant_docs.push_back(r);
  Confidence: band=high; score=0.74

### src/rag/bayesian_optimizer.cpp
Total findings: 6

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 73: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> BayesianOptimizer::getBestParams() const {
  Confidence: band=medium; score=0.66
- Line 85: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> BayesianOptimizer::sampleRandom() {
  Confidence: band=medium; score=0.66
- Line 86: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> params;
  Confidence: band=medium; score=0.66
- Line 96: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> BayesianOptimizer::sampleAroundBest() {
  Confidence: band=medium; score=0.66
- Line 97: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> params;
  Confidence: band=medium; score=0.66

### src/rag/coherence_evaluator.cpp
Total findings: 6

- Line 317: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (sent_i.find(neg) != std::string::npos) i_has_negation = true;
  Confidence: band=very_high; score=0.9
- Line 318: severity=HIGH; category=performance; pattern=nested_loop_find
  Description: O(n²) pattern: linear search inside nested loop
  Context: if (sent_j.find(neg) != std::string::npos) j_has_negation = true;
  Confidence: band=very_high; score=0.9
- Line 291: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sentences.push_back(it->str());
  Confidence: band=high; score=0.74
- Line 292: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sentences.push_back(it->str());
- Line 317: severity=MEDIUM; category=expensive_copy
  Description: Unnecessary expensive copy
  Remediation: Use const reference (const T&) or std::move if transfer is needed
  Context: if (sent_i.find(neg) != std::string::npos) i_has_negation = true;
- Line 342: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: contradictions.push_back(sentences[i] + " <-> " + sentences[j]);

### src/rag/examples/loop_orchestration_example.cpp
Total findings: 6

- Line 53: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: std::cout << "\n  [ExplainabilityReasonBuilder — PLANNED]\n"
- Line 151: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: std::cout << "\n  [ExplainabilityReasonBuilder — PLANNED — IMPL-B9]\n"
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 147: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cout << "    → " << step << "\n";
- Line 153: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "  Trigger  : QPS spike +3 200 req/s; p99 latency 85 ms\n"
- Line 169: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "\nSee docs/issues/ for implementation specs.\n";

### src/rag/lora_enhanced_retriever.cpp
Total findings: 6

- Line 128: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const double lora_s = scorer_->score(query, doc.content, config_.domain);
- Line 152: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: return std::stod(it->second) >= config_.min_lora_score;
- Line 134: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: doc.metadata["lora_score"]  = std::to_string(lora_s);
- Line 46: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: double jaccardTokens(const std::unordered_set<std::string>& A,
  Confidence: band=medium; score=0.66
- Line 47: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_set<std::string>& B)
  Confidence: band=medium; score=0.66
- Line 153: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/rag/rag_context_assembler.cpp
Total findings: 6

- Line 83: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "RAGContextAssembler::assemble start: input_chunks={} query_chars={} model_ctx={} context_budget={} response_budget={}",
  Confidence: band=very_high; score=0.99
- Line 95: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "RAGContextAssembler::assemble short-circuit: has_context_budget={} input_chunks={} response_tokens_remaining={}",
  Confidence: band=very_high; score=0.99
- Line 12: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: * @brief Budget-aware context assembler for RAG inference.
  Confidence: band=very_high; score=0.9
- Line 83: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "RAGContextAssembler::assemble start: input_chunks={} query_chars={} model_ctx={} context_budget={} response_budget={}",
  Confidence: band=very_high; score=0.9
- Line 95: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "RAGContextAssembler::assemble short-circuit: has_context_budget={} input_chunks={} response_tokens_remaining={}",
  Confidence: band=very_high; score=0.9
- Line 129: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.chunks_used.push_back(chunk);
  Confidence: band=high; score=0.74

### src/rag/citation_highlighter.cpp
Total findings: 5

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3321 [rag] Fix CitationHighlight... (2026-03-12) | #2749 feat(rag): citation
- Line 112: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: bool isDelim = (cfg.sentence_delimiters.find(ch) != std::string::npos);
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 223: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scored.push_back({ci, sim});
  Confidence: band=high; score=0.74
- Line 259: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: mapping.secondary_sources.push_back(sec);
  Confidence: band=high; score=0.74

### src/rag/ontology_aware_retriever.cpp
Total findings: 5

- Line 187: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: const std::string src_type = entityTypeName(src_node->type);
- Line 188: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: const std::string tgt_type = entityTypeName(tgt_node->type);
- Line 88: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> expanded;
  Confidence: band=medium; score=0.66
- Line 97: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> visited = {concept_id};
  Confidence: band=medium; score=0.66
- Line 109: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: queue.push_back(parent);
  Confidence: band=high; score=0.74

### src/rag/pairwise_comparator.cpp
Total findings: 5

- Line 294: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: result.overall_confidence = static_cast<double>(a_votes) / impl_->config.num_samples;
- Line 297: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: result.overall_confidence = static_cast<double>(b_votes) / impl_->config.num_samples;
- Line 32: severity=MEDIUM; category=determinism; pattern=random_unseeded
  Description: RNG engine appears default-constructed without explicit seeding
  Context: std::mt19937 rng;
  Confidence: band=high; score=0.74
- Line 277: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(compareWithLLM(query, documents, answer_a, answer_b, a_first));
  Confidence: band=high; score=0.74
- Line 278: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(compareWithLLM(query, documents, answer_a, answer_b, a_first));

### src/rag/completeness_evaluator.cpp
Total findings: 4

- Line 369: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = 0; i < std::min(result.missing_information.size(), size_t(3)); ++i) {
- Line 157: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: aspects.push_back(aspect);
  Confidence: band=high; score=0.74
- Line 270: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: missing_info.push_back(aspect.aspect_text);
  Confidence: band=high; score=0.74
- Line 271: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: missing_info.push_back(aspect.aspect_text);

### src/rag/flare_retrieval.cpp
Total findings: 4

- Line 17: severity=CRITICAL; category=audit_logging; pattern=sensitive_data_logging
  Description: Potential PII/credential logging: token
  Context: *  1. Emit token t with log-probability log(p(t)).
  Confidence: band=very_high; score=0.92
- Line 18: severity=CRITICAL; category=audit_logging; pattern=sensitive_data_logging
  Description: Potential PII/credential logging: token
  Context: *  2. If log(p(t)) < confidence_threshold → mark token as uncertain.
  Confidence: band=very_high; score=0.92
- Line 180: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string FlareRetrieval::buildQuery() const {
  Confidence: band=high; score=0.74
- Line 221: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/rag/http_metrics_client.cpp
Total findings: 4

- Line 194: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: backoff.wait();
- Line 213: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: backoff.wait();
- Line 124: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, std::string>& headers) {
  Confidence: band=medium; score=0.66
- Line 315: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j["metrics"].push_back(metric_json);
  Confidence: band=high; score=0.74

### src/rag/learning_metrics.cpp
Total findings: 4

- Line 128: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " +/- " << snap.std_accuracy << "\n";
- Line 128: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " +/- " << snap.std_accuracy << "\n";
- Line 130: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " +/- " << snap.std_faithfulness << "\n";
- Line 130: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " +/- " << snap.std_faithfulness << "\n";

### src/rag/nli_faithfulness_verifier.cpp
Total findings: 4

- Line 358: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: void NLIFaithfulnessVerifier::loadModel(const std::string& model_path) {
  Confidence: band=very_high; score=0.99
- Line 180: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: claims.push_back(claim);
  Confidence: band=high; score=0.74
- Line 276: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: claim_result.supporting_doc_ids.push_back(best_document_id);
  Confidence: band=high; score=0.74
- Line 298: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.claims.push_back(claim_result);
  Confidence: band=high; score=0.74

### src/rag/tensor_rag_pipeline.cpp
Total findings: 4

- Line 129: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: "threw for FLARE query (len=%zu); embedding left empty "
- Line 129: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function query without trace point
  Context: "threw for FLARE query (len=%zu); embedding left empty "
  Confidence: band=very_high; score=0.9
- Line 123: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 129: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: "threw for FLARE query (len=%zu); embedding left empty "
  Confidence: band=high; score=0.74

### src/rag/bias_detector.cpp
Total findings: 3

- Line 131: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scores.push_back(score);
  Confidence: band=high; score=0.74
- Line 133: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: lengths.push_back(static_cast<double>(length));
- Line 173: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: score_length_pairs.emplace_back(eval.overall_score, estimated_length);
  Confidence: band=high; score=0.74

### src/rag/judge_config.cpp
Total findings: 3

- Line 166: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: THEMIS_ERROR("quality_threshold must be in [0, 1], got {}", quality_threshold);
- Line 186: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 198: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/rag/rubric_evaluator.cpp
Total findings: 3

- Line 63: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (size_t i = 0; i < std::min(documents.size(), size_t(3)); ++i) {
- Line 137: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dim.levels.push_back(level);
  Confidence: band=high; score=0.74
- Line 137: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: dim.levels.push_back(level);
  Confidence: band=high; score=0.74

### src/rag/streaming_retriever.cpp
Total findings: 2

- Line 38: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> tokens;
  Confidence: band=medium; score=0.66
- Line 273: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.skipped_documents.push_back(doc);
  Confidence: band=high; score=0.74

### src/rag/ab_testing_framework.cpp
Total findings: 1

- Line 146: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active.push_back(test_id);
  Confidence: band=high; score=0.74

### src/rag/adaptive_retrieval.cpp
Total findings: 1

- Line 78: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: while (ss >> tok) tokens.push_back(tok);
  Confidence: band=high; score=0.74

### src/rag/document_splitter.cpp
Total findings: 1

- Line 84: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sentences.emplace_back(sent.substr(ltrim),
  Confidence: band=high; score=0.74

### src/rag/hallucination_dashboard.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #2753 [rag] Hallucination rate tr... (2026-03-12) | #2613 feat(rag): hallucin

### src/rag/judge_ensemble.cpp
Total findings: 1

- Line 12: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Deprecated shim: functionality now lives in rag_rag_judge.cpp (JudgeEnsemble implementation).
  Confidence: band=high; score=0.8

### src/rag/targ_retrieval.cpp
Total findings: 1

- Line 141: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: if (p > 0.0f) entropy -= p * std::log(p);
  Confidence: band=medium; score=0.6

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
