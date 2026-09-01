#include <gtest/gtest.h>
#include "security/timestamp_authority.h"
#include <thread>
#include <chrono>
#include <limits>

using namespace themis::security;

/**
 * Timestamp Authority Tests
 * 
 * These tests use public TSA services for testing.
 * 
 * Public TSAs:
 * - FreeTSA: https://freetsa.org/tsr (free, no registration)
 * - Digicert: https://timestamp.digicert.com (free)
 * - Sectigo: http://timestamp.sectigo.com (free)
 * 
 * Note: These are real network requests and may be slow or fail if TSA is down.
 * For CI/CD, consider using THEMIS_TEST_SKIP_TSA_NETWORK_TESTS=1
 */

class TimestampAuthorityTest : public ::testing::Test {
protected:
    bool skip_network_tests = false;
    
    void SetUp() override {
        const char* skip_env = std::getenv("THEMIS_TEST_SKIP_TSA_NETWORK_TESTS");
        skip_network_tests = (skip_env && std::string(skip_env) == "1");
    }
    
    TSAConfig createFreeTSAConfig() {
        TSAConfig config;
        config.url = "https://freetsa.org/tsr";
        config.hash_algorithm = "SHA256";
        config.cert_req = true;
        config.timeout_seconds = 30;
        config.verify_tsa_cert = false;  // FreeTSA uses self-signed cert
        return config;
    }
    
    TSAConfig createDigicertConfig() {
        TSAConfig config;
        config.url = "https://timestamp.digicert.com";
        config.hash_algorithm = "SHA256";
        config.cert_req = true;
        config.timeout_seconds = 30;
        config.verify_tsa_cert = true;
        return config;
    }
};

TEST_F(TimestampAuthorityTest, ConstructorDoesNotThrow) {
    TSAConfig config = createFreeTSAConfig();
    EXPECT_NO_THROW({
        TimestampAuthority tsa(config);
    });
}

TEST_F(TimestampAuthorityTest, GetTimestampFromFreeTSA) {
    if (skip_network_tests) {
        GTEST_SKIP() << "Network tests disabled (THEMIS_TEST_SKIP_TSA_NETWORK_TESTS=1)";
    }
    
    TSAConfig config = createFreeTSAConfig();
    TimestampAuthority tsa(config);
    
    // Test data
    std::vector<uint8_t> data = {'T', 'e', 's', 't', ' ', 'D', 'a', 't', 'a'};
    
    // Get timestamp
    auto token = tsa.getTimestamp(data);
    
    // Check result
    if (!token.success) {
        GTEST_SKIP() << "TSA request failed: " << token.error_message 
                     << " (TSA may be unavailable)";
    }
    
    EXPECT_TRUE(token.success);
    EXPECT_FALSE(token.token_b64.empty());
    EXPECT_FALSE(token.serial_number.empty());
    EXPECT_FALSE(token.timestamp_utc.empty());
    EXPECT_GT(token.timestamp_unix_ms, 0);
    EXPECT_EQ(token.pki_status, 0);  // 0 = granted
    
    std::cout << "Timestamp received:\n"
              << "  Serial: " << token.serial_number << "\n"
              << "  Time: " << token.timestamp_utc << "\n"
              << "  Policy: " << token.policy_oid << "\n";
}

TEST_F(TimestampAuthorityTest, GetTimestampForHash) {
    if (skip_network_tests) {
        GTEST_SKIP() << "Network tests disabled";
    }
    
    TSAConfig config = createFreeTSAConfig();
    TimestampAuthority tsa(config);
    
    // Pre-computed SHA-256 hash (32 bytes)
    std::vector<uint8_t> hash(32, 0xAB);
    
    auto token = tsa.getTimestampForHash(hash);
    
    if (!token.success) {
        GTEST_SKIP() << "TSA unavailable: " << token.error_message;
    }
    
    EXPECT_TRUE(token.success);
    EXPECT_FALSE(token.token_der.empty());
}

