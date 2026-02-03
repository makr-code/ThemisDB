// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/error_handling.h"
#include "sharding/exceptions.h"
#include <gtest/gtest.h>

using namespace themisdb::sharding;

class ErrorHandlingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test setup
    }
    
    void TearDown() override {
        // Test cleanup
    }
};

// Test error code to string conversion
TEST_F(ErrorHandlingTest, ErrorToString) {
    EXPECT_EQ(errorToString(DistributedSystemError::OK), "OK");
    EXPECT_EQ(errorToString(DistributedSystemError::TRANSACTION_NOT_FOUND), "Transaction not found");
    EXPECT_EQ(errorToString(DistributedSystemError::PARTICIPANT_UNREACHABLE), "Participant unreachable");
    EXPECT_EQ(errorToString(DistributedSystemError::CONSENSUS_FAILED), "Consensus failed");
    EXPECT_EQ(errorToString(DistributedSystemError::REPLICATION_FAILED), "Replication failed");
    EXPECT_EQ(errorToString(DistributedSystemError::HEALTH_CHECK_FAILED), "Health check failed");
    EXPECT_EQ(errorToString(DistributedSystemError::INTERNAL_ERROR), "Internal error");
}

// Test retriable error detection
TEST_F(ErrorHandlingTest, IsRetriableError) {
    // Retriable errors
    EXPECT_TRUE(isRetriableError(DistributedSystemError::TRANSACTION_TIMEOUT));
    EXPECT_TRUE(isRetriableError(DistributedSystemError::PARTICIPANT_UNREACHABLE));
    EXPECT_TRUE(isRetriableError(DistributedSystemError::CONNECTION_TIMEOUT));
    EXPECT_TRUE(isRetriableError(DistributedSystemError::NETWORK_UNSTABLE));
    EXPECT_TRUE(isRetriableError(DistributedSystemError::REPLICA_UNAVAILABLE));
    EXPECT_TRUE(isRetriableError(DistributedSystemError::QUORUM_NOT_REACHED));
    
    // Non-retriable errors
    EXPECT_FALSE(isRetriableError(DistributedSystemError::TRANSACTION_NOT_FOUND));
    EXPECT_FALSE(isRetriableError(DistributedSystemError::TRANSACTION_CONFLICT));
    EXPECT_FALSE(isRetriableError(DistributedSystemError::DEADLOCK_DETECTED));
    EXPECT_FALSE(isRetriableError(DistributedSystemError::AUTHENTICATION_FAILED));
    EXPECT_FALSE(isRetriableError(DistributedSystemError::INVALID_ARGUMENT));
}

// Test Result type with success
TEST_F(ErrorHandlingTest, ResultSuccess) {
    Result<int> result = Ok(42);
    
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(static_cast<bool>(result));
    EXPECT_EQ(*result, 42);
    EXPECT_EQ(result.error, DistributedSystemError::OK);
}

// Test Result type with error
TEST_F(ErrorHandlingTest, ResultError) {
    Result<int> result = Err<int>(
        DistributedSystemError::TRANSACTION_NOT_FOUND,
        "Transaction not found"
    );
    
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(static_cast<bool>(result));
    EXPECT_EQ(result.error, DistributedSystemError::TRANSACTION_NOT_FOUND);
    EXPECT_EQ(result.error_message, "Transaction not found");
}

// Test Result<void> with success
TEST_F(ErrorHandlingTest, ResultVoidSuccess) {
    Result<void> result = Ok();
    
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(static_cast<bool>(result));
    EXPECT_EQ(result.error, DistributedSystemError::OK);
}

// Test Result<void> with error
TEST_F(ErrorHandlingTest, ResultVoidError) {
    Result<void> result = Err(
        DistributedSystemError::REPLICATION_FAILED,
        "Replication failed"
    );
    
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(static_cast<bool>(result));
    EXPECT_EQ(result.error, DistributedSystemError::REPLICATION_FAILED);
    EXPECT_EQ(result.error_message, "Replication failed");
}

