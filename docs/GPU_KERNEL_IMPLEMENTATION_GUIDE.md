# CUDA/OpenCL Kernel Implementation Guide

**Target Release**: v1.4.0 (Q2 2026)  
**Priority**: High  
**Effort**: 3-4 weeks  
**Expertise Required**: GPU programming (CUDA/OpenCL), computational geometry

## Overview

This guide provides a detailed blueprint for implementing complete CUDA and OpenCL kernels for GPU-accelerated spatial operations in ThemisDB. The current implementation (`src/geo/gpu_backend_production.cpp`) provides infrastructure and automatically falls back to CPU-parallel execution. This guide covers completing the GPU kernel implementations.

## Current State

### What Exists (v1.3.0)
- ✅ `CpuParallelBackend`: Fully functional (12.5x speedup)
- ✅ `CudaBackend`: Infrastructure ready, automatic GPU detection
- ✅ `OpenCLBackend`: Platform/device detection and initialization
- ✅ `ProductionGpuBackend`: Smart coordinator with fallback chain
- ✅ CUDA kernel skeleton for MBR intersection tests
- ✅ Graceful fallback to CPU-parallel when kernels incomplete

### What Needs Implementation
- ❌ Complete CUDA kernel for batch spatial operations
- ❌ Geometry data structure transfer to GPU device memory
- ❌ OpenCL kernel compilation and execution
- ❌ Error handling for GPU memory allocation failures
- ❌ Performance optimization (memory coalescing, shared memory)

## Architecture

### Data Flow
```
1. Input: SpatialBatchInputs with candidate geometries
   ↓
2. Host → Device: Transfer geometry data to GPU memory
   ↓
3. Kernel Launch: Parallel execution on GPU
   ↓
4. Device → Host: Transfer results back to CPU
   ↓
5. Output: SpatialBatchResults with intersection mask
```

### Memory Layout

**Recommended SoA (Structure of Arrays) layout for GPU efficiency:**

```cpp
struct GpuGeometryBatch {
    // Coordinates (flattened)
    double* d_coords_x;      // X coordinates
    double* d_coords_y;      // Y coordinates
    double* d_coords_z;      // Z coordinates (optional)
    
    // Geometry metadata
    uint32_t* d_geom_offsets;   // Starting index for each geometry
    uint32_t* d_geom_counts;    // Number of coordinates per geometry
    uint8_t* d_geom_types;      // GeometryType enum values
    
    // MBRs (for fast filtering)
    double* d_mbr_minx;
    double* d_mbr_miny;
    double* d_mbr_maxx;
    double* d_mbr_maxy;
    
    // Output
    uint8_t* d_results;         // Intersection results (1 = hit, 0 = miss)
    
    size_t num_geometries;
};
```

## CUDA Implementation

### Step 1: Data Transfer to GPU

**File**: `src/geo/gpu_backend_production.cpp`  
**Function**: `CudaBackend::batchIntersects`

```cpp
SpatialBatchResults CudaBackend::batchIntersects(const SpatialBatchInputs& in) override {
    SpatialBatchResults out;
    out.mask.resize(in.count);
    
    if (!is_available_ || in.count == 0) {
        return out;
    }
    
    // 1. Allocate device memory
    GpuGeometryBatch gpu_batch;
    gpu_batch.num_geometries = in.count;
    
    cudaMalloc(&gpu_batch.d_results, in.count * sizeof(uint8_t));
    cudaMalloc(&gpu_batch.d_mbr_minx, in.count * sizeof(double));
    cudaMalloc(&gpu_batch.d_mbr_miny, in.count * sizeof(double));
    cudaMalloc(&gpu_batch.d_mbr_maxx, in.count * sizeof(double));
    cudaMalloc(&gpu_batch.d_mbr_maxy, in.count * sizeof(double));
    
    // TODO: Allocate memory for detailed geometry data
    // This requires extending SpatialBatchInputs to include geometry details
    
    // 2. Copy query geometry to device
    // TODO: Implement based on actual SpatialBatchInputs structure
    
    // 3. Copy candidate MBRs to device
    // TODO: Implement based on actual data source
    
    // 4. Launch kernel
    const int threads_per_block = 256;
    const int num_blocks = (in.count + threads_per_block - 1) / threads_per_block;
    
    cuda_batch_intersects_kernel<<<num_blocks, threads_per_block>>>(
        gpu_batch.d_mbr_minx, gpu_batch.d_mbr_miny,
        gpu_batch.d_mbr_maxx, gpu_batch.d_mbr_maxy,
        gpu_batch.d_results,
        in.count
    );
    
    // 5. Check for kernel launch errors
    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
        THEMIS_ERROR("CUDA kernel launch failed: {}", cudaGetErrorString(err));
        // Fallback to CPU
        CpuParallelBackend cpu_fallback;
        return cpu_fallback.batchIntersects(in);
    }
    
    // 6. Wait for kernel completion
    cudaDeviceSynchronize();
    
    // 7. Copy results back to host
    cudaMemcpy(out.mask.data(), gpu_batch.d_results, 
               in.count * sizeof(uint8_t), cudaMemcpyDeviceToHost);
    
    // 8. Cleanup device memory
    cudaFree(gpu_batch.d_results);
    cudaFree(gpu_batch.d_mbr_minx);
    cudaFree(gpu_batch.d_mbr_miny);
    cudaFree(gpu_batch.d_mbr_maxx);
    cudaFree(gpu_batch.d_mbr_maxy);
    
    return out;
}
```

