/**
 * @file test_personalized_ranker.cpp
 * @brief Unit tests for PersonalizedRanker (v2.0.0)
 */

#include <gtest/gtest.h>
#include "search/personalized_ranker.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>
#include <vector>

using namespace themis;

// ============================================================================
// Helpers
// ============================================================================

static UserInteraction makeInteraction(
    const std::string& user_id,
    const std::string& document_id,
    InteractionType type,
    std::chrono::system_clock::time_point ts = std::chrono::system_clock::now()) {

    UserInteraction i;
    i.user_id     = user_id;
    i.document_id = document_id;
    i.type        = type;
    i.timestamp   = ts;
    return i;
}

static RankedResult makeCandidate(const std::string& doc_id, double final_score) {
    RankedResult r;
    r.document_id = doc_id;
    r.final_score = final_score;
    return r;
}

// ============================================================================
// Config validation
// ============================================================================

TEST(PersonalizedRankerConfig, DefaultConfigIsValid) {
    EXPECT_NO_THROW(PersonalizedRanker{});
}

TEST(PersonalizedRankerConfig, NegativeDecayRateThrows) {
    PersonalizedRanker::Config cfg;
    cfg.decay_rate = -0.1;
    EXPECT_THROW(PersonalizedRanker{cfg}, std::invalid_argument);
}

TEST(PersonalizedRankerConfig, ZeroDecayRateIsValid) {
    PersonalizedRanker::Config cfg;
    cfg.decay_rate = 0.0;
    EXPECT_NO_THROW(PersonalizedRanker{cfg});
}

TEST(PersonalizedRankerConfig, ZeroMaxInteractionsThrows) {
    PersonalizedRanker::Config cfg;
    cfg.max_interactions_per_user = 0;
    EXPECT_THROW(PersonalizedRanker{cfg}, std::invalid_argument);
}

TEST(PersonalizedRankerConfig, NegativeBoostWeightThrows) {
    PersonalizedRanker::Config cfg;
    cfg.boost_weight = -0.1;
    EXPECT_THROW(PersonalizedRanker{cfg}, std::invalid_argument);
}

TEST(PersonalizedRankerConfig, ZeroBoostWeightIsValid) {
    PersonalizedRanker::Config cfg;
    cfg.boost_weight = 0.0;
    EXPECT_NO_THROW(PersonalizedRanker{cfg});
}

TEST(PersonalizedRankerConfig, ConfigRoundtrip) {
    PersonalizedRanker::Config cfg;
    cfg.decay_rate = 0.1;
    cfg.max_interactions_per_user = 200;
    cfg.boost_weight = 0.3;
    PersonalizedRanker pr{cfg};
    EXPECT_DOUBLE_EQ(pr.getConfig().decay_rate, 0.1);
    EXPECT_EQ(pr.getConfig().max_interactions_per_user, 200u);
    EXPECT_DOUBLE_EQ(pr.getConfig().boost_weight, 0.3);
}

// ============================================================================
// recordInteraction / userCount / getUserInteractions
// ============================================================================

TEST(PersonalizedRankerRecord, InitialStateEmpty) {
    PersonalizedRanker pr;
    EXPECT_EQ(pr.userCount(), 0u);
    EXPECT_TRUE(pr.getUserInteractions("alice").empty());
}

TEST(PersonalizedRankerRecord, RecordCreatesUser) {
    PersonalizedRanker pr;
    pr.recordInteraction(makeInteraction("alice", "doc_1", InteractionType::CLICK));
    EXPECT_EQ(pr.userCount(), 1u);
}

TEST(PersonalizedRankerRecord, GetUserInteractionsReturnsRecentFirst) {
    PersonalizedRanker pr;
    auto t1 = std::chrono::system_clock::now() - std::chrono::hours(2);
    auto t2 = std::chrono::system_clock::now() - std::chrono::hours(1);
    pr.recordInteraction(makeInteraction("alice", "doc_1", InteractionType::VIEW, t1));
    pr.recordInteraction(makeInteraction("alice", "doc_2", InteractionType::CLICK, t2));

    auto history = pr.getUserInteractions("alice");
    ASSERT_EQ(history.size(), 2u);
    // Most recent first
    EXPECT_EQ(history[0].document_id, "doc_2");
    EXPECT_EQ(history[1].document_id, "doc_1");
}

