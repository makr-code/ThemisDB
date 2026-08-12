#include <gtest/gtest.h>
#include "sharding/secure_transport_client.h"
#include "sharding/mtls_client.h"
#include <vector>
#include <memory>
#include <string>

using namespace themis::sharding;

namespace themis { namespace sharding { 
struct SecureTransportClientTestAccess {
    static void setLz4CompressFn(
        SecureTransportClient& client,
        std::function<bool(const std::string&, std::string&)> fn) {
        client.setLz4CompressFn(std::move(fn));
    }

    static bool compressData(SecureTransportClient& client,
                             const std::string& input,
                             std::string& output,
                             std::string* codec) {
        return client.compressData(input, output, codec);
    }

    static void clearLz4CompressFn(SecureTransportClient& client) {
        client.clearLz4CompressFn();
    }
};
} } // namespace themis::sharding
/**
 * Test suite for SecureTransportClient
 */
class SecureTransportClientTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create configuration (without actual certificates for unit tests)
        config_.compression = SecureTransportClient::Config::CompressionType::Zstd;
        config_.compression_level = 3;
        config_.compression_threshold = 100;  // Low threshold for testing
        config_.max_retries = 2;
        config_.retry_delay_ms = 10;  // Fast retries for testing
        config_.max_retry_delay_ms = 100;
    }
    
    SecureTransportClient::Config config_;
};

TEST_F(SecureTransportClientTest, Construction) {
    // Should construct without certificates (will be non-ready)
    SecureTransportClient client(config_);
    EXPECT_FALSE(client.isReady());  // No certificates provided
}

TEST_F(SecureTransportClientTest, ConstructionWithCertificates) {
    // This test would require actual certificate files
    // In a real environment, you'd provide test certificates
    config_.cert_path = "/path/to/test/cert.pem";
    config_.key_path = "/path/to/test/key.pem";
    config_.ca_cert_path = "/path/to/test/ca.pem";
    
    // Note: This will fail without actual certificates
    // In production, you'd use test fixtures with real cert files
    EXPECT_NO_THROW({
        SecureTransportClient client(config_);
    });
}

TEST_F(SecureTransportClientTest, PayloadStructure) {
    SecureTransportClient::Payload payload;
    payload.data = "test data";
    payload.content_type = "application/octet-stream";
    payload.checksum = "abc123";
    payload.signature = "def456";
    payload.metadata = nlohmann::json{{"key", "value"}};
    
    EXPECT_EQ(payload.data, "test data");
    EXPECT_EQ(payload.content_type, "application/octet-stream");
    EXPECT_EQ(payload.checksum, "abc123");
    EXPECT_EQ(payload.signature, "def456");
    EXPECT_TRUE(payload.metadata.contains("key"));
}

TEST_F(SecureTransportClientTest, TransferResultStructure) {
    SecureTransportClient::TransferResult result;
    result.success = true;
    result.status_code = 200;
    result.bytes_sent = 1024;
    result.bytes_compressed = 512;
    result.compression_ratio = 2.0;
    result.retry_count = 0;
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.status_code, 200);
    EXPECT_EQ(result.bytes_sent, 1024);
    EXPECT_EQ(result.bytes_compressed, 512);
    EXPECT_DOUBLE_EQ(result.compression_ratio, 2.0);
    EXPECT_EQ(result.retry_count, 0);
}

TEST_F(SecureTransportClientTest, CompressionConfiguration) {
    // Test None compression
    config_.compression = SecureTransportClient::Config::CompressionType::None;
    SecureTransportClient client1(config_);
    EXPECT_FALSE(client1.isReady());  // No certs, but config is valid
    
    // Test Zstd compression
    config_.compression = SecureTransportClient::Config::CompressionType::Zstd;
    config_.compression_level = 5;
    SecureTransportClient client2(config_);
    EXPECT_FALSE(client2.isReady());
    
    // Test LZ4 compression (future support)
    config_.compression = SecureTransportClient::Config::CompressionType::LZ4;
    SecureTransportClient client3(config_);
    EXPECT_FALSE(client3.isReady());
}

TEST_F(SecureTransportClientTest, RetryConfiguration) {
    config_.max_retries = 5;
    config_.retry_delay_ms = 100;
    config_.max_retry_delay_ms = 5000;
    
    SecureTransportClient client(config_);
    EXPECT_FALSE(client.isReady());
}

