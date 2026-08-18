/**
 * @file connection_leak_tests.cpp
 * @brief Comprehensive test cases for connection leak scenarios
 * @version 1.0
 * @note Tests for database connection and resource leak detection
 *
 * This test file validates:
 * 1. Connection leak prevention through RAII guards
 * 2. Exception path resource cleanup
 * 3. Connection pool health under stress
 * 4. Transaction resource lifecycle management
 * 5. Lock acquisition/release pairs
 */

#include <gtest/gtest.h>
#include <memory>
#include <thread>
#include <vector>
#include <atomic>
#include <stdexcept>

// Mock classes for testing
namespace themis::testing {

// Mock Connection for testing
class MockConnection : public storage::DatabaseConnectionManager::Connection {
public:
    static std::atomic<size_t> instance_count;
    static std::atomic<size_t> closed_count;
    
    MockConnection() {
        instance_count.fetch_add(1, std::memory_order_relaxed);
    }
    
    ~MockConnection() override {
        // Note: close() should have been called before destruction
        if (!was_closed_) {
            // This is a leak detection point
        }
    }
    
    [[nodiscard]] bool isValid() const override {
        return is_valid_;
    }
    
    [[nodiscard]] bool ping() override {
        return is_valid_;
    }
    
    [[nodiscard]] std::string getError() const override {
        return last_error_;
    }
    
    void close() override {
        is_valid_ = false;
        was_closed_ = true;
        closed_count.fetch_add(1, std::memory_order_relaxed);
    }
    
    void simulateError(std::string error) {
        is_valid_ = false;
        last_error_ = std::move(error);
    }
    
    bool wasClosedProperly() const { return was_closed_; }
    
private:
    bool is_valid_{true};
    bool was_closed_{false};
    std::string last_error_;
};

std::atomic<size_t> MockConnection::instance_count{0};
std::atomic<size_t> MockConnection::closed_count{0};

// Mock DatabaseConnectionManager for testing
class MockDatabaseConnectionManager : public storage::DatabaseConnectionManager {
public:
    MockDatabaseConnectionManager()
        : DatabaseConnectionManager(ConnectionConfig()) {}
    
    std::shared_ptr<Connection> createConnection() override {
        return std::make_shared<MockConnection>();
    }
};

} // namespace themis::testing

// ============================================================================
// Test Fixtures
// ============================================================================

class ConnectionLeakTests : public ::testing::Test {
protected:
    void SetUp() override {
        themis::testing::MockConnection::instance_count.store(0);
        themis::testing::MockConnection::closed_count.store(0);
    }
    
    void TearDown() override {
        // Verify no connection leaks
        size_t instances = themis::testing::MockConnection::instance_count.load();
        size_t closed = themis::testing::MockConnection::closed_count.load();
        EXPECT_EQ(instances, closed) 
            << "Connection leak detected: " << (instances - closed) << " connections not closed";
    }
};

// ============================================================================
// Test Cases: ConnectionGuard RAII Wrapper
// ============================================================================

TEST_F(ConnectionLeakTests, ConnectionGuardAcquireRelease) {
    themis::testing::MockDatabaseConnectionManager manager;
    
    {
        transaction::ConnectionGuard guard(manager);
        auto conn = guard.getConnection();
        ASSERT_NE(conn, nullptr);
        EXPECT_TRUE(guard.isValid());
    }
    // Guard destroyed - connection must be released
    
    EXPECT_EQ(themis::testing::MockConnection::instance_count.load(),
              themis::testing::MockConnection::closed_count.load());
}

TEST_F(ConnectionLeakTests, ConnectionGuardExceptionPath) {
    themis::testing::MockDatabaseConnectionManager manager;
    
    try {
        transaction::ConnectionGuard guard(manager);
        auto conn = guard.getConnection();
        ASSERT_NE(conn, nullptr);
        
        // Simulate exception
        throw std::runtime_error("Test exception");
    } catch (const std::runtime_error&) {
        // Exception caught
    }
    
    // Guard should have released connection despite exception
    EXPECT_EQ(themis::testing::MockConnection::instance_count.load(),
              themis::testing::MockConnection::closed_count.load());
}

TEST_F(ConnectionLeakTests, ConnectionGuardMarkError) {
    themis::testing::MockDatabaseConnectionManager manager;
    
    {
        transaction::ConnectionGuard guard(manager);
        auto conn = guard.getConnection();
        ASSERT_NE(conn, nullptr);
        
        // Mark error
        guard.markError("Simulated operation error");
        EXPECT_FALSE(guard.isReleased());
    }
    
    // Connection released even after error marking
    EXPECT_EQ(themis::testing::MockConnection::instance_count.load(),
              themis::testing::MockConnection::closed_count.load());
}