TEST(PersonalizedRankerRecord, MultipleUsersAreIsolated) {
    PersonalizedRanker pr;
    pr.recordInteraction(makeInteraction("alice", "doc_1", InteractionType::CLICK));
    pr.recordInteraction(makeInteraction("bob",   "doc_2", InteractionType::LIKE));
    EXPECT_EQ(pr.userCount(), 2u);
    EXPECT_EQ(pr.getUserInteractions("alice").size(), 1u);
    EXPECT_EQ(pr.getUserInteractions("bob").size(),   1u);
    EXPECT_TRUE(pr.getUserInteractions("charlie").empty());
}

TEST(PersonalizedRankerRecord, MaxInteractionsPerUserEvictsOldest) {
    PersonalizedRanker::Config cfg;
    cfg.max_interactions_per_user = 3;
    PersonalizedRanker pr{cfg};

    for (int i = 0; i < 5; ++i) {
        auto t = std::chrono::system_clock::now() + std::chrono::seconds(i);
        pr.recordInteraction(
            makeInteraction("alice", "doc_" + std::to_string(i), InteractionType::VIEW, t));
    }

    // Buffer capped at 3 - oldest evicted, most recent 3 kept
    auto history = pr.getUserInteractions("alice");
    ASSERT_EQ(history.size(), 3u);
    // Most recent first; the last 3 recorded were doc_2, doc_3, doc_4
    EXPECT_EQ(history[0].document_id, "doc_4");
    EXPECT_EQ(history[1].document_id, "doc_3");
    EXPECT_EQ(history[2].document_id, "doc_2");
}

// ============================================================================
// computeScore
// ============================================================================

TEST(PersonalizedRankerScore, UnknownUserScoresZero) {
    PersonalizedRanker pr;
    EXPECT_DOUBLE_EQ(pr.computeScore("nobody", "doc_1"), 0.0);
}

TEST(PersonalizedRankerScore, UnknownDocumentScoresZero) {
    PersonalizedRanker pr;
    pr.recordInteraction(makeInteraction("alice", "doc_1", InteractionType::CLICK));
    EXPECT_DOUBLE_EQ(pr.computeScore("alice", "unknown_doc"), 0.0);
}

TEST(PersonalizedRankerScore, ClickProducesPositiveScore) {
    PersonalizedRanker::Config cfg;
    cfg.decay_rate = 0.0; // no decay
    PersonalizedRanker pr{cfg};
    pr.recordInteraction(makeInteraction("alice", "doc_1", InteractionType::CLICK));
    double score = pr.computeScore("alice", "doc_1");
    EXPECT_GT(score, 0.0);
}

TEST(PersonalizedRankerScore, LikeScoresHigherThanView) {
    PersonalizedRanker::Config cfg;
    cfg.decay_rate = 0.0;
    PersonalizedRanker pr{cfg};
    pr.recordInteraction(makeInteraction("alice", "doc_like", InteractionType::LIKE));
    pr.recordInteraction(makeInteraction("alice", "doc_view", InteractionType::VIEW));
    double like_score = pr.computeScore("alice", "doc_like");
    double view_score = pr.computeScore("alice", "doc_view");
    EXPECT_GT(like_score, view_score);
}

TEST(PersonalizedRankerScore, BookmarkScoresHigherThanClick) {
    PersonalizedRanker::Config cfg;
    cfg.decay_rate = 0.0;
    PersonalizedRanker pr{cfg};
    pr.recordInteraction(makeInteraction("alice", "doc_bm", InteractionType::BOOKMARK));
    pr.recordInteraction(makeInteraction("alice", "doc_cl", InteractionType::CLICK));
    EXPECT_GT(pr.computeScore("alice", "doc_bm"), pr.computeScore("alice", "doc_cl"));
}

TEST(PersonalizedRankerScore, DislikeProducesNegativeScore) {
    PersonalizedRanker::Config cfg;
    cfg.decay_rate = 0.0;
    PersonalizedRanker pr{cfg};
    pr.recordInteraction(makeInteraction("alice", "doc_1", InteractionType::DISLIKE));
    EXPECT_LT(pr.computeScore("alice", "doc_1"), 0.0);
}

TEST(PersonalizedRankerScore, ScoreClampedToNegativeOne) {
    PersonalizedRanker::Config cfg;
    cfg.decay_rate = 0.0;
    cfg.max_interactions_per_user = 1000;
    PersonalizedRanker pr{cfg};
    // Record many dislikes to push score well below -1
    for (int i = 0; i < 100; ++i) {
        pr.recordInteraction(makeInteraction("alice", "doc_1", InteractionType::DISLIKE));
    }
    double score = pr.computeScore("alice", "doc_1");
    EXPECT_GE(score, -1.0);
}

