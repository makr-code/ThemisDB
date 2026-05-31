# training Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: training
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 463
- Actionable Findings (Critical + High): 295
- Affected Files: 15

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 83 |
| High | 212 |
| Medium | 168 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| llm_ai_safety | 142 |
| container | 73 |
| reliability | 68 |
| performance_patterns | 53 |
| performance | 28 |
| audit_logging | 20 |
| exception_safety | 18 |
| raii | 13 |
| memory | 11 |
| security | 9 |
| determinism | 8 |
| observability | 6 |
| platform | 5 |
| concurrency | 4 |
| legacy_duplication | 3 |
| uninitialized | 2 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/training/incremental_lora_trainer.cpp | 100 | 33 | 54 | 13 | 0 |
| src/training/lora_data_selection.cpp | 65 | 11 | 22 | 32 | 0 |
| src/training/ada_lora_adapter.cpp | 48 | 8 | 37 | 3 | 0 |
| src/training/auto_labeler.cpp | 43 | 3 | 9 | 31 | 0 |
| src/training/lora_adapter.cpp | 36 | 10 | 24 | 2 | 0 |
| src/training/training_pipeline.cpp | 32 | 6 | 10 | 16 | 0 |
| src/training/modality_parser.cpp | 28 | 4 | 3 | 21 | 0 |
| src/training/examples/database_optimizer_labeler.cpp | 23 | 0 | 17 | 6 | 0 |
| src/training/adalora_tt_bridge.cpp | 21 | 1 | 15 | 5 | 0 |
| src/training/provenance_tracker.cpp | 20 | 4 | 2 | 14 | 0 |
| src/training/lora_adapter_merger.cpp | 14 | 0 | 4 | 10 | 0 |
| src/training/knowledge_graph_enricher.cpp | 12 | 2 | 3 | 7 | 0 |
| src/training/lora_checkpoint_manager.cpp | 11 | 1 | 10 | 0 | 0 |
| src/training/database_domain_auto_labeler.cpp | 9 | 0 | 1 | 8 | 0 |
| src/training/adapter_serving.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### src/training/incremental_lora_trainer.cpp
Total findings: 100

- Line 136: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Production: FOR sample IN @collection RETURN {input: sample.input, output: sample.output}
  Confidence: band=very_high; score=0.99
- Line 166: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: //   1. Create input/target batch (from training_data or synthetic)
  Confidence: band=very_high; score=0.99
- Line 167: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: //   2. Forward pass: output = input @ B @ A * scaling
  Confidence: band=very_high; score=0.99
- Line 291: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: loadCheckpointWeights(checkpoint_path);
  Confidence: band=very_high; score=0.99
- Line 618: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const bool weight_set = llm_router_->setAdapterWeight(adapter_version, traffic_split);
- Line 639: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const bool weight_set = llm_router_->setAdapterWeight(target_version, 1.0f);
- Line 907: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Build input and target tensors
  Confidence: band=very_high; score=0.99
- Line 908: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: llm::lora::Tensor input ({batch_size, feature_dim});
  Confidence: band=very_high; score=0.99
- Line 912: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::vector<float> input_vec, target_vec;
  Confidence: band=very_high; score=0.99
- Line 915: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_vec  = encodeSample(training_data[idx].first,  feature_dim);
  Confidence: band=very_high; score=0.99
- Line 926: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_vec.resize(feature_dim);
  Confidence: band=very_high; score=0.99
- Line 928: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (auto& v : input_vec)  v = d(gen_in);
  Confidence: band=very_high; score=0.99
- Line 932: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input [b * feature_dim + d] = input_vec[d];
  Confidence: band=very_high; score=0.99
- Line 948: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: output = q_lora_layer_->forward(input);
  Confidence: band=very_high; score=0.99
- Line 950: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // output = input @ B @ A * scaling
  Confidence: band=very_high; score=0.99
- Line 951: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: output = lora_layer_->forward(input);
  Confidence: band=very_high; score=0.99
- Line 996: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::vector<float> input_data (batch_size * feature_dim);
  Confidence: band=very_high; score=0.99
- Line 1000: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::vector<float> input_vec, target_vec;
  Confidence: band=very_high; score=0.99
- Line 1003: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_vec  = encodeSample(training_data[idx].first,  feature_dim);
  Confidence: band=very_high; score=0.99
- Line 1009: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_vec.resize(feature_dim);
  Confidence: band=very_high; score=0.99
- Line 1011: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (auto& v : input_vec)  v = d(gen_in);
  Confidence: band=very_high; score=0.99
- Line 1015: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_data [b * feature_dim + fd] = input_vec[fd];
  Confidence: band=very_high; score=0.99
- Line 1021: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: llm::lora::GPUTensor input ({batch_size, feature_dim}, gpu_device_);
  Confidence: band=very_high; score=0.99
- Line 1023: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.upload(input_data);
  Confidence: band=very_high; score=0.99
- Line 1028: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: float loss = trainer.train_step(input, target);
  Confidence: band=very_high; score=0.99
- Line 1044: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::vector<float> full_input (batch_size * feature_dim);
  Confidence: band=very_high; score=0.99
- Line 1061: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: full_input [b * feature_dim + fd] = in_vec[fd];
  Confidence: band=very_high; score=0.99
- Line 1068: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::vector<llm::lora::GPUTensor> gpu_inputs, gpu_targets;
  Confidence: band=very_high; score=0.99
- Line 1077: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: in_t.upload(full_input.data()  + offset * feature_dim, rows * feature_dim);
  Confidence: band=very_high; score=0.99
