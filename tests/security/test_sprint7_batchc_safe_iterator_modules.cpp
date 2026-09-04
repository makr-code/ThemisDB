/**
 * @file test_sprint7_batchc_safe_iterator_modules.cpp
 * @brief Unit tests for Sprint 7 Batch C Phase 2B-D SafeIterator remediations.
 *
 * Verifies that the iterator-safety patterns (BoundsChecker, AdvanceSafe,
 * RangeValidator) applied to the six new production modules correctly
 * reject out-of-bounds access and accept valid inputs.
 *
 * Modules covered:
 *   - themis::network::PacketParser / PacketBuilder   (gaps B001, B003, B005)
 *   - themis::query::ResultSet / QueryExecutor        (gaps B002, B006, B008)
 *   - themis::analytics::Aggregator                  (gaps A004, C001, C002)
 *   - themis::analytics::TimeSeries                  (gaps B011, B012, B013)
 *   - themis::cache::EvictionScheduler               (gaps B012, B013)
 *   - themis::graph::AdjacencyList                   (gaps A003, A006, B009)
 */

#include <gtest/gtest.h>

#include "network/wire_protocol.h"
#include "query/query_executor.h"
#include "analytics/aggregation.h"
#include "analytics/time_series.h"
#include "cache/eviction.h"
#include "graph/adjacency_list.h"

// ============================================================================
// Helper: build a minimal valid wire packet buffer
// ============================================================================

namespace {

/// Build a raw packet with the given type byte, payload, using real framing.
std::vector<uint8_t> build_packet(uint8_t type_byte,
                                  const std::vector<uint8_t>& payload)
{
    // Header: 0xDB 0x01 <type> <payload_len_be32> <payload...>
    uint32_t len = static_cast<uint32_t>(payload.size());
    std::vector<uint8_t> buf;
    buf.push_back(0xDB);
    buf.push_back(0x01);
    buf.push_back(type_byte);
    buf.push_back(static_cast<uint8_t>(len >> 24u));
    buf.push_back(static_cast<uint8_t>(len >> 16u));
    buf.push_back(static_cast<uint8_t>(len >>  8u));
    buf.push_back(static_cast<uint8_t>(len >>  0u));
    buf.insert(buf.end(), payload.begin(), payload.end());
    return buf;
}

}  // namespace

// ============================================================================
// 1. PacketParser / PacketBuilder  — gaps B001, B003, B005
// ============================================================================

class PacketParserSafeIteratorTest : public ::testing::Test {};

// Gap B005: buffer too short → must throw before any dereference
TEST_F(PacketParserSafeIteratorTest, GapB005_TooShortBufferThrows)
{
    std::vector<uint8_t> buf{0xDB};  // only 1 byte — truncated magic
    EXPECT_THROW(themis::network::PacketParser::parse(buf),
                 themis::network::ParseError);
}

// Gap B005: wrong magic byte 0 → ParseError, not UB
TEST_F(PacketParserSafeIteratorTest, GapB005_WrongMagic0Throws)
{
    auto buf = build_packet(0x01, {});
    buf[0] = 0xAA;  // corrupt magic byte 0
    EXPECT_THROW(themis::network::PacketParser::parse(buf),
                 themis::network::ParseError);
}

// Gap B005: wrong magic byte 1 → ParseError
TEST_F(PacketParserSafeIteratorTest, GapB005_WrongMagic1Throws)
{
    auto buf = build_packet(0x01, {});
    buf[1] = 0xFF;  // corrupt magic byte 1
    EXPECT_THROW(themis::network::PacketParser::parse(buf),
                 themis::network::ParseError);
}

// Gap B001/B003: payload_len > actual bytes remaining → ParseError (not OOB)
TEST_F(PacketParserSafeIteratorTest, GapB001B003_TruncatedPayloadThrows)
{
    // Claim 10-byte payload but provide none after header.
    std::vector<uint8_t> buf{0xDB, 0x01, 0x02,  // magic + query type
                              0x00, 0x00, 0x00, 0x0A};  // length = 10
    // No payload bytes → AdvanceSafe must throw.
    EXPECT_THROW(themis::network::PacketParser::parse(buf),
                 themis::network::ParseError);
}

// Happy path: minimal valid PING packet (empty payload)
TEST_F(PacketParserSafeIteratorTest, ValidPingPacket)
{
    auto buf = build_packet(0x20, {});  // kPing = 0x20
    EXPECT_NO_THROW({
        auto pkt = themis::network::PacketParser::parse(buf);
        EXPECT_EQ(pkt.type, themis::network::PacketType::kPing);
        EXPECT_TRUE(pkt.payload.empty());
    });
}

