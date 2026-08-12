/**
 * @file test_prompt_version_control_diff.cpp
 * @brief Tests for the improved LCS-based diff, three-way merge, and
 *        TF-IDF pattern extraction (issue 4.1, 4.2, 4.3)
 */

#include <gtest/gtest.h>
#include "prompt_engineering/prompt_version_control.h"
#include "prompt_engineering/feedback_collector.h"

using namespace themis::prompt_engineering;

// ============================================================================
// Helper: create a PromptVersionControl and commit a version
// ============================================================================

static PromptVersionControl make_vc() {
    return PromptVersionControl(nullptr, nullptr);
}

static std::string commit(PromptVersionControl& vc,
                          const std::string& prompt_id,
                          const std::string& branch,
                          const std::string& content,
                          const std::string& message,
                          const std::string& parent = "") {
    return vc.commit(prompt_id, branch, content, message, "test-author", parent);
}

// ============================================================================
// 4.1 LCS-based computeDiff
// ============================================================================

TEST(PromptDiffTest, IdenticalContent_ZeroDelta) {
    auto vc = make_vc();
    const std::string pid = "p1";
    auto v1 = commit(vc, pid, "main", "line1\nline2\nline3\n", "v1");
    auto v2 = commit(vc, pid, "main", "line1\nline2\nline3\n", "v2", v1);

    auto d = vc.diff(v1, v2);
    EXPECT_EQ(d.additions, 0u);
    EXPECT_EQ(d.deletions, 0u);
    EXPECT_TRUE(d.added_lines.empty());
    EXPECT_TRUE(d.removed_lines.empty());
}

TEST(PromptDiffTest, AddOneLine_OnlyAddition) {
    auto vc = make_vc();
    const std::string pid = "p2";
    auto v1 = commit(vc, pid, "main", "alpha\nbeta\n", "v1");
    auto v2 = commit(vc, pid, "main", "alpha\nbeta\ngamma\n", "v2", v1);

    auto d = vc.diff(v1, v2);
    EXPECT_EQ(d.additions, 1u);
    EXPECT_EQ(d.deletions, 0u);
    ASSERT_EQ(d.added_lines.size(), 1u);
    EXPECT_EQ(d.added_lines[0], "gamma");
}

TEST(PromptDiffTest, RemoveOneLine_OnlyDeletion) {
    auto vc = make_vc();
    const std::string pid = "p3";
    auto v1 = commit(vc, pid, "main", "alpha\nbeta\ngamma\n", "v1");
    auto v2 = commit(vc, pid, "main", "alpha\ngamma\n", "v2", v1);

    auto d = vc.diff(v1, v2);
    EXPECT_EQ(d.additions, 0u);
    EXPECT_EQ(d.deletions, 1u);
    ASSERT_EQ(d.removed_lines.size(), 1u);
    EXPECT_EQ(d.removed_lines[0], "beta");
}

TEST(PromptDiffTest, ReplaceLine_OneAddOneRemove) {
    auto vc = make_vc();
    const std::string pid = "p4";
    auto v1 = commit(vc, pid, "main", "x\ny\nz\n", "v1");
    auto v2 = commit(vc, pid, "main", "x\nY\nz\n", "v2", v1);

    auto d = vc.diff(v1, v2);
    EXPECT_EQ(d.additions, 1u);
    EXPECT_EQ(d.deletions, 1u);
    EXPECT_EQ(d.removed_lines[0], "y");
    EXPECT_EQ(d.added_lines[0], "Y");
}

TEST(PromptDiffTest, OrderIsPreserved) {
    // The OLD set-based diff lost ordering; the LCS diff must respect order.
    // Content A has lines [a, b, c, b]; content B has [a, c, b].
    // Removal should be the first 'b' (index 1), not the last.
    auto vc = make_vc();
    const std::string pid = "p5";
    auto v1 = commit(vc, pid, "main", "a\nb\nc\nb\n", "v1");
    auto v2 = commit(vc, pid, "main", "a\nc\nb\n", "v2", v1);

    auto d = vc.diff(v1, v2);
    // Net: one 'b' removed
    EXPECT_EQ(d.deletions, 1u);
    EXPECT_EQ(d.additions, 0u);
}

