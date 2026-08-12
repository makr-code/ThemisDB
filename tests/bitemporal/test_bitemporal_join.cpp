// RESTORED FROM HISTORY: 892fbc132819cf3446b54bb51b8b14ec2dd61db5


#include <gtest/gtest.h>
#include "temporal/bitemporal_join.h"

namespace themisdb {
namespace temporal {
namespace {

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static BiTemporalRow makeRow(const std::string& key,
                              Timestamp valid_from, Timestamp valid_to,
                              Timestamp sys_from  = 0,
                              Timestamp sys_to    = kMaxTimestamp) {
    BiTemporalRow r;
    r.key        = key;
    r.valid_time = {valid_from, valid_to};
    r.sys_time   = {sys_from, sys_to};
    r.payload    = {{"key", key}};
    return r;
}

// ─────────────────────────────────────────────────────────────────────────────
// Static predicate helpers
// ─────────────────────────────────────────────────────────────────────────────

TEST(BiTemporalJoinPredicates, OverlapsTrue) {
    EXPECT_TRUE(BiTemporalJoin::overlaps({0, 10}, {5, 15}));
}

TEST(BiTemporalJoinPredicates, OverlapsFalseAdjacent) {
    EXPECT_FALSE(BiTemporalJoin::overlaps({0, 10}, {10, 20}));
}

TEST(BiTemporalJoinPredicates, OverlapsFalseDisjoint) {
    EXPECT_FALSE(BiTemporalJoin::overlaps({0, 5}, {10, 20}));
}

TEST(BiTemporalJoinPredicates, ContainedInTrue) {
    EXPECT_TRUE(BiTemporalJoin::containedIn({5, 10}, {0, 20}));
}

TEST(BiTemporalJoinPredicates, ContainedInFalse) {
    EXPECT_FALSE(BiTemporalJoin::containedIn({0, 15}, {5, 10}));
}

TEST(BiTemporalJoinPredicates, IntersectionOverlapping) {
    auto r = BiTemporalJoin::intersection({0, 10}, {5, 15});
    EXPECT_EQ(r.start, 5);
    EXPECT_EQ(r.end,   10);
}

TEST(BiTemporalJoinPredicates, IntersectionDisjoint) {
    auto r = BiTemporalJoin::intersection({0, 5}, {10, 20});
    EXPECT_GE(r.start, r.end);  // empty / invalid
}

// ─────────────────────────────────────────────────────────────────────────────
// SEQUENCED join
// ─────────────────────────────────────────────────────────────────────────────

TEST(BiTemporalJoinSequenced, OverlappingValidTimeJoined) {
    std::vector<BiTemporalRow> left  = {makeRow("k1", 0, 10)};
    std::vector<BiTemporalRow> right = {makeRow("k1", 5, 15)};
    BiTemporalJoin::Config cfg;
    cfg.mode = BiTemporalJoin::JoinMode::SEQUENCED;
    BiTemporalJoin join(left, right, cfg);
    auto res = join.execute();
    ASSERT_EQ(res.size(), 1u);
    EXPECT_EQ(res[0].key, "k1");
    EXPECT_EQ(res[0].valid_time_overlap.start, 5);
    EXPECT_EQ(res[0].valid_time_overlap.end,   10);
}

TEST(BiTemporalJoinSequenced, NonOverlappingValidTimeNotJoined) {
    std::vector<BiTemporalRow> left  = {makeRow("k1", 0, 5)};
    std::vector<BiTemporalRow> right = {makeRow("k1", 10, 20)};
    BiTemporalJoin::Config cfg;
    cfg.mode = BiTemporalJoin::JoinMode::SEQUENCED;
    auto res = BiTemporalJoin(left, right, cfg).execute();
    EXPECT_TRUE(res.empty());
}

TEST(BiTemporalJoinSequenced, DifferentKeyNotJoined) {
    std::vector<BiTemporalRow> left  = {makeRow("k1", 0, 10)};
    std::vector<BiTemporalRow> right = {makeRow("k2", 0, 10)};
    BiTemporalJoin::Config cfg;
    cfg.mode = BiTemporalJoin::JoinMode::SEQUENCED;
    auto res = BiTemporalJoin(left, right, cfg).execute();
    EXPECT_TRUE(res.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// NON-SEQUENCED join (ignore temporal axes)
// ─────────────────────────────────────────────────────────────────────────────

TEST(BiTemporalJoinNonSequenced, SameKeyAlwaysJoined) {
    std::vector<BiTemporalRow> left  = {makeRow("k1", 0, 5)};
    std::vector<BiTemporalRow> right = {makeRow("k1", 100, 200)};
    BiTemporalJoin::Config cfg;
    cfg.mode = BiTemporalJoin::JoinMode::NON_SEQUENCED;
    auto res = BiTemporalJoin(left, right, cfg).execute();
    ASSERT_EQ(res.size(), 1u);
    EXPECT_EQ(res[0].key, "k1");
}

// ─────────────────────────────────────────────────────────────────────────────
// CONTAINED_IN join
// ─────────────────────────────────────────────────────────────────────────────

TEST(BiTemporalJoinContainedIn, LeftContainedInRight_Joined) {
    std::vector<BiTemporalRow> left  = {makeRow("k1", 5, 10)};
    std::vector<BiTemporalRow> right = {makeRow("k1", 0, 20)};
    BiTemporalJoin::Config cfg;
    cfg.mode = BiTemporalJoin::JoinMode::CONTAINED_IN;
    auto res = BiTemporalJoin(left, right, cfg).execute();
    ASSERT_EQ(res.size(), 1u);
}

TEST(BiTemporalJoinContainedIn, LeftNotContainedInRight_NotJoined) {
    std::vector<BiTemporalRow> left  = {makeRow("k1", 0, 20)};
    std::vector<BiTemporalRow> right = {makeRow("k1", 5, 10)};
    BiTemporalJoin::Config cfg;
    cfg.mode = BiTemporalJoin::JoinMode::CONTAINED_IN;
    auto res = BiTemporalJoin(left, right, cfg).execute();
    EXPECT_TRUE(res.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// Result ordering
// ─────────────────────────────────────────────────────────────────────────────

TEST(BiTemporalJoinOrdering, ResultsSortedByKeyThenValidTimeStart) {
    std::vector<BiTemporalRow> left = {
        makeRow("b", 10, 20),
        makeRow("a", 0, 10),
        makeRow("a", 5, 15),
    };
    std::vector<BiTemporalRow> right = {
        makeRow("a", 3, 12),
        makeRow("b", 8, 18),
    };
    BiTemporalJoin::Config cfg;
    cfg.mode = BiTemporalJoin::JoinMode::SEQUENCED;
    auto res = BiTemporalJoin(left, right, cfg).execute();
    ASSERT_GE(res.size(), 1u);
    // First result must be key "a" (alphabetically before "b")
    EXPECT_EQ(res[0].key, "a");
}

// ─────────────────────────────────────────────────────────────────────────────
// forEach() early exit
// ─────────────────────────────────────────────────────────────────────────────

TEST(BiTemporalJoinForEach, EarlyExitStopsIteration) {
    std::vector<BiTemporalRow> left = {
        makeRow("k", 0, 100),
        makeRow("k", 10, 110),
        makeRow("k", 20, 120),
    };
    std::vector<BiTemporalRow> right = {makeRow("k", 0, 100)};
    BiTemporalJoin::Config cfg;
    cfg.mode = BiTemporalJoin::JoinMode::SEQUENCED;
    BiTemporalJoin join(left, right, cfg);

    int count = 0;
    join.forEach([&count](BiTemporalJoinResult) -> bool {
        ++count;
        return count < 2;  // Stop after second result
    });
    EXPECT_LE(count, 2);
}

} // anonymous namespace
} // namespace temporal
} // namespace themisdb
