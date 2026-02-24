# Acceleration Module — Operational Troubleshooting Guide

**Module:** `src/acceleration`  
**Version:** 2026 Q1  
**Status:** Production

---

## Table of Contents

1. [Quick Diagnostics Checklist](#quick-diagnostics-checklist)
2. [Backend Not Selected / Always Falls Back to CPU](#backend-not-selected--always-falls-back-to-cpu)
3. [Initialization Failures](#initialization-failures)
4. [GPU Memory Errors](#gpu-memory-errors)
5. [Kernel Execution Failures](#kernel-execution-failures)
6. [Performance Degradation](#performance-degradation)
7. [Backend Health Issues](#backend-health-issues)
8. [Plugin Loading Failures](#plugin-loading-failures)
9. [Platform-Specific Issues](#platform-specific-issues)
10. [Logging and Diagnostics](#logging-and-diagnostics)
11. [Environment Variables Reference](#environment-variables-reference)
12. [See Also](#see-also)

---

## Quick Diagnostics Checklist

Run through this checklist whenever acceleration behaves unexpectedly:

```
[ ] 1. Check which backend is actually selected:
        auto* vb = BackendRegistry::instance().getSelectedVectorBackend();
        // nullptr → no backend matched requirements

[ ] 2. Check initialization error code:
        ErrorContext err = backend->getLastError();
        std::cerr << err.format() << std::endl;

[ ] 3. Verify GPU is visible to the OS:
        Linux:   lspci | grep -iE "vga|3d|nvidia|amd|intel"
        Windows: Device Manager → Display Adapters
        macOS:   system_profiler SPDisplaysDataType

[ ] 4. Verify driver is installed and loaded:
        NVIDIA:  nvidia-smi
        AMD:     rocm-smi
        OpenCL:  clinfo
        Vulkan:  vulkaninfo --summary

[ ] 5. Confirm build flags match the installed SDK:
        strings themisdb | grep -E "THEMIS_ENABLE_(CUDA|VULKAN|HIP)"

[ ] 6. Check available GPU memory:
        nvidia-smi --query-gpu=memory.free,memory.total --format=csv
        rocm-smi --showmeminfo vram

[ ] 7. Check backend health status:
        BackendHealthStatus h = backend->getHealthStatus();
        // h.status == "healthy" | "degraded" | "unhealthy"
```

---

## Backend Not Selected / Always Falls Back to CPU

### Symptom

`getSelectedVectorBackend()` returns `nullptr`, or all operations run on CPU even though a GPU is present.

### Cause A — Build flag not set

**Diagnosis:**
```bash
strings ./themisdb | grep -E "THEMIS_ENABLE_(CUDA|VULKAN|HIP)"
# No output → the backend was compiled out
```

**Fix:** Rebuild with the appropriate flag:
```bash
cmake -DTHEMIS_ENABLE_CUDA=ON ..
cmake -DTHEMIS_ENABLE_VULKAN=ON ..
```

### Cause B — GPU driver not installed or not loaded

**Diagnosis:**
```bash
# NVIDIA
nvidia-smi
# Expected: table showing driver version, GPU name, memory
# If missing: driver not installed

# AMD
rocm-smi
```

**Fix:** Install the driver for your platform. See [Initialization Failures](#initialization-failures) for per-backend commands.

### Cause C — Capability requirements too strict

**Diagnosis:**
```cpp
// Check whether default requirements would select a backend:
auto* fallback = BackendRegistry::instance().selectVectorBackendFor(
    BackendRegistry::defaultVectorRequirements());
// If non-null, your custom requirements are stricter than the backend supports.
```

**Fix:** Relax requirements, or remove the requirement for async/FP16 if not needed:
```cpp
BackendRegistry::CapabilityRequirements req;
req.needsVectorOps = true;
// Do NOT set req.needsAsync = true unless you require it.
BackendRegistry::instance().initializeRuntime(req, ...);
```

### Cause D — `initializeRuntime()` not called

**Diagnosis:** `isRuntimeInitialized()` returns `false`.

**Fix:** Call `BackendRegistry::instance().initializeRuntime()` once during single-threaded startup, before spawning worker threads.

---

## Initialization Failures

### Error 101 — No Devices Found

**Symptom:** Backend returns error code 101 on `initialize()`.

**Steps:**
1. Verify GPU is physically installed and seated in the PCIe slot.
2. Check OS visibility: `lspci | grep -i vga` (Linux) or Device Manager (Windows).
3. Check BIOS/UEFI — ensure the GPU is not disabled.
4. Check for hardware failures (try a different slot, reseat GPU).

### Error 102 — Driver Not Installed

**Symptom:** `isAvailable()` returns `false`; error code 102.

**NVIDIA / CUDA:**
```bash
# Check if driver is present
nvidia-smi
# Ubuntu — install driver
sudo apt install nvidia-driver-535
# Verify CUDA toolkit
nvcc --version
```

**AMD / HIP (ROCm):**
```bash
# Check ROCm
rocm-smi
# Ubuntu — install ROCm
sudo apt install rocm-dkms
# Check HIP
hipcc --version
```

**OpenCL:**
```bash
# List OpenCL platforms
clinfo
# Install ICD loader
sudo apt install ocl-icd-libopencl1 ocl-icd-opencl-dev
```

**Metal (macOS):** Requires macOS 10.11 (El Capitan) or later. No separate driver installation needed.

**Vulkan:**
```bash
# Check Vulkan ICD
vulkaninfo --summary
# AMD Mesa (Linux)
sudo apt install mesa-vulkan-drivers
# NVIDIA (Linux)
sudo apt install libvulkan1
# macOS — install MoltenVK
brew install molten-vk
```

### Error 104 — Context Creation Failed

**Symptom:** Driver is present but `initialize()` fails with code 104.

**Steps:**
1. Close other GPU-intensive applications (rendering, ML training, other database instances).
2. Restart the GPU driver service:
   ```bash
   # Linux — NVIDIA persistence daemon
   sudo systemctl restart nvidia-persistenced
   ```
3. Check for exclusive-mode conflicts: another process may hold the GPU context.
4. Reboot if the driver is in an inconsistent state.

### Error 105 — Queue / Stream Creation Failed

**Symptom:** Context created but command queue creation fails (code 105).

**Steps:**
1. Ensure context was successfully created (check code 104 first).
2. Check available GPU memory (`nvidia-smi`); low memory can prevent stream allocation.
3. Reduce the number of concurrent streams/queues your application creates.
4. Update the GPU driver to the latest stable version.

---

## GPU Memory Errors

### Error 201 — Out of Device Memory

**Symptom:** Operations fail with code 201; GPU memory is exhausted.

**Diagnosis:**
```bash
# Check current GPU memory usage
nvidia-smi --query-gpu=memory.used,memory.free,memory.total --format=csv,noheader
# AMD
rocm-smi --showmeminfo vram
```

**Estimate required VRAM:**
```
Required VRAM ≈ (num_vectors × dimensions × 4 bytes) + ~20% overhead
Example:  1 000 000 vectors × 768 dimensions × 4 bytes = 3.0 GB + 600 MB = 3.6 GB
```

**Fix options:**
- Reduce batch size: process vectors in smaller chunks.
- Close other GPU applications.
- Use a GPU with more VRAM.
- Switch to CPU backend for large batches; see [Backend Health Issues](#backend-health-issues).

### Error 204 — Memory Copy Failed

**Symptom:** Data transfer between host and device fails (code 204).

**Steps:**
1. Verify all host-side pointers are valid and properly aligned.
2. Confirm the copy size matches the allocated buffer size.
3. Verify GPU is still responsive: `nvidia-smi` should show the device.
4. Check for PCIe errors in the kernel log:
   ```bash
   dmesg | grep -iE "pcie|aer|error"
   ```
5. If PCIe errors appear, reseat the GPU or test with another slot.

---

## Kernel Execution Failures

### Error 301 — Kernel Launch Failed

**Symptom:** Kernel fails to launch (code 301).

**Steps:**
1. Validate kernel arguments — check for null pointers, zero dimensions, or negative sizes.
2. Ensure work group size is within device limits:
   ```cpp
   // CUDA: query max threads per block
   cudaDeviceProp prop;
   cudaGetDeviceProperties(&prop, 0);
   // prop.maxThreadsPerBlock is the upper limit
   ```
3. Confirm the kernel was compiled successfully (no error 501 earlier in the sequence).
4. Reduce grid/block dimensions if the launch configuration is too large.

### Error 302 — Kernel Execution Failed

**Symptom:** Kernel launched but failed during GPU execution (code 302).

**Steps:**
1. Enable CUDA compute sanitizer for detailed diagnostics:
   ```bash
   compute-sanitizer --tool memcheck ./themisdb
   ```
2. Add bounds checking to custom kernel code.
3. Test with a minimal, reduced-size input to isolate the failure.
4. Check for race conditions if multiple streams access shared memory.

### Error 303 / 304 / 305 — Transient Errors (Sync / Timeout / Device Lost)

These are **transient** — the kernel dispatcher retries them automatically before falling back to CPU.

| Code | Name | Common Cause |
|------|------|--------------|
| 303 | SynchronizationFailed | Windows TDR timeout, GPU hang |
| 304 | OperationTimeout | Kernel exceeded deadline |
| 305 | DeviceLost | GPU reset or physically disconnected |

**If retries do not resolve the issue:**

- **Windows TDR timeout (303):** Extend the timeout:
  ```
  Registry: HKLM\System\CurrentControlSet\Control\GraphicsDrivers
  Value: TdrDelay = 60 (seconds)
  ```
- **GPU overheating (303/304):** Check GPU temperature:
  ```bash
  nvidia-smi --query-gpu=temperature.gpu --format=csv,noheader
  # Values above 85°C indicate thermal throttling
  ```
  Improve case airflow or clean GPU heatsink.
- **Device lost (305):** Check for PCIe slot issues or driver crash:
  ```bash
  dmesg | grep -iE "GPU|NVRM|amdgpu|device lost"
  ```
- Increase retry count in `RetryPolicy` for unstable hardware:
  ```cpp
  RetryPolicy policy;
  policy.maxAttempts    = 5;
  policy.initialDelayMs = 10;
  policy.maxDelayMs     = 500;
  ANNKernelFallbackDispatcher dispatcher(gpuTable, cpuTable, policy);
  ```

---

## Performance Degradation

### GPU Backend Selected but Performance Is CPU-Like

**Diagnosis:**
1. Check health status: `backend->getHealthStatus()` — a `degraded` backend may be falling back.
2. Check if kernels are actually dispatching to GPU:
   ```bash
   # Monitor GPU utilization while running queries
   watch -n 1 nvidia-smi
   # Should show >0% GPU utilization during vector operations
   ```
3. Check whether the kernel dispatcher is silently falling back:
   ```cpp
   ErrorContext err = backend->getLastError();
   // code != 0 → dispatcher fell back to CPU after repeated failures
   ```

### High Memory Bandwidth / PCIe Bottleneck

**Symptom:** GPU utilization is high but throughput is lower than expected.

**Steps:**
1. Profile PCIe transfer overhead:
   ```bash
   nvidia-smi dmon -s u   # utilization monitor
   ```
2. Batch multiple small queries into a single large kernel launch to amortize PCIe overhead.
3. Use pinned (page-locked) host memory when allocating input buffers to improve transfer speed.
4. Verify GPU is in PCIe Gen 3/4 × 16 mode, not x1/x4:
   ```bash
   nvidia-smi --query-gpu=pcie.link.gen.current,pcie.link.width.current --format=csv
   ```

### Unexpected Throughput Regression After Driver Update

1. Pin the driver version in your deployment manifest.
2. Run the backend consistency tests to verify L2 distance values are unchanged:
   ```bash
   ctest -R test_backend_consistency --output-on-failure
   ```
3. Roll back the driver if parity tests fail.

---

## Backend Health Issues

### `getHealthStatus()` Returns `degraded`

**Symptom:** `status.status == "degraded"` — driver is reachable but last operation failed.

**Steps:**
1. Read `status.issues` for specific, actionable descriptions:
   ```cpp
   auto h = backend->getHealthStatus();
   for (const auto& issue : h.issues) {
       std::cerr << "Issue: " << issue << std::endl;
   }
   ```
2. Check the structured error context:
   ```cpp
   ErrorContext err = backend->getLastError();
   std::cerr << err.format() << std::endl;
   // Includes: backend name, error code, description, troubleshooting hint
   ```
3. Check GPU driver logs:
   ```bash
   journalctl -k | grep -i nvidia   # Linux — NVIDIA
   journalctl -k | grep -i amdgpu   # Linux — AMD
   ```
4. Re-run backend detection to promote a healthy alternative:
   ```cpp
   BackendRegistry::instance().initializeRuntime();
   ```
5. If the issue is transient (temperature spike, PCIe event), increase `RetryPolicy::maxAttempts`.

### `getHealthStatus()` Returns `unhealthy`

**Symptom:** Backend completely unavailable; `healthy == false`, `alive == false`.

**Cause:** Driver is unreachable or device is lost.

**Steps:**
1. Verify the GPU is still visible to the OS (see [Quick Diagnostics Checklist](#quick-diagnostics-checklist)).
2. Restart the driver service or reboot.
3. After recovery, call `initializeRuntime()` again to re-detect backends.

---

## Plugin Loading Failures

### Plugin Not Loaded

**Symptom:** Expected backend from a dynamically loaded plugin is not available.

**Steps:**
1. Verify the plugin `.so` / `.dll` file is in the plugin search path (check `THEMIS_PLUGIN_PATH`).
2. Check that the plugin was signed or is on the allow-list in `plugin_security.cpp`:
   ```bash
   # Verify plugin SHA-256 hash matches the allow-list entry
   sha256sum /path/to/backend_plugin.so
   ```
3. Check the plugin loader log for rejection reason:
   ```
   [WARN] Plugin rejected: signature mismatch — /path/to/backend_plugin.so
   ```
4. Ensure the plugin ABI version matches the ThemisDB version (no breaking changes before v2.0).

### Plugin Causes Crash on Load

**Steps:**
1. Load the plugin in isolation using a test harness before integrating.
2. Check `THEMIS_PLUGIN_SANDBOX=strict` is set to prevent unsafe operations.
3. Contact the plugin vendor with the crash log from `journalctl` or the Windows event log.

---

## Platform-Specific Issues

### Linux

**Kernel module not loaded (NVIDIA):**
```bash
lsmod | grep nvidia
# If missing:
sudo modprobe nvidia
```

**Permission denied on GPU device node:**
```bash
ls -la /dev/nvidia*
# If group is 'video', add your user:
sudo usermod -aG video $USER
# Then log out and back in
```

**ROCm device permissions:**
```bash
ls -la /dev/kfd /dev/dri/renderD*
sudo usermod -aG render,video $USER
```

### Windows

**GPU not detected in a VM / container:**
- Ensure GPU passthrough (VFIO / SR-IOV) is configured.
- CUDA requires WDDM 2.x drivers for GPU virtualization.

**TDR timeout (error 303):**
See [Error 303 — Transient Errors](#error-303--304--305--transient-errors-sync--timeout--device-lost) above.

**DirectX Compute not available:**
- Requires Windows 10 version 1607 or later.
- Verify DirectX feature level 11_0 is supported: `dxdiag`.

### macOS

**Metal not available:**
- Metal requires macOS 10.11 (El Capitan) or newer.
- GPU must be supported: run `system_profiler SPDisplaysDataType | grep Metal`.

**MoltenVK (Vulkan on macOS):**
```bash
brew install molten-vk
# Set the ICD path
export VK_ICD_FILENAMES=/usr/local/share/vulkan/icd.d/MoltenVK_icd.json
```

### Docker / Kubernetes

**NVIDIA GPU not visible in container:**
```bash
# Requires nvidia-container-toolkit on the host
docker run --gpus all themisdb nvidia-smi
```

**Enable GPU in Kubernetes:**
```yaml
resources:
  limits:
    nvidia.com/gpu: 1
```

**Check GPU sharing policy:**
```bash
nvidia-smi --query-gpu=compute-mode --format=csv
# "Exclusive_Process" → only one process allowed at a time
# Change to "Default" for shared access:
sudo nvidia-smi -c 0
```

---

## Logging and Diagnostics

### Structured Error Context

Every error from a backend carries a structured `ErrorContext` object:

```cpp
ErrorContext err = backend->getLastError();
// Fields:
//   err.code               — AccelerationErrorCode enum value
//   err.backendName        — "CUDA", "HIP", "OpenCL", etc.
//   err.message            — human-readable description
//   err.troubleshootingHint — actionable next step
std::cerr << err.format() << std::endl;
```

### Prometheus Metrics

The acceleration metrics system exports Prometheus-compatible data for operational monitoring.

**Key metrics to watch:**

| Metric | Alert Threshold | Meaning |
|--------|----------------|---------|
| `acceleration_errors_total{code="201"}` | > 0 sustained | Out of device memory |
| `acceleration_errors_total{code="303"}` | > 5/min | GPU synchronization failures |
| `acceleration_fallback_total` | Increasing | Kernels falling back to CPU |
| `acceleration_latency_p99_seconds` | > 2× baseline | Performance regression |
| `acceleration_memory_used_bytes` | > 80% GPU VRAM | Memory pressure |

Scrape endpoint: `http://localhost:<port>/metrics`

See [metrics.md](metrics.md) for the full metrics reference.

### Enabling Verbose Logging

Set the `THEMIS_LOG_LEVEL` environment variable before starting the server:

```bash
# Show all acceleration-related debug messages
export THEMIS_LOG_LEVEL=DEBUG
./themisdb
```

Look for log lines tagged `[acceleration]` or `[backend_registry]`.

---

## Environment Variables Reference

| Variable | Default | Description |
|----------|---------|-------------|
| `THEMIS_LOG_LEVEL` | `INFO` | Log verbosity: `DEBUG`, `INFO`, `WARN`, `ERROR` |
| `THEMIS_PLUGIN_PATH` | `./plugins` | Directory to search for backend plugins |
| `THEMIS_PLUGIN_SANDBOX` | `strict` | Plugin security mode: `strict` or `permissive` |
| `THEMIS_GPU_DEVICE_INDEX` | `0` | Index of the preferred GPU device (0-based) |
| `THEMIS_MAX_RETRY_ATTEMPTS` | `3` | Default `RetryPolicy::maxAttempts` for all dispatchers |
| `THEMIS_DISABLE_GPU` | unset | If set to `1`, forces CPU-only mode regardless of hardware |

---

## See Also

- [error_codes.md](error_codes.md) — full error code reference with per-code resolution steps
- [capability_negotiation.md](capability_negotiation.md) — backend selection, fallback chain, and retry policy configuration
- [production_readiness.md](production_readiness.md) — production deployment checklist
- [metrics.md](metrics.md) — Prometheus metrics reference
- `src/acceleration/README.md` — module overview, build flags, and directory layout
- `include/acceleration/compute_backend.h` — `BackendCapabilities`, `CapabilityRequirements`, `BackendRegistry` API
- `include/acceleration/kernel_fallback_dispatcher.h` — `RetryPolicy`, `ANNKernelFallbackDispatcher`, `GeoKernelFallbackDispatcher`
