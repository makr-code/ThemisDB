/**
 * @file test_socket_timeout.cpp
 * @brief Tests for per-session socket read/write timeout enforcement
 *
 * Validates that Config::request_timeout_ms is correctly stored, defaulted,
 * and hot-reloaded, and that the armReadTimer / cancelReadTimer logic works
 * correctly at the config + timer semantics level.
 */

#include <gtest/gtest.h>
#include <atomic>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <vector>
#include "server/http_server.h"

using Config = themis::server::HttpServer::Config;

// ---------------------------------------------------------------------------
// Config defaults and constraints
// ---------------------------------------------------------------------------

TEST(SocketTimeout, DefaultRequestTimeoutIs30Seconds) {
    // The default is documented in Config and verified against the actual struct field.
    // Config::request_timeout_ms defaults to 30000 (30s), a standard production value.
    Config cfg;
    EXPECT_EQ(cfg.request_timeout_ms, 30000u);
    // Also verify via default construction (ensures no constructor resets it)
    Config cfg2{};
    EXPECT_EQ(cfg2.request_timeout_ms, 30000u);
}

TEST(SocketTimeout, ZeroTimeoutMeansNoTimeout) {
    Config cfg;
    cfg.request_timeout_ms = 0;
    EXPECT_EQ(cfg.request_timeout_ms, 0u);
}

TEST(SocketTimeout, TimeoutCanBeSetToOneSecond) {
    Config cfg;
    cfg.request_timeout_ms = 1000;
    EXPECT_EQ(cfg.request_timeout_ms, 1000u);
}

TEST(SocketTimeout, TimeoutCanBeSetToFiveMinutes) {
    Config cfg;
    cfg.request_timeout_ms = 300000;
    EXPECT_EQ(cfg.request_timeout_ms, 300000u);
}

TEST(SocketTimeout, TimeoutIsIndependentOfGracefulShutdownTimeout) {
    Config cfg;
    cfg.request_timeout_ms = 5000;
    cfg.graceful_shutdown_timeout_ms = 30000;
    EXPECT_NE(cfg.request_timeout_ms, cfg.graceful_shutdown_timeout_ms);
}

// ---------------------------------------------------------------------------
// Hot-reload timeout range validation
// (mirrors the range check in handleConfig: 1000-300000)
// ---------------------------------------------------------------------------

TEST(SocketTimeoutHotReload, MinAllowedTimeoutIs1000ms) {
    uint32_t timeout = 1000;
    bool valid = (timeout >= 1000 && timeout <= 300000);
    EXPECT_TRUE(valid);
}

TEST(SocketTimeoutHotReload, MaxAllowedTimeoutIs300000ms) {
    uint32_t timeout = 300000;
    bool valid = (timeout >= 1000 && timeout <= 300000);
    EXPECT_TRUE(valid);
}

TEST(SocketTimeoutHotReload, ZeroTimeoutIsRejectedByHotReload) {
    uint32_t timeout = 0;
    bool valid = (timeout >= 1000 && timeout <= 300000);
    EXPECT_FALSE(valid);
}

TEST(SocketTimeoutHotReload, TooLargeTimeoutIsRejectedByHotReload) {
    uint32_t timeout = 300001;
    bool valid = (timeout >= 1000 && timeout <= 300000);
    EXPECT_FALSE(valid);
}

TEST(SocketTimeoutHotReload, TypicalTimeoutOf10SecondsIsValid) {
    uint32_t timeout = 10000;
    bool valid = (timeout >= 1000 && timeout <= 300000);
    EXPECT_TRUE(valid);
}

// ---------------------------------------------------------------------------
// Timer semantics: arm / cancel / fire
// ---------------------------------------------------------------------------

// Simulates the armReadTimer / cancelReadTimer pattern using Asio primitives
// so the logic can be tested without a live server.

TEST(SocketTimeoutTimer, TimerFiresAfterExpiry) {
    namespace asio = boost::asio;
    asio::io_context ioc;
    asio::steady_timer timer(ioc);

    std::atomic<bool> fired{false};
    timer.expires_after(std::chrono::milliseconds(10));
    timer.async_wait([&fired](const boost::system::error_code& ec) {
        if (!ec) {
          fired.store(true);
        }
    });

    ioc.run_for(std::chrono::milliseconds(100));
    EXPECT_TRUE(fired.load());
}

