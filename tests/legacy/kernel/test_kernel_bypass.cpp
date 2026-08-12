/**
 * @file test_kernel_bypass.cpp
 * @brief Focused unit tests for Kernel Bypass (DPDK / io_uring) components.
 *
 * All tests are platform-portable.  DPDK-specific tests compile only when
 * THEMIS_ENABLE_DPDK is set; io_uring-specific tests need both
 * THEMIS_ENABLE_IO_URING and Linux.  In all other configurations the
 * tests verify the safe no-op/fallback behaviour.
 *
 * Test IDs: KBP-01 … KBP-30
 */

#include <gtest/gtest.h>
#include "network/kernel_bypass.h"

#include <array>
#include <atomic>
#include <chrono>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

using namespace themis::network;
using namespace std::chrono_literals;

// =============================================================================
// CpuPinner tests
// =============================================================================

// KBP-01 — logicalCpuCount returns at least 1.
TEST(KernelBypassTest, KBP01_LogicalCpuCountPositive) {
    int n = CpuPinner::logicalCpuCount();
    EXPECT_GE(n, 1);
}

// KBP-02 — currentCpu returns -1 (non-Linux) or a valid core index.
TEST(KernelBypassTest, KBP02_CurrentCpuValid) {
    int cpu = CpuPinner::currentCpu();
#ifdef __linux__
    int ncpu = CpuPinner::logicalCpuCount();
    // On Linux the result must be in [0, ncpu).
    EXPECT_GE(cpu, 0);
    EXPECT_LT(cpu, ncpu);
#else
    EXPECT_EQ(cpu, -1);
#endif
}

// KBP-03 — pinCallerToCore(0) does not crash.
TEST(KernelBypassTest, KBP03_PinCallerToCoreNoThrow) {
    EXPECT_NO_THROW(CpuPinner::pinCallerToCore(0));
}

// KBP-04 — pinCallerToCore with a negative ID returns false.
TEST(KernelBypassTest, KBP04_PinCallerToNegativeCoreReturnsFalse) {
    EXPECT_FALSE(CpuPinner::pinCallerToCore(-1));
}

// KBP-05 — numaNodeForCore(0) returns -1 or a non-negative node ID.
TEST(KernelBypassTest, KBP05_NumaNodeForCore0) {
    int node = CpuPinner::numaNodeForCore(0);
    // May be -1 on non-Linux, or 0+ on Linux.
    EXPECT_GE(node, -1);
}

// KBP-06 — coresOnNuma does not throw; result is consistent with
//          numaNodeForCore().
TEST(KernelBypassTest, KBP06_CoresOnNumaConsistency) {
    EXPECT_NO_THROW({
        auto cores = CpuPinner::coresOnNuma(0);
        for (int c : cores) {
            EXPECT_GE(c, 0);
        }
    });
}

// KBP-07 — pinThreadToCore assigns affinity without throwing.
TEST(KernelBypassTest, KBP07_PinThreadToCore) {
    bool pin_result = false;
    std::thread t([&pin_result]() {
        // Thread function just sleeps briefly; pinning is done externally.
        std::this_thread::sleep_for(1ms);
        pin_result = true;
    });
    CpuPinner::pinThreadToCore(t, 0);  // must not throw
    t.join();
    EXPECT_TRUE(pin_result);
}

// =============================================================================
// NumaAllocator tests
// =============================================================================

// KBP-08 — isNumaAvailable returns a consistent bool.
TEST(KernelBypassTest, KBP08_NumaAvailableConsistent) {
    bool first  = NumaAllocator::isNumaAvailable();
    bool second = NumaAllocator::isNumaAvailable();
    EXPECT_EQ(first, second);
}

// KBP-09 — allocate/deallocate round-trip with system default node.
TEST(KernelBypassTest, KBP09_AllocateDeallocateDefault) {
    constexpr size_t kSize = 1024;
    void* p = nullptr;
    ASSERT_NO_THROW(p = NumaAllocator::allocate(kSize, -1));
    ASSERT_NE(p, nullptr);
    // Verify the memory is writable.
    std::memset(p, 0xAB, kSize);
    EXPECT_EQ(static_cast<uint8_t*>(p)[0], 0xABu);
    EXPECT_EQ(static_cast<uint8_t*>(p)[kSize - 1], 0xABu);
    EXPECT_NO_THROW(NumaAllocator::deallocate(p, kSize));
}

