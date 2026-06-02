# training Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: training
- Generated: 2026-06-02 11:09:13
- Status: Critical Findings Present
- Total Findings: 269
- Actionable Findings (Critical + High): 200
- Affected Files: 16

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 92 |
| High | 108 |
| Medium | 69 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| llm_ai_safety | 191 |
| performance_patterns | 58 |
| container | 29 |
| performance | 29 |
| reliability | 23 |
| audit_logging | 21 |
| exception_safety | 18 |
| raii | 12 |
| determinism | 9 |
| security | 9 |
| observability | 7 |
| memory | 6 |
| platform | 5 |
| concurrency | 3 |
| legacy_duplication | 2 |
| uninitialized | 2 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/training/incremental_lora_trainer.cpp | 68 | 31 | 33 | 4 | 0 |
| src/training/multi_task_lora.cpp | 64 | 25 | 32 | 7 | 0 |
| src/training/lora_data_selection.cpp | 33 | 11 | 13 | 9 | 0 |
| src/training/lora_adapter.cpp | 21 | 10 | 11 | 0 | 0 |
| src/training/training_pipeline.cpp | 19 | 5 | 7 | 7 | 0 |
| src/training/auto_labeler.cpp | 17 | 2 | 2 | 13 | 0 |
| src/training/ada_lora_adapter.cpp | 15 | 7 | 7 | 1 | 0 |
| src/training/modality_parser.cpp | 8 | 0 | 0 | 8 | 0 |
| src/training/lora_adapter_merger.cpp | 7 | 0 | 1 | 6 | 0 |
| src/training/knowledge_graph_enricher.cpp | 6 | 1 | 1 | 4 | 0 |
| src/training/provenance_tracker.cpp | 5 | 0 | 0 | 5 | 0 |
| src/training/adalora_tt_bridge.cpp | 4 | 0 | 1 | 3 | 0 |
| src/training/database_domain_auto_labeler.cpp | 1 | 0 | 0 | 1 | 0 |
| src/training/examples/database_optimizer_labeler.cpp | 1 | 0 | 0 | 1 | 0 |
| src/training/adapter_serving.cpp | 0 | 0 | 0 | 0 | 0 |
| src/training/lora_checkpoint_manager.cpp | 0 | 0 | 0 | 0 | 0 |

## Full Scanner Findings

### src/training/incremental_lora_trainer.cpp
Total findings: 68

- Line 239: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Production: FOR sample IN @collection RETURN {input: sample.input, output: sample.output}
  Confidence: band=very_high; score=0.99
- Line 269: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: //   1. Create input/target batch (from training_data or synthetic)
  Confidence: band=very_high; score=0.99
- Line 270: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: //   2. Forward pass: output = input @ B @ A * scaling
  Confidence: band=very_high; score=0.99