// Happy path: packet with payload
TEST_F(PacketParserSafeIteratorTest, ValidQueryPacketWithPayload)
{
    std::vector<uint8_t> payload{0x01, 0x02, 0x03};
    auto buf = build_packet(0x02, payload);  // kQuery = 0x02
    EXPECT_NO_THROW({
        auto pkt = themis::network::PacketParser::parse(buf);
        EXPECT_EQ(pkt.type, themis::network::PacketType::kQuery);
        EXPECT_EQ(pkt.payload, payload);
    });
}

// PacketBuilder::build_pong produces a parseable pong
TEST_F(PacketParserSafeIteratorTest, BuildPongIsParseable)
{
    auto buf = themis::network::PacketBuilder::build_pong();
    EXPECT_NO_THROW({
        auto pkt = themis::network::PacketParser::parse(buf);
        EXPECT_EQ(pkt.type, themis::network::PacketType::kPong);
    });
}

// PacketBuilder::build_error produces a parseable error packet
TEST_F(PacketParserSafeIteratorTest, BuildErrorIsParseable)
{
    auto buf = themis::network::PacketBuilder::build_error(404u, "not found");
    EXPECT_NO_THROW({
        auto pkt = themis::network::PacketParser::parse(buf);
        EXPECT_EQ(pkt.type, themis::network::PacketType::kError);
        EXPECT_FALSE(pkt.payload.empty());
    });
}

// ============================================================================
// 2. ResultSet / QueryExecutor  — gaps B002, B006, B008
// ============================================================================

class ResultSetSafeIteratorTest : public ::testing::Test {
protected:
    themis::query::ResultSet make_result(std::size_t n_rows) {
        themis::query::ResultSet rs;
        rs.column_names = {"id", "value"};
        for (std::size_t i = 0; i < n_rows; ++i) {
            rs.rows.push_back({static_cast<int64_t>(i), static_cast<double>(i) * 1.5});
        }
        return rs;
    }
};

// Gap B002: at() on empty ResultSet → std::out_of_range
TEST_F(ResultSetSafeIteratorTest, GapB002_AtOnEmptyThrows)
{
    auto rs = make_result(0);
    EXPECT_THROW(rs.at(0), std::out_of_range);
}

// Gap B002: at() past end → std::out_of_range
TEST_F(ResultSetSafeIteratorTest, GapB002_AtPastEndThrows)
{
    auto rs = make_result(3);
    EXPECT_THROW(rs.at(3), std::out_of_range);
    EXPECT_THROW(rs.at(100), std::out_of_range);
}

// Gap B002: at() valid index → correct row
TEST_F(ResultSetSafeIteratorTest, GapB002_AtValidIndex)
{
    auto rs = make_result(5);
    EXPECT_NO_THROW({
        const auto& row = rs.at(2);
        EXPECT_EQ(std::get<int64_t>(row[0]), 2);
    });
}

// Gap B006: page() with offset beyond end returns empty (not UB)
TEST_F(ResultSetSafeIteratorTest, GapB006_PageOffsetBeyondEndThrows)
{
    auto rs = make_result(5);
    // offset == size is valid (empty result), offset > size throws
    EXPECT_THROW(rs.page(10, 5), std::out_of_range);
}

// Gap B006: page() with zero limit returns empty
TEST_F(ResultSetSafeIteratorTest, GapB006_PageZeroLimitEmpty)
{
    auto rs = make_result(5);
    EXPECT_TRUE(rs.page(0, 0).empty());
}

// Gap B008: page() sub-range via RangeValidator — correct slice
TEST_F(ResultSetSafeIteratorTest, GapB008_PageSubRangeCorrect)
{
    auto rs = make_result(10);
    auto page = rs.page(3, 4);
    EXPECT_EQ(page.size(), 4u);
    EXPECT_EQ(std::get<int64_t>(page[0][0]), 3);
    EXPECT_EQ(std::get<int64_t>(page[3][0]), 6);
}

// Gap B008: page() clamps limit at container boundary
TEST_F(ResultSetSafeIteratorTest, GapB008_PageLimitClamped)
{
    auto rs = make_result(5);
    auto page = rs.page(3, 100);  // only 2 rows remain after offset 3
    EXPECT_EQ(page.size(), 2u);
}

// ============================================================================
// 3. Aggregator (analytics) — gaps A004, C001, C002
// ============================================================================

