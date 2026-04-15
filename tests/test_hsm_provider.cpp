/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_hsm_provider.cpp                              ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:18:53                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟠 BETA                                         ║
    • Quality Score:   41.0/100                                       ║
    • Total Lines:     434                                            ║
    • Open Issues:     TODOs: 0, Stubs: 16                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🔧 In Progress                                               ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include <gtest/gtest.h>
#include "security/hsm_provider.h"
#include <filesystem>
#include <fstream>

using namespace themis::security;

/**
 * HSM Provider Tests
 * 
 * These tests require SoftHSM2 to be installed:
 * 
 * Installation:
 * - Ubuntu/Debian: sudo apt-get install softhsm2
 * - macOS: brew install softhsm
 * - Windows: Download from https://github.com/opendnssec/SoftHSMv2/releases
 * 
 * Configuration (one-time setup):
 * ```bash
 * # Initialize token
 * softhsm2-util --init-token --slot 0 --label "themis-test" --pin 1234 --so-pin 5678
 * 
 * # Generate test key
 * pkcs11-tool --module /usr/lib/softhsm/libsofthsm2.so \
 *   --login --pin 1234 \
 *   --keypairgen --key-type RSA:2048 \
 *   --label "themis-signing-key"
 * ```
 * 
 * Environment Variables:
 * - THEMIS_TEST_HSM_LIBRARY: Path to PKCS#11 library (default: /usr/lib/softhsm/libsofthsm2.so)
 * - THEMIS_TEST_HSM_PIN: HSM PIN (default: 1234)
 */

class HSMProviderTest : public ::testing::Test {
protected:
    std::string hsm_library_path;
    std::string hsm_pin;
    uint32_t hsm_slot = 0;
    
    void SetUp() override {
        // Get HSM library path from environment or use default
        const char* env_lib = std::getenv("THEMIS_TEST_HSM_LIBRARY");
        if (env_lib) {
            hsm_library_path = env_lib;
        } else {
            // Try common locations
            std::vector<std::string> common_paths = {
                "/usr/lib/softhsm/libsofthsm2.so",                    // Debian/Ubuntu
                "/usr/lib/x86_64-linux-gnu/softhsm/libsofthsm2.so",  // Ubuntu
                "/usr/local/lib/softhsm/libsofthsm2.so",             // macOS/Linux
                "/opt/homebrew/lib/softhsm/libsofthsm2.so",          // macOS M1
            };
            
            for (const auto& path : common_paths) {
                if (std::filesystem::exists(path)) {
                    hsm_library_path = path;
                    break;
                }
            }
        }
        
        // Get PIN from environment or use default
        const char* env_pin = std::getenv("THEMIS_TEST_HSM_PIN");
        hsm_pin = env_pin ? env_pin : "1234";
    }
    
    bool isHSMAvailable() const {
        return !hsm_library_path.empty() && 
               std::filesystem::exists(hsm_library_path);
    }
    
    HSMConfig createTestConfig() {
        HSMConfig config;
        config.library_path = hsm_library_path;
        config.slot_id = hsm_slot;
        config.pin = hsm_pin;
        config.key_label = "themis-signing-key";
        config.signature_algorithm = "RSA-SHA256";
        config.verbose = true;
        return config;
    }
};

TEST_F(HSMProviderTest, ConstructorDoesNotThrow) {
    HSMConfig config = createTestConfig();
    EXPECT_NO_THROW({
        HSMProvider hsm(config);
    });
}

TEST_F(HSMProviderTest, InitializeWithoutLibraryFallsBackStub) {
    HSMConfig config;
    config.library_path = "/nonexistent/library.so"; // force fallback
    config.slot_id = 0;
    config.pin = "1234";
    HSMProvider hsm(config);
    // Fallback design: initialize returns true but real session not active
    EXPECT_TRUE(hsm.initialize());
    EXPECT_TRUE(hsm.isReady()); // stub ready
    std::string info = hsm.getTokenInfo();
    EXPECT_NE(info.find("fallback"), std::string::npos);
}

TEST_F(HSMProviderTest, InitializeWithSoftHSMRealOrSkip) {
    if (!isHSMAvailable()) {
        GTEST_SKIP() << "SoftHSM2 not available. Install with: sudo apt-get install softhsm2";
    }
    HSMConfig config = createTestConfig();
    HSMProvider hsm(config);
    ASSERT_TRUE(hsm.initialize());
    std::string info = hsm.getTokenInfo();
    // Either real session or fallback if key missing
    EXPECT_TRUE(hsm.isReady());
    EXPECT_FALSE(info.empty());
    hsm.finalize();
    EXPECT_FALSE(hsm.isReady());
}

TEST_F(HSMProviderTest, SignAndVerifyFallbackOrReal) {
    HSMConfig config = createTestConfig();
    HSMProvider hsm(config);
    ASSERT_TRUE(hsm.initialize());
    std::vector<uint8_t> data = {'H','S','M'};
    auto sig = hsm.sign(data);
    EXPECT_TRUE(sig.success);
    EXPECT_EQ(sig.signature_b64.rfind("hex:",0),0u); // hex encoding for both modes currently
    EXPECT_TRUE(hsm.verify(data, sig.signature_b64));
    data[0] = 'X';
    EXPECT_FALSE(hsm.verify(data, sig.signature_b64));
}

