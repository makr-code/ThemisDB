// Metal Backend Implementation for Apple Silicon/macOS
// Accelerates vector operations on M1/M2/M3/M4 chips
// Copyright (c) 2024 ThemisDB

#include "acceleration/graphics_backends.h"
#import <Metal/Metal.h>
#import <Foundation/Foundation.h>
#include <vector>
#include <cmath>
#include <iostream>

namespace themis {
namespace acceleration {

// Metal Shading Language kernel source (embedded)
static const char* metalKernelSource = R"(
#include <metal_stdlib>
using namespace metal;

// L2 Distance Kernel
kernel void computeL2Distance(
    device const float* queries [[buffer(0)]],
    device const float* vectors [[buffer(1)]],
    device float* distances [[buffer(2)]],
    constant uint& numQueries [[buffer(3)]],
    constant uint& numVectors [[buffer(4)]],
    constant uint& dimension [[buffer(5)]],
    uint2 gid [[thread_position_in_grid]])
{
    uint queryIdx = gid.x;
    uint vectorIdx = gid.y;
    
    if (queryIdx >= numQueries || vectorIdx >= numVectors) return;
    
    float sum = 0.0f;
    for (uint d = 0; d < dimension; d++) {
        float diff = queries[queryIdx * dimension + d] - vectors[vectorIdx * dimension + d];
        sum += diff * diff;
    }
    
    // Return squared L2 distance (no sqrt for consistency and performance)
    distances[queryIdx * numVectors + vectorIdx] = sum;
}

// Cosine Distance Kernel
kernel void computeCosineDistance(
    device const float* queries [[buffer(0)]],
    device const float* vectors [[buffer(1)]],
    device float* distances [[buffer(2)]],
    constant uint& numQueries [[buffer(3)]],
    constant uint& numVectors [[buffer(4)]],
    constant uint& dimension [[buffer(5)]],
    uint2 gid [[thread_position_in_grid]])
{
    uint queryIdx = gid.x;
    uint vectorIdx = gid.y;
    
    if (queryIdx >= numQueries || vectorIdx >= numVectors) return;
    
    float dotProduct = 0.0f;
    float normQ = 0.0f;
    float normV = 0.0f;
    
    for (uint d = 0; d < dimension; d++) {
        float q = queries[queryIdx * dimension + d];
        float v = vectors[vectorIdx * dimension + d];
        dotProduct += q * v;
        normQ += q * q;
        normV += v * v;
    }
    
    float cosineSim = dotProduct / (sqrt(normQ) * sqrt(normV) + 1e-8f);
    distances[queryIdx * numVectors + vectorIdx] = 1.0f - cosineSim;
}
)";

class MetalVectorBackend : public IVectorBackend {
private:
    id<MTLDevice> device_ = nil;
    id<MTLCommandQueue> commandQueue_ = nil;
    id<MTLLibrary> library_ = nil;
    id<MTLComputePipelineState> l2Pipeline_ = nil;
    id<MTLComputePipelineState> cosinePipeline_ = nil;
    bool initialized_ = false;

public:
    MetalVectorBackend() = default;
    
    ~MetalVectorBackend() {
        if (commandQueue_) [commandQueue_ release];
        if (l2Pipeline_) [l2Pipeline_ release];
        if (cosinePipeline_) [cosinePipeline_ release];
        if (library_) [library_ release];
        if (device_) [device_ release];
    }

    BackendType type() const override { return BackendType::METAL; }
    
    std::string name() const override { return "Metal (Apple Silicon)"; }
    
    bool isAvailable() const override {
        @autoreleasepool {
            id<MTLDevice> testDevice = MTLCreateSystemDefaultDevice();
            if (testDevice) {
                [testDevice release];
                return true;
            }
            return false;
        }
    }
    
