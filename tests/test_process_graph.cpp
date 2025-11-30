#include <gtest/gtest.h>
#include "index/edge_types.h"
#include "index/process_graph.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
#include <filesystem>

namespace fs = std::filesystem;

// ============================================================================
// Edge Type Registry Tests
// ============================================================================

class EdgeTypeRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Registry is a singleton, ensure it's initialized
        themis::EdgeTypeRegistry::instance();
    }
};

TEST_F(EdgeTypeRegistryTest, BuiltinTypesRegistered) {
    auto& registry = themis::EdgeTypeRegistry::instance();
    
    // Check STRUCTURAL types
    EXPECT_TRUE(registry.isRegistered("PARENT_OF"));
    EXPECT_TRUE(registry.isRegistered("CHILD_OF"));
    EXPECT_TRUE(registry.isRegistered("CONTAINS"));
    EXPECT_TRUE(registry.isRegistered("PART_OF"));
    
    // Check REFERENCE types
    EXPECT_TRUE(registry.isRegistered("REFERENCES"));
    EXPECT_TRUE(registry.isRegistered("LINKS_TO"));
    EXPECT_TRUE(registry.isRegistered("CITES"));
    
    // Check TEMPORAL types
    EXPECT_TRUE(registry.isRegistered("VALID_DURING"));
    EXPECT_TRUE(registry.isRegistered("PRECEDED_BY"));
    EXPECT_TRUE(registry.isRegistered("SUCCEEDED_BY"));
    
    // Check SEMANTIC types
    EXPECT_TRUE(registry.isRegistered("IS_A"));
    EXPECT_TRUE(registry.isRegistered("SIMILAR_TO"));
    EXPECT_TRUE(registry.isRegistered("RELATED_TO"));
    
    // Check WORKFLOW types
    EXPECT_TRUE(registry.isRegistered("TRIGGERS"));
    EXPECT_TRUE(registry.isRegistered("DEPENDS_ON"));
    EXPECT_TRUE(registry.isRegistered("FOLLOWS"));
    
    // Check ACCESS types
    EXPECT_TRUE(registry.isRegistered("CAN_READ"));
    EXPECT_TRUE(registry.isRegistered("CAN_WRITE"));
    EXPECT_TRUE(registry.isRegistered("OWNS"));
}

TEST_F(EdgeTypeRegistryTest, GetTypeInfo) {
    auto& registry = themis::EdgeTypeRegistry::instance();
    
    auto info = registry.getTypeInfo("PARENT_OF");
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->type_name, "PARENT_OF");
    EXPECT_EQ(info->category, themis::EdgeCategory::STRUCTURAL);
    EXPECT_TRUE(info->is_bidirectional);
    EXPECT_FALSE(info->requires_temporal);
    EXPECT_EQ(info->inverse_type.value_or(""), "CHILD_OF");
}

TEST_F(EdgeTypeRegistryTest, GetInverseType) {
    auto& registry = themis::EdgeTypeRegistry::instance();
    
    auto inverse = registry.getInverseType("PARENT_OF");
    ASSERT_TRUE(inverse.has_value());
    EXPECT_EQ(*inverse, "CHILD_OF");
    
    // Check bidirectional inverse
    auto inverseBack = registry.getInverseType("CHILD_OF");
    ASSERT_TRUE(inverseBack.has_value());
    EXPECT_EQ(*inverseBack, "PARENT_OF");
    
    // Check self-inverse (SIMILAR_TO is its own inverse)
    auto selfInverse = registry.getInverseType("SIMILAR_TO");
    ASSERT_TRUE(selfInverse.has_value());
    EXPECT_EQ(*selfInverse, "SIMILAR_TO");
}

