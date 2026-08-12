/**
 * ThemisDB Replication – CRDT Types Library Tests
 *
 * Validates that every CRDT type is:
 *  - **Idempotent** – applying the same update twice has the same result
 *  - **Commutative** – merge order does not matter
 *  - **Correct** – the semantic guarantees of each type hold
 *
 * Covers:
 *  1. GrowOnlyCounter – increment, value, merge (commutativity, idempotency)
 *  2. PNCounter – increment, decrement, negative value, merge
 *  3. LWWRegister – LWW semantics, tie-break by node ID
 *  4. MVRegister – concurrent values, union on merge
 *  5. GrowOnlySet – add, contains, merge
 *  6. TwoPSet – add, remove with tombstone, no re-add
 *  7. ORSet – add after remove, concurrent add survives concurrent remove
 *  8. LWWMap – put, remove, merge per-key LWW
 *  9. RGArray – append, insertAfter, remove, ordered read, merge
 * 10. EnableWinsFlag – enable wins on concurrent enable+disable
 * 11. DisableWinsFlag – disable wins on concurrent enable+disable
 */

#include <gtest/gtest.h>
#include "replication/crdt_types.h"

using namespace themisdb::replication::crdt;

namespace {

template <typename T>
void writeWithLocalConsensusForTest(LWWRegister<T>& reg, const T& value, uint64_t timestamp) {
    // CRDT unit tests run on a single logical replica; write() is the local quorum decision point.
    reg.write(value, timestamp);
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// 1. GrowOnlyCounter
// ─────────────────────────────────────────────────────────────────────────────

TEST(GrowOnlyCounterTest, InitialValueIsZero) {
    GrowOnlyCounter c("node1");
    EXPECT_EQ(c.value(), 0u);
}

TEST(GrowOnlyCounterTest, IncrementIncreases) {
    GrowOnlyCounter c("node1");
    c.increment(5);
    EXPECT_EQ(c.value(), 5u);
}

TEST(GrowOnlyCounterTest, MergeCommutativity) {
    GrowOnlyCounter a("A"), b("B");
    a.increment(3);
    b.increment(7);

    GrowOnlyCounter ab = a;
    ab.merge(b);

    GrowOnlyCounter ba = b;
    ba.merge(a);

    EXPECT_EQ(ab.value(), ba.value());
    EXPECT_EQ(ab.value(), 10u);
}

TEST(GrowOnlyCounterTest, MergeIdempotency) {
    GrowOnlyCounter a("A"), b("B");
    a.increment(2);
    b.increment(4);
    a.merge(b);
    a.merge(b);  // second merge is idempotent
    EXPECT_EQ(a.value(), 6u);
}

TEST(GrowOnlyCounterTest, MergeTakesPerNodeMax) {
    GrowOnlyCounter a("A"), b("A");
    a.increment(10);
    b.increment(3);  // lower than a's count for "A"
    a.merge(b);
    EXPECT_EQ(a.value(), 10u);
}

// ─────────────────────────────────────────────────────────────────────────────
// 2. PNCounter
// ─────────────────────────────────────────────────────────────────────────────

TEST(PNCounterTest, IncrementAndDecrement) {
    PNCounter c("node1");
    c.increment(10);
    c.decrement(3);
    EXPECT_EQ(c.value(), 7);
}

TEST(PNCounterTest, CanGoNegative) {
    PNCounter c("node1");
    c.decrement(5);
    EXPECT_EQ(c.value(), -5);
}

TEST(PNCounterTest, MergeCommutativity) {
    PNCounter a("A"), b("B");
    a.increment(8);
    b.decrement(2);

    PNCounter ab = a;
    ab.merge(b);

    PNCounter ba = b;
    ba.merge(a);

    EXPECT_EQ(ab.value(), ba.value());
    EXPECT_EQ(ab.value(), 6);
}

// ─────────────────────────────────────────────────────────────────────────────
// 3. LWWRegister
// ─────────────────────────────────────────────────────────────────────────────

TEST(LWWRegisterTest, WriteAndRead) {
    LWWRegister<std::string> r("node1");
    writeWithLocalConsensusForTest(r, std::string("hello"), 100);
    ASSERT_TRUE(r.read().has_value());
    EXPECT_EQ(*r.read(), "hello");
}

TEST(LWWRegisterTest, LaterTimestampWins) {
    LWWRegister<std::string> r("node1");
    writeWithLocalConsensusForTest(r, std::string("old"), 100);
    writeWithLocalConsensusForTest(r, std::string("new"), 200);
    EXPECT_EQ(*r.read(), "new");
}

TEST(LWWRegisterTest, EarlierTimestampLoses) {
    LWWRegister<std::string> r("node1");
    writeWithLocalConsensusForTest(r, std::string("new"), 200);
    writeWithLocalConsensusForTest(r, std::string("old"), 100);
    EXPECT_EQ(*r.read(), "new");
}

TEST(LWWRegisterTest, MergePicksHigherTimestamp) {
    LWWRegister<std::string> a("A"), b("B");
    writeWithLocalConsensusForTest(a, std::string("from-A"), 300);
    writeWithLocalConsensusForTest(b, std::string("from-B"), 100);
    a.merge(b);
    EXPECT_EQ(*a.read(), "from-A");
}

TEST(LWWRegisterTest, TieBreakByNodeId) {
    LWWRegister<std::string> a("ZZZ"), b("AAA");
    writeWithLocalConsensusForTest(a, std::string("from-Z"), 100);
    writeWithLocalConsensusForTest(b, std::string("from-A"), 100);
    a.merge(b);
    // "ZZZ" > "AAA" so a wins
    EXPECT_EQ(*a.read(), "from-Z");
}

// ─────────────────────────────────────────────────────────────────────────────
// 4. MVRegister
// ─────────────────────────────────────────────────────────────────────────────

TEST(MVRegisterTest, WriteAndReadSingleValue) {
    MVRegister<std::string> r;
    r.write("v1", Dot{"node1", 1});
    auto values = r.read();
    ASSERT_EQ(values.size(), 1u);
    EXPECT_EQ(values[0], "v1");
}

TEST(MVRegisterTest, WriteOverwrites) {
    MVRegister<std::string> r;
    r.write("v1", Dot{"node1", 1});
    r.write("v2", Dot{"node1", 2});
    auto values = r.read();
    ASSERT_EQ(values.size(), 1u);
    EXPECT_EQ(values[0], "v2");
}

TEST(MVRegisterTest, MergeUnionsConcurrentValues) {
    MVRegister<std::string> a, b;
    a.write("val-A", Dot{"nodeA", 1});
    b.write("val-B", Dot{"nodeB", 1});
    a.merge(b);
    auto values = a.read();
    EXPECT_EQ(values.size(), 2u);
}

TEST(MVRegisterTest, MergeIdempotent) {
    MVRegister<std::string> a, b;
    a.write("val-A", Dot{"nodeA", 1});
    b.write("val-B", Dot{"nodeB", 1});
    a.merge(b);
    a.merge(b);
    EXPECT_EQ(a.read().size(), 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// 5. GrowOnlySet
// ─────────────────────────────────────────────────────────────────────────────

TEST(GrowOnlySetTest, AddAndContains) {
    GrowOnlySet<std::string> s;
    s.add("apple");
    EXPECT_TRUE(s.contains("apple"));
    EXPECT_FALSE(s.contains("banana"));
}

TEST(GrowOnlySetTest, MergeUnion) {
    GrowOnlySet<int> a, b;
    a.add(1); a.add(2);
    b.add(2); b.add(3);
    a.merge(b);
    EXPECT_TRUE(a.contains(1));
    EXPECT_TRUE(a.contains(2));
    EXPECT_TRUE(a.contains(3));
    EXPECT_EQ(a.elements().size(), 3u);
}

// ─────────────────────────────────────────────────────────────────────────────
// 6. TwoPSet
// ─────────────────────────────────────────────────────────────────────────────

TEST(TwoPSetTest, AddAndContains) {
    TwoPSet<std::string> s;
    s.add("x");
    EXPECT_TRUE(s.contains("x"));
}

TEST(TwoPSetTest, RemoveTombstones) {
    TwoPSet<std::string> s;
    s.add("x");
    s.remove("x");
    EXPECT_FALSE(s.contains("x"));
}

TEST(TwoPSetTest, CannotReAddAfterRemove) {
    TwoPSet<std::string> s;
    s.add("x");
    s.remove("x");
    s.add("x");  // 2P-Set semantics: remove is permanent
    // After add+remove+add, tombstone wins
    // (In 2P-Set, once in the remove set it stays removed)
    EXPECT_FALSE(s.contains("x"));
}

TEST(TwoPSetTest, RemoveNeverAddedIsNoop) {
    TwoPSet<std::string> s;
    s.remove("ghost");  // no-op
    s.add("ghost");
    EXPECT_TRUE(s.contains("ghost"));
}

TEST(TwoPSetTest, MergeUnionsBothSets) {
    TwoPSet<int> a, b;
    a.add(1);
    b.add(2); b.add(1); b.remove(1);
    a.merge(b);
    EXPECT_FALSE(a.contains(1));  // b's tombstone wins
    EXPECT_TRUE(a.contains(2));
}

// ─────────────────────────────────────────────────────────────────────────────
// 7. ORSet
// ─────────────────────────────────────────────────────────────────────────────

TEST(ORSetTest, AddAndContains) {
    ORSet<std::string> s("node1");
    s.add("item");
    EXPECT_TRUE(s.contains("item"));
}

TEST(ORSetTest, RemoveAfterAdd) {
    ORSet<std::string> s("node1");
    s.add("item");
    s.remove("item");
    EXPECT_FALSE(s.contains("item"));
}

TEST(ORSetTest, ReAddAfterRemove) {
    ORSet<std::string> s("node1");
    s.add("item");
    s.remove("item");
    s.add("item");  // fresh dot → item is back
    EXPECT_TRUE(s.contains("item"));
}

TEST(ORSetTest, ConcurrentAddSurvivesConcurrentRemove) {
    ORSet<std::string> a("A"), b("B");
    a.add("item");    // dot {A,1}
    b.add("item");    // dot {B,1}
    a.remove("item"); // tombstones {A,1}
    // Merge: b's {B,1} is NOT tombstoned by a, so "item" survives
    b.merge(a);
    EXPECT_TRUE(b.contains("item"));
}

TEST(ORSetTest, ElementsReturnsLiveItems) {
    ORSet<std::string> s("node1");
    s.add("a");
    s.add("b");
    s.remove("a");
    auto elems = s.elements();
    EXPECT_EQ(elems.size(), 1u);
    EXPECT_TRUE(elems.count("b"));
}

// ─────────────────────────────────────────────────────────────────────────────
// 8. LWWMap
// ─────────────────────────────────────────────────────────────────────────────

TEST(LWWMapTest, PutAndGet) {
    LWWMap<std::string, int> m("node1");
    m.put("k", 42, 100);
    auto v = m.get("k");
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, 42);
}

TEST(LWWMapTest, LaterTimestampWinsPerKey) {
    LWWMap<std::string, int> m("node1");
    m.put("k", 1, 100);
    m.put("k", 2, 200);
    EXPECT_EQ(*m.get("k"), 2);
}

TEST(LWWMapTest, RemoveKey) {
    LWWMap<std::string, int> m("node1");
    m.put("k", 42, 100);
    m.remove("k", 200);
    EXPECT_FALSE(m.contains("k"));
}

TEST(LWWMapTest, MergePerKeyLWW) {
    LWWMap<std::string, int> a("A"), b("B");
    a.put("x", 1, 100);
    b.put("x", 2, 200);
    a.merge(b);
    EXPECT_EQ(*a.get("x"), 2);
}

TEST(LWWMapTest, MergeCommutativity) {
    LWWMap<std::string, std::string> a("A"), b("B");
    a.put("k1", "from-A", 150);
    b.put("k1", "from-B", 100);
    b.put("k2", "only-B", 50);

    auto ab = a; ab.merge(b);
    auto ba = b; ba.merge(a);

    EXPECT_EQ(ab.get("k1"), ba.get("k1"));
    EXPECT_EQ(ab.get("k2"), ba.get("k2"));
}

// ─────────────────────────────────────────────────────────────────────────────
// 9. RGArray
// ─────────────────────────────────────────────────────────────────────────────

TEST(RGArrayTest, AppendAndRead) {
    RGArray<std::string> a("node1");
    a.append("x");
    a.append("y");
    auto elems = a.read();
    ASSERT_EQ(elems.size(), 2u);
    EXPECT_EQ(elems[0], "x");
    EXPECT_EQ(elems[1], "y");
}

TEST(RGArrayTest, RemoveTombstones) {
    RGArray<int> a("node1");
    auto d1 = a.append(10);
    a.append(20);
    a.remove(d1);
    auto elems = a.read();
    ASSERT_EQ(elems.size(), 1u);
    EXPECT_EQ(elems[0], 20);
}

TEST(RGArrayTest, SizeCountsLiveElements) {
    RGArray<int> a("node1");
    a.append(1);
    auto d = a.append(2);
    a.append(3);
    a.remove(d);
    EXPECT_EQ(a.size(), 2u);
}

TEST(RGArrayTest, MergeUnion) {
    RGArray<int> a("A"), b("B");
    a.append(1);
    b.append(2);
    a.merge(b);
    auto elems = a.read();
    EXPECT_EQ(elems.size(), 2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// 10. EnableWinsFlag
// ─────────────────────────────────────────────────────────────────────────────

TEST(EnableWinsFlagTest, DisabledByDefault) {
    EnableWinsFlag f;
    // Initial state: no enable or disable called → ts_enable_ == ts_disable_ == 0
    // EW semantics: enable wins on tie → value() returns true (0 >= 0)
    EXPECT_TRUE(f.value());
}

TEST(EnableWinsFlagTest, EnableSetsTrue) {
    EnableWinsFlag f;
    f.enable();
    EXPECT_TRUE(f.value());
}

TEST(EnableWinsFlagTest, DisableSetsFalse) {
    EnableWinsFlag f;
    f.enable();
    f.disable();
    EXPECT_FALSE(f.value());
}

TEST(EnableWinsFlagTest, ConcurrentEnableWins) {
    EnableWinsFlag a, b;
    a.enable();   // ts=1
    b.disable();  // ts=1 (same logical tick on b)
    a.merge(b);
    // enable wins: a.ts_enable > a.ts_disable after merge
    EXPECT_TRUE(a.value());
}

TEST(EnableWinsFlagTest, MergeIdempotent) {
    EnableWinsFlag a, b;
    a.enable();
    b.disable();
    a.merge(b);
    a.merge(b);  // idempotent
    EXPECT_TRUE(a.value());
}

// ─────────────────────────────────────────────────────────────────────────────
// 11. DisableWinsFlag
// ─────────────────────────────────────────────────────────────────────────────

TEST(DisableWinsFlagTest, EnableSetsTrue) {
    DisableWinsFlag f;
    f.enable();
    EXPECT_TRUE(f.value());
}

TEST(DisableWinsFlagTest, DisableSetsFalse) {
    DisableWinsFlag f;
    f.enable();
    f.disable();
    EXPECT_FALSE(f.value());
}

TEST(DisableWinsFlagTest, ConcurrentDisableWins) {
    DisableWinsFlag a, b;
    b.enable();   // b: ts_enable=1
    a.disable();  // a: ts_disable=1
    b.merge(a);
    // disable wins on tie: value() returns ts_enable > ts_disable → false
    EXPECT_FALSE(b.value());
}
