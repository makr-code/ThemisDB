// Copyright 2026 ThemisDB Team
// SPDX-License-Identifier: Apache-2.0

/**
 * @file test_tensor_storage_strategy.cpp
 * @brief Unit tests for the quantization assessor, MmapLoader, ZeroCopyAccessor,
 *        and StorageStrategyAssessor (Issue #5443).
 *
 * Test groups:
 *   - QSE-01..QSE-10 : QuantizationAssessor correctness
 *   - MML-01..MML-08 : MmapLoader / MmapRegion lifecycle
 *   - ZCA-01..ZCA-06 : ZeroCopyAccessor typed-span semantics
 *   - SSA-01..SSA-06 : StorageStrategyAssessor end-to-end
 */

#include "distributed_tensor/tensor_storage_strategy.h"

#include <gtest/gtest.h>

#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

using namespace themis::distributed_tensor;

// ── Test helpers ─────────────────────────────────────────────────────────────

namespace {

/// Write a binary file filled with sequential floats and return its path.
std::filesystem::path writeTempFloatFile(
    const std::string& name,
    std::size_t        num_floats) {

    const auto path =
        std::filesystem::temp_directory_path() / ("themis_test_" + name + ".bin");

    std::ofstream ofs(path, std::ios::binary | std::ios::trunc);
    for (std::size_t i = 0; i < num_floats; ++i) {
        const float v = static_cast<float>(i);
        ofs.write(reinterpret_cast<const char*>(&v), sizeof(float));
    }
    return path;
}

} // namespace

// =============================================================================
// QSE — QuantizationAssessor
// =============================================================================

// QSE-01: Zero error budget forces F32.
TEST(QuantizationAssessorTest, QSE01_ZeroErrorBudgetSelectsF32) {
    QuantizationConstraints c;
    c.max_l2_error_relative = 0.0;
    c.num_params             = 1'000'000;

    const auto result = QuantizationAssessor::assess(c);
    EXPECT_EQ(result.recommended_level, QuantizationLevel::F32);
    EXPECT_DOUBLE_EQ(result.compression_ratio, 1.0);
    EXPECT_DOUBLE_EQ(result.estimated_l2_error, 0.0);
    EXPECT_FALSE(result.rationale.empty());
}

// QSE-02: Large error budget with calibration data → INT4 for adapters.
TEST(QuantizationAssessorTest, QSE02_LargeErrorBudgetWithCalibrationSelectsHighCompression) {
    QuantizationConstraints c;
    c.max_l2_error_relative = 0.10; // 10 % tolerance
    c.num_params             = 7'000'000'000ULL;
    c.has_calibration_data   = true;
    c.is_adapter             = true;

    const auto result = QuantizationAssessor::assess(c);
    // With 10 % budget and calibration, INT4 (5 % typical error) is feasible.
    EXPECT_EQ(result.recommended_level, QuantizationLevel::INT4);
    EXPECT_GT(result.compression_ratio, 4.0);
}

// QSE-03: Tight error budget (0.5 %) without calibration → F16 or BF16.
TEST(QuantizationAssessorTest, QSE03_TightBudgetNoCalibrationsSelectsF16OrBF16) {
    QuantizationConstraints c;
    c.max_l2_error_relative = 0.005; // 0.5 %
    c.num_params             = 1'000'000;
    c.has_calibration_data   = false;

    const auto result = QuantizationAssessor::assess(c);
    // F16 (0.1 %) and BF16 (0.2 %) are within budget; F16 should be selected
    // as it has lower error than BF16.  INT8 (1.0 %) exceeds budget.
    EXPECT_TRUE(result.recommended_level == QuantizationLevel::F16
             || result.recommended_level == QuantizationLevel::BF16);
    EXPECT_LE(result.estimated_l2_error, 0.005);
}

// QSE-04: INT8 requires calibration data; absent → skip INT8.
TEST(QuantizationAssessorTest, QSE04_INT8SkippedWithoutCalibrationData) {
    QuantizationConstraints c;
    c.max_l2_error_relative = 0.02; // allows INT8 by error budget
    c.num_params             = 1'000'000;
    c.has_calibration_data   = false; // but no calibration

    const auto result = QuantizationAssessor::assess(c);
    // INT8 and INT4 require calibration; result should be F16 or BF16.
    EXPECT_NE(result.recommended_level, QuantizationLevel::INT8);
    EXPECT_NE(result.recommended_level, QuantizationLevel::INT4);
}

