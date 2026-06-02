# acceleration Module - Developer Gap Note

> Auto-generated from ai_working/gap_scan_v3_aggregate.json.
> This file is overwritten on each regeneration.

## Scan Snapshot

- Module: acceleration
- Generated: 2026-06-02 11:55:47
- Status: Critical Findings Present
- Total Findings: 480
- Actionable Findings (Critical + High): 342
- Affected Files: 26

## Severity Summary

| Severity | Count |
|---|---:|
| Critical | 80 |
| High | 262 |
| Medium | 133 |
| Low | 0 |

## Category Summary

| Category | Count |
|---|---:|
| llm_ai_safety | 140 |
| audit_logging | 99 |
| raii | 57 |
| memory | 55 |
| exception_safety | 50 |
| performance_patterns | 46 |
| gpu_memory_safety | 32 |
| container | 27 |
| performance | 17 |
| concurrency | 13 |
| legacy_duplication | 9 |
| platform | 9 |
| observability | 7 |
| determinism | 5 |
| reliability | 4 |
| type_conversion | 2 |
| uninitialized | 2 |

## File Overview

| File | Findings | Critical | High | Medium | Low |
|---|---:|---:|---:|---:|---:|
| src/acceleration/ai_hardware_dispatcher.cpp | 117 | 39 | 75 | 3 | 0 |
| src/acceleration/cuda_backend.cpp | 87 | 2 | 79 | 6 | 0 |
| src/acceleration/faiss_gpu_backend.cpp | 73 | 27 | 33 | 13 | 0 |
| src/acceleration/graphics_backends.cpp | 54 | 0 | 32 | 18 | 4 |
| src/acceleration/plugin_security.cpp | 42 | 3 | 5 | 34 | 0 |
| src/acceleration/oneapi_backend.cpp | 13 | 7 | 1 | 5 | 0 |
| src/acceleration/cpu_backend.cpp | 9 | 0 | 3 | 6 | 0 |
| src/acceleration/vllm_resource_manager.cpp | 9 | 2 | 5 | 2 | 0 |
| src/acceleration/backend_registry.cpp | 8 | 0 | 5 | 3 | 0 |
| src/acceleration/multi_gpu_backend.cpp | 7 | 0 | 0 | 7 | 0 |
| src/acceleration/geo_acceleration_bridge.cpp | 6 | 0 | 4 | 2 | 0 |
| src/acceleration/plugin_loader.cpp | 6 | 0 | 2 | 4 | 0 |
| src/acceleration/directx_backend_full.cpp | 5 | 0 | 2 | 3 | 0 |
| src/acceleration/hip_backend.cpp | 5 | 0 | 2 | 3 | 0 |
| src/acceleration/nccl_vector_backend.cpp | 5 | 0 | 0 | 5 | 0 |
| src/acceleration/rccl_vector_backend.cpp | 5 | 0 | 0 | 5 | 0 |
| src/acceleration/shader_integrity.cpp | 5 | 0 | 3 | 2 | 0 |
| src/acceleration/opencl_backend.cpp | 4 | 0 | 2 | 1 | 1 |
| src/acceleration/zluda_backend.cpp | 4 | 0 | 3 | 1 | 0 |
| src/acceleration/compute_backend.cpp | 3 | 0 | 1 | 2 | 0 |
| src/acceleration/device_manager.cpp | 3 | 0 | 1 | 2 | 0 |
| src/acceleration/vec_knn.cpp | 3 | 0 | 1 | 2 | 0 |
| src/acceleration/cpu_backend_mt.cpp | 2 | 0 | 0 | 2 | 0 |
| src/acceleration/cpu_backend_tbb.cpp | 2 | 0 | 0 | 2 | 0 |
| src/acceleration/tensor_core_matmul.cpp | 2 | 0 | 2 | 0 | 0 |
| src/acceleration/vulkan_backend_full.cpp | 1 | 0 | 1 | 0 | 0 |

## Full Scanner Findings

### src/acceleration/ai_hardware_dispatcher.cpp
Total findings: 117

- Line 20: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: *   AiHardwareDispatcher::dispatch(task, model, input)
  Confidence: band=very_high; score=0.99