// KBP-10 — allocate with size 0 throws std::bad_alloc.
TEST(KernelBypassTest, KBP10_AllocateZeroThrows) {
    EXPECT_THROW(NumaAllocator::allocate(0), std::bad_alloc);
}

// KBP-11 — deallocate(nullptr, ...) is a no-op.
TEST(KernelBypassTest, KBP11_DeallocateNullIsNoOp) {
    EXPECT_NO_THROW(NumaAllocator::deallocate(nullptr, 0));
    EXPECT_NO_THROW(NumaAllocator::deallocate(nullptr, 4096));
}

// =============================================================================
// ZeroCopyDmaBuffer tests
// =============================================================================

// KBP-12 — Construction with a small size succeeds.
TEST(KernelBypassTest, KBP12_DmaBufferSmallAlloc) {
    ZeroCopyDmaBuffer buf(4096);
    EXPECT_TRUE(buf.valid());
    EXPECT_NE(buf.data(), nullptr);
    EXPECT_GE(buf.size(), static_cast<size_t>(4096));
}

// KBP-13 — Buffer is writable.
TEST(KernelBypassTest, KBP13_DmaBufferWritable) {
    ZeroCopyDmaBuffer buf(512);
    ASSERT_TRUE(buf.valid());
    auto* p = static_cast<uint8_t*>(buf.data());
    std::memset(p, 0x55, 512);
    EXPECT_EQ(p[0],   0x55u);
    EXPECT_EQ(p[511], 0x55u);
}

// KBP-14 — isHugePage() returns a bool (true on Linux with huge pages).
TEST(KernelBypassTest, KBP14_DmaBufferHugePageFlag) {
    ZeroCopyDmaBuffer buf(2 * 1024 * 1024 + 1);
    // Just check it returns a consistent bool — huge-page availability is
    // environment-dependent.
    bool hp = buf.isHugePage();
    EXPECT_EQ(hp, buf.isHugePage());
}

// KBP-15 — Move construction transfers ownership.
TEST(KernelBypassTest, KBP15_DmaBufferMoveConstruct) {
    ZeroCopyDmaBuffer a(1024);
    ASSERT_TRUE(a.valid());
    void* original_ptr = a.data();

    ZeroCopyDmaBuffer b(std::move(a));
    EXPECT_FALSE(a.valid());
    EXPECT_EQ(a.data(), nullptr);
    EXPECT_TRUE(b.valid());
    EXPECT_EQ(b.data(), original_ptr);
}

// KBP-16 — Move assignment transfers ownership.
TEST(KernelBypassTest, KBP16_DmaBufferMoveAssign) {
    ZeroCopyDmaBuffer a(1024);
    ZeroCopyDmaBuffer b(512);
    ASSERT_TRUE(a.valid());
    void* original_ptr = a.data();

    b = std::move(a);
    EXPECT_FALSE(a.valid());
    EXPECT_EQ(b.data(), original_ptr);
}

// =============================================================================
// DPDKServer config / API tests (no hardware required)
// =============================================================================

// KBP-17 — Default Config has sensible values.
TEST(KernelBypassTest, KBP17_DpdkConfigDefaults) {
    DPDKServer::Config cfg;
    EXPECT_EQ(cfg.port, static_cast<uint16_t>(8772));
    EXPECT_GE(cfg.num_rx_queues, static_cast<uint16_t>(1));
    EXPECT_GE(cfg.num_tx_queues, static_cast<uint16_t>(1));
    EXPECT_GT(cfg.huge_pages_mb, static_cast<uint32_t>(0));
    EXPECT_GT(cfg.mbuf_pool_size, static_cast<uint32_t>(0));
    EXPECT_GT(cfg.rx_burst_size, static_cast<uint16_t>(0));
    EXPECT_GT(cfg.tx_burst_size, static_cast<uint16_t>(0));
    EXPECT_GT(cfg.max_connections, static_cast<uint32_t>(0));
}

