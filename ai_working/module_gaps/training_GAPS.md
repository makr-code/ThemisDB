# training Module - Developer Gap Note

> Auto-generated from ai_working\gap_scan_results.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: training
- Generated: 2026-06-04 08:50:22
- Status: Critical Findings Present
- Total Findings: 171
- Actionable Findings (Critical + High): 80
- Affected Files: 18

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 7 |
| High | 73 |
| Medium | 68 |
| Low | 23 |

## Category Summary

| Category | Count |
|---|---:|
| string_concat_loop | 33 |
| hardcoded_output | 21 |
| resource_leaked_in_exception | 18 |
| db_connection_leak | 12 |
| uninitialized_access | 10 |
| copy_overhead | 9 |
| pointer_arithmetic_unbounded | 8 |
| missing_latency_metric | 7 |
| null_dereference | 7 |
| unordered_container_iter | 5 |
| fp_exact_comparison | 4 |
| o_n_squared | 4 |
| range_temporary | 4 |
| hardcoded_path | 3 |
| model_integrity_gap | 3 |
| no_timeout | 3 |
| stale_doc_section_reference | 3 |
| deadlock_risk | 2 |
| generic_catch | 2 |
| legacy_or_compat_path | 2 |
| module_doc_linkset_drift | 2 |
| uncaught_exception | 2 |
| uninitialized_member_field | 2 |
| data_race | 1 |
| lock_contention | 1 |
| lock_in_loop | 1 |
| missing_resource_limits | 1 |
| primitive_no_volatile | 1 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| training/incremental_lora_trainer.cpp | 33 | 4 | 21 | 5 | 3 |
| training/auto_labeler.cpp | 23 | 0 | 6 | 17 | 0 |
| training/ada_lora_adapter.cpp | 21 | 0 | 21 | 0 | 0 |
| training/lora_data_selection.cpp | 21 | 0 | 5 | 16 | 0 |
| training/examples/database_optimizer_labeler.cpp | 19 | 0 | 0 | 2 | 17 |
| training/provenance_tracker.cpp | 12 | 2 | 1 | 9 | 0 |
| training/database_domain_auto_labeler.cpp | 7 | 0 | 1 | 6 | 0 |
| training/adalora_tt_bridge.cpp | 5 | 0 | 4 | 1 | 0 |
| training/lora_adapter.cpp | 5 | 0 | 5 | 0 | 0 |
| training/knowledge_graph_enricher.cpp | 4 | 0 | 1 | 3 | 0 |
| training/lora_adapter_merger.cpp | 4 | 0 | 2 | 2 | 0 |
| training/modality_parser.cpp | 4 | 0 | 0 | 4 | 0 |
| training/training_pipeline.cpp | 4 | 1 | 1 | 2 | 0 |
| training/lora_checkpoint_manager.cpp | 3 | 0 | 3 | 0 | 0 |
| training/multi_task_lora.cpp | 3 | 0 | 1 | 1 | 1 |
| training/FUTURE_ENHANCEMENTS.md | 1 | 0 | 0 | 0 | 1 |
| training/PRODUCTION_REQUIREMENTS.md | 1 | 0 | 0 | 0 | 1 |
| training/adapter_serving.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### training/incremental_lora_trainer.cpp
Total findings: 33

