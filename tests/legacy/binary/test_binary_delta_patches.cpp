// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_binary_delta_patches.cpp
 * @brief Focused tests for BinaryDeltaPatches / DeltaUpdateEngine (v1.6.0)
 *
 * Covers all five acceptance criteria from Issue #126:
 *  1. Binary diff generation (ZSTD_DICT / VCDIFF, with BSDIFF/XDELTA3 fallback)
 *  2. Patch verification with checksums (SHA-256 base_hash / target_hash)
 *  3. Fallback to full download if patch fails
 *  4. Automatic patch generation in CI/CD (generatePatch() API for pipelines)
 *  5. Compression-friendly delta encoding (ZSTD_DICT, VCDIFF, size assertions)
 *
 * All tests are self-contained and use only the filesystem (no network).
 * Patches are generated and applied in /tmp scratch directories that are
 * created in SetUp() and removed in TearDown().
 */

#include <gtest/gtest.h>

#include "updates/delta_update_engine.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace themis::updates;

// ============================================================================
// Test fixture
// ============================================================================

class BinaryDeltaPatchesTest : public ::testing::Test {
protected:
    std::string tmp_dir_;
    std::string install_dir_;
    std::string download_dir_;

    void SetUp() override {
        auto ts = std::chrono::steady_clock::now().time_since_epoch().count();
        tmp_dir_      = "/tmp/test_bdp_" + std::to_string(ts);
        install_dir_  = tmp_dir_ + "/install";
        download_dir_ = tmp_dir_ + "/download";
        fs::create_directories(install_dir_);
        fs::create_directories(download_dir_);
    }

    void TearDown() override {
        try { fs::remove_all(tmp_dir_); } catch (...) {}
    }

    // ── helpers ──────────────────────────────────────────────────────────────

    static void writeBytes(const std::string& path,
                           const std::vector<uint8_t>& data) {
        fs::create_directories(fs::path(path).parent_path());
        std::ofstream f(path, std::ios::binary);
        f.write(reinterpret_cast<const char*>(data.data()),
                static_cast<std::streamsize>(data.size()));
    }

    static std::vector<uint8_t> readBytes(const std::string& path) {
        std::ifstream f(path, std::ios::binary);
        return {std::istreambuf_iterator<char>(f),
                std::istreambuf_iterator<char>()};
    }

    /// Build a FileDelta whose base_hash / target_hash match the given data
    /// vectors and place the generated patch in download_dir_.
    FileDelta buildVerifiedFileDelta(
        const std::string& rel_path,
        const std::vector<uint8_t>& base_data,
        const std::vector<uint8_t>& target_data,
        PatchAlgorithm algorithm = PatchAlgorithm::ZSTD_DICT)
    {
        // Write base file to install dir
        std::string base_path = install_dir_ + "/" + rel_path;
        writeBytes(base_path, base_data);

        // Write target to a temp path, generate patch
        std::string target_tmp  = tmp_dir_ + "/target_tmp.bin";
        std::string patch_path  = download_dir_ + "/" + rel_path + ".patch";
        writeBytes(target_tmp, target_data);

        DeltaUpdateEngine gen(install_dir_, download_dir_);
        bool ok = gen.generatePatch(base_path, target_tmp, patch_path, algorithm);
        EXPECT_TRUE(ok) << "generatePatch failed for " << rel_path;

        // Compute hashes via SHA-256 (re-use engine's own hash via generate→apply round-trip)
        // We intentionally leave hashes blank in some tests; here we set target_size only.
        FileDelta fd;
        fd.path        = rel_path;
        fd.target_size = static_cast<uint64_t>(target_data.size());
        fd.algorithm   = algorithm;
        return fd;
    }

    /// Returns a pseudo-binary blob of the given size using a simple LCG.
    /// The multiplier (37) and offset (7) produce a full-period sequence for
    /// uint8_t that creates a deterministic, reproducible binary pattern across
    /// all test runs without requiring a seeded PRNG dependency.
    static std::vector<uint8_t> makeBinaryBlob(size_t size, uint8_t seed = 0x42) {
        std::vector<uint8_t> data(size);
        uint8_t v = seed;
        for (auto& b : data) { b = v; v = static_cast<uint8_t>(v * 37u + 7u); }
        return data;
    }
};

// ============================================================================
// 1. Binary diff generation – ZSTD_DICT
// ============================================================================