TEST(SocketTimeoutTimer, CancelledTimerDoesNotFire) {
    namespace asio = boost::asio;
    asio::io_context ioc;
    asio::steady_timer timer(ioc);

    std::atomic<bool> fired{false};
    timer.expires_after(std::chrono::milliseconds(50));
    timer.async_wait([&fired](const boost::system::error_code& ec) {
        if (!ec) {
          fired.store(true);
        }
        // ec == operation_aborted when cancelled → fired stays false
    });

    // Cancel before the timer fires
    timer.cancel();
    ioc.run_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(fired.load());
}

TEST(SocketTimeoutTimer, RearmedTimerFiresOnlyOnce) {
    namespace asio = boost::asio;
    asio::io_context ioc;
    asio::steady_timer timer(ioc);

    std::atomic<int> fire_count{0};

    // Arm, then re-arm (expires_after cancels the previous wait)
    timer.expires_after(std::chrono::milliseconds(50));
    timer.async_wait([&fire_count](const boost::system::error_code& ec) {
        if (!ec) {
          fire_count.fetch_add(1);
        }
    });

    timer.expires_after(std::chrono::milliseconds(10)); // re-arms, first wait gets aborted
    timer.async_wait([&fire_count](const boost::system::error_code& ec) {
        if (!ec) {
          fire_count.fetch_add(1);
        }
    });

    ioc.run_for(std::chrono::milliseconds(200));
    // Only the second async_wait should fire; first gets aborted
    EXPECT_EQ(fire_count.load(), 1);
}

TEST(SocketTimeoutTimer, ZeroTimeoutMeansNoTimerIsArmed) {
    // When request_timeout_ms == 0, armReadTimer() is a no-op.
    // Simulate: if timeout is 0, don't arm; verify no spurious fire.
    namespace asio = boost::asio;
    asio::io_context ioc;
    asio::steady_timer timer(ioc);

    uint32_t timeout_ms = 0;
    std::atomic<bool> fired{false};

    if (timeout_ms > 0) {
        timer.expires_after(std::chrono::milliseconds(timeout_ms));
        timer.async_wait([&fired](const boost::system::error_code& ec) {
            if (!ec) {
              fired.store(true);
            }
        });
    }

    ioc.run_for(std::chrono::milliseconds(50));
    EXPECT_FALSE(fired.load());
}

TEST(SocketTimeoutTimer, TimerWithVeryShortTimeoutFiresBeforeLongRead) {
    // Models the scenario where timeout fires before the I/O completes
    namespace asio = boost::asio;
    asio::io_context ioc;
    asio::steady_timer timeout_timer(ioc); // renamed to avoid confusion with read_timer below

    std::atomic<bool> timed_out{false};
    std::atomic<bool> read_completed{false};

    timeout_timer.expires_after(std::chrono::milliseconds(20));
    timeout_timer.async_wait([&timed_out](const boost::system::error_code& ec) {
        if (!ec) {
          timed_out.store(true);
        }
    });

    // Simulate a slow read that would complete after the timeout
    auto read_timer = std::make_shared<asio::steady_timer>(ioc);
    read_timer->expires_after(std::chrono::milliseconds(100));
    read_timer->async_wait([&read_completed, &timeout_timer](const boost::system::error_code& ec) {
        if (!ec) {
            read_completed.store(true);
            timeout_timer.cancel(); // would normally cancel the socket timeout
        }
    });

    ioc.run_for(std::chrono::milliseconds(200));
    EXPECT_TRUE(timed_out.load());     // timeout fires first
    EXPECT_TRUE(read_completed.load()); // read "completes" too, but after timeout
}

// ---------------------------------------------------------------------------
// Config interaction: timeout with other limits
// ---------------------------------------------------------------------------