TEST_F(HSMProviderTest, SignHashDirectly) {
    if (!isHSMAvailable()) {
        GTEST_SKIP() << "SoftHSM2 not available";
    }
    
    HSMConfig config = createTestConfig();
    HSMProvider hsm(config);
    
    if (!hsm.initialize()) {
        GTEST_SKIP() << "HSM initialization failed";
    }
    
    // Pre-computed SHA-256 hash (32 bytes)
    std::vector<uint8_t> hash(32, 0xAB);
    
    auto result = hsm.signHash(hash);
    
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(result.signature_b64.empty());
    EXPECT_GT(result.timestamp_ms, 0);
}

TEST_F(HSMProviderTest, ListKeysReturnsOneEntry) {
    HSMConfig config = createTestConfig();
    HSMProvider hsm(config);
    ASSERT_TRUE(hsm.initialize());
    auto keys = hsm.listKeys();
    ASSERT_EQ(keys.size(), 1u);
    EXPECT_FALSE(keys[0].label.empty());
}

TEST_F(HSMProviderTest, GenerateKeyPairNotImplemented) {
    HSMConfig config = createTestConfig();
    HSMProvider hsm(config);
    ASSERT_TRUE(hsm.initialize());
    EXPECT_FALSE(hsm.generateKeyPair("test-key", 2048, false));
}

TEST_F(HSMProviderTest, HSMPKIClientIntegrationBasic) {
    HSMConfig config = createTestConfig();
    HSMPKIClient client(config);
    if(!client.isReady()) GTEST_SKIP() << "Not ready";
    std::vector<uint8_t> data = {'D','a','t','a'};
    auto sig = client.sign(data);
    EXPECT_TRUE(sig.success);
    EXPECT_TRUE(client.verify(data, sig.signature_b64));
}

TEST_F(HSMProviderTest, MultipleSignOperations) {
    HSMConfig config = createTestConfig();
    HSMProvider hsm(config);
    ASSERT_TRUE(hsm.initialize());
    for(int i=0;i<10;++i){
        std::vector<uint8_t> data(64, (uint8_t)i);
        auto sig = hsm.sign(data);
        EXPECT_TRUE(sig.success);
    }
}

TEST_F(HSMProviderTest, DifferentAlgorithmsFallbackHex) {
    std::vector<std::string> algos = {"RSA-SHA256","RSA-SHA384"};
    for(auto& a: algos){
        HSMConfig cfg = createTestConfig(); cfg.signature_algorithm = a; HSMProvider hsm(cfg); hsm.initialize();
        std::vector<uint8_t> data = {'T','e','s','t'};
        auto sig = hsm.sign(data);
        EXPECT_TRUE(sig.success); EXPECT_EQ(sig.algorithm, a);
    }
}

// Performance benchmark test (disabled by default)
TEST_F(HSMProviderTest, DISABLED_SignPerformanceBenchmark) {
    if (!isHSMAvailable()) {
        GTEST_SKIP() << "SoftHSM2 not available";
    }
    
    HSMConfig config = createTestConfig();
    HSMProvider hsm(config);
    
    if (!hsm.initialize()) {
        GTEST_SKIP() << "HSM initialization failed";
    }
    
    const int num_operations = 100;
    std::vector<uint8_t> data(1024, 0xAA);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_operations; ++i) {
        auto result = hsm.sign(data);
        EXPECT_TRUE(result.success);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    double ops_per_sec = (num_operations * 1000.0) / duration.count();
    
    std::cout << "HSM Sign Performance:\n"
              << "  Operations: " << num_operations << "\n"
              << "  Duration: " << duration.count() << " ms\n"
              << "  Operations/sec: " << ops_per_sec << "\n";
}

// Documentation test - shows example usage
TEST_F(HSMProviderTest, UsageExample) {
    if (!isHSMAvailable()) {
        GTEST_SKIP() << "SoftHSM2 not available";
    }
    
    // Example: How to use HSMProvider
    
    // 1. Configure HSM
    HSMConfig config;
    config.library_path = hsm_library_path;
    config.slot_id = 0;
    config.pin = hsm_pin;
    config.key_label = "themis-signing-key";
    config.signature_algorithm = "RSA-SHA256";
    
    // 2. Create provider
    auto hsm = std::make_unique<HSMProvider>(config);
    
    // 3. Initialize
    if (!hsm->initialize()) {
        GTEST_SKIP() << "Initialization failed: " << hsm->getLastError();
    }
    
    // 4. Sign data
    std::vector<uint8_t> data_to_sign = {'M', 'y', ' ', 'D', 'a', 't', 'a'};
    auto signature = hsm->sign(data_to_sign);
    
    ASSERT_TRUE(signature.success);
    
    // 5. Verify signature
    bool is_valid = hsm->verify(data_to_sign, signature.signature_b64);
    EXPECT_TRUE(is_valid);
    
    // 6. Cleanup (automatic via destructor)
    hsm->finalize();
}

