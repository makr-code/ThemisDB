# GPU Module - Future Enhancements

## Status Key

- ✅ **Infrastructure implemented** — CPU-level bookkeeping and API in place;
  ready to wire up real CUDA/ROCm calls.
- ⬜ **Blocked on hardware** — requires a CUDA/ROCm driver or device to complete.

---

## Features

### CUDA Kernel Support
**Priority:** High | **Target Version:** v1.1.0 | **Status:** ⬜ Blocked on hardware

Custom CUDA kernels for specialised operations.

**Implemented infrastructure:**
- ✅ `GPUKernelValidator` — checksum/whitelist registry, validate-before-launch
- ✅ `GPULauncher` — typed async work-item / batch launcher with `BackendFn` hook
- ✅ `GPUStreamManager` — named async streams, CPU fallback budget enforcement

**Remaining (hardware required):**
- Wire `cudaMalloc` / `hipMalloc` into `GPUMemoryManager`
- Implement CUDA stream creation in `GPUStreamManager`
- Plug kernel `.ptx` / `.hsaco` blobs into `GPULauncher::BackendFn`
- Activate `cudaMemset` zero-on-free in `GPUMemoryPool::release()`

---

### GPU Query Acceleration
**Priority:** High | **Target Version:** v1.2.0 | **Status:** ✅ Infrastructure implemented

Accelerate database query operations using GPU.

**Implemented:**
- ✅ `GPUQueryAccelerator` — parallel scan with filter pushdown, sort (ASC/DESC),
  aggregate (SUM/COUNT/MIN/MAX/AVG), hash join
- ✅ CPU-path fallback for environments without GPU
- ✅ GPU-threshold dispatch: switches to GPU path above
  `Config::gpu_threshold_rows`
- ✅ Full unit-test coverage (`tests/test_gpu_query_accelerator.cpp`)

**Remaining (hardware required):**
- Replace CPU `std::stable_sort` with Thrust `stable_sort_by_key`
- Replace CPU reduction with `cub::DeviceReduce`
- Replace CPU hash join with a parallel GPU hash join kernel
- Replace sequential scan with `Thrust::copy_if` / `cub::DeviceSelect`

---

### Multi-GPU Support
**Priority:** Medium | **Target Version:** v1.3.0 | **Status:** ✅ Infrastructure implemented

Support for multiple GPUs and distributed computation.

**Implemented:**
- ✅ `GPULoadBalancer` — ROUND_ROBIN / LEAST_LOADED / FIRST_HEALTHY strategies,
  per-device VRAM tracking, `markDeviceFailed` / `resetDevice`
- ✅ `GPUDeviceDiscovery` — enumerate CUDA/ROCm devices, CPU-fallback sentinel,
  `GetBestDevice`, `GetHealthyDevices`

**Remaining (hardware required):**
- `cudaMemcpyPeer` / `hipMemcpyPeer` for GPU-to-GPU transfers
- NVLink / XGMI topology detection

---

### GPU Memory Pooling
**Priority:** Medium | **Target Version:** v1.2.0 | **Status:** ✅ Infrastructure implemented

Efficient VRAM allocation with pooling.

**Implemented:**
- ✅ `GPUMemoryPool` — slab-based pre-allocator, `setZeroOnFree`, fragmentation
  tracking, pool stats
- ✅ `GPUMemoryManager` — pre-allocation hints (`ReserveHint` / `ConsumeHint`),
  tenant-aware quotas, peak tracking

**Remaining (hardware required):**
- Replace bookkeeping counters with real `cudaMalloc` / `hipMalloc` calls

---

### GPU Tensor Buffer
**Priority:** Medium | **Target Version:** v1.2.0 | **Status:** ✅ Infrastructure implemented

Typed, self-describing tensor containers for ML workloads.

**Implemented:**
- ✅ `GPUTensorBuffer` — shape/dtype, host-side backing store, fill, copy,
  named views, serialise / deserialise for checkpointing, global stats
- ✅ Full unit-test coverage (`tests/test_gpu_tensor.cpp`)

**Remaining (hardware required):**
- Add `device_ptr_` member populated by `cudaMalloc` / `hipMalloc`
- `uploadToDevice()` / `downloadFromDevice()` via `cudaMemcpy`

---

### GPU Training Loop
**Priority:** Medium | **Target Version:** v1.3.0 | **Status:** ✅ Infrastructure implemented

Training loop coordinator for GPU-backed ML workloads.

**Implemented:**
- ✅ `GPUTrainingLoop` — batch iteration, loss tracking, early stopping,
  checkpoint callbacks, per-epoch statistics
- ✅ Full unit-test coverage (`tests/test_gpu_training_loop.cpp`)

**Remaining (hardware required):**
- Wire a real CUDA/ROCm forward+backward pass into the `LossFn` callback

---

## See Also

- [README.md](README.md) — Current module documentation
- [../../docs/gpu_roadmap.md](../../docs/gpu_roadmap.md) — Production-readiness
  assessment and full roadmap

---

*Last Updated: February 2026*  
*Module Version: v1.2.0*