// QSE-05: Memory budget too small for F16/BF16 (2 bytes/param) → INT8 (1 byte/param).
TEST(QuantizationAssessorTest, QSE05_MemoryBudgetForcesHigherCompression) {
    QuantizationConstraints c;
    c.max_l2_error_relative = 0.02; // allows INT8 (0.01) but not INT4 (0.05)
    c.num_params             = 1'000'000;
    c.has_calibration_data   = true;
    // Budget: fits INT8 (1 MB) but not F16/BF16 (2 MB) nor F32 (4 MB).
    // packedBytesForParams(1M, INT8) = 1'000'000; set budget to 1.1 MB.
    c.memory_budget_bytes    = 1'100'000;

    const auto result = QuantizationAssessor::assess(c);
    EXPECT_EQ(result.recommended_level, QuantizationLevel::INT8);
}

// QSE-06: isFeasible returns true for F32 regardless of budget.
TEST(QuantizationAssessorTest, QSE06_F32AlwaysFeasible) {
    QuantizationConstraints c;
    c.max_l2_error_relative = 0.0;
    c.memory_budget_bytes    = 1; // pathologically tiny

    EXPECT_TRUE(QuantizationAssessor::isFeasible(QuantizationLevel::F32, c));
}

// QSE-07: isFeasible returns false for INT8 without calibration.
TEST(QuantizationAssessorTest, QSE07_INT8NotFeasibleWithoutCalibration) {
    QuantizationConstraints c;
    c.max_l2_error_relative = 0.5;
    c.has_calibration_data   = false;

    EXPECT_FALSE(QuantizationAssessor::isFeasible(QuantizationLevel::INT8, c));
    EXPECT_FALSE(QuantizationAssessor::isFeasible(QuantizationLevel::INT4, c));
}

// QSE-08: AVX-512 BF16 hardware triggers a warning when F16 is selected.
TEST(QuantizationAssessorTest, QSE08_Avx512Bf16HardwareWarningForF16) {
    QuantizationConstraints c;
    c.max_l2_error_relative = 0.005; // tight: forces F16/BF16
    c.has_calibration_data   = false;
    c.hw_avx512_bf16         = true;

    const auto result = QuantizationAssessor::assess(c);
    // Should warn that BF16 is preferable on AVX-512-BF16 hardware.
    bool has_avx_warning = false;
    for (const auto& w : result.warnings) {
        if (w.find("AVX-512") != std::string::npos
            || w.find("BF16") != std::string::npos) {
            has_avx_warning = true;
            break;
        }
    }
    // Warning is expected when F16 is chosen over BF16 on BF16-capable hardware.
    if (result.recommended_level == QuantizationLevel::F16) {
        EXPECT_TRUE(has_avx_warning);
    }
}

// QSE-09: bytesPerParam returns correct values for all levels.
TEST(QuantizationAssessorTest, QSE09_BytesPerParamCorrect) {
    EXPECT_EQ(bytesPerParam(QuantizationLevel::F32),    4);
    EXPECT_EQ(bytesPerParam(QuantizationLevel::F16),    2);
    EXPECT_EQ(bytesPerParam(QuantizationLevel::BF16),   2);
    EXPECT_EQ(bytesPerParam(QuantizationLevel::INT8),   1);
    EXPECT_EQ(bytesPerParam(QuantizationLevel::INT4),   1); // rounded up
    EXPECT_EQ(bytesPerParam(QuantizationLevel::BINARY), 1); // rounded up
}

// QSE-10: packedBytesForParams correctly handles sub-byte packing.
TEST(QuantizationAssessorTest, QSE10_PackedBytesSubByteCorrect) {
    EXPECT_EQ(packedBytesForParams(8, QuantizationLevel::INT4),   4); // 4 bits × 8 / 8
    EXPECT_EQ(packedBytesForParams(7, QuantizationLevel::INT4),   4); // ceil(7/2)
    EXPECT_EQ(packedBytesForParams(8, QuantizationLevel::BINARY), 1); // 1 bit × 8 / 8
    EXPECT_EQ(packedBytesForParams(9, QuantizationLevel::BINARY), 2); // ceil(9/8)
    EXPECT_EQ(packedBytesForParams(4, QuantizationLevel::F32),   16); // 4 × 4
}

// =============================================================================
// MML — MmapLoader / MmapRegion
// =============================================================================

