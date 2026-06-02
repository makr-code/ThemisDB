# aql Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: aql
- Generated: 2026-06-02 11:09:12
- Status: Critical Findings Present
- Total Findings: 275
- Actionable Findings (Critical + High): 157
- Affected Files: 21

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 44 |
| High | 113 |
| Medium | 118 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| llm_ai_safety | 167 |
| container | 118 |
| performance_patterns | 80 |
| reliability | 52 |
| platform | 33 |
| determinism | 21 |
| exception_safety | 14 |
| observability | 14 |
| concurrency | 10 |
| performance | 8 |
| uninitialized | 8 |
| audit_logging | 6 |
| memory | 5 |
| raii | 4 |
| security | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/aql/llm_aql_handler.cpp | 133 | 27 | 83 | 23 | 0 |
| src/aql/aql_agent.cpp | 28 | 9 | 14 | 5 | 0 |
| src/aql/llm_metrics_collector.cpp | 22 | 8 | 14 | 0 | 0 |
| src/aql/aql_autocomplete.cpp | 20 | 0 | 0 | 20 | 0 |
| src/aql/aql_query_diff_explainer.cpp | 9 | 0 | 0 | 9 | 0 |
| src/aql/aql_query_validator.cpp | 8 | 0 | 0 | 8 | 0 |
| src/aql/aql_fewshot_example_library.cpp | 7 | 0 | 0 | 7 | 0 |
| src/aql/aql_query_builder.cpp | 7 | 0 | 0 | 7 | 0 |
| src/aql/classify_bridge.cpp | 7 | 0 | 1 | 6 | 0 |
| src/aql/aql_query_template_library.cpp | 6 | 0 | 0 | 6 | 0 |
| src/aql/aql_migration_assistant.cpp | 5 | 0 | 1 | 4 | 0 |
| src/aql/aql_conversation_context.cpp | 4 | 0 | 0 | 4 | 0 |
| src/aql/aql_model_router.cpp | 4 | 0 | 0 | 4 | 0 |
| src/aql/aql_syntax_highlighter.cpp | 4 | 0 | 0 | 4 | 0 |
| src/aql/docs_assistant_functions.cpp | 4 | 0 | 0 | 4 | 0 |
| src/aql/aql_lora_finetuner.cpp | 3 | 0 | 0 | 3 | 0 |
| src/aql/aql_confidence_scorer.cpp | 1 | 0 | 0 | 1 | 0 |
| src/aql/aql_ingestion_bridge.cpp | 1 | 0 | 0 | 1 | 0 |
| src/aql/aql_optimizer_advisor.cpp | 1 | 0 | 0 | 1 | 0 |
| src/aql/aql_rollback_suggester.cpp | 1 | 0 | 0 | 1 | 0 |
| src/aql/llm_aql_embedding_bridge.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/aql/llm_aql_handler.cpp
Total findings: 133

- Line 152: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * @brief Reject input that contains well-known prompt injection patterns.
  Confidence: band=very_high; score=0.99
- Line 161: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * @param input       The raw user-supplied text.
  Confidence: band=very_high; score=0.99
- Line 165: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * @throws LLMException(PROMPT_TOO_LONG)  when the input exceeds @p max_length.
  Confidence: band=very_high; score=0.99
- Line 184: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const std::string lower = toLower(input);
  Confidence: band=very_high; score=0.99
- Line 627: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Input validation
  Confidence: band=very_high; score=0.99
- Line 758: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t input_tokens  = impl_->token_estimator_->estimate(prompt);
  Confidence: band=very_high; score=0.99
- Line 769: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: {"input_tokens", input_tokens},
  Confidence: band=very_high; score=0.99
