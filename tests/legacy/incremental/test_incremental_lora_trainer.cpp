/**
 * @file test_incremental_lora_trainer.cpp
 * @brief Unit tests for IncrementalLoRATrainer federation bridges (IMPL-A3).
 *
 * Tests ILT-EG-01…03  — exportGradient() contract
 * Tests ILT-AG-01…02  — applyGlobalDelta() contract
 */

#include <gtest/gtest.h>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "training/lora_checkpoint_manager.h"
#include "training/incremental_lora_trainer.h"
#include "distributed_knowledge/lora_federation_coordinator.h"

using namespace themis::training;
using namespace themis::distributed_knowledge;
namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static IncrementalTrainingConfig makeConfig() {
    IncrementalTrainingConfig cfg;
    cfg.training_data_collection = "";
    cfg.base_model_path          = "";
    cfg.adapter_version          = "";
    cfg.rank                     = 4;
    cfg.alpha                    = 8.0f;
    cfg.learning_rate            = 0.001f;
    cfg.batch_size               = 2;
    cfg.num_epochs               = 1;
    cfg.max_seq_length           = 16;
    cfg.device                   = "cpu";
    return cfg;
}

// Run one training cycle so the gradient accumulator is non-empty.
static TrainingResult runOneTrain(IncrementalLoRATrainer& trainer) {
    return trainer.train(TrainingMode::INITIAL);
}

static void writeCheckpointMetadata(const std::string& checkpoint_prefix,
                                    const std::string& version,
                                    size_t epoch,
                                    size_t step) {
    std::ofstream metadata(checkpoint_prefix + "_metadata.txt");
    ASSERT_TRUE(metadata.is_open());
    metadata << "version=" << version << "\n";
    metadata << "epoch=" << epoch << "\n";
    metadata << "step=" << step << "\n";
    metadata << "loss=0.1\n";
    metadata << "accuracy=0.2\n";
}

static void registerCheckpointManifest(const fs::path& checkpoint_dir,
                                       const std::string& version,
                                       size_t epoch,
                                       size_t step,
                                       const fs::path& payload_path) {
    CheckpointManagerConfig manager_cfg;
    manager_cfg.checkpoint_dir = checkpoint_dir.string();
    LoRACheckpointManager manager(manager_cfg);
    CheckpointManifestEntry manifest_entry;
    manifest_entry.adapter_version = version;
    manifest_entry.epoch = epoch;
    manifest_entry.step = step;
    manifest_entry.loss = 0.1;
    manifest_entry.accuracy = 0.2;
    manager.save(payload_path.string(), manifest_entry);
}

static void writeManifestEntry(const fs::path& checkpoint_dir,
                               const std::string& checkpoint_path,
                               const std::string& sha256,
                               const std::string& version,
                               size_t epoch,
                               size_t step) {
    std::ofstream manifest(checkpoint_dir / "checkpoint_manifest.json", std::ios::trunc);
    ASSERT_TRUE(manifest.is_open());
    manifest << "checkpoint_path=" << checkpoint_path << "\n";
    manifest << "sha256=" << sha256 << "\n";
    manifest << "base_model_hash=\n";
    manifest << "adapter_version=" << version << "\n";
    manifest << "epoch=" << epoch << "\n";
    manifest << "step=" << step << "\n";
    manifest << "loss=0.1\n";
    manifest << "accuracy=0.2\n";
    manifest << "saved_at=1\n";
    manifest << "---\n";
}

static void writeTwoMatrixWeights(const fs::path& output_path,
                                  const std::array<float, 4>& first,
                                  const std::array<float, 4>& second) {
    std::ofstream out(output_path, std::ios::binary);
    ASSERT_TRUE(out.is_open());

    const auto writeMatrix = [&out](const std::array<float, 4>& values) {
        const uint32_t rows = 2;
        const uint32_t cols = 2;
        out.write(reinterpret_cast<const char*>(&rows), sizeof(rows));
        out.write(reinterpret_cast<const char*>(&cols), sizeof(cols));
        out.write(reinterpret_cast<const char*>(values.data()),
                  static_cast<std::streamsize>(values.size() * sizeof(float)));
    };

    writeMatrix(first);
    writeMatrix(second);
}

