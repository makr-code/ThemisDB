// OpenCL Backend Implementation - Universal GPU Fallback
// Supports any OpenCL 1.2+ capable device (NVIDIA/AMD/Intel/ARM/Qualcomm)
// Copyright (c) 2024 ThemisDB

#include "acceleration/compute_backend.h"
#include <vector>
#include <cmath>
#include <iostream>
#include <algorithm>
#include <cstring>

// OpenCL headers (conditionally included if available)
#ifdef THEMIS_ENABLE_OPENCL
#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif
#endif

namespace themis {
namespace acceleration {

#ifdef THEMIS_ENABLE_OPENCL

// OpenCL kernel source
static const char* openclKernelSource = R"(
__kernel void computeL2Distance(
    __global const float* queries,
    __global const float* vectors,
    __global float* distances,
    const unsigned int numQueries,
    const unsigned int numVectors,
    const unsigned int dimension)
{
    size_t q = get_global_id(0);
    size_t v = get_global_id(1);
    
    if (q >= numQueries || v >= numVectors) return;
    
    float sum = 0.0f;
    for (unsigned int d = 0; d < dimension; d++) {
        float diff = queries[q * dimension + d] - vectors[v * dimension + d];
        sum += diff * diff;
    }
    
    // Return squared L2 distance (no sqrt for consistency and performance)
    distances[q * numVectors + v] = sum;
}

__kernel void computeCosineDistance(
    __global const float* queries,
    __global const float* vectors,
    __global float* distances,
    const unsigned int numQueries,
    const unsigned int numVectors,
    const unsigned int dimension)
{
    size_t q = get_global_id(0);
    size_t v = get_global_id(1);
    
    if (q >= numQueries || v >= numVectors) return;
    
    float dotProduct = 0.0f;
    float normQ = 0.0f;
    float normV = 0.0f;
    
    for (unsigned int d = 0; d < dimension; d++) {
        float qVal = queries[q * dimension + d];
        float vVal = vectors[v * dimension + d];
        dotProduct += qVal * vVal;
        normQ += qVal * qVal;
        normV += vVal * vVal;
    }
    
    float cosineSim = dotProduct / (sqrt(normQ) * sqrt(normV) + 1e-8f);
    distances[q * numVectors + v] = 1.0f - cosineSim;
}
)";

class OpenCLVectorBackend : public IVectorBackend {
private:
    cl_platform_id platform_ = nullptr;
    cl_device_id device_ = nullptr;
    cl_context context_ = nullptr;
    cl_command_queue queue_ = nullptr;
    cl_program program_ = nullptr;
    cl_kernel l2Kernel_ = nullptr;
    cl_kernel cosineKernel_ = nullptr;
    bool initialized_ = false;

public:
    OpenCLVectorBackend() = default;
    
    ~OpenCLVectorBackend() {
        if (l2Kernel_) clReleaseKernel(l2Kernel_);
        if (cosineKernel_) clReleaseKernel(cosineKernel_);
        if (program_) clReleaseProgram(program_);
        if (queue_) clReleaseCommandQueue(queue_);
        if (context_) clReleaseContext(context_);
    }

    BackendType type() const noexcept override { return BackendType::OPENCL; }
    
    const char* name() const noexcept override { return "OpenCL (Universal)"; }
    
    bool isAvailable() const noexcept override {
        cl_platform_id platform;
        cl_uint numPlatforms;
        cl_int err = clGetPlatformIDs(1, &platform, &numPlatforms);
        return (err == CL_SUCCESS && numPlatforms > 0);
    }
    
    BackendCapabilities getCapabilities() const override {
        BackendCapabilities caps;
        caps.supportsVectorOps = initialized_;
        caps.supportsAsync = false;
        caps.deviceName = "OpenCL Device";
        return caps;
    }
    