class AggregatorSafeIteratorTest : public ::testing::Test {
protected:
    using AggRow  = themis::analytics::AggregationRow;
    using AggSpec = themis::analytics::AggregateSpec;
    using AggFn   = themis::analytics::AggregateFunction;

    themis::analytics::Aggregator make_count_agg(const std::string& group_col) {
        AggSpec spec;
        spec.function      = AggFn::kCount;
        spec.source_column = "value";
        spec.output_column = "cnt";
        return themis::analytics::Aggregator({group_col}, {spec});
    }
};

// Gap A004: push_back inside iterator loop — verified by correct group counts
TEST_F(AggregatorSafeIteratorTest, GapA004_PushBackDuringMerge_CorrectResult)
{
    auto agg = make_count_agg("category");
    agg.feed(AggRow{{"category", std::string{"A"}}, {"value", int64_t{1}}});
    agg.feed(AggRow{{"category", std::string{"B"}}, {"value", int64_t{2}}});
    agg.feed(AggRow{{"category", std::string{"A"}}, {"value", int64_t{3}}});

    auto result = agg.finalise();
    EXPECT_EQ(result.rows.size(), 2u);  // two distinct groups: A, B
}

// Gap C001: page() with valid offset
TEST_F(AggregatorSafeIteratorTest, GapC001_PageValidOffset)
{
    auto agg = make_count_agg("cat");
    for (int i = 0; i < 10; ++i) {
        agg.feed(AggRow{{"cat", std::string(1, static_cast<char>('A' + i))},
                        {"value", int64_t{i}}});
    }
    auto result = agg.finalise();
    EXPECT_EQ(result.rows.size(), 10u);

    // page() with valid offset: must not throw and return correct count
    EXPECT_NO_THROW({
        auto page = result.page(5, 3);
        EXPECT_EQ(page.size(), 3u);
    });
}

// Gap C001: page() with out-of-range offset → std::out_of_range
TEST_F(AggregatorSafeIteratorTest, GapC001_PageInvalidOffsetThrows)
{
    auto agg = make_count_agg("cat");
    agg.feed(AggRow{{"cat", std::string{"X"}}, {"value", int64_t{1}}});
    auto result = agg.finalise();

    EXPECT_THROW(result.page(100, 5), std::out_of_range);
}

// Gap C002: at() bounds-checked access
TEST_F(AggregatorSafeIteratorTest, GapC002_AtBoundsChecked)
{
    auto agg = make_count_agg("cat");
    agg.feed(AggRow{{"cat", std::string{"Y"}}, {"value", int64_t{7}}});
    auto result = agg.finalise();

    EXPECT_NO_THROW(result.at(0));
    EXPECT_THROW(result.at(1), std::out_of_range);
}

// Verify empty aggregation is safe
TEST_F(AggregatorSafeIteratorTest, EmptyAggregationSafe)
{
    auto agg = make_count_agg("col");
    auto result = agg.finalise();
    EXPECT_TRUE(result.rows.empty());
    EXPECT_THROW(result.at(0), std::out_of_range);
}

// ============================================================================
// 4. TimeSeries — gaps B011, B012, B013
// ============================================================================

class TimeSeriesSafeIteratorTest : public ::testing::Test {
protected:
    using TS = themis::analytics::TimeSeries;
    using TW = themis::analytics::TimeWindow;
    using TP = themis::analytics::TimePoint;

    TS make_series(int n) {
        TS ts("test");
        for (int i = 0; i < n; ++i) {
            ts.append(static_cast<TP>(i * 1000), static_cast<double>(i));
        }
        return ts;
    }
};

// Constructor: empty name → invalid_argument
TEST_F(TimeSeriesSafeIteratorTest, EmptyNameThrows)
{
    EXPECT_THROW(TS(""), std::invalid_argument);
}

// Gap B013: query_window on empty series → safe empty result
TEST_F(TimeSeriesSafeIteratorTest, GapB013_EmptySeriesWindowSafe)
{
    TS ts("empty");
    auto result = ts.query_window(TW{0, 100});
    EXPECT_TRUE(result.empty());
}

// Gap B013: query_window below all data → safe empty result
TEST_F(TimeSeriesSafeIteratorTest, GapB013_WindowBelowDataSafe)
{
    auto ts = make_series(5);  // timestamps 0..4000
    auto result = ts.query_window(TW{10000, 20000});
    EXPECT_TRUE(result.empty());
}

