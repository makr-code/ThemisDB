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
    
    auto [validateStatus, validateResult] = pgm_->validateProcess("valid-process");
    ASSERT_TRUE(validateStatus.ok) << validateStatus.message;
    EXPECT_TRUE(validateResult.is_valid) << "Errors: " << (validateResult.errors.empty() ? "none" : validateResult.errors[0]);
    EXPECT_TRUE(validateResult.errors.empty());
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
    
    auto [validateStatus, validateResult] = pgm_->validateProcess("no-start");
    ASSERT_TRUE(validateStatus.ok) << validateStatus.message;
    EXPECT_FALSE(validateResult.is_valid);
    EXPECT_FALSE(validateResult.errors.empty());
    
    // Check that the error mentions missing start
    bool hasStartError = false;
    for (const auto& err : validateResult.errors) {
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
    auto [startStatus, instanceId] = pgm_->startProcess("exec-test", initialVars);
    ASSERT_TRUE(startStatus.ok) << startStatus.message;
    EXPECT_FALSE(instanceId.empty());
    
    // Get the process instance
    auto [instanceStatus, instance] = pgm_->getProcessInstance(instanceId);
    ASSERT_TRUE(instanceStatus.ok) << instanceStatus.message;
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
    auto [startStatus, instanceId] = pgm_->startProcess("advance-test");
    ASSERT_TRUE(startStatus.ok) << startStatus.message;

    auto [instanceStatus, instance] = pgm_->getProcessInstance(instanceId);
    ASSERT_TRUE(instanceStatus.ok) << instanceStatus.message;
    ASSERT_FALSE(instance.tokens.empty());
    
    std::string tokenId = instance.tokens[0].token_id;
    
    // Advance token from start to task
    auto st3 = pgm_->advanceToken(instanceId, tokenId);
    ASSERT_TRUE(st3.ok) << st3.message;
    
    // Verify token moved
    auto [instanceStatus2, instance2] = pgm_->getProcessInstance(instanceId);
    ASSERT_TRUE(instanceStatus2.ok) << instanceStatus2.message;
    ASSERT_FALSE(instance2.tokens.empty());
    EXPECT_EQ(instance2.tokens[0].current_node, "task");
}

TEST_F(ProcessGraphTest, SuspendResumeProcess) {
    pgm_->registerProcess("suspend-test", "Suspend Test");
    
    themis::ProcessNodeInfo start{.node_id = "start", .name = "Start", .node_type = themis::BPMNNodeType::START_EVENT};
    pgm_->addProcessNode("suspend-test", start);
    
    auto [startStatus, instanceId] = pgm_->startProcess("suspend-test");
    ASSERT_TRUE(startStatus.ok) << startStatus.message;
    
    // Suspend
    auto st2 = pgm_->suspendProcess(instanceId);
    ASSERT_TRUE(st2.ok);

    auto [instanceStatus, instance] = pgm_->getProcessInstance(instanceId);
    ASSERT_TRUE(instanceStatus.ok) << instanceStatus.message;
    EXPECT_EQ(instance.state, themis::ProcessInstance::State::SUSPENDED);
    
    // Resume
    auto st4 = pgm_->resumeProcess(instanceId);
    ASSERT_TRUE(st4.ok);

    auto [instanceStatus2, instance2] = pgm_->getProcessInstance(instanceId);
    ASSERT_TRUE(instanceStatus2.ok) << instanceStatus2.message;
    EXPECT_EQ(instance2.state, themis::ProcessInstance::State::RUNNING);
}

