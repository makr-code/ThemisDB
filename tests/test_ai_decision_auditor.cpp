#include <gtest/gtest.h>
#include "llm/ai_decision_auditor.h"
#include "security/mock_key_provider.h"
#include <filesystem>
#include <rocksdb/db.h>
#include <rocksdb/utilities/transaction_db.h>

using namespace themis;
using namespace themis::llm;

class AIDecisionAuditorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup temporary database
        db_path_ = "data/test_ai_audit_db";
        std::filesystem::remove_all(db_path_);
        std::filesystem::create_directories(db_path_);
        
        // Open RocksDB
        rocksdb::TransactionDBOptions txn_options;
        rocksdb::Options options;
        options.create_if_missing = true;
        
        rocksdb::TransactionDB* db_ptr;
        auto s = rocksdb::TransactionDB::Open(
            options, txn_options, db_path_, &db_ptr
        );
        
        ASSERT_TRUE(s.ok()) << "Failed to open database: " << s.ToString();
        db_.reset(db_ptr);
        
        // Create auditor
        auditor_ = std::make_unique<AIDecisionAuditor>(db_.get());
    }
    
    void TearDown() override {
        auditor_.reset();
        db_.reset();
        std::filesystem::remove_all(db_path_);
    }
    
    AIDecisionAudit createTestAudit() {
        AIDecisionAudit audit;
        audit.user_id = "user_123";
        audit.session_id = "session_456";
        audit.query = "What is the capital of France?";
        audit.model_name = "test-model";
        audit.model_version = "1.0";
        audit.response = "Paris";
        audit.confidence_score = 0.95f;
        audit.latency_ms = 150;
        audit.token_count = 25;
        
        audit.reasoning_steps = {
            "Parsed query intent",
            "Identified geographical question",
            "Retrieved fact from knowledge base",
            "Generated response"
        };
        
        audit.key_factors = json::object();
        audit.key_factors["query_type"] = "factual";
        audit.key_factors["knowledge_base"] = "geography";
        
        return audit;
    }
    
    std::string db_path_;
    std::unique_ptr<rocksdb::TransactionDB> db_;
    std::unique_ptr<AIDecisionAuditor> auditor_;
};

TEST_F(AIDecisionAuditorTest, LogDecisionWithCompleteContext) {
    auto audit = createTestAudit();
    
    // Log the decision
    auto stored = auditor_->logDecision(audit);
    
    // Verify ID was generated
    EXPECT_FALSE(stored.decision_id.empty());
    
    // Verify all fields preserved
    EXPECT_EQ(stored.user_id, audit.user_id);
    EXPECT_EQ(stored.query, audit.query);
    EXPECT_EQ(stored.response, audit.response);
    EXPECT_FLOAT_EQ(stored.confidence_score, audit.confidence_score);
    EXPECT_EQ(stored.reasoning_steps.size(), 4);
}

TEST_F(AIDecisionAuditorTest, RetrieveDecisionById) {
    auto audit = createTestAudit();
    auto stored = auditor_->logDecision(audit);
    
    // Retrieve the decision
    auto retrieved = auditor_->getDecision(stored.decision_id);
    
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->decision_id, stored.decision_id);
    EXPECT_EQ(retrieved->query, stored.query);
    EXPECT_EQ(retrieved->response, stored.response);
    EXPECT_FLOAT_EQ(retrieved->confidence_score, stored.confidence_score);
}

TEST_F(AIDecisionAuditorTest, LowConfidenceAutoFlagsForReview) {
    auto audit = createTestAudit();
    audit.confidence_score = 0.5f; // Below 0.7 threshold
    
    auto stored = auditor_->logDecision(audit);
    
    // Should be automatically flagged
    EXPECT_TRUE(stored.requires_human_review);
}

TEST_F(AIDecisionAuditorTest, HighConfidenceNotFlagged) {
    auto audit = createTestAudit();
    audit.confidence_score = 0.95f; // Above threshold
    
    auto stored = auditor_->logDecision(audit);
    
    // Should not be flagged
    EXPECT_FALSE(stored.requires_human_review);
}

TEST_F(AIDecisionAuditorTest, FlagForReview) {
    auto audit = createTestAudit();
    auto stored = auditor_->logDecision(audit);
    
    // Initially not flagged
    EXPECT_FALSE(stored.requires_human_review);
    
    // Flag for review
    bool success = auditor_->flagForReview(stored.decision_id, "Suspicious response");
    EXPECT_TRUE(success);
    
    // Verify flagged
    auto retrieved = auditor_->getDecision(stored.decision_id);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_TRUE(retrieved->requires_human_review);
}

TEST_F(AIDecisionAuditorTest, RecordHumanOverride) {
    auto audit = createTestAudit();
    auto stored = auditor_->logDecision(audit);
    
    // Record override
    bool success = auditor_->recordOverride(
        stored.decision_id,
        "Corrected response",
        "reviewer_789"
    );
    EXPECT_TRUE(success);
    
    // Verify override recorded
    auto retrieved = auditor_->getDecision(stored.decision_id);
    ASSERT_TRUE(retrieved.has_value());
    EXPECT_EQ(retrieved->human_override, "Corrected response");
    EXPECT_EQ(retrieved->reviewer_id, "reviewer_789");
    EXPECT_FALSE(retrieved->requires_human_review); // Should clear flag
}

TEST_F(AIDecisionAuditorTest, GenerateExplanation) {
    auto audit = createTestAudit();
    audit.explanation = ""; // Clear default explanation
    auto stored = auditor_->logDecision(audit);
    
    // Generate explanation
    std::string explanation = auditor_->generateExplanation(stored.decision_id);
    
    EXPECT_FALSE(explanation.empty());
    EXPECT_NE(explanation.find("Query:"), std::string::npos);
    EXPECT_NE(explanation.find("Model:"), std::string::npos);
    EXPECT_NE(explanation.find("Confidence:"), std::string::npos);
}

