/**
 * @file test_fuzz_core.cpp
 * @brief Fuzz-style security and robustness tests for Themis Core Framework
 *
 * These tests exercise new core components with boundary values, malformed
 * inputs, adversarial data, and semi-random payloads to surface crashes,
 * unhandled exceptions, and unexpected allow/deny decisions.
 *
 * Components under test:
 *   1. Wire Protocol V2 frame header parser – bad magic, truncated frames,
 *      oversized payloads, garbage flags
 *   2. CompressionAdvisor – extreme payload sizes (0, SIZE_MAX, …)
 *   3. PayloadBufferPool  – rapid concurrent acquire/release cycles
 *   4. WireProtocolMetrics – extreme latency values, concurrent writes
 *   5. AbiChecker         – garbage metadata, null handles
 *   6. LicenseData fields – empty strings, huge strings, special characters
 *   7. BuildInfo manifest – invalid paths, corrupted manifest files
 */

#include <gtest/gtest.h>

// Core-framework headers
#include "themis/network/wire_protocol_v2.hpp"
#include "network/wire_protocol_performance.h"
#include "themis/base/module_sandbox.h"
#include "themis/base/module_loader.h"
#include "themis/license_info.h"
#include "themis/build_info.h"

#include <algorithm>
#include <array>
#include <climits>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace themis;
using namespace themis::wire;
using namespace themis::network;
using namespace themis::modules;
using namespace themis::license;
using namespace themis::build_info;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

// Deterministic PRNG seeded with a fixed value for reproducibility
std::mt19937_64& rng() {
    static std::mt19937_64 gen(0xDEADBEEFCAFEBABEULL);
    return gen;
}

std::string randomString(size_t len) {
    std::uniform_int_distribution<int> dist(0, 255);
    std::string s(len, '\0');
    for (auto& c : s) {
      c = static_cast<char>(dist(rng()));
    }
    return s;
}

std::string repeat(char c, size_t n) { return std::string(n, c); }

