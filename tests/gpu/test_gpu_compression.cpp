/**
 * ThemisDB GPU-Accelerated Compression Tests
 *
 * Covers:
 *  - GpuCompressionManager (Zstd, Snappy, LZ4) with CPU fallback
 *  - Integration with CompressionStrategyManager via GPU_ZSTD,
 *    GPU_SNAPPY, GPU_LZ4 CompressionMethod values
 *  - Round-trip correctness for all three algorithms
 *  - Batch compress / decompress
 *  - min_size_for_gpu threshold (small data → CPU path)
 *  - force_cpu_fallback toggle
 *  - Statistics accounting
 *  - string_to_method / method_to_string for new enum values
 */

#include <gtest/gtest.h>

#include "storage/gpu_compression.h"
#include "storage/compression_strategy.h"

#include <algorithm>
#include <chrono>
#include <random>
#include <string>
#include <vector>

using namespace themis::storage;
using namespace themis::compression;

// ═══════════════════════════════════════════════════════════
// Helpers
// ═══════════════════════════════════════════════════════════

namespace {

std::vector<uint8_t> make_random(size_t n, uint32_t seed = 42) {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(0, 255);
    std::vector<uint8_t> v(n);
    for (auto& b : v) b = static_cast<uint8_t>(dist(rng));
    return v;
}

// Compressible data: repeated pattern
std::vector<uint8_t> make_compressible(size_t n) {
    std::vector<uint8_t> v(n);
    const char pattern[] = "ThemisDB GPU-Accelerated Compression test data pattern. ";
    size_t pat_len = sizeof(pattern) - 1;
    for (size_t i = 0; i < n; ++i)
        v[i] = static_cast<uint8_t>(pattern[i % pat_len]);
    return v;
}

} // namespace

// ═══════════════════════════════════════════════════════════
// GpuCompressionManager — construction
// ═══════════════════════════════════════════════════════════

class GpuCompressionManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create manager in AUTO mode (CPU fallback when no GPU)
        GpuCompressionConfig cfg;
        cfg.accel_type = GpuAccelerationType::AUTO;
        mgr_ = std::make_unique<GpuCompressionManager>(cfg);
    }

    std::unique_ptr<GpuCompressionManager> mgr_;
};

TEST_F(GpuCompressionManagerTest, ConstructionSucceeds) {
    ASSERT_NE(mgr_, nullptr);
}

TEST_F(GpuCompressionManagerTest, ActiveAccelTypeIsValidAfterConstruction) {
    auto t = mgr_->active_accel_type();
    // Regardless of GPU availability the type must be a known value
    EXPECT_TRUE(
        t == GpuAccelerationType::CPU_ONLY ||
        t == GpuAccelerationType::CUDA     ||
        t == GpuAccelerationType::HIP
    );
}

TEST_F(GpuCompressionManagerTest, ForceCpuFallbackToggle) {
    mgr_->force_cpu_fallback(true);
    EXPECT_FALSE(mgr_->is_gpu_available());
    EXPECT_EQ(mgr_->active_accel_type(), GpuAccelerationType::CPU_ONLY);

    mgr_->force_cpu_fallback(false);
    // After disabling the force flag, the type may be CPU or GPU
    // depending on hardware — just assert it doesn't throw.
}

TEST_F(GpuCompressionManagerTest, CpuOnlyModeNeverUsesGpu) {
    GpuCompressionConfig cfg;
    cfg.accel_type = GpuAccelerationType::CPU_ONLY;
    GpuCompressionManager cpu_mgr(cfg);

    EXPECT_FALSE(cpu_mgr.is_gpu_available());
    EXPECT_EQ(cpu_mgr.active_accel_type(), GpuAccelerationType::CPU_ONLY);
}

// ═══════════════════════════════════════════════════════════
// Round-trip tests — Zstd
// ═══════════════════════════════════════════════════════════

class GpuZstdTest : public ::testing::Test {
protected:
    GpuCompressionConfig cfg_;
    std::unique_ptr<GpuCompressionManager> mgr_;

    void SetUp() override {
        cfg_.accel_type = GpuAccelerationType::CPU_ONLY; // deterministic
        mgr_ = std::make_unique<GpuCompressionManager>(cfg_);
    }
};