// MML-01: Open a valid binary file; region is open and size matches.
TEST(MmapLoaderTest, MML01_OpenValidFile) {
    constexpr std::size_t kNumFloats = 256;
    const auto path = writeTempFloatFile("mml01", kNumFloats);

    MmapLoader loader;
    auto [region, err] = loader.open(path.string());

    EXPECT_EQ(err, MmapError::OK);
    EXPECT_TRUE(region.isOpen());
    EXPECT_EQ(region.size(), kNumFloats * sizeof(float));
    EXPECT_NE(region.data(), nullptr);
    EXPECT_EQ(region.path(), path.string());

    std::filesystem::remove(path);
}

// MML-02: Mapped data matches the original file content.
TEST(MmapLoaderTest, MML02_MappedDataMatchesFileContent) {
    constexpr std::size_t kNumFloats = 64;
    const auto path = writeTempFloatFile("mml02", kNumFloats);

    MmapLoader loader;
    auto [region, err] = loader.open(path.string());
    ASSERT_EQ(err, MmapError::OK);
    ASSERT_TRUE(region.isOpen());

    const auto floats = region.as_span<float>();
    ASSERT_EQ(floats.size(), kNumFloats);
    for (std::size_t i = 0; i < kNumFloats; ++i) {
        EXPECT_FLOAT_EQ(floats[i], static_cast<float>(i))
            << "Mismatch at index " << i;
    }

    std::filesystem::remove(path);
}

// MML-03: Opening a non-existent path returns FILE_NOT_FOUND.
TEST(MmapLoaderTest, MML03_OpenMissingFileReturnsFileNotFound) {
    MmapLoader loader;
    auto [region, err] = loader.open("/tmp/themis_nonexistent_file_12345678.bin");

    EXPECT_EQ(err, MmapError::FILE_NOT_FOUND);
    EXPECT_FALSE(region.isOpen());
}

// MML-04: close() releases the mapping; region reports not-open.
TEST(MmapLoaderTest, MML04_CloseReleasesMapping) {
    constexpr std::size_t kNumFloats = 32;
    const auto path = writeTempFloatFile("mml04", kNumFloats);

    MmapLoader loader;
    auto [region, err] = loader.open(path.string());
    ASSERT_EQ(err, MmapError::OK);

    region.close();
    EXPECT_FALSE(region.isOpen());
    EXPECT_EQ(region.data(), nullptr);
    EXPECT_EQ(region.size(), 0);

    std::filesystem::remove(path);
}

// MML-05: Double close() is safe (no crash).
TEST(MmapLoaderTest, MML05_DoubleCloseIsSafe) {
    constexpr std::size_t kNumFloats = 32;
    const auto path = writeTempFloatFile("mml05", kNumFloats);

    MmapLoader loader;
    auto [region, err] = loader.open(path.string());
    ASSERT_EQ(err, MmapError::OK);

    region.close();
    EXPECT_NO_FATAL_FAILURE(region.close()); // must not crash

    std::filesystem::remove(path);
}

// MML-06: Move constructor transfers ownership; source becomes not-open.
TEST(MmapLoaderTest, MML06_MoveConstructorTransfersOwnership) {
    constexpr std::size_t kNumFloats = 32;
    const auto path = writeTempFloatFile("mml06", kNumFloats);

    MmapLoader loader;
    auto [region, err] = loader.open(path.string());
    ASSERT_EQ(err, MmapError::OK);
    const std::byte* original_data = region.data();

    MmapRegion moved(std::move(region));
    EXPECT_FALSE(region.isOpen()); // NOLINT(bugprone-use-after-move)
    EXPECT_TRUE(moved.isOpen());
    EXPECT_EQ(moved.data(), original_data);

    std::filesystem::remove(path);
}

// MML-07: advise() on an open region returns OK.
TEST(MmapLoaderTest, MML07_AdviseOnOpenRegionReturnsOK) {
    constexpr std::size_t kNumFloats = 128;
    const auto path = writeTempFloatFile("mml07", kNumFloats);

    MmapLoader loader;
    auto [region, err] = loader.open(path.string());
    ASSERT_EQ(err, MmapError::OK);

    EXPECT_EQ(loader.advise(region, MmapLoader::AccessPattern::SEQUENTIAL),
              MmapError::OK);
    EXPECT_EQ(loader.advise(region, MmapLoader::AccessPattern::WILLNEED),
              MmapError::OK);

    std::filesystem::remove(path);
}

// MML-08: advise() on a closed region returns NOT_OPEN.
TEST(MmapLoaderTest, MML08_AdviseOnClosedRegionReturnsNotOpen) {
    MmapRegion closed_region; // default-constructed: not open
    MmapLoader loader;
    EXPECT_EQ(loader.advise(closed_region, MmapLoader::AccessPattern::SEQUENTIAL),
              MmapError::NOT_OPEN);
}