- Line 406: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: if (!loadCheckpointWeights(checkpoint_path, &load_error)) {
  Confidence: band=very_high; score=0.99
- Line 779: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: *error_reason = "unable to hash checkpoint payload: " + weights_path;
  Confidence: band=very_high; score=0.99
- Line 1159: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Build input and target tensors
  Confidence: band=very_high; score=0.99
- Line 1160: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: llm::lora::Tensor input ({batch_size, feature_dim});
  Confidence: band=very_high; score=0.99
- Line 1164: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::vector<float> input_vec, target_vec;
  Confidence: band=very_high; score=0.99
- Line 1167: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_vec  = encodeSample(training_data[idx].first,  feature_dim);
  Confidence: band=very_high; score=0.99
- Line 1178: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_vec.resize(feature_dim);
  Confidence: band=very_high; score=0.99
- Line 1180: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (auto& v : input_vec)  v = d(gen_in);
  Confidence: band=very_high; score=0.99
- Line 1184: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input [b * feature_dim + d] = input_vec[d];
  Confidence: band=very_high; score=0.99
- Line 1200: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: output = q_lora_layer_->forward(input);
  Confidence: band=very_high; score=0.99
- Line 1202: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // output = input @ B @ A * scaling
  Confidence: band=very_high; score=0.99
- Line 1203: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: output = lora_layer_->forward(input);
  Confidence: band=very_high; score=0.99
- Line 1248: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::vector<float> input_data (batch_size * feature_dim);
  Confidence: band=very_high; score=0.99
- Line 1252: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::vector<float> input_vec, target_vec;
  Confidence: band=very_high; score=0.99
- Line 1255: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_vec  = encodeSample(training_data[idx].first,  feature_dim);
  Confidence: band=very_high; score=0.99
- Line 1261: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_vec.resize(feature_dim);
  Confidence: band=very_high; score=0.99
- Line 1263: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (auto& v : input_vec)  v = d(gen_in);
  Confidence: band=very_high; score=0.99
- Line 1267: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input_data [b * feature_dim + fd] = input_vec[fd];
  Confidence: band=very_high; score=0.99
- Line 1273: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: llm::lora::GPUTensor input ({batch_size, feature_dim}, gpu_device_);
  Confidence: band=very_high; score=0.99
- Line 1275: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: input.upload(input_data);
  Confidence: band=very_high; score=0.99
- Line 1280: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: float loss = trainer.train_step(input, target);
  Confidence: band=very_high; score=0.99
- Line 1296: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::vector<float> full_input (batch_size * feature_dim);
  Confidence: band=very_high; score=0.99
- Line 1313: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: full_input [b * feature_dim + fd] = in_vec[fd];
  Confidence: band=very_high; score=0.99
- Line 1320: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::vector<llm::lora::GPUTensor> gpu_inputs, gpu_targets;
  Confidence: band=very_high; score=0.99
- Line 1329: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: in_t.upload(full_input.data()  + offset * feature_dim, rows * feature_dim);
  Confidence: band=very_high; score=0.99
- Line 1331: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: gpu_inputs.push_back(std::move(in_t));
  Confidence: band=very_high; score=0.99
- Line 1337: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: gpu_inputs,
  Confidence: band=very_high; score=0.99
- Line 1418: severity=CRITICAL; category=llm_ai_safety; pattern=model_integrity_gap
  Description: Model loading without integrity verification (poisoning risk)
  Context: bool loadCheckpointWeights(const std::string& checkpoint_prefix,
  Confidence: band=very_high; score=0.99
- Line 239: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Production: FOR sample IN @collection RETURN {input: sample.input, output: sample.output}
  Confidence: band=very_high; score=0.9
- Line 269: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: //   1. Create input/target batch (from training_data or synthetic)
  Confidence: band=very_high; score=0.9
- Line 270: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: //   2. Forward pass: output = input @ B @ A * scaling
  Confidence: band=very_high; score=0.9
- Line 690: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // applied; unknown layer names are silently ignored for forward-compatibility.
  Confidence: band=high; score=0.8
- Line 755: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: entry.step == step) {
  Confidence: band=very_high; score=0.9
- Line 1159: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Build input and target tensors
  Confidence: band=very_high; score=0.9
- Line 1160: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: llm::lora::Tensor input ({batch_size, feature_dim});
  Confidence: band=very_high; score=0.9
- Line 1164: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::vector<float> input_vec, target_vec;
  Confidence: band=very_high; score=0.9
- Line 1167: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_vec  = encodeSample(training_data[idx].first,  feature_dim);
  Confidence: band=very_high; score=0.9
- Line 1178: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_vec.resize(feature_dim);
  Confidence: band=very_high; score=0.9
- Line 1180: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: for (auto& v : input_vec)  v = d(gen_in);
  Confidence: band=very_high; score=0.9
- Line 1184: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input [b * feature_dim + d] = input_vec[d];
  Confidence: band=very_high; score=0.9
- Line 1200: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: output = q_lora_layer_->forward(input);
  Confidence: band=very_high; score=0.9
- Line 1202: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // output = input @ B @ A * scaling
  Confidence: band=very_high; score=0.9
- Line 1203: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: output = lora_layer_->forward(input);
  Confidence: band=very_high; score=0.9
- Line 1248: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::vector<float> input_data (batch_size * feature_dim);
  Confidence: band=very_high; score=0.9
- Line 1252: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::vector<float> input_vec, target_vec;
  Confidence: band=very_high; score=0.9
- Line 1255: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_vec  = encodeSample(training_data[idx].first,  feature_dim);
  Confidence: band=very_high; score=0.9
- Line 1261: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_vec.resize(feature_dim);
  Confidence: band=very_high; score=0.9
- Line 1263: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: for (auto& v : input_vec)  v = d(gen_in);
  Confidence: band=very_high; score=0.9
- Line 1267: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input_data [b * feature_dim + fd] = input_vec[fd];
  Confidence: band=very_high; score=0.9
- Line 1273: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: llm::lora::GPUTensor input ({batch_size, feature_dim}, gpu_device_);
  Confidence: band=very_high; score=0.9
- Line 1275: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: input.upload(input_data);
  Confidence: band=very_high; score=0.9
- Line 1280: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: float loss = trainer.train_step(input, target);
  Confidence: band=very_high; score=0.9
- Line 1296: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::vector<float> full_input (batch_size * feature_dim);
  Confidence: band=very_high; score=0.9
- Line 1313: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: full_input [b * feature_dim + fd] = in_vec[fd];
  Confidence: band=very_high; score=0.9
- Line 1320: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::vector<llm::lora::GPUTensor> gpu_inputs, gpu_targets;
  Confidence: band=very_high; score=0.9
- Line 1320: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::vector<llm::lora::GPUTensor> gpu_inputs, gpu_targets;
  Confidence: band=very_high; score=0.9
- Line 1329: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: in_t.upload(full_input.data()  + offset * feature_dim, rows * feature_dim);
  Confidence: band=very_high; score=0.9
- Line 1331: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: gpu_inputs.push_back(std::move(in_t));
  Confidence: band=very_high; score=0.9
- Line 1331: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: gpu_inputs.push_back(std::move(in_t));
  Confidence: band=very_high; score=0.9
- Line 1337: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: gpu_inputs,
  Confidence: band=very_high; score=0.9
- Line 1337: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: gpu_inputs,
  Confidence: band=very_high; score=0.9
- Line 562: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: versions.push_back(ver);
  Confidence: band=high; score=0.74
- Line 578: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: active.emplace_back(ver, rec.traffic_split);
  Confidence: band=high; score=0.74
- Line 1330: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: gpu_inputs.push_back(std::move(in_t));
  Confidence: band=high; score=0.74
- Line 1330: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: gpu_inputs.push_back(std::move(in_t));
  Confidence: band=high; score=0.74

### src/training/multi_task_lora.cpp
Total findings: 64

- Line 17: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: *          prototype vectors (centroid of task inputs seen during training).
  Confidence: band=very_high; score=0.99
- Line 85: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Infer input dimension.
  Confidence: band=very_high; score=0.99
- Line 86: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const size_t in_dim = cfg_.input_dim > 0
  Confidence: band=very_high; score=0.99
- Line 87: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: ? cfg_.input_dim
  Confidence: band=very_high; score=0.99
- Line 88: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: : samples[0].input.size();
  Confidence: band=very_high; score=0.99
- Line 91: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: throw std::runtime_error("MultiTaskLoRATrainer: zero input dimension");
  Confidence: band=very_high; score=0.99
- Line 139: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const auto& inp = samples[idx].input;
  Confidence: band=very_high; score=0.99
- Line 181: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Forward: shared_hidden = B^T * input  (shared_rank output)
  Confidence: band=very_high; score=0.99
- Line 184: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (size_t j = 0; j < in_dim && j < s.input.size(); ++j) {
  Confidence: band=very_high; score=0.99
- Line 185: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: hidden[k] += shared_B_[j * shared_rank + k] * s.input[j];
  Confidence: band=very_high; score=0.99
- Line 219: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // dL/d(B[j][k]) = grad_pred * head[k][j] * input[j]  (simplified)
  Confidence: band=very_high; score=0.99
- Line 220: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (j < s.input.size()) {
  Confidence: band=very_high; score=0.99
- Line 222: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * task_heads_[ti][k * in_dim + j] * s.input[j];
  Confidence: band=very_high; score=0.99
- Line 265: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: DomainGatingResult inferTask(const std::vector<float>& input) const {
  Confidence: band=very_high; score=0.99
- Line 278: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t n = std::min(input.size(), proto.size());
  Confidence: band=very_high; score=0.99
- Line 282: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: dot     += input[k] * proto[k];
  Confidence: band=very_high; score=0.99
- Line 283: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: norm_in += input[k] * input[k];
  Confidence: band=very_high; score=0.99
- Line 315: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::vector<float> forward(const std::vector<float>& input) const {
  Confidence: band=very_high; score=0.99
- Line 319: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto gate = inferTask(input);
  Confidence: band=very_high; score=0.99
- Line 327: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (size_t j = 0; j < in_dim && j < input.size(); ++j) {
  Confidence: band=very_high; score=0.99
- Line 328: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: hidden[k] += shared_B_[j * shared_rank + k] * input[j];
  Confidence: band=very_high; score=0.99
- Line 394: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: DomainGatingResult MultiTaskLoRATrainer::inferTask(const std::vector<float>& input) const {
  Confidence: band=very_high; score=0.99
- Line 395: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return impl_->inferTask(input);
  Confidence: band=very_high; score=0.99
- Line 398: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::vector<float> MultiTaskLoRATrainer::forward(const std::vector<float>& input) const {
  Confidence: band=very_high; score=0.99
- Line 399: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return impl_->forward(input);
  Confidence: band=very_high; score=0.99
- Line 17: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: *          prototype vectors (centroid of task inputs seen during training).
  Confidence: band=very_high; score=0.9
- Line 17: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: *          prototype vectors (centroid of task inputs seen during training).
  Confidence: band=very_high; score=0.9
- Line 85: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Infer input dimension.
  Confidence: band=very_high; score=0.9
- Line 85: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Infer input dimension.
  Confidence: band=very_high; score=0.9
- Line 86: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const size_t in_dim = cfg_.input_dim > 0
  Confidence: band=very_high; score=0.9
- Line 87: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: ? cfg_.input_dim
  Confidence: band=very_high; score=0.9
- Line 88: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: : samples[0].input.size();
  Confidence: band=very_high; score=0.9
- Line 91: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: throw std::runtime_error("MultiTaskLoRATrainer: zero input dimension");
  Confidence: band=very_high; score=0.9
- Line 139: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const auto& inp = samples[idx].input;
  Confidence: band=very_high; score=0.9
- Line 181: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Forward: shared_hidden = B^T * input  (shared_rank output)
  Confidence: band=very_high; score=0.9
- Line 184: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: for (size_t j = 0; j < in_dim && j < s.input.size(); ++j) {
  Confidence: band=very_high; score=0.9
- Line 185: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: hidden[k] += shared_B_[j * shared_rank + k] * s.input[j];
  Confidence: band=very_high; score=0.9
- Line 219: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // dL/d(B[j][k]) = grad_pred * head[k][j] * input[j]  (simplified)
  Confidence: band=very_high; score=0.9
- Line 220: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (j < s.input.size()) {
  Confidence: band=very_high; score=0.9
- Line 222: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * task_heads_[ti][k * in_dim + j] * s.input[j];
  Confidence: band=very_high; score=0.9
- Line 262: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Inference
  Confidence: band=very_high; score=0.9
- Line 265: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: DomainGatingResult inferTask(const std::vector<float>& input) const {
  Confidence: band=very_high; score=0.9
- Line 265: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: DomainGatingResult inferTask(const std::vector<float>& input) const {
  Confidence: band=very_high; score=0.9
- Line 278: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t n = std::min(input.size(), proto.size());
  Confidence: band=very_high; score=0.9
- Line 282: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: dot     += input[k] * proto[k];
  Confidence: band=very_high; score=0.9
- Line 283: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: norm_in += input[k] * input[k];
  Confidence: band=very_high; score=0.9
- Line 315: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::vector<float> forward(const std::vector<float>& input) const {
  Confidence: band=very_high; score=0.9
- Line 319: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto gate = inferTask(input);
  Confidence: band=very_high; score=0.9
- Line 319: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto gate = inferTask(input);
  Confidence: band=very_high; score=0.9
- Line 327: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: for (size_t j = 0; j < in_dim && j < input.size(); ++j) {
  Confidence: band=very_high; score=0.9
- Line 328: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: hidden[k] += shared_B_[j * shared_rank + k] * input[j];
  Confidence: band=very_high; score=0.9
- Line 394: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: DomainGatingResult MultiTaskLoRATrainer::inferTask(const std::vector<float>& input) const {
  Confidence: band=very_high; score=0.9
- Line 394: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: DomainGatingResult MultiTaskLoRATrainer::inferTask(const std::vector<float>& input) const {
  Confidence: band=very_high; score=0.9
- Line 395: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return impl_->inferTask(input);
  Confidence: band=very_high; score=0.9
- Line 395: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: return impl_->inferTask(input);
  Confidence: band=very_high; score=0.9
- Line 398: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::vector<float> MultiTaskLoRATrainer::forward(const std::vector<float>& input) const {
  Confidence: band=very_high; score=0.9
- Line 399: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return impl_->forward(input);
  Confidence: band=very_high; score=0.9
- Line 125: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, std::vector<size_t>> task_sample_map;
  Confidence: band=medium; score=0.66
- Line 130: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: task_sample_map[s.task_id].push_back(i);
  Confidence: band=high; score=0.74
- Line 130: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: task_sample_map[s.task_id].push_back(i);
  Confidence: band=high; score=0.74
- Line 130: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: task_sample_map[s.task_id].push_back(i);
  Confidence: band=high; score=0.74
- Line 130: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: task_sample_map[s.task_id].push_back(i);
  Confidence: band=high; score=0.74
- Line 290: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.scores.push_back({tasks_[ti].id, score});
  Confidence: band=high; score=0.74
- Line 290: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.scores.push_back({tasks_[ti].id, score});
  Confidence: band=high; score=0.74

### src/training/lora_data_selection.cpp
Total findings: 33

- Line 560: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const std::vector<DataSample>& input,
  Confidence: band=very_high; score=0.99
- Line 567: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto s1 = filterByQuality(input);
  Confidence: band=very_high; score=0.99
- Line 594: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: result.audit_entry.input_sample_count  = input.size();
  Confidence: band=very_high; score=0.99
- Line 596: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: result.audit_entry.filtered_by_quality = input.size() - s1.size();
  Confidence: band=very_high; score=0.99
- Line 636: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const std::vector<DataSample>& input_samples,
  Confidence: band=very_high; score=0.99
- Line 638: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: auto result = impl_->run(input_samples, std::move(callback));
  Confidence: band=very_high; score=0.99
- Line 689: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Rejection rates (fraction removed at each stage relative to total input)
  Confidence: band=very_high; score=0.99
- Line 690: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (ae.input_sample_count > 0) {
  Confidence: band=very_high; score=0.99
- Line 692: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: static_cast<double>(ae.input_sample_count);
  Confidence: band=very_high; score=0.99
- Line 694: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: static_cast<double>(ae.input_sample_count);
  Confidence: band=very_high; score=0.99
- Line 697: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (ae.input_sample_count > 1) {
  Confidence: band=very_high; score=0.99
- Line 560: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const std::vector<DataSample>& input,
  Confidence: band=very_high; score=0.9
- Line 567: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto s1 = filterByQuality(input);
  Confidence: band=very_high; score=0.9
- Line 594: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: result.audit_entry.input_sample_count  = input.size();
  Confidence: band=very_high; score=0.9
- Line 596: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: result.audit_entry.filtered_by_quality = input.size() - s1.size();
  Confidence: band=very_high; score=0.9
- Line 636: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const std::vector<DataSample>& input_samples,
  Confidence: band=very_high; score=0.9
- Line 638: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: auto result = impl_->run(input_samples, std::move(callback));
  Confidence: band=very_high; score=0.9
- Line 689: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Rejection rates (fraction removed at each stage relative to total input)
  Confidence: band=very_high; score=0.9
- Line 690: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (ae.input_sample_count > 0) {
  Confidence: band=very_high; score=0.9
- Line 692: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: static_cast<double>(ae.input_sample_count);
  Confidence: band=very_high; score=0.9
- Line 694: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: static_cast<double>(ae.input_sample_count);
  Confidence: band=very_high; score=0.9
- Line 697: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (ae.input_sample_count > 1) {
  Confidence: band=very_high; score=0.9
- Line 976: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: << "\"input_sample_count\":" << input_sample_count           << ","
  Confidence: band=very_high; score=0.9
- Line 1186: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: if (name == "inference_latency_ms")  return m.inference_latency_ms;
  Confidence: band=very_high; score=0.9
- Line 135: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: while (iss >> w) words.push_back(w);
  Confidence: band=high; score=0.74
- Line 138: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_set<std::string> shingles;
  Confidence: band=medium; score=0.66
- Line 243: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<char, int> freq;
  Confidence: band=medium; score=0.66
- Line 370: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: if (!is_dup[i]) out.push_back(samples[i]);
  Confidence: band=high; score=0.74
- Line 413: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: centroids.push_back(embeddings[i * step]);
  Confidence: band=high; score=0.74
- Line 469: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(samples[best_idx]);
  Confidence: band=high; score=0.74
- Line 469: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(samples[best_idx]);
  Confidence: band=high; score=0.74
- Line 599: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.audit_entry.selected_ids.push_back(s.id);
  Confidence: band=high; score=0.74
- Line 929: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: if      (c == '"')  out += "\\\"";
  Confidence: band=high; score=0.74

### src/training/lora_adapter.cpp
Total findings: 21

- Line 19: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: *  3. Forward pass      – output = (input @ B @ A) × scaling
  Confidence: band=very_high; score=0.99
- Line 60: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: * @param fan_in  Input feature count (used to compute the uniform bound)
  Confidence: band=very_high; score=0.99
- Line 316: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const std::vector<float>& input,
  Confidence: band=very_high; score=0.99
- Line 324: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (input.size() != batch_size * e.in_dim) {
  Confidence: band=very_high; score=0.99
- Line 326: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: oss << "LoRAAdapter::forward: input size mismatch for layer '" << layer_name
  Confidence: band=very_high; score=0.99
- Line 329: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: << "), got " << input.size() << ")";
  Confidence: band=very_high; score=0.99
- Line 333: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Step 1: hidden = input @ B  →  (batch_size × in_dim) @ (in_dim × rank)
  Confidence: band=very_high; score=0.99
- Line 335: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::vector<float> hidden = detail::matmul(input,  batch_size, e.in_dim,
  Confidence: band=very_high; score=0.99
- Line 445: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const std::vector<float>& input,
  Confidence: band=very_high; score=0.99
- Line 447: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return impl_->forward(layer_name, input, batch_size);
  Confidence: band=very_high; score=0.99
- Line 19: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: *  3. Forward pass      – output = (input @ B @ A) × scaling
  Confidence: band=very_high; score=0.9
- Line 60: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: * @param fan_in  Input feature count (used to compute the uniform bound)
  Confidence: band=very_high; score=0.9
- Line 103: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (a == 0.0f) continue;  // skip zero multiplication (common at init)
  Confidence: band=very_high; score=0.9
- Line 316: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const std::vector<float>& input,
  Confidence: band=very_high; score=0.9
- Line 324: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (input.size() != batch_size * e.in_dim) {
  Confidence: band=very_high; score=0.9
- Line 326: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: oss << "LoRAAdapter::forward: input size mismatch for layer '" << layer_name
  Confidence: band=very_high; score=0.9
- Line 329: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: << "), got " << input.size() << ")";
  Confidence: band=very_high; score=0.9
- Line 333: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Step 1: hidden = input @ B  →  (batch_size × in_dim) @ (in_dim × rank)
  Confidence: band=very_high; score=0.9
- Line 335: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::vector<float> hidden = detail::matmul(input,  batch_size, e.in_dim,
  Confidence: band=very_high; score=0.9
- Line 445: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const std::vector<float>& input,
  Confidence: band=very_high; score=0.9
- Line 447: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return impl_->forward(layer_name, input, batch_size);
  Confidence: band=very_high; score=0.9

### src/training/training_pipeline.cpp
Total findings: 19

- Line 182: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: stats.quality_issues_found = qr.missing_input + qr.missing_output
  Confidence: band=very_high; score=0.99
- Line 229: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: stats.selection_input_count    = sel.audit_entry.input_sample_count;
  Confidence: band=very_high; score=0.99
- Line 232: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: sel.audit_entry.input_sample_count - sel.selected_samples.size();
  Confidence: band=very_high; score=0.99
- Line 323: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: //     RETURN {id: sample._key, text: CONCAT(sample.input, " ", sample.output)}
  Confidence: band=very_high; score=0.99
- Line 361: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: report.missing_input    = 0;
  Confidence: band=very_high; score=0.99
- Line 182: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: stats.quality_issues_found = qr.missing_input + qr.missing_output
  Confidence: band=very_high; score=0.9
- Line 229: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: stats.selection_input_count    = sel.audit_entry.input_sample_count;
  Confidence: band=very_high; score=0.9
- Line 232: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: sel.audit_entry.input_sample_count - sel.selected_samples.size();
  Confidence: band=very_high; score=0.9
- Line 323: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: //     RETURN {id: sample._key, text: CONCAT(sample.input, " ", sample.output)}
  Confidence: band=very_high; score=0.9
- Line 352: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: //     missing_input  = SUM(sample.input  == null ? 1 : 0)
  Confidence: band=very_high; score=0.9
- Line 355: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: //   RETURN {missing_input, missing_output, low_conf}
  Confidence: band=very_high; score=0.9
- Line 520: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: result.best_val_loss = (best_val_loss == std::numeric_limits<double>::max())
  Confidence: band=very_high; score=0.9
- Line 150: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: prov_records.push_back(std::move(rec));
  Confidence: band=high; score=0.74
- Line 445: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: trials.push_back({r, lr});
  Confidence: band=high; score=0.74
- Line 445: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: trials.push_back({r, lr});
  Confidence: band=high; score=0.74
- Line 668: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: by_category[s.category].push_back(s);
  Confidence: band=high; score=0.74
- Line 693: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: blocks.push_back({yi, 1});
  Confidence: band=high; score=0.74
- Line 693: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: blocks.push_back({yi, 1});
  Confidence: band=high; score=0.74
- Line 767: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.thresholds.push_back(std::move(entry));
  Confidence: band=high; score=0.74

### src/training/auto_labeler.cpp
Total findings: 17

- Line 749: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Structured input/output pair for LoRA fine-tuning
  Confidence: band=very_high; score=0.99
- Line 750: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: sample.input  = "Analyze the legal modality in: \"" + text + "\"";
  Confidence: band=very_high; score=0.99
- Line 749: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Structured input/output pair for LoRA fine-tuning
  Confidence: band=very_high; score=0.9
- Line 750: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: sample.input  = "Analyze the legal modality in: \"" + text + "\"";
  Confidence: band=very_high; score=0.9
- Line 248: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: samples.push_back(std::move(sample));
  Confidence: band=high; score=0.74
- Line 282: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: samples.push_back(std::move(sample));
  Confidence: band=high; score=0.74
- Line 293: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: LabelingStats labelQuery(const std::string& aql_query, LabelingCallback callback) {
  Confidence: band=high; score=0.74
- Line 384: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string getFetchAllQuery() const {
  Confidence: band=high; score=0.74
- Line 389: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::string getBatchInsertQuery() const {
  Confidence: band=high; score=0.74
- Line 430: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(entity.getPrimaryKey());
  Confidence: band=high; score=0.74
- Line 467: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(entity.getPrimaryKey());
  Confidence: band=high; score=0.74
- Line 476: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: std::vector<std::string> executeAqlQuery(const std::string& aql) const {
  Confidence: band=high; score=0.74
- Line 486: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: ids.push_back(item.get<std::string>());
  Confidence: band=high; score=0.74
- Line 518: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '\\': safe_id += "\\\\"; break;
  Confidence: band=high; score=0.74
- Line 518: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '\\': safe_id += "\\\\"; break;
  Confidence: band=high; score=0.74
- Line 688: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: static bool isReadOnlyAqlQuery(const std::string& aql) {
  Confidence: band=high; score=0.74
- Line 695: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: normalized.push_back(static_cast<char>(std::toupper(c)));
  Confidence: band=high; score=0.74

### src/training/ada_lora_adapter.cpp
Total findings: 15

- Line 315: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const std::vector<float>& input,
  Confidence: band=very_high; score=0.99
- Line 322: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (input.size() != batch_size * D_in)
  Confidence: band=very_high; score=0.99
- Line 323: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: throw std::invalid_argument("Input size mismatch");
  Confidence: band=very_high; score=0.99
- Line 327: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // hidden = input @ B[:, :r]   → batch_size × r
  Confidence: band=very_high; score=0.99
- Line 333: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: acc += input[b * D_in + d] * lay.B[d * lay.max_rank + i];
  Confidence: band=very_high; score=0.99
- Line 461: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const std::vector<float>& input,
  Confidence: band=very_high; score=0.99
- Line 463: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return impl_->forward(layer_name, input, batch_size);
  Confidence: band=very_high; score=0.99
- Line 315: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const std::vector<float>& input,
  Confidence: band=very_high; score=0.9
- Line 322: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (input.size() != batch_size * D_in)
  Confidence: band=very_high; score=0.9
- Line 323: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: throw std::invalid_argument("Input size mismatch");
  Confidence: band=very_high; score=0.9
- Line 327: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // hidden = input @ B[:, :r]   → batch_size × r
  Confidence: band=very_high; score=0.9
- Line 333: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: acc += input[b * D_in + d] * lay.B[d * lay.max_rank + i];
  Confidence: band=very_high; score=0.9
- Line 461: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const std::vector<float>& input,
  Confidence: band=very_high; score=0.9
- Line 463: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return impl_->forward(layer_name, input, batch_size);
  Confidence: band=very_high; score=0.9
- Line 279: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: stats.push_back(s);
  Confidence: band=high; score=0.74

### src/training/modality_parser.cpp
Total findings: 8

- Line 79: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: lines.push_back(std::move(line));
  Confidence: band=high; score=0.74
- Line 275: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: clean_text += '\n';
  Confidence: band=high; score=0.74
- Line 275: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: clean_text += '\n';
  Confidence: band=high; score=0.74
- Line 275: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: clean_text += '\n';
  Confidence: band=high; score=0.74
- Line 583: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (auto& s : clauses) result.samples.push_back(std::move(s));
  Confidence: band=high; score=0.74
- Line 588: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (auto& s : tables) result.samples.push_back(std::move(s));
  Confidence: band=high; score=0.74
- Line 593: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (auto& s : citations) result.samples.push_back(std::move(s));
  Confidence: band=high; score=0.74
- Line 627: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: for (auto& s : res.samples) out_samples.push_back(std::move(s));
  Confidence: band=high; score=0.74

### src/training/lora_adapter_merger.cpp
Total findings: 7

- Line 322: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (v == 0.0f) continue;
  Confidence: band=very_high; score=0.9
- Line 232: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: descs.push_back({adapters[i], lname, weights[i]});
  Confidence: band=high; score=0.74
- Line 232: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: descs.push_back({adapters[i], lname, weights[i]});
  Confidence: band=high; score=0.74
- Line 232: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: descs.push_back({adapters[i], lname, weights[i]});
  Confidence: band=high; score=0.74
- Line 386: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: descs.push_back({a, lname, 1.0f});
  Confidence: band=high; score=0.74
- Line 386: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: descs.push_back({a, lname, 1.0f});
  Confidence: band=high; score=0.74
- Line 386: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: descs.push_back({a, lname, 1.0f});
  Confidence: band=high; score=0.74

### src/training/knowledge_graph_enricher.cpp
Total findings: 6

- Line 68: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "FILTER sample.input != null "
  Confidence: band=very_high; score=0.99
- Line 68: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "FILTER sample.input != null "
  Confidence: band=very_high; score=0.9
- Line 266: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: context.similar_documents.push_back(doc_id);
  Confidence: band=high; score=0.74
- Line 291: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: EnrichmentStats enrichQuery(const std::string& aql_query, EnrichmentCallback callback) {
  Confidence: band=high; score=0.74
- Line 436: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: similar.emplace_back(r.pk, distanceToSimilarityScore(r.distance));
  Confidence: band=high; score=0.74
- Line 443: severity=MEDIUM; category=observability; pattern=missing_latency_metric
  Description: No latency measurement for operation
  Context: void setCustomQuery(const std::string& query_name, const std::string& aql_query) {
  Confidence: band=high; score=0.74

### src/training/provenance_tracker.cpp
Total findings: 5

- Line 222: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: root.parents.push_back(std::move(node));
  Confidence: band=high; score=0.74
- Line 273: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: sample_node.parents.push_back(std::move(doc_node));
  Confidence: band=high; score=0.74
- Line 342: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, ProvenanceRecord> store_;
  Confidence: band=medium; score=0.66
- Line 368: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '\\': out += "\\\\"; break;
  Confidence: band=high; score=0.74
- Line 368: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '\\': out += "\\\\"; break;
  Confidence: band=high; score=0.74

### src/training/adalora_tt_bridge.cpp
Total findings: 4

- Line 379: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto& layer : query_exp.layers) {
  Confidence: band=very_high; score=0.9
- Line 260: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.layers.push_back(exportLayer(adapter, ls.layer_name));
  Confidence: band=high; score=0.74
- Line 377: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: std::unordered_map<std::string, SimilarAdapter> best_by_adapter;
  Confidence: band=medium; score=0.66
- Line 402: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: out.push_back(kv.second);
  Confidence: band=high; score=0.74

### src/training/database_domain_auto_labeler.cpp
Total findings: 1

- Line 178: severity=MEDIUM; category=performance; pattern=string_concat_loop
  Description: String concatenation in loop (use std::stringstream)
  Context: case '"':  out += "\\\""; break;
  Confidence: band=high; score=0.74

### src/training/examples/database_optimizer_labeler.cpp
Total findings: 1

- Line 112: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: labeled_samples.push_back(std::move(sample));
  Confidence: band=high; score=0.74

### src/training/adapter_serving.cpp
Total findings: 0


### src/training/lora_checkpoint_manager.cpp
Total findings: 0

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