// Gap B012: query_window valid range → correct points returned
TEST_F(TimeSeriesSafeIteratorTest, GapB012_ValidWindowReturnsCorrectPoints)
{
    auto ts = make_series(10);
    // timestamps: 0, 1000, 2000, ..., 9000
    auto result = ts.query_window(TW{2000, 5000});
    EXPECT_EQ(result.size(), 3u);  // ts 2000, 3000, 4000
}

// Gap B011: page() with valid offset on large series
TEST_F(TimeSeriesSafeIteratorTest, GapB011_PageValidOffset)
{
    auto ts = make_series(20);
    EXPECT_NO_THROW({
        auto page = ts.page(5, 10);
        EXPECT_EQ(page.size(), 10u);
    });
}

// Gap B011: page() with out-of-range offset → std::out_of_range
TEST_F(TimeSeriesSafeIteratorTest, GapB011_PageOutOfRangeThrows)
{
    auto ts = make_series(5);
    EXPECT_THROW(ts.page(100, 5), std::out_of_range);
}

// append_batch + sorted order preserved
TEST_F(TimeSeriesSafeIteratorTest, AppendBatchSorted)
{
    TS ts("batch");
    ts.append_batch({{3000, 3.0}, {1000, 1.0}, {2000, 2.0}});
    auto all = ts.page(0, 10);
    EXPECT_EQ(all.size(), 3u);
    EXPECT_LE(all[0].timestamp, all[1].timestamp);
    EXPECT_LE(all[1].timestamp, all[2].timestamp);
}

// ============================================================================
// 5. EvictionScheduler — gaps B012, B013
// ============================================================================

class EvictionSchedulerSafeIteratorTest : public ::testing::Test {
protected:
    using Candidate = themis::cache::EvictionCandidate;
    using Scheduler = themis::cache::EvictionScheduler;

    Candidate make_candidate(std::string key, std::size_t size_bytes,
                             std::int64_t last_access_ns = 0) {
        Candidate c;
        c.key             = std::move(key);
        c.size_bytes      = size_bytes;
        c.last_access_ns  = last_access_ns;
        c.access_count    = 1;
        return c;
    }
};

// Null scoring function → invalid_argument
TEST_F(EvictionSchedulerSafeIteratorTest, NullScoringFnThrows)
{
    EXPECT_THROW(Scheduler(nullptr), std::invalid_argument);
}

// Gap B012: target_free_bytes = 0 → invalid_argument
TEST_F(EvictionSchedulerSafeIteratorTest, GapB012_ZeroTargetThrows)
{
    Scheduler sched(Scheduler::lru_policy());
    std::vector<Candidate> cands{make_candidate("k1", 100)};
    EXPECT_THROW(sched.select(cands, 0), std::invalid_argument);
}

// Gap B013: empty candidate list → safe empty result
TEST_F(EvictionSchedulerSafeIteratorTest, GapB013_EmptyCandidatesSafe)
{
    Scheduler sched(Scheduler::lru_policy());
    std::vector<Candidate> empty;
    EXPECT_NO_THROW({
        auto result = sched.select(empty, 100);
        EXPECT_TRUE(result.evicted_keys.empty());
        EXPECT_EQ(result.freed_bytes, 0u);
    });
}

// Happy path: select frees at least the requested bytes
TEST_F(EvictionSchedulerSafeIteratorTest, SelectFreesEnoughBytes)
{
    Scheduler sched(Scheduler::lru_policy());
    std::vector<Candidate> cands = {
        make_candidate("a", 512, 1000),
        make_candidate("b", 256, 2000),
        make_candidate("c", 128, 3000),
    };
    auto result = sched.select(cands, 600);
    EXPECT_GE(result.freed_bytes, 600u);
    EXPECT_FALSE(result.evicted_keys.empty());
}

// Gap B012: unsigned overflow guard — select stops before wrapping
TEST_F(EvictionSchedulerSafeIteratorTest, GapB012_NoWrapAround)
{
    Scheduler sched(Scheduler::lru_policy());
    constexpr std::size_t kHuge = std::numeric_limits<std::size_t>::max() / 2 + 1;
    std::vector<Candidate> cands = {make_candidate("big", kHuge, 100)};
    // Must not crash or overflow.
    EXPECT_NO_THROW(sched.select(cands, 1024));
}