static fs::path makeUniqueTempDir(const std::string& prefix) {
    const auto unique_suffix =
        std::to_string(static_cast<long long>(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    const fs::path temp_dir = fs::temp_directory_path() /
        (prefix + "_" + unique_suffix);
    fs::create_directories(temp_dir);
    return temp_dir;
}

class TestRouter final : public ILLMRouter {
public:
    bool available = true;
    bool accept_weight = true;
    std::string last_version;
    float last_weight = -1.0f;

    bool setAdapterWeight(const std::string& version, float weight) override {
        last_version = version;
        last_weight = weight;
        return accept_weight;
    }

    bool isAvailable() const override {
        return available;
    }

    std::string activeVersion() const override {
        return last_version;
    }
};

class ThrowingRouter final : public ILLMRouter {
public:
    bool throw_on_is_available = false;
    bool throw_on_set_weight = false;
    int is_available_calls = 0;
    int set_weight_calls = 0;
    std::string last_version;

    bool setAdapterWeight(const std::string& version, float) override {
        ++set_weight_calls;
        last_version = version;
        if (throw_on_set_weight) {
            throw std::runtime_error("setAdapterWeight failure");
        }
        return true;
    }

    bool isAvailable() const override {
        auto* self = const_cast<ThrowingRouter*>(this);
        ++self->is_available_calls;
        if (throw_on_is_available) {
            throw std::runtime_error("isAvailable failure");
        }
        return true;
    }

    std::string activeVersion() const override {
        return last_version;
    }
};

// ---------------------------------------------------------------------------
// ILT-EG-01 — exportGradient() after at least one train() call gives
//             non-empty data
// ---------------------------------------------------------------------------

TEST(ImplA3_ExportGradient, EG01_NonEmptyDataAfterTraining) {
    IncrementalLoRATrainer trainer(makeConfig(), "");
    trainer.setShardId("shard_test_01");

    auto result = runOneTrain(trainer);
    EXPECT_TRUE(result.success);

    EncryptedGradient grad = trainer.exportGradient(/*round=*/1);

    EXPECT_EQ(grad.shard_id, "shard_test_01");
    EXPECT_EQ(grad.round, 1u);
    EXPECT_GT(grad.sample_count, 0u);
    EXPECT_FALSE(grad.data.empty()) << "Gradient data map must be non-empty after training";

    // Verify the expected layer names are present
    EXPECT_TRUE(grad.data.contains("lora_A_layer_0"));
    EXPECT_TRUE(grad.data.contains("lora_B_layer_0"));
}

// ---------------------------------------------------------------------------
// ILT-EG-02 — exportGradient() throws std::runtime_error when no training
//             has occurred since the last export (or ever)
// ---------------------------------------------------------------------------

TEST(ImplA3_ExportGradient, EG02_ThrowsWithoutTraining) {
    IncrementalLoRATrainer trainer(makeConfig(), "");

    EXPECT_THROW(
        trainer.exportGradient(/*round=*/1),
        std::runtime_error);
}

// ---------------------------------------------------------------------------
// ILT-EG-03 — exportGradient() resets the accumulator; a second immediate
//             call throws std::runtime_error
// ---------------------------------------------------------------------------

TEST(ImplA3_ExportGradient, EG03_ResetsAccumulatorOnExport) {
    IncrementalLoRATrainer trainer(makeConfig(), "");
    runOneTrain(trainer);

    // First export succeeds
    EXPECT_NO_THROW(trainer.exportGradient(/*round=*/1));

    // Second export immediately after must throw because update_count == 0
    EXPECT_THROW(
        trainer.exportGradient(/*round=*/2),
        std::runtime_error);
}

// ---------------------------------------------------------------------------
// ILT-AG-01 — applyGlobalDelta() changes local weights measurably
// ---------------------------------------------------------------------------

TEST(ImplA3_ApplyGlobalDelta, AG01_ChangesLocalWeightsMeasurably) {
    IncrementalLoRATrainer trainer(makeConfig(), "");
    trainer.setFederatedLearningRate(0.1);

    // Train first so known_layers_ is populated (required for applyGlobalDelta)
    auto res = runOneTrain(trainer);
    EXPECT_TRUE(res.success);

    // Verify initial state (before applying delta)
    EXPECT_DOUBLE_EQ(trainer.getLocalWeight("lora_A_layer_0"), 0.0);

    // Build a delta with a known value for a known layer
    GlobalAdapterDelta delta;
    delta.round        = 1;
    delta.version      = "global-v1";
    delta.participants = 2;
    delta.algorithm    = "FedAvg";
    delta.epsilon_spent = 0.05;
    delta.delta["lora_A_layer_0"] = 1.0;
    delta.delta["lora_B_layer_0"] = 2.0;

    trainer.applyGlobalDelta(delta);

    // Expected: 0.0 + 0.1 * 1.0 = 0.1
    EXPECT_NEAR(trainer.getLocalWeight("lora_A_layer_0"), 0.1, 1e-9);
    // Expected: 0.0 + 0.1 * 2.0 = 0.2
    EXPECT_NEAR(trainer.getLocalWeight("lora_B_layer_0"), 0.2, 1e-9);
}

// ---------------------------------------------------------------------------
// ILT-AG-02 — applyGlobalDelta() with unknown layer names does not throw
// ---------------------------------------------------------------------------

TEST(ImplA3_ApplyGlobalDelta, AG02_UnknownLayerNamesAreIgnored) {
    IncrementalLoRATrainer trainer(makeConfig(), "");
    // No training → known_layers_ is empty → all delta layers are "unknown"

    GlobalAdapterDelta delta;
    delta.round        = 1;
    delta.version      = "global-v1";
    delta.participants = 1;
    delta.algorithm    = "FedAvg";
    delta.epsilon_spent = 0.01;
    // Layers that do not exist in the local weight map (no training done)
    delta.delta["unknown_layer_xyz"] = 99.0;
    delta.delta["non_existent_head"] = -7.5;

    // Must not throw
    EXPECT_NO_THROW(trainer.applyGlobalDelta(delta));

    // Because no training was done, known_layers_ is empty, so the unknown
    // layers should have been ignored — local_weights_ is empty, getLocalWeight
    // returns 0.0 for any name.
    EXPECT_DOUBLE_EQ(trainer.getLocalWeight("unknown_layer_xyz"), 0.0)
        << "Unknown layers (not seen during training) must be ignored";
}

// ============================================================================
// Router mutex / data-race hardening (#5414)
// ============================================================================

// A null router must not be called; deployVersionEx without an attached router
// should still succeed via local registry update.
TEST(RouterMutex, DeployVersionEx_NoVersion_NullRouter_FailsClean) {
    IncrementalTrainingConfig cfg;
    cfg.adapter_version = "v1";
    IncrementalLoRATrainer trainer(cfg, "");
    // No setLLMRouter() call — router is null.
    auto r = trainer.deployVersionEx("nonexistent", 0.5f);
    EXPECT_TRUE(r.success);
    EXPECT_TRUE(r.error.empty());
}

// setLLMRouter(nullptr) detaches the router; subsequent deploy/rollback must
// not crash and should still succeed via local registry update.
TEST(RouterMutex, SetNullRouter_DeployVersionEx_DoesNotCrash) {
    IncrementalTrainingConfig cfg;
    cfg.adapter_version = "v1";
    IncrementalLoRATrainer trainer(cfg, "");
    trainer.setLLMRouter(nullptr);
    // Should not crash with a null router.
    auto r = trainer.deployVersionEx("nonexistent", 0.5f);
    EXPECT_TRUE(r.success);
}

TEST(DeploymentDeterminism, DeployVersion_NearFullSplitTreatsAsFullDeployment) {
    IncrementalLoRATrainer trainer(makeConfig(), "");

    ASSERT_TRUE(trainer.deployVersion("v1", 0.40f));
    ASSERT_TRUE(trainer.deployVersion("v2", 0.9999996f));

    for (int i = 0; i < 50; ++i) {
        EXPECT_EQ(trainer.selectAdapterForRequest(), "v2");
    }
}

TEST(RouterMutex, DeployVersionEx_RouterRejectsUpdate_RevertsRegistryState) {
    IncrementalLoRATrainer trainer(makeConfig(), "");
    ASSERT_TRUE(trainer.deployVersion("stable", 1.0f));

    TestRouter router;
    router.accept_weight = false;
    trainer.setLLMRouter(&router);

    auto result = trainer.deployVersionEx("candidate", 0.25f);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, "router_update_failed");
    EXPECT_EQ(router.last_version, "candidate");
    EXPECT_FLOAT_EQ(router.last_weight, 0.25f);

    const auto versions = trainer.listVersions();
    ASSERT_EQ(versions.size(), 1u);
    EXPECT_EQ(versions.front(), "stable");
    for (int i = 0; i < 20; ++i) {
        EXPECT_EQ(trainer.selectAdapterForRequest(), "stable");
    }
}

TEST(RouterMutex, RollbackVersionEx_RouterRejectsUpdate_RevertsRegistryState) {
    IncrementalLoRATrainer trainer(makeConfig(), "");
    ASSERT_TRUE(trainer.deployVersion("stable", 1.0f));
    ASSERT_TRUE(trainer.deployVersion("candidate", 0.0f));

    TestRouter router;
    router.accept_weight = false;
    trainer.setLLMRouter(&router);

    auto result = trainer.rollbackVersionEx("candidate");

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, "router_update_failed");
    EXPECT_EQ(router.last_version, "candidate");
    EXPECT_FLOAT_EQ(router.last_weight, 1.0f);

    const auto versions = trainer.listVersions();
    ASSERT_EQ(versions.size(), 2u);
    EXPECT_EQ(versions[0], "candidate");
    EXPECT_EQ(versions[1], "stable");
    for (int i = 0; i < 20; ++i) {
        EXPECT_EQ(trainer.selectAdapterForRequest(), "stable");
    }
}

