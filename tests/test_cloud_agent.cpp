#include <gtest/gtest.h>
#include "sharding/cloud_agent.h"
#include "sharding/shard_topology.h"
#include <memory>
#include <chrono>
#include <thread>

using namespace themis::sharding;

// ============================================================================
// Cloud Agent Tests
// ============================================================================

class CloudAgentTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a mock topology
        ShardTopology::Config topology_config;
        topology_ = std::make_shared<ShardTopology>(topology_config);
        
        // Add some test shards
        ShardInfo shard1;
        shard1.shard_id = "shard_001";
        shard1.primary_endpoint = "localhost:8081";
        shard1.is_healthy = true;
        shard1.capabilities = {"read", "write"};
        topology_->addShard(shard1);
        
        ShardInfo shard2;
        shard2.shard_id = "shard_002";
        shard2.primary_endpoint = "localhost:8082";
        shard2.is_healthy = true;
        shard2.capabilities = {"read", "write"};
        topology_->addShard(shard2);
    }
    
    void TearDown() override {
        topology_.reset();
    }
    
    std::shared_ptr<ShardTopology> topology_;
};

TEST_F(CloudAgentTest, CreateCloudAgent) {
    CloudAgent::Config config;
    config.agent_id = "test_agent";
    config.datacenter = "dc1";
    config.region = "eu-central-1";
    
    // Create agent without executor and metrics (for unit testing)
    CloudAgent agent(topology_, nullptr, nullptr, config);
    
    EXPECT_EQ(agent.getConfig().agent_id, "test_agent");
    EXPECT_EQ(agent.getConfig().datacenter, "dc1");
    EXPECT_EQ(agent.getConfig().region, "eu-central-1");
    EXPECT_FALSE(agent.isRunning());
}

TEST_F(CloudAgentTest, StartAndStop) {
    CloudAgent::Config config;
    config.agent_id = "test_agent";
    config.enable_health_monitoring = false;  // Disable for faster tests
    
    CloudAgent agent(topology_, nullptr, nullptr, config);
    
    EXPECT_FALSE(agent.isRunning());
    
    agent.start();
    EXPECT_TRUE(agent.isRunning());
    
    agent.stop();
    EXPECT_FALSE(agent.isRunning());
}

TEST_F(CloudAgentTest, GetStatistics) {
    CloudAgent::Config config;
    config.agent_id = "test_agent";
    
    CloudAgent agent(topology_, nullptr, nullptr, config);
    
    auto stats = agent.getStatistics();
    EXPECT_EQ(stats.total_operations, 0);
    EXPECT_EQ(stats.completed_operations, 0);
    EXPECT_EQ(stats.failed_operations, 0);
    EXPECT_EQ(stats.pending_operations, 0);
}

TEST_F(CloudAgentTest, GetStatisticsJson) {
    CloudAgent::Config config;
    config.agent_id = "test_agent";
    config.datacenter = "dc1";
    config.region = "eu-central-1";
    
    CloudAgent agent(topology_, nullptr, nullptr, config);
    
    auto stats_json = agent.getStatisticsJson();
    
    EXPECT_EQ(stats_json["agent_id"], "test_agent");
    EXPECT_EQ(stats_json["datacenter"], "dc1");
    EXPECT_EQ(stats_json["region"], "eu-central-1");
    EXPECT_EQ(stats_json["total_operations"], 0);
    EXPECT_FALSE(stats_json["running"].get<bool>());
}

TEST_F(CloudAgentTest, GetHealthStatus) {
    CloudAgent::Config config;
    config.agent_id = "test_agent";
    
    CloudAgent agent(topology_, nullptr, nullptr, config);
    
    auto health = agent.getHealthStatus();
    
    EXPECT_EQ(health["agent_id"], "test_agent");
    EXPECT_EQ(health["status"], "stopped");
    EXPECT_FALSE(health["running"].get<bool>());
    
    // Check shard count
    EXPECT_EQ(health["shards"]["total"], 2);
    EXPECT_EQ(health["shards"]["healthy"], 2);
    EXPECT_EQ(health["shards"]["unhealthy"], 0);
}

TEST_F(CloudAgentTest, GetHealthStatusWhenRunning) {
    CloudAgent::Config config;
    config.agent_id = "test_agent";
    config.enable_health_monitoring = false;
    
    CloudAgent agent(topology_, nullptr, nullptr, config);
    agent.start();
    
    auto health = agent.getHealthStatus();
    
    EXPECT_EQ(health["status"], "healthy");
    EXPECT_TRUE(health["running"].get<bool>());
    
    agent.stop();
}

TEST_F(CloudAgentTest, DelegateWhenNotRunning) {
    CloudAgent::Config config;
    config.agent_id = "test_agent";
    
    CloudAgent agent(topology_, nullptr, nullptr, config);
    
    // Agent is not running
    EXPECT_FALSE(agent.isRunning());
    
    CloudAgentOperation op;
    op.operation_type = "query";
    op.parameters = {{"aql", "FOR doc IN users RETURN doc"}};
    
    auto result = agent.delegate(op);
    
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.status, "failed");
    EXPECT_FALSE(result.error_message.empty());
}

TEST_F(CloudAgentTest, DelegateHealthCheck) {
    CloudAgent::Config config;
    config.agent_id = "test_agent";
    config.enable_health_monitoring = false;
    
    CloudAgent agent(topology_, nullptr, nullptr, config);
    agent.start();
    
    CloudAgentOperation op;
    op.operation_type = "health_check";
    
    auto result = agent.delegate(op);
    
    // Health check should succeed even without executor
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.status, "completed");
    EXPECT_TRUE(result.result.contains("shard_count"));
    EXPECT_EQ(result.result["shard_count"], 2);
    
    agent.stop();
}

