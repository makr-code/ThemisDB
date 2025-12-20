/**
 * Unit tests for Multi-Agent LLM Reasoning framework
 * Tests for v1.4.0 implementation
 */

#include <gtest/gtest.h>
#include "llm/multi_agent_orchestrator.h"
#include "llm/llm_agent.h"
#include "llm/agent_role_registry.h"
#include "llm/consensus_builder.h"
#include "llm/lora_registry.h"
#include <rocksdb/db.h>
#include <rocksdb/transaction_db.h>
#include <rocksdb/options.h>
#include <filesystem>

using namespace themis::llm;

class MultiAgentTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create temporary database for testing
        test_db_path_ = std::filesystem::temp_directory_path() / "test_multi_agent_db";
        std::filesystem::remove_all(test_db_path_);
        
        rocksdb::Options options;
        options.create_if_missing = true;
        rocksdb::TransactionDBOptions tx_options;
        
        rocksdb::TransactionDB* db_ptr;
        auto status = rocksdb::TransactionDB::Open(options, tx_options, test_db_path_.string(), &db_ptr);
        ASSERT_TRUE(status.ok()) << "Failed to open test database";
        db_.reset(db_ptr);
        
        // Create components
        role_registry_ = std::make_shared<AgentRoleRegistry>(db_.get());
        consensus_builder_ = std::make_shared<ConsensusBuilder>();
        orchestrator_ = std::make_shared<MultiAgentOrchestrator>(
            db_.get(), role_registry_, consensus_builder_
        );
        lora_registry_ = std::make_shared<LoRARegistry>(db_.get());
        
        // Load default roles
        role_registry_->loadDefaultRoles();
    }
    
    void TearDown() override {
        orchestrator_.reset();
        role_registry_.reset();
        consensus_builder_.reset();
        lora_registry_.reset();
        db_.reset();
        std::filesystem::remove_all(test_db_path_);
    }
    
    std::filesystem::path test_db_path_;
    std::unique_ptr<rocksdb::TransactionDB> db_;
    std::shared_ptr<AgentRoleRegistry> role_registry_;
    std::shared_ptr<ConsensusBuilder> consensus_builder_;
    std::shared_ptr<MultiAgentOrchestrator> orchestrator_;
    std::shared_ptr<LoRARegistry> lora_registry_;
};

// Test AgentRoleRegistry
TEST_F(MultiAgentTest, RoleRegistry_LoadDefaultRoles) {
    auto roles = role_registry_->listAllRoles();
    EXPECT_GE(roles.size(), 5); // At least 5 built-in roles
    
    // Check specific roles exist
    auto legal_role = role_registry_->getRole("legal_expert");
    ASSERT_TRUE(legal_role.has_value());
    EXPECT_EQ(legal_role->role_name, "Legal Expert");
    EXPECT_FALSE(legal_role->capabilities.empty());
}

TEST_F(MultiAgentTest, RoleRegistry_RegisterCustomRole) {
    AgentRoleRegistry::RoleDefinition custom_role;
    custom_role.role_id = "custom_tester";
    custom_role.role_name = "Custom Test Role";
    custom_role.description = "A custom role for testing";
    custom_role.capabilities = {"testing", "validation"};
    
    EXPECT_TRUE(role_registry_->registerRole(custom_role));
    
    auto retrieved = role_registry_->getRole("custom_tester");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->role_name, "Custom Test Role");
}

TEST_F(MultiAgentTest, RoleRegistry_FindRolesByCapability) {
    auto roles = role_registry_->getRolesByCapability("contract_analysis");
    EXPECT_GT(roles.size(), 0);
}

// Test LLMAgent
TEST_F(MultiAgentTest, LLMAgent_CreateAndProcess) {
    LLMAgent::AgentConfig config;
    config.agent_id = "test_agent_1";
    config.role = "legal_expert";
    config.base_model = "mistral-7b";
    config.max_context_length = 4096;
    config.temperature = 0.7f;
    
    LLMAgent agent(config, db_.get());
    
    LLMAgent::AgentRequest request;
    request.prompt = "Analyze this contract for risks.";
    
    auto result = agent.processRequest(request);
    
    EXPECT_FALSE(result.response.empty());
    EXPECT_GT(result.confidence, 0.0f);
    EXPECT_LE(result.confidence, 1.0f);
}

TEST_F(MultiAgentTest, LLMAgent_Statistics) {
    LLMAgent::AgentConfig config;
    config.agent_id = "test_agent_2";
    config.role = "technical_analyst";
    
    LLMAgent agent(config, db_.get());
    
    // Process multiple requests
    for (int i = 0; i < 3; i++) {
        LLMAgent::AgentRequest request;
        request.prompt = "Test prompt " + std::to_string(i);
        agent.processRequest(request);
    }
    
    auto stats = agent.getStats();
    EXPECT_EQ(stats["total_requests"].get<size_t>(), 3);
    EXPECT_GT(stats["total_tokens"].get<int64_t>(), 0);
}

