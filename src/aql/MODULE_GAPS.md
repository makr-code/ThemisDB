# aql Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: aql
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 650
- Actionable Findings (Critical + High): 328
- Affected Files: 21

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 74 |
| High | 254 |
| Medium | 322 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| container | 159 |
| llm_ai_safety | 154 |
| reliability | 113 |
| performance_patterns | 79 |
| platform | 33 |
| concurrency | 21 |
| determinism | 21 |
| exception_safety | 14 |
| memory | 13 |
| observability | 13 |
| uninitialized | 8 |
| audit_logging | 6 |
| performance | 6 |
| security | 6 |
| raii | 4 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/aql/llm_aql_handler.cpp | 187 | 28 | 119 | 40 | 0 |
| src/aql/aql_query_builder.cpp | 81 | 2 | 38 | 41 | 0 |
| src/aql/aql_lora_finetuner.cpp | 53 | 12 | 20 | 21 | 0 |
| src/aql/aql_autocomplete.cpp | 38 | 0 | 1 | 37 | 0 |
| src/aql/aql_fewshot_example_library.cpp | 37 | 5 | 5 | 27 | 0 |
| src/aql/aql_agent.cpp | 36 | 11 | 18 | 7 | 0 |
| src/aql/llm_metrics_collector.cpp | 29 | 10 | 16 | 3 | 0 |
| src/aql/aql_syntax_highlighter.cpp | 27 | 0 | 1 | 26 | 0 |
| src/aql/aql_query_validator.cpp | 21 | 1 | 0 | 20 | 0 |
| src/aql/aql_migration_assistant.cpp | 20 | 0 | 3 | 17 | 0 |
| src/aql/aql_query_template_library.cpp | 20 | 0 | 10 | 10 | 0 |
| src/aql/docs_assistant_functions.cpp | 17 | 2 | 7 | 8 | 0 |
| src/aql/aql_query_diff_explainer.cpp | 16 | 0 | 2 | 14 | 0 |
| src/aql/aql_rollback_suggester.cpp | 15 | 0 | 0 | 15 | 0 |
| src/aql/classify_bridge.cpp | 12 | 0 | 4 | 8 | 0 |
| src/aql/aql_conversation_context.cpp | 11 | 0 | 7 | 4 | 0 |
| src/aql/aql_model_router.cpp | 9 | 0 | 0 | 9 | 0 |
| src/aql/aql_confidence_scorer.cpp | 8 | 2 | 1 | 5 | 0 |
| src/aql/aql_ingestion_bridge.cpp | 5 | 1 | 2 | 2 | 0 |
| src/aql/llm_aql_embedding_bridge.cpp | 5 | 0 | 0 | 5 | 0 |
| src/aql/aql_optimizer_advisor.cpp | 3 | 0 | 0 | 3 | 0 |

## Full Scanner Findings

### src/aql/llm_aql_handler.cpp
Total findings: 187

- Line 124: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * @brief Reject input that contains well-known prompt injection patterns.
  Confidence: band=very_high; score=0.99
- Line 133: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * @param input       The raw user-supplied text.
  Confidence: band=very_high; score=0.99
- Line 137: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * @throws LLMException(PROMPT_TOO_LONG)  when the input exceeds @p max_length.
  Confidence: band=very_high; score=0.99
- Line 156: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const std::string lower = toLower(input);
  Confidence: band=very_high; score=0.99
- Line 499: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: impl_->sharding_manager_ = sharding_manager;
- Line 547: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Input validation
  Confidence: band=very_high; score=0.99
- Line 681: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t input_tokens  = impl_->token_estimator_->estimate(prompt);
  Confidence: band=very_high; score=0.99