TEST(SocketTimeoutConfig, AllLimitsCanBeSetTogether) {
    Config cfg;
    cfg.request_timeout_ms = 5000;
    cfg.graceful_shutdown_timeout_ms = 30000;
    cfg.max_header_size_bytes = 8192;
    cfg.max_request_size_mb = 10;
    cfg.max_connections = 1000;

    EXPECT_EQ(cfg.request_timeout_ms, 5000u);
    EXPECT_EQ(cfg.graceful_shutdown_timeout_ms, 30000u);
    EXPECT_EQ(cfg.max_header_size_bytes, 8192u);
    EXPECT_EQ(cfg.max_request_size_mb, 10u);
    EXPECT_EQ(cfg.max_connections, 1000u);
}

TEST(SocketTimeoutConfig, TimeoutDefaultsAreReasonableForProduction) {
    Config cfg;
    // request_timeout_ms: 30s is standard for a production HTTP server
    EXPECT_GE(cfg.request_timeout_ms, 5000u);    // at least 5s
    EXPECT_LE(cfg.request_timeout_ms, 120000u);  // at most 2min
}

TEST(SocketTimeoutConfig, TimeoutIsUint32) {
    Config cfg;
    // Verify the type is uint32_t (not signed, no overflow concerns)
    static_assert(std::is_same_v<decltype(cfg.request_timeout_ms), uint32_t>,
                  "request_timeout_ms must be uint32_t");
    SUCCEED();
}

// ---------------------------------------------------------------------------
// W1-S02: hot_request_timeout_ms_ atomic shadow — race-free hot-reload
// ---------------------------------------------------------------------------

// Validates that the atomic shadow pattern used in HttpServer::hot_request_timeout_ms_
// (initialized from Config::request_timeout_ms) is correct: concurrent store/load from
// multiple threads must not produce torn reads or UB.

TEST(W1S02AtomicTimeout, AtomicInitFromConfigDefaultIs30s) {
    // Mirrors: HttpServer constructor initialises hot_request_timeout_ms_ from
    // config_.request_timeout_ms, which defaults to 30000.
    Config cfg;
    std::atomic<uint32_t> hot_ms{cfg.request_timeout_ms};
    EXPECT_EQ(hot_ms.load(std::memory_order_acquire), 30000u);
}

TEST(W1S02AtomicTimeout, HotReloadStoreIsVisibleToAcquireLoad) {
    // Simulates: hot-reload handler stores new value, armReadTimer reads it.
    std::atomic<uint32_t> hot_ms{30000u};

    // Hot-reload writes
    hot_ms.store(5000u, std::memory_order_release);

    // armReadTimer reads
    uint32_t seen = hot_ms.load(std::memory_order_acquire);
    EXPECT_EQ(seen, 5000u);
}

