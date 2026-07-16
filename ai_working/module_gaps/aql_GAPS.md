# aql Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: aql
- Generated: 2026-06-04 08:50:21
- Status: Critical Findings Present
- Total Findings: 271
- Actionable Findings (Critical + High): 117
- Affected Files: 23

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 19 |
| High | 98 |
| Medium | 146 |
| Low | 8 |

## Category Summary

| Category | Count |
|---|---:|
| copy_overhead | 41 |
| uncaught_exception | 30 |
| hardcoded_path | 25 |
| unordered_container_iter | 19 |
| missing_resource_limits | 14 |
| uninitialized_access | 12 |
| string_concat_loop | 11 |
| missing_latency_metric | 10 |
| no_retry_logic | 9 |
| data_race | 8 |
| resource_leaked_in_exception | 8 |
| uninitialized_member_field | 8 |
| o_n_squared | 7 |
| pointer_arithmetic_unbounded | 7 |
| unvalidated_llm_output | 7 |
| hardcoded_output | 6 |
| missing_move_constructor_defaulted | 5 |
| generic_catch | 4 |
| unsanitized_llm_input | 4 |
| command_injection | 3 |
| missing_trace_point | 3 |
| model_integrity_gap | 3 |
| repeated_search | 3 |
| array_bounds_violation | 2 |
| db_connection_leak | 2 |
| module_doc_linkset_drift | 2 |
| range_temporary | 2 |
| repeated_lookup | 2 |
| smart_ptr_misuse | 2 |
| delete_without_nullptr | 1 |
| exception_in_destructor | 1 |
| explicit_delete | 1 |
| fp_exact_comparison | 1 |
| missing_correlation_id | 1 |
| missing_vector_reserve | 1 |
| new_without_delete | 1 |
| new_without_raii | 1 |
| null_dereference | 1 |
| prompt_injection | 1 |
| shared_state_no_sync | 1 |
| timestamp_sorting_unstable | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| aql/llm_aql_handler.cpp | 96 | 7 | 51 | 32 | 6 |
| aql/aql_query_builder.cpp | 30 | 0 | 3 | 27 | 0 |
| aql/aql_syntax_highlighter.cpp | 19 | 0 | 1 | 18 | 0 |
| aql/aql_fewshot_example_library.cpp | 14 | 3 | 4 | 7 | 0 |
| aql/aql_lora_finetuner.cpp | 13 | 1 | 7 | 5 | 0 |
| aql/aql_rollback_suggester.cpp | 12 | 0 | 2 | 10 | 0 |
| aql/aql_migration_assistant.cpp | 11 | 0 | 6 | 5 | 0 |
| aql/aql_query_diff_explainer.cpp | 11 | 0 | 2 | 9 | 0 |
| aql/aql_agent.cpp | 9 | 1 | 5 | 3 | 0 |
| aql/classify_bridge.cpp | 9 | 0 | 4 | 5 | 0 |
| aql/docs_assistant_functions.cpp | 9 | 3 | 1 | 5 | 0 |
| aql/aql_query_template_library.cpp | 7 | 0 | 4 | 3 | 0 |
| aql/aql_confidence_scorer.cpp | 6 | 3 | 2 | 1 | 0 |
| aql/aql_conversation_context.cpp | 5 | 0 | 3 | 2 | 0 |
| aql/aql_query_validator.cpp | 5 | 0 | 1 | 4 | 0 |
| aql/aql_autocomplete.cpp | 4 | 0 | 1 | 3 | 0 |
| aql/llm_aql_embedding_bridge.cpp | 4 | 0 | 0 | 4 | 0 |
| aql/aql_model_router.cpp | 2 | 0 | 0 | 2 | 0 |
| aql/FUTURE_ENHANCEMENTS.md | 1 | 0 | 0 | 0 | 1 |
| aql/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| aql/aql_ingestion_bridge.cpp | 1 | 1 | 0 | 0 | 0 |
| aql/aql_optimizer_advisor.cpp | 1 | 0 | 0 | 1 | 0 |
| aql/llm_metrics_collector.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### aql/llm_aql_handler.cpp
Total findings: 96