// KBP-18 — Default Stats are all zero.
TEST(KernelBypassTest, KBP18_DpdkStatsDefaultZero) {
    DPDKServer::Stats s{};
    EXPECT_EQ(s.rx_packets, 0u);
    EXPECT_EQ(s.tx_packets, 0u);
    EXPECT_EQ(s.rx_bytes,   0u);
    EXPECT_EQ(s.tx_bytes,   0u);
    EXPECT_EQ(s.rx_dropped, 0u);
    EXPECT_EQ(s.tx_errors,  0u);
    EXPECT_EQ(s.connections,0u);
    EXPECT_EQ(s.requests,   0u);
    EXPECT_EQ(s.poll_iterations, 0u);
}

// KBP-19 — Construction does not start the server.
TEST(KernelBypassTest, KBP19_DpdkConstructionNotRunning) {
    DPDKServer::Config cfg;
    DPDKServer srv(cfg);
    EXPECT_FALSE(srv.isRunning());
}

// KBP-20 — isDpdkAvailable returns a consistent bool.
TEST(KernelBypassTest, KBP20_DpdkAvailableConsistent) {
    bool a = DPDKServer::isDpdkAvailable();
    bool b = DPDKServer::isDpdkAvailable();
    EXPECT_EQ(a, b);
#ifdef THEMIS_ENABLE_DPDK
    EXPECT_TRUE(a);
#else
    EXPECT_FALSE(a);
#endif
}

// KBP-21 — start() fails gracefully when DPDK is unavailable.
TEST(KernelBypassTest, KBP21_DpdkStartFailsWithoutHardware) {
#ifndef THEMIS_ENABLE_DPDK
    DPDKServer::Config cfg;
    DPDKServer srv(cfg);
    bool ok = srv.start();
    EXPECT_FALSE(ok);
    EXPECT_FALSE(srv.isRunning());
    EXPECT_FALSE(srv.lastError().empty());
#else
    GTEST_SKIP() << "DPDK compiled in; hardware-dependent test skipped";
#endif
}

// KBP-22 — stop() is safe when not running.
TEST(KernelBypassTest, KBP22_DpdkStopWhenNotRunning) {
    DPDKServer::Config cfg;
    DPDKServer srv(cfg);
    EXPECT_NO_THROW(srv.stop());
    EXPECT_FALSE(srv.isRunning());
}

// KBP-23 — coresFromMask returns the correct core set.
TEST(KernelBypassTest, KBP23_DpdkCoresFromMask) {
    auto cores = DPDKServer::coresFromMask(0x0F);  // cores 0-3
    ASSERT_EQ(cores.size(), 4u);
    EXPECT_EQ(cores[0], 0);
    EXPECT_EQ(cores[1], 1);
    EXPECT_EQ(cores[2], 2);
    EXPECT_EQ(cores[3], 3);
}

TEST(KernelBypassTest, KBP23b_DpdkCoresFromMaskSingle) {
    auto cores = DPDKServer::coresFromMask(0x01);
    ASSERT_EQ(cores.size(), 1u);
    EXPECT_EQ(cores[0], 0);
}

TEST(KernelBypassTest, KBP23c_DpdkCoresFromMaskZero) {
    auto cores = DPDKServer::coresFromMask(0x00);
    EXPECT_TRUE(cores.empty());
}

// =============================================================================
// IoUringServer config / API tests
// =============================================================================

// KBP-24 — Default Config has sensible values.
TEST(KernelBypassTest, KBP24_IoUringConfigDefaults) {
    IoUringServer::Config cfg;
    EXPECT_EQ(cfg.port, static_cast<uint16_t>(8773));
    EXPECT_EQ(cfg.host, "0.0.0.0");
    EXPECT_GT(cfg.ring_size, static_cast<uint32_t>(0));
    EXPECT_GT(cfg.num_worker_threads, static_cast<uint32_t>(0));
    EXPECT_GT(cfg.max_connections, static_cast<uint32_t>(0));
    EXPECT_GT(cfg.recv_buf_size, static_cast<uint32_t>(0));
    EXPECT_GT(cfg.send_buf_size, static_cast<uint32_t>(0));
    EXPECT_GT(cfg.num_fixed_buffers, static_cast<uint32_t>(0));
}

// KBP-25 — ring_size default is a power of two.
TEST(KernelBypassTest, KBP25_IoUringRingSizePowerOfTwo) {
    IoUringServer::Config cfg;
    uint32_t rs = cfg.ring_size;
    EXPECT_GT(rs, static_cast<uint32_t>(0));
    EXPECT_EQ(rs & (rs - 1), static_cast<uint32_t>(0));
}

