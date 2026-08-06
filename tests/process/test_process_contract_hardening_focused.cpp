/**
 * @file test_process_contract_hardening_focused.cpp
 * @brief Phase 2 contract-hardening tests for the process module.
 * @note Tests for model validation, serializer bounds checking, and deterministic behavior.
 * @note Test IDs: PRC-01..PRC-50+
 */

#include <gtest/gtest.h>
#include "process/process_api_contract.h"
#include "process/process_model_manager.h"
#include "process/bpmn_serializer.h"
#include "storage/rocksdb_wrapper.h"

#include <algorithm>
#include <cstdint>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

using namespace themis::process;
using json = nlohmann::json;

// ─────────────────────────────────────────────────────────────────────────────
// Error Code Contract Tests (PRC-01 to PRC-08)
// ─────────────────────────────────────────────────────────────────────────────

TEST(ProcessContractTest, PRC01_ErrorCodesUnique) {
    std::vector<int32_t> codes = {
        static_cast<int32_t>(ProcError::kUnsupportedElement),
        static_cast<int32_t>(ProcError::kInvalidTransition),
        static_cast<int32_t>(ProcError::kSerialiserFailed),
        static_cast<int32_t>(ProcError::kDeserialiserFailed),
        static_cast<int32_t>(ProcError::kExecutionTimeout),
    };
    std::sort(codes.begin(), codes.end());
    EXPECT_EQ(std::unique(codes.begin(), codes.end()), codes.end());
}

TEST(ProcessContractTest, PRC02_ErrorCodesInRange) {
    auto check = [](ProcError e) {
        int32_t v = static_cast<int32_t>(e);
        EXPECT_GE(v, 7600); EXPECT_LE(v, 7699);
    };
    check(ProcError::kUnsupportedElement);
    check(ProcError::kInvalidTransition);
    check(ProcError::kSerialiserFailed);
    check(ProcError::kDeserialiserFailed);
    check(ProcError::kExecutionTimeout);
}

TEST(ProcessContractTest, PRC03_UnsupportedDistinctFromInvalidTransition) {
    EXPECT_NE(static_cast<int32_t>(ProcError::kUnsupportedElement),
              static_cast<int32_t>(ProcError::kInvalidTransition));
}

TEST(ProcessContractTest, PRC04_SerialiserDistinctFromDeserialiser) {
    EXPECT_NE(static_cast<int32_t>(ProcError::kSerialiserFailed),
              static_cast<int32_t>(ProcError::kDeserialiserFailed));
}

TEST(ProcessContractTest, PRC05_ExecutionTimeoutIsHighestCode) {
    int32_t to = static_cast<int32_t>(ProcError::kExecutionTimeout);
    EXPECT_GE(to, static_cast<int32_t>(ProcError::kUnsupportedElement));
    EXPECT_GE(to, static_cast<int32_t>(ProcError::kInvalidTransition));
}

TEST(ProcessContractTest, PRC06_ErrorSwitchDispatch) {
    ProcError err = ProcError::kDeserialiserFailed;
    bool handled = false;
    switch (err) {
        case ProcError::kUnsupportedElement:  break;
        case ProcError::kInvalidTransition:   break;
        case ProcError::kSerialiserFailed:    break;
        case ProcError::kDeserialiserFailed:  handled = true; break;
        case ProcError::kExecutionTimeout:    break;
    }
    EXPECT_TRUE(handled);
}

TEST(ProcessContractTest, PRC07_UnsupportedElementLowestCode) {
    int32_t v = static_cast<int32_t>(ProcError::kUnsupportedElement);
    EXPECT_EQ(v, 7600);
}

TEST(ProcessContractTest, PRC08_ExecutionTimeoutCode) {
    int32_t v = static_cast<int32_t>(ProcError::kExecutionTimeout);
    EXPECT_EQ(v, 7604);
}

// ─────────────────────────────────────────────────────────────────────────────
// Process Model Validation Tests (PRC-20 to PRC-40)
// ─────────────────────────────────────────────────────────────────────────────

class ProcessModelHardeningTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a temporary database for testing
        temp_db_path_ = "/tmp/themis_test_proc_" + std::to_string(getpid()) + ".db";
        db_ = std::make_unique<themis::RocksDBWrapper>(temp_db_path_);
        manager_ = std::make_unique<ProcessModelManager>(*db_);
    }

    void TearDown() override {
        manager_.reset();
        db_.reset();
    }

    std::unique_ptr<themis::RocksDBWrapper> db_;
    std::unique_ptr<ProcessModelManager> manager_;
    std::string temp_db_path_;
};

TEST_F(ProcessModelHardeningTest, PRC20_ValidateEmptyId) {
    ProcessModelRecord record;
    record.id = "";  // Empty ID
    record.name = "Test";
    record.version = "1.0.0";
    record.normalized = json::object();
    record.normalized["nodes"] = json::array();
    record.normalized["edges"] = json::array();

    auto result = manager_->validateModelConsistency(record);
    EXPECT_FALSE(result.ok);
    EXPECT_THAT(result.message, ::testing::HasSubstr("id must not be empty"));
}

TEST_F(ProcessModelHardeningTest, PRC21_ValidateEmptyName) {
    ProcessModelRecord record;
    record.id = "test_model";
    record.name = "";  // Empty name
    record.version = "1.0.0";
    record.normalized = json::object();
    record.normalized["nodes"] = json::array();
    record.normalized["edges"] = json::array();

    auto result = manager_->validateModelConsistency(record);
    EXPECT_FALSE(result.ok);
    EXPECT_THAT(result.message, ::testing::HasSubstr("name must not be empty"));
}

TEST_F(ProcessModelHardeningTest, PRC22_ValidateValidModel) {
    ProcessModelRecord record;
    record.id = "test_model";
    record.name = "Test Model";
    record.version = "1.0.0";
    record.normalized = json::object();
    
    json node1;
    node1["id"] = "node1";
    node1["name"] = "Start";
    json node2;
    node2["id"] = "node2";
    node2["name"] = "End";
    record.normalized["nodes"] = json::array({node1, node2});
    
    json edge;
    edge["id"] = "edge1";
    edge["from"] = "node1";
    edge["to"] = "node2";
    record.normalized["edges"] = json::array({edge});

    auto result = manager_->validateModelConsistency(record);
    EXPECT_TRUE(result.ok) << result.message;
}

TEST_F(ProcessModelHardeningTest, PRC23_ValidateDuplicateNodeIds) {
    ProcessModelRecord record;
    record.id = "test_model";
    record.name = "Test Model";
    record.version = "1.0.0";
    record.normalized = json::object();
    
    json node1;
    node1["id"] = "node1";
    json node2;
    node2["id"] = "node1";  // Duplicate
    record.normalized["nodes"] = json::array({node1, node2});
    record.normalized["edges"] = json::array();

    auto result = manager_->validateModelConsistency(record);
    EXPECT_FALSE(result.ok);
    EXPECT_THAT(result.message, ::testing::HasSubstr("Duplicate node id"));
}

TEST_F(ProcessModelHardeningTest, PRC24_ValidateDanglingEdge) {
    ProcessModelRecord record;
    record.id = "test_model";
    record.name = "Test Model";
    record.version = "1.0.0";
    record.normalized = json::object();
    
    json node1;
    node1["id"] = "node1";
    record.normalized["nodes"] = json::array({node1});
    
    json edge;
    edge["id"] = "edge1";
    edge["from"] = "node1";
    edge["to"] = "node99";  // Non-existent target
    record.normalized["edges"] = json::array({edge});

    auto result = manager_->validateModelConsistency(record);
    EXPECT_FALSE(result.ok);
    EXPECT_THAT(result.message, ::testing::HasSubstr("non-existent"));
}

TEST_F(ProcessModelHardeningTest, PRC25_ValidateNameLengthLimit) {
    ProcessModelRecord record;
    record.id = "test_model";
    record.name = std::string(2000, 'x');  // Exceeds 1000 char limit
    record.version = "1.0.0";
    record.normalized = json::object();
    record.normalized["nodes"] = json::array();
    record.normalized["edges"] = json::array();

    auto result = manager_->validateModelConsistency(record);
    EXPECT_FALSE(result.ok);
    EXPECT_THAT(result.message, ::testing::HasSubstr("maximum length"));
}