// Common adversarial string payloads
const std::vector<std::string>& adversarialStrings() {
    static const std::vector<std::string> v = {
        "",
        " ",
        repeat('\0', 256),
        repeat('A', 65536),
        repeat('/', 1024),
        std::string("\xff\xfe\xfd\xfc"),   // high bytes
        "null\x00byte",
        "../../etc/passwd",
        "' OR '1'='1",
        "<script>alert(1)</script>",
        "${7*7}",
        "👾🔥💀",     // emoji (multi-byte UTF-8)
        "\r\n\r\n",   // HTTP header injection attempt
        std::string(1, '\x01') + std::string(1, '\x02') + std::string(1, '\x03'),
        randomString(4096),
        randomString(1),
    };
    return v;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// 1. Wire Protocol V2 – frame header
// ─────────────────────────────────────────────────────────────────────────────

class FuzzWireV2FrameHeader : public ::testing::Test {};

TEST_F(FuzzWireV2FrameHeader, IsValidRejectsAllZeroHeader) {
    V2FrameHeader h{};
    std::memset(&h, 0, sizeof(h));
    EXPECT_FALSE(h.is_valid());
}

TEST_F(FuzzWireV2FrameHeader, IsValidRejectsWrongMagic) {
    static const std::array<uint32_t, 8> bad_magics = {
        0x00000000u, 0xFFFFFFFFu, 0xDEADBEEFu,
        WIRE_V2_MAGIC + 1, WIRE_V2_MAGIC - 1,
        0x544D4442u, // "TMDB" (V1 magic)
        0x544D4433u, // "TMD3"
        0x01020304u,
    };
    for (uint32_t m : bad_magics) {
        V2FrameHeader h{};
        h.magic   = m;
        h.version = WIRE_VERSION_2;
        EXPECT_FALSE(h.is_valid()) << "Should reject magic=0x" << std::hex << m;
    }
}

TEST_F(FuzzWireV2FrameHeader, IsValidRejectsWrongVersion) {
    for (int v = 0; v <= 255; ++v) {
        if (v == static_cast<int>(WIRE_VERSION_2)) {
          continue;
        }
        V2FrameHeader h{};
        h.magic   = WIRE_V2_MAGIC;
        h.version = static_cast<uint8_t>(v);
        EXPECT_FALSE(h.is_valid()) << "Should reject version=" << v;
    }
}

TEST_F(FuzzWireV2FrameHeader, GetTypeDoesNotCrashOnAllByteValues) {
    for (int t = 0; t <= 255; ++t) {
        V2FrameHeader h{};
        h.frame_type = static_cast<uint8_t>(t);
        // get_type() just casts – must not crash
        EXPECT_NO_THROW({ auto _ = h.get_type(); (void)_; });
    }
}

TEST_F(FuzzWireV2FrameHeader, HasFlagDoesNotCrashOnAnyFlagCombination) {
    static const std::array<V2FrameFlags, 8> all_flags = {
        V2FrameFlags::NONE, V2FrameFlags::END_STREAM, V2FrameFlags::END_HEADERS,
        V2FrameFlags::PADDED, V2FrameFlags::PRIORITY_FLAG, V2FrameFlags::ACK,
        V2FrameFlags::COMPRESSED,
        static_cast<V2FrameFlags>(0xFFFF), // all bits set
    };
    V2FrameHeader h{};
    h.flags = 0xFFFF;
    for (auto f : all_flags) {
        EXPECT_NO_THROW({ bool _ = h.has_flag(f); (void)_; });
    }
}

TEST_F(FuzzWireV2FrameHeader, OversizedPayloadLengthFieldNoCrash) {
    V2FrameHeader h{};
    h.magic          = WIRE_V2_MAGIC;
    h.version        = WIRE_VERSION_2;
    h.frame_type     = static_cast<uint8_t>(V2FrameType::DATA);
    h.payload_length = 0xFFFFFFFFu; // 4 GiB claimed payload

    // is_valid() only checks magic+version – not payload size
    EXPECT_TRUE(h.is_valid());
    // Payload-size enforcement is in the async read path, not the header struct
}

TEST_F(FuzzWireV2FrameHeader, AllFrameTypesKnown) {
    // Ensure no gap in frame-type enum coverage
    for (uint8_t t = 0; t <= 9; ++t) {
        V2FrameHeader h{};
        h.frame_type = t;
        EXPECT_NO_THROW({ auto d = h.get_type(); (void)d; });
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// 2. CompressionAdvisor – extreme / boundary inputs
// ─────────────────────────────────────────────────────────────────────────────

class FuzzCompressionAdvisor : public ::testing::Test {};

TEST_F(FuzzCompressionAdvisor, ZeroSizeAlwaysSkip) {
    CompressionAdvisor advisor;
    EXPECT_EQ(advisor.advise(0), CompressionAdvisor::Decision::SKIP);
}

TEST_F(FuzzCompressionAdvisor, MaxSizeTDoesNotCrash) {
    CompressionAdvisor advisor;
    EXPECT_NO_THROW({
        auto d = advisor.advise(std::numeric_limits<size_t>::max());
        (void)d;
    });
}

TEST_F(FuzzCompressionAdvisor, AllReturnedDecisionsAreValid) {
    CompressionAdvisor advisor;
    static const std::array<size_t, 12> sizes = {
        0, 1, 511, 512, 513, 1024, 65535, 65536, 65537,
        1024*1024, 16*1024*1024, std::numeric_limits<size_t>::max()
    };
    static const std::array<CompressionAdvisor::Decision, 4> valid_decisions = {
        CompressionAdvisor::Decision::SKIP,
        CompressionAdvisor::Decision::LZ4_FAST,
        CompressionAdvisor::Decision::LZ4_FAST_X,
        CompressionAdvisor::Decision::LZ4_HC,
    };
    for (size_t sz : sizes) {
        auto d = advisor.advise(sz);
        bool found = std::any_of(valid_decisions.begin(), valid_decisions.end(),
                                  [d](auto vd) { return d == vd; });
        EXPECT_TRUE(found) << "Unknown decision for size=" << sz;
    }
}

TEST_F(FuzzCompressionAdvisor, LargeAccelerationValues) {
    CompressionAdvisor::Config cfg;
    cfg.lz4_fast_acceleration   = INT_MAX;
    cfg.lz4_fast_x_acceleration = INT_MAX;
    cfg.lz4_hc_level            = 12;
    CompressionAdvisor advisor(cfg);

    EXPECT_NO_THROW({
        auto d   = advisor.advise(1024);
        auto acc = advisor.lz4Acceleration(d);
        (void)acc;
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// 3. PayloadBufferPool – concurrent acquire/release stress
// ─────────────────────────────────────────────────────────────────────────────

class FuzzBufferPool : public ::testing::Test {};

TEST_F(FuzzBufferPool, ConcurrentAcquireRelease_NoCrash) {
    PayloadBufferPool pool(4096, 16);

    constexpr int kThreads = 8;
    constexpr int kIters   = 100;
    std::vector<std::thread> threads;

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&pool]() {
            for (int i = 0; i < kIters; ++i) {
                auto buf = pool.acquire();
                buf->resize(static_cast<size_t>(i * 7 % 4096));
                // Verify capacity invariant
                EXPECT_GE(buf->capacity(), 0u);
            }
        });
    }
    for (auto& t : threads) {
      t.join();
    }

    EXPECT_GE(pool.hitCount() + pool.missCount(),
              static_cast<uint64_t>(kThreads * kIters));
}

TEST_F(FuzzBufferPool, AcquireEmptyBuffer) {
    PayloadBufferPool pool(1, 4);
    auto buf = pool.acquire();
    EXPECT_TRUE(buf->empty());
    EXPECT_NO_THROW(buf->resize(0));
}

TEST_F(FuzzBufferPool, AcquireMaxSizeBuffer) {
    // Pool with large slab size
    PayloadBufferPool pool(16 * 1024 * 1024, 1);
    EXPECT_NO_THROW({
        auto buf = pool.acquire();
        (void)buf;
    });
}

TEST_F(FuzzBufferPool, PoolDepthZeroStillWorks) {
    // Depth=0: every acquire allocates fresh; every return is dropped
    PayloadBufferPool pool(256, 0);
    {
        auto b1 = pool.acquire();
        auto b2 = pool.acquire();
        (void)b1; (void)b2;
    }
    EXPECT_EQ(pool.poolDepth(), 0u);
    EXPECT_EQ(pool.hitCount(),  0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// 4. WireProtocolMetrics – extreme latency values, concurrent writes
// ─────────────────────────────────────────────────────────────────────────────

class FuzzWireV2Metrics : public ::testing::Test {};

TEST_F(FuzzWireV2Metrics, ExtremeLatencyValues_NoCrash) {
    WireProtocolMetrics m;
    static const std::array<double, 8> extreme_ms = {
        0.0, -1.0, 1e-9, 1e9, std::numeric_limits<double>::max(),
        std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(),
        std::numeric_limits<double>::quiet_NaN(),
    };
    for (double ms : extreme_ms) {
        EXPECT_NO_THROW({ m.recordLatencyMs(ms); });
    }
    EXPECT_NO_THROW({ auto snap = m.snapshot(); (void)snap; });
}

TEST_F(FuzzWireV2Metrics, ConcurrentRecordCalls_NoCrash) {
    WireProtocolMetrics m;
    constexpr int kThreads = 8;
    constexpr int kIters   = 200;
    std::vector<std::thread> threads;

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&m, t]() {
            for (int i = 0; i < kIters; ++i) {
                m.recordLatencyMs(static_cast<double>(i + t));
                m.recordBytes(static_cast<uint64_t>(i), static_cast<uint64_t>(i * 2));
                m.recordError("timeout");
            }
        });
    }
    for (auto& thr : threads) {
      thr.join();
    }

    EXPECT_NO_THROW({ auto snap = m.snapshot(); (void)snap; });
    EXPECT_EQ(m.totalRequests(),
              static_cast<uint64_t>(kThreads * kIters));
}

TEST_F(FuzzWireV2Metrics, SnapshotOnEmpty_NoCrash) {
    WireProtocolMetrics m;
    EXPECT_NO_THROW({
        auto snap = m.snapshot();
        EXPECT_EQ(snap.latency.sample_count, 0u);
    });
}

TEST_F(FuzzWireV2Metrics, UnknownErrorKinds_NoCrash) {
    WireProtocolMetrics m;
    for (const auto& s : adversarialStrings()) {
        EXPECT_NO_THROW({ m.recordError(s.c_str()); });
    }
    EXPECT_GE(m.totalErrors(), 0u);
}

TEST_F(FuzzWireV2Metrics, ResetThenSnapshot_NoCrash) {
    WireProtocolMetrics m;
    for (int i = 0; i < 50; ++i) {
      m.recordLatencyMs(static_cast<double>(i));
    }
    m.reset();
    EXPECT_NO_THROW({ auto snap = m.snapshot(); (void)snap; });
    EXPECT_EQ(m.totalRequests(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// 5. AbiChecker – garbage metadata, null handles
// ─────────────────────────────────────────────────────────────────────────────

class FuzzAbiChecker : public ::testing::Test {};

TEST_F(FuzzAbiChecker, GarbageVersionStrings_NoCrash) {
    AbiChecker checker;
    for (const auto& s : adversarialStrings()) {
        ModuleMetadata meta;
        meta.version     = s;
        meta.themisMajor = 1;
        meta.themisMinor = 0;
        meta.themisPatch = 0;
        EXPECT_NO_THROW({
            auto r = checker.checkVersions(meta, 1, 0);
            (void)r;
        }) << "Crashed on version string: " << s.substr(0, 40);
    }
}

TEST_F(FuzzAbiChecker, ExtremeVersionNumbers_NoCrash) {
    AbiChecker checker;
    static const std::array<uint32_t, 5> extreme_versions = {
        0, 1, 100, 0xFFFFFFFF, UINT32_MAX / 2
    };
    for (uint32_t maj : extreme_versions) {
        for (uint32_t min : extreme_versions) {
            ModuleMetadata meta;
            meta.version      = "1.0.0";
            meta.themisMajor  = maj;
            meta.themisMinor  = min;
            EXPECT_NO_THROW({
                auto r = checker.checkVersions(meta, 1, 0);
                (void)r;
            });
        }
    }
}

TEST_F(FuzzAbiChecker, NullHandleWithManyRequiredSymbols_NoCrash) {
    AbiChecker checker;
    for (const auto& s : adversarialStrings()) {
        checker.addRequiredSymbol(s);
    }
    EXPECT_NO_THROW({
        auto r = checker.checkRequiredSymbols(nullptr);
        (void)r;
    });
}

TEST_F(FuzzAbiChecker, NullHandleDeprecatedSymbols_NoCrash) {
    AbiChecker checker;
    for (const auto& s : adversarialStrings()) {
        checker.addDeprecatedSymbol(s);
    }
    EXPECT_NO_THROW({
        auto r = checker.checkDeprecatedSymbols(nullptr);
        (void)r;
    });
}

TEST_F(FuzzAbiChecker, FullCheckGarbage_NoCrash) {
    AbiChecker checker;
    checker.useDefaultLists();

    ModuleMetadata meta;
    meta.version      = repeat('\x01', 512);
    meta.themisMajor  = 0xDEADBEEF;
    meta.themisMinor  = 0xCAFEBABE;
    meta.themisPatch  = 0xFFFFFFFF;
    EXPECT_NO_THROW({
        auto r = checker.check(nullptr, meta, 1, 7);
        EXPECT_FALSE(r.summary.empty());
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// 6. LicenseData fields – adversarial strings
// ─────────────────────────────────────────────────────────────────────────────

class FuzzLicenseData : public ::testing::Test {};

TEST_F(FuzzLicenseData, GetEmbeddedLicense_NoCrash) {
    // Calling with whatever is embedded (could be empty) must not crash
    EXPECT_NO_THROW({
        auto ld = getEmbeddedLicense();
        (void)ld;
    });
}

TEST_F(FuzzLicenseData, IsLicenseValidOnDefault_NoCrash) {
    auto maybe_ld = getEmbeddedLicense();
    if (!maybe_ld) {
      GTEST_SKIP() << "No embedded license – skip";
    }
    EXPECT_NO_THROW({
        bool v = isLicenseValid(*maybe_ld);
        (void)v;
    });
}

TEST_F(FuzzLicenseData, VerifyLicenseSignature_AdversarialKey_NoCrash) {
    // Build a LicenseData with garbage in every field
    LicenseData ld;
    ld.license_key          = repeat('\xff', 256);
    ld.organization_name    = repeat('\x00', 64);
    ld.edition              = "GARBAGE";
    ld.issued_date          = "not-a-date";
    ld.expiry_date          = "9999-99-99";
    ld.signature            = randomString(512);
    EXPECT_NO_THROW({
        bool v = verifyLicenseSignature(ld);
        (void)v;
    });
}

TEST_F(FuzzLicenseData, LicenseClientConfig_AdversarialServerUrl_NoCrash) {
    for (const auto& s : adversarialStrings()) {
        LicenseClientConfig cfg;
        cfg.server_url    = s;
        cfg.allow_offline = true; // prevent actual network calls
        cfg.grace_period_days = 0;
        EXPECT_NO_THROW({
            LicenseClient client(cfg);
            // Do NOT call activate() – that would hit the network
            auto cached = client.getCachedLicense();
            (void)cached;
        }) << "Crashed on server_url: " << s.substr(0, 40);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// 7. BuildInfo manifest – invalid paths and corrupted files
// ─────────────────────────────────────────────────────────────────────────────

class FuzzBuildInfo : public ::testing::Test {};

TEST_F(FuzzBuildInfo, ExportToAdversarialPaths_NoCrash) {
    static const std::vector<std::string> bad_paths = {
        "",
        "/",
        "/dev/null",           // should succeed (writes nothing to /dev/null)
        "/no/such/dir/x.json",
        repeat('/', 512),
        repeat('\x00', 4),
        ".",                   // directory, not a file
    };
    for (const auto& p : bad_paths) {
        EXPECT_NO_THROW({
            bool r = exportBuildManifest(p);
            (void)r;
            // We don't assert the result – just that it doesn't crash
        }) << "Crashed on path: " << p.substr(0, 40);
    }
}

TEST_F(FuzzBuildInfo, VerifyCorruptedManifest_ReturnsFalse) {
    // Write garbage to a temp file and verify it returns false
    auto tmp = std::filesystem::temp_directory_path() / "fuzz_corrupt_manifest.json";
    {
        std::ofstream f(tmp, std::ios::binary);
        f.write(randomString(1024).c_str(), 1024);
    }
    EXPECT_FALSE(verifyBuildManifest(tmp.string()));
    std::filesystem::remove(tmp);
}

TEST_F(FuzzBuildInfo, VerifyManifestWithAdversarialFields_NoCrash) {
    // Build a syntactically valid JSON but with adversarial field values
    auto tmp = std::filesystem::temp_directory_path() / "fuzz_adversarial_manifest.json";
    {
        std::ofstream f(tmp);
        f << "{\n";
        f << "  \"schema_version\": \"1\",\n";
        f << "  \"git_commit\": \"" << repeat('A', 65536) << "\",\n";
        f << "  \"toolchain\": \"" << repeat('\x01', 256) << "\",\n";
        f << "  \"git_dirty\": false,\n";
        f << "  \"dependencies\": {}\n";
        f << "}\n";
    }
    EXPECT_NO_THROW({
        bool r = verifyBuildManifest(tmp.string());
        (void)r;
    });
    std::filesystem::remove(tmp);
}

TEST_F(FuzzBuildInfo, GetReproducibilityInfo_AlwaysNonEmpty) {
    EXPECT_NO_THROW({
        auto info = getReproducibilityInfo();
        EXPECT_FALSE(info.toolchain.empty());
        EXPECT_FALSE(info.git_commit.empty());
        EXPECT_FALSE(info.build_host.empty());
    });
}