// Test ConsensusBuilder
TEST_F(MultiAgentTest, ConsensusBuilder_MajorityVote) {
    std::vector<MultiAgentOrchestrator::AgentResponse> responses;
    
    responses.push_back({
        "agent1", "legal_expert", "Option A is recommended", {}, 0.9f, {}, 0, 0
    });
    responses.push_back({
        "agent2", "legal_expert", "Option A is recommended", {}, 0.8f, {}, 0, 0
    });
    responses.push_back({
        "agent3", "technical_analyst", "Option B is better", {}, 0.7f, {}, 0, 0
    });
    
    ConsensusBuilder::ConsensusConfig config;
    config.strategy = ConsensusBuilder::StrategyType::MAJORITY_VOTE;
    config.confidence_threshold = 0.6f;
    
    auto result = consensus_builder_->buildConsensus(responses, config);
    
    EXPECT_FALSE(result.final_response.empty());
    EXPECT_GT(result.consensus_score, 0.0f);
}

TEST_F(MultiAgentTest, ConsensusBuilder_WeightedAverage) {
    std::vector<MultiAgentOrchestrator::AgentResponse> responses;
    
    responses.push_back({
        "agent1", "security_analyst", "High risk detected", {}, 0.95f, {}, 0, 0
    });
    responses.push_back({
        "agent2", "performance_analyst", "Performance OK", {}, 0.7f, {}, 0, 0
    });
    
    ConsensusBuilder::ConsensusConfig config;
    config.strategy = ConsensusBuilder::StrategyType::WEIGHTED_AVERAGE;
    config.role_weights = {
        {"security_analyst", 0.7f},
        {"performance_analyst", 0.3f}
    };
    
    auto result = consensus_builder_->buildConsensus(responses, config);
    
    EXPECT_FALSE(result.final_response.empty());
    EXPECT_EQ(result.agent_contributions.size(), 2);
}

TEST_F(MultiAgentTest, ConsensusBuilder_BestResponse) {
    std::vector<MultiAgentOrchestrator::AgentResponse> responses;
    
    responses.push_back({
        "agent1", "legal_expert", "Response 1", {}, 0.6f, {}, 0, 0
    });
    responses.push_back({
        "agent2", "technical_analyst", "Response 2", {}, 0.9f, {}, 0, 0
    });
    responses.push_back({
        "agent3", "business_strategist", "Response 3", {}, 0.7f, {}, 0, 0
    });
    
    ConsensusBuilder::ConsensusConfig config;
    config.strategy = ConsensusBuilder::StrategyType::BEST_RESPONSE;
    
    auto result = consensus_builder_->buildConsensus(responses, config);
    
    // Should select Response 2 (highest confidence 0.9)
    EXPECT_NE(result.final_response.find("Response 2"), std::string::npos);
}

// Test MultiAgentOrchestrator
TEST_F(MultiAgentTest, Orchestrator_TaskDecomposition) {
    std::string complex_prompt = "Analyze this contract from legal and technical perspectives";
    
    auto tasks = orchestrator_->decomposeProblem(complex_prompt);
    
    EXPECT_GE(tasks.size(), 2); // Should identify at least legal and technical
    
    bool has_legal = false;
    bool has_technical = false;
    
    for (const auto& task : tasks) {
        if (!task.required_roles.empty()) {
            if (task.required_roles[0] == "legal_expert") has_legal = true;
            if (task.required_roles[0] == "technical_analyst") has_technical = true;
        }
    }
    
    EXPECT_TRUE(has_legal);
    EXPECT_TRUE(has_technical);
}

TEST_F(MultiAgentTest, Orchestrator_RegisterAndGetAgent) {
    LLMAgent::AgentConfig config;
    config.agent_id = "test_agent_orch";
    config.role = "legal_expert";
    
    auto agent = std::make_shared<LLMAgent>(config, db_.get());
    orchestrator_->registerAgent(agent);
    
    auto retrieved = orchestrator_->getAgent("test_agent_orch");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getId(), "test_agent_orch");
    
    auto agents_by_role = orchestrator_->getAgentsByRole("legal_expert");
    EXPECT_GE(agents_by_role.size(), 1);
}