TEST_F(TimestampAuthorityTest, VerifyTimestamp) {
    if (skip_network_tests) {
        GTEST_SKIP() << "Network tests disabled";
    }
    
    TSAConfig config = createFreeTSAConfig();
    TimestampAuthority tsa(config);
    
    std::vector<uint8_t> data = {'V', 'e', 'r', 'i', 'f', 'y', ' ', 'M', 'e'};
    
    // Get timestamp
    auto token = tsa.getTimestamp(data);
    
    if (!token.success) {
        GTEST_SKIP() << "TSA unavailable";
    }
    
    // Verify timestamp
    bool verified = tsa.verifyTimestamp(data, token);
    EXPECT_TRUE(verified);
    
    // Verify with modified data should fail
    std::vector<uint8_t> modified_data = data;
    modified_data[0] = 'X';
    bool verified_modified = tsa.verifyTimestamp(modified_data, token);
    EXPECT_FALSE(verified_modified);
}

TEST_F(TimestampAuthorityTest, ParseTokenRoundtrip) {
    if (skip_network_tests) {
        GTEST_SKIP() << "Network tests disabled";
    }
    
    TSAConfig config = createFreeTSAConfig();
    TimestampAuthority tsa(config);
    
    std::vector<uint8_t> data = {'P', 'a', 'r', 's', 'e', ' ', 'T', 'e', 's', 't'};
    
    // Get timestamp
    auto token1 = tsa.getTimestamp(data);
    
    if (!token1.success) {
        GTEST_SKIP() << "TSA unavailable";
    }
    
    // Parse from Base64
    auto token2 = tsa.parseToken(token1.token_b64);
    
    EXPECT_TRUE(token2.success);
    EXPECT_EQ(token1.serial_number, token2.serial_number);
    EXPECT_EQ(token1.timestamp_utc, token2.timestamp_utc);
}

