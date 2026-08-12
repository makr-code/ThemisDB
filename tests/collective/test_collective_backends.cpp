// RESTORED FROM HISTORY: 892fbc132819cf3446b54bb51b8b14ec2dd61db5


#include "acceleration/nccl_vector_backend.h"
#include "acceleration/rccl_vector_backend.h"
#include <gtest/gtest.h>
#include <vector>
#include <cmath>
#include <string>
#include <iostream>

#ifdef THEMIS_ENABLE_NCCL
#include <cuda_runtime.h>
#endif

#ifdef THEMIS_ENABLE_RCCL
#include <hip/hip_runtime.h>
#endif

using namespace themis::acceleration;

// =============================================================================
// NCCL Backend Tests
// =============================================================================

#ifdef THEMIS_ENABLE_NCCL

TEST(NCCLVectorBackendTest, Initialization) {
    if (!NCCLVectorBackend::isNCCLAvailable()) {
        GTEST_SKIP() << "NCCL not available, skipping test";
    }
    
    NCCLVectorBackend backend;
    NCCLVectorBackend::Config config;
    config.worldSize = 1;
    config.rank = 0;
    config.deviceIds = {0};
    
    EXPECT_TRUE(backend.initialize(config));
    EXPECT_TRUE(backend.isInitialized());
    EXPECT_EQ(backend.getRank(), 0);
    EXPECT_EQ(backend.getWorldSize(), 1);
}

TEST(NCCLVectorBackendTest, VersionInfo) {
    if (!NCCLVectorBackend::isNCCLAvailable()) {
        GTEST_SKIP() << "NCCL not available, skipping test";
    }
    
    int version = NCCLVectorBackend::getNCCLVersion();
    std::string versionStr = NCCLVectorBackend::getNCCLVersionString();
    
    EXPECT_GT(version, 0);
    EXPECT_FALSE(versionStr.empty());
    EXPECT_NE(versionStr, "Not available");
    
    std::cout << "NCCL Version: " << versionStr << " (" << version << ")\n";
}

TEST(NCCLVectorBackendTest, P2PCapabilityCheck) {
    if (!NCCLVectorBackend::isNCCLAvailable()) {
        GTEST_SKIP() << "NCCL not available, skipping test";
    }
    
    NCCLVectorBackend backend;
    NCCLVectorBackend::Config config;
    config.worldSize = 1;
    config.rank = 0;
    config.deviceIds = {0};
    config.enableP2P = true;
    
    if (backend.initialize(config)) {
        // Check if P2P is available (may not be on all systems)
        bool p2pAvailable = backend.isP2PEnabled();
        std::cout << "P2P transfers: " << (p2pAvailable ? "Available" : "Not available") << "\n";
    }
}

TEST(NCCLVectorBackendTest, NVLinkDetection) {
    if (!NCCLVectorBackend::isNCCLAvailable()) {
        GTEST_SKIP() << "NCCL not available, skipping test";
    }
    
    std::vector<int> deviceIds = {0, 1};  // Try with 2 GPUs
    bool nvlinkSupported = NCCLVectorBackend::checkNVLinkSupport(deviceIds);
    
    std::cout << "NVLink support: " << (nvlinkSupported ? "Yes" : "No") << "\n";
}

TEST(NCCLVectorBackendTest, StatisticsTracking) {
    if (!NCCLVectorBackend::isNCCLAvailable()) {
        GTEST_SKIP() << "NCCL not available, skipping test";
    }
    
    NCCLVectorBackend backend;
    NCCLVectorBackend::Config config;
    config.worldSize = 1;
    config.rank = 0;
    config.deviceIds = {0};
    
    if (backend.initialize(config)) {
        auto stats = backend.getStatistics();
        
        EXPECT_EQ(stats.numCollectives, 0);
        EXPECT_EQ(stats.numP2PTransfers, 0);
        EXPECT_EQ(stats.totalBytesSent, 0);
        EXPECT_EQ(stats.totalBytesReceived, 0);
        
        std::cout << "Initial statistics: OK\n";
    }
}