TEST(PromptDiffTest, UnifiedDiff_ContainsHunkHeader) {
    auto vc = make_vc();
    const std::string pid = "p6";
    auto v1 = commit(vc, pid, "main", "a\nb\nc\n", "v1");
    auto v2 = commit(vc, pid, "main", "a\nB\nc\n", "v2", v1);

    auto d = vc.diff(v1, v2);
    EXPECT_NE(d.unified_diff.find("@@"), std::string::npos)
        << "unified_diff must contain hunk header @@";
    EXPECT_NE(d.unified_diff.find("---"), std::string::npos);
    EXPECT_NE(d.unified_diff.find("+++"), std::string::npos);
    EXPECT_NE(d.unified_diff.find("-b"), std::string::npos);
    EXPECT_NE(d.unified_diff.find("+B"), std::string::npos);
}

TEST(PromptDiffTest, UnifiedDiff_HasContextLines) {
    auto vc = make_vc();
    const std::string pid = "p7";
    // Single change in the middle; context lines (a, c) must appear with ' '
    auto v1 = commit(vc, pid, "main", "a\nb\nc\n", "v1");
    auto v2 = commit(vc, pid, "main", "a\nB\nc\n", "v2", v1);

    auto d = vc.diff(v1, v2);
    EXPECT_NE(d.unified_diff.find(" a"), std::string::npos) << "context line 'a' expected";
    EXPECT_NE(d.unified_diff.find(" c"), std::string::npos) << "context line 'c' expected";
}

// ============================================================================
// 4.2 Three-way autoMerge
// ============================================================================

TEST(PromptMergeTest, FastForward_OnlySourceChanged) {
    auto vc = make_vc();
    const std::string pid = "m1";

    // Create base on 'main'
    auto base_id = commit(vc, pid, "main", "line1\nline2\n", "base");
    // source (feature) changes line2 → line2-modified
    auto src_id  = commit(vc, pid, "feature", "line1\nline2-modified\n", "src", base_id);
    // target is untouched from base
    // We test merge strategy 'auto' which should fast-forward to source
    auto result = vc.merge(pid, "feature", "main", "auto", "merge feature");
    EXPECT_TRUE(result.success);
    EXPECT_NE(result.merged_content.find("line2-modified"), std::string::npos);
}

TEST(PromptMergeTest, FastForward_OnlyTargetChanged) {
    auto vc = make_vc();
    const std::string pid = "m2";

    auto base_id = commit(vc, pid, "main", "base-line\n", "base");
    // feature branch stays at base
    commit(vc, pid, "feature", "base-line\n", "no-change", base_id);
    // main gets a new commit
    commit(vc, pid, "main", "base-line\nmain-addition\n", "main-change", base_id);

    auto result = vc.merge(pid, "feature", "main", "auto", "merge");
    EXPECT_TRUE(result.success);
}

TEST(PromptMergeTest, TwoSidesChangeDifferentLines_Success) {
    auto vc = make_vc();
    const std::string pid = "m3";

    auto base_id = commit(vc, pid, "main", "intro\nbody\noutro\n", "base");
    // source changes intro
    auto src_id  = commit(vc, pid, "feature", "INTRO\nbody\noutro\n", "src", base_id);
    // target changes outro
    auto tgt_id  = commit(vc, pid, "main", "intro\nbody\nOUTRO\n", "tgt", base_id);

    auto result = vc.merge(pid, "feature", "main", "auto", "merge");
    // Both sides changed different lines → should succeed
    EXPECT_TRUE(result.success);
}

TEST(PromptMergeTest, StrategyOurs_AlwaysSucceeds) {
    auto vc = make_vc();
    const std::string pid = "m4";

    auto base_id = commit(vc, pid, "main", "base\n", "base");
    commit(vc, pid, "feature", "src-change\n", "src", base_id);
    commit(vc, pid, "main", "tgt-change\n", "tgt", base_id);

    auto result = vc.merge(pid, "feature", "main", "ours", "merge-ours");
    EXPECT_TRUE(result.success);
    EXPECT_NE(result.merged_content.find("tgt-change"), std::string::npos);
}

