// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <gtest/gtest.h>

#include "training/lora_checkpoint_manager.h"

#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <vector>

using namespace themis::training;
namespace fs = std::filesystem;

namespace {

std::vector<uint8_t> makeDeterministicPayload(size_t size, uint32_t seed) {
    std::vector<uint8_t> payload(size);
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> dist(0, 255);
    for (auto& byte : payload) {
        byte = static_cast<uint8_t>(dist(rng));
    }
    return payload;
}

void writeBinaryFile(const fs::path& path, const std::vector<uint8_t>& data) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    ASSERT_TRUE(out.is_open());
    out.write(reinterpret_cast<const char*>(data.data()),
              static_cast<std::streamsize>(data.size()));
    ASSERT_TRUE(out.good());
}

} // namespace

class CheckpointResumeGpuStressTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto unique = std::to_string(std::random_device{}());
        root_dir_ = fs::temp_directory_path() / ("themis_training_gpu_cp_" + unique);
        source_dir_ = root_dir_ / "source";
        checkpoint_dir_ = root_dir_ / "checkpoints";
        fs::create_directories(source_dir_);
        fs::create_directories(checkpoint_dir_);
    }

    void TearDown() override {
        if (fs::exists(root_dir_)) {
            fs::remove_all(root_dir_);
        }
    }

    [[nodiscard]] CheckpointManagerConfig makeConfig() const {
        CheckpointManagerConfig cfg;
        cfg.checkpoint_dir = checkpoint_dir_.string();
        cfg.max_checkpoints = 16;
        cfg.validate_on_load = true;
        cfg.auto_rollback = true;
        return cfg;
    }

    [[nodiscard]] fs::path writeSourceCheckpoint(size_t index, size_t bytes) const {
        const fs::path source = source_dir_ / ("gpu_payload_" + std::to_string(index) + ".bin");
        writeBinaryFile(source, makeDeterministicPayload(bytes, 1000u + static_cast<uint32_t>(index)));
        return source;
    }

    fs::path root_dir_;
    fs::path source_dir_;
    fs::path checkpoint_dir_;
};

TEST_F(CheckpointResumeGpuStressTest, ResumeDeterministicAcrossRepeatedCalls) {
    LoRACheckpointManager mgr(makeConfig());

    const fs::path source = writeSourceCheckpoint(1, 512 * 1024);
    CheckpointManifestEntry meta;
    meta.epoch = 1;
    meta.step = 64;
    meta.loss = 0.8;
    meta.accuracy = 0.2;
    meta.adapter_version = "gpu_stress_v1";
    const auto saved = mgr.save(source.string(), meta);

    const auto first = mgr.resume();
    ASSERT_TRUE(first.has_value());

    for (int i = 0; i < 25; ++i) {
        const auto resumed = mgr.resume();
        ASSERT_TRUE(resumed.has_value());
        EXPECT_EQ(resumed->checkpoint_path, first->checkpoint_path);
        EXPECT_EQ(resumed->sha256, first->sha256);
        EXPECT_EQ(resumed->epoch, first->epoch);
        EXPECT_EQ(resumed->step, first->step);
    }

    EXPECT_TRUE(fs::exists(saved.checkpoint_path));
}

TEST_F(CheckpointResumeGpuStressTest, CorruptedLatestRollsBackUnderStress) {
    LoRACheckpointManager mgr(makeConfig());

    std::vector<CheckpointManifestEntry> saved_entries;
    saved_entries.reserve(10);
    for (size_t epoch = 1; epoch <= 10; ++epoch) {
        const fs::path source = writeSourceCheckpoint(epoch, 256 * 1024 + epoch * 1024);
        CheckpointManifestEntry meta;
        meta.epoch = epoch;
        meta.step = epoch * 128;
        meta.loss = 1.0 / static_cast<double>(epoch);
        meta.accuracy = 0.5 + static_cast<double>(epoch) * 0.01;
        meta.adapter_version = "gpu_stress";
        saved_entries.push_back(mgr.save(source.string(), meta));
    }

    ASSERT_FALSE(saved_entries.empty());
    const auto& newest = saved_entries.back();
    {
        std::ofstream corrupt(newest.checkpoint_path, std::ios::binary | std::ios::trunc);
        ASSERT_TRUE(corrupt.is_open());
        corrupt << "CORRUPTED_CHECKPOINT";
        ASSERT_TRUE(corrupt.good());
    }

    std::string diagnostics = {};
    const auto resumed = mgr.resumeWithDiagnostics(&diagnostics);
    ASSERT_TRUE(resumed.has_value());
    EXPECT_EQ(resumed->epoch, 9u);
    EXPECT_EQ(resumed->step, 9u * 128u);
    EXPECT_NE(diagnostics.find("CORRUPT"), std::string::npos);
}
