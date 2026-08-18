/**
 * @file test_access_model_observability.cpp
 * @brief Tests for Phase 5 Observability Infrastructure (logging, trace context).
 *
 * Tests verify:
 * - Structured logging framework compilation and usage
 * - Trace context propagation through thread-local storage
 * - Correlation ID generation and propagation
 * - Integration with AccessCoordinator
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <thread>
#include <chrono>

#include "access_model/access_model_logging.h"
#include "access_model/access_model_trace.h"
#include "access_model/access_tier_interface.h"

namespace themis {
namespace access_model {

// ============================================================================
// § 1  Trace Context Tests
// ============================================================================

class TraceContextTest : public ::testing::Test {
protected:
    void SetUp() override {
        TraceContextManager::clearContext();
    }
};

TEST_F(TraceContextTest, GenerateCorrelationID) {
    auto id1 = TraceContextManager::generateCorrelationID("test");
    auto id2 = TraceContextManager::generateCorrelationID("test");
    
    // IDs should be non-empty
    EXPECT_FALSE(id1.empty());
    EXPECT_FALSE(id2.empty());
    
    // IDs should start with prefix
    EXPECT_THAT(id1, ::testing::StartsWith("test-"));
    EXPECT_THAT(id2, ::testing::StartsWith("test-"));
    
    // IDs should be unique
    EXPECT_NE(id1, id2);
}

TEST_F(TraceContextTest, SetAndGetContext) {
    auto corr_id = TraceContextManager::generateCorrelationID("ctx");
    TraceContext ctx{corr_id};
    
    TraceContextManager::setContext(ctx);
    
    auto retrieved = TraceContextManager::getContext();
    EXPECT_EQ(retrieved.correlation_id, corr_id);
}

TEST_F(TraceContextTest, CurrentCorrelationID) {
    auto corr_id = TraceContextManager::generateCorrelationID("curr");
    TraceContext ctx{corr_id};
    
    TraceContextManager::setContext(ctx);
    
    auto current = TraceContextManager::currentCorrelationID();
    EXPECT_EQ(current, corr_id);
}

TEST_F(TraceContextTest, ScopedContextRAII) {
    auto outer_id = TraceContextManager::generateCorrelationID("outer");
    TraceContext outer_ctx{outer_id};
    TraceContextManager::setContext(outer_ctx);
    
    {
        auto inner_id = TraceContextManager::generateCorrelationID("inner");
        TraceContext inner_ctx{inner_id};
        auto scoped = TraceContextManager::ScopedContext(inner_ctx);
        
        // Inside scope, inner context is active
        auto current = TraceContextManager::currentCorrelationID();
        EXPECT_EQ(current, inner_id);
    }
    
    // After scope, outer context is restored
    auto current = TraceContextManager::currentCorrelationID();
    EXPECT_EQ(current, outer_id);
}

TEST_F(TraceContextTest, ThreadLocalIsolation) {
    auto main_id = TraceContextManager::generateCorrelationID("main");
    TraceContext main_ctx{main_id};
    TraceContextManager::setContext(main_ctx);
    
    std::string thread_id;
    std::thread t([&thread_id]() {
        // In new thread, context should be empty
        auto ctx = TraceContextManager::getContext();
        thread_id = ctx.correlation_id;
    });
    t.join();
    
    // Thread had empty context, main thread still has main_id
    EXPECT_TRUE(thread_id.empty());
    EXPECT_EQ(TraceContextManager::currentCorrelationID(), main_id);
}

// ============================================================================
// § 2  Structured Logging Tests
// ============================================================================

class LoggingTest : public ::testing::Test {
protected:
    void SetUp() override {
        logger_instance_ = &accessModelLogger();
    }
    
    AccessModelLogger* logger_instance_;
};

TEST_F(LoggingTest, LogTierTransition) {
    TierTransitionLog log{
        .key = "test_key",
        .from_tier = TierLevel::L1_WORKING,
        .to_tier = TierLevel::L2_EPISODIC,
        .reason = "policy_driven",
        .latency_ms = 5,
        .correlation_id = "corr-123",
        .thread_id = std::this_thread::get_id(),
        .timestamp = std::chrono::system_clock::now(),
        .status = "SUCCESS",
    };
    
    // Should not throw
    EXPECT_NO_THROW(logger_instance_->logTierTransition(log));
}

TEST_F(LoggingTest, LogEvictionEvent) {
    EvictionEventLog log{
        .key = "evict_key",
        .from_tier = TierLevel::L3_SEMANTIC,
        .eviction_reason = "lru",
        .size_bytes = 1024,
        .access_count = 42,
        .last_access_age = std::chrono::seconds(300),
        .decision = "DEMOTE",
        .correlation_id = "corr-456",
        .thread_id = std::this_thread::get_id(),
        .timestamp = std::chrono::system_clock::now(),
    };
    
    // Should not throw
    EXPECT_NO_THROW(logger_instance_->logEvictionEvent(log));
}

TEST_F(LoggingTest, LogPromotionDecision) {
    PromotionDecisionLog log{
        .key = "promo_key",
        .current_tier = TierLevel::STORAGE_COLD,
        .target_tier = TierLevel::STORAGE_WARM,
        .decision = "PROMOTE",
        .access_count = 100,
        .age_secs = std::chrono::seconds(3600),
        .threshold_name = "storage_promotion_threshold",
        .threshold_value = 50,
        .actual_value = 100,
        .reason = "hot_access_pattern",
        .correlation_id = "corr-789",
        .thread_id = std::this_thread::get_id(),
        .timestamp = std::chrono::system_clock::now(),
    };
    
    // Should not throw
    EXPECT_NO_THROW(logger_instance_->logPromotionDecision(log));
}

TEST_F(LoggingTest, LogCoordinatorLifecycle) {
    CoordinatorLifecycleLog log{
        .event_type = "START",
        .details = "worker_thread_count=4",
        .correlation_id = "startup-001",
        .thread_id = std::this_thread::get_id(),
        .timestamp = std::chrono::system_clock::now(),
    };
    
    // Should not throw
    EXPECT_NO_THROW(logger_instance_->logCoordinatorLifecycle(log));
}

// ============================================================================
// § 3  Integration Tests
// ============================================================================

class ObservabilityIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        TraceContextManager::clearContext();
    }
};

TEST_F(ObservabilityIntegrationTest, CorrelationPropagation) {
    auto op_id = TraceContextManager::generateCorrelationID("operation");
    TraceContext ctx{op_id};
    
    {
        auto scoped = TraceContextManager::ScopedContext(ctx);
        
        // Log event (would pick up active correlation ID)
        TierTransitionLog log{
            .key = "data",
            .from_tier = TierLevel::L2_EPISODIC,
            .to_tier = TierLevel::L3_SEMANTIC,
            .reason = "age_policy",
            .latency_ms = 10,
            .correlation_id = TraceContextManager::currentCorrelationID(),
            .thread_id = std::this_thread::get_id(),
            .timestamp = std::chrono::system_clock::now(),
            .status = "SUCCESS",
        };
        
        // Correlation ID should match operation ID
        EXPECT_EQ(log.correlation_id, op_id);
        
        // Logging should not throw
        EXPECT_NO_THROW(accessModelLogger().logTierTransition(log));
    }
}

TEST_F(ObservabilityIntegrationTest, HierarchicalTracing) {
    auto parent_id = TraceContextManager::generateCorrelationID("parent");
    TraceContext parent_ctx{parent_id};
    
    {
        auto parent_guard = TraceContextManager::ScopedContext(parent_ctx);
        
        // Create child context with parent reference
        auto child_id = TraceContextManager::generateCorrelationID("child");
        TraceContext child_ctx{child_id, parent_id};
        
        {
            auto child_guard = TraceContextManager::ScopedContext(child_ctx);
            
            auto current = TraceContextManager::getContext();
            EXPECT_EQ(current.correlation_id, child_id);
            EXPECT_EQ(current.parent_span_id, parent_id);
        }
        
        // Back to parent
        auto current = TraceContextManager::currentCorrelationID();
        EXPECT_EQ(current, parent_id);
    }
}

}  // namespace access_model
}  // namespace themis

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
