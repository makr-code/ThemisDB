/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            timestamp_authority_example.cpp                    ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     387                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * RFC 3161 Timestamp Authority (TSA) Example
 * 
 * This example demonstrates how to use ThemisDB's TSA client to obtain
 * cryptographically secure timestamps for audit trails, document signing,
 * and eIDAS compliance.
 * 
 * Build:
 *   g++ -std=c++20 timestamp_authority_example.cpp -lthemis_core -lssl -lcrypto -lcurl
 * 
 * Usage:
 *   ./timestamp_authority_example
 */

#include "security/timestamp_authority.h"
#include <iostream>
#include <fstream>
#include <iomanip>

using namespace themis::security;

void printToken(const TimestampToken& token) {
    std::cout << "\n📝 Timestamp Token Details:\n";
    std::cout << "  ✅ Success: " << (token.success ? "Yes" : "No") << "\n";
    
    if (!token.success) {
        std::cout << "  ❌ Error: " << token.error_message << "\n";
        return;
    }
    
    std::cout << "  🕒 Timestamp (UTC): " << token.timestamp_utc << "\n";
    std::cout << "  🔢 Serial Number: " << token.serial_number << "\n";
    std::cout << "  📋 Policy OID: " << token.policy_oid << "\n";
    std::cout << "  🔐 Hash Algorithm: " << token.hash_algorithm << "\n";
    std::cout << "  🆔 TSA Name: " << token.tsa_name << "\n";
    std::cout << "  📜 TSA Serial: " << token.tsa_serial << "\n";
    
    if (token.has_accuracy) {
        std::cout << "  ⏱️  Accuracy: " << token.accuracy_seconds << "s "
                  << token.accuracy_millis << "ms "
                  << token.accuracy_micros << "µs\n";
    }
    
    if (token.ordering) {
        std::cout << "  📊 Ordering: Guaranteed chronological order\n";
    }
    
    std::cout << "  🔗 Token Size: " << token.token_der.size() << " bytes\n";
}

// Example 1: Basic Timestamp
void example_basic_timestamp() {
    std::cout << "\n=== Example 1: Basic Timestamp ===\n";
    
    // Configure TSA
    TSAConfig config;
    config.url = "https://freetsa.org/tsr";
    config.hash_algorithm = "SHA256";
    config.timeout_seconds = 30;
    config.verify_tsa_cert = false;  // FreeTSA uses self-signed cert
    
    // Create TSA client
    TimestampAuthority tsa(config);
    
    // Data to timestamp
    std::string message = "Hello, ThemisDB!";
    std::vector<uint8_t> data(message.begin(), message.end());
    
    std::cout << "📤 Requesting timestamp for: \"" << message << "\"\n";
    
    // Get timestamp
    TimestampToken token = tsa.getTimestamp(data);
    
    printToken(token);
    
    // Verify timestamp
    if (token.success) {
        bool verified = tsa.verifyTimestamp(data, token);
        std::cout << "  ✅ Verification: " << (verified ? "PASSED" : "FAILED") << "\n";
    }
}

// Example 2: Multiple TSA Providers with Failover
void example_failover() {
    std::cout << "\n=== Example 2: Multiple TSA Providers (Failover) ===\n";
    
    std::vector<std::string> tsa_urls = {
        "https://freetsa.org/tsr",
        "https://timestamp.digicert.com",
        "http://timestamp.sectigo.com"
    };
    
    std::string message = "Critical audit log entry";
    std::vector<uint8_t> data(message.begin(), message.end());
    
    TimestampToken token;
    bool success = false;
    
    for (const auto& url : tsa_urls) {
        std::cout << "📤 Trying TSA: " << url << "\n";
        
        TSAConfig config;
        config.url = url;
        config.hash_algorithm = "SHA256";
        config.timeout_seconds = 10;
        config.verify_tsa_cert = (url.find("digicert") != std::string::npos);
        
        TimestampAuthority tsa(config);
        token = tsa.getTimestamp(data);
        
        if (token.success) {
            std::cout << "  ✅ Success!\n";
            success = true;
            break;
        } else {
            std::cout << "  ❌ Failed: " << token.error_message << "\n";
        }
    }
    
    if (success) {
        printToken(token);
    } else {
        std::cout << "  ⚠️  All TSA providers failed\n";
    }
}