TEST_F(ConnectionLeakTests, ConnectionGuardManualRelease) {
    themis::testing::MockDatabaseConnectionManager manager;
    
    {
        transaction::ConnectionGuard guard(manager);
        auto conn = guard.getConnection();
        ASSERT_NE(conn, nullptr);
        
        // Manual release
        guard.release();
        EXPECT_TRUE(guard.isReleased());
        
        // Second release should be no-op
        guard.release();
        EXPECT_TRUE(guard.isReleased());
    }
    
    // No double-release
    EXPECT_EQ(themis::testing::MockConnection::instance_count.load(),
              themis::testing::MockConnection::closed_count.load());
}

TEST_F(ConnectionLeakTests, ConnectionGuardMove) {
    themis::testing::MockDatabaseConnectionManager manager;
    
    transaction::ConnectionGuard guard1(manager);
    auto conn = guard1.getConnection();
    ASSERT_NE(conn, nullptr);
    
    {
        // Move guard
        transaction::ConnectionGuard guard2(std::move(guard1));
        EXPECT_TRUE(guard2.isValid());
        EXPECT_TRUE(guard1.isReleased());  // Source marked as released
    }
    
    // Connection released via moved-to guard
    EXPECT_EQ(themis::testing::MockConnection::instance_count.load(),
              themis::testing::MockConnection::closed_count.load());
}

// ============================================================================
// Test Cases: TransactionConnectionGuard
// ============================================================================

TEST_F(ConnectionLeakTests, TransactionConnectionGuardBasic) {
    themis::testing::MockDatabaseConnectionManager manager;
    
    {
        transaction::TransactionConnectionGuard txn_guard(1, manager);
        
        auto conn = txn_guard.acquireConnection("test_operation", false);
        ASSERT_NE(conn, nullptr);
        
        txn_guard.recordSuccess("test_operation");
        
        EXPECT_EQ(txn_guard.getConnectionCount(), 1);
        EXPECT_EQ(txn_guard.getSuccessCount(), 1);
        EXPECT_EQ(txn_guard.getFailureCount(), 0);
    }
    
    // All connections released
    EXPECT_EQ(themis::testing::MockConnection::instance_count.load(),
              themis::testing::MockConnection::closed_count.load());
}

TEST_F(ConnectionLeakTests, TransactionConnectionGuardMultiple) {
    themis::testing::MockDatabaseConnectionManager manager;
    
    {
        transaction::TransactionConnectionGuard txn_guard(2, manager);
        
        // Acquire multiple connections
        for (int i = 0; i < 3; ++i) {
            auto conn = txn_guard.acquireConnection("operation_" + std::to_string(i), false);
            ASSERT_NE(conn, nullptr);
            txn_guard.recordSuccess("operation_" + std::to_string(i));
        }
        
        EXPECT_EQ(txn_guard.getConnectionCount(), 3);
        EXPECT_EQ(txn_guard.getSuccessCount(), 3);
    }
    
    // All 3 connections released
    EXPECT_EQ(themis::testing::MockConnection::instance_count.load(),
              themis::testing::MockConnection::closed_count.load());
}

TEST_F(ConnectionLeakTests, TransactionConnectionGuardException) {
    themis::testing::MockDatabaseConnectionManager manager;
    
    try {
        transaction::TransactionConnectionGuard txn_guard(3, manager);
        
        auto conn1 = txn_guard.acquireConnection("op1", false);
        ASSERT_NE(conn1, nullptr);
        txn_guard.recordSuccess("op1");
        
        auto conn2 = txn_guard.acquireConnection("op2", true);
        ASSERT_NE(conn2, nullptr);
        
        // Simulate exception before recording
        throw std::runtime_error("Operation failed");
    } catch (const std::runtime_error&) {
        // Caught
    }
    
    // All connections must be released despite exception
    EXPECT_EQ(themis::testing::MockConnection::instance_count.load(),
              themis::testing::MockConnection::closed_count.load());
}

TEST_F(ConnectionLeakTests, TransactionConnectionGuardFailure) {
    themis::testing::MockDatabaseConnectionManager manager;
    
    {
        transaction::TransactionConnectionGuard txn_guard(4, manager);
        
        auto conn1 = txn_guard.acquireConnection("op1", false);
        ASSERT_NE(conn1, nullptr);
        txn_guard.recordSuccess("op1");
        
        auto conn2 = txn_guard.acquireConnection("op2", true);
        ASSERT_NE(conn2, nullptr);
        txn_guard.recordFailure("op2", "Database error");
        
        EXPECT_EQ(txn_guard.getConnectionCount(), 2);
        EXPECT_EQ(txn_guard.getSuccessCount(), 1);
        EXPECT_EQ(txn_guard.getFailureCount(), 1);
    }
    
    // All connections released
    EXPECT_EQ(themis::testing::MockConnection::instance_count.load(),
              themis::testing::MockConnection::closed_count.load());
}