- Line 600: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (!req.input_data || req.input_elements == 0) {
  Confidence: band=very_high; score=0.99
- Line 601: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return makeError(BackendType::NPU_APPLE, "Invalid input: null or empty");
  Confidence: band=very_high; score=0.99
- Line 628: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // MLMultiArray from req.input_data, run prediction, and extract results.
  Confidence: band=very_high; score=0.99
- Line 646: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (!req.input_data || req.input_elements == 0) {
  Confidence: band=very_high; score=0.99
- Line 647: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return makeError(BackendType::NPU_INTEL, "Invalid input: null or empty");
  Confidence: band=very_high; score=0.99
- Line 659: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Bind input tensor
  Confidence: band=very_high; score=0.99
- Line 660: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: ov::Shape shape(req.input_shape.begin(), req.input_shape.end());
  Confidence: band=very_high; score=0.99
- Line 661: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: ov::Tensor input_tensor(ov::element::f32, shape, const_cast<float *>(req.input_data));
  Confidence: band=very_high; score=0.99
- Line 662: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: infer_req.set_input_tensor(input_tensor);
  Confidence: band=very_high; score=0.99
- Line 692: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (!req.input_data || req.input_elements == 0) {
  Confidence: band=very_high; score=0.99
- Line 693: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return makeError(BackendType::NPU_QUALCOMM, "Invalid input: null or empty");
  Confidence: band=very_high; score=0.99
- Line 713: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (!req.input_data || req.input_elements == 0) {
  Confidence: band=very_high; score=0.99
- Line 714: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return makeError(BackendType::NPU_ARM, "Invalid input: null or empty");
  Confidence: band=very_high; score=0.99
- Line 731: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (!req.input_data || req.input_elements == 0) {
  Confidence: band=very_high; score=0.99
- Line 732: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return makeError(BackendType::NNAPI, "Invalid input: null or empty");
  Confidence: band=very_high; score=0.99
- Line 763: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (req.input_data == nullptr || req.input_elements == 0) {
  Confidence: band=very_high; score=0.99
- Line 764: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return makeError(BackendType::ONNX_RUNTIME, "Invalid input: null or empty");
  Confidence: band=very_high; score=0.99
- Line 817: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Build input shape
  Confidence: band=very_high; score=0.99
- Line 818: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::vector<int64_t> shape(req.input_shape.begin(), req.input_shape.end());
  Confidence: band=very_high; score=0.99
- Line 824: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Input tensor
  Confidence: band=very_high; score=0.99
- Line 825: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: OrtValue *input_tensor = nullptr;
  Confidence: band=very_high; score=0.99
- Line 826: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: ort->CreateTensorWithDataAsOrtValue(mem_info, const_cast<float *>(req.input_data),
  Confidence: band=very_high; score=0.99
- Line 827: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: req.input_elements * sizeof(float), shape.data(), shape.size(),
  Confidence: band=very_high; score=0.99
- Line 828: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, &input_tensor);
  Confidence: band=very_high; score=0.99
- Line 830: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: // Query input/output names
  Confidence: band=very_high; score=0.99
- Line 833: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: size_t input_count = 0, output_count = 0;
  Confidence: band=very_high; score=0.99
- Line 834: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: ort->SessionGetInputCount(session, &input_count);
  Confidence: band=very_high; score=0.99
- Line 837: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: std::vector<char *> input_names_raw(input_count);
  Confidence: band=very_high; score=0.99
- Line 839: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: for (size_t i = 0; i < input_count; ++i) {
  Confidence: band=very_high; score=0.99
- Line 840: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: ort->SessionGetInputName(session, i, alloc, &input_names_raw[i]);
  Confidence: band=very_high; score=0.99
- Line 846: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: const char *const *in_names  = const_cast<const char *const *>(input_names_raw.data());
  Confidence: band=very_high; score=0.99
- Line 851: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: status = ort->Run(session, nullptr, in_names, &input_tensor, input_count, out_names, output_count,
  Confidence: band=very_high; score=0.99
- Line 858: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: ort->ReleaseValue(input_tensor);
  Confidence: band=very_high; score=0.99
- Line 892: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: ort->ReleaseValue(input_tensor);
  Confidence: band=very_high; score=0.99
- Line 922: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: if (req.input_data == nullptr || req.input_elements == 0) {
  Confidence: band=very_high; score=0.99
- Line 923: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: return makeError(BackendType::CPU, "Invalid input: null or empty");
  Confidence: band=very_high; score=0.99
- Line 934: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: result.output.assign(req.input_data, req.input_data + req.input_elements);
  Confidence: band=very_high; score=0.99
- Line 935: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: result.output_shape = req.input_shape;
  Confidence: band=very_high; score=0.99
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4928 [Docs][acceleration] Aktual... (2026-05-10)
- Line 13: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: * Routes AI inference workloads to the best available AI accelerator using a
  Confidence: band=very_high; score=0.9
- Line 15: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: * and specialises in AI inference rather than general ANN / geospatial / graph
  Confidence: band=very_high; score=0.9
- Line 20: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: *   AiHardwareDispatcher::dispatch(task, model, input)
  Confidence: band=very_high; score=0.9
- Line 38: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: *   AiHardwareDispatcher::dispatch()   — route inference task through priority chain
  Confidence: band=very_high; score=0.9
- Line 238: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Inference dispatch
  Confidence: band=very_high; score=0.9
- Line 255: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Check precision compatibility
  Confidence: band=high; score=0.8
- Line 271: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult result = runOn(cap.type, req);
  Confidence: band=very_high; score=0.9
- Line 279: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult err;
  Confidence: band=very_high; score=0.9
- Line 281: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: err.error   = "All AI hardware backends exhausted — no successful inference path";
  Confidence: band=very_high; score=0.9
- Line 286: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult AiHardwareDispatcher::runOn(BackendType backend, AiInferenceRequest &req) {
  Confidence: band=very_high; score=0.9
- Line 565: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: //   3. Returns a filled AiInferenceResult (success or error).
  Confidence: band=very_high; score=0.9
- Line 570: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: static AiInferenceResult makeError(BackendType bt, const std::string &msg) {
  Confidence: band=very_high; score=0.9
- Line 571: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult r;
  Confidence: band=very_high; score=0.9
- Line 578: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult AiHardwareDispatcher::dispatchAppleANE([[maybe_unused]] AiInferenceRequest &req) {
  Confidence: band=very_high; score=0.9
- Line 600: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (!req.input_data || req.input_elements == 0) {
  Confidence: band=very_high; score=0.9
- Line 601: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return makeError(BackendType::NPU_APPLE, "Invalid input: null or empty");
  Confidence: band=very_high; score=0.9
- Line 615: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Production Delta: Apple Neural Engine (ANE) / Core ML inference is
  Confidence: band=very_high; score=0.9
- Line 628: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // MLMultiArray from req.input_data, run prediction, and extract results.
  Confidence: band=very_high; score=0.9
- Line 629: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult result;
  Confidence: band=very_high; score=0.9
- Line 644: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult AiHardwareDispatcher::dispatchIntelNPU([[maybe_unused]] AiInferenceRequest &req) {
  Confidence: band=very_high; score=0.9
- Line 646: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (!req.input_data || req.input_elements == 0) {
  Confidence: band=very_high; score=0.9
- Line 647: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return makeError(BackendType::NPU_INTEL, "Invalid input: null or empty");
  Confidence: band=very_high; score=0.9
- Line 657: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto infer_req = compiled.create_infer_request();
  Confidence: band=very_high; score=0.9
- Line 659: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Bind input tensor
  Confidence: band=very_high; score=0.9
- Line 660: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: ov::Shape shape(req.input_shape.begin(), req.input_shape.end());
  Confidence: band=very_high; score=0.9
- Line 661: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: ov::Tensor input_tensor(ov::element::f32, shape, const_cast<float *>(req.input_data));
  Confidence: band=very_high; score=0.9
- Line 662: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: infer_req.set_input_tensor(input_tensor);
  Confidence: band=very_high; score=0.9
- Line 662: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: infer_req.set_input_tensor(input_tensor);
  Confidence: band=very_high; score=0.9
- Line 664: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: infer_req.infer();
  Confidence: band=very_high; score=0.9
- Line 666: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: auto output_tensor   = infer_req.get_output_tensor();
  Confidence: band=very_high; score=0.9
- Line 670: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult result;
  Confidence: band=very_high; score=0.9
- Line 683: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: return makeError(BackendType::NPU_INTEL, std::string("OpenVINO NPU inference failed: ") + e.what());
  Confidence: band=very_high; score=0.9
- Line 690: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult AiHardwareDispatcher::dispatchQualcommQNN([[maybe_unused]] AiInferenceRequest &req) {
  Confidence: band=very_high; score=0.9
- Line 692: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (!req.input_data || req.input_elements == 0) {
  Confidence: band=very_high; score=0.9
- Line 693: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return makeError(BackendType::NPU_QUALCOMM, "Invalid input: null or empty");
  Confidence: band=very_high; score=0.9
- Line 699: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult result;
  Confidence: band=very_high; score=0.9
- Line 711: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult AiHardwareDispatcher::dispatchArmEthos([[maybe_unused]] AiInferenceRequest &req) {
  Confidence: band=very_high; score=0.9
- Line 713: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (!req.input_data || req.input_elements == 0) {
  Confidence: band=very_high; score=0.9
- Line 714: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return makeError(BackendType::NPU_ARM, "Invalid input: null or empty");
  Confidence: band=very_high; score=0.9
- Line 718: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult result;
  Confidence: band=very_high; score=0.9
- Line 729: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult AiHardwareDispatcher::dispatchNNAPI([[maybe_unused]] AiInferenceRequest &req) {
  Confidence: band=very_high; score=0.9
- Line 731: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (!req.input_data || req.input_elements == 0) {
  Confidence: band=very_high; score=0.9
- Line 732: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return makeError(BackendType::NNAPI, "Invalid input: null or empty");
  Confidence: band=very_high; score=0.9
- Line 748: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult result;
  Confidence: band=very_high; score=0.9
- Line 761: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult AiHardwareDispatcher::dispatchOnnxRuntime([[maybe_unused]] AiInferenceRequest &req) {
  Confidence: band=very_high; score=0.9
- Line 763: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (req.input_data == nullptr || req.input_elements == 0) {
  Confidence: band=very_high; score=0.9
- Line 764: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return makeError(BackendType::ONNX_RUNTIME, "Invalid input: null or empty");
  Confidence: band=very_high; score=0.9
- Line 771: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult result;
  Confidence: band=very_high; score=0.9
- Line 817: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Build input shape
  Confidence: band=very_high; score=0.9
- Line 818: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::vector<int64_t> shape(req.input_shape.begin(), req.input_shape.end());
  Confidence: band=very_high; score=0.9
- Line 824: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Input tensor
  Confidence: band=very_high; score=0.9
- Line 825: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: OrtValue *input_tensor = nullptr;
  Confidence: band=very_high; score=0.9
- Line 826: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: ort->CreateTensorWithDataAsOrtValue(mem_info, const_cast<float *>(req.input_data),
  Confidence: band=very_high; score=0.9
- Line 826: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ort->CreateTensorWithDataAsOrtValue(mem_info, const_cast<float *>(req.input_data),
- Line 827: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: req.input_elements * sizeof(float), shape.data(), shape.size(),
  Confidence: band=very_high; score=0.9
- Line 828: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, &input_tensor);
  Confidence: band=very_high; score=0.9
- Line 830: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: // Query input/output names
  Confidence: band=very_high; score=0.9
- Line 833: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: size_t input_count = 0, output_count = 0;
  Confidence: band=very_high; score=0.9
- Line 834: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: ort->SessionGetInputCount(session, &input_count);
  Confidence: band=very_high; score=0.9
- Line 837: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: std::vector<char *> input_names_raw(input_count);
  Confidence: band=very_high; score=0.9
- Line 839: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: for (size_t i = 0; i < input_count; ++i) {
  Confidence: band=very_high; score=0.9
- Line 840: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: ort->SessionGetInputName(session, i, alloc, &input_names_raw[i]);
  Confidence: band=very_high; score=0.9
- Line 846: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: const char *const *in_names  = const_cast<const char *const *>(input_names_raw.data());
  Confidence: band=very_high; score=0.9
- Line 849: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Run inference
  Confidence: band=very_high; score=0.9
- Line 851: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: status = ort->Run(session, nullptr, in_names, &input_tensor, input_count, out_names, output_count,
  Confidence: band=very_high; score=0.9
- Line 851: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: status = ort->Run(session, nullptr, in_names, &input_tensor, input_count, out_names, output_count,
- Line 912: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult AiHardwareDispatcher::dispatchGpuFallback(AiInferenceRequest &req) {
  Confidence: band=very_high; score=0.9
- Line 915: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: // Here we provide a graceful fallback path to CPU when no GPU inference
  Confidence: band=very_high; score=0.9
- Line 921: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult AiHardwareDispatcher::dispatchCpuFallback(AiInferenceRequest &req) {
  Confidence: band=very_high; score=0.9
- Line 922: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: if (req.input_data == nullptr || req.input_elements == 0) {
  Confidence: band=very_high; score=0.9
- Line 923: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: return makeError(BackendType::CPU, "Invalid input: null or empty");
  Confidence: band=very_high; score=0.9
- Line 930: severity=HIGH; category=llm_ai_safety; pattern=unvalidated_llm_output
  Description: LLM output used without validation (hallucination/bias risk)
  Context: AiInferenceResult result;
  Confidence: band=very_high; score=0.9
- Line 934: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: result.output.assign(req.input_data, req.input_data + req.input_elements);
  Confidence: band=very_high; score=0.9
- Line 935: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: result.output_shape = req.input_shape;
  Confidence: band=very_high; score=0.9
- Line 664: severity=MEDIUM; category=llm_ai_safety; pattern=missing_resource_limits
  Description: LLM inference without token limit or timeout (DOS risk)
  Context: infer_req.infer();
  Confidence: band=high; score=0.74
- Line 675: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.output_shape.push_back(static_cast<int64_t>(dim));
  Confidence: band=high; score=0.74
- Line 676: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: result.output_shape.push_back(static_cast<int64_t>(dim));

### src/acceleration/cuda_backend.cpp
Total findings: 87

- Line 348: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto hnswResults = hnswEngine_->batchSearch(queries, numQueries, static_cast<uint32_t>(k), ef);
- Line 457: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto hnswResults = hnswEngine_->batchSearch(queries, numQueries, static_cast<uint32_t>(k));
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
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4618 feat(acceleration): Kernel ... (2026-04-13) | #4320 feat(acceleration):
- Line 737: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_queries.get(), 0, querySize);
  Confidence: band=very_high; score=0.9
- Line 738: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_vectors.get(), 0, vectorSize);
  Confidence: band=very_high; score=0.9
- Line 739: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_distances.get(), 0, distanceSize);
  Confidence: band=very_high; score=0.9
- Line 740: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_topkIndices.get(), 0, topkIdxSize);
  Confidence: band=very_high; score=0.9
- Line 741: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_topkDistances.get(), 0, topkDistSize);
  Confidence: band=very_high; score=0.9
- Line 742: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: // cudaMemset is synchronous: it blocks the host until the fill is
  Confidence: band=very_high; score=0.9
- Line 836: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyAsync(entry->d_queries.get(), queries, querySize, cudaMemcpyHostToDevice, mainStream);
  Confidence: band=very_high; score=0.9
- Line 837: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyAsync(entry->d_vectors.get(), vectors, vectorSize, cudaMemcpyHostToDevice, mainStream);
  Confidence: band=very_high; score=0.9
- Line 853: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyAsync(topkIndices.data(), entry->d_topkIndices.get(), topkIdxSize, cudaMemcpyDeviceToHost,
  Confidence: band=very_high; score=0.9
- Line 853: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: cudaMemcpyAsync(topkIndices.data(), entry->d_topkIndices.get(), topkIdxSize, cudaMemcpyDeviceToHost,
- Line 855: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyAsync(topkDistances.data(), entry->d_topkDistances.get(), topkDistSize, cudaMemcpyDeviceToHost,
  Confidence: band=very_high; score=0.9
- Line 855: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: cudaMemcpyAsync(topkDistances.data(), entry->d_topkDistances.get(), topkDistSize, cudaMemcpyDeviceTo
- Line 1254: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_adjacency.get(), 0, adjSize);
  Confidence: band=very_high; score=0.9
- Line 1255: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_startVertices.get(), 0, svSize);
  Confidence: band=very_high; score=0.9
- Line 1256: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_frontier_a.get(), 0, frontierSz);
  Confidence: band=very_high; score=0.9
- Line 1257: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_frontier_b.get(), 0, frontierSz);
  Confidence: band=very_high; score=0.9
- Line 1258: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_visited.get(), 0, frontierSz);
  Confidence: band=very_high; score=0.9
- Line 1259: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_depths.get(), 0, frontierSz);
  Confidence: band=very_high; score=0.9
- Line 1260: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_result_vertices.get(), 0, resultsSz);
  Confidence: band=very_high; score=0.9
- Line 1261: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_result_sizes.get(), 0, sizesSz);
  Confidence: band=very_high; score=0.9
- Line 1343: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // Replay: copy inputs → device, launch graph, copy results ← device
  Confidence: band=very_high; score=0.9
- Line 1345: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyAsync(entry->d_adjacency.get(), adjacency, adjSize, cudaMemcpyHostToDevice, mainStream);
  Confidence: band=very_high; score=0.9
- Line 1346: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyAsync(entry->d_startVertices.get(), startVertices, svSize, cudaMemcpyHostToDevice, mainStream);
  Confidence: band=very_high; score=0.9
- Line 1360: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyAsync(h_result_vertices.data(), entry->d_result_vertices.get(), resultsSz, cudaMemcpyDeviceToHost,
  Confidence: band=very_high; score=0.9
- Line 1362: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: cudaMemcpyAsync(h_result_sizes.data(), entry->d_result_sizes.get(), sizesSz, cudaMemcpyDeviceToHost,
- Line 1458: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_adjacency.get(), 0, adjSize);
  Confidence: band=very_high; score=0.9
- Line 1459: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_weights.get(), 0, wgtSize);
  Confidence: band=very_high; score=0.9
- Line 1460: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_startVertices.get(), 0, svSize);
  Confidence: band=very_high; score=0.9
- Line 1461: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_distances.get(), 0, distSize);
  Confidence: band=very_high; score=0.9
- Line 1462: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemset() without error checking
  Context: cudaMemset(newEntry.d_predecessors.get(), 0, predSize);
  Confidence: band=very_high; score=0.9
- Line 1530: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyAsync(entry->d_adjacency.get(), adjacency, adjSize, cudaMemcpyHostToDevice, mainStream);
  Confidence: band=very_high; score=0.9
- Line 1531: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyAsync(entry->d_weights.get(), weights, wgtSize, cudaMemcpyHostToDevice, mainStream);
  Confidence: band=very_high; score=0.9
- Line 1532: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyAsync(entry->d_startVertices.get(), startVertices, svSize, cudaMemcpyHostToDevice, mainStream);
  Confidence: band=very_high; score=0.9
- Line 1546: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyAsync(h_distances.data(), entry->d_distances.get(), distSize, cudaMemcpyDeviceToHost, mainStream);
  Confidence: band=very_high; score=0.9
- Line 1547: severity=HIGH; category=gpu_memory_safety; pattern=unchecked_cuda_call
  Description: CUDA call cudaMemcpy() without error checking
  Context: cudaMemcpyAsync(h_predecessors.data(), entry->d_predecessors.get(), predSize, cudaMemcpyDeviceToHost,
  Confidence: band=very_high; score=0.9
- Line 1547: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: cudaMemcpyAsync(h_predecessors.data(), entry->d_predecessors.get(), predSize, cudaMemcpyDeviceToHost
- Line 1816: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: const size_t resultSize = numPoints * sizeof(uint8_t);
- Line 356: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: row.emplace_back(static_cast<uint32_t>(r.id), r.score);
  Confidence: band=high; score=0.74
- Line 463: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: row.emplace_back(static_cast<uint32_t>(r.id), r.score);
  Confidence: band=high; score=0.74
- Line 550: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results[q].emplace_back(static_cast<uint32_t>(topkIndices[idx]), topkDistances[idx]);
  Confidence: band=high; score=0.74
- Line 872: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results[q].emplace_back(static_cast<uint32_t>(topkIndices[idx]), topkDistances[idx]);
  Confidence: band=high; score=0.74
- Line 1575: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: path.push_back(cur);
  Confidence: band=high; score=0.74
- Line 1575: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: path.push_back(cur);
  Confidence: band=high; score=0.74

### src/acceleration/faiss_gpu_backend.cpp
Total findings: 73

- Line 129: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.99
- Line 163: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto* idx = new faiss::gpu::GpuIndexFlatL2(
- Line 172: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto* idx = new faiss::gpu::GpuIndexFlatIP(
- Line 182: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto* quantizer = new faiss::gpu::GpuIndexFlatL2(
- Line 191: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto* idx = new faiss::gpu::GpuIndexIVFFlat(
- Line 199: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: idx->nprobe = config_.nprobe;
- Line 205: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto* quantizer = new faiss::gpu::GpuIndexFlatL2(
- Line 214: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto* idx = new faiss::gpu::GpuIndexIVFPQ(
- Line 223: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: idx->nprobe = config_.nprobe;
- Line 232: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto* idx = new faiss::gpu::GpuIndexIVFScalarQuantizer(
- Line 241: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: idx->nprobe = config_.nprobe;
- Line 248: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: auto* idx = new faiss::IndexHNSWFlat(dimension, config_.hnswM);
- Line 253: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.99
- Line 299: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.99
- Line 327: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.99
- Line 354: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.99
- Line 382: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.99
- Line 418: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.99
- Line 446: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.99
- Line 502: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.99
- Line 503: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: "computeDistances: null pointers or zero-size inputs");
  Confidence: band=very_high; score=0.99
- Line 566: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.99
- Line 690: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.99
- Line 750: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: config_.dimension = gpuIndex->d;
- Line 800: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: stats.numVectors = static_cast<size_t>(idx->ntotal);
- Line 801: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: stats.dimension  = static_cast<size_t>(idx->d);
- Line 844: severity=CRITICAL; category=llm_ai_safety; pattern=prompt_injection
  Description: User input in prompt without sanitization (injection risk)
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.99
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 129: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.9
- Line 253: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.9
- Line 266: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<faiss::gpu::GpuIndexFlatL2*>(index_);
- Line 269: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<faiss::gpu::GpuIndexFlatIP*>(index_);
- Line 272: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<faiss::gpu::GpuIndexIVFFlat*>(index_);
- Line 275: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<faiss::gpu::GpuIndexIVFPQ*>(index_);
- Line 278: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<faiss::gpu::GpuIndexIVFScalarQuantizer*>(index_);
- Line 281: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<faiss::IndexHNSWFlat*>(index_);
- Line 285: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<faiss::Index*>(index_);
- Line 299: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.9
- Line 327: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.9
- Line 354: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.9
- Line 382: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.9
- Line 418: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.9
- Line 446: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.9
- Line 502: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.9
- Line 503: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: "computeDistances: null pointers or zero-size inputs");
  Confidence: band=very_high; score=0.9
- Line 503: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: "computeDistances: null pointers or zero-size inputs");
  Confidence: band=very_high; score=0.9
- Line 536: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<faiss::gpu::GpuIndexFlatL2*>(tempIndex);
- Line 538: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<faiss::gpu::GpuIndexFlatIP*>(tempIndex);
- Line 547: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<faiss::gpu::GpuIndexFlatL2*>(tempIndex);
- Line 549: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<faiss::gpu::GpuIndexFlatIP*>(tempIndex);
- Line 566: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.9
- Line 567: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: "batchKnnSearch: null pointers or zero-size inputs");
  Confidence: band=very_high; score=0.9
- Line 619: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<faiss::gpu::GpuIndexFlatL2*>(tempIndex);
- Line 621: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<faiss::gpu::GpuIndexFlatIP*>(tempIndex);
- Line 632: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<faiss::gpu::GpuIndexFlatL2*>(tempIndex);
- Line 634: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: static_cast = nullptr;
  Context: delete static_cast<faiss::gpu::GpuIndexFlatIP*>(tempIndex);
- Line 690: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.9
- Line 697: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: cpuIndex = nullptr;
  Context: delete cpuIndex;
- Line 741: severity=HIGH; category=delete_no_nullptr
  Description: Delete without nullifying pointer — use-after-free risk
  Remediation: After delete: cpuIndex = nullptr;
  Context: delete cpuIndex;
- Line 844: severity=HIGH; category=llm_ai_safety; pattern=unsanitized_llm_input
  Description: User input passed to LLM without normalization/sanitization
  Context: setError(AccelerationErrorCode::InvalidInputShape,
  Confidence: band=very_high; score=0.9
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['    Config tempConfig;', '    tempConfig.indexType = useL2 ? IndexType::FLAT_L2 : IndexType::FLAT_IP;', '    tempConfig.dimension = static_cast<int>(dim);', '    tempConfig.deviceId = config_.deviceId;', '']
  Confidence: band=medium; score=0.65
- Line 0: severity=MEDIUM; category=uncategorized
  Context: ['    Config tempConfig;', '    tempConfig.indexType = useL2 ? IndexType::FLAT_L2 : IndexType::FLAT_IP;', '    tempConfig.dimension = static_cast<int>(dim);', '    tempConfig.deviceId = config_.deviceId;', '']
  Confidence: band=medium; score=0.65
- Line 272: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete static_cast<faiss::gpu::GpuIndexIVFFlat*>(index_);
- Line 275: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete static_cast<faiss::gpu::GpuIndexIVFPQ*>(index_);
- Line 278: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete static_cast<faiss::gpu::GpuIndexIVFScalarQuantizer*>(index_);
- Line 281: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete static_cast<faiss::IndexHNSWFlat*>(index_);
- Line 285: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete static_cast<faiss::Index*>(index_);
- Line 475: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results[q].emplace_back(
  Confidence: band=high; score=0.74
- Line 536: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete static_cast<faiss::gpu::GpuIndexFlatL2*>(tempIndex);
- Line 538: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete static_cast<faiss::gpu::GpuIndexFlatIP*>(tempIndex);
- Line 608: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results[q].emplace_back(
  Confidence: band=high; score=0.74
- Line 619: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete static_cast<faiss::gpu::GpuIndexFlatL2*>(tempIndex);
- Line 621: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: delete static_cast<faiss::gpu::GpuIndexFlatIP*>(tempIndex);

### src/acceleration/graphics_backends.cpp
Total findings: 54

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4928 [Docs][acceleration] Aktual... (2026-05-10) | #4206 feat(acceleration):
- Line 1227: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: std::cout << "[Vulkan] VK_KHR_buffer_device_address: "
- Line 1337: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: ? " [VK_KHR_buffer_device_address]"
- Line 1338: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: : " [no VK_KHR_buffer_device_address]");
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

### src/acceleration/plugin_security.cpp
Total findings: 42

- Line 1287: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: X509 *cert             = d2i_X509(nullptr, &p, static_cast<long>(cert_data->size()));
- Line 1391: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: const std::string &signing_cert = metadata->signature.signingCertificate;
- Line 2046: severity=CRITICAL; category=no_timeout
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
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4292 fix(acceleration): PE certi... (2026-03-16) | #4283 feat(acceleration):
- Line 1148: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: j["events"] = json::array();
- Line 0: severity=MEDIUM; category=uncategorized
  Context: Struct with uninitialized fields
  Confidence: band=medium; score=0.65
- Line 193: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: outBytes.push_back(byte);
  Confidence: band=high; score=0.74
- Line 287: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(mdctx);
- Line 295: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(mdctx);
- Line 298: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(mdctx);
- Line 578: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(mdctx);
- Line 579: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(pubKey);
- Line 580: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(cert);
- Line 765: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_STORE_CTX_free(chain_ctx);
- Line 772: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(crl_issuer_key);
- Line 773: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_STORE_free(trust_store);
- Line 774: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_CRL_free(crl);
- Line 777: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_PKEY_free(crl_issuer_key);
- Line 783: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_STORE_free(trust_store);
- Line 793: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_CRL_free(crl);
- Line 828: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: sk_DIST_POINT_pop_free(crldp, DIST_POINT_free);
- Line 902: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_STORE_CTX_free(chain_ctx);
- Line 943: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: OCSP_REQUEST_free(req);
- Line 950: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: OPENSSL_free(req_der);
- Line 982: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: OCSP_BASICRESP_free(basic);
- Line 999: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: OCSP_CERTID_free(lookup_id);
- Line 1005: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: OCSP_BASICRESP_free(basic);
- Line 1037: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_free(issuer_cert);
- Line 1040: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_STORE_free(trust_store);
- Line 1042: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: X509_email_free(ocsp_list);
- Line 1092: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(event);
  Confidence: band=high; score=0.74
- Line 1158: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: j["events"].push_back(eventJson);
  Confidence: band=high; score=0.74
- Line 1838: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: if (pe_signature == 0x00004550) { // "PE\0\0"
- Line 2034: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(pipefd[1]);
- Line 2050: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: close(pipefd[0]);
- Line 2114: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(mdctx);
- Line 2122: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(mdctx);
- Line 2125: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(mdctx);
- Line 2150: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(mdctx);

### src/acceleration/oneapi_backend.cpp
Total findings: 13

- Line 72: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: queue_ = new sycl::queue(sycl::gpu_selector_v);
- Line 76: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: queue_ = new sycl::queue(sycl::default_selector_v);
- Line 79: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: queue_ = new sycl::queue(sycl::default_selector_v);
- Line 83: severity=CRITICAL; category=smart_ptr_misuse
  Description: Raw new without immediate wrapping in smart pointer
  Remediation: Use auto ptr = std::make_unique<T>(...);
  Context: queue_ = new sycl::queue(sycl::default_selector_v);
- Line 86: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: auto device = queue_->get_device();
- Line 171: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: }).wait();
- Line 175: severity=CRITICAL; category=no_timeout
  Description: semaphore_wait without timeout — can block indefinitely
  Remediation: Add timeout parameter (e.g., wait_for(timeout), with_timeout())
  Context: queue_->memcpy(distances.data(), d_distances, resultSize * sizeof(float)).wait();
- Line 175: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: queue_->memcpy(distances.data(), d_distances, resultSize * sizeof(float)).wait();
- Line 68: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: bool initialize() override {
  Confidence: band=medium; score=0.66
- Line 178: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: sycl::free(d_queries, *queue_);
- Line 179: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: sycl::free(d_vectors, *queue_);
- Line 202: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pairs.push_back({distances[q * numVectors + v], v});
  Confidence: band=high; score=0.74
- Line 259: severity=MEDIUM; category=observability; pattern=missing_health_check
  Description: Service initialization without nearby health/status handling
  Context: bool initialize() override { return false; }
  Confidence: band=medium; score=0.66

### src/acceleration/cpu_backend.cpp
Total findings: 9

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3466 docs(acceleration): Add IEE... (2026-03-12) | #3111 [geo] Implement run
- Line 97: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: // happens when the same computational path is applied to equal inputs.
  Confidence: band=very_high; score=0.9
- Line 101: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (a.second != b.second) {
  Confidence: band=very_high; score=0.9
- Line 88: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: distances.emplace_back(static_cast<uint32_t>(v), dist);
  Confidence: band=high; score=0.74
- Line 137: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results[s].push_back(start);
  Confidence: band=high; score=0.74
- Line 158: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results[s].push_back(v);
  Confidence: band=high; score=0.74
- Line 229: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "[CPUGraph] batchShortestPath: negative weight " << raw_w << " on edge " << u << "→"
- Line 248: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: path.push_back(static_cast<uint32_t>(v));
  Confidence: band=high; score=0.74
- Line 249: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: path.push_back(static_cast<uint32_t>(v));

### src/acceleration/vllm_resource_manager.cpp
Total findings: 9

- Line 153: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: if (shared_future->wait_for(std::chrono::milliseconds(500)) == std::future_status::ready) {
- Line 154: severity=CRITICAL; category=data_race
  Description: Shared data access without lock protection
  Remediation: Protect shared data with std::lock_guard or std::unique_lock
  Context: gpu_util = shared_future->get();
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 153: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: if (shared_future->wait_for(std::chrono::milliseconds(500)) == std::future_status::ready) {
- Line 237: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 319: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: std::this_thread::sleep_for(std::chrono::milliseconds(100));
- Line 426: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: nvml_devices_.push_back(static_cast<void *>(dev));
  Confidence: band=high; score=0.74
- Line 427: severity=MEDIUM; category=copy_overhead
  Description: push_back in loop — consider pre-allocating with reserve()
  Remediation: Call vector.reserve(expected_size) before loop to avoid reallocations
  Context: nvml_devices_.push_back(static_cast<void *>(dev));

### src/acceleration/backend_registry.cpp
Total findings: 8

- Line 0: severity=HIGH; category=uncategorized
  Confidence: band=high; score=0.73
- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4928 [Docs][acceleration] Aktual... (2026-05-10) | #4620 feat(acceleration):
- Line 114: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // Register OpenCL backend for broad hardware compatibility.
  Confidence: band=high; score=0.8
- Line 234: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: return (it != typeIndex_.end()) ? it->second.base : nullptr;
- Line 543: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: return runtimeInitialized_.load(std::memory_order_acquire);
- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 274: severity=MEDIUM; category=determinism; pattern=unordered_container_iter
  Description: Non-deterministic unordered_map/set iteration order
  Context: static T *selectTyped(const std::unordered_map<BackendType, RegisteredBackend> &index,
  Confidence: band=medium; score=0.66
- Line 415: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: types.push_back(backend->type());
  Confidence: band=high; score=0.74

### src/acceleration/multi_gpu_backend.cpp
Total findings: 7

- Line 96: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: deviceIds.push_back(i);
  Confidence: band=high; score=0.74
- Line 133: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "MultiGPUVectorBackend: sub-backend init failed for device " << shardDescs[i].deviceId
- Line 137: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "MultiGPUVectorBackend: warning — sub-backend init failed "
- Line 140: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: subBackends.push_back(std::move(sb));
  Confidence: band=high; score=0.74
- Line 272: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged[q].emplace_back(globalIdx, dist);
  Confidence: band=high; score=0.74
- Line 272: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged[q].emplace_back(globalIdx, dist);
  Confidence: band=high; score=0.74
- Line 272: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: merged[q].emplace_back(globalIdx, dist);
  Confidence: band=high; score=0.74

### src/acceleration/geo_acceleration_bridge.cpp
Total findings: 6

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4928 [Docs][acceleration] Aktual... (2026-05-10) | #3609 feat(acceleration):
- Line 159: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: themis::geo::SpatialBatchInputs batch;
  Confidence: band=very_high; score=0.9
- Line 283: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: "invalid inputs (null pointer or < 3 polygon vertices)");
  Confidence: band=very_high; score=0.9
- Line 299: severity=HIGH; category=audit_logging; pattern=hardcoded_output
  Description: Hardcoded std::cout/printf instead of structured logging
  Context: themis::geo::SpatialBatchInputs batch;
  Confidence: band=very_high; score=0.9
- Line 164: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pt.coords.push_back({point_lats[i], point_lons[i]});
  Confidence: band=high; score=0.74
- Line 305: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pt.coords.push_back({pointLats[i], pointLons[i]});
  Confidence: band=high; score=0.74

### src/acceleration/plugin_loader.cpp
Total findings: 6

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4928 [Docs][acceleration] Aktual... (2026-05-10) | #3581 docs(plugins, promp
- Line 247: severity=HIGH; category=range_temporary
  Description: Range-for on temporary container — references may be invalid
  Remediation: Store container in variable first: auto c = func(); for (auto x : c) { ... }
  Context: for (const auto &entry : fs::directory_iterator(directoryPath)) {
- Line 261: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "SECURITY: Skipping symlink that escapes plugin directory: " << entry.path() << std::en
- Line 293: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cout << "Unloading plugin: " << pluginName << std::endl;
- Line 303: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cout << "Unloading plugin: " << plugin.name << std::endl;
- Line 322: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(plugin.plugin.get());
  Confidence: band=high; score=0.74

### src/acceleration/directx_backend_full.cpp
Total findings: 5

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3665 feat(acceleration): Impleme... (2026-03-12) | #417 [DOCS] CRITICAL: Cor
- Line 58: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: snprintf(_buf, sizeof(_buf), "DirectX error: HRESULT 0x%08X", \
- Line 618: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: DirectXVectorBackend::getCapabilities()
  Context: BackendCapabilities DirectXVectorBackend::getCapabilities() const {
  Confidence: band=medium; score=0.56
- Line 630: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: DirectXVectorBackend::initialize()
  Context: bool DirectXVectorBackend::initialize() {
  Confidence: band=medium; score=0.56
- Line 642: severity=MEDIUM; category=legacy_duplication; pattern=duplicate_qualified_signature
  Description: Potential duplicate implementation signature across files: DirectXVectorBackend::shutdown()
  Context: void DirectXVectorBackend::shutdown() {
  Confidence: band=medium; score=0.56

### src/acceleration/hip_backend.cpp
Total findings: 5

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4618 feat(acceleration): Kernel ... (2026-04-13) | #4470 feat(acceleration):
- Line 1053: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: const size_t resultBytes = numPoints          * sizeof(uint8_t);
- Line 435: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cout << "Device " << i << ": " << prop.name
- Line 735: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results[q].emplace_back(indices[q * effectiveK + i], topKDistances[q * effectiveK + i]);
  Confidence: band=high; score=0.74
- Line 799: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: devices.push_back(info);
  Confidence: band=high; score=0.74

### src/acceleration/nccl_vector_backend.cpp
Total findings: 5

- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 146: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "CUDA error: " << cudaGetErrorString(err)
- Line 146: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "CUDA error: " << cudaGetErrorString(err)
- Line 157: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "CUDA error: " << cudaGetErrorString(err)
- Line 157: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "CUDA error: " << cudaGetErrorString(err)

### src/acceleration/rccl_vector_backend.cpp
Total findings: 5

- Line 0: severity=MEDIUM; category=uncategorized
  Confidence: band=medium; score=0.57
- Line 173: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "HIP error: " << hipGetErrorString(err)
- Line 173: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "HIP error: " << hipGetErrorString(err)
- Line 184: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "HIP error: " << hipGetErrorString(err)
- Line 184: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cerr << "HIP error: " << hipGetErrorString(err)

### src/acceleration/shader_integrity.cpp
Total findings: 5

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4928 [Docs][acceleration] Aktual... (2026-05-10) | #3609 feat(acceleration):
- Line 125: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: return verify(name, reinterpret_cast<const uint8_t *>(spvWords.data()), spvWords.size() * sizeof(uin
- Line 202: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: return sha256Hex(reinterpret_cast<const uint8_t *>(spvWords.data()), spvWords.size() * sizeof(uint32
- Line 189: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);
- Line 192: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: EVP_MD_CTX_free(ctx);

### src/acceleration/opencl_backend.cpp
Total findings: 4

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #2708 feat(acceleration): OpenCL ... (2026-03-12) | #417 [DOCS] CRITICAL: Cor
- Line 290: severity=HIGH; category=size_assumption
  Description: Hardcoded size assumption — pointer/int size may differ on platforms
  Remediation: Use <cstdint> types (uint32_t, uint64_t) and sizeof() checks, not constants
  Context: clSetKernelArg(kernel, 4, sizeof(unsigned int), &uNumVectors);
- Line 336: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: results[q].push_back({pairs[i].second, pairs[i].first});
  Confidence: band=high; score=0.74
- Line 205: severity=LOW; category=observability; pattern=unstructured_log
  Description: Unstructured logging (use structured format)
  Context: std::vector<char> log(logSize);
  Confidence: band=medium; score=0.6

### src/acceleration/zluda_backend.cpp
Total findings: 4

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #3609 feat(acceleration): wire mi... (2026-03-12) | #3551 docs(chimera + acce
- Line 11: severity=HIGH; category=legacy_duplication; pattern=legacy_or_compat_path
  Description: Legacy/compatibility/deprecation marker detected (review removal/containment plan).
  Context: // ZLUDA: CUDA compatibility layer for AMD GPUs
  Confidence: band=high; score=0.8
- Line 363: severity=HIGH; category=pointer_arithmetic
  Description: Pointer/array access without bounds validation
  Remediation: Add bounds check before dereferencing
  Context: [[nodiscard]] std::unique_ptr<IVectorBackend> createZLUDABackend() {
- Line 190: severity=MEDIUM; category=manual_cleanup
  Description: Manual cleanup outside exception handler — not exception-safe
  Remediation: Use RAII or smart pointers for automatic cleanup in all exception paths
  Context: if (zludaLib_) dlclose(zludaLib_);

### src/acceleration/compute_backend.cpp
Total findings: 3

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4928 [Docs][acceleration] Aktual... (2026-05-10)
- Line 77: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: validIndices.push_back(q);
  Confidence: band=high; score=0.74
- Line 77: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: validIndices.push_back(q);
  Confidence: band=high; score=0.74

### src/acceleration/device_manager.cpp
Total findings: 3

- Line 5: severity=HIGH; category=uninitialized_access
  Description: Container element access before initialization
  Remediation: Use .at() for bounds checking or initialize element first
  Context: * PR History (last 5): #4928 [Docs][acceleration] Aktual... (2026-05-10) | #4207 feat(acceleration):
- Line 111: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: result.push_back(fromGpuDeviceInfo(d));
  Confidence: band=high; score=0.74
- Line 223: severity=MEDIUM; category=expensive_inner_op
  Description: I/O operation in inner loop — very expensive
  Remediation: Buffer output or move I/O outside loop
  Context: std::cout << "  [" << (d.is_healthy ? "OK" : "!!") << "] " << d.name

### src/acceleration/vec_knn.cpp
Total findings: 3

- Line 342: severity=HIGH; category=repeated_search
  Description: find/search in loop — O(n²) or worse
  Remediation: Move search outside loop or build index/map before loop
  Context: auto oi = std::find(order_.begin(), order_.end(), k);
- Line 480: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: pks.push_back(e.getPrimaryKey());
  Confidence: band=high; score=0.74
- Line 511: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: batch.push_back(entities[i]);
  Confidence: band=high; score=0.74

### src/acceleration/cpu_backend_mt.cpp
Total findings: 2

- Line 72: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cout << "  SIMD: AVX2/AVX-512\n";
- Line 187: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: distances.emplace_back(static_cast<uint32_t>(v), dist);
  Confidence: band=high; score=0.74

### src/acceleration/cpu_backend_tbb.cpp
Total findings: 2

- Line 61: severity=MEDIUM; category=hardcoded_path
  Description: Hardcoded path separator — not portable
  Remediation: Use std::filesystem::path or boost::filesystem for cross-platform paths
  Context: std::cout << "  SIMD: AVX2/AVX-512\n";
- Line 298: severity=MEDIUM; category=performance; pattern=missing_vector_reserve
  Description: vector::push_back in loop without prior reserve()
  Context: distances.emplace_back(static_cast<uint32_t>(v), dist);
  Confidence: band=high; score=0.74

### src/acceleration/tensor_core_matmul.cpp
Total findings: 2

- Line 42: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: if (beta == 0.0f) {
  Confidence: band=very_high; score=0.9
- Line 44: severity=HIGH; category=determinism; pattern=fp_exact_comparison
  Description: Floating-point exact comparison (use tolerance/epsilon)
  Context: } else if (beta != 1.0f) {
  Confidence: band=very_high; score=0.9

### src/acceleration/vulkan_backend_full.cpp
Total findings: 1

- Line 533: severity=HIGH; category=db_connection_leak
  Description: Resource acquired but not released — potential leak
  Remediation: Ensure all acquire() calls are matched with release() in all code paths
  Context: throw std::runtime_error("Failed to allocate buffer memory");

## Update Workflow

- Refresh scan artifacts with: python tools/gap_scanner_v3.py
- Regenerate all module notes with: python tools/module_doc_generator.py . ai_working ai_working/module_gaps
- The generator mirrors each archive document directly into src/<module>/MODULE_GAPS.md.

Format: THEMIS_MODULE_GAPS_V3