TEST_F(HSMProviderTest, PerformanceStatsTracking) {
    HSMConfig config = createTestConfig();
    HSMProvider hsm(config);
    ASSERT_TRUE(hsm.initialize());
    
    // Reset stats
    hsm.resetStats();
    auto initial_stats = hsm.getStats();
    EXPECT_EQ(initial_stats.sign_count, 0);
    EXPECT_EQ(initial_stats.verify_count, 0);
    
    // Perform sign operations
    std::vector<uint8_t> data = {'t', 'e', 's', 't'};
    for (int i = 0; i < 5; ++i) {
        auto sig = hsm.sign(data);
        EXPECT_TRUE(sig.success);
    }
    
    auto after_sign = hsm.getStats();
    EXPECT_EQ(after_sign.sign_count, 5);
    EXPECT_GT(after_sign.total_sign_time_us, 0);
    
    // Perform verify operations
    auto sig = hsm.sign(data);
    for (int i = 0; i < 3; ++i) {
        bool ok = hsm.verify(data, sig.signature_b64);
        EXPECT_TRUE(ok);
    }
    
    auto final_stats = hsm.getStats();
    EXPECT_EQ(final_stats.sign_count, 6); // 5 + 1
    EXPECT_EQ(final_stats.verify_count, 3);
    EXPECT_GT(final_stats.total_verify_time_us, 0);
    
    // Test reset
    hsm.resetStats();
    auto reset_stats = hsm.getStats();
    EXPECT_EQ(reset_stats.sign_count, 0);
    EXPECT_EQ(reset_stats.verify_count, 0);
    
    hsm.finalize();
}

// Security tests for FIND-002: HSM stub detection
TEST_F(HSMProviderTest, StubProviderDetection) {
    // Test with stub provider (no library path)
    HSMConfig config;
    config.library_path = "";  // Force stub
    config.slot_id = 0;
    config.pin = "1234";
    
    HSMProvider hsm(config);
    ASSERT_TRUE(hsm.initialize());
    
    // Stub provider should be detected
    EXPECT_TRUE(hsm.isStubProvider());
    EXPECT_TRUE(hsm.isReady());  // Stub is "ready" but insecure
}

TEST_F(HSMProviderTest, RealProviderDetection) {
    if (!isHSMAvailable()) {
        GTEST_SKIP() << "SoftHSM2 not available - cannot test real provider detection";
    }
    
    HSMConfig config = createTestConfig();
    HSMProvider hsm(config);
    ASSERT_TRUE(hsm.initialize());
    
    // Real provider status depends on whether key exists
    // If no key configured, will fall back to stub
    bool isStub = hsm.isStubProvider();
    std::string info = hsm.getTokenInfo();
    
    // Either way, should be ready
    EXPECT_TRUE(hsm.isReady());
}

TEST_F(HSMProviderTest, PeriodicSecurityCheckStub) {
    HSMConfig config;
    config.library_path = "";  // Force stub
    
    HSMProvider hsm(config);
    ASSERT_TRUE(hsm.initialize());
    ASSERT_TRUE(hsm.isStubProvider());
    
    // Should not throw when called
    EXPECT_NO_THROW({
        hsm.periodicSecurityCheck();
    });
    
    // Call multiple times (simulating periodic checks)
    for (int i = 0; i < 3; ++i) {
        hsm.periodicSecurityCheck();
    }
}

TEST_F(HSMProviderTest, SecurityWarningOnInitialization) {
    // This test verifies that security warnings are logged
    // In a real test, you'd capture log output and verify it
    
    HSMConfig config;
    config.library_path = "";  // Force stub
    
    HSMProvider hsm(config);
    
    // Initialize should succeed but log warnings
    EXPECT_TRUE(hsm.initialize());
    EXPECT_TRUE(hsm.isStubProvider());
    
    // Verify stub status
    std::string tokenInfo = hsm.getTokenInfo();
    EXPECT_NE(tokenInfo.find("stub"), std::string::npos);
}

TEST_F(HSMProviderTest, StubProviderStillFunctional) {
    // Even though stub is insecure, it should still work for development
    HSMConfig config;
    config.library_path = "";  // Force stub
    
    HSMProvider hsm(config);
    ASSERT_TRUE(hsm.initialize());
    ASSERT_TRUE(hsm.isStubProvider());
    
    // Basic operations should still work
    std::vector<uint8_t> data = {'t', 'e', 's', 't'};
    auto sig = hsm.sign(data);
    EXPECT_TRUE(sig.success);
    EXPECT_FALSE(sig.signature_b64.empty());
    EXPECT_EQ(sig.cert_serial, "STUB-CERT");
    
    // Verify should work
    EXPECT_TRUE(hsm.verify(data, sig.signature_b64));
    
    // Stats should work
    auto stats = hsm.getStats();
    EXPECT_GE(stats.sign_count, 1);
}


