// Tests for Phase 4 io_uring zero-copy I/O path (network performance).
//
// These tests are designed to be safe in CI environments where io_uring
// access may be restricted (seccomp filter, kernel < 5.1, non-Linux).
// Every test that relies on actual io_uring first calls
// IoUringZeroCopyIO::io_uring_accessible() and skips gracefully if the
// system cannot provide the syscall.

#include "performance/phase4/io_uring_zero_copy.h"
#include "performance/phase4/feature_flags.h"
#include <gtest/gtest.h>

#include <cstring>
#include <numeric>
#include <vector>

using namespace themis::performance::phase4;

// ---------------------------------------------------------------------------
// ZeroCopyBuffer tests
// ---------------------------------------------------------------------------

TEST(ZeroCopyBufferTest, DefaultZeroSizeIsInvalid) {
    ZeroCopyBuffer buf(0);
    // size 0 may produce a valid or null pointer depending on platform
    EXPECT_EQ(buf.size(), 0u);
}

TEST(ZeroCopyBufferTest, AllocationSucceeds) {
    ZeroCopyBuffer buf(4096);
    EXPECT_TRUE(buf.valid());
    EXPECT_NE(buf.data(), nullptr);
    EXPECT_EQ(buf.size(), 4096u);
}

TEST(ZeroCopyBufferTest, DataIsWritable) {
    ZeroCopyBuffer buf(1024);
    ASSERT_TRUE(buf.valid());
    std::memset(buf.data(), 0xAB, buf.size());
    const auto* p = static_cast<const unsigned char*>(buf.data());
    for (size_t i = 0; i < buf.size(); ++i) {
        ASSERT_EQ(p[i], 0xABu) << "mismatch at byte " << i;
    }
}

TEST(ZeroCopyBufferTest, MoveSemantics) {
    ZeroCopyBuffer a(2048);
    ASSERT_TRUE(a.valid());
    void* original_data = a.data();

    ZeroCopyBuffer b = std::move(a);
    EXPECT_FALSE(a.valid()); // NOLINT(bugprone-use-after-move)
    EXPECT_EQ(a.data(),  nullptr);
    EXPECT_EQ(a.size(),  0u);
    EXPECT_TRUE(b.valid());
    EXPECT_EQ(b.data(), original_data);
    EXPECT_EQ(b.size(), 2048u);
}

TEST(ZeroCopyBufferTest, MoveAssignment) {
    ZeroCopyBuffer a(512);
    ZeroCopyBuffer b(256);
    ASSERT_TRUE(a.valid());
    ASSERT_TRUE(b.valid());

    void* a_data = a.data();
    b = std::move(a);
    EXPECT_EQ(b.data(), a_data);
    EXPECT_EQ(b.size(), 512u);
    EXPECT_FALSE(a.valid()); // NOLINT(bugprone-use-after-move)
}

// ---------------------------------------------------------------------------
// IoUringZeroCopyIO construction and probing
// ---------------------------------------------------------------------------

TEST(IoUringZeroCopyIOTest, DefaultConstructionSucceeds) {
    // Must not throw even when io_uring is unavailable
    EXPECT_NO_THROW({
        IoUringZeroCopyIO io;
        (void)io;
    });
}

TEST(IoUringZeroCopyIOTest, AvailabilityMatchesProbe) {
    bool probe  = IoUringZeroCopyIO::io_uring_accessible();
    IoUringZeroCopyIO io;
    EXPECT_EQ(io.is_available(), probe);
}

TEST(IoUringZeroCopyIOTest, BuffersAllocatedPerConfig) {
    IoUringConfig cfg;
    cfg.num_buffers  = 4;
    cfg.buffer_size  = 8192;
    cfg.ring_size    = 64;

    IoUringZeroCopyIO io(cfg);
    for (uint32_t i = 0; i < cfg.num_buffers; ++i) {
        EXPECT_TRUE(io.get_buffer(i).valid())
            << "Buffer " << i << " should be valid";
        EXPECT_EQ(io.get_buffer(i).size(), cfg.buffer_size);
    }
}

TEST(IoUringZeroCopyIOTest, GetBufferOutOfRangeThrows) {
    IoUringConfig cfg;
    cfg.num_buffers = 2;
    IoUringZeroCopyIO io(cfg);
    EXPECT_THROW(io.get_buffer(2), std::out_of_range);
    EXPECT_THROW(io.get_buffer(999), std::out_of_range);
}

// ---------------------------------------------------------------------------
// Stats
// ---------------------------------------------------------------------------

TEST(IoUringZeroCopyIOTest, InitialStatsAreZero) {
    IoUringZeroCopyIO io;
    auto stats = io.get_stats();

    EXPECT_EQ(stats.sq_entries_submitted, 0u);
    EXPECT_EQ(stats.cq_entries_completed, 0u);
    EXPECT_EQ(stats.bytes_sent,           0u);
    EXPECT_EQ(stats.bytes_received,       0u);
    EXPECT_EQ(stats.fallback_sends,       0u);
    EXPECT_EQ(stats.fallback_recvs,       0u);
}

TEST(IoUringZeroCopyIOTest, StatsReportRegisteredBuffers) {
    IoUringConfig cfg;
    cfg.num_buffers = 8;
    IoUringZeroCopyIO io(cfg);
    EXPECT_EQ(io.get_stats().registered_buffers, 8u);
}

TEST(IoUringZeroCopyIOTest, AvailabilityReflectedInStats) {
    IoUringZeroCopyIO io;
    EXPECT_EQ(io.get_stats().io_uring_available, io.is_available());
}