TEST_F(GpuZstdTest, CompressDecompressSmallData) {
    auto original = make_compressible(512);
    auto result   = mgr_->compress(original, GpuCompressionAlgorithm::ZSTD);

    ASSERT_TRUE(result.success) << result.error_message;
    EXPECT_EQ(result.algorithm, GpuCompressionAlgorithm::ZSTD);
    EXPECT_EQ(result.original_size, original.size());
    EXPECT_FALSE(result.used_gpu); // CPU-only mode

    auto restored = mgr_->decompress(result.data, GpuCompressionAlgorithm::ZSTD,
                                     original.size());
    ASSERT_EQ(restored.size(), original.size());
    EXPECT_EQ(restored, original);
}

TEST_F(GpuZstdTest, CompressDecompressLargeCompressibleData) {
    auto original = make_compressible(1 * 1024 * 1024); // 1 MB
    auto result   = mgr_->compress(original, GpuCompressionAlgorithm::ZSTD);

    ASSERT_TRUE(result.success) << result.error_message;
    EXPECT_GT(result.compression_ratio, 1.0f)
        << "Expected compression ratio > 1 for highly compressible data";

    auto restored = mgr_->decompress(result.data, GpuCompressionAlgorithm::ZSTD,
                                     original.size());
    ASSERT_EQ(restored.size(), original.size());
    EXPECT_EQ(restored, original);
}

TEST_F(GpuZstdTest, CompressDecompressRandomData) {
    auto original = make_random(64 * 1024); // 64 KB random
    auto result   = mgr_->compress(original, GpuCompressionAlgorithm::ZSTD);

    ASSERT_TRUE(result.success) << result.error_message;

    auto restored = mgr_->decompress(result.data, GpuCompressionAlgorithm::ZSTD,
                                     original.size());
    ASSERT_EQ(restored.size(), original.size());
    EXPECT_EQ(restored, original);
}

TEST_F(GpuZstdTest, CompressEmptyData) {
    std::vector<uint8_t> empty;
    // compress() with size=0 — should not crash and must be decompressible
    auto result = mgr_->compress(empty.data(), 0, GpuCompressionAlgorithm::ZSTD);
    // Empty input may succeed or gracefully report failure — must not crash
    if (result.success) {
        auto restored = mgr_->decompress(result.data,
                                          GpuCompressionAlgorithm::ZSTD, 0);
        EXPECT_TRUE(restored.empty());
    }
}

// ═══════════════════════════════════════════════════════════
// Round-trip tests — Snappy
// ═══════════════════════════════════════════════════════════

class GpuSnappyTest : public ::testing::Test {
protected:
    std::unique_ptr<GpuCompressionManager> mgr_;

    void SetUp() override {
        GpuCompressionConfig cfg;
        cfg.accel_type = GpuAccelerationType::CPU_ONLY;
        mgr_ = std::make_unique<GpuCompressionManager>(cfg);
    }
};

TEST_F(GpuSnappyTest, CompressDecompressSmallData) {
    auto original = make_compressible(512);
    auto result   = mgr_->compress(original, GpuCompressionAlgorithm::SNAPPY);

    ASSERT_TRUE(result.success) << result.error_message;
    EXPECT_EQ(result.algorithm, GpuCompressionAlgorithm::SNAPPY);

    auto restored = mgr_->decompress(result.data, GpuCompressionAlgorithm::SNAPPY);
    ASSERT_EQ(restored.size(), original.size());
    EXPECT_EQ(restored, original);
}

TEST_F(GpuSnappyTest, CompressDecompressLargeData) {
    auto original = make_compressible(2 * 1024 * 1024);
    auto result   = mgr_->compress(original, GpuCompressionAlgorithm::SNAPPY);

    ASSERT_TRUE(result.success) << result.error_message;
    EXPECT_GT(result.compression_ratio, 1.0f);

    auto restored = mgr_->decompress(result.data, GpuCompressionAlgorithm::SNAPPY);
    ASSERT_EQ(restored.size(), original.size());
    EXPECT_EQ(restored, original);
}

TEST_F(GpuSnappyTest, CompressDecompressRandomData) {
    auto original = make_random(128 * 1024);
    auto result   = mgr_->compress(original, GpuCompressionAlgorithm::SNAPPY);

    ASSERT_TRUE(result.success) << result.error_message;

    auto restored = mgr_->decompress(result.data, GpuCompressionAlgorithm::SNAPPY);
    ASSERT_EQ(restored.size(), original.size());
    EXPECT_EQ(restored, original);
}

// ═══════════════════════════════════════════════════════════
// Round-trip tests — LZ4
// ═══════════════════════════════════════════════════════════

class GpuLz4Test : public ::testing::Test {
protected:
    std::unique_ptr<GpuCompressionManager> mgr_;

