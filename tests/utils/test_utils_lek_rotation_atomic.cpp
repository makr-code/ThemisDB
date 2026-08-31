/**
 * @file test_utils_lek_rotation_atomic.cpp
 * @brief Focused LEK manager tests for current rotation and revocation behavior.
 */

#include <gtest/gtest.h>

#include "security/mock_key_provider.h"
#include "storage/rocksdb_wrapper.h"
#include "utils/lek_manager.h"
#include "utils/pki_client.h"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

namespace themis {
namespace utils {

class LEKRotationTest : public ::testing::Test {
protected:
    static std::shared_ptr<RocksDBWrapper> openDb(const fs::path& path) {
        if (fs::exists(path)) {
            fs::remove_all(path);
        }

        RocksDBWrapper::Config config;
        config.db_path = path.string();
        config.memtable_size_mb = 64;
        config.block_cache_size_mb = 64;

        auto db = std::make_shared<RocksDBWrapper>(config);
        if (!db->open()) {
            throw std::runtime_error("Failed to open test RocksDB at " + path.string());
        }
        return db;
    }

    void SetUp() override {
        temp_dir_ = fs::temp_directory_path() / "themis_lek_rotation_atomic_test";
        db_ = openDb(temp_dir_);
        key_provider_ = std::make_shared<themis::MockKeyProvider>();

        PKIConfig pki_config;
        pki_config.service_id = "lek-rotation-test";
        pki_ = std::make_shared<VCCPKIClient>(pki_config);

        manager_ = std::make_unique<LEKManager>(db_, pki_, key_provider_);
    }

    void TearDown() override {
        manager_.reset();
        db_.reset();
        if (fs::exists(temp_dir_)) {
            fs::remove_all(temp_dir_);
        }
    }

    fs::path temp_dir_;
    std::shared_ptr<RocksDBWrapper> db_;
    std::shared_ptr<VCCPKIClient> pki_;
    std::shared_ptr<themis::MockKeyProvider> key_provider_;
    std::unique_ptr<LEKManager> manager_;
};

TEST_F(LEKRotationTest, CurrentLEKIsStableAcrossRepeatedCalls) {
    const auto first = manager_->getCurrentLEK();
    const auto second = manager_->getCurrentLEK();

    EXPECT_FALSE(first.empty());
    EXPECT_EQ(first, second);
}

TEST_F(LEKRotationTest, RotateKeepsCurrentKeyAccessible) {
    const auto before = manager_->getCurrentLEK();
    ASSERT_FALSE(before.empty());

    EXPECT_NO_THROW(manager_->rotate());

    const auto after = manager_->getCurrentLEK();
    EXPECT_FALSE(after.empty());
    EXPECT_EQ(after, manager_->getLEKForDate(LEKManager::getCurrentDateString()));
}

TEST_F(LEKRotationTest, RevokeAndCheckKeyState) {
    const auto today = LEKManager::getCurrentDateString();

    EXPECT_FALSE(manager_->isRevoked(today));
    EXPECT_TRUE(manager_->revokeKey(today));
    EXPECT_TRUE(manager_->isRevoked(today));
    EXPECT_FALSE(manager_->getRevokedKeys().empty());
}

TEST_F(LEKRotationTest, ConcurrentAccessDuringRotationDoesNotLoseKey) {
    ASSERT_FALSE(manager_->getCurrentLEK().empty());

    std::atomic<int> successful_reads{0};
    std::atomic<bool> error_detected{false};
    std::vector<std::thread> threads;

    threads.emplace_back([this, &error_detected]() {
        for (int i = 0; i < 10; ++i) {
            try {
                manager_->rotate();
            } catch (...) {
                error_detected = true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });

    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([this, &successful_reads, &error_detected]() {
            for (int j = 0; j < 50; ++j) {
                try {
                    auto key_id = manager_->getCurrentLEK();
                    if (key_id.empty()) {
                        error_detected = true;
                    } else {
                        ++successful_reads;
                    }
                } catch (...) {
                    error_detected = true;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    EXPECT_FALSE(error_detected);
    EXPECT_GT(successful_reads.load(), 0);
}

} // namespace utils
} // namespace themis