TEST(RouterMutex, DeployVersionEx_RouterUnavailable_RevertsRegistryState) {
    IncrementalLoRATrainer trainer(makeConfig(), "");
    ASSERT_TRUE(trainer.deployVersion("stable", 1.0f));

    TestRouter router;
    router.available = false;
    trainer.setLLMRouter(&router);

    auto result = trainer.deployVersionEx("candidate", 0.25f);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, "router_unavailable");
    EXPECT_TRUE(router.last_version.empty());
    EXPECT_LT(router.last_weight, 0.0f);

    const auto versions = trainer.listVersions();
    ASSERT_EQ(versions.size(), 1u);
    EXPECT_EQ(versions.front(), "stable");
    for (int i = 0; i < 20; ++i) {
        EXPECT_EQ(trainer.selectAdapterForRequest(), "stable");
    }
}

TEST(RouterMutex, RollbackVersionEx_RouterUnavailable_RevertsRegistryState) {
    IncrementalLoRATrainer trainer(makeConfig(), "");
    ASSERT_TRUE(trainer.deployVersion("stable", 1.0f));
    ASSERT_TRUE(trainer.deployVersion("candidate", 0.0f));

    TestRouter router;
    router.available = false;
    trainer.setLLMRouter(&router);

    auto result = trainer.rollbackVersionEx("candidate");

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, "router_unavailable");
    EXPECT_TRUE(router.last_version.empty());
    EXPECT_LT(router.last_weight, 0.0f);

    const auto versions = trainer.listVersions();
    ASSERT_EQ(versions.size(), 2u);
    EXPECT_EQ(versions[0], "candidate");
    EXPECT_EQ(versions[1], "stable");
    for (int i = 0; i < 20; ++i) {
        EXPECT_EQ(trainer.selectAdapterForRequest(), "stable");
    }
}

TEST(RouterMutex, DeployVersionEx_RouterThrowsOnSetWeight_RevertsRegistryState) {
    IncrementalLoRATrainer trainer(makeConfig(), "");
    ASSERT_TRUE(trainer.deployVersion("stable", 1.0f));

    ThrowingRouter router;
    router.throw_on_set_weight = true;
    trainer.setLLMRouter(&router);

    auto result = trainer.deployVersionEx("candidate", 0.25f);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, "router_update_failed");
    EXPECT_EQ(router.set_weight_calls, 1);
    EXPECT_EQ(router.last_version, "candidate");

    const auto versions = trainer.listVersions();
    ASSERT_EQ(versions.size(), 1u);
    EXPECT_EQ(versions.front(), "stable");
    for (int i = 0; i < 20; ++i) {
        EXPECT_EQ(trainer.selectAdapterForRequest(), "stable");
    }
}

