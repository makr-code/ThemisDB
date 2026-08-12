/**
 * @file test_feedback_collector.cpp
 * @brief Unit tests for FeedbackCollector
 */

#include <gtest/gtest.h>
#include "prompt_engineering/feedback_collector.h"

using namespace themis::prompt_engineering;

class FeedbackCollectorTest : public ::testing::Test {
protected:
    void SetUp() override {
        collector_ = std::make_unique<FeedbackCollector>();
    }
    
    std::unique_ptr<FeedbackCollector> collector_;
};

TEST_F(FeedbackCollectorTest, RecordAndRetrieveFeedback) {
    std::string prompt_id = "test_prompt_1";
    
    // Record some feedback
    std::string id1 = collector_->recordFeedback(
        prompt_id,
        "What is AI?",
        "AI stands for Artificial Intelligence...",
        FeedbackType::USER_POSITIVE,
        "Very helpful!"
    );
    
    std::string id2 = collector_->recordFeedback(
        prompt_id,
        "Explain quantum computing",
        "Quantum computing uses qubits...",
        FeedbackType::USER_NEGATIVE,
        "Too technical"
    );
    
    EXPECT_FALSE(id1.empty());
    EXPECT_FALSE(id2.empty());
    EXPECT_NE(id1, id2);
    
    // Retrieve feedback
    auto feedback = collector_->getFeedback(prompt_id);
    EXPECT_EQ(feedback.size(), 2);
}

TEST_F(FeedbackCollectorTest, FeedbackTypeConversion) {
    // Test type to string
    EXPECT_EQ(feedbackTypeToString(FeedbackType::USER_POSITIVE), "USER_POSITIVE");
    EXPECT_EQ(feedbackTypeToString(FeedbackType::HALLUCINATION_DETECTED), "HALLUCINATION_DETECTED");
    EXPECT_EQ(feedbackTypeToString(FeedbackType::TIMEOUT), "TIMEOUT");
    
    // Test string to type
    auto type1 = stringToFeedbackType("USER_NEGATIVE");
    ASSERT_TRUE(type1.has_value());
    EXPECT_EQ(type1.value(), FeedbackType::USER_NEGATIVE);
    
    auto type2 = stringToFeedbackType("PARSE_ERROR");
    ASSERT_TRUE(type2.has_value());
    EXPECT_EQ(type2.value(), FeedbackType::PARSE_ERROR);
    
    // Test invalid string
    auto type3 = stringToFeedbackType("INVALID_TYPE");
    EXPECT_FALSE(type3.has_value());
}

TEST_F(FeedbackCollectorTest, GetStats) {
    std::string prompt_id = "test_prompt_2";
    
    // Record mixed feedback
    collector_->recordFeedback(prompt_id, "query1", "response1", FeedbackType::USER_POSITIVE);
    collector_->recordFeedback(prompt_id, "query2", "response2", FeedbackType::USER_POSITIVE);
    collector_->recordFeedback(prompt_id, "query3", "response3", FeedbackType::USER_POSITIVE);
    collector_->recordFeedback(prompt_id, "query4", "response4", FeedbackType::USER_NEGATIVE);
    collector_->recordFeedback(prompt_id, "query5", "response5", FeedbackType::HALLUCINATION_DETECTED);
    collector_->recordFeedback(prompt_id, "query6", "response6", FeedbackType::PARSE_ERROR);
    
    auto stats = collector_->getStats(prompt_id);
    
    EXPECT_EQ(stats.prompt_id, prompt_id);
    EXPECT_EQ(stats.total_feedback, 6);
    EXPECT_DOUBLE_EQ(stats.positive_ratio, 0.5); // 3/6
    EXPECT_DOUBLE_EQ(stats.negative_ratio, 1.0/6.0); // 1/6
    EXPECT_EQ(stats.hallucination_count, 1);
    EXPECT_EQ(stats.error_count, 1);
}

TEST_F(FeedbackCollectorTest, FilterByType) {
    std::string prompt_id = "test_prompt_3";
    
    // Record different types
    collector_->recordFeedback(prompt_id, "q1", "r1", FeedbackType::USER_POSITIVE);
    collector_->recordFeedback(prompt_id, "q2", "r2", FeedbackType::USER_NEGATIVE);
    collector_->recordFeedback(prompt_id, "q3", "r3", FeedbackType::HALLUCINATION_DETECTED);
    collector_->recordFeedback(prompt_id, "q4", "r4", FeedbackType::USER_NEGATIVE);
    collector_->recordFeedback(prompt_id, "q5", "r5", FeedbackType::TIMEOUT);
    
    // Filter for negative feedback only
    auto negative = collector_->getFeedback(prompt_id, 0, FeedbackType::USER_NEGATIVE);
    EXPECT_EQ(negative.size(), 2);
    
    // Filter for hallucinations only
    auto hallucinations = collector_->getFeedback(prompt_id, 0, FeedbackType::HALLUCINATION_DETECTED);
    EXPECT_EQ(hallucinations.size(), 1);
}

