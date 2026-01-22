#include <gtest/gtest.h>
#include "sharding/shard_rpc_client.h"
#include "sharding/shard_rpc_server.h"

using namespace themis::sharding;

/**
 * Test suite for mTLS configuration in ShardRPCClient and ShardRPCServer
 * 
 * These tests verify that mTLS configuration options are correctly
 * stored and accessible, but do not test actual TLS connections
 * (which would require valid certificates and a running server).
 */

// ============================================================================
// ShardRPCClient mTLS Configuration Tests
// ============================================================================

TEST(ShardRPCClientMTLSTest, DefaultConfigurationNoMTLS) {
    ShardRPCClient::Config config;
    config.endpoint = "shard1:50051";
    
    // By default, mTLS should be disabled
    EXPECT_FALSE(config.enable_mtls);
    EXPECT_TRUE(config.tls_cert_path.empty());
    EXPECT_TRUE(config.tls_key_path.empty());
    EXPECT_TRUE(config.tls_ca_cert_path.empty());
    EXPECT_TRUE(config.tls_verify_server);
}

TEST(ShardRPCClientMTLSTest, EnableMTLSConfiguration) {
    ShardRPCClient::Config config;
    config.endpoint = "shard1:50051";
    config.enable_mtls = true;
    config.tls_cert_path = "/etc/themisdb/certs/shard-client.pem";
    config.tls_key_path = "/etc/themisdb/certs/shard-client-key.pem";
    config.tls_ca_cert_path = "/etc/themisdb/certs/ca.pem";
    config.tls_verify_server = true;
    
    // Verify mTLS configuration is stored correctly
    EXPECT_TRUE(config.enable_mtls);
    EXPECT_EQ(config.tls_cert_path, "/etc/themisdb/certs/shard-client.pem");
    EXPECT_EQ(config.tls_key_path, "/etc/themisdb/certs/shard-client-key.pem");
    EXPECT_EQ(config.tls_ca_cert_path, "/etc/themisdb/certs/ca.pem");
    EXPECT_TRUE(config.tls_verify_server);
}

TEST(ShardRPCClientMTLSTest, DisableServerVerification) {
    ShardRPCClient::Config config;
    config.endpoint = "shard1:50051";
    config.enable_mtls = true;
    config.tls_verify_server = false;
    
    // Verify server verification can be disabled (for testing only)
    EXPECT_TRUE(config.enable_mtls);
    EXPECT_FALSE(config.tls_verify_server);
}

TEST(ShardRPCClientMTLSTest, ClientCreationWithMTLSConfig) {
    ShardRPCClient::Config config;
    config.endpoint = "localhost:50051";  // Use localhost to avoid actual connection
    config.timeout_ms = 5000;
    config.max_retries = 3;
    config.enable_mtls = true;
    config.tls_cert_path = "/nonexistent/cert.pem";
    config.tls_key_path = "/nonexistent/key.pem";
    config.tls_ca_cert_path = "/nonexistent/ca.pem";
    
    // Client creation should succeed even with invalid paths
    // (actual connection will fail when attempted, but config is valid)
    EXPECT_NO_THROW({
        ShardRPCClient client(config);
    });
}

// ============================================================================
// ShardRPCServer mTLS Configuration Tests
// ============================================================================

TEST(ShardRPCServerMTLSTest, DefaultServerConfigurationNoMTLS) {
    ShardRPCServer::Config config;
    config.listen_address = "0.0.0.0:50051";
    
    // By default, mTLS should be disabled
    EXPECT_FALSE(config.enable_mtls);
    EXPECT_TRUE(config.tls_cert_path.empty());
    EXPECT_TRUE(config.tls_key_path.empty());
    EXPECT_TRUE(config.tls_ca_cert_path.empty());
    EXPECT_TRUE(config.tls_require_client_cert);
}