TEST_F(EdgeTypeRegistryTest, GetTypesByCategory) {
    auto& registry = themis::EdgeTypeRegistry::instance();
    
    auto structuralTypes = registry.getTypesByCategory(themis::EdgeCategory::STRUCTURAL);
    EXPECT_GE(structuralTypes.size(), 4u);
    EXPECT_TRUE(std::find(structuralTypes.begin(), structuralTypes.end(), "PARENT_OF") != structuralTypes.end());
    EXPECT_TRUE(std::find(structuralTypes.begin(), structuralTypes.end(), "CHILD_OF") != structuralTypes.end());
    
    auto temporalTypes = registry.getTypesByCategory(themis::EdgeCategory::TEMPORAL);
    EXPECT_GE(temporalTypes.size(), 4u);
    for (const auto& type : temporalTypes) {
        auto info = registry.getTypeInfo(type);
        ASSERT_TRUE(info.has_value());
        EXPECT_TRUE(info->requires_temporal) << "Type " << type << " should require temporal";
    }
}

TEST_F(EdgeTypeRegistryTest, RegisterCustomType) {
    auto& registry = themis::EdgeTypeRegistry::instance();
    
    themis::EdgeTypeInfo customType{
        .type_name = "CUSTOM_TEST_TYPE",
        .category = themis::EdgeCategory::CUSTOM,
        .description = "A custom test type",
        .is_bidirectional = false,
        .requires_temporal = false,
        .is_weighted = true,
        .inverse_type = std::nullopt
    };
    
    auto st = registry.registerType(customType);
    ASSERT_TRUE(st.ok) << st.message;
    
    EXPECT_TRUE(registry.isRegistered("CUSTOM_TEST_TYPE"));
    
    auto info = registry.getTypeInfo("CUSTOM_TEST_TYPE");
    ASSERT_TRUE(info.has_value());
    EXPECT_EQ(info->category, themis::EdgeCategory::CUSTOM);
    EXPECT_TRUE(info->is_weighted);
}

TEST_F(EdgeTypeRegistryTest, CategoryConversion) {
    // Test categoryToString
    EXPECT_EQ(themis::EdgeTypeRegistry::categoryToString(themis::EdgeCategory::STRUCTURAL), "STRUCTURAL");
    EXPECT_EQ(themis::EdgeTypeRegistry::categoryToString(themis::EdgeCategory::TEMPORAL), "TEMPORAL");
    EXPECT_EQ(themis::EdgeTypeRegistry::categoryToString(themis::EdgeCategory::WORKFLOW), "WORKFLOW");
    
    // Test categoryFromString
    EXPECT_EQ(themis::EdgeTypeRegistry::categoryFromString("STRUCTURAL"), themis::EdgeCategory::STRUCTURAL);
    EXPECT_EQ(themis::EdgeTypeRegistry::categoryFromString("TEMPORAL"), themis::EdgeCategory::TEMPORAL);
    EXPECT_EQ(themis::EdgeTypeRegistry::categoryFromString("INVALID"), std::nullopt);
}

TEST_F(EdgeTypeRegistryTest, HelperFunctions) {
    // Test requiresTemporalValidity
    EXPECT_TRUE(themis::requiresTemporalValidity("VALID_DURING"));
    EXPECT_TRUE(themis::requiresTemporalValidity("PRECEDED_BY"));
    EXPECT_FALSE(themis::requiresTemporalValidity("PARENT_OF"));
    EXPECT_FALSE(themis::requiresTemporalValidity("REFERENCES"));
    
    // Test isWeightedEdgeType
    EXPECT_TRUE(themis::isWeightedEdgeType("SIMILAR_TO"));
    EXPECT_TRUE(themis::isWeightedEdgeType("RELATED_TO"));
    EXPECT_TRUE(themis::isWeightedEdgeType("DEPENDS_ON"));
    EXPECT_FALSE(themis::isWeightedEdgeType("PARENT_OF"));
    
    // Test isBidirectionalEdgeType
    EXPECT_TRUE(themis::isBidirectionalEdgeType("PARENT_OF"));
    EXPECT_TRUE(themis::isBidirectionalEdgeType("SIMILAR_TO"));
    EXPECT_FALSE(themis::isBidirectionalEdgeType("REFERENCES"));
    EXPECT_FALSE(themis::isBidirectionalEdgeType("TRIGGERS"));
}

// ============================================================================
// Process Graph Tests
// ============================================================================

class ProcessGraphTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "./data/themis_process_graph_test";
        fs::remove_all(test_db_path_);
        
        themis::RocksDBWrapper::Config config;
        config.db_path = test_db_path_;
        config.memtable_size_mb = 64;
        config.block_cache_size_mb = 256;
        config.max_background_jobs = 2;
        config.compression_default = "lz4";
        config.compression_bottommost = "zstd";
        
        db_ = std::make_unique<themis::RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());
        pgm_ = std::make_unique<themis::ProcessGraphManager>(*db_);
        
        // Register process edge types
        themis::registerProcessEdgeTypes();
    }

    void TearDown() override {
        pgm_.reset();
        db_.reset();
        fs::remove_all(test_db_path_);
    }

    std::string test_db_path_;
    std::unique_ptr<themis::RocksDBWrapper> db_;
    std::unique_ptr<themis::ProcessGraphManager> pgm_;
};

TEST_F(ProcessGraphTest, RegisterProcess) {
    auto st = pgm_->registerProcess("order-process", "Order Processing");
    ASSERT_TRUE(st.ok) << st.message;
}

TEST_F(ProcessGraphTest, AddBPMNNodes) {
    pgm_->registerProcess("simple-bpmn", "Simple BPMN Process");
    
    // Add start event
    themis::ProcessNodeInfo startEvent{
        .node_id = "start",
        .name = "Start",
        .description = "Process start",
        .node_type = themis::BPMNNodeType::START_EVENT
    };
    auto st1 = pgm_->addProcessNode("simple-bpmn", startEvent);
    ASSERT_TRUE(st1.ok) << st1.message;
    
    // Add task
    themis::ProcessNodeInfo task{
        .node_id = "review-task",
        .name = "Review Order",
        .description = "Review the incoming order",
        .node_type = themis::BPMNNodeType::TASK,
        .subtype = "USER_TASK"
    };
    auto st2 = pgm_->addProcessNode("simple-bpmn", task);
    ASSERT_TRUE(st2.ok) << st2.message;
    
    // Add end event
    themis::ProcessNodeInfo endEvent{
        .node_id = "end",
        .name = "End",
        .description = "Process end",
        .node_type = themis::BPMNNodeType::END_EVENT
    };
    auto st3 = pgm_->addProcessNode("simple-bpmn", endEvent);
    ASSERT_TRUE(st3.ok) << st3.message;
}

TEST_F(ProcessGraphTest, AddEPKNodes) {
    pgm_->registerProcess("simple-epk", "Simple EPK Process");
    
    // Add start event
    themis::ProcessNodeInfo startEvent{
        .node_id = "e1",
        .name = "Order Received",
        .description = "Initial event",
        .node_type = themis::EPKNodeType::EVENT
    };
    auto st1 = pgm_->addProcessNode("simple-epk", startEvent);
    ASSERT_TRUE(st1.ok) << st1.message;
    
    // Add function
    themis::ProcessNodeInfo function{
        .node_id = "f1",
        .name = "Check Order",
        .description = "Check order validity",
        .node_type = themis::EPKNodeType::FUNCTION
    };
    auto st2 = pgm_->addProcessNode("simple-epk", function);
    ASSERT_TRUE(st2.ok) << st2.message;
    
    // Add XOR connector
    themis::ProcessNodeInfo xor_conn{
        .node_id = "xor1",
        .name = "Order Valid?",
        .description = "XOR decision",
        .node_type = themis::EPKNodeType::XOR_CONNECTOR
    };
    auto st3 = pgm_->addProcessNode("simple-epk", xor_conn);
    ASSERT_TRUE(st3.ok) << st3.message;
}