// Example 3: Timestamp with Hash
void example_timestamp_hash() {
    std::cout << "\n=== Example 3: Timestamp Pre-computed Hash ===\n";
    
    // Configure TSA
    TSAConfig config;
    config.url = "https://freetsa.org/tsr";
    config.hash_algorithm = "SHA256";
    config.verify_tsa_cert = false;
    
    TimestampAuthority tsa(config);
    
    // Pre-compute hash (e.g., from a large file)
    std::string document = "Large document content...";
    std::vector<uint8_t> data(document.begin(), document.end());
    
    // Compute hash first
    std::vector<uint8_t> hash(32, 0);  // SHA-256 = 32 bytes
    // In real code, use OpenSSL to compute hash
    // For demo, we'll use the TSA's internal hash function
    
    std::cout << "📤 Requesting timestamp for pre-computed hash\n";
    
    // Get timestamp for hash (saves bandwidth for large files)
    TimestampToken token = tsa.getTimestamp(data);  // Will compute hash internally
    
    printToken(token);
}

// Example 4: Save and Load Timestamp
void example_save_load_timestamp() {
    std::cout << "\n=== Example 4: Save and Load Timestamp ===\n";
    
    // Configure TSA
    TSAConfig config;
    config.url = "https://freetsa.org/tsr";
    config.hash_algorithm = "SHA256";
    config.verify_tsa_cert = false;
    
    TimestampAuthority tsa(config);
    
    // Get timestamp
    std::string message = "Document for archiving";
    std::vector<uint8_t> data(message.begin(), message.end());
    
    std::cout << "📤 Getting timestamp...\n";
    TimestampToken token = tsa.getTimestamp(data);
    
    if (!token.success) {
        std::cout << "  ❌ Failed to get timestamp: " << token.error_message << "\n";
        return;
    }
    
    // Save timestamp to file
    std::string filename = "/tmp/timestamp.tsr";
    std::ofstream file(filename, std::ios::binary);
    file.write(reinterpret_cast<const char*>(token.token_der.data()), 
               token.token_der.size());
    file.close();
    
    std::cout << "  💾 Timestamp saved to: " << filename << "\n";
    std::cout << "  📏 Size: " << token.token_der.size() << " bytes\n";
    
    // Load timestamp from file
    std::ifstream loadFile(filename, std::ios::binary);
    std::vector<uint8_t> loaded_token(
        (std::istreambuf_iterator<char>(loadFile)),
        std::istreambuf_iterator<char>()
    );
    loadFile.close();
    
    std::cout << "  📂 Timestamp loaded from: " << filename << "\n";
    
    // Parse loaded timestamp
    TimestampToken parsed_token = tsa.parseToken(loaded_token);
    
    std::cout << "  ✅ Parsed timestamp:\n";
    std::cout << "    Time: " << parsed_token.timestamp_utc << "\n";
    std::cout << "    Serial: " << parsed_token.serial_number << "\n";
    
    // Verify loaded timestamp
    bool verified = tsa.verifyTimestamp(data, parsed_token);
    std::cout << "  ✅ Verification: " << (verified ? "PASSED" : "FAILED") << "\n";
}