- Line 406: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: if (!loadCheckpointWeights(checkpoint_path, &weight_load_error)) {
- Line 779: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: *error_reason = "unable to hash checkpoint payload: " + weights_path;
- Line 1328: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: float avg_loss = multi_gpu_trainer_->train_step(*multi_gpu_layer_,
- Line 1410: severity=CRITICAL; category=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: bool loadCheckpointWeights(const std::string& checkpoint_prefix,
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4519 [WIP] Update developer docu... (2026-04-12) | #3733 feat(training): imp
- Line 679: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 690: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // applied; unknown layer names are silently ignored for forward-compatibility.
- Line 755: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: entry.step == step) {
- Line 817: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lk(router_mutex_);
- Line 859: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::concurrency
  Context: std::lock_guard<std::mutex> lk(router_mutex_);
- Line 979: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: if (multi_gpu_ctx_->num_gpus() < 2) {
- Line 984: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: std::to_string(multi_gpu_ctx_->num_gpus()) +
- Line 1005: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: multi_gpu_ctx_->num_gpus());
- Line 1159: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: std::vector<float> input_vec, target_vec;

            if (!training_data.empty()) {

                size_t idx = (batch_offset + b) % training_data.size();

                input_vec  = encodeSample(training_data[idx].first,  feature_dim);

                target_vec = encodeSample(training_data[idx].second, feature_dim);

            } else {

                // Synthetic deterministic batch: varied across steps and batch positions
- Line 1160: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (!training_data.empty()) {

                size_t idx = (batch_offset + b) % training_data.size();

                input_vec  = encodeSample(training_data[idx].first,  feature_dim);

                target_vec = encodeSample(training_data[idx].second, feature_dim);

            } else {

                // Synthetic deterministic batch: varied across steps and batch positions

                const auto seed_in = static_cast<std::uint64_t>(
- Line 1247: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: std::vector<float> input_vec, target_vec;

            if (!training_data.empty()) {

                size_t idx = (batch_offset + b) % training_data.size();

                input_vec  = encodeSample(training_data[idx].first,  feature_dim);

                target_vec = encodeSample(training_data[idx].second, feature_dim);

            } else {

                std::mt19937 gen_in(step_idx * kSyntheticSeedBase + b * kSyntheticBatchMultiplier);
- Line 1248: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (!training_data.empty()) {

                size_t idx = (batch_offset + b) % training_data.size();

                input_vec  = encodeSample(training_data[idx].first,  feature_dim);

                target_vec = encodeSample(training_data[idx].second, feature_dim);

            } else {

                std::mt19937 gen_in(step_idx * kSyntheticSeedBase + b * kSyntheticBatchMultiplier);

                std::mt19937 gen_tg(step_idx * kSyntheticSeedBase + b * kSyntheticBatchMultiplier + 1u);
- Line 1259: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: for (auto& v : target_vec) v = d(gen_tg);

            }

            for (size_t fd = 0; fd < feature_dim; ++fd) {

                input_data [b * feature_dim + fd] = input_vec[fd];

                target_data[b * feature_dim + fd] = target_vec[fd];

            }

        }
- Line 1260: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: }

            for (size_t fd = 0; fd < feature_dim; ++fd) {

                input_data [b * feature_dim + fd] = input_vec[fd];

                target_data[b * feature_dim + fd] = target_vec[fd];

            }

        }
- Line 1283: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: const int n_gpus = multi_gpu_ctx_->num_gpus();
- Line 1294: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: std::vector<float> in_vec, tg_vec;

            if (!training_data.empty()) {

                size_t idx = (batch_offset + b) % training_data.size();

                in_vec = encodeSample(training_data[idx].first,  feature_dim);

                tg_vec = encodeSample(training_data[idx].second, feature_dim);

            } else {

                std::mt19937 gen_in(step_idx * kSyntheticSeedBase + b * kSyntheticBatchMultiplier);
- Line 1295: severity=HIGH; category=pointer_arithmetic_unbounded
  Description: Pointer/array access without bounds validation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_memory_safety
  Context: if (!training_data.empty()) {

                size_t idx = (batch_offset + b) % training_data.size();

                in_vec = encodeSample(training_data[idx].first,  feature_dim);

                tg_vec = encodeSample(training_data[idx].second, feature_dim);

            } else {

                std::mt19937 gen_in(step_idx * kSyntheticSeedBase + b * kSyntheticBatchMultiplier);

                std::mt19937 gen_tg(step_idx * kSyntheticSeedBase + b * kSyntheticBatchMultiplier + 1u);
- Line 1314: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: llm::lora::Device dev = multi_gpu_ctx_->get_device(g);
- Line 1526: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: return multi_gpu_ctx_->num_gpus();
- Line 1632: severity=HIGH; category=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::legacy_duplication
  Context: // Try path as a metadata file directly (legacy format)
- Line 591: severity=MEDIUM; category=primitive_no_volatile
  Description: Primitive shared across threads without volatile
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_thread_safety
  Context: float cumulative = 0.0f;
- Line 1033: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: gpu_training_    = true;

                lora_initialized_ = true;

                return;

            } catch (...) {

                // GPU init failed – fall through to CPU path

                gpu_lora_layer_.reset();

                gpu_optimizer_.reset();
- Line 1033: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 1051: severity=MEDIUM; category=generic_catch
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::phase1_error_handling
  Context: gpu_training_    = true;

                lora_initialized_ = true;

                return;

            } catch (...) {

                // GPU init failed – fall through to CPU path

                gpu_lora_layer_.reset();

                gpu_optimizer_.reset();
- Line 1051: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: } catch (...) {
- Line 1312: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::vector<llm::lora::GPUTensor> gpu_inputs, gpu_targets;
- Line 1323: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: gpu_inputs.push_back(std::move(in_t));
- Line 1329: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: gpu_inputs,

### training/auto_labeler.cpp
Total findings: 23

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #4833 Continue Phase-6 tensorgrap... (2026-05-07) | #4519 [WIP] Update develo
- Line 428: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entity : fetchAllDocumentsDirect()) {
- Line 461: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entity : fetchAllDocumentsDirect()) {
- Line 541: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::security
  Context: doc_ptr->contains("text") && (*doc_ptr)["text"].is_string()) {
- Line 546: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (const auto& entity : fetchAllDocumentsDirect()) {
- Line 679: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: while ((pos = query.find(token, pos)) != std::string::npos) {
- Line 293: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: LabelingStats labelQuery(const std::string& aql_query, LabelingCallback callback) {
- Line 384: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::string getFetchAllQuery() const {
- Line 389: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::string getBatchInsertQuery() const {
- Line 431: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: ids.push_back(entity.getPrimaryKey());
- Line 468: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: ids.push_back(entity.getPrimaryKey());
- Line 476: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: std::vector<std::string> executeAqlQuery(const std::string& aql) const {
- Line 487: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: ids.push_back(item.get<std::string>());
- Line 490: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: ids.push_back(item["pk"].get<std::string>());
- Line 492: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: ids.push_back(item["_key"].get<std::string>());
- Line 499: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: ids.push_back(item.get<std::string>());
- Line 518: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: case '\\': safe_id += "\\\\"; break;
- Line 519: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\\': safe_id += "\\\\"; break;
- Line 520: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '"':  safe_id += "\\\""; break;
- Line 521: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\n': safe_id += "\\n";  break;
- Line 522: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\r': safe_id += "\\r";  break;
- Line 523: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\t': safe_id += "\\t";  break;
- Line 688: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: static bool isReadOnlyAqlQuery(const std::string& aql) {

### training/ada_lora_adapter.cpp
Total findings: 21

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #5082 [Docs][training] Update mod... (2026-05-13) | #4405 [WIP] Add AdaLoRA i
- Line 177: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ReallocResult reallocateRanks(size_t total_budget) {
- Line 194: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 195: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 196: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 197: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 198: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 207: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: size_t allocated = 0;
- Line 216: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: allocated += raw;
- Line 221: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (allocated != total_budget && !order.empty()) {
- Line 231: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: if (allocated > total_budget) {
- Line 232: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: size_t excess = allocated - total_budget;
- Line 236: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: size_t deficit = total_budget - allocated;
- Line 245: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 246: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 247: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 248: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 421: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ReallocResult AdaLoRAAdapter::reallocateRanks(size_t total_budget) {
- Line 422: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return impl_->reallocateRanks(total_budget);
- Line 425: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: ReallocResult AdaLoRAAdapter::reallocateRanks() {
- Line 426: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: return impl_->reallocateRanks(impl_->rankBudget());

### training/lora_data_selection.cpp
Total findings: 21

- Line 114: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: if (lower.find(p) != std::string::npos) return true;
- Line 209: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: while ((pos = lower.find(lkw, pos)) != std::string::npos) {
- Line 439: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 449: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 1070: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 138: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_set<std::string> shingles;
- Line 243: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<char, int> freq;
- Line 406: severity=MEDIUM; category=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::llm_ai_safety
  Context: for (const auto& s : samples) embeddings.push_back(embed(s.text));
- Line 929: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: if      (c == '"')  out += "\\\"";
- Line 930: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if      (c == '"')  out += "\\\"";
- Line 931: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (c == '\\') out += "\\\\";
- Line 932: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (c == '\n') out += "\\n";
- Line 933: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (c == '\r') out += "\\r";
- Line 934: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (c == '\t') out += "\\t";
- Line 935: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: else if (c < 0x20)  out += "\\u00" + std::string(1, "0123456789abcdef"[c >> 4])
- Line 951: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (i > 0) ids_arr += ',';
- Line 952: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: ids_arr += '"';
- Line 954: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: ids_arr += '"';
- Line 962: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: if (!first_domain) domain_obj += ',';
- Line 964: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: domain_obj += '"';
- Line 966: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: domain_obj += "\":";

### training/examples/database_optimizer_labeler.cpp
Total findings: 19

- Line 53: severity=MEDIUM; category=uninitialized_member_field
  Description: Default constructor does not initialize POD members
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Struct with uninitialized fields
- Line 190: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: << "\nSee docs/issues/lora_loops/ for implementation specs.\n";
- Line 88: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "=== DATABASE_OPTIMIZER Labeler Example (IMPL-A1 + IMPL-A3) ===\n\n";
- Line 93: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Step 1: Labeling optimizer-log entries\n";
- Line 107: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  query: \"" << entry.query_text.substr(0, 40) << "...\"\n"
- Line 117: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "\n  Accepted " << labeled_samples.size()
- Line 124: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Step 2: Applying LoRADataSelectionPipeline quality filters\n";
- Line 132: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  After dedup + confidence filter: " << filtered.size() << " samples\n\n";
- Line 134: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  [PLANNED — LoRADataSelectionPipeline not yet wired for DATABASE_OPTIMIZER domain]\n\n";
- Line 139: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Step 3: Incremental LoRA training (Loop 4)\n";
- Line 150: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  Training complete — adapter version: " << trainer.activeVersion() << "\n\n";
- Line 152: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  [PLANNED — full training cycle requires IMPL-A1 domain wiring]\n\n";
- Line 157: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Step 4: Export gradient for federated aggregation (IMPL-A3)\n";
- Line 166: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  Exported " << grad.blob.size() << " bytes (AES-256-GCM encrypted)\n\n";
- Line 168: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  [PLANNED — exportGradient() to be implemented in IMPL-A3]\n\n";
- Line 173: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "Step 5: Apply GlobalAdapterDelta from federation coordinator (IMPL-A3)\n";
- Line 178: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  Applied global FedAvg delta — adapter weights updated.\n\n";
- Line 180: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "  [PLANNED — applyGlobalDelta() to be implemented in IMPL-A3]\n\n";
- Line 185: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: std::cout << "=== Summary ===\n"

### training/provenance_tracker.cpp
Total findings: 12

- Line 391: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: ProvenanceWriteStats ProvenanceTracker::write(const std::vector<ProvenanceRecord>& records) {
- Line 392: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: return impl_->write(records);
- Line 354: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: while ((pos = query.find(token, pos)) != std::string::npos) {
- Line 258: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Provenance Graph Integration' that was not found in 'src/training/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/training/FUTURE_ENHANCEMENTS.md §"Provenance Graph Integration"
- Line 259: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'Stub Lifecycle' that was not found in 'src/training/ROADMAP.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: ROADMAP.md §Stub Lifecycle
- Line 342: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, ProvenanceRecord> store_;
- Line 368: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: case '\\': out += "\\\\"; break;
- Line 369: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\\': out += "\\\\"; break;
- Line 370: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '"':  out += "\\\""; break;
- Line 371: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\n': out += "\\n";  break;
- Line 372: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\r': out += "\\r";  break;
- Line 373: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\t': out += "\\t";  break;

### training/database_domain_auto_labeler.cpp
Total findings: 7

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #5082 [Docs][training] Update mod... (2026-05-13)
- Line 178: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: case '"':  out += "\\\""; break;
- Line 179: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '"':  out += "\\\""; break;
- Line 180: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\\': out += "\\\\"; break;
- Line 181: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\n': out += "\\n";  break;
- Line 182: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\r': out += "\\r";  break;
- Line 183: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: case '\t': out += "\\t";  break;

### training/adalora_tt_bridge.cpp
Total findings: 5

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #5082 [Docs][training] Update mod... (2026-05-13)
- Line 346: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::raii
  Context: std::size_t AdaLoraTTBridge::roundAndReallocate(AdaLoraTTExport& exp, double eps) const {
- Line 379: severity=HIGH; category=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: for (const auto& layer : query_exp.layers) {
- Line 382: severity=HIGH; category=lock_contention
  Description: Mutex lock in loop — high contention
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: std::lock_guard<std::mutex> fg_lk(impl_->fingerprint_graph_mutex);
- Line 377: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, SimilarAdapter> best_by_adapter;

### training/lora_adapter.cpp
Total findings: 5

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #5082 [Docs][training] Update mod... (2026-05-13) | #3758 feat(training): Rea
- Line 57: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * Values are drawn uniformly from [-limit, +limit].
- Line 103: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (a == 0.0f) continue;  // skip zero multiplication (common at init)
- Line 260: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 262: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### training/knowledge_graph_enricher.cpp
Total findings: 4

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #5421 fix: thread-safety for Prov... (2026-06-01) | #4268 ProvenanceTracker:
- Line 291: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: EnrichmentStats enrichQuery(const std::string& aql_query, EnrichmentCallback callback) {
- Line 443: severity=MEDIUM; category=missing_latency_metric
  Description: No latency measurement for operation
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::observability
  Context: void setCustomQuery(const std::string& query_name, const std::string& aql_query) {
- Line 534: severity=MEDIUM; category=stale_doc_section_reference
  Description: Code comment references section 'AQL metadata API").' that was not found in 'src/training/FUTURE_ENHANCEMENTS.md'
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: src/training/FUTURE_ENHANCEMENTS.md §"AQL metadata API").

### training/lora_adapter_merger.cpp
Total findings: 4

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #5082 [Docs][training] Update mod... (2026-05-13) | #4405 [WIP] Add AdaLoRA i
- Line 322: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: if (v == 0.0f) continue;
- Line 233: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: descs.push_back({adapters[i], lname, weights[i]});
- Line 387: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: descs.push_back({a, lname, 1.0f});

### training/modality_parser.cpp
Total findings: 4

- Line 144: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: R"((?:BGH|BVerwG|BAG|BSG|BFH|BVerfG|OLG|LG|AG|VG|OVG|VGH|LAG|FG|FGH|LSG|SGG?)\b[,\s]*(?:Urt\.|Beschl
- Line 150: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::platform
  Context: R"((?:EuGH|EuG|EGMR|ECtHR)\b[,\s]*(?:Rs\.\s*)?[CT]-?\d+/\d{2,4})",
- Line 275: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance_patterns
  Context: clean_text += '\n';
- Line 276: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::performance
  Context: clean_text += '\n';

### training/training_pipeline.cpp
Total findings: 4

- Line 169: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::reliability
  Context: auto pstats = provenance_tracker_->write(prov_records);
- Line 536: severity=HIGH; category=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: result.best_val_loss = (best_val_loss == std::numeric_limits<double>::max())
- Line 457: severity=MEDIUM; category=uninitialized_member_field
  Description: Default constructor does not initialize POD members
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::uninitialized
  Context: Struct with uninitialized fields
- Line 710: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: blocks.push_back({yi, 1});

### training/lora_checkpoint_manager.cpp
Total findings: 3

- Line 194: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 221: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety
- Line 317: severity=HIGH; category=resource_leaked_in_exception
  Description: Exception before delete causes resource leak
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::exception_safety

### training/multi_task_lora.cpp
Total findings: 3

- Line 140: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: for (size_t k = 0; k < std::min(inp.size(), in_dim); ++k) {
- Line 125: severity=MEDIUM; category=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::determinism
  Context: std::unordered_map<std::string, std::vector<size_t>> task_sample_map;
- Line 17: severity=LOW; category=hardcoded_output
  Description: Hardcoded stdout output instead of structured logging
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::audit_logging
  Context: *          prototype vectors (centroid of task inputs seen during training).

### training/FUTURE_ENHANCEMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'FUTURE_ENHANCEMENTS.md' is missing expected cross-links: ARCHITECTURE.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### training/PRODUCTION_REQUIREMENTS.md
Total findings: 1

- Line 1: severity=LOW; category=module_doc_linkset_drift
  Description: Module doc 'PRODUCTION_REQUIREMENTS.md' is missing expected cross-links: FUTURE_ENHANCEMENTS.md, README.md, ROADMAP.md
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::themis_doc_freshness_rules
  Context: Refresh the top-level <!-- Links: ... --> metadata to match the current module doc set

### training/adapter_serving.cpp
Total findings: 1

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Review finding and apply recommended module-specific fix.
  Scanner: Uniform::container
  Context: * PR History (last 5): #5082 [Docs][training] Update mod... (2026-05-13) | #4519 [WIP] Update develo

## Update Workflow

- Refresh scanner artifacts with: python tools/gs3_orchestrator.py ./src --output ai_working/gap_scan_results.json
- Regenerate docs with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- Add --no-mirror when you only want archive docs in ai_working/module_gaps.

Format: THEMIS_MODULE_GAPS_V4
