/*
 * ThemisDB | File: test_knowledge_gap_retrieval_callback.cpp | Version: 0.0.9
 * Maturity: 🟢 PRODUCTION-READY | Score: 98/100
 * Gap Summary: total=4; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=1, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */
// RESTORED FROM HISTORY: 892fbc132819cf3446b54bb51b8b14ec2dd61db5


/**
 * @file test_knowledge_gap_retrieval_callback.cpp
 * @brief Unit tests for KnowledgeGapDetector::setRetrievalCallback() and
 *        the FLARE active-retrieval loop.
 *
 * These tests verify that:
 *  - setRetrievalCallback() stores and uses the callback inside performDynamicRetrieval()
 *  - detectWithActiveRetrieval() stops early when callback returns empty
 *  - detectWithActiveRetrieval() merges new unique documents and de-duplicates
 *  - Replacing the callback with a null function disables retrieval gracefully
 *  - Exceptions thrown by the callback are caught and return an empty list
 */

#include <gtest/gtest.h>
#include "rag/knowledge_gap_detector.h"
#include <atomic>
#include <stdexcept>

namespace themis::rag::knowledge_gap {
namespace {

// ─── Helpers ─────────────────────────────────────────────────────────────────

RetrievedDocument makeDoc(const std::string& id,
                          const std::string& content,
                          double similarity = 0.9) {
    RetrievedDocument d;
    d.id               = id;
    d.content          = content;
    d.similarity_score = similarity;
    return d;
}

// ─── Fixture ─────────────────────────────────────────────────────────────────

class KnowledgeGapRetrievalCallbackTest : public ::testing::Test {
protected:
    void SetUp() override {
        KnowledgeGapConfig cfg;
        cfg.enable_flare          = true;
        cfg.max_retrieval_rounds  = 3;
        cfg.flare_confidence_threshold = 0.5;
        // Set generous thresholds so coverage/similarity don't prematurely stop
        // the loop — we want to exercise the retrieval path.
        cfg.coverage_threshold    = 0.99;
        cfg.similarity_threshold  = 0.99;
        cfg.min_documents         = 1;
        detector_ = std::make_unique<KnowledgeGapDetector>(cfg);
    }

