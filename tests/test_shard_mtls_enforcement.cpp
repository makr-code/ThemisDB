/**
 * @file test_shard_mtls_enforcement.cpp
 * @brief Tests for mTLS default-on enforcement in ShardRPC client and server.
 *
 * Phase 2.2: mTLS Default-On for Shard RPC
 * Verifies that production-mode detection and shard RPC TLS failures never
 * degrade to insecure credentials.
 */

#include <gtest/gtest.h>
#include "sharding/shard_rpc_client.h"
#include "sharding/shard_rpc_server.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

using namespace themis::sharding;

#if defined(THEMIS_ENABLE_GRPC) && __has_include("shard_rpc.grpc.pb.h")
#define THEMIS_TEST_HAS_SHARD_GRPC 1
#else
#define THEMIS_TEST_HAS_SHARD_GRPC 0
#endif

// ─── Helper: RAII environment variable setter ──────────────────────────────
struct ScopedEnv {
    const char* name;
    std::string previous = {};
    bool had_previous = {};

    ScopedEnv(const char* n, const char* v) : name(n) {
        const char* prev = getenv(n);
        had_previous = (prev != nullptr);
        if (prev) {
          previous = prev;
        }
#ifdef _WIN32
        _putenv_s(n, v);
#else
        ::setenv(n, v, 1);
#endif
    }
    ~ScopedEnv() {
        if (had_previous) {
#ifdef _WIN32
            _putenv_s(name, previous.c_str());
#else
            ::setenv(name, previous.c_str(), 1);
#endif
        } else {
#ifdef _WIN32
            _putenv_s(name, "");
#else
            ::unsetenv(name);
#endif
        }
    }
};

// ---------------------------------------------------------------------------
// Test 1 — Default ShardRPCServer config has enable_mtls=true
// ---------------------------------------------------------------------------
TEST(ShardMtlsEnforcement, DefaultServerConfig_EnableMtlsTrue) {
    ShardRPCServer::Config cfg;
    EXPECT_TRUE(cfg.enable_mtls)
        << "Default ShardRPCServer::Config must have enable_mtls=true";
}

// ---------------------------------------------------------------------------
// Test 2 — The legacy override cannot disable production mTLS enforcement
// ---------------------------------------------------------------------------
TEST(ShardMtlsEnforcement, ProductionMode_MtlsDisabledOverrideStillFails) {
#if THEMIS_TEST_HAS_SHARD_GRPC
    ScopedEnv environment("THEMIS_ENVIRONMENT", "development");
    ScopedEnv prod("THEMIS_PRODUCTION_MODE", "1");
    ScopedEnv disable("THEMIS_SHARD_MTLS_DISABLED", "1");

    ShardRPCServer::Config cfg;
    cfg.enable_mtls = false;
    cfg.listen_address = "127.0.0.1:0";
    ShardRPCServer server(cfg);
    EXPECT_FALSE(server.start());
#else
    GTEST_SKIP() << "Shard gRPC support is not available";
#endif
}

// ---------------------------------------------------------------------------
// Test 3 — Production mode with enable_mtls=false fails to start
// ---------------------------------------------------------------------------
TEST(ShardMtlsEnforcement, ProductionMode_MtlsDisabled_ThrowsOnStart) {
#if THEMIS_TEST_HAS_SHARD_GRPC
    ScopedEnv environment("THEMIS_ENVIRONMENT", "development");
    ScopedEnv prod("THEMIS_PRODUCTION_MODE", "1");

    ShardRPCServer::Config cfg;
    cfg.enable_mtls = false;
    cfg.listen_address = "127.0.0.1:0";

    ShardRPCServer server(cfg);
    EXPECT_FALSE(server.start())
        << "Production mode with mTLS disabled must not start an insecure listener";
#else
    GTEST_SKIP() << "gRPC not available — skipping start() test";
#endif
}