TEST(RouterMutex, RollbackVersionEx_RouterThrowsOnAvailability_RevertsRegistryState) {
    IncrementalLoRATrainer trainer(makeConfig(), "");
    ASSERT_TRUE(trainer.deployVersion("stable", 1.0f));
    ASSERT_TRUE(trainer.deployVersion("candidate", 0.0f));

    ThrowingRouter router;
    router.throw_on_is_available = true;
    trainer.setLLMRouter(&router);

    auto result = trainer.rollbackVersionEx("candidate");

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, "router_update_failed");
    EXPECT_EQ(router.is_available_calls, 1);
    EXPECT_EQ(router.set_weight_calls, 0);

    const auto versions = trainer.listVersions();
    ASSERT_EQ(versions.size(), 2u);
    EXPECT_EQ(versions[0], "candidate");
    EXPECT_EQ(versions[1], "stable");
    for (int i = 0; i < 20; ++i) {
        EXPECT_EQ(trainer.selectAdapterForRequest(), "stable");
    }
}

TEST(RouterMutex, ConcurrentRouterDetachAndDeployRollback_DoesNotThrowOrCorruptRegistry) {
    IncrementalLoRATrainer trainer(makeConfig(), "");
    ASSERT_TRUE(trainer.deployVersion("stable", 1.0f));
    ASSERT_TRUE(trainer.deployVersion("candidate", 0.0f));

    TestRouter router;
    std::atomic<bool> had_exception{false};

    auto detach_attach = std::thread([&]() {
        try {
            for (int i = 0; i < 2000; ++i) {
                trainer.setLLMRouter((i % 2 == 0) ? &router : nullptr);
            }
        } catch (...) {
            had_exception.store(true, std::memory_order_relaxed);
        }
    });

    auto deploy_rollback = std::thread([&]() {
        try {
            for (int i = 0; i < 1000; ++i) {
                (void)trainer.deployVersionEx("candidate", 0.3f);
                (void)trainer.rollbackVersionEx("stable");
                (void)trainer.selectAdapterForRequest();
                (void)trainer.listVersions();
            }
        } catch (...) {
            had_exception.store(true, std::memory_order_relaxed);
        }
    });

    detach_attach.join();
    deploy_rollback.join();

    trainer.setLLMRouter(&router);
    const auto final_result = trainer.rollbackVersionEx("stable");
    EXPECT_TRUE(final_result.success);
    EXPECT_FALSE(had_exception.load(std::memory_order_relaxed));

    const auto versions = trainer.listVersions();
    ASSERT_EQ(versions.size(), 2u);
    EXPECT_EQ(versions[0], "candidate");
    EXPECT_EQ(versions[1], "stable");
    for (int i = 0; i < 20; ++i) {
        EXPECT_EQ(trainer.selectAdapterForRequest(), "stable");
    }
}

TEST(CheckpointManagerMutex, ConcurrentDeployRollbackAndResume_NoThrows_NoManagerRaces) {
    const fs::path checkpoint_dir = makeUniqueTempDir("themis_checkpoint_manager_mutex");
    const std::string checkpoint_prefix = (checkpoint_dir / "resume_ckpt").string();
    writeCheckpointMetadata(checkpoint_prefix, "resume_v1", 1, 1);

    IncrementalTrainingConfig cfg = makeConfig();
    cfg.checkpoint_dir = checkpoint_dir.string();
    IncrementalLoRATrainer trainer(cfg, "");
    ASSERT_TRUE(trainer.deployVersion("stable", 1.0f));
    ASSERT_TRUE(trainer.deployVersion("candidate", 0.0f));

    std::atomic<bool> had_exception{false};
    std::atomic<int> unexpected_resume_outcomes{0};

    auto deploy_rollback = std::thread([&]() {
        try {
            for (int i = 0; i < 400; ++i) {
                (void)trainer.deployVersionEx("candidate", 0.25f);
                (void)trainer.rollbackVersionEx("stable");
            }
        } catch (...) {
            had_exception.store(true, std::memory_order_relaxed);
        }
    });

    auto resume_stress = std::thread([&]() {
        try {
            for (int i = 0; i < 400; ++i) {
                const auto result = trainer.resumeFromCheckpoint(checkpoint_prefix);
                if (result.success ||
                    result.error_message.find("no matching manifest entry") == std::string::npos) {
                    ++unexpected_resume_outcomes;
                }
            }
        } catch (...) {
            had_exception.store(true, std::memory_order_relaxed);
        }
    });

    deploy_rollback.join();
    resume_stress.join();

    EXPECT_FALSE(had_exception.load(std::memory_order_relaxed));
    EXPECT_EQ(unexpected_resume_outcomes.load(std::memory_order_relaxed), 0);
}

// ============================================================================
// resumeFromCheckpoint() failure-path hardening (#5414)
// ============================================================================

// RFC-01: Empty checkpoint path must fail gracefully without throwing.
TEST(ResumeFromCheckpoint, RFC01_EmptyPath_ReturnsFailureWithMessage) {
    IncrementalLoRATrainer trainer(makeConfig(), "");
    auto result = trainer.resumeFromCheckpoint("");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty())
        << "error_message must be set when checkpoint_path is empty";
    EXPECT_NE(result.error_message.find("checkpoint"), std::string::npos)
        << "error_message must mention 'checkpoint'";
}