    bool initialize() override {
        @autoreleasepool {
            // Get default Metal device
            device_ = MTLCreateSystemDefaultDevice();
            if (!device_) {
                std::cerr << "Metal initialization failed:" << std::endl;
                std::cerr << "  No Metal-capable device found" << std::endl;
                std::cerr << "  Check: Running on macOS 10.11+ or iOS 8+?" << std::endl;
                std::cerr << "  Check: GPU supports Metal API?" << std::endl;
                return false;
            }
            
            [device_ retain];
            
            // Get device info
            NSString* deviceName = [device_ name];
            bool isLowPower = [device_ isLowPower];
            bool isHeadless = [device_ isHeadless];
            
            NSLog(@"Metal Backend: Device initialized");
            NSLog(@"  Device: %@", deviceName);
            NSLog(@"  Low Power: %@", isLowPower ? @"Yes" : @"No");
            NSLog(@"  Headless: %@", isHeadless ? @"Yes" : @"No");
            
            // Check Metal feature set
            #if TARGET_OS_OSX
            if (@available(macOS 10.15, *)) {
                NSLog(@"  Metal Feature Set: macOS GPU Family 2 (or newer)");
            }
            #elif TARGET_OS_IOS
            if (@available(iOS 13.0, *)) {
                NSLog(@"  Metal Feature Set: iOS GPU Family 5 (or newer)");
            }
            #endif
            
            // Create command queue
            commandQueue_ = [device_ newCommandQueue];
            if (!commandQueue_) {
                std::cerr << "Metal: Failed to create command queue" << std::endl;
                std::cerr << "  Device may not support compute operations" << std::endl;
                [device_ release];
                return false;
            }
            
            [commandQueue_ retain];
            
            // Compile Metal kernels
            NSError* error = nil;
            NSString* source = [NSString stringWithUTF8String:metalKernelSource];
            
            library_ = [device_ newLibraryWithSource:source options:nil error:&error];
            if (!library_) {
                std::cerr << "Metal: Kernel compilation failed" << std::endl;
                if (error) {
                    NSLog(@"  Error: %@", error);
                    NSLog(@"  Description: %@", [error localizedDescription]);
                }
                [commandQueue_ release];
                [device_ release];
                return false;
            }
            
            [library_ retain];
            
            // Create compute pipelines
            id<MTLFunction> l2Function = [library_ newFunctionWithName:@"computeL2Distance"];
            id<MTLFunction> cosineFunction = [library_ newFunctionWithName:@"computeCosineDistance"];
            
            if (!l2Function || !cosineFunction) {
                std::cerr << "Metal: Failed to find kernel functions" << std::endl;
                std::cerr << "  Check: Kernel names match shader code?" << std::endl;
                [library_ release];
                [commandQueue_ release];
                [device_ release];
                return false;
            }
            
            l2Pipeline_ = [device_ newComputePipelineStateWithFunction:l2Function error:&error];
            if (!l2Pipeline_) {
                std::cerr << "Metal: L2 pipeline creation failed" << std::endl;
                if (error) {
                    NSLog(@"  Error: %@", [error localizedDescription]);
                }
                [l2Function release];
                [cosineFunction release];
                [library_ release];
                [commandQueue_ release];
                [device_ release];
                return false;
            }
            
            cosinePipeline_ = [device_ newComputePipelineStateWithFunction:cosineFunction error:&error];
            if (!cosinePipeline_) {
                std::cerr << "Metal: Cosine pipeline creation failed" << std::endl;
                if (error) {
                    NSLog(@"  Error: %@", [error localizedDescription]);
                }
                [l2Pipeline_ release];
                [l2Function release];
                [cosineFunction release];
                [library_ release];
                [commandQueue_ release];
                [device_ release];
                return false;
            }
            
            [l2Function release];
            [cosineFunction release];
            [l2Pipeline_ retain];
            [cosinePipeline_ retain];
            
            initialized_ = true;
            std::cout << "Metal backend initialized successfully on " 
                      << [deviceName UTF8String] << std::endl;
            return true;
        }
    }
    
    void shutdown() override {
        initialized_ = false;
    }
    
