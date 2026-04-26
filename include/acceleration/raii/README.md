> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# RAII Wrappers for GPU Resources

This directory contains header-only RAII (Resource Acquisition Is Initialization) wrappers for GPU backend resources, providing automatic resource cleanup and exception safety.

## Overview

RAII wrappers ensure that GPU resources are properly released even when exceptions occur, preventing resource leaks and simplifying error handling. All wrappers follow these principles:

1. **Automatic cleanup**: Resources are released in destructors
2. **Move semantics**: Resources can be moved but not copied
3. **Exception safety**: Resources are cleaned up even when exceptions are thrown
4. **No overhead**: Header-only, inline implementations with zero runtime cost

## Available Wrappers

### CUDA (`cuda_raii.h`)

- **CudaStream**: CUDA stream management
  - Automatic `cudaStreamDestroy` on destruction
  - Support for priority streams
  - Stream synchronization

- **CudaDeviceMemory**: Device memory allocation
  - Automatic `cudaFree` on destruction
  - Convenience methods for host↔device transfers
  - Size tracking

- **ScopedCudaDevice**: Device context switching
  - Restores previous device on destruction
  - RAII for `cudaSetDevice`

### OpenCL (`opencl_raii.h`)

- **OpenCLContext**: OpenCL context management
  - Automatic `clReleaseContext` on destruction
  - Reference counting support

- **OpenCLQueue**: Command queue management
  - Automatic `clReleaseCommandQueue` on destruction
  - Queue finishing and synchronization

- **OpenCLProgram**: Program management
  - Automatic `clReleaseProgram` on destruction
  - Convenience methods for source compilation

- **OpenCLKernel**: Kernel management
  - Automatic `clReleaseKernel` on destruction
  - Named kernel creation

- **OpenCLBuffer**: Buffer memory management
  - Automatic `clReleaseMemObject` on destruction
  - Size tracking

## Usage Examples

### CUDA Stream

```cpp
#include "acceleration/raii/cuda_raii.h"

void myFunction() {
    // Create stream (automatically destroyed at end of scope)
    CudaStream stream(true);

    // Use stream
    launchKernel<<<gridDim, blockDim, 0, stream.get()>>>(...);

    // Stream automatically destroyed here
}
```

### CUDA Device Memory

```cpp
#include "acceleration/raii/cuda_raii.h"

void processData(const float* hostData, size_t size) {
    // Allocate device memory
    CudaDeviceMemory deviceMem(size);

    // Copy to device
    deviceMem.copyFrom(hostData, size);

    // Process...
    launchKernel<<<...>>>(deviceMem.get(), ...);

    // Memory automatically freed here
}
```

### OpenCL Context and Queue

```cpp
#include "acceleration/raii/opencl_raii.h"

void setupOpenCL(cl_device_id device) {
    // Create context
    OpenCLContext context;
    context.create(nullptr, 1, &device);

    // Create queue
    OpenCLQueue queue;
    queue.create(context.get(), device);

    // Use resources...

    // Everything automatically cleaned up here
}
```

### Exception Safety

```cpp
#include "acceleration/raii/cuda_raii.h"

void riskyOperation() {
    CudaStream stream(true);
    CudaDeviceMemory mem(1024 * 1024);

    // If exception thrown here, stream and memory
    // are still properly cleaned up
    if (someCondition) {
        throw std::runtime_error("Operation failed");
    }

    // Normal cleanup if no exception
}
```

## Design Principles

### 1. Header-Only

All wrappers are header-only to avoid build system complexity and enable inlining.

### 2. Move-Only Semantics

Resources cannot be copied (deleted copy constructor/assignment) but can be moved:

```cpp
CudaStream stream1(true);
CudaStream stream2 = std::move(stream1);  // OK
CudaStream stream3 = stream2;             // Compile error
```

### 3. Ownership Transfer

Use `release()` to transfer ownership to external code:

```cpp
CudaStream stream(true);
cudaStream_t raw = stream.release();  // stream no longer owns resource
// Caller must now call cudaStreamDestroy(raw)
```

### 4. Non-Owning Wrappers

Use `wrap()` to create non-owning wrappers for external resources:

```cpp
cudaStream_t externalStream = ...;
CudaStream wrapper = CudaStream::wrap(externalStream);
// wrapper won't destroy stream on destruction
```

## Integration with Existing Code

### Gradual Migration

Backends can be refactored incrementally:

```cpp
// Before
bool initialize() {
    cudaStreamCreate(&stream_);
    // ... error handling
}

void shutdown() {
    if (stream_) cudaStreamDestroy(stream_);
}

// After
bool initialize() {
    stream_.create();  // CudaStream member
    // Exception-safe, no manual cleanup needed
}

void shutdown() {
    // stream_ automatically destroyed
}
```

### Backward Compatibility

```cpp
class MyBackend {
private:
    CudaStream stream_;  // RAII wrapper

public:
    // Legacy interface still works
    cudaStream_t getStream() const {
        return stream_.get();
    }
};
```

## Testing

Comprehensive tests are in `tests/test_raii_wrappers.cpp`:

- Lifecycle tests (creation/destruction)
- Move semantics validation
- Exception safety verification
- Resource leak detection

Run tests with:
```bash
ctest -R test_raii_wrappers
```

## Performance

RAII wrappers have **zero runtime overhead**:
- Inline methods (header-only)
- No virtual functions
- No dynamic allocation
- Same assembly as manual resource management

Benchmark comparisons show identical performance to raw resource management.

## Future Extensions

Planned additions:
- HIP resource wrappers (`hip_raii.h`)
- Vulkan resource wrappers (`vulkan_raii.h`)
- Metal resource wrappers (`metal_raii.h`)
- DirectX resource wrappers (`directx_raii.h`)

## References

- [RAII on cppreference.com](https://en.cppreference.com/w/cpp/language/raii)
- [CUDA Best Practices Guide](https://docs.nvidia.com/cuda/cuda-c-best-practices-guide/)
- [OpenCL Programming Guide](https://www.khronos.org/opencl/)

## Contributing

When adding new wrappers:
1. Follow existing patterns (move-only, header-only)
2. Add comprehensive tests
3. Document usage in this README
4. Ensure exception safety
5. Verify zero overhead with benchmarks

## Installation

This module is included as part of ThemisDB. Add the module headers to your include path:

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