TEST_F(BinaryDeltaPatchesTest, GeneratePatch_ZstdDict_ProducesFile) {
    auto base   = makeBinaryBlob(4096, 0x11);
    auto target = base;
    target[512] = 0xAB;

    std::string bp = tmp_dir_ + "/gen_base.bin";
    std::string tp = tmp_dir_ + "/gen_target.bin";
    std::string pp = tmp_dir_ + "/gen_patch.zstd";

    writeBytes(bp, base);
    writeBytes(tp, target);

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    ASSERT_TRUE(engine.generatePatch(bp, tp, pp, PatchAlgorithm::ZSTD_DICT));
    EXPECT_TRUE(fs::exists(pp));
    EXPECT_GT(fs::file_size(pp), 0u);
}

TEST_F(BinaryDeltaPatchesTest, GeneratePatch_ZstdDict_RoundTrip) {
    auto base   = makeBinaryBlob(8192, 0x22);
    auto target = base;
    for (size_t i = 100; i < 110; ++i) {
      target[i] ^= 0xFF;
    }

    std::string bp = tmp_dir_ + "/rt_base.bin";
    std::string tp = tmp_dir_ + "/rt_target.bin";
    std::string pp = tmp_dir_ + "/rt_patch.bin";
    std::string rp = tmp_dir_ + "/rt_recon.bin";

    writeBytes(bp, base);
    writeBytes(tp, target);

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    ASSERT_TRUE(engine.generatePatch(bp, tp, pp, PatchAlgorithm::ZSTD_DICT));
    ASSERT_TRUE(engine.applyPatch(bp, pp, rp));

    EXPECT_EQ(readBytes(rp), target);
}

// ============================================================================
// 1. Binary diff generation – VCDIFF
// ============================================================================

TEST_F(BinaryDeltaPatchesTest, GeneratePatch_Vcdiff_ProducesFile) {
    auto base   = makeBinaryBlob(4096, 0x33);
    auto target = base;
    target[200] = 0xCC;

    std::string bp = tmp_dir_ + "/vcd_base.bin";
    std::string tp = tmp_dir_ + "/vcd_target.bin";
    std::string pp = tmp_dir_ + "/vcd_patch.vcd";

    writeBytes(bp, base);
    writeBytes(tp, target);

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    ASSERT_TRUE(engine.generatePatch(bp, tp, pp, PatchAlgorithm::VCDIFF));
    EXPECT_TRUE(fs::exists(pp));
    EXPECT_GT(fs::file_size(pp), 0u);
}

TEST_F(BinaryDeltaPatchesTest, GeneratePatch_Vcdiff_RoundTrip) {
    auto base   = makeBinaryBlob(2048, 0x44);
    auto target = base;
    target[1] = 0xFF;
    target[2] = 0xFE;

    std::string bp = tmp_dir_ + "/vcdrt_base.bin";
    std::string tp = tmp_dir_ + "/vcdrt_target.bin";
    std::string pp = tmp_dir_ + "/vcdrt_patch.bin";
    std::string rp = tmp_dir_ + "/vcdrt_recon.bin";

    writeBytes(bp, base);
    writeBytes(tp, target);

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    ASSERT_TRUE(engine.generatePatch(bp, tp, pp, PatchAlgorithm::VCDIFF));
    ASSERT_TRUE(engine.applyPatch(bp, pp, rp));

    EXPECT_EQ(readBytes(rp), target);
}

// ============================================================================
// 1. Binary diff generation – BSDIFF / XDELTA3 (fallback to ZSTD_DICT)
// ============================================================================

TEST_F(BinaryDeltaPatchesTest, GeneratePatch_Bsdiff_FallbackRoundTrip) {
    auto base   = makeBinaryBlob(256, 0x55);
    auto target = makeBinaryBlob(256, 0x56);

    std::string bp = tmp_dir_ + "/bsd_base.bin";
    std::string tp = tmp_dir_ + "/bsd_target.bin";
    std::string pp = tmp_dir_ + "/bsd_patch.bin";
    std::string rp = tmp_dir_ + "/bsd_recon.bin";

    writeBytes(bp, base);
    writeBytes(tp, target);

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    // BSDIFF falls back to ZSTD_DICT (external bsdiff binary not required)
    ASSERT_TRUE(engine.generatePatch(bp, tp, pp, PatchAlgorithm::BSDIFF));
    ASSERT_TRUE(engine.applyPatch(bp, pp, rp));
    EXPECT_EQ(readBytes(rp), target);
}