TEST(NCCLVectorBackendTest, MergeTopK_SingleRank_CopiesLocalResult) {
    if (!NCCLVectorBackend::isNCCLAvailable()) {
        GTEST_SKIP() << "NCCL not available, skipping test";
    }

    NCCLVectorBackend backend;
    NCCLVectorBackend::Config config;
    config.worldSize = 1;
    config.rank = 0;
    config.deviceIds = {0};

    if (!backend.initialize(config)) {
        GTEST_SKIP() << "NCCL initialization failed";
    }

    const size_t k = 4;
    // Build local top-K on device
    std::vector<uint32_t> h_local_idx   = {3u, 1u, 0u, 2u};
    std::vector<float>    h_local_dist  = {0.1f, 0.3f, 0.5f, 0.9f};
    std::vector<uint32_t> h_global_idx(k, 0u);
    std::vector<float>    h_global_dist(k, 0.0f);

    uint32_t* d_li; float* d_ld; uint32_t* d_gi; float* d_gd;
    ASSERT_EQ(cudaMalloc(&d_li, k * sizeof(uint32_t)), cudaSuccess);
    ASSERT_EQ(cudaMalloc(&d_ld, k * sizeof(float)),    cudaSuccess);
    ASSERT_EQ(cudaMalloc(&d_gi, k * sizeof(uint32_t)), cudaSuccess);
    ASSERT_EQ(cudaMalloc(&d_gd, k * sizeof(float)),    cudaSuccess);

    ASSERT_EQ(cudaMemcpy(d_li, h_local_idx.data(),  k * sizeof(uint32_t), cudaMemcpyHostToDevice), cudaSuccess);
    ASSERT_EQ(cudaMemcpy(d_ld, h_local_dist.data(), k * sizeof(float),    cudaMemcpyHostToDevice), cudaSuccess);

    EXPECT_TRUE(backend.mergeTopK(d_li, d_ld, k, d_gi, d_gd, k, 0, nullptr));

    ASSERT_EQ(cudaMemcpy(h_global_idx.data(),  d_gi, k * sizeof(uint32_t), cudaMemcpyDeviceToHost), cudaSuccess);
    ASSERT_EQ(cudaMemcpy(h_global_dist.data(), d_gd, k * sizeof(float),    cudaMemcpyDeviceToHost), cudaSuccess);

    for (size_t i = 0; i < k; ++i) {
        EXPECT_EQ(h_global_idx[i],  h_local_idx[i])  << "index mismatch at " << i;
        EXPECT_FLOAT_EQ(h_global_dist[i], h_local_dist[i]) << "distance mismatch at " << i;
    }

    cudaFree(d_li); cudaFree(d_ld); cudaFree(d_gi); cudaFree(d_gd);
}

TEST(NCCLVectorBackendTest, MergeTopK_RejectsKGreaterThanLocalK) {
    NCCLVectorBackend backend;
    NCCLVectorBackend::Config config;
    config.worldSize = 1;
    config.rank = 0;
    if (!NCCLVectorBackend::isNCCLAvailable() || !backend.initialize(config)) {
        // Run validation even without NCCL hardware
        EXPECT_FALSE(backend.mergeTopK(nullptr, nullptr, 3u, nullptr, nullptr, 5u, 0, nullptr));
        return;
    }
    // k(5) > localK(3) must be rejected
    EXPECT_FALSE(backend.mergeTopK(nullptr, nullptr, 3u, nullptr, nullptr, 5u, 0, nullptr));
}

#endif // THEMIS_ENABLE_NCCL

#ifdef THEMIS_ENABLE_RCCL

TEST(RCCLVectorBackendTest, Initialization) {
    if (!RCCLVectorBackend::isRCCLAvailable()) {
        GTEST_SKIP() << "RCCL not available, skipping test";
    }
    
    RCCLVectorBackend backend;
    RCCLVectorBackend::Config config;
    config.worldSize = 1;
    config.rank = 0;
    config.deviceIds = {0};
    
    EXPECT_TRUE(backend.initialize(config));
    EXPECT_TRUE(backend.isInitialized());
    EXPECT_EQ(backend.getRank(), 0);
    EXPECT_EQ(backend.getWorldSize(), 1);
}

TEST(RCCLVectorBackendTest, VersionInfo) {
    if (!RCCLVectorBackend::isRCCLAvailable()) {
        GTEST_SKIP() << "RCCL not available, skipping test";
    }
    
    int version = RCCLVectorBackend::getRCCLVersion();
    std::string versionStr = RCCLVectorBackend::getRCCLVersionString();
    
    EXPECT_GT(version, 0);
    EXPECT_FALSE(versionStr.empty());
    EXPECT_NE(versionStr, "Not available");
    
    std::cout << "RCCL Version: " << versionStr << " (" << version << ")\n";
}

TEST(RCCLVectorBackendTest, P2PCapabilityCheck) {
    if (!RCCLVectorBackend::isRCCLAvailable()) {
        GTEST_SKIP() << "RCCL not available, skipping test";
    }
    
    RCCLVectorBackend backend;
    RCCLVectorBackend::Config config;
    config.worldSize = 1;
    config.rank = 0;
    config.deviceIds = {0};
    config.enableP2P = true;
    
    if (backend.initialize(config)) {
        // Check if P2P is available (may not be on all systems)
        bool p2pAvailable = backend.isP2PEnabled();
        std::cout << "P2P transfers: " << (p2pAvailable ? "Available" : "Not available") << "\n";
    }
}

TEST(RCCLVectorBackendTest, XGMIDetection) {
    if (!RCCLVectorBackend::isRCCLAvailable()) {
        GTEST_SKIP() << "RCCL not available, skipping test";
    }
    
    std::vector<int> deviceIds = {0, 1};  // Try with 2 GPUs
    bool xgmiSupported = RCCLVectorBackend::checkXGMISupport(deviceIds);
    
    std::cout << "XGMI (Infinity Fabric) support: " << (xgmiSupported ? "Yes" : "No") << "\n";
}

