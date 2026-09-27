/**
 * @file test_mongodb_adapter_phase2.cpp
 * @brief MongoDB adapter smoke tests for the Chimera-focused target.
 */

#include <gtest/gtest.h>

#include "chimera/mongodb_adapter.hpp"

namespace chimera {

TEST(MongoDBAdapterPhase2, DisconnectWhenNotConnectedIsOk) {
    MongoDBAdapter adapter;
    const auto result = adapter.disconnect();
    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(adapter.is_connected());
}

TEST(MongoDBAdapterPhase2, EmptyConnectionStringIsRejected) {
    MongoDBAdapter adapter;
    const auto result = adapter.connect("");
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::INVALID_ARGUMENT);
    EXPECT_FALSE(adapter.is_connected());
}

TEST(MongoDBAdapterPhase2, QueryWithoutConnectionReturnsError) {
    MongoDBAdapter adapter;
    const auto result = adapter.execute_query("SELECT 1");
    EXPECT_TRUE(result.is_err());
}

TEST(MongoDBAdapterPhase2, BatchInsertWithEmptyRowsIsHandled) {
    MongoDBAdapter adapter;
    std::vector<RelationalRow> rows;
    const auto result = adapter.batch_insert("", rows);
    EXPECT_TRUE(result.is_err());
}

TEST(MongoDBAdapterPhase2, UnsupportedVectorPathReturnsNotImplemented) {
    MongoDBAdapter adapter;
    Vector vector;
    vector.data = {1.0f, 2.0f, 3.0f};

    const auto result = adapter.insert_vector("vectors", vector);
    EXPECT_TRUE(result.is_err());
    EXPECT_EQ(result.error_code, ErrorCode::NOT_IMPLEMENTED);
}

} // namespace chimera
