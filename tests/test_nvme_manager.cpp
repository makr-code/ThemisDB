/// @file test_nvme_manager.cpp
/// @brief Focused unit tests for NVMeManager (NVMe storage optimizations).
///
/// Tests cover:
///  - Default construction and configuration defaults
///  - Capability detection (non-crashing on any host)
///  - initialize() / shutdown() lifecycle (idempotent)
///  - Direct I/O flag recommendation logic
///  - Background thread count recommendation
///  - io_uring async I/O fallback path (synchronous pread/pwrite)
///  - ZNS management no-ops when ZNS is disabled
///  - RocksDBWrapper NVMe integration (config fields wired through)

#include <gtest/gtest.h>

#include "storage/nvme_manager.h"
#include "storage/rocksdb_wrapper.h"

#include <filesystem>
#include <fstream>
#include <cstring>
#include <string>
#include <thread>

// Platform-specific headers for POSIX I/O tests
#ifndef _WIN32
#  include <fcntl.h>
#  include <unistd.h>
#endif

namespace fs = std::filesystem;
using namespace themis::storage;

// ─────────────────────────────────────────────────────────────────────────────
// NVMeConfigTest – default values
// ─────────────────────────────────────────────────────────────────────────────

class NVMeConfigTest : public ::testing::Test {};