// KBP-26 — Default Stats are all zero.
TEST(KernelBypassTest, KBP26_IoUringStatsDefaultZero) {
    IoUringServer::Stats s{};
    EXPECT_EQ(s.accept_completions, 0u);
    EXPECT_EQ(s.recv_completions,   0u);
    EXPECT_EQ(s.send_completions,   0u);
    EXPECT_EQ(s.recv_bytes,         0u);
    EXPECT_EQ(s.send_bytes,         0u);
    EXPECT_EQ(s.errors,             0u);
    EXPECT_EQ(s.connections,        0u);
    EXPECT_EQ(s.requests,           0u);
    EXPECT_EQ(s.sq_poll_wakeups,    0u);
    EXPECT_EQ(s.zero_copy_sends,    0u);
}

// KBP-27 — Construction does not start the server.
TEST(KernelBypassTest, KBP27_IoUringConstructionNotRunning) {
    IoUringServer::Config cfg;
    IoUringServer srv(cfg);
    EXPECT_FALSE(srv.isRunning());
}

// KBP-28 — isIoUringAvailable returns a consistent bool.
TEST(KernelBypassTest, KBP28_IoUringAvailableConsistent) {
    bool a = IoUringServer::isIoUringAvailable();
    bool b = IoUringServer::isIoUringAvailable();
    EXPECT_EQ(a, b);
#if !defined(THEMIS_ENABLE_IO_URING) || !defined(__linux__)
    EXPECT_FALSE(a);
#endif
}

// KBP-29 — start() fails gracefully when io_uring is unavailable.
TEST(KernelBypassTest, KBP29_IoUringStartFailsWithoutSupport) {
#if !defined(THEMIS_ENABLE_IO_URING) || !defined(__linux__)
    IoUringServer::Config cfg;
    IoUringServer srv(cfg);
    bool ok = srv.start();
    EXPECT_FALSE(ok);
    EXPECT_FALSE(srv.isRunning());
    EXPECT_FALSE(srv.lastError().empty());
#else
    GTEST_SKIP() << "io_uring compiled in; hardware-dependent test skipped";
#endif
}

// KBP-30 — stop() is safe when not running.
TEST(KernelBypassTest, KBP30_IoUringStopWhenNotRunning) {
    IoUringServer::Config cfg;
    IoUringServer srv(cfg);
    EXPECT_NO_THROW(srv.stop());
    EXPECT_FALSE(srv.isRunning());
}

// KBP-31 — ioUringVersion returns 0 when unavailable.
TEST(KernelBypassTest, KBP31_IoUringVersionUnavailable) {
#if !defined(THEMIS_ENABLE_IO_URING) || !defined(__linux__)
    EXPECT_EQ(IoUringServer::ioUringVersion(), static_cast<uint32_t>(0));
#else
    // When available, just verify it doesn't crash.
    EXPECT_NO_THROW(IoUringServer::ioUringVersion());
#endif
}

// KBP-32 — stats() on a freshly constructed (not-started) server returns zeros.
TEST(KernelBypassTest, KBP32_IoUringStatsOnUnstartedServer) {
    IoUringServer::Config cfg;
    IoUringServer srv(cfg);
    auto s = srv.stats();
    EXPECT_EQ(s.accept_completions, 0u);
    EXPECT_EQ(s.recv_completions,   0u);
    EXPECT_EQ(s.errors,             0u);
}

// KBP-33 — DPDKServer stats() on unstarted server returns zeros.
TEST(KernelBypassTest, KBP33_DpdkStatsOnUnstartedServer) {
    DPDKServer::Config cfg;
    DPDKServer srv(cfg);
    auto s = srv.stats();
    EXPECT_EQ(s.rx_packets, 0u);
    EXPECT_EQ(s.tx_packets, 0u);
    EXPECT_EQ(s.tx_errors,  0u);
    EXPECT_EQ(s.rx_dropped, 0u);
}

