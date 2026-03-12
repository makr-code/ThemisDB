### Context

This issue implements the roadmap item 'TensorCore Matmul: INT8 Quantized Precision Path' for the acceleration domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Mid-term (v1.9.0 – v2.0.0) and targets milestone v1.9.0.

Primary detail section: TensorCore Matmul: INT8 Quantized Precision Path

### Goal

Deliver the scoped changes for TensorCore Matmul: INT8 Quantized Precision Path in src/acceleration/ and complete the linked detail section in a release-ready state for v1.9.0.

### Detailed Scope

### TensorCore Matmul: INT8 Quantized Precision Path
**Priority:** Medium
**Target Version:** v1.9.0

`compute_backend.h:83` declares `PrecisionMode::INT8` in the `PrecisionMode` bitmask enum. `tensor_core_matmul.cpp` implements FP16 (line 99), BF16 (line 108), and FP32 (line 117) dispatch cases but has no `INT8` case. Any caller requesting `MatrixPrecision::INT8` will fall through to an unhandled case with undefined behavior (no `default:` branch in the switch). CUDA `imma` (Integer Matrix Multiply Accumulate) instructions on sm_75+ (Turing and later) can provide 4× throughput over FP16 for inference workloads.

**Implementation Notes:**
- `[ ]` Add an `INT8` case in `TensorCoreMatmul::multiply()` (`tensor_core_matmul.cpp`) that dispatches to `launchINT8MatmulKernel()` using CUDA `cublasGemmEx` with `CUDA_R_8I` input type and `CUDA_R_32I` accumulator; include runtime guard `if (computeMajor < 7) return fallbackFP32(...)`.
- `[ ]` Add the corresponding `launchINT8MatmulKernel()` implementation in `cuda/tensor_core_matmul.cu` following the same structure as the FP16 kernel.
- `[ ]` Expose a `quantize(const float* src, int8_t* dst, size_t n, float scale)` helper and `dequantize()` inverse in `tensor_core_matmul.h` for callers that need to convert FP32 embeddings to INT8 before calling `multiply()`.
- `[ ]` Add a `default: /* log error and return {} */` branch to the switch in `TensorCoreMatmul::multiply()` to prevent undefined-behavior fall-through for any future unrecognised precision values.
- `[ ]` Update `CUDAMatrixBackend::getCapabilities()` to advertise `PrecisionMode::INT8` only when `computeMajor >= 7`.

**Performance Targets:**
- INT8 matmul throughput ≥ 2× FP16 throughput on RTX 3090 (sm_86) for 4096×4096 matrices.

---

### Acceptance Criteria

- [ ] Add an `INT8` case in `TensorCoreMatmul::multiply()` (`tensor_core_matmul.cpp`) that dispatches to `launchINT8MatmulKernel()` using CUDA `cublasGemmEx` with `CUDA_R_8I` input type and `CUDA_R_32I` accumulator; include runtime guard `if (computeMajor < 7) return fallbackFP32(...)`.
- [ ] Add the corresponding `launchINT8MatmulKernel()` implementation in `cuda/tensor_core_matmul.cu` following the same structure as the FP16 kernel.
- [ ] Expose a `quantize(const float* src, int8_t* dst, size_t n, float scale)` helper and `dequantize()` inverse in `tensor_core_matmul.h` for callers that need to convert FP32 embeddings to INT8 before calling `multiply()`.
- [ ] Add a `default: /* log error and return {} */` branch to the switch in `TensorCoreMatmul::multiply()` to prevent undefined-behavior fall-through for any future unrecognised precision values.
- [ ] Update `CUDAMatrixBackend::getCapabilities()` to advertise `PrecisionMode::INT8` only when `computeMajor >= 7`.
- [ ] INT8 matmul throughput ≥ 2× FP16 throughput on RTX 3090 (sm_86) for 4096×4096 matrices.

### Relationships

- Roadmap row: #220 (🟡 Medium Priority — Mid-term (v1.9.0 – v2.0.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/acceleration/FUTURE_ENHANCEMENTS.md#tensorcore-matmul-int8-quantized-precision-path
- Source key: roadmap:220:acceleration:v1.9.0:tensorcore-matmul-int8-quantized-precision-path

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:220:acceleration:v1.9.0:tensorcore-matmul-int8-quantized-precision-path -->
<!-- roadmap-ref: row=220;module=acceleration;target=v1.9.0 -->
<!-- roadmap-detail: src/acceleration/FUTURE_ENHANCEMENTS.md#tensorcore-matmul-int8-quantized-precision-path -->
