# prompt_engineering Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: prompt_engineering
- Generated: 2026-11-01 06:43:14
- Status: Critical Findings Present
- Total Findings: 96
- Actionable Findings (Critical + High): 54
- Affected Files: 29

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 2 |
| High | 52 |
| Medium | 37 |
| Low | 5 |

## Category Summary

| Category | Count |
|---|---:|
| pointer_arithmetic_unbounded | 25 |
| missing_resource_limits | 21 |
| unchecked_result | 15 |
| const_value_param_string | 12 |
| module_doc_linkset_drift | 5 |
| resource_leaked_in_exception | 3 |
| unvalidated_llm_output | 3 |
| generic_catch | 2 |
| missing_noexcept_on_move | 2 |
| passthrough_methods | 2 |
| prompt_injection | 2 |
| critical_function_noexcept | 1 |
| silent_error_swallow | 1 |
| simulation_stub_marker | 1 |
| uncaught_exception | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/prompt_engineering/reflection_tuner.cpp | 12 | 0 | 6 | 6 | 0 |
| src/prompt_engineering/prompt_evaluator.cpp | 10 | 0 | 8 | 2 | 0 |
| src/prompt_engineering/meta_prompt_generator.cpp | 9 | 0 | 7 | 2 | 0 |
| src/prompt_engineering/prompt_version_control.cpp | 9 | 0 | 7 | 2 | 0 |
| src/prompt_engineering/self_improvement_orchestrator.cpp | 7 | 2 | 5 | 0 | 0 |
| src/prompt_engineering/tree_of_thoughts.cpp | 7 | 0 | 0 | 7 | 0 |
| src/prompt_engineering/llm_reflection_adapter.cpp | 4 | 0 | 0 | 4 | 0 |
| src/prompt_engineering/prompt_ab_experiment.cpp | 4 | 0 | 3 | 1 | 0 |
| src/prompt_engineering/prompt_optimizer.cpp | 4 | 0 | 4 | 0 | 0 |
| src/prompt_engineering/dspy_module.cpp | 3 | 0 | 1 | 2 | 0 |
| src/prompt_engineering/prompt_injection_detector.cpp | 3 | 0 | 3 | 0 | 0 |
| src/prompt_engineering/system_prompt_manager.cpp | 3 | 0 | 0 | 3 | 0 |
| src/prompt_engineering/feedback_collector.cpp | 2 | 0 | 1 | 1 | 0 |
| src/prompt_engineering/prompt_library_io.cpp | 2 | 0 | 0 | 2 | 0 |
| src/prompt_engineering/prompt_manager.cpp | 2 | 0 | 1 | 1 | 0 |
| src/prompt_engineering/prompt_template_compiler.cpp | 2 | 0 | 1 | 1 | 0 |
| src/prompt_engineering/ARCHITECTURE.md | 1 | 0 | 0 | 0 | 1 |
| src/prompt_engineering/FUTURE_ENHANCEMENTS.md | 1 | 0 | 0 | 0 | 1 |
| src/prompt_engineering/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| src/prompt_engineering/README.md | 1 | 0 | 0 | 0 | 1 |
| src/prompt_engineering/ROADMAP.md | 1 | 0 | 0 | 0 | 1 |
| src/prompt_engineering/context_window_manager.cpp | 1 | 0 | 0 | 1 | 0 |
| src/prompt_engineering/prompt_engineering_integration.cpp | 1 | 0 | 0 | 1 | 0 |
| src/prompt_engineering/prompt_engineering_metrics.cpp | 1 | 0 | 0 | 1 | 0 |
| src/prompt_engineering/prompt_performance_tracker.cpp | 1 | 0 | 1 | 0 | 0 |
| src/prompt_engineering/prompt_regression_runner.cpp | 1 | 0 | 1 | 0 | 0 |
| src/prompt_engineering/rag_context_budget_manager.cpp | 1 | 0 | 1 | 0 | 0 |
| src/prompt_engineering/rewrite_engine.cpp | 1 | 0 | 1 | 0 | 0 |
| src/prompt_engineering/rewrite_metrics.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### src/prompt_engineering/reflection_tuner.cpp
Total findings: 12