- Line 779: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: metrics.recordInference(model_id.empty() ? "default" : model_id, lora_id, latency, input_tokens, output_tokens,
  Confidence: band=very_high; score=0.99
- Line 782: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: spdlog::debug("LLM INFER completed: model={}, latency={}ms, input_tokens={}, output_tokens={}", model_id,
  Confidence: band=very_high; score=0.99
- Line 783: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: latency.count(), input_tokens, output_tokens);
  Confidence: band=very_high; score=0.99
- Line 794: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t input_tokens = impl_->token_estimator_->estimate(prompt);
  Confidence: band=very_high; score=0.99
- Line 796: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: metrics.recordInference(model_id.empty() ? "default" : model_id, lora_id, latency, input_tokens, 0, false,
  Confidence: band=very_high; score=0.99
- Line 838: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Input validation (same as executeInfer)
  Confidence: band=very_high; score=0.99
- Line 882: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t input_tokens  = impl_->token_estimator_->estimate(prompt);
  Confidence: band=very_high; score=0.99
- Line 893: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: {"input_tokens", input_tokens},
  Confidence: band=very_high; score=0.99
- Line 902: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: metrics.recordInference(model_id.empty() ? "default" : model_id, lora_id, latency, input_tokens, output_tokens,
  Confidence: band=very_high; score=0.99
- Line 905: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: spdlog::debug("LLM INFER STREAMING completed: model={}, latency={}ms, input_tokens={}, output_tokens={}",
  Confidence: band=very_high; score=0.99
- Line 906: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: model_id, latency.count(), input_tokens, output_tokens);
  Confidence: band=very_high; score=0.99
- Line 943: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Input validation
  Confidence: band=very_high; score=0.99
- Line 1093: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t input_tokens  = impl_->token_estimator_->estimate(query);
  Confidence: band=very_high; score=0.99
- Line 1104: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: {"input_tokens", input_tokens},
  Confidence: band=very_high; score=0.99
- Line 1210: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: plugin_mgr.loadModel(model_id, path);
  Confidence: band=very_high; score=0.99
- Line 1219: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: plugin_mgr.unloadModel(model_id);
  Confidence: band=very_high; score=0.99
- Line 1237: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: plugin_mgr.loadModel(model_id, blob_urn);
  Confidence: band=very_high; score=0.99
- Line 1567: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::string full_prompt = sys_prompt + user_prompt_str;
  Confidence: band=very_high; score=0.99
- Line 1916: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: prompt << "The user intended:\n[USERINPUT_START]\n" << original_intent << "\n[USERINPUT_END]\n\n";
  Confidence: band=very_high; score=0.99
- Line 1919: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: prompt << "AQL query to evaluate:\n[USERINPUT_START]\n" << aql_query << "\n[USERINPUT_END]\n\n";
  Confidence: band=very_high; score=0.99
- Line 117: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::string batchDomainKey(const LLMAQLHandler::BatchInferRequest &req) {
  Confidence: band=very_high; score=0.9
- Line 152: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * @brief Reject input that contains well-known prompt injection patterns.
  Confidence: band=very_high; score=0.9
- Line 161: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * @param input       The raw user-supplied text.
  Confidence: band=very_high; score=0.9
- Line 165: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * @throws LLMException(PROMPT_TOO_LONG)  when the input exceeds @p max_length.
  Confidence: band=very_high; score=0.9
- Line 184: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const std::string lower = toLower(input);
  Confidence: band=very_high; score=0.9
- Line 275: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto safety_result = config.c1_cai_eval_fn(generated_response, original_query);
  Confidence: band=very_high; score=0.9
- Line 451: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: circuit_breakers_.emplace(std::piecewise_construct, std::forward_as_tuple("infer"),
  Confidence: band=very_high; score=0.9
- Line 452: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::forward_as_tuple(cfg.infer_circuit_breaker));
  Confidence: band=very_high; score=0.9
- Line 627: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Input validation
  Confidence: band=very_high; score=0.9
- Line 633: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (!impl_->getBreaker("infer").allowRequest()) {
  Confidence: band=very_high; score=0.9
- Line 634: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: metrics.recordCircuitBreakerState("infer", "open");
  Confidence: band=very_high; score=0.9
- Line 635: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: throw LLMException(LLMErrorCode::INFERENCE_FAILED,
  Confidence: band=very_high; score=0.9
- Line 640: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto result = impl_->timeout_manager_.executeInferWithCancelToken([&](auto cancel_token) {
  Confidence: band=very_high; score=0.9
- Line 645: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Build inference request with model and LoRA selection
  Confidence: band=very_high; score=0.9
- Line 646: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm::InferenceRequest request;
  Confidence: band=very_high; score=0.9
- Line 748: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto response = plugin_mgr.generate(request);
  Confidence: band=very_high; score=0.9
- Line 758: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t input_tokens  = impl_->token_estimator_->estimate(prompt);
  Confidence: band=very_high; score=0.9
- Line 766: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: {"operation", "infer"},
  Confidence: band=very_high; score=0.9
- Line 769: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: {"input_tokens", input_tokens},
  Confidence: band=very_high; score=0.9
- Line 774: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: LLMErrorCode::INFERENCE_FAILED);
  Confidence: band=very_high; score=0.9
- Line 777: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: impl_->getBreaker("infer").recordSuccess();
  Confidence: band=very_high; score=0.9
- Line 779: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: metrics.recordInference(model_id.empty() ? "default" : model_id, lora_id, latency, input_tokens, output_tokens,
  Confidence: band=very_high; score=0.9
- Line 779: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: metrics.recordInference(model_id.empty() ? "default" : model_id, lora_id, latency, input_tokens, output_tokens,
  Confidence: band=very_high; score=0.9
- Line 782: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: spdlog::debug("LLM INFER completed: model={}, latency={}ms, input_tokens={}, output_tokens={}", model_id,
  Confidence: band=very_high; score=0.9
- Line 782: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::debug("LLM INFER completed: model={}, latency={}ms, input_tokens={}, output_tokens={}", model_id,
  Confidence: band=very_high; score=0.9
- Line 783: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: latency.count(), input_tokens, output_tokens);
  Confidence: band=very_high; score=0.9
- Line 789: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: impl_->getBreaker("infer").recordFailure();
  Confidence: band=very_high; score=0.9
- Line 794: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t input_tokens = impl_->token_estimator_->estimate(prompt);
  Confidence: band=very_high; score=0.9
- Line 796: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: metrics.recordInference(model_id.empty() ? "default" : model_id, lora_id, latency, input_tokens, 0, false,
  Confidence: band=very_high; score=0.9
- Line 796: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: metrics.recordInference(model_id.empty() ? "default" : model_id, lora_id, latency, input_tokens, 0, false,
  Confidence: band=very_high; score=0.9
- Line 799: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::error("LLM INFER failed: model={}, error={}", model_id, e.what());
  Confidence: band=very_high; score=0.9
- Line 805: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: impl_->getBreaker("infer").recordFailure();
  Confidence: band=very_high; score=0.9
- Line 810: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: metrics.recordInference(model_id.empty() ? "default" : model_id, lora_id, latency,
  Confidence: band=very_high; score=0.9
- Line 817: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: impl_->getBreaker("infer").recordFailure();
  Confidence: band=very_high; score=0.9
- Line 822: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: metrics.recordInference(model_id.empty() ? "default" : model_id, lora_id, latency,
  Confidence: band=very_high; score=0.9
- Line 823: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: impl_->token_estimator_->estimate(prompt), 0, false, "INFERENCE_FAILED");
  Confidence: band=very_high; score=0.9
- Line 838: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Input validation (same as executeInfer)
  Confidence: band=very_high; score=0.9
- Line 844: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (!impl_->getBreaker("infer").allowRequest()) {
  Confidence: band=very_high; score=0.9
- Line 845: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: metrics.recordCircuitBreakerState("infer", "open");
  Confidence: band=very_high; score=0.9
- Line 846: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: throw LLMException(LLMErrorCode::INFERENCE_FAILED,
  Confidence: band=very_high; score=0.9
- Line 850: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Build the inference request
  Confidence: band=very_high; score=0.9
- Line 851: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm::InferenceRequest request;
  Confidence: band=very_high; score=0.9
- Line 871: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Tokens are delivered sequentially by the inference thread, so there is
  Confidence: band=very_high; score=0.9
- Line 877: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto response    = plugin_mgr.generate(request);
  Confidence: band=very_high; score=0.9
- Line 882: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t input_tokens  = impl_->token_estimator_->estimate(prompt);
  Confidence: band=very_high; score=0.9
- Line 890: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: {"operation", "infer_streaming"},
  Confidence: band=very_high; score=0.9
- Line 893: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: {"input_tokens", input_tokens},
  Confidence: band=very_high; score=0.9
- Line 898: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: LLMErrorCode::INFERENCE_FAILED);
  Confidence: band=very_high; score=0.9
- Line 900: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: impl_->getBreaker("infer").recordSuccess();
  Confidence: band=very_high; score=0.9
- Line 902: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: metrics.recordInference(model_id.empty() ? "default" : model_id, lora_id, latency, input_tokens, output_tokens,
  Confidence: band=very_high; score=0.9
- Line 902: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: metrics.recordInference(model_id.empty() ? "default" : model_id, lora_id, latency, input_tokens, output_tokens,
  Confidence: band=very_high; score=0.9
- Line 905: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: spdlog::debug("LLM INFER STREAMING completed: model={}, latency={}ms, input_tokens={}, output_tokens={}",
  Confidence: band=very_high; score=0.9
- Line 905: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::debug("LLM INFER STREAMING completed: model={}, latency={}ms, input_tokens={}, output_tokens={}",
  Confidence: band=very_high; score=0.9
- Line 906: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: model_id, latency.count(), input_tokens, output_tokens);
  Confidence: band=very_high; score=0.9
- Line 911: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: impl_->getBreaker("infer").recordFailure();
  Confidence: band=very_high; score=0.9
- Line 916: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: metrics.recordInference(model_id.empty() ? "default" : model_id, lora_id, latency,
  Confidence: band=very_high; score=0.9
- Line 920: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::error("LLM INFER STREAMING failed: model={}, error={}", model_id, e.what());
  Confidence: band=very_high; score=0.9
- Line 923: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: impl_->getBreaker("infer").recordFailure();
  Confidence: band=very_high; score=0.9
- Line 928: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: metrics.recordInference(model_id.empty() ? "default" : model_id, lora_id, latency,
  Confidence: band=very_high; score=0.9
- Line 943: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Input validation
  Confidence: band=very_high; score=0.9
- Line 1051: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Build inference request with RAG context
  Confidence: band=very_high; score=0.9
- Line 1052: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm::InferenceRequest request;
  Confidence: band=very_high; score=0.9
- Line 1083: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto response = plugin_mgr.generateRAG(context, request);
  Confidence: band=very_high; score=0.9
- Line 1093: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t input_tokens  = impl_->token_estimator_->estimate(query);
  Confidence: band=very_high; score=0.9
- Line 1104: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: {"input_tokens", input_tokens},
  Confidence: band=very_high; score=0.9
- Line 1170: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: throw LLMException(LLMErrorCode::INFERENCE_FAILED,
  Confidence: band=very_high; score=0.9
- Line 1184: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm::InferenceRequest request;
  Confidence: band=very_high; score=0.9
- Line 1289: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: oss << "    infer:    " << cb_states.infer << "\n";
  Confidence: band=very_high; score=0.9
- Line 1302: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: states.infer    = sharding::CircuitBreaker::stateToString(impl_->getBreaker("infer").getState());
  Confidence: band=very_high; score=0.9
- Line 1340: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::vector<std::string> LLMAQLHandler::executeBatchInfer(const std::vector<BatchInferRequest> &requests) {
  Confidence: band=very_high; score=0.9
- Line 1362: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: shard_results.emplace_back(idx, executeInfer(req.prompt, req.model_id, req.lora_id, req.options));
  Confidence: band=very_high; score=0.9
- Line 1376: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: throw std::runtime_error(std::string("Batch LLM INFER failed: ") + e.what());
  Confidence: band=very_high; score=0.9
- Line 1452: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Sanitize inputs before embedding them in the LLM prompt.
  Confidence: band=very_high; score=0.9
- Line 1548: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Sanitize inputs (same rules as translateNLToAQL)
  Confidence: band=very_high; score=0.9
- Line 1569: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Stream via executeInferStreaming; collect tokens so we can post-process.
  Confidence: band=very_high; score=0.9
- Line 1723: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: LLMErrorCode::INFERENCE_FAILED);
  Confidence: band=very_high; score=0.9
- Line 1734: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Sanitize inputs before embedding them in the LLM prompt
  Confidence: band=very_high; score=0.9
- Line 1753: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Sanitize inputs before embedding them in the LLM prompt
  Confidence: band=very_high; score=0.9
- Line 1792: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Sanitize inputs (same rules as translateNLToAQL)
  Confidence: band=very_high; score=0.9
- Line 1901: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // LLM-4: sanitize user-supplied inputs before embedding in prompt
  Confidence: band=very_high; score=0.9
- Line 1916: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: prompt << "The user intended:\n[USERINPUT_START]\n" << original_intent << "\n[USERINPUT_END]\n\n";
  Confidence: band=very_high; score=0.9
- Line 1919: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: prompt << "AQL query to evaluate:\n[USERINPUT_START]\n" << aql_query << "\n[USERINPUT_END]\n\n";
  Confidence: band=very_high; score=0.9
- Line 1926: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const std::string response = executeInfer(prompt.str());
  Confidence: band=very_high; score=0.9
- Line 125: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string buildChatOriginalQuery(const std::vector<llm::ChatMessage>& messages) {
  Confidence: band=high; score=0.74
- Line 129: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: combined_user_content += "\n";
  Confidence: band=high; score=0.74
- Line 142: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: all_content += "\n";
  Confidence: band=high; score=0.74
- Line 340: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: referenced_collections.push_back((*it)[1].str());
  Confidence: band=high; score=0.74
- Line 366: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: static std::unordered_set<std::string> extractReferencedCollectionsForAccessCheck(
  Confidence: band=medium; score=0.66
- Line 369: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> collections;
  Confidence: band=medium; score=0.66
- Line 425: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: history_.push_back({nl_query, aql_result});
  Confidence: band=high; score=0.74
- Line 620: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string LLMAQLHandler::executeInfer(const std::string &prompt, const std::string &model_id,
  Confidence: band=high; score=0.74
- Line 748: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto response = plugin_mgr.generate(request);
  Confidence: band=high; score=0.74
- Line 877: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto response    = plugin_mgr.generate(request);
  Confidence: band=high; score=0.74
- Line 1265: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(lora.lora_id.empty() ? lora.id : lora.lora_id);
  Confidence: band=high; score=0.74
- Line 1340: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::vector<std::string> LLMAQLHandler::executeBatchInfer(const std::vector<BatchInferRequest> &requests) {
  Confidence: band=high; score=0.74
- Line 1347: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<size_t>> indices_by_domain;
  Confidence: band=medium; score=0.66
- Line 1361: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard_results.emplace_back(idx, executeInfer(req.prompt, req.model_id, req.lora_id, req.options));
  Confidence: band=high; score=0.74
- Line 1362: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: shard_results.emplace_back(idx, executeInfer(req.prompt, req.model_id, req.lora_id, req.options));
  Confidence: band=high; score=0.74
- Line 1388: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "You are an expert in AQL (Application Query Language) for ThemisDB.\n";
  Confidence: band=high; score=0.74
- Line 1388: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "You are an expert in AQL (Application Query Language) for ThemisDB.\n";
  Confidence: band=high; score=0.74
- Line 1388: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "You are an expert in AQL (Application Query Language) for ThemisDB.\n";
  Confidence: band=high; score=0.74
- Line 1478: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: messages.emplace_back("system", sys_prompt);
  Confidence: band=high; score=0.74
- Line 1575: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: messages.emplace_back("system", sys_prompt);
  Confidence: band=high; score=0.74
- Line 1663: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(result));
  Confidence: band=high; score=0.74
- Line 1820: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: messages.emplace_back("system", sys_prompt);
  Confidence: band=high; score=0.74
- Line 1926: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: const std::string response = executeInfer(prompt.str());
  Confidence: band=high; score=0.74

### src/aql/aql_agent.cpp
Total findings: 28

- Line 93: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::string llm_input = system_prompt + "\n\n" + conversation;
  Confidence: band=very_high; score=0.99
- Line 99: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: raw_response        = handler_->executeInfer(llm_input, config_.model_alias,
  Confidence: band=very_high; score=0.99
- Line 133: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: json tool_output            = invokeTool(*step.tool_name, step.tool_input.value_or(json::object()));
  Confidence: band=very_high; score=0.99
- Line 171: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "Action Input: <JSON arguments>\n\n"
  Confidence: band=very_high; score=0.99
- Line 239: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::string action_input = extract_field("Action Input:");
  Confidence: band=very_high; score=0.99
- Line 243: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (!action_input.empty()) {
  Confidence: band=very_high; score=0.99
- Line 245: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: step.tool_input = json::parse(action_input);
  Confidence: band=very_high; score=0.99
- Line 247: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // If the input is not valid JSON, wrap it as a string argument.
  Confidence: band=very_high; score=0.99
- Line 248: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: step.tool_input = json{{"input", action_input}};
  Confidence: band=very_high; score=0.99
- Line 78: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: AgentResult execute(const std::string &task, const json &context) {
  Confidence: band=very_high; score=0.9
- Line 93: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::string llm_input = system_prompt + "\n\n" + conversation;
  Confidence: band=very_high; score=0.9
- Line 99: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: raw_response        = handler_->executeInfer(llm_input, config_.model_alias,
  Confidence: band=very_high; score=0.9
- Line 99: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: raw_response        = handler_->executeInfer(llm_input, config_.model_alias,
  Confidence: band=very_high; score=0.9
- Line 102: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // LLM inference unavailable (e.g. no model loaded).
  Confidence: band=very_high; score=0.9
- Line 106: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: error_step.thought = std::string("LLM inference unavailable: ") + e.what();
  Confidence: band=very_high; score=0.9
- Line 133: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: json tool_output            = invokeTool(*step.tool_name, step.tool_input.value_or(json::object()));
  Confidence: band=very_high; score=0.9
- Line 171: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "Action Input: <JSON arguments>\n\n"
  Confidence: band=very_high; score=0.9
- Line 239: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::string action_input = extract_field("Action Input:");
  Confidence: band=very_high; score=0.9
- Line 243: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (!action_input.empty()) {
  Confidence: band=very_high; score=0.9
- Line 245: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: step.tool_input = json::parse(action_input);
  Confidence: band=very_high; score=0.9
- Line 247: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // If the input is not valid JSON, wrap it as a string argument.
  Confidence: band=very_high; score=0.9
- Line 248: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: step.tool_input = json{{"input", action_input}};
  Confidence: band=very_high; score=0.9
- Line 293: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: AgentResult ReActAgent::execute(const std::string &task, const json &context) {
  Confidence: band=very_high; score=0.9
- Line 57: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(kv.second);
  Confidence: band=high; score=0.74
- Line 78: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: AgentResult execute(const std::string &task, const json &context) {
  Confidence: band=high; score=0.74
- Line 99: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: raw_response        = handler_->executeInfer(llm_input, config_.model_alias,
  Confidence: band=high; score=0.74
- Line 106: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.reasoning_trace.push_back(error_step);
  Confidence: band=high; score=0.74
- Line 293: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: AgentResult ReActAgent::execute(const std::string &task, const json &context) {
  Confidence: band=high; score=0.74

### src/aql/llm_metrics_collector.cpp
Total findings: 22

- Line 44: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: {"operation", "model", "direction"} // direction: input/output
  Confidence: band=very_high; score=0.99
- Line 89: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::chrono::milliseconds latency, size_t input_tokens, size_t output_tokens,
  Confidence: band=very_high; score=0.99
- Line 99: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: {{"operation", "infer"}, {"model", model_id}, {"direction", "input"}},
  Confidence: band=very_high; score=0.99
- Line 100: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: static_cast<double>(input_tokens));
  Confidence: band=very_high; score=0.99
- Line 133: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: {{"operation", "rag"}, {"model", collection}, {"direction", "input"}},
  Confidence: band=very_high; score=0.99
- Line 134: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: static_cast<double>(input_tokens));
  Confidence: band=very_high; score=0.99
- Line 153: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t input_tokens, bool success, const std::string &error_code) {
  Confidence: band=very_high; score=0.99
- Line 163: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: static_cast<double>(input_tokens));
  Confidence: band=very_high; score=0.99
- Line 44: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: {"operation", "model", "direction"} // direction: input/output
  Confidence: band=very_high; score=0.9
- Line 88: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void LLMMetricsCollector::recordInference(const std::string &model_id, const std::string &lora_id,
  Confidence: band=very_high; score=0.9
- Line 89: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::chrono::milliseconds latency, size_t input_tokens, size_t output_tokens,
  Confidence: band=very_high; score=0.9
- Line 95: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: {{"operation", "infer"}, {"model", model_id}, {"lora", lora_id}});
  Confidence: band=very_high; score=0.9
- Line 99: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: {{"operation", "infer"}, {"model", model_id}, {"direction", "input"}},
  Confidence: band=very_high; score=0.9
- Line 99: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: {{"operation", "infer"}, {"model", model_id}, {"direction", "input"}},
  Confidence: band=very_high; score=0.9
- Line 100: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: static_cast<double>(input_tokens));
  Confidence: band=very_high; score=0.9
- Line 103: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: {{"operation", "infer"}, {"model", model_id}, {"direction", "output"}},
  Confidence: band=very_high; score=0.9
- Line 109: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: {{"operation", "infer"}, {"model", model_id}, {"status", success ? "success" : "failure"}});
  Confidence: band=very_high; score=0.9
- Line 114: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: {{"operation", "infer"}, {"model", model_id}, {"error_code", error_code}});
  Confidence: band=very_high; score=0.9
- Line 133: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: {{"operation", "rag"}, {"model", collection}, {"direction", "input"}},
  Confidence: band=very_high; score=0.9
- Line 134: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: static_cast<double>(input_tokens));
  Confidence: band=very_high; score=0.9
- Line 153: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t input_tokens, bool success, const std::string &error_code) {
  Confidence: band=very_high; score=0.9
- Line 163: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: static_cast<double>(input_tokens));
  Confidence: band=very_high; score=0.9

### src/aql/aql_autocomplete.cpp
Total findings: 20

- Line 292: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vars.push_back((*it)[1].str());
  Confidence: band=high; score=0.74
- Line 304: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 307: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: unique_vars.push_back(v);
  Confidence: band=high; score=0.74
- Line 355: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: info.fields.push_back((*fit)[0].str());
  Confidence: band=high; score=0.74
- Line 355: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: info.fields.push_back((*fit)[0].str());
  Confidence: band=high; score=0.74
- Line 371: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(info));
  Confidence: band=high; score=0.74
- Line 394: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(std::move(item));
  Confidence: band=high; score=0.74
- Line 405: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(std::move(item));
  Confidence: band=high; score=0.74
- Line 439: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(std::move(item));
  Confidence: band=high; score=0.74
- Line 459: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(std::move(item));
  Confidence: band=high; score=0.74
- Line 501: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(std::move(item));
  Confidence: band=high; score=0.74
- Line 501: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(std::move(item));
  Confidence: band=high; score=0.74
- Line 510: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 519: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(std::move(item));
  Confidence: band=high; score=0.74
- Line 519: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(std::move(item));
  Confidence: band=high; score=0.74
- Line 541: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filtered.push_back(std::move(item));
  Confidence: band=high; score=0.74
- Line 602: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(e.keyword);
  Confidence: band=high; score=0.74
- Line 605: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(e.keyword);
  Confidence: band=high; score=0.74
- Line 608: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(e.keyword);
  Confidence: band=high; score=0.74
- Line 616: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(e.name);
  Confidence: band=high; score=0.74

### src/aql/aql_query_diff_explainer.cpp
Total findings: 9

- Line 34: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += ' ';
  Confidence: band=high; score=0.74
- Line 62: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> splitClauses(const std::string &norm) {
  Confidence: band=medium; score=0.66
- Line 63: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> clauses;
  Confidence: band=medium; score=0.66
- Line 85: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matches.push_back({p, kw});
  Confidence: band=high; score=0.74
- Line 165: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> detectFunctions(const std::string &norm) {
  Confidence: band=medium; score=0.66
- Line 168: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> fns;
  Confidence: band=medium; score=0.66
- Line 213: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> all_keys;
  Confidence: band=medium; score=0.66
- Line 266: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.diffs.push_back(std::move(e));
  Confidence: band=high; score=0.74
- Line 275: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.diffs.push_back(std::move(e));
  Confidence: band=high; score=0.74

### src/aql/aql_query_validator.cpp
Total findings: 8

- Line 111: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vars.push_back((*it)[1].str());
  Confidence: band=high; score=0.74
- Line 123: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vars.push_back((*it)[1].str());
  Confidence: band=high; score=0.74
- Line 217: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.issues.push_back({ValidationIssue::Severity::ERROR,
  Confidence: band=high; score=0.74
- Line 318: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.issues.push_back({ValidationIssue::Severity::WARNING,
  Confidence: band=high; score=0.74
- Line 338: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> var_to_collection;
  Confidence: band=medium; score=0.66
- Line 356: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<std::string>> collection_fields;
  Confidence: band=medium; score=0.66
- Line 362: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: collection_fields[col_lower].push_back(field_lower);
  Confidence: band=high; score=0.74
- Line 362: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: collection_fields[col_lower].push_back(field_lower);
  Confidence: band=high; score=0.74

### src/aql/aql_fewshot_example_library.cpp
Total findings: 7

- Line 51: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(ex);
  Confidence: band=high; score=0.74
- Line 67: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(ex);
  Confidence: band=high; score=0.74
- Line 67: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(ex);
  Confidence: band=high; score=0.74
- Line 117: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scored.emplace_back(sim, c);
  Confidence: band=high; score=0.74
- Line 134: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(*scored[i].second);
  Confidence: band=high; score=0.74
- Line 276: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: examples_.push_back({"doc_all_documents",
  Confidence: band=high; score=0.74
- Line 484: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: "SORT r.timestamp ASC\n  RETURN r",
  Confidence: band=high; score=0.74

### src/aql/aql_query_builder.cpp
Total findings: 7

- Line 407: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: AQLQueryBuilder &AQLQueryBuilder::subquery(const std::string &variable, const AQLQueryBuilder &inner) {
  Confidence: band=high; score=0.74
- Line 438: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fields.push_back(f.name);
  Confidence: band=high; score=0.74
- Line 455: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string AQLQueryBuilder::getPartialQuery() const {
  Confidence: band=high; score=0.74
- Line 502: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.issues.push_back({ValidationIssue::Severity::ERROR,
  Confidence: band=high; score=0.74
- Line 502: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.issues.push_back({ValidationIssue::Severity::ERROR,
  Confidence: band=high; score=0.74
- Line 521: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.issues.push_back({ValidationIssue::Severity::WARNING,
  Confidence: band=high; score=0.74
- Line 521: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.issues.push_back({ValidationIssue::Severity::WARNING,
  Confidence: band=high; score=0.74

### src/aql/classify_bridge.cpp
Total findings: 7

- Line 175: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (max_raw == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 111: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> scoreCategories(const std::string &query_lower,
  Confidence: band=medium; score=0.66
- Line 113: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> scores;
  Confidence: band=medium; score=0.66
- Line 133: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> softmax(const std::unordered_map<std::string, double> &raw) {
  Confidence: band=medium; score=0.66
- Line 143: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> exp_vals;
  Confidence: band=medium; score=0.66
- Line 151: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> result;
  Confidence: band=medium; score=0.66
- Line 219: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cats.push_back(c);
  Confidence: band=high; score=0.74

### src/aql/aql_query_template_library.cpp
Total findings: 6

- Line 68: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(tmpl);
  Confidence: band=high; score=0.74
- Line 68: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(tmpl);
  Confidence: band=high; score=0.74
- Line 84: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(tmpl);
  Confidence: band=high; score=0.74
- Line 90: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(tmpl);
  Confidence: band=high; score=0.74
- Line 115: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, std::string>& parameters
  Confidence: band=medium; score=0.66
- Line 129: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, std::string>& parameters
  Confidence: band=medium; score=0.66

### src/aql/aql_migration_assistant.cpp
Total findings: 5

- Line 68: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function migrate without trace point
  Context: MigrationResult AQLMigrationAssistant::migrate(const std::string &arango_aql) const {
  Confidence: band=very_high; score=0.9
- Line 156: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(b == std::string::npos ? "" : part.substr(b, e - b + 1));
  Confidence: band=high; score=0.74
- Line 258: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += '@';
  Confidence: band=high; score=0.74
- Line 267: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: issues.push_back({MigrationIssue::Severity::INFO, "@@collection bind parameters rewritten to @collection",
  Confidence: band=high; score=0.74
- Line 661: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: issues.push_back({MigrationIssue::Severity::INFO, fn + "() is an ArangoDB-specific type-check function",
  Confidence: band=high; score=0.74

### src/aql/aql_conversation_context.cpp
Total findings: 4

- Line 72: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: static std::string cleanQuery(const std::string &raw) {
  Confidence: band=high; score=0.74
- Line 159: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pairs.emplace_back(m.role, m.content);
  Confidence: band=high; score=0.74
- Line 324: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string AQLConversationContext::lastQuery() const {
  Confidence: band=high; score=0.74
- Line 333: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.emplace_back(msg.role, msg.content);
  Confidence: band=high; score=0.74

### src/aql/aql_model_router.cpp
Total findings: 4

- Line 123: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: routes_.push_back(route);
  Confidence: band=high; score=0.74
- Line 155: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scores.push_back({rule.type, hits});
  Confidence: band=high; score=0.74
- Line 155: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scores.push_back({rule.type, hits});
  Confidence: band=high; score=0.74
- Line 173: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(s.type);
  Confidence: band=high; score=0.74

### src/aql/aql_syntax_highlighter.cpp
Total findings: 4

- Line 328: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stack.push_back({c, tok.line, tok.column});
  Confidence: band=high; score=0.74
- Line 346: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back({entry.line, entry.col, std::string("Unclosed '") + entry.ch + "'"});
  Confidence: band=high; score=0.74
- Line 355: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back({tok.line, tok.column, "Unterminated string literal"});
  Confidence: band=high; score=0.74
- Line 392: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back({tok.line, tok.column, "FOR clause is missing IN keyword"});
  Confidence: band=high; score=0.74

### src/aql/docs_assistant_functions.cpp
Total findings: 4

- Line 317: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string DocsAssistantFunctions::extractTopicFromQuery(const std::string &query) {
  Confidence: band=high; score=0.74
- Line 344: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string DocsAssistantFunctions::extractSearchQuery(const std::string &query) {
  Confidence: band=high; score=0.74
- Line 387: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string DocsAssistantFunctions::docsQuery(const std::string &query) {
  Confidence: band=high; score=0.74
- Line 422: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(doc_json);
  Confidence: band=high; score=0.74

### src/aql/aql_lora_finetuner.cpp
Total findings: 3

- Line 44: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "llm/lora_framework/lora_training_service.h"
  Confidence: band=high; score=0.74
- Line 451: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: samples_.push_back(std::move(s));
  Confidence: band=high; score=0.74
- Line 479: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back({{"input", s.input}, {"output", s.output}, {"metadata", s.metadata}});
  Confidence: band=high; score=0.74

### src/aql/aql_confidence_scorer.cpp
Total findings: 1

- Line 202: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: xs.push_back(scoreStructure(lower));
  Confidence: band=high; score=0.74

### src/aql/aql_ingestion_bridge.cpp
Total findings: 1

- Line 67: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entity_array.push_back({
  Confidence: band=high; score=0.74

### src/aql/aql_optimizer_advisor.cpp
Total findings: 1

- Line 142: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: suggestions.push_back({ValidationIssue::Severity::INFO, msg, "INDEX"});
  Confidence: band=high; score=0.74

### src/aql/aql_rollback_suggester.cpp
Total findings: 1

- Line 35: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += ' ';
  Confidence: band=high; score=0.74

### src/aql/llm_aql_embedding_bridge.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
