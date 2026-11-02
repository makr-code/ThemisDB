#include <gtest/gtest.h>
#include "sharding/shard_rpc_client.h"
#include "sharding/shard_rpc_server.h"
#include "test_crypto_material_utils.h"
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <thread>

using namespace themis::sharding;

#if defined(THEMIS_ENABLE_GRPC) && __has_include("shard_rpc.grpc.pb.h")
#define THEMIS_TEST_HAS_SHARD_GRPC 1
#else
#define THEMIS_TEST_HAS_SHARD_GRPC 0
#endif

namespace {

class TemporaryPemFiles {
public:
    TemporaryPemFiles()
        : directory_(std::filesystem::temp_directory_path() /
                     ("themis-shard-peer-binding-" +
                      std::to_string(std::chrono::steady_clock::now()
                                         .time_since_epoch()
                                         .count()))) {
        std::filesystem::create_directories(directory_);
    }

    ~TemporaryPemFiles() {
        std::error_code error;
        std::filesystem::remove_all(directory_, error);
    }

    std::filesystem::path write(const std::string& name, const std::string& pem) const {
        const auto path = directory_ / name;
        std::ofstream output(path, std::ios::binary);
        output << pem;
        if (!output) {
            throw std::runtime_error("failed to write test PEM file");
        }
        return path;
    }

private:
    std::filesystem::path directory_;
};

#if THEMIS_TEST_HAS_SHARD_GRPC
class PeerBindingRequestHandler : public ShardRPCServer::RequestHandler {
public:
    bool onPrepare(
        [[maybe_unused]] const std::string& transaction_id,
        [[maybe_unused]] const std::string& coordinator_shard_id,
        [[maybe_unused]] const std::string& transaction_data
    ) override {
        ++prepare_count;
        return true;
    }

    bool onCommit([[maybe_unused]] const std::string& transaction_id) override {
        return true;
    }

    bool onAbort([[maybe_unused]] const std::string& transaction_id) override {
        return true;
    }

    ShardRPCServer::HealthInfo onHealthCheck() override {
        return {};
    }

    std::atomic<int> prepare_count{0};
};
#endif

} // namespace

/**
 * Test suite for mTLS configuration in ShardRPCClient and ShardRPCServer
 * 
 * These tests verify that mTLS configuration is stored correctly and that
 * PREPARE is bound to an explicitly configured X.509 peer identity.
 */

// ============================================================================
// ShardRPCClient mTLS Configuration Tests
// ============================================================================

TEST(ShardRPCClientMTLSTest, DefaultConfigurationNoMTLS) {
    ShardRPCClient::Config config;
    config.endpoint = "shard1:50051";
    
    // Shard RPC transport requires mTLS by default.
    EXPECT_TRUE(config.enable_mtls);
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
    
#if THEMIS_TEST_HAS_SHARD_GRPC
    EXPECT_THROW(
        {
            ShardRPCClient client(config);
        },
        std::runtime_error);
#else
    EXPECT_NO_THROW({
        ShardRPCClient client(config);
    });
#endif
}

// ============================================================================
// ShardRPCServer mTLS Configuration Tests
// ============================================================================