- Line 558: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without visible bounds check
  Remediation: Add bounds check (index < size) before access
  Scanner: Uniform::phase1_memory_safety
  Context: score            = provider_->score(prompt, revised_response);



        // Store the critique prompt in metadata for observability.

        step.metadata["critique_prompt"] =

            prompt_builder_.buildCritiquePrompt(prompt, current_response, ctx);

    } else {

        // Fallback: template-based critique + heuristic score.
- Line 595: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without visible bounds check
  Remediation: Add bounds check (index < size) before access
  Scanner: Uniform::phase1_memory_safety
  Context: revised_response = current_response;  // cannot revise without an LLM

        score            = computeHeuristicScore(prompt, revised_response);



        step.metadata["critique_prompt"]  = critique_prompt;

        step.metadata["revision_prompt"]  =

            prompt_builder_.buildRevisionPrompt(prompt, current_response, critique, ctx);

    }
- Line 596: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without visible bounds check
  Remediation: Add bounds check (index < size) before access
  Scanner: Uniform::phase1_memory_safety
  Context: score            = computeHeuristicScore(prompt, revised_response);



        step.metadata["critique_prompt"]  = critique_prompt;

        step.metadata["revision_prompt"]  =

            prompt_builder_.buildRevisionPrompt(prompt, current_response, critique, ctx);

    }