TEST_F(FeedbackCollectorTest, LimitResults) {
    std::string prompt_id = "test_prompt_4";
    
    // Record 10 feedback entries
    for (int i = 0; i < 10; ++i) {
        collector_->recordFeedback(
            prompt_id,
            "query" + std::to_string(i),
            "response" + std::to_string(i),
            FeedbackType::USER_POSITIVE
        );
    }
    
    // Get with limit
    auto limited = collector_->getFeedback(prompt_id, 5);
    EXPECT_EQ(limited.size(), 5);
    
    // Get all
    auto all = collector_->getFeedback(prompt_id, 0);
    EXPECT_EQ(all.size(), 10);
}

TEST_F(FeedbackCollectorTest, GetPromptsWithNegativeFeedback) {
    // Create prompts with different feedback ratios
    
    // Good prompt: 90% positive
    for (int i = 0; i < 9; ++i) {
        collector_->recordFeedback("good_prompt", "q", "r", FeedbackType::USER_POSITIVE);
    }
    collector_->recordFeedback("good_prompt", "q", "r", FeedbackType::USER_NEGATIVE);
    
    // Bad prompt: 60% negative
    for (int i = 0; i < 6; ++i) {
        collector_->recordFeedback("bad_prompt", "q", "r", FeedbackType::USER_NEGATIVE);
    }
    for (int i = 0; i < 4; ++i) {
        collector_->recordFeedback("bad_prompt", "q", "r", FeedbackType::USER_POSITIVE);
    }
    
    // Mediocre prompt: 30% negative
    for (int i = 0; i < 3; ++i) {
        collector_->recordFeedback("mediocre_prompt", "q", "r", FeedbackType::USER_NEGATIVE);
    }
    for (int i = 0; i < 7; ++i) {
        collector_->recordFeedback("mediocre_prompt", "q", "r", FeedbackType::USER_POSITIVE);
    }
    
    // Get prompts with >= 30% negative feedback
    auto prompts = collector_->getPromptsWithNegativeFeedback(0.3, 10);
    
    // Should include bad_prompt (60%) and mediocre_prompt (30%)
    EXPECT_EQ(prompts.size(), 2);
    EXPECT_TRUE(std::find(prompts.begin(), prompts.end(), "bad_prompt") != prompts.end());
    EXPECT_TRUE(std::find(prompts.begin(), prompts.end(), "mediocre_prompt") != prompts.end());
}

TEST_F(FeedbackCollectorTest, GetFailedQueries) {
    std::string prompt_id = "test_prompt_5";
    
    // Record failures and successes
    collector_->recordFeedback(prompt_id, "failed1", "bad response", FeedbackType::USER_NEGATIVE);
    collector_->recordFeedback(prompt_id, "success", "good response", FeedbackType::USER_POSITIVE);
    collector_->recordFeedback(prompt_id, "failed2", "error", FeedbackType::PARSE_ERROR);
    collector_->recordFeedback(prompt_id, "failed3", "hallucinated", FeedbackType::HALLUCINATION_DETECTED);
    
    auto failed = collector_->getFailedQueries(prompt_id);
    
    // Should only include failures (not USER_POSITIVE)
    EXPECT_EQ(failed.size(), 3);
    
    // Check first failure
    EXPECT_EQ(std::get<0>(failed[0]), "failed1");
    EXPECT_EQ(std::get<1>(failed[0]), "bad response");
    EXPECT_EQ(std::get<2>(failed[0]), FeedbackType::USER_NEGATIVE);
}

TEST_F(FeedbackCollectorTest, AnalyzeFailurePatterns) {
    std::string prompt_id = "test_prompt_6";
    
    // Record failures with similar patterns
    collector_->recordFeedback(prompt_id, "how to install", "...", FeedbackType::USER_NEGATIVE);
    collector_->recordFeedback(prompt_id, "how to configure", "...", FeedbackType::USER_NEGATIVE);
    collector_->recordFeedback(prompt_id, "how to setup", "...", FeedbackType::USER_NEGATIVE);
    collector_->recordFeedback(prompt_id, "what is the", "...", FeedbackType::PARSE_ERROR);
    collector_->recordFeedback(prompt_id, "what is a", "...", FeedbackType::PARSE_ERROR);
    collector_->recordFeedback(prompt_id, "what is an", "...", FeedbackType::PARSE_ERROR);
    
    auto patterns = collector_->analyzeFailurePatterns(prompt_id, 3);
    
    // Should identify patterns based on first few words
    EXPECT_GE(patterns.size(), 1);
    
    // Check pattern details
    if (!patterns.empty()) {
        EXPECT_GE(patterns[0].occurrences, 3);
        EXPECT_FALSE(patterns[0].examples.empty());
    }
}