- Line 1079: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: gpu_inputs.push_back(std::move(in_t));
  Confidence: band=very_high; score=0.99
- Line 1084: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: float avg_loss = multi_gpu_trainer_->train_step(*multi_gpu_layer_,
- Line 1085: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: gpu_inputs,
  Confidence: band=very_high; score=0.99
- Line 1166: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: void loadCheckpointWeights(const std::string& checkpoint_prefix) {
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #4519 [WIP] Update developer documentation for module training (2026-04-12T20:28:15Z)
- Line 136: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Production: FOR sample IN @collection RETURN {input: sample.input, output: sample.output}
  Confidence: band=very_high; score=0.9
- Line 166: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: //   1. Create input/target batch (from training_data or synthetic)
  Confidence: band=very_high; score=0.9
- Line 167: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: //   2. Forward pass: output = input @ B @ A * scaling
  Confidence: band=very_high; score=0.9
- Line 386: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (traffic_split == 1.0f) {
  Confidence: band=very_high; score=0.9
- Line 470: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: if (rank <= 0) throw std::invalid_argument("LoRA rank must be positive");
- Line 471: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: if (alpha <= 0.0f) throw std::invalid_argument("LoRA alpha must be positive");
- Line 472: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: if (learning_rate <= 0.0f) throw std::invalid_argument("Learning rate must be positive");
- Line 522: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Federated learning rate must be positive");
- Line 529: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("no training since last export");
- Line 540: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data_map[layer] = sum / static_cast<double>(gradient_update_count_);
- Line 555: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // applied; unknown layer names are silently ignored for forward-compatibility.
  Confidence: band=high; score=0.8
- Line 907: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Build input and target tensors
  Confidence: band=very_high; score=0.9
- Line 908: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: llm::lora::Tensor input ({batch_size, feature_dim});
  Confidence: band=very_high; score=0.9
- Line 912: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::vector<float> input_vec, target_vec;
  Confidence: band=very_high; score=0.9
- Line 915: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_vec  = encodeSample(training_data[idx].first,  feature_dim);
  Confidence: band=very_high; score=0.9
- Line 926: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_vec.resize(feature_dim);
  Confidence: band=very_high; score=0.9
- Line 928: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: for (auto& v : input_vec)  v = d(gen_in);
  Confidence: band=very_high; score=0.9
- Line 932: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input [b * feature_dim + d] = input_vec[d];
  Confidence: band=very_high; score=0.9
- Line 948: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: output = q_lora_layer_->forward(input);
  Confidence: band=very_high; score=0.9
- Line 950: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // output = input @ B @ A * scaling
  Confidence: band=very_high; score=0.9
- Line 951: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: output = lora_layer_->forward(input);
  Confidence: band=very_high; score=0.9
- Line 996: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::vector<float> input_data (batch_size * feature_dim);
  Confidence: band=very_high; score=0.9
- Line 1000: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::vector<float> input_vec, target_vec;
  Confidence: band=very_high; score=0.9
- Line 1003: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_vec  = encodeSample(training_data[idx].first,  feature_dim);
  Confidence: band=very_high; score=0.9
- Line 1009: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_vec.resize(feature_dim);
  Confidence: band=very_high; score=0.9
- Line 1011: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: for (auto& v : input_vec)  v = d(gen_in);
  Confidence: band=very_high; score=0.9
- Line 1015: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_data [b * feature_dim + fd] = input_vec[fd];
  Confidence: band=very_high; score=0.9
- Line 1015: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: input_data [b * feature_dim + fd] = input_vec[fd];
- Line 1016: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: target_data[b * feature_dim + fd] = target_vec[fd];
- Line 1021: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: llm::lora::GPUTensor input ({batch_size, feature_dim}, gpu_device_);
  Confidence: band=very_high; score=0.9
- Line 1023: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.upload(input_data);
  Confidence: band=very_high; score=0.9
- Line 1028: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: float loss = trainer.train_step(input, target);
  Confidence: band=very_high; score=0.9
- Line 1044: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::vector<float> full_input (batch_size * feature_dim);
  Confidence: band=very_high; score=0.9
- Line 1061: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: full_input [b * feature_dim + fd] = in_vec[fd];
  Confidence: band=very_high; score=0.9
- Line 1068: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::vector<llm::lora::GPUTensor> gpu_inputs, gpu_targets;
  Confidence: band=very_high; score=0.9
- Line 1068: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::vector<llm::lora::GPUTensor> gpu_inputs, gpu_targets;
  Confidence: band=very_high; score=0.9
- Line 1077: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: in_t.upload(full_input.data()  + offset * feature_dim, rows * feature_dim);
  Confidence: band=very_high; score=0.9
- Line 1079: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: gpu_inputs.push_back(std::move(in_t));
  Confidence: band=very_high; score=0.9
- Line 1079: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: gpu_inputs.push_back(std::move(in_t));
  Confidence: band=very_high; score=0.9
- Line 1085: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: gpu_inputs,
  Confidence: band=very_high; score=0.9
- Line 1085: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: gpu_inputs,
  Confidence: band=very_high; score=0.9
- Line 1139: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::overflow_error(
- Line 1213: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("LoRA rank must be positive");
- Line 1215: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("LoRA alpha must be positive");
- Line 1217: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Learning rate must be positive");
- Line 1219: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Batch size must be positive");
- Line 1221: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("num_gpus must be at least 1");
- Line 1223: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("sync_steps must be at least 1");
- Line 1225: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Quantization block_size must be positive");
- Line 1315: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: checkpoint_manager_->save(prefix + "_weights.bin", meta);
- Line 1336: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Try path as a metadata file directly (legacy format)
  Confidence: band=high; score=0.8
- Line 1348: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // No checkpoint file found: use default values for test/demo compatibility
  Confidence: band=high; score=0.8
- Line 191: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: metrics_.step_losses.push_back(step_loss);
- Line 431: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: versions.push_back(ver);
  Confidence: band=high; score=0.74
- Line 432: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: versions.push_back(ver);
- Line 445: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active.emplace_back(ver, rec.traffic_split);
  Confidence: band=high; score=0.74
- Line 793: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 811: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1078: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: gpu_inputs.push_back(std::move(in_t));
  Confidence: band=high; score=0.74
- Line 1078: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: gpu_inputs.push_back(std::move(in_t));
  Confidence: band=high; score=0.74
- Line 1079: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: gpu_inputs.push_back(std::move(in_t));
- Line 1080: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: gpu_targets.push_back(std::move(tg_t));
- Line 1159: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: f.close();
- Line 1197: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 1393: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {

### src/training/lora_data_selection.cpp
Total findings: 65

- Line 537: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const std::vector<DataSample>& input,
  Confidence: band=very_high; score=0.99
- Line 544: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto s1 = filterByQuality(input);
  Confidence: band=very_high; score=0.99
- Line 571: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: result.audit_entry.input_sample_count  = input.size();
  Confidence: band=very_high; score=0.99
- Line 573: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: result.audit_entry.filtered_by_quality = input.size() - s1.size();
  Confidence: band=very_high; score=0.99
- Line 613: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const std::vector<DataSample>& input_samples,
  Confidence: band=very_high; score=0.99
- Line 615: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto result = impl_->run(input_samples, std::move(callback));
  Confidence: band=very_high; score=0.99
- Line 666: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Rejection rates (fraction removed at each stage relative to total input)
  Confidence: band=very_high; score=0.99
- Line 667: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (ae.input_sample_count > 0) {
  Confidence: band=very_high; score=0.99
- Line 669: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: static_cast<double>(ae.input_sample_count);
  Confidence: band=very_high; score=0.99
- Line 671: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: static_cast<double>(ae.input_sample_count);
  Confidence: band=very_high; score=0.99
- Line 674: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (ae.input_sample_count > 1) {
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 102: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: if (lower.find(p) != std::string::npos) return true;
- Line 197: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: while ((pos = lower.find(lkw, pos)) != std::string::npos) {
- Line 537: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const std::vector<DataSample>& input,
  Confidence: band=very_high; score=0.9
- Line 544: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto s1 = filterByQuality(input);
  Confidence: band=very_high; score=0.9
- Line 571: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: result.audit_entry.input_sample_count  = input.size();
  Confidence: band=very_high; score=0.9
- Line 573: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: result.audit_entry.filtered_by_quality = input.size() - s1.size();
  Confidence: band=very_high; score=0.9
- Line 613: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const std::vector<DataSample>& input_samples,
  Confidence: band=very_high; score=0.9
- Line 615: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto result = impl_->run(input_samples, std::move(callback));
  Confidence: band=very_high; score=0.9
- Line 666: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Rejection rates (fraction removed at each stage relative to total input)
  Confidence: band=very_high; score=0.9
- Line 667: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (ae.input_sample_count > 0) {
  Confidence: band=very_high; score=0.9
- Line 669: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: static_cast<double>(ae.input_sample_count);
  Confidence: band=very_high; score=0.9
- Line 671: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: static_cast<double>(ae.input_sample_count);
  Confidence: band=very_high; score=0.9
- Line 674: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (ae.input_sample_count > 1) {
  Confidence: band=very_high; score=0.9
- Line 782: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 786: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 887: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("LoRADataSelectionConfig: cannot open file: " + path);
- Line 953: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: << "\"input_sample_count\":" << input_sample_count           << ","
  Confidence: band=very_high; score=0.9
- Line 1101: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("SelfImprovementConfig: cannot open file: " + path);
- Line 1163: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (name == "inference_latency_ms")  return m.inference_latency_ms;
  Confidence: band=very_high; score=0.9
- Line 123: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: while (iss >> w) words.push_back(w);
  Confidence: band=high; score=0.74
- Line 124: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: while (iss >> w) words.push_back(w);
- Line 126: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> shingles;
  Confidence: band=medium; score=0.66
- Line 231: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<char, int> freq;
  Confidence: band=medium; score=0.66
- Line 316: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(std::move(s));
  Confidence: band=high; score=0.74
- Line 317: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(std::move(s));
- Line 331: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sigs.push_back(detail::buildMinHash(s.text, config_.minhash_num_perm));
- Line 347: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!is_dup[i]) out.push_back(samples[i]);
  Confidence: band=high; score=0.74
- Line 348: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: if (!is_dup[i]) out.push_back(samples[i]);
- Line 383: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& s : samples) embeddings.push_back(embed(s.text));
- Line 383: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: for (const auto& s : samples) embeddings.push_back(embed(s.text));
  Confidence: band=high; score=0.74
- Line 390: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: centroids.push_back(embeddings[i * step]);
  Confidence: band=high; score=0.74
- Line 391: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: centroids.push_back(embeddings[i * step]);
- Line 446: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(samples[best_idx]);
  Confidence: band=high; score=0.74
- Line 446: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(samples[best_idx]);
  Confidence: band=high; score=0.74
- Line 447: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(samples[best_idx]);
- Line 576: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.audit_entry.selected_ids.push_back(s.id);
  Confidence: band=high; score=0.74
- Line 577: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.audit_entry.selected_ids.push_back(s.id);
- Line 906: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if      (c == '"')  out += "\\\"";
  Confidence: band=high; score=0.74
- Line 907: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if      (c == '"')  out += "\\\"";
- Line 908: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\\') out += "\\\\";
- Line 909: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\n') out += "\\n";
- Line 910: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\r') out += "\\r";
- Line 911: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c == '\t') out += "\\t";
- Line 912: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: else if (c < 0x20)  out += "\\u00" + std::string(1, "0123456789abcdef"[c >> 4])
- Line 928: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (i > 0) ids_arr += ',';
- Line 929: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: ids_arr += '"';
- Line 931: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: ids_arr += '"';
- Line 939: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: if (!first_domain) domain_obj += ',';
- Line 941: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: domain_obj += '"';
- Line 943: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: domain_obj += "\":";
- Line 1143: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: catch (...) { return false; } // malformed threshold: treat as not triggered

### src/training/ada_lora_adapter.cpp
Total findings: 48

- Line 114: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = layers_.find(name);
- Line 317: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const std::vector<float>& input,
  Confidence: band=very_high; score=0.99
- Line 324: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (input.size() != batch_size * D_in)
  Confidence: band=very_high; score=0.99
- Line 325: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: throw std::invalid_argument("Input size mismatch");
  Confidence: band=very_high; score=0.99
- Line 329: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // hidden = input @ B[:, :r]   → batch_size × r
  Confidence: band=very_high; score=0.99
- Line 335: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: acc += input[b * D_in + d] * lay.B[d * lay.max_rank + i];
  Confidence: band=very_high; score=0.99
- Line 463: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const std::vector<float>& input,
  Confidence: band=very_high; score=0.99
- Line 465: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return impl_->forward(layer_name, input, batch_size);
  Confidence: band=very_high; score=0.99
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
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #5082 [Docs][training] Update module docs across src/include with API, ru... (2026-05-13T11:01
- Line 87: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Layer name must not be empty");
- Line 89: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Dimensions must be > 0");
- Line 91: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Layer '" + name + "' already exists");
- Line 179: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: ReallocResult reallocateRanks(size_t total_budget) {
- Line 181: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("total_budget must be > 0");
- Line 209: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: size_t allocated = 0;
- Line 218: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: allocated += raw;
- Line 223: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (allocated != total_budget && !order.empty()) {
- Line 233: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: if (allocated > total_budget) {
- Line 234: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: size_t excess = allocated - total_budget;
- Line 238: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: size_t deficit = total_budget - allocated;
- Line 299: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("B size mismatch");
- Line 301: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("A size mismatch");
- Line 317: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const std::vector<float>& input,
  Confidence: band=very_high; score=0.9
- Line 324: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (input.size() != batch_size * D_in)
  Confidence: band=very_high; score=0.9
- Line 325: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: throw std::invalid_argument("Input size mismatch");
  Confidence: band=very_high; score=0.9
- Line 325: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Input size mismatch");
- Line 329: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // hidden = input @ B[:, :r]   → batch_size × r
  Confidence: band=very_high; score=0.9
- Line 335: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: acc += input[b * D_in + d] * lay.B[d * lay.max_rank + i];
  Confidence: band=very_high; score=0.9
- Line 369: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::out_of_range("AdaLoRAAdapter: unknown layer '" + name + "'");
- Line 376: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::out_of_range("AdaLoRAAdapter: unknown layer '" + name + "'");
- Line 423: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: ReallocResult AdaLoRAAdapter::reallocateRanks(size_t total_budget) {
- Line 424: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return impl_->reallocateRanks(total_budget);
- Line 427: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: ReallocResult AdaLoRAAdapter::reallocateRanks() {
- Line 428: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return impl_->reallocateRanks(impl_->rankBudget());
- Line 463: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const std::vector<float>& input,
  Confidence: band=very_high; score=0.9
- Line 465: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return impl_->forward(layer_name, input, batch_size);
  Confidence: band=very_high; score=0.9
- Line 110: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: insertion_order_.push_back(name);
- Line 281: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.push_back(s);
  Confidence: band=high; score=0.74
- Line 282: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: stats.push_back(s);

### src/training/auto_labeler.cpp
Total findings: 43

- Line 666: severity=CRITICAL; category=hardcoded_secret
  Description: hardcoded_secret: Hardcoded secret — use environment variable
  Remediation: Hardcoded secret — use environment variable
  Context: std::string token = "@" + placeholder;
- Line 689: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Structured input/output pair for LoRA fine-tuning
  Confidence: band=very_high; score=0.99
- Line 690: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: sample.input  = "Analyze the legal modality in: \"" + text + "\"";
  Confidence: band=very_high; score=0.99
- Line 419: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entity : fetchAllDocumentsDirect()) {
- Line 434: severity=HIGH; category=no_retry_logic
  Description: database_query without retry logic — transient failures will propagate
  Remediation: Add retry loop with exponential backoff (e.g., 3 retries, 100ms-1s)
  Context: static const std::regex simple_key_query(
- Line 452: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entity : fetchAllDocumentsDirect()) {
- Line 531: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (doc_ptr && doc_ptr->is_object() &&
- Line 532: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: doc_ptr->contains("text") && (*doc_ptr)["text"].is_string()) {
- Line 537: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto& entity : fetchAllDocumentsDirect()) {
- Line 667: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: while ((pos = query.find(token, pos)) != std::string::npos) {
- Line 689: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Structured input/output pair for LoRA fine-tuning
  Confidence: band=very_high; score=0.9
- Line 690: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: sample.input  = "Analyze the legal modality in: \"" + text + "\"";
  Confidence: band=very_high; score=0.9
- Line 134: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: batch.push_back(std::move(sample));
- Line 152: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 243: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: samples.push_back(std::move(sample));
  Confidence: band=high; score=0.74
- Line 244: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples.push_back(std::move(sample));
- Line 247: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples.push_back(std::move(sample));
- Line 264: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 276: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: samples.push_back(std::move(sample));
  Confidence: band=high; score=0.74
- Line 277: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples.push_back(std::move(sample));
- Line 280: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples.push_back(std::move(sample));
- Line 287: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: LabelingStats labelQuery(const std::string& aql_query, LabelingCallback callback) {
  Confidence: band=high; score=0.74
- Line 313: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: batch.push_back(std::move(sample));
- Line 330: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 378: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string getFetchAllQuery() const {
  Confidence: band=high; score=0.74
- Line 383: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string getBatchInsertQuery() const {
  Confidence: band=high; score=0.74
- Line 421: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(entity.getPrimaryKey());
  Confidence: band=high; score=0.74
- Line 422: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ids.push_back(entity.getPrimaryKey());
- Line 458: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(entity.getPrimaryKey());
  Confidence: band=high; score=0.74
- Line 459: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ids.push_back(entity.getPrimaryKey());
- Line 467: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::vector<std::string> executeAqlQuery(const std::string& aql) const {
  Confidence: band=high; score=0.74
- Line 477: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(item.get<std::string>());
  Confidence: band=high; score=0.74
- Line 478: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ids.push_back(item.get<std::string>());
- Line 481: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ids.push_back(item["pk"].get<std::string>());
- Line 483: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ids.push_back(item["_key"].get<std::string>());
- Line 490: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ids.push_back(item.get<std::string>());
- Line 509: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '\\': safe_id += "\\\\"; break;
  Confidence: band=high; score=0.74
- Line 509: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '\\': safe_id += "\\\\"; break;
  Confidence: band=high; score=0.74
- Line 510: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\\': safe_id += "\\\\"; break;
- Line 511: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"':  safe_id += "\\\""; break;
- Line 512: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\n': safe_id += "\\n";  break;
- Line 513: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\r': safe_id += "\\r";  break;
- Line 514: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\t': safe_id += "\\t";  break;

### src/training/lora_adapter.cpp
Total findings: 36

- Line 21: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: *  3. Forward pass      – output = (input @ B @ A) × scaling
  Confidence: band=very_high; score=0.99
- Line 62: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * @param fan_in  Input feature count (used to compute the uniform bound)
  Confidence: band=very_high; score=0.99
- Line 318: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const std::vector<float>& input,
  Confidence: band=very_high; score=0.99
- Line 326: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (input.size() != batch_size * e.in_dim) {
  Confidence: band=very_high; score=0.99
- Line 328: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: oss << "LoRAAdapter::forward: input size mismatch for layer '" << layer_name
  Confidence: band=very_high; score=0.99
- Line 331: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: << "), got " << input.size() << ")";
  Confidence: band=very_high; score=0.99
- Line 335: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Step 1: hidden = input @ B  →  (batch_size × in_dim) @ (in_dim × rank)
  Confidence: band=very_high; score=0.99
- Line 337: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::vector<float> hidden = detail::matmul(input,  batch_size, e.in_dim,
  Confidence: band=very_high; score=0.99
- Line 447: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const std::vector<float>& input,
  Confidence: band=very_high; score=0.99
- Line 449: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return impl_->forward(layer_name, input, batch_size);
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #5082 [Docs][training] Update module docs across src/include with API, ru... (2026-05-13T11:01
- Line 21: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: *  3. Forward pass      – output = (input @ B @ A) × scaling
  Confidence: band=very_high; score=0.9
- Line 59: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * Values are drawn uniformly from [-limit, +limit].
- Line 62: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * @param fan_in  Input feature count (used to compute the uniform bound)
  Confidence: band=very_high; score=0.9
- Line 105: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (a == 0.0f) continue;  // skip zero multiplication (common at init)
  Confidence: band=very_high; score=0.9
- Line 126: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("LoRAAdapter: default_rank must be > 0");
- Line 128: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("LoRAAdapter: default_alpha must be > 0");
- Line 139: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("LoRAAdapter::addLayer: layer_name must not be empty");
- Line 141: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("LoRAAdapter::addLayer: in_dim and out_dim must be > 0");
- Line 143: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("LoRAAdapter::addLayer: layer '" + layer_name + "' already exists");
- Line 151: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 246: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::out_of_range("LoRAAdapter::applyUpdate: unknown layer '" + layer_name + "'");
- Line 276: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument(
- Line 318: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const std::vector<float>& input,
  Confidence: band=very_high; score=0.9
- Line 322: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::out_of_range("LoRAAdapter::forward: unknown layer '" + layer_name + "'");
- Line 326: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (input.size() != batch_size * e.in_dim) {
  Confidence: band=very_high; score=0.9
- Line 328: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: oss << "LoRAAdapter::forward: input size mismatch for layer '" << layer_name
  Confidence: band=very_high; score=0.9
- Line 331: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: << "), got " << input.size() << ")";
  Confidence: band=very_high; score=0.9
- Line 335: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Step 1: hidden = input @ B  →  (batch_size × in_dim) @ (in_dim × rank)
  Confidence: band=very_high; score=0.9
- Line 337: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::vector<float> hidden = detail::matmul(input,  batch_size, e.in_dim,
  Confidence: band=very_high; score=0.9
- Line 447: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const std::vector<float>& input,
  Confidence: band=very_high; score=0.9
- Line 449: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return impl_->forward(layer_name, input, batch_size);
  Confidence: band=very_high; score=0.9
- Line 183: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& kv : layers_) names.push_back(kv.first);
- Line 359: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (const auto& kv : layers_) entries.push_back(kv.second);

### src/training/training_pipeline.cpp
Total findings: 32

- Line 125: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: auto pstats = provenance_tracker_->write(prov_records);
- Line 154: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: stats.quality_issues_found = qr.missing_input + qr.missing_output
  Confidence: band=very_high; score=0.99
- Line 201: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: stats.selection_input_count    = sel.audit_entry.input_sample_count;
  Confidence: band=very_high; score=0.99
- Line 204: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: sel.audit_entry.input_sample_count - sel.selected_samples.size();
  Confidence: band=very_high; score=0.99
- Line 277: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: //     RETURN {id: sample._key, text: CONCAT(sample.input, " ", sample.output)}
  Confidence: band=very_high; score=0.99
- Line 307: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: report.missing_input    = 0;
  Confidence: band=very_high; score=0.99
- Line 154: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: stats.quality_issues_found = qr.missing_input + qr.missing_output
  Confidence: band=very_high; score=0.9
- Line 201: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: stats.selection_input_count    = sel.audit_entry.input_sample_count;
  Confidence: band=very_high; score=0.9
- Line 204: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: sel.audit_entry.input_sample_count - sel.selected_samples.size();
  Confidence: band=very_high; score=0.9
- Line 277: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: //     RETURN {id: sample._key, text: CONCAT(sample.input, " ", sample.output)}
  Confidence: band=very_high; score=0.9
- Line 284: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data_selector_->setConfig(config_.data_selection_config);
- Line 286: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return data_selector_->run(candidates, std::move(callback));
- Line 298: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: //     missing_input  = SUM(sample.input  == null ? 1 : 0)
  Confidence: band=very_high; score=0.9
- Line 301: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: //   RETURN {missing_input, missing_output, low_conf}
  Confidence: band=very_high; score=0.9
- Line 466: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: result.best_val_loss = (best_val_loss == std::numeric_limits<double>::max())
  Confidence: band=very_high; score=0.9
- Line 571: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return impl_->detectLabelDrift(reference_samples);
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 122: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: prov_records.push_back(std::move(rec));
  Confidence: band=high; score=0.74
- Line 123: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: prov_records.push_back(std::move(rec));
- Line 242: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 361: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 391: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: trials.push_back({r, lr});
  Confidence: band=high; score=0.74
- Line 391: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: trials.push_back({r, lr});
  Confidence: band=high; score=0.74
- Line 392: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: trials.push_back({r, lr});
- Line 445: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 610: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: by_category[s.category].push_back(s);
  Confidence: band=high; score=0.74
- Line 611: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: by_category[s.category].push_back(s);
- Line 635: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: blocks.push_back({yi, 1});
  Confidence: band=high; score=0.74
- Line 635: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: blocks.push_back({yi, 1});
  Confidence: band=high; score=0.74
- Line 636: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: blocks.push_back({yi, 1});
- Line 709: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.thresholds.push_back(std::move(entry));
  Confidence: band=high; score=0.74
- Line 710: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.thresholds.push_back(std::move(entry));

### src/training/modality_parser.cpp
Total findings: 28

- Line 274: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: s.input      = sentence;
  Confidence: band=very_high; score=0.99
- Line 318: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: s.input      = blk.content;
  Confidence: band=very_high; score=0.99
- Line 352: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: s.input      = m;
  Confidence: band=very_high; score=0.99
- Line 437: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: s.input      = image_path; // real: replaced by OCR text
  Confidence: band=very_high; score=0.99
- Line 318: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: s.input      = blk.content;
  Confidence: band=very_high; score=0.9
- Line 352: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: s.input      = m;
  Confidence: band=very_high; score=0.9
- Line 437: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: s.input      = image_path; // real: replaced by OCR text
  Confidence: band=very_high; score=0.9
- Line 67: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: lines.push_back(std::move(line));
  Confidence: band=high; score=0.74
- Line 68: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: lines.push_back(std::move(line));
- Line 132: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: R"((?:BGH|BVerwG|BAG|BSG|BFH|BVerfG|OLG|LG|AG|VG|OVG|VGH|LAG|FG|FGH|LSG|SGG?)\b[,\s]*(?:Urt\.|Beschl
- Line 138: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: R"((?:EuGH|EuG|EGMR|ECtHR)\b[,\s]*(?:Rs\.\s*)?[CT]-?\d+/\d{2,4})",
- Line 223: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: blocks.push_back({start, i - 1, std::move(content)});
- Line 263: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: clean_text += '\n';
  Confidence: band=high; score=0.74
- Line 263: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: clean_text += '\n';
  Confidence: band=high; score=0.74
- Line 263: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: clean_text += '\n';
  Confidence: band=high; score=0.74
- Line 264: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: clean_text += '\n';
- Line 281: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: samples.push_back(std::move(s));
  Confidence: band=high; score=0.74
- Line 282: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples.push_back(std::move(s));
- Line 445: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: samples.push_back(std::move(s));
- Line 526: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (auto& s : ocr) result.samples.push_back(std::move(s));
- Line 531: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (auto& s : clauses) result.samples.push_back(std::move(s));
  Confidence: band=high; score=0.74
- Line 532: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (auto& s : clauses) result.samples.push_back(std::move(s));
- Line 536: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (auto& s : tables) result.samples.push_back(std::move(s));
  Confidence: band=high; score=0.74
- Line 537: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (auto& s : tables) result.samples.push_back(std::move(s));
- Line 541: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (auto& s : citations) result.samples.push_back(std::move(s));
  Confidence: band=high; score=0.74
- Line 542: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (auto& s : citations) result.samples.push_back(std::move(s));
- Line 575: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (auto& s : res.samples) out_samples.push_back(std::move(s));
  Confidence: band=high; score=0.74
- Line 576: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: for (auto& s : res.samples) out_samples.push_back(std::move(s));

### src/training/examples/database_optimizer_labeler.cpp
Total findings: 23

- Line 90: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "=== DATABASE_OPTIMIZER Labeler Example (IMPL-A1 + IMPL-A3) ===\n\n";
  Confidence: band=very_high; score=0.9
- Line 95: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Step 1: Labeling optimizer-log entries\n";
  Confidence: band=very_high; score=0.9
- Line 109: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  query: \"" << entry.query_text.substr(0, 40) << "...\"\n"
  Confidence: band=very_high; score=0.9
- Line 119: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "\n  Accepted " << labeled_samples.size()
  Confidence: band=very_high; score=0.9
- Line 126: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Step 2: Applying LoRADataSelectionPipeline quality filters\n";
  Confidence: band=very_high; score=0.9
- Line 134: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  After dedup + confidence filter: " << filtered.size() << " samples\n\n";
  Confidence: band=very_high; score=0.9
- Line 136: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  [PLANNED — LoRADataSelectionPipeline not yet wired for DATABASE_OPTIMIZER domain]\n\n";
  Confidence: band=very_high; score=0.9
- Line 141: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Step 3: Incremental LoRA training (Loop 4)\n";
  Confidence: band=very_high; score=0.9
- Line 152: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Training complete — adapter version: " << trainer.activeVersion() << "\n\n";
  Confidence: band=very_high; score=0.9
- Line 154: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  [PLANNED — full training cycle requires IMPL-A1 domain wiring]\n\n";
  Confidence: band=very_high; score=0.9
- Line 159: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Step 4: Export gradient for federated aggregation (IMPL-A3)\n";
  Confidence: band=very_high; score=0.9
- Line 168: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Exported " << grad.blob.size() << " bytes (AES-256-GCM encrypted)\n\n";
  Confidence: band=very_high; score=0.9
- Line 170: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  [PLANNED — exportGradient() to be implemented in IMPL-A3]\n\n";
  Confidence: band=very_high; score=0.9
- Line 175: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Step 5: Apply GlobalAdapterDelta from federation coordinator (IMPL-A3)\n";
  Confidence: band=very_high; score=0.9
- Line 180: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Applied global FedAvg delta — adapter weights updated.\n\n";
  Confidence: band=very_high; score=0.9
- Line 182: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  [PLANNED — applyGlobalDelta() to be implemented in IMPL-A3]\n\n";
  Confidence: band=very_high; score=0.9
- Line 187: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "=== Summary ===\n"
  Confidence: band=very_high; score=0.9
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 114: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: labeled_samples.push_back(std::move(sample));
  Confidence: band=high; score=0.74
- Line 115: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: labeled_samples.push_back(std::move(sample));
- Line 120: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " / " << log_entries.size() << " samples\n\n";
- Line 120: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << " / " << log_entries.size() << " samples\n\n";
- Line 192: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: << "\nSee docs/issues/lora_loops/ for implementation specs.\n";

### src/training/adalora_tt_bridge.cpp
Total findings: 21

- Line 340: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it == impl_->export_cache.end()) {
- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #5082 [Docs][training] Update module docs across src/include with API, ru... (2026-05-13T11:01
- Line 95: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data[row * r + i] = sign * (B[row * r + i] / norm_b) * scale;
- Line 131: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: data[i * k + col] = sign * (A[i * k + col] / norm_a) * scale;
- Line 197: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Layer not found: " + layer_name);
- Line 206: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Layer has zero rank: " + layer_name);
- Line 209: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Layer rank exceeds max_tt_rank: " + layer_name);
- Line 212: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Invalid matrix shape for layer: " + layer_name);
- Line 240: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ++impl_->stats_data.exports_total;
- Line 266: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("No exportable layers in adapter: " + adapter_name);
- Line 274: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("Empty TT export");
- Line 307: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ++impl_->stats_data.imports_total;
- Line 322: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& layer : exp.layers) {
  Confidence: band=very_high; score=0.9
- Line 332: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ++impl_->stats_data.stores_total;
- Line 346: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: std::size_t AdaLoraTTBridge::roundAndReallocate(AdaLoraTTExport& exp, double eps) const {
- Line 412: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return impl_->stats_data;
- Line 261: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.layers.push_back(exportLayer(adapter, ls.layer_name));
  Confidence: band=high; score=0.74
- Line 262: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.layers.push_back(exportLayer(adapter, ls.layer_name));
- Line 377: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, SimilarAdapter> best_by_adapter;
  Confidence: band=medium; score=0.66
- Line 398: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(kv.second);
  Confidence: band=high; score=0.74
- Line 399: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(kv.second);

### src/training/provenance_tracker.cpp
Total findings: 20

- Line 79: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: ProvenanceWriteStats write(const std::vector<ProvenanceRecord>& records) {
- Line 345: severity=CRITICAL; category=hardcoded_secret
  Description: hardcoded_secret: Hardcoded secret — use environment variable
  Remediation: Hardcoded secret — use environment variable
  Context: std::string token = "@" + placeholder;
- Line 383: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: ProvenanceWriteStats ProvenanceTracker::write(const std::vector<ProvenanceRecord>& records) {
- Line 384: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: return impl_->write(records);
- Line 301: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (doc_ptr && doc_ptr->is_object()) {
- Line 346: severity=HIGH; category=o_n_squared
  Description: O(n²) pattern: find() on vector inside loop
  Remediation: Use std::unordered_map or std::set for O(log n) or O(1) lookup
  Context: while ((pos = query.find(token, pos)) != std::string::npos) {
- Line 179: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: audit_log_.push_back(oss.str());
- Line 214: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: root.parents.push_back(std::move(node));
  Confidence: band=high; score=0.74
- Line 215: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: root.parents.push_back(std::move(node));
- Line 265: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sample_node.parents.push_back(std::move(doc_node));
  Confidence: band=high; score=0.74
- Line 266: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: sample_node.parents.push_back(std::move(doc_node));
- Line 269: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: root.parents.push_back(std::move(sample_node));
- Line 334: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, ProvenanceRecord> store_;
  Confidence: band=medium; score=0.66
- Line 360: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '\\': out += "\\\\"; break;
  Confidence: band=high; score=0.74
- Line 360: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '\\': out += "\\\\"; break;
  Confidence: band=high; score=0.74
- Line 361: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\\': out += "\\\\"; break;
- Line 362: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"':  out += "\\\""; break;
- Line 363: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\n': out += "\\n";  break;
- Line 364: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\r': out += "\\r";  break;
- Line 365: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\t': out += "\\t";  break;

### src/training/lora_adapter_merger.cpp
Total findings: 14

- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #5082 [Docs][training] Update module docs across src/include with API, ru... (2026-05-13T11:01
- Line 228: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: const LoRAWeightEntry& ref_entry = adapters[0]->getWeights(lname);
- Line 324: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (v == 0.0f) continue;
  Confidence: band=very_high; score=0.9
- Line 382: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: const LoRAWeightEntry& ref = adapters[0]->getWeights(lname);
- Line 234: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: descs.push_back({adapters[i], lname, weights[i]});
  Confidence: band=high; score=0.74
- Line 234: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: descs.push_back({adapters[i], lname, weights[i]});
  Confidence: band=high; score=0.74
- Line 234: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: descs.push_back({adapters[i], lname, weights[i]});
  Confidence: band=high; score=0.74
- Line 235: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: descs.push_back({adapters[i], lname, weights[i]});
- Line 240: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.layers.push_back(std::move(lr));
- Line 388: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: descs.push_back({a, lname, 1.0f});
  Confidence: band=high; score=0.74
- Line 388: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: descs.push_back({a, lname, 1.0f});
  Confidence: band=high; score=0.74
- Line 388: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: descs.push_back({a, lname, 1.0f});
  Confidence: band=high; score=0.74
- Line 389: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: descs.push_back({a, lname, 1.0f});
- Line 395: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.layers.push_back(std::move(lr));

### src/training/knowledge_graph_enricher.cpp
Total findings: 12

- Line 70: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "FILTER sample.input != null "
  Confidence: band=very_high; score=0.99
- Line 110: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto it = map_.find(key);
- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #3768 [WIP] Implement vector similarity search in KnowledgeGraphEnricher (2026-03-12T07:52:46Z
- Line 70: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "FILTER sample.input != null "
  Confidence: band=very_high; score=0.9
- Line 413: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 216: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 268: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: context.similar_documents.push_back(doc_id);
  Confidence: band=high; score=0.74
- Line 269: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: context.similar_documents.push_back(doc_id);
- Line 293: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: EnrichmentStats enrichQuery(const std::string& aql_query, EnrichmentCallback callback) {
  Confidence: band=high; score=0.74
- Line 328: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 438: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: similar.emplace_back(r.pk, distanceToSimilarityScore(r.distance));
  Confidence: band=high; score=0.74
- Line 445: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void setCustomQuery(const std::string& query_name, const std::string& aql_query) {
  Confidence: band=high; score=0.74

### src/training/lora_checkpoint_manager.cpp
Total findings: 11

- Line 45: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: // Deserialize manifest blocks from the manifest file content
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 109: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("LoRACheckpointManager: checkpoint_dir must not be empty");
- Line 112: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::invalid_argument("LoRACheckpointManager: max_checkpoints must be >= 1");
- Line 142: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 151: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 158: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 244: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 250: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(

### src/training/database_domain_auto_labeler.cpp
Total findings: 9

- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #5082 [Docs][training] Update module docs across src/include with API, ru... (2026-05-13T11:01
- Line 58: severity=MEDIUM; category=uncaught_exception
  Description: Generic catch(...) — specific exception types ignored
  Remediation: Catch specific exceptions: catch(std::exception& e) { ... }
  Context: } catch (...) {
- Line 158: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(std::move(sample));
- Line 180: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '"':  out += "\\\""; break;
  Confidence: band=high; score=0.74
- Line 181: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '"':  out += "\\\""; break;
- Line 182: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\\': out += "\\\\"; break;
- Line 183: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\n': out += "\\n";  break;
- Line 184: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\r': out += "\\r";  break;
- Line 185: severity=MEDIUM; category=string_concat_loop
  Description: String concatenation in loop — O(n²) behavior
  Remediation: Use std::ostringstream or pre-allocate string with .reserve()
  Context: case '\t': out += "\\t";  break;

### src/training/adapter_serving.cpp
Total findings: 1

- Line 7: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR: #5082 [Docs][training] Update module docs across src/include with API, ru... (2026-05-13T11:01

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
