#include <gtest/gtest.h>
#include "security/hsm_provider.h"
#include <filesystem>
#include <fstream>
#include <stdexcept>

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

TEST_F(HSMProviderTest, HSMPKIClientGetCertSerialDerivesDeterministicFallbackValue) {
    HSMConfig config = createTestConfig();
    HSMPKIClient client(config);

    auto serial = client.getCertSerial();
    ASSERT_TRUE(serial.has_value());
    EXPECT_NE(serial->find("stub-"), std::string::npos);

    auto serial_again = client.getCertSerial();
    EXPECT_EQ(*serial_again, *serial);
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

TEST_F(HSMProviderTest, SignHashBridgeIsUsed) {
    HSMConfig config = createTestConfig();
    HSMProvider hsm(config);
    ASSERT_TRUE(hsm.initialize());

    HSMProvider::setSignHashFn([](const std::vector<uint8_t>& hash, const std::string& key_label) {
        HSMSignatureResult result;
        result.success = true;
        result.signature_b64 = "bridge-signature";
        result.algorithm = "bridge";
        result.key_id = key_label;
        result.timestamp_ms = hash.size();
        return result;
    });

    auto result = hsm.signHash({1, 2, 3}, "bridge-key");
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.signature_b64, "bridge-signature");
    EXPECT_EQ(result.key_id, "bridge-key");

    HSMProvider::setSignHashFn({});
}

