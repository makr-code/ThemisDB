/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_collective_backends.cpp                       ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:09:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     243                                            ║
    • Open Issues:     TODOs: 0, Stubs: 3                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "acceleration/nccl_vector_backend.h"
#include "acceleration/rccl_vector_backend.h"
#include <gtest/gtest.h>
#include <vector>
#include <cmath>
#include <string>
#include <iostream>

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

#endif // THEMIS_ENABLE_NCCL

// =============================================================================
// RCCL Backend Tests
// =============================================================================

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

#endif // THEMIS_ENABLE_RCCL

// =============================================================================
// Stub Tests (when backends not available)
// =============================================================================

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