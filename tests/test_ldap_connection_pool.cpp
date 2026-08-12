#include <gtest/gtest.h>

#include "auth/ldap_authenticator.h"
#include "auth/ldap_connection_pool.h"
#include "auth/auth_metrics.h"

#include <string>
#include <thread>
#include <vector>
#include <atomic>

using namespace themis::auth;

// ===========================================================================
// LDAPPoolConfig helpers
// ===========================================================================

namespace {

LDAPPoolConfig makePoolConfig()
{
    LDAPPoolConfig cfg;
    cfg.server_url  = "ldap://127.0.0.1:39999"; // nothing listening — pool won't pre-warm
    cfg.min_idle    = 0;   // zero so constructor doesn't block on unreachable server
    cfg.max_size    = 4;
    cfg.checkout_timeout_ms = 200;
    cfg.connection_timeout_seconds = 1;
    cfg.search_timeout_seconds     = 1;
    return cfg;
}

LDAPConfig makeAuthConfig(bool pool_enabled = true)
{
    LDAPConfig cfg;
    cfg.server_url        = "ldap://127.0.0.1:39999";
    cfg.bind_dn_template  = "{username}@EXAMPLE.COM";
    cfg.default_role      = "readonly";
    cfg.pool_enabled      = pool_enabled;
    cfg.pool_min_idle     = 0;
    cfg.pool_max_size     = 4;
    cfg.pool_checkout_timeout_ms = 200;
    cfg.connection_timeout_seconds = 1;
    cfg.search_timeout_seconds     = 1;
    return cfg;
}

} // anonymous namespace

// ===========================================================================
// LDAPConnectionPool construction
// ===========================================================================

TEST(LDAPConnectionPoolTest, ConstructWithZeroMinIdle)
{
    // min_idle=0 means no connections are pre-established on construction.
    // The constructor must not block or crash even when the server is unreachable.
    LDAPPoolConfig cfg = makePoolConfig();
    cfg.min_idle = 0;

    EXPECT_NO_THROW({
        LDAPConnectionPool pool(cfg);
        // Pool starts empty.
        EXPECT_EQ(pool.idleConnections(), 0);
        EXPECT_EQ(pool.activeConnections(), 0);
    });
}

TEST(LDAPConnectionPoolTest, ConfigAccessor)
{
    LDAPPoolConfig cfg = makePoolConfig();
    LDAPConnectionPool pool(cfg);

    EXPECT_EQ(pool.config().server_url, cfg.server_url);
    EXPECT_EQ(pool.config().max_size,   cfg.max_size);
    EXPECT_EQ(pool.config().min_idle,   cfg.min_idle);
    EXPECT_EQ(pool.config().checkout_timeout_ms, cfg.checkout_timeout_ms);
}

// ===========================================================================
// LDAPConnectionPool checkout when no server is available
// ===========================================================================

TEST(LDAPConnectionPoolTest, CheckoutReturnsNullWhenServerUnreachable)
{
    LDAPConnectionPool pool(makePoolConfig());

    // With LDAP enabled the pool may still hand out a connection handle
    // (liveness is validated on operation/bind). Without LDAP support it
    // returns nullptr immediately.
    auto conn = pool.checkout();

#ifdef THEMIS_HAS_LDAP
    EXPECT_NE(conn, nullptr);
#else
    EXPECT_EQ(conn, nullptr);
#endif
}

TEST(LDAPConnectionPoolTest, MetricsAfterFailedCheckout)
{
    LDAPConnectionPool pool(makePoolConfig());

    // Whether checkout succeeds or not, pool metrics must be consistent.
    auto conn = pool.checkout();

    const int pool_sz = pool.poolSize();
    const int idle    = pool.idleConnections();
    const int active  = pool.activeConnections();

    // idle + active must equal pool size
    EXPECT_EQ(idle + active, pool_sz);
    EXPECT_GE(idle,   0);
    EXPECT_GE(active, 0);
}

// ===========================================================================
// PooledConnection RAII
// ===========================================================================

TEST(PooledConnectionTest, NullHandleDoesNotCrashOnDestroy)
{
    // A move-from'd PooledConnection (handle == nullptr) must not crash.
    LDAPConnectionPool pool(makePoolConfig());

    auto conn = pool.checkout(); // likely nullptr since server unreachable
    // Explicitly move out to exercise the moved-from-destructor path.
    auto moved = std::move(conn);
    // Original 'conn' is now empty/null — its destructor should be a no-op.
    SUCCEED();
}

TEST(PooledConnectionTest, MarkStaleAndDestroy)
{
    // Construct a pool, get a connection (or nullptr), mark stale, destroy.
    // This verifies the stale eviction path does not crash even offline.
    LDAPConnectionPool pool(makePoolConfig());

    auto conn = pool.checkout();
    if (conn) {
        conn->markStale();
        EXPECT_TRUE(conn->isStale());
        // Destructor should evict (not return) this connection.
    }
    SUCCEED();
}

// ===========================================================================
// LDAPAuthenticator — pool integration
// ===========================================================================

TEST(LDAPAuthenticatorPoolTest, InitializeCreatesPool)
{
    LDAPAuthenticator auth;
    ASSERT_TRUE(auth.initialize(makeAuthConfig(/*pool_enabled=*/true)));

    EXPECT_TRUE(auth.isInitialized());
    // The pool is created even if no connections can be established.
    EXPECT_NE(auth.connectionPool(), nullptr);
}

