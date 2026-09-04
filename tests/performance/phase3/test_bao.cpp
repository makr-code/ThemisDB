// Tests for Bao ML-based query optimizer

#include "performance/phase3/bao.h"
#include <gtest/gtest.h>
#include <thread>
#include <vector>

using namespace themis::performance::phase3;

class BaoTest : public ::testing::Test {
protected:
    BaoOptimizer optimizer;
};

// Test plan generation
TEST_F(BaoTest, GeneratePlans) {
    auto plans = optimizer.generate_plans("SELECT * FROM users");
    EXPECT_GE(plans.size(), 3);
    
    // Check that plans have valid structure
    for (const auto& plan : plans) {
        EXPECT_FALSE(plan.plan_id.empty());
        EXPECT_FALSE(plan.operators.empty());
        EXPECT_GT(plan.estimated_cost, 0.0);
    }
}

// Test plan generation for complex query
TEST_F(BaoTest, GeneratePlansComplex) {
    auto plans = optimizer.generate_plans("SELECT * FROM users JOIN orders ON users.id = orders.user_id");
    EXPECT_GE(plans.size(), 5); // Complex queries get more plans
    
    for (const auto& plan : plans) {
        EXPECT_FALSE(plan.plan_id.empty());
    }
}

// Test plan selection
TEST_F(BaoTest, SelectPlan) {
    auto plans = optimizer.generate_plans("SELECT * FROM users");
    auto selected = optimizer.select_plan("SELECT * FROM users", plans);
    
    EXPECT_FALSE(selected.plan_id.empty());
    EXPECT_FALSE(selected.operators.empty());
}

// Test empty plans
TEST_F(BaoTest, SelectPlanEmpty) {
    std::vector<QueryPlan> empty_plans;
    auto selected = optimizer.select_plan("SELECT * FROM users", empty_plans);
    
    EXPECT_TRUE(selected.plan_id.empty());
}

// Test model update with successful execution
TEST_F(BaoTest, UpdateModelSuccess) {
    auto plans = optimizer.generate_plans("SELECT * FROM users");
    auto selected = optimizer.select_plan("SELECT * FROM users", plans);
    
    QueryResult result;
    result.execution_time_ms = 50.0;  // Fast execution
    result.rows_returned = 100;
    result.success = true;
    
    optimizer.update_model(selected, result);
    
    auto stats = optimizer.get_stats();
    EXPECT_EQ(stats.model_updates, 1);
}

// Test model update with slow execution
TEST_F(BaoTest, UpdateModelSlow) {
    auto plans = optimizer.generate_plans("SELECT * FROM users");
    auto selected = optimizer.select_plan("SELECT * FROM users", plans);
    
    QueryResult result;
    result.execution_time_ms = 900.0;  // Slow execution
    result.rows_returned = 100;
    result.success = true;
    
    optimizer.update_model(selected, result);
    
    auto stats = optimizer.get_stats();
    EXPECT_EQ(stats.model_updates, 1);
}

// Test model update with failed execution
TEST_F(BaoTest, UpdateModelFailure) {
    auto plans = optimizer.generate_plans("SELECT * FROM users");
    auto selected = optimizer.select_plan("SELECT * FROM users", plans);
    
    QueryResult result;
    result.execution_time_ms = 100.0;
    result.rows_returned = 0;
    result.success = false;  // Failed execution
    
    optimizer.update_model(selected, result);
    
    auto stats = optimizer.get_stats();
    EXPECT_EQ(stats.model_updates, 0);  // Failed executions are not used for training
}

// Test Thompson Sampling learns from feedback
TEST_F(BaoTest, ThompsonSamplingLearns) {
    auto plans = optimizer.generate_plans("SELECT * FROM users");
    ASSERT_GE(plans.size(), 2);
    
    // Simulate that plan_0 is always fast, plan_1 is always slow
    for (int i = 0; i < 20; ++i) {
        auto selected = optimizer.select_plan("SELECT * FROM users", plans);
        
        QueryResult result;
        result.success = true;
        result.rows_returned = 100;
        
        if (selected.plan_id == "plan_0") {
            result.execution_time_ms = 10.0;  // Very fast
        } else if (selected.plan_id == "plan_1") {
            result.execution_time_ms = 500.0; // Slow
        } else {
            result.execution_time_ms = 100.0; // Medium
        }
        
        optimizer.update_model(selected, result);
    }
    
    // After learning, optimizer should prefer plan_0
    int plan_0_selected = 0;
    for (int i = 0; i < 10; ++i) {
        auto selected = optimizer.select_plan("SELECT * FROM users", plans);
        if (selected.plan_id == "plan_0") {
            plan_0_selected++;
        }
    }
    
    // Most selections should be plan_0 after learning
    EXPECT_GT(plan_0_selected, 5);
}

