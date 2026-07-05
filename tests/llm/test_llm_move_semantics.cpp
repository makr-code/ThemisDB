/**
 * @file test_llm_move_semantics.cpp
 * @brief Tests for LLM module move semantics (ModelCacheEntry, Factories)
 * @version 1.0.0
 * @date 2026-07-05
 */

#include <gtest/gtest.h>
#include "llm/model_cache.h"
#include "llm/llm_adapter_factory.h"
#include <memory>
#include <utility>

namespace themis {
namespace llm {

// Forward declarations for testing
class MockModel {
public:
    MockModel(const std::string& id) : id_(id) {}
    ~MockModel() = default;
    const std::string& getId() const { return id_; }
private:
    std::string id_;
};

class LLMModuleMoveTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

// ============================================================================
// ModelCacheEntry Tests (Gap 1.1)
// ============================================================================

TEST_F(LLMModuleMoveTest, ModelCacheEntry_MoveConstruction) {
    // Create source entry
    auto entry1 = std::make_unique<ModelCacheEntry>();
    
    // Move construct to entry2
    auto entry2(std::move(*entry1));
    
    // Verify entry2 has valid empty state after move construction
    EXPECT_FALSE(entry2.isValid());
    EXPECT_EQ(entry2.access_count(), 0);
}

TEST_F(LLMModuleMoveTest, ModelCacheEntry_MoveAssignment) {
    // Create source and destination
    auto entry1 = std::make_unique<ModelCacheEntry>();
    auto entry2 = std::make_unique<ModelCacheEntry>();
    
    // Move assign from entry1 to entry2
    *entry2 = std::move(*entry1);
    
    // Verify entry2 has valid state
    EXPECT_FALSE(entry2->isValid());
    EXPECT_EQ(entry2->access_count(), 0);
    
    // Verify entry1 is in valid empty state
    EXPECT_FALSE(entry1->isValid());
    EXPECT_EQ(entry1->access_count(), 0);
}

TEST_F(LLMModuleMoveTest, ModelCacheEntry_MoveChain) {
    // Create and move in a chain
    auto a = std::make_unique<ModelCacheEntry>();
    auto b(std::move(*a));
    auto c(std::move(b));
    
    // Verify final state
    EXPECT_FALSE(c.isValid());
    EXPECT_EQ(c.access_count(), 0);
    
    // Verify intermediate states
    EXPECT_FALSE(b.isValid());
    EXPECT_EQ(b.access_count(), 0);
    
    EXPECT_FALSE(a->isValid());
    EXPECT_EQ(a->access_count(), 0);
}

TEST_F(LLMModuleMoveTest, ModelCacheEntry_DeletesCopy) {
    // Verify copy semantics are deleted
    static_assert(!std::is_copy_constructible_v<ModelCacheEntry>,
                  "ModelCacheEntry should not be copy constructible");
    static_assert(!std::is_copy_assignable_v<ModelCacheEntry>,
                  "ModelCacheEntry should not be copy assignable");
}

TEST_F(LLMModuleMoveTest, ModelCacheEntry_IsMove) {
    // Verify move semantics exist
    static_assert(std::is_move_constructible_v<ModelCacheEntry>,
                  "ModelCacheEntry should be move constructible");
    static_assert(std::is_move_assignable_v<ModelCacheEntry>,
                  "ModelCacheEntry should be move assignable");
}

TEST_F(LLMModuleMoveTest, ModelCacheEntry_AccessCountTracking) {
    auto entry1 = std::make_unique<ModelCacheEntry>();
    entry1->incrementAccessCount();
    entry1->incrementAccessCount();
    EXPECT_EQ(entry1->access_count(), 2);
    
    auto entry2(std::move(*entry1));
    
    // Access count transferred
    EXPECT_EQ(entry2.access_count(), 2);
    // Source reset to 0
    EXPECT_EQ(entry1->access_count(), 0);
}

// ============================================================================
// LLMAdapterFactory Tests (Gaps 1.2-1.5)
// ============================================================================

TEST_F(LLMModuleMoveTest, LLMAdapterFactory_MoveConstruction) {
    auto factory1 = std::make_unique<LLMAdapterFactory>();
    
    // Move construct to factory2
    auto factory2(std::move(*factory1));
    
    // Verify factory2 is valid (empty registry)
    EXPECT_EQ(factory2.getAdapterCount(), 0);
}

TEST_F(LLMModuleMoveTest, LLMAdapterFactory_MoveAssignment) {
    auto factory1 = std::make_unique<LLMAdapterFactory>();
    auto factory2 = std::make_unique<LLMAdapterFactory>();
    
    // Move assign
    *factory2 = std::move(*factory1);
    
    // Verify factory2 has valid state
    EXPECT_EQ(factory2->getAdapterCount(), 0);
    
    // Verify factory1 is empty
    EXPECT_EQ(factory1->getAdapterCount(), 0);
}

TEST_F(LLMModuleMoveTest, LLMAdapterFactory_DeletesCopy) {
    static_assert(!std::is_copy_constructible_v<LLMAdapterFactory>,
                  "LLMAdapterFactory should not be copy constructible");
    static_assert(!std::is_copy_assignable_v<LLMAdapterFactory>,
                  "LLMAdapterFactory should not be copy assignable");
}

TEST_F(LLMModuleMoveTest, LLMAdapterFactory_IsMove) {
    static_assert(std::is_move_constructible_v<LLMAdapterFactory>,
                  "LLMAdapterFactory should be move constructible");
    static_assert(std::is_move_assignable_v<LLMAdapterFactory>,
                  "LLMAdapterFactory should be move assignable");
}

// ============================================================================
// Edge Cases and Comprehensive Tests
// ============================================================================

TEST_F(LLMModuleMoveTest, ModelCacheEntry_SelfAssignmentSafe) {
    auto entry = std::make_unique<ModelCacheEntry>();
    entry->incrementAccessCount();
    entry->incrementAccessCount();
    
    // Self-assignment should be safe (no-op)
    ModelCacheEntry* ptr = entry.get();
    *ptr = std::move(*entry);
    
    // Verify no corruption
    EXPECT_FALSE(entry->isValid());
    EXPECT_EQ(entry->access_count(), 0);
}

TEST_F(LLMModuleMoveTest, LLMAdapterFactory_MoveNoexceptGuarantee) {
    // Verify noexcept specifications
    static_assert(std::is_nothrow_move_constructible_v<LLMAdapterFactory>,
                  "LLMAdapterFactory move constructor must be noexcept");
    static_assert(std::is_nothrow_move_assignable_v<LLMAdapterFactory>,
                  "LLMAdapterFactory move assignment must be noexcept");
}

TEST_F(LLMModuleMoveTest, ModelCacheEntry_MoveNoexceptGuarantee) {
    static_assert(std::is_nothrow_move_constructible_v<ModelCacheEntry>,
                  "ModelCacheEntry move constructor must be noexcept");
    static_assert(std::is_nothrow_move_assignable_v<ModelCacheEntry>,
                  "ModelCacheEntry move assignment must be noexcept");
}

}  // namespace llm
}  // namespace themis

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