TEST(PromptMergeTest, StrategyTheirs_UsesSourceContent) {
    auto vc = make_vc();
    const std::string pid = "m5";

    auto base_id = commit(vc, pid, "main", "base\n", "base");
    commit(vc, pid, "feature", "feature-content\n", "src", base_id);
    commit(vc, pid, "main", "main-content\n", "tgt", base_id);

    auto result = vc.merge(pid, "feature", "main", "theirs", "merge-theirs");
    EXPECT_TRUE(result.success);
    EXPECT_NE(result.merged_content.find("feature-content"), std::string::npos);
}

TEST(PromptMergeTest, LCA_FindsTrueCommonAncestor) {
    // Build a diamond history:
    //   root → v1 (main)
    //   root → v1 → v2 (main advances again)
    //   root → v1 → vF (feature branches from v1)
    //   Merge feature into main: LCA should be v1, not root.
    auto vc = make_vc();
    const std::string pid = "m6";

    auto root = commit(vc, pid, "main",    "root\n",      "root");
    auto v1   = commit(vc, pid, "main",    "root\nv1\n",  "v1", root);
    // feature branches from v1
    auto vF   = commit(vc, pid, "feature", "root\nv1\nfeature\n", "feat", v1);
    // main advances one more commit
    auto v2   = commit(vc, pid, "main",    "root\nv1\nmain-extra\n", "main-extra", v1);

    auto result = vc.merge(pid, "feature", "main", "auto", "merge");
    // Both sides changed from v1 (the real LCA):
    //   feature added  "feature"
    //   main    added  "main-extra"
    // These touch different positions, so merge should succeed.
    EXPECT_TRUE(result.success)
        << "LCA-based merge should succeed for non-overlapping changes";
    EXPECT_NE(result.merged_content.find("feature"),    std::string::npos);
    EXPECT_NE(result.merged_content.find("main-extra"), std::string::npos);
    (void)vF; (void)v2;
}

TEST(PromptMergeTest, AppendedLines_PreservedInMerge) {
    // Both sides append distinct lines after all base content.
    // The end-of-file insertion fix ensures neither side's appended lines are lost.
    auto vc = make_vc();
    const std::string pid = "m7";

    auto base_id = commit(vc, pid, "main",    "common\n",                  "base");
    auto src_id  = commit(vc, pid, "feature", "common\nfrom-feature\n",    "src", base_id);
    auto tgt_id  = commit(vc, pid, "main",    "common\nfrom-main\n",       "tgt", base_id);

    auto result = vc.merge(pid, "feature", "main", "auto", "merge");
    EXPECT_TRUE(result.success);
    EXPECT_NE(result.merged_content.find("from-feature"), std::string::npos)
        << "Feature's appended line must survive the merge";
    EXPECT_NE(result.merged_content.find("from-main"),    std::string::npos)
        << "Main's appended line must survive the merge";
    (void)src_id; (void)tgt_id;
}

// ============================================================================
// 4.3 TF-IDF pattern extraction
// ============================================================================

class PatternExtractionTest : public ::testing::Test {
protected:
    void SetUp() override {
        collector_ = std::make_unique<FeedbackCollector>(nullptr, nullptr);
    }
    std::unique_ptr<FeedbackCollector> collector_;
};

