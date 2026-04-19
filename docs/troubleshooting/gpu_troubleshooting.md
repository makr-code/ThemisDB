# GPU Troubleshooting Guide

The `gpu` module provides GPU acceleration for ThemisDB's vector search, LLM inference, and geospatial operations. It manages device discovery, VRAM allocation, kernel validation, multi-GPU load balancing, ROCm support, safe-fail circuit breakers, and edition-based feature limits.

## Quick Diagnostics

| Symptom | Likely Cause | Quick Fix |
|---------|-------------|-----------|
| `DeviceDiscovery: no GPU found` | CUDA/ROCm driver not installed | Install NVIDIA/AMD drivers; check `nvidia-smi` |
| `GpuMemoryManagerEdition: limit exceeded` | Edition VRAM cap reached | Upgrade edition or reduce model size |
| `GpuModule: kernel validation failed` | CUDA binary checksum mismatch | Rebuild GPU kernels; `make gpu-kernels` |
| Load balancer always picks GPU-0 | Load balancer not initialized | Enable `gpu.load_balancer.enabled: true` |
| `SafeFail: GPU-0 disabled` | GPU error threshold exceeded | Check hardware; reset safe-fail |
| ROCm init fails | Wrong ROCm version or missing libs | Verify `rocm-smi` works; check library paths |
| Tensor buffer corruption | Unaligned memory access | Enable `gpu.debug.sanitize_buffers` |
| Feature flag disabled on GPU | Edition feature not available | Check `gpu.feature_flags` for your edition |
| GPU kernel audit failure | Kernel binary not signed | Re-sign kernels with trust anchor |
| Out-of-memory on embedding batch | Batch too large for VRAM | Reduce `gpu.embedding_batch_size` |

## Common Issues

### Issue 1: GPU Not Detected

**Description:** ThemisDB starts in CPU-only mode because no GPU device is found.

**Symptoms:**
- Log: `DeviceDiscovery: 0 CUDA devices found; falling back to CPU`
- LLM inference and vector search are 10-50× slower than expected

**Cause:** CUDA driver not installed, or running in a container without GPU passthrough.

**Solution:**
```bash
# Check CUDA driver
nvidia-smi
ls /dev/nvidia*

# Check ROCm (AMD)
rocm-smi
ls /dev/kfd /dev/dri/renderD*

# In Docker, add --gpus all
docker run --gpus all --rm nvidia/cuda:12.0-base nvidia-smi
```
```yaml
gpu:
  enabled: true
  backend: cuda                  # "cuda" | "rocm" | "vulkan" | "cpu"
  fallback_to_cpu: true          # don't fail if GPU unavailable
  device_ids: []                 # empty = use all available GPUs
```

---

### Issue 2: Edition VRAM Limit Exceeded

**Description:** ThemisDB refuses to allocate more VRAM because the edition cap is reached.

**Symptoms:**
- Log: `GpuMemoryManagerEdition: VRAM allocation rejected – edition limit=4096MB reached`
- LLM model loading fails

**Cause:** Community Edition has a 4 GB VRAM limit; model requires more.

**Solution:**
```bash
# Check edition and VRAM limits
themisdb-admin gpu vram-limits

# Check current VRAM allocation
nvidia-smi --query-gpu=memory.used,memory.free --format=csv
```
```yaml
gpu:
  memory:
    max_vram_fraction: 0.85      # use at most 85% of physical VRAM
    reserve_mb: 512              # always reserve 512MB for driver
```
*Upgrade to Enterprise Edition to remove the VRAM limit.*

---

### Issue 3: GPU Kernel Validation Failure

**Description:** GPU kernel binaries fail checksum validation, preventing execution.

**Symptoms:**
- Log: `GpuModule: kernel validation failed for hnsw_kernel.cubin (checksum mismatch)`
- Vector search falls back to CPU

**Cause:** GPU kernels were rebuilt without updating the stored checksum, or binary was modified.

**Solution:**
```bash
# Rebuild and re-validate GPU kernels
cd /opt/themisdb && make gpu-kernels
themisdb-admin gpu validate-kernels --rehash

# If kernels are from a package, reinstall
apt reinstall themisdb-gpu-kernels
```
```yaml
gpu:
  kernel_validation:
    enabled: true
    strict: false                # allow startup despite validation failure (dev only)
    rehash_on_startup: false
```

---

### Issue 4: Load Balancer Always Uses One GPU

**Description:** All GPU workloads are routed to GPU-0 even with multiple GPUs available.

**Symptoms:**
- `nvidia-smi` shows GPU-0 at 100% utilisation, GPU-1/2/3 at 0%
- Log: `GpuModule: load balancer using device 0 (single device mode)`

**Cause:** Multi-GPU load balancing is disabled or only one device is configured.

**Solution:**
```yaml
gpu:
  load_balancer:
    enabled: true
    strategy: least_loaded       # "round_robin" | "least_loaded" | "memory_aware"
    device_ids: [0, 1, 2, 3]
    rebalance_interval_ms: 1000
    memory_threshold_pct: 80     # rebalance if any GPU exceeds 80% VRAM
```

---

### Issue 5: Safe-Fail Disables a GPU Permanently

**Description:** A GPU is disabled by the safe-fail circuit breaker after errors.

**Symptoms:**
- Log: `SafeFail: GPU-1 disabled after 10 consecutive errors`
- Only N-1 GPUs are used

