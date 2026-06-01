# acceleration Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: acceleration
- Generated: 2026-05-31 08:50:11
- Status: Critical Findings Present
- Total Findings: 731
- Actionable Findings (Critical + High): 486
- Affected Files: 26

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 97 |
| High | 389 |
| Medium | 245 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| llm_ai_safety | 140 |
| raii | 120 |
| audit_logging | 99 |
| container | 65 |
| memory | 63 |
| performance_patterns | 63 |
| exception_safety | 50 |
| gpu_memory_safety | 32 |
| concurrency | 22 |
| reliability | 19 |
| performance | 17 |
| legacy_duplication | 9 |
| platform | 9 |
| observability | 7 |
| security | 7 |
| determinism | 5 |
| type_conversion | 2 |
| uninitialized | 2 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/acceleration/ai_hardware_dispatcher.cpp | 127 | 39 | 77 | 11 | 0 |
| src/acceleration/cuda_backend.cpp | 112 | 8 | 95 | 9 | 0 |
| src/acceleration/plugin_security.cpp | 101 | 9 | 8 | 84 | 0 |
| src/acceleration/faiss_gpu_backend.cpp | 95 | 28 | 47 | 20 | 0 |
| src/acceleration/graphics_backends.cpp | 75 | 0 | 51 | 20 | 4 |
| src/acceleration/oneapi_backend.cpp | 23 | 8 | 6 | 9 | 0 |
| src/acceleration/hip_backend.cpp | 21 | 0 | 17 | 4 | 0 |
| src/acceleration/vec_knn.cpp | 16 | 2 | 8 | 6 | 0 |
| src/acceleration/geo_acceleration_bridge.cpp | 15 | 0 | 3 | 12 | 0 |
| src/acceleration/plugin_loader.cpp | 13 | 1 | 6 | 6 | 0 |
| src/acceleration/vllm_resource_manager.cpp | 12 | 2 | 8 | 2 | 0 |
| src/acceleration/multi_gpu_backend.cpp | 11 | 0 | 1 | 10 | 0 |
| src/acceleration/opencl_backend.cpp | 11 | 0 | 7 | 3 | 1 |
| src/acceleration/backend_registry.cpp | 10 | 0 | 6 | 4 | 0 |
| src/acceleration/cpu_backend.cpp | 10 | 0 | 2 | 8 | 0 |
| src/acceleration/zluda_backend.cpp | 10 | 0 | 7 | 3 | 0 |
| src/acceleration/directx_backend_full.cpp | 9 | 0 | 6 | 3 | 0 |
| src/acceleration/vulkan_backend_full.cpp | 9 | 0 | 8 | 1 | 0 |
| src/acceleration/cpu_backend_mt.cpp | 8 | 0 | 6 | 2 | 0 |
| src/acceleration/cpu_backend_tbb.cpp | 8 | 0 | 6 | 2 | 0 |
| src/acceleration/device_manager.cpp | 8 | 0 | 4 | 4 | 0 |
| src/acceleration/shader_integrity.cpp | 8 | 0 | 4 | 4 | 0 |
| src/acceleration/nccl_vector_backend.cpp | 7 | 0 | 2 | 5 | 0 |
| src/acceleration/rccl_vector_backend.cpp | 7 | 0 | 2 | 5 | 0 |
| src/acceleration/compute_backend.cpp | 3 | 0 | 0 | 3 | 0 |
| src/acceleration/tensor_core_matmul.cpp | 2 | 0 | 2 | 0 | 0 |

## Full Scanner Findings

### src/acceleration/ai_hardware_dispatcher.cpp
Total findings: 127

- Line 19: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: *   AiHardwareDispatcher::dispatch(task, model, input)
  Confidence: band=very_high; score=0.99