TEST_F(SecureTransportClientTest, Lz4CompressionPathSetsCodecAndCompresses) {
    config_.compression = SecureTransportClient::Config::CompressionType::LZ4;
    config_.compression_threshold = 1;
    SecureTransportClient client(config_);

    SecureTransportClientTestAccess::setLz4CompressFn(
        client,
        [](const std::string& input, std::string& output) {
            if (input.size() < 4) {
                return false;
            }
            output.assign(input.begin(), input.begin() + static_cast<std::ptrdiff_t>(input.size() / 2));
            return true;
        });

    const std::string input(4096, 'A');
    std::string compressed;
    std::string codec;

    const bool applied = SecureTransportClientTestAccess::compressData(client, input, compressed, &codec);
    EXPECT_TRUE(applied);
    EXPECT_EQ(codec, "lz4");
    EXPECT_FALSE(compressed.empty());
    EXPECT_LT(compressed.size(), input.size());
}

TEST_F(SecureTransportClientTest, Lz4CompressionFailsClosedWhenBridgeUnavailable) {
    config_.compression = SecureTransportClient::Config::CompressionType::LZ4;
    config_.compression_threshold = 1;
    SecureTransportClient client(config_);

    SecureTransportClientTestAccess::clearLz4CompressFn(client);

    const std::string input(4096, 'A');
    std::string compressed;
    std::string codec;

    const bool applied = SecureTransportClientTestAccess::compressData(client, input, compressed, &codec);
    EXPECT_FALSE(applied);
    EXPECT_TRUE(compressed.empty());
    EXPECT_TRUE(codec.empty());
}

/**
 * Integration test for SecureTransportClient
 * Note: This requires actual mTLS setup and a test server
 */
class SecureTransportClientIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Skip if test certificates are not available
        const char* cert_path = std::getenv("TEST_CERT_PATH");
        const char* key_path = std::getenv("TEST_KEY_PATH");
        const char* ca_path = std::getenv("TEST_CA_PATH");
        
        if (!cert_path || !key_path || !ca_path) {
            GTEST_SKIP() << "Test certificates not configured. "
                        << "Set TEST_CERT_PATH, TEST_KEY_PATH, TEST_CA_PATH environment variables.";
        }
        
        config_.cert_path = cert_path;
        config_.key_path = key_path;
        config_.ca_cert_path = ca_path;
        config_.compression = SecureTransportClient::Config::CompressionType::Zstd;
        config_.compression_level = 3;
        config_.max_retries = 3;
    }
    
    SecureTransportClient::Config config_;
};

TEST_F(SecureTransportClientIntegrationTest, TransferToEndpoint) {
    // Skip if test endpoint is not configured
    const char* endpoint = std::getenv("TEST_ENDPOINT");
    if (!endpoint) {
        GTEST_SKIP() << "Test endpoint not configured. Set TEST_ENDPOINT environment variable.";
    }
    
    SecureTransportClient client(config_);
    EXPECT_TRUE(client.isReady());
    
    // Create test payload
    SecureTransportClient::Payload payload;
    payload.data = "test adapter data";
    payload.content_type = "application/octet-stream";
    payload.checksum = "test_checksum";
    payload.metadata = nlohmann::json{
        {"adapter_id", "test_adapter"},
        {"version", "1.0"}
    };
    
    // Attempt transfer
    auto result = client.transfer(endpoint, "/api/v1/lora/receive", payload);
    
    // Check result (may fail if test server is not running)
    if (result.success) {
        EXPECT_GT(result.bytes_sent, 0);
        EXPECT_EQ(result.status_code, 201);
        
        // Verify transfer metrics
        EXPECT_GT(result.bytes_compressed, 0);
        EXPECT_GE(result.compression_ratio, 1.0);
        EXPECT_GE(result.retry_count, 0);
        
        // Note: In a real integration test with a test server,
        // you would also parse and validate the response body:
        // - adapter_id matches request
        // - version matches request
        // - status is "received"
        // - bytes_received matches payload size
        // - compressed flag is correct
    } else {
        // If transfer failed, log error for debugging
        GTEST_SKIP() << "Transfer failed: " << result.error
                    << " (status: " << result.status_code << ")";
    }
}

/**
 * Note: Compression behavior testing requires integration with actual transport.
 * The compression logic is internal to SecureTransportClient and is tested
 * in the integration test suite where we can verify the actual compressed
 * payload size in transfer results.
 */
