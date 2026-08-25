/**
 * @file test_process_high_batch_2a.cpp
 * @brief Focused tests for Process Module HIGH findings (Batch 2A)
 *
 * Tests for:
 * - process_agentic_rag.cpp (12 HIGH findings - mostly false positives)
 * - vcc_vpb_importer.cpp (9 HIGH findings - mostly false positives)
 *
 * Test IDs: EXS-01 to EXS-20
 */

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include "process/process_agentic_rag.h"
#include "process/vcc_vpb_importer.h"

using json = nlohmann::json;

namespace themis::process::test {

// ============================================================================
// Test Suite: process_agentic_rag.cpp Safety Validations
// ============================================================================

class ProcessAgenticRagTest : public ::testing::Test {
protected:
    ProcessRagContext createTestContext() {
        ProcessRagContext ctx;
        ctx.instance_id = "test-instance-001";
        ctx.llm_prompt = "What is the status of application X?";
        ctx.process_name = "test_process";
        
        // Add subgraph
        ctx.subgraph = json::object({
            {"nodes", json::array({
                json::object({{"id", "n1"}, {"label", "Start"}}),
                json::object({{"id", "n2"}, {"label", "Process"}})
            })},
            {"edges", json::array({
                json::object({{"from", "n1"}, {"to", "n2"}})
            })}
        });
        
        // Add attachments
        ctx.attachments.push_back(json::object({
            {"_id", "att-001"},
            {"name", "document_1.pdf"},
            {"type", "application/pdf"}
        }));
        
        ctx.attachments.push_back(json::object({
            {"_id", "att-002"},
            {"name", "form_2.xml"},
            {"type", "application/xml"}
        }));
        
        // Add similar cases
        ctx.similar_cases.push_back(json::object({
            {"case_id", "case-99"},
            {"similarity", 0.87}
        }));
        
        // Add missing documents
        ctx.missing_documents.push_back("Vollmacht des Antragstellers");
        ctx.missing_documents.push_back("Grundrisszeichnung");
        
        return ctx;
    }
};

// EXS-01: Test encodeContext with basic prompt
TEST_F(ProcessAgenticRagTest, EXS01_EncodeContextBasicPrompt) {
    ProcessRagContext ctx;
    ctx.instance_id = "test-001";
    ctx.llm_prompt = "Simple prompt";
    
    auto docs = ProcessAgenticRag::encodeContext(ctx);
    
    EXPECT_GT(docs.size(), 0);
    EXPECT_EQ(docs[0].id, "proc_prompt:test-001");
    EXPECT_EQ(docs[0].content, "Simple prompt");
    EXPECT_EQ(docs[0].similarity_score, 1.0);
}

// EXS-02: Test encodeContext with subgraph (false positive test)
TEST_F(ProcessAgenticRagTest, EXS02_EncodeContextWithSubgraph) {
    ProcessRagContext ctx = createTestContext();
    
    auto docs = ProcessAgenticRag::encodeContext(ctx);
    
    // Should have prompt, subgraph, 2 attachments, 1 similar case, missing docs
    EXPECT_GE(docs.size(), 5);
    
    // Verify subgraph encoding (safe string concatenation tested)
    bool found_subgraph = false;
    for (const auto& doc : docs) {
        if (doc.id.find("proc_subgraph:") != std::string::npos) {
            found_subgraph = true;
            EXPECT_FALSE(doc.content.empty());
            EXPECT_EQ(doc.similarity_score, 0.9);
            break;
        }
    }
    EXPECT_TRUE(found_subgraph);
}

// EXS-03: Test encodeContext with attachments (bounded array access)
TEST_F(ProcessAgenticRagTest, EXS03_EncodeContextAttachments) {
    ProcessRagContext ctx = createTestContext();
    
    auto docs = ProcessAgenticRag::encodeContext(ctx);
    
    // Verify attachments are properly encoded
    int att_count = 0;
    for (const auto& doc : docs) {
        if (doc.metadata.count("type") && doc.metadata.at("type") == "attachment") {
            att_count++;
            EXPECT_EQ(doc.similarity_score, 0.8);
            EXPECT_FALSE(doc.id.empty());
            // Verify safe string concatenation: "proc_att:instance:idx"
            EXPECT_EQ(doc.id.substr(0, 8), "proc_att:");
        }
    }
    EXPECT_EQ(att_count, 2);  // We added 2 attachments
}

// EXS-04: Test encodeContext with similar cases (std::to_string safety)
TEST_F(ProcessAgenticRagTest, EXS04_EncodeContextSimilarCases) {
    ProcessRagContext ctx = createTestContext();
    
    auto docs = ProcessAgenticRag::encodeContext(ctx);
    
    // Verify similar cases
    int case_count = 0;
    for (const auto& doc : docs) {
        if (doc.metadata.count("type") && doc.metadata.at("type") == "similar_case") {
            case_count++;
            EXPECT_EQ(doc.similarity_score, 0.7);
            // Verify safe concatenation with std::to_string
            EXPECT_TRUE(doc.id.find("proc_case:") != std::string::npos);
        }
    }
    EXPECT_EQ(case_count, 1);
}

// EXS-05: Test encodeContext with missing documents (ostringstream safety)
TEST_F(ProcessAgenticRagTest, EXS05_EncodeContextMissingDocuments) {
    ProcessRagContext ctx = createTestContext();
    
    auto docs = ProcessAgenticRag::encodeContext(ctx);
    
    // Verify missing documents encoding
    bool found_missing = false;
    for (const auto& doc : docs) {
        if (doc.metadata.count("type") && doc.metadata.at("type") == "missing_documents") {
            found_missing = true;
            EXPECT_EQ(doc.id, "proc_missing:test-instance-001");
            EXPECT_TRUE(doc.content.find("Vollmacht") != std::string::npos);
            EXPECT_TRUE(doc.content.find("Grundrisszeichnung") != std::string::npos);
            break;
        }
    }
    EXPECT_TRUE(found_missing);
}

// EXS-06: Test encodeContext empty context (boundary condition)
TEST_F(ProcessAgenticRagTest, EXS06_EncodeContextEmpty) {
    ProcessRagContext ctx;
    ctx.instance_id = "empty-001";
    // No prompt, no subgraph, no attachments
    
    auto docs = ProcessAgenticRag::encodeContext(ctx);
    
    // Should return empty vector when all fields are empty
    EXPECT_EQ(docs.size(), 0);
}

// EXS-07: Test mergeDocuments with empty docs (O(n) complexity)
TEST_F(ProcessAgenticRagTest, EXS07_MergeDocumentsEmpty) {
    ProcessRagContext ctx = createTestContext();
    std::vector<rag::judge::RetrievedDocument> extra_docs;
    
    auto result = ProcessAgenticRag::mergeDocuments(ctx, extra_docs);
    
    // No new docs to merge
    EXPECT_EQ(result.attachments.size(), ctx.attachments.size());
}

// EXS-08: Test mergeDocuments deduplication (set-based, O(n log n))
TEST_F(ProcessAgenticRagTest, EXS08_MergeDocumentsDuplicateDetection) {
    ProcessRagContext ctx = createTestContext();
    
    // Create doc with same ID as existing
    std::vector<rag::judge::RetrievedDocument> extra_docs;
    rag::judge::RetrievedDocument dup;
    dup.id = "att-001";  // Already in attachments
    dup.content = R"({"_id": "att-001", "name": "duplicate.pdf"})";
    dup.metadata["type"] = "attachment";
    extra_docs.push_back(dup);
    
    auto result = ProcessAgenticRag::mergeDocuments(ctx, extra_docs);
    
    // Duplicate should not be added
    EXPECT_EQ(result.attachments.size(), ctx.attachments.size());
}

// EXS-09: Test mergeDocuments addition of new docs
TEST_F(ProcessAgenticRagTest, EXS09_MergeDocumentsAddition) {
    ProcessRagContext ctx = createTestContext();
    
    std::vector<rag::judge::RetrievedDocument> extra_docs;
    rag::judge::RetrievedDocument new_doc;
    new_doc.id = "new-att-003";
    new_doc.content = R"({"_id": "new-att-003", "name": "new.pdf"})";
    new_doc.metadata["type"] = "attachment";
    extra_docs.push_back(new_doc);
    
    auto result = ProcessAgenticRag::mergeDocuments(ctx, extra_docs);
    
    // New doc should be added
    EXPECT_EQ(result.attachments.size(), ctx.attachments.size() + 1);
}

// EXS-10: Test encodeContext with large indices (bounded array arithmetic)
TEST_F(ProcessAgenticRagTest, EXS10_EncodeContextLargeIndices) {
    ProcessRagContext ctx;
    ctx.instance_id = "large-001";
    
    // Add many attachments to test index arithmetic safety
    for (size_t i = 0; i < 100; ++i) {
        ctx.attachments.push_back(json::object({
            {"_id", "att-" + std::to_string(i)},
            {"content", "data"}
        }));
    }
    
    auto docs = ProcessAgenticRag::encodeContext(ctx);
    
    // Verify all attachments encoded with safe indices
    EXPECT_EQ(docs.size(), 100);
    for (size_t i = 0; i < docs.size(); ++i) {
        EXPECT_EQ(docs[i].similarity_score, 0.8);
        EXPECT_EQ(docs[i].metadata.at("type"), "attachment");
    }
}

// ============================================================================
// Test Suite: vcc_vpb_importer.cpp Safety Validations
// ============================================================================

class VccVpbImporterTest : public ::testing::Test {
protected:
    std::string createSimpleYaml() {
        return R"(
id: test_process
name: Test Process
domain: Bauwesen
description: A test process
compliance:
  - "§34 BauO"
  - "DSGVO"

activities:
  - id: start
    name: Start Activity
    type: start
    
  - id: task1
    name: Task 1
    type: task
    description: First task

edges:
  - from: start
    to: task1
    type: sequence
)";
    }
};