- Line 579: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: impl_->sharding_manager_ = sharding_manager;
- Line 1174: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: if (impl_->sharding_manager_ != nullptr) {
- Line 1175: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const auto target_shard = impl_->sharding_manager_->GetShardForKey("llm_embeddings", text);
- Line 1210: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: plugin_mgr.loadModel(model_id, path);
- Line 1219: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: plugin_mgr.unloadModel(model_id);
- Line 1237: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: plugin_mgr.loadModel(model_id, blob_urn);
- Line 1567: severity=CRITICAL; category=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string full_prompt = sys_prompt + user_prompt_str;
- Line 170: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: void sanitizePromptInput(const std::string &input, const std::string &field_name, std::size_t max_length = 0) {

    // --- Length check ---

    if (max_length > 0 && input.size() > max_length) {

        throw LLMException(LLMErrorCode::PROMPT_TOO_LONG, field_name + " exceeds maximum allowed length of "

                                                              + std::to_string(max_length) + " characters");

    }
- Line 177: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // --- Null-byte / dangerous control character check ---

    for (unsigned char c : input) {

        if (c == '\0') {

            throw LLMException(LLMErrorCode::PROMPT_INJECTION,

                               field_name + " contains a null byte (potential injection vector)");

        }

    }
- Line 235: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: for (const auto &pattern : kInjectionPatterns) {

        if (lower.find(pattern) != std::string::npos) {

            spdlog::warn("Prompt injection attempt detected in {}: pattern \"{}\"", field_name, pattern);

            throw LLMException(LLMErrorCode::PROMPT_INJECTION,

                               field_name + " rejected: potential prompt injection detected");

        }

    }
- Line 270: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: LLMErrorCode failure_code) {

    if (config.enable_c1_cai_safety_gate) {

        if (!config.c1_cai_eval_fn) {

            throw LLMException(

                failure_code,

                "Wave C C1 safety gate enabled but c1_cai_eval_fn is not configured");

        }
- Line 275: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto safety_result = config.c1_cai_eval_fn(generated_response, original_query);
- Line 277: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: auto safety_result = config.c1_cai_eval_fn(generated_response, original_query);

        if (!safety_result) {

            throw LLMException(

                failure_code,

                "Wave C C1 safety evaluation failed: " + safety_result.error().message());

        }
- Line 282: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: "Wave C C1 safety evaluation failed: " + safety_result.error().message());

        }

        if (!std::isfinite(*safety_result)) {

            throw LLMException(

                failure_code,

                "Wave C C1 safety evaluation returned non-finite score");

        }
- Line 287: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: "Wave C C1 safety evaluation returned non-finite score");

        }

        if (*safety_result < config.c1_min_safety_score) {

            throw LLMException(

                failure_code,

                "Wave C C1 safety gate rejected response (score="

                    + std::to_string(*safety_result)
- Line 640: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto result = impl_->timeout_manager_.executeInferWithCancelToken([&](auto cancel_token) {
- Line 697: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: } else {

                            routing_decision = "LOCAL_FALLBACK_NO_RESOLVER";

                        }

                        request.metadata["domain_hint"]      = *domain;

                        request.metadata["routing_decision"] = routing_decision;

                        if (!routed_shard_id.empty()) {

                            request.metadata["target_shard_id"] = routed_shard_id;
- Line 698: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: routing_decision = "LOCAL_FALLBACK_NO_RESOLVER";

                        }

                        request.metadata["domain_hint"]      = *domain;

                        request.metadata["routing_decision"] = routing_decision;

                        if (!routed_shard_id.empty()) {

                            request.metadata["target_shard_id"] = routed_shard_id;

                        }
- Line 700: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: request.metadata["domain_hint"]      = *domain;

                        request.metadata["routing_decision"] = routing_decision;

                        if (!routed_shard_id.empty()) {

                            request.metadata["target_shard_id"] = routed_shard_id;

                        }

                    }
- Line 721: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Parse options for generation parameters

                    if (options.count("max_tokens")) {

                        request.max_tokens = std::stoi(options.at("max_tokens"));

                    }

                    if (options.count("temperature")) {

                        request.temperature = std::stof(options.at("temperature"));
- Line 724: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: request.max_tokens = std::stoi(options.at("max_tokens"));

                    }

                    if (options.count("temperature")) {

                        request.temperature = std::stof(options.at("temperature"));

                    }

                    if (options.count("top_p")) {

                        request.top_p = std::stof(options.at("top_p"));
- Line 727: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: request.temperature = std::stof(options.at("temperature"));

                    }

                    if (options.count("top_p")) {

                        request.top_p = std::stof(options.at("top_p"));

                    }

                    if (options.count("top_k")) {

                        request.top_k = std::stoi(options.at("top_k"));
- Line 730: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: request.top_p = std::stof(options.at("top_p"));

                    }

                    if (options.count("top_k")) {

                        request.top_k = std::stoi(options.at("top_k"));

                    }

                    if (options.count("repetition_penalty")) {

                        request.repetition_penalty = std::stof(options.at("repetition_penalty"));
- Line 733: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: request.top_k = std::stoi(options.at("top_k"));

                    }

                    if (options.count("repetition_penalty")) {

                        request.repetition_penalty = std::stof(options.at("repetition_penalty"));

                    }



                    // Wrap any streaming callback so token delivery stops on cancellation.
- Line 741: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (!cancel_token->load(std::memory_order_acquire)) {
- Line 748: severity=HIGH; category=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto response = plugin_mgr.generate(request);
- Line 748: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto response = plugin_mgr.generate(request);
- Line 814: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: impl_->token_estimator_->estimate(prompt), 0, false, "INVALID_OPTIONS");



        // Catch option parsing errors

        throw LLMException(LLMErrorCode::INVALID_OPTIONS, std::string("Invalid option value: ") + e.what());

    } catch (const std::exception &e) {

        // Record failure

        impl_->getBreaker("infer").recordFailure();
- Line 826: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: impl_->token_estimator_->estimate(prompt), 0, false, "INFERENCE_FAILED");



        // Wrap other exceptions as internal errors (mask details)

        throw LLMException(LLMErrorCode::INFERENCE_FAILED, std::string("Inference operation failed: ") + e.what());

    }

}
- Line 860: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: request.lora_adapter_id = lora_id;

        }

        if (options.count("max_tokens")) {

            request.max_tokens = std::stoi(options.at("max_tokens"));

        }

        if (options.count("temperature")) {

            request.temperature = std::stof(options.at("temperature"));
- Line 877: severity=HIGH; category=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto response    = plugin_mgr.generate(request);
- Line 877: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto response    = plugin_mgr.generate(request);
- Line 931: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: metrics.recordInference(model_id.empty() ? "default" : model_id, lora_id, latency,

                                impl_->token_estimator_->estimate(prompt), 0, false, "INFERENCE_FAILED");



        throw LLMException(LLMErrorCode::INFERENCE_FAILED, std::string("Streaming inference failed: ") + e.what());

    }

}
- Line 1062: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // Parse options

                    if (options.count("max_tokens")) {

                        request.max_tokens = std::stoi(options.at("max_tokens"));

                    }

                    if (options.count("temperature")) {

                        request.temperature = std::stof(options.at("temperature"));
- Line 1065: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: request.max_tokens = std::stoi(options.at("max_tokens"));

                    }

                    if (options.count("temperature")) {

                        request.temperature = std::stof(options.at("temperature"));

                    }

                    if (options.count("top_p")) {

                        request.top_p = std::stof(options.at("top_p"));
- Line 1068: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: request.temperature = std::stof(options.at("temperature"));

                    }

                    if (options.count("top_p")) {

                        request.top_p = std::stof(options.at("top_p"));

                    }



                    // Wrap any streaming callback so token delivery stops on cancellation.