    void SetUp() override {
        GpuCompressionConfig cfg;
        cfg.accel_type = GpuAccelerationType::CPU_ONLY;
        mgr_ = std::make_unique<GpuCompressionManager>(cfg);
    }
};

TEST_F(GpuLz4Test, CompressDecompressSmallData) {
    auto original = make_compressible(512);
    auto result   = mgr_->compress(original, GpuCompressionAlgorithm::LZ4);

    ASSERT_TRUE(result.success) << result.error_message;
    EXPECT_EQ(result.algorithm, GpuCompressionAlgorithm::LZ4);

    auto restored = mgr_->decompress(result.data, GpuCompressionAlgorithm::LZ4,
                                     original.size());
    ASSERT_EQ(restored.size(), original.size());
    EXPECT_EQ(restored, original);
}

TEST_F(GpuLz4Test, CompressDecompressLargeCompressibleData) {
    auto original = make_compressible(4 * 1024 * 1024); // 4 MB
    auto result   = mgr_->compress(original, GpuCompressionAlgorithm::LZ4);

    ASSERT_TRUE(result.success) << result.error_message;
    EXPECT_GT(result.compression_ratio, 1.0f);

    auto restored = mgr_->decompress(result.data, GpuCompressionAlgorithm::LZ4,
                                     original.size());
    ASSERT_EQ(restored.size(), original.size());
    EXPECT_EQ(restored, original);
}

TEST_F(GpuLz4Test, CompressDecompressRandomData) {
    auto original = make_random(256 * 1024);
    auto result   = mgr_->compress(original, GpuCompressionAlgorithm::LZ4);

    ASSERT_TRUE(result.success) << result.error_message;

    // LZ4 decompression requires original size; it is stored in the header
    auto restored = mgr_->decompress(result.data, GpuCompressionAlgorithm::LZ4,
                                     original.size());
    ASSERT_EQ(restored.size(), original.size());
    EXPECT_EQ(restored, original);
}

TEST_F(GpuLz4Test, OriginalSizeStoredInHeader) {
    auto original = make_compressible(10000);
    auto result   = mgr_->compress(original, GpuCompressionAlgorithm::LZ4);

    ASSERT_TRUE(result.success);

    // Decompress without providing original_size (rely on embedded header)
    auto restored = mgr_->decompress(result.data, GpuCompressionAlgorithm::LZ4);
    ASSERT_EQ(restored.size(), original.size());
    EXPECT_EQ(restored, original);
}

// ═══════════════════════════════════════════════════════════
// min_size_for_gpu threshold
// ═══════════════════════════════════════════════════════════

TEST(GpuCompressionThresholdTest, SmallDataAlwaysUsesCpuPath) {
    GpuCompressionConfig cfg;
    cfg.accel_type    = GpuAccelerationType::AUTO;
    cfg.min_size_for_gpu = 1024 * 1024; // 1 MB threshold

    GpuCompressionManager mgr(cfg);

    // 1 KB << 1 MB threshold → must always use CPU path
    auto original = make_compressible(1024);
    auto result   = mgr.compress(original, GpuCompressionAlgorithm::LZ4);

    ASSERT_TRUE(result.success);
    EXPECT_FALSE(result.used_gpu);
}

// ═══════════════════════════════════════════════════════════
// Batch compress / decompress
// ═══════════════════════════════════════════════════════════

class GpuBatchTest : public ::testing::Test {
protected:
    std::unique_ptr<GpuCompressionManager> mgr_;

    void SetUp() override {
        GpuCompressionConfig cfg;
        cfg.accel_type = GpuAccelerationType::CPU_ONLY;
        mgr_ = std::make_unique<GpuCompressionManager>(cfg);
    }
};

TEST_F(GpuBatchTest, BatchCompressDecompressLz4) {
    constexpr size_t kBatches = 8;
    std::vector<std::vector<uint8_t>> inputs;
    inputs.reserve(kBatches);
    for (size_t i = 0; i < kBatches; ++i)
        inputs.push_back(make_compressible(4096 + i * 128));

    auto compressed = mgr_->compress_batch(inputs, GpuCompressionAlgorithm::LZ4);
    ASSERT_EQ(compressed.size(), kBatches);

    std::vector<std::vector<uint8_t>> comp_bufs;
    std::vector<size_t> orig_sizes;
    comp_bufs.reserve(kBatches);
    orig_sizes.reserve(kBatches);
    for (size_t i = 0; i < kBatches; ++i) {
        ASSERT_TRUE(compressed[i].success) << "batch " << i;
        comp_bufs.push_back(compressed[i].data);
        orig_sizes.push_back(compressed[i].original_size);
    }

    auto restored = mgr_->decompress_batch(comp_bufs,
                                           GpuCompressionAlgorithm::LZ4,
                                           orig_sizes);
    ASSERT_EQ(restored.size(), kBatches);
    for (size_t i = 0; i < kBatches; ++i)
        EXPECT_EQ(restored[i], inputs[i]) << "mismatch at batch " << i;
}

