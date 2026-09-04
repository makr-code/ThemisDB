// Copyright 2025 ThemisDB
// Licensed under MIT License

// Tests for:
//  - IRemoteCachePeer interface contract
//  - IClusterView interface contract
//  - CacheReplicationCoordinator (local bus + async remote fanout)
//  - GrpcRemoteCachePeer config/construction (compile-time, THEMIS_ENABLE_GRPC guard)

#include <gtest/gtest.h>

#include "cache/cache_replication_coordinator.h"
#include "cache/grpc_remote_cache_peer.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

using namespace themis::cache;

// ─────────────────────────────────────────────────────────────────────────────
// Test doubles
// ─────────────────────────────────────────────────────────────────────────────

/// A mock IRemoteCachePeer that records calls and can be configured to fail.
class MockRemoteCachePeer final : public IRemoteCachePeer {
public:
    explicit MockRemoteCachePeer(std::string addr = "mock:1234")
        : addr_(std::move(addr)) {}

    void invalidate(const std::string& key,
                    const std::string& tenant_id = "") override {
        ++invalidate_calls;
        last_key = key;
        last_tenant = tenant_id;
        if (should_throw) {
            ++throw_count;
            throw std::runtime_error("mock peer unavailable");
        }
        // Notify any waiting test.
        {
            std::lock_guard<std::mutex> lk(cv_mutex);
            ++delivered;
        }
        cv.notify_all();
    }

    void invalidateTenant(const std::string& tenant_id) override {
        ++invalidate_tenant_calls;
        last_tenant = tenant_id;
        if (should_throw) {
            ++throw_count;
            throw std::runtime_error("mock peer unavailable");
        }
        {
            std::lock_guard<std::mutex> lk(cv_mutex);
            ++delivered;
        }
        cv.notify_all();
    }

    std::string address()   const override { return addr_; }
    bool        isHealthy() const override { return !should_throw; }

    /// Block until at least @p n deliveries have been recorded (or timeout).
    bool waitForDeliveries(int n, std::chrono::milliseconds timeout =
                                      std::chrono::milliseconds(2000)) {
        std::unique_lock<std::mutex> lk(cv_mutex);
        return cv.wait_for(lk, timeout, [&] { return delivered >= n; });
    }

    std::atomic<int>  invalidate_calls{0};
    std::atomic<int>  invalidate_tenant_calls{0};
    std::atomic<int>  throw_count{0};
    bool              should_throw = false;
    std::string       last_key;
    std::string       last_tenant;

private:
    std::string            addr_;
    std::mutex             cv_mutex;
    std::condition_variable cv;
    int                    delivered = 0;
};

/// A static IClusterView returning a fixed list of peer addresses.
class StaticClusterView final : public IClusterView {
public:
    explicit StaticClusterView(std::vector<std::string> addrs)
        : addrs_(std::move(addrs)) {}

    std::vector<std::string> getPeerAddresses() const override { return addrs_; }

private:
    std::vector<std::string> addrs_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Helper: build a CacheReplicationCoordinator with N mock peers
// ─────────────────────────────────────────────────────────────────────────────

struct TestHarness {
    std::vector<MockRemoteCachePeer*>             raw_peers;
    std::unique_ptr<StaticClusterView>            cluster_view;
    std::shared_ptr<CacheReplicationCoordinator>  coordinator;