TEST_F(FeedbackCollectorTest, GetFeedbackInTimeRange) {
    std::string prompt_id = "test_prompt_7";
    
    auto now = std::chrono::system_clock::now();
    auto one_hour_ago = now - std::chrono::hours(1);
    auto two_hours_ago = now - std::chrono::hours(2);
    
    // Record feedback (will have current timestamp)
    collector_->recordFeedback(prompt_id, "recent", "...", FeedbackType::USER_POSITIVE);
    
    // Get feedback in time range
    auto recent = collector_->getFeedbackInTimeRange(prompt_id, one_hour_ago, now);
    EXPECT_EQ(recent.size(), 1);
    
    // Get feedback in past time range (should be empty)
    auto old = collector_->getFeedbackInTimeRange(prompt_id, two_hours_ago, one_hour_ago);
    EXPECT_EQ(old.size(), 0);
}

TEST_F(FeedbackCollectorTest, SeverityScoring) {
    std::string prompt_id = "test_prompt_8";
    
    // Record feedback with different severities
    collector_->recordFeedback(prompt_id, "q1", "r1", FeedbackType::USER_NEGATIVE, "", 0.3);
    collector_->recordFeedback(prompt_id, "q2", "r2", FeedbackType::USER_NEGATIVE, "", 0.7);
    collector_->recordFeedback(prompt_id, "q3", "r3", FeedbackType::USER_NEGATIVE, "", 0.9);
    
    auto feedback = collector_->getFeedback(prompt_id);
    
    EXPECT_EQ(feedback.size(), 3);
    EXPECT_DOUBLE_EQ(feedback[0].severity, 0.3);
    EXPECT_DOUBLE_EQ(feedback[1].severity, 0.7);
    EXPECT_DOUBLE_EQ(feedback[2].severity, 0.9);
}

TEST_F(FeedbackCollectorTest, ClearFeedback) {
    std::string prompt_id = "test_prompt_9";
    
    // Record some feedback
    collector_->recordFeedback(prompt_id, "q1", "r1", FeedbackType::USER_POSITIVE);
    collector_->recordFeedback(prompt_id, "q2", "r2", FeedbackType::USER_POSITIVE);
    collector_->recordFeedback(prompt_id, "q3", "r3", FeedbackType::USER_POSITIVE);
    
    auto feedback_before = collector_->getFeedback(prompt_id);
    EXPECT_EQ(feedback_before.size(), 3);
    
    // Clear feedback
    size_t cleared = collector_->clearFeedback(prompt_id);
    EXPECT_EQ(cleared, 3);
    
    auto feedback_after = collector_->getFeedback(prompt_id);
    EXPECT_EQ(feedback_after.size(), 0);
}

TEST_F(FeedbackCollectorTest, GetSummary) {
    // Record feedback for multiple prompts
    collector_->recordFeedback("prompt1", "q", "r", FeedbackType::USER_POSITIVE);
    collector_->recordFeedback("prompt1", "q", "r", FeedbackType::USER_NEGATIVE);
    collector_->recordFeedback("prompt2", "q", "r", FeedbackType::HALLUCINATION_DETECTED);
    collector_->recordFeedback("prompt2", "q", "r", FeedbackType::PARSE_ERROR);
    collector_->recordFeedback("prompt3", "q", "r", FeedbackType::USER_POSITIVE);
    
    auto summary = collector_->getSummary();
    
    EXPECT_EQ(summary["total_prompts_tracked"].get<size_t>(), 3);
    EXPECT_EQ(summary["total_feedback"].get<size_t>(), 5);
    EXPECT_EQ(summary["positive_feedback"].get<size_t>(), 2);
    EXPECT_EQ(summary["negative_feedback"].get<size_t>(), 1);
    EXPECT_EQ(summary["hallucinations"].get<size_t>(), 1);
    EXPECT_EQ(summary["errors"].get<size_t>(), 1);
}

TEST_F(FeedbackCollectorTest, FeedbackEntrySerialization) {
    FeedbackEntry entry;
    entry.id = "test_id";
    entry.prompt_id = "prompt_123";
    entry.type = FeedbackType::HALLUCINATION_DETECTED;
    entry.query = "test query";
    entry.response = "test response";
    entry.feedback_text = "detected hallucination";
    entry.severity = 0.8;
    entry.metadata = {{"key", "value"}};
    entry.timestamp = std::chrono::system_clock::now();
    
    // Serialize to JSON
    auto json = entry.toJson();
    
    EXPECT_EQ(json["id"], "test_id");
    EXPECT_EQ(json["prompt_id"], "prompt_123");
    EXPECT_EQ(json["type"], "HALLUCINATION_DETECTED");
    EXPECT_EQ(json["query"], "test query");
    EXPECT_EQ(json["severity"], 0.8);
    
    // Deserialize from JSON
    auto reconstructed = FeedbackEntry::fromJson(json);
    
    EXPECT_EQ(reconstructed.id, entry.id);
    EXPECT_EQ(reconstructed.prompt_id, entry.prompt_id);
    EXPECT_EQ(reconstructed.type, entry.type);
    EXPECT_EQ(reconstructed.query, entry.query);
    EXPECT_DOUBLE_EQ(reconstructed.severity, entry.severity);
}

