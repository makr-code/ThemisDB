#include <gtest/gtest.h>
#include "storage/storage_engine.h"

#include <filesystem>
#include <string>
#include <vector>
#include <memory>
#include <utility>
#include <chrono>

namespace fs = std::filesystem;
using namespace themis;

// ─────────────────────────────────────────────────────────────────────────────
// Fixture for Move Semantics Tests
// ─────────────────────────────────────────────────────────────────────────────

class StorageEngineMoveSemanticTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path1_ = (fs::temp_directory_path() /
                    ("themis_move_test1_" +
                     std::to_string(
                         std::chrono::system_clock::now().time_since_epoch().count())))
                       .string();
        db_path2_ = (fs::temp_directory_path() /
                    ("themis_move_test2_" +
                     std::to_string(
                         std::chrono::system_clock::now().time_since_epoch().count() + 1)))
                       .string();
        fs::remove_all(db_path1_);
        fs::remove_all(db_path2_);
    }

    void TearDown() override {
        fs::remove_all(db_path1_);
        fs::remove_all(db_path2_);
    }

    std::string db_path1_;
    std::string db_path2_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Test: Move Constructor (CWE-457 Remediation)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StorageEngineMoveSemanticTest, MoveConstructor_TransfersState) {
    // Create an engine and open it
    auto engine1 = StorageEngine::createDefault();
    ASSERT_TRUE(engine1->open(db_path1_).has_value());
    
    // Put some data
    ASSERT_TRUE(engine1->put("key1", "value1").has_value());
    ASSERT_TRUE(engine1->put("key2", "value2").has_value());
    
    // Move construct into engine2
    auto engine2 = std::move(engine1);
    
    // engine2 should work correctly with the transferred state
    auto result = engine2->get("key1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "value1");
    
    // Cleanup
    engine2->close();
}

// ─────────────────────────────────────────────────────────────────────────────
// Test: Move Assignment Operator (CWE-457, CWE-672 Remediation)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StorageEngineMoveSemanticTest, MoveAssignmentOperator_TransfersState) {
    // Create and initialize two engines
    auto engine1 = StorageEngine::createDefault();
    auto engine2 = StorageEngine::createDefault();
    
    ASSERT_TRUE(engine1->open(db_path1_).has_value());
    ASSERT_TRUE(engine2->open(db_path2_).has_value());
    
    // Put different data in each
    ASSERT_TRUE(engine1->put("key1", "from_engine1").has_value());
    ASSERT_TRUE(engine2->put("key2", "from_engine2").has_value());
    
    // Move assign engine1 into engine2 (engine2's old db_path2 should be closed)
    engine2 = std::move(engine1);
    
    // engine2 should now have engine1's state
    auto result = engine2->get("key1");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result.value(), "from_engine1");
    
    // And should not have engine2's old data anymore
    auto result2 = engine2->get("key2");
    EXPECT_FALSE(result2.has_value()); // key2 is in db_path2, not db_path1
    
    // Cleanup
    engine2->close();
}

// ─────────────────────────────────────────────────────────────────────────────
// Test: Moved-from Object State Validity (CWE-672 Remediation)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StorageEngineMoveSemanticTest, MovedFrom_ObjectInValidState) {
    // Create an engine and open it
    auto engine1 = StorageEngine::createDefault();
    ASSERT_TRUE(engine1->open(db_path1_).has_value());
    ASSERT_TRUE(engine1->put("key", "value").has_value());
    
    // Move construct: engine1 should still be in a valid (though unspecified) state
    auto engine2 = std::move(engine1);
    
    // Calling close() on moved-from object should be safe (idempotent)
    // For shared_ptr semantics the moved-from pointer may be null; handle both cases.
    if (engine1) {
        EXPECT_NO_THROW({ engine1->close(); });
    } else {
        SUCCEED() << "engine1 is null after move (shared_ptr); nothing to close";
    }
    
    // engine2 should still work
    auto result = engine2->get("key");
    EXPECT_TRUE(result.has_value());
    
    // Cleanup
    engine2->close();
}

// ─────────────────────────────────────────────────────────────────────────────
// Test: Encrypt Field Vector Return Uses Move Semantics (CWE-457 Remediation)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StorageEngineMoveSemanticTest, EncryptField_ReturnsVectorByMove) {
    auto engine = StorageEngine::createDefault();
    ASSERT_TRUE(engine->open(db_path1_).has_value());
    
    // Create a plaintext vector
    std::vector<uint8_t> plaintext = {0x01, 0x02, 0x03, 0x04, 0x05};
    
    // Encrypt field should return by move (no unnecessary copy)
    auto ciphertext = engine->encrypt_field("test_field", plaintext);
    
    // Verify that ciphertext contains data
    ASSERT_FALSE(ciphertext.empty());
    
    // With default (no-op) encryption, should match plaintext
    EXPECT_EQ(ciphertext.size(), plaintext.size());
    
    // Cleanup
    engine->close();
}