// LFU policy: less-frequently-accessed entries evicted first
TEST_F(EvictionSchedulerSafeIteratorTest, LfuPolicyOrdering)
{
    Scheduler sched(Scheduler::lfu_policy());
    std::vector<Candidate> cands = {};

    for (int i = 0; i < 5; ++i) {
        Candidate c = make_candidate("k" + std::to_string(i), 100);
        c.access_count = static_cast<uint64_t>(i + 1);
        cands.push_back(c);
    }
    auto result = sched.select(cands, 100);
    EXPECT_EQ(result.evicted_keys.front(), "k0");  // lowest access_count
}

// ============================================================================
// 6. AdjacencyList — gaps A003, A006, B009
// ============================================================================

class AdjacencyListSafeIteratorTest : public ::testing::Test {
protected:
    using AL     = themis::graph::AdjacencyList;
    using Edge   = themis::graph::Edge;
    using VtxId  = themis::graph::VertexId;

    AL make_graph_triangle() {
        AL g;
        g.add_vertex(1, "v1");
        g.add_vertex(2, "v2");
        g.add_vertex(3, "v3");
        g.add_edge(1, Edge{2, 1.0, "e12"});
        g.add_edge(1, Edge{3, 1.0, "e13"});
        g.add_edge(2, Edge{3, 2.0, "e23"});
        return g;
    }
};

// Gap B009: neighbour_at() on non-existent vertex → std::out_of_range
TEST_F(AdjacencyListSafeIteratorTest, GapB009_NeighbourAtUnknownVertexThrows)
{
    AL g;
    EXPECT_THROW(g.neighbour_at(99, 0), std::out_of_range);
}

// Gap B009: neighbour_at() index out of range → std::out_of_range
TEST_F(AdjacencyListSafeIteratorTest, GapB009_NeighbourAtOutOfRangeThrows)
{
    AL g;
    g.add_vertex(1, "v1");
    g.add_edge(1, Edge{2, 1.0, "e12"});
    EXPECT_THROW(g.neighbour_at(1, 5), std::out_of_range);
}

// Gap B009: neighbour_at() valid index returns correct edge
TEST_F(AdjacencyListSafeIteratorTest, GapB009_NeighbourAtValidIndex)
{
    AL g;
    g.add_vertex(1, "v1");
    g.add_edge(1, Edge{2, 1.0, "e12"});
    EXPECT_NO_THROW({
        const auto& edge = g.neighbour_at(1, 0);
        EXPECT_EQ(edge.target, VtxId{2});
    });
}

// Gap A003: remove_edges_if during iteration must not corrupt state
TEST_F(AdjacencyListSafeIteratorTest, GapA003_RemoveEdgesIfIterationSafe)
{
    auto g = make_graph_triangle();
    // Remove all edges from v1 pointing to v3 (collect-then-erase).
    EXPECT_NO_THROW({
        auto removed = g.remove_edges_if(1,
            [](const Edge& e) { return e.target == 3; });
        EXPECT_EQ(removed, 1u);
    });
    EXPECT_EQ(g.out_degree(1), 1u);  // only e12 remains
}

// Gap A006: remove_vertex removes all incident edges via collect-then-erase
TEST_F(AdjacencyListSafeIteratorTest, GapA006_RemoveVertexIncidentEdgesSafe)
{
    auto g = make_graph_triangle();
    EXPECT_TRUE(g.has_vertex(2));
    EXPECT_NO_THROW(g.remove_vertex(2));
    EXPECT_FALSE(g.has_vertex(2));
    // v1 should no longer have an edge to v2
    for (const auto& e : g.neighbours(1)) {
        EXPECT_NE(e.target, VtxId{2});
    }
}

// add_edge to non-existent vertex → std::out_of_range
TEST_F(AdjacencyListSafeIteratorTest, AddEdgeToUnknownVertexThrows)
{
    AL g;
    g.add_vertex(1, "v1");
    EXPECT_THROW(g.add_edge(99, Edge{1, 1.0, ""}), std::out_of_range);
}

// remove_edges_if on non-existent vertex → std::out_of_range
TEST_F(AdjacencyListSafeIteratorTest, RemoveEdgesIfUnknownVertexThrows)
{
    AL g;
    EXPECT_THROW(g.remove_edges_if(42, [](const Edge&) { return true; }),
                 std::out_of_range);
}

// edge_count sums across all vertices
TEST_F(AdjacencyListSafeIteratorTest, EdgeCountCorrect)
{
    auto g = make_graph_triangle();
    EXPECT_EQ(g.edge_count(), 3u);
}

// neighbours on unknown vertex returns empty (no throw)
TEST_F(AdjacencyListSafeIteratorTest, NeighboursUnknownVertexEmpty)
{
    AL g;
    EXPECT_TRUE(g.neighbours(99).empty());
}