// RFC-02: Non-existent checkpoint path must fail gracefully without throwing.
TEST(ResumeFromCheckpoint, RFC02_NonExistentPath_ReturnsFailureWithMessage) {
    IncrementalLoRATrainer trainer(makeConfig(), "");
    auto result = trainer.resumeFromCheckpoint("/nonexistent/path/ckpt.bin");
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty())
        << "error_message must describe the load failure";
}

// RFC-03: training_time_seconds is always populated (>= 0) even on failure.
TEST(ResumeFromCheckpoint, RFC03_FailedResume_ElapsedTimeRecorded) {
    IncrementalLoRATrainer trainer(makeConfig(), "");
    auto result = trainer.resumeFromCheckpoint("/no/such/checkpoint");
    EXPECT_GE(result.training_time_seconds, 0.0)
        << "training_time_seconds must be non-negative even after a failed resume";
}

TEST(ResumeFromCheckpoint, RFC09_PayloadChecksumMismatch_ReturnsIntegrityFailure) {
    const fs::path temp_dir = makeUniqueTempDir("themis_resume_weights_checksum_mismatch");

    IncrementalTrainingConfig cfg = makeConfig();
    cfg.checkpoint_dir = temp_dir.string();
    cfg.adapter_version = "resume_v6";
    IncrementalLoRATrainer trainer(cfg, "");

    const std::string version = "resume_v6";
    constexpr size_t epoch = 1;
    constexpr size_t step = 7;
    const std::string checkpoint_prefix =
        (temp_dir / (version + "_epoch1_step7")).string();
    writeCheckpointMetadata(checkpoint_prefix, version, epoch, step);

    const fs::path seed_weights = temp_dir / "seed_checksum_weights.bin";
    writeTwoMatrixWeights(seed_weights, {1.0f, 2.0f, 3.0f, 4.0f}, {5.0f, 6.0f, 7.0f, 8.0f});
    registerCheckpointManifest(temp_dir, version, epoch, step, seed_weights);

    const fs::path checkpoint_weights = checkpoint_prefix + "_weights.bin";
    fs::copy_file(seed_weights, checkpoint_weights, fs::copy_options::overwrite_existing);
    {
        std::fstream tamper(checkpoint_weights, std::ios::in | std::ios::out | std::ios::binary);
        ASSERT_TRUE(tamper.is_open());
        char first_byte = '\0';
        tamper.read(&first_byte, 1);
        ASSERT_EQ(tamper.gcount(), 1);
        tamper.seekp(0, std::ios::beg);
        first_byte = static_cast<char>(first_byte ^ 0x01);
        tamper.write(&first_byte, 1);
        ASSERT_TRUE(tamper.good());
    }

    const auto result = trainer.resumeFromCheckpoint(checkpoint_prefix);
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error_message.find("Checkpoint integrity verification failed"), std::string::npos);
    EXPECT_NE(result.error_message.find("SHA-256 mismatch"), std::string::npos);

    fs::remove_all(temp_dir);
}

TEST(ResumeFromCheckpoint, RFC10_MalformedMetadataNumericFields_ReturnsLoadFailure) {
    const fs::path temp_dir = makeUniqueTempDir("themis_resume_metadata_malformed");
    const std::string checkpoint_prefix = (temp_dir / "bad_metadata").string();

    {
        std::ofstream metadata(checkpoint_prefix + "_metadata.txt");
        ASSERT_TRUE(metadata.is_open());
        metadata << "version=resume_bad\n";
        metadata << "epoch=one\n";
        metadata << "step=2\n";
        metadata << "loss=0.1\n";
        metadata << "accuracy=0.2\n";
    }

    IncrementalLoRATrainer trainer(makeConfig(), "");
    const auto result = trainer.resumeFromCheckpoint(checkpoint_prefix);
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error_message.find("Failed to load checkpoint"), std::string::npos);
    EXPECT_NE(result.error_message.find("invalid epoch"), std::string::npos);

    fs::remove_all(temp_dir);
}

TEST(ResumeFromCheckpoint, RFC11_MissingManifestEntry_ReturnsIntegrityFailure) {
    const fs::path temp_dir = makeUniqueTempDir("themis_resume_missing_manifest_entry");

    IncrementalTrainingConfig cfg = makeConfig();
    cfg.checkpoint_dir = temp_dir.string();
    cfg.adapter_version = "resume_v7";
    IncrementalLoRATrainer trainer(cfg, "");

    const std::string version = "resume_v7";
    constexpr size_t epoch = 1;
    constexpr size_t step = 8;
    const std::string checkpoint_prefix =
        (temp_dir / (version + "_epoch1_step8")).string();
    writeCheckpointMetadata(checkpoint_prefix, version, epoch, step);

    const auto result = trainer.resumeFromCheckpoint(checkpoint_prefix);
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error_message.find("Checkpoint integrity verification failed"), std::string::npos);
    EXPECT_NE(result.error_message.find("no matching manifest entry"), std::string::npos);

    fs::remove_all(temp_dir);
}