TEST(PersonalizedRankerScore, ScoreClampedToPositiveOne) {
    PersonalizedRanker::Config cfg;
    cfg.decay_rate = 0.0;
    cfg.max_interactions_per_user = 1000;
    PersonalizedRanker pr{cfg};
    for (int i = 0; i < 100; ++i) {
        pr.recordInteraction(makeInteraction("alice", "doc_1", InteractionType::LIKE));
    }
    double score = pr.computeScore("alice", "doc_1");
    EXPECT_LE(score, 1.0);
}

TEST(PersonalizedRankerScore, RecentInteractionScoresHigherThanOld) {
    PersonalizedRanker::Config cfg;
    cfg.decay_rate = 0.5; // fast decay for test
    PersonalizedRanker pr{cfg};

    auto now = std::chrono::system_clock::now();
    auto old_ts = now - std::chrono::hours(24 * 30); // 30 days ago
    auto new_ts = now - std::chrono::seconds(60);    // 1 minute ago

    pr.recordInteraction(makeInteraction("alice", "doc_old",  InteractionType::CLICK, old_ts));
    pr.recordInteraction(makeInteraction("alice", "doc_new",  InteractionType::CLICK, new_ts));

    double old_score = pr.computeScore("alice", "doc_old", now);
    double new_score = pr.computeScore("alice", "doc_new", now);
    EXPECT_GT(new_score, old_score);
}

TEST(PersonalizedRankerScore, MultipleInteractionsSameDocAccumulate) {
    PersonalizedRanker::Config cfg;
    cfg.decay_rate = 0.0;
    PersonalizedRanker pr{cfg};

    pr.recordInteraction(makeInteraction("alice", "doc_1", InteractionType::VIEW));
    double score_one = pr.computeScore("alice", "doc_1");

    pr.recordInteraction(makeInteraction("alice", "doc_1", InteractionType::CLICK));
    double score_two = pr.computeScore("alice", "doc_1");

    EXPECT_GT(score_two, score_one);
}

// ============================================================================
// applyPersonalization
// ============================================================================

TEST(PersonalizedRankerApply, EmptyCandidatesNoOp) {
    PersonalizedRanker pr;
    std::vector<RankedResult> empty;
    EXPECT_NO_THROW(pr.applyPersonalization("alice", empty));
    EXPECT_TRUE(empty.empty());
}

TEST(PersonalizedRankerApply, EmptyUserIdNoOp) {
    PersonalizedRanker pr;
    auto candidates = std::vector<RankedResult>{makeCandidate("doc_1", 0.5)};
    double original = candidates[0].final_score;
    pr.applyPersonalization("", candidates);
    EXPECT_DOUBLE_EQ(candidates[0].final_score, original);
}

TEST(PersonalizedRankerApply, NoInteractionNoScoreChange) {
    PersonalizedRanker pr;
    auto candidates = std::vector<RankedResult>{makeCandidate("doc_1", 0.5)};
    double original = candidates[0].final_score;
    pr.applyPersonalization("alice", candidates);
    EXPECT_DOUBLE_EQ(candidates[0].final_score, original);
}

TEST(PersonalizedRankerApply, LikedDocumentBoostIncreasesScore) {
    PersonalizedRanker::Config cfg;
    cfg.decay_rate   = 0.0;
    cfg.boost_weight = 0.2;
    PersonalizedRanker pr{cfg};

    pr.recordInteraction(makeInteraction("alice", "doc_liked", InteractionType::LIKE));

    auto candidates = std::vector<RankedResult>{makeCandidate("doc_liked", 0.5)};
    double before = candidates[0].final_score;
    pr.applyPersonalization("alice", candidates);
    EXPECT_GT(candidates[0].final_score, before);
}

TEST(PersonalizedRankerApply, DislikedDocumentScoreDecreases) {
    PersonalizedRanker::Config cfg;
    cfg.decay_rate   = 0.0;
    cfg.boost_weight = 0.2;
    PersonalizedRanker pr{cfg};

    pr.recordInteraction(makeInteraction("alice", "doc_bad", InteractionType::DISLIKE));

    auto candidates = std::vector<RankedResult>{makeCandidate("doc_bad", 0.5)};
    double before = candidates[0].final_score;
    pr.applyPersonalization("alice", candidates);
    EXPECT_LT(candidates[0].final_score, before);
}