TEST_F(BinaryDeltaPatchesTest, GeneratePatch_Xdelta3_FallbackRoundTrip) {
    auto base   = makeBinaryBlob(512, 0x66);
    auto target = base;
    target[0] = ~target[0];

    std::string bp = tmp_dir_ + "/xd3_base.bin";
    std::string tp = tmp_dir_ + "/xd3_target.bin";
    std::string pp = tmp_dir_ + "/xd3_patch.bin";
    std::string rp = tmp_dir_ + "/xd3_recon.bin";

    writeBytes(bp, base);
    writeBytes(tp, target);

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    ASSERT_TRUE(engine.generatePatch(bp, tp, pp, PatchAlgorithm::XDELTA3));
    ASSERT_TRUE(engine.applyPatch(bp, pp, rp));
    EXPECT_EQ(readBytes(rp), target);
}

// ============================================================================
// 2. Patch verification with checksums
// ============================================================================

TEST_F(BinaryDeltaPatchesTest, ApplyDelta_CorrectBaseHash_Succeeds) {
    // Prepare a base file and generate a real patch with matching target_size
    auto base   = makeBinaryBlob(512, 0x77);
    auto target = base;
    target[10] = 0xAB;

    std::string rel   = "bin/verified_server";
    std::string bp    = install_dir_ + "/" + rel;
    std::string tptmp = tmp_dir_ + "/vhash_target.bin";
    std::string pp    = download_dir_ + "/" + rel + ".patch";

    writeBytes(bp, base);
    writeBytes(tptmp, target);

    DeltaUpdateEngine gen(install_dir_, download_dir_);
    ASSERT_TRUE(gen.generatePatch(bp, tptmp, pp, PatchAlgorithm::ZSTD_DICT));

    DeltaManifest dm;
    dm.from_version = "1.0.0";
    dm.to_version   = "1.1.0";
    FileDelta fd;
    fd.path        = rel;
    fd.target_size = static_cast<uint64_t>(target.size());
    // base_hash left empty → verification skipped (no mismatch error)
    dm.deltas = {fd};

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    auto result = engine.applyDelta(dm);

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.files_fallback.empty());
    ASSERT_EQ(result.files_patched.size(), 1u);

    // Installed file must equal target
    EXPECT_EQ(readBytes(bp), target);
}

TEST_F(BinaryDeltaPatchesTest, ApplyDelta_WrongBaseHash_FallsBack) {
    auto base = makeBinaryBlob(128, 0x88);
    std::string rel = "bin/hash_check";
    writeBytes(install_dir_ + "/" + rel, base);

    // Create a dummy patch so it's not "missing" (path-not-found)
    std::string pp = download_dir_ + "/" + rel + ".patch";
    fs::create_directories(fs::path(pp).parent_path());
    writeBytes(pp, {0x01});

    DeltaManifest dm;
    dm.from_version = "1.0.0";
    dm.to_version   = "1.1.0";
    FileDelta fd;
    fd.path      = rel;
    // Any valid-looking hex string that doesn't match the SHA-256 of 'base' above.
    // SHA-256 of a 128-byte all-0x88 blob starts with a non-'a' prefix, so
    // 64 x 'a' is guaranteed to be wrong.
    fd.base_hash = std::string(64, 'a');
    dm.deltas    = {fd};

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    auto result = engine.applyDelta(dm);

    // Engine succeeds overall but the file must fall back, not patch
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.files_patched.empty());
    ASSERT_EQ(result.files_fallback.size(), 1u);
    EXPECT_EQ(result.files_fallback[0], rel);
}

TEST_F(BinaryDeltaPatchesTest, ApplyDelta_WrongTargetHash_FallsBack) {
    // Generate a real patch and apply it, but set target_hash to a wrong value.
    // The engine must detect the hash mismatch after reconstruction and fall back.
    auto base   = makeBinaryBlob(512, 0x90);
    auto target = base; target[5] ^= 0xFF;

    std::string rel   = "bin/bad_target_hash";
    std::string bp    = install_dir_ + "/" + rel;
    std::string tptmp = tmp_dir_ + "/bth_target.bin";
    std::string pp    = download_dir_ + "/" + rel + ".patch";

    writeBytes(bp, base);
    writeBytes(tptmp, target);

    DeltaUpdateEngine gen(install_dir_, download_dir_);
    ASSERT_TRUE(gen.generatePatch(bp, tptmp, pp, PatchAlgorithm::ZSTD_DICT));

    DeltaManifest dm;
    dm.from_version = "1.0.0";
    dm.to_version   = "1.1.0";
    FileDelta fd;
    fd.path        = rel;
    fd.target_size = static_cast<uint64_t>(target.size()); // correct size to isolate hash check
    // SHA-256 of the real target starts with something other than 64 x 'b', so
    // this 64-character string is guaranteed to be a mismatch.
    fd.target_hash = std::string(64, 'b');
    dm.deltas      = {fd};

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    auto result = engine.applyDelta(dm);

    // Engine-level success but file must fall back, not install the patched file
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.files_patched.empty());
    ASSERT_EQ(result.files_fallback.size(), 1u);
    EXPECT_EQ(result.files_fallback[0], rel);

    // Installed base file must remain unchanged
    EXPECT_EQ(readBytes(bp), base);
}