- Line 1076: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (!cancel_token->load(std::memory_order_acquire)) {
- Line 1083: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto response = plugin_mgr.generateRAG(context, request);
- Line 1147: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: false, "INVALID_OPTIONS");



        // Catch option parsing errors

        throw LLMException(LLMErrorCode::INVALID_OPTIONS, std::string("Invalid option value: ") + e.what());

    } catch (const std::exception &e) {

        // Record failure

        impl_->getBreaker("rag").recordFailure();
- Line 1159: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: false, "RAG_FAILED");



        // Wrap other exceptions as internal errors (mask details)

        throw LLMException(LLMErrorCode::RAG_FAILED, std::string("RAG operation failed: ") + e.what());

    }

}
- Line 1296: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return oss.str();

    } catch (const std::exception &e) {

        throw std::runtime_error(std::string("LLM STATS failed: ") + e.what());

    }

}
- Line 1340: severity=HIGH; category=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::vector<std::string> LLMAQLHandler::executeBatchInfer(const std::vector<BatchInferRequest> &requests) {
- Line 1376: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: return results;

    } catch (const std::exception &e) {

        throw std::runtime_error(std::string("Batch LLM INFER failed: ") + e.what());

    }

}
- Line 1498: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: });

                validation_feedback = (err_it != vresult.issues.end()) ? err_it->message : "unknown validation error";

                if (mode == TranslationValidationMode::REJECT_ON_ERROR || attempt + 1 >= max_attempts) {

                    throw LLMException(LLMErrorCode::INVALID_RESPONSE,

                                       "Generated AQL failed validation: " + validation_feedback);

                }

                // RETRY_ON_ERROR: log warning and retry with feedback
- Line 1524: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::string acl_err =

                    checkGeneratedAQLCollectionAccess(aql_query, impl_->collection_access_checker_);

                if (!acl_err.empty()) {

                    throw LLMException(LLMErrorCode::ACCESS_DENIED, acl_err);

                }

            }
- Line 1535: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // unchanged so callers can distinguish them from generic errors.

            throw;

        } catch (const std::exception &e) {

            throw std::runtime_error(std::string("NL to AQL translation failed: ") + e.what());

        }

    }
- Line 1540: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



    // Reached only when max_attempts > 1 and all retries produced validation errors.

    throw LLMException(LLMErrorCode::INVALID_RESPONSE,

                       "Generated AQL failed validation after all retries: " + validation_feedback);

}
- Line 1600: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: });

                validation_feedback = (err_it != vresult.issues.end()) ? err_it->message : "unknown validation error";

                if (mode == TranslationValidationMode::REJECT_ON_ERROR || attempt + 1 >= max_attempts) {

                    throw LLMException(LLMErrorCode::INVALID_RESPONSE,

                                       "Generated AQL failed validation: " + validation_feedback);

                }

                // RETRY_ON_ERROR: log warning and retry with feedback