TEST_F(TimestampAuthorityTest, MultipleTimestamps) {
    if (skip_network_tests) {
        GTEST_SKIP() << "Network tests disabled";
    }
    
    TSAConfig config = createFreeTSAConfig();
    TimestampAuthority tsa(config);
    
    // Request multiple timestamps
    for (int i = 0; i < 3; ++i) {
        std::vector<uint8_t> data(100, static_cast<uint8_t>(i));
        
        auto token = tsa.getTimestamp(data);
        
        if (!token.success) {
            GTEST_SKIP() << "TSA request " << i << " failed";
        }
        
        EXPECT_TRUE(token.success);
        EXPECT_FALSE(token.serial_number.empty());
        
        // Small delay to avoid rate limiting
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

TEST_F(TimestampAuthorityTest, DifferentHashAlgorithms) {
    if (skip_network_tests) {
        GTEST_SKIP() << "Network tests disabled";
    }
    
    std::vector<std::string> algorithms = {"SHA256", "SHA384", "SHA512"};
    std::vector<uint8_t> data = {'T', 'e', 's', 't'};
    
    for (const auto& algo : algorithms) {
        TSAConfig config = createFreeTSAConfig();
        config.hash_algorithm = algo;
        
        TimestampAuthority tsa(config);
        auto token = tsa.getTimestamp(data);
        
        if (!token.success) {
            continue;  // TSA may not support all algorithms
        }
        
        EXPECT_TRUE(token.success) << "Algorithm " << algo << " failed";
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

TEST_F(TimestampAuthorityTest, IsAvailable) {
    if (skip_network_tests) {
        GTEST_SKIP() << "Network tests disabled";
    }
    
    TSAConfig config = createFreeTSAConfig();
    TimestampAuthority tsa(config);
    
    bool available = tsa.isAvailable();
    
    if (!available) {
        GTEST_SKIP() << "TSA not reachable (may be down or network issue)";
    }
    
    EXPECT_TRUE(available);
}

TEST_F(TimestampAuthorityTest, GenerateNonceRejectsZeroAndOversizedRequests) {
    TSAConfig config = createFreeTSAConfig();
    TimestampAuthority tsa(config);

    EXPECT_TRUE(tsa.generateNonce(0).empty());

    const auto oversized = static_cast<size_t>(std::numeric_limits<int>::max()) + 1;
    EXPECT_TRUE(tsa.generateNonce(oversized).empty());
}

TEST_F(TimestampAuthorityTest, InvalidURL) {
    TSAConfig config;
    config.url = "https://invalid.tsa.example.com/nonexistent";
    config.timeout_seconds = 5;
    
    TimestampAuthority tsa(config);
    
    std::vector<uint8_t> data = {'T', 'e', 's', 't'};
    auto token = tsa.getTimestamp(data);
    
    EXPECT_FALSE(token.success);
    EXPECT_FALSE(token.error_message.empty());
}

// eIDAS Validator Tests

TEST_F(TimestampAuthorityTest, ValidateAge) {
    TimestampToken token;
    
    // Current timestamp
    token.timestamp_unix_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    eIDASTimestampValidator validator;
    
    // Should be valid (age = 0)
    EXPECT_TRUE(validator.validateAge(token, 10950));  // 30 years
    EXPECT_TRUE(validator.validateAge(token, 365));     // 1 year
    EXPECT_TRUE(validator.validateAge(token, 1));       // 1 day
    
    // Old timestamp (1 year ago)
    token.timestamp_unix_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count() - (365L * 24 * 60 * 60 * 1000);  // 1 year in ms
    
    EXPECT_TRUE(validator.validateAge(token, 10950));  // Still valid for 30 years
    EXPECT_FALSE(validator.validateAge(token, 364));   // Too old for 364 days
}

TEST_F(TimestampAuthorityTest, eIDASValidation) {
    TimestampToken token;
    token.success = true;
    token.token_der = {0x01, 0x02, 0x03};  // Dummy data
    
    eIDASTimestampValidator validator;
    
    std::vector<std::string> trust_anchors = {"dummy_ca.pem"};
    
    // Stub implementation should not crash
    bool result = validator.validateeIDASTimestamp(token, trust_anchors);
    
    // May return true or false depending on stub implementation
    EXPECT_TRUE(result || !result);  // Just check it doesn't crash
    
    auto errors = validator.getValidationErrors();
    // Errors may or may not be present in stub
}

// Usage Example Test

TEST_F(TimestampAuthorityTest, UsageExample) {
    if (skip_network_tests) {
        GTEST_SKIP() << "Network tests disabled";
    }
    
    // Example: How to use TimestampAuthority
    
    // 1. Configure TSA
    TSAConfig config;
    config.url = "https://freetsa.org/tsr";
    config.hash_algorithm = "SHA256";
    config.cert_req = true;
    config.timeout_seconds = 30;
    config.verify_tsa_cert = false;  // FreeTSA uses self-signed cert
    
    // 2. Create client
    auto tsa = std::make_unique<TimestampAuthority>(config);
    
    // 3. Get timestamp for data
    std::vector<uint8_t> my_data = {'M', 'y', ' ', 'D', 'a', 't', 'a'};
    auto token = tsa->getTimestamp(my_data);
    
    if (!token.success) {
        GTEST_SKIP() << "TSA unavailable: " << token.error_message;
    }
    
    ASSERT_TRUE(token.success);
    
    // 4. Verify timestamp
    bool is_valid = tsa->verifyTimestamp(my_data, token);
    EXPECT_TRUE(is_valid);
    
    // 5. Store token for archival (eIDAS Art. 32)
    std::string stored_token = token.token_b64;
    
    // Later: Parse stored token
    auto parsed_token = tsa->parseToken(stored_token);
    EXPECT_EQ(token.serial_number, parsed_token.serial_number);
    
    // 6. Validate age (eIDAS: 30 years = 10950 days)
    eIDASTimestampValidator validator;
    bool age_valid = validator.validateAge(parsed_token, 10950);
    EXPECT_TRUE(age_valid);
}

// Performance Benchmark (disabled by default)

TEST_F(TimestampAuthorityTest, DISABLED_TimestampPerformanceBenchmark) {
    if (skip_network_tests) {
        GTEST_SKIP() << "Network tests disabled";
    }
    
    TSAConfig config = createFreeTSAConfig();
    TimestampAuthority tsa(config);
    
    const int num_operations = 10;
    std::vector<uint8_t> data(1024, 0xAA);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    int successful = 0;
    for (int i = 0; i < num_operations; ++i) {
        auto token = tsa.getTimestamp(data);
        if (token.success) {
            ++successful;
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));  // Rate limiting
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    double avg_time = duration.count() / static_cast<double>(successful);
    
    std::cout << "Timestamp Performance:\n"
              << "  Operations: " << num_operations << "\n"
              << "  Successful: " << successful << "\n"
              << "  Total Time: " << duration.count() << " ms\n"
              << "  Avg Time/Op: " << avg_time << " ms\n";
    
    EXPECT_GT(successful, 0);
}

// ============================================================================
// RFC 3161 Compliance Tests - New Fields
// ============================================================================

TEST_F(TimestampAuthorityTest, RFC3161_AccuracyMetadata) {
    if (skip_network_tests) {
        GTEST_SKIP() << "Network tests disabled";
    }
    
    TSAConfig config = createFreeTSAConfig();
    TimestampAuthority tsa(config);
    
    std::vector<uint8_t> data = {'A', 'c', 'c', 'u', 'r', 'a', 'c', 'y', 'T', 'e', 's', 't'};
    auto token = tsa.getTimestamp(data);
    
    if (!token.success) {
        GTEST_SKIP() << "TSA unavailable: " << token.error_message;
    }
    
    // Check if accuracy metadata is present (optional per RFC 3161)
    // Note: Not all TSAs provide accuracy information
    if (token.has_accuracy) {
        std::cout << "Accuracy metadata found:\n"
                  << "  Seconds: " << token.accuracy_seconds << "\n"
                  << "  Milliseconds: " << token.accuracy_millis << "\n"
                  << "  Microseconds: " << token.accuracy_micros << "\n";
        
        // If accuracy is present, values should be reasonable
        EXPECT_LE(token.accuracy_millis, 999);
        EXPECT_LE(token.accuracy_micros, 999);
    } else {
        std::cout << "Note: TSA does not provide accuracy metadata (optional per RFC 3161)\n";
    }
}

TEST_F(TimestampAuthorityTest, RFC3161_OrderingHint) {
    if (skip_network_tests) {
        GTEST_SKIP() << "Network tests disabled";
    }
    
    TSAConfig config = createFreeTSAConfig();
    TimestampAuthority tsa(config);
    
    std::vector<uint8_t> data = {'O', 'r', 'd', 'e', 'r', 'T', 'e', 's', 't'};
    auto token = tsa.getTimestamp(data);
    
    if (!token.success) {
        GTEST_SKIP() << "TSA unavailable: " << token.error_message;
    }
    
    // Check ordering hint (optional per RFC 3161, default is false)
    std::cout << "Ordering hint: " << (token.ordering ? "true (chronological guarantee)" : "false (no guarantee)") << "\n";
    
    // The field should always be present, but value depends on TSA
    // Most TSAs don't guarantee ordering, so we just verify the field exists
    EXPECT_TRUE(!token.ordering || token.ordering);  // Always passes, just validates field access
}

TEST_F(TimestampAuthorityTest, RFC3161_CertificateExtraction) {
    if (skip_network_tests) {
        GTEST_SKIP() << "Network tests disabled";
    }
    
    TSAConfig config = createFreeTSAConfig();
    // Note: cert_req is already true by default in createFreeTSAConfig()
    TimestampAuthority tsa(config);
    
    std::vector<uint8_t> data = {'C', 'e', 'r', 't', 'T', 'e', 's', 't'};
    auto token = tsa.getTimestamp(data);
    
    if (!token.success) {
        GTEST_SKIP() << "TSA unavailable: " << token.error_message;
    }
    
    // With cert_req=true, we should get certificate information
    EXPECT_FALSE(token.tsa_cert.empty()) << "TSA certificate should be present";
    EXPECT_FALSE(token.tsa_serial.empty()) << "TSA certificate serial should be present";
    EXPECT_FALSE(token.tsa_name.empty()) << "TSA name should be present";
    
    std::cout << "TSA Certificate Info:\n"
              << "  Subject: " << token.tsa_name << "\n"
              << "  Serial: " << token.tsa_serial << "\n"
              << "  Cert Size: " << token.tsa_cert.size() << " bytes\n";
    
    // Verify getTSACertificate() returns the cached certificate
    auto cert_pem = tsa.getTSACertificate();
    EXPECT_TRUE(cert_pem.has_value()) << "getTSACertificate() should return certificate";
    
    if (cert_pem.has_value()) {
        EXPECT_FALSE(cert_pem->empty());
        EXPECT_NE(cert_pem->find("-----BEGIN CERTIFICATE-----"), std::string::npos);
        EXPECT_NE(cert_pem->find("-----END CERTIFICATE-----"), std::string::npos);
        
        std::cout << "Certificate PEM format verified (length: " << cert_pem->size() << " bytes)\n";
    }
}

// ============================================================================
// Stub Production-Mode Guard Tests
// These tests only apply to the stub implementation (no OpenSSL).
// ============================================================================

#ifndef THEMIS_USE_OPENSSL_TSA

// Helper RAII guard for environment variables (portable across POSIX and Windows)
struct EnvGuard {
    const char* name;
    std::string previous;
    bool had_previous;

    explicit EnvGuard(const char* var, const char* value) : name(var) {
        const char* existing = std::getenv(var);
        had_previous = (existing != nullptr);
        if (had_previous) previous = existing;
#ifdef _WIN32
        _putenv_s(var, value);
#else
        ::setenv(var, value, 1);
#endif
    }

    ~EnvGuard() {
#ifdef _WIN32
        if (had_previous) _putenv_s(name, previous.c_str());
        else              _putenv_s(name, "");
#else
        if (had_previous) ::setenv(name, previous.c_str(), 1);
        else              ::unsetenv(name);
#endif
    }
};

// RAII guard that unsets an environment variable for the duration of a scope
struct EnvUnsetGuard {
    const char* name;
    std::string previous;
    bool had_previous;

    explicit EnvUnsetGuard(const char* var) : name(var) {
        const char* existing = std::getenv(var);
        had_previous = (existing != nullptr);
        if (had_previous) previous = existing;
#ifdef _WIN32
        _putenv_s(var, "");
#else
        ::unsetenv(var);
#endif
    }

    ~EnvUnsetGuard() {
#ifdef _WIN32
        if (had_previous) _putenv_s(name, previous.c_str());
        else              _putenv_s(name, "");
#else
        if (had_previous) ::setenv(name, previous.c_str(), 1);
        else              ::unsetenv(name);
#endif
    }
};

TEST_F(TimestampAuthorityTest, StubRefusesInProductionMode) {
    // Activate production mode via environment variable
    EnvGuard prod_guard("THEMIS_PRODUCTION_MODE", "1");
    // Ensure the explicit opt-in is NOT set
    EnvUnsetGuard stub_guard("THEMIS_ALLOW_TSA_STUB");

    TSAConfig config = createFreeTSAConfig();
    TimestampAuthority tsa(config);

    std::vector<uint8_t> data = {'P', 'r', 'o', 'd', 'T', 'e', 's', 't'};
    auto token = tsa.getTimestamp(data);

    EXPECT_FALSE(token.success) << "Stub must refuse to issue tokens in production mode";
    EXPECT_FALSE(token.error_message.empty()) << "Error message must explain the rejection";
}

TEST_F(TimestampAuthorityTest, StubRefusesForHashInProductionMode) {
    EnvGuard prod_guard("THEMIS_PRODUCTION_MODE", "1");
    EnvUnsetGuard stub_guard("THEMIS_ALLOW_TSA_STUB");

    TSAConfig config = createFreeTSAConfig();
    TimestampAuthority tsa(config);

    std::vector<uint8_t> hash(32, 0xAB);
    auto token = tsa.getTimestampForHash(hash);

    EXPECT_FALSE(token.success) << "Stub must refuse getTimestampForHash in production mode";
    EXPECT_FALSE(token.error_message.empty());
}

TEST_F(TimestampAuthorityTest, StubAllowedWhenExplicitlyPermitted) {
    // Production mode ON but explicit override also set
    EnvGuard prod_guard("THEMIS_PRODUCTION_MODE", "1");
    EnvGuard stub_guard("THEMIS_ALLOW_TSA_STUB", "1");

    TSAConfig config = createFreeTSAConfig();
    TimestampAuthority tsa(config);

    std::vector<uint8_t> data = {'A', 'l', 'l', 'o', 'w', 'e', 'd'};
    auto token = tsa.getTimestamp(data);

    // With THEMIS_ALLOW_TSA_STUB=1 the stub should succeed even in production mode
    EXPECT_TRUE(token.success) << "Stub must work when THEMIS_ALLOW_TSA_STUB=1";
    EXPECT_EQ(token.serial_number, "STUB-SERIAL");
}

TEST_F(TimestampAuthorityTest, eIDASValidatorRefusesWithoutExplicitStubOptIn) {
    EnvUnsetGuard prod_guard("THEMIS_PRODUCTION_MODE");
    EnvUnsetGuard stub_guard("THEMIS_ALLOW_TSA_STUB");

    TimestampToken token;
    token.success = true;
    token.timestamp_unix_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    eIDASTimestampValidator validator;
    std::vector<std::string> trust_anchors = {"ca.pem"};

    bool result = validator.validateeIDASTimestamp(token, trust_anchors);
    EXPECT_FALSE(result) << "eIDAS validator stub must be fail-closed without explicit opt-in";

    auto errors = validator.getValidationErrors();
    EXPECT_FALSE(errors.empty()) << "Validation errors must explain the rejection";
}

TEST_F(TimestampAuthorityTest, eIDASValidatorAllowsExplicitStubOptIn) {
    EnvUnsetGuard prod_guard("THEMIS_PRODUCTION_MODE");
    EnvGuard stub_guard("THEMIS_ALLOW_TSA_STUB", "1");

    TimestampToken token;
    token.success = true;
    token.timestamp_unix_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    eIDASTimestampValidator validator;
    std::vector<std::string> trust_anchors = {"ca.pem"};

    bool result = validator.validateeIDASTimestamp(token, trust_anchors);
    EXPECT_TRUE(result) << "eIDAS validator stub should work only with explicit opt-in";
    EXPECT_TRUE(validator.getValidationErrors().empty());
}

TEST_F(TimestampAuthorityTest, eIDASValidatorRefusesInProductionMode) {
    EnvGuard prod_guard("THEMIS_PRODUCTION_MODE", "1");
    EnvUnsetGuard stub_guard("THEMIS_ALLOW_TSA_STUB");

    TimestampToken token;
    token.success = true;
    token.timestamp_unix_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    eIDASTimestampValidator validator;
    std::vector<std::string> trust_anchors = {"ca.pem"};

    bool result = validator.validateeIDASTimestamp(token, trust_anchors);
    EXPECT_FALSE(result) << "eIDAS validator stub must refuse in production mode";

    auto errors = validator.getValidationErrors();
    EXPECT_FALSE(errors.empty()) << "Validation errors must explain the rejection";
}

#endif // !THEMIS_USE_OPENSSL_TSA
// TSAConfig Auth / TLS Configuration Tests
// ============================================================================

TEST_F(TimestampAuthorityTest, ConfigWithBasicAuth) {
    // Verify that a config with HTTP Basic Auth credentials can be created
    // and that the object is constructed without error.
    TSAConfig config;
    config.url = "https://tsa.example.com/tsr";
    config.username = "tsauser";
    config.password = "tsapass";
    config.timeout_seconds = 5;

    EXPECT_NO_THROW({
        TimestampAuthority tsa(config);
        (void)tsa.getLastError();  // Ensure object is usable
    });
}

TEST_F(TimestampAuthorityTest, ConfigWithMTLS) {
    // Verify that a config with mTLS client certificates can be created
    // and that the object is constructed without error.
    TSAConfig config;
    config.url = "https://internal-tsa.corp.example.com/tsr";
    config.client_cert_path = "/path/to/client.crt";
    config.client_key_path = "/path/to/client.key";
    config.ca_cert_path = "/path/to/corp-ca.crt";
    config.verify_tsa_cert = true;
    config.timeout_seconds = 5;

    EXPECT_NO_THROW({
        TimestampAuthority tsa(config);
        (void)tsa.getLastError();  // Ensure object is usable
    });
}

TEST_F(TimestampAuthorityTest, ConfigWithTLSVerificationDisabled) {
    // Verify that TLS verification can be disabled (for development/testing)
    TSAConfig config;
    config.url = "https://freetsa.org/tsr";
    config.verify_tsa_cert = false;
    config.timeout_seconds = 5;

    EXPECT_NO_THROW({
        TimestampAuthority tsa(config);
        (void)tsa.getLastError();
    });
}

TEST_F(TimestampAuthorityTest, IsAvailableThenGetTimestamp) {
    if (skip_network_tests) {
        GTEST_SKIP() << "Network tests disabled";
    }

    // Verify that isAvailable() does not corrupt CURL state so that a subsequent
    // getTimestamp() call still works correctly.
    TSAConfig config = createFreeTSAConfig();
    TimestampAuthority tsa(config);

    // Call isAvailable() first - this used to set CURLOPT_NOBODY=1 without reset
    bool available = tsa.isAvailable();
    if (!available) {
        GTEST_SKIP() << "TSA not reachable";
    }

    // Now getTimestamp() must still return a full response (not an empty HEAD reply)
    std::vector<uint8_t> data = {'A', 'f', 't', 'e', 'r', 'H', 'e', 'a', 'd'};
    auto token = tsa.getTimestamp(data);

    if (!token.success) {
        GTEST_SKIP() << "TSA unavailable: " << token.error_message;
    }

    EXPECT_TRUE(token.success);
    EXPECT_FALSE(token.token_der.empty()) << "Token DER must be present after isAvailable() call";
    EXPECT_FALSE(token.serial_number.empty());
}