TEST_F(AIDecisionAuditorTest, QueryByUserFilter) {
    // Create multiple decisions for different users
    auto audit1 = createTestAudit();
    audit1.user_id = "user_A";
    auditor_->logDecision(audit1);
    
    auto audit2 = createTestAudit();
    audit2.user_id = "user_B";
    auditor_->logDecision(audit2);
    
    auto audit3 = createTestAudit();
    audit3.user_id = "user_A";
    auditor_->logDecision(audit3);
    
    // Query for user_A
    AIDecisionAuditor::QueryFilter filter;
    filter.user_id = "user_A";
    
    auto results = auditor_->queryAuditLog(filter);
    
    EXPECT_EQ(results.size(), 2);
    for (const auto& result : results) {
        EXPECT_EQ(result.user_id, "user_A");
    }
}

TEST_F(AIDecisionAuditorTest, QueryByConfidenceRange) {
    // Create decisions with different confidence levels
    auto audit1 = createTestAudit();
    audit1.confidence_score = 0.95f;
    auditor_->logDecision(audit1);
    
    auto audit2 = createTestAudit();
    audit2.confidence_score = 0.6f;
    auditor_->logDecision(audit2);
    
    auto audit3 = createTestAudit();
    audit3.confidence_score = 0.85f;
    auditor_->logDecision(audit3);
    
    // Query for high confidence (>= 0.8)
    AIDecisionAuditor::QueryFilter filter;
    filter.min_confidence = 0.8f;
    
    auto results = auditor_->queryAuditLog(filter);
    
    EXPECT_EQ(results.size(), 2);
    for (const auto& result : results) {
        EXPECT_GE(result.confidence_score, 0.8f);
    }
}

TEST_F(AIDecisionAuditorTest, QueryByReviewFlag) {
    // Create decisions with different review flags
    auto audit1 = createTestAudit();
    audit1.confidence_score = 0.95f; // Won't be flagged
    auditor_->logDecision(audit1);
    
    auto audit2 = createTestAudit();
    audit2.confidence_score = 0.5f; // Will be auto-flagged
    auditor_->logDecision(audit2);
    
    // Query for decisions needing review
    AIDecisionAuditor::QueryFilter filter;
    filter.requires_review = true;
    
    auto results = auditor_->queryAuditLog(filter);
    
    EXPECT_GE(results.size(), 1);
    for (const auto& result : results) {
        EXPECT_TRUE(result.requires_human_review);
    }
}

TEST_F(AIDecisionAuditorTest, GetStatistics) {
    // Create multiple decisions
    auto audit1 = createTestAudit();
    audit1.confidence_score = 0.9f;
    auditor_->logDecision(audit1);
    
    auto audit2 = createTestAudit();
    audit2.confidence_score = 0.6f; // Will be flagged
    auditor_->logDecision(audit2);
    
    auto audit3 = createTestAudit();
    audit3.confidence_score = 0.8f;
    auto stored3 = auditor_->logDecision(audit3);
    
    // Override one decision
    auditor_->recordOverride(stored3.decision_id, "Override", "reviewer");
    
    // Get stats
    auto stats = auditor_->getStats();
    
    EXPECT_EQ(stats.total_decisions, 3);
    EXPECT_GE(stats.flagged_for_review, 1);
    EXPECT_EQ(stats.human_overrides, 1);
    EXPECT_GT(stats.avg_confidence, 0.0f);
}

TEST_F(AIDecisionAuditorTest, ExportForCompliance) {
    // Create test decisions
    auto audit = createTestAudit();
    auditor_->logDecision(audit);
    
    std::string export_path = "data/test_compliance_export.json";
    std::error_code cleanup_ec = {};
    std::filesystem::remove(export_path, cleanup_ec);
    
    // Export
    AIDecisionAuditor::QueryFilter filter;
    bool success = auditor_->exportForCompliance(export_path, filter);
    
    EXPECT_TRUE(success);
    EXPECT_TRUE(std::filesystem::exists(export_path));
    
    // Verify export contains valid JSON
    std::ifstream ifs(export_path);
    json export_data = json::parse(ifs);
    ifs.close();
    
    EXPECT_TRUE(export_data.contains("total_decisions"));
    EXPECT_TRUE(export_data.contains("decisions"));
    EXPECT_GT(export_data["total_decisions"].get<int>(), 0);
    
    std::filesystem::remove(export_path, cleanup_ec);
}

TEST_F(AIDecisionAuditorTest, VerifyIntegrity) {
    auto audit = createTestAudit();
    auto stored = auditor_->logDecision(audit);
    
    // Verify integrity (should pass even without PKI client for basic hash)
    bool valid = auditor_->verifyIntegrity(stored.decision_id);
    EXPECT_TRUE(valid);
}

TEST_F(AIDecisionAuditorTest, SerializationRoundTrip) {
    auto audit = createTestAudit();
    
    // Serialize to JSON
    json j = audit.toJson();
    
    // Deserialize back
    auto deserialized = AIDecisionAudit::fromJson(j);
    
    // Verify fields match
    EXPECT_EQ(deserialized.user_id, audit.user_id);
    EXPECT_EQ(deserialized.query, audit.query);
    EXPECT_EQ(deserialized.response, audit.response);
    EXPECT_FLOAT_EQ(deserialized.confidence_score, audit.confidence_score);
    EXPECT_EQ(deserialized.reasoning_steps.size(), audit.reasoning_steps.size());
}