// KBP-34 — DPDKServer: custom Config fields are stored correctly.
TEST(KernelBypassTest, KBP34_DpdkConfigCustomFields) {
    DPDKServer::Config cfg;
    cfg.port            = 9999;
    cfg.pci_address     = "0000:03:00.1";
    cfg.num_rx_queues   = 8;
    cfg.num_tx_queues   = 8;
    cfg.cpu_core_mask   = 0xFF;
    cfg.huge_pages_mb   = 4096;
    cfg.enable_rss      = false;
    cfg.enable_hw_checksum = false;

    DPDKServer srv(cfg);
    EXPECT_EQ(srv.config().port,          9999u);
    EXPECT_EQ(srv.config().pci_address,   "0000:03:00.1");
    EXPECT_EQ(srv.config().num_rx_queues, 8u);
    EXPECT_EQ(srv.config().num_tx_queues, 8u);
    EXPECT_EQ(srv.config().cpu_core_mask, 0xFFu);
    EXPECT_EQ(srv.config().huge_pages_mb, 4096u);
    EXPECT_FALSE(srv.config().enable_rss);
    EXPECT_FALSE(srv.config().enable_hw_checksum);
}

// KBP-35 — IoUringServer: custom Config fields are stored correctly.
TEST(KernelBypassTest, KBP35_IoUringConfigCustomFields) {
    IoUringServer::Config cfg;
    cfg.host              = "127.0.0.1";
    cfg.port              = 7777;
    cfg.ring_size         = 1024;
    cfg.sq_thread_cpu     = 3;
    cfg.sq_thread_idle_ms = 500;
    cfg.num_worker_threads = 8;
    cfg.max_connections    = 512;

    IoUringServer srv(cfg);
    EXPECT_EQ(srv.config().host,               "127.0.0.1");
    EXPECT_EQ(srv.config().port,               7777u);
    EXPECT_EQ(srv.config().ring_size,          1024u);
    EXPECT_EQ(srv.config().sq_thread_cpu,      3);
    EXPECT_EQ(srv.config().sq_thread_idle_ms,  500u);
    EXPECT_EQ(srv.config().num_worker_threads, 8u);
    EXPECT_EQ(srv.config().max_connections,    512u);
}

// KBP-36 — Calling start() twice returns false on the second call.
TEST(KernelBypassTest, KBP36_DpdkDoubleStartReturnsFalse) {
#ifndef THEMIS_ENABLE_DPDK
    DPDKServer::Config cfg;
    DPDKServer srv(cfg);
    // First start() returns false (no DPDK).
    EXPECT_FALSE(srv.start());
    // Second start() also returns false (not running from the previous attempt).
    EXPECT_FALSE(srv.start());
#else
    GTEST_SKIP() << "DPDK compiled in; test skipped";
#endif
}

// KBP-37 — ZeroCopyDmaBuffer: NUMA node parameter does not crash.
TEST(KernelBypassTest, KBP37_DmaBufferNumaNode) {
    // NUMA node 0 is almost always valid.
    EXPECT_NO_THROW({
        ZeroCopyDmaBuffer buf(4096, 0);
        if (buf.valid()) {
            EXPECT_GE(buf.size(), static_cast<size_t>(4096));
        }
    });
}

// KBP-38 — CpuPinner::pinCallerToCore returns true for core 0 on Linux.
TEST(KernelBypassTest, KBP38_PinCallerToCore0OnLinux) {
#ifdef __linux__
    bool ok = CpuPinner::pinCallerToCore(0);
    EXPECT_TRUE(ok);
    // Restore to all-cores affinity.
    cpu_set_t set;
    CPU_ZERO(&set);
    int ncpu = CpuPinner::logicalCpuCount();
    for (int i = 0; i < ncpu; ++i) CPU_SET(i, &set);
    ::sched_setaffinity(0, sizeof(set), &set);
#else
    GTEST_SKIP() << "Linux only";
#endif
}

// KBP-39 — NumaAllocator allocates with explicit NUMA node 0.
TEST(KernelBypassTest, KBP39_NumaAllocNode0) {
    constexpr size_t kSz = 256;
    void* p = nullptr;
    ASSERT_NO_THROW(p = NumaAllocator::allocate(kSz, 0));
    ASSERT_NE(p, nullptr);
    EXPECT_NO_THROW(NumaAllocator::deallocate(p, kSz));
}

// KBP-40 — DPDKServer lastError is empty before any start() attempt.
TEST(KernelBypassTest, KBP40_DpdkLastErrorEmptyBeforeStart) {
    DPDKServer::Config cfg;
    DPDKServer srv(cfg);
    EXPECT_TRUE(srv.lastError().empty());
}