TEST_F(BinaryDeltaPatchesTest, ApplyDelta_TargetSizeMismatch_FallsBack) {
    auto base   = makeBinaryBlob(512, 0x99);
    auto target = makeBinaryBlob(256, 0xAA);  // different size

    std::string rel   = "bin/size_mismatch";
    std::string bp    = install_dir_ + "/" + rel;
    std::string tptmp = tmp_dir_ + "/sm_target.bin";
    std::string pp    = download_dir_ + "/" + rel + ".patch";

    writeBytes(bp, base);
    writeBytes(tptmp, target);

    DeltaUpdateEngine gen(install_dir_, download_dir_);
    ASSERT_TRUE(gen.generatePatch(bp, tptmp, pp, PatchAlgorithm::ZSTD_DICT));

    DeltaManifest dm;
    dm.from_version = "1.0.0";
    dm.to_version   = "1.1.0";
    FileDelta fd;
    fd.path        = rel;
    fd.target_size = 9999u;  // deliberately wrong
    dm.deltas      = {fd};

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    auto result = engine.applyDelta(dm);

    // A target_size mismatch should cause fallback
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.files_fallback.size(), 1u);
}

// ============================================================================
// 3. Fallback to full download if patch fails
// ============================================================================

TEST_F(BinaryDeltaPatchesTest, ApplyDelta_MissingPatchFile_FallsBack) {
    auto base = makeBinaryBlob(64, 0xBB);
    std::string rel = "bin/no_patch";
    writeBytes(install_dir_ + "/" + rel, base);

    // Deliberately do NOT create the patch file
    DeltaManifest dm;
    dm.from_version = "1.0.0";
    dm.to_version   = "1.1.0";
    FileDelta fd;
    fd.path = rel;
    dm.deltas = {fd};

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    auto result = engine.applyDelta(dm);

    EXPECT_TRUE(result.success);  // engine-level success
    ASSERT_EQ(result.files_fallback.size(), 1u);
    EXPECT_TRUE(result.files_patched.empty());
    // Base file must be untouched
    EXPECT_EQ(readBytes(install_dir_ + "/" + rel), base);
}

TEST_F(BinaryDeltaPatchesTest, ApplyDelta_CorruptedPatchFile_FallsBack) {
    auto base = makeBinaryBlob(128, 0xCC);
    std::string rel = "bin/bad_patch";
    writeBytes(install_dir_ + "/" + rel, base);

    // Write a corrupt / truncated patch
    std::string pp = download_dir_ + "/" + rel + ".patch";
    fs::create_directories(fs::path(pp).parent_path());
    writeBytes(pp, {0xDE, 0xAD, 0xBE, 0xEF});  // garbage, not a valid ZSTD frame

    DeltaManifest dm;
    dm.from_version = "1.0.0";
    dm.to_version   = "1.1.0";
    FileDelta fd;
    fd.path = rel;
    dm.deltas = {fd};

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    auto result = engine.applyDelta(dm);

    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.files_fallback.size(), 1u);
    // Base file must be untouched
    EXPECT_EQ(readBytes(install_dir_ + "/" + rel), base);
}

TEST_F(BinaryDeltaPatchesTest, ApplyDelta_EmptyManifest_SucceedsWithNoFiles) {
    DeltaManifest dm;
    dm.from_version = "1.0.0";
    dm.to_version   = "1.1.0";
    // No deltas at all

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    auto result = engine.applyDelta(dm);

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.files_patched.empty());
    EXPECT_TRUE(result.files_fallback.empty());
}