TEST_F(ProcessGraphTest, AddProcessEdges) {
    pgm_->registerProcess("flow-test", "Flow Test Process");
    
    // Add nodes
    themis::ProcessNodeInfo start{.node_id = "start", .name = "Start", .node_type = themis::BPMNNodeType::START_EVENT};
    themis::ProcessNodeInfo task{.node_id = "task", .name = "Task", .node_type = themis::BPMNNodeType::TASK};
    themis::ProcessNodeInfo end{.node_id = "end", .name = "End", .node_type = themis::BPMNNodeType::END_EVENT};
    
    pgm_->addProcessNode("flow-test", start);
    pgm_->addProcessNode("flow-test", task);
    pgm_->addProcessNode("flow-test", end);
    
    // Add edges
    themis::ProcessEdgeInfo edge1{
        .edge_id = "flow1",
        .edge_type = themis::ProcessEdgeType::SEQUENCE_FLOW,
        .from_node = "start",
        .to_node = "task"
    };
    auto st1 = pgm_->addProcessEdge("flow-test", edge1);
    ASSERT_TRUE(st1.ok) << st1.message;
    
    themis::ProcessEdgeInfo edge2{
        .edge_id = "flow2",
        .edge_type = themis::ProcessEdgeType::SEQUENCE_FLOW,
        .from_node = "task",
        .to_node = "end"
    };
    auto st2 = pgm_->addProcessEdge("flow-test", edge2);
    ASSERT_TRUE(st2.ok) << st2.message;
}

TEST_F(ProcessGraphTest, AddHyperedge) {
    pgm_->registerProcess("parallel-test", "Parallel Test Process");
    
    // Add nodes for AND-join
    themis::ProcessNodeInfo task1{.node_id = "task1", .name = "Task 1", .node_type = themis::BPMNNodeType::TASK};
    themis::ProcessNodeInfo task2{.node_id = "task2", .name = "Task 2", .node_type = themis::BPMNNodeType::TASK};
    themis::ProcessNodeInfo join{.node_id = "join", .name = "AND Join", .node_type = themis::BPMNNodeType::PARALLEL_GATEWAY};
    themis::ProcessNodeInfo next{.node_id = "next", .name = "Next Task", .node_type = themis::BPMNNodeType::TASK};
    
    pgm_->addProcessNode("parallel-test", task1);
    pgm_->addProcessNode("parallel-test", task2);
    pgm_->addProcessNode("parallel-test", join);
    pgm_->addProcessNode("parallel-test", next);
    
    // Add hyperedge for AND-join
    themis::Hyperedge hyperedge{
        .hyperedge_id = "and-join-1",
        .name = "Wait for all tasks",
        .source_nodes = {"task1", "task2"},
        .target_nodes = {"join"},
        .sync_type = themis::Hyperedge::SyncType::AND_JOIN
    };
    
    auto st = pgm_->addHyperedge("parallel-test", hyperedge);
    ASSERT_TRUE(st.ok) << st.message;
}

TEST_F(ProcessGraphTest, ValidateProcess_ValidProcess) {
    pgm_->registerProcess("valid-process", "Valid Process");
    
    // Create a valid process: Start -> Task -> End
    themis::ProcessNodeInfo start{.node_id = "start", .name = "Start", .node_type = themis::BPMNNodeType::START_EVENT};
    themis::ProcessNodeInfo task{.node_id = "task", .name = "Task", .node_type = themis::BPMNNodeType::TASK};
    themis::ProcessNodeInfo end{.node_id = "end", .name = "End", .node_type = themis::BPMNNodeType::END_EVENT};
    
    pgm_->addProcessNode("valid-process", start);
    pgm_->addProcessNode("valid-process", task);
    pgm_->addProcessNode("valid-process", end);
    
    themis::ProcessEdgeInfo flow1{.edge_id = "f1", .from_node = "start", .to_node = "task"};
    themis::ProcessEdgeInfo flow2{.edge_id = "f2", .from_node = "task", .to_node = "end"};
    pgm_->addProcessEdge("valid-process", flow1);
    pgm_->addProcessEdge("valid-process", flow2);
    
    auto [st, result] = pgm_->validateProcess("valid-process");
    ASSERT_TRUE(st.ok) << st.message;
    EXPECT_TRUE(result.is_valid) << "Errors: " << (result.errors.empty() ? "none" : result.errors[0]);
    EXPECT_TRUE(result.errors.empty());
}