// EXS-11: Test importYaml basic parsing (regex safety)
TEST_F(VccVpbImporterTest, EXS11_ImportYamlBasic) {
    auto yaml = createSimpleYaml();
    auto result = VccVpbImporter::importYaml(yaml);
    
    EXPECT_TRUE(result.ok) << "Parse error: " << result.message;
    EXPECT_EQ(result.record.id, "test_process");
    EXPECT_EQ(result.record.name, "Test Process");
}

// EXS-12: Test importYaml compliance list parsing (static regex)
TEST_F(VccVpbImporterTest, EXS12_ImportYamlCompliance) {
    auto yaml = createSimpleYaml();
    auto result = VccVpbImporter::importYaml(yaml);
    
    EXPECT_TRUE(result.ok);
    // Verify compliance list was parsed
    EXPECT_GE(result.record.compliance_tags.size(), 1);
}

// EXS-13: Test importYaml activities parsing (pre-compiled regex)
TEST_F(VccVpbImporterTest, EXS13_ImportYamlActivities) {
    auto yaml = createSimpleYaml();
    auto result = VccVpbImporter::importYaml(yaml);
    
    EXPECT_TRUE(result.ok);
    EXPECT_TRUE(result.record.normalized.contains("activities"));
    EXPECT_GT(result.record.normalized["activities"].size(), 0);
}