TEST_F(ProcessModelHardeningTest, PRC26_ConsistencyDiagnostics) {
    // Add a valid model
    ProcessModelRecord record;
    record.id = "model_1";
    record.name = "Model 1";
    record.version = "1.0.0";
    record.notation = ProcessNotation::BPMN_2_0;
    record.normalized = json::object();
    record.normalized["nodes"] = json::array();
    record.normalized["edges"] = json::array();
    
    auto save_result = manager_->save(record);
    EXPECT_TRUE(save_result.ok);

    // Get diagnostics
    auto diag = manager_->getConsistencyDiagnostics();
    EXPECT_TRUE(diag["coherency_ok"].is_boolean());
    EXPECT_TRUE(diag["total_models"].is_number());
    EXPECT_GE(diag["total_models"].get<int>(), 1);
}

// ─────────────────────────────────────────────────────────────────────────────
// BPMN Serializer Validation Tests (PRC-30 to PRC-45)
// ─────────────────────────────────────────────────────────────────────────────

TEST(BpmnSerializerHardeningTest, PRC30_ValidateEmptyNodes) {
    std::vector<ProcessNodeInfo> nodes;
    std::vector<ProcessEdgeInfo> edges;
    
    std::string error = BpmnSerializer::validateStructure(nodes, edges);
    EXPECT_TRUE(error.empty());  // Empty is valid
}

TEST(BpmnSerializerHardeningTest, PRC31_ValidateNodeWithEmptyId) {
    ProcessNodeInfo node;
    node.node_id = "";  // Empty ID
    node.name = "Test Node";
    node.node_type = BPMNNodeType::TASK;
    
    std::vector<ProcessNodeInfo> nodes = {node};
    std::vector<ProcessEdgeInfo> edges;
    
    std::string error = BpmnSerializer::validateStructure(nodes, edges);
    EXPECT_THAT(error, ::testing::HasSubstr("empty id"));
}

TEST(BpmnSerializerHardeningTest, PRC32_ValidateDuplicateNodeIds) {
    ProcessNodeInfo node1, node2;
    node1.node_id = "node1";
    node1.name = "Node 1";
    node1.node_type = BPMNNodeType::TASK;
    node2.node_id = "node1";  // Duplicate
    node2.name = "Node 1 Copy";
    node2.node_type = BPMNNodeType::TASK;
    
    std::vector<ProcessNodeInfo> nodes = {node1, node2};
    std::vector<ProcessEdgeInfo> edges;
    
    std::string error = BpmnSerializer::validateStructure(nodes, edges);
    EXPECT_THAT(error, ::testing::HasSubstr("Duplicate"));
}

TEST(BpmnSerializerHardeningTest, PRC33_ValidateDanglingEdge) {
    ProcessNodeInfo node;
    node.node_id = "node1";
    node.name = "Node 1";
    node.node_type = BPMNNodeType::START_EVENT;
    
    ProcessEdgeInfo edge;
    edge.edge_id = "edge1";
    edge.from_node = "node1";
    edge.to_node = "node99";  // Non-existent
    edge.edge_type = ProcessEdgeType::SEQUENCE_FLOW;
    
    std::vector<ProcessNodeInfo> nodes = {node};
    std::vector<ProcessEdgeInfo> edges_vec = {edge};
    
    std::string error = BpmnSerializer::validateStructure(nodes, edges_vec);
    EXPECT_THAT(error, ::testing::HasSubstr("non-existent"));
}

TEST(BpmnSerializerHardeningTest, PRC34_ValidateExcessiveNodes) {
    std::vector<ProcessNodeInfo> nodes;
    for (size_t i = 0; i <= 10000; ++i) {  // Exceed limit
        ProcessNodeInfo n;
        n.node_id = "node_" + std::to_string(i);
        n.name = "Node";
        n.node_type = BPMNNodeType::TASK;
        nodes.push_back(n);
    }
    
    std::string error = BpmnSerializer::validateStructure(nodes, {});
    EXPECT_THAT(error, ::testing::HasSubstr("exceeds maximum"));
}