// ---------------------------------------------------------------------------
// Test 4 — Missing server credentials fail closed
// ---------------------------------------------------------------------------
TEST(ShardMtlsEnforcement, ProductionMode_MissingServerCredentialsFailClosed) {
#if THEMIS_TEST_HAS_SHARD_GRPC
    ScopedEnv environment("THEMIS_ENVIRONMENT", "development");
    ScopedEnv prod("THEMIS_PRODUCTION_MODE", "1");

    ShardRPCServer::Config cfg;
    cfg.enable_mtls = true;
    cfg.listen_address = "127.0.0.1:0";

    ShardRPCServer server(cfg);
    EXPECT_FALSE(server.start())
        << "Missing certificate, key, and CA files must not create an insecure listener";
#else
    GTEST_SKIP() << "gRPC not available — skipping start() test";
#endif
}

TEST(ShardMtlsEnforcement, ProductionMode_InvalidPemCredentialsFailClosed) {
#if THEMIS_TEST_HAS_SHARD_GRPC
    ScopedEnv environment("THEMIS_ENVIRONMENT", "development");
    ScopedEnv prod("THEMIS_PRODUCTION_MODE", "1");

    const auto test_dir = std::filesystem::temp_directory_path() /
                          "themis_invalid_shard_mtls_credentials";
    std::filesystem::remove_all(test_dir);
    std::filesystem::create_directories(test_dir);
    const auto cert_path = test_dir / "server.pem";
    const auto key_path = test_dir / "server-key.pem";
    const auto ca_path = test_dir / "ca.pem";
    for (const auto& path : {cert_path, key_path, ca_path}) {
        std::ofstream(path) << "not a PEM credential";
    }

    ShardRPCServer::Config cfg;
    cfg.enable_mtls = true;
    cfg.listen_address = "127.0.0.1:0";
    cfg.tls_cert_path = cert_path.string();
    cfg.tls_key_path = key_path.string();
    cfg.tls_ca_cert_path = ca_path.string();
    cfg.tls_require_client_cert = true;

    ShardRPCServer server(cfg);
    EXPECT_FALSE(server.start())
        << "Invalid certificate, key, or CA data must not start a plaintext listener";
    std::filesystem::remove_all(test_dir);
#else
    GTEST_SKIP() << "Shard gRPC support is not available";
#endif
}

TEST(ShardMtlsEnforcement, ProductionMode_ClientInvalidPemCannotUseInsecureOverride) {
#if THEMIS_TEST_HAS_SHARD_GRPC
    ScopedEnv environment("THEMIS_ENVIRONMENT", "development");
    ScopedEnv prod("THEMIS_PRODUCTION_MODE", "1");

    const auto test_dir = std::filesystem::temp_directory_path() /
                          "themis_invalid_shard_mtls_client_credentials";
    std::filesystem::remove_all(test_dir);
    std::filesystem::create_directories(test_dir);
    const auto cert_path = test_dir / "client.pem";
    const auto key_path = test_dir / "client-key.pem";
    const auto ca_path = test_dir / "ca.pem";
    for (const auto& path : {cert_path, key_path, ca_path}) {
        std::ofstream(path) << "not a PEM credential";
    }

    ShardRPCClient::Config cfg;
    cfg.endpoint = "127.0.0.1:1";
    cfg.enable_mtls = true;
    cfg.allow_insecure = true;
    cfg.tls_cert_path = cert_path.string();
    cfg.tls_key_path = key_path.string();
    cfg.tls_ca_cert_path = ca_path.string();

    EXPECT_THROW(
        {
            ShardRPCClient client(cfg);
        },
        std::runtime_error);
    std::filesystem::remove_all(test_dir);
#else
    GTEST_SKIP() << "gRPC not available — skipping client PEM validation test";
#endif
}

TEST(ShardMtlsEnforcement, ProductionMode_ClientInsecureOverrideCannotMaskMissingCredentials) {
#if THEMIS_TEST_HAS_SHARD_GRPC
    ScopedEnv environment("THEMIS_ENVIRONMENT", "development");
    ScopedEnv prod("THEMIS_PRODUCTION_MODE", "1");

    ShardRPCClient::Config cfg;
    cfg.endpoint = "127.0.0.1:1";
    cfg.enable_mtls = true;
    cfg.allow_insecure = true;

    EXPECT_THROW(
        {
            ShardRPCClient client(cfg);
        },
        std::runtime_error);
#else
    GTEST_SKIP() << "Shard gRPC support is not available";
#endif
}