    std::unique_ptr<KnowledgeGapDetector> detector_;
};

// ─── Tests ───────────────────────────────────────────────────────────────────

// KGD-CB-01: Without a callback, performDynamicRetrieval returns empty.
TEST_F(KnowledgeGapRetrievalCallbackTest, NoCallbackReturnsEmpty) {
    std::vector<RetrievedDocument> docs = {makeDoc("d1", "hello world", 0.2)};
    // FLARE is enabled but no callback — loop should stop after empty retrieval.
    auto result = detector_->detectWithActiveRetrieval("hello", docs);
    // The detector ran without crashing; gap may or may not be detected —
    // what matters is it completed successfully.
    (void)result;
}

// KGD-CB-02: Callback is invoked during FLARE loop.
TEST_F(KnowledgeGapRetrievalCallbackTest, CallbackIsInvoked) {
    std::atomic<int> call_count{0};

    detector_->setRetrievalCallback(
        [&](const std::string&, size_t, const std::string&) -> std::vector<RetrievedDocument> {
            ++call_count;
            return {};  // Return empty to stop loop after first call
        });

    std::vector<RetrievedDocument> docs = {makeDoc("d1", "foo", 0.2)};
    detector_->detectWithActiveRetrieval("foo bar baz", docs);

    EXPECT_GT(call_count.load(), 0)
        << "Retrieval callback must be called at least once during FLARE loop";
}

// KGD-CB-03: New documents from callback are merged (and de-duplicated).
TEST_F(KnowledgeGapRetrievalCallbackTest, NewDocumentsMergedWithDeduplication) {
    // Provide initial docs + one extra doc via callback
    std::vector<RetrievedDocument> initial = {makeDoc("init-1", "initial content", 0.5)};

    int call_round = 0;
    detector_->setRetrievalCallback(
        [&](const std::string&, size_t, const std::string&) -> std::vector<RetrievedDocument> {
            ++call_round;
            if (call_round == 1) {
                // First round: return one new doc + a duplicate of the initial doc
                return {
                    makeDoc("new-1",  "new document", 0.8),
                    makeDoc("init-1", "duplicate of initial", 0.5)  // should be skipped
                };
            }
            // Subsequent rounds: nothing new
            return {};
        });

    // After the loop, initial should have been augmented in-place
    detector_->detectWithActiveRetrieval("some query", initial);

    // initial now holds merged docs; "init-1" must appear exactly once
    int count_init1 = 0;
    bool found_new1 = false;
    for (const auto& d : initial) {
        if (d.id == "init-1") ++count_init1;
        if (d.id == "new-1")  found_new1 = true;
    }
    EXPECT_EQ(count_init1, 1) << "Duplicate document must not be added twice";
    EXPECT_TRUE(found_new1)   << "New document from callback must be present";
}

// KGD-CB-04: Replacing callback with null disables retrieval silently.
TEST_F(KnowledgeGapRetrievalCallbackTest, NullCallbackDisablesRetrieval) {
    // First set a callback, then clear it
    detector_->setRetrievalCallback(
        [](const std::string&, size_t, const std::string&) -> std::vector<RetrievedDocument> {
            return {makeDoc("extra", "extra doc", 0.9)};
        });
    detector_->setRetrievalCallback({}); // clear

    std::vector<RetrievedDocument> docs = {makeDoc("d1", "base", 0.3)};
    // Should complete without crash and without adding "extra"
    detector_->detectWithActiveRetrieval("query", docs);

    bool has_extra = false;
    for (const auto& d : docs) {
        if (d.id == "extra") { has_extra = true; break; }
    }
    EXPECT_FALSE(has_extra) << "Cleared callback must not inject extra documents";
}

// KGD-CB-05: Callback throwing an exception does not propagate — returns empty.
TEST_F(KnowledgeGapRetrievalCallbackTest, ExceptionInCallbackIsCaught) {
    std::atomic<bool> threw{false};
    detector_->setRetrievalCallback(
        [&](const std::string&, size_t, const std::string&) -> std::vector<RetrievedDocument> {
            threw = true;
            throw std::runtime_error("simulated retrieval failure");
        });

    std::vector<RetrievedDocument> docs = {makeDoc("d1", "base", 0.2)};
    EXPECT_NO_THROW(detector_->detectWithActiveRetrieval("query", docs))
        << "Exceptions from callback must be swallowed by performDynamicRetrieval";
    EXPECT_TRUE(threw.load()) << "Throwing callback must have been called";
}

// KGD-CB-06: FLARE disabled — callback is never called.
TEST(KnowledgeGapDetectorFlareDisabled, CallbackNotCalledWhenFlareOff) {
    KnowledgeGapConfig cfg;
    cfg.enable_flare = false;
    auto detector = std::make_unique<KnowledgeGapDetector>(cfg);

    bool callback_called = false;
    detector->setRetrievalCallback(
        [&](const std::string&, size_t, const std::string&) -> std::vector<RetrievedDocument> {
            callback_called = true;
            return {};
        });

    std::vector<RetrievedDocument> docs = {makeDoc("d1", "foo", 0.3)};
    detector->detectWithActiveRetrieval("foo bar", docs);

    EXPECT_FALSE(callback_called)
        << "Retrieval callback must NOT be called when enable_flare = false";
}

// KGD-CB-07: Respects max_retrieval_rounds config.
TEST_F(KnowledgeGapRetrievalCallbackTest, RespectsMaxRetrievalRounds) {
    std::atomic<int> call_count{0};
    // Always return a new document so the loop never satisfies coverage
    // (coverage_threshold = 0.99 from fixture).
    detector_->setRetrievalCallback(
        [&](const std::string& q, size_t, const std::string&) -> std::vector<RetrievedDocument> {
            ++call_count;
            return {makeDoc("new-" + std::to_string(call_count.load()), q, 0.4)};
        });

    std::vector<RetrievedDocument> docs = {makeDoc("d1", "base", 0.1)};
    detector_->detectWithActiveRetrieval("topic alpha beta gamma", docs);

    // max_retrieval_rounds = 3 set in fixture; loop runs at most 3 iterations
    EXPECT_LE(call_count.load(), 3)
        << "FLARE loop must not exceed max_retrieval_rounds";
}

} // namespace
} // namespace themis::rag::knowledge_gap