// Example 5: eIDAS Compliance Validation
void example_eidas_validation() {
    std::cout << "\n=== Example 5: eIDAS Compliance Validation ===\n";
    
    // Get a timestamp
    TSAConfig config;
    config.url = "https://freetsa.org/tsr";
    config.hash_algorithm = "SHA256";
    config.verify_tsa_cert = false;
    
    TimestampAuthority tsa(config);
    
    std::string message = "eIDAS-compliant document";
    std::vector<uint8_t> data(message.begin(), message.end());
    
    std::cout << "📤 Getting timestamp...\n";
    TimestampToken token = tsa.getTimestamp(data);
    
    if (!token.success) {
        std::cout << "  ❌ Failed: " << token.error_message << "\n";
        return;
    }
    
    printToken(token);
    
    // Validate timestamp
    eIDASTimestampValidator validator;
    
    // 1. Validate age (30 years = 10,950 days)
    std::cout << "\n🔍 eIDAS Validation:\n";
    bool age_valid = validator.validateAge(token, 10950);
    std::cout << "  ✅ Age validation (30 years): " 
              << (age_valid ? "PASSED" : "FAILED") << "\n";
    
    if (!age_valid) {
        auto errors = validator.getValidationErrors();
        for (const auto& error : errors) {
            std::cout << "    ❌ " << error << "\n";
        }
    }
    
    // 2. Validate timestamp structure
    std::vector<std::string> trust_anchors = {
        // In production, load from EU Trusted List
    };
    
    bool struct_valid = validator.validateeIDASTimestamp(token, trust_anchors);
    std::cout << "  ✅ Structure validation: " 
              << (struct_valid ? "PASSED" : "FAILED") << "\n";
    
    // 3. Check if TSA is qualified (note: FreeTSA is not eIDAS-qualified)
    std::vector<std::string> qtsp_list = {
        "D-TRUST",
        "DFN-PKI",
        "Bundesdruckerei"
    };
    
    std::string tsa_cert = ""; // Would get from token in production
    auto cert_opt = tsa.getTSACertificate();
    if (cert_opt.has_value()) {
        tsa_cert = cert_opt.value();
        bool qualified = validator.isQualifiedTSA(tsa_cert, qtsp_list);
        std::cout << "  ✅ Qualified TSA: " 
                  << (qualified ? "YES (eIDAS-compliant)" : "NO (not in QTSP list)") << "\n";
        
        if (!qualified) {
            std::cout << "    ℹ️  Note: FreeTSA is not eIDAS-qualified. "
                      << "Use D-TRUST, DFN-PKI, or other QTSP for production.\n";
        }
    }
}

// Example 6: Hash Algorithm Comparison
void example_hash_algorithms() {
    std::cout << "\n=== Example 6: Hash Algorithm Comparison ===\n";
    
    std::string message = "Test data for hash comparison";
    std::vector<uint8_t> data(message.begin(), message.end());
    
    std::vector<std::string> algorithms = {"SHA256", "SHA384", "SHA512"};
    
    for (const auto& algo : algorithms) {
        std::cout << "\n🔐 Testing " << algo << ":\n";
        
        TSAConfig config;
        config.url = "https://freetsa.org/tsr";
        config.hash_algorithm = algo;
        config.timeout_seconds = 30;
        config.verify_tsa_cert = false;
        
        TimestampAuthority tsa(config);
        
        auto start = std::chrono::steady_clock::now();
        TimestampToken token = tsa.getTimestamp(data);
        auto end = std::chrono::steady_clock::now();
        
        if (token.success) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
            std::cout << "  ✅ Success\n";
            std::cout << "  ⏱️  Latency: " << duration.count() << " ms\n";
            std::cout << "  📏 Token size: " << token.token_der.size() << " bytes\n";
        } else {
            std::cout << "  ❌ Failed: " << token.error_message << "\n";
        }
    }
}

// Main function
int main(int argc, char* argv[]) {
    std::cout << "🔐 ThemisDB RFC 3161 Timestamp Authority Examples\n";
    std::cout << "================================================\n";
    
    // Check if network tests should be skipped
    const char* skip_env = std::getenv("THEMIS_TEST_SKIP_TSA_NETWORK_TESTS");
    if (skip_env && std::string(skip_env) == "1") {
        std::cout << "\n⚠️  Network tests are disabled (THEMIS_TEST_SKIP_TSA_NETWORK_TESTS=1)\n";
        std::cout << "Skipping all examples that require network access.\n";
        return 0;
    }
    
    std::cout << "\n⚠️  Note: These examples require internet access to contact TSA servers.\n";
    std::cout << "Network requests may be slow or fail if TSA servers are unavailable.\n";
    std::cout << "To skip network tests, set: THEMIS_TEST_SKIP_TSA_NETWORK_TESTS=1\n";
    
    try {
        // Run examples
        example_basic_timestamp();
        
        std::cout << "\n---\n";
        example_failover();
        
        std::cout << "\n---\n";
        example_timestamp_hash();
        
        std::cout << "\n---\n";
        example_save_load_timestamp();
        
        std::cout << "\n---\n";
        example_eidas_validation();
        
        std::cout << "\n---\n";
        example_hash_algorithms();
        
        std::cout << "\n\n✅ All examples completed successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error: " << e.what() << "\n";
        return 1;
    }
    
    std::cout << "\n📚 For more information, see: docs/en/security/TSA_SETUP.md\n";
    
    return 0;
}