TEST_F(GpuBatchTest, BatchCompressDecompressSnappy) {
    constexpr size_t kBatches = 5;
    std::vector<std::vector<uint8_t>> inputs;
    for (size_t i = 0; i < kBatches; ++i)
        inputs.push_back(make_compressible(8192));

    auto compressed = mgr_->compress_batch(inputs, GpuCompressionAlgorithm::SNAPPY);
    ASSERT_EQ(compressed.size(), kBatches);

    std::vector<std::vector<uint8_t>> comp_bufs;
    for (const auto& r : compressed) {
        ASSERT_TRUE(r.success);
        comp_bufs.push_back(r.data);
    }

    auto restored = mgr_->decompress_batch(comp_bufs, GpuCompressionAlgorithm::SNAPPY);
    ASSERT_EQ(restored.size(), kBatches);
    for (size_t i = 0; i < kBatches; ++i)
        EXPECT_EQ(restored[i], inputs[i]);
}

// ═══════════════════════════════════════════════════════════
// Statistics
// ═══════════════════════════════════════════════════════════

TEST(GpuCompressionStatsTest, StatsAccumulateCorrectly) {
    GpuCompressionConfig cfg;
    cfg.accel_type = GpuAccelerationType::CPU_ONLY;
    GpuCompressionManager mgr(cfg);

    auto data = make_compressible(4096);

    mgr.compress(data, GpuCompressionAlgorithm::LZ4);
    mgr.compress(data, GpuCompressionAlgorithm::SNAPPY);
    mgr.compress(data, GpuCompressionAlgorithm::ZSTD);

    auto stats = mgr.get_stats();
    EXPECT_EQ(stats.total_compress_ops, 3u);
    EXPECT_EQ(stats.gpu_compress_ops, 0u); // CPU-only mode
    EXPECT_GT(stats.bytes_in,  0u);
    EXPECT_GT(stats.bytes_out, 0u);

    mgr.reset_stats();
    auto reset_stats = mgr.get_stats();
    EXPECT_EQ(reset_stats.total_compress_ops, 0u);
}

// ═══════════════════════════════════════════════════════════
// CompressionStrategyManager integration
// ═══════════════════════════════════════════════════════════

class CompressionStrategyGpuTest : public ::testing::Test {
protected:
    void SetUp() override {
        CompressionConfig cfg;
        cfg.min_size = 0; // compress even tiny data for these tests
        // Force CPU fallback so tests are deterministic without a GPU
        cfg.gpu_config.accel_type = GpuAccelerationType::CPU_ONLY;
        mgr_ = std::make_unique<CompressionStrategyManager>(cfg);
    }

    std::unique_ptr<CompressionStrategyManager> mgr_;
};

TEST_F(CompressionStrategyGpuTest, GpuZstdRoundTrip) {
    CompressionConfig cfg;
    cfg.method    = CompressionMethod::GPU_ZSTD;
    cfg.min_size  = 0;
    cfg.gpu_config.accel_type = GpuAccelerationType::CPU_ONLY;
    CompressionStrategyManager mgr(cfg);

    auto original = make_compressible(4096);
    auto result   = mgr.compress(original.data(), original.size());

    ASSERT_TRUE(result.success) << "GPU_ZSTD compress failed";
    EXPECT_EQ(result.method_used, CompressionMethod::GPU_ZSTD);

    auto restored = mgr.decompress(result.data, CompressionMethod::GPU_ZSTD);
    ASSERT_EQ(restored.size(), original.size());
    EXPECT_EQ(restored, original);
}

TEST_F(CompressionStrategyGpuTest, GpuSnappyRoundTrip) {
    CompressionConfig cfg;
    cfg.method    = CompressionMethod::GPU_SNAPPY;
    cfg.min_size  = 0;
    cfg.gpu_config.accel_type = GpuAccelerationType::CPU_ONLY;
    CompressionStrategyManager mgr(cfg);

    auto original = make_compressible(8192);
    auto result   = mgr.compress(original.data(), original.size());

    ASSERT_TRUE(result.success) << "GPU_SNAPPY compress failed";
    EXPECT_EQ(result.method_used, CompressionMethod::GPU_SNAPPY);

    auto restored = mgr.decompress(result.data, CompressionMethod::GPU_SNAPPY);
    ASSERT_EQ(restored.size(), original.size());
    EXPECT_EQ(restored, original);
}