TEST(RCCLVectorBackendTest, StatisticsTracking) {
    if (!RCCLVectorBackend::isRCCLAvailable()) {
        GTEST_SKIP() << "RCCL not available, skipping test";
    }
    
    RCCLVectorBackend backend;
    RCCLVectorBackend::Config config;
    config.worldSize = 1;
    config.rank = 0;
    config.deviceIds = {0};
    
    if (backend.initialize(config)) {
        auto stats = backend.getStatistics();
        
        EXPECT_EQ(stats.numCollectives, 0);
        EXPECT_EQ(stats.numP2PTransfers, 0);
        EXPECT_EQ(stats.totalBytesSent, 0);
        EXPECT_EQ(stats.totalBytesReceived, 0);
        
        std::cout << "Initial statistics: OK\n";
    }
}

TEST(RCCLVectorBackendTest, MergeTopK_SingleRank_CopiesLocalResult) {
    if (!RCCLVectorBackend::isRCCLAvailable()) {
        GTEST_SKIP() << "RCCL not available, skipping test";
    }

    RCCLVectorBackend backend;
    RCCLVectorBackend::Config config;
    config.worldSize = 1;
    config.rank = 0;
    config.deviceIds = {0};

    if (!backend.initialize(config)) {
        GTEST_SKIP() << "RCCL initialization failed";
    }

    const size_t k = 4;
    std::vector<uint32_t> h_local_idx  = {3u, 1u, 0u, 2u};
    std::vector<float>    h_local_dist = {0.1f, 0.3f, 0.5f, 0.9f};
    std::vector<uint32_t> h_global_idx(k, 0u);
    std::vector<float>    h_global_dist(k, 0.0f);

    uint32_t* d_li; float* d_ld; uint32_t* d_gi; float* d_gd;
    ASSERT_EQ(hipMalloc(&d_li, k * sizeof(uint32_t)), hipSuccess);
    ASSERT_EQ(hipMalloc(&d_ld, k * sizeof(float)),    hipSuccess);
    ASSERT_EQ(hipMalloc(&d_gi, k * sizeof(uint32_t)), hipSuccess);
    ASSERT_EQ(hipMalloc(&d_gd, k * sizeof(float)),    hipSuccess);

    ASSERT_EQ(hipMemcpy(d_li, h_local_idx.data(),  k * sizeof(uint32_t), hipMemcpyHostToDevice), hipSuccess);
    ASSERT_EQ(hipMemcpy(d_ld, h_local_dist.data(), k * sizeof(float),    hipMemcpyHostToDevice), hipSuccess);

    EXPECT_TRUE(backend.mergeTopK(d_li, d_ld, k, d_gi, d_gd, k, 0, nullptr));

    ASSERT_EQ(hipMemcpy(h_global_idx.data(),  d_gi, k * sizeof(uint32_t), hipMemcpyDeviceToHost), hipSuccess);
    ASSERT_EQ(hipMemcpy(h_global_dist.data(), d_gd, k * sizeof(float),    hipMemcpyDeviceToHost), hipSuccess);

    for (size_t i = 0; i < k; ++i) {
        EXPECT_EQ(h_global_idx[i],  h_local_idx[i])  << "index mismatch at " << i;
        EXPECT_FLOAT_EQ(h_global_dist[i], h_local_dist[i]) << "distance mismatch at " << i;
    }

    hipFree(d_li); hipFree(d_ld); hipFree(d_gi); hipFree(d_gd);
}

TEST(RCCLVectorBackendTest, MergeTopK_RejectsKGreaterThanLocalK) {
    RCCLVectorBackend backend;
    RCCLVectorBackend::Config config;
    config.worldSize = 1;
    config.rank = 0;
    if (!RCCLVectorBackend::isRCCLAvailable() || !backend.initialize(config)) {
        EXPECT_FALSE(backend.mergeTopK(nullptr, nullptr, 3u, nullptr, nullptr, 5u, 0, nullptr));
        return;
    }
    EXPECT_FALSE(backend.mergeTopK(nullptr, nullptr, 3u, nullptr, nullptr, 5u, 0, nullptr));
}

#endif // THEMIS_ENABLE_RCCL

#if !defined(THEMIS_ENABLE_NCCL) && !defined(THEMIS_ENABLE_RCCL)

TEST(CollectiveBackendsTest, NoBackendsAvailable) {
    // When no backends are compiled in, this test just confirms the stubs work
    EXPECT_FALSE(NCCLVectorBackend::isNCCLAvailable());
    EXPECT_FALSE(RCCLVectorBackend::isRCCLAvailable());
    
    EXPECT_EQ(NCCLVectorBackend::getNCCLVersion(), 0);
    EXPECT_EQ(RCCLVectorBackend::getRCCLVersion(), 0);
    
    EXPECT_EQ(NCCLVectorBackend::getNCCLVersionString(), "Not available");
    EXPECT_EQ(RCCLVectorBackend::getRCCLVersionString(), "Not available");
    
    std::cout << "No NCCL/RCCL backends compiled in (expected in CPU-only builds)\n";
}

#endif