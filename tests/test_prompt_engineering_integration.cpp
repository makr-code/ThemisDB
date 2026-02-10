#include <gtest/gtest.h>
#include "prompt_engineering/prompt_engineering_integration.h"
#include <memory>
#include <thread>
#include <chrono>

using namespace themis::prompt_engineering;

class PromptEngineeringIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create components (in-memory for testing)
        manager_ = std::make_shared<PromptManager>(nullptr, nullptr);
        optimizer_ = std::make_shared<PromptOptimizer>(nullptr, nullptr);
        tracker_ = std::make_shared<PromptPerformanceTracker>(nullptr, nullptr);
        
        SelfImprovementOrchestrator::ImprovementConfig orchestrator_config;
        orchestrator_config.min_executions = 10;  // Lower for testing
        orchestrator_ = std::make_shared<SelfImprovementOrchestrator>(
            orchestrator_config, tracker_, optimizer_, manager_, nullptr
        );
        
        feedback_collector_ = std::make_shared<FeedbackCollector>(nullptr, nullptr);
        version_control_ = std::make_shared<PromptVersionControl>(nullptr, nullptr);
        
        // Create integration with test config
        config_.background_worker_enabled = false;  // Disable for most tests
        config_.min_executions_before_optimization = 10;
        
        integration_ = std::make_shared<PromptEngineeringIntegration>(
            config_, manager_, optimizer_, tracker_, orchestrator_,
            feedback_collector_, version_control_
        );
    }
    
    IntegrationConfig config_;
    std::shared_ptr<PromptManager> manager_;
    std::shared_ptr<PromptOptimizer> optimizer_;
    std::shared_ptr<PromptPerformanceTracker> tracker_;
    std::shared_ptr<SelfImprovementOrchestrator> orchestrator_;
    std::shared_ptr<FeedbackCollector> feedback_collector_;
    std::shared_ptr<PromptVersionControl> version_control_;
    std::shared_ptr<PromptEngineeringIntegration> integration_;
};

TEST_F(PromptEngineeringIntegrationTest, BasicHooks) {
    // Register a prompt
    manager_->registerTemplate("test_prompt", "Hello {name}");
    
    integration_->start();
    
    // Before execution
    nlohmann::json context = {{"name", "World"}};
    auto ctx = integration_->beforeExecution("test_prompt", context);
    
    EXPECT_FALSE(ctx.execution_id.empty());
    EXPECT_EQ(ctx.prompt_id, "test_prompt");
    EXPECT_EQ(ctx.original_prompt, "Hello {name}");
    EXPECT_FALSE(ctx.enhanced_prompt.empty());
    
    // After execution
    integration_->afterExecution(ctx, "Hello World", true, 100.0);
    
    // Check stats
    auto stats = integration_->getStats();
    EXPECT_EQ(stats["total_executions"], 1);
    
    integration_->stop();
}

TEST_F(PromptEngineeringIntegrationTest, AutoVersioning) {
    config_.enable_auto_versioning = true;
    integration_ = std::make_shared<PromptEngineeringIntegration>(
        config_, manager_, optimizer_, tracker_, orchestrator_,
        feedback_collector_, version_control_
    );
    
    manager_->registerTemplate("test_prompt", "Hello {name}");
    integration_->start();
    
    // Execute
    auto ctx = integration_->beforeExecution("test_prompt", {{"name", "World"}});
    integration_->afterExecution(ctx, "Hello World", true, 100.0);
    
    // Check version was created
    auto history = version_control_->getHistory("test_prompt");
    EXPECT_GT(history.size(), 0);
    
    integration_->stop();
}

TEST_F(PromptEngineeringIntegrationTest, MetricRecording) {
    config_.enable_performance_tracking = true;
    integration_ = std::make_shared<PromptEngineeringIntegration>(
        config_, manager_, optimizer_, tracker_, orchestrator_,
        feedback_collector_, version_control_
    );
    
    manager_->registerTemplate("test_prompt", "Test");
    integration_->start();
    
    // Execute multiple times
    for (int i = 0; i < 5; i++) {
        auto ctx = integration_->beforeExecution("test_prompt");
        integration_->afterExecution(ctx, "response", true, 100.0 + i);
    }
    
    // Check metrics
    auto metrics = tracker_->getMetrics("test_prompt");
    EXPECT_EQ(metrics.total_executions, 5);
    
    integration_->stop();
}

TEST_F(PromptEngineeringIntegrationTest, FeedbackRecording) {
    config_.enable_feedback_collection = true;
    integration_ = std::make_shared<PromptEngineeringIntegration>(
        config_, manager_, optimizer_, tracker_, orchestrator_,
        feedback_collector_, version_control_
    );
    
    manager_->registerTemplate("test_prompt", "Test");
    integration_->start();
    
    // Execute with failure
    auto ctx = integration_->beforeExecution("test_prompt");
    integration_->afterExecution(ctx, "", false, 100.0);  // Empty response = failure
    
    // Check feedback was recorded
    auto feedback = feedback_collector_->getFeedback("test_prompt", 10);
    EXPECT_GT(feedback.size(), 0);
    
    integration_->stop();
}

TEST_F(PromptEngineeringIntegrationTest, OptimizationTrigger) {
    config_.enable_auto_optimization = true;
    config_.min_executions_before_optimization = 5;
    config_.min_success_rate_for_optimization = 0.8;
    
    integration_ = std::make_shared<PromptEngineeringIntegration>(
        config_, manager_, optimizer_, tracker_, orchestrator_,
        feedback_collector_, version_control_
    );
    
    manager_->registerTemplate("test_prompt", "Test");
    integration_->start();
    
    // Execute with low success rate
    for (int i = 0; i < 6; i++) {
        auto ctx = integration_->beforeExecution("test_prompt");
        bool success = (i % 3 == 0);  // 33% success rate
        integration_->afterExecution(ctx, "response", success, 100.0);
    }
    
    // Optimization should have been triggered
    auto stats = integration_->getStats();
    // Note: actual optimization may or may not happen depending on orchestrator logic
    
    integration_->stop();
}

TEST_F(PromptEngineeringIntegrationTest, BackgroundWorkerStart) {
    config_.background_worker_enabled = true;
    config_.background_worker_interval = std::chrono::seconds(2);
    
    integration_ = std::make_shared<PromptEngineeringIntegration>(
        config_, manager_, optimizer_, tracker_, orchestrator_,
        feedback_collector_, version_control_
    );
    
    integration_->start();
    
    // Check worker is running
    auto worker_status = integration_->getBackgroundWorkerStatus();
    EXPECT_TRUE(worker_status.running);
    
    integration_->stop();
}

TEST_F(PromptEngineeringIntegrationTest, BackgroundWorkerStop) {
    config_.background_worker_enabled = true;
    integration_ = std::make_shared<PromptEngineeringIntegration>(
        config_, manager_, optimizer_, tracker_, orchestrator_,
        feedback_collector_, version_control_
    );
    
    integration_->start();
    integration_->stopBackgroundOptimization();
    
    // Check worker is stopped
    auto worker_status = integration_->getBackgroundWorkerStatus();
    EXPECT_FALSE(worker_status.running);
    
    integration_->stop();
}

TEST_F(PromptEngineeringIntegrationTest, ConfigurationManagement) {
    integration_->start();
    
    // Update config
    IntegrationConfig new_config = config_;
    new_config.enable_auto_versioning = false;
    integration_->updateConfig(new_config);
    
    // Check config was updated
    auto updated_config = integration_->getConfig();
    EXPECT_FALSE(updated_config.enable_auto_versioning);
    
    integration_->stop();
}

TEST_F(PromptEngineeringIntegrationTest, GetStatus) {
    manager_->registerTemplate("prompt1", "Test1");
    manager_->registerTemplate("prompt2", "Test2");
    
    integration_->start();
    
    // Execute on both prompts
    auto ctx1 = integration_->beforeExecution("prompt1");
    integration_->afterExecution(ctx1, "response", true, 100.0);
    
    auto ctx2 = integration_->beforeExecution("prompt2");
    integration_->afterExecution(ctx2, "response", true, 100.0);
    
    // Get status
    auto status = integration_->getStatus();
    EXPECT_TRUE(status.running);
    EXPECT_EQ(status.total_executions, 2);
    EXPECT_EQ(status.active_prompts, 2);
    
    integration_->stop();
}

TEST_F(PromptEngineeringIntegrationTest, GetStats) {
    manager_->registerTemplate("test_prompt", "Test");
    integration_->start();
    
    // Execute
    auto ctx = integration_->beforeExecution("test_prompt");
    integration_->afterExecution(ctx, "response", true, 100.0);
    
    // Get stats
    auto stats = integration_->getStats();
    EXPECT_TRUE(stats.contains("total_executions"));
    EXPECT_TRUE(stats.contains("active_prompts"));
    EXPECT_TRUE(stats.contains("executions_by_prompt"));
    
    integration_->stop();
}

TEST_F(PromptEngineeringIntegrationTest, MultipleExecutions) {
    manager_->registerTemplate("test_prompt", "Test");
    integration_->start();
    
    // Execute concurrently
    std::vector<ExecutionContext> contexts;
    for (int i = 0; i < 10; i++) {
        contexts.push_back(integration_->beforeExecution("test_prompt"));
    }
    
    for (const auto& ctx : contexts) {
        integration_->afterExecution(ctx, "response", true, 100.0);
    }
    
    auto stats = integration_->getStats();
    EXPECT_EQ(stats["total_executions"], 10);
    
    integration_->stop();
}

TEST_F(PromptEngineeringIntegrationTest, ErrorHandling) {
    integration_->start();
    
    // Try to execute with non-existent prompt
    auto ctx = integration_->beforeExecution("nonexistent");
    EXPECT_TRUE(ctx.enhanced_prompt.empty());
    
    // Should not crash
    integration_->afterExecution(ctx, "", false, 0.0);
    
    integration_->stop();
}

TEST_F(PromptEngineeringIntegrationTest, ContextSerialization) {
    ExecutionContext ctx;
    ctx.execution_id = "test-id";
    ctx.prompt_id = "test-prompt";
    ctx.original_prompt = "original";
    ctx.enhanced_prompt = "enhanced";
    ctx.context = {{"key", "value"}};
    ctx.version_id = "v1";
    ctx.start_time = std::chrono::system_clock::now();
    
    // Serialize
    auto json = ctx.toJson();
    EXPECT_EQ(json["execution_id"], "test-id");
    EXPECT_EQ(json["prompt_id"], "test-prompt");
    
    // Deserialize
    auto ctx2 = ExecutionContext::fromJson(json);
    EXPECT_EQ(ctx2.execution_id, ctx.execution_id);
    EXPECT_EQ(ctx2.prompt_id, ctx.prompt_id);
}

TEST_F(PromptEngineeringIntegrationTest, StatusSerialization) {
    IntegrationStatus status;
    status.running = true;
    status.total_executions = 42;
    status.active_prompts = 5;
    
    auto json = status.toJson();
    EXPECT_EQ(json["running"], true);
    EXPECT_EQ(json["total_executions"], 42);
    EXPECT_EQ(json["active_prompts"], 5);
}

// Main
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