TEST_F(ProcessGraphTest, TerminateProcess) {
    pgm_->registerProcess("terminate-test", "Terminate Test");
    
    themis::ProcessNodeInfo start{.node_id = "start", .name = "Start", .node_type = themis::BPMNNodeType::START_EVENT};
    pgm_->addProcessNode("terminate-test", start);
    
    auto [startStatus, instanceId] = pgm_->startProcess("terminate-test");
    ASSERT_TRUE(startStatus.ok) << startStatus.message;

    auto st2 = pgm_->terminateProcess(instanceId, "Testing termination");
    ASSERT_TRUE(st2.ok);

    auto [instanceStatus, instance] = pgm_->getProcessInstance(instanceId);
    ASSERT_TRUE(instanceStatus.ok) << instanceStatus.message;
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

// ============================================================================
// Process Mining Features Tests (8 TODO markers resolved)
// ============================================================================

TEST_F(ProcessGraphTest, ConditionEvaluation) {
    pgm_->registerProcess("cond-test", "Condition Test Process");
    
    // Create a process with conditional gateway
    themis::ProcessNodeInfo start{.node_id = "start", .name = "Start", .node_type = themis::BPMNNodeType::START_EVENT};
    themis::ProcessNodeInfo gateway{.node_id = "gateway", .name = "Decision", .node_type = themis::BPMNNodeType::EXCLUSIVE_GATEWAY};
    themis::ProcessNodeInfo taskHigh{.node_id = "task_high", .name = "High Value", .node_type = themis::BPMNNodeType::TASK};
    themis::ProcessNodeInfo taskLow{.node_id = "task_low", .name = "Low Value", .node_type = themis::BPMNNodeType::TASK};
    themis::ProcessNodeInfo end{.node_id = "end", .name = "End", .node_type = themis::BPMNNodeType::END_EVENT};
    
    pgm_->addProcessNode("cond-test", start);
    pgm_->addProcessNode("cond-test", gateway);
    pgm_->addProcessNode("cond-test", taskHigh);
    pgm_->addProcessNode("cond-test", taskLow);
    pgm_->addProcessNode("cond-test", end);
    
    // Add edges
    themis::ProcessEdgeInfo flow1{.edge_id = "f1", .from_node = "start", .to_node = "gateway"};
    themis::ProcessEdgeInfo flowHigh{
        .edge_id = "f_high",
        .edge_type = themis::ProcessEdgeType::CONDITIONAL_FLOW,
        .from_node = "gateway",
        .to_node = "task_high",
        .condition_expression = "amount > 1000",
        .priority = 1
    };
    themis::ProcessEdgeInfo flowLow{
        .edge_id = "f_low",
        .edge_type = themis::ProcessEdgeType::DEFAULT_FLOW,
        .from_node = "gateway",
        .to_node = "task_low",
        .is_default = true,
        .priority = 0
    };
    themis::ProcessEdgeInfo flowEnd1{.edge_id = "f_end1", .from_node = "task_high", .to_node = "end"};
    themis::ProcessEdgeInfo flowEnd2{.edge_id = "f_end2", .from_node = "task_low", .to_node = "end"};
    
    pgm_->addProcessEdge("cond-test", flow1);
    pgm_->addProcessEdge("cond-test", flowHigh);
    pgm_->addProcessEdge("cond-test", flowLow);
    pgm_->addProcessEdge("cond-test", flowEnd1);
    pgm_->addProcessEdge("cond-test", flowEnd2);
    
    // Test with high amount (should go to task_high)
    nlohmann::json vars1 = {{"amount", 2000}};
    auto [startStatus1, instanceId1] = pgm_->startProcess("cond-test", vars1);
    ASSERT_TRUE(startStatus1.ok) << startStatus1.message;

    auto [instanceStatus1, instance1] = pgm_->getProcessInstance(instanceId1);
    ASSERT_TRUE(instanceStatus1.ok) << instanceStatus1.message;
    ASSERT_FALSE(instance1.tokens.empty());
    
    // Advance to gateway
    auto st3 = pgm_->advanceToken(instanceId1, instance1.tokens[0].token_id);
    ASSERT_TRUE(st3.ok);
    
    // Advance through gateway (should choose high value path)
    auto [instanceStatus2, instance2] = pgm_->getProcessInstance(instanceId1);
    ASSERT_TRUE(instanceStatus2.ok) << instanceStatus2.message;
    auto st5 = pgm_->advanceToken(instanceId1, instance2.tokens[0].token_id);
    ASSERT_TRUE(st5.ok);
    
    auto [instanceStatus3, instance3] = pgm_->getProcessInstance(instanceId1);
    ASSERT_TRUE(instanceStatus3.ok) << instanceStatus3.message;
    EXPECT_EQ(instance3.tokens[0].current_node, "task_high");
    
    // Test with low amount (should go to task_low)
    nlohmann::json vars2 = {{"amount", 500}};
    auto [startStatus2, instanceId2] = pgm_->startProcess("cond-test", vars2);
    ASSERT_TRUE(startStatus2.ok) << startStatus2.message;

    auto [instanceStatus4, instance4] = pgm_->getProcessInstance(instanceId2);
    ASSERT_TRUE(instanceStatus4.ok) << instanceStatus4.message;
    
    // Advance to gateway and through it
    pgm_->advanceToken(instanceId2, instance4.tokens[0].token_id);
    auto [instanceStatus5, instance5] = pgm_->getProcessInstance(instanceId2);
    ASSERT_TRUE(instanceStatus5.ok) << instanceStatus5.message;
    pgm_->advanceToken(instanceId2, instance5.tokens[0].token_id);

    auto [instanceStatus6, instance6] = pgm_->getProcessInstance(instanceId2);
    ASSERT_TRUE(instanceStatus6.ok) << instanceStatus6.message;
    EXPECT_EQ(instance6.tokens[0].current_node, "task_low");
}

TEST_F(ProcessGraphTest, EventHandling) {
    pgm_->registerProcess("event-test", "Event Test Process");
    
    // Create process with intermediate event
    themis::ProcessNodeInfo start{.node_id = "start", .name = "Start", .node_type = themis::BPMNNodeType::START_EVENT};
    themis::ProcessNodeInfo task1{.node_id = "task1", .name = "Task 1", .node_type = themis::BPMNNodeType::TASK};
    themis::ProcessNodeInfo event{
        .node_id = "event1",
        .name = "Wait for Message",
        .description = "",
        .node_type = themis::BPMNNodeType::INTERMEDIATE_EVENT,
        .subtype = "MESSAGE"
    };
    themis::ProcessNodeInfo task2{.node_id = "task2", .name = "Task 2", .node_type = themis::BPMNNodeType::TASK};
    themis::ProcessNodeInfo end{.node_id = "end", .name = "End", .node_type = themis::BPMNNodeType::END_EVENT};
    
    pgm_->addProcessNode("event-test", start);
    pgm_->addProcessNode("event-test", task1);
    pgm_->addProcessNode("event-test", event);
    pgm_->addProcessNode("event-test", task2);
    pgm_->addProcessNode("event-test", end);
    
    // Add event_name to event node
    std::string eventKey = "process:node:event-test:event1";
    auto eventBlob = db_->get(eventKey);
    if (eventBlob) {
        themis::BaseEntity eventEntity = themis::BaseEntity::deserialize("event1", *eventBlob);
        eventEntity.setField("event_name", "approval_received");
        db_->put(eventKey, eventEntity.serialize());
    }
    
    // Add edges
    themis::ProcessEdgeInfo flow1{.edge_id = "f1", .from_node = "start", .to_node = "task1"};
    themis::ProcessEdgeInfo flow2{.edge_id = "f2", .from_node = "task1", .to_node = "event1"};
    themis::ProcessEdgeInfo flow3{.edge_id = "f3", .from_node = "event1", .to_node = "task2"};
    themis::ProcessEdgeInfo flow4{.edge_id = "f4", .from_node = "task2", .to_node = "end"};
    
    pgm_->addProcessEdge("event-test", flow1);
    pgm_->addProcessEdge("event-test", flow2);
    pgm_->addProcessEdge("event-test", flow3);
    pgm_->addProcessEdge("event-test", flow4);
    
    // Start process
    auto [startStatus, instanceId] = pgm_->startProcess("event-test");
    ASSERT_TRUE(startStatus.ok) << startStatus.message;

    auto [instanceStatus, instance] = pgm_->getProcessInstance(instanceId);
    ASSERT_TRUE(instanceStatus.ok) << instanceStatus.message;
    
    // Advance to task1
    pgm_->advanceToken(instanceId, instance.tokens[0].token_id);
    
    // Advance to event (token should be at event now)
    auto [instanceStatus2, instance2] = pgm_->getProcessInstance(instanceId);
    ASSERT_TRUE(instanceStatus2.ok) << instanceStatus2.message;
    pgm_->advanceToken(instanceId, instance2.tokens[0].token_id);

    auto [instanceStatus3, instance3] = pgm_->getProcessInstance(instanceId);
    ASSERT_TRUE(instanceStatus3.ok) << instanceStatus3.message;
    EXPECT_EQ(instance3.tokens[0].current_node, "event1");
    
    // Set token to WAITING state to simulate waiting for event
    std::string tokenKey = "process:token:" + instanceId + ":" + instance3.tokens[0].token_id;
    auto tokenBlob = db_->get(tokenKey);
    if (tokenBlob) {
        themis::BaseEntity tokenEntity = themis::BaseEntity::deserialize(instance3.tokens[0].token_id, *tokenBlob);
        tokenEntity.setField("state", "WAITING");
        db_->put(tokenKey, tokenEntity.serialize());
    }
    
    // Signal the event
    nlohmann::json payload = {{"approved", true}, {"approver", "manager"}};
    auto st5 = pgm_->signalEvent(instanceId, "approval_received", payload);
    ASSERT_TRUE(st5.ok) << st5.message;
    
    // Verify token is now READY
    auto [instanceStatus4, instance4] = pgm_->getProcessInstance(instanceId);
    ASSERT_TRUE(instanceStatus4.ok) << instanceStatus4.message;
    EXPECT_EQ(instance4.tokens[0].state, themis::ProcessToken::State::READY);
    EXPECT_TRUE(instance4.tokens[0].variables.contains("approved"));
    EXPECT_EQ(instance4.tokens[0].variables["approved"], true);
}

TEST_F(ProcessGraphTest, TaskAssignmentQueries) {
    pgm_->registerProcess("assign-test", "Assignment Test Process");
    
    // Create process with tasks
    themis::ProcessNodeInfo start{.node_id = "start", .name = "Start", .node_type = themis::BPMNNodeType::START_EVENT};
    themis::ProcessNodeInfo task1{.node_id = "task1", .name = "Task 1", .node_type = themis::BPMNNodeType::TASK};
    themis::ProcessNodeInfo task2{.node_id = "task2", .name = "Task 2", .node_type = themis::BPMNNodeType::TASK};
    themis::ProcessNodeInfo end{.node_id = "end", .name = "End", .node_type = themis::BPMNNodeType::END_EVENT};
    
    pgm_->addProcessNode("assign-test", start);
    pgm_->addProcessNode("assign-test", task1);
    pgm_->addProcessNode("assign-test", task2);
    pgm_->addProcessNode("assign-test", end);
    
    // Add edges
    themis::ProcessEdgeInfo flow1{.edge_id = "f1", .from_node = "start", .to_node = "task1"};
    themis::ProcessEdgeInfo flow2{.edge_id = "f2", .from_node = "task1", .to_node = "task2"};
    themis::ProcessEdgeInfo flow3{.edge_id = "f3", .from_node = "task2", .to_node = "end"};
    
    pgm_->addProcessEdge("assign-test", flow1);
    pgm_->addProcessEdge("assign-test", flow2);
    pgm_->addProcessEdge("assign-test", flow3);
    
    // Start process
    auto [startStatus, instanceId] = pgm_->startProcess("assign-test");
    ASSERT_TRUE(startStatus.ok) << startStatus.message;

    auto [instanceStatus, instance] = pgm_->getProcessInstance(instanceId);
    ASSERT_TRUE(instanceStatus.ok) << instanceStatus.message;
    
    // Advance to task1
    pgm_->advanceToken(instanceId, instance.tokens[0].token_id);
    
    // Add assignment metadata to token at task1
    auto [instanceStatus2, instance2] = pgm_->getProcessInstance(instanceId);
    ASSERT_TRUE(instanceStatus2.ok) << instanceStatus2.message;
    std::string tokenKey = "process:token:" + instanceId + ":" + instance2.tokens[0].token_id;
    auto tokenBlob = db_->get(tokenKey);
    if (tokenBlob) {
        themis::BaseEntity tokenEntity = themis::BaseEntity::deserialize(instance2.tokens[0].token_id, *tokenBlob);
        tokenEntity.setField("assignee", "john");
        tokenEntity.setField("role", "manager");
        db_->put(tokenKey, tokenEntity.serialize());
    }
    
    // Query tasks by assignee
    auto [tasksStatus, tasks] = pgm_->findActiveTasks("john");
    ASSERT_TRUE(tasksStatus.ok) << tasksStatus.message;
    EXPECT_FALSE(tasks.empty());
    
    if (!tasks.empty()) {
        EXPECT_EQ(tasks[0].current_node, "task1");
    }
    
    // Query by role
    auto [tasksStatus2, tasks2] = pgm_->findActiveTasks("manager");
    ASSERT_TRUE(tasksStatus2.ok) << tasksStatus2.message;
    EXPECT_FALSE(tasks2.empty());
}

TEST_F(ProcessGraphTest, NodeHistory) {
    pgm_->registerProcess("history-test", "History Test Process");
    
    // Create simple process
    themis::ProcessNodeInfo start{.node_id = "start", .name = "Start", .node_type = themis::BPMNNodeType::START_EVENT};
    themis::ProcessNodeInfo task{.node_id = "task", .name = "Task", .node_type = themis::BPMNNodeType::TASK};
    themis::ProcessNodeInfo end{.node_id = "end", .name = "End", .node_type = themis::BPMNNodeType::END_EVENT};
    
    pgm_->addProcessNode("history-test", start);
    pgm_->addProcessNode("history-test", task);
    pgm_->addProcessNode("history-test", end);
    
    themis::ProcessEdgeInfo flow1{.edge_id = "f1", .from_node = "start", .to_node = "task"};
    themis::ProcessEdgeInfo flow2{.edge_id = "f2", .from_node = "task", .to_node = "end"};
    
    pgm_->addProcessEdge("history-test", flow1);
    pgm_->addProcessEdge("history-test", flow2);
    
    // Start multiple instances
    auto [startStatus1, instanceId1] = pgm_->startProcess("history-test");
    auto [startStatus2, instanceId2] = pgm_->startProcess("history-test");

    ASSERT_TRUE(startStatus1.ok) << startStatus1.message;
    ASSERT_TRUE(startStatus2.ok) << startStatus2.message;
    
    // Advance both to task
    auto [instanceStatus1, instance1] = pgm_->getProcessInstance(instanceId1);
    ASSERT_TRUE(instanceStatus1.ok) << instanceStatus1.message;
    pgm_->advanceToken(instanceId1, instance1.tokens[0].token_id);

    auto [instanceStatus2b, instance2] = pgm_->getProcessInstance(instanceId2);
    ASSERT_TRUE(instanceStatus2b.ok) << instanceStatus2b.message;
    pgm_->advanceToken(instanceId2, instance2.tokens[0].token_id);
    
    // Query history for task node
    auto [historyStatus, history] = pgm_->getNodeHistory("history-test", "task", std::nullopt);
    ASSERT_TRUE(historyStatus.ok) << historyStatus.message;
    EXPECT_GE(history.size(), 2u); // At least 2 tokens should have visited this node
}

TEST_F(ProcessGraphTest, ProcessMetrics) {
    pgm_->registerProcess("metrics-test", "Metrics Test Process");
    
    // Create process
    themis::ProcessNodeInfo start{.node_id = "start", .name = "Start", .node_type = themis::BPMNNodeType::START_EVENT};
    themis::ProcessNodeInfo task{.node_id = "task", .name = "Task", .node_type = themis::BPMNNodeType::TASK};
    themis::ProcessNodeInfo end{.node_id = "end", .name = "End", .node_type = themis::BPMNNodeType::END_EVENT};
    
    pgm_->addProcessNode("metrics-test", start);
    pgm_->addProcessNode("metrics-test", task);
    pgm_->addProcessNode("metrics-test", end);
    
    themis::ProcessEdgeInfo flow1{.edge_id = "f1", .from_node = "start", .to_node = "task"};
    themis::ProcessEdgeInfo flow2{.edge_id = "f2", .from_node = "task", .to_node = "end"};
    
    pgm_->addProcessEdge("metrics-test", flow1);
    pgm_->addProcessEdge("metrics-test", flow2);
    
    // Start an instance and complete it
    auto [startStatus, instanceId] = pgm_->startProcess("metrics-test");
    ASSERT_TRUE(startStatus.ok) << startStatus.message;

    auto [instanceStatus, instance] = pgm_->getProcessInstance(instanceId);
    ASSERT_TRUE(instanceStatus.ok) << instanceStatus.message;
    
    // Advance and complete task
    pgm_->advanceToken(instanceId, instance.tokens[0].token_id);
    
    // Add completed_at timestamp to simulate completion
    auto [instanceStatus2, instance2] = pgm_->getProcessInstance(instanceId);
    ASSERT_TRUE(instanceStatus2.ok) << instanceStatus2.message;
    std::string tokenKey = "process:token:" + instanceId + ":" + instance2.tokens[0].token_id;
    auto tokenBlob = db_->get(tokenKey);
    if (tokenBlob) {
        themis::BaseEntity tokenEntity = themis::BaseEntity::deserialize(instance2.tokens[0].token_id, *tokenBlob);
        int64_t now = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        tokenEntity.setField("started_at", now - 1000);
        tokenEntity.setField("completed_at", now);
        tokenEntity.setField("state", "COMPLETED");
        db_->put(tokenKey, tokenEntity.serialize());
    }
    
    // Get metrics
    auto [metricsStatus, metrics] = pgm_->getProcessMetrics("metrics-test");
    ASSERT_TRUE(metricsStatus.ok) << metricsStatus.message;
    EXPECT_FALSE(metrics.empty());
    
    // Verify metrics structure
    for (const auto& m : metrics) {
        EXPECT_FALSE(m.node_id.empty());
        EXPECT_GE(m.execution_count, 0u);
    }
}

TEST_F(ProcessGraphTest, CriticalPath) {
    pgm_->registerProcess("critical-test", "Critical Path Test Process");
    
    // Create process
    themis::ProcessNodeInfo start{.node_id = "start", .name = "Start", .node_type = themis::BPMNNodeType::START_EVENT};
    themis::ProcessNodeInfo task1{.node_id = "task1", .name = "Task 1", .node_type = themis::BPMNNodeType::TASK};
    themis::ProcessNodeInfo task2{.node_id = "task2", .name = "Task 2", .node_type = themis::BPMNNodeType::TASK};
    themis::ProcessNodeInfo end{.node_id = "end", .name = "End", .node_type = themis::BPMNNodeType::END_EVENT};
    
    pgm_->addProcessNode("critical-test", start);
    pgm_->addProcessNode("critical-test", task1);
    pgm_->addProcessNode("critical-test", task2);
    pgm_->addProcessNode("critical-test", end);
    
    themis::ProcessEdgeInfo flow1{.edge_id = "f1", .from_node = "start", .to_node = "task1"};
    themis::ProcessEdgeInfo flow2{.edge_id = "f2", .from_node = "task1", .to_node = "task2"};
    themis::ProcessEdgeInfo flow3{.edge_id = "f3", .from_node = "task2", .to_node = "end"};
    
    pgm_->addProcessEdge("critical-test", flow1);
    pgm_->addProcessEdge("critical-test", flow2);
    pgm_->addProcessEdge("critical-test", flow3);
    
    // Find critical path (may be empty if no executions yet)
    auto [criticalStatus, criticalPath] = pgm_->findCriticalPath("critical-test");
    ASSERT_TRUE(criticalStatus.ok) << criticalStatus.message;
    // Path can be empty if no metrics available, which is ok
}

TEST_F(ProcessGraphTest, HyperedgeStatus) {
    pgm_->registerProcess("hyperedge-test", "Hyperedge Test Process");
    
    // Create nodes
    themis::ProcessNodeInfo task1{.node_id = "task1", .name = "Task 1", .node_type = themis::BPMNNodeType::TASK};
    themis::ProcessNodeInfo task2{.node_id = "task2", .name = "Task 2", .node_type = themis::BPMNNodeType::TASK};
    themis::ProcessNodeInfo join{.node_id = "join", .name = "Join", .node_type = themis::BPMNNodeType::PARALLEL_GATEWAY};
    
    pgm_->addProcessNode("hyperedge-test", task1);
    pgm_->addProcessNode("hyperedge-test", task2);
    pgm_->addProcessNode("hyperedge-test", join);
    
    // Add hyperedge
    themis::Hyperedge hyperedge{
        .hyperedge_id = "he1",
        .name = "AND Join",
        .source_nodes = {"task1", "task2"},
        .target_nodes = {"join"},
        .sync_type = themis::Hyperedge::SyncType::AND_JOIN
    };
    
    pgm_->addHyperedge("hyperedge-test", hyperedge);
    
    // Get hyperedge status
    auto [hyperedgeStatus, retrievedHe] = pgm_->getHyperedgeStatus("he1");
    ASSERT_TRUE(hyperedgeStatus.ok) << hyperedgeStatus.message;
    EXPECT_EQ(retrievedHe.hyperedge_id, "he1");
    EXPECT_EQ(retrievedHe.source_nodes.size(), 2u);
}

TEST_F(ProcessGraphTest, HyperedgeReadiness) {
    pgm_->registerProcess("ready-test", "Readiness Test Process");
    
    // Create nodes
    themis::ProcessNodeInfo task1{.node_id = "task1", .name = "Task 1", .node_type = themis::BPMNNodeType::TASK};
    themis::ProcessNodeInfo task2{.node_id = "task2", .name = "Task 2", .node_type = themis::BPMNNodeType::TASK};
    themis::ProcessNodeInfo join{.node_id = "join", .name = "Join", .node_type = themis::BPMNNodeType::PARALLEL_GATEWAY};
    
    pgm_->addProcessNode("ready-test", task1);
    pgm_->addProcessNode("ready-test", task2);
    pgm_->addProcessNode("ready-test", join);
    
    // Add hyperedge
    themis::Hyperedge hyperedge{
        .hyperedge_id = "he2",
        .name = "AND Join",
        .source_nodes = {"task1", "task2"},
        .target_nodes = {"join"},
        .sync_type = themis::Hyperedge::SyncType::AND_JOIN
    };
    
    pgm_->addHyperedge("ready-test", hyperedge);
    
    // Check readiness (should not be ready as no sources activated)
    auto [readyStatus, ready] = pgm_->isHyperedgeReady("he2");
    ASSERT_TRUE(readyStatus.ok) << readyStatus.message;
    EXPECT_FALSE(ready); // Not ready yet as no sources activated
}

// ============================================================================
// ProcessGraphVisitLog / Visit Timestamp Tests
// ============================================================================

TEST_F(ProcessGraphTest, VisitTimestampPopulatedOnStartProcess) {
    pgm_->registerProcess("ts-start-test", "Visit Timestamp Start Test");

    themis::ProcessNodeInfo start{.node_id = "start", .name = "Start",
                                  .node_type = themis::BPMNNodeType::START_EVENT};
    pgm_->addProcessNode("ts-start-test", start);

    auto beforeStart = std::chrono::system_clock::now();
    auto [startStatus, instanceId] = pgm_->startProcess("ts-start-test");
    auto afterStart = std::chrono::system_clock::now();
    ASSERT_TRUE(startStatus.ok) << startStatus.message;

    // The start node visit timestamp should be recorded and retrievable
    auto ts = pgm_->getVisitTimestamp(instanceId, "start");
    ASSERT_TRUE(ts.has_value()) << "Expected visit timestamp for 'start' node";
    EXPECT_GE(ts.value(), beforeStart);
    EXPECT_LE(ts.value(), afterStart);
}

TEST_F(ProcessGraphTest, VisitTimestampsPopulatedAndOrderedAcrossMultiHopTraversal) {
    pgm_->registerProcess("ts-multihop-test", "Visit Timestamp Multi-Hop Test");

    themis::ProcessNodeInfo start{.node_id = "start", .name = "Start",
                                  .node_type = themis::BPMNNodeType::START_EVENT};
    themis::ProcessNodeInfo task1{.node_id = "task1", .name = "Task 1",
                                  .node_type = themis::BPMNNodeType::TASK};
    themis::ProcessNodeInfo task2{.node_id = "task2", .name = "Task 2",
                                  .node_type = themis::BPMNNodeType::TASK};
    themis::ProcessNodeInfo end{.node_id = "end", .name = "End",
                                .node_type = themis::BPMNNodeType::END_EVENT};

    pgm_->addProcessNode("ts-multihop-test", start);
    pgm_->addProcessNode("ts-multihop-test", task1);
    pgm_->addProcessNode("ts-multihop-test", task2);
    pgm_->addProcessNode("ts-multihop-test", end);

    themis::ProcessEdgeInfo f1{.edge_id = "f1", .from_node = "start", .to_node = "task1"};
    themis::ProcessEdgeInfo f2{.edge_id = "f2", .from_node = "task1", .to_node = "task2"};
    themis::ProcessEdgeInfo f3{.edge_id = "f3", .from_node = "task2", .to_node = "end"};
    pgm_->addProcessEdge("ts-multihop-test", f1);
    pgm_->addProcessEdge("ts-multihop-test", f2);
    pgm_->addProcessEdge("ts-multihop-test", f3);

    auto [startStatus, instanceId] = pgm_->startProcess("ts-multihop-test");
    ASSERT_TRUE(startStatus.ok) << startStatus.message;

    // Get initial token id
    auto [inst0Status, inst0] = pgm_->getProcessInstance(instanceId);
    ASSERT_TRUE(inst0Status.ok);
    ASSERT_FALSE(inst0.tokens.empty());
    std::string tokenId = inst0.tokens[0].token_id;

    // start node should have a timestamp already
    auto tsStart = pgm_->getVisitTimestamp(instanceId, "start");
    ASSERT_TRUE(tsStart.has_value()) << "Expected timestamp for 'start'";

    // Advance: start -> task1
    auto adv1 = pgm_->advanceToken(instanceId, tokenId);
    ASSERT_TRUE(adv1.ok) << adv1.message;

    auto tsTask1 = pgm_->getVisitTimestamp(instanceId, "task1");
    ASSERT_TRUE(tsTask1.has_value()) << "Expected timestamp for 'task1'";
    EXPECT_GE(tsTask1.value(), tsStart.value())
        << "task1 timestamp must be >= start timestamp";

    // Advance: task1 -> task2
    auto adv2 = pgm_->advanceToken(instanceId, tokenId);
    ASSERT_TRUE(adv2.ok) << adv2.message;

    auto tsTask2 = pgm_->getVisitTimestamp(instanceId, "task2");
    ASSERT_TRUE(tsTask2.has_value()) << "Expected timestamp for 'task2'";
    EXPECT_GE(tsTask2.value(), tsTask1.value())
        << "task2 timestamp must be >= task1 timestamp";

    // Advance: task2 -> end (completes token)
    auto adv3 = pgm_->advanceToken(instanceId, tokenId);
    ASSERT_TRUE(adv3.ok) << adv3.message;

    // Verify visited_nodes also round-trips correctly through DB
    auto [instFinalStatus, instFinal] = pgm_->getProcessInstance(instanceId);
    ASSERT_TRUE(instFinalStatus.ok);
    ASSERT_FALSE(instFinal.tokens.empty());

    const auto& finalToken = instFinal.tokens[0];
    // visited_nodes should contain all traversed nodes
    const auto& vn = finalToken.visited_nodes;
    EXPECT_NE(std::find(vn.begin(), vn.end(), "start"), vn.end());
    EXPECT_NE(std::find(vn.begin(), vn.end(), "task1"), vn.end());
    EXPECT_NE(std::find(vn.begin(), vn.end(), "task2"), vn.end());

    // visit_timestamps should be present for all visited nodes
    EXPECT_TRUE(finalToken.visit_timestamps.count("start") > 0);
    EXPECT_TRUE(finalToken.visit_timestamps.count("task1") > 0);
    EXPECT_TRUE(finalToken.visit_timestamps.count("task2") > 0);
}

TEST_F(ProcessGraphTest, GetVisitTimestampReturnsNulloptForUnvisitedNode) {
    pgm_->registerProcess("ts-missing-test", "Visit Timestamp Missing Test");

    themis::ProcessNodeInfo start{.node_id = "start", .name = "Start",
                                  .node_type = themis::BPMNNodeType::START_EVENT};
    pgm_->addProcessNode("ts-missing-test", start);

    auto [startStatus, instanceId] = pgm_->startProcess("ts-missing-test");
    ASSERT_TRUE(startStatus.ok);

    // "unvisited_node" was never traversed — should return nullopt
    auto ts = pgm_->getVisitTimestamp(instanceId, "unvisited_node");
    EXPECT_FALSE(ts.has_value())
        << "Expected nullopt for a node that was never visited";
}

TEST_F(ProcessGraphTest, VisitTimestampsPersistedAfterTokenCompletion) {
    // Regression test: the COMPLETED write path must also persist visited_nodes
    // and visit_timestamps so history is not lost when a token reaches its
    // terminal node (no outgoing edges).
    pgm_->registerProcess("ts-complete-test", "Visit Timestamp Completion Test");

    themis::ProcessNodeInfo start{.node_id = "start", .name = "Start",
                                  .node_type = themis::BPMNNodeType::START_EVENT};
    themis::ProcessNodeInfo task{.node_id = "task", .name = "Task",
                                 .node_type = themis::BPMNNodeType::TASK};
    themis::ProcessNodeInfo end{.node_id = "end", .name = "End",
                                .node_type = themis::BPMNNodeType::END_EVENT};

    pgm_->addProcessNode("ts-complete-test", start);
    pgm_->addProcessNode("ts-complete-test", task);
    pgm_->addProcessNode("ts-complete-test", end);

    themis::ProcessEdgeInfo f1{.edge_id = "f1", .from_node = "start", .to_node = "task"};
    themis::ProcessEdgeInfo f2{.edge_id = "f2", .from_node = "task",  .to_node = "end"};
    pgm_->addProcessEdge("ts-complete-test", f1);
    pgm_->addProcessEdge("ts-complete-test", f2);

    auto [startStatus, instanceId] = pgm_->startProcess("ts-complete-test");
    ASSERT_TRUE(startStatus.ok) << startStatus.message;

    auto [inst0Status, inst0] = pgm_->getProcessInstance(instanceId);
    ASSERT_TRUE(inst0Status.ok);
    ASSERT_FALSE(inst0.tokens.empty());
    std::string tokenId = inst0.tokens[0].token_id;

    // start → task
    ASSERT_TRUE(pgm_->advanceToken(instanceId, tokenId).ok);
    // task → end
    ASSERT_TRUE(pgm_->advanceToken(instanceId, tokenId).ok);
    // end → (no outgoing) — token reaches COMPLETED state
    ASSERT_TRUE(pgm_->advanceToken(instanceId, tokenId).ok);

    // Reload from DB and verify that the full traversal history is intact
    auto [finalStatus, finalInstance] = pgm_->getProcessInstance(instanceId);
    ASSERT_TRUE(finalStatus.ok);
    ASSERT_FALSE(finalInstance.tokens.empty());

    const auto& tok = finalInstance.tokens[0];
    EXPECT_EQ(tok.state, themis::ProcessToken::State::COMPLETED);

    // All nodes must be in visited_nodes
    const auto& vn = tok.visited_nodes;
    EXPECT_NE(std::find(vn.begin(), vn.end(), "start"), vn.end())
        << "visited_nodes missing 'start' after completion";
    EXPECT_NE(std::find(vn.begin(), vn.end(), "task"), vn.end())
        << "visited_nodes missing 'task' after completion";
    EXPECT_NE(std::find(vn.begin(), vn.end(), "end"), vn.end())
        << "visited_nodes missing 'end' after completion";

    // All nodes must have timestamps in visit_timestamps
    EXPECT_TRUE(tok.visit_timestamps.count("start") > 0)
        << "visit_timestamps missing 'start' after completion";
    EXPECT_TRUE(tok.visit_timestamps.count("task") > 0)
        << "visit_timestamps missing 'task' after completion";
    EXPECT_TRUE(tok.visit_timestamps.count("end") > 0)
        << "visit_timestamps missing 'end' after completion";

    // getVisitTimestamp must still return values after completion
    EXPECT_TRUE(pgm_->getVisitTimestamp(instanceId, "start").has_value());
    EXPECT_TRUE(pgm_->getVisitTimestamp(instanceId, "task").has_value());
    EXPECT_TRUE(pgm_->getVisitTimestamp(instanceId, "end").has_value());
}

// ── Tests for setAqlQueryExecutor injection API (stub #56) ─────────────────

TEST_F(ProcessGraphTest, AqlQueryExecutorInjection_queryTasksByFormData) {
    // Register a minimal process
    auto reg = pgm_->registerProcess("aql-inject-test", "AQL Inject Test");
    ASSERT_TRUE(reg.ok) << reg.message;

    nlohmann::json filter;
    filter["status"] = "pending";

    // Without executor: falls back to in-process O(n) scan (returns OK + empty)
    {
        auto [st, tokens] = pgm_->queryTasksByFormData("aql-inject-test", filter);
        EXPECT_TRUE(st.ok) << st.message;
    }

    // With injected executor: should be called and results mapped correctly
    bool executor_called = false;
    pgm_->setAqlQueryExecutor(
        [&](std::string_view aql, const nlohmann::json& bind_vars) -> std::vector<nlohmann::json> {
            executor_called = true;
            EXPECT_NE(aql.find("process_tokens"), std::string::npos);
            EXPECT_EQ(bind_vars.value("pid", std::string{}), "aql-inject-test");
            nlohmann::json row;
            row["token_id"] = "t1";
            row["process_instance_id"] = "i1";
            row["current_node"] = "node1";
            row["state"] = "ACTIVE";
            return {row};
        });

    auto [st2, tokens2] = pgm_->queryTasksByFormData("aql-inject-test", filter);
    EXPECT_TRUE(st2.ok) << st2.message;
    EXPECT_TRUE(executor_called);
    ASSERT_EQ(tokens2.size(), 1u);
    EXPECT_EQ(tokens2[0].token_id, "t1");
    EXPECT_EQ(tokens2[0].state, themis::ProcessToken::State::ACTIVE);
}

TEST_F(ProcessGraphTest, AqlQueryExecutorInjection_aggregateByField) {
    auto reg = pgm_->registerProcess("aql-agg-test", "AQL Agg Test");
    ASSERT_TRUE(reg.ok) << reg.message;

    bool executor_called = false;
    pgm_->setAqlQueryExecutor(
        [&](std::string_view, const nlohmann::json& bind_vars) -> std::vector<nlohmann::json> {
            executor_called = true;
            nlohmann::json row;
            row["group_key"] = "alice";
            row["count"] = 3;
            row["sum"] = 300.0;
            row["min"] = 50.0;
            row["max"] = 150.0;
            return {row};
        });

    auto [st, results] = pgm_->aggregateByField("aql-agg-test", "assignee", "amount", "SUM");
    EXPECT_TRUE(st.ok) << st.message;
    EXPECT_TRUE(executor_called);
    ASSERT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].count, 3u);
    EXPECT_DOUBLE_EQ(results[0].sum, 300.0);
    EXPECT_DOUBLE_EQ(results[0].avg, 100.0);

    // Reset executor to null — falls back to in-process scan (no crash)
    pgm_->setAqlQueryExecutor(nullptr);
    auto [st2, results2] = pgm_->aggregateByField("aql-agg-test", "assignee", "amount", "SUM");
    EXPECT_TRUE(st2.ok) << st2.message;
}
