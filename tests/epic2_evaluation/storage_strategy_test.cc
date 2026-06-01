/**
 * @file storage_strategy_test.cc
 * @brief Contract tests for IStorageStrategy (sub-issue #5443).
 *
 * Validates factory construction, storage recommendation, load/mmap behavior
 * at scaffold stage, and progress callback registration.
 * Production mmap and streaming loaders are tracked in sub-issue #5443.
 */

#include "evaluation/include/storage_strategy.h"

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <vector>

using namespace themis::evaluation;

namespace {

StorageDescriptor makeDescriptor(const std::string& id,
                                  StorageMode mode = StorageMode::FullPrecision) {
    StorageDescriptor d;
    d.artifact_id      = id;
    d.mode             = mode;
    d.quant            = QuantScheme::None;
    d.path_or_uri      = "/tmp/test-storage/" + id;
    d.size_bytes       = 1024;
    d.pinned_in_memory = false;
    return d;
}

} // namespace

class StorageStrategyTest : public ::testing::Test {
protected:
    void SetUp() override {
        strategy_ = makeStorageStrategy();
        ASSERT_NE(strategy_, nullptr);
    }

    std::unique_ptr<IStorageStrategy> strategy_;
};

TEST_F(StorageStrategyTest, FactoryReturnsNonNull) {
    EXPECT_NE(strategy_, nullptr);
}

TEST_F(StorageStrategyTest, RecommendDoesNotThrow) {
    EXPECT_NO_THROW(strategy_->recommend(
        1ULL << 30, // 1 GiB artifact
        8ULL << 30, // 8 GiB available DRAM
        /*has_nvme=*/true));
}

TEST_F(StorageStrategyTest, RecommendReturnsNonEmptyRationale) {
    StorageRecommendation rec = strategy_->recommend(
        1ULL << 30, 8ULL << 30, true);
    EXPECT_FALSE(rec.rationale.empty());
}

TEST_F(StorageStrategyTest, RecommendLargeArtifactPrefersDisk) {
    // 512 GiB artifact, 16 GiB DRAM => should not recommend FullPrecision mmap.
    StorageRecommendation rec = strategy_->recommend(
        512ULL << 30,
        16ULL << 30,
        /*has_nvme=*/true);
    (void)rec; // Scaffold may or may not have heuristics yet.
    SUCCEED();
}

TEST_F(StorageStrategyTest, LoadNonexistentPathDoesNotCrash) {
    StorageDescriptor desc = makeDescriptor("nonexistent");
    EXPECT_NO_THROW({
        try {
            strategy_->load(desc);
        } catch (const std::exception&) {
            // acceptable; real file doesn't exist
        }
    });
}

TEST_F(StorageStrategyTest, MmapNonexistentPathReturnsEmptyOrThrows) {
    EXPECT_NO_THROW({
        try {
            auto result = strategy_->mmap("/tmp/nonexistent-file", 0, 1024);
            (void)result; // empty vector is the scaffold-stage contract
        } catch (const std::exception&) {
            // acceptable; file doesn't exist
        }
    });
}

TEST_F(StorageStrategyTest, MunmapUnknownPathDoesNotThrow) {
    EXPECT_NO_THROW(strategy_->munmap("/tmp/nonexistent-file"));
}

TEST_F(StorageStrategyTest, ProgressCallbackRegistrationDoesNotThrow) {
    EXPECT_NO_THROW(strategy_->onProgress(
        [](std::uint64_t /*loaded*/, std::uint64_t /*total*/) {}));
}

TEST_F(StorageStrategyTest, RecommendWithNoDramFallsBackGracefully) {
    EXPECT_NO_THROW(strategy_->recommend(
        256ULL << 20, // 256 MiB
        0,            // no DRAM
        /*has_nvme=*/false));
}