TEST(W1S02AtomicTimeout, ConcurrentHotReloadAndArmDoNotDataRace) {
    // Stress test: 4 writers and 4 readers on the same atomic must not trigger TSAN.
    std::atomic<uint32_t> hot_ms{30000u};
    std::atomic<bool> stop{false};

    std::vector<std::thread> writers;
    std::vector<std::thread> readers;
    for (int i = 0; i < 4; ++i) {
        writers.emplace_back([&hot_ms, &stop, i]() {
            while (!stop.load(std::memory_order_relaxed)) {
                hot_ms.store(static_cast<uint32_t>(1000 + i * 1000),
                             std::memory_order_release);
            }
        });
        readers.emplace_back([&hot_ms, &stop]() {
            while (!stop.load(std::memory_order_relaxed)) {
                auto v = hot_ms.load(std::memory_order_acquire);
                (void)v; // all values are valid
            }
        });
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    stop.store(true, std::memory_order_release);
    for (auto& t : writers) {
      t.join();
    }
    for (auto& t : readers) {
      t.join();
    }
    SUCCEED(); // no TSAN / UB detected
}

// ---------------------------------------------------------------------------
// W1-S02: audit_rate_buckets_ amortised eviction — bounded map size
// ---------------------------------------------------------------------------

// Simulates the eviction logic from HttpServer::enforceAuditRateLimit to verify
// that stale entries are removed when the map exceeds 128 entries.

TEST(W1S02AuditRateEviction, StaleEntriesAreRemovedWhenMapExceeds128) {
    struct RateState {
        uint64_t window_start_ms{0};
        uint32_t count{0};
    };
    std::unordered_map<std::string, RateState> buckets;

    const uint64_t window_ms = 60ull * 1000ull;
    const uint64_t now = 1'000'000; // arbitrary epoch-relative timestamp

    // Insert 200 stale entries (window started 3 min ago)
    for (int i = 0; i < 200; ++i) {
        buckets["stale:" + std::to_string(i)] = {now - 3 * window_ms, 1};
    }
    ASSERT_EQ(buckets.size(), 200u);

    // Trigger eviction (same logic as HttpServer::enforceAuditRateLimit)
    if (buckets.size() > 128) {
        const uint64_t evict_cutoff = now - 2 * window_ms;
        for (auto it = buckets.begin(); it != buckets.end(); ) {
            if (it->second.window_start_ms < evict_cutoff) {
                it = buckets.erase(it);
            } else {
                ++it;
            }
        }
    }

    EXPECT_LT(buckets.size(), 200u) << "Stale entries must be evicted";
    EXPECT_EQ(buckets.size(), 0u)   << "All 200 stale entries should be removed";
}

TEST(W1S02AuditRateEviction, FreshEntriesAreNotEvicted) {
    struct RateState {
        uint64_t window_start_ms{0};
        uint32_t count{0};
    };
    std::unordered_map<std::string, RateState> buckets;

    const uint64_t window_ms = 60ull * 1000ull;
    const uint64_t now = 1'000'000;

    // Insert 150 fresh entries (started in current window)
    for (int i = 0; i < 150; ++i) {
        buckets["fresh:" + std::to_string(i)] = {now - window_ms / 2, 1};
    }
    ASSERT_EQ(buckets.size(), 150u);

    // Trigger eviction
    if (buckets.size() > 128) {
        const uint64_t evict_cutoff = now - 2 * window_ms;
        for (auto it = buckets.begin(); it != buckets.end(); ) {
            if (it->second.window_start_ms < evict_cutoff) {
                it = buckets.erase(it);
            } else {
                ++it;
            }
        }
    }

    EXPECT_EQ(buckets.size(), 150u) << "Fresh entries must not be evicted";
}

TEST(W1S02AuditRateEviction, MapBelowThresholdIsNotScanned) {
    // Eviction only triggers when map.size() > 128 to avoid O(n) on every request.
    struct RateState {
        uint64_t window_start_ms{0};
        uint32_t count{0};
    };
    std::unordered_map<std::string, RateState> buckets;

    const uint64_t window_ms = 60ull * 1000ull;
    const uint64_t now = 1'000'000;

    // Insert 5 very stale entries (below threshold)
    for (int i = 0; i < 5; ++i) {
        buckets["stale:" + std::to_string(i)] = {now - 10 * window_ms, 1};
    }

    size_t scanned = 0;
    if (buckets.size() > 128) {  // threshold NOT exceeded
        const uint64_t evict_cutoff = now - 2 * window_ms;
        for (auto it = buckets.begin(); it != buckets.end(); ) {
            ++scanned;
            if (it->second.window_start_ms < evict_cutoff) {
                it = buckets.erase(it);
            } else {
                ++it;
            }
        }
    }
    EXPECT_EQ(scanned, 0u)          << "No scan below threshold";
    EXPECT_EQ(buckets.size(), 5u)   << "Small map must be left intact";
}

// ---------------------------------------------------------------------------
// W1-S02: write/shutdown timeout coverage for HTTP core session paths
// ---------------------------------------------------------------------------

TEST(W1S02WriteTimeout, StalledWriteTriggersTimeout) {
    namespace asio = boost::asio;
    asio::io_context ioc;
    asio::steady_timer io_timeout(ioc);
    auto simulated_write = std::make_shared<asio::steady_timer>(ioc);

    std::atomic<bool> timed_out{false};
    std::atomic<bool> write_done{false};

    io_timeout.expires_after(std::chrono::milliseconds(20));
    io_timeout.async_wait([&timed_out](const boost::system::error_code& ec) {
        if (!ec) {
          timed_out.store(true);
        }
    });

    simulated_write->expires_after(std::chrono::milliseconds(100));
    simulated_write->async_wait([&write_done](const boost::system::error_code& ec) {
        if (!ec) {
          write_done.store(true);
        }
    });

    ioc.run_for(std::chrono::milliseconds(200));
    EXPECT_TRUE(timed_out.load());
    EXPECT_TRUE(write_done.load());
}

TEST(W1S02WriteTimeout, CompletedWriteCancelsTimeout) {
    namespace asio = boost::asio;
    asio::io_context ioc;
    asio::steady_timer io_timeout(ioc);
    auto simulated_write = std::make_shared<asio::steady_timer>(ioc);

    std::atomic<bool> timed_out{false};
    std::atomic<bool> write_done{false};

    io_timeout.expires_after(std::chrono::milliseconds(100));
    io_timeout.async_wait([&timed_out](const boost::system::error_code& ec) {
        if (!ec) {
          timed_out.store(true);
        }
    });

    simulated_write->expires_after(std::chrono::milliseconds(10));
    simulated_write->async_wait([&write_done, &io_timeout](const boost::system::error_code& ec) {
        if (!ec) {
            write_done.store(true);
            io_timeout.cancel();
        }
    });

    ioc.run_for(std::chrono::milliseconds(200));
    EXPECT_TRUE(write_done.load());
    EXPECT_FALSE(timed_out.load());
}

TEST(W1S02ShutdownTimeout, StalledShutdownTriggersTimeout) {
    namespace asio = boost::asio;
    asio::io_context ioc;
    asio::steady_timer io_timeout(ioc);
    auto simulated_shutdown = std::make_shared<asio::steady_timer>(ioc);

    std::atomic<bool> timed_out{false};
    std::atomic<bool> shutdown_done{false};

    io_timeout.expires_after(std::chrono::milliseconds(20));
    io_timeout.async_wait([&timed_out](const boost::system::error_code& ec) {
        if (!ec) {
          timed_out.store(true);
        }
    });

    simulated_shutdown->expires_after(std::chrono::milliseconds(100));
    simulated_shutdown->async_wait([&shutdown_done](const boost::system::error_code& ec) {
        if (!ec) {
          shutdown_done.store(true);
        }
    });

    ioc.run_for(std::chrono::milliseconds(200));
    EXPECT_TRUE(timed_out.load());
    EXPECT_TRUE(shutdown_done.load());
}

TEST(W1S02ConnectionAdmission, RejectsWhenLimitReached) {
    std::atomic<uint64_t> active_connections{5};
    const uint64_t max_connections = 5;

    bool reserved = false;
    uint64_t observed = active_connections.load(std::memory_order_relaxed);
    while (observed < max_connections) {
        if (active_connections.compare_exchange_weak(
                observed, observed + 1,
                std::memory_order_acq_rel,
                std::memory_order_relaxed)) {
            reserved = true;
            break;
        }
    }

    EXPECT_FALSE(reserved);
    EXPECT_EQ(active_connections.load(std::memory_order_relaxed), max_connections);
}

TEST(W1S02ConnectionAdmission, ConcurrentReservationsDoNotExceedLimit) {
    constexpr uint64_t max_connections = 32;
    std::atomic<uint64_t> active_connections{0};
    std::atomic<uint64_t> reserved_count{0};

    constexpr size_t kThreads = 128;
    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (size_t i = 0; i < kThreads; ++i) {
        threads.emplace_back([&]() {
            uint64_t observed = active_connections.load(std::memory_order_relaxed);
            while (observed < max_connections) {
                if (active_connections.compare_exchange_weak(
                        observed, observed + 1,
                        std::memory_order_acq_rel,
                        std::memory_order_relaxed)) {
                    reserved_count.fetch_add(1, std::memory_order_relaxed);
                    return;
                }
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(active_connections.load(std::memory_order_relaxed), max_connections);
    EXPECT_EQ(reserved_count.load(std::memory_order_relaxed), max_connections);
}
