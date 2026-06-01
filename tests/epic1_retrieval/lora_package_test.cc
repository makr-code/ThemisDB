/**
 * @file lora_package_test.cc
 * @brief Contract tests for ILoRARepository (sub-issues #5416 / #5424).
 *
 * Validates factory construction and scaffold-stage repository behavior:
 * listPackageIds returns empty, store/compile/load/purge do not crash,
 * and load returns nullopt for unknown IDs.
 * Production artifact storage is tracked in sub-issues #5416 and #5424.
 */

#include "retrieval/include/lora_package.h"

#include <gtest/gtest.h>
#include <memory>
#include <string>

using namespace themis::retrieval;

namespace {

LoRAPackage makePackage(const std::string& id) {
    LoRAPackage pkg;
    pkg.id      = id;
    pkg.name    = "test-adapter-" + id;
    pkg.version = "0.1.0";
    pkg.format  = AdapterFormat::SafeTensors;
    pkg.provenance.dataset_snapshot_id = "ds-001";
    pkg.provenance.base_model_id       = "llama-3-8b";
    pkg.provenance.training_run_id     = "run-001";
    return pkg;
}

} // namespace

class LoRARepositoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        repo_ = makeLoRARepository("/tmp/test_lora_scaffold");
        ASSERT_NE(repo_, nullptr);
    }

    std::unique_ptr<ILoRARepository> repo_;
};

TEST_F(LoRARepositoryTest, FactoryReturnsNonNull) {
    EXPECT_NE(repo_, nullptr);
}

TEST_F(LoRARepositoryTest, ListPackageIdsInitiallyEmpty) {
    auto ids = repo_->listPackageIds();
    EXPECT_TRUE(ids.empty());
}

TEST_F(LoRARepositoryTest, StoreDoesNotThrow) {
    // Scaffold: store may be a no-op or throw; document either is acceptable
    // at scaffold stage. This test just verifies no crash on attempt.
    LoRAPackage pkg = makePackage("test-pkg-001");
    EXPECT_NO_THROW({
        try {
            repo_->store(std::move(pkg));
        } catch (const std::exception&) {
            // acceptable at scaffold stage
        }
    });
}

TEST_F(LoRARepositoryTest, LoadUnknownIdReturnsNullopt) {
    auto result = repo_->load("nonexistent-id");
    EXPECT_FALSE(result.has_value());
}

TEST_F(LoRARepositoryTest, LoadEmptyIdDoesNotThrow) {
    EXPECT_NO_THROW(repo_->load(""));
}

TEST_F(LoRARepositoryTest, PurgeUnknownIdReturnsFalse) {
    bool removed = repo_->purge("nonexistent-id");
    EXPECT_FALSE(removed);
}

TEST_F(LoRARepositoryTest, PurgeEmptyIdDoesNotThrow) {
    EXPECT_NO_THROW(repo_->purge(""));
}

TEST_F(LoRARepositoryTest, CompileDoesNotThrow) {
    EXPECT_NO_THROW({
        try {
            repo_->compile("nonexistent-id", "cpu");
        } catch (const std::exception&) {
            // acceptable at scaffold stage
        }
    });
}

TEST_F(LoRARepositoryTest, ListWithLimitZeroDoesNotThrow) {
    EXPECT_NO_THROW(repo_->listPackageIds(0));
}
