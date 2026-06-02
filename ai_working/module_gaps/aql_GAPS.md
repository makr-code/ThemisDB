# aql Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: aql
- Generated: 2026-06-02 11:55:47
- Status: Critical Findings Present
- Total Findings: 528
- Actionable Findings (Critical + High): 251
- Affected Files: 21

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 58 |
| High | 193 |
| Medium | 277 |
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
| src/aql/llm_aql_handler.cpp | 190 | 30 | 121 | 39 | 0 |
| src/aql/aql_query_builder.cpp | 41 | 0 | 3 | 38 | 0 |
| src/aql/aql_fewshot_example_library.cpp | 33 | 5 | 4 | 24 | 0 |
| src/aql/aql_agent.cpp | 32 | 10 | 17 | 5 | 0 |
| src/aql/aql_syntax_highlighter.cpp | 27 | 0 | 1 | 26 | 0 |
| src/aql/llm_metrics_collector.cpp | 26 | 8 | 15 | 3 | 0 |
| src/aql/aql_lora_finetuner.cpp | 25 | 1 | 4 | 20 | 0 |
| src/aql/aql_autocomplete.cpp | 24 | 0 | 1 | 23 | 0 |
| src/aql/aql_query_validator.cpp | 20 | 0 | 1 | 19 | 0 |
| src/aql/aql_migration_assistant.cpp | 15 | 0 | 3 | 12 | 0 |
| src/aql/aql_rollback_suggester.cpp | 15 | 0 | 0 | 15 | 0 |
| src/aql/aql_query_diff_explainer.cpp | 14 | 0 | 2 | 12 | 0 |
| src/aql/aql_query_template_library.cpp | 14 | 0 | 7 | 7 | 0 |
| src/aql/docs_assistant_functions.cpp | 13 | 2 | 4 | 7 | 0 |
| src/aql/classify_bridge.cpp | 11 | 0 | 4 | 7 | 0 |
| src/aql/aql_confidence_scorer.cpp | 7 | 1 | 2 | 4 | 0 |
| src/aql/aql_conversation_context.cpp | 7 | 0 | 3 | 4 | 0 |
| src/aql/aql_model_router.cpp | 6 | 0 | 0 | 6 | 0 |
| src/aql/aql_ingestion_bridge.cpp | 4 | 1 | 1 | 2 | 0 |
| src/aql/aql_optimizer_advisor.cpp | 3 | 0 | 0 | 3 | 0 |
| src/aql/llm_aql_embedding_bridge.cpp | 1 | 0 | 0 | 1 | 0 |

## Full Scanner Findings

