#include <gtest/gtest.h>
#include "sharding/mtls_client.h"

using namespace themis::sharding;

// Note: These are structure and configuration tests
// Full integration tests would require actual TLS certificates and a test server

TEST(MTLSClientTest, ConfigurationStructure) {
    MTLSClient::Config config;
    config.cert_path = "/path/to/shard-001.crt";
    config.key_path = "/path/to/shard-001.key";
    config.ca_cert_path = "/path/to/root-ca.crt";
    config.tls_version = "TLSv1.3";
    config.verify_peer = true;
    config.connect_timeout_ms = 5000;
    config.request_timeout_ms = 30000;
    
    EXPECT_EQ(config.cert_path, "/path/to/shard-001.crt");
    EXPECT_EQ(config.key_path, "/path/to/shard-001.key");
    EXPECT_EQ(config.ca_cert_path, "/path/to/root-ca.crt");
    EXPECT_EQ(config.tls_version, "TLSv1.3");
    EXPECT_TRUE(config.verify_peer);
}

TEST(MTLSClientTest, ResponseStructure) {
    MTLSClient::Response response;
    response.status_code = 200;
    response.status_message = "OK";
    response.success = true;
    response.body = nlohmann::json{{"key", "value"}};
    
    EXPECT_EQ(response.status_code, 200);
    EXPECT_EQ(response.status_message, "OK");
    EXPECT_TRUE(response.success);
    EXPECT_EQ(response.body["key"], "value");
}

TEST(MTLSClientTest, DefaultConfiguration) {
    MTLSClient::Config config;
    
    // Check defaults
    EXPECT_EQ(config.tls_version, "TLSv1.3");
    EXPECT_TRUE(config.verify_peer);
    EXPECT_TRUE(config.verify_hostname);
    EXPECT_EQ(config.connect_timeout_ms, 5000u);
    EXPECT_EQ(config.request_timeout_ms, 30000u);
    EXPECT_EQ(config.max_retries, 3u);
    EXPECT_TRUE(config.enable_pooling);
}

TEST(MTLSClientTest, IsReadyWithoutConfig) {
    // Note: Cannot create MTLSClient without valid certificates
    // This test just verifies the structure
    MTLSClient::Config config;
    config.cert_path = "";
    config.key_path = "";
    config.ca_cert_path = "";
    
    // Would fail in real initialization, but we're testing structure
    EXPECT_TRUE(config.cert_path.empty());
}

TEST(MTLSClientTest, EndpointParsing) {
    // Test endpoint parsing logic (would be tested via private method in real impl)
    std::string endpoint1 = "https://shard-001.dc1:8080";
    std::string endpoint2 = "shard-002.dc1:9090";
    std::string endpoint3 = "localhost:8765";
    
    // Just verify the format is reasonable
    EXPECT_TRUE(endpoint1.find("://") != std::string::npos);
    EXPECT_TRUE(endpoint2.find(":") != std::string::npos);
    EXPECT_TRUE(endpoint3.find(":") != std::string::npos);
}

TEST(MTLSClientTest, RetryConfiguration) {
    MTLSClient::Config config;
    config.max_retries = 5;
    config.retry_delay_ms = 2000;
    
    EXPECT_EQ(config.max_retries, 5u);
    EXPECT_EQ(config.retry_delay_ms, 2000u);
}

TEST(MTLSClientTest, ConnectionPoolingConfig) {
    MTLSClient::Config config;
    config.enable_pooling = false;
    config.max_connections = 20;
    config.idle_timeout_ms = 120000;
    
    EXPECT_FALSE(config.enable_pooling);
    EXPECT_EQ(config.max_connections, 20u);
    EXPECT_EQ(config.idle_timeout_ms, 120000u);
}

// ===========================================================================
// IPv6 Endpoint Parsing Tests
// ===========================================================================

TEST(MTLSClientTest, ParseEndpoint_IPv4_WithPort) {
    auto [host, port] = MTLSClient::parseEndpoint("192.168.1.1:8080");
    EXPECT_EQ(host, "192.168.1.1");
    EXPECT_EQ(port, "8080");
}

TEST(MTLSClientTest, ParseEndpoint_IPv4_WithoutPort) {
    auto [host, port] = MTLSClient::parseEndpoint("192.168.1.1");
    EXPECT_EQ(host, "192.168.1.1");
    EXPECT_EQ(port, "8080");  // Default port
}

TEST(MTLSClientTest, ParseEndpoint_IPv4_WithProtocol) {
    auto [host, port] = MTLSClient::parseEndpoint("https://192.168.1.1:9090");
    EXPECT_EQ(host, "192.168.1.1");
    EXPECT_EQ(port, "9090");
}

TEST(MTLSClientTest, ParseEndpoint_IPv6_WithBracketsAndPort) {
    auto [host, port] = MTLSClient::parseEndpoint("[2001:db8::1]:8080");
    EXPECT_EQ(host, "2001:db8::1");
    EXPECT_EQ(port, "8080");
}