TEST_F(HSMProviderTest, VerifyAndEncryptDecryptBridgesAreUsed) {
    HSMConfig config = createTestConfig();
    HSMProvider hsm(config);
    ASSERT_TRUE(hsm.initialize());

    HSMProvider::setVerifyFn([](const std::vector<uint8_t>& data,
                                const std::string& signature,
                                const std::string& key_label) {
        return data.size() == 3 && signature == "ok" && key_label == "verify-key";
    });
    HSMProvider::setEncryptDataFn([](const std::vector<uint8_t>& data, const std::string&) {
        return std::vector<uint8_t>(data.rbegin(), data.rend());
    });
    HSMProvider::setDecryptDataFn([](const std::vector<uint8_t>& data, const std::string&) {
        return std::vector<uint8_t>(data.rbegin(), data.rend());
    });

    EXPECT_TRUE(hsm.verify({1, 2, 3}, "ok", "verify-key"));
    auto encrypted = hsm.encryptData({1, 2, 3});
    EXPECT_EQ(encrypted, (std::vector<uint8_t>{3, 2, 1}));
    auto decrypted = hsm.decryptData(encrypted);
    EXPECT_EQ(decrypted, (std::vector<uint8_t>{1, 2, 3}));

    HSMProvider::setVerifyFn({});
    HSMProvider::setEncryptDataFn({});
    HSMProvider::setDecryptDataFn({});
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

// ============================================================================
// HSM-215: getCertificate() fail-closed hardening
// ============================================================================

namespace {
bool setEnvVar(const std::string& name, const std::string& value) {
#ifdef _WIN32
    return _putenv_s(name.c_str(), value.c_str()) == 0;
#else
    return ::setenv(name.c_str(), value.c_str(), 1) == 0;
#endif
}

bool unsetEnvVar(const std::string& name) {
#ifdef _WIN32
    return _putenv_s(name.c_str(), "") == 0;
#else
    return ::unsetenv(name.c_str()) == 0;
#endif
}
} // namespace

struct HsmProviderEnvGuard {
    std::string name;
    std::string previous;
    bool had_previous{false};

    HsmProviderEnvGuard(const std::string& var_name, const std::string& value)
        : name(var_name) {
        const char* existing = std::getenv(name.c_str());
        had_previous = (existing != nullptr);
        if (had_previous) previous = existing;
        (void)setEnvVar(name, value);
    }

    ~HsmProviderEnvGuard() {
        if (had_previous) {
            (void)setEnvVar(name, previous);
        } else {
            (void)unsetEnvVar(name);
        }
    }
};

struct HsmProviderEnvUnsetGuard {
    std::string name;
    std::string previous;
    bool had_previous{false};

    explicit HsmProviderEnvUnsetGuard(const std::string& var_name) : name(var_name) {
        const char* existing = std::getenv(name.c_str());
        had_previous = (existing != nullptr);
        if (had_previous) previous = existing;
        (void)unsetEnvVar(name);
    }

    ~HsmProviderEnvUnsetGuard() {
        if (had_previous) {
            (void)setEnvVar(name, previous);
        } else {
            (void)unsetEnvVar(name);
        }
    }
};

// HSM-CERT-01: getCertificate() returns nullopt when no stub opt-in is set
TEST_F(HSMProviderTest, GetCertificateFailsClosedWithoutOptIn) {
    HsmProviderEnvUnsetGuard guard("THEMIS_ALLOW_HSM_STUB");
    HsmProviderEnvUnsetGuard prod_guard("THEMIS_PRODUCTION_MODE");

    HSMConfig config;
    config.library_path = "";
    HSMProvider hsm(config);
    {
        HsmProviderEnvGuard allow_guard("THEMIS_ALLOW_HSM_STUB", "1");
        // Initialize with stub allowed so we get a usable instance
        ASSERT_TRUE(hsm.initialize());
    }

    // Now the opt-in is gone again; getCertificate must fail closed.
    HsmProviderEnvUnsetGuard no_stub("THEMIS_ALLOW_HSM_STUB");

    auto cert = hsm.getCertificate("test-key");
    EXPECT_FALSE(cert.has_value())
        << "getCertificate() must return nullopt without THEMIS_ALLOW_HSM_STUB=1";
}

// HSM-CERT-02: getCertificate() returns dummy PEM when stub is explicitly allowed
TEST_F(HSMProviderTest, GetCertificateReturnsDummyPemWithOptIn) {
    HsmProviderEnvGuard allow_guard("THEMIS_ALLOW_HSM_STUB", "1");
    HsmProviderEnvUnsetGuard prod_guard("THEMIS_PRODUCTION_MODE");

    HSMConfig config;
    config.library_path = "";

    HSMProvider hsm(config);
    ASSERT_TRUE(hsm.initialize());

    auto cert = hsm.getCertificate("test-key");
    ASSERT_TRUE(cert.has_value())
        << "getCertificate() must return value when THEMIS_ALLOW_HSM_STUB=1";
    EXPECT_NE(cert->find("STUB"), std::string::npos)
        << "Returned PEM should be the stub placeholder";
    EXPECT_NE(cert->find("BEGIN CERTIFICATE"), std::string::npos);
}

// HSM-CERT-03: generateKeyPair returns false (not a silent no-op that claims success)
TEST_F(HSMProviderTest, GenerateKeyPairReturnsFalseInStub) {
    HsmProviderEnvGuard allow_guard("THEMIS_ALLOW_HSM_STUB", "1");
    HsmProviderEnvUnsetGuard prod_guard("THEMIS_PRODUCTION_MODE");

    HSMConfig config;
    config.library_path = "";

    HSMProvider hsm(config);
    ASSERT_TRUE(hsm.initialize());

    bool result = hsm.generateKeyPair("test-label", 2048, false);
    EXPECT_FALSE(result)
        << "generateKeyPair() must return false in stub mode to signal no real key was created";
}

// HSM-CERT-04: importCertificate returns false (not a silent no-op that claims success)
TEST_F(HSMProviderTest, ImportCertificateReturnsFalseInStub) {
    HsmProviderEnvGuard allow_guard("THEMIS_ALLOW_HSM_STUB", "1");
    HsmProviderEnvUnsetGuard prod_guard("THEMIS_PRODUCTION_MODE");

    HSMConfig config;
    config.library_path = "";

    HSMProvider hsm(config);
    ASSERT_TRUE(hsm.initialize());

    bool result = hsm.importCertificate("test-key", "-----BEGIN CERTIFICATE-----\n...\n-----END CERTIFICATE-----\n");
    EXPECT_FALSE(result)
        << "importCertificate() must return false in stub mode to signal no real cert was stored";
}

// HSM-KM-BRIDGE-01: injected generate/import callbacks are used in stub mode
TEST_F(HSMProviderTest, KeyManagementCallbacksAreUsed) {
    HsmProviderEnvGuard allow_guard("THEMIS_ALLOW_HSM_STUB", "1");
    HsmProviderEnvUnsetGuard prod_guard("THEMIS_PRODUCTION_MODE");

    HSMConfig config;
    config.library_path = "";
    HSMProvider hsm(config);
    ASSERT_TRUE(hsm.initialize());

    bool generate_called = false;
    bool import_called = false;
    HSMProvider::setGenerateKeyPairFn(
        [&](const std::string& label, uint32_t key_size, bool extractable) {
            generate_called = true;
            EXPECT_EQ(label, "bridge-key");
            EXPECT_EQ(key_size, 3072u);
            EXPECT_TRUE(extractable);
            return true;
        });
    HSMProvider::setImportCertificateFn(
        [&](const std::string& key_label, const std::string& cert_pem) {
            import_called = true;
            EXPECT_EQ(key_label, "bridge-key");
            EXPECT_NE(cert_pem.find("BEGIN CERTIFICATE"), std::string::npos);
            return true;
        });

    EXPECT_TRUE(hsm.generateKeyPair("bridge-key", 3072, true));
    EXPECT_TRUE(hsm.importCertificate("bridge-key",
                                      "-----BEGIN CERTIFICATE-----\nX\n-----END CERTIFICATE-----\n"));
    EXPECT_TRUE(generate_called);
    EXPECT_TRUE(import_called);

    HSMProvider::setGenerateKeyPairFn({});
    HSMProvider::setImportCertificateFn({});
}

// HSM-KM-BRIDGE-02: injected getCertificate callback bypasses stub dummy cert path
TEST_F(HSMProviderTest, GetCertificateCallbackOverridesStubPath) {
    HsmProviderEnvUnsetGuard allow_guard("THEMIS_ALLOW_HSM_STUB");
    HsmProviderEnvUnsetGuard prod_guard("THEMIS_PRODUCTION_MODE");

    HSMConfig config;
    config.library_path = "";
    HSMProvider hsm(config);
    {
        HsmProviderEnvGuard init_allow("THEMIS_ALLOW_HSM_STUB", "1");
        ASSERT_TRUE(hsm.initialize());
    }

    HSMProvider::setGetCertificateFn(
        [](const std::string& key_label) -> std::optional<std::string> {
            if (key_label == "bridge-cert-key") {
                return std::string(
                    "-----BEGIN CERTIFICATE-----\nBRIDGE-CERT\n-----END CERTIFICATE-----\n");
            }
            return std::nullopt;
        });

    auto cert = hsm.getCertificate("bridge-cert-key");
    ASSERT_TRUE(cert.has_value());
    EXPECT_NE(cert->find("BRIDGE-CERT"), std::string::npos);

    HSMProvider::setGetCertificateFn({});
}

// HSM-KM-BRIDGE-03: callback exceptions fail closed
TEST_F(HSMProviderTest, KeyManagementCallbackExceptionsFailClosed) {
    HsmProviderEnvGuard allow_guard("THEMIS_ALLOW_HSM_STUB", "1");
    HsmProviderEnvUnsetGuard prod_guard("THEMIS_PRODUCTION_MODE");

    HSMConfig config;
    config.library_path = "";
    HSMProvider hsm(config);
    ASSERT_TRUE(hsm.initialize());

    HSMProvider::setGenerateKeyPairFn(
        [](const std::string&, uint32_t, bool) -> bool {
            throw std::runtime_error("generate failed");
        });
    HSMProvider::setImportCertificateFn(
        [](const std::string&, const std::string&) -> bool {
            throw std::runtime_error("import failed");
        });
    HSMProvider::setGetCertificateFn(
        [](const std::string&) -> std::optional<std::string> {
            throw std::runtime_error("get cert failed");
        });

    EXPECT_FALSE(hsm.generateKeyPair("k", 2048, false));
    EXPECT_FALSE(hsm.importCertificate("k", "pem"));
    EXPECT_FALSE(hsm.getCertificate("k").has_value());

    HSMProvider::setGenerateKeyPairFn({});
    HSMProvider::setImportCertificateFn({});
    HSMProvider::setGetCertificateFn({});
}
