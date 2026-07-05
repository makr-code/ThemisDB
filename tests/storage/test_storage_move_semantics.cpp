/**
 * @file test_storage_move_semantics.cpp
 * @brief Tests for Storage module move semantics
 * @version 1.0.0
 * @date 2026-07-05
 */

#include <gtest/gtest.h>
#include "storage/storage_move_semantics.h"
#include <memory>
#include <utility>

namespace themis {
namespace storage {

class StorageModuleMoveTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(StorageModuleMoveTest, IndexCursor_MoveConstruction) {
    auto cursor1 = std::make_unique<IndexCursor>();
    IndexCursor cursor2(std::move(*cursor1));
    
    EXPECT_FALSE(cursor2.isValid());
    EXPECT_EQ(cursor2.getPosition(), 0);
    EXPECT_FALSE(cursor1->isValid());
}

TEST_F(StorageModuleMoveTest, IndexCursor_DeletesCopy) {
    static_assert(!std::is_copy_constructible_v<IndexCursor>);
    static_assert(!std::is_copy_assignable_v<IndexCursor>);
}

TEST_F(StorageModuleMoveTest, IndexBuilder_MoveConstruction) {
    auto builder1 = std::make_unique<IndexBuilder>();
    IndexBuilder builder2(std::move(*builder1));
    
    EXPECT_EQ(builder2.getEntryCount(), 0);
    EXPECT_EQ(builder1->getEntryCount(), 0);
}

TEST_F(StorageModuleMoveTest, IndexBuilder_DeletesCopy) {
    static_assert(!std::is_copy_constructible_v<IndexBuilder>);
    static_assert(!std::is_copy_assignable_v<IndexBuilder>);
}

TEST_F(StorageModuleMoveTest, ColumnStore_MoveConstruction) {
    auto store1 = std::make_unique<ColumnStore>();
    ColumnStore store2(std::move(*store1));
    
    EXPECT_EQ(store2.getColumnCount(), 0);
    EXPECT_EQ(store1->getColumnCount(), 0);
}

TEST_F(StorageModuleMoveTest, ColumnStore_DeletesCopy) {
    static_assert(!std::is_copy_constructible_v<ColumnStore>);
    static_assert(!std::is_copy_assignable_v<ColumnStore>);
}

TEST_F(StorageModuleMoveTest, RocksDBHandleWrapper_MoveConstruction) {
    auto handle1 = std::make_unique<RocksDBHandleWrapper>();
    RocksDBHandleWrapper handle2(std::move(*handle1));
    
    EXPECT_FALSE(handle2.isOpen());
    EXPECT_FALSE(handle1->isOpen());
}

TEST_F(StorageModuleMoveTest, RocksDBHandleWrapper_MoveAssignment) {
    auto handle1 = std::make_unique<RocksDBHandleWrapper>();
    auto handle2 = std::make_unique<RocksDBHandleWrapper>();
    
    *handle2 = std::move(*handle1);
    
    EXPECT_FALSE(handle2->isOpen());
    EXPECT_FALSE(handle1->isOpen());
}

TEST_F(StorageModuleMoveTest, RocksDBHandleWrapper_DeletesCopy) {
    static_assert(!std::is_copy_constructible_v<RocksDBHandleWrapper>);
    static_assert(!std::is_copy_assignable_v<RocksDBHandleWrapper>);
}

TEST_F(StorageModuleMoveTest, StorageModule_MoveNoexceptGuarantee) {
    static_assert(std::is_nothrow_move_constructible_v<RocksDBHandleWrapper>);
    static_assert(std::is_nothrow_move_assignable_v<RocksDBHandleWrapper>);
}

}  // namespace storage
}  // namespace themis

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