- Line 1624: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::string acl_err =

                    checkGeneratedAQLCollectionAccess(aql_query, impl_->collection_access_checker_);

                if (!acl_err.empty()) {

                    throw LLMException(LLMErrorCode::ACCESS_DENIED, acl_err);

                }

            }
- Line 1632: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



        // Reached only when max_attempts > 1 and all retries produced validation errors.

        throw LLMException(LLMErrorCode::INVALID_RESPONSE,

                           "Generated AQL failed validation after all retries: " + validation_feedback);



    } catch (const LLMException &) {
- Line 1640: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // unchanged so callers can distinguish security-related failures from generic errors.

        throw;

    } catch (const std::exception &e) {

        throw std::runtime_error(std::string("NL to AQL streaming translation failed: ") + e.what());

    }

}
- Line 1838: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: });

                validation_feedback = (err_it != vresult.issues.end()) ? err_it->message : "unknown validation error";

                if (mode == TranslationValidationMode::REJECT_ON_ERROR || attempt + 1 >= max_attempts) {

                    throw LLMException(LLMErrorCode::INVALID_RESPONSE,

                                       "Generated AQL failed validation: " + validation_feedback);

                }

                // RETRY_ON_ERROR: log warning and retry with feedback
- Line 1861: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: std::string acl_err =

                    checkGeneratedAQLCollectionAccess(aql_query, impl_->collection_access_checker_);

                if (!acl_err.empty()) {

                    throw LLMException(LLMErrorCode::ACCESS_DENIED, acl_err);

                }

            }
- Line 1875: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: // unchanged so callers can distinguish them from generic errors.

            throw;

        } catch (const std::exception &e) {

            throw std::runtime_error(std::string("NL to AQL translation with examples failed: ") + e.what());

        }

    }