TEST(ResumeFromCheckpoint, RFC12_EmptyManifestSha_ReturnsIntegrityFailure) {
    const fs::path temp_dir = makeUniqueTempDir("themis_resume_manifest_empty_sha");

    IncrementalTrainingConfig cfg = makeConfig();
    cfg.checkpoint_dir = temp_dir.string();
    cfg.adapter_version = "resume_v8";
    IncrementalLoRATrainer trainer(cfg, "");

    const std::string version = "resume_v8";
    constexpr size_t epoch = 1;
    constexpr size_t step = 9;
    const std::string checkpoint_prefix =
        (temp_dir / (version + "_epoch1_step9")).string();
    writeCheckpointMetadata(checkpoint_prefix, version, epoch, step);

    writeManifestEntry(temp_dir, checkpoint_prefix + "_weights.bin", "", version, epoch, step);

    const auto result = trainer.resumeFromCheckpoint(checkpoint_prefix);
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error_message.find("Checkpoint integrity verification failed"), std::string::npos);
    EXPECT_NE(result.error_message.find("empty SHA-256"), std::string::npos);

    fs::remove_all(temp_dir);
}

TEST(ResumeFromCheckpoint, RFC13_MissingPayloadFile_ReturnsHashFailure) {
    const fs::path temp_dir = makeUniqueTempDir("themis_resume_missing_payload");

    IncrementalTrainingConfig cfg = makeConfig();
    cfg.checkpoint_dir = temp_dir.string();
    cfg.adapter_version = "resume_v9";
    IncrementalLoRATrainer trainer(cfg, "");

    const std::string version = "resume_v9";
    constexpr size_t epoch = 1;
    constexpr size_t step = 10;
    const std::string checkpoint_prefix =
        (temp_dir / (version + "_epoch1_step10")).string();
    writeCheckpointMetadata(checkpoint_prefix, version, epoch, step);

    const fs::path seed_weights = temp_dir / "seed_missing_payload_weights.bin";
    writeTwoMatrixWeights(seed_weights, {1.0f, 2.0f, 3.0f, 4.0f}, {5.0f, 6.0f, 7.0f, 8.0f});
    registerCheckpointManifest(temp_dir, version, epoch, step, seed_weights);

    const auto result = trainer.resumeFromCheckpoint(checkpoint_prefix);
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error_message.find("Checkpoint integrity verification failed"), std::string::npos);
    EXPECT_NE(result.error_message.find("unable to hash checkpoint payload"), std::string::npos);

    fs::remove_all(temp_dir);
}