TEST_F(ProcessGraphTest, ValidateProcess_MissingStart) {
    pgm_->registerProcess("no-start", "No Start Process");
    
    // Create a process without start event
    themis::ProcessNodeInfo task{.node_id = "task", .name = "Task", .node_type = themis::BPMNNodeType::TASK};
    themis::ProcessNodeInfo end{.node_id = "end", .name = "End", .node_type = themis::BPMNNodeType::END_EVENT};
    
    pgm_->addProcessNode("no-start", task);
    pgm_->addProcessNode("no-start", end);
    
    themis::ProcessEdgeInfo flow{.edge_id = "f1", .from_node = "task", .to_node = "end"};
    pgm_->addProcessEdge("no-start", flow);
    
    auto [st, result] = pgm_->validateProcess("no-start");
    ASSERT_TRUE(st.ok);
    EXPECT_FALSE(result.is_valid);
    EXPECT_FALSE(result.errors.empty());
    
    // Check that the error mentions missing start
    bool hasStartError = false;
    for (const auto& err : result.errors) {
        if (err.find("start") != std::string::npos) {
            hasStartError = true;
            break;
        }
    }
    EXPECT_TRUE(hasStartError);
}

TEST_F(ProcessGraphTest, StartAndGetProcessInstance) {
    pgm_->registerProcess("exec-test", "Execution Test");
    
    // Create a simple process
    themis::ProcessNodeInfo start{.node_id = "start", .name = "Start", .node_type = themis::BPMNNodeType::START_EVENT};
    themis::ProcessNodeInfo task{.node_id = "task", .name = "Task", .node_type = themis::BPMNNodeType::TASK};
    themis::ProcessNodeInfo end{.node_id = "end", .name = "End", .node_type = themis::BPMNNodeType::END_EVENT};
    
    pgm_->addProcessNode("exec-test", start);
    pgm_->addProcessNode("exec-test", task);
    pgm_->addProcessNode("exec-test", end);
    
    themis::ProcessEdgeInfo flow1{.edge_id = "f1", .from_node = "start", .to_node = "task"};
    themis::ProcessEdgeInfo flow2{.edge_id = "f2", .from_node = "task", .to_node = "end"};
    pgm_->addProcessEdge("exec-test", flow1);
    pgm_->addProcessEdge("exec-test", flow2);
    
    // Start the process
    nlohmann::json initialVars = {{"orderId", "ORD-123"}, {"amount", 100.0}};
    auto [st, instanceId] = pgm_->startProcess("exec-test", initialVars);
    ASSERT_TRUE(st.ok) << st.message;
    EXPECT_FALSE(instanceId.empty());
    
    // Get the process instance
    auto [st2, instance] = pgm_->getProcessInstance(instanceId);
    ASSERT_TRUE(st2.ok) << st2.message;
    EXPECT_EQ(instance.instance_id, instanceId);
    EXPECT_EQ(instance.process_definition_id, "exec-test");
    EXPECT_EQ(instance.state, themis::ProcessInstance::State::RUNNING);
    EXPECT_FALSE(instance.tokens.empty());
    
    // Check initial token is at start
    EXPECT_EQ(instance.tokens[0].current_node, "start");
    EXPECT_EQ(instance.tokens[0].state, themis::ProcessToken::State::READY);
}