### src/aql/llm_aql_handler.cpp
Total findings: 190

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
- Line 579: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->sharding_manager_ = sharding_manager;
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
- Line 1174: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (impl_->sharding_manager_ != nullptr) {
- Line 1175: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto target_shard = impl_->sharding_manager_->GetShardForKey("llm_embeddings", text);
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
- Line 170: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(LLMErrorCode::PROMPT_TOO_LONG, field_name + " exceeds maximum allowed length of "
- Line 177: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(LLMErrorCode::PROMPT_INJECTION,
- Line 184: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const std::string lower = toLower(input);
  Confidence: band=very_high; score=0.9
- Line 235: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(LLMErrorCode::PROMPT_INJECTION,
- Line 270: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(
- Line 275: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto safety_result = config.c1_cai_eval_fn(generated_response, original_query);
  Confidence: band=very_high; score=0.9
- Line 277: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(
- Line 282: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(
- Line 287: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(
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
- Line 697: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: request.metadata["domain_hint"]      = *domain;
- Line 698: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: request.metadata["routing_decision"] = routing_decision;
- Line 708: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: && !request.system_prompt->empty()) {
- Line 721: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.max_tokens = std::stoi(options.at("max_tokens"));
- Line 724: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.temperature = std::stof(options.at("temperature"));
- Line 727: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.top_p = std::stof(options.at("top_p"));
- Line 730: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.top_k = std::stoi(options.at("top_k"));
- Line 733: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.repetition_penalty = std::stof(options.at("repetition_penalty"));
- Line 741: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!cancel_token->load(std::memory_order_acquire)) {
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
- Line 814: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(LLMErrorCode::INVALID_OPTIONS, std::string("Invalid option value: ") + e.what());
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
- Line 826: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(LLMErrorCode::INFERENCE_FAILED, std::string("Inference operation failed: ") + e.w
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
- Line 860: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.max_tokens = std::stoi(options.at("max_tokens"));
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
- Line 931: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(LLMErrorCode::INFERENCE_FAILED, std::string("Streaming inference failed: ") + e.w
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
- Line 1062: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.max_tokens = std::stoi(options.at("max_tokens"));
- Line 1065: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.temperature = std::stof(options.at("temperature"));
- Line 1068: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.top_p = std::stof(options.at("top_p"));
- Line 1076: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!cancel_token->load(std::memory_order_acquire)) {
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
- Line 1147: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(LLMErrorCode::INVALID_OPTIONS, std::string("Invalid option value: ") + e.what());
- Line 1159: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(LLMErrorCode::RAG_FAILED, std::string("RAG operation failed: ") + e.what());
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
- Line 1296: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(std::string("LLM STATS failed: ") + e.what());
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
- Line 1498: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(LLMErrorCode::INVALID_RESPONSE,
- Line 1524: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(LLMErrorCode::ACCESS_DENIED, acl_err);
- Line 1540: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(LLMErrorCode::INVALID_RESPONSE,
- Line 1548: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Sanitize inputs (same rules as translateNLToAQL)
  Confidence: band=very_high; score=0.9
- Line 1569: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Stream via executeInferStreaming; collect tokens so we can post-process.
  Confidence: band=very_high; score=0.9
- Line 1600: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(LLMErrorCode::INVALID_RESPONSE,
- Line 1624: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(LLMErrorCode::ACCESS_DENIED, acl_err);
- Line 1632: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(LLMErrorCode::INVALID_RESPONSE,
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
- Line 1838: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(LLMErrorCode::INVALID_RESPONSE,
- Line 1861: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(LLMErrorCode::ACCESS_DENIED, acl_err);
- Line 1880: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(LLMErrorCode::INVALID_RESPONSE,
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
- Line 1938: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isspace(c); }
- Line 1939: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c) { return !std::isspace(c); }).base(),
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 125: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string buildChatOriginalQuery(const std::vector<llm::ChatMessage>& messages) {
  Confidence: band=high; score=0.74
- Line 129: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: combined_user_content += "\n";
  Confidence: band=high; score=0.74
- Line 130: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: combined_user_content += "\n";
- Line 142: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: all_content += "\n";
  Confidence: band=high; score=0.74
- Line 143: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: all_content += "\n";
- Line 340: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: referenced_collections.push_back((*it)[1].str());
  Confidence: band=high; score=0.74
- Line 341: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: referenced_collections.push_back((*it)[1].str());
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
- Line 426: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: history_.push_back({nl_query, aql_result});
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
- Line 1014: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1020: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1265: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(lora.lora_id.empty() ? lora.id : lora.lora_id);
  Confidence: band=high; score=0.74
- Line 1266: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ids.push_back(lora.lora_id.empty() ? lora.id : lora.lora_id);
- Line 1285: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "  Throughput: " << stats.throughput << " req/s\n";
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
- Line 1659: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1663: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(result));
  Confidence: band=high; score=0.74
- Line 1820: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: messages.emplace_back("system", sys_prompt);
  Confidence: band=high; score=0.74
- Line 1896: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: empty_result.suggestions.push_back("Provide a non-empty AQL query");
- Line 1926: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: const std::string response = executeInfer(prompt.str());
  Confidence: band=high; score=0.74
- Line 1952: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/aql/aql_query_builder.cpp
Total findings: 41

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4222 feat(aql): AQLQueryBuilder ... (2026-03-15) | #3479 [Docs-Audit] src/aq
- Line 407: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: AQLQueryBuilder &AQLQueryBuilder::subquery(const std::string &variable, const AQLQueryBuilder &inner
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 220: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: impl_->for_clauses.push_back({variable, collection});
- Line 236: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: impl_->sorts.push_back({field, ascending});
- Line 320: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: impl_->for_traverse_clauses.push_back(
- Line 336: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: impl_->dml_clauses.push_back({DMLType::INSERT, collection, doc_expr, {}, {}, {}});
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
- Line 503: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.issues.push_back({ValidationIssue::Severity::ERROR,
- Line 521: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.issues.push_back({ValidationIssue::Severity::WARNING,
  Confidence: band=high; score=0.74
- Line 521: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.issues.push_back({ValidationIssue::Severity::WARNING,
  Confidence: band=high; score=0.74
- Line 522: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.issues.push_back({ValidationIssue::Severity::WARNING,
- Line 549: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("FOR");
- Line 550: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("INSERT");
- Line 551: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("UPSERT");
- Line 552: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("UPDATE");
- Line 553: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("REMOVE");
- Line 554: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("REPLACE");
- Line 561: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("LET");
- Line 562: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("FILTER");
- Line 563: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("WINDOW");
- Line 564: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("COLLECT");
- Line 565: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("SORT");
- Line 567: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("LIMIT");
- Line 569: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("RETURN");
- Line 571: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("INSERT");
- Line 572: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("UPDATE");
- Line 573: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("REMOVE");
- Line 574: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("REPLACE");
- Line 575: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("UPSERT");
- Line 578: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("FOR");

### src/aql/aql_fewshot_example_library.cpp
Total findings: 33

- Line 106: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const std::vector<float> query_emb = embedding_provider_->embed(nl_query);
- Line 179: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: embedding_cache_[i] = embedding_provider_->embed(examples_[i].nl_query);
- Line 179: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: embedding_cache_[i] = embedding_provider_->embed(examples_[i].nl_query);
- Line 197: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: embedding_cache_[idx] = embedding_provider_->embed(examples_[idx].nl_query);
- Line 197: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: embedding_cache_[idx] = embedding_provider_->embed(examples_[idx].nl_query);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4175 fix(aql/test): correct AC-4... (2026-03-13) | #3002 [aql] Few-shot AQL
- Line 108: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it    = index_by_id_.find(c->id);
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
- Line 90: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: candidates.push_back(&ex);
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
- Line 277: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: examples_.push_back({"doc_all_documents",
- Line 355: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "FOR v, e IN 1..1 OUTBOUND \"users/42\" friends RETURN v",
- Line 363: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "FOR v IN 1..3 OUTBOUND \"users/1\" edges RETURN v",
- Line 379: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "FOR v IN 1..2 ANY \"product/99\" related_to RETURN v",
- Line 387: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "FOR v, e IN 1..1 OUTBOUND \"users/alice\" works_with\n  FILTER e.strength > 5\n  RETURN v",
- Line 387: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "FOR v, e IN 1..1 OUTBOUND \"users/alice\" works_with\n  FILTER e.strength > 5\n  RETURN v",
- Line 395: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "FOR v, e, p IN 1..5 OUTBOUND \"users/1\" edges\n  FILTER v._id == \"users/10\"\n  RETURN p",
- Line 395: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "FOR v, e, p IN 1..5 OUTBOUND \"users/1\" edges\n  FILTER v._id == \"users/10\"\n  RETURN p",
- Line 395: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "FOR v, e, p IN 1..5 OUTBOUND \"users/1\" edges\n  FILTER v._id == \"users/10\"\n  RETURN p",
- Line 484: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: "SORT r.timestamp ASC\n  RETURN r",
  Confidence: band=high; score=0.74
- Line 490: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: examples_.push_back({"ts_between",
- Line 499: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: examples_.push_back({"ts_hourly_avg",
- Line 509: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: examples_.push_back({"ts_latest_per_device",
- Line 522: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: examples_.push_back({"agg_count_by_group",
- Line 548: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: examples_.push_back({"agg_having",
- Line 587: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: examples_.push_back({"gen_array_filter",
- Line 595: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: examples_.push_back({"gen_distinct",

### src/aql/aql_agent.cpp
Total findings: 32

- Line 93: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::string llm_input = system_prompt + "\n\n" + conversation;
  Confidence: band=very_high; score=0.99
- Line 99: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: raw_response        = handler_->executeInfer(llm_input, config_.model_alias,
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
- Line 99: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: raw_response        = handler_->executeInfer(llm_input, config_.model_alias,
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
- Line 293: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: AgentResult ReActAgent::execute(const std::string &task, const json &context) {
- Line 293: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: AgentResult ReActAgent::execute(const std::string &task, const json &context) {
  Confidence: band=very_high; score=0.9
- Line 294: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return impl_->execute(task, context);
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

### src/aql/aql_syntax_highlighter.cpp
Total findings: 27

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 72: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: constexpr const char *FG_CYAN      = "\x1b[36m"; // core keywords
- Line 73: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: constexpr const char *FG_MAGENTA   = "\x1b[35m"; // LLM keywords
- Line 74: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: constexpr const char *FG_YELLOW    = "\x1b[33m"; // built-in functions
- Line 75: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: constexpr const char *FG_GREEN     = "\x1b[32m"; // string literals
- Line 76: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: constexpr const char *FG_BLUE      = "\x1b[34m"; // numbers
- Line 77: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: constexpr const char *FG_RED       = "\x1b[31m"; // error annotation marker
- Line 78: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: constexpr const char *FG_DARK_GREY = "\x1b[90m"; // comments
- Line 119: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({AQLTokenType::UNKNOWN, ws, tl, tc});
- Line 131: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({AQLTokenType::COMMENT, comment, tl, tc});
- Line 148: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({AQLTokenType::COMMENT, comment, tl, tc});
- Line 166: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({AQLTokenType::STRING, str, tl, tc});
- Line 177: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({AQLTokenType::IDENTIFIER, ident, tl, tc});
- Line 197: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({AQLTokenType::NUMBER, num, tl, tc});
- Line 227: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({ttype, ident, tl, tc});
- Line 328: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stack.push_back({c, tok.line, tok.column});
  Confidence: band=high; score=0.74
- Line 329: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stack.push_back({c, tok.line, tok.column});
- Line 332: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back({tok.line, tok.column, std::string("Unmatched closing '") + c + "'"});
- Line 337: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back({tok.line, tok.column,
- Line 346: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back({entry.line, entry.col, std::string("Unclosed '") + entry.ch + "'"});
  Confidence: band=high; score=0.74
- Line 347: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back({entry.line, entry.col, std::string("Unclosed '") + entry.ch + "'"});
- Line 355: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back({tok.line, tok.column, "Unterminated string literal"});
  Confidence: band=high; score=0.74
- Line 356: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back({tok.line, tok.column, "Unterminated string literal"});
- Line 361: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back({tok.line, tok.column, "Unterminated string literal"});
- Line 392: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back({tok.line, tok.column, "FOR clause is missing IN keyword"});
  Confidence: band=high; score=0.74
- Line 393: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back({tok.line, tok.column, "FOR clause is missing IN keyword"});

### src/aql/llm_metrics_collector.cpp
Total findings: 26

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
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3479 [Docs-Audit] src/aql: Fix s... (2026-03-12) | #1262 Add production hard
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
- Line 109: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: {{"operation", "infer"}, {"model", model_id}, {"status", success ? "success" : "failure"}});
- Line 143: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: {{"operation", "rag"}, {"model", collection}, {"status", success ? "success" : "failure"}});
- Line 168: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: {{"operation", "embed"}, {"model", model_id}, {"status", success ? "success" : "failure"}});

### src/aql/aql_lora_finetuner.cpp
Total findings: 25

- Line 358: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: samples_.push_back(makeSample("Create a new collection called events", "CREATE COLLECTION events", C
- Line 366: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: "  FIELDS ['embedding']\n"
- Line 373: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: "  FIELDS ['body']",
- Line 513: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: throw std::invalid_argument("LoRA 'rank' must be in [1, 256], got " + it->second);
- Line 623: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto dataset = impl_->dataset_builder.build("themisdb_aql");
- Line 44: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "llm/lora_framework/lora_training_service.h"
  Confidence: band=high; score=0.74
- Line 160: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "FOR v IN 1..2 OUTBOUND 'users/alice' friends\n"
- Line 177: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "FOR v, e IN 1..2 INBOUND 'products/42' recommendations\n"
- Line 247: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples_.push_back(makeSample("Get the average CPU usage per hour for the last 24 hours",
- Line 256: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples_.push_back(makeSample("Calculate a 5-minute rolling average of temperature readings",
- Line 263: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples_.push_back(makeSample("Find timestamps where latency spiked above the 95th percentile",
- Line 299: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples_.push_back(makeSample("Generate an embedding for a text field and store it",
- Line 306: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples_.push_back(makeSample("Load a GGUF model into the LLM runtime",
- Line 308: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "  FROM '/models/llama-3-8b-instruct.gguf'\n"
- Line 313: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples_.push_back(makeSample("Unload an LLM model to free memory", "LLM MODEL UNLOAD 'llama-3-8b'",
- Line 316: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples_.push_back(makeSample("List all loaded LLM models", "LLM MODEL LIST", C::NL_TO_AQL));
- Line 326: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples_.push_back(makeSample("Load a LoRA adapter for domain-specific inference",
- Line 328: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "  FROM '/adapters/legal-v1.gguf'\n"
- Line 333: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples_.push_back(makeSample("List all currently loaded LoRA adapters", "LLM LORA LIST", C::AQL_LOR
- Line 335: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples_.push_back(makeSample("Unload a LoRA adapter", "LLM LORA UNLOAD 'legal-adapter'", C::AQL_LOR
- Line 337: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples_.push_back(makeSample("Run inference using a LoRA adapter for technical documentation",
- Line 344: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples_.push_back(makeSample("Train a LoRA adapter from a query collection",
- Line 451: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: samples_.push_back(std::move(s));
  Confidence: band=high; score=0.74
- Line 479: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back({{"input", s.input}, {"output", s.output}, {"metadata", s.metadata}});
  Confidence: band=high; score=0.74
- Line 480: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back({{"input", s.input}, {"output", s.output}, {"metadata", s.metadata}});

### src/aql/aql_autocomplete.cpp
Total findings: 24

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 292: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vars.push_back((*it)[1].str());
  Confidence: band=high; score=0.74
- Line 293: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vars.push_back((*it)[1].str());
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
- Line 356: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: info.fields.push_back((*fit)[0].str());
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
- Line 617: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(e.name);

### src/aql/aql_query_validator.cpp
Total findings: 20

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4222 feat(aql): AQLQueryBuilder ... (2026-03-15) | #3479 [Docs-Audit] src/aq
- Line 99: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 111: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vars.push_back((*it)[1].str());
  Confidence: band=high; score=0.74
- Line 112: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vars.push_back((*it)[1].str());
- Line 123: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vars.push_back((*it)[1].str());
  Confidence: band=high; score=0.74
- Line 124: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vars.push_back((*it)[1].str());
- Line 133: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.issues.push_back(
- Line 143: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.issues.push_back({ValidationIssue::Severity::WARNING,
- Line 157: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.issues.push_back({ValidationIssue::Severity::ERROR, "Query is missing a RETURN clause", "RETU
- Line 186: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.issues.push_back({ValidationIssue::Severity::WARNING,
- Line 200: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.issues.push_back({ValidationIssue::Severity::INFO,
- Line 217: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.issues.push_back({ValidationIssue::Severity::ERROR,
  Confidence: band=high; score=0.74
- Line 218: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.issues.push_back({ValidationIssue::Severity::ERROR,
- Line 318: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.issues.push_back({ValidationIssue::Severity::WARNING,
  Confidence: band=high; score=0.74
- Line 319: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.issues.push_back({ValidationIssue::Severity::WARNING,
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
- Line 399: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.issues.push_back({ValidationIssue::Severity::WARNING,

### src/aql/aql_migration_assistant.cpp
Total findings: 15

- Line 68: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function migrate without trace point
  Context: MigrationResult AQLMigrationAssistant::migrate(const std::string &arango_aql) const {
  Confidence: band=very_high; score=0.9
- Line 205: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * @param kw_pos     [out] Position of the keyword when found.
- Line 206: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * @param paren_pos  [out] Position of the '(' when found.
- Line 156: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(b == std::string::npos ? "" : part.substr(b, e - b + 1));
  Confidence: band=high; score=0.74
- Line 157: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(b == std::string::npos ? "" : part.substr(b, e - b + 1));
- Line 165: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(b == std::string::npos ? "" : part.substr(b, e - b + 1));
- Line 258: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += '@';
  Confidence: band=high; score=0.74
- Line 259: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += '@';
- Line 267: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: issues.push_back({MigrationIssue::Severity::INFO, "@@collection bind parameters rewritten to @collection",
  Confidence: band=high; score=0.74
- Line 630: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: issues.push_back({MigrationIssue::Severity::ERROR, "V8() is not supported in ThemisDB AQL",
- Line 661: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: issues.push_back({MigrationIssue::Severity::INFO, fn + "() is an ArangoDB-specific type-check function",
  Confidence: band=high; score=0.74
- Line 662: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: issues.push_back({MigrationIssue::Severity::INFO, fn + "() is an ArangoDB-specific type-check functi
- Line 692: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: issues.push_back({MigrationIssue::Severity::INFO, "HASH() is not available in ThemisDB AQL",
- Line 721: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: issues.push_back({MigrationIssue::Severity::INFO, "ATTRIBUTES() is not available in ThemisDB AQL",
- Line 749: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: issues.push_back({MigrationIssue::Severity::INFO, "TRANSLATE() is not available in ThemisDB AQL",

### src/aql/aql_rollback_suggester.cpp
Total findings: 15

- Line 35: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += ' ';
  Confidence: band=high; score=0.74
- Line 36: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += ' ';
- Line 225: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: rq << "// Rollback for: INSERT INTO " << coll << "\n"
- Line 225: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: rq << "// Rollback for: INSERT INTO " << coll << "\n"
- Line 243: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: rq << "// Rollback for: REMOVE … IN " << coll << "\n"
- Line 243: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: rq << "// Rollback for: REMOVE … IN " << coll << "\n"
- Line 260: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: rq << "// Rollback for: UPDATE … IN " << coll << "\n"
- Line 260: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: rq << "// Rollback for: UPDATE … IN " << coll << "\n"
- Line 278: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: rq << "// Rollback for: REPLACE … IN " << coll << "\n"
- Line 278: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: rq << "// Rollback for: REPLACE … IN " << coll << "\n"
- Line 297: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: rq << "// Rollback for: UPSERT … IN " << coll << "\n"
- Line 297: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: rq << "// Rollback for: UPSERT … IN " << coll << "\n"
- Line 298: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "// Remove documents that were inserted by the UPSERT (i.e. did\n"
- Line 299: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "// not exist before).\n"
- Line 303: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "// Documents that were UPDATED (existed before) must be\n"

### src/aql/aql_query_diff_explainer.cpp
Total findings: 14

- Line 77: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: size_t p = norm.find(kw, search_from);
- Line 170: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto it = begin; it != std::sregex_iterator(); ++it) {
- Line 34: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += ' ';
  Confidence: band=high; score=0.74
- Line 35: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += ' ';
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
- Line 86: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: matches.push_back({p, kw});
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
- Line 223: severity=MEDIUM; category=repeated_lookup
  Description: Repeated find() for same key: kw
  Remediation: Cache the result or use lower_bound/upper_bound for range operations
  Context: if (all_keys.find(kw) == all_keys.end()) {
- Line 266: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.diffs.push_back(std::move(e));
  Confidence: band=high; score=0.74
- Line 275: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.diffs.push_back(std::move(e));
  Confidence: band=high; score=0.74

### src/aql/aql_query_template_library.cpp
Total findings: 14

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3479 [Docs-Audit] src/aql: Fix s... (2026-03-12)
- Line 82: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (toLower(tmpl.name).find(lower_kw) != std::string::npos ||
- Line 82: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (toLower(tmpl.name).find(lower_kw) != std::string::npos ||
- Line 83: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: toLower(tmpl.description).find(lower_kw) != std::string::npos) {
- Line 83: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: toLower(tmpl.description).find(lower_kw) != std::string::npos) {
- Line 83: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: toLower(tmpl.description).find(lower_kw) != std::string::npos) {
- Line 89: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (toLower(t).find(lower_kw) != std::string::npos) {
- Line 68: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(tmpl);
  Confidence: band=high; score=0.74
- Line 68: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(tmpl);
  Confidence: band=high; score=0.74
- Line 83: severity=MEDIUM; category=repeated_lookup
  Description: Repeated find() for same key: lower_kw
  Remediation: Cache the result or use lower_bound/upper_bound for range operations
  Context: if (toLower(tmpl.name).find(lower_kw) != std::string::npos ||
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

### src/aql/docs_assistant_functions.cpp
Total findings: 13

- Line 557: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Use std::unique_ptr<T> or std::make_unique<T>()
  Context: g_docs_assistant_functions = new DocsAssistantFunctions();
- Line 557: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: g_docs_assistant_functions = new DocsAssistantFunctions();
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4220 feat(aql): wire detectInten... (2026-03-14) | #3479 [Docs-Audit] src/aq
- Line 158: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: answer     = lora->query(query, user_id);
- Line 199: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result = assistant->query(query);
- Line 390: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result     = assistant->query(query);
- Line 239: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 281: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
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
- Line 513: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/aql/classify_bridge.cpp
Total findings: 11

- Line 118: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto &spec : categorySpecs()) {
- Line 119: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (scores.find(spec.label) == scores.end()) {
- Line 123: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (query_lower.find(kw) != std::string::npos) {
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
- Line 243: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/aql/aql_confidence_scorer.cpp
Total findings: 7

- Line 179: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto isWordChar = [](char c) -> bool { return std::isalnum(static_cast<unsigned char>(c)) || c == '_
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4161 feat(aql): Runtime-configur... (2026-03-13) | #3139 feat(aql): Stream n
- Line 129: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: auto it = std::find_if(line.begin(), line.end(), [](unsigned char c) { return !std::isspace(c); });
- Line 202: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: xs.push_back(scoreStructure(lower));
  Confidence: band=high; score=0.74
- Line 203: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: xs.push_back(scoreStructure(lower));
- Line 204: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ys.push_back(scoreCompleteness(lower));
- Line 205: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: zs.push_back(scoreSchemaMatch(lower, ""));

### src/aql/aql_conversation_context.cpp
Total findings: 7

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
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
Total findings: 6

- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
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
- Line 156: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: scores.push_back({rule.type, hits});
- Line 173: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(s.type);
  Confidence: band=high; score=0.74

### src/aql/aql_ingestion_bridge.cpp
Total findings: 4

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 74: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: payload["_entities"] = std::move(entity_array);
- Line 67: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entity_array.push_back({
  Confidence: band=high; score=0.74
- Line 68: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entity_array.push_back({

### src/aql/aql_optimizer_advisor.cpp
Total findings: 3

- Line 42: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 142: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: suggestions.push_back({ValidationIssue::Severity::INFO, msg, "INDEX"});
  Confidence: band=high; score=0.74
- Line 143: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: suggestions.push_back({ValidationIssue::Severity::INFO, msg, "INDEX"});

### src/aql/llm_aql_embedding_bridge.cpp
Total findings: 1

- Line 28: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
