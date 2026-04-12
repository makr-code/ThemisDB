/**
 * @file test_io_uring_batcher.cpp
 * @brief Focused unit tests for IoUringBatchedSender.
 *
 * These tests exercise the public API and invariants of IoUringBatchedSender.
 * All socket I/O tests use a loopback socketpair so no real network is needed.
 * When io_uring is unavailable (non-Linux or build flag absent) the tests
 * verify the transparent writev(2) fallback path.
 *
 * Test IDs: IUB-01 … IUB-12
 */

#include <gtest/gtest.h>
#include "network/io_uring_batcher.h"
#include "network/wire_protocol_batch.h"

#include <array>
#include <cstring>
#include <string>
#include <vector>

#ifdef __linux__
#  include <sys/socket.h>
#  include <unistd.h>
#endif

namespace themis {
namespace network {
namespace {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Create a connected socket pair.  Returns {send_fd, recv_fd} or {-1,-1}.
static std::pair<int,int> makeSockPair() {
#ifdef __linux__
    int fds[2] = {-1, -1};
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0) {
        return {fds[0], fds[1]};
    }
#endif
    return {-1, -1};
}

/// Read exactly `n` bytes from fd into buf.  Returns false on error/EOF.
static bool readAll(int fd, void* buf, size_t n) {
#ifdef __linux__
    size_t done = 0;
    auto*  p    = static_cast<char*>(buf);
    while (done < n) {
        ssize_t r = ::read(fd, p + done, n - done);
        if (r <= 0) return false;
        done += static_cast<size_t>(r);
    }
    return true;
#else
    (void)fd; (void)buf; (void)n;
    return false;
#endif
}

// ---------------------------------------------------------------------------
// IUB-01  Default construction succeeds without throwing
// ---------------------------------------------------------------------------
TEST(IoUringBatchedSenderTest, IUB01_DefaultConstruction) {
    EXPECT_NO_THROW(IoUringBatchedSender sender);
}

// ---------------------------------------------------------------------------
// IUB-02  isAvailable() returns a consistent bool (not undefined)
// ---------------------------------------------------------------------------
TEST(IoUringBatchedSenderTest, IUB02_IsAvailableIsBool) {
    IoUringBatchedSender sender;
    bool avail = sender.isAvailable();
    // Just verify it is reachable and returns the same value twice.
    EXPECT_EQ(avail, sender.isAvailable());
}

// ---------------------------------------------------------------------------
// IUB-03  submitAndWait() on empty queue returns 0 without error
// ---------------------------------------------------------------------------
TEST(IoUringBatchedSenderTest, IUB03_SubmitWaitEmptyQueue) {
    IoUringBatchedSender sender;
    size_t bytes = sender.submitAndWait();
    EXPECT_EQ(bytes, 0u);
}

// ---------------------------------------------------------------------------
// IUB-04  enqueue() with no pending data in batcher returns true
// ---------------------------------------------------------------------------
TEST(IoUringBatchedSenderTest, IUB04_EnqueueNoPendingData) {
#ifdef __linux__
    auto [send_fd, recv_fd] = makeSockPair();
    if (send_fd < 0) GTEST_SKIP() << "socketpair unavailable";

    IoUringBatchedSender sender;
    WireProtocolBatcher  batcher(send_fd);
    // Nothing added to batcher — pending() == false, enqueue() should short-circuit.
    EXPECT_TRUE(sender.enqueue(batcher));

    ::close(send_fd);
    ::close(recv_fd);
#else
    GTEST_SKIP() << "Linux-only test";
#endif
}

// ---------------------------------------------------------------------------
// IUB-05  Single message enqueued and data arrives at receiver
// ---------------------------------------------------------------------------
TEST(IoUringBatchedSenderTest, IUB05_SingleMessageDelivered) {
#ifdef __linux__
    auto [send_fd, recv_fd] = makeSockPair();
    if (send_fd < 0) GTEST_SKIP() << "socketpair unavailable";

    const std::string msg = "hello-io_uring";
    IoUringBatchedSender sender;
    WireProtocolBatcher  batcher(send_fd);
    batcher.add(msg.data(), msg.size());

    EXPECT_TRUE(sender.enqueue(batcher));
    sender.submitAndWait();

    std::string received(msg.size(), '\0');
    EXPECT_TRUE(readAll(recv_fd, received.data(), msg.size()));
    EXPECT_EQ(received, msg);

    ::close(send_fd);
    ::close(recv_fd);
#else
    GTEST_SKIP() << "Linux-only test";
#endif
}

// ---------------------------------------------------------------------------
// IUB-06  Multiple batchers in one round: all data delivered
// ---------------------------------------------------------------------------
TEST(IoUringBatchedSenderTest, IUB06_MultipleBatchersOneRound) {
#ifdef __linux__
    constexpr size_t N = 4;
    std::array<int, N> send_fds, recv_fds;
    send_fds.fill(-1); recv_fds.fill(-1);

    for (size_t i = 0; i < N; ++i) {
        auto [s, r] = makeSockPair();
        if (s < 0) GTEST_SKIP() << "socketpair unavailable";
        send_fds[i] = s;
        recv_fds[i] = r;
    }

    const std::string payload = "PING";
    IoUringBatchedSender sender;

    std::vector<std::unique_ptr<WireProtocolBatcher>> batchers;
    for (size_t i = 0; i < N; ++i) {
        auto b = std::make_unique<WireProtocolBatcher>(send_fds[i]);
        b->add(payload.data(), payload.size());
        EXPECT_TRUE(sender.enqueue(*b));
        batchers.push_back(std::move(b));
    }
    sender.submitAndWait();

    for (size_t i = 0; i < N; ++i) {
        std::string buf(payload.size(), '\0');
        EXPECT_TRUE(readAll(recv_fds[i], buf.data(), payload.size()))
            << "Failed on connection " << i;
        EXPECT_EQ(buf, payload) << "Data mismatch on connection " << i;
        ::close(send_fds[i]);
        ::close(recv_fds[i]);
    }
#else
    GTEST_SKIP() << "Linux-only test";
#endif
}

// ---------------------------------------------------------------------------
// IUB-07  lastStats() reflects submitted bytes after submitAndWait()
// ---------------------------------------------------------------------------
TEST(IoUringBatchedSenderTest, IUB07_LastStatsBytes) {
#ifdef __linux__
    auto [send_fd, recv_fd] = makeSockPair();
    if (send_fd < 0) GTEST_SKIP() << "socketpair unavailable";

    const std::string msg = "stats-test-payload";
    IoUringBatchedSender sender;
    WireProtocolBatcher  batcher(send_fd);
    batcher.add(msg.data(), msg.size());
    sender.enqueue(batcher);
    sender.submitAndWait();

    EXPECT_GE(sender.lastStats().bytes_sent, msg.size());

    ::close(send_fd);
    ::close(recv_fd);
#else
    GTEST_SKIP() << "Linux-only test";
#endif
}

// ---------------------------------------------------------------------------
// IUB-08  totalStats() is monotonically increasing across rounds
// ---------------------------------------------------------------------------
TEST(IoUringBatchedSenderTest, IUB08_TotalStatsMonotonic) {
#ifdef __linux__
    auto [s1, r1] = makeSockPair();
    auto [s2, r2] = makeSockPair();
    if (s1 < 0 || s2 < 0) GTEST_SKIP() << "socketpair unavailable";

    const std::string msg = "round";
    IoUringBatchedSender sender;

    WireProtocolBatcher b1(s1);
    b1.add(msg.data(), msg.size());
    sender.enqueue(b1);
    sender.submitAndWait();
    size_t bytes_after_r1 = sender.totalStats().bytes_sent;
    EXPECT_GE(bytes_after_r1, msg.size());

    WireProtocolBatcher b2(s2);
    b2.add(msg.data(), msg.size());
    sender.enqueue(b2);
    sender.submitAndWait();
    EXPECT_GE(sender.totalStats().bytes_sent, bytes_after_r1);

    ::close(s1); ::close(r1);
    ::close(s2); ::close(r2);
#else
    GTEST_SKIP() << "Linux-only test";
#endif
}

// ---------------------------------------------------------------------------
// IUB-09  rounds counter increments on each submitAndWait()
// ---------------------------------------------------------------------------
TEST(IoUringBatchedSenderTest, IUB09_RoundsCounter) {
    IoUringBatchedSender sender;
    EXPECT_EQ(sender.totalStats().rounds, 0u);
    sender.submitAndWait();
    EXPECT_EQ(sender.totalStats().rounds, 1u);
    sender.submitAndWait();
    EXPECT_EQ(sender.totalStats().rounds, 2u);
}

// ---------------------------------------------------------------------------
// IUB-10  Batcher with multiple iov entries (multi-add) delivers full payload
// ---------------------------------------------------------------------------
TEST(IoUringBatchedSenderTest, IUB10_BatcherMultiAdd) {
#ifdef __linux__
    auto [send_fd, recv_fd] = makeSockPair();
    if (send_fd < 0) GTEST_SKIP() << "socketpair unavailable";

    const std::string part1 = "HEADER:";
    const std::string part2 = "BODY_DATA";
    const std::string expected = part1 + part2;

    IoUringBatchedSender sender;
    WireProtocolBatcher  batcher(send_fd);
    batcher.add(part1.data(), part1.size());
    batcher.add(part2.data(), part2.size());
    sender.enqueue(batcher);
    sender.submitAndWait();

    std::string received(expected.size(), '\0');
    EXPECT_TRUE(readAll(recv_fd, received.data(), expected.size()));
    EXPECT_EQ(received, expected);

    ::close(send_fd);
    ::close(recv_fd);
#else
    GTEST_SKIP() << "Linux-only test";
#endif
}

// ---------------------------------------------------------------------------
// IUB-11  Non-Linux: isAvailable() always false, enqueue falls back silently
// ---------------------------------------------------------------------------
TEST(IoUringBatchedSenderTest, IUB11_NonLinuxFallbackCompiles) {
    // This test is intentionally trivial on non-Linux; it ensures the code
    // compiles and the fallback path is exercised.
#ifndef __linux__
    IoUringBatchedSender sender;
    EXPECT_FALSE(sender.isAvailable());
    // Enqueue with invalid fd must not crash (no socket on non-Linux path).
    WireProtocolBatcher batcher(-1);
    // batcher has nothing pending, so enqueue is a no-op.
    EXPECT_TRUE(sender.enqueue(batcher));
    sender.submitAndWait();
#else
    GTEST_SKIP() << "Linux platform — see IUB-02 for availability check";
#endif
}

// ---------------------------------------------------------------------------
// IUB-12  Destruction while pending entries exist does not crash
// ---------------------------------------------------------------------------
TEST(IoUringBatchedSenderTest, IUB12_DestructionWithPending) {
#ifdef __linux__
    auto [send_fd, recv_fd] = makeSockPair();
    if (send_fd < 0) GTEST_SKIP() << "socketpair unavailable";

    const std::string msg = "will-be-flushed-by-enqueue";
    {
        IoUringBatchedSender sender;
        WireProtocolBatcher  batcher(send_fd);
        batcher.add(msg.data(), msg.size());
        sender.enqueue(batcher);
        // Let sender go out of scope without calling submitAndWait().
        // The enqueue() call already flushed synchronously in fallback mode
        // or via the ring in io_uring mode.  Either way, no crash.
    }
    ::close(send_fd);
    ::close(recv_fd);
#else
    GTEST_SKIP() << "Linux-only test";
#endif
}

} // namespace
} // namespace network
} // namespace themis
