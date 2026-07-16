### Context

This issue implements the roadmap item '`GpuErasureCoderOpenCL`: Implement OpenCL Encode/Decode' for the sharding domain. It is sourced from the consolidated roadmap under 🟠 High Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: `GpuErasureCoderOpenCL`: Implement OpenCL Encode/Decode

### Goal

Deliver the scoped changes for `GpuErasureCoderOpenCL`: Implement OpenCL Encode/Decode in src/sharding/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### `GpuErasureCoderOpenCL`: Implement OpenCL Encode/Decode
**Priority:** High
**Target Version:** v1.8.0

`gpu_erasure_coder_opencl.cpp` (line 42: "OpenCL Implementation Class (Stub)") throws `std::runtime_error("OpenCL encode not implemented")` for all three operations: `encode`, `decode`, and `batchEncode`. The OpenCL erasure coding backend is completely non-functional.

**Implementation Notes:**
- `[ ]` Implement `OpenCLErasureCoder::encode()`: compile a Galois Field GF(2^8) multiply kernel via `clCreateProgramWithSource` at construction; enqueue an NDRange kernel to compute parity blocks in parallel.
- `[ ]` Implement `OpenCLErasureCoder::decode()`: perform syndrome computation and Gaussian elimination on the GPU to recover erased data blocks.
- `[ ]` Implement `batchEncode()`: batch multiple stripe operations into a single kernel dispatch.
- `[ ]` Add CPU/GPU parity test for encode+decode round-trip with 1, 2, and 3 erasures.

---

### Acceptance Criteria

- [ ] Implement `OpenCLErasureCoder::encode()`: compile a Galois Field GF(2^8) multiply kernel via `clCreateProgramWithSource` at construction; enqueue an NDRange kernel to compute parity blocks in parallel.
- [ ] Implement `OpenCLErasureCoder::decode()`: perform syndrome computation and Gaussian elimination on the GPU to recover erased data blocks.
- [ ] Implement `batchEncode()`: batch multiple stripe operations into a single kernel dispatch.
- [ ] Add CPU/GPU parity test for encode+decode round-trip with 1, 2, and 3 erasures.

### Relationships

- Roadmap row: #105 (🟠 High Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/sharding/FUTURE_ENHANCEMENTS.md#gpuerasurecoderopencl-implement-opencl-encodedecode
- Source key: roadmap:105:sharding:v1.8.0:gpuerasurecoderopencl-implement-opencl-encodedecode

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:105:sharding:v1.8.0:gpuerasurecoderopencl-implement-opencl-encodedecode -->
<!-- roadmap-ref: row=105;module=sharding;target=v1.8.0 -->
<!-- roadmap-detail: src/sharding/FUTURE_ENHANCEMENTS.md#gpuerasurecoderopencl-implement-opencl-encodedecode -->
