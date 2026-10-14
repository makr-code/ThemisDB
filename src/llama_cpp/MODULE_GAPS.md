# llama_cpp Module - Current Governance Status

This module currently satisfies the required governance and module-doc baseline.
The previously reported missing `PRODUCTION_REQUIREMENTS.md` issue has been resolved,
and the active docs are aligned with the live implementation in `src/llama_cpp/` and
`include/llama_cpp/`.

## Current status

- Module: `llama_cpp`
- Governance doc: `src/llama_cpp/PRODUCTION_REQUIREMENTS.md` — present and source-aligned
- Primary docs: `README.md`, `ARCHITECTURE.md`, `ROADMAP.md`, `FUTURE_ENHANCEMENTS.md`, `SECURITY.md`
- Validation: direct Doxygen and module-readability checks remain green for the current code path

## Active findings

- None at the governance/doc drift level for the module surface under review.
- Remaining implementation-specific follow-ups, if any, belong in the live code and test suite rather than in a stale missing-doc report.
- Line 384: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (rag_mode == "tensor_hybrid" || rag_mode == "tensor_prefix") {

        int rag_tensor_slots = 6;

        int rag_tensor_slot_chars = 280;

        if (request.metadata.is_object()) {

            const auto slots_it = request.metadata.find("rag_tensor_slots");

            if (slots_it != request.metadata.end() && slots_it->is_number_integer()) {

                rag_tensor_slots = slots_it->get<int>();
- Line 385: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: int rag_tensor_slots = 6;

        int rag_tensor_slot_chars = 280;

        if (request.metadata.is_object()) {

            const auto slots_it = request.metadata.find("rag_tensor_slots");

            if (slots_it != request.metadata.end() && slots_it->is_number_integer()) {

                rag_tensor_slots = slots_it->get<int>();

            }
- Line 386: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: int rag_tensor_slot_chars = 280;

        if (request.metadata.is_object()) {

            const auto slots_it = request.metadata.find("rag_tensor_slots");

            if (slots_it != request.metadata.end() && slots_it->is_number_integer()) {

                rag_tensor_slots = slots_it->get<int>();

            }

            const auto chars_it = request.metadata.find("rag_tensor_slot_chars");
- Line 389: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: if (slots_it != request.metadata.end() && slots_it->is_number_integer()) {

                rag_tensor_slots = slots_it->get<int>();

            }

            const auto chars_it = request.metadata.find("rag_tensor_slot_chars");

            if (chars_it != request.metadata.end() && chars_it->is_number_integer()) {

                rag_tensor_slot_chars = chars_it->get<int>();

            }
- Line 390: severity=HIGH; category=no_retry_logic
  Description: RPC/network call without retry logic — transient failures will propagate
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: rag_tensor_slots = slots_it->get<int>();

            }

            const auto chars_it = request.metadata.find("rag_tensor_slot_chars");

            if (chars_it != request.metadata.end() && chars_it->is_number_integer()) {

                rag_tensor_slot_chars = chars_it->get<int>();

            }

        }
- Line 537: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: stats["model_vram_required_mb"] = info->vram_required_mb;

            stats["model_ram_required_mb"] = info->ram_required_mb;

            if (info->metadata.contains("runtime_gpu_offload_requested")) {

                stats["runtime_gpu_offload_requested"] = info->metadata["runtime_gpu_offload_requested"];

            }

            if (info->metadata.contains("runtime_gpu_offload_effective")) {

                stats["runtime_gpu_offload_effective"] = info->metadata["runtime_gpu_offload_effective"];
- Line 540: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: stats["runtime_gpu_offload_requested"] = info->metadata["runtime_gpu_offload_requested"];

            }

            if (info->metadata.contains("runtime_gpu_offload_effective")) {

                stats["runtime_gpu_offload_effective"] = info->metadata["runtime_gpu_offload_effective"];

            }

            if (info->metadata.contains("runtime_llama_assigned_cpu_tensors")) {

                stats["runtime_llama_assigned_cpu_tensors"] = info->metadata["runtime_llama_assigned_cpu_tensors"];
- Line 543: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: stats["runtime_gpu_offload_effective"] = info->metadata["runtime_gpu_offload_effective"];

            }

            if (info->metadata.contains("runtime_llama_assigned_cpu_tensors")) {

                stats["runtime_llama_assigned_cpu_tensors"] = info->metadata["runtime_llama_assigned_cpu_tensors"];

            }

            if (info->metadata.contains("runtime_llama_assigned_non_cpu_tensors")) {

                stats["runtime_llama_assigned_non_cpu_tensors"] = info->metadata["runtime_llama_assigned_non_cpu_tensors"];
- Line 546: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: stats["runtime_llama_assigned_cpu_tensors"] = info->metadata["runtime_llama_assigned_cpu_tensors"];

            }

            if (info->metadata.contains("runtime_llama_assigned_non_cpu_tensors")) {

                stats["runtime_llama_assigned_non_cpu_tensors"] = info->metadata["runtime_llama_assigned_non_cpu_tensors"];

            }

            if (info->metadata.contains("runtime_llama_backend_cpu_only_hint")) {

                stats["runtime_llama_backend_cpu_only_hint"] = info->metadata["runtime_llama_backend_cpu_only_hint"];
- Line 549: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: stats["runtime_llama_assigned_non_cpu_tensors"] = info->metadata["runtime_llama_assigned_non_cpu_tensors"];

            }

            if (info->metadata.contains("runtime_llama_backend_cpu_only_hint")) {

                stats["runtime_llama_backend_cpu_only_hint"] = info->metadata["runtime_llama_backend_cpu_only_hint"];

            }

        }

    }
- Line 726: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::memory
  Context: delete p;
- Line 726: severity=HIGH; category=delete_without_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: extern "C" THEMIS_PLUGIN_EXPORT

void themis_llm_destroy(themis::llm::ILLMPlugin* p) {

    delete p;

}

#endif
- Line 726: severity=HIGH; category=explicit_delete
  Description: Explicit delete statement (prefer smart pointers)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_raii
  Context: delete p;
- Line 287: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section '6; AI_ML_IMPACT_ASSESSMENT' that was not found in 'src/llama_cpp/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: FUTURE_ENHANCEMENTS.md §6; AI_ML_IMPACT_ASSESSMENT.md §7 Gap 1.
- Line 421: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: compact_prompt << "[/MEMORY_SLOT]\n\n";
- Line 491: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'LlamaCppPlugin Embed.' that was not found in 'src/llama_cpp/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/llama_cpp/FUTURE_ENHANCEMENTS.md §LlamaCppPlugin Embed.
- Line 726: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: delete p;

### llama_cpp/tests/test_llama_cpp_plugin.cpp
Total findings: 11

- Line 577: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: th.join();
- Line 614: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: th.join();
- Line 656: severity=CRITICAL; category=thread_join_no_timeout
  Description: Thread join/wait without timeout (blocking indefinitely)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: th.join();
- Line 549: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: constexpr int kThreads  = 8;
- Line 550: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: constexpr int kPerThread = 10;
- Line 573: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int t = 0; t < kThreads; ++t) {
- Line 586: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: constexpr int kThreads    = 4;
- Line 610: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int t = 0; t < kThreads; ++t) {
- Line 622: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: constexpr int kThreads = 6;
- Line 648: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: for (int t = 0; t < kThreads; ++t) {
- Line 671: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: bool called = false;

### llama_cpp/llama_cpp_registrar.cpp
Total findings: 2

- Line 59: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: return [](LlamaCppPlugin& plugin, const json& config) -> bool {
- Line 79: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'LlamaCpp Plugin Model Reload' that was not found in 'src/llm/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/llm/FUTURE_ENHANCEMENTS.md §"LlamaCpp Plugin Model Reload"

### llama_cpp
Total findings: 1

- Line 1: severity=MEDIUM; category=missing_module_doc
  Description: Module 'llama_cpp' missing required governance doc 'PRODUCTION_REQUIREMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_module_governance_rules
  Context: Expected file: src/llama_cpp/PRODUCTION_REQUIREMENTS.md

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