// Test statistics tracking
TEST_F(BaoTest, Statistics) {
    auto plans = optimizer.generate_plans("SELECT * FROM users");
    
    // Run multiple queries
    for (int i = 0; i < 5; ++i) {
        auto selected = optimizer.select_plan("SELECT * FROM users", plans);
        
        QueryResult result;
        result.execution_time_ms = 50.0;
        result.rows_returned = 100;
        result.success = true;
        
        optimizer.update_model(selected, result);
    }
    
    auto stats = optimizer.get_stats();
    EXPECT_EQ(stats.queries_optimized, 5);
    EXPECT_EQ(stats.model_updates, 5);
    EXPECT_GT(stats.avg_speedup, 0.0);
}

// Test multiple queries
TEST_F(BaoTest, MultipleQueries) {
    std::vector<std::string> queries = {
        "SELECT * FROM users",
        "SELECT * FROM orders",
        "SELECT * FROM products JOIN categories"
    };
    
    for (const auto& query : queries) {
        auto plans = optimizer.generate_plans(query);
        auto selected = optimizer.select_plan(query, plans);
        EXPECT_FALSE(selected.plan_id.empty());
    }
    
    auto stats = optimizer.get_stats();
    EXPECT_EQ(stats.queries_optimized, queries.size());
}

// Test concurrent optimization
TEST_F(BaoTest, ConcurrentOptimization) {
    auto plans = optimizer.generate_plans("SELECT * FROM users");
    
    std::vector<std::thread> threads = {};

    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([this, &plans]() {
            for (int j = 0; j < 10; ++j) {
                auto selected = optimizer.select_plan("SELECT * FROM users", plans);
                
                QueryResult result;
                result.execution_time_ms = 50.0;
                result.rows_returned = 100;
                result.success = true;
                
                optimizer.update_model(selected, result);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    auto stats = optimizer.get_stats();
    EXPECT_EQ(stats.queries_optimized, 40);
    EXPECT_EQ(stats.model_updates, 40);
}

// Test plan diversity
TEST_F(BaoTest, PlanDiversity) {
    auto plans = optimizer.generate_plans("SELECT * FROM users");
    
    // Check that plans have different operators
    std::set<std::string> unique_operators = {};

    for (const auto& plan : plans) {
        for (const auto& op : plan.operators) {
            unique_operators.insert(op);
        }
    }
    
    EXPECT_GT(unique_operators.size(), 1);
}

// Test extreme execution times
TEST_F(BaoTest, ExtremeExecutionTimes) {
    auto plans = optimizer.generate_plans("SELECT * FROM users");
    auto selected = optimizer.select_plan("SELECT * FROM users", plans);
    
    // Test very fast query
    QueryResult fast_result;
    fast_result.execution_time_ms = 0.1;
    fast_result.rows_returned = 10;
    fast_result.success = true;
    optimizer.update_model(selected, fast_result);
    
    // Test very slow query
    QueryResult slow_result;
    slow_result.execution_time_ms = 10000.0;
    slow_result.rows_returned = 1000000;
    slow_result.success = true;
    optimizer.update_model(selected, slow_result);
    
    auto stats = optimizer.get_stats();
    EXPECT_EQ(stats.model_updates, 2);
}

// Test optimizer state persistence
TEST_F(BaoTest, OptimizerLearning) {
    auto plans = optimizer.generate_plans("SELECT * FROM users");
    
    // Train the optimizer
    for (int i = 0; i < 10; ++i) {
        auto selected = optimizer.select_plan("SELECT * FROM users", plans);
        
        QueryResult result;
        result.execution_time_ms = 25.0;  // Fast
        result.rows_returned = 100;
        result.success = true;
        
        optimizer.update_model(selected, result);
    }
    
    // Check that learning improved average speedup
    auto stats = optimizer.get_stats();
    EXPECT_GT(stats.avg_speedup, 0.5);
}