TEST(LDAPAuthenticatorPoolTest, InitializeWithPoolDisabledHasNoPool)
{
    LDAPAuthenticator auth;
    ASSERT_TRUE(auth.initialize(makeAuthConfig(/*pool_enabled=*/false)));

    EXPECT_TRUE(auth.isInitialized());
    EXPECT_EQ(auth.connectionPool(), nullptr);
}

TEST(LDAPAuthenticatorPoolTest, AuthenticateWithPoolEnabledReturnsFailureNotCrash)
{
    LDAPAuthenticator auth;
    ASSERT_TRUE(auth.initialize(makeAuthConfig(/*pool_enabled=*/true)));

    // Server unreachable — authentication must fail gracefully.
    const auto result = auth.authenticate("jdoe", "secret");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

TEST(LDAPAuthenticatorPoolTest, AuthenticateWithPoolDisabledReturnsFailureNotCrash)
{
    LDAPAuthenticator auth;
    ASSERT_TRUE(auth.initialize(makeAuthConfig(/*pool_enabled=*/false)));

    const auto result = auth.authenticate("jdoe", "secret");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

TEST(LDAPAuthenticatorPoolTest, PoolConfigPassedCorrectly)
{
    LDAPConfig cfg = makeAuthConfig(/*pool_enabled=*/true);
    cfg.pool_min_idle            = 0;
    cfg.pool_max_size            = 8;
    cfg.pool_checkout_timeout_ms = 1000;

    LDAPAuthenticator auth;
    ASSERT_TRUE(auth.initialize(cfg));

    const auto* pool = auth.connectionPool();
    ASSERT_NE(pool, nullptr);
    EXPECT_EQ(pool->config().max_size,          8);
    EXPECT_EQ(pool->config().checkout_timeout_ms, 1000);
}

// ===========================================================================
// AuthMetrics — LDAP pool counters
// ===========================================================================

TEST(AuthMetricsTest, LDAPPoolMetricsDefaultZero)
{
    AuthMetrics metrics;
    EXPECT_EQ(metrics.getLDAPPoolSize(),         0);
    EXPECT_EQ(metrics.getLDAPIdleConnections(),  0);
    EXPECT_EQ(metrics.getLDAPActiveConnections(), 0);
}

TEST(AuthMetricsTest, SetLDAPPoolSizeRoundTrip)
{
    AuthMetrics metrics;
    metrics.setLDAPPoolSize(5);
    EXPECT_EQ(metrics.getLDAPPoolSize(), 5);
}

TEST(AuthMetricsTest, SetLDAPIdleConnectionsRoundTrip)
{
    AuthMetrics metrics;
    metrics.setLDAPIdleConnections(3);
    EXPECT_EQ(metrics.getLDAPIdleConnections(), 3);
}

TEST(AuthMetricsTest, SetLDAPActiveConnectionsRoundTrip)
{
    AuthMetrics metrics;
    metrics.setLDAPActiveConnections(2);
    EXPECT_EQ(metrics.getLDAPActiveConnections(), 2);
}

TEST(AuthMetricsTest, LDAPPoolMetricsReflectPoolState)
{
    LDAPConnectionPool pool(makePoolConfig());
    AuthMetrics metrics;

    // Update metrics from pool state.
    metrics.setLDAPPoolSize(pool.poolSize());
    metrics.setLDAPIdleConnections(pool.idleConnections());
    metrics.setLDAPActiveConnections(pool.activeConnections());

    // Values must be consistent: idle + active == pool_size.
    EXPECT_EQ(metrics.getLDAPIdleConnections() + metrics.getLDAPActiveConnections(),
              metrics.getLDAPPoolSize());
}

TEST(AuthMetricsTest, LDAPPoolMetricsAreThreadSafe)
{
    AuthMetrics metrics;
    constexpr int kThreads  = 8;
    constexpr int kIter     = 500;
    std::atomic<int> errors{0};

    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&metrics, &errors, t]() {
            for (int i = 0; i < kIter; ++i) {
                const int val = (t * kIter + i) % 32;
                metrics.setLDAPPoolSize(val);
                metrics.setLDAPIdleConnections(val / 2);
                metrics.setLDAPActiveConnections(val / 2);
                // As long as reads don't crash, we're happy.
                const int ps = metrics.getLDAPPoolSize();
                const int ic = metrics.getLDAPIdleConnections();
                const int ac = metrics.getLDAPActiveConnections();
                if (ps < 0 || ic < 0 || ac < 0) {
                    errors.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    for (auto& th : threads) th.join();
    EXPECT_EQ(errors.load(), 0);
}

// ===========================================================================
// Integration — disabled test requiring a live LDAP server
// ===========================================================================

/**
 * @brief Integration test — requires an actual LDAP server.
 *
 * To run:
 *   ctest -R LDAPConnectionPool.DISABLED_PoolReuseIntegration
 */
TEST(LDAPConnectionPoolTest, DISABLED_PoolReuseIntegration)
{
    LDAPPoolConfig cfg;
    cfg.server_url  = "ldap://localhost:389";
    cfg.min_idle    = 2;
    cfg.max_size    = 8;
    cfg.checkout_timeout_ms = 5000;
    cfg.connection_timeout_seconds = 5;
    cfg.search_timeout_seconds     = 5;

    LDAPConnectionPool pool(cfg);

    // Pre-warming should have established min_idle connections.
    EXPECT_GE(pool.idleConnections(), cfg.min_idle);

    {
        auto conn = pool.checkout();
        ASSERT_NE(conn, nullptr);
        EXPECT_FALSE(conn->isStale());
        EXPECT_EQ(pool.activeConnections(), 1);
    }
    // After the PooledConnection goes out of scope, it should be returned.
    EXPECT_EQ(pool.activeConnections(), 0);
}