### Step 2: CUDA Kernel Implementation

**File**: `src/geo/gpu_backend_production.cpp` (or separate `.cu` file)

```cpp
__global__ void cuda_batch_intersects_kernel(
    const double* query_mbr,        // Query MBR: [minx, miny, maxx, maxy]
    const double* candidate_mbrs,   // Candidate MBRs (flattened)
    uint8_t* results,               // Output: intersection mask
    int count                       // Number of candidates
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= count) return;
    
    // Load query MBR from constant/global memory
    double q_minx = query_mbr[0];
    double q_miny = query_mbr[1];
    double q_maxx = query_mbr[2];
    double q_maxy = query_mbr[3];
    
    // Load candidate MBR
    int offset = idx * 4;
    double c_minx = candidate_mbrs[offset + 0];
    double c_miny = candidate_mbrs[offset + 1];
    double c_maxx = candidate_mbrs[offset + 2];
    double c_maxy = candidate_mbrs[offset + 3];
    
    // MBR intersection test
    bool intersects = !(q_minx > c_maxx || q_maxx < c_minx ||
                       q_miny > c_maxy || q_maxy < c_miny);
    
    results[idx] = intersects ? 1 : 0;
}
```

### Step 3: Advanced CUDA Kernel (Point-in-Polygon)

```cpp
__device__ bool cuda_point_in_polygon(
    double px, double py,
    const double* ring_x, const double* ring_y,
    int ring_start, int ring_size
) {
    bool inside = false;
    int j = ring_size - 1;
    
    for (int i = 0; i < ring_size; j = i++) {
        double xi = ring_x[ring_start + i];
        double yi = ring_y[ring_start + i];
        double xj = ring_x[ring_start + j];
        double yj = ring_y[ring_start + j];
        
        if (((yi > py) != (yj > py)) &&
            (px < (xj - xi) * (py - yi) / (yj - yi) + xi)) {
            inside = !inside;
        }
    }
    
    return inside;
}

__global__ void cuda_point_polygon_intersects_kernel(
    const double* query_points_x,
    const double* query_points_y,
    const double* polygon_rings_x,
    const double* polygon_rings_y,
    const uint32_t* ring_offsets,
    const uint32_t* ring_sizes,
    uint8_t* results,
    int count
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= count) return;
    
    double px = query_points_x[idx];
    double py = query_points_y[idx];
    
    bool inside = cuda_point_in_polygon(
        px, py,
        polygon_rings_x, polygon_rings_y,
        ring_offsets[idx], ring_sizes[idx]
    );
    
    results[idx] = inside ? 1 : 0;
}
```

### Step 4: Performance Optimization

**Shared Memory for Geometry Caching:**

```cpp
__global__ void cuda_optimized_intersects_kernel(
    const double* query_mbr,
    const double* candidate_mbrs,
    uint8_t* results,
    int count
) {
    // Cache query MBR in shared memory
    __shared__ double s_query_mbr[4];
    
    if (threadIdx.x == 0) {
        s_query_mbr[0] = query_mbr[0];
        s_query_mbr[1] = query_mbr[1];
        s_query_mbr[2] = query_mbr[2];
        s_query_mbr[3] = query_mbr[3];
    }
    __syncthreads();
    
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= count) return;
    
    // Use cached query MBR
    double q_minx = s_query_mbr[0];
    double q_miny = s_query_mbr[1];
    double q_maxx = s_query_mbr[2];
    double q_maxy = s_query_mbr[3];
    
    // Load candidate MBR with coalesced access
    int offset = idx * 4;
    double c_minx = candidate_mbrs[offset + 0];
    double c_miny = candidate_mbrs[offset + 1];
    double c_maxx = candidate_mbrs[offset + 2];
    double c_maxy = candidate_mbrs[offset + 3];
    
    bool intersects = !(q_minx > c_maxx || q_maxx < c_minx ||
                       q_miny > c_maxy || q_maxy < c_miny);
    
    results[idx] = intersects ? 1 : 0;
}
```

## OpenCL Implementation