- Line 1880: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: }



    // Reached only when max_attempts > 1 and all retries produced validation errors.

    throw LLMException(LLMErrorCode::INVALID_RESPONSE,

                       "Generated AQL failed validation after all retries: " + validation_feedback);

}
- Line 1926: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: const std::string response = executeInfer(prompt.str());
- Line 1938: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isspace(c); }
- Line 1939: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c) { return !std::isspace(c); }).base(),
- Line 125: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::string buildChatOriginalQuery(const std::vector<llm::ChatMessage>& messages) {
- Line 129: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: combined_user_content += "\n";
- Line 130: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: combined_user_content += "\n";
- Line 142: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: all_content += "\n";
- Line 143: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: all_content += "\n";
- Line 341: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: referenced_collections.push_back((*it)[1].str());
- Line 366: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: static std::unordered_set<std::string> extractReferencedCollectionsForAccessCheck(
- Line 369: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> collections;
- Line 620: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string LLMAQLHandler::executeInfer(const std::string &prompt, const std::string &model_id,
- Line 748: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto response = plugin_mgr.generate(request);
- Line 877: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto response    = plugin_mgr.generate(request);
- Line 972: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto query_embedding = THEMIS_LLM_EMBED(query);
- Line 1014: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: } else {

                                                            doc.content = raw_str;

                                                        }

                                                    } catch (...) {

                                                        doc.content = raw_str;

                                                    }

                                                } else {
- Line 1014: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 1020: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: } else {

                                                    doc.content = result.pk;

                                                }

                                            } catch (...) {

                                                doc.content = result.pk;

                                            }

                                        } else {
- Line 1020: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 1163: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::vector<float> LLMAQLHandler::executeEmbed(const std::string &text, const std::string &model_id) {
- Line 1193: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto embedding = THEMIS_LLM_EMBED(text);
- Line 1266: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: ids.push_back(lora.lora_id.empty() ? lora.id : lora.lora_id);
- Line 1285: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: oss << "  Throughput: " << stats.throughput << " req/s\n";
- Line 1340: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::vector<std::string> LLMAQLHandler::executeBatchInfer(const std::vector<BatchInferRequest> &requests) {
- Line 1347: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::vector<size_t>> indices_by_domain;
- Line 1362: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: shard_results.emplace_back(idx, executeInfer(req.prompt, req.model_id, req.lora_id, req.options));
- Line 1388: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: out += "You are an expert in AQL (Application Query Language) for ThemisDB.\n";
- Line 1530: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1635: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1743: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1762: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1870: severity=MEDIUM; category=missing_move_constructor_defaulted
  Description: Missing move ctor prevents efficient rvalue transfers
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1926: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: const std::string response = executeInfer(prompt.str());
- Line 1952: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: result.score = std::stof(line.substr(7));

                    // Clamp to [0, 1]

                    result.score = std::max(0.0f, std::min(1.0f, result.score));

                } catch (...) {

                    result.score = -1.0f;

                }

                in_suggestions = false;
- Line 1952: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 1452: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // Sanitize inputs before embedding them in the LLM prompt.
- Line 1548: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // Sanitize inputs (same rules as translateNLToAQL)
- Line 1734: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // Sanitize inputs before embedding them in the LLM prompt
- Line 1753: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // Sanitize inputs before embedding them in the LLM prompt
- Line 1792: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // Sanitize inputs (same rules as translateNLToAQL)
- Line 1901: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: // LLM-4: sanitize user-supplied inputs before embedding in prompt

### aql/aql_query_builder.cpp
Total findings: 30

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4222 feat(aql): AQLQueryBuilder ... (2026-03-15) | #3479 [Docs-Audit] src/aq
- Line 416: severity=HIGH; category=command_injection
  Description: sql_injection_concat: SQL injection risk — use prepared statements
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: impl_->let_clauses.push_back(variable + " = ( " + inner_query + " )");
- Line 620: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 28: severity=MEDIUM; category=uninitialized_member_field
  Description: Default constructor does not initialize POD members
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Struct with uninitialized fields
- Line 33: severity=MEDIUM; category=uninitialized_member_field
  Description: Default constructor does not initialize POD members
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Struct with uninitialized fields
- Line 38: severity=MEDIUM; category=uninitialized_member_field
  Description: Default constructor does not initialize POD members
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Struct with uninitialized fields
- Line 43: severity=MEDIUM; category=uninitialized_member_field
  Description: Default constructor does not initialize POD members
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Struct with uninitialized fields
- Line 56: severity=MEDIUM; category=uninitialized_member_field
  Description: Default constructor does not initialize POD members
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Struct with uninitialized fields
- Line 65: severity=MEDIUM; category=uninitialized_member_field
  Description: Default constructor does not initialize POD members
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Struct with uninitialized fields
- Line 407: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: AQLQueryBuilder &AQLQueryBuilder::subquery(const std::string &variable, const AQLQueryBuilder &inner) {
- Line 455: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::string AQLQueryBuilder::getPartialQuery() const {
- Line 549: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: steps.push_back("FOR");
- Line 550: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: steps.push_back("INSERT");
- Line 551: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: steps.push_back("UPSERT");
- Line 552: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: steps.push_back("UPDATE");
- Line 553: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: steps.push_back("REMOVE");
- Line 554: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: steps.push_back("REPLACE");
- Line 561: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: steps.push_back("LET");
- Line 562: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: steps.push_back("FILTER");
- Line 563: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: steps.push_back("WINDOW");
- Line 564: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: steps.push_back("COLLECT");
- Line 565: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: steps.push_back("SORT");
- Line 567: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: steps.push_back("LIMIT");
- Line 569: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: steps.push_back("RETURN");
- Line 571: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: steps.push_back("INSERT");
- Line 572: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: steps.push_back("UPDATE");
- Line 573: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: steps.push_back("REMOVE");
- Line 574: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: steps.push_back("REPLACE");
- Line 575: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: steps.push_back("UPSERT");
- Line 578: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: steps.push_back("FOR");

### aql/aql_syntax_highlighter.cpp
Total findings: 19

- Line 432: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 72: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: constexpr const char *FG_CYAN      = "\x1b[36m"; // core keywords
- Line 73: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: constexpr const char *FG_MAGENTA   = "\x1b[35m"; // LLM keywords
- Line 74: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: constexpr const char *FG_YELLOW    = "\x1b[33m"; // built-in functions
- Line 75: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: constexpr const char *FG_GREEN     = "\x1b[32m"; // string literals
- Line 76: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: constexpr const char *FG_BLUE      = "\x1b[34m"; // numbers
- Line 77: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: constexpr const char *FG_RED       = "\x1b[31m"; // error annotation marker
- Line 78: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: constexpr const char *FG_DARK_GREY = "\x1b[90m"; // comments
- Line 119: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: tokens.push_back({AQLTokenType::UNKNOWN, ws, tl, tc});
- Line 131: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: tokens.push_back({AQLTokenType::COMMENT, comment, tl, tc});
- Line 148: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: tokens.push_back({AQLTokenType::COMMENT, comment, tl, tc});
- Line 166: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: tokens.push_back({AQLTokenType::STRING, str, tl, tc});
- Line 316: severity=MEDIUM; category=uninitialized_member_field
  Description: Default constructor does not initialize POD members
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Struct with uninitialized fields
- Line 329: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: stack.push_back({c, tok.line, tok.column});
- Line 332: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: errors.push_back({tok.line, tok.column, std::string("Unmatched closing '") + c + "'"});
- Line 337: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: errors.push_back({tok.line, tok.column,
- Line 347: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: errors.push_back({entry.line, entry.col, std::string("Unclosed '") + entry.ch + "'"});
- Line 356: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: errors.push_back({tok.line, tok.column, "Unterminated string literal"});
- Line 361: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: errors.push_back({tok.line, tok.column, "Unterminated string literal"});

### aql/aql_fewshot_example_library.cpp
Total findings: 14

- Line 106: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: const std::vector<float> query_emb = embedding_provider_->embed(nl_query);
- Line 179: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: embedding_cache_[i] = embedding_provider_->embed(examples_[i].nl_query);
- Line 197: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: embedding_cache_[idx] = embedding_provider_->embed(examples_[idx].nl_query);
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4175 fix(aql/test): correct AC-4... (2026-03-13) | #3002 [aql] Few-shot AQL
- Line 108: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: auto it    = index_by_id_.find(c->id);
- Line 168: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 313: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 90: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: candidates.push_back(&ex);
- Line 355: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "FOR v, e IN 1..1 OUTBOUND \"users/42\" friends RETURN v",
- Line 363: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "FOR v IN 1..3 OUTBOUND \"users/1\" edges RETURN v",
- Line 379: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "FOR v IN 1..2 ANY \"product/99\" related_to RETURN v",
- Line 387: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "FOR v, e IN 1..1 OUTBOUND \"users/alice\" works_with\n  FILTER e.strength > 5\n  RETURN v",
- Line 395: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "FOR v, e, p IN 1..5 OUTBOUND \"users/1\" edges\n  FILTER v._id == \"users/10\"\n  RETURN p",
- Line 484: severity=MEDIUM; category=timestamp_sorting_unstable
  Description: Timestamp-based sorting without stable_sort (non-deterministic ties)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: "SORT r.timestamp ASC\n  RETURN r",

### aql/aql_lora_finetuner.cpp
Total findings: 13

- Line 358: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: samples_.push_back(makeSample("Create a new collection called events", "CREATE COLLECTION events", C
- Line 92: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: TrainingDataSample s;

    s.input                = input;

    s.output               = output;

    s.metadata["category"] = static_cast<int>(cat);

    return s;

}
- Line 366: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: "  FIELDS ['embedding']\n"
- Line 373: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: "  FIELDS ['body']",
- Line 462: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: TrainingData dataset;

    dataset.dataset_name             = dataset_name;

    dataset.samples                  = samples_;

    dataset.metadata["created_at"]   = isoTimestamp();

    dataset.metadata["num_samples"]  = samples_.size();

    dataset.metadata["dataset_type"] = "aql_finetuning";

    return dataset;
- Line 463: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: dataset.dataset_name             = dataset_name;

    dataset.samples                  = samples_;

    dataset.metadata["created_at"]   = isoTimestamp();

    dataset.metadata["num_samples"]  = samples_.size();

    dataset.metadata["dataset_type"] = "aql_finetuning";

    return dataset;

}
- Line 464: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: dataset.samples                  = samples_;

    dataset.metadata["created_at"]   = isoTimestamp();

    dataset.metadata["num_samples"]  = samples_.size();

    dataset.metadata["dataset_type"] = "aql_finetuning";

    return dataset;

}
- Line 513: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: throw std::invalid_argument("LoRA 'rank' must be in [1, 256], got " + it->second);
- Line 44: severity=MEDIUM; category=missing_correlation_id
  Description: Distributed call without correlation ID
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: #include "llm/lora_framework/lora_training_service.h"
- Line 160: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "FOR v IN 1..2 OUTBOUND 'users/alice' friends\n"
- Line 177: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "FOR v, e IN 1..2 INBOUND 'products/42' recommendations\n"
- Line 308: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "  FROM '/models/llama-3-8b-instruct.gguf'\n"
- Line 328: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: "  FROM '/adapters/legal-v1.gguf'\n"

### aql/aql_rollback_suggester.cpp
Total findings: 12

- Line 220: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: switch (result.mutation_type) {

        // ------------------------------------------------------------------

        // INSERT rollback: delete the inserted documents.

        // We identify them via their _key (caller must supply @keys bind param

        // or adapt the filter).

        // ------------------------------------------------------------------
- Line 220: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: // INSERT rollback: delete the inserted documents.
- Line 35: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: out += ' ';
- Line 36: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: out += ' ';
- Line 225: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: rq << "// Rollback for: INSERT INTO " << coll << "\n"
- Line 243: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: rq << "// Rollback for: REMOVE … IN " << coll << "\n"
- Line 260: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: rq << "// Rollback for: UPDATE … IN " << coll << "\n"
- Line 278: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: rq << "// Rollback for: REPLACE … IN " << coll << "\n"
- Line 297: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: rq << "// Rollback for: UPSERT … IN " << coll << "\n"
- Line 298: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "// Remove documents that were inserted by the UPSERT (i.e. did\n"
- Line 299: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "// not exist before).\n"
- Line 303: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "// Documents that were UPDATED (existed before) must be\n"

### aql/aql_migration_assistant.cpp
Total findings: 11

- Line 68: severity=HIGH; category=missing_trace_point
  Description: Critical function migrate without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: MigrationResult AQLMigrationAssistant::migrate(const std::string &arango_aql) const {
- Line 72: severity=HIGH; category=command_injection
  Description: sql_injection_concat: SQL injection risk — use prepared statements
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: result.migrated_query       = "";
- Line 205: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * @param kw_pos     [out] Position of the keyword when found.
- Line 206: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * @param paren_pos  [out] Position of the '(' when found.
- Line 489: severity=HIGH; category=command_injection
  Description: sql_injection_concat: SQL injection risk — use prepared statements
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: + search_query + ", 1)" + " RETURN _ft_doc)";
- Line 527: severity=HIGH; category=shared_state_no_sync
  Description: Shared state accessed without synchronization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: //          → (FOR _doc IN <collection> FILTER _doc._key == <key> LIMIT 1 RETURN _doc)
- Line 156: severity=MEDIUM; category=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: result.push_back(b == std::string::npos ? "" : part.substr(b, e - b + 1));
- Line 157: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(b == std::string::npos ? "" : part.substr(b, e - b + 1));
- Line 165: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: result.push_back(b == std::string::npos ? "" : part.substr(b, e - b + 1));
- Line 258: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: result += '@';
- Line 259: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: result += '@';

### aql/aql_query_diff_explainer.cpp
Total findings: 11

- Line 77: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: size_t p = norm.find(kw, search_from);
- Line 170: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (auto it = begin; it != std::sregex_iterator(); ++it) {
- Line 34: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: out += ' ';
- Line 35: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: out += ' ';
- Line 62: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::string> splitClauses(const std::string &norm) {
- Line 63: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::string> clauses;
- Line 86: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: matches.push_back({p, kw});
- Line 165: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> detectFunctions(const std::string &norm) {
- Line 168: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> fns;
- Line 213: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> all_keys;
- Line 223: severity=MEDIUM; category=repeated_lookup
  Description: Repeated find() for same key: kw
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (all_keys.find(kw) == all_keys.end()) {

### aql/aql_agent.cpp
Total findings: 9

- Line 99: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: raw_response        = handler_->executeInfer(llm_input, config_.model_alias,
- Line 78: severity=HIGH; category=missing_trace_point
  Description: Critical function execute without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: AgentResult execute(const std::string &task, const json &context) {
- Line 99: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: raw_response        = handler_->executeInfer(llm_input, config_.model_alias,
- Line 99: severity=HIGH; category=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: raw_response        = handler_->executeInfer(llm_input, config_.model_alias,
- Line 99: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: raw_response        = handler_->executeInfer(llm_input, config_.model_alias,
- Line 293: severity=HIGH; category=missing_trace_point
  Description: Critical function execute without trace point
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: AgentResult ReActAgent::execute(const std::string &task, const json &context) {
- Line 78: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: AgentResult execute(const std::string &task, const json &context) {
- Line 99: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: raw_response        = handler_->executeInfer(llm_input, config_.model_alias,
- Line 293: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: AgentResult ReActAgent::execute(const std::string &task, const json &context) {

### aql/classify_bridge.cpp
Total findings: 9

- Line 118: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto &spec : categorySpecs()) {
- Line 119: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (scores.find(spec.label) == scores.end()) {
- Line 123: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (query_lower.find(kw) != std::string::npos) {
- Line 175: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (max_raw == 0.0) {
- Line 111: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, double> scoreCategories(const std::string &query_lower,
- Line 113: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, double> scores;
- Line 133: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, double> softmax(const std::unordered_map<std::string, double> &raw) {
- Line 143: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, double> exp_vals;
- Line 151: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, double> result;

### aql/docs_assistant_functions.cpp
Total findings: 9

- Line 557: severity=CRITICAL; category=new_without_delete
  Description: Raw new without RAII wrapper — potential memory leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: g_docs_assistant_functions = new DocsAssistantFunctions();
- Line 557: severity=CRITICAL; category=new_without_raii
  Description: Raw new() without RAII wrapper — potential memory leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: DocsAssistantFunctions &getDocsAssistantFunctions() {

    if (!g_docs_assistant_functions) {

        g_docs_assistant_functions = new DocsAssistantFunctions();

    }

    return *g_docs_assistant_functions;

}
- Line 557: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: g_docs_assistant_functions = new DocsAssistantFunctions();
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4220 feat(aql): wire detectInten... (2026-03-14) | #3479 [Docs-Audit] src/aq
- Line 317: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::string DocsAssistantFunctions::extractTopicFromQuery(const std::string &query) {
- Line 344: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::string DocsAssistantFunctions::extractSearchQuery(const std::string &query) {
- Line 387: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::string DocsAssistantFunctions::docsQuery(const std::string &query) {
- Line 513: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: try {

            auto *assistant           = impl->getAssistant();

            metrics["base_assistant"] = assistant->getStats();

        } catch (...) {

            metrics["base_assistant"] = nullptr;

        }

    }
- Line 513: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {

### aql/aql_query_template_library.cpp
Total findings: 7

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3479 [Docs-Audit] src/aql: Fix s... (2026-03-12)
- Line 82: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (toLower(tmpl.name).find(lower_kw) != std::string::npos ||
- Line 83: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: toLower(tmpl.description).find(lower_kw) != std::string::npos) {
- Line 89: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (toLower(t).find(lower_kw) != std::string::npos) {
- Line 83: severity=MEDIUM; category=repeated_lookup
  Description: Repeated find() for same key: lower_kw
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (toLower(tmpl.name).find(lower_kw) != std::string::npos ||
- Line 115: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: const std::unordered_map<std::string, std::string>& parameters
- Line 129: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: const std::unordered_map<std::string, std::string>& parameters

### aql/aql_confidence_scorer.cpp
Total findings: 6

- Line 179: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: auto isWordChar = [](char c) -> bool { return std::isalnum(static_cast<unsigned char>(c)) || c == '_
- Line 234: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 3 > array size 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

    }



    // Solve 3×3 system via Cramer's rule (small fixed size; no external deps).

    auto det3 = [](const double M[3][3]) -> double {

        return M[0][0] * (M[1][1] * M[2][2] - M[1][2] * M[2][1]) - M[0][1] * (M[1][0] * M[2][2] - M[1][2] * M[2][0])

               + M[0][2] * (M[1][0] * M[2][1] - M[1][1] * M[2][0]);

    };



    double D = det3(XtX);

    if (std::abs(D) < 1e-12) {
- Line 235: severity=CRITICAL; category=array_bounds_violation
  Description: Array bounds violation: loop 3 > array size 0
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }



    // Solve 3×3 system via Cramer's rule (small fixed size; no external deps).

    auto det3 = [](const double M[3][3]) -> double {

        return M[0][0] * (M[1][1] * M[2][2] - M[1][2] * M[2][1]) - M[0][1] * (M[1][0] * M[2][2] - M[1][2] * M[2][0])

               + M[0][2] * (M[1][0] * M[2][1] - M[1][1] * M[2][0]);

    };



    double D = det3(XtX);

    if (std::abs(D) < 1e-12) {

        // Singular matrix: leave weights unchanged.
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4161 feat(aql): Runtime-configur... (2026-03-13) | #3139 feat(aql): Stream n
- Line 129: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: auto it = std::find_if(line.begin(), line.end(), [](unsigned char c) { return !std::isspace(c); });
- Line 203: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: xs.push_back(scoreStructure(lower));

### aql/aql_conversation_context.cpp
Total findings: 5

- Line 144: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 148: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 149: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 72: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: static std::string cleanQuery(const std::string &raw) {
- Line 324: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::string AQLConversationContext::lastQuery() const {

### aql/aql_query_validator.cpp
Total findings: 5

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4222 feat(aql): AQLQueryBuilder ... (2026-03-15) | #3479 [Docs-Audit] src/aq
- Line 112: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: vars.push_back((*it)[1].str());
- Line 124: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: vars.push_back((*it)[1].str());
- Line 338: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::string> var_to_collection;
- Line 356: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::vector<std::string>> collection_fields;

### aql/aql_autocomplete.cpp
Total findings: 4

- Line 283: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 293: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: vars.push_back((*it)[1].str());
- Line 304: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> seen;
- Line 510: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> seen;

### aql/llm_aql_embedding_bridge.cpp
Total findings: 4

- Line 18: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::vector<float> LLMAQLEmbeddingBridge::embed(const std::string& text) {
- Line 23: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return handler_.executeEmbed(text);
- Line 25: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: spdlog::debug("LLMAQLEmbeddingBridge::embed(): executeEmbed failed ({}); "
- Line 29: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: spdlog::debug("LLMAQLEmbeddingBridge::embed(): executeEmbed threw unknown exception; "

### aql/aql_model_router.cpp
Total findings: 2

- Line 142: severity=MEDIUM; category=uninitialized_member_field
  Description: Default constructor does not initialize POD members
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Struct with uninitialized fields
- Line 156: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: scores.push_back({rule.type, hits});

### aql/FUTURE_ENHANCEMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'FUTURE_ENHANCEMENTS.md' is missing expected cross-links: ARCHITECTURE.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### aql/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### aql/aql_ingestion_bridge.cpp
Total findings: 1

- Line 36: severity=CRITICAL; category=exception_in_destructor
  Description: Destructors must be noexcept; exceptions here cause std::terminate()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### aql/aql_optimizer_advisor.cpp
Total findings: 1

- Line 143: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: suggestions.push_back({ValidationIssue::Severity::INFO, msg, "INDEX"});

### aql/llm_metrics_collector.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #3479 [Docs-Audit] src/aql: Fix s... (2026-03-12) | #1262 Add production hard

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