    std::vector<float> computeDistances(
        const float* queries, size_t numQueries,
        const float* vectors, size_t numVectors,
        size_t dimension, bool useL2) override 
    {
        if (!initialized_) {
            std::cerr << "Metal: Backend not initialized\n";
            return {};
        }
        
        @autoreleasepool {
            size_t resultSize = numQueries * numVectors;
            std::vector<float> distances(resultSize);
            
            // Create Metal buffers
            id<MTLBuffer> queriesBuffer = [device_ newBufferWithBytes:queries
                                                               length:numQueries * dimension * sizeof(float)
                                                              options:MTLResourceStorageModeShared];
            id<MTLBuffer> vectorsBuffer = [device_ newBufferWithBytes:vectors
                                                               length:numVectors * dimension * sizeof(float)
                                                              options:MTLResourceStorageModeShared];
            id<MTLBuffer> distancesBuffer = [device_ newBufferWithLength:resultSize * sizeof(float)
                                                                 options:MTLResourceStorageModeShared];
            
            // Create command buffer
            id<MTLCommandBuffer> commandBuffer = [commandQueue_ commandBuffer];
            id<MTLComputeCommandEncoder> encoder = [commandBuffer computeCommandEncoder];
            
            // Select pipeline
            id<MTLComputePipelineState> pipeline = useL2 ? l2Pipeline_ : cosinePipeline_;
            [encoder setComputePipelineState:pipeline];
            
            // Set buffers
            [encoder setBuffer:queriesBuffer offset:0 atIndex:0];
            [encoder setBuffer:vectorsBuffer offset:0 atIndex:1];
            [encoder setBuffer:distancesBuffer offset:0 atIndex:2];
            
            uint32_t params[3] = {
                static_cast<uint32_t>(numQueries),
                static_cast<uint32_t>(numVectors),
                static_cast<uint32_t>(dimension)
            };
            [encoder setBytes:&params[0] length:sizeof(uint32_t) atIndex:3];
            [encoder setBytes:&params[1] length:sizeof(uint32_t) atIndex:4];
            [encoder setBytes:&params[2] length:sizeof(uint32_t) atIndex:5];
            
            // Dispatch
            MTLSize gridSize = MTLSizeMake(numQueries, numVectors, 1);
            NSUInteger threadGroupSize = [pipeline maxTotalThreadsPerThreadgroup];
            NSUInteger w = std::min(static_cast<NSUInteger>(16), numQueries);
            NSUInteger h = std::min(threadGroupSize / w, numVectors);
            MTLSize threadgroupSize = MTLSizeMake(w, h, 1);
            
            [encoder dispatchThreads:gridSize threadsPerThreadgroup:threadgroupSize];
            [encoder endEncoding];
            
            // Execute
            [commandBuffer commit];
            [commandBuffer waitUntilCompleted];
            
            // Copy results
            memcpy(distances.data(), [distancesBuffer contents], resultSize * sizeof(float));
            
            // Cleanup
            [queriesBuffer release];
            [vectorsBuffer release];
            [distancesBuffer release];
            
            return distances;
        }
    }
    
    std::vector<VectorSearchResult> batchKnnSearch(
        const float* queries, size_t numQueries,
        const float* vectors, size_t numVectors,
        size_t dimension, size_t k, bool useL2) override 
    {
        // Compute all distances first
        auto distances = computeDistances(queries, numQueries, vectors, numVectors, dimension, useL2);
        
        // Extract top-k for each query (CPU-based for now)
        std::vector<VectorSearchResult> results(numQueries * k);
        
        for (size_t q = 0; q < numQueries; q++) {
            std::vector<std::pair<float, size_t>> pairs;
            pairs.reserve(numVectors);
            
            for (size_t v = 0; v < numVectors; v++) {
                pairs.push_back({distances[q * numVectors + v], v});
            }
            
            std::partial_sort(pairs.begin(), pairs.begin() + k, pairs.end());
            
            for (size_t i = 0; i < k; i++) {
                results[q * k + i].vectorId = pairs[i].second;
                results[q * k + i].distance = pairs[i].first;
            }
        }
        
        return results;
    }
};

// Factory function
std::unique_ptr<IVectorBackend> createMetalBackend() {
    return std::make_unique<MetalVectorBackend>();
}

} // namespace acceleration
} // namespace themis