#ifdef THEMIS_ENABLE_LLM
TEST(ResumeFromCheckpoint, RFC04_MalformedWeightsPayload_ReturnsRestoreFailure) {
    const auto unique_suffix =
        std::to_string(static_cast<long long>(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    const fs::path temp_dir = fs::temp_directory_path() /
        ("themis_resume_weights_corrupt_" + unique_suffix);
    fs::create_directories(temp_dir);

    IncrementalTrainingConfig cfg = makeConfig();
    cfg.checkpoint_dir = temp_dir.string();
    cfg.adapter_version = "resume_v1";
    IncrementalLoRATrainer trainer(cfg, "");

    const std::string version = "resume_v1";
    constexpr size_t epoch = 1;
    constexpr size_t step = 2;
    const std::string checkpoint_prefix =
        (temp_dir / (version + "_epoch1_step2")).string();

    writeCheckpointMetadata(checkpoint_prefix, version, epoch, step);

    const fs::path seed_weights = temp_dir / "seed_weights.bin";
    {
        std::ofstream out(seed_weights, std::ios::binary);
        ASSERT_TRUE(out.is_open());
        out.write("BAD", 3);
    }

    registerCheckpointManifest(temp_dir, version, epoch, step, seed_weights);

    fs::copy_file(seed_weights,
                  checkpoint_prefix + "_weights.bin",
                  fs::copy_options::overwrite_existing);

    const auto result = trainer.resumeFromCheckpoint(checkpoint_prefix);
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error_message.find("Checkpoint weight restore failed"), std::string::npos);

    fs::remove_all(temp_dir);
}

TEST(ResumeFromCheckpoint, RFC05_ZeroDimensionWeightsPayload_ReturnsRestoreFailure) {
    const auto unique_suffix =
        std::to_string(static_cast<long long>(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    const fs::path temp_dir = fs::temp_directory_path() /
        ("themis_resume_weights_zero_dim_" + unique_suffix);
    fs::create_directories(temp_dir);

    IncrementalTrainingConfig cfg = makeConfig();
    cfg.checkpoint_dir = temp_dir.string();
    cfg.adapter_version = "resume_v2";
    IncrementalLoRATrainer trainer(cfg, "");

    const std::string version = "resume_v2";
    constexpr size_t epoch = 1;
    constexpr size_t step = 3;
    const std::string checkpoint_prefix =
        (temp_dir / (version + "_epoch1_step3")).string();
    writeCheckpointMetadata(checkpoint_prefix, version, epoch, step);

    const fs::path seed_weights = temp_dir / "seed_zero_dim_weights.bin";
    {
        std::ofstream out(seed_weights, std::ios::binary);
        ASSERT_TRUE(out.is_open());
        const uint32_t rows = 0;
        const uint32_t cols = 4;
        out.write(reinterpret_cast<const char*>(&rows), sizeof(rows));
        out.write(reinterpret_cast<const char*>(&cols), sizeof(cols));
    }
    registerCheckpointManifest(temp_dir, version, epoch, step, seed_weights);

    fs::copy_file(seed_weights,
                  checkpoint_prefix + "_weights.bin",
                  fs::copy_options::overwrite_existing);

    const auto result = trainer.resumeFromCheckpoint(checkpoint_prefix);
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error_message.find("Checkpoint weight restore failed"), std::string::npos);
    EXPECT_NE(result.error_message.find("invalid matrix shape"), std::string::npos);

    fs::remove_all(temp_dir);
}

TEST(ResumeFromCheckpoint, RFC06_TrailingBytesWeightsPayload_ReturnsRestoreFailure) {
    const fs::path temp_dir = makeUniqueTempDir("themis_resume_weights_trailing");

    IncrementalTrainingConfig cfg = makeConfig();
    cfg.checkpoint_dir = temp_dir.string();
    cfg.adapter_version = "resume_v3";
    IncrementalLoRATrainer trainer(cfg, "");

    const std::string version = "resume_v3";
    constexpr size_t epoch = 1;
    constexpr size_t step = 4;
    const std::string checkpoint_prefix =
        (temp_dir / (version + "_epoch1_step4")).string();
    writeCheckpointMetadata(checkpoint_prefix, version, epoch, step);

    const fs::path seed_weights = temp_dir / "seed_trailing_weights.bin";
    {
        std::ofstream out(seed_weights, std::ios::binary);
        ASSERT_TRUE(out.is_open());

        const auto writeMatrix = [&out](uint32_t rows, uint32_t cols,
                                        const std::array<float, 4>& values) {
            out.write(reinterpret_cast<const char*>(&rows), sizeof(rows));
            out.write(reinterpret_cast<const char*>(&cols), sizeof(cols));
            out.write(reinterpret_cast<const char*>(values.data()),
                      static_cast<std::streamsize>(values.size() * sizeof(float)));
        };

        writeMatrix(2, 2, {1.0f, 2.0f, 3.0f, 4.0f});
        writeMatrix(2, 2, {5.0f, 6.0f, 7.0f, 8.0f});
        const char trailing = 'X';
        out.write(&trailing, 1);
    }
    registerCheckpointManifest(temp_dir, version, epoch, step, seed_weights);

    fs::copy_file(seed_weights,
                  checkpoint_prefix + "_weights.bin",
                  fs::copy_options::overwrite_existing);

    const auto result = trainer.resumeFromCheckpoint(checkpoint_prefix);
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error_message.find("Checkpoint weight restore failed"), std::string::npos);
    EXPECT_NE(result.error_message.find("unexpected trailing bytes"), std::string::npos);

    fs::remove_all(temp_dir);
}

TEST(ResumeFromCheckpoint, RFC07_OversizedMatrixPayload_ReturnsRestoreFailure) {
    const fs::path temp_dir = makeUniqueTempDir("themis_resume_weights_oversized");

    IncrementalTrainingConfig cfg = makeConfig();
    cfg.checkpoint_dir = temp_dir.string();
    cfg.adapter_version = "resume_v4";
    IncrementalLoRATrainer trainer(cfg, "");

    const std::string version = "resume_v4";
    constexpr size_t epoch = 1;
    constexpr size_t step = 5;
    const std::string checkpoint_prefix =
        (temp_dir / (version + "_epoch1_step5")).string();
    writeCheckpointMetadata(checkpoint_prefix, version, epoch, step);

    const fs::path seed_weights = temp_dir / "seed_oversized_weights.bin";
    {
        std::ofstream out(seed_weights, std::ios::binary);
        ASSERT_TRUE(out.is_open());
        const uint32_t rows = 65536;
        const uint32_t cols = 65537;
        out.write(reinterpret_cast<const char*>(&rows), sizeof(rows));
        out.write(reinterpret_cast<const char*>(&cols), sizeof(cols));
    }
    registerCheckpointManifest(temp_dir, version, epoch, step, seed_weights);

    fs::copy_file(seed_weights,
                  checkpoint_prefix + "_weights.bin",
                  fs::copy_options::overwrite_existing);

    const auto result = trainer.resumeFromCheckpoint(checkpoint_prefix);
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error_message.find("Checkpoint weight restore failed"), std::string::npos);
    EXPECT_NE(result.error_message.find("matrix too large"), std::string::npos);

    fs::remove_all(temp_dir);
}

TEST(ResumeFromCheckpoint, RFC08_TruncatedSecondMatrixData_ReturnsRestoreFailure) {
    const fs::path temp_dir = makeUniqueTempDir("themis_resume_weights_truncated_second");

    IncrementalTrainingConfig cfg = makeConfig();
    cfg.checkpoint_dir = temp_dir.string();
    cfg.adapter_version = "resume_v5";
    IncrementalLoRATrainer trainer(cfg, "");

    const std::string version = "resume_v5";
    constexpr size_t epoch = 1;
    constexpr size_t step = 6;
    const std::string checkpoint_prefix =
        (temp_dir / (version + "_epoch1_step6")).string();
    writeCheckpointMetadata(checkpoint_prefix, version, epoch, step);

    const fs::path seed_weights = temp_dir / "seed_truncated_second_weights.bin";
    {
        std::ofstream out(seed_weights, std::ios::binary);
        ASSERT_TRUE(out.is_open());

        const auto writeMatrix = [&out](uint32_t rows, uint32_t cols,
                                        const std::array<float, 4>& values) {
            out.write(reinterpret_cast<const char*>(&rows), sizeof(rows));
            out.write(reinterpret_cast<const char*>(&cols), sizeof(cols));
            out.write(reinterpret_cast<const char*>(values.data()),
                      static_cast<std::streamsize>(values.size() * sizeof(float)));
        };

        writeMatrix(2, 2, {1.0f, 2.0f, 3.0f, 4.0f});

        const uint32_t rows = 2;
        const uint32_t cols = 2;
        out.write(reinterpret_cast<const char*>(&rows), sizeof(rows));
        out.write(reinterpret_cast<const char*>(&cols), sizeof(cols));
        const float partial = 5.0f;
        out.write(reinterpret_cast<const char*>(&partial), sizeof(partial));
    }
    registerCheckpointManifest(temp_dir, version, epoch, step, seed_weights);

    fs::copy_file(seed_weights,
                  checkpoint_prefix + "_weights.bin",
                  fs::copy_options::overwrite_existing);

    const auto result = trainer.resumeFromCheckpoint(checkpoint_prefix);
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error_message.find("Checkpoint weight restore failed"), std::string::npos);
    EXPECT_NE(result.error_message.find("truncated checkpoint payload"), std::string::npos);

    fs::remove_all(temp_dir);
}
#endif