TEST(BpmnSerializerHardeningTest, PRC35_ValidateValidStructure) {
    ProcessNodeInfo node1, node2;
    node1.node_id = "node1";
    node1.name = "Start";
    node1.node_type = BPMNNodeType::START_EVENT;
    node2.node_id = "node2";
    node2.name = "End";
    node2.node_type = BPMNNodeType::END_EVENT;
    
    ProcessEdgeInfo edge;
    edge.edge_id = "edge1";
    edge.from_node = "node1";
    edge.to_node = "node2";
    edge.edge_type = ProcessEdgeType::SEQUENCE_FLOW;
    
    std::vector<ProcessNodeInfo> nodes = {node1, node2};
    std::vector<ProcessEdgeInfo> edges = {edge};
    
}

// ─────────────────────────────────────────────────────────────────────────────
// Phase 3: Comprehensive Error Handling and Diagnostics Tests (PRC-50 to PRC-80)
// ─────────────────────────────────────────────────────────────────────────────

// Test the new comprehensive error taxonomy
TEST(ProcessErrorTaxonomyTest, P3_ErrorCodeToStringMapping) {
    EXPECT_EQ(errorCodeToString(ProcessErrorCode::EMPTY_INPUT), "EMPTY_INPUT");
    EXPECT_EQ(errorCodeToString(ProcessErrorCode::MALFORMED_INPUT), "MALFORMED_INPUT");
    EXPECT_EQ(errorCodeToString(ProcessErrorCode::INPUT_TOO_LARGE), "INPUT_TOO_LARGE");
    EXPECT_EQ(errorCodeToString(ProcessErrorCode::FILE_READ_ERROR), "FILE_READ_ERROR");
    EXPECT_EQ(errorCodeToString(ProcessErrorCode::SEMANTIC_VIOLATION), "SEMANTIC_VIOLATION");
    EXPECT_EQ(errorCodeToString(ProcessErrorCode::UNSUPPORTED_ELEMENT), "UNSUPPORTED_ELEMENT");
}

TEST(ProcessErrorTaxonomyTest, P3_ErrorCodeCategoryMapping) {
    // Import category (7600-7609)
    EXPECT_EQ(errorCodeCategory(ProcessErrorCode::EMPTY_INPUT), "IMPORT");
    EXPECT_EQ(errorCodeCategory(ProcessErrorCode::MALFORMED_INPUT), "IMPORT");
    EXPECT_EQ(errorCodeCategory(ProcessErrorCode::INPUT_TOO_LARGE), "IMPORT");
    
    // Lifecycle category (7610-7619)
    EXPECT_EQ(errorCodeCategory(ProcessErrorCode::MODEL_NOT_FOUND), "LIFECYCLE");
    EXPECT_EQ(errorCodeCategory(ProcessErrorCode::MODEL_PERSISTENCE_FAILED), "LIFECYCLE");
    
    // Retrieval category (7620-7629)
    EXPECT_EQ(errorCodeCategory(ProcessErrorCode::RETRIEVAL_MODEL_NOT_FOUND), "RETRIEVAL");
    EXPECT_EQ(errorCodeCategory(ProcessErrorCode::GRAPH_TRAVERSAL_FAILED), "RETRIEVAL");
    
    // System category (7650-7699)
    EXPECT_EQ(errorCodeCategory(ProcessErrorCode::DATABASE_ERROR), "SYSTEM");
    EXPECT_EQ(errorCodeCategory(ProcessErrorCode::OPERATION_TIMEOUT), "SYSTEM");
}