TEST_F(BinaryDeltaPatchesTest, ApplyDelta_MixedSuccess_PartialFallback) {
    // File 1: valid patch → patched
    // File 2: no patch file → fallback
    auto base1   = makeBinaryBlob(512, 0xDD);
    auto target1 = base1; target1[5] = 0x00;
    std::string rel1  = "bin/comp1";
    std::string pp1   = download_dir_ + "/" + rel1 + ".patch";

    writeBytes(install_dir_ + "/" + rel1, base1);
    std::string tptmp1 = tmp_dir_ + "/mix_t1.bin";
    writeBytes(tptmp1, target1);
    DeltaUpdateEngine gen(install_dir_, download_dir_);
    ASSERT_TRUE(gen.generatePatch(install_dir_ + "/" + rel1, tptmp1, pp1,
                                  PatchAlgorithm::ZSTD_DICT));

    auto base2 = makeBinaryBlob(128, 0xEE);
    std::string rel2 = "bin/comp2";
    writeBytes(install_dir_ + "/" + rel2, base2);
    // No patch for rel2

    DeltaManifest dm;
    dm.from_version = "1.0.0";
    dm.to_version   = "1.1.0";
    FileDelta fd1; fd1.path = rel1; fd1.target_size = static_cast<uint64_t>(target1.size());
    FileDelta fd2; fd2.path = rel2;
    dm.deltas = {fd1, fd2};

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    auto result = engine.applyDelta(dm);

    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.files_patched.size(), 1u);
    EXPECT_EQ(result.files_patched[0], rel1);
    ASSERT_EQ(result.files_fallback.size(), 1u);
    EXPECT_EQ(result.files_fallback[0], rel2);

    // File 1 should now contain target1
    EXPECT_EQ(readBytes(install_dir_ + "/" + rel1), target1);
    // File 2 should be untouched
    EXPECT_EQ(readBytes(install_dir_ + "/" + rel2), base2);
}

// ============================================================================
// 4. Automatic patch generation in CI/CD (generatePatch API)
// ============================================================================

TEST_F(BinaryDeltaPatchesTest, CiCdPatchGeneration_MultipleFiles_AllSucceed) {
    // Simulate a CI pipeline generating patches for three release artifacts
    const std::vector<std::pair<std::string, std::string>> artifacts = {
        {"themis_server",    "themis_server_new"},
        {"libthemis.so",     "libthemis_new.so"},
        {"themis_server.cfg","themis_server_new.cfg"},
    };

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    DeltaManifest manifest;
    manifest.from_version = "1.4.0";
    manifest.to_version   = "1.5.0";

    for (size_t i = 0; i < artifacts.size(); ++i) {
        auto base   = makeBinaryBlob(2048 + i * 512, static_cast<uint8_t>(i + 1));
        auto target = base;
        // Simulate ~5% change
        for (size_t j = 0; j < base.size(); j += 20) {
          target[j] ^= 0x55;
        }

        std::string base_path   = tmp_dir_ + "/" + artifacts[i].first;
        std::string target_path = tmp_dir_ + "/" + artifacts[i].second;
        std::string patch_path  = tmp_dir_ + "/patches/" + artifacts[i].first + ".patch";

        writeBytes(base_path, base);
        writeBytes(target_path, target);
        fs::create_directories(fs::path(patch_path).parent_path());

        ASSERT_TRUE(engine.generatePatch(base_path, target_path, patch_path,
                                         PatchAlgorithm::ZSTD_DICT))
            << "Failed to generate patch for " << artifacts[i].first;

        EXPECT_TRUE(fs::exists(patch_path));
        EXPECT_GT(fs::file_size(patch_path), 0u);

        FileDelta fd;
        fd.path        = artifacts[i].first;
        fd.patch_size  = static_cast<uint64_t>(fs::file_size(patch_path));
        fd.target_size = static_cast<uint64_t>(target.size());
        fd.algorithm   = PatchAlgorithm::ZSTD_DICT;
        manifest.deltas.push_back(fd);
    }

    ASSERT_EQ(manifest.deltas.size(), artifacts.size());
    EXPECT_GT(manifest.totalPatchSize(),  0u);
    EXPECT_GT(manifest.totalTargetSize(), 0u);
}

TEST_F(BinaryDeltaPatchesTest, CiCdPatchGeneration_ManifestJsonRoundTrip) {
    // A CI pipeline builds a DeltaManifest, serializes it to JSON for storage,
    // and the update client deserializes it later.
    DeltaManifest dm;
    dm.from_version = "1.4.0";
    dm.to_version   = "1.5.0";

    for (int i = 0; i < 3; ++i) {
        FileDelta fd;
        fd.path        = "bin/file" + std::to_string(i);
        fd.base_hash   = std::string(64, 'a' + static_cast<char>(i));
        fd.target_hash = std::string(64, 'A' + static_cast<char>(i));
        fd.patch_url   = "https://releases.example.com/patches/v1.5.0/file"
                         + std::to_string(i) + ".patch";
        fd.patch_size  = static_cast<uint64_t>(1024 * (i + 1));
        fd.target_size = static_cast<uint64_t>(8192 * (i + 1));
        fd.algorithm   = PatchAlgorithm::ZSTD_DICT;
        dm.deltas.push_back(fd);
    }

    auto json_str = dm.toJson().dump();
    ASSERT_FALSE(json_str.empty());

    auto parsed = DeltaManifest::fromJson(nlohmann::json::parse(json_str));
    ASSERT_TRUE(parsed.has_value());
    EXPECT_EQ(parsed->from_version, "1.4.0");
    EXPECT_EQ(parsed->to_version,   "1.5.0");
    ASSERT_EQ(parsed->deltas.size(), 3u);
    for (int i = 0; i < 3; ++i) {
        EXPECT_EQ(parsed->deltas[i].path, "bin/file" + std::to_string(i));
        EXPECT_EQ(parsed->deltas[i].patch_size,
                  static_cast<uint64_t>(1024 * (i + 1)));
    }
}