TEST_F(CloudAgentTest, DelegateAsync) {
    CloudAgent::Config config;
    config.agent_id = "test_agent";
    config.enable_health_monitoring = false;
    
    CloudAgent agent(topology_, nullptr, nullptr, config);
    agent.start();
    
    CloudAgentOperation op;
    op.operation_type = "health_check";
    
    std::string op_id = agent.delegateAsync(op);
    
    EXPECT_FALSE(op_id.empty());
    EXPECT_TRUE(op_id.starts_with("op_"));  // Starts with "op_"
    
    // Give time for async processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    auto status = agent.getOperationStatus(op_id);
    
    // Should be either completed or pending
    EXPECT_TRUE(status.status == "completed" || status.status == "pending");
    
    agent.stop();
}

TEST_F(CloudAgentTest, CancelOperation) {
    CloudAgent::Config config;
    config.agent_id = "test_agent";
    config.enable_health_monitoring = false;
    
    CloudAgent agent(topology_, nullptr, nullptr, config);
    
    // Don't start the agent so operations stay pending
    
    CloudAgentOperation op;
    op.operation_id = "test_op_123";
    op.operation_type = "query";
    
    // Manually add to pending (simulate async delegation)
    std::string op_id = agent.delegateAsync(op);
    
    // Now start and quickly try to cancel
    agent.start();
    
    // Try to cancel
    auto pending = agent.getPendingOperations();
    if (!pending.empty()) {
        bool cancelled = agent.cancelOperation(pending[0]);
        if (cancelled) {
            auto status = agent.getOperationStatus(pending[0]);
            EXPECT_EQ(status.status, "cancelled");
        }
    }
    
    agent.stop();
}

TEST_F(CloudAgentTest, GetPendingOperations) {
    CloudAgent::Config config;
    config.agent_id = "test_agent";
    config.enable_health_monitoring = false;
    
    CloudAgent agent(topology_, nullptr, nullptr, config);
    
    // Initially no pending operations
    auto pending = agent.getPendingOperations();
    EXPECT_TRUE(pending.empty());
}

TEST_F(CloudAgentTest, GetOperationStatusNotFound) {
    CloudAgent::Config config;
    config.agent_id = "test_agent";
    
    CloudAgent agent(topology_, nullptr, nullptr, config);
    
    auto status = agent.getOperationStatus("non_existent_op");
    
    EXPECT_FALSE(status.success);
    EXPECT_EQ(status.status, "not_found");
    EXPECT_FALSE(status.error_message.empty());
}

TEST_F(CloudAgentTest, UpdateConfig) {
    CloudAgent::Config config;
    config.agent_id = "test_agent";
    config.datacenter = "dc1";
    
    CloudAgent agent(topology_, nullptr, nullptr, config);
    
    EXPECT_EQ(agent.getConfig().datacenter, "dc1");
    
    CloudAgent::Config new_config = config;
    new_config.datacenter = "dc2";
    
    agent.updateConfig(new_config);
    
    EXPECT_EQ(agent.getConfig().datacenter, "dc2");
}

TEST_F(CloudAgentTest, ExecuteHealthCheck) {
    CloudAgent::Config config;
    config.agent_id = "test_agent";
    
    CloudAgent agent(topology_, nullptr, nullptr, config);
    
    auto health = agent.executeHealthCheck();
    
    EXPECT_TRUE(health.contains("timestamp"));
    EXPECT_TRUE(health.contains("shard_count"));
    EXPECT_EQ(health["shard_count"], 2);
    EXPECT_TRUE(health.contains("shards"));
    EXPECT_EQ(health["shards"].size(), 2);
}

TEST_F(CloudAgentTest, AutoGenerateAgentId) {
    CloudAgent::Config config;
    // Don't set agent_id - should be auto-generated
    
    CloudAgent agent(topology_, nullptr, nullptr, config);
    
    // Agent ID should be auto-generated
    EXPECT_FALSE(agent.getConfig().agent_id.empty());
    EXPECT_TRUE(agent.getConfig().agent_id.starts_with("cloud_agent_"));
}

TEST_F(CloudAgentTest, MultipleStartStop) {
    CloudAgent::Config config;
    config.agent_id = "test_agent";
    config.enable_health_monitoring = false;
    
    CloudAgent agent(topology_, nullptr, nullptr, config);
    
    // Start multiple times should be safe
    agent.start();
    EXPECT_TRUE(agent.isRunning());
    
    agent.start();  // Should be no-op
    EXPECT_TRUE(agent.isRunning());
    
    // Stop multiple times should be safe
    agent.stop();
    EXPECT_FALSE(agent.isRunning());
    
    agent.stop();  // Should be no-op
    EXPECT_FALSE(agent.isRunning());
}

TEST_F(CloudAgentTest, StatisticsAfterOperations) {
    CloudAgent::Config config;
    config.agent_id = "test_agent";
    config.enable_health_monitoring = false;
    
    CloudAgent agent(topology_, nullptr, nullptr, config);
    agent.start();
    
    // Execute a health check operation
    CloudAgentOperation op;
    op.operation_type = "health_check";
    agent.delegate(op);
    
    auto stats = agent.getStatistics();
    EXPECT_GE(stats.total_operations, 1);
    EXPECT_GE(stats.completed_operations, 1);
    
    agent.stop();
}