- Line 684: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: metrics.recordInference(model_id.empty() ? "default" : model_id, lora_id, latency, input_tokens, output_tokens,
  Confidence: band=very_high; score=0.99
- Line 687: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: spdlog::debug("LLM INFER completed: model={}, latency={}ms, input_tokens={}, output_tokens={}", model_id,
  Confidence: band=very_high; score=0.99
- Line 688: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: latency.count(), input_tokens, output_tokens);
  Confidence: band=very_high; score=0.99
- Line 699: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t input_tokens = impl_->token_estimator_->estimate(prompt);
  Confidence: band=very_high; score=0.99
- Line 701: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: metrics.recordInference(model_id.empty() ? "default" : model_id, lora_id, latency, input_tokens, 0, false,
  Confidence: band=very_high; score=0.99
- Line 743: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Input validation (same as executeInfer)
  Confidence: band=very_high; score=0.99
- Line 789: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t input_tokens  = impl_->token_estimator_->estimate(prompt);
  Confidence: band=very_high; score=0.99
- Line 792: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: metrics.recordInference(model_id.empty() ? "default" : model_id, lora_id, latency, input_tokens, output_tokens,
  Confidence: band=very_high; score=0.99
- Line 795: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: spdlog::debug("LLM INFER STREAMING completed: model={}, latency={}ms, input_tokens={}, output_tokens={}",
  Confidence: band=very_high; score=0.99
- Line 796: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: model_id, latency.count(), input_tokens, output_tokens);
  Confidence: band=very_high; score=0.99
- Line 833: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Input validation
  Confidence: band=very_high; score=0.99
- Line 986: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t input_tokens  = impl_->token_estimator_->estimate(query);
  Confidence: band=very_high; score=0.99
- Line 989: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: metrics.recordRAG(collection, lora_id, latency, retrieved_docs, input_tokens, output_tokens, true, "");
  Confidence: band=very_high; score=0.99
- Line 1048: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (impl_->sharding_manager_ != nullptr) {
- Line 1049: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const auto target_shard = impl_->sharding_manager_->GetShardForKey("llm_embeddings", text);
- Line 1084: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: plugin_mgr.loadModel(model_id, path);
  Confidence: band=very_high; score=0.99
- Line 1093: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: plugin_mgr.unloadModel(model_id);
  Confidence: band=very_high; score=0.99
- Line 1111: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: plugin_mgr.loadModel(model_id, blob_urn);
  Confidence: band=very_high; score=0.99
- Line 1441: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::string full_prompt = sys_prompt + user_prompt_str;
  Confidence: band=very_high; score=0.99
- Line 1778: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: prompt << "The user intended:\n[USERINPUT_START]\n" << original_intent << "\n[USERINPUT_END]\n\n";
  Confidence: band=very_high; score=0.99
- Line 1781: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: prompt << "AQL query to evaluate:\n[USERINPUT_START]\n" << aql_query << "\n[USERINPUT_END]\n\n";
  Confidence: band=very_high; score=0.99
- Line 124: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * @brief Reject input that contains well-known prompt injection patterns.
  Confidence: band=very_high; score=0.9
- Line 133: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * @param input       The raw user-supplied text.
  Confidence: band=very_high; score=0.9
- Line 137: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * @throws LLMException(PROMPT_TOO_LONG)  when the input exceeds @p max_length.
  Confidence: band=very_high; score=0.9
- Line 142: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(LLMErrorCode::PROMPT_TOO_LONG, field_name + " exceeds maximum allowed length of "
- Line 149: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(LLMErrorCode::PROMPT_INJECTION,
- Line 156: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const std::string lower = toLower(input);
  Confidence: band=very_high; score=0.9
- Line 207: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(LLMErrorCode::PROMPT_INJECTION,
- Line 372: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: circuit_breakers_.emplace(std::piecewise_construct, std::forward_as_tuple("infer"),
  Confidence: band=very_high; score=0.9
- Line 373: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::forward_as_tuple(cfg.infer_circuit_breaker));
  Confidence: band=very_high; score=0.9
- Line 509: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: impl_->kv_prefix_transfer_mgr_ = std::move(mgr);
- Line 547: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Input validation
  Confidence: band=very_high; score=0.9
- Line 553: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (!impl_->getBreaker("infer").allowRequest()) {
  Confidence: band=very_high; score=0.9
- Line 554: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: metrics.recordCircuitBreakerState("infer", "open");
  Confidence: band=very_high; score=0.9
- Line 555: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: throw LLMException(LLMErrorCode::INFERENCE_FAILED,
  Confidence: band=very_high; score=0.9
- Line 560: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto result = impl_->timeout_manager_.executeInferWithCancelToken([&](auto cancel_token) {
  Confidence: band=very_high; score=0.9
- Line 565: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Build inference request with model and LoRA selection
  Confidence: band=very_high; score=0.9
- Line 566: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm::InferenceRequest request;
  Confidence: band=very_high; score=0.9
- Line 617: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: request.metadata["domain_hint"]      = *domain;
- Line 618: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: request.metadata["routing_decision"] = routing_decision;
- Line 627: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (!routed_shard_id.empty() && impl_->kv_prefix_transfer_mgr_ && request.system_prompt
- Line 628: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: && !request.system_prompt->empty()) {
- Line 635: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: impl_->kv_prefix_transfer_mgr_->transferIfBeneficial(target_info, *request.system_prompt,
- Line 641: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.max_tokens = std::stoi(options.at("max_tokens"));
- Line 644: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.temperature = std::stof(options.at("temperature"));
- Line 647: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.top_p = std::stof(options.at("top_p"));
- Line 650: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.top_k = std::stoi(options.at("top_k"));
- Line 653: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.repetition_penalty = std::stof(options.at("repetition_penalty"));
- Line 661: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!cancel_token->load(std::memory_order_acquire)) {
- Line 668: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto response = plugin_mgr.generate(request);
  Confidence: band=very_high; score=0.9
- Line 675: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: impl_->getBreaker("infer").recordSuccess();
  Confidence: band=very_high; score=0.9
- Line 681: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t input_tokens  = impl_->token_estimator_->estimate(prompt);
  Confidence: band=very_high; score=0.9
- Line 684: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: metrics.recordInference(model_id.empty() ? "default" : model_id, lora_id, latency, input_tokens, output_tokens,
  Confidence: band=very_high; score=0.9
- Line 684: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: metrics.recordInference(model_id.empty() ? "default" : model_id, lora_id, latency, input_tokens, output_tokens,
  Confidence: band=very_high; score=0.9
- Line 687: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: spdlog::debug("LLM INFER completed: model={}, latency={}ms, input_tokens={}, output_tokens={}", model_id,
  Confidence: band=very_high; score=0.9
- Line 687: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::debug("LLM INFER completed: model={}, latency={}ms, input_tokens={}, output_tokens={}", model_id,
  Confidence: band=very_high; score=0.9
- Line 688: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: latency.count(), input_tokens, output_tokens);
  Confidence: band=very_high; score=0.9
- Line 694: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: impl_->getBreaker("infer").recordFailure();
  Confidence: band=very_high; score=0.9
- Line 699: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t input_tokens = impl_->token_estimator_->estimate(prompt);
  Confidence: band=very_high; score=0.9
- Line 701: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: metrics.recordInference(model_id.empty() ? "default" : model_id, lora_id, latency, input_tokens, 0, false,
  Confidence: band=very_high; score=0.9
- Line 701: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: metrics.recordInference(model_id.empty() ? "default" : model_id, lora_id, latency, input_tokens, 0, false,
  Confidence: band=very_high; score=0.9
- Line 704: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::error("LLM INFER failed: model={}, error={}", model_id, e.what());
  Confidence: band=very_high; score=0.9
- Line 710: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: impl_->getBreaker("infer").recordFailure();
  Confidence: band=very_high; score=0.9
- Line 715: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: metrics.recordInference(model_id.empty() ? "default" : model_id, lora_id, latency,
  Confidence: band=very_high; score=0.9
- Line 719: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(LLMErrorCode::INVALID_OPTIONS, std::string("Invalid option value: ") + e.what());
- Line 722: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: impl_->getBreaker("infer").recordFailure();
  Confidence: band=very_high; score=0.9
- Line 727: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: metrics.recordInference(model_id.empty() ? "default" : model_id, lora_id, latency,
  Confidence: band=very_high; score=0.9
- Line 728: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: impl_->token_estimator_->estimate(prompt), 0, false, "INFERENCE_FAILED");
  Confidence: band=very_high; score=0.9
- Line 731: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(LLMErrorCode::INFERENCE_FAILED, std::string("Inference operation failed: ") + e.w
- Line 743: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Input validation (same as executeInfer)
  Confidence: band=very_high; score=0.9
- Line 749: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (!impl_->getBreaker("infer").allowRequest()) {
  Confidence: band=very_high; score=0.9
- Line 750: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: metrics.recordCircuitBreakerState("infer", "open");
  Confidence: band=very_high; score=0.9
- Line 751: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: throw LLMException(LLMErrorCode::INFERENCE_FAILED,
  Confidence: band=very_high; score=0.9
- Line 755: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Build the inference request
  Confidence: band=very_high; score=0.9
- Line 756: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm::InferenceRequest request;
  Confidence: band=very_high; score=0.9
- Line 765: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.max_tokens = std::stoi(options.at("max_tokens"));
- Line 776: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Tokens are delivered sequentially by the inference thread, so there is
  Confidence: band=very_high; score=0.9
- Line 782: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto response    = plugin_mgr.generate(request);
  Confidence: band=very_high; score=0.9
- Line 784: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: impl_->getBreaker("infer").recordSuccess();
  Confidence: band=very_high; score=0.9
- Line 789: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t input_tokens  = impl_->token_estimator_->estimate(prompt);
  Confidence: band=very_high; score=0.9
- Line 792: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: metrics.recordInference(model_id.empty() ? "default" : model_id, lora_id, latency, input_tokens, output_tokens,
  Confidence: band=very_high; score=0.9
- Line 792: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: metrics.recordInference(model_id.empty() ? "default" : model_id, lora_id, latency, input_tokens, output_tokens,
  Confidence: band=very_high; score=0.9
- Line 795: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: spdlog::debug("LLM INFER STREAMING completed: model={}, latency={}ms, input_tokens={}, output_tokens={}",
  Confidence: band=very_high; score=0.9
- Line 795: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::debug("LLM INFER STREAMING completed: model={}, latency={}ms, input_tokens={}, output_tokens={}",
  Confidence: band=very_high; score=0.9
- Line 796: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: model_id, latency.count(), input_tokens, output_tokens);
  Confidence: band=very_high; score=0.9
- Line 801: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: impl_->getBreaker("infer").recordFailure();
  Confidence: band=very_high; score=0.9
- Line 806: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: metrics.recordInference(model_id.empty() ? "default" : model_id, lora_id, latency,
  Confidence: band=very_high; score=0.9
- Line 810: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: spdlog::error("LLM INFER STREAMING failed: model={}, error={}", model_id, e.what());
  Confidence: band=very_high; score=0.9
- Line 813: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: impl_->getBreaker("infer").recordFailure();
  Confidence: band=very_high; score=0.9
- Line 818: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: metrics.recordInference(model_id.empty() ? "default" : model_id, lora_id, latency,
  Confidence: band=very_high; score=0.9
- Line 821: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(LLMErrorCode::INFERENCE_FAILED, std::string("Streaming inference failed: ") + e.w
- Line 833: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Input validation
  Confidence: band=very_high; score=0.9
- Line 941: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Build inference request with RAG context
  Confidence: band=very_high; score=0.9
- Line 942: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm::InferenceRequest request;
  Confidence: band=very_high; score=0.9
- Line 952: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.max_tokens = std::stoi(options.at("max_tokens"));
- Line 955: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.temperature = std::stof(options.at("temperature"));
- Line 958: severity=HIGH; category=no_retry_logic
  Description: http_call without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: request.top_p = std::stof(options.at("top_p"));
- Line 966: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (!cancel_token->load(std::memory_order_acquire)) {
- Line 973: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto response = plugin_mgr.generateRAG(context, request);
  Confidence: band=very_high; score=0.9
- Line 986: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t input_tokens  = impl_->token_estimator_->estimate(query);
  Confidence: band=very_high; score=0.9
- Line 989: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: metrics.recordRAG(collection, lora_id, latency, retrieved_docs, input_tokens, output_tokens, true, "");
  Confidence: band=very_high; score=0.9
- Line 1021: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(LLMErrorCode::INVALID_OPTIONS, std::string("Invalid option value: ") + e.what());
- Line 1033: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(LLMErrorCode::RAG_FAILED, std::string("RAG operation failed: ") + e.what());
- Line 1044: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: throw LLMException(LLMErrorCode::INFERENCE_FAILED,
  Confidence: band=very_high; score=0.9
- Line 1048: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (impl_->sharding_manager_ != nullptr) {
- Line 1058: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: llm::InferenceRequest request;
  Confidence: band=very_high; score=0.9
- Line 1074: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(std::string("LLM EMBED failed: ") + e.what());
- Line 1077: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(std::string("LLM EMBED failed: ") + e.what());
- Line 1163: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: oss << "    infer:    " << cb_states.infer << "\n";
  Confidence: band=very_high; score=0.9
- Line 1170: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(std::string("LLM STATS failed: ") + e.what());
- Line 1176: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: states.infer    = sharding::CircuitBreaker::stateToString(impl_->getBreaker("infer").getState());
  Confidence: band=very_high; score=0.9
- Line 1214: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: std::vector<std::string> LLMAQLHandler::executeBatchInfer(const std::vector<BatchInferRequest> &requests) {
  Confidence: band=very_high; score=0.9
- Line 1236: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: shard_results.emplace_back(idx, executeInfer(req.prompt, req.model_id, req.lora_id, req.options));
  Confidence: band=very_high; score=0.9
- Line 1250: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: throw std::runtime_error(std::string("Batch LLM INFER failed: ") + e.what());
  Confidence: band=very_high; score=0.9
- Line 1250: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(std::string("Batch LLM INFER failed: ") + e.what());
- Line 1326: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Sanitize inputs before embedding them in the LLM prompt.
  Confidence: band=very_high; score=0.9
- Line 1372: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(LLMErrorCode::INVALID_RESPONSE,
- Line 1398: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(LLMErrorCode::ACCESS_DENIED, acl_err);
- Line 1409: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(std::string("NL to AQL translation failed: ") + e.what());
- Line 1414: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(LLMErrorCode::INVALID_RESPONSE,
- Line 1422: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Sanitize inputs (same rules as translateNLToAQL)
  Confidence: band=very_high; score=0.9
- Line 1443: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Stream via executeInferStreaming; collect tokens so we can post-process.
  Confidence: band=very_high; score=0.9
- Line 1474: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(LLMErrorCode::INVALID_RESPONSE,
- Line 1498: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(LLMErrorCode::ACCESS_DENIED, acl_err);
- Line 1506: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(LLMErrorCode::INVALID_RESPONSE,
- Line 1514: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(std::string("NL to AQL streaming translation failed: ") + e.what());
- Line 1589: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(std::string("LLM CHAT failed: ") + e.what());
- Line 1596: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Sanitize inputs before embedding them in the LLM prompt
  Confidence: band=very_high; score=0.9
- Line 1615: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Sanitize inputs before embedding them in the LLM prompt
  Confidence: band=very_high; score=0.9
- Line 1654: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Sanitize inputs (same rules as translateNLToAQL)
  Confidence: band=very_high; score=0.9
- Line 1700: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(LLMErrorCode::INVALID_RESPONSE,
- Line 1723: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(LLMErrorCode::ACCESS_DENIED, acl_err);
- Line 1737: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(std::string("NL to AQL translation with examples failed: ") + e.what());
- Line 1742: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw LLMException(LLMErrorCode::INVALID_RESPONSE,
- Line 1763: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // LLM-4: sanitize user-supplied inputs before embedding in prompt
  Confidence: band=very_high; score=0.9
- Line 1778: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: prompt << "The user intended:\n[USERINPUT_START]\n" << original_intent << "\n[USERINPUT_END]\n\n";
  Confidence: band=very_high; score=0.9
- Line 1781: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: prompt << "AQL query to evaluate:\n[USERINPUT_START]\n" << aql_query << "\n[USERINPUT_END]\n\n";
  Confidence: band=very_high; score=0.9
- Line 1788: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: const std::string response = executeInfer(prompt.str());
  Confidence: band=very_high; score=0.9
- Line 1800: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isspace(c); }
- Line 1801: severity=HIGH; category=repeated_search
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
- Line 261: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: referenced_collections.push_back((*it)[1].str());
  Confidence: band=high; score=0.74
- Line 262: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: referenced_collections.push_back((*it)[1].str());
- Line 287: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: static std::unordered_set<std::string> extractReferencedCollectionsForAccessCheck(
  Confidence: band=medium; score=0.66
- Line 290: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> collections;
  Confidence: band=medium; score=0.66
- Line 346: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: history_.push_back({nl_query, aql_result});
  Confidence: band=high; score=0.74
- Line 347: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: history_.push_back({nl_query, aql_result});
- Line 540: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::string LLMAQLHandler::executeInfer(const std::string &prompt, const std::string &model_id,
  Confidence: band=high; score=0.74
- Line 668: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto response = plugin_mgr.generate(request);
  Confidence: band=high; score=0.74
- Line 782: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto response    = plugin_mgr.generate(request);
  Confidence: band=high; score=0.74
- Line 862: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto query_embedding = THEMIS_LLM_EMBED(query);
  Confidence: band=high; score=0.74
- Line 904: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 910: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1037: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::vector<float> LLMAQLHandler::executeEmbed(const std::string &text, const std::string &model_id) {
  Confidence: band=high; score=0.74
- Line 1067: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: auto embedding = THEMIS_LLM_EMBED(text);
  Confidence: band=high; score=0.74
- Line 1139: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(lora.lora_id.empty() ? lora.id : lora.lora_id);
  Confidence: band=high; score=0.74
- Line 1140: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ids.push_back(lora.lora_id.empty() ? lora.id : lora.lora_id);
- Line 1159: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: oss << "  Throughput: " << stats.throughput << " req/s\n";
- Line 1214: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::vector<std::string> LLMAQLHandler::executeBatchInfer(const std::vector<BatchInferRequest> &requests) {
  Confidence: band=high; score=0.74
- Line 1221: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<size_t>> indices_by_domain;
  Confidence: band=medium; score=0.66
- Line 1223: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: indices_by_domain[batchDomainKey(requests[i])].push_back(i);
- Line 1231: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: futures.push_back(std::async(std::launch::async, [this, &requests, indices]() {
- Line 1235: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: shard_results.emplace_back(idx, executeInfer(req.prompt, req.model_id, req.lora_id, req.options));
  Confidence: band=high; score=0.74
- Line 1236: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: shard_results.emplace_back(idx, executeInfer(req.prompt, req.model_id, req.lora_id, req.options));
  Confidence: band=high; score=0.74
- Line 1262: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "You are an expert in AQL (Application Query Language) for ThemisDB.\n";
  Confidence: band=high; score=0.74
- Line 1262: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "You are an expert in AQL (Application Query Language) for ThemisDB.\n";
  Confidence: band=high; score=0.74
- Line 1262: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += "You are an expert in AQL (Application Query Language) for ThemisDB.\n";
  Confidence: band=high; score=0.74
- Line 1352: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: messages.emplace_back("system", sys_prompt);
  Confidence: band=high; score=0.74
- Line 1449: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: messages.emplace_back("system", sys_prompt);
  Confidence: band=high; score=0.74
- Line 1533: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1537: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(std::move(result));
  Confidence: band=high; score=0.74
- Line 1538: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(std::move(result));
- Line 1682: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: messages.emplace_back("system", sys_prompt);
  Confidence: band=high; score=0.74
- Line 1758: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: empty_result.suggestions.push_back("Provide a non-empty AQL query");
- Line 1788: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: const std::string response = executeInfer(prompt.str());
  Confidence: band=high; score=0.74
- Line 1814: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/aql/aql_query_builder.cpp
Total findings: 81

- Line 624: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator start may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto start = line.find_first_not_of(" \t\r\n");
- Line 625: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator end may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto end   = line.find_last_not_of(" \t\r\n");
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 116: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::logic_error("AQLQueryBuilder: query requires at least one FOR clause");
- Line 119: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::logic_error("AQLQueryBuilder: query requires a RETURN clause or a DML clause");
- Line 214: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLQueryBuilder::forIn: variable must not be empty");
- Line 217: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLQueryBuilder::forIn: collection must not be empty");
- Line 225: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLQueryBuilder::filter: condition must not be empty");
- Line 233: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLQueryBuilder::sort: field must not be empty");
- Line 241: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLQueryBuilder::limit: count must be non-negative");
- Line 244: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLQueryBuilder::limit: offset must be non-negative");
- Line 253: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLQueryBuilder::ret: expression must not be empty");
- Line 261: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLQueryBuilder::let: variable must not be empty");
- Line 264: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLQueryBuilder::let: expression must not be empty");
- Line 272: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLQueryBuilder::collect: variable must not be empty");
- Line 275: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLQueryBuilder::collect: expression must not be empty");
- Line 295: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLQueryBuilder::forTraverse: vertex_var must not be empty");
- Line 298: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLQueryBuilder::forTraverse: edge_var must not be empty");
- Line 301: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLQueryBuilder::forTraverse: path_var must not be empty");
- Line 304: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLQueryBuilder::forTraverse: start must not be empty");
- Line 307: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLQueryBuilder::forTraverse: graph must not be empty");
- Line 310: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLQueryBuilder::forTraverse: min_depth must be non-negative");
- Line 313: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLQueryBuilder::forTraverse: max_depth must be non-negative");
- Line 316: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLQueryBuilder::forTraverse: min_depth (" + std::to_string(min_depth)
- Line 330: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLQueryBuilder::insertInto: collection must not be empty");
- Line 333: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLQueryBuilder::insertInto: doc_expr must not be empty");
- Line 341: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLQueryBuilder::updateIn: collection must not be empty");
- Line 344: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLQueryBuilder::updateIn: doc_expr must not be empty");
- Line 352: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLQueryBuilder::removeIn: collection must not be empty");
- Line 355: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLQueryBuilder::removeIn: doc_expr must not be empty");
- Line 364: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLQueryBuilder::upsertIn: collection must not be empty");
- Line 367: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLQueryBuilder::upsertIn: filter_expr must not be empty");
- Line 370: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLQueryBuilder::upsertIn: insert_expr must not be empty");
- Line 373: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLQueryBuilder::upsertIn: update_expr must not be empty");
- Line 381: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLQueryBuilder::replaceIn: collection must not be empty");
- Line 384: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLQueryBuilder::replaceIn: doc_expr must not be empty");
- Line 396: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLQueryBuilder::window: window_spec must not be empty");
- Line 406: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: AQLQueryBuilder &AQLQueryBuilder::subquery(const std::string &variable, const AQLQueryBuilder &inner
- Line 408: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLQueryBuilder::subquery: variable must not be empty");
- Line 412: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLQueryBuilder::subquery: inner builder has no clauses");
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
- Line 219: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: impl_->for_clauses.push_back({variable, collection});
- Line 227: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: impl_->filters.push_back(condition);
- Line 235: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: impl_->sorts.push_back({field, ascending});
- Line 319: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: impl_->for_traverse_clauses.push_back(
- Line 335: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: impl_->dml_clauses.push_back({DMLType::INSERT, collection, doc_expr, {}, {}, {}});
- Line 406: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: AQLQueryBuilder &AQLQueryBuilder::subquery(const std::string &variable, const AQLQueryBuilder &inner) {
  Confidence: band=high; score=0.74
- Line 437: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: fields.push_back(f.name);
  Confidence: band=high; score=0.74
- Line 438: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: fields.push_back(f.name);
- Line 454: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string AQLQueryBuilder::getPartialQuery() const {
  Confidence: band=high; score=0.74
- Line 501: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.issues.push_back({ValidationIssue::Severity::ERROR,
  Confidence: band=high; score=0.74
- Line 501: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.issues.push_back({ValidationIssue::Severity::ERROR,
  Confidence: band=high; score=0.74
- Line 502: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.issues.push_back({ValidationIssue::Severity::ERROR,
- Line 520: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.issues.push_back({ValidationIssue::Severity::WARNING,
  Confidence: band=high; score=0.74
- Line 520: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.issues.push_back({ValidationIssue::Severity::WARNING,
  Confidence: band=high; score=0.74
- Line 521: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.issues.push_back({ValidationIssue::Severity::WARNING,
- Line 548: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("FOR");
- Line 549: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("INSERT");
- Line 550: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("UPSERT");
- Line 551: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("UPDATE");
- Line 552: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("REMOVE");
- Line 553: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("REPLACE");
- Line 560: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("LET");
- Line 561: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("FILTER");
- Line 562: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("WINDOW");
- Line 563: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("COLLECT");
- Line 564: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("SORT");
- Line 566: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("LIMIT");
- Line 568: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("RETURN");
- Line 570: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("INSERT");
- Line 571: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("UPDATE");
- Line 572: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("REMOVE");
- Line 573: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("REPLACE");
- Line 574: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("UPSERT");
- Line 577: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: steps.push_back("FOR");
- Line 631: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: suggestions.push_back(line);

### src/aql/aql_lora_finetuner.cpp
Total findings: 53

- Line 357: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: samples_.push_back(makeSample("Create a new collection called events", "CREATE COLLECTION events", C
- Line 622: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto dataset = impl_->dataset_builder.build("themisdb_aql");
- Line 645: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto result = impl_->training_service->trainOnTheFly(
- Line 668: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: meta.training_config.epochs         = impl_->config.hyperparameters.num_epochs;
- Line 669: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: meta.training_config.learning_rate  = static_cast<double>(impl_->config.hyperparameters.learning_rat
- Line 669: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: meta.training_config.learning_rate  = static_cast<double>(impl_->config.hyperparameters.learning_rat
- Line 670: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: meta.training_config.lora_rank      = impl_->config.hyperparameters.rank;
- Line 671: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: meta.training_config.lora_alpha     = static_cast<double>(impl_->config.hyperparameters.alpha);
- Line 671: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: meta.training_config.lora_alpha     = static_cast<double>(impl_->config.hyperparameters.alpha);
- Line 672: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: meta.training_config.lora_dropout   = static_cast<double>(impl_->config.hyperparameters.dropout);
- Line 672: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: meta.training_config.lora_dropout   = static_cast<double>(impl_->config.hyperparameters.dropout);
- Line 673: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: meta.training_config.target_modules = impl_->config.hyperparameters.target_modules;
- Line 91: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: s.metadata["category"] = static_cast<int>(cat);
- Line 365: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: "  FIELDS ['embedding']\n"
- Line 372: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: "  FIELDS ['body']",
- Line 421: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("AQLDatasetBuilder: cannot open dataset file: " + json_path);
- Line 461: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: dataset.metadata["created_at"]   = isoTimestamp();
- Line 512: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("LoRA 'rank' must be in [1, 256], got " + it->second);
- Line 512: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: throw std::invalid_argument("LoRA 'rank' must be in [1, 256], got " + it->second);
- Line 521: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("LoRA 'alpha' must be > 0, got " + it->second);
- Line 530: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("LoRA 'dropout' must be in [0.0, 1.0), got " + it->second);
- Line 539: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("LoRA 'learning_rate' must be > 0, got " + it->second);
- Line 548: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("LoRA 'batch_size' must be > 0, got " + it->second);
- Line 557: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("LoRA 'epochs' must be > 0, got " + it->second);
- Line 566: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("LoRA 'max_seq_length' must be > 0, got " + it->second);
- Line 622: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: auto dataset = impl_->dataset_builder.build("themisdb_aql");
- Line 625: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("AQLLoRAFinetuner: insufficient training samples (" + std::to_string(datase
- Line 646: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: impl_->config.adapter_id, dataset, std::optional<LoRAHyperparameters>{impl_->config.hyperparameters}
- Line 692: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: impl_->dataset_builder.addCustomSample(nl_input, aql_output, category);
- Line 697: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: impl_->dataset_builder.loadFromJson(json_path);
- Line 720: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return impl_->dataset_builder.build("themisdb_aql");
- Line 725: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return impl_->dataset_builder.toJson();
- Line 43: severity=MEDIUM; category=observability; pattern=missing_correlation_id
  Description: Distributed call without correlation ID
  Context: #include "llm/lora_framework/lora_training_service.h"
  Confidence: band=high; score=0.74
- Line 159: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "FOR v IN 1..2 OUTBOUND 'users/alice' friends\n"
- Line 176: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "FOR v, e IN 1..2 INBOUND 'products/42' recommendations\n"
- Line 246: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples_.push_back(makeSample("Get the average CPU usage per hour for the last 24 hours",
- Line 255: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples_.push_back(makeSample("Calculate a 5-minute rolling average of temperature readings",
- Line 262: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples_.push_back(makeSample("Find timestamps where latency spiked above the 95th percentile",
- Line 298: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples_.push_back(makeSample("Generate an embedding for a text field and store it",
- Line 305: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples_.push_back(makeSample("Load a GGUF model into the LLM runtime",
- Line 307: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "  FROM '/models/llama-3-8b-instruct.gguf'\n"
- Line 312: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples_.push_back(makeSample("Unload an LLM model to free memory", "LLM MODEL UNLOAD 'llama-3-8b'",
- Line 315: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples_.push_back(makeSample("List all loaded LLM models", "LLM MODEL LIST", C::NL_TO_AQL));
- Line 325: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples_.push_back(makeSample("Load a LoRA adapter for domain-specific inference",
- Line 327: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "  FROM '/adapters/legal-v1.gguf'\n"
- Line 332: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples_.push_back(makeSample("List all currently loaded LoRA adapters", "LLM LORA LIST", C::AQL_LOR
- Line 334: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples_.push_back(makeSample("Unload a LoRA adapter", "LLM LORA UNLOAD 'legal-adapter'", C::AQL_LOR
- Line 336: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples_.push_back(makeSample("Run inference using a LoRA adapter for technical documentation",
- Line 343: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples_.push_back(makeSample("Train a LoRA adapter from a query collection",
- Line 450: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: samples_.push_back(std::move(s));
  Confidence: band=high; score=0.74
- Line 451: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples_.push_back(std::move(s));
- Line 478: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: arr.push_back({{"input", s.input}, {"output", s.output}, {"metadata", s.metadata}});
  Confidence: band=high; score=0.74
- Line 479: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: arr.push_back({{"input", s.input}, {"output", s.output}, {"metadata", s.metadata}});

### src/aql/aql_autocomplete.cpp
Total findings: 38

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 291: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vars.push_back((*it)[1].str());
  Confidence: band=high; score=0.74
- Line 292: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vars.push_back((*it)[1].str());
- Line 303: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 306: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: unique_vars.push_back(v);
  Confidence: band=high; score=0.74
- Line 307: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: unique_vars.push_back(v);
- Line 354: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: info.fields.push_back((*fit)[0].str());
  Confidence: band=high; score=0.74
- Line 354: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: info.fields.push_back((*fit)[0].str());
  Confidence: band=high; score=0.74
- Line 355: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: info.fields.push_back((*fit)[0].str());
- Line 357: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(info));
- Line 370: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(std::move(info));
  Confidence: band=high; score=0.74
- Line 371: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(info));
- Line 393: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(std::move(item));
  Confidence: band=high; score=0.74
- Line 394: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: items.push_back(std::move(item));
- Line 404: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(std::move(item));
  Confidence: band=high; score=0.74
- Line 405: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: items.push_back(std::move(item));
- Line 417: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: items.push_back(std::move(item));
- Line 438: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(std::move(item));
  Confidence: band=high; score=0.74
- Line 439: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: items.push_back(std::move(item));
- Line 458: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(std::move(item));
  Confidence: band=high; score=0.74
- Line 459: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: items.push_back(std::move(item));
- Line 500: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(std::move(item));
  Confidence: band=high; score=0.74
- Line 500: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(std::move(item));
  Confidence: band=high; score=0.74
- Line 501: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: items.push_back(std::move(item));
- Line 509: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> seen;
  Confidence: band=medium; score=0.66
- Line 518: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(std::move(item));
  Confidence: band=high; score=0.74
- Line 518: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: items.push_back(std::move(item));
  Confidence: band=high; score=0.74
- Line 519: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: items.push_back(std::move(item));
- Line 540: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: filtered.push_back(std::move(item));
  Confidence: band=high; score=0.74
- Line 541: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: filtered.push_back(std::move(item));
- Line 601: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(e.keyword);
  Confidence: band=high; score=0.74
- Line 602: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(e.keyword);
- Line 604: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(e.keyword);
  Confidence: band=high; score=0.74
- Line 605: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(e.keyword);
- Line 607: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(e.keyword);
  Confidence: band=high; score=0.74
- Line 608: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(e.keyword);
- Line 615: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(e.name);
  Confidence: band=high; score=0.74
- Line 616: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(e.name);

### src/aql/aql_fewshot_example_library.cpp
Total findings: 37

- Line 105: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const std::vector<float> query_emb = embedding_provider_->embed(nl_query);
- Line 178: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: embedding_cache_[i] = embedding_provider_->embed(examples_[i].nl_query);
- Line 178: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: embedding_cache_[i] = embedding_provider_->embed(examples_[i].nl_query);
- Line 196: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: embedding_cache_[idx] = embedding_provider_->embed(examples_[idx].nl_query);
- Line 196: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: embedding_cache_[idx] = embedding_provider_->embed(examples_[idx].nl_query);
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 34: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLFewShotExample id must not be empty");
- Line 37: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLFewShotExample id already registered: " + example.id);
- Line 107: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: auto it    = index_by_id_.find(c->id);
- Line 50: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(ex);
  Confidence: band=high; score=0.74
- Line 51: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(ex);
- Line 66: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(ex);
  Confidence: band=high; score=0.74
- Line 66: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(ex);
  Confidence: band=high; score=0.74
- Line 67: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(ex);
- Line 89: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: candidates.push_back(&ex);
- Line 116: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scored.emplace_back(sim, c);
  Confidence: band=high; score=0.74
- Line 133: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(*scored[i].second);
  Confidence: band=high; score=0.74
- Line 134: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(*scored[i].second);
- Line 275: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: examples_.push_back({"doc_all_documents",
  Confidence: band=high; score=0.74
- Line 276: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: examples_.push_back({"doc_all_documents",
- Line 354: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "FOR v, e IN 1..1 OUTBOUND \"users/42\" friends RETURN v",
- Line 362: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "FOR v IN 1..3 OUTBOUND \"users/1\" edges RETURN v",
- Line 378: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "FOR v IN 1..2 ANY \"product/99\" related_to RETURN v",
- Line 386: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "FOR v, e IN 1..1 OUTBOUND \"users/alice\" works_with\n  FILTER e.strength > 5\n  RETURN v",
- Line 386: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "FOR v, e IN 1..1 OUTBOUND \"users/alice\" works_with\n  FILTER e.strength > 5\n  RETURN v",
- Line 394: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "FOR v, e, p IN 1..5 OUTBOUND \"users/1\" edges\n  FILTER v._id == \"users/10\"\n  RETURN p",
- Line 394: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "FOR v, e, p IN 1..5 OUTBOUND \"users/1\" edges\n  FILTER v._id == \"users/10\"\n  RETURN p",
- Line 394: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: "FOR v, e, p IN 1..5 OUTBOUND \"users/1\" edges\n  FILTER v._id == \"users/10\"\n  RETURN p",
- Line 483: severity=MEDIUM; category=determinism; pattern=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Context: "SORT r.timestamp ASC\n  RETURN r",
  Confidence: band=high; score=0.74
- Line 489: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: examples_.push_back({"ts_between",
- Line 498: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: examples_.push_back({"ts_hourly_avg",
- Line 508: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: examples_.push_back({"ts_latest_per_device",
- Line 521: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: examples_.push_back({"agg_count_by_group",
- Line 547: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: examples_.push_back({"agg_having",
- Line 586: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: examples_.push_back({"gen_array_filter",
- Line 594: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: examples_.push_back({"gen_distinct",

### src/aql/aql_agent.cpp
Total findings: 36

- Line 46: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = tools_.find(name);
- Line 92: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::string llm_input = system_prompt + "\n\n" + conversation;
  Confidence: band=very_high; score=0.99
- Line 98: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: raw_response        = handler_->executeInfer(llm_input, config_.model_alias,
- Line 98: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: raw_response        = handler_->executeInfer(llm_input, config_.model_alias,
  Confidence: band=very_high; score=0.99
- Line 132: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: json tool_output            = invokeTool(*step.tool_name, step.tool_input.value_or(json::object()));
  Confidence: band=very_high; score=0.99
- Line 170: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "Action Input: <JSON arguments>\n\n"
  Confidence: band=very_high; score=0.99
- Line 238: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::string action_input = extract_field("Action Input:");
  Confidence: band=very_high; score=0.99
- Line 242: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (!action_input.empty()) {
  Confidence: band=very_high; score=0.99
- Line 244: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: step.tool_input = json::parse(action_input);
  Confidence: band=very_high; score=0.99
- Line 246: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // If the input is not valid JSON, wrap it as a string argument.
  Confidence: band=very_high; score=0.99
- Line 247: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: step.tool_input = json{{"input", action_input}};
  Confidence: band=very_high; score=0.99
- Line 77: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: AgentResult execute(const std::string &task, const json &context) {
  Confidence: band=very_high; score=0.9
- Line 92: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::string llm_input = system_prompt + "\n\n" + conversation;
  Confidence: band=very_high; score=0.9
- Line 98: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: raw_response        = handler_->executeInfer(llm_input, config_.model_alias,
  Confidence: band=very_high; score=0.9
- Line 98: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: raw_response        = handler_->executeInfer(llm_input, config_.model_alias,
  Confidence: band=very_high; score=0.9
- Line 98: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: raw_response        = handler_->executeInfer(llm_input, config_.model_alias,
- Line 101: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // LLM inference unavailable (e.g. no model loaded).
  Confidence: band=very_high; score=0.9
- Line 105: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: error_step.thought = std::string("LLM inference unavailable: ") + e.what();
  Confidence: band=very_high; score=0.9
- Line 132: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: json tool_output            = invokeTool(*step.tool_name, step.tool_input.value_or(json::object()));
  Confidence: band=very_high; score=0.9
- Line 170: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "Action Input: <JSON arguments>\n\n"
  Confidence: band=very_high; score=0.9
- Line 214: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: auto extract_field = [&](const std::string &field_prefix) -> std::string {
- Line 238: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::string action_input = extract_field("Action Input:");
  Confidence: band=very_high; score=0.9
- Line 242: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (!action_input.empty()) {
  Confidence: band=very_high; score=0.9
- Line 244: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: step.tool_input = json::parse(action_input);
  Confidence: band=very_high; score=0.9
- Line 246: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // If the input is not valid JSON, wrap it as a string argument.
  Confidence: band=very_high; score=0.9
- Line 247: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: step.tool_input = json{{"input", action_input}};
  Confidence: band=very_high; score=0.9
- Line 292: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: AgentResult ReActAgent::execute(const std::string &task, const json &context) {
- Line 292: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function execute without trace point
  Context: AgentResult ReActAgent::execute(const std::string &task, const json &context) {
  Confidence: band=very_high; score=0.9
- Line 293: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: return impl_->execute(task, context);
- Line 56: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(kv.second);
  Confidence: band=high; score=0.74
- Line 57: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(kv.second);
- Line 77: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: AgentResult execute(const std::string &task, const json &context) {
  Confidence: band=high; score=0.74
- Line 98: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: raw_response        = handler_->executeInfer(llm_input, config_.model_alias,
  Confidence: band=high; score=0.74
- Line 105: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.reasoning_trace.push_back(error_step);
  Confidence: band=high; score=0.74
- Line 106: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.reasoning_trace.push_back(error_step);
- Line 292: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: AgentResult ReActAgent::execute(const std::string &task, const json &context) {
  Confidence: band=high; score=0.74

### src/aql/llm_metrics_collector.cpp
Total findings: 29

- Line 43: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: {"operation", "model", "direction"} // direction: input/output
  Confidence: band=very_high; score=0.99
- Line 88: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::chrono::milliseconds latency, size_t input_tokens, size_t output_tokens,
  Confidence: band=very_high; score=0.99
- Line 98: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: {{"operation", "infer"}, {"model", model_id}, {"direction", "input"}},
  Confidence: band=very_high; score=0.99
- Line 99: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: static_cast<double>(input_tokens));
  Confidence: band=very_high; score=0.99
- Line 118: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::chrono::milliseconds latency, size_t retrieved_docs, size_t input_tokens,
  Confidence: band=very_high; score=0.99
- Line 132: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: {{"operation", "rag"}, {"model", collection}, {"direction", "input"}},
  Confidence: band=very_high; score=0.99
- Line 133: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: static_cast<double>(input_tokens));
  Confidence: band=very_high; score=0.99
- Line 152: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t input_tokens, bool success, const std::string &error_code) {
  Confidence: band=very_high; score=0.99
- Line 161: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: {{"operation", "embed"}, {"model", model_id}, {"direction", "input"}},
  Confidence: band=very_high; score=0.99
- Line 162: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: static_cast<double>(input_tokens));
  Confidence: band=very_high; score=0.99
- Line 43: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: {"operation", "model", "direction"} // direction: input/output
  Confidence: band=very_high; score=0.9
- Line 87: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: void LLMMetricsCollector::recordInference(const std::string &model_id, const std::string &lora_id,
  Confidence: band=very_high; score=0.9
- Line 88: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::chrono::milliseconds latency, size_t input_tokens, size_t output_tokens,
  Confidence: band=very_high; score=0.9
- Line 94: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: {{"operation", "infer"}, {"model", model_id}, {"lora", lora_id}});
  Confidence: band=very_high; score=0.9
- Line 98: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: {{"operation", "infer"}, {"model", model_id}, {"direction", "input"}},
  Confidence: band=very_high; score=0.9
- Line 98: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: {{"operation", "infer"}, {"model", model_id}, {"direction", "input"}},
  Confidence: band=very_high; score=0.9
- Line 99: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: static_cast<double>(input_tokens));
  Confidence: band=very_high; score=0.9
- Line 102: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: {{"operation", "infer"}, {"model", model_id}, {"direction", "output"}},
  Confidence: band=very_high; score=0.9
- Line 108: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: {{"operation", "infer"}, {"model", model_id}, {"status", success ? "success" : "failure"}});
  Confidence: band=very_high; score=0.9
- Line 113: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: {{"operation", "infer"}, {"model", model_id}, {"error_code", error_code}});
  Confidence: band=very_high; score=0.9
- Line 118: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::chrono::milliseconds latency, size_t retrieved_docs, size_t input_tokens,
  Confidence: band=very_high; score=0.9
- Line 132: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: {{"operation", "rag"}, {"model", collection}, {"direction", "input"}},
  Confidence: band=very_high; score=0.9
- Line 133: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: static_cast<double>(input_tokens));
  Confidence: band=very_high; score=0.9
- Line 152: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t input_tokens, bool success, const std::string &error_code) {
  Confidence: band=very_high; score=0.9
- Line 161: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: {{"operation", "embed"}, {"model", model_id}, {"direction", "input"}},
  Confidence: band=very_high; score=0.9
- Line 162: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: static_cast<double>(input_tokens));
  Confidence: band=very_high; score=0.9
- Line 108: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: {{"operation", "infer"}, {"model", model_id}, {"status", success ? "success" : "failure"}});
- Line 142: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: {{"operation", "rag"}, {"model", collection}, {"status", success ? "success" : "failure"}});
- Line 167: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: {{"operation", "embed"}, {"model", model_id}, {"status", success ? "success" : "failure"}});

### src/aql/aql_syntax_highlighter.cpp
Total findings: 27

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 71: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: constexpr const char *FG_CYAN      = "\x1b[36m"; // core keywords
- Line 72: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: constexpr const char *FG_MAGENTA   = "\x1b[35m"; // LLM keywords
- Line 73: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: constexpr const char *FG_YELLOW    = "\x1b[33m"; // built-in functions
- Line 74: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: constexpr const char *FG_GREEN     = "\x1b[32m"; // string literals
- Line 75: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: constexpr const char *FG_BLUE      = "\x1b[34m"; // numbers
- Line 76: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: constexpr const char *FG_RED       = "\x1b[31m"; // error annotation marker
- Line 77: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: constexpr const char *FG_DARK_GREY = "\x1b[90m"; // comments
- Line 118: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({AQLTokenType::UNKNOWN, ws, tl, tc});
- Line 130: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({AQLTokenType::COMMENT, comment, tl, tc});
- Line 147: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({AQLTokenType::COMMENT, comment, tl, tc});
- Line 165: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({AQLTokenType::STRING, str, tl, tc});
- Line 176: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({AQLTokenType::IDENTIFIER, ident, tl, tc});
- Line 196: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({AQLTokenType::NUMBER, num, tl, tc});
- Line 226: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: tokens.push_back({ttype, ident, tl, tc});
- Line 327: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stack.push_back({c, tok.line, tok.column});
  Confidence: band=high; score=0.74
- Line 328: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stack.push_back({c, tok.line, tok.column});
- Line 331: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back({tok.line, tok.column, std::string("Unmatched closing '") + c + "'"});
- Line 336: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back({tok.line, tok.column,
- Line 345: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back({entry.line, entry.col, std::string("Unclosed '") + entry.ch + "'"});
  Confidence: band=high; score=0.74
- Line 346: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back({entry.line, entry.col, std::string("Unclosed '") + entry.ch + "'"});
- Line 354: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back({tok.line, tok.column, "Unterminated string literal"});
  Confidence: band=high; score=0.74
- Line 355: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back({tok.line, tok.column, "Unterminated string literal"});
- Line 360: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back({tok.line, tok.column, "Unterminated string literal"});
- Line 391: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: errors.push_back({tok.line, tok.column, "FOR clause is missing IN keyword"});
  Confidence: band=high; score=0.74
- Line 392: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: errors.push_back({tok.line, tok.column, "FOR clause is missing IN keyword"});

### src/aql/aql_query_validator.cpp
Total findings: 21

- Line 389: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator col_it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto col_it            = collection_fields.find(col);
- Line 98: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 110: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vars.push_back((*it)[1].str());
  Confidence: band=high; score=0.74
- Line 111: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vars.push_back((*it)[1].str());
- Line 122: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: vars.push_back((*it)[1].str());
  Confidence: band=high; score=0.74
- Line 123: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vars.push_back((*it)[1].str());
- Line 132: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.issues.push_back(
- Line 142: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.issues.push_back({ValidationIssue::Severity::WARNING,
- Line 156: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.issues.push_back({ValidationIssue::Severity::ERROR, "Query is missing a RETURN clause", "RETU
- Line 185: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.issues.push_back({ValidationIssue::Severity::WARNING,
- Line 199: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.issues.push_back({ValidationIssue::Severity::INFO,
- Line 216: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.issues.push_back({ValidationIssue::Severity::ERROR,
  Confidence: band=high; score=0.74
- Line 217: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.issues.push_back({ValidationIssue::Severity::ERROR,
- Line 317: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.issues.push_back({ValidationIssue::Severity::WARNING,
  Confidence: band=high; score=0.74
- Line 318: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.issues.push_back({ValidationIssue::Severity::WARNING,
- Line 337: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> var_to_collection;
  Confidence: band=medium; score=0.66
- Line 355: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<std::string>> collection_fields;
  Confidence: band=medium; score=0.66
- Line 361: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: collection_fields[col_lower].push_back(field_lower);
  Confidence: band=high; score=0.74
- Line 361: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: collection_fields[col_lower].push_back(field_lower);
  Confidence: band=high; score=0.74
- Line 362: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: collection_fields[col_lower].push_back(field_lower);
- Line 398: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.issues.push_back({ValidationIssue::Severity::WARNING,

### src/aql/aql_migration_assistant.cpp
Total findings: 20

- Line 67: severity=HIGH; category=observability; pattern=missing_trace_point
  Description: Critical function migrate without trace point
  Context: MigrationResult AQLMigrationAssistant::migrate(const std::string &arango_aql) const {
  Confidence: band=very_high; score=0.9
- Line 204: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * @param kw_pos     [out] Position of the keyword when found.
- Line 205: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * @param paren_pos  [out] Position of the '(' when found.
- Line 155: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(b == std::string::npos ? "" : part.substr(b, e - b + 1));
  Confidence: band=high; score=0.74
- Line 156: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(b == std::string::npos ? "" : part.substr(b, e - b + 1));
- Line 164: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(b == std::string::npos ? "" : part.substr(b, e - b + 1));
- Line 257: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: result += '@';
  Confidence: band=high; score=0.74
- Line 258: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: result += '@';
- Line 266: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: issues.push_back({MigrationIssue::Severity::INFO, "@@collection bind parameters rewritten to @collection",
  Confidence: band=high; score=0.74
- Line 267: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: issues.push_back({MigrationIssue::Severity::INFO, "@@collection bind parameters rewritten to @collec
- Line 310: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: issues.push_back({MigrationIssue::Severity::WARNING,
- Line 388: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: issues.push_back({MigrationIssue::Severity::WARNING,
- Line 466: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: issues.push_back({MigrationIssue::Severity::WARNING,
- Line 550: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: issues.push_back({MigrationIssue::Severity::WARNING,
- Line 629: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: issues.push_back({MigrationIssue::Severity::ERROR, "V8() is not supported in ThemisDB AQL",
- Line 660: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: issues.push_back({MigrationIssue::Severity::INFO, fn + "() is an ArangoDB-specific type-check function",
  Confidence: band=high; score=0.74
- Line 661: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: issues.push_back({MigrationIssue::Severity::INFO, fn + "() is an ArangoDB-specific type-check functi
- Line 691: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: issues.push_back({MigrationIssue::Severity::INFO, "HASH() is not available in ThemisDB AQL",
- Line 720: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: issues.push_back({MigrationIssue::Severity::INFO, "ATTRIBUTES() is not available in ThemisDB AQL",
- Line 748: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: issues.push_back({MigrationIssue::Severity::INFO, "TRANSLATE() is not available in ThemisDB AQL",

### src/aql/aql_query_template_library.cpp
Total findings: 20

- Line 33: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLQueryTemplateLibrary: template id must not be empty");
- Line 36: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 81: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (toLower(tmpl.name).find(lower_kw) != std::string::npos ||
- Line 81: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (toLower(tmpl.name).find(lower_kw) != std::string::npos ||
- Line 82: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: toLower(tmpl.description).find(lower_kw) != std::string::npos) {
- Line 82: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: toLower(tmpl.description).find(lower_kw) != std::string::npos) {
- Line 82: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: toLower(tmpl.description).find(lower_kw) != std::string::npos) {
- Line 88: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (toLower(t).find(lower_kw) != std::string::npos) {
- Line 118: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 133: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 67: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(tmpl);
  Confidence: band=high; score=0.74
- Line 67: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(tmpl);
  Confidence: band=high; score=0.74
- Line 68: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(tmpl);
- Line 82: severity=MEDIUM; category=repeated_lookup
  Description: Repeated find() for same key: lower_kw
  Remediation: Cache the result or use lower_bound/upper_bound for range operations
  Context: if (toLower(tmpl.name).find(lower_kw) != std::string::npos ||
- Line 83: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(tmpl);
  Confidence: band=high; score=0.74
- Line 84: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(tmpl);
- Line 89: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(tmpl);
  Confidence: band=high; score=0.74
- Line 90: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(tmpl);
- Line 114: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, std::string>& parameters
  Confidence: band=medium; score=0.66
- Line 128: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: const std::unordered_map<std::string, std::string>& parameters
  Confidence: band=medium; score=0.66

### src/aql/docs_assistant_functions.cpp
Total findings: 17

- Line 556: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Use std::unique_ptr<T> or std::make_unique<T>()
  Context: g_docs_assistant_functions = new DocsAssistantFunctions();
- Line 556: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: g_docs_assistant_functions = new DocsAssistantFunctions();
- Line 87: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Documentation database not loaded. Please ensure docs.db is available.");
- Line 157: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: answer     = lora->query(query, user_id);
- Line 198: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result = assistant->query(query);
- Line 211: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(std::string("HELP failed: ") + e.what());
- Line 389: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: auto result     = assistant->query(query);
- Line 427: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(std::string("DOCS_SEARCH failed: ") + e.what());
- Line 513: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: metrics["base_assistant"] = nullptr;
- Line 238: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 280: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 316: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string DocsAssistantFunctions::extractTopicFromQuery(const std::string &query) {
  Confidence: band=high; score=0.74
- Line 343: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string DocsAssistantFunctions::extractSearchQuery(const std::string &query) {
  Confidence: band=high; score=0.74
- Line 386: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string DocsAssistantFunctions::docsQuery(const std::string &query) {
  Confidence: band=high; score=0.74
- Line 421: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results.push_back(doc_json);
  Confidence: band=high; score=0.74
- Line 422: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results.push_back(doc_json);
- Line 512: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/aql/aql_query_diff_explainer.cpp
Total findings: 16

- Line 76: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: size_t p = norm.find(kw, search_from);
- Line 169: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (auto it = begin; it != std::sregex_iterator(); ++it) {
- Line 33: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += ' ';
  Confidence: band=high; score=0.74
- Line 34: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += ' ';
- Line 61: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> splitClauses(const std::string &norm) {
  Confidence: band=medium; score=0.66
- Line 62: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::string> clauses;
  Confidence: band=medium; score=0.66
- Line 84: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: matches.push_back({p, kw});
  Confidence: band=high; score=0.74
- Line 85: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: matches.push_back({p, kw});
- Line 164: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> detectFunctions(const std::string &norm) {
  Confidence: band=medium; score=0.66
- Line 167: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> fns;
  Confidence: band=medium; score=0.66
- Line 212: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> all_keys;
  Confidence: band=medium; score=0.66
- Line 222: severity=MEDIUM; category=repeated_lookup
  Description: Repeated find() for same key: kw
  Remediation: Cache the result or use lower_bound/upper_bound for range operations
  Context: if (all_keys.find(kw) == all_keys.end()) {
- Line 265: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.diffs.push_back(std::move(e));
  Confidence: band=high; score=0.74
- Line 266: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.diffs.push_back(std::move(e));
- Line 274: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.diffs.push_back(std::move(e));
  Confidence: band=high; score=0.74
- Line 275: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.diffs.push_back(std::move(e));

### src/aql/aql_rollback_suggester.cpp
Total findings: 15

- Line 34: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: out += ' ';
  Confidence: band=high; score=0.74
- Line 35: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: out += ' ';
- Line 224: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: rq << "// Rollback for: INSERT INTO " << coll << "\n"
- Line 224: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: rq << "// Rollback for: INSERT INTO " << coll << "\n"
- Line 242: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: rq << "// Rollback for: REMOVE … IN " << coll << "\n"
- Line 242: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: rq << "// Rollback for: REMOVE … IN " << coll << "\n"
- Line 259: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: rq << "// Rollback for: UPDATE … IN " << coll << "\n"
- Line 259: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: rq << "// Rollback for: UPDATE … IN " << coll << "\n"
- Line 277: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: rq << "// Rollback for: REPLACE … IN " << coll << "\n"
- Line 277: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: rq << "// Rollback for: REPLACE … IN " << coll << "\n"
- Line 296: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: rq << "// Rollback for: UPSERT … IN " << coll << "\n"
- Line 296: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: rq << "// Rollback for: UPSERT … IN " << coll << "\n"
- Line 297: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "// Remove documents that were inserted by the UPSERT (i.e. did\n"
- Line 298: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "// not exist before).\n"
- Line 302: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "// Documents that were UPDATED (existed before) must be\n"

### src/aql/classify_bridge.cpp
Total findings: 12

- Line 117: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto &spec : categorySpecs()) {
- Line 118: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (scores.find(spec.label) == scores.end()) {
- Line 122: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (query_lower.find(kw) != std::string::npos) {
- Line 174: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (max_raw == 0.0) {
  Confidence: band=very_high; score=0.9
- Line 110: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> scoreCategories(const std::string &query_lower,
  Confidence: band=medium; score=0.66
- Line 112: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> scores;
  Confidence: band=medium; score=0.66
- Line 132: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> softmax(const std::unordered_map<std::string, double> &raw) {
  Confidence: band=medium; score=0.66
- Line 142: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> exp_vals;
  Confidence: band=medium; score=0.66
- Line 150: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, double> result;
  Confidence: band=medium; score=0.66
- Line 218: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: cats.push_back(c);
  Confidence: band=high; score=0.74
- Line 219: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: cats.push_back(c);
- Line 242: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/aql/aql_conversation_context.cpp
Total findings: 11

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 158: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &m : history_snapshot) {
  Confidence: band=very_high; score=0.9
- Line 257: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLConversationContext::start: intent must not be empty");
- Line 276: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("AQLConversationContext::refine: instruction must not be empty");
- Line 281: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::logic_error("AQLConversationContext::refine: call start() before refine()");
- Line 71: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: static std::string cleanQuery(const std::string &raw) {
  Confidence: band=high; score=0.74
- Line 158: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pairs.emplace_back(m.role, m.content);
  Confidence: band=high; score=0.74
- Line 323: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string AQLConversationContext::lastQuery() const {
  Confidence: band=high; score=0.74
- Line 332: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.emplace_back(msg.role, msg.content);
  Confidence: band=high; score=0.74

### src/aql/aql_model_router.cpp
Total findings: 9

- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 122: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: routes_.push_back(route);
  Confidence: band=high; score=0.74
- Line 123: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: routes_.push_back(route);
- Line 154: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scores.push_back({rule.type, hits});
  Confidence: band=high; score=0.74
- Line 154: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: scores.push_back({rule.type, hits});
  Confidence: band=high; score=0.74
- Line 155: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: scores.push_back({rule.type, hits});
- Line 172: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(s.type);
  Confidence: band=high; score=0.74
- Line 173: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(s.type);
- Line 180: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(QueryModelType::RELATIONAL);

### src/aql/aql_confidence_scorer.cpp
Total findings: 8

- Line 140: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator colon may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto colon = rest.find(':');
- Line 178: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto isWordChar = [](char c) -> bool { return std::isalnum(static_cast<unsigned char>(c)) || c == '_
- Line 128: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: auto it = std::find_if(line.begin(), line.end(), [](unsigned char c) { return !std::isspace(c); });
- Line 201: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: xs.push_back(scoreStructure(lower));
  Confidence: band=high; score=0.74
- Line 202: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: xs.push_back(scoreStructure(lower));
- Line 203: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ys.push_back(scoreCompleteness(lower));
- Line 204: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: zs.push_back(scoreSchemaMatch(lower, ""));
- Line 205: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: targets.push_back(truth);

### src/aql/aql_ingestion_bridge.cpp
Total findings: 5

- Line 0: severity=CRITICAL; category=uncategorized
  Confidence: band=very_high; score=0.85
- Line 30: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 73: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: payload["_entities"] = std::move(entity_array);
- Line 66: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: entity_array.push_back({
  Confidence: band=high; score=0.74
- Line 67: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: entity_array.push_back({

### src/aql/llm_aql_embedding_bridge.cpp
Total findings: 5

- Line 17: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: std::vector<float> LLMAQLEmbeddingBridge::embed(const std::string& text) {
  Confidence: band=high; score=0.74
- Line 22: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: return handler_.executeEmbed(text);
  Confidence: band=high; score=0.74
- Line 24: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: spdlog::debug("LLMAQLEmbeddingBridge::embed(): executeEmbed failed ({}); "
  Confidence: band=high; score=0.74
- Line 27: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 28: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: spdlog::debug("LLMAQLEmbeddingBridge::embed(): executeEmbed threw unknown exception; "
  Confidence: band=high; score=0.74

### src/aql/aql_optimizer_advisor.cpp
Total findings: 3

- Line 41: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 141: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: suggestions.push_back({ValidationIssue::Severity::INFO, msg, "INDEX"});
  Confidence: band=high; score=0.74
- Line 142: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: suggestions.push_back({ValidationIssue::Severity::INFO, msg, "INDEX"});

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