TEST(ShardRPCServerMTLSTest, DefaultServerConfigurationNoMTLS) {
    ShardRPCServer::Config config;
    config.listen_address = "0.0.0.0:50051";
    
    // Shard RPC transport requires mTLS by default.
    EXPECT_TRUE(config.enable_mtls);
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

#if THEMIS_TEST_HAS_SHARD_GRPC
TEST(ShardRPCMTLSIntegrationTest, PrepareBindsCoordinatorToAuthenticatedPeer) {
    const auto server_material =
        themis::tests::generateSelfSignedPemMaterial("7E570001", "localhost");
    const auto client_material =
        themis::tests::generateSelfSignedPemMaterial("7E570002", "coordinator-A");
    TemporaryPemFiles pem_files;

    ShardRPCServer::Config server_config;
    server_config.listen_address = "127.0.0.1:50066";
    server_config.enable_mtls = true;
    server_config.tls_require_client_cert = true;
    server_config.tls_cert_path =
        pem_files.write("server-cert.pem", server_material.certificate_pem).string();
    server_config.tls_key_path =
        pem_files.write("server-key.pem", server_material.private_key_pem).string();
    server_config.tls_ca_cert_path =
        pem_files.write("client-ca.pem", client_material.certificate_pem).string();
    server_config.coordinator_identity_bindings.push_back(
        {"x509_common_name", "coordinator-A", "coordinator-A"});

    PeerBindingRequestHandler handler;
    ShardRPCServer server(server_config);
    server.setRequestHandler(&handler);
    ASSERT_TRUE(server.start());

    ShardRPCClient::Config client_config;
    client_config.endpoint = "localhost:50066";
    client_config.enable_mtls = true;
    client_config.tls_verify_server = true;
    client_config.tls_cert_path =
        pem_files.write("client-cert.pem", client_material.certificate_pem).string();
    client_config.tls_key_path =
        pem_files.write("client-key.pem", client_material.private_key_pem).string();
    client_config.tls_ca_cert_path =
        pem_files.write("server-ca.pem", server_material.certificate_pem).string();
    ShardRPCClient client(client_config);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    const auto operations = nlohmann::json::array();
    EpochToken token;
    token.epoch = 1;
    token.issuer = "coordinator-A";
    token.shard_id = "shard-A";
    token.issued_at = std::chrono::system_clock::now();

    EXPECT_TRUE(client.prepare("txn-authenticated-coordinator", operations, token));
    EXPECT_EQ(handler.prepare_count.load(), 1);

    token.issuer = "spoofed-coordinator";
    EXPECT_FALSE(client.prepare("txn-spoofed-coordinator", operations, token));
    EXPECT_EQ(handler.prepare_count.load(), 1);
}

TEST(ShardRPCMTLSIntegrationTest, PrepareRejectsUnmappedAuthenticatedPeer) {
    const auto server_material =
        themis::tests::generateSelfSignedPemMaterial("7E570003", "localhost");
    const auto client_material =
        themis::tests::generateSelfSignedPemMaterial("7E570004", "coordinator-A");
    TemporaryPemFiles pem_files;

    ShardRPCServer::Config server_config;
    server_config.listen_address = "127.0.0.1:50067";
    server_config.enable_mtls = true;
    server_config.tls_require_client_cert = true;
    server_config.tls_cert_path =
        pem_files.write("server-cert.pem", server_material.certificate_pem).string();
    server_config.tls_key_path =
        pem_files.write("server-key.pem", server_material.private_key_pem).string();
    server_config.tls_ca_cert_path =
        pem_files.write("client-ca.pem", client_material.certificate_pem).string();
    server_config.coordinator_identity_bindings.push_back(
        {"x509_common_name", "different-coordinator", "coordinator-A"});

    PeerBindingRequestHandler handler;
    ShardRPCServer server(server_config);
    server.setRequestHandler(&handler);
    ASSERT_TRUE(server.start());

    ShardRPCClient::Config client_config;
    client_config.endpoint = "localhost:50067";
    client_config.enable_mtls = true;
    client_config.tls_verify_server = true;
    client_config.tls_cert_path =
        pem_files.write("client-cert.pem", client_material.certificate_pem).string();
    client_config.tls_key_path =
        pem_files.write("client-key.pem", client_material.private_key_pem).string();
    client_config.tls_ca_cert_path =
        pem_files.write("server-ca.pem", server_material.certificate_pem).string();
    ShardRPCClient client(client_config);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    const auto operations = nlohmann::json::array();
    EpochToken token;
    token.epoch = 1;
    token.issuer = "coordinator-A";
    token.shard_id = "shard-A";
    token.issued_at = std::chrono::system_clock::now();

    EXPECT_FALSE(client.prepare("txn-unmapped-coordinator", operations, token));
    EXPECT_EQ(handler.prepare_count.load(), 0);
}

TEST(ShardRPCMTLSIntegrationTest, PrepareBindsCoordinatorToAuthenticatedSanPeer) {
    const auto server_material =
        themis::tests::generateSelfSignedPemMaterial("7E570005", "localhost");
    const auto client_material =
        themis::tests::generateSelfSignedPemMaterial(
            "7E570006", "", {"coordinator-A"});
    TemporaryPemFiles pem_files;

    ShardRPCServer::Config server_config;
    server_config.listen_address = "127.0.0.1:50068";
    server_config.enable_mtls = true;
    server_config.tls_require_client_cert = true;
    server_config.tls_cert_path =
        pem_files.write("server-cert.pem", server_material.certificate_pem).string();
    server_config.tls_key_path =
        pem_files.write("server-key.pem", server_material.private_key_pem).string();
    server_config.tls_ca_cert_path =
        pem_files.write("client-ca.pem", client_material.certificate_pem).string();
    server_config.coordinator_identity_bindings.push_back(
        {"x509_subject_alternative_name", "coordinator-A", "coordinator-A"});

    PeerBindingRequestHandler handler;
    ShardRPCServer server(server_config);
    server.setRequestHandler(&handler);
    ASSERT_TRUE(server.start());

    ShardRPCClient::Config client_config;
    client_config.endpoint = "localhost:50068";
    client_config.enable_mtls = true;
    client_config.tls_verify_server = true;
    client_config.tls_cert_path =
        pem_files.write("client-cert.pem", client_material.certificate_pem).string();
    client_config.tls_key_path =
        pem_files.write("client-key.pem", client_material.private_key_pem).string();
    client_config.tls_ca_cert_path =
        pem_files.write("server-ca.pem", server_material.certificate_pem).string();
    ShardRPCClient client(client_config);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    const auto operations = nlohmann::json::array();
    EpochToken token;
    token.epoch = 1;
    token.issuer = "coordinator-A";
    token.shard_id = "shard-A";
    token.issued_at = std::chrono::system_clock::now();

    EXPECT_TRUE(client.prepare("txn-san-coordinator", operations, token));
    EXPECT_EQ(handler.prepare_count.load(), 1);
}

TEST(ShardRPCMTLSIntegrationTest, PrepareRejectsAmbiguousSanPeer) {
    const auto server_material =
        themis::tests::generateSelfSignedPemMaterial("7E570007", "localhost");
    const auto client_material =
        themis::tests::generateSelfSignedPemMaterial(
            "7E570008", "", {"coordinator-A", "coordinator-B"});
    TemporaryPemFiles pem_files;

    ShardRPCServer::Config server_config;
    server_config.listen_address = "127.0.0.1:50069";
    server_config.enable_mtls = true;
    server_config.tls_require_client_cert = true;
    server_config.tls_cert_path =
        pem_files.write("server-cert.pem", server_material.certificate_pem).string();
    server_config.tls_key_path =
        pem_files.write("server-key.pem", server_material.private_key_pem).string();
    server_config.tls_ca_cert_path =
        pem_files.write("client-ca.pem", client_material.certificate_pem).string();
    server_config.coordinator_identity_bindings = {
        {"x509_subject_alternative_name", "coordinator-A", "coordinator-A"},
        {"x509_subject_alternative_name", "coordinator-B", "coordinator-B"}};

    PeerBindingRequestHandler handler;
    ShardRPCServer server(server_config);
    server.setRequestHandler(&handler);
    ASSERT_TRUE(server.start());

    ShardRPCClient::Config client_config;
    client_config.endpoint = "localhost:50069";
    client_config.enable_mtls = true;
    client_config.tls_verify_server = true;
    client_config.tls_cert_path =
        pem_files.write("client-cert.pem", client_material.certificate_pem).string();
    client_config.tls_key_path =
        pem_files.write("client-key.pem", client_material.private_key_pem).string();
    client_config.tls_ca_cert_path =
        pem_files.write("server-ca.pem", server_material.certificate_pem).string();
    ShardRPCClient client(client_config);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    const auto operations = nlohmann::json::array();
    EpochToken token;
    token.epoch = 1;
    token.issuer = "coordinator-A";
    token.shard_id = "shard-A";
    token.issued_at = std::chrono::system_clock::now();

    EXPECT_FALSE(client.prepare("txn-ambiguous-san-coordinator", operations, token));
    EXPECT_EQ(handler.prepare_count.load(), 0);
}
#endif