TEST_F(NVMeConfigTest, DefaultsAreSafe) {
    NVMeConfig cfg;
    EXPECT_FALSE(cfg.enable_io_uring);
    EXPECT_FALSE(cfg.enable_zns);
    EXPECT_FALSE(cfg.use_direct_reads);
    EXPECT_FALSE(cfg.use_direct_io_for_flush_and_compaction);
    EXPECT_EQ(cfg.io_uring_queue_depth, 128u);
    EXPECT_EQ(cfg.direct_io_alignment_bytes, 4096u);
    EXPECT_GT(cfg.zone_capacity_bytes, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// NVMeManagerLifecycleTest – construct / init / shutdown
// ─────────────────────────────────────────────────────────────────────────────

class NVMeManagerLifecycleTest : public ::testing::Test {};

TEST_F(NVMeManagerLifecycleTest, DefaultConstructDoesNotCrash) {
    EXPECT_NO_THROW({
        NVMeManager mgr;
    });
}

TEST_F(NVMeManagerLifecycleTest, InitializeSucceeds) {
    NVMeManager mgr;
    bool ok = mgr.initialize();
    // initialize() should always return true (falls back gracefully)
    EXPECT_TRUE(ok);
}

TEST_F(NVMeManagerLifecycleTest, InitializeIsIdempotent) {
    NVMeManager mgr;
    EXPECT_TRUE(mgr.initialize());
    EXPECT_TRUE(mgr.initialize());  // second call is a no-op
}

TEST_F(NVMeManagerLifecycleTest, ShutdownBeforeInitIsHarmless) {
    NVMeManager mgr;
    EXPECT_NO_THROW(mgr.shutdown());
}

TEST_F(NVMeManagerLifecycleTest, ShutdownAfterInitIsClean) {
    NVMeManager mgr;
    mgr.initialize();
    EXPECT_NO_THROW(mgr.shutdown());
}

TEST_F(NVMeManagerLifecycleTest, DestructorCallsShutdownImplicitly) {
    // Simply constructing and destroying must not crash
    EXPECT_NO_THROW({
        NVMeManager mgr;
        mgr.initialize();
        // destructor runs here
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// NVMeCapabilityTest – detectCapabilities()
// ─────────────────────────────────────────────────────────────────────────────

class NVMeCapabilityTest : public ::testing::Test {};

TEST_F(NVMeCapabilityTest, DetectDoesNotCrash) {
    NVMeManager mgr;
    EXPECT_NO_THROW({
        NVMeCapabilities caps = mgr.detectCapabilities();
        (void)caps;
    });
}

TEST_F(NVMeCapabilityTest, HwQueueCountIsPositive) {
    NVMeManager mgr;
    NVMeCapabilities caps = mgr.detectCapabilities();
    EXPECT_GE(caps.hw_queue_count, 1u);
}

TEST_F(NVMeCapabilityTest, DetectIsCached) {
    NVMeManager mgr;
    NVMeCapabilities c1 = mgr.detectCapabilities();
    NVMeCapabilities c2 = mgr.detectCapabilities();
    EXPECT_EQ(c1.io_uring_available,  c2.io_uring_available);
    EXPECT_EQ(c1.zns_available,       c2.zns_available);
    EXPECT_EQ(c1.hw_queue_count,      c2.hw_queue_count);
    EXPECT_EQ(c1.kernel_major,        c2.kernel_major);
}

TEST_F(NVMeCapabilityTest, NoDevicePathZnsAlwaysFalse) {
    NVMeConfig cfg;
    cfg.device_path = "";  // no path → cannot probe ZNS
    NVMeManager mgr(cfg);
    NVMeCapabilities caps = mgr.detectCapabilities();
    EXPECT_FALSE(caps.zns_available);
}

// ─────────────────────────────────────────────────────────────────────────────
// NVMeDirectIOTest – recommendedDirectIOFlags()
// ─────────────────────────────────────────────────────────────────────────────

class NVMeDirectIOTest : public ::testing::Test {};

TEST_F(NVMeDirectIOTest, BothFlagsFalseByDefault) {
    NVMeManager mgr;  // defaults: use_direct_reads=false, use_direct_io_for_flush=false
    mgr.initialize();
    auto [reads, flush] = mgr.recommendedDirectIOFlags();
    EXPECT_FALSE(reads);
    EXPECT_FALSE(flush);
}

TEST_F(NVMeDirectIOTest, ReadsOnlyWhenCapabilityPresent) {
    NVMeConfig cfg;
    cfg.use_direct_reads = true;
    NVMeManager mgr(cfg);
    mgr.initialize();
    auto [reads, flush] = mgr.recommendedDirectIOFlags();
    // If direct_io_available is false (e.g. tmpfs), reads must be false too
    NVMeCapabilities caps = mgr.detectCapabilities();
    EXPECT_EQ(reads, cfg.use_direct_reads && caps.direct_io_available);
    EXPECT_FALSE(flush);
}

TEST_F(NVMeDirectIOTest, FlushOnlyWhenCapabilityPresent) {
    NVMeConfig cfg;
    cfg.use_direct_io_for_flush_and_compaction = true;
    NVMeManager mgr(cfg);
    mgr.initialize();
    auto [reads, flush] = mgr.recommendedDirectIOFlags();
    NVMeCapabilities caps = mgr.detectCapabilities();
    EXPECT_FALSE(reads);
    EXPECT_EQ(flush,
              cfg.use_direct_io_for_flush_and_compaction && caps.direct_io_available);
}

// ─────────────────────────────────────────────────────────────────────────────
// NVMeBackgroundThreadsTest – recommendedBackgroundThreads()
// ─────────────────────────────────────────────────────────────────────────────

class NVMeBackgroundThreadsTest : public ::testing::Test {};

TEST_F(NVMeBackgroundThreadsTest, AtLeastTwo) {
    NVMeManager mgr;
    EXPECT_GE(mgr.recommendedBackgroundThreads(), 2u);
}

TEST_F(NVMeBackgroundThreadsTest, AtMostSixteen) {
    NVMeManager mgr;
    EXPECT_LE(mgr.recommendedBackgroundThreads(), 16u);
}

// ─────────────────────────────────────────────────────────────────────────────
// NVMeIoUringTest – io_uring disabled → isIoUringActive() == false
// ─────────────────────────────────────────────────────────────────────────────

class NVMeIoUringTest : public ::testing::Test {};

TEST_F(NVMeIoUringTest, DisabledByDefault) {
    NVMeManager mgr;
    mgr.initialize();
    // io_uring is off by default
    EXPECT_FALSE(mgr.isIoUringActive());
}

TEST_F(NVMeIoUringTest, PollCompletionsReturnsZeroWhenInactive) {
    NVMeManager mgr;
    mgr.initialize();
    std::vector<NVMeIOResult> results;
    int n = mgr.pollCompletions(results, 0);
    EXPECT_EQ(n, 0);
    EXPECT_TRUE(results.empty());
}

TEST_F(NVMeIoUringTest, SubmitReadFallbackOnInvalidFd) {
    NVMeManager mgr;
    mgr.initialize();
    NVMeIORequest req;
    req.fd      = -1;
    req.buf     = nullptr;
    req.len     = 0;
    req.offset  = 0;
    bool ok = mgr.submitRead(req);
    EXPECT_FALSE(ok);
}

TEST_F(NVMeIoUringTest, SubmitWriteFallbackOnInvalidFd) {
    NVMeManager mgr;
    mgr.initialize();
    NVMeIORequest req;
    req.fd      = -1;
    req.buf     = nullptr;
    req.len     = 0;
    req.offset  = 0;
    req.is_write = true;
    bool ok = mgr.submitWrite(req);
    EXPECT_FALSE(ok);
}

TEST_F(NVMeIoUringTest, SubmitReadFallbackOnRealFile) {
#ifndef _WIN32
    // Write a small temp file and verify the synchronous fallback reads it back
    fs::path tmp = fs::temp_directory_path() / "themis_nvme_test_read.bin";
    {
        std::ofstream f(tmp, std::ios::binary);
        ASSERT_TRUE(f.is_open()) << "Cannot create temp file: " << tmp;
        const char data[] = "hello nvme";
        f.write(data, sizeof(data));
    }
    int fd = ::open(tmp.c_str(), O_RDONLY);
    ASSERT_GE(fd, 0) << "Cannot open temp file for reading";

    char buf[16];
    std::memset(buf, 0, sizeof(buf));

    NVMeManager mgr;
    mgr.initialize();

    NVMeIORequest req;
    req.fd     = fd;
    req.buf    = buf;
    req.len    = 11;
    req.offset = 0;

    bool ok = mgr.submitRead(req);
    ::close(fd);
    fs::remove(tmp);

    EXPECT_TRUE(ok);
    EXPECT_STREQ(buf, "hello nvme");
#else
    GTEST_SKIP() << "POSIX open/read not available on Windows";
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// NVMeZNSTest – ZNS ops are no-ops when disabled
// ─────────────────────────────────────────────────────────────────────────────

class NVMeZNSTest : public ::testing::Test {};

TEST_F(NVMeZNSTest, ResetZoneNoOpWhenDisabled) {
    NVMeConfig cfg;
    cfg.enable_zns = false;
    NVMeManager mgr(cfg);
    mgr.initialize();
    EXPECT_FALSE(mgr.resetZone(0));
}

TEST_F(NVMeZNSTest, FinishZoneNoOpWhenDisabled) {
    NVMeConfig cfg;
    cfg.enable_zns = false;
    NVMeManager mgr(cfg);
    mgr.initialize();
    EXPECT_FALSE(mgr.finishZone(0));
}

TEST_F(NVMeZNSTest, WritePointerReturnsMaxWhenDisabled) {
    NVMeConfig cfg;
    cfg.enable_zns = false;
    NVMeManager mgr(cfg);
    mgr.initialize();
    EXPECT_EQ(mgr.getZoneWritePointer(0), UINT64_MAX);
}

// ─────────────────────────────────────────────────────────────────────────────
// NVMeDetectedQueueCountTest
// ─────────────────────────────────────────────────────────────────────────────

TEST(NVMeDetectedQueueCountTest, DefaultsToOneBeforeInit) {
    NVMeManager mgr;
    // Before detect, returns 1
    EXPECT_EQ(mgr.detectedQueueCount(), 1u);
}

TEST(NVMeDetectedQueueCountTest, PositiveAfterDetect) {
    NVMeManager mgr;
    mgr.detectCapabilities();
    EXPECT_GE(mgr.detectedQueueCount(), 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// RocksDBWrapper NVMe integration
// ─────────────────────────────────────────────────────────────────────────────

class RocksDBNVMeIntegrationTest : public ::testing::Test {
protected:
    fs::path db_path_;

    void SetUp() override {
        // Use std::hash for a portable process-unique suffix
        std::string suffix = std::to_string(
            std::hash<std::thread::id>{}(std::this_thread::get_id()));
        db_path_ = fs::temp_directory_path() / ("themis_nvme_rocksdb_" + suffix);
        fs::create_directories(db_path_);
    }

    void TearDown() override {
        fs::remove_all(db_path_);
    }
};

TEST_F(RocksDBNVMeIntegrationTest, NVMeDisabledByDefault) {
    themis::RocksDBWrapper::Config cfg;
    cfg.db_path = db_path_.string();
    EXPECT_FALSE(cfg.enable_nvme_optimizations);
}

TEST_F(RocksDBNVMeIntegrationTest, NVMeConfigFieldsExist) {
    themis::RocksDBWrapper::Config cfg;
    cfg.enable_nvme_optimizations     = true;
    cfg.nvme_device_path              = "/dev/nvme0n1";
    cfg.nvme_io_uring_queue_depth     = 256;
    cfg.nvme_enable_io_uring          = false;
    cfg.nvme_enable_zns               = false;
    EXPECT_TRUE(cfg.enable_nvme_optimizations);
    EXPECT_EQ(cfg.nvme_io_uring_queue_depth, 256u);
}

TEST_F(RocksDBNVMeIntegrationTest, OpenWithNVMeEnabledDoesNotCrash) {
    themis::RocksDBWrapper::Config cfg;
    cfg.db_path                   = db_path_.string();
    cfg.enable_nvme_optimizations = true;
    // No real NVMe device; manager will fall back gracefully
    cfg.nvme_device_path          = "";
    cfg.nvme_enable_io_uring      = false;
    cfg.nvme_enable_zns           = false;

    EXPECT_NO_THROW({
        themis::RocksDBWrapper db(cfg);
        auto status = db.open();
        // open may succeed or fail depending on env, but must not throw
        (void)status;
    });
}

TEST_F(RocksDBNVMeIntegrationTest, BasicPutGetWithNVMeEnabled) {
    themis::RocksDBWrapper::Config cfg;
    cfg.db_path                   = db_path_.string();
    cfg.enable_nvme_optimizations = true;
    cfg.nvme_device_path          = "";
    cfg.nvme_enable_io_uring      = false;
    cfg.nvme_enable_zns           = false;

    themis::RocksDBWrapper db(cfg);
    auto open_status = db.open();
    if (!open_status) {
        GTEST_SKIP() << "RocksDB open failed in test environment; skipping I/O tests";
    }

    auto put_result = db.put("nvme_key", "nvme_value");
    ASSERT_TRUE(put_result) << "put failed";

    auto get_result = db.get("nvme_key");
    ASSERT_TRUE(get_result.has_value()) << "get failed";
    EXPECT_EQ(std::string(get_result->begin(), get_result->end()), "nvme_value");
}
