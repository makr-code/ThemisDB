# Acceleration Backend Error Codes

Comprehensive reference for error codes used across all GPU acceleration backends.

## Overview

ThemisDB uses structured error codes to provide clear, actionable diagnostic information when GPU operations fail. Each error code is:
- **Categorized** by type (initialization, resource, runtime, etc.)
- **Unique** across all backends
- **Documented** with troubleshooting steps
- **Actionable** with specific resolution hints

## Error Code Categories

| Range | Category | Description |
|-------|----------|-------------|
| 0 | Success | Operation completed successfully |
| 100-199 | Initialization | Backend/device initialization failures |
| 200-299 | Resource Management | Memory allocation and resource errors |
| 300-399 | Runtime/Execution | Kernel execution and synchronization errors |
| 400-499 | Configuration | Configuration and parameter errors |
| 500-599 | Kernel/Shader | Compilation and linking errors |
| 900-999 | Generic | Unknown or internal errors |

---

## Initialization Errors (100-199)

### 101: No Devices Found

**Description:** No compatible GPU devices were found on the system.

**Common Causes:**
- No GPU physically installed
- GPU not detected by system (BIOS/UEFI issue)
- Driver not installed or loaded
- GPU disabled in device manager

**Troubleshooting:**
1. Verify GPU is physically installed: `lspci | grep -i vga` (Linux) or Device Manager (Windows)
2. Check driver installation: `nvidia-smi` (NVIDIA), `rocm-smi` (AMD)
3. Ensure GPU is enabled in BIOS/UEFI
4. Check for hardware failures

**Backends:** All (CUDA, HIP, OpenCL, Metal, Vulkan)

---

### 102: Driver Not Installed

**Description:** Required GPU driver not installed or not accessible.

**Common Causes:**
- Driver not installed
- Driver version too old
- Driver service not running
- Permission issues

**Troubleshooting:**
1. **CUDA/NVIDIA:**
   ```bash
   # Check driver
   nvidia-smi
   
   # Install driver (Ubuntu)
   sudo apt install nvidia-driver-535
   ```

2. **HIP/AMD:**
   ```bash
   # Check driver
   rocm-smi
   
   # Install ROCm (Ubuntu)
   sudo apt install rocm-dkms
   ```

3. **OpenCL:**
   ```bash
   # Check OpenCL platforms
   clinfo
   
   # Install OpenCL ICD loader
   sudo apt install ocl-icd-libopencl1
   ```

4. **Metal:** Update to macOS 10.11+ or iOS 8+

**Backends:** All

---

### 103: Device Not Supported

**Description:** Device exists but is not supported (too old, wrong architecture).

**Common Causes:**
- GPU architecture too old (compute capability < 3.0 for CUDA)
- Missing required features
- Unsupported vendor (e.g., NVIDIA GPU with AMD driver)

**Troubleshooting:**
1. Check GPU specifications against backend requirements
2. **CUDA:** Requires compute capability 3.0+ (Kepler or newer)
3. **HIP:** Requires GCN 3.0+ architecture (Fiji or newer)
4. **Metal:** Requires macOS 10.11+ GPU (2012+ Macs)

**Backends:** All

---

### 104: Context Creation Failed

**Description:** Failed to create GPU context/environment.

**Common Causes:**
- GPU already in exclusive use
- Insufficient system resources
- Driver error
- Conflicting applications

**Troubleshooting:**
1. Close other GPU-using applications
2. Restart GPU driver: `sudo systemctl restart nvidia-persistenced` (Linux)
3. Check for competing CUDA/OpenCL applications
4. Reboot system if persistent

**Backends:** All

---

### 105: Queue Creation Failed

**Description:** Failed to create command queue or stream.

**Common Causes:**
- Context not properly initialized
- Out of GPU resources
- Driver bug

**Troubleshooting:**
1. Ensure device context was created successfully
2. Check GPU memory availability
3. Update driver to latest version
4. Reduce concurrent stream/queue count

**Backends:** All

---

## Resource Management Errors (200-299)

### 201: Out of Device Memory

**Description:** GPU ran out of memory during allocation.

**Common Causes:**
- Requested allocation too large
- Memory fragmentation
- Other applications using GPU memory
- Memory leaks in application

**Troubleshooting:**
1. Check GPU memory usage: `nvidia-smi` or `rocm-smi`
2. Reduce batch size or data dimensions
3. Close other GPU applications
4. Restart application to clear leaks
5. Consider using a GPU with more VRAM

**Calculation:**
```
Required VRAM ≈ (num_vectors × dimensions × 4 bytes) + overhead
Example: 1M vectors × 768 dims × 4 = 3GB + ~20% overhead = 3.6GB
```

**Backends:** All

---

### 202: Out of Host Memory

**Description:** System RAM exhausted during operation.

**Common Causes:**
- Large dataset doesn't fit in RAM
- Memory leak
- Insufficient system memory

**Troubleshooting:**
1. Check system memory: `free -h` (Linux) or Task Manager (Windows)
2. Reduce dataset size
3. Use streaming/batching to process data in chunks
4. Close other applications
5. Add more RAM if persistent

**Backends:** All

---

### 204: Memory Copy Failed

**Description:** Failed to copy data between host and device.

**Common Causes:**
- Invalid pointer
- Size mismatch
- Device disconnected
- PCIe error

**Troubleshooting:**
1. Verify pointers are valid and allocated
2. Check copy size matches allocated size
3. Ensure GPU is still responsive: `nvidia-smi`
4. Check PCIe connection (reseat GPU if physical issue)
5. Update motherboard firmware if PCIe errors persist

**Backends:** All

---

## Runtime/Execution Errors (300-399)

### 301: Kernel Launch Failed