### Step 1: Kernel Source Code

**File**: `src/geo/opencl_kernels.cl` (new file)

```c
__kernel void opencl_mbr_intersects(
    __global const double* query_mbr,
    __global const double* candidate_mbrs,
    __global uchar* results,
    const int count
) {
    int idx = get_global_id(0);
    if (idx >= count) return;
    
    // Load query MBR
    double q_minx = query_mbr[0];
    double q_miny = query_mbr[1];
    double q_maxx = query_mbr[2];
    double q_maxy = query_mbr[3];
    
    // Load candidate MBR
    int offset = idx * 4;
    double c_minx = candidate_mbrs[offset + 0];
    double c_miny = candidate_mbrs[offset + 1];
    double c_maxx = candidate_mbrs[offset + 2];
    double c_maxy = candidate_mbrs[offset + 3];
    
    // MBR intersection test
    uchar intersects = (q_minx <= c_maxx && q_maxx >= c_minx &&
                        q_miny <= c_maxy && q_maxy >= c_miny) ? 1 : 0;
    
    results[idx] = intersects;
}

__kernel void opencl_point_in_polygon(
    __global const double* points_x,
    __global const double* points_y,
    __global const double* polygon_x,
    __global const double* polygon_y,
    __global const uint* ring_offsets,
    __global const uint* ring_sizes,
    __global uchar* results,
    const int count
) {
    int idx = get_global_id(0);
    if (idx >= count) return;
    
    double px = points_x[idx];
    double py = points_y[idx];
    
    uint ring_start = ring_offsets[idx];
    uint ring_size = ring_sizes[idx];
    
    // Ray-casting algorithm
    uchar inside = 0;
    uint j = ring_size - 1;
    
    for (uint i = 0; i < ring_size; j = i++) {
        double xi = polygon_x[ring_start + i];
        double yi = polygon_y[ring_start + i];
        double xj = polygon_x[ring_start + j];
        double yj = polygon_y[ring_start + j];
        
        if (((yi > py) != (yj > py)) &&
            (px < (xj - xi) * (py - yi) / (yj - yi) + xi)) {
            inside = !inside;
        }
    }
    
    results[idx] = inside;
}
```

### Step 2: OpenCL Backend Implementation

**File**: `src/geo/gpu_backend_production.cpp`

```cpp
SpatialBatchResults OpenCLBackend::batchIntersects(const SpatialBatchInputs& in) override {
    SpatialBatchResults out;
    out.mask.resize(in.count);
    
    if (!is_available_ || in.count == 0) {
        return out;
    }
    
    cl_int err;
    
    // 1. Load and compile kernel (cache compiled kernel)
    if (!program_) {
        std::string kernel_source = loadKernelSource("opencl_kernels.cl");
        const char* source_ptr = kernel_source.c_str();
        size_t source_len = kernel_source.length();
        
        program_ = clCreateProgramWithSource(context_, 1, &source_ptr, &source_len, &err);
        if (err != CL_SUCCESS) {
            THEMIS_ERROR("OpenCL program creation failed: {}", err);
            CpuParallelBackend cpu_fallback;
            return cpu_fallback.batchIntersects(in);
        }
        
        err = clBuildProgram(program_, 1, &device_, nullptr, nullptr, nullptr);
        if (err != CL_SUCCESS) {
            // Get build log
            size_t log_size;
            clGetProgramBuildInfo(program_, device_, CL_PROGRAM_BUILD_LOG, 0, nullptr, &log_size);
            std::vector<char> log(log_size);
            clGetProgramBuildInfo(program_, device_, CL_PROGRAM_BUILD_LOG, log_size, log.data(), nullptr);
            THEMIS_ERROR("OpenCL build failed: {}", log.data());
            
            CpuParallelBackend cpu_fallback;
            return cpu_fallback.batchIntersects(in);
        }
    }
    
    // 2. Create kernel
    cl_kernel kernel = clCreateKernel(program_, "opencl_mbr_intersects", &err);
    if (err != CL_SUCCESS) {
        THEMIS_ERROR("OpenCL kernel creation failed: {}", err);
        CpuParallelBackend cpu_fallback;
        return cpu_fallback.batchIntersects(in);
    }
    
    // 3. Create buffers
    cl_mem d_query_mbr = clCreateBuffer(context_, CL_MEM_READ_ONLY, 4 * sizeof(double), nullptr, &err);
    cl_mem d_candidate_mbrs = clCreateBuffer(context_, CL_MEM_READ_ONLY, in.count * 4 * sizeof(double), nullptr, &err);
    cl_mem d_results = clCreateBuffer(context_, CL_MEM_WRITE_ONLY, in.count * sizeof(uint8_t), nullptr, &err);
    
    // 4. Copy data to device
    // TODO: Copy actual geometry data from SpatialBatchInputs
    
    // 5. Set kernel arguments
    clSetKernelArg(kernel, 0, sizeof(cl_mem), &d_query_mbr);
    clSetKernelArg(kernel, 1, sizeof(cl_mem), &d_candidate_mbrs);
    clSetKernelArg(kernel, 2, sizeof(cl_mem), &d_results);
    clSetKernelArg(kernel, 3, sizeof(int), &in.count);
    
    // 6. Execute kernel
    size_t global_work_size = in.count;
    size_t local_work_size = 256;
    
    err = clEnqueueNDRangeKernel(queue_, kernel, 1, nullptr, &global_work_size, &local_work_size, 0, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        THEMIS_ERROR("OpenCL kernel execution failed: {}", err);
        
        // Cleanup and fallback
        clReleaseMemObject(d_query_mbr);
        clReleaseMemObject(d_candidate_mbrs);
        clReleaseMemObject(d_results);
        clReleaseKernel(kernel);
        
        CpuParallelBackend cpu_fallback;
        return cpu_fallback.batchIntersects(in);
    }
    
    // 7. Read results back
    clEnqueueReadBuffer(queue_, d_results, CL_TRUE, 0, in.count * sizeof(uint8_t), out.mask.data(), 0, nullptr, nullptr);
    
    // 8. Cleanup
    clReleaseMemObject(d_query_mbr);
    clReleaseMemObject(d_candidate_mbrs);
    clReleaseMemObject(d_results);
    clReleaseKernel(kernel);
    
    return out;
}
```