**Cause:** GPU hardware errors exceeded the circuit breaker threshold.

**Solution:**
```bash
# Check GPU hardware errors
nvidia-smi -q | grep -A2 "Retired Pages"
nvidia-smi -q | grep "Error Count"

# Reset safe-fail after hardware inspection
themisdb-admin gpu safe-fail reset --device 1

# Check safe-fail status
themisdb-admin gpu safe-fail status
```
```yaml
gpu:
  safe_fail:
    enabled: true
    error_threshold: 20          # increase threshold if transient errors are normal
    reset_timeout_ms: 300000
    auto_reset: false            # require manual reset for safety
```

---

### Issue 6: ROCm Initialisation Fails

**Description:** ThemisDB fails to initialise ROCm for AMD GPU support.

**Symptoms:**
- Log: `GpuModule: ROCm init failed: HSA_STATUS_ERROR_OPEN_FAILED`
- `rocm-smi` works but ThemisDB does not

**Cause:** ROCm version mismatch; missing `libhsa-runtime64.so`; user not in `render` group.

**Solution:**
```bash
# Check ROCm installation
rocm-smi --version
ldconfig -p | grep hsa

# Add ThemisDB service user to render group
usermod -aG render,video themisdb

# Check ROCm library path
ls /opt/rocm/lib/

# Set ROCm device path explicitly
export HSA_OVERRIDE_GFX_VERSION=10.3.0
```
```yaml
gpu:
  backend: rocm
  rocm:
    library_path: /opt/rocm/lib
    device_ordinal: 0
```

---

### Issue 7: Tensor Buffer Memory Corruption

**Description:** GPU operations produce garbage results due to buffer corruption.

**Symptoms:**
- Vector search returns `NaN` distances
- Log: `GpuModule: tensor validation failed: NaN detected in output buffer`

**Cause:** Memory alignment error in kernel launch parameters or buffer reuse bug.

**Solution:**
```yaml
gpu:
  debug:
    sanitize_buffers: true         # zero-fill buffers before use (dev only – slow)
    validate_outputs: true
    nan_detection: true
    log_kernel_launches: false
```
```bash
# Run GPU diagnostic
themisdb-admin gpu diagnostic --device 0 --test memory

# Check for ECC errors
nvidia-smi --query-gpu=ecc.errors.corrected.aggregate.total \
           --format=csv,noheader
```

---

### Issue 8: Embedding Batch OOM During Bulk Indexing

**Description:** Bulk embedding generation causes GPU OOM during index builds.

**Symptoms:**
- Log: `GpuModule: CUDA OOM during embedding batch (batch_size=512)`
- Index build fails partway through

**Cause:** Embedding batch size is too large for available VRAM.

**Solution:**
```yaml
gpu:
  embedding_batch_size: 64        # reduce from 512
  dynamic_batch_sizing: true      # automatically reduce batch size on OOM
  min_batch_size: 8
```

## Diagnostic Commands

```bash
# GPU status
themisdb-admin gpu status

# VRAM allocation
themisdb-admin gpu vram-usage

# Load balancer stats
themisdb-admin gpu load-balancer stats

# Safe-fail status
themisdb-admin gpu safe-fail status

# Kernel validation
themisdb-admin gpu validate-kernels

# Hardware diagnostic
themisdb-admin gpu diagnostic --device 0

# Live GPU metrics
curl -s http://localhost:9100/metrics | grep themisdb_gpu

# nvidia-smi monitoring
watch -n 1 nvidia-smi
```

## Configuration Reference

```yaml
gpu:
  enabled: true
  backend: cuda
  device_ids: []
  fallback_to_cpu: true
  memory:
    max_vram_fraction: 0.80
    reserve_mb: 512
  load_balancer:
    enabled: true
    strategy: least_loaded
  safe_fail:
    enabled: true
    error_threshold: 10
    auto_reset: false
  kernel_validation:
    enabled: true
    strict: true
  embedding_batch_size: 128
  dynamic_batch_sizing: true
```

**Common misconfigurations:**

| Key | Wrong | Correct |
|-----|-------|---------|
| `fallback_to_cpu` | `false` | `true` for resilience |
| `load_balancer.enabled` | `false` | `true` for multi-GPU |
| `memory.max_vram_fraction` | `1.0` | `0.80–0.85` |
| `safe_fail.auto_reset` | `true` | `false` in production |

## Known Limitations

- CUDA and ROCm backends cannot be used simultaneously; choose one at startup.
- Vulkan backend supports basic vector operations only; HNSW on Vulkan is not yet fully optimised.
- GPU memory fragmentation can cause OOM even with apparent free VRAM; restart to defragment.
- Kernel validation requires the trust anchor certificate to be present at `/etc/themisdb/gpu-trust-anchor.crt`.

## Related Documentation

- [GPU Module ROADMAP](../../src/gpu/ROADMAP.md)
- [GPU Roadmap](../gpu_roadmap.md)
- [GPU Runbooks](../de/features/gpu_runbooks.md)
- [NCCL/RCCL Integration Guide](../llm_orchestration/NCCL_RCCL_INTEGRATION_GUIDE.md)
- [GDAL Implementation Complete](../ARCHIVED/implementation-summaries/GDAL_IMPLEMENTATION_COMPLETE.md)