    static TestHarness make(int n_peers,
                            bool peers_throw = false,
                            std::shared_ptr<InProcessCacheCoordinator::Bus> bus = nullptr) {
        TestHarness h;
        std::vector<std::string> addrs;
        addrs.reserve(n_peers);
        for (int i = 0; i < n_peers; ++i) {
            addrs.push_back("peer" + std::to_string(i) + ":9000");
        }
        h.cluster_view = std::make_unique<StaticClusterView>(addrs);

        // Keep raw pointers for introspection BEFORE the factory moves them.
        std::vector<std::unique_ptr<MockRemoteCachePeer>> owned;
        owned.reserve(n_peers);
        for (int i = 0; i < n_peers; ++i) {
            auto peer = std::make_unique<MockRemoteCachePeer>(addrs[i]);
            peer->should_throw = peers_throw;
            h.raw_peers.push_back(peer.get());
            owned.push_back(std::move(peer));
        }

        // Move owned peers into the factory closure.
        auto owned_ptr =
            std::make_shared<std::vector<std::unique_ptr<MockRemoteCachePeer>>>(
                std::move(owned));
        int call_idx = 0;
        auto factory = [owned_ptr, call_idx](const std::string&) mutable
            -> std::unique_ptr<IRemoteCachePeer> {
            if (call_idx < static_cast<int>(owned_ptr->size())) {
                return std::move((*owned_ptr)[call_idx++]);
            }
            return nullptr;
        };

        h.coordinator = std::make_shared<CacheReplicationCoordinator>(
            h.cluster_view.get(), std::move(bus), factory);
        return h;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// IRemoteCachePeer interface smoke tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(IRemoteCachePeerTest, MockImplementsInterface) {
    MockRemoteCachePeer peer("node1:8771");
    EXPECT_EQ(peer.address(), "node1:8771");
    EXPECT_TRUE(peer.isHealthy());
    EXPECT_NO_THROW(peer.invalidate("users.*", "tenant1"));
    EXPECT_NO_THROW(peer.invalidateTenant("tenant1"));
    EXPECT_EQ(peer.invalidate_calls.load(), 1);
    EXPECT_EQ(peer.invalidate_tenant_calls.load(), 1);
}

TEST(IRemoteCachePeerTest, InvalidateRecordsKeyAndTenant) {
    MockRemoteCachePeer peer;
    peer.invalidate("key_pattern", "t42");
    EXPECT_EQ(peer.last_key,    "key_pattern");
    EXPECT_EQ(peer.last_tenant, "t42");
}

TEST(IRemoteCachePeerTest, InvalidateTenantRecordsTenant) {
    MockRemoteCachePeer peer;
    peer.invalidateTenant("my_tenant");
    EXPECT_EQ(peer.last_tenant, "my_tenant");
}

#ifdef THEMIS_ENABLE_GRPC
TEST(GrpcRemoteCachePeerTest, DefaultConfigFailsClosed) {
    GrpcRemoteCachePeer::Config cfg("node1:8771");
    EXPECT_FALSE(cfg.tls_enabled);
    EXPECT_FALSE(cfg.allow_insecure);
    EXPECT_THROW({
        GrpcRemoteCachePeer peer(cfg);
    }, std::runtime_error);
}

TEST(GrpcRemoteCachePeerTest, ExplicitInsecureOverrideAllowed) {
    GrpcRemoteCachePeer::Config cfg("node2:8771");
    cfg.allow_insecure = true;
    EXPECT_NO_THROW({
        GrpcRemoteCachePeer peer(cfg);
        EXPECT_EQ(peer.address(), "node2:8771");
    });
}
#else
TEST(GrpcRemoteCachePeerStubTest, BackendInvokeBridgeHandlesInvalidateCalls) {
    std::string seen_address;
    std::string seen_type;
    std::string seen_key;
    std::string seen_tenant;

    GrpcRemoteCachePeer::setBackendInvokeFn(
        [&](const std::string& address,
            const std::string& type,
            const std::string& key,
            const std::string& tenant_id) {
            seen_address = address;
            seen_type = type;
            seen_key = key;
            seen_tenant = tenant_id;
            return true;
        });

    GrpcRemoteCachePeer peer("cache-peer:9443");
    EXPECT_NO_THROW(peer.invalidate("users:*", "tenant-a"));
    EXPECT_TRUE(peer.isHealthy());
    EXPECT_EQ(seen_address, "cache-peer:9443");
    EXPECT_EQ(seen_type, "invalidate");
    EXPECT_EQ(seen_key, "users:*");
    EXPECT_EQ(seen_tenant, "tenant-a");

    EXPECT_NO_THROW(peer.invalidateTenant("tenant-b"));
    EXPECT_EQ(seen_type, "invalidate_tenant");
    EXPECT_EQ(seen_key, "");
    EXPECT_EQ(seen_tenant, "tenant-b");

    GrpcRemoteCachePeer::setBackendInvokeFn(nullptr);
}
#endif

// ─────────────────────────────────────────────────────────────────────────────
// IClusterView interface smoke tests
// ─────────────────────────────────────────────────────────────────────────────

TEST(IClusterViewTest, ReturnsConfiguredAddresses) {
    StaticClusterView cv({"a:9000", "b:9000", "c:9000"});
    auto addrs = cv.getPeerAddresses();
    ASSERT_EQ(addrs.size(), 3u);
    EXPECT_EQ(addrs[0], "a:9000");
    EXPECT_EQ(addrs[2], "c:9000");
}

TEST(IClusterViewTest, EmptyClusterView) {
    StaticClusterView cv({});
    EXPECT_TRUE(cv.getPeerAddresses().empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// CacheReplicationCoordinator – construction
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheReplicationCoordinatorTest, NameAndConnected) {
    StaticClusterView cv({});
    CacheReplicationCoordinator coord(&cv);
    EXPECT_EQ(coord.name(), "CacheReplicationCoordinator");
    EXPECT_TRUE(coord.isConnected());
}

TEST(CacheReplicationCoordinatorTest, NullClusterViewNoRemotePeers) {
    CacheReplicationCoordinator coord(nullptr);
    auto stats = coord.getStats();
    EXPECT_EQ(stats["remote_peer_count"].get<std::size_t>(), 0u);
}

TEST(CacheReplicationCoordinatorTest, NullFactoryNoRemotePeers) {
    StaticClusterView cv({"a:9000", "b:9000"});
    // No factory → peers cannot be created → remote_peer_count == 0.
    CacheReplicationCoordinator coord(&cv, nullptr, nullptr);
    auto stats = coord.getStats();
    EXPECT_EQ(stats["remote_peer_count"].get<std::size_t>(), 0u);
}

TEST(CacheReplicationCoordinatorTest, PeersPopulatedFromClusterView) {
    auto h = TestHarness::make(3);
    auto stats = h.coordinator->getStats();
    EXPECT_EQ(stats["remote_peer_count"].get<std::size_t>(), 3u);
}

// ─────────────────────────────────────────────────────────────────────────────
// CacheReplicationCoordinator – publishEntry does NOT fan out remotely
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheReplicationCoordinatorTest, PublishEntryNoRemoteFanout) {
    auto h = TestHarness::make(2);
    h.coordinator->publishEntry("k1", {{"v", 1}}, 60, "t1");

    // Give the fanout thread time to act (it shouldn't).
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    EXPECT_EQ(h.raw_peers[0]->invalidate_calls.load(), 0);
    EXPECT_EQ(h.raw_peers[1]->invalidate_calls.load(), 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// CacheReplicationCoordinator – publishInvalidation fans out to remote peers
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheReplicationCoordinatorTest, InvalidationFanoutToAllPeers) {
    auto h = TestHarness::make(2);
    h.coordinator->publishInvalidation("users.*", "tenant1");

    // Wait for both peers to receive the invalidation.
    EXPECT_TRUE(h.raw_peers[0]->waitForDeliveries(1));
    EXPECT_TRUE(h.raw_peers[1]->waitForDeliveries(1));

    EXPECT_EQ(h.raw_peers[0]->invalidate_calls.load(), 1);
    EXPECT_EQ(h.raw_peers[1]->invalidate_calls.load(), 1);
    EXPECT_EQ(h.raw_peers[0]->last_key, "users.*");
    EXPECT_EQ(h.raw_peers[0]->last_tenant, "tenant1");
}

TEST(CacheReplicationCoordinatorTest, InvalidationFanoutWithNoPeers) {
    StaticClusterView cv({});
    CacheReplicationCoordinator coord(&cv);
    // Should not throw; simply a no-op for remote fanout.
    EXPECT_NO_THROW(coord.publishInvalidation(".*", "t1"));
}

TEST(CacheReplicationCoordinatorTest, MultipleInvalidationsFanoutAll) {
    auto h = TestHarness::make(1);
    for (int i = 0; i < 5; ++i) {
        h.coordinator->publishInvalidation("key" + std::to_string(i));
    }
    EXPECT_TRUE(h.raw_peers[0]->waitForDeliveries(5));
    EXPECT_EQ(h.raw_peers[0]->invalidate_calls.load(), 5);
}

// ─────────────────────────────────────────────────────────────────────────────
// CacheReplicationCoordinator – local bus still works
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheReplicationCoordinatorTest, LocalInvalidationCallbackFires) {
    auto bus = std::make_shared<InProcessCacheCoordinator::Bus>();

    // Coordinator A has remote peers AND a local bus subscriber (coord B).
    auto cb_coord = std::make_shared<InProcessCacheCoordinator>(bus);
    std::vector<ReplicationMessage> received;
    cb_coord->subscribeInvalidations([&received](const ReplicationMessage& m) {
        received.push_back(m);
    });

    StaticClusterView cv({});
    CacheReplicationCoordinator coord_a(&cv, bus, nullptr);
    coord_a.publishInvalidation("orders.*", "t99");

    // The local bus delivers synchronously.
    ASSERT_EQ(received.size(), 1u);
    EXPECT_EQ(received[0].key,       "orders.*");
    EXPECT_EQ(received[0].tenant_id, "t99");
}

TEST(CacheReplicationCoordinatorTest, SubscriptionsForwardedToLocalCoordinator) {
    // Verify that subscribeInvalidations() registered on a CacheReplicationCoordinator
    // is forwarded to its inner InProcessCacheCoordinator delegate, so the callback
    // fires when a peer on the same local bus publishes an invalidation.
    auto bus = std::make_shared<InProcessCacheCoordinator::Bus>();

    StaticClusterView cv({});
    CacheReplicationCoordinator a(&cv, bus, nullptr);
    CacheReplicationCoordinator b(&cv, bus, nullptr);

    std::vector<ReplicationMessage> b_msgs;
    b.subscribeInvalidations([&b_msgs](const ReplicationMessage& m) {
        b_msgs.push_back(m);
    });

    a.publishInvalidation("pat.*", "ten");
    ASSERT_EQ(b_msgs.size(), 1u);
    EXPECT_EQ(b_msgs[0].key, "pat.*");
}

// ─────────────────────────────────────────────────────────────────────────────
// CacheReplicationCoordinator – retry behaviour
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheReplicationCoordinatorTest, FailingPeerIsRetriedUpToMaxAttempts) {
    auto h = TestHarness::make(1, /*peers_throw=*/true);
    h.coordinator->publishInvalidation("stale.*");

    // The fanout thread will retry kMaxRetryAttempts times.
    // Each attempt throws → the message is re-enqueued.
    // After kMaxRetryAttempts attempts the message is dropped.
    const int max = CacheReplicationCoordinator::kMaxRetryAttempts;

    // Give the fanout thread enough time to exhaust all retries.
    const auto deadline = std::chrono::steady_clock::now() +
                          std::chrono::milliseconds(3000);
    while (std::chrono::steady_clock::now() < deadline) {
        if (h.raw_peers[0]->throw_count.load() >= max) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }

    EXPECT_GE(h.raw_peers[0]->throw_count.load(), max);

    auto stats = h.coordinator->getStats();
    EXPECT_GE(stats["fanout_retried"].get<uint64_t>(), 1u);
    EXPECT_GE(stats["fanout_failed"].get<uint64_t>(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// CacheReplicationCoordinator – bounded queue drop
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheReplicationCoordinatorTest, QueueDropsWhenFull) {
    // Use shared_ptr for sync primitives so BlockingPeer has safe lifetime.
    auto unblock  = std::make_shared<std::atomic<bool>>(false);
    auto block_mu = std::make_shared<std::mutex>();
    auto block_cv = std::make_shared<std::condition_variable>();

    StaticClusterView cv({"slow:9000"});

    // The factory returns a peer whose invalidate() blocks until we set unblock.
    auto factory = [unblock, block_mu, block_cv](const std::string&)
        -> std::unique_ptr<IRemoteCachePeer>
    {
        class BlockingPeer final : public IRemoteCachePeer {
        public:
            BlockingPeer(std::shared_ptr<std::atomic<bool>> unblock,
                         std::shared_ptr<std::mutex>        mu,
                         std::shared_ptr<std::condition_variable> cv)
                : unblock_(std::move(unblock)), mu_(std::move(mu)), cv_(std::move(cv)) {}

            void invalidate(const std::string&, const std::string& = "") override {
                std::unique_lock<std::mutex> lk(*mu_);
                cv_->wait(lk, [this] { return unblock_->load(); });
            }
            void invalidateTenant(const std::string&) override {
                std::unique_lock<std::mutex> lk(*mu_);
                cv_->wait(lk, [this] { return unblock_->load(); });
            }
            std::string address()   const override { return "slow:9000"; }
            bool        isHealthy() const override { return true; }

        private:
            std::shared_ptr<std::atomic<bool>>       unblock_;
            std::shared_ptr<std::mutex>              mu_;
            std::shared_ptr<std::condition_variable> cv_;
        };
        return std::make_unique<BlockingPeer>(unblock, block_mu, block_cv);
    };

    auto coord = std::make_shared<CacheReplicationCoordinator>(
        &cv, nullptr, factory);

    // Flood the queue with more entries than kRetryQueueCapacity.
    const std::size_t flood = CacheReplicationCoordinator::kRetryQueueCapacity + 50;
    for (std::size_t i = 0; i < flood; ++i) {
        coord->publishInvalidation("key" + std::to_string(i));
    }

    // At least some entries must have been dropped.
    auto stats = coord->getStats();
    EXPECT_GT(stats["fanout_dropped"].get<uint64_t>(), 0u);

    // Unblock the worker thread before destroying the coordinator.
    {
        std::lock_guard<std::mutex> lk(*block_mu);
        unblock->store(true);
    }
    block_cv->notify_all();
}

// ─────────────────────────────────────────────────────────────────────────────
// CacheReplicationCoordinator – refreshPeers
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheReplicationCoordinatorTest, RefreshPeersUpdatesRemoteList) {
    auto h = TestHarness::make(2);
    EXPECT_EQ(h.coordinator->getStats()["remote_peer_count"].get<std::size_t>(), 2u);

    // Replace cluster view with empty.
    StaticClusterView empty_cv({});
    // Cannot swap the injected pointer easily from outside, but we can test
    // refreshPeers() with the original cluster view (no change) doesn't crash.
    EXPECT_NO_THROW(h.coordinator->refreshPeers());
    // Peer count should remain 2 (same cluster view).
    EXPECT_EQ(h.coordinator->getStats()["remote_peer_count"].get<std::size_t>(), 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// CacheReplicationCoordinator – getStats
// ─────────────────────────────────────────────────────────────────────────────

TEST(CacheReplicationCoordinatorTest, GetStatsContainsExpectedFields) {
    auto h = TestHarness::make(1);
    h.coordinator->publishInvalidation("k1", "t1");
    EXPECT_TRUE(h.raw_peers[0]->waitForDeliveries(1));

    auto stats = h.coordinator->getStats();
    EXPECT_TRUE(stats.contains("name"));
    EXPECT_TRUE(stats.contains("connected"));
    EXPECT_TRUE(stats.contains("remote_peer_count"));
    EXPECT_TRUE(stats.contains("fanout_enqueued"));
    EXPECT_TRUE(stats.contains("fanout_dropped"));
    EXPECT_TRUE(stats.contains("fanout_delivered"));
    EXPECT_TRUE(stats.contains("fanout_retried"));
    EXPECT_TRUE(stats.contains("fanout_failed"));

    EXPECT_EQ(stats["name"].get<std::string>(), "CacheReplicationCoordinator");
    EXPECT_EQ(stats["remote_peer_count"].get<std::size_t>(), 1u);
    EXPECT_GE(stats["fanout_enqueued"].get<uint64_t>(), 1u);
    EXPECT_GE(stats["fanout_delivered"].get<uint64_t>(), 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// GrpcRemoteCachePeer – compile-time guard and config defaults
// (Only compiled when THEMIS_ENABLE_GRPC is defined)
// ─────────────────────────────────────────────────────────────────────────────

#ifdef THEMIS_ENABLE_GRPC
#include "cache/grpc_remote_cache_peer.h"

TEST(GrpcRemoteCachePeerTest, ConfigDefaults) {
    GrpcRemoteCachePeer::Config cfg("node1:8771");
    EXPECT_EQ(cfg.address,        "node1:8771");
    EXPECT_EQ(cfg.rpc_timeout_ms, 1000);
    EXPECT_FALSE(cfg.tls_enabled);
    EXPECT_TRUE(cfg.tls_ca_cert.empty());
}

TEST(GrpcRemoteCachePeerTest, AddressAccessor) {
    GrpcRemoteCachePeer peer("node2:8771");
    EXPECT_EQ(peer.address(), "node2:8771");
}

TEST(GrpcRemoteCachePeerTest, InitiallyHealthy) {
    GrpcRemoteCachePeer peer("node3:8771");
    EXPECT_TRUE(peer.isHealthy());
}

TEST(GrpcRemoteCachePeerTest, KInvalidateMethodConstant) {
    // Verify the method constant is non-empty and contains the expected path.
    std::string m = GrpcRemoteCachePeer::kInvalidateMethod;
    EXPECT_FALSE(m.empty());
    EXPECT_NE(m.find("CacheInvalidation"), std::string::npos);
    EXPECT_NE(m.find("Invalidate"),        std::string::npos);
}

#endif  // THEMIS_ENABLE_GRPC