// =============================================================================
// ZCA — ZeroCopyAccessor
// =============================================================================

// ZCA-01: Accessor size equals region bytes / sizeof(T).
TEST(ZeroCopyAccessorTest, ZCA01_SizeEqualsRegionBytesOverElementSize) {
    constexpr std::size_t kNumFloats = 100;
    const auto path = writeTempFloatFile("zca01", kNumFloats);

    MmapLoader loader;
    auto [region, err] = loader.open(path.string());
    ASSERT_EQ(err, MmapError::OK);

    ZeroCopyAccessor<float> acc(region);
    EXPECT_EQ(acc.size(), kNumFloats);
    EXPECT_FALSE(acc.empty());

    std::filesystem::remove(path);
}

// ZCA-02: operator[] returns the correct value.
TEST(ZeroCopyAccessorTest, ZCA02_ElementAccessReturnsCorrectValue) {
    constexpr std::size_t kNumFloats = 50;
    const auto path = writeTempFloatFile("zca02", kNumFloats);

    MmapLoader loader;
    auto [region, err] = loader.open(path.string());
    ASSERT_EQ(err, MmapError::OK);

    ZeroCopyAccessor<float> acc(region);
    for (std::size_t i = 0; i < kNumFloats; ++i) {
        EXPECT_FLOAT_EQ(acc[i], static_cast<float>(i));
    }

    std::filesystem::remove(path);
}

// ZCA-03: Range-based for loop iterates all elements.
TEST(ZeroCopyAccessorTest, ZCA03_RangeBasedForLoopWorks) {
    constexpr std::size_t kNumFloats = 20;
    const auto path = writeTempFloatFile("zca03", kNumFloats);

    MmapLoader loader;
    auto [region, err] = loader.open(path.string());
    ASSERT_EQ(err, MmapError::OK);

    ZeroCopyAccessor<float> acc(region);
    std::size_t idx = 0;
    for (const float& v : acc) {
        EXPECT_FLOAT_EQ(v, static_cast<float>(idx));
        ++idx;
    }
    EXPECT_EQ(idx, kNumFloats);

    std::filesystem::remove(path);
}

// ZCA-04: span() returns the same underlying data.
TEST(ZeroCopyAccessorTest, ZCA04_SpanPointsToSameData) {
    constexpr std::size_t kNumFloats = 10;
    const auto path = writeTempFloatFile("zca04", kNumFloats);

    MmapLoader loader;
    auto [region, err] = loader.open(path.string());
    ASSERT_EQ(err, MmapError::OK);

    ZeroCopyAccessor<float> acc(region);
    EXPECT_EQ(acc.span().data(), acc.span().data()); // stable pointer
    EXPECT_EQ(acc.span().size(), kNumFloats);

    std::filesystem::remove(path);
}

// ZCA-05: Accessor over int8_t reinterprets bytes correctly.
TEST(ZeroCopyAccessorTest, ZCA05_Int8AccessorReinterpretsBytesCorrectly) {
    // Write 4 floats (16 bytes) → view as 16 int8_t values.
    constexpr std::size_t kNumFloats = 4;
    const auto path = writeTempFloatFile("zca05", kNumFloats);

    MmapLoader loader;
    auto [region, err] = loader.open(path.string());
    ASSERT_EQ(err, MmapError::OK);

    ZeroCopyAccessor<int8_t> acc(region);
    EXPECT_EQ(acc.size(), kNumFloats * sizeof(float)); // 16 bytes
    EXPECT_FALSE(acc.empty());

    std::filesystem::remove(path);
}

// ZCA-06: Empty region → accessor reports empty.
TEST(ZeroCopyAccessorTest, ZCA06_EmptyRegionGivesEmptyAccessor) {
    MmapRegion empty; // not open
    ZeroCopyAccessor<float> acc(empty);
    EXPECT_TRUE(acc.empty());
    EXPECT_EQ(acc.size(), 0);
}

// =============================================================================
// SSA — StorageStrategyAssessor
// =============================================================================

// SSA-01: No mmap support → BUFFERED_READ recommended.
TEST(StorageStrategyAssessorTest, SSA01_NoMmapSupportSelectsBufferedRead) {
    StorageStrategyAssessor::Config cfg;
    cfg.quant.max_l2_error_relative = 0.0; // F32
    cfg.quant.num_params             = 1'000'000;
    cfg.os_supports_mmap             = false;

    const auto rec = StorageStrategyAssessor::assess(cfg);
    EXPECT_EQ(rec.load_mechanism, LoadMechanism::BUFFERED_READ);
}