// Test Result with ErrorContext
TEST_F(ErrorHandlingTest, ResultWithContext) {
    ErrorContext ctx("test_operation", "TestComponent");
    ctx.transaction_id = "txn_123";
    
    Result<void> result = Err(
        DistributedSystemError::TRANSACTION_TIMEOUT,
        "Operation timed out",
        ctx
    );
    
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, DistributedSystemError::TRANSACTION_TIMEOUT);
    EXPECT_TRUE(result.context.has_value());
    EXPECT_EQ(result.context->transaction_id, "txn_123");
    EXPECT_EQ(result.context->operation_name, "test_operation");
    EXPECT_EQ(result.context->source_component, "TestComponent");
}

// Test valueOrThrow with success
TEST_F(ErrorHandlingTest, ValueOrThrowSuccess) {
    Result<int> result = Ok(42);
    EXPECT_NO_THROW({
        int value = result.valueOrThrow();
        EXPECT_EQ(value, 42);
    });
}

// Test valueOrThrow with error
TEST_F(ErrorHandlingTest, ValueOrThrowError) {
    Result<int> result = Err<int>(
        DistributedSystemError::TRANSACTION_NOT_FOUND,
        "Transaction not found"
    );
    
    EXPECT_THROW({
        result.valueOrThrow();
    }, ThemisDBException);
}

// Test ErrorContext construction
TEST_F(ErrorHandlingTest, ErrorContextConstruction) {
    ErrorContext ctx1;
    // Default constructed context should have empty operation name
    EXPECT_TRUE(ctx1.operation_name.empty());
    
    ErrorContext ctx2("my_operation");
    EXPECT_EQ(ctx2.operation_name, "my_operation");
    
    ErrorContext ctx3("my_operation", "MyComponent");
    EXPECT_EQ(ctx3.operation_name, "my_operation");
    EXPECT_EQ(ctx3.source_component, "MyComponent");
}

// Test ErrorContext with additional context
TEST_F(ErrorHandlingTest, ErrorContextAdditionalInfo) {
    ErrorContext ctx("test_op", "TestComp");
    ctx.transaction_id = "txn_456";
    ctx.call_stack.push_back("function1");
    ctx.call_stack.push_back("function2");
    ctx.additional_context["key1"] = "value1";
    ctx.additional_context["key2"] = "value2";
    
    EXPECT_EQ(ctx.transaction_id, "txn_456");
    EXPECT_EQ(ctx.call_stack.size(), 2);
    EXPECT_EQ(ctx.additional_context.size(), 2);
    EXPECT_EQ(ctx.additional_context["key1"], "value1");
}

// Test ThemisDBException
TEST_F(ErrorHandlingTest, ThemisDBException) {
    ThemisDBException ex(
        DistributedSystemError::INTERNAL_ERROR,
        "Test error",
        "TestComponent"
    );
    
    EXPECT_EQ(ex.error(), DistributedSystemError::INTERNAL_ERROR);
    EXPECT_EQ(ex.component(), "TestComponent");
    EXPECT_STREQ(ex.what(), "Test error");
}

// Test TransactionException
TEST_F(ErrorHandlingTest, TransactionException) {
    TransactionException ex(
        DistributedSystemError::TRANSACTION_TIMEOUT,
        "Transaction timed out",
        "txn_789"
    );
    
    EXPECT_EQ(ex.error(), DistributedSystemError::TRANSACTION_TIMEOUT);
    EXPECT_EQ(ex.transactionId(), "txn_789");
    EXPECT_STREQ(ex.what(), "Transaction timed out");
}

// Test NetworkException
TEST_F(ErrorHandlingTest, NetworkException) {
    NetworkException ex(
        DistributedSystemError::CONNECTION_TIMEOUT,
        "Connection timed out"
    );
    
    EXPECT_EQ(ex.error(), DistributedSystemError::CONNECTION_TIMEOUT);
    EXPECT_EQ(ex.component(), "Network");
}

// Test TimeoutException
TEST_F(ErrorHandlingTest, TimeoutException) {
    TimeoutException ex("Operation timed out", "TimeoutComp");
    
    EXPECT_EQ(ex.error(), DistributedSystemError::TRANSACTION_TIMEOUT);
    EXPECT_EQ(ex.component(), "TimeoutComp");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