- Line 643: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without visible bounds check
  Remediation: Add bounds check (index < size) before access
  Scanner: Uniform::phase1_memory_safety
  Context: ReflectionResult ReflectionTuner::tune(const std::string& prompt,

                                        const std::string& initial_response) {

    ReflectionResult result;

    result.metadata["strategy"]       = static_cast<int>(config_.strategy);

    result.metadata["max_iterations"] = config_.max_iterations;

    result.metadata["provider"]       =

        provider_ ? provider_->name() : std::string("fallback");
- Line 644: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without visible bounds check
  Remediation: Add bounds check (index < size) before access
  Scanner: Uniform::phase1_memory_safety
  Context: const std::string& initial_response) {

    ReflectionResult result;

    result.metadata["strategy"]       = static_cast<int>(config_.strategy);

    result.metadata["max_iterations"] = config_.max_iterations;

    result.metadata["provider"]       =

        provider_ ? provider_->name() : std::string("fallback");
- Line 645: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without visible bounds check
  Remediation: Add bounds check (index < size) before access
  Scanner: Uniform::phase1_memory_safety
  Context: ReflectionResult result;

    result.metadata["strategy"]       = static_cast<int>(config_.strategy);

    result.metadata["max_iterations"] = config_.max_iterations;

    result.metadata["provider"]       =

        provider_ ? provider_->name() : std::string("fallback");



    std::string current_response = initial_response;
- Line 93: severity=MEDIUM; category=const_value_param_string
  Description: Heavy read-only parameter passed by const value
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::const_correctness
  Context: const std::string lower = toLower(response);
- Line 369: severity=MEDIUM; category=const_value_param_string
  Description: Heavy read-only parameter passed by const value
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::const_correctness
  Context: const std::string lower_resp   = toLower(response);
- Line 370: severity=MEDIUM; category=const_value_param_string
  Description: Heavy read-only parameter passed by const value
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::const_correctness
  Context: const std::string lower_crit   = toLower(critique);
- Line 506: severity=MEDIUM; category=const_value_param_string
  Description: Heavy read-only parameter passed by const value
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::const_correctness
  Context: const std::string lower = toLower(response);
- Line 625: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: * @details Calls: generate(), tune().
- Line 629: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: provider_ ? provider_->generate(prompt) : prompt;

### src/prompt_engineering/prompt_evaluator.cpp
Total findings: 10

- Line 38: severity=HIGH; category=unchecked_result
  Description: Result/Error variable created but never checked
  Remediation: Check result or explicitly ignore with [[nodiscard]] cast
  Scanner: Uniform::phase1_ai_error_handling_consistency
  Context: const auto embedding_provider = getEmbeddingProviderSnapshot();
- Line 152: severity=HIGH; category=unchecked_result
  Description: Result/Error variable created but never checked
  Remediation: Check result or explicitly ignore with [[nodiscard]] cast
  Scanner: Uniform::phase1_ai_error_handling_consistency
  Context: auto norm1 = normalizeString(s1);
- Line 153: severity=HIGH; category=unchecked_result
  Description: Result/Error variable created but never checked
  Remediation: Check result or explicitly ignore with [[nodiscard]] cast
  Scanner: Uniform::phase1_ai_error_handling_consistency
  Context: auto norm2 = normalizeString(s2);
- Line 214: severity=HIGH; category=passthrough_methods
  Description: Passthrough method detected - delegates to wrapped object without transformation. Part of boring code anti-pattern.
  Remediation: Consider removing wrapper class and using wrapped object directly, or add meaningful transformation logic.
  Scanner: Uniform::wrapper_abstraction_excess
  Context: if (norm_output.empty() && norm_expected.empty()) {
- Line 240: severity=HIGH; category=unchecked_result
  Description: Result/Error variable created but never checked
  Remediation: Check result or explicitly ignore with [[nodiscard]] cast
  Scanner: Uniform::phase1_ai_error_handling_consistency
  Context: auto tokens_output = tokenize(normalizeString(output));
- Line 386: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without visible bounds check
  Remediation: Add bounds check (index < size) before access
  Scanner: Uniform::phase1_memory_safety
  Context: for (int m = 1; m <= MAX_ITER; ++m) {

            double m2 = 2.0 * m;

            // Even step

            double dm = m * (bb - m) * xx / ((qam + m2) * (aa + m2));

            d = 1.0 + dm * d;

            if (std::abs(d) < 1e-30) {

              d = 1e-30;
- Line 398: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without visible bounds check
  Remediation: Add bounds check (index < size) before access
  Scanner: Uniform::phase1_memory_safety
  Context: d = 1.0 / d;

            h *= d * c;

            // Odd step

            dm = -(aa + m) * (qab + m) * xx / ((aa + m2) * (qap + m2));

            d = 1.0 + dm * d;

            if (std::abs(d) < 1e-30) {

              d = 1e-30;
- Line 421: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception path may leak resource
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 599: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto v1 = provider->embed(s1);
- Line 600: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto v2 = provider->embed(s2);

### src/prompt_engineering/meta_prompt_generator.cpp
Total findings: 9

- Line 97: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string llm_response = llm_provider_->complete(result.meta_prompt);
- Line 100: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without visible bounds check
  Remediation: Add bounds check (index < size) before access
  Scanner: Uniform::phase1_memory_safety
  Context: std::string llm_response = llm_provider_->complete(result.meta_prompt);

            if (!llm_response.empty()) {

                result.improvement_suggestion = llm_response;

                result.metadata["llm_provider"] = llm_provider_->name();

                result.metadata["llm_generated"] = true;

                THEMIS_DEBUG("LLM provider returned {} chars",llm_response.size());

                // key_insights remain from the template-based path below
- Line 101: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without visible bounds check
  Remediation: Add bounds check (index < size) before access
  Scanner: Uniform::phase1_memory_safety
  Context: if (!llm_response.empty()) {

                result.improvement_suggestion = llm_response;

                result.metadata["llm_provider"] = llm_provider_->name();

                result.metadata["llm_generated"] = true;

                THEMIS_DEBUG("LLM provider returned {} chars",llm_response.size());

                // key_insights remain from the template-based path below

            } else {
- Line 144: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without visible bounds check
  Remediation: Add bounds check (index < size) before access
  Scanner: Uniform::phase1_memory_safety
  Context: result.key_insights.push_back("Minor optimizations possible");

    }

    

    result.metadata["score"] = score;

    result.metadata["strategy"] = config_.improvement_strategy;

    result.metadata["original_length"] = original_prompt.length();
- Line 145: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without visible bounds check
  Remediation: Add bounds check (index < size) before access
  Scanner: Uniform::phase1_memory_safety
  Context: }

    

    result.metadata["score"] = score;

    result.metadata["strategy"] = config_.improvement_strategy;

    result.metadata["original_length"] = original_prompt.length();

    

    THEMIS_DEBUG("Generated meta-prompt of length {}", result.meta_prompt.length());
- Line 146: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without visible bounds check
  Remediation: Add bounds check (index < size) before access
  Scanner: Uniform::phase1_memory_safety
  Context: result.metadata["score"] = score;

    result.metadata["strategy"] = config_.improvement_strategy;

    result.metadata["original_length"] = original_prompt.length();

    

    THEMIS_DEBUG("Generated meta-prompt of length {}", result.meta_prompt.length());
- Line 188: severity=HIGH; category=unchecked_result
  Description: Result/Error variable created but never checked
  Remediation: Check result or explicitly ignore with [[nodiscard]] cast
  Scanner: Uniform::phase1_ai_error_handling_consistency
  Context: auto structure = analyzePromptStructure(prompt);
- Line 95: severity=MEDIUM; category=passthrough_methods
  Description: Passthrough method detected - delegates to wrapped object without transformation. Part of boring code anti-pattern.
  Remediation: Consider removing wrapper class and using wrapped object directly, or add meaningful transformation logic.
  Scanner: Uniform::wrapper_abstraction_excess
  Context: THEMIS_DEBUG("Calling LLM provider '{}' for real-time improvement", llm_provider_->name());
- Line 97: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string llm_response = llm_provider_->complete(result.meta_prompt);

### src/prompt_engineering/prompt_version_control.cpp
Total findings: 9

- Line 586: severity=HIGH; category=unchecked_result
  Description: Result/Error variable created but never checked
  Remediation: Check result or explicitly ignore with [[nodiscard]] cast
  Scanner: Uniform::phase1_ai_error_handling_consistency
  Context: auto src_ancestors  = collect_ancestors(source_id);
- Line 1114: severity=HIGH; category=unchecked_result
  Description: Result/Error variable created but never checked
  Remediation: Check result or explicitly ignore with [[nodiscard]] cast
  Scanner: Uniform::phase1_ai_error_handling_consistency
  Context: const auto base_lines   = split_lines(base.content);
- Line 1115: severity=HIGH; category=unchecked_result
  Description: Result/Error variable created but never checked
  Remediation: Check result or explicitly ignore with [[nodiscard]] cast
  Scanner: Uniform::phase1_ai_error_handling_consistency
  Context: const auto source_lines = split_lines(source.content);
- Line 1116: severity=HIGH; category=unchecked_result
  Description: Result/Error variable created but never checked
  Remediation: Check result or explicitly ignore with [[nodiscard]] cast
  Scanner: Uniform::phase1_ai_error_handling_consistency
  Context: const auto target_lines = split_lines(target.content);
- Line 1151: severity=HIGH; category=unchecked_result
  Description: Result/Error variable created but never checked
  Remediation: Check result or explicitly ignore with [[nodiscard]] cast
  Scanner: Uniform::phase1_ai_error_handling_consistency
  Context: auto src_edits = lcs_edits(base_lines, source_lines);
- Line 1152: severity=HIGH; category=unchecked_result
  Description: Result/Error variable created but never checked
  Remediation: Check result or explicitly ignore with [[nodiscard]] cast
  Scanner: Uniform::phase1_ai_error_handling_consistency
  Context: auto tgt_edits = lcs_edits(base_lines, target_lines);
- Line 1258: severity=HIGH; category=missing_noexcept_on_move
  Description: Move ctors should be noexcept for strong container guarantees
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 548: severity=MEDIUM; category=const_value_param_string
  Description: Heavy read-only parameter passed by const value
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::const_correctness
  Context: const std::string source_id = prompt_branches.at(source_branch);
- Line 549: severity=MEDIUM; category=const_value_param_string
  Description: Heavy read-only parameter passed by const value
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::const_correctness
  Context: const std::string target_id = prompt_branches.at(target_branch);

### src/prompt_engineering/self_improvement_orchestrator.cpp
Total findings: 7

- Line 222: severity=CRITICAL; category=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: // between (prompt + input) and expected_output, which serves as a proxy for
- Line 241: severity=CRITICAL; category=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: prompt_with_inputs.push_back(prompt + "\n" + tc.input);
- Line 288: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without visible bounds check
  Remediation: Add bounds check (index < size) before access
  Scanner: Uniform::phase1_memory_safety
  Context: config_.ab_test_sample_size

                );

                

                result.metadata["ab_test_id"] = test_id;

                

                THEMIS_INFO("Optimization completed, starting A/B test: {}", test_id);

            } else {
- Line 308: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without visible bounds check
  Remediation: Add bounds check (index < size) before access
  Scanner: Uniform::phase1_memory_safety
  Context: } catch (const std::exception& e) {

        THEMIS_ERROR("Optimization failed: {}", e.what());

        result.status = OptimizationStatus::FAILED;

        result.metadata["error"] = e.what();

    }

    

    return finalizeResult(result.status == OptimizationStatus::NOT_STARTED
- Line 437: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without visible bounds check
  Remediation: Add bounds check (index < size) before access
  Scanner: Uniform::phase1_memory_safety
  Context: // Update optimization history

        for (auto& results : optimization_history_[test.prompt_id]) {

            if (results.metadata.contains("ab_test_id") && 

                results.metadata["ab_test_id"] == test_id) {

                results.status = OptimizationStatus::DEPLOYED;

            }

        }
- Line 448: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without visible bounds check
  Remediation: Add bounds check (index < size) before access
  Scanner: Uniform::phase1_memory_safety
  Context: // Update optimization history

        for (auto& results : optimization_history_[test.prompt_id]) {

            if (results.metadata.contains("ab_test_id") && 

                results.metadata["ab_test_id"] == test_id) {

                results.status = OptimizationStatus::COMPLETED;

            }

        }
- Line 685: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without visible bounds check
  Remediation: Add bounds check (index < size) before access
  Scanner: Uniform::phase1_memory_safety
  Context: updated_template.content = version;

    

    // Update metadata to track deployment

    updated_template.metadata["last_deployed"] = 

        std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    

    // Create new version (in production, this would use PromptVersionControl)

### src/prompt_engineering/tree_of_thoughts.cpp
Total findings: 7

- Line 33: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::vector<std::string> HeuristicThoughtGenerator::generate(
- Line 249: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto root_thoughts = generator->generate(problem, {}, config.branching_factor);
- Line 295: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto children = generator->generate(problem, child_path, config.branching_factor);
- Line 335: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto root_thoughts = generator->generate(problem, {}, config.branching_factor);
- Line 379: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto children = generator->generate(problem, child_path, config.branching_factor);
- Line 419: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto root_thoughts = generator->generate(problem, {}, config.branching_factor);
- Line 457: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto children = generator->generate(problem, child_path, config.branching_factor);

### src/prompt_engineering/llm_reflection_adapter.cpp
Total findings: 4

- Line 31: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string ILLMProviderReflectionAdapter::generate(
- Line 34: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return llm_->complete(prompt);
- Line 44: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return llm_->complete(critique_prompt);
- Line 57: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: return llm_->complete(revision_prompt);

### src/prompt_engineering/prompt_ab_experiment.cpp
Total findings: 4

- Line 233: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without visible bounds check
  Remediation: Add bounds check (index < size) before access
  Scanner: Uniform::phase1_memory_safety
  Context: const double dm  = static_cast<double>(m);

        // Even step.

        double num = dm * (b - dm) * x /

                     ((a + 2.0 * dm - 1.0) * (a + 2.0 * dm));

        D = 1.0 + num * D;

        if (std::abs(D) < kEps) { D = kEps; }

        C = 1.0 + num / C;
- Line 241: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without visible bounds check
  Remediation: Add bounds check (index < size) before access
  Scanner: Uniform::phase1_memory_safety
  Context: D = 1.0 / D;

        f *= C * D;

        // Odd step.

        num = -(a + dm) * (a + b + dm) * x /

              ((a + 2.0 * dm) * (a + 2.0 * dm + 1.0));

        D = 1.0 + num * D;

        if (std::abs(D) < kEps) { D = kEps; }
- Line 242: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without visible bounds check
  Remediation: Add bounds check (index < size) before access
  Scanner: Uniform::phase1_memory_safety
  Context: f *= C * D;

        // Odd step.

        num = -(a + dm) * (a + b + dm) * x /

              ((a + 2.0 * dm) * (a + 2.0 * dm + 1.0));

        D = 1.0 + num * D;

        if (std::abs(D) < kEps) { D = kEps; }

        C = 1.0 + num / C;
- Line 109: severity=MEDIUM; category=const_value_param_string
  Description: Heavy read-only parameter passed by const value
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::const_correctness
  Context: const std::string st   = j.value("status", std::string{"running"});

### src/prompt_engineering/prompt_optimizer.cpp
Total findings: 4

- Line 116: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without visible bounds check
  Remediation: Add bounds check (index < size) before access
  Scanner: Uniform::phase1_memory_safety
  Context: result.final_score = current_score;

    result.converged = result.converged || (current_score >= config_.target_score);

    

    result.metadata["initial_score"] = result.score_history[0];

    result.metadata["improvement"] = current_score - result.score_history[0];

    result.metadata["relative_improvement"] = 

        (result.score_history[0] > 0) ?
- Line 117: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without visible bounds check
  Remediation: Add bounds check (index < size) before access
  Scanner: Uniform::phase1_memory_safety
  Context: result.converged = result.converged || (current_score >= config_.target_score);

    

    result.metadata["initial_score"] = result.score_history[0];

    result.metadata["improvement"] = current_score - result.score_history[0];

    result.metadata["relative_improvement"] = 

        (result.score_history[0] > 0) ? 

        (current_score - result.score_history[0]) / result.score_history[0] : 0.0;
- Line 118: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without visible bounds check
  Remediation: Add bounds check (index < size) before access
  Scanner: Uniform::phase1_memory_safety
  Context: result.metadata["initial_score"] = result.score_history[0];

    result.metadata["improvement"] = current_score - result.score_history[0];

    result.metadata["relative_improvement"] = 

        (result.score_history[0] > 0) ? 

        (current_score - result.score_history[0]) / result.score_history[0] : 0.0;
- Line 181: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: auto meta_result = meta_gen.generateImprovementPrompt(

### src/prompt_engineering/dspy_module.cpp
Total findings: 3

- Line 276: severity=HIGH; category=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string response = llm_provider_->complete(prompt);
- Line 184: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string EchoDspyLLMProvider::complete(const std::string& prompt)
- Line 276: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: std::string response = llm_provider_->complete(prompt);

### src/prompt_engineering/prompt_injection_detector.cpp
Total findings: 3

- Line 196: severity=HIGH; category=unchecked_result
  Description: Result/Error variable created but never checked
  Remediation: Check result or explicitly ignore with [[nodiscard]] cast
  Scanner: Uniform::phase1_ai_error_handling_consistency
  Context: DetectionResult result = detect(response);
- Line 205: severity=HIGH; category=simulation_stub_marker
  Description: Simulation/stub marker found: Fake implementation prefix
  Remediation: Remove stub/mock code or move to test/example directory; replace with real implementation
  Scanner: Uniform::phase1_ai_simulation_stub_leak
  Context: "response:fake_system_prefix"},
- Line 210: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception path may leak resource
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### src/prompt_engineering/system_prompt_manager.cpp
Total findings: 3

- Line 133: severity=MEDIUM; category=const_value_param_string
  Description: Heavy read-only parameter passed by const value
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::const_correctness
  Context: const std::string key = roleToString(role);
- Line 147: severity=MEDIUM; category=const_value_param_string
  Description: Heavy read-only parameter passed by const value
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::const_correctness
  Context: const std::string key = roleToString(role);
- Line 179: severity=MEDIUM; category=const_value_param_string
  Description: Heavy read-only parameter passed by const value
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::const_correctness
  Context: const std::string key = roleToString(role);

### src/prompt_engineering/feedback_collector.cpp
Total findings: 2

- Line 246: severity=HIGH; category=missing_noexcept_on_move
  Description: Move ctors should be noexcept for strong container guarantees
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 251: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: summary.reason_embedding    = embedding_model_->embed(query);

### src/prompt_engineering/prompt_library_io.cpp
Total findings: 2

- Line 90: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Scanner: Uniform::phase1_error_handling
  Context: try {

            t.metadata = nlohmann::json::parse(

                node["metadata"].as<std::string>());

        } catch (...) {

            t.metadata = nlohmann::json::object();

        }

    }
- Line 290: severity=MEDIUM; category=const_value_param_string
  Description: Heavy read-only parameter passed by const value
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::const_correctness
  Context: const std::string ext = p.extension().string();

### src/prompt_engineering/prompt_manager.cpp
Total findings: 2

- Line 255: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without visible bounds check
  Remediation: Add bounds check (index < size) before access
  Scanner: Uniform::phase1_memory_safety
  Context: return false;

    }

    

    acc->second.metadata["experiment_id"] = experiment_id;



    if (db_) {

        std::string key = std::string(KEY_PREFIX) + id;
- Line 327: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Scanner: Uniform::phase1_error_handling
  Context: YAML::Emitter emitter;

                    emitter << prompt_node["metadata"];

                    pt.metadata = nlohmann::json::parse(emitter.c_str());

                } catch (...) {

                    pt.metadata = nlohmann::json::object();

                }

            }

### src/prompt_engineering/prompt_template_compiler.cpp
Total findings: 2

- Line 548: severity=HIGH; category=unchecked_result
  Description: Result/Error variable created but never checked
  Remediation: Check result or explicitly ignore with [[nodiscard]] cast
  Scanner: Uniform::phase1_ai_error_handling_consistency
  Context: auto ast = parse(tokens, idx,  slot_index,
- Line 511: severity=MEDIUM; category=critical_function_noexcept
  Description: Critical function marked noexcept; may mask error handling
  Remediation: Remove noexcept if function can fail; document error behavior
  Scanner: Uniform::phase1_ai_error_handling_consistency
  Context: CompiledPromptTemplate::validate(const PromptContext& ctx) const noexcept {

### src/prompt_engineering/ARCHITECTURE.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'ARCHITECTURE.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### src/prompt_engineering/FUTURE_ENHANCEMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'FUTURE_ENHANCEMENTS.md' is missing expected cross-links: ARCHITECTURE.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### src/prompt_engineering/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### src/prompt_engineering/README.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'README.md' is missing expected cross-links: ARCHITECTURE.md, FUTURE_ENHANCEMENTS.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### src/prompt_engineering/ROADMAP.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'ROADMAP.md' is missing expected cross-links: ARCHITECTURE.md, FUTURE_ENHANCEMENTS.md, README.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### src/prompt_engineering/context_window_manager.cpp
Total findings: 1

- Line 222: severity=MEDIUM; category=const_value_param_string
  Description: Heavy read-only parameter passed by const value
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::const_correctness
  Context: const std::string full_text = t->description.empty() ? t->name : t->description;

### src/prompt_engineering/prompt_engineering_integration.cpp
Total findings: 1

- Line 526: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: snap_metrics->recordReflectionCycleComplete(

### src/prompt_engineering/prompt_engineering_metrics.cpp
Total findings: 1

- Line 1057: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: void PromptEngineeringMetrics::recordReflectionCycleComplete(

### src/prompt_engineering/prompt_performance_tracker.cpp
Total findings: 1

- Line 113: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception path may leak resource
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### src/prompt_engineering/prompt_regression_runner.cpp
Total findings: 1

- Line 286: severity=HIGH; category=silent_error_swallow
  Description: catch(...) without re-throw; error is swallowed
  Remediation: Either re-throw or handle explicitly; never silently ignore exceptions
  Scanner: Uniform::phase1_ai_error_handling_consistency
  Context: } catch (...) {

### src/prompt_engineering/rag_context_budget_manager.cpp
Total findings: 1

- Line 41: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Scanner: Uniform::phase1_error_handling
  Context: ? (total_budget_ - current_allocated)

                             : 0;

    if (tokens > avail) {

        throw BudgetExhaustedError(tokens, avail, total_budget_);

    }



    allocated_.fetch_add(tokens, std::memory_order_relaxed);

### src/prompt_engineering/rewrite_engine.cpp
Total findings: 1

- Line 226: severity=HIGH; category=unchecked_result
  Description: Result/Error variable created but never checked
  Remediation: Check result or explicitly ignore with [[nodiscard]] cast
  Scanner: Uniform::phase1_ai_error_handling_consistency
  Context: auto phase_rules = get_rules_for_phase(phase);

### src/prompt_engineering/rewrite_metrics.cpp
Total findings: 1

- Line 107: severity=HIGH; category=unchecked_result
  Description: Result/Error variable created but never checked
  Remediation: Check result or explicitly ignore with [[nodiscard]] cast
  Scanner: Uniform::phase1_ai_error_handling_consistency
  Context: auto metrics = get_phase_metrics(phase);

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