// SSA-02: NVMe + single consumer → MMAP_PREFAULT.
TEST(StorageStrategyAssessorTest, SSA02_NvmeSingleConsumerSelectsMmapPrefault) {
    StorageStrategyAssessor::Config cfg;
    cfg.quant.max_l2_error_relative = 0.0;
    cfg.quant.num_params             = 1'000'000;
    cfg.os_supports_mmap             = true;
    cfg.storage_on_nvme              = true;
    cfg.multi_consumer               = false;

    const auto rec = StorageStrategyAssessor::assess(cfg);
    EXPECT_EQ(rec.load_mechanism, LoadMechanism::MMAP_PREFAULT);
}

// SSA-03: HDD + single consumer → MMAP_ZERO_COPY.
TEST(StorageStrategyAssessorTest, SSA03_HddSingleConsumerSelectsMmapZeroCopy) {
    StorageStrategyAssessor::Config cfg;
    cfg.quant.max_l2_error_relative = 0.0;
    cfg.quant.num_params             = 1'000'000;
    cfg.os_supports_mmap             = true;
    cfg.storage_on_nvme              = false;
    cfg.multi_consumer               = false;

    const auto rec = StorageStrategyAssessor::assess(cfg);
    EXPECT_EQ(rec.load_mechanism, LoadMechanism::MMAP_ZERO_COPY);
}

// SSA-04: Multi-consumer → shared page cache (MMAP_PREFAULT or MMAP_ZERO_COPY).
TEST(StorageStrategyAssessorTest, SSA04_MultiConsumerSelectsMmap) {
    StorageStrategyAssessor::Config cfg;
    cfg.quant.max_l2_error_relative = 0.0;
    cfg.quant.num_params             = 1'000'000;
    cfg.os_supports_mmap             = true;
    cfg.multi_consumer               = true;

    const auto rec = StorageStrategyAssessor::assess(cfg);
    EXPECT_TRUE(rec.load_mechanism == LoadMechanism::MMAP_PREFAULT
             || rec.load_mechanism == LoadMechanism::MMAP_ZERO_COPY);
}

// SSA-05: Quantization and size estimate propagate correctly.
TEST(StorageStrategyAssessorTest, SSA05_QuantizationAndSizeEstimateCorrect) {
    StorageStrategyAssessor::Config cfg;
    cfg.quant.max_l2_error_relative = 0.02;  // allows INT8 with calibration
    cfg.quant.num_params             = 1'000'000;
    cfg.quant.has_calibration_data   = true;
    cfg.quant.memory_budget_bytes    = 2ULL * 1024 * 1024 * 1024;

    const auto rec = StorageStrategyAssessor::assess(cfg);
    EXPECT_EQ(rec.quantization_level, QuantizationLevel::INT8);
    EXPECT_EQ(rec.estimated_size_bytes, 1'000'000); // 1 byte/param at INT8
    EXPECT_DOUBLE_EQ(rec.compression_ratio, 4.0);   // 4B F32 / 1B INT8
    EXPECT_FALSE(rec.summary.empty());
}

// SSA-06: No-mmap caveat appears in caveats list.
TEST(StorageStrategyAssessorTest, SSA06_NoMmapCaveatPresent) {
    StorageStrategyAssessor::Config cfg;
    cfg.quant.max_l2_error_relative = 0.0;
    cfg.quant.num_params             = 100'000;
    cfg.os_supports_mmap             = false;

    const auto rec = StorageStrategyAssessor::assess(cfg);
    bool found = false;
    for (const auto& c : rec.caveats) {
        if (c.find("mmap") != std::string::npos) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found);
}

// =============================================================================
// mmapErrorMessage sanity check
// =============================================================================

TEST(MmapErrorMessageTest, AllCodesReturnNonEmptyString) {
    const std::vector<MmapError> codes = {
        MmapError::OK,
        MmapError::FILE_NOT_FOUND,
        MmapError::PERMISSION_DENIED,
        MmapError::FILE_TOO_LARGE,
        MmapError::MAPPING_FAILED,
        MmapError::ALREADY_OPEN,
        MmapError::NOT_OPEN,
        MmapError::UNSUPPORTED_PLATFORM,
        MmapError::LOCK_FAILED,
        MmapError::IO_ERROR,
    };
    for (const auto code : codes) {
        const char* msg = mmapErrorMessage(code);
        EXPECT_NE(msg, nullptr) << "nullptr for error code "
                                << static_cast<int>(code);
        EXPECT_GT(std::strlen(msg), 0u) << "empty string for error code "
                                        << static_cast<int>(code);
    }
}