TEST(ProcessErrorTaxonomyTest, P3_ErrorCodeRanges) {
    // Verify all error codes are within valid range
    std::vector<ProcessErrorCode> all_codes = {
        ProcessErrorCode::EMPTY_INPUT,
        ProcessErrorCode::MALFORMED_INPUT,
        ProcessErrorCode::INPUT_TOO_LARGE,
        ProcessErrorCode::FILE_READ_ERROR,
        ProcessErrorCode::MODEL_NOT_FOUND,
        ProcessErrorCode::RETRIEVAL_MODEL_NOT_FOUND,
        ProcessErrorCode::DATABASE_ERROR,
        ProcessErrorCode::INTERNAL_ERROR,
    };
    
    for (auto code : all_codes) {
        int32_t c = static_cast<int32_t>(code);
        EXPECT_GE(c, 7600) << "Code " << c << " is below 7600";
        EXPECT_LE(c, 7699) << "Code " << c << " is above 7699";
    }
}

TEST(ProcessErrorTaxonomyTest, P3_DiagnosticMessageFormatting) {
    std::string diag = formatDiagnostic(
        ProcessErrorCode::MALFORMED_INPUT,
        "import BPMN from XML",
        "line 42: unexpected token"
    );
    
    EXPECT_THAT(diag, ::testing::HasSubstr("IMPORT"));
    EXPECT_THAT(diag, ::testing::HasSubstr("MALFORMED_INPUT"));
    EXPECT_THAT(diag, ::testing::HasSubstr("import BPMN from XML"));
    EXPECT_THAT(diag, ::testing::HasSubstr("line 42"));
}

TEST(ProcessErrorTaxonomyTest, P3_DiagnosticMessageWithoutDetail) {
    std::string diag = formatDiagnostic(
        ProcessErrorCode::EMPTY_INPUT,
        "import process model"
    );
    
    EXPECT_THAT(diag, ::testing::HasSubstr("IMPORT"));
    EXPECT_THAT(diag, ::testing::HasSubstr("EMPTY_INPUT"));
    EXPECT_THAT(diag, ::testing::HasSubstr("import process model"));
}

// BPMN Import Malformed Input Handling Tests
TEST(BpmnImportErrorHandlingTest, P3_EmptyInputReturnsEmptyInputError) {
    auto result = BpmnSerializer::importXml("");
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error_code, ProcessErrorCode::EMPTY_INPUT);
    EXPECT_THAT(result.message, ::testing::HasSubstr("EMPTY_INPUT"));
    EXPECT_THAT(result.message, ::testing::HasSubstr("empty"));
    EXPECT_TRUE(result.nodes.empty());
    EXPECT_TRUE(result.edges.empty());
}

TEST(BpmnImportErrorHandlingTest, P3_OversizedInputReturnsInputTooLargeError) {
    // Create an XML document larger than 10 MiB
    std::string oversized(11u * 1024u * 1024u, 'x');
    oversized.front() = '<';
    oversized.back() = '>';
    
    auto result = BpmnSerializer::importXml(oversized);
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error_code, ProcessErrorCode::INPUT_TOO_LARGE);
    EXPECT_THAT(result.message, ::testing::HasSubstr("INPUT_TOO_LARGE"));
    EXPECT_THAT(result.message, ::testing::HasSubstr("10 MiB"));
}

TEST(BpmnImportErrorHandlingTest, P3_MalformedXmlReturnsMalformedInputError) {
    std::string malformed = "<process><task id=\"t1\"></process>";  // Unclosed tag
    auto result = BpmnSerializer::importXml(malformed);
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error_code, ProcessErrorCode::MALFORMED_INPUT);
    EXPECT_THAT(result.message, ::testing::HasSubstr("MALFORMED_INPUT"));
}

TEST(BpmnImportErrorHandlingTest, P3_NoElementsReturnsEmptyInputError) {
    std::string empty_process = "<?xml version=\"1.0\"?><bpmn2:definitions xmlns:bpmn2=\"http://www.omg.org/spec/BPMN/20100524/MODEL\"><process id=\"proc1\" /></bpmn2:definitions>";
    auto result = BpmnSerializer::importXml(empty_process);
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error_code, ProcessErrorCode::EMPTY_INPUT);
    EXPECT_THAT(result.message, ::testing::HasSubstr("EMPTY_INPUT"));
}