TEST(ShardMtlsEnforcement, ProductionMode_RejectsInProcessClientTransport) {
    ScopedEnv environment("THEMIS_ENVIRONMENT", "development");
    ScopedEnv prod("THEMIS_PRODUCTION_MODE", "production");

    ShardRPCClient::Config cfg;
    cfg.endpoint = "inproc://shard-a";
    cfg.allow_insecure = true;

    EXPECT_THROW(
        {
            ShardRPCClient client(cfg);
        },
        std::runtime_error);
}

TEST(ShardMtlsEnforcement, ProductionMode_ClientRejectsDisabledMtlsAndVerification) {
#if THEMIS_TEST_HAS_SHARD_GRPC
    ScopedEnv environment("THEMIS_ENVIRONMENT", "development");
    ScopedEnv prod("THEMIS_PRODUCTION_MODE", "1");

    ShardRPCClient::Config cfg;
    cfg.endpoint = "127.0.0.1:1";
    cfg.enable_mtls = false;
    cfg.allow_insecure = true;
    EXPECT_THROW(
        {
            ShardRPCClient client(cfg);
        },
        std::runtime_error);

    cfg.enable_mtls = true;
    cfg.tls_verify_server = false;
    EXPECT_THROW(
        {
            ShardRPCClient client(cfg);
        },
        std::runtime_error);
#else
    GTEST_SKIP() << "Shard gRPC support is not available";
#endif
}

// ---------------------------------------------------------------------------
// Test 5 — Config with enable_mtls=true does not throw
// ---------------------------------------------------------------------------
TEST(ShardMtlsEnforcement, MtlsEnabled_NoThrowOnConstruct) {
    ShardRPCServer::Config cfg;
    cfg.enable_mtls = true;
    cfg.listen_address = "localhost:0";
    cfg.tls_cert_path    = "/dev/null"; // Non-existent but won't be opened at construct
    cfg.tls_key_path     = "/dev/null";
    cfg.tls_ca_cert_path = "/dev/null";

    EXPECT_NO_THROW(ShardRPCServer server(cfg))
        << "ShardRPCServer construction with mTLS=true must not throw";
}

// ---------------------------------------------------------------------------
// Test 6 — Certificate paths can be set
// ---------------------------------------------------------------------------
TEST(ShardMtlsEnforcement, CertificatePaths_CanBeSet) {
    ShardRPCServer::Config cfg;
    cfg.tls_cert_path    = "/etc/ssl/certs/server.pem";
    cfg.tls_key_path     = "/etc/ssl/private/server.key";
    cfg.tls_ca_cert_path = "/etc/ssl/certs/ca.pem";

    EXPECT_EQ(cfg.tls_cert_path,    "/etc/ssl/certs/server.pem");
    EXPECT_EQ(cfg.tls_key_path,     "/etc/ssl/private/server.key");
    EXPECT_EQ(cfg.tls_ca_cert_path, "/etc/ssl/certs/ca.pem");
    EXPECT_TRUE(cfg.tls_require_client_cert) << "Client cert requirement defaults to true";
}

// ---------------------------------------------------------------------------
// Test 7 — Pool config accessible
// ---------------------------------------------------------------------------
TEST(ShardMtlsEnforcement, PoolConfig_Accessible) {
    ShardRPCClient::Config cfg;
    cfg.max_pool_connections = 100;
    EXPECT_EQ(cfg.max_pool_connections, 100);
    EXPECT_EQ(cfg.connection_pool, nullptr) << "Default pool pointer is null";
}

// ---------------------------------------------------------------------------
// Test 8 — ShardRPCClient default has enable_mtls=true
// ---------------------------------------------------------------------------
TEST(ShardMtlsEnforcement, DefaultClientConfig_EnableMtlsTrue) {
    ShardRPCClient::Config cfg;
    EXPECT_TRUE(cfg.enable_mtls)
        << "Default ShardRPCClient::Config must have enable_mtls=true";
}