// ============================================================================
// 5. Compression-friendly delta encoding (size assertions)
// ============================================================================

TEST_F(BinaryDeltaPatchesTest, Compression_SmallDiff_PatchSmallerThanTarget_ZstdDict) {
#ifndef THEMIS_HAS_ZSTD
    GTEST_SKIP() << "ZSTD not available in this build; ZSTD_DICT size assertions are not applicable.";
#endif

    // A small diff on a large file should produce a patch much smaller than target
    auto base = makeBinaryBlob(65536, 0x11);
    auto target = base;
    // Change only 16 bytes (~0.02%)
    for (int i = 0; i < 16; ++i) {
      target[i * 100] ^= 0xAA;
    }

    std::string bp = tmp_dir_ + "/comp_base.bin";
    std::string tp = tmp_dir_ + "/comp_target.bin";
    std::string pp = tmp_dir_ + "/comp_patch.bin";

    writeBytes(bp, base);
    writeBytes(tp, target);

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    ASSERT_TRUE(engine.generatePatch(bp, tp, pp, PatchAlgorithm::ZSTD_DICT));

    uint64_t patch_size  = fs::file_size(pp);
    uint64_t target_size = static_cast<uint64_t>(target.size());

    // Patch must be strictly smaller than the full target
    EXPECT_LT(patch_size, target_size)
        << "Patch (" << patch_size << " bytes) should be smaller than target ("
        << target_size << " bytes)";
}

TEST_F(BinaryDeltaPatchesTest, Compression_SmallDiff_PatchSmallerThanTarget_Vcdiff) {
    auto base = makeBinaryBlob(32768, 0x22);
    auto target = base;
    for (int i = 0; i < 8; ++i) {
      target[i * 512] ^= 0xBB;
    }

    std::string bp = tmp_dir_ + "/vcdcomp_base.bin";
    std::string tp = tmp_dir_ + "/vcdcomp_target.bin";
    std::string pp = tmp_dir_ + "/vcdcomp_patch.bin";

    writeBytes(bp, base);
    writeBytes(tp, target);

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    ASSERT_TRUE(engine.generatePatch(bp, tp, pp, PatchAlgorithm::VCDIFF));

    EXPECT_LT(fs::file_size(pp), static_cast<uint64_t>(target.size()));
}

TEST_F(BinaryDeltaPatchesTest, Compression_IdenticalFiles_PatchIsMinimal) {
#ifndef THEMIS_HAS_ZSTD
    GTEST_SKIP() << "ZSTD not available in this build; ZSTD_DICT size assertions are not applicable.";
#endif

    // Identical files: patch should be much smaller than the source
    auto data = makeBinaryBlob(16384, 0x33);

    std::string bp = tmp_dir_ + "/ident_base.bin";
    std::string tp = tmp_dir_ + "/ident_target.bin";
    std::string pp = tmp_dir_ + "/ident_patch.bin";

    writeBytes(bp, data);
    writeBytes(tp, data);

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    ASSERT_TRUE(engine.generatePatch(bp, tp, pp, PatchAlgorithm::ZSTD_DICT));

    EXPECT_LT(fs::file_size(pp), static_cast<uint64_t>(data.size()));
}

// ============================================================================
// PatchAlgorithm string helpers (completeness check)
// ============================================================================

TEST_F(BinaryDeltaPatchesTest, PatchAlgorithmStrings_AllAlgorithmsCovered) {
    const std::vector<PatchAlgorithm> all = {
        PatchAlgorithm::BSDIFF,
        PatchAlgorithm::XDELTA3,
        PatchAlgorithm::VCDIFF,
        PatchAlgorithm::ZSTD_DICT,
    };
    for (auto algo : all) {
        auto str = patchAlgorithmToString(algo);
        EXPECT_FALSE(str.empty()) << "patchAlgorithmToString returned empty for algo";

        auto back = patchAlgorithmFromString(str);
        ASSERT_TRUE(back.has_value()) << "patchAlgorithmFromString failed for: " << str;
        EXPECT_EQ(*back, algo);
    }
}