TEST(PersonalizedRankerApply, PersonalizedDocRankedFirst) {
    PersonalizedRanker::Config cfg;
    cfg.decay_rate   = 0.0;
    cfg.boost_weight = 0.5;
    PersonalizedRanker pr{cfg};

    // alice liked doc_B but not doc_A
    pr.recordInteraction(makeInteraction("alice", "doc_B", InteractionType::LIKE));

    // doc_A starts with a slightly higher score
    auto candidates = std::vector<RankedResult>{
        makeCandidate("doc_A", 0.8),
        makeCandidate("doc_B", 0.6)
    };
    pr.applyPersonalization("alice", candidates);

    // After personalization, doc_B should be ranked first (0.6 + 0.5*1.0 > 0.8)
    ASSERT_EQ(candidates.size(), 2u);
    EXPECT_EQ(candidates[0].document_id, "doc_B");
}

TEST(PersonalizedRankerApply, ResultReSortedAfterBoost) {
    PersonalizedRanker::Config cfg;
    cfg.decay_rate   = 0.0;
    cfg.boost_weight = 1.0;
    PersonalizedRanker pr{cfg};

    pr.recordInteraction(makeInteraction("alice", "doc_3", InteractionType::BOOKMARK));

    std::vector<RankedResult> candidates = {
        makeCandidate("doc_1", 0.9),
        makeCandidate("doc_2", 0.8),
        makeCandidate("doc_3", 0.1)
    };
    pr.applyPersonalization("alice", candidates);

    // doc_3 (0.1 + 1.0*1.0 = 1.1) should now rank first
    ASSERT_EQ(candidates.size(), 3u);
    EXPECT_EQ(candidates[0].document_id, "doc_3");
}

// ============================================================================
// clearUser / clear
// ============================================================================

TEST(PersonalizedRankerClear, ClearUserRemovesHistory) {
    PersonalizedRanker pr;
    pr.recordInteraction(makeInteraction("alice", "doc_1", InteractionType::CLICK));
    pr.recordInteraction(makeInteraction("bob",   "doc_2", InteractionType::LIKE));
    EXPECT_EQ(pr.userCount(), 2u);

    pr.clearUser("alice");
    EXPECT_EQ(pr.userCount(), 1u);
    EXPECT_TRUE(pr.getUserInteractions("alice").empty());
    EXPECT_EQ(pr.getUserInteractions("bob").size(), 1u);
}

TEST(PersonalizedRankerClear, ClearUnknownUserIsNoOp) {
    PersonalizedRanker pr;
    EXPECT_NO_THROW(pr.clearUser("nobody"));
    EXPECT_EQ(pr.userCount(), 0u);
}

TEST(PersonalizedRankerClear, ClearAllRemovesAllUsers) {
    PersonalizedRanker pr;
    pr.recordInteraction(makeInteraction("alice", "doc_1", InteractionType::CLICK));
    pr.recordInteraction(makeInteraction("bob",   "doc_2", InteractionType::LIKE));
    pr.clear();
    EXPECT_EQ(pr.userCount(), 0u);
}

TEST(PersonalizedRankerClear, ScoreZeroAfterClearUser) {
    PersonalizedRanker::Config cfg;
    cfg.decay_rate = 0.0;
    PersonalizedRanker pr{cfg};
    pr.recordInteraction(makeInteraction("alice", "doc_1", InteractionType::LIKE));
    EXPECT_GT(pr.computeScore("alice", "doc_1"), 0.0);
    pr.clearUser("alice");
    EXPECT_DOUBLE_EQ(pr.computeScore("alice", "doc_1"), 0.0);
}

// ============================================================================
// Thread safety
// ============================================================================

TEST(PersonalizedRankerThread, ConcurrentRecordAndQuerySafe) {
    PersonalizedRanker pr;
    constexpr int N = 200;

    std::thread writer([&]() {
        for (int i = 0; i < N; ++i) {
            pr.recordInteraction(
                makeInteraction("alice", "doc_" + std::to_string(i % 10),
                                InteractionType::CLICK));
        }
    });

    std::thread reader([&]() {
        for (int i = 0; i < N; ++i) {
            (void)pr.computeScore("alice", "doc_" + std::to_string(i % 10));
        }
    });

    writer.join();
    reader.join();
    EXPECT_LE(pr.getUserInteractions("alice").size(), 500u); // bounded by default
}
