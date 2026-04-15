/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            opencl_backend.cpp                                 ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:11:18                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     385                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// OpenCL Backend Implementation - Universal GPU Fallback
// Supports any OpenCL 1.2+ capable device (NVIDIA/AMD/Intel/ARM/Qualcomm)
// Copyright (c) 2024 ThemisDB

#include "acceleration/opencl_backend.h"
#include "acceleration/error_codes.h"
#include "acceleration/error_context.h"
#include <iostream>
#include <algorithm>
#include <cstring>

namespace themis {
namespace acceleration {

#ifdef THEMIS_ENABLE_OPENCL

// OpenCL kernel source — kept in the .cpp to avoid polluting the public header
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

BackendType OpenCLVectorBackend::type() const noexcept { return BackendType::OPENCL; }

const char* OpenCLVectorBackend::name() const noexcept { return "OpenCL (Universal)"; }

bool OpenCLVectorBackend::isAvailable() const noexcept {
    cl_platform_id platform;
    cl_uint numPlatforms;
    cl_int err = clGetPlatformIDs(1, &platform, &numPlatforms);
    return (err == CL_SUCCESS && numPlatforms > 0);
}

BackendCapabilities OpenCLVectorBackend::getCapabilities() const {
    BackendCapabilities caps;
    caps.supportsVectorOps      = initialized_;
    caps.supportsBatchProcessing = initialized_;
    caps.supportsAsync          = false;
    caps.supportedPrecisions    = PrecisionMode::FP32;
    caps.supportedMetrics       = metricBit(DistanceMetric::L2)
                                | metricBit(DistanceMetric::COSINE);
    caps.deviceName = initialized_ ? "OpenCL Device" : "OpenCL Device (not initialized)";
    return caps;
}

bool OpenCLVectorBackend::initialize() {
    cl_int err;
    
    // Get platform with error context
    cl_uint numPlatforms;
    err = clGetPlatformIDs(1, &platform_, &numPlatforms);
    if (err != CL_SUCCESS || numPlatforms == 0) {
        if (numPlatforms == 0) {
            setError(ErrorContext(
                AccelerationErrorCode::PlatformNotAvailable,
                "OpenCL",
                "No OpenCL platforms found",
                "Install OpenCL drivers for your GPU (NVIDIA, AMD, or Intel)"
            ));
        } else {
            setError(ErrorContextHelpers::createDriverError("OpenCL"));
        }
        std::cerr << lastError_.format() << std::endl;
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
            setError(ErrorContextHelpers::createNoDevicesError("OpenCL"));
            std::cerr << lastError_.format() << std::endl;
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
    
    // Create context using RAII
    try {
        context_.create(nullptr, 1, &device_);
    } catch (const std::exception& e) {
        setError(ErrorContextHelpers::createContextError("OpenCL", e.what()));
        std::cerr << lastError_.format() << std::endl;
        return false;
    }
    
    // Create command queue using RAII
    try {
        queue_.create(context_.get(), device_);
    } catch (const std::exception& e) {
        setError(ErrorContextHelpers::createQueueError("OpenCL", e.what()));
        std::cerr << lastError_.format() << std::endl;
        return false;
    }
    
    // Create program using RAII
    try {
        program_.createWithSource(context_.get(), openclKernelSource);
    } catch (const std::exception& e) {
        setError(ErrorContext(
            AccelerationErrorCode::KernelCompilationFailed,
            "OpenCL",
            "Failed to create program: " + std::string(e.what()),
            "Check kernel source code syntax"
        ));
        std::cerr << lastError_.format() << std::endl;
        return false;
    }
    
    // Build program
    try {
        program_.build(1, &device_);
    } catch (const std::exception& e) {
        // Get detailed build log
        std::string buildLog;
        size_t logSize;
        if (clGetProgramBuildInfo(program_.get(), device_, CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize) == CL_SUCCESS && logSize > 0) {
            std::vector<char> log(logSize);
            clGetProgramBuildInfo(program_.get(), device_, CL_PROGRAM_BUILD_LOG, logSize, log.data(), nullptr);
            buildLog = std::string(log.data());
        }
        setError(ErrorContextHelpers::createKernelCompilationError("OpenCL", "distance kernels", buildLog));
        std::cerr << lastError_.format() << std::endl;
        return false;
    }
    
    // Create kernels using RAII
    try {
        l2Kernel_.create(program_.get(), "computeL2Distance");
    } catch (const std::exception& e) {
        setError(ErrorContext(
            AccelerationErrorCode::KernelNotFound,
            "OpenCL",
            "Failed to create L2 kernel: " + std::string(e.what()),
            "Ensure computeL2Distance kernel is defined in program"
        ));
        std::cerr << lastError_.format() << std::endl;
        return false;
    }
    
    try {
        cosineKernel_.create(program_.get(), "computeCosineDistance");
    } catch (const std::exception& e) {
        setError(ErrorContext(
            AccelerationErrorCode::KernelNotFound,
            "OpenCL",
            "Failed to create cosine kernel: " + std::string(e.what()),
            "Ensure computeCosineDistance kernel is defined in program"
        ));
        std::cerr << lastError_.format() << std::endl;
        return false;
    }
    
    // Clear error on success
    clearError();
    initialized_ = true;
    return true;
}

void OpenCLVectorBackend::shutdown() {
    // All resources automatically cleaned up by RAII
    initialized_ = false;
}

std::vector<float> OpenCLVectorBackend::computeDistances(
    const float* queries, size_t numQueries, size_t dimension,
    const float* vectors, size_t numVectors,
    bool useL2)
{
    if (!initialized_) {
        std::cerr << "OpenCL: Backend not initialized\n";
        return {};
    }
    
    cl_int err;
    size_t resultSize = numQueries * numVectors;
    std::vector<float> distances(resultSize);
    
    // Create buffers
    cl_mem d_queries = clCreateBuffer(context_.get(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                      numQueries * dimension * sizeof(float),
                                      const_cast<float*>(queries), &err);
    
    cl_mem d_vectors = clCreateBuffer(context_.get(), CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                                      numVectors * dimension * sizeof(float),
                                      const_cast<float*>(vectors), &err);
    
    cl_mem d_distances = clCreateBuffer(context_.get(), CL_MEM_WRITE_ONLY,
                                        resultSize * sizeof(float), nullptr, &err);
    
    // Select kernel (get raw handle from RAII wrapper)
    cl_kernel kernel = useL2 ? l2Kernel_.get() : cosineKernel_.get();
    
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
    err = clEnqueueNDRangeKernel(queue_.get(), kernel, 2, nullptr, globalWorkSize, nullptr, 0, nullptr, nullptr);
    if (err != CL_SUCCESS) {
        std::cerr << "OpenCL: Kernel execution failed: " << err << "\n";
        clReleaseMemObject(d_queries);
        clReleaseMemObject(d_vectors);
        clReleaseMemObject(d_distances);
        return {};
    }
    
    // Read results
    clEnqueueReadBuffer(queue_.get(), d_distances, CL_TRUE, 0, resultSize * sizeof(float),
                       distances.data(), 0, nullptr, nullptr);
    
    // Cleanup
    clReleaseMemObject(d_queries);
    clReleaseMemObject(d_vectors);
    clReleaseMemObject(d_distances);
    
    return distances;
}

std::vector<std::vector<std::pair<uint32_t, float>>> OpenCLVectorBackend::batchKnnSearch(
    const float* queries, size_t numQueries, size_t dimension,
    const float* vectors, size_t numVectors,
    size_t k, bool useL2)
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

#else // THEMIS_ENABLE_OPENCL not defined

// Stub method definitions when OpenCL is not available
BackendType OpenCLVectorBackend::type() const noexcept { return BackendType::OPENCL; }
const char* OpenCLVectorBackend::name() const noexcept { return "OpenCL (Not Available)"; }
bool OpenCLVectorBackend::isAvailable() const noexcept { return false; }
BackendCapabilities OpenCLVectorBackend::getCapabilities() const { return {}; }
bool OpenCLVectorBackend::initialize() { return false; }
void OpenCLVectorBackend::shutdown() {}

std::vector<float> OpenCLVectorBackend::computeDistances(
    const float*, size_t, size_t, const float*, size_t, bool) {
    return {};
}

std::vector<std::vector<std::pair<uint32_t, float>>> OpenCLVectorBackend::batchKnnSearch(
    const float*, size_t, size_t, const float*, size_t, size_t, bool) {
    return {};
}

#endif // THEMIS_ENABLE_OPENCL

// Factory function
std::unique_ptr<IVectorBackend> createOpenCLBackend() {
    return std::make_unique<OpenCLVectorBackend>();
}

} // namespace acceleration
} // namespace themis