- Line 599: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (!req.input_data || req.input_elements == 0) {
  Confidence: band=very_high; score=0.99
- Line 600: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return makeError(BackendType::NPU_APPLE, "Invalid input: null or empty");
  Confidence: band=very_high; score=0.99
- Line 627: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // MLMultiArray from req.input_data, run prediction, and extract results.
  Confidence: band=very_high; score=0.99
- Line 645: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (!req.input_data || req.input_elements == 0) {
  Confidence: band=very_high; score=0.99
- Line 646: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return makeError(BackendType::NPU_INTEL, "Invalid input: null or empty");
  Confidence: band=very_high; score=0.99
- Line 658: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Bind input tensor
  Confidence: band=very_high; score=0.99
- Line 659: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: ov::Shape shape(req.input_shape.begin(), req.input_shape.end());
  Confidence: band=very_high; score=0.99
- Line 660: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: ov::Tensor input_tensor(ov::element::f32, shape, const_cast<float *>(req.input_data));
  Confidence: band=very_high; score=0.99
- Line 661: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: infer_req.set_input_tensor(input_tensor);
  Confidence: band=very_high; score=0.99
- Line 691: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (!req.input_data || req.input_elements == 0) {
  Confidence: band=very_high; score=0.99
- Line 692: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return makeError(BackendType::NPU_QUALCOMM, "Invalid input: null or empty");
  Confidence: band=very_high; score=0.99
- Line 712: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (!req.input_data || req.input_elements == 0) {
  Confidence: band=very_high; score=0.99
- Line 713: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return makeError(BackendType::NPU_ARM, "Invalid input: null or empty");
  Confidence: band=very_high; score=0.99
- Line 730: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (!req.input_data || req.input_elements == 0) {
  Confidence: band=very_high; score=0.99
- Line 731: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return makeError(BackendType::NNAPI, "Invalid input: null or empty");
  Confidence: band=very_high; score=0.99
- Line 762: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (req.input_data == nullptr || req.input_elements == 0) {
  Confidence: band=very_high; score=0.99
- Line 763: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return makeError(BackendType::ONNX_RUNTIME, "Invalid input: null or empty");
  Confidence: band=very_high; score=0.99
- Line 816: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Build input shape
  Confidence: band=very_high; score=0.99
- Line 817: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::vector<int64_t> shape(req.input_shape.begin(), req.input_shape.end());
  Confidence: band=very_high; score=0.99
- Line 823: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Input tensor
  Confidence: band=very_high; score=0.99
- Line 824: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: OrtValue *input_tensor = nullptr;
  Confidence: band=very_high; score=0.99
- Line 825: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: ort->CreateTensorWithDataAsOrtValue(mem_info, const_cast<float *>(req.input_data),
  Confidence: band=very_high; score=0.99
- Line 826: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: req.input_elements * sizeof(float), shape.data(), shape.size(),
  Confidence: band=very_high; score=0.99
- Line 827: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, &input_tensor);
  Confidence: band=very_high; score=0.99
- Line 829: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Query input/output names
  Confidence: band=very_high; score=0.99
- Line 832: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t input_count = 0, output_count = 0;
  Confidence: band=very_high; score=0.99
- Line 833: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: ort->SessionGetInputCount(session, &input_count);
  Confidence: band=very_high; score=0.99
- Line 836: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::vector<char *> input_names_raw(input_count);
  Confidence: band=very_high; score=0.99
- Line 838: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (size_t i = 0; i < input_count; ++i) {
  Confidence: band=very_high; score=0.99
- Line 839: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: ort->SessionGetInputName(session, i, alloc, &input_names_raw[i]);
  Confidence: band=very_high; score=0.99
- Line 845: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const char *const *in_names  = const_cast<const char *const *>(input_names_raw.data());
  Confidence: band=very_high; score=0.99
- Line 850: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: status = ort->Run(session, nullptr, in_names, &input_tensor, input_count, out_names, output_count,
  Confidence: band=very_high; score=0.99
- Line 857: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: ort->ReleaseValue(input_tensor);
  Confidence: band=very_high; score=0.99
- Line 891: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: ort->ReleaseValue(input_tensor);
  Confidence: band=very_high; score=0.99
- Line 921: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (req.input_data == nullptr || req.input_elements == 0) {
  Confidence: band=very_high; score=0.99
- Line 922: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return makeError(BackendType::CPU, "Invalid input: null or empty");
  Confidence: band=very_high; score=0.99
- Line 933: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: result.output.assign(req.input_data, req.input_data + req.input_elements);
  Confidence: band=very_high; score=0.99
- Line 934: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: result.output_shape = req.input_shape;
  Confidence: band=very_high; score=0.99
- Line 12: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: * Routes AI inference workloads to the best available AI accelerator using a
  Confidence: band=very_high; score=0.9
- Line 14: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: * and specialises in AI inference rather than general ANN / geospatial / graph
  Confidence: band=very_high; score=0.9
- Line 19: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: *   AiHardwareDispatcher::dispatch(task, model, input)
  Confidence: band=very_high; score=0.9
- Line 37: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: *   AiHardwareDispatcher::dispatch()   — route inference task through priority chain
  Confidence: band=very_high; score=0.9
- Line 237: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Inference dispatch
  Confidence: band=very_high; score=0.9
- Line 254: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Check precision compatibility
  Confidence: band=high; score=0.8
- Line 270: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult result = runOn(cap.type, req);
  Confidence: band=very_high; score=0.9
- Line 278: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult err;
  Confidence: band=very_high; score=0.9
- Line 280: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: err.error   = "All AI hardware backends exhausted — no successful inference path";
  Confidence: band=very_high; score=0.9
- Line 285: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult AiHardwareDispatcher::runOn(BackendType backend, AiInferenceRequest &req) {
  Confidence: band=very_high; score=0.9
- Line 564: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: //   3. Returns a filled AiInferenceResult (success or error).
  Confidence: band=very_high; score=0.9
- Line 569: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: static AiInferenceResult makeError(BackendType bt, const std::string &msg) {
  Confidence: band=very_high; score=0.9
- Line 570: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult r;
  Confidence: band=very_high; score=0.9
- Line 577: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult AiHardwareDispatcher::dispatchAppleANE([[maybe_unused]] AiInferenceRequest &req) {
  Confidence: band=very_high; score=0.9
- Line 599: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (!req.input_data || req.input_elements == 0) {
  Confidence: band=very_high; score=0.9
- Line 600: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return makeError(BackendType::NPU_APPLE, "Invalid input: null or empty");
  Confidence: band=very_high; score=0.9
- Line 614: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Production Delta: Apple Neural Engine (ANE) / Core ML inference is
  Confidence: band=very_high; score=0.9
- Line 627: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // MLMultiArray from req.input_data, run prediction, and extract results.
  Confidence: band=very_high; score=0.9
- Line 628: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult result;
  Confidence: band=very_high; score=0.9
- Line 643: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult AiHardwareDispatcher::dispatchIntelNPU([[maybe_unused]] AiInferenceRequest &req) {
  Confidence: band=very_high; score=0.9
- Line 645: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (!req.input_data || req.input_elements == 0) {
  Confidence: band=very_high; score=0.9
- Line 646: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return makeError(BackendType::NPU_INTEL, "Invalid input: null or empty");
  Confidence: band=very_high; score=0.9
- Line 656: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto infer_req = compiled.create_infer_request();
  Confidence: band=very_high; score=0.9
- Line 658: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Bind input tensor
  Confidence: band=very_high; score=0.9
- Line 659: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: ov::Shape shape(req.input_shape.begin(), req.input_shape.end());
  Confidence: band=very_high; score=0.9
- Line 660: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: ov::Tensor input_tensor(ov::element::f32, shape, const_cast<float *>(req.input_data));
  Confidence: band=very_high; score=0.9
- Line 661: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: infer_req.set_input_tensor(input_tensor);
  Confidence: band=very_high; score=0.9
- Line 661: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: infer_req.set_input_tensor(input_tensor);
  Confidence: band=very_high; score=0.9
- Line 663: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: infer_req.infer();
  Confidence: band=very_high; score=0.9
- Line 665: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto output_tensor   = infer_req.get_output_tensor();
  Confidence: band=very_high; score=0.9
- Line 669: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult result;
  Confidence: band=very_high; score=0.9
- Line 682: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: return makeError(BackendType::NPU_INTEL, std::string("OpenVINO NPU inference failed: ") + e.what());
  Confidence: band=very_high; score=0.9
- Line 689: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult AiHardwareDispatcher::dispatchQualcommQNN([[maybe_unused]] AiInferenceRequest &req) {
  Confidence: band=very_high; score=0.9
- Line 691: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (!req.input_data || req.input_elements == 0) {
  Confidence: band=very_high; score=0.9
- Line 692: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return makeError(BackendType::NPU_QUALCOMM, "Invalid input: null or empty");
  Confidence: band=very_high; score=0.9
- Line 698: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult result;
  Confidence: band=very_high; score=0.9
- Line 710: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult AiHardwareDispatcher::dispatchArmEthos([[maybe_unused]] AiInferenceRequest &req) {
  Confidence: band=very_high; score=0.9
- Line 712: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (!req.input_data || req.input_elements == 0) {
  Confidence: band=very_high; score=0.9
- Line 713: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return makeError(BackendType::NPU_ARM, "Invalid input: null or empty");
  Confidence: band=very_high; score=0.9
- Line 717: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult result;
  Confidence: band=very_high; score=0.9
- Line 728: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult AiHardwareDispatcher::dispatchNNAPI([[maybe_unused]] AiInferenceRequest &req) {
  Confidence: band=very_high; score=0.9
- Line 730: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (!req.input_data || req.input_elements == 0) {
  Confidence: band=very_high; score=0.9
- Line 731: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return makeError(BackendType::NNAPI, "Invalid input: null or empty");
  Confidence: band=very_high; score=0.9
- Line 747: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult result;
  Confidence: band=very_high; score=0.9
- Line 760: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult AiHardwareDispatcher::dispatchOnnxRuntime([[maybe_unused]] AiInferenceRequest &req) {
  Confidence: band=very_high; score=0.9
- Line 762: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (req.input_data == nullptr || req.input_elements == 0) {
  Confidence: band=very_high; score=0.9
- Line 763: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return makeError(BackendType::ONNX_RUNTIME, "Invalid input: null or empty");
  Confidence: band=very_high; score=0.9
- Line 770: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult result;
  Confidence: band=very_high; score=0.9
- Line 816: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Build input shape
  Confidence: band=very_high; score=0.9
- Line 817: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::vector<int64_t> shape(req.input_shape.begin(), req.input_shape.end());
  Confidence: band=very_high; score=0.9
- Line 823: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Input tensor
  Confidence: band=very_high; score=0.9
- Line 824: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: OrtValue *input_tensor = nullptr;
  Confidence: band=very_high; score=0.9
- Line 825: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: ort->CreateTensorWithDataAsOrtValue(mem_info, const_cast<float *>(req.input_data),
  Confidence: band=very_high; score=0.9
- Line 825: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ort->CreateTensorWithDataAsOrtValue(mem_info, const_cast<float *>(req.input_data),
- Line 826: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: req.input_elements * sizeof(float), shape.data(), shape.size(),
  Confidence: band=very_high; score=0.9
- Line 827: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, &input_tensor);
  Confidence: band=very_high; score=0.9
- Line 829: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Query input/output names
  Confidence: band=very_high; score=0.9
- Line 832: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t input_count = 0, output_count = 0;
  Confidence: band=very_high; score=0.9
- Line 833: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: ort->SessionGetInputCount(session, &input_count);
  Confidence: band=very_high; score=0.9
- Line 836: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::vector<char *> input_names_raw(input_count);
  Confidence: band=very_high; score=0.9
- Line 838: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: for (size_t i = 0; i < input_count; ++i) {
  Confidence: band=very_high; score=0.9
- Line 839: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: ort->SessionGetInputName(session, i, alloc, &input_names_raw[i]);
  Confidence: band=very_high; score=0.9
- Line 845: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const char *const *in_names  = const_cast<const char *const *>(input_names_raw.data());
  Confidence: band=very_high; score=0.9
- Line 848: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Run inference
  Confidence: band=very_high; score=0.9
- Line 850: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: status = ort->Run(session, nullptr, in_names, &input_tensor, input_count, out_names, output_count,
  Confidence: band=very_high; score=0.9
- Line 850: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: status = ort->Run(session, nullptr, in_names, &input_tensor, input_count, out_names, output_count,
- Line 850: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: status = ort->Run(session, nullptr, in_names, &input_tensor, input_count, out_names, output_count,
- Line 876: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ort->GetDimensions(shape_info, out_shape.data(), rank);
- Line 911: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult AiHardwareDispatcher::dispatchGpuFallback(AiInferenceRequest &req) {
  Confidence: band=very_high; score=0.9
- Line 914: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Here we provide a graceful fallback path to CPU when no GPU inference
  Confidence: band=very_high; score=0.9
- Line 920: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult AiHardwareDispatcher::dispatchCpuFallback(AiInferenceRequest &req) {
  Confidence: band=very_high; score=0.9
- Line 921: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (req.input_data == nullptr || req.input_elements == 0) {
  Confidence: band=very_high; score=0.9
- Line 922: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return makeError(BackendType::CPU, "Invalid input: null or empty");
  Confidence: band=very_high; score=0.9
- Line 927: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // embedding or run the model via a separate thread-pool).  Real inference
  Confidence: band=very_high; score=0.9
- Line 929: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult result;
  Confidence: band=very_high; score=0.9
- Line 933: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: result.output.assign(req.input_data, req.input_data + req.input_elements);
  Confidence: band=very_high; score=0.9
- Line 934: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: result.output_shape = req.input_shape;
  Confidence: band=very_high; score=0.9
- Line 152: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: caps.push_back(probeAppleANE());
- Line 153: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: caps.push_back(probeIntelNPU());
- Line 154: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: caps.push_back(probeQualcommQNN());
- Line 155: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: caps.push_back(probeArmEthos());
- Line 156: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: caps.push_back(probeNNAPI());
- Line 157: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: caps.push_back(probeOnnxRuntime());
- Line 158: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: caps.push_back(probeGpuFallback());
- Line 159: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: caps.push_back(probeCpuFallback());
- Line 663: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: infer_req.infer();
  Confidence: band=high; score=0.74
- Line 674: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.output_shape.push_back(static_cast<int64_t>(dim));
  Confidence: band=high; score=0.74
- Line 675: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.output_shape.push_back(static_cast<int64_t>(dim));

### src/acceleration/cuda_backend.cpp
Total findings: 112

- Line 347: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto hnswResults = hnswEngine_->batchSearch(queries, numQueries, static_cast<uint32_t>(k), ef);
- Line 456: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto hnswResults = hnswEngine_->batchSearch(queries, numQueries, static_cast<uint32_t>(k));
- Line 648: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator lru may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto lru = entries_.begin();
- Line 649: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = entries_.begin(); it != entries_.end(); ++it) {
- Line 984: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator lru may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto lru = entries_.begin();
- Line 985: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = entries_.begin(); it != entries_.end(); ++it) {
- Line 1069: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator lru may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto lru = entries_.begin();
- Line 1070: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = entries_.begin(); it != entries_.end(); ++it) {
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
- Line 177: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "CUDA: Created low-priority stream for vLLM co-location (priority=" << leastPriority << ")"
  Confidence: band=very_high; score=0.9
- Line 205: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "CUDA Backend initialized successfully:" << std::endl;
  Confidence: band=very_high; score=0.9
- Line 206: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Device: " << prop.name << std::endl;
  Confidence: band=very_high; score=0.9
- Line 207: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Compute Capability: " << prop.major << "." << prop.minor << std::endl;
  Confidence: band=very_high; score=0.9
- Line 208: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Global Memory: " << (prop.totalGlobalMem / (1024 * 1024 * 1024)) << " GB" << std::endl;
  Confidence: band=very_high; score=0.9
- Line 209: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Multiprocessors: " << prop.multiProcessorCount << std::endl;
  Confidence: band=very_high; score=0.9
- Line 210: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  CUDA Runtime: " << (runtimeVersion / 1000) << "." << ((runtimeVersion % 100) / 10) << std::endl;
  Confidence: band=very_high; score=0.9
- Line 212: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  vLLM Co-Location: ENABLED (low-priority stream, max " << THEMIS_MAX_GPU_VRAM_MB << " MB VRAM)"
  Confidence: band=very_high; score=0.9
- Line 219: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Occupancy-tuned vector block dim: " << vecBlockDim << "x" << vecBlockDim << std::endl;
  Confidence: band=very_high; score=0.9
- Line 284: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "CUDAVectorBackend: clamping maxBatchSize from " << maxBatchSize_ << " to "
  Confidence: band=very_high; score=0.9
- Line 736: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_queries.get(), 0, querySize);
  Confidence: band=very_high; score=0.9
- Line 737: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_vectors.get(), 0, vectorSize);
  Confidence: band=very_high; score=0.9
- Line 738: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_distances.get(), 0, distanceSize);
  Confidence: band=very_high; score=0.9
- Line 739: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_topkIndices.get(), 0, topkIdxSize);
  Confidence: band=very_high; score=0.9
- Line 740: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_topkDistances.get(), 0, topkDistSize);
  Confidence: band=very_high; score=0.9
- Line 741: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: // cudaMemset is synchronous: it blocks the host until the fill is
  Confidence: band=very_high; score=0.9
- Line 835: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyAsync(entry->d_queries.get(), queries, querySize, cudaMemcpyHostToDevice, mainStream);
  Confidence: band=very_high; score=0.9
- Line 836: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyAsync(entry->d_vectors.get(), vectors, vectorSize, cudaMemcpyHostToDevice, mainStream);
  Confidence: band=very_high; score=0.9
- Line 852: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyAsync(topkIndices.data(), entry->d_topkIndices.get(), topkIdxSize, cudaMemcpyDeviceToHost,
  Confidence: band=very_high; score=0.9
- Line 852: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: cudaMemcpyAsync(topkIndices.data(), entry->d_topkIndices.get(), topkIdxSize, cudaMemcpyDeviceToHost,
- Line 854: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyAsync(topkDistances.data(), entry->d_topkDistances.get(), topkDistSize, cudaMemcpyDeviceToHost,
  Confidence: band=very_high; score=0.9
- Line 854: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: cudaMemcpyAsync(topkDistances.data(), entry->d_topkDistances.get(), topkDistSize, cudaMemcpyDeviceTo
- Line 1157: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "CUDA Graph Backend: occupancy-tuned BFS block dim = " << bfsBlockDim << std::endl;
  Confidence: band=very_high; score=0.9
- Line 1253: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_adjacency.get(), 0, adjSize);
  Confidence: band=very_high; score=0.9
- Line 1254: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_startVertices.get(), 0, svSize);
  Confidence: band=very_high; score=0.9
- Line 1255: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_frontier_a.get(), 0, frontierSz);
  Confidence: band=very_high; score=0.9
- Line 1256: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_frontier_b.get(), 0, frontierSz);
  Confidence: band=very_high; score=0.9
- Line 1257: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_visited.get(), 0, frontierSz);
  Confidence: band=very_high; score=0.9
- Line 1258: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_depths.get(), 0, frontierSz);
  Confidence: band=very_high; score=0.9
- Line 1259: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_result_vertices.get(), 0, resultsSz);
  Confidence: band=very_high; score=0.9
- Line 1260: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_result_sizes.get(), 0, sizesSz);
  Confidence: band=very_high; score=0.9
- Line 1342: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Replay: copy inputs → device, launch graph, copy results ← device
  Confidence: band=very_high; score=0.9
- Line 1344: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyAsync(entry->d_adjacency.get(), adjacency, adjSize, cudaMemcpyHostToDevice, mainStream);
  Confidence: band=very_high; score=0.9
- Line 1345: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyAsync(entry->d_startVertices.get(), startVertices, svSize, cudaMemcpyHostToDevice, mainStream);
  Confidence: band=very_high; score=0.9
- Line 1359: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyAsync(h_result_vertices.data(), entry->d_result_vertices.get(), resultsSz, cudaMemcpyDeviceToHost,
  Confidence: band=very_high; score=0.9
- Line 1359: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: cudaMemcpyAsync(h_result_vertices.data(), entry->d_result_vertices.get(), resultsSz, cudaMemcpyDevic
- Line 1361: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyAsync(h_result_sizes.data(), entry->d_result_sizes.get(), sizesSz, cudaMemcpyDeviceToHost,
  Confidence: band=very_high; score=0.9
- Line 1361: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: cudaMemcpyAsync(h_result_sizes.data(), entry->d_result_sizes.get(), sizesSz, cudaMemcpyDeviceToHost,
- Line 1457: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_adjacency.get(), 0, adjSize);
  Confidence: band=very_high; score=0.9
- Line 1458: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_weights.get(), 0, wgtSize);
  Confidence: band=very_high; score=0.9
- Line 1459: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_startVertices.get(), 0, svSize);
  Confidence: band=very_high; score=0.9
- Line 1460: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_distances.get(), 0, distSize);
  Confidence: band=very_high; score=0.9
- Line 1461: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_predecessors.get(), 0, predSize);
  Confidence: band=very_high; score=0.9
- Line 1529: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyAsync(entry->d_adjacency.get(), adjacency, adjSize, cudaMemcpyHostToDevice, mainStream);
  Confidence: band=very_high; score=0.9
- Line 1530: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyAsync(entry->d_weights.get(), weights, wgtSize, cudaMemcpyHostToDevice, mainStream);
  Confidence: band=very_high; score=0.9
- Line 1531: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyAsync(entry->d_startVertices.get(), startVertices, svSize, cudaMemcpyHostToDevice, mainStream);
  Confidence: band=very_high; score=0.9
- Line 1545: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyAsync(h_distances.data(), entry->d_distances.get(), distSize, cudaMemcpyDeviceToHost, mainStream);
  Confidence: band=very_high; score=0.9
- Line 1545: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: cudaMemcpyAsync(h_distances.data(), entry->d_distances.get(), distSize, cudaMemcpyDeviceToHost, main
- Line 1546: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyAsync(h_predecessors.data(), entry->d_predecessors.get(), predSize, cudaMemcpyDeviceToHost,
  Confidence: band=very_high; score=0.9
- Line 1546: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: cudaMemcpyAsync(h_predecessors.data(), entry->d_predecessors.get(), predSize, cudaMemcpyDeviceToHost
- Line 1685: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "CUDA Geo Backend initialized successfully:" << std::endl;
  Confidence: band=very_high; score=0.9
- Line 1686: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Device: " << prop.name << std::endl;
  Confidence: band=very_high; score=0.9
- Line 1690: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Occupancy-tuned geo block size: " << geoBlockSize << std::endl;
  Confidence: band=very_high; score=0.9
- Line 1815: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: const size_t resultSize = numPoints * sizeof(uint8_t);
- Line 355: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: row.emplace_back(static_cast<uint32_t>(r.id), r.score);
  Confidence: band=high; score=0.74
- Line 358: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(std::move(row));
- Line 462: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: row.emplace_back(static_cast<uint32_t>(r.id), r.score);
  Confidence: band=high; score=0.74
- Line 465: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: out.push_back(std::move(row));
- Line 549: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results[q].emplace_back(static_cast<uint32_t>(topkIndices[idx]), topkDistances[idx]);
  Confidence: band=high; score=0.74
- Line 871: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results[q].emplace_back(static_cast<uint32_t>(topkIndices[idx]), topkDistances[idx]);
  Confidence: band=high; score=0.74
- Line 1574: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: path.push_back(cur);
  Confidence: band=high; score=0.74
- Line 1574: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: path.push_back(cur);
  Confidence: band=high; score=0.74
- Line 1575: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: path.push_back(cur);

### src/acceleration/plugin_security.cpp
Total findings: 101

- Line 686: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != g_crl_cache.end() && std::chrono::system_clock::now() < it->second.expires_at) {
- Line 686: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != g_crl_cache.end() && std::chrono::system_clock::now() < it->second.expires_at) {
- Line 687: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: bool cached_result = !it->second.is_revoked;
- Line 865: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != g_ocsp_cache.end() && std::chrono::system_clock::now() < it->second.expires_at) {
- Line 865: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (it != g_ocsp_cache.end() && std::chrono::system_clock::now() < it->second.expires_at) {
- Line 866: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: bool cached_result = !it->second.is_revoked;
- Line 1286: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: X509 *cert             = d2i_X509(nullptr, &p, static_cast<long>(cert_data->size()));
- Line 1390: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const std::string &signing_cert = metadata->signature.signingCertificate;
- Line 2045: severity=CRITICAL; category=no_timeout
  Description: file_io without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: while ((n = read(pipefd[0], buf, sizeof(buf) - 1)) > 0) {
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Context: Array declared without initialization
  Confidence: band=high; score=0.81
- Line 86: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: buf->insert(buf->end(), ptr, ptr + size * nmemb);
- Line 620: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: for (int i = 0; ca_paths[i] != nullptr; i++) {
- Line 1090: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &event : events_) {
  Confidence: band=very_high; score=0.9
- Line 1147: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: j["events"] = json::array();
- Line 1286: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: X509 *cert             = d2i_X509(nullptr, &p, static_cast<long>(cert_data->size()));
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 163: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: OPENSSL_free(hex);
- Line 164: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: BN_free(bn);
- Line 192: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: outBytes.push_back(byte);
  Confidence: band=high; score=0.74
- Line 193: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: outBytes.push_back(byte);
- Line 277: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(mdctx);
- Line 286: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(mdctx);
- Line 294: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(mdctx);
- Line 297: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(mdctx);
- Line 524: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: BIO_free(bio);
- Line 536: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
- Line 543: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
- Line 550: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pubKey);
- Line 551: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
- Line 558: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pubKey);
- Line 559: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
- Line 577: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(mdctx);
- Line 578: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pubKey);
- Line 579: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
- Line 597: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: BIO_free(bio);
- Line 606: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
- Line 628: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_STORE_free(store);
- Line 629: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
- Line 637: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_STORE_free(store);
- Line 638: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
- Line 644: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_STORE_CTX_free(ctx);
- Line 645: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_STORE_free(store);
- Line 646: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
- Line 654: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_STORE_CTX_free(ctx);
- Line 655: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_STORE_free(store);
- Line 656: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
- Line 673: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: BIO_free(bio);
- Line 688: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
- Line 764: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_STORE_CTX_free(chain_ctx);
- Line 771: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(crl_issuer_key);
- Line 772: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_STORE_free(trust_store);
- Line 773: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_CRL_free(crl);
- Line 776: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(crl_issuer_key);
- Line 782: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_STORE_free(trust_store);
- Line 792: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_CRL_free(crl);
- Line 823: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_CRL_free(crl);
- Line 827: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: sk_DIST_POINT_pop_free(crldp, DIST_POINT_free);
- Line 836: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
- Line 852: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: BIO_free(bio);
- Line 867: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
- Line 901: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_STORE_CTX_free(chain_ctx);
- Line 928: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: OCSP_REQUEST_free(req);
- Line 934: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: OCSP_CERTID_free(certid);
- Line 935: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: OCSP_REQUEST_free(req);
- Line 942: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: OCSP_REQUEST_free(req);
- Line 949: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: OPENSSL_free(req_der);
- Line 965: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: OCSP_RESPONSE_free(resp);
- Line 970: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: OCSP_RESPONSE_free(resp);
- Line 981: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: OCSP_BASICRESP_free(basic);
- Line 988: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: OCSP_BASICRESP_free(basic);
- Line 998: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: OCSP_CERTID_free(lookup_id);
- Line 1004: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: OCSP_BASICRESP_free(basic);
- Line 1032: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: OCSP_BASICRESP_free(basic);
- Line 1036: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(issuer_cert);
- Line 1039: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_STORE_free(trust_store);
- Line 1041: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_email_free(ocsp_list);
- Line 1050: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
- Line 1091: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(event);
  Confidence: band=high; score=0.74
- Line 1092: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(event);
- Line 1157: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j["events"].push_back(eventJson);
  Confidence: band=high; score=0.74
- Line 1158: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: j["events"].push_back(eventJson);
- Line 1296: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
- Line 1303: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
- Line 1311: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
- Line 1321: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pubkey);
- Line 1328: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
- Line 1532: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pkcs7_blobs.push_back(std::move(blob));
- Line 1837: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: if (pe_signature == 0x00004550) { // "PE\0\0"
- Line 2024: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: posix_spawn_file_actions_addclose(&actions, pipefd[0]);
- Line 2033: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(pipefd[1]);
- Line 2036: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(pipefd[0]);
- Line 2049: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(pipefd[0]);
- Line 2104: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(mdctx);
- Line 2113: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(mdctx);
- Line 2121: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(mdctx);
- Line 2124: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(mdctx);
- Line 2149: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(mdctx);
- Line 2169: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: OPENSSL_free(issuer_str);
- Line 2189: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: OPENSSL_free(subject_str);

### src/acceleration/faiss_gpu_backend.cpp
Total findings: 95

- Line 128: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.99
- Line 162: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto* idx = new faiss::gpu::GpuIndexFlatL2(
- Line 171: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto* idx = new faiss::gpu::GpuIndexFlatIP(
- Line 181: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto* quantizer = new faiss::gpu::GpuIndexFlatL2(
- Line 190: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto* idx = new faiss::gpu::GpuIndexIVFFlat(
- Line 198: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: idx->nprobe = config_.nprobe;
- Line 204: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto* quantizer = new faiss::gpu::GpuIndexFlatL2(
- Line 213: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto* idx = new faiss::gpu::GpuIndexIVFPQ(
- Line 222: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: idx->nprobe = config_.nprobe;
- Line 231: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto* idx = new faiss::gpu::GpuIndexIVFScalarQuantizer(
- Line 240: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: idx->nprobe = config_.nprobe;
- Line 247: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto* idx = new faiss::IndexHNSWFlat(dimension, config_.hnswM);
- Line 252: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.99
- Line 298: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.99
- Line 326: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.99
- Line 353: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.99
- Line 381: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.99
- Line 417: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.99
- Line 445: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.99
- Line 501: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.99
- Line 502: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "computeDistances: null pointers or zero-size inputs");
  Confidence: band=very_high; score=0.99
- Line 565: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.99
- Line 566: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "batchKnnSearch: null pointers or zero-size inputs");
  Confidence: band=very_high; score=0.99
- Line 689: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.99
- Line 749: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: config_.dimension = gpuIndex->d;
- Line 799: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: stats.numVectors = static_cast<size_t>(idx->ntotal);
- Line 800: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: stats.dimension  = static_cast<size_t>(idx->d);
- Line 843: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 99: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Faiss GPU Backend initialized successfully" << std::endl;
  Confidence: band=very_high; score=0.9
- Line 100: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Device ID: " << config_.deviceId << std::endl;
  Confidence: band=very_high; score=0.9
- Line 101: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Memory Limit: " << config_.maxMemoryMB << " MB" << std::endl;
  Confidence: band=very_high; score=0.9
- Line 128: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.9
- Line 144: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Faiss index created — type: "
  Confidence: band=very_high; score=0.9
- Line 252: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.9
- Line 265: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<faiss::gpu::GpuIndexFlatL2*>(index_);
- Line 268: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<faiss::gpu::GpuIndexFlatIP*>(index_);
- Line 271: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<faiss::gpu::GpuIndexIVFFlat*>(index_);
- Line 274: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<faiss::gpu::GpuIndexIVFPQ*>(index_);
- Line 277: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<faiss::gpu::GpuIndexIVFScalarQuantizer*>(index_);
- Line 280: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<faiss::IndexHNSWFlat*>(index_);
- Line 284: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<faiss::Index*>(index_);
- Line 298: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.9
- Line 326: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.9
- Line 332: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Training Faiss index with " << numVectors << " vectors..." << std::endl;
  Confidence: band=very_high; score=0.9
- Line 334: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Training complete" << std::endl;
  Confidence: band=very_high; score=0.9
- Line 353: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.9
- Line 381: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.9
- Line 394: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Added " << numVectors << " vectors to index (total: "
  Confidence: band=very_high; score=0.9
- Line 417: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.9
- Line 445: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.9
- Line 501: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.9
- Line 502: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: "computeDistances: null pointers or zero-size inputs");
  Confidence: band=very_high; score=0.9
- Line 502: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "computeDistances: null pointers or zero-size inputs");
  Confidence: band=very_high; score=0.9
- Line 535: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<faiss::gpu::GpuIndexFlatL2*>(tempIndex);
- Line 537: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<faiss::gpu::GpuIndexFlatIP*>(tempIndex);
- Line 546: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<faiss::gpu::GpuIndexFlatL2*>(tempIndex);
- Line 548: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<faiss::gpu::GpuIndexFlatIP*>(tempIndex);
- Line 565: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.9
- Line 566: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: "batchKnnSearch: null pointers or zero-size inputs");
  Confidence: band=very_high; score=0.9
- Line 566: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "batchKnnSearch: null pointers or zero-size inputs");
  Confidence: band=very_high; score=0.9
- Line 618: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<faiss::gpu::GpuIndexFlatL2*>(tempIndex);
- Line 620: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<faiss::gpu::GpuIndexFlatIP*>(tempIndex);
- Line 631: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<faiss::gpu::GpuIndexFlatL2*>(tempIndex);
- Line 633: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<faiss::gpu::GpuIndexFlatIP*>(tempIndex);
- Line 685: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Index saved to: " << filepath << std::endl;
  Confidence: band=very_high; score=0.9
- Line 689: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.9
- Line 696: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: cpuIndex = nullptr;
  Context: delete cpuIndex;
- Line 697: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Index saved to: " << filepath << std::endl;
  Confidence: band=very_high; score=0.9
- Line 740: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: cpuIndex = nullptr;
  Context: delete cpuIndex;
- Line 751: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Index loaded from: " << filepath << std::endl;
  Confidence: band=very_high; score=0.9
- Line 752: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Vectors: " << gpuIndex->ntotal << std::endl;
  Confidence: band=very_high; score=0.9
- Line 753: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Dimension: " << gpuIndex->d << std::endl;
  Confidence: band=very_high; score=0.9
- Line 843: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.9
- Line 850: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Index reset" << std::endl;
  Confidence: band=very_high; score=0.9
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['    Config tempConfig;', '    tempConfig.indexType = useL2 ? IndexType::FLAT_L2 : IndexType::FLAT_IP;', '    tempConfig.dimension = static_cast<int>(dim);', '    tempConfig.deviceId = config_.deviceId;', '']
  Confidence: band=medium; score=0.65
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['    Config tempConfig;', '    tempConfig.indexType = useL2 ? IndexType::FLAT_L2 : IndexType::FLAT_IP;', '    tempConfig.dimension = static_cast<int>(dim);', '    tempConfig.deviceId = config_.deviceId;', '']
  Confidence: band=medium; score=0.65
- Line 265: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete static_cast<faiss::gpu::GpuIndexFlatL2*>(index_);
- Line 268: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete static_cast<faiss::gpu::GpuIndexFlatIP*>(index_);
- Line 271: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete static_cast<faiss::gpu::GpuIndexIVFFlat*>(index_);
- Line 274: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete static_cast<faiss::gpu::GpuIndexIVFPQ*>(index_);
- Line 277: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete static_cast<faiss::gpu::GpuIndexIVFScalarQuantizer*>(index_);
- Line 280: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete static_cast<faiss::IndexHNSWFlat*>(index_);
- Line 284: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete static_cast<faiss::Index*>(index_);
- Line 474: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results[q].emplace_back(
  Confidence: band=high; score=0.74
- Line 535: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete static_cast<faiss::gpu::GpuIndexFlatL2*>(tempIndex);
- Line 537: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete static_cast<faiss::gpu::GpuIndexFlatIP*>(tempIndex);
- Line 548: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete static_cast<faiss::gpu::GpuIndexFlatIP*>(tempIndex);
- Line 607: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results[q].emplace_back(
  Confidence: band=high; score=0.74
- Line 618: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete static_cast<faiss::gpu::GpuIndexFlatL2*>(tempIndex);
- Line 620: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete static_cast<faiss::gpu::GpuIndexFlatIP*>(tempIndex);
- Line 631: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete static_cast<faiss::gpu::GpuIndexFlatL2*>(tempIndex);
- Line 633: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete static_cast<faiss::gpu::GpuIndexFlatIP*>(tempIndex);
- Line 696: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete cpuIndex;
- Line 740: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete cpuIndex;

### src/acceleration/graphics_backends.cpp
Total findings: 75

- Line 203: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("vkCreateBuffer failed");
- Line 214: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("vkAllocateMemory failed");
- Line 237: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("findMemoryType: no suitable type");
- Line 248: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("vkCreateShaderModule failed");
- Line 255: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Cannot open SPIR-V: " + path);
- Line 273: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "[ShaderIntegrity] " << result.message << std::endl;
  Confidence: band=very_high; score=0.9
- Line 1226: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "[Vulkan] Initialized: " << impl_->deviceProps.deviceName << std::endl;
  Confidence: band=very_high; score=0.9
- Line 1227: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "[Vulkan] VK_KHR_buffer_device_address: "
  Confidence: band=very_high; score=0.9
- Line 1227: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::cout << "[Vulkan] VK_KHR_buffer_device_address: "
- Line 1239: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (uint32_t i = 0; i < impl_->memoryProps.memoryHeapCount; ++i) {
  Confidence: band=very_high; score=0.9
- Line 1337: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ? " [VK_KHR_buffer_device_address]"
- Line 1338: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: : " [no VK_KHR_buffer_device_address]");
- Line 1512: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t q = 0; q < numQueries; ++q) {
  Confidence: band=very_high; score=0.9
- Line 1515: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t v = 0; v < numVectors; ++v)
  Confidence: band=very_high; score=0.9
- Line 1523: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < effectiveK; ++i)
  Confidence: band=very_high; score=0.9
- Line 1669: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: layout(std430, binding = 0) readonly buffer QBuf { float q[]; };
- Line 1670: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: layout(std430, binding = 1) readonly buffer VBuf { float v[]; };
- Line 1671: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: layout(std430, binding = 2) writeonly buffer DBuf { float d[]; };
- Line 1697: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: layout(std430, binding = 0) readonly buffer QBuf { float q[]; };
- Line 1698: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: layout(std430, binding = 1) readonly buffer VBuf { float v[]; };
- Line 1699: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: layout(std430, binding = 2) writeonly buffer DBuf { float d[]; };
- Line 1732: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: layout(std430, binding = 0) readonly buffer Lat1Buf { float lat1[]; };
- Line 1733: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: layout(std430, binding = 1) readonly buffer Lon1Buf { float lon1[]; };
- Line 1734: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: layout(std430, binding = 2) readonly buffer Lat2Buf { float lat2[]; };
- Line 1735: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: layout(std430, binding = 3) readonly buffer Lon2Buf { float lon2[]; };
- Line 1736: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: layout(std430, binding = 4) writeonly buffer OutBuf  { float dist[]; };
- Line 1769: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: layout(std430, binding = 0) readonly  buffer PointLats { float pLat[]; };
- Line 1770: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: layout(std430, binding = 1) readonly  buffer PointLons { float pLon[]; };
- Line 1771: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: layout(std430, binding = 2) readonly  buffer PolyBuf   { float poly[]; };
- Line 1772: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: layout(std430, binding = 3) writeonly buffer ResBuf    { uint result[]; };
- Line 1810: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: layout(std430, binding = 0) readonly  buffer StartsBuf { uint startVerts[]; };
- Line 1811: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: layout(std430, binding = 1) writeonly buffer FrontBuf  { uint frontier[]; };
- Line 1812: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: layout(std430, binding = 2) writeonly buffer VisitBuf  { uint visited[]; };
- Line 1844: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: layout(std430, binding = 0) readonly buffer AdjBuf    { uint adj[]; };
- Line 1845: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: layout(std430, binding = 1) readonly buffer FrontBuf  { uint frontier[]; };
- Line 1846: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: layout(std430, binding = 2)          buffer VisitBuf  { uint visited[]; };
- Line 1847: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: layout(std430, binding = 3) writeonly buffer NextBuf  { uint nextFront[]; };
- Line 1883: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: layout(std430, binding = 0) readonly  buffer StartsBuf { uint startVerts[]; };
- Line 1884: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: layout(std430, binding = 1) writeonly buffer DistBuf   { float dist[]; };
- Line 1885: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: layout(std430, binding = 2) writeonly buffer PredBuf   { int pred[]; };
- Line 1894: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: dist[p * uNumVerts + v] = (v == startVerts[p]) ? 0.0 : 1e30;
  Confidence: band=very_high; score=0.9
- Line 1911: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: layout(std430, binding = 0) readonly buffer AdjBuf  { uint adj[]; };
- Line 1912: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: layout(std430, binding = 1) readonly buffer WgtBuf  { float wgt[]; };
- Line 1913: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: layout(std430, binding = 2)          buffer DistBuf { float dist[]; };
- Line 1914: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: layout(std430, binding = 3)          buffer PredBuf { int pred[]; };
- Line 2602: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Dispatch haversine shader; lat/lon inputs converted from double to float.
  Confidence: band=very_high; score=0.9
- Line 2645: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Dispatch PIP shader; inputs converted from double to float.
  Confidence: band=very_high; score=0.9
- Line 3217: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "[OpenGL] Initialized: " << impl_->rendererName_
  Confidence: band=very_high; score=0.9
- Line 3415: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t q = 0; q < numQueries; ++q) {
  Confidence: band=very_high; score=0.9
- Line 3418: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t v = 0; v < numVectors; ++v)
  Confidence: band=very_high; score=0.9
- Line 3426: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (size_t i = 0; i < effectiveK; ++i)
  Confidence: band=very_high; score=0.9
- Line 943: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: DirectXVectorBackend::getCapabilities()
  Context: BackendCapabilities DirectXVectorBackend::getCapabilities() const {
  Confidence: band=medium; score=0.56
- Line 947: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: DirectXVectorBackend::initialize()
  Context: bool DirectXVectorBackend::initialize() {
  Confidence: band=medium; score=0.56
- Line 964: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: DirectXVectorBackend::shutdown()
  Context: void DirectXVectorBackend::shutdown() {}
  Confidence: band=medium; score=0.56
- Line 1297: severity=MEDIUM; category=no_health_check
  Description: Status field defined but no initialization or health check
  Remediation: Initialize status to UNKNOWN and implement periodic health checks
  Context: BackendHealthStatus s;
- Line 1303: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: s.issues.push_back("Call initialize() before use");
- Line 1319: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: s.issues.push_back("Compile SPIR-V shaders: glslc shader.comp -o shader.spv");
- Line 1963: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: dlclose(lib);
- Line 3801: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results[s].push_back(static_cast<uint32_t>(v));
  Confidence: band=high; score=0.74
- Line 3801: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results[s].push_back(static_cast<uint32_t>(v));
  Confidence: band=high; score=0.74
- Line 3802: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results[s].push_back(static_cast<uint32_t>(v));
- Line 3822: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results[s].push_back(cur);
  Confidence: band=high; score=0.74
- Line 3823: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results[s].push_back(cur);
- Line 3897: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: path.push_back(static_cast<uint32_t>(cur));
  Confidence: band=high; score=0.74
- Line 3897: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: path.push_back(static_cast<uint32_t>(cur));
  Confidence: band=high; score=0.74
- Line 3898: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: path.push_back(static_cast<uint32_t>(cur));
- Line 3939: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: path.push_back(static_cast<uint32_t>(cur));
  Confidence: band=high; score=0.74
- Line 3939: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: path.push_back(static_cast<uint32_t>(cur));
  Confidence: band=high; score=0.74
- Line 3939: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: path.push_back(static_cast<uint32_t>(cur));
  Confidence: band=high; score=0.74
- Line 3939: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: path.push_back(static_cast<uint32_t>(cur));
  Confidence: band=high; score=0.74
- Line 3940: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: path.push_back(static_cast<uint32_t>(cur));
- Line 2210: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: std::vector<char> log(static_cast<size_t>(logLen));
  Confidence: band=medium; score=0.6
- Line 2231: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: std::vector<char> log(static_cast<size_t>(logLen));
  Confidence: band=medium; score=0.6
- Line 2579: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: std::vector<char> log(static_cast<size_t>(logLen));
  Confidence: band=medium; score=0.6
- Line 2889: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: std::vector<char> log(static_cast<size_t>(logLen));
  Confidence: band=medium; score=0.6

### src/acceleration/oneapi_backend.cpp
Total findings: 23

- Line 71: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: queue_ = new sycl::queue(sycl::gpu_selector_v);
- Line 75: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: queue_ = new sycl::queue(sycl::default_selector_v);
- Line 78: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: queue_ = new sycl::queue(sycl::default_selector_v);
- Line 82: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: queue_ = new sycl::queue(sycl::default_selector_v);
- Line 85: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto device = queue_->get_device();
- Line 130: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: queue_->memcpy(d_queries, queries, numQueries * dimension * sizeof(float)).wait();
- Line 170: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: }).wait();
- Line 174: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: queue_->memcpy(distances.data(), d_distances, resultSize * sizeof(float)).wait();
- Line 88: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "OneAPI backend initialized successfully\n";
  Confidence: band=very_high; score=0.9
- Line 89: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Platform: " << platform.get_info<sycl::info::platform::name>() << "\n";
  Confidence: band=very_high; score=0.9
- Line 90: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Device: " << device.get_info<sycl::info::device::name>() << "\n";
  Confidence: band=very_high; score=0.9
- Line 91: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Max Compute Units: " << device.get_info<sycl::info::device::max_compute_units>() << "\n";
  Confidence: band=very_high; score=0.9
- Line 92: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Max Work Group Size: " << device.get_info<sycl::info::device::max_work_group_size>() << "\n";
  Confidence: band=very_high; score=0.9
- Line 174: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: queue_->memcpy(distances.data(), d_distances, resultSize * sizeof(float)).wait();
- Line 42: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete queue_;
- Line 67: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: bool initialize() override {
  Confidence: band=medium; score=0.66
- Line 104: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete queue_;
- Line 177: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: sycl::free(d_queries, *queue_);
- Line 178: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: sycl::free(d_vectors, *queue_);
- Line 179: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: sycl::free(d_distances, *queue_);
- Line 201: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pairs.push_back({distances[q * numVectors + v], v});
  Confidence: band=high; score=0.74
- Line 202: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pairs.push_back({distances[q * numVectors + v], v});
- Line 258: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: bool initialize() override { return false; }
  Confidence: band=medium; score=0.66

### src/acceleration/hip_backend.cpp
Total findings: 21

- Line 61: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(std::string("HIP error: ") + hipGetErrorString(error)); \
- Line 406: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "HIP Backend: Initializing..." << std::endl;
  Confidence: band=very_high; score=0.9
- Line 434: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Device " << i << ": " << prop.name
  Confidence: band=very_high; score=0.9
- Line 476: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "HIP Backend: Selected device " << impl_->deviceId
  Confidence: band=very_high; score=0.9
- Line 478: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Compute Units: " << impl_->deviceProps.multiProcessorCount << std::endl;
  Confidence: band=very_high; score=0.9
- Line 479: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Global Memory: " << (impl_->deviceProps.totalGlobalMem / (1024*1024*1024)) << " GB" << std::endl;
  Confidence: band=very_high; score=0.9
- Line 480: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Warp Size: " << impl_->deviceProps.warpSize << std::endl;
  Confidence: band=very_high; score=0.9
- Line 481: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  GCN Arch: " << impl_->deviceProps.gcnArchName << std::endl;
  Confidence: band=very_high; score=0.9
- Line 482: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  ROCm Runtime: " << (runtimeVersion / 10000000) << "."
  Confidence: band=very_high; score=0.9
- Line 488: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Auto-detected Wave Size: " << impl_->config.waveSize << std::endl;
  Confidence: band=very_high; score=0.9
- Line 520: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Occupancy-tuned block size: " << impl_->occupancyTunedBlockSize << std::endl;
  Confidence: band=very_high; score=0.9
- Line 939: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "HIP Geo Backend initialized successfully:" << std::endl;
  Confidence: band=very_high; score=0.9
- Line 940: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Device: " << prop.name << std::endl;
  Confidence: band=very_high; score=0.9
- Line 941: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  GCN Arch: " << prop.gcnArchName << std::endl;
  Confidence: band=very_high; score=0.9
- Line 942: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Global Memory: " << (prop.totalGlobalMem / (1024*1024*1024)) << " GB" << std::endl;
  Confidence: band=very_high; score=0.9
- Line 1003: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("hip_launchGeoDistanceKernel failed with code " +
- Line 1052: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: const size_t resultBytes = numPoints          * sizeof(uint8_t);
- Line 434: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cout << "Device " << i << ": " << prop.name
- Line 734: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results[q].emplace_back(indices[q * effectiveK + i], topKDistances[q * effectiveK + i]);
  Confidence: band=high; score=0.74
- Line 798: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: devices.push_back(info);
  Confidence: band=high; score=0.74
- Line 799: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: devices.push_back(info);

### src/acceleration/vec_knn.cpp
Total findings: 16

- Line 336: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = map_.begin(); it != map_.end();) {
- Line 338: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator sep may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: auto sep             = k.find('\0');
- Line 80: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(addBatchBridgeMutex());
- Line 85: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(addBatchBridgeMutex());
- Line 263: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (std::size_t v = 0; v < n; ++v) {
  Confidence: band=very_high; score=0.9
- Line 336: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (auto it = map_.begin(); it != map_.end();) {
  Confidence: band=very_high; score=0.9
- Line 341: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: auto oi = std::find(order_.begin(), order_.end(), k);
- Line 406: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (std::size_t q = 0; q < numQueries; ++q) {
  Confidence: band=very_high; score=0.9
- Line 504: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (std::size_t begin = 0; begin < entities.size(); begin += batchSize) {
  Confidence: band=very_high; score=0.9
- Line 510: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (std::size_t i = begin; i < end; ++i) {
  Confidence: band=very_high; score=0.9
- Line 330: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: order_.push_back(key);
- Line 479: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pks.push_back(e.getPrimaryKey());
  Confidence: band=high; score=0.74
- Line 480: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pks.push_back(e.getPrimaryKey());
- Line 481: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: vectors.push_back(std::move(*vec));
- Line 510: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch.push_back(entities[i]);
  Confidence: band=high; score=0.74
- Line 511: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: batch.push_back(entities[i]);

### src/acceleration/geo_acceleration_bridge.cpp
Total findings: 15

- Line 158: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: themis::geo::SpatialBatchInputs batch;
  Confidence: band=very_high; score=0.9
- Line 282: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: "invalid inputs (null pointer or < 3 polygon vertices)");
  Confidence: band=very_high; score=0.9
- Line 298: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: themis::geo::SpatialBatchInputs batch;
  Confidence: band=very_high; score=0.9
- Line 152: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ring.push_back({polygon_coords[v * 2], polygon_coords[v * 2 + 1]});
- Line 154: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: poly.rings.push_back(std::move(ring));
- Line 163: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pt.coords.push_back({point_lats[i], point_lons[i]});
  Confidence: band=high; score=0.74
- Line 164: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pt.coords.push_back({point_lats[i], point_lons[i]});
- Line 165: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: batch.geoms_a.push_back(std::move(pt));
- Line 166: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: batch.geoms_b.push_back(poly);
- Line 293: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: ring.push_back({polygonCoords[v * 2], polygonCoords[v * 2 + 1]});
- Line 295: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: poly.rings.push_back(std::move(ring));
- Line 304: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pt.coords.push_back({pointLats[i], pointLons[i]});
  Confidence: band=high; score=0.74
- Line 305: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pt.coords.push_back({pointLats[i], pointLons[i]});
- Line 306: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: batch.geoms_a.push_back(std::move(pt));
- Line 307: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: batch.geoms_b.push_back(poly); // shared polygon — copy is cheap for small rings

### src/acceleration/plugin_loader.cpp
Total findings: 13

- Line 290: severity=CRITICAL; category=iterator_invalidation
  Description: Iterator it may be invalidated by container modification
  Remediation: Re-create iterator after modification or use erase() return value
  Context: for (auto it = plugins_.begin(); it != plugins_.end(); ++it) {
- Line 174: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "SECURITY: Plugin verification passed: " << libraryPath << std::endl;
  Confidence: band=very_high; score=0.9
- Line 212: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Loaded plugin: " << plugin->pluginName() << " v" << plugin->pluginVersion()
  Confidence: band=very_high; score=0.9
- Line 246: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto &entry : fs::directory_iterator(directoryPath)) {
- Line 284: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Loaded " << loadedCount << " acceleration plugins from " << directoryPath << std::endl;
  Confidence: band=very_high; score=0.9
- Line 292: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Unloading plugin: " << pluginName << std::endl;
  Confidence: band=very_high; score=0.9
- Line 302: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Unloading plugin: " << plugin.name << std::endl;
  Confidence: band=very_high; score=0.9
- Line 100: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: dlclose(handle);
- Line 260: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "SECURITY: Skipping symlink that escapes plugin directory: " << entry.path() << std::en
- Line 292: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cout << "Unloading plugin: " << pluginName << std::endl;
- Line 302: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cout << "Unloading plugin: " << plugin.name << std::endl;
- Line 321: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(plugin.plugin.get());
  Confidence: band=high; score=0.74
- Line 322: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(plugin.plugin.get());

### src/acceleration/vllm_resource_manager.cpp
Total findings: 12

- Line 152: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (shared_future->wait_for(std::chrono::milliseconds(500)) == std::future_status::ready) {
- Line 153: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: gpu_util = shared_future->get();
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 131: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: std::async(std::launch::async, [device_handles]() -> std::optional<double> {
- Line 152: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: if (shared_future->wait_for(std::chrono::milliseconds(500)) == std::future_status::ready) {
- Line 236: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
  Confidence: band=very_high; score=0.9
- Line 236: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 318: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
  Confidence: band=very_high; score=0.9
- Line 318: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 425: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nvml_devices_.push_back(static_cast<void *>(dev));
  Confidence: band=high; score=0.74
- Line 426: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nvml_devices_.push_back(static_cast<void *>(dev));

### src/acceleration/multi_gpu_backend.cpp
Total findings: 11

- Line 146: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "MultiGPUVectorBackend: initialised with " << shardDescs.size()
  Confidence: band=very_high; score=0.9
- Line 95: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: deviceIds.push_back(i);
  Confidence: band=high; score=0.74
- Line 96: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: deviceIds.push_back(i);
- Line 122: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: shardDescs.push_back(sd);
- Line 132: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "MultiGPUVectorBackend: sub-backend init failed for device " << shardDescs[i].deviceId
- Line 136: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "MultiGPUVectorBackend: warning — sub-backend init failed "
- Line 139: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: subBackends.push_back(std::move(sb));
  Confidence: band=high; score=0.74
- Line 140: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: subBackends.push_back(std::move(sb));
- Line 271: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged[q].emplace_back(globalIdx, dist);
  Confidence: band=high; score=0.74
- Line 271: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged[q].emplace_back(globalIdx, dist);
  Confidence: band=high; score=0.74
- Line 271: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged[q].emplace_back(globalIdx, dist);
  Confidence: band=high; score=0.74

### src/acceleration/opencl_backend.cpp
Total findings: 11

- Line 157: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "OpenCL backend initialized successfully" << std::endl;
  Confidence: band=very_high; score=0.9
- Line 158: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Platform: " << platformName << " (" << platformVersion << ")" << std::endl;
  Confidence: band=very_high; score=0.9
- Line 159: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Device: " << deviceName << std::endl;
  Confidence: band=very_high; score=0.9
- Line 160: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Device Version: " << deviceVersion << std::endl;
  Confidence: band=very_high; score=0.9
- Line 161: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Compute Units: " << computeUnits << std::endl;
  Confidence: band=very_high; score=0.9
- Line 162: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Global Memory: " << (globalMemSize / (1024*1024*1024)) << " GB" << std::endl;
  Confidence: band=very_high; score=0.9
- Line 289: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: clSetKernelArg(kernel, 4, sizeof(unsigned int), &uNumVectors);
- Line 329: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: pairs.push_back({distances[q * numVectors + v], static_cast<uint32_t>(v)});
- Line 335: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results[q].push_back({pairs[i].second, pairs[i].first});
  Confidence: band=high; score=0.74
- Line 336: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results[q].push_back({pairs[i].second, pairs[i].first});
- Line 204: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: std::vector<char> log(logSize);
  Confidence: band=medium; score=0.6

### src/acceleration/backend_registry.cpp
Total findings: 10

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 113: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Register OpenCL backend for broad hardware compatibility.
  Confidence: band=high; score=0.8
- Line 233: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: return (it != typeIndex_.end()) ? it->second.base : nullptr;
- Line 233: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return (it != typeIndex_.end()) ? it->second.base : nullptr;
- Line 414: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (const auto &backend : backends_) {
  Confidence: band=very_high; score=0.9
- Line 542: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return runtimeInitialized_.load(std::memory_order_acquire);
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 273: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: static T *selectTyped(const std::unordered_map<BackendType, RegisteredBackend> &index,
  Confidence: band=medium; score=0.66
- Line 414: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: types.push_back(backend->type());
  Confidence: band=high; score=0.74
- Line 415: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: types.push_back(backend->type());

### src/acceleration/cpu_backend.cpp
Total findings: 10

- Line 96: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // happens when the same computational path is applied to equal inputs.
  Confidence: band=very_high; score=0.9
- Line 100: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (a.second != b.second) {
  Confidence: band=very_high; score=0.9
- Line 87: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: distances.emplace_back(static_cast<uint32_t>(v), dist);
  Confidence: band=high; score=0.74
- Line 136: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results[s].push_back(start);
  Confidence: band=high; score=0.74
- Line 137: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results[s].push_back(start);
- Line 157: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results[s].push_back(v);
  Confidence: band=high; score=0.74
- Line 158: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: results[s].push_back(v);
- Line 228: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "[CPUGraph] batchShortestPath: negative weight " << raw_w << " on edge " << u << "→"
- Line 247: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: path.push_back(static_cast<uint32_t>(v));
  Confidence: band=high; score=0.74
- Line 248: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: path.push_back(static_cast<uint32_t>(v));

### src/acceleration/zluda_backend.cpp
Total findings: 10

- Line 10: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // ZLUDA: CUDA compatibility layer for AMD GPUs
  Confidence: band=high; score=0.8
- Line 132: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "ZLUDA Backend: Initializing..." << std::endl;
  Confidence: band=very_high; score=0.9
- Line 133: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "ZLUDA: CUDA compatibility layer for AMD GPUs" << std::endl;
  Confidence: band=very_high; score=0.9
- Line 164: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "ZLUDA: Found " << deviceCount << " AMD GPU(s)" << std::endl;
  Confidence: band=very_high; score=0.9
- Line 180: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "ZLUDA Backend: Successfully initialized" << std::endl;
  Confidence: band=very_high; score=0.9
- Line 181: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Note: ZLUDA allows running CUDA kernels on AMD GPUs" << std::endl;
  Confidence: band=very_high; score=0.9
- Line 362: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: [[nodiscard]] std::unique_ptr<IVectorBackend> createZLUDABackend() {
- Line 96: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: dlclose(handle);
- Line 101: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: dlclose(handle);
- Line 189: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: if (zludaLib_) dlclose(zludaLib_);

### src/acceleration/directx_backend_full.cpp
Total findings: 9

- Line 57: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: snprintf(_buf, sizeof(_buf), "DirectX error: HRESULT 0x%08X", \
  Confidence: band=very_high; score=0.9
- Line 57: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: snprintf(_buf, sizeof(_buf), "DirectX error: HRESULT 0x%08X", \
- Line 59: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(_buf); \
- Line 289: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: DX_CHECK_THROW(commandList_->Reset(commandAllocator_.Get(), nullptr));
- Line 376: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error(
- Line 637: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "[DirectX] Initialized: " << impl_->deviceName() << std::endl;
  Confidence: band=very_high; score=0.9
- Line 617: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: DirectXVectorBackend::getCapabilities()
  Context: BackendCapabilities DirectXVectorBackend::getCapabilities() const {
  Confidence: band=medium; score=0.56
- Line 629: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: DirectXVectorBackend::initialize()
  Context: bool DirectXVectorBackend::initialize() {
  Confidence: band=medium; score=0.56
- Line 641: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: DirectXVectorBackend::shutdown()
  Context: void DirectXVectorBackend::shutdown() {
  Confidence: band=medium; score=0.56

### src/acceleration/vulkan_backend_full.cpp
Total findings: 9

- Line 139: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to find suitable memory type");
- Line 150: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to create shader module");
- Line 214: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to open SPIR-V file: " + filename);
- Line 259: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Validation layers requested but not available" << std::endl;
  Confidence: band=very_high; score=0.9
- Line 308: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Selected Vulkan device: " << ctx.deviceProps.deviceName << std::endl;
  Confidence: band=very_high; score=0.9
- Line 519: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to create buffer");
- Line 532: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: throw std::runtime_error("Failed to allocate buffer memory");
- Line 532: severity=HIGH; category=uncaught_exception
  Description: Exception thrown without try/catch context
  Remediation: Wrap throwing code in try/catch or add proper error handling
  Context: throw std::runtime_error("Failed to allocate buffer memory");
- Line 222: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: file.close();

### src/acceleration/cpu_backend_mt.cpp
Total findings: 8

- Line 67: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Multi-threaded CPU backend initialized\n";
  Confidence: band=very_high; score=0.9
- Line 68: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Threads: " << numThreads_ << "\n";
  Confidence: band=very_high; score=0.9
- Line 69: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  OpenMP: " << (THEMIS_HAS_OPENMP ? "Yes" : "No") << "\n";
  Confidence: band=very_high; score=0.9
- Line 71: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  SIMD: AVX2/AVX-512\n";
  Confidence: band=very_high; score=0.9
- Line 73: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  SIMD: NEON\n";
  Confidence: band=very_high; score=0.9
- Line 75: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  SIMD: No\n";
  Confidence: band=very_high; score=0.9
- Line 71: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cout << "  SIMD: AVX2/AVX-512\n";
- Line 186: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: distances.emplace_back(static_cast<uint32_t>(v), dist);
  Confidence: band=high; score=0.74

### src/acceleration/cpu_backend_tbb.cpp
Total findings: 8

- Line 56: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "Intel TBB CPU backend initialized\n";
  Confidence: band=very_high; score=0.9
- Line 57: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  Threads: " << numThreads_ << "\n";
  Confidence: band=very_high; score=0.9
- Line 58: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  TBB Version: " << TBB_VERSION_MAJOR << "." << TBB_VERSION_MINOR << "\n";
  Confidence: band=very_high; score=0.9
- Line 60: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  SIMD: AVX2/AVX-512\n";
  Confidence: band=very_high; score=0.9
- Line 62: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  SIMD: NEON\n";
  Confidence: band=very_high; score=0.9
- Line 64: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  SIMD: Scalar\n";
  Confidence: band=very_high; score=0.9
- Line 60: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cout << "  SIMD: AVX2/AVX-512\n";
- Line 297: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: distances.emplace_back(static_cast<uint32_t>(v), dist);
  Confidence: band=high; score=0.74

### src/acceleration/device_manager.cpp
Total findings: 8

- Line 177: severity=HIGH; category=null_dereference
  Description: Potential null pointer dereference
  Remediation: Add null check before dereferencing
  Context: if (best == nullptr || d.free_vram_bytes > best->free_vram_bytes) {
- Line 219: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "[acceleration] Device capability probe — " << devices.size() << " device(s) found:" << std::endl;
  Confidence: band=very_high; score=0.9
- Line 222: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "  [" << (d.is_healthy ? "OK" : "!!") << "] " << d.name
  Confidence: band=very_high; score=0.9
- Line 230: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "[acceleration] Best device: " << best.name << " (backend=" << static_cast<int>(best.backend_type)
  Confidence: band=very_high; score=0.9
- Line 110: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(fromGpuDeviceInfo(d));
  Confidence: band=high; score=0.74
- Line 111: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(fromGpuDeviceInfo(d));
- Line 120: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.push_back(cpu);
- Line 222: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cout << "  [" << (d.is_healthy ? "OK" : "!!") << "] " << d.name

### src/acceleration/shader_integrity.cpp
Total findings: 8

- Line 124: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: return verify(name, reinterpret_cast<const uint8_t *>(spvWords.data()), spvWords.size() * sizeof(uin
- Line 194: severity=HIGH; category=performance; pattern=lock_in_loop
  Description: Mutex lock acquired per iteration (move outside loop)
  Context: for (unsigned int i = 0; i < hashLen; ++i) {
  Confidence: band=very_high; score=0.9
- Line 201: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: return sha256Hex(reinterpret_cast<const uint8_t *>(spvWords.data()), spvWords.size() * sizeof(uint32
- Line 209: severity=HIGH; category=deadlock_risk
  Description: Multiple locks acquired in nested scope — potential deadlock
  Remediation: Use std::scoped_lock(m1, m2) or enforce consistent lock ordering
  Context: std::lock_guard<std::mutex> lk(mutex_);
- Line 177: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 181: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 188: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 191: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);

### src/acceleration/nccl_vector_backend.cpp
Total findings: 7

- Line 84: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "NCCL: Initializing with rank " << config.rank
  Confidence: band=very_high; score=0.9
- Line 113: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "NCCL: Initialization successful" << std::endl;
  Confidence: band=very_high; score=0.9
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 145: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "CUDA error: " << cudaGetErrorString(err)
- Line 145: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "CUDA error: " << cudaGetErrorString(err)
- Line 156: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "CUDA error: " << cudaGetErrorString(err)
- Line 156: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "CUDA error: " << cudaGetErrorString(err)

### src/acceleration/rccl_vector_backend.cpp
Total findings: 7

- Line 111: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "RCCL: Initializing with rank " << config.rank
  Confidence: band=very_high; score=0.9
- Line 140: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: std::cout << "RCCL: Initialization successful" << std::endl;
  Confidence: band=very_high; score=0.9
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 172: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "HIP error: " << hipGetErrorString(err)
- Line 172: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "HIP error: " << hipGetErrorString(err)
- Line 183: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "HIP error: " << hipGetErrorString(err)
- Line 183: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "HIP error: " << hipGetErrorString(err)

### src/acceleration/compute_backend.cpp
Total findings: 3

- Line 76: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: validIndices.push_back(q);
  Confidence: band=high; score=0.74
- Line 76: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: validIndices.push_back(q);
  Confidence: band=high; score=0.74
- Line 77: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: validIndices.push_back(q);

### src/acceleration/tensor_core_matmul.cpp
Total findings: 2

- Line 41: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (beta == 0.0f) {
  Confidence: band=very_high; score=0.9
- Line 43: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: } else if (beta != 1.0f) {
  Confidence: band=very_high; score=0.9

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