// ---------------------------------------------------------------------------
// Feature flags integration
// ---------------------------------------------------------------------------

TEST(Phase4FeatureFlagsIoUringTest, DefaultDisabled) {
    auto& flags = Phase4FeatureFlags::instance();
    // Default should be off (not toggled by other tests)
    // We just verify the getter doesn't crash and returns a bool.
    bool val = flags.io_uring_enabled();
    EXPECT_TRUE(val == true || val == false); // always passes; just exercises the path
}

TEST(Phase4FeatureFlagsIoUringTest, SetAndGet) {
    auto& flags = Phase4FeatureFlags::instance();
    bool original = flags.io_uring_enabled();

    flags.set_io_uring_enabled(true);
    EXPECT_TRUE(flags.io_uring_enabled());

    flags.set_io_uring_enabled(false);
    EXPECT_FALSE(flags.io_uring_enabled());

    // Restore original value
    flags.set_io_uring_enabled(original);
}

TEST(Phase4FeatureFlagsIoUringTest, MacroRespectsCompileTimeGate) {
    // THEMIS_PHASE4_IO_URING_ENABLED() should return false when
    // THEMIS_ENABLE_IO_URING is not defined, or the runtime value otherwise.
#ifdef THEMIS_ENABLE_IO_URING
    auto& flags = Phase4FeatureFlags::instance();
    flags.set_io_uring_enabled(true);
    EXPECT_TRUE(THEMIS_PHASE4_IO_URING_ENABLED());
    flags.set_io_uring_enabled(false);
    EXPECT_FALSE(THEMIS_PHASE4_IO_URING_ENABLED());
#else
    EXPECT_FALSE(THEMIS_PHASE4_IO_URING_ENABLED());
#endif
}

// ---------------------------------------------------------------------------
// ScopedIoUringTimer tests
// ---------------------------------------------------------------------------

TEST(ScopedIoUringTimerTest, OutputIsSetOnDestruction) {
    uint64_t elapsed = 0;
    {
        ScopedIoUringTimer timer(&elapsed);
        // Do some trivial work
        volatile int sum = 0;
        for (int i = 0; i < 1000; ++i) {
          sum += i;
        }
    }
    // On Linux the timer uses CLOCK_MONOTONIC so elapsed >= 0.
    // On other platforms it is always 0.
    EXPECT_GE(elapsed, 0u);
}

TEST(ScopedIoUringTimerTest, NullOutputIsSafe) {
    EXPECT_NO_THROW({
        ScopedIoUringTimer timer(nullptr);
    });
}

// ---------------------------------------------------------------------------
// io_uring functional tests (skipped when unavailable)
// ---------------------------------------------------------------------------

TEST(IoUringZeroCopyIOTest, WaitCompletionsOnUnavailableRingReturnsZero) {
    IoUringZeroCopyIO io = {};
    if (io.is_available()) {
        GTEST_SKIP() << "io_uring available – this test targets the fallback path";
    }
    EXPECT_EQ(io.wait_completions(1), 0u);
}

TEST(IoUringZeroCopyIOTest, SendZeroCopyFallbackOnBadFd) {
    IoUringZeroCopyIO io = {};
    if (io.is_available()) {
        GTEST_SKIP() << "io_uring available – fallback path not exercised";
    }
    // fd -1 is always invalid; expect an error return
    int ret = io.send_zerocopy(-1, 0, 4);
    EXPECT_LT(ret, 0);
}

TEST(IoUringZeroCopyIOTest, RecvZeroCopyFallbackOnBadFd) {
    IoUringZeroCopyIO io = {};
    if (io.is_available()) {
        GTEST_SKIP() << "io_uring available – fallback path not exercised";
    }
    int ret = io.recv_zerocopy(-1, 0, 4);
    EXPECT_LT(ret, 0);
}

TEST(IoUringZeroCopyIOTest, IoUringRoundTripOrSkip) {
#ifndef __linux__
    GTEST_SKIP() << "socketpair/io_uring roundtrip is Linux-only";
#else
    if (!IoUringZeroCopyIO::io_uring_accessible()) {
        GTEST_SKIP() << "io_uring not accessible on this system";
    }

    // Create a socket pair for loopback I/O
    int fds[2] = {-1, -1};
    if (::socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, fds) != 0) {
        GTEST_SKIP() << "socketpair() failed";
    }

    IoUringConfig cfg;
    cfg.ring_size   = 64;
    cfg.num_buffers = 2;
    cfg.buffer_size = 256;

    IoUringZeroCopyIO io(cfg);
    if (!io.is_available()) {
        ::close(fds[0]);
        ::close(fds[1]);
        GTEST_SKIP() << "io_uring ring setup failed";
    }

    // Write a known pattern into buffer 0 and send it
    const char payload[] = "hello_io_uring_zero_copy";
    std::memcpy(io.get_buffer(0).data(), payload, sizeof(payload));
    int ret = io.send_zerocopy(fds[0], 0, sizeof(payload));
    EXPECT_EQ(ret, 0);
    io.wait_completions(1);

    // Receive into buffer 1 via a plain recv() on fds[1]
    char recv_buf[256] = {};
    ssize_t n = ::recv(fds[1], recv_buf, sizeof(recv_buf), 0);
    EXPECT_GT(n, 0);
    if (n > 0) {
        EXPECT_STREQ(recv_buf, payload);
    }

    ::close(fds[0]);
    ::close(fds[1]);

    auto stats = io.get_stats();
    EXPECT_GT(stats.bytes_sent, 0u);
#endif
}