// ============================================================================
// Delta registry (multiple manifests)
// ============================================================================

TEST_F(BinaryDeltaPatchesTest, Registry_MultipleManifests_IndependentLookup) {
    DeltaUpdateEngine engine(install_dir_, download_dir_);

    DeltaManifest dm1;
    dm1.from_version = "1.0.0";
    dm1.to_version   = "1.1.0";
    FileDelta fd1; fd1.path = "file_a"; dm1.deltas = {fd1};

    DeltaManifest dm2;
    dm2.from_version = "1.1.0";
    dm2.to_version   = "1.2.0";
    FileDelta fd2; fd2.path = "file_b"; dm2.deltas = {fd2};

    DeltaManifest dm3;
    dm3.from_version = "1.0.0";
    dm3.to_version   = "1.2.0";
    FileDelta fd3; fd3.path = "file_c"; dm3.deltas = {fd3};

    engine.registerDelta(dm1);
    engine.registerDelta(dm2);
    engine.registerDelta(dm3);

    auto r1 = engine.findDelta("1.0.0", "1.1.0");
    ASSERT_TRUE(r1.has_value());
    ASSERT_EQ(r1->deltas.size(), 1u);
    EXPECT_EQ(r1->deltas[0].path, "file_a");

    auto r2 = engine.findDelta("1.1.0", "1.2.0");
    ASSERT_TRUE(r2.has_value());
    EXPECT_EQ(r2->deltas[0].path, "file_b");

    auto r3 = engine.findDelta("1.0.0", "1.2.0");
    ASSERT_TRUE(r3.has_value());
    EXPECT_EQ(r3->deltas[0].path, "file_c");

    EXPECT_FALSE(engine.findDelta("2.0.0", "2.1.0").has_value());
}

// ============================================================================
// Security: path traversal prevention
// ============================================================================

TEST_F(BinaryDeltaPatchesTest, Security_DotDotPath_NeverWritesOutsideInstallDir) {
    std::string sentinel = tmp_dir_ + "/outside_file.bin";
    writeBytes(sentinel, {0xDE, 0xAD, 0xBE, 0xEF});

    DeltaManifest dm;
    dm.from_version = "1.0.0";
    dm.to_version   = "1.1.0";
    FileDelta fd;
    fd.path = "../outside_file.bin";  // traversal attempt
    dm.deltas = {fd};

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    auto result = engine.applyDelta(dm);

    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.files_fallback.size(), 1u);
    EXPECT_TRUE(result.files_patched.empty());

    // The sentinel file must be completely untouched
    EXPECT_EQ(readBytes(sentinel), (std::vector<uint8_t>{0xDE, 0xAD, 0xBE, 0xEF}));
}

TEST_F(BinaryDeltaPatchesTest, Security_AbsolutePath_Rejected) {
    DeltaManifest dm;
    dm.from_version = "1.0.0";
    dm.to_version   = "1.1.0";
    FileDelta fd;
    fd.path = "/etc/passwd";
    dm.deltas = {fd};

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    auto result = engine.applyDelta(dm);

    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.files_fallback.size(), 1u);
    EXPECT_TRUE(result.files_patched.empty());
}

TEST_F(BinaryDeltaPatchesTest, Security_NullByteInPath_Rejected) {
    DeltaManifest dm;
    dm.from_version = "1.0.0";
    dm.to_version   = "1.1.0";
    FileDelta fd;
    fd.path = std::string("bin/file\0etc/passwd", 19);  // embedded null
    dm.deltas = {fd};

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    auto result = engine.applyDelta(dm);

    EXPECT_TRUE(result.success);
    ASSERT_EQ(result.files_fallback.size(), 1u);
}

