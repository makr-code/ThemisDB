# Acceleration Troubleshooting Guide

The `acceleration` module provides a hardware-agnostic compute backend registry for ThemisDB, enabling transparent selection between CPU (single-threaded, multi-threaded, TBB), CUDA, HIP/ROCm, DirectX, Metal, and FAISS GPU backends for compute-intensive operations.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| `BackendRegistry: no backend for device` | Requested backend not compiled in | Check build flags; use CPU fallback |
| CUDA backend fails to load | Missing `libcuda.so` | Install CUDA runtime |
| `FaissGpuBackend: index not on GPU` | GPU not available | Check `nvidia-smi`; enable fallback |
| Performance same with GPU enabled | Backend not registered | Verify `acceleration.default_backend` |
| `MetalBackend: macOS only` | Running on Linux | Use CUDA/HIP instead |
| `CpuBackendTbb: TBB not found` | Intel TBB not installed | Install `libtbb-dev` |
| Geo acceleration bridge fails | Geo module not initialised | Check `geo.enabled: true` |
| `HipBackend: device not found` | ROCm not installed | Install ROCm drivers |
| Backend switching overhead too high | Backend changes per-query | Pin to one backend |
| `DirectXBackend: D3D12 not available` | Not a Windows build | Use CUDA/HIP on Linux |

## Common Issues

### Issue 1: No Suitable Backend Found

**Description:** ThemisDB cannot find a compiled-in backend for the requested computation.

**Symptoms:**
- Log: `BackendRegistry: no backend registered for compute_type=vector_distance`
- Feature falls back to naive CPU implementation

**Cause:** ThemisDB was built without the required backend support.

**Solution:**
```bash
# Check which backends are compiled in
themisdb --version | grep -E "CUDA|ROCm|TBB|FAISS"
themisdb-admin acceleration backends list

# Check build flags
cat /usr/share/themisdb/build_info.json | grep -i backend
```
```yaml
acceleration:
  default_backend: cpu_mt          # "cpu" | "cpu_mt" | "cpu_tbb" | "cuda" | "hip"
  fallback_chain:
    - cuda
    - cpu_mt
    - cpu
```

---

### Issue 2: CUDA Backend Fails to Load

**Description:** CUDA backend cannot be used because the CUDA runtime library is missing.

**Symptoms:**
- Log: `CudaBackend: libcuda.so not found; backend disabled`
- All vector operations fall back to CPU

**Cause:** CUDA runtime not installed, or `LD_LIBRARY_PATH` not set.

**Solution:**
```bash
# Check CUDA installation
ldconfig -p | grep libcuda
nvidia-smi

# Add CUDA to library path
echo "/usr/local/cuda/lib64" >> /etc/ld.so.conf.d/cuda.conf
ldconfig
```

---

### Issue 3: FAISS GPU Backend Out of Memory

**Description:** FAISS GPU backend cannot allocate index on GPU.

**Symptoms:**
- Log: `FaissGpuBackend: failed to allocate index on GPU 0: out of memory`
- Fallback to FAISS CPU backend

**Cause:** GPU VRAM already used by other operations.

**Solution:**
```yaml
acceleration:
  faiss_gpu:
    enabled: true
    device_id: 0
    max_vram_fraction: 0.5         # use at most 50% of VRAM for FAISS
    tempMemory: 536870912          # 512MB scratch space
    use_float16: true              # halve memory usage
```

---

### Issue 4: TBB Backend Not Available

**Description:** Multi-threaded CPU backend cannot use TBB for parallel execution.

**Symptoms:**
- Log: `CpuBackendTbb: Intel TBB not found; using std::thread fallback`
- CPU parallelism is less efficient

**Cause:** TBB library not installed.

**Solution:**
```bash
# Install TBB
apt install libtbb-dev

# Verify
ldconfig -p | grep tbb
```

---

### Issue 5: Backend Not Selected Despite Configuration

**Description:** Configured backend is ignored; operations always use CPU.

**Symptoms:**
- `acceleration.default_backend: cuda` set but GPU not used
- No log line about CUDA backend initialisation

**Cause:** `AccelerationBackend` not initialised before the first operation; or backend init failed silently.

**Solution:**
```yaml
acceleration:
  default_backend: cuda
  strict_backend: false            # allow fallback instead of failing
  log_backend_selection: true      # log which backend is used for each op
  init_on_startup: true            # initialise backends at startup
```

## Diagnostic Commands

```bash
# List available backends
themisdb-admin acceleration backends list

# Benchmark backends
themisdb-admin acceleration benchmark \
  --operation vector_distance \
  --dimension 1536 \
  --count 10000

# Backend selection for specific operation
themisdb-admin acceleration select-backend \
  --operation matrix_multiply

# Live acceleration metrics
curl -s http://localhost:9100/metrics | grep themisdb_acceleration
```

## Configuration Reference

```yaml
acceleration:
  default_backend: cpu_mt
  fallback_chain: [cuda, cpu_tbb, cpu_mt, cpu]
  strict_backend: false
  faiss_gpu:
    enabled: false
    device_id: 0
    max_vram_fraction: 0.5
```

## Known Limitations

- Metal backend is available on macOS builds only; not available in Linux Docker images.
- DirectX backend requires a Windows build with D3D12; not available on Linux/macOS.
- FAISS GPU backend requires CUDA 11.0+; older CUDA versions fall back to CPU FAISS.
- TBB backend uses dynamic thread scheduling; thread count follows `omp_num_threads` or system CPU count.

## Related Documentation

- [Acceleration Module ROADMAP](../../src/acceleration/ROADMAP.md)
- [GPU Roadmap](../gpu_roadmap.md)
- [GPU Troubleshooting](./gpu_troubleshooting.md)
- [NCCL/RCCL Integration Guide](../llm_orchestration/NCCL_RCCL_INTEGRATION_GUIDE.md)