TEST(BpmnImportErrorHandlingTest, P3_FileNotFoundReturnsFileReadError) {
    auto result = BpmnSerializer::importFile("/nonexistent/file/path/bpmn.xml");
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error_code, ProcessErrorCode::FILE_READ_ERROR);
    EXPECT_THAT(result.message, ::testing::HasSubstr("FILE_READ_ERROR"));
}

TEST(BpmnImportErrorHandlingTest, P3_SuccessfulImportReturnsValidResult) {
    std::string valid_bpmn = R"(<?xml version="1.0"?>
<bpmn2:definitions xmlns:bpmn2="http://www.omg.org/spec/BPMN/20100524/MODEL">
  <process id="proc1" name="Test Process">
    <startEvent id="start1" name="Start"/>
    <endEvent id="end1" name="End"/>
    <sequenceFlow id="flow1" sourceRef="start1" targetRef="end1"/>
  </process>
</bpmn2:definitions>)";
    
    auto result = BpmnSerializer::importXml(valid_bpmn);
    EXPECT_TRUE(result.ok) << result.message;
    EXPECT_EQ(result.process_id, "proc1");
    EXPECT_EQ(result.process_name, "Test Process");
    EXPECT_GT(result.nodes.size(), 0);
}

TEST(BpmnImportErrorHandlingTest, P3_NoSilentFailures) {
    // Verify that each possible error path returns an explicit error code
    std::vector<std::pair<std::string, ProcessErrorCode>> test_cases = {
        {"", ProcessErrorCode::EMPTY_INPUT},
        {"x", ProcessErrorCode::MALFORMED_INPUT},
        {"<", ProcessErrorCode::MALFORMED_INPUT},
    };
    
    for (const auto& [input, expected_code] : test_cases) {
        auto result = BpmnSerializer::importXml(input);
        EXPECT_FALSE(result.ok) << "Input '" << input << "' should fail";
        EXPECT_EQ(result.error_code, expected_code) 
            << "Input '" << input << "' should have error code " 
            << static_cast<int32_t>(expected_code);
        EXPECT_FALSE(result.message.empty()) << "Error message should not be empty";
    }
}

// Diagnostic Message Quality Tests
TEST(ProcessDiagnosticsTest, P3_DiagnosticsIncludeActionableContext) {
    auto diag = formatDiagnostic(
        ProcessErrorCode::MALFORMED_INPUT,
        "parse BPMN workflow",
        "XML line 15: unexpected closing tag"
    );
    
    // Verify diagnostic includes:
    // 1. Error category
    EXPECT_THAT(diag, ::testing::HasSubstr("IMPORT"));
    // 2. Error code name
    EXPECT_THAT(diag, ::testing::HasSubstr("MALFORMED_INPUT"));
    // 3. Operation context
    EXPECT_THAT(diag, ::testing::HasSubstr("parse BPMN"));
    // 4. Specific detail
    EXPECT_THAT(diag, ::testing::HasSubstr("line 15"));
}

TEST(ProcessDiagnosticsTest, P3_DiagnosticsAreActionable) {
    // Verify that operators can triage incidents from diagnostic messages
    std::vector<std::string> diags = {
        formatDiagnostic(ProcessErrorCode::EMPTY_INPUT, "import model"),
        formatDiagnostic(ProcessErrorCode::FILE_READ_ERROR, "load BPMN", "/path/to/file.xml"),
        formatDiagnostic(ProcessErrorCode::MALFORMED_INPUT, "parse XML", "bad syntax"),
        formatDiagnostic(ProcessErrorCode::MODEL_NOT_FOUND, "retrieve model", "model-id-xyz"),
    };
    
    for (const auto& diag : diags) {
        // Each diagnostic should indicate what failed, why, and what to check
        EXPECT_FALSE(diag.empty());
        EXPECT_THAT(diag, ::testing::HasSubstr("["));
        EXPECT_THAT(diag, ::testing::HasSubstr("/"));  // Category/Code separator
        EXPECT_THAT(diag, ::testing::HasSubstr("]"));
    }
}

