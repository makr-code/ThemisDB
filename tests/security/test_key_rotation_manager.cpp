/**
 * @file test_key_rotation_manager.cpp
 * @brief Tests for KeyRotationManager
 *
 * Test cases: KEYROT-01..06
 *
 * @date 2026-09-24
 */

#include <gtest/gtest.h>
#include <filesystem>

#include "security/key_rotation_manager.h"

using namespace themis::security;

class KeyRotationManagerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    temp_dir_ = std::filesystem::temp_directory_path() / "test_keyrot_XXXXXX";
    std::filesystem::create_directories(temp_dir_);
  }

  void TearDown() override {
    if (std::filesystem::exists(temp_dir_)) {
      std::filesystem::remove_all(temp_dir_);
    }
  }

  std::filesystem::path temp_dir_;
  std::string master_key_{"my_master_key_123456"};

  std::unique_ptr<KeyRotationManager> CreateManager() {
    return KeyRotationManager::Open(temp_dir_.string());
  }
};

/**
 * @test KEYROT-01: Start key rotation and check state
 */
TEST_F(KeyRotationManagerTest, KEYROT_01_StartRotation) {
  auto manager = CreateManager();

  uint32_t rotation_id = manager->StartKeyRotation(master_key_);
  EXPECT_EQ(rotation_id, 1);

  auto state = manager->GetRotationState();
  EXPECT_EQ(state.phase, RotationPhase::InProgress);
  EXPECT_EQ(state.rotation_id, 1);
  EXPECT_FALSE(state.new_key_tag.empty());
  EXPECT_GT(state.validation_deadline.time_since_epoch().count(),
            state.started_at.time_since_epoch().count());
}

/**
 * @test KEYROT-02: Migrate chunk with new key
 */
TEST_F(KeyRotationManagerTest, KEYROT_02_MigrateChunk) {
  auto manager = CreateManager();

  uint32_t rotation_id = manager->StartKeyRotation(master_key_);
  EXPECT_EQ(rotation_id, 1);

  bool success = manager->MigrateChunk(100, master_key_);
  EXPECT_TRUE(success);

  auto mapping = manager->GetChunkMapping(100);
  EXPECT_TRUE(mapping.has_value());
  EXPECT_EQ(mapping->chunk_id, 100);
  EXPECT_FALSE(mapping->old_key_tag.empty());
  EXPECT_FALSE(mapping->new_key_tag.empty());
  EXPECT_FALSE(mapping->validated);
}

/**
 * @test KEYROT-03: Multiple chunks migration
 */
TEST_F(KeyRotationManagerTest, KEYROT_03_MultipleChunks) {
  auto manager = CreateManager();

  manager->StartKeyRotation(master_key_);

  for (uint64_t chunk_id = 1; chunk_id <= 5; ++chunk_id) {
    EXPECT_TRUE(manager->MigrateChunk(chunk_id, master_key_));
  }

  auto state = manager->GetRotationState();
  EXPECT_EQ(state.chunks_migrated, 5);
}

/**
 * @test KEYROT-04: Validate chunk during rotation
 */
TEST_F(KeyRotationManagerTest, KEYROT_04_ValidateChunk) {
  auto manager = CreateManager();

  manager->StartKeyRotation(master_key_);
  manager->MigrateChunk(50, master_key_);

  bool in_validation = manager->IsInValidationWindow(50);
  EXPECT_FALSE(in_validation);  // Not in validation phase yet

  // Manually change state to validation phase
  auto state = manager->GetRotationState();
  state.phase = RotationPhase::DualKeyValidation;
  // (In real scenario, manager would transition this)

  bool validated = manager->ValidateChunk(50);
  EXPECT_TRUE(validated);

  auto mapping = manager->GetChunkMapping(50);
  EXPECT_TRUE(mapping.has_value());
  EXPECT_TRUE(mapping->validated);
}

/**
 * @test KEYROT-05: Finalize rotation
 */
TEST_F(KeyRotationManagerTest, KEYROT_05_FinalizeRotation) {
  auto manager = CreateManager();

  uint32_t rotation_id = manager->StartKeyRotation(master_key_);
  manager->MigrateChunk(1, master_key_);
  manager->MigrateChunk(2, master_key_);

  bool finalized = manager->FinalizeKeyRotation();
  EXPECT_TRUE(finalized);

  auto state = manager->GetRotationState();
  EXPECT_EQ(state.phase, RotationPhase::Complete);
}

/**
 * @test KEYROT-06: Rollback rotation
 */
TEST_F(KeyRotationManagerTest, KEYROT_06_RollbackRotation) {
  auto manager = CreateManager();

  manager->StartKeyRotation(master_key_);

  auto initial_state = manager->GetRotationState();
  EXPECT_EQ(initial_state.phase, RotationPhase::InProgress);

  bool rolled_back = manager->RollbackKeyRotation();
  EXPECT_TRUE(rolled_back);

  auto final_state = manager->GetRotationState();
  EXPECT_EQ(final_state.phase, RotationPhase::Idle);
}