TEST_F(PatternExtractionTest, StopWords_NotUsedAsPatterns) {
    const std::string pid = "pe1";

    // If pattern extraction used stop words like "the", "is", "a" as keys,
    // all queries would be grouped under the same trivial pattern.
    // With stop-word filtering, each query should produce a meaningful keyword.
    collector_->recordFeedback(pid, "the database is slow", "r",
                               FeedbackType::USER_NEGATIVE, "", 0.8);
    collector_->recordFeedback(pid, "the database is slow", "r",
                               FeedbackType::USER_NEGATIVE, "", 0.8);
    collector_->recordFeedback(pid, "the database is slow", "r",
                               FeedbackType::USER_NEGATIVE, "", 0.8);

    auto patterns = collector_->analyzeFailurePatterns(pid, 1);
    ASSERT_FALSE(patterns.empty());

    // The pattern keyword must NOT be a stop word
    static const std::vector<std::string> stop_words = {"the","is","a","an"};
    for (const auto& p : patterns) {
        for (const auto& sw : stop_words) {
            EXPECT_NE(p.pattern, sw)
                << "Pattern should not be a stop word, got: " << p.pattern;
        }
    }
}

TEST_F(PatternExtractionTest, SimilarQueries_GroupedUnderSameKeyword) {
    const std::string pid = "pe2";

    // All queries share the distinctive word "timeout" — it should be
    // the top-scoring TF-IDF keyword and group them together.
    for (int i = 0; i < 5; ++i) {
        collector_->recordFeedback(pid,
            "connection timeout exceeded on request " + std::to_string(i),
            "r", FeedbackType::TIMEOUT, "", 0.9);
    }

    auto patterns = collector_->analyzeFailurePatterns(pid, 2);
    ASSERT_FALSE(patterns.empty());
    // "timeout" is the most distinctive shared keyword
    EXPECT_EQ(patterns[0].pattern, "timeout");
    EXPECT_GE(patterns[0].occurrences, 2u);
}

TEST_F(PatternExtractionTest, DistinctTopics_SeparatePatterns) {
    const std::string pid = "pe3";

    // Two groups of queries with different distinctive keywords
    for (int i = 0; i < 3; ++i) {
        collector_->recordFeedback(pid, "authentication failed for user", "r",
                                   FeedbackType::VALIDATION_FAILED, "", 0.7);
    }
    for (int i = 0; i < 3; ++i) {
        collector_->recordFeedback(pid, "memory allocation exceeded limit", "r",
                                   FeedbackType::PERFORMANCE_ISSUE, "", 0.8);
    }

    auto patterns = collector_->analyzeFailurePatterns(pid, 1);
    // Should have at least 2 distinct patterns (one per topic)
    EXPECT_GE(patterns.size(), 2u);
}

TEST_F(PatternExtractionTest, MinOccurrences_FiltersLowFrequency) {
    const std::string pid = "pe4";

    // Only one entry → should be filtered out with min_occurrences=2
    collector_->recordFeedback(pid, "unique obscure error xyz", "r",
                               FeedbackType::USER_NEGATIVE, "", 0.5);

    auto patterns = collector_->analyzeFailurePatterns(pid, 2);
    EXPECT_TRUE(patterns.empty());
}

TEST_F(PatternExtractionTest, EmptyQuery_HandledGracefully) {
    const std::string pid = "pe5";
    collector_->recordFeedback(pid, "", "r", FeedbackType::USER_NEGATIVE, "", 0.5);
    collector_->recordFeedback(pid, "", "r", FeedbackType::USER_NEGATIVE, "", 0.5);

    EXPECT_NO_THROW({
        auto patterns = collector_->analyzeFailurePatterns(pid, 1);
        // Empty queries should map to "[empty]" pattern
        if (!patterns.empty()) {
            EXPECT_EQ(patterns[0].pattern, "[empty]");
        }
    });
}

TEST_F(PatternExtractionTest, SortedByOccurrencesDescending) {
    const std::string pid = "pe6";

    for (int i = 0; i < 5; ++i)
        collector_->recordFeedback(pid, "timeout connection request", "r",
                                   FeedbackType::TIMEOUT, "", 0.9);
    for (int i = 0; i < 2; ++i)
        collector_->recordFeedback(pid, "parsing error malformed json", "r",
                                   FeedbackType::PARSE_ERROR, "", 0.6);

    auto patterns = collector_->analyzeFailurePatterns(pid, 1);
    ASSERT_GE(patterns.size(), 2u);
    EXPECT_GE(patterns[0].occurrences, patterns[1].occurrences);
}