// ============================================================================
// Batch-14: concurrent full-lifecycle integrity stress (#5414 batch 14)
//
// Exercises all three checkpoint_manager_mutex_ protected paths simultaneously:
//   Thread A – verifyAdapterIntegrity via deployVersionEx/rollbackVersionEx
//              (checkpoint_manager_->listCheckpoints + validate)
//   Thread B – verifyCheckpointPayloadIntegrity via resumeFromCheckpoint
//              (checkpoint_manager_->listCheckpoints + SHA-256 check)
//   Thread C – listVersions / selectAdapterForRequest (read-only registry ops)
//
// The manifest entry registered before the threads start refers to a payload
// whose SHA-256 was computed by LoRACheckpointManager::save(); the tampered
// weights file at <prefix>_weights.bin has a mismatched SHA, so
// verifyCheckpointPayloadIntegrity returns "SHA-256 mismatch" — a deterministic
// non-empty error message under any interleaving.
// ============================================================================
TEST(CheckpointManagerMutex, ConcurrentFullLifecycleIntegrity_AllPathsNoThrow) {
    const fs::path checkpoint_dir =
        makeUniqueTempDir("themis_ckpt_full_lifecycle");

    // Build a minimal but structurally valid weights payload so the manifest
    // SHA-256 is computed from real bytes.
    const fs::path seed_weights = checkpoint_dir / "seed_fl_weights.bin";
    writeTwoMatrixWeights(seed_weights,
                          {1.0f, 2.0f, 3.0f, 4.0f},
                          {5.0f, 6.0f, 7.0f, 8.0f});

    const std::string version = "fl_v1";
    constexpr size_t epoch = 1;
    constexpr size_t step  = 1;
    registerCheckpointManifest(checkpoint_dir, version, epoch, step, seed_weights);

    // Place a tampered copy at the checkpoint prefix path so that
    // verifyCheckpointPayloadIntegrity finds a manifest entry but fails with
    // "SHA-256 mismatch" — a stable, expected failure, not a crash.
    const std::string checkpoint_prefix =
        (checkpoint_dir / (version + "_epoch1_step1")).string();
    writeCheckpointMetadata(checkpoint_prefix, version, epoch, step);
    const fs::path checkpoint_weights = checkpoint_prefix + "_weights.bin";
    fs::copy_file(seed_weights, checkpoint_weights,
                  fs::copy_options::overwrite_existing);
    {
        std::fstream tamper(checkpoint_weights,
                            std::ios::in | std::ios::out | std::ios::binary);
        ASSERT_TRUE(tamper.is_open());
        char b = '\0';
        tamper.read(&b, 1);
        ASSERT_EQ(tamper.gcount(), 1);
        tamper.seekp(0, std::ios::beg);
        b = static_cast<char>(b ^ 0x01);
        tamper.write(&b, 1);
        ASSERT_TRUE(tamper.good());
    }

    IncrementalTrainingConfig cfg = makeConfig();
    cfg.checkpoint_dir = checkpoint_dir.string();
    IncrementalLoRATrainer trainer(cfg, "");
    ASSERT_TRUE(trainer.deployVersion("stable", 1.0f));
    ASSERT_TRUE(trainer.deployVersion("candidate", 0.0f));

    std::atomic<bool> had_exception{false};
    // Expected outcomes per thread:
    //   Thread A: deployVersionEx returns success or integrity_failure
    //             (version "fl_v1" is not in version_registry_ so it may
    //              be "version_not_found" after verifyAdapterIntegrity pass-through)
    //   Thread B: resumeFromCheckpoint returns failure with SHA-256 mismatch
    //   Thread C: no exception; listVersions/selectAdapterForRequest stable

    auto thread_a = std::thread([&]() {
        try {
            for (int i = 0; i < 300; ++i) {
                (void)trainer.deployVersionEx("candidate", 0.2f);
                (void)trainer.rollbackVersionEx("stable");
            }
        } catch (...) {
            had_exception.store(true, std::memory_order_relaxed);
        }
    });

    auto thread_b = std::thread([&]() {
        try {
            for (int i = 0; i < 300; ++i) {
                const auto r = trainer.resumeFromCheckpoint(checkpoint_prefix);
                // Must always fail (SHA-256 mismatch) — never succeed silently.
                if (r.success) {
                    had_exception.store(true, std::memory_order_relaxed);
                }
            }
        } catch (...) {
            had_exception.store(true, std::memory_order_relaxed);
        }
    });

    auto thread_c = std::thread([&]() {
        try {
            for (int i = 0; i < 300; ++i) {
                (void)trainer.listVersions();
                (void)trainer.selectAdapterForRequest();
            }
        } catch (...) {
            had_exception.store(true, std::memory_order_relaxed);
        }
    });

    thread_a.join();
    thread_b.join();
    thread_c.join();

    EXPECT_FALSE(had_exception.load(std::memory_order_relaxed));

    fs::remove_all(checkpoint_dir);
}