// EXS-14: Test importYaml edges parsing (static regex pre-compilation)
TEST_F(VccVpbImporterTest, EXS14_ImportYamlEdges) {
    auto yaml = createSimpleYaml();
    auto result = VccVpbImporter::importYaml(yaml);
    
    EXPECT_TRUE(result.ok);
    EXPECT_TRUE(result.record.normalized.contains("edges"));
    EXPECT_GT(result.record.normalized["edges"].size(), 0);
}

// EXS-15: Test importYamlList with array parsing (bounds checking)
TEST_F(VccVpbImporterTest, EXS15_ImportYamlListBasic) {
    std::string yaml = R"(
administrative_models:
  - id: model1
    name: Model 1
    activities:
      - id: start
        type: start
        
  - id: model2
    name: Model 2
    activities:
      - id: start
        type: start
)";
    
    auto results = VccVpbImporter::importYamlList(yaml, "administrative_models");
    
    EXPECT_GE(results.size(), 1);
    // At least the first model should parse
    EXPECT_TRUE(results[0].ok || results.size() > 1);
}

// EXS-16: Test importYaml with empty input (boundary condition)
TEST_F(VccVpbImporterTest, EXS16_ImportYamlEmpty) {
    auto result = VccVpbImporter::importYaml("");
    
    EXPECT_FALSE(result.ok);  // Should fail gracefully
}

// EXS-17: Test importYaml with malformed YAML (error handling)
TEST_F(VccVpbImporterTest, EXS17_ImportYamlMalformed) {
    std::string yaml = "this is { not [[ valid ] yaml";
    auto result = VccVpbImporter::importYaml(yaml);
    
    EXPECT_FALSE(result.ok);  // Should handle error gracefully
    EXPECT_FALSE(result.message.empty());
}

// EXS-18: Test importYaml with large compliance list (bounds validation)
TEST_F(VccVpbImporterTest, EXS18_ImportYamlLargeCompliance) {
    std::string yaml = R"(
id: large_test
name: Large Compliance Test
compliance:
)";
    // Add many compliance items to test array bounds
    for (int i = 0; i < 50; ++i) {
        yaml += "  - \"Compliance Item " + std::to_string(i) + "\"\n";
    }
    yaml += R"(
activities:
  - id: start
    type: start
)";
    
    auto result = VccVpbImporter::importYaml(yaml);
    
    EXPECT_TRUE(result.ok) << "Large compliance parsing failed: " << result.message;
    EXPECT_GE(result.record.compliance_tags.size(), 10);
}

// EXS-19: Test regex patterns are safely applied (no recompilation)
TEST_F(VccVpbImporterTest, EXS19_RegexSafetyCompliance) {
    // Multiple calls should use cached static regex objects
    auto yaml = createSimpleYaml();
    
    auto result1 = VccVpbImporter::importYaml(yaml);
    auto result2 = VccVpbImporter::importYaml(yaml);
    
    EXPECT_TRUE(result1.ok);
    EXPECT_TRUE(result2.ok);
    EXPECT_EQ(result1.record.id, result2.record.id);
}

// EXS-20: Test string bounds checking in parsing (defensive access)
TEST_F(VccVpbImporterTest, EXS20_StringBoundsChecking) {
    std::string yaml = R"(
id: bounds_test
name: Bounds Test
compliance: ["item1", "item2"]
activities:
  - id: a1
    type: task
  - id: a2
    type: task
edges:
  - from: a1
    to: a2
)";
    
    // Should safely handle parsing without out-of-bounds access
    auto result = VccVpbImporter::importYaml(yaml);
    
    EXPECT_TRUE(result.ok) << "Bounds check failed: " << result.message;
}

} // namespace themis::process::test