TEST_F(MultiAgentTest, Orchestrator_ProcessTasks) {
    // Register test agents
    LLMAgent::AgentConfig config1;
    config1.agent_id = "legal_agent_test";
    config1.role = "legal_expert";
    auto agent1 = std::make_shared<LLMAgent>(config1, db_.get());
    orchestrator_->registerAgent(agent1);
    
    LLMAgent::AgentConfig config2;
    config2.agent_id = "tech_agent_test";
    config2.role = "technical_analyst";
    auto agent2 = std::make_shared<LLMAgent>(config2, db_.get());
    orchestrator_->registerAgent(agent2);
    
    // Create tasks
    std::vector<MultiAgentOrchestrator::Task> tasks;
    tasks.push_back({
        "task1",
        "Legal analysis needed",
        MultiAgentOrchestrator::TaskType::PARALLEL,
        {"legal_expert"},
        {}
    });
    tasks.push_back({
        "task2",
        "Technical analysis needed",
        MultiAgentOrchestrator::TaskType::PARALLEL,
        {"technical_analyst"},
        {}
    });
    
    auto result = orchestrator_->processTasks(tasks);
    
    EXPECT_FALSE(result.session_id.empty());
    EXPECT_GE(result.agent_responses.size(), 2);
    EXPECT_FALSE(result.synthesized_result.empty());
    EXPECT_GT(result.overall_confidence, 0.0f);
}

// Test LoRARegistry
TEST_F(MultiAgentTest, LoRARegistry_RegisterAdapter) {
    LoRARegistry::LoRAAdapter adapter;
    adapter.adapter_id = "test_lora_1";
    adapter.name = "Test LoRA Adapter";
    adapter.base_model = "mistral-7b";
    adapter.domain = "testing";
    adapter.rank = 8;
    adapter.alpha = 16.0f;
    
    EXPECT_TRUE(lora_registry_->registerAdapter(adapter));
    
    auto retrieved = lora_registry_->getAdapter("test_lora_1");
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->name, "Test LoRA Adapter");
}

TEST_F(MultiAgentTest, LoRARegistry_LoadUnload) {
    LoRARegistry::LoRAAdapter adapter;
    adapter.adapter_id = "test_lora_2";
    adapter.name = "Test LoRA 2";
    adapter.base_model = "llama-3-8b";
    adapter.domain = "legal";
    
    lora_registry_->registerAdapter(adapter);
    
    EXPECT_TRUE(lora_registry_->loadAdapter("test_lora_2"));
    EXPECT_TRUE(lora_registry_->isLoaded("test_lora_2"));
    
    auto loaded = lora_registry_->getLoadedAdapters();
    EXPECT_EQ(loaded.size(), 1);
    
    EXPECT_TRUE(lora_registry_->unloadAdapter("test_lora_2"));
    EXPECT_FALSE(lora_registry_->isLoaded("test_lora_2"));
}

TEST_F(MultiAgentTest, LoRARegistry_Statistics) {
    // Register multiple adapters
    for (int i = 0; i < 3; i++) {
        LoRARegistry::LoRAAdapter adapter;
        adapter.adapter_id = "lora_" + std::to_string(i);
        adapter.name = "LoRA " + std::to_string(i);
        adapter.base_model = "mistral-7b";
        adapter.size_bytes = 1024 * 1024; // 1MB
        lora_registry_->registerAdapter(adapter);
    }
    
    auto stats = lora_registry_->getStats();
    EXPECT_EQ(stats.total_adapters, 3);
}

// Integration test
TEST_F(MultiAgentTest, Integration_EndToEnd) {
    // Setup agents
    LLMAgent::AgentConfig config1;
    config1.agent_id = "legal_agent_e2e";
    config1.role = "legal_expert";
    config1.lora_adapter_id = "legal_lora";
    auto agent1 = std::make_shared<LLMAgent>(config1, db_.get());
    
    LLMAgent::AgentConfig config2;
    config2.agent_id = "tech_agent_e2e";
    config2.role = "technical_analyst";
    config2.lora_adapter_id = "tech_lora";
    auto agent2 = std::make_shared<LLMAgent>(config2, db_.get());
    
    orchestrator_->registerAgent(agent1);
    orchestrator_->registerAgent(agent2);
    
    // Register LoRA adapters
    LoRARegistry::LoRAAdapter lora1;
    lora1.adapter_id = "legal_lora";
    lora1.name = "Legal LoRA";
    lora1.domain = "legal";
    lora_registry_->registerAdapter(lora1);
    
    LoRARegistry::LoRAAdapter lora2;
    lora2.adapter_id = "tech_lora";
    lora2.name = "Tech LoRA";
    lora2.domain = "technical";
    lora_registry_->registerAdapter(lora2);
    
    // Process complex query
    std::string complex_query = "Analyze this SaaS contract for legal risks and technical feasibility";
    auto tasks = orchestrator_->decomposeProblem(complex_query);
    auto result = orchestrator_->processTasks(tasks);
    
    EXPECT_FALSE(result.session_id.empty());
    EXPECT_GT(result.agent_responses.size(), 0);
    EXPECT_FALSE(result.synthesized_result.empty());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