    bool initialize() override {
        cl_int err;
        
        // Get platform with error context
        cl_uint numPlatforms;
        err = clGetPlatformIDs(1, &platform_, &numPlatforms);
        if (err != CL_SUCCESS || numPlatforms == 0) {
            std::cerr << "OpenCL initialization failed:" << std::endl;
            std::cerr << "  Error code: " << err << std::endl;
            std::cerr << "  Platforms found: " << numPlatforms << std::endl;
            if (numPlatforms == 0) {
                std::cerr << "  No OpenCL platforms found" << std::endl;
                std::cerr << "  Check: OpenCL drivers installed?" << std::endl;
                std::cerr << "  Check: GPU vendor drivers up to date?" << std::endl;
            }
            return false;
        }
        
        // Get platform info
        char platformName[128] = {0};
        char platformVersion[128] = {0};
        clGetPlatformInfo(platform_, CL_PLATFORM_NAME, sizeof(platformName), platformName, nullptr);
        clGetPlatformInfo(platform_, CL_PLATFORM_VERSION, sizeof(platformVersion), platformVersion, nullptr);
        
        // Get GPU device (prefer GPU over CPU)
        cl_uint numDevices;
        err = clGetDeviceIDs(platform_, CL_DEVICE_TYPE_GPU, 1, &device_, &numDevices);
        if (err != CL_SUCCESS) {
            // Fallback to CPU
            std::cerr << "OpenCL: No GPU devices found, trying CPU..." << std::endl;
            err = clGetDeviceIDs(platform_, CL_DEVICE_TYPE_CPU, 1, &device_, &numDevices);
            if (err != CL_SUCCESS) {
                std::cerr << "OpenCL: No devices found (GPU or CPU)" << std::endl;
                std::cerr << "  Error code: " << err << std::endl;
                std::cerr << "  Platform: " << platformName << std::endl;
                return false;
            }
        }
        
        // Get device info
        char deviceName[128] = {0};
        char deviceVersion[128] = {0};
        cl_uint computeUnits = 0;
        cl_ulong globalMemSize = 0;
        
        clGetDeviceInfo(device_, CL_DEVICE_NAME, sizeof(deviceName), deviceName, nullptr);
        clGetDeviceInfo(device_, CL_DEVICE_VERSION, sizeof(deviceVersion), deviceVersion, nullptr);
        clGetDeviceInfo(device_, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(computeUnits), &computeUnits, nullptr);
        clGetDeviceInfo(device_, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(globalMemSize), &globalMemSize, nullptr);
        
        std::cout << "OpenCL backend initialized successfully" << std::endl;
        std::cout << "  Platform: " << platformName << " (" << platformVersion << ")" << std::endl;
        std::cout << "  Device: " << deviceName << std::endl;
        std::cout << "  Device Version: " << deviceVersion << std::endl;
        std::cout << "  Compute Units: " << computeUnits << std::endl;
        std::cout << "  Global Memory: " << (globalMemSize / (1024*1024*1024)) << " GB" << std::endl;
        
        // Create context
        context_ = clCreateContext(nullptr, 1, &device_, nullptr, nullptr, &err);
        if (err != CL_SUCCESS) {
            std::cerr << "OpenCL: Failed to create context" << std::endl;
            std::cerr << "  Error code: " << err << std::endl;
            return false;
        }
        
        // Create command queue
        queue_ = clCreateCommandQueue(context_, device_, 0, &err);
        if (err != CL_SUCCESS) {
            std::cerr << "OpenCL: Failed to create command queue" << std::endl;
            std::cerr << "  Error code: " << err << std::endl;
            clReleaseContext(context_);
            return false;
        }
        
        // Create program
        const char* source = openclKernelSource;
        size_t sourceSize = strlen(source);
        program_ = clCreateProgramWithSource(context_, 1, &source, &sourceSize, &err);
        if (err != CL_SUCCESS) {
            std::cerr << "OpenCL: Failed to create program" << std::endl;
            std::cerr << "  Error code: " << err << std::endl;
            clReleaseCommandQueue(queue_);
            clReleaseContext(context_);
            return false;
        }
        
        // Build program
        err = clBuildProgram(program_, 1, &device_, nullptr, nullptr, nullptr);
        if (err != CL_SUCCESS) {
            std::cerr << "OpenCL: Kernel compilation failed" << std::endl;
            std::cerr << "  Error code: " << err << std::endl;
            
            size_t logSize;
            clGetProgramBuildInfo(program_, device_, CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize);
            if (logSize > 0) {
                std::vector<char> log(logSize);
                clGetProgramBuildInfo(program_, device_, CL_PROGRAM_BUILD_LOG, logSize, log.data(), nullptr);
                std::cerr << "  Build log:" << std::endl;
                std::cerr << log.data() << std::endl;
            }
            
            clReleaseProgram(program_);
            clReleaseCommandQueue(queue_);
            clReleaseContext(context_);
            return false;
        }
        
        // Create kernels
        l2Kernel_ = clCreateKernel(program_, "computeL2Distance", &err);
        if (err != CL_SUCCESS) {
            std::cerr << "OpenCL: Failed to create L2 kernel" << std::endl;
            std::cerr << "  Error code: " << err << std::endl;
            clReleaseProgram(program_);
            clReleaseCommandQueue(queue_);
            clReleaseContext(context_);
            return false;
        }
        
        cosineKernel_ = clCreateKernel(program_, "computeCosineDistance", &err);
        if (err != CL_SUCCESS) {
            std::cerr << "OpenCL: Failed to create cosine kernel" << std::endl;
            std::cerr << "  Error code: " << err << std::endl;
            clReleaseKernel(l2Kernel_);
            clReleaseProgram(program_);
            clReleaseCommandQueue(queue_);
            clReleaseContext(context_);
            return false;
        }
        
        initialized_ = true;
        return true;
    }
    