TEST_F(CompressionStrategyGpuTest, GpuLz4RoundTrip) {
    CompressionConfig cfg;
    cfg.method    = CompressionMethod::GPU_LZ4;
    cfg.min_size  = 0;
    cfg.gpu_config.accel_type = GpuAccelerationType::CPU_ONLY;
    CompressionStrategyManager mgr(cfg);

    auto original = make_compressible(16384);
    auto result   = mgr.compress(original.data(), original.size());

    ASSERT_TRUE(result.success) << "GPU_LZ4 compress failed";
    EXPECT_EQ(result.method_used, CompressionMethod::GPU_LZ4);

    auto restored = mgr.decompress(result.data, CompressionMethod::GPU_LZ4);
    ASSERT_EQ(restored.size(), original.size());
    EXPECT_EQ(restored, original);
}

// ═══════════════════════════════════════════════════════════
// method_to_string / string_to_method
// ═══════════════════════════════════════════════════════════

TEST(CompressionMethodStringTest, GpuMethodsHaveCorrectStrings) {
    EXPECT_EQ(CompressionStrategyManager::method_to_string(
                  CompressionMethod::GPU_ZSTD),   "gpu_zstd");
    EXPECT_EQ(CompressionStrategyManager::method_to_string(
                  CompressionMethod::GPU_SNAPPY), "gpu_snappy");
    EXPECT_EQ(CompressionStrategyManager::method_to_string(
                  CompressionMethod::GPU_LZ4),    "gpu_lz4");
}

TEST(CompressionMethodStringTest, StringToGpuMethod) {
    auto opt_zstd   = CompressionStrategyManager::string_to_method("gpu_zstd");
    auto opt_snappy = CompressionStrategyManager::string_to_method("gpu_snappy");
    auto opt_lz4    = CompressionStrategyManager::string_to_method("gpu_lz4");

    ASSERT_TRUE(opt_zstd.has_value());
    ASSERT_TRUE(opt_snappy.has_value());
    ASSERT_TRUE(opt_lz4.has_value());

    EXPECT_EQ(*opt_zstd,   CompressionMethod::GPU_ZSTD);
    EXPECT_EQ(*opt_snappy, CompressionMethod::GPU_SNAPPY);
    EXPECT_EQ(*opt_lz4,    CompressionMethod::GPU_LZ4);
}

TEST(CompressionMethodStringTest, UnknownStringReturnsNullopt) {
    EXPECT_FALSE(
        CompressionStrategyManager::string_to_method("no_such_method").has_value());
}

// ═══════════════════════════════════════════════════════════
// algorithm_to_string / accel_type_to_string helpers
// ═══════════════════════════════════════════════════════════

TEST(GpuCompressionHelperTest, AlgorithmToStringIsCorrect) {
    EXPECT_EQ(GpuCompressionManager::algorithm_to_string(
                  GpuCompressionAlgorithm::ZSTD),   "gpu_zstd");
    EXPECT_EQ(GpuCompressionManager::algorithm_to_string(
                  GpuCompressionAlgorithm::SNAPPY), "gpu_snappy");
    EXPECT_EQ(GpuCompressionManager::algorithm_to_string(
                  GpuCompressionAlgorithm::LZ4),    "gpu_lz4");
}

TEST(GpuCompressionHelperTest, AccelTypeToStringIsCorrect) {
    EXPECT_EQ(GpuCompressionManager::accel_type_to_string(
                  GpuAccelerationType::CPU_ONLY), "cpu_only");
    EXPECT_EQ(GpuCompressionManager::accel_type_to_string(
                  GpuAccelerationType::CUDA),     "cuda");
    EXPECT_EQ(GpuCompressionManager::accel_type_to_string(
                  GpuAccelerationType::HIP),      "hip");
    EXPECT_EQ(GpuCompressionManager::accel_type_to_string(
                  GpuAccelerationType::AUTO),     "auto");
}

// ═══════════════════════════════════════════════════════════
// Factory helper
// ═══════════════════════════════════════════════════════════

TEST(GpuCompressionFactoryTest, CreateManagerReturnsNonNull) {
    GpuCompressionConfig cfg;
    cfg.accel_type = GpuAccelerationType::CPU_ONLY;
    auto mgr = create_gpu_compression_manager(cfg);
    ASSERT_NE(mgr, nullptr);
}