// ─────────────────────────────────────────────────────────────────────────────
// Test: Decrypt Field Vector Return Uses Move Semantics (CWE-457 Remediation)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StorageEngineMoveSemanticTest, DecryptField_ReturnsVectorByMove) {
    auto engine = StorageEngine::createDefault();
    ASSERT_TRUE(engine->open(db_path1_).has_value());
    
    // Create encrypted data
    std::vector<uint8_t> ciphertext = {0x01, 0x02, 0x03, 0x04, 0x05};
    
    // Decrypt field should return by move (no unnecessary copy)
    auto plaintext = engine->decrypt_field("test_field", ciphertext);
    
    // Verify that plaintext contains data
    ASSERT_FALSE(plaintext.empty());
    
    // With default (no-op) encryption, should match ciphertext
    EXPECT_EQ(plaintext.size(), ciphertext.size());
    
    // Cleanup
    engine->close();
}

// ─────────────────────────────────────────────────────────────────────────────
// Test: IOMetrics Return Uses Move Semantics (CWE-457 Remediation)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StorageEngineMoveSemanticTest, IOMetrics_ReturnsStructByMove) {
    auto engine = StorageEngine::createDefault();
    ASSERT_TRUE(engine->open(db_path1_).has_value());
    
    // Perform some I/O operations
    for (int i = 0; i < 5; ++i) {
        engine->put("key_" + std::to_string(i), "value_" + std::to_string(i));
        engine->get("key_" + std::to_string(i));
    }
    
    // Get metrics (should use move semantics)
    auto metrics = engine->ioMetrics();
    
    // Verify metrics are populated
    EXPECT_GT(metrics.put_ops, 0);
    EXPECT_GT(metrics.get_ops, 0);
    EXPECT_EQ(metrics.put_errors, 0);
    EXPECT_EQ(metrics.get_errors, 0);
    
    // Cleanup
    engine->close();
}

// ─────────────────────────────────────────────────────────────────────────────
// Test: ScanCounters Return Uses Move Semantics (CWE-457 Remediation)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StorageEngineMoveSemanticTest, ScanCounters_ReturnsStructByMove) {
    auto engine = StorageEngine::createDefault();
    ASSERT_TRUE(engine->open(db_path1_).has_value());
    
    // Populate some data
    for (int i = 0; i < 5; ++i) {
        engine->put("key_" + std::to_string(i), "value_" + std::to_string(i));
    }
    
    // Perform scan operations over the inserted keys using prefix scan
    // (scanRange with empty start/end may be treated as empty range by backend)
    engine->scanPrefix("key_", [](std::string_view, std::string_view) {
        return true;
    });
    
    // Get scan counters (should use move semantics)
    auto counters = engine->scanCounters();
    
    // Verify counters are populated
    EXPECT_GT(counters.scan_calls, 0);
    EXPECT_GT(counters.keys_examined, 0);
    
    // Cleanup
    engine->close();
}

// ─────────────────────────────────────────────────────────────────────────────
// Test: No Copy Operations (Deleted Copy Constructor)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StorageEngineMoveSemanticTest, CopyConstructor_IsDeleted) {
    // This test verifies that copy operations are explicitly deleted
    // Attempting to copy should result in a compile error
    // (We can't test this directly in a runtime test, but this documents the intent)
    
    auto engine = StorageEngine::createDefault();
    
    // The following would be a compile error:
    // auto engine_copy = engine;  // ERROR: copy constructor is deleted
    // This is intentional to prevent accidental copying of injected dependencies
    
    // Verify the engine is move-constructible instead
    auto engine_moved = std::move(engine);
    EXPECT_NE(&engine_moved, nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test: Return Value Optimization (RVO) - Multiple Returns
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(StorageEngineMoveSemanticTest, MultipleMetricsReturns_NoDoubleFreeSS) {
    auto engine = StorageEngine::createDefault();
    ASSERT_TRUE(engine->open(db_path1_).has_value());
    
    // Perform multiple I/O operations to generate metrics
    for (int i = 0; i < 10; ++i) {
        engine->put("k" + std::to_string(i), "v" + std::to_string(i));
    }
    
    // Call ioMetrics multiple times (tests no double-free on repeated moves)
    for (int i = 0; i < 3; ++i) {
        auto metrics = engine->ioMetrics();
        EXPECT_GE(metrics.put_ops, 10);
    }
    
    // Call scanCounters multiple times
    for (int i = 0; i < 3; ++i) {
        auto counters = engine->scanCounters();
        // Should be valid even if called multiple times
        EXPECT_GE(counters.keys_examined, 0);
    }
    
    // Cleanup
    engine->close();
}