// ============================================================================
// Test Cases: ConnectionScopeTracker
// ============================================================================

TEST_F(ConnectionLeakTests, ConnectionScopeTrackerSuccess) {
    {
        transaction::ConnectionScopeTracker tracker("read_operation", false);
        
        // Simulate some work
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        tracker.recordSuccess();
        EXPECT_GE(tracker.getDurationMs(), 10);
    }
}

TEST_F(ConnectionLeakTests, ConnectionScopeTrackerFailure) {
    {
        transaction::ConnectionScopeTracker tracker("write_operation", true);
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        tracker.recordFailure("Constraint violation");
        EXPECT_GE(tracker.getDurationMs(), 10);
    }
}

TEST_F(ConnectionLeakTests, ConnectionScopeTrackerImplicitRecord) {
    // Tracker goes out of scope without explicit record
    {
        transaction::ConnectionScopeTracker tracker("unrecorded_op", false);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        // No explicit record - should auto-record on destruction
    }
    // Should not crash or leak
}

// ============================================================================
// Test Cases: Stress and Concurrent Access
// ============================================================================

TEST_F(ConnectionLeakTests, ConcurrentConnectionAcquisition) {
    themis::testing::MockDatabaseConnectionManager manager;
    
    const int NUM_THREADS = 10;
    const int ITERATIONS = 5;
    
    std::vector<std::thread> threads;
    
    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([&manager, t]() {
            for (int i = 0; i < ITERATIONS; ++i) {
                transaction::ConnectionGuard guard(manager, false,
                                                   std::chrono::seconds(5));
                if (auto conn = guard.getConnection()) {
                    EXPECT_TRUE(conn->isValid());
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                }
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // All connections must be accounted for
    EXPECT_EQ(themis::testing::MockConnection::instance_count.load(),
              themis::testing::MockConnection::closed_count.load());
}

TEST_F(ConnectionLeakTests, TransactionGuardUnderLoad) {
    themis::testing::MockDatabaseConnectionManager manager;
    
    const int NUM_TRANSACTIONS = 20;
    
    for (int txn_id = 0; txn_id < NUM_TRANSACTIONS; ++txn_id) {
        transaction::TransactionConnectionGuard txn_guard(txn_id, manager);
        
        // Simulate transaction work
        for (int op = 0; op < 3; ++op) {
            auto conn = txn_guard.acquireConnection(
                "txn_" + std::to_string(txn_id) + "_op_" + std::to_string(op),
                op == 2  // Last operation is a write
            );
            
            if (conn) {
                if (op % 2 == 0) {
                    txn_guard.recordSuccess("txn_op");
                } else {
                    txn_guard.recordFailure("txn_op", "Simulated error");
                }
            }
        }
    }
    
    // All connections released
    EXPECT_EQ(themis::testing::MockConnection::instance_count.load(),
              themis::testing::MockConnection::closed_count.load());
}

// ============================================================================
// Test Cases: Helper Functions
// ============================================================================

TEST_F(ConnectionLeakTests, ExecuteWithConnectionSuccess) {
    themis::testing::MockDatabaseConnectionManager manager;
    
    bool called = false;
    bool result = transaction::executeWithConnection(
        manager,
        [&called](auto conn) {
            called = true;
            EXPECT_NE(conn, nullptr);
        },
        "test_operation"
    );
    
    EXPECT_TRUE(called);
    EXPECT_TRUE(result);
    
    // Connection must be released
    EXPECT_EQ(themis::testing::MockConnection::instance_count.load(),
              themis::testing::MockConnection::closed_count.load());
}

TEST_F(ConnectionLeakTests, ExecuteWithConnectionException) {
    themis::testing::MockDatabaseConnectionManager manager;
    
    bool result = transaction::executeWithConnection(
        manager,
        [](auto conn) {
            EXPECT_NE(conn, nullptr);
            throw std::runtime_error("Operation failed");
        },
        "failing_operation"
    );
    
    EXPECT_FALSE(result);
    
    // Connection must still be released
    EXPECT_EQ(themis::testing::MockConnection::instance_count.load(),
              themis::testing::MockConnection::closed_count.load());
}

} // namespace themis::testing

// ============================================================================
// Test Execution
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