TEST(ShardRPCServerMTLSTest, EnableServerMTLSConfiguration) {
    ShardRPCServer::Config config;
    config.listen_address = "0.0.0.0:50051";
    config.enable_mtls = true;
    config.tls_cert_path = "/etc/themisdb/certs/shard-server.pem";
    config.tls_key_path = "/etc/themisdb/certs/shard-server-key.pem";
    config.tls_ca_cert_path = "/etc/themisdb/certs/ca.pem";
    config.tls_require_client_cert = true;
    
    // Verify mTLS configuration is stored correctly
    EXPECT_TRUE(config.enable_mtls);
    EXPECT_EQ(config.tls_cert_path, "/etc/themisdb/certs/shard-server.pem");
    EXPECT_EQ(config.tls_key_path, "/etc/themisdb/certs/shard-server-key.pem");
    EXPECT_EQ(config.tls_ca_cert_path, "/etc/themisdb/certs/ca.pem");
    EXPECT_TRUE(config.tls_require_client_cert);
}

TEST(ShardRPCServerMTLSTest, DisableClientCertRequirement) {
    ShardRPCServer::Config config;
    config.listen_address = "0.0.0.0:50051";
    config.enable_mtls = true;
    config.tls_require_client_cert = false;
    
    // Verify client cert requirement can be disabled
    // (useful for TLS without mutual authentication)
    EXPECT_TRUE(config.enable_mtls);
    EXPECT_FALSE(config.tls_require_client_cert);
}

TEST(ShardRPCServerMTLSTest, ServerCreationWithMTLSConfig) {
    ShardRPCServer::Config config;
    config.listen_address = "127.0.0.1:50052";
    config.enable_mtls = true;
    config.tls_cert_path = "/nonexistent/server-cert.pem";
    config.tls_key_path = "/nonexistent/server-key.pem";
    config.tls_ca_cert_path = "/nonexistent/ca.pem";
    config.tls_require_client_cert = true;
    
    // Server creation should succeed with config
    EXPECT_NO_THROW({
        ShardRPCServer server(config);
    });
}

TEST(ShardRPCServerMTLSTest, BackwardCompatibilityWithoutConfig) {
    // Test that the old constructor still works (backward compatibility)
    EXPECT_NO_THROW({
        ShardRPCServer server("127.0.0.1:50053");
    });
}

// ============================================================================
// Integration Configuration Tests
// ============================================================================

TEST(ShardRPCMTLSIntegrationTest, ClientServerConfigurationMatch) {
    // Verify that client and server can be configured with matching mTLS settings
    
    ShardRPCClient::Config client_config;
    client_config.endpoint = "shard1:50051";
    client_config.enable_mtls = true;
    client_config.tls_cert_path = "/etc/themisdb/certs/shard-client.pem";
    client_config.tls_key_path = "/etc/themisdb/certs/shard-client-key.pem";
    client_config.tls_ca_cert_path = "/etc/themisdb/certs/ca.pem";
    
    ShardRPCServer::Config server_config;
    server_config.listen_address = "0.0.0.0:50051";
    server_config.enable_mtls = true;
    server_config.tls_cert_path = "/etc/themisdb/certs/shard-server.pem";
    server_config.tls_key_path = "/etc/themisdb/certs/shard-server-key.pem";
    server_config.tls_ca_cert_path = "/etc/themisdb/certs/ca.pem";
    server_config.tls_require_client_cert = true;
    
    // Both should have mTLS enabled
    EXPECT_TRUE(client_config.enable_mtls);
    EXPECT_TRUE(server_config.enable_mtls);
    
    // Both should use the same CA for verification
    EXPECT_EQ(client_config.tls_ca_cert_path, server_config.tls_ca_cert_path);
}

TEST(ShardRPCMTLSIntegrationTest, ProductionConfigurationExample) {
    // Example production configuration
    ShardRPCClient::Config prod_client_config;
    prod_client_config.endpoint = "shard-001.dc1.example.com:50051";
    prod_client_config.timeout_ms = 5000;
    prod_client_config.max_retries = 3;
    prod_client_config.retry_delay_ms = 100;
    prod_client_config.enable_mtls = true;
    prod_client_config.tls_cert_path = "/etc/themisdb/certs/shard-client.pem";
    prod_client_config.tls_key_path = "/etc/themisdb/certs/shard-client-key.pem";
    prod_client_config.tls_ca_cert_path = "/etc/themisdb/certs/root-ca.pem";
    prod_client_config.tls_verify_server = true;
    
    // Verify production settings
    EXPECT_TRUE(prod_client_config.enable_mtls);
    EXPECT_TRUE(prod_client_config.tls_verify_server);
    EXPECT_GT(prod_client_config.timeout_ms, 0);
    EXPECT_GT(prod_client_config.max_retries, 0);
}