**Description:** Failed to launch GPU kernel/shader.

**Common Causes:**
- Invalid kernel arguments
- Work group size too large
- Kernel not compiled
- Resource limits exceeded

**Troubleshooting:**
1. Validate kernel arguments match kernel signature
2. Check work group size is within device limits:
   ```cpp
   // CUDA: query max threads per block
   cudaDeviceProp prop;
   cudaGetDeviceProperties(&prop, 0);
   // Max threads: prop.maxThreadsPerBlock
   ```
3. Ensure kernel was successfully compiled
4. Reduce work group size or grid dimensions

**Backends:** CUDA, HIP, OpenCL, Metal, Vulkan

---

### 302: Kernel Execution Failed

**Description:** Kernel launched but failed during execution.

**Common Causes:**
- Out-of-bounds memory access
- Division by zero
- Infinite loop in kernel
- Stack overflow in kernel

**Troubleshooting:**
1. Enable compute-sanitizer (CUDA) or similar tools
2. Add bounds checking to kernel code
3. Review kernel logic for edge cases
4. Reduce problem size to isolate issue
5. Check for race conditions in kernel

**Backends:** CUDA, HIP, OpenCL, Metal, Vulkan

---

### 303: Synchronization Failed

**Description:** Failed to synchronize with GPU (wait for completion).

**Common Causes:**
- Kernel timeout (Windows TDR)
- Device hang
- Driver crash

**Troubleshooting:**
1. **Windows TDR:** Increase timeout in registry:
   ```
   HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\GraphicsDrivers
   TdrDelay = 60 (seconds)
   ```
2. Check for infinite loops in kernels
3. Reduce kernel complexity or work size
4. Update GPU driver
5. Check GPU temperature (overheating can cause hangs)

**Backends:** CUDA, HIP, OpenCL, Metal

---

## Kernel Compilation Errors (500-599)

### 501: Kernel Compilation Failed

**Description:** GPU kernel/shader failed to compile.

**Common Causes:**
- Syntax errors in kernel code
- Unsupported language features
- Compiler version mismatch
- Missing includes or definitions

**Troubleshooting:**
1. Check compilation log for specific errors
2. Verify kernel syntax matches backend requirements
3. Ensure all required extensions/features are supported
4. Test kernel with simple example to isolate issue
5. Check compiler version compatibility

**Example (OpenCL):**
```cpp
// Check build log on error
size_t logSize;
clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 
                      0, nullptr, &logSize);
std::vector<char> log(logSize);
clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG,
                      logSize, log.data(), nullptr);
std::cerr << "Build log:\n" << log.data() << std::endl;
```

**Backends:** CUDA, HIP, OpenCL, Metal, Vulkan

---

## Configuration Errors (400-499)

### 401: Invalid Configuration

**Description:** Backend configuration is invalid or inconsistent.

**Common Causes:**
- Conflicting configuration options
- Invalid parameter values
- Missing required settings

**Troubleshooting:**
1. Review configuration for invalid combinations
2. Check parameter value ranges
3. Consult backend documentation for valid configs
4. Use default configuration to verify basic operation

**Backends:** All

---

### 404: Backend Not Initialized

**Description:** Attempted operation on uninitialized backend.

**Common Causes:**
- Forgot to call `initialize()`
- Initialization failed but error not checked
- Backend was shutdown

**Troubleshooting:**
1. Ensure `backend->initialize()` is called before use
2. Check return value of `initialize()` for success
3. Add initialization checks: `if (!initialized_) return error;`
4. Don't use backend after calling `shutdown()`

**Backends:** All

---

## Best Practices

### For Users

1. **Check Return Values:** Always check if operations succeeded
   ```cpp
   if (!backend->initialize()) {
       // Handle error
   }
   ```

2. **Log Error Context:** Use error context for debugging
   ```cpp
   ErrorContext error = backend->getLastError();
   std::cerr << error.format() << std::endl;
   ```

3. **Follow Hints:** Error hints provide actionable next steps

### For Developers

1. **Use Structured Errors:** Always use error codes, not just strings
   ```cpp
   return ErrorContext(
       AccelerationErrorCode::NoDevicesFound,
       "CUDA",
       "No CUDA devices found",
       "Install NVIDIA GPU and driver"
   );
   ```

2. **Provide Context:** Include relevant system information
3. **Be Specific:** Detailed messages help users resolve issues
4. **Test Error Paths:** Validate error handling works correctly

---

## Common Error Patterns

### Initialization Sequence

Typical initialization error sequence:
```
1. Check if backend is available (isAvailable())
   ↓ No → Error 102 (Driver Not Installed)
   ↓ Yes
2. Initialize backend (initialize())
   ↓ Fail → Error 101 (No Devices)
   ↓      → Error 104 (Context Failed)
   ↓      → Error 105 (Queue Failed)
   ↓ Success
3. Ready to use
```

### Memory Allocation Pattern

```
1. Allocate device memory
   ↓ Fail → Error 201 (Out of Device Memory)
   ↓ Success
2. Copy data to device
   ↓ Fail → Error 204 (Memory Copy Failed)
   ↓ Success
3. Execute kernel
```

---

## Quick Reference

| Error | Code | Category | Common Fix |
|-------|------|----------|------------|
| No devices | 101 | Init | Install GPU/driver |
| No driver | 102 | Init | Install driver |
| No memory | 201 | Resource | Reduce batch size |
| Kernel failed | 301 | Runtime | Check arguments |
| Won't compile | 501 | Kernel | Fix syntax |

---

## See Also

- [Production Readiness Guide](production_readiness.md)
- [Backend Selection Guide](../README.md)
- [Troubleshooting FAQ](troubleshooting.md)