TEST_F(FeedbackCollectorTest, FeedbackStatsSerialization) {
    std::string prompt_id = "test_prompt_10";
    
    // Record feedback to generate stats
    collector_->recordFeedback(prompt_id, "q1", "r1", FeedbackType::USER_POSITIVE);
    collector_->recordFeedback(prompt_id, "q2", "r2", FeedbackType::USER_NEGATIVE);
    collector_->recordFeedback(prompt_id, "q3", "r3", FeedbackType::HALLUCINATION_DETECTED);
    
    auto stats = collector_->getStats(prompt_id);
    auto json = stats.toJson();
    
    EXPECT_EQ(json["prompt_id"], prompt_id);
    EXPECT_EQ(json["total_feedback"].get<size_t>(), 3);
    EXPECT_TRUE(json.contains("counts_by_type"));
    EXPECT_TRUE(json.contains("positive_ratio"));
    EXPECT_TRUE(json.contains("common_issues"));
}

TEST_F(FeedbackCollectorTest, MetadataStorage) {
    std::string prompt_id = "test_prompt_11";
    
    nlohmann::json metadata = {
        {"user_id", "user123"},
        {"session_id", "session456"},
        {"context", "testing"}
    };
    
    collector_->recordFeedback(
        prompt_id,
        "query with metadata",
        "response",
        FeedbackType::USER_POSITIVE,
        "",
        0.5,
        metadata
    );
    
    auto feedback = collector_->getFeedback(prompt_id);
    ASSERT_EQ(feedback.size(), 1);
    
    EXPECT_EQ(feedback[0].metadata["user_id"], "user123");
    EXPECT_EQ(feedback[0].metadata["session_id"], "session456");
    EXPECT_EQ(feedback[0].metadata["context"], "testing");
}

// ─────────────────────────────────────────────────────────────────────────────
// DK-5: Cross-shard feedback sync tests (FC-CSS-01, FC-CSS-02)
// ─────────────────────────────────────────────────────────────────────────────

#include "distributed_knowledge/cross_shard_feedback_sync.h"

using namespace themis::distributed_knowledge;

// FC-CSS-01: recordFeedback() with sync + embedding model → publishFeedback() called
TEST(FeedbackCollectorCrossShardTest, FC_CSS_01_RecordFeedback_WithSync_PublishesSummary) {
    using namespace themis::prompt_engineering;

    int publish_call_count = 0;

    FeedbackSyncConfig sync_cfg;
    sync_cfg.max_embedding_dim   = 3; // small dimension for test
    sync_cfg.validate_embedding_dim = true;

    auto sync = std::make_shared<CrossShardFeedbackSync>(
        sync_cfg, "shard-local",
        [&publish_call_count](nlohmann::json /*payload*/) {
            ++publish_call_count;
        });

    // Mock embedding model: returns exactly 3 floats
    struct FixedEmbed : public FeedbackCollector::IEmbeddingModel {
        std::vector<float> embed(const std::string&) const override {
            return {0.1f, 0.2f, 0.3f};
        }
    };

    FeedbackCollector collector;
    collector.setCrossShardSync(sync);
    collector.setEmbeddingModel(std::make_shared<FixedEmbed>());

    collector.recordFeedback("p1", "query text", "response", FeedbackType::USER_NEGATIVE);

    EXPECT_EQ(publish_call_count, 1) << "publishFeedback() must be called exactly once";
    EXPECT_GE(sync->publishedCount(), 1u);
}

// FC-CSS-02: recordFeedback() without sync → no crash, local recording intact
TEST(FeedbackCollectorCrossShardTest, FC_CSS_02_RecordFeedback_NoSync_LocalRecordingComplete) {
    using namespace themis::prompt_engineering;

    FeedbackCollector collector; // no setCrossShardSync / setEmbeddingModel

    // Must not throw
    std::string id;
    EXPECT_NO_THROW(
        id = collector.recordFeedback("p1", "query", "response",
                                      FeedbackType::USER_POSITIVE));

    EXPECT_FALSE(id.empty()) << "Feedback ID must still be generated";
    auto entries = collector.getFeedback("p1");
    ASSERT_EQ(entries.size(), 1u);
    EXPECT_EQ(entries[0].type, FeedbackType::USER_POSITIVE);
}