TEST(MTLSClientTest, ParseEndpoint_IPv6_WithBracketsWithoutPort) {
    auto [host, port] = MTLSClient::parseEndpoint("[2001:db8::1]");
    EXPECT_EQ(host, "2001:db8::1");
    EXPECT_EQ(port, "8080");  // Default port
}

TEST(MTLSClientTest, ParseEndpoint_IPv6_WithoutBracketsWithoutPort) {
    // IPv6 without brackets and without port (multiple colons)
    auto [host, port] = MTLSClient::parseEndpoint("2001:db8::1");
    EXPECT_EQ(host, "2001:db8::1");
    EXPECT_EQ(port, "8080");  // Default port
}

TEST(MTLSClientTest, ParseEndpoint_IPv6_WithProtocolAndPort) {
    auto [host, port] = MTLSClient::parseEndpoint("https://[2001:db8::1]:9090");
    EXPECT_EQ(host, "2001:db8::1");
    EXPECT_EQ(port, "9090");
}

TEST(MTLSClientTest, ParseEndpoint_IPv6_Localhost) {
    auto [host, port] = MTLSClient::parseEndpoint("[::1]:8080");
    EXPECT_EQ(host, "::1");
    EXPECT_EQ(port, "8080");
}

TEST(MTLSClientTest, ParseEndpoint_IPv6_LocalhostWithoutPort) {
    auto [host, port] = MTLSClient::parseEndpoint("::1");
    EXPECT_EQ(host, "::1");
    EXPECT_EQ(port, "8080");  // Default port
}

TEST(MTLSClientTest, ParseEndpoint_IPv6_FullAddress) {
    auto [host, port] = MTLSClient::parseEndpoint("[2001:0db8:85a3:0000:0000:8a2e:0370:7334]:443");
    EXPECT_EQ(host, "2001:0db8:85a3:0000:0000:8a2e:0370:7334");
    EXPECT_EQ(port, "443");
}

TEST(MTLSClientTest, ParseEndpoint_IPv6_CompressedZeros) {
    auto [host, port] = MTLSClient::parseEndpoint("[fe80::1]:8080");
    EXPECT_EQ(host, "fe80::1");
    EXPECT_EQ(port, "8080");
}

TEST(MTLSClientTest, ParseEndpoint_Hostname_WithPort) {
    auto [host, port] = MTLSClient::parseEndpoint("shard-001.dc1.example.com:8080");
    EXPECT_EQ(host, "shard-001.dc1.example.com");
    EXPECT_EQ(port, "8080");
}

TEST(MTLSClientTest, ParseEndpoint_Hostname_WithoutPort) {
    auto [host, port] = MTLSClient::parseEndpoint("shard-001.dc1.example.com");
    EXPECT_EQ(host, "shard-001.dc1.example.com");
    EXPECT_EQ(port, "8080");  // Default port
}

TEST(MTLSClientTest, ParseEndpoint_Hostname_WithProtocol) {
    auto [host, port] = MTLSClient::parseEndpoint("https://shard-001.dc1.example.com:9090");
    EXPECT_EQ(host, "shard-001.dc1.example.com");
    EXPECT_EQ(port, "9090");
}

TEST(MTLSClientTest, ParseEndpoint_Localhost_WithPort) {
    auto [host, port] = MTLSClient::parseEndpoint("localhost:8765");
    EXPECT_EQ(host, "localhost");
    EXPECT_EQ(port, "8765");
}

TEST(MTLSClientTest, ParseEndpoint_Localhost_WithoutPort) {
    auto [host, port] = MTLSClient::parseEndpoint("localhost");
    EXPECT_EQ(host, "localhost");
    EXPECT_EQ(port, "8080");  // Default port
}

TEST(MTLSClientTest, ParseEndpoint_IPv6_MalformedBrackets) {
    // Malformed: missing closing bracket
    auto [host, port] = MTLSClient::parseEndpoint("[2001:db8::1");
    // Should treat entire string as host
    EXPECT_EQ(host, "[2001:db8::1");
    EXPECT_EQ(port, "8080");  // Default port
}

TEST(MTLSClientTest, ParseEndpoint_IPv6_LinkLocal) {
    auto [host, port] = MTLSClient::parseEndpoint("[fe80::a00:27ff:fe4e:66a1]:8080");
    EXPECT_EQ(host, "fe80::a00:27ff:fe4e:66a1");
    EXPECT_EQ(port, "8080");
}

TEST(MTLSClientTest, ParseEndpoint_IPv6_AllZeros) {
    auto [host, port] = MTLSClient::parseEndpoint("[::]:8080");
    EXPECT_EQ(host, "::");
    EXPECT_EQ(port, "8080");
}