TEST_F(ProcessGraphTest, AdvanceToken) {
    pgm_->registerProcess("advance-test", "Advance Test");
    
    // Create a simple process
    themis::ProcessNodeInfo start{.node_id = "start", .name = "Start", .node_type = themis::BPMNNodeType::START_EVENT};
    themis::ProcessNodeInfo task{.node_id = "task", .name = "Task", .node_type = themis::BPMNNodeType::TASK};
    themis::ProcessNodeInfo end{.node_id = "end", .name = "End", .node_type = themis::BPMNNodeType::END_EVENT};
    
    pgm_->addProcessNode("advance-test", start);
    pgm_->addProcessNode("advance-test", task);
    pgm_->addProcessNode("advance-test", end);
    
    themis::ProcessEdgeInfo flow1{.edge_id = "f1", .from_node = "start", .to_node = "task"};
    themis::ProcessEdgeInfo flow2{.edge_id = "f2", .from_node = "task", .to_node = "end"};
    pgm_->addProcessEdge("advance-test", flow1);
    pgm_->addProcessEdge("advance-test", flow2);
    
    // Start and get instance
    auto [st, instanceId] = pgm_->startProcess("advance-test");
    ASSERT_TRUE(st.ok);
    
    auto [st2, instance] = pgm_->getProcessInstance(instanceId);
    ASSERT_TRUE(st2.ok);
    ASSERT_FALSE(instance.tokens.empty());
    
    std::string tokenId = instance.tokens[0].token_id;
    
    // Advance token from start to task
    auto st3 = pgm_->advanceToken(instanceId, tokenId);
    ASSERT_TRUE(st3.ok) << st3.message;
    
    // Verify token moved
    auto [st4, instance2] = pgm_->getProcessInstance(instanceId);
    ASSERT_TRUE(st4.ok);
    ASSERT_FALSE(instance2.tokens.empty());
    EXPECT_EQ(instance2.tokens[0].current_node, "task");
}

TEST_F(ProcessGraphTest, SuspendResumeProcess) {
    pgm_->registerProcess("suspend-test", "Suspend Test");
    
    themis::ProcessNodeInfo start{.node_id = "start", .name = "Start", .node_type = themis::BPMNNodeType::START_EVENT};
    pgm_->addProcessNode("suspend-test", start);
    
    auto [st, instanceId] = pgm_->startProcess("suspend-test");
    ASSERT_TRUE(st.ok);
    
    // Suspend
    auto st2 = pgm_->suspendProcess(instanceId);
    ASSERT_TRUE(st2.ok);
    
    auto [st3, instance] = pgm_->getProcessInstance(instanceId);
    EXPECT_EQ(instance.state, themis::ProcessInstance::State::SUSPENDED);
    
    // Resume
    auto st4 = pgm_->resumeProcess(instanceId);
    ASSERT_TRUE(st4.ok);
    
    auto [st5, instance2] = pgm_->getProcessInstance(instanceId);
    EXPECT_EQ(instance2.state, themis::ProcessInstance::State::RUNNING);
}

TEST_F(ProcessGraphTest, TerminateProcess) {
    pgm_->registerProcess("terminate-test", "Terminate Test");
    
    themis::ProcessNodeInfo start{.node_id = "start", .name = "Start", .node_type = themis::BPMNNodeType::START_EVENT};
    pgm_->addProcessNode("terminate-test", start);
    
    auto [st, instanceId] = pgm_->startProcess("terminate-test");
    ASSERT_TRUE(st.ok);
    
    auto st2 = pgm_->terminateProcess(instanceId, "Testing termination");
    ASSERT_TRUE(st2.ok);
    
    auto [st3, instance] = pgm_->getProcessInstance(instanceId);
    EXPECT_EQ(instance.state, themis::ProcessInstance::State::TERMINATED);
}

// ============================================================================
// Process Edge Types Registration Tests
// ============================================================================

TEST_F(ProcessGraphTest, ProcessEdgeTypesRegistered) {
    auto& registry = themis::EdgeTypeRegistry::instance();
    
    // BPMN types
    EXPECT_TRUE(registry.isRegistered("SEQUENCE_FLOW"));
    EXPECT_TRUE(registry.isRegistered("MESSAGE_FLOW"));
    
    // EPK types
    EXPECT_TRUE(registry.isRegistered("CONTROL_FLOW"));
    EXPECT_TRUE(registry.isRegistered("INFORMATION_FLOW"));
    
    // Execution types
    EXPECT_TRUE(registry.isRegistered("CONDITIONAL_FLOW"));
    EXPECT_TRUE(registry.isRegistered("DEFAULT_FLOW"));
    EXPECT_TRUE(registry.isRegistered("EXCEPTION_FLOW"));
    
    // Assignment types
    EXPECT_TRUE(registry.isRegistered("ASSIGNED_TO"));
    EXPECT_TRUE(registry.isRegistered("CALLS_PROCESS"));
}