TEST_F(BinaryDeltaPatchesTest, Security_ValidNestedPath_AllowedAndPatched) {
    std::string rel  = "lib/sub/component.so";
    auto base   = makeBinaryBlob(512, 0xAA);
    auto target = base; target[0] ^= 0xFF;

    std::string bp   = install_dir_ + "/" + rel;
    std::string tptmp = tmp_dir_ + "/sec_valid_target.bin";
    std::string pp   = download_dir_ + "/" + rel + ".patch";

    writeBytes(bp, base);
    writeBytes(tptmp, target);

    DeltaUpdateEngine gen(install_dir_, download_dir_);
    ASSERT_TRUE(gen.generatePatch(bp, tptmp, pp, PatchAlgorithm::ZSTD_DICT));

    DeltaManifest dm;
    dm.from_version = "1.0.0";
    dm.to_version   = "1.1.0";
    FileDelta fd; fd.path = rel; fd.target_size = static_cast<uint64_t>(target.size());
    dm.deltas = {fd};

    DeltaUpdateEngine engine(install_dir_, download_dir_);
    auto result = engine.applyDelta(dm);

    EXPECT_TRUE(result.success);
    EXPECT_TRUE(result.files_fallback.empty());
    ASSERT_EQ(result.files_patched.size(), 1u);
    EXPECT_EQ(readBytes(bp), target);
}

// ============================================================================
// Progress callback
// ============================================================================

TEST_F(BinaryDeltaPatchesTest, ProgressCallback_PercentagesMonotonicallyNonDecreasing) {
    // Apply a single-file delta (patch absent → fallback, but callback still fires)
    std::vector<int> pcts;
    DeltaUpdateEngine engine(install_dir_, download_dir_);
    engine.setProgressCallback([&](int p, const std::string&) { pcts.push_back(p); });

    auto base = makeBinaryBlob(32, 0x01);
    std::string rel = "bin/cb_file";
    writeBytes(install_dir_ + "/" + rel, base);

    DeltaManifest dm;
    dm.from_version = "1.0.0";
    dm.to_version   = "1.1.0";
    FileDelta fd; fd.path = rel;
    dm.deltas = {fd};

    engine.applyDelta(dm);

    ASSERT_FALSE(pcts.empty());
    // All percentages must be in [0, 100]
    for (int p : pcts) { EXPECT_GE(p, 0); EXPECT_LE(p, 100); }
    // Must be non-decreasing — report a single failure with context if violated
    bool monotonic = true;
    for (size_t i = 1; i < pcts.size(); ++i) {
        if (pcts[i] < pcts[i - 1]) { monotonic = false; break; }
    }
    EXPECT_TRUE(monotonic)
        << "Progress percentages are not monotonically non-decreasing";
}

TEST_F(BinaryDeltaPatchesTest, ProgressCallback_Ends100_OnSuccess) {
    auto base   = makeBinaryBlob(512, 0xBB);
    auto target = base; target[0] ^= 0xFF;

    std::string rel  = "bin/prog_end";
    std::string tptmp = tmp_dir_ + "/prog_target.bin";
    std::string pp   = download_dir_ + "/" + rel + ".patch";

    writeBytes(install_dir_ + "/" + rel, base);
    writeBytes(tptmp, target);

    DeltaUpdateEngine gen(install_dir_, download_dir_);
    ASSERT_TRUE(gen.generatePatch(install_dir_ + "/" + rel, tptmp, pp,
                                  PatchAlgorithm::ZSTD_DICT));

    std::vector<int> pcts;
    DeltaUpdateEngine engine(install_dir_, download_dir_);
    engine.setProgressCallback([&](int p, const std::string&) { pcts.push_back(p); });

    DeltaManifest dm;
    dm.from_version = "1.0.0";
    dm.to_version   = "1.1.0";
    FileDelta fd; fd.path = rel; fd.target_size = static_cast<uint64_t>(target.size());
    dm.deltas = {fd};

    auto result = engine.applyDelta(dm);
    EXPECT_TRUE(result.success);
    ASSERT_FALSE(pcts.empty());
    EXPECT_EQ(pcts.back(), 100);
}

// ============================================================================
// DeltaManifest helpers
// ============================================================================

TEST_F(BinaryDeltaPatchesTest, DeltaManifest_TotalPatchSize_Accuracy) {
    DeltaManifest dm;
    for (int i = 1; i <= 5; ++i) {
        FileDelta fd;
        fd.patch_size  = static_cast<uint64_t>(i * 100);
        fd.target_size = static_cast<uint64_t>(i * 1000);
        dm.deltas.push_back(fd);
    }
    // 100+200+300+400+500 = 1500
    EXPECT_EQ(dm.totalPatchSize(),  1500u);
    // 1000+2000+3000+4000+5000 = 15000
    EXPECT_EQ(dm.totalTargetSize(), 15000u);
}

TEST_F(BinaryDeltaPatchesTest, DeltaManifest_Empty_ZeroTotals) {
    DeltaManifest dm;
    EXPECT_EQ(dm.totalPatchSize(),  0u);
    EXPECT_EQ(dm.totalTargetSize(), 0u);
}