## Testing Strategy

### Unit Tests
1. **Small batch tests**: 10-100 geometries
2. **Medium batch tests**: 1,000-10,000 geometries
3. **Large batch tests**: 100,000+ geometries
4. **Correctness tests**: Compare GPU results with CPU reference
5. **Error handling tests**: GPU memory exhaustion, invalid inputs

### Performance Tests
1. **Benchmark suite**: Compare CUDA vs OpenCL vs CPU-parallel
2. **Memory profiling**: Check for leaks and optimal usage
3. **Scaling tests**: Performance with increasing batch sizes

### Integration Tests
1. **End-to-end spatial queries**: Full query pipeline with GPU backend
2. **Fallback behavior**: Verify graceful degradation when GPU unavailable
3. **Multi-threaded access**: Thread safety of GPU backend

## Dependencies

### CUDA
```cmake
find_package(CUDA REQUIRED)
target_link_libraries(themis_core PRIVATE CUDA::cudart)
```

### OpenCL
```cmake
find_package(OpenCL REQUIRED)
target_link_libraries(themis_core PRIVATE OpenCL::OpenCL)
```

## Performance Targets

| Batch Size | CPU-Parallel | CUDA Target | OpenCL Target |
|-----------|--------------|-------------|---------------|
| 1,000 | 42 ms | 3-5 ms | 5-8 ms |
| 10,000 | 420 ms | 30-50 ms | 50-80 ms |
| 100,000 | 4,200 ms | 300-500 ms | 500-800 ms |

**Target Speedup**: 10-15x over CPU-parallel, 125-188x over single-threaded CPU

## References

1. **CUDA Programming Guide**: https://docs.nvidia.com/cuda/cuda-c-programming-guide/
2. **OpenCL Programming Guide**: https://www.khronos.org/opencl/
3. **Computational Geometry Algorithms**: De Berg et al., "Computational Geometry: Algorithms and Applications"
4. **GPU Spatial Join**: "Efficient Parallel Spatial Join on GPUs" (various papers)

## Implementation Checklist

- [ ] Extend `SpatialBatchInputs` to include geometry details
- [ ] Implement CUDA memory transfer for geometries
- [ ] Implement basic CUDA MBR intersection kernel
- [ ] Implement advanced CUDA point-in-polygon kernel
- [ ] Implement CUDA polygon-polygon intersection kernel
- [ ] Add CUDA error handling and fallback
- [ ] Create OpenCL kernel source files
- [ ] Implement OpenCL kernel compilation
- [ ] Implement OpenCL buffer management
- [ ] Implement OpenCL kernel execution
- [ ] Add OpenCL error handling and fallback
- [ ] Create unit tests for CUDA backend
- [ ] Create unit tests for OpenCL backend
- [ ] Create performance benchmarks
- [ ] Add integration tests
- [ ] Profile and optimize memory access patterns
- [ ] Document API and usage

## Estimated Timeline

- **Week 1**: CUDA basic kernels and memory management
- **Week 2**: CUDA advanced kernels and optimization
- **Week 3**: OpenCL implementation and testing
- **Week 4**: Integration, benchmarking, and documentation

---

**Created**: February 7, 2026  
**For**: ThemisDB v1.4.0  
**Status**: Implementation Guide