    void shutdown() override {
        initialized_ = false;
    }
    
    std::vector<float> computeDistances(
        const float* queries, size_t numQueries, size_t dimension,
        const float* vectors, size_t numVectors,
        bool useL2) override 
    {
        if (!initialized_) {
            std::cerr << "OpenCL: Backend not initialized\n";
            return {};
        }
        
        cl_int err;
        size_t resultSize = numQueries * numVectors;
        std::vector<float> distances(resultSize);
        
        // Create buffers
        cl_mem d_queries = clCreateBuffer(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                          numQueries * dimension * sizeof(float),
                                          const_cast<float*>(queries), &err);
        
        cl_mem d_vectors = clCreateBuffer(context_, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                          numVectors * dimension * sizeof(float),
                                          const_cast<float*>(vectors), &err);
        
        cl_mem d_distances = clCreateBuffer(context_, CL_MEM_WRITE_ONLY,
                                            resultSize * sizeof(float), nullptr, &err);
        
        // Select kernel
        cl_kernel kernel = useL2 ? l2Kernel_ : cosineKernel_;
        
        // Set kernel arguments
        unsigned int uNumQueries = static_cast<unsigned int>(numQueries);
        unsigned int uNumVectors = static_cast<unsigned int>(numVectors);
        unsigned int uDimension = static_cast<unsigned int>(dimension);
        
        clSetKernelArg(kernel, 0, sizeof(cl_mem), &d_queries);
        clSetKernelArg(kernel, 1, sizeof(cl_mem), &d_vectors);
        clSetKernelArg(kernel, 2, sizeof(cl_mem), &d_distances);
        clSetKernelArg(kernel, 3, sizeof(unsigned int), &uNumQueries);
        clSetKernelArg(kernel, 4, sizeof(unsigned int), &uNumVectors);
        clSetKernelArg(kernel, 5, sizeof(unsigned int), &uDimension);
        
        // Execute kernel
        size_t globalWorkSize[2] = {numQueries, numVectors};
        err = clEnqueueNDRangeKernel(queue_, kernel, 2, nullptr, globalWorkSize, nullptr, 0, nullptr, nullptr);
        if (err != CL_SUCCESS) {
            std::cerr << "OpenCL: Kernel execution failed: " << err << "\n";
            clReleaseMemObject(d_queries);
            clReleaseMemObject(d_vectors);
            clReleaseMemObject(d_distances);
            return {};
        }
        
        // Read results
        clEnqueueReadBuffer(queue_, d_distances, CL_TRUE, 0, resultSize * sizeof(float),
                           distances.data(), 0, nullptr, nullptr);
        
        // Cleanup
        clReleaseMemObject(d_queries);
        clReleaseMemObject(d_vectors);
        clReleaseMemObject(d_distances);
        
        return distances;
    }
    
    std::vector<std::vector<std::pair<uint32_t, float>>> batchKnnSearch(
        const float* queries, size_t numQueries, size_t dimension,
        const float* vectors, size_t numVectors,
        size_t k, bool useL2) override 
    {
        auto distances = computeDistances(queries, numQueries, dimension, vectors, numVectors, useL2);
        
        std::vector<std::vector<std::pair<uint32_t, float>>> results(numQueries);
        
        for (size_t q = 0; q < numQueries; q++) {
            std::vector<std::pair<float, uint32_t>> pairs;
            pairs.reserve(numVectors);
            
            for (size_t v = 0; v < numVectors; v++) {
                pairs.push_back({distances[q * numVectors + v], static_cast<uint32_t>(v)});
            }
            
            std::partial_sort(pairs.begin(), pairs.begin() + k, pairs.end());
            
            results[q].reserve(k);
            for (size_t i = 0; i < k; i++) {
                results[q].push_back({pairs[i].second, pairs[i].first});
            }
        }
        
        return results;
    }
};

#else

// Stub implementation when OpenCL is not available
class OpenCLVectorBackend : public IVectorBackend {
public:
    BackendType type() const noexcept override { return BackendType::OPENCL; }
    const char* name() const noexcept override { return "OpenCL (Not Available)"; }
    bool isAvailable() const noexcept override { return false; }
    BackendCapabilities getCapabilities() const override { return {}; }
    bool initialize() override { return false; }
    void shutdown() override {}
    
    std::vector<float> computeDistances(
        const float*, size_t, size_t, const float*, size_t, bool) override {
        return {};
    }
    
    std::vector<std::vector<std::pair<uint32_t, float>>> batchKnnSearch(
        const float*, size_t, size_t, const float*, size_t, size_t, bool) override {
        return {};
    }
};

#endif

// Factory function
std::unique_ptr<IVectorBackend> createOpenCLBackend() {
    return std::make_unique<OpenCLVectorBackend>();
}

} // namespace acceleration
} // namespace themis
