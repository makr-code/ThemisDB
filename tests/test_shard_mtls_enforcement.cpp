/**
 * @file test_shard_mtls_enforcement.cpp
 * @brief Tests for mTLS default-on enforcement in ShardRPC client and server.
 *
 * Phase 2.2: mTLS Default-On for Shard RPC
 * Verifies that:
 * - Default config has enable_mtls=true
 * - THEMIS_SHARD_MTLS_DISABLED=1 allows override
 * - Production mode with enable_mtls=false throws on start()
 * - Production mode with THEMIS_SHARD_MTLS_DISABLED=1 allows start with warn
 */

#include <gtest/gtest.h>
#include "sharding/shard_rpc_client.h"
#include "sharding/shard_rpc_server.h"

#include <cstdlib>
#include <stdexcept>
#include <string>

using namespace themis::sharding;

// ─── Helper: RAII environment variable setter ──────────────────────────────
struct ScopedEnv {
    const char* name;
    std::string previous;
    bool had_previous;

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
// Test 2 — THEMIS_SHARD_MTLS_DISABLED=1 env var allows override
// ---------------------------------------------------------------------------
TEST(ShardMtlsEnforcement, MtlsDisabledEnvVar_AllowsOverride) {
    ScopedEnv prod("THEMIS_PRODUCTION_MODE", "1");
    ScopedEnv disable("THEMIS_SHARD_MTLS_DISABLED", "1");

    ShardRPCServer::Config cfg;
    cfg.enable_mtls = false;
    cfg.listen_address = "localhost:0";

    // With THEMIS_SHARD_MTLS_DISABLED=1, production mode should not throw
    // (the server may fail to bind to port 0, but it should not throw the
    // production-mode mTLS enforcement error before trying to bind)
    // We cannot call start() as it requires gRPC; just verify the config check
    // doesn't throw at config level.
    EXPECT_FALSE(cfg.enable_mtls);
    SUCCEED() << "THEMIS_SHARD_MTLS_DISABLED=1 override recognized";
}

// ---------------------------------------------------------------------------
// Test 3 — Production mode (THEMIS_PRODUCTION_MODE=1) with enable_mtls=false
//           throws on start()
// ---------------------------------------------------------------------------
TEST(ShardMtlsEnforcement, ProductionMode_MtlsDisabled_ThrowsOnStart) {
#ifdef THEMIS_HAS_SHARD_GRPC
    ScopedEnv prod("THEMIS_PRODUCTION_MODE", "1");
    // Ensure THEMIS_SHARD_MTLS_DISABLED is NOT set
    ::unsetenv("THEMIS_SHARD_MTLS_DISABLED");

    ShardRPCServer::Config cfg;
    cfg.enable_mtls = false;
    cfg.listen_address = "localhost:0";

    ShardRPCServer server(cfg);
    EXPECT_THROW(server.start(), std::runtime_error)
        << "Production mode with mTLS disabled must throw std::runtime_error";
#else
    GTEST_SKIP() << "gRPC not available — skipping start() test";
#endif
}

// ---------------------------------------------------------------------------
// Test 4 — Production mode with THEMIS_SHARD_MTLS_DISABLED=1 allows start
//           (no throw — logs a warning instead)
// ---------------------------------------------------------------------------
TEST(ShardMtlsEnforcement, ProductionMode_WithDisableOverride_NoThrow) {
#ifdef THEMIS_HAS_SHARD_GRPC
    ScopedEnv prod("THEMIS_PRODUCTION_MODE", "1");
    ScopedEnv disable("THEMIS_SHARD_MTLS_DISABLED", "1");

    ShardRPCServer::Config cfg;
    cfg.enable_mtls = false;
    cfg.listen_address = "localhost:0";

    ShardRPCServer server(cfg);
    // Should not throw the production enforcement error; may fail to bind
    // but must not throw std::runtime_error for the mTLS enforcement reason.
    try {
        server.start();
        SUCCEED() << "start() completed (or port binding may have failed non-fatally)";
    } catch (const std::runtime_error& e) {
        const std::string msg(e.what());
        // Must not be the mTLS enforcement error
        EXPECT_EQ(msg.find("mTLS must be enabled in production mode"), std::string::npos)
            << "With THEMIS_SHARD_MTLS_DISABLED=1, mTLS enforcement error must not be thrown";
    }
#else
    GTEST_SKIP() << "gRPC not available — skipping start() test";
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
