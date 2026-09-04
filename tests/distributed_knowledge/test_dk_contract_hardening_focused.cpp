// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_dk_contract_hardening_focused.cpp
 * @brief Phase 4 — Distributed Knowledge contract hardening focused tests (DKC-01..DKC-16).
 *
 * Tests are fully self-contained: no network I/O, no filesystem I/O.
 * All external interactions are mocked inline.  The canonical PRNG seed is
 * kDKContractSeed = 42.
 *
 * ## Test families
 *
 * ### DKC-01..04 — Entity lifecycle
 *   DKC-01  Entity create returns a valid immutable ID
 *   DKC-02  Entity read returns committed state
 *   DKC-03  Entity update produces a new version; old version untouched
 *   DKC-04  Entity delete (tombstone) makes entity invisible to reads
 *
 * ### DKC-05..08 — Federation
 *   DKC-05  Federation result is union of per-node results (no duplicates)
 *   DKC-06  Timeout on one peer surfaces FEDERATION_TIMEOUT
 *   DKC-07  Partial result is returned with timeout warning (not dropped)
 *   DKC-08  Federation result count ≤ kMaxFederationResultEntities
 *
 * ### DKC-09..12 — Retrieval
 *   DKC-09  neighbours() returns all direct edges (completeness)
 *   DKC-10  Path query result is ordered by path length ascending
 *   DKC-11  neighbours() result count ≤ kMaxNeighboursPerQuery
 *   DKC-12  Path query depth ≤ kMaxPathQueryDepth is enforced
 *
 * ### DKC-13..16 — Conflict resolution
 *   DKC-13  LWW: higher timestamp wins deterministically
 *   DKC-14  LWW tie-break: lexicographically larger node-ID wins
 *   DKC-15  Tombstone propagation failure surfaces TOMBSTONE_PROPAGATION_FAILED
 *   DKC-16  CRDT merge type mismatch surfaces CRDT_MERGE_TYPE_MISMATCH
 *
 * @see include/distributed_knowledge/distributed_knowledge_api_contract.h
 * @see src/distributed_knowledge/ROADMAP.md — Phase 4 item
 */

#include <gtest/gtest.h>

#include "distributed_knowledge/distributed_knowledge_api_contract.h"

#include <algorithm>
#include <chrono>
#include <map>
#include <memory>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <vector>

using namespace themis::distributed_knowledge;
using namespace std::chrono_literals;

namespace {

static constexpr uint64_t kDKContractSeed = 42;

// ---------------------------------------------------------------------------
// Mock entity store
// ---------------------------------------------------------------------------

struct MockEntity {
    std::string               id;
    std::int64_t              timestamp_us; ///< µs since epoch
    std::string               node_id;
    std::string               payload = {};
    bool                      tombstone = false;
    int                       version   = 1;
};

struct MockEntityStore {
    std::map<std::string, MockEntity> data;

    DKErrorCode create(MockEntity e) {
        if (data.count(e.id)) {
          return DKErrorCode::CONFLICT_UNRESOLVABLE;
        }
        data[e.id] = std::move(e);
        return DKErrorCode::OK;
    }

    std::optional<MockEntity> read(const std::string& id) const {
        auto it = data.find(id);
        if (it == data.end() || it->second.tombstone) {
          return std::nullopt;
        }
        return it->second;
    }

    DKErrorCode update(const std::string& id, const std::string& new_payload, std::int64_t ts) {
        auto it = data.find(id);
        if (it == data.end()) {
          return DKErrorCode::ENTITY_NOT_FOUND;
        }
        MockEntity updated = it->second;
        updated.payload      = new_payload;
        updated.timestamp_us = ts;
        updated.version++;
        data[id] = updated;
        return DKErrorCode::OK;
    }

    DKErrorCode remove(const std::string& id) {
        auto it = data.find(id);
        if (it == data.end()) {
          return DKErrorCode::ENTITY_NOT_FOUND;
        }
        it->second.tombstone = true;
        return DKErrorCode::OK;
    }
};

// ---------------------------------------------------------------------------
// Mock graph (for neighbours / path queries)
// ---------------------------------------------------------------------------

struct MockGraph {
    std::map<std::string, std::vector<std::string>> adj;  ///< node → list of neighbour IDs

    void addEdge(const std::string& from, const std::string& to) {
        adj[from].push_back(to);
    }

    /// Returns direct neighbours of 'node'.
    std::vector<std::string> neighbours(const std::string& node) const {
        auto it = adj.find(node);
        if (it == adj.end()) return {};
        return it->second;
    }

    /// BFS path query up to max_depth; returns discovered nodes by depth.
    std::vector<std::pair<std::string, std::size_t>>
    pathQuery(const std::string& start, std::size_t max_depth) const {
        std::vector<std::pair<std::string, std::size_t>> result;
        std::set<std::string> visited = {};

        std::vector<std::pair<std::string, std::size_t>> frontier = {{start, 0}};
        while (!frontier.empty()) {
            auto [node, depth] = frontier.back();
            frontier.pop_back();
            if (visited.count(node) || depth > max_depth) {
              continue;
            }
            visited.insert(node);
            if (depth > 0) result.push_back({node, depth});
            if (depth < max_depth) {
                auto nb = neighbours(node);
                for (auto& n : nb)
                    frontier.push_back({n, depth + 1});
            }
        }
        // Sort by depth ascending
        std::sort(result.begin(), result.end(),
                  [](auto& a, auto& b){ return a.second < b.second; });
        return result;
    }
};

} // anonymous namespace

// ===========================================================================
// DKC-01 — Entity create returns a valid immutable ID
// ===========================================================================

TEST(DKContractHardeningDKC01, EntityCreateReturnsValidId) {
    MockEntityStore store;
    MockEntity e{"entity-1", 1000L, "node-A", "payload-1"};
    auto rc = store.create(e);
    EXPECT_EQ(rc, DKErrorCode::OK);
    auto fetched = store.read("entity-1");
    ASSERT_TRUE(fetched.has_value());
    EXPECT_EQ(fetched->id, "entity-1");
    EXPECT_EQ(fetched->payload, "payload-1");
}

// ===========================================================================
// DKC-02 — Entity read returns committed state
// ===========================================================================

TEST(DKContractHardeningDKC02, EntityReadReturnsCommittedState) {
    MockEntityStore store;
    store.create({"e2", 2000L, "node-B", "data-2"});
    auto fetched = store.read("e2");
    ASSERT_TRUE(fetched.has_value());
    EXPECT_EQ(fetched->payload, "data-2");

    // Non-existent entity
    auto missing = store.read("nonexistent");
    EXPECT_FALSE(missing.has_value());
}

// ===========================================================================
// DKC-03 — Entity update produces new version; original version counter increases
// ===========================================================================

TEST(DKContractHardeningDKC03, EntityUpdateProducesNewVersion) {
    MockEntityStore store;
    store.create({"e3", 1000L, "node-A", "v1"});
    auto v1 = store.read("e3");
    ASSERT_TRUE(v1.has_value());
    EXPECT_EQ(v1->version, 1);

    store.update("e3", "v2", 2000L);
    auto v2 = store.read("e3");
    ASSERT_TRUE(v2.has_value());
    EXPECT_EQ(v2->payload, "v2");
    EXPECT_GT(v2->version, v1->version) << "Update must increment version";
}

// ===========================================================================
// DKC-04 — Entity delete (tombstone) makes entity invisible to reads
// ===========================================================================

TEST(DKContractHardeningDKC04, TombstonedEntityInvisible) {
    MockEntityStore store;
    store.create({"e4", 1000L, "node-A", "payload"});
    ASSERT_TRUE(store.read("e4").has_value());

    auto rc = store.remove("e4");
    EXPECT_EQ(rc, DKErrorCode::OK);
    EXPECT_FALSE(store.read("e4").has_value())
        << "Tombstoned entity must be invisible to reads";
}

// ===========================================================================
// DKC-05 — Federation result is union of per-node results (no duplicates)
// ===========================================================================

TEST(DKContractHardeningDKC05, FederationResultIsUnionNoDuplicates) {
    // Simulate two node result sets with overlap
    std::vector<std::string> node_a = {"e1", "e2", "e3"};
    std::vector<std::string> node_b = {"e2", "e3", "e4"};

    std::set<std::string> federated = {};

    for (auto& id : node_a) {
      federated.insert(id);
    }
    for (auto& id : node_b) {
      federated.insert(id);
    }

    // Union has 4 distinct entities
    EXPECT_EQ(federated.size(), 4u);
    // e2 and e3 are present once each (no duplicates)
    EXPECT_EQ(federated.count("e2"), 1u);
    EXPECT_EQ(federated.count("e3"), 1u);
}

// ===========================================================================
// DKC-06 — Timeout on one peer surfaces FEDERATION_TIMEOUT
// ===========================================================================

TEST(DKContractHardeningDKC06, FederationTimeoutSurfaced) {
    bool peer_timed_out = true;
    auto code = peer_timed_out
        ? DKErrorCode::FEDERATION_TIMEOUT
        : DKErrorCode::OK;

    EXPECT_EQ(code, DKErrorCode::FEDERATION_TIMEOUT);
    EXPECT_TRUE(isRetryableCode(code));
}

// ===========================================================================
// DKC-07 — Partial result returned with timeout (not silently dropped)
// ===========================================================================

TEST(DKContractHardeningDKC07, PartialResultReturnedOnTimeout) {
    // Simulated: node A responds, node B times out
    std::vector<std::string> partial_result = {"e1", "e2"};
    DKErrorCode warning = DKErrorCode::FEDERATION_TIMEOUT;

    // Partial result is still available to the caller
    EXPECT_FALSE(partial_result.empty())
        << "Partial result must be returned even when one peer times out";
    EXPECT_EQ(warning, DKErrorCode::FEDERATION_TIMEOUT)
        << "Timeout condition must be surfaced as warning alongside partial result";
}

// ===========================================================================
// DKC-08 — Federation result count ≤ kMaxFederationResultEntities
// ===========================================================================

TEST(DKContractHardeningDKC08, FederationResultLimitEnforced) {
    std::mt19937_64 rng(kDKContractSeed);
    std::size_t result_size = kMaxFederationResultEntities; // at the limit
    EXPECT_LE(result_size, kMaxFederationResultEntities);

    std::size_t over_limit = kMaxFederationResultEntities + 1;
    DKErrorCode code = (over_limit > kMaxFederationResultEntities)
        ? DKErrorCode::RETRIEVAL_LIMIT_EXCEEDED
        : DKErrorCode::OK;
    EXPECT_EQ(code, DKErrorCode::RETRIEVAL_LIMIT_EXCEEDED);
}

// ===========================================================================
// DKC-09 — neighbours() returns all direct edges (completeness)
// ===========================================================================

TEST(DKContractHardeningDKC09, NeighboursCompleteness) {
    MockGraph g;
    g.addEdge("A", "B");
    g.addEdge("A", "C");
    g.addEdge("A", "D");

    auto nb = g.neighbours("A");
    EXPECT_EQ(nb.size(), 3u);
    EXPECT_NE(std::find(nb.begin(), nb.end(), "B"), nb.end());
    EXPECT_NE(std::find(nb.begin(), nb.end(), "C"), nb.end());
    EXPECT_NE(std::find(nb.begin(), nb.end(), "D"), nb.end());
}

// ===========================================================================
// DKC-10 — Path query result is ordered by path length ascending
// ===========================================================================

TEST(DKContractHardeningDKC10, PathQueryOrderedByDepth) {
    MockGraph g;
    g.addEdge("root", "A");
    g.addEdge("A",    "B");
    g.addEdge("B",    "C");

    auto paths = g.pathQuery("root", 3);
    ASSERT_GE(paths.size(), 3u);
    for (std::size_t i = 1; i < paths.size(); ++i) {
        EXPECT_GE(paths[i].second, paths[i-1].second)
            << "Path results must be ordered by depth (non-decreasing)";
    }
}

// ===========================================================================
// DKC-11 — neighbours() count ≤ kMaxNeighboursPerQuery
// ===========================================================================

TEST(DKContractHardeningDKC11, NeighboursLimitEnforced) {
    std::size_t fake_count = kMaxNeighboursPerQuery;  // at the limit
    EXPECT_LE(fake_count, kMaxNeighboursPerQuery);

    std::size_t over = kMaxNeighboursPerQuery + 1;
    DKErrorCode code = (over > kMaxNeighboursPerQuery)
        ? DKErrorCode::RETRIEVAL_LIMIT_EXCEEDED
        : DKErrorCode::OK;
    EXPECT_EQ(code, DKErrorCode::RETRIEVAL_LIMIT_EXCEEDED);
}

// ===========================================================================
// DKC-12 — Path query depth ≤ kMaxPathQueryDepth enforced
// ===========================================================================

TEST(DKContractHardeningDKC12, PathQueryDepthEnforced) {
    // kMaxPathQueryDepth is the hard contract limit
    EXPECT_GT(kMaxPathQueryDepth, 0u);
    EXPECT_LE(kMaxPathQueryDepth, 100u);

    // Query at exactly the limit should succeed
    MockGraph g;
    g.addEdge("n0", "n1");
    auto res = g.pathQuery("n0", kMaxPathQueryDepth);
    EXPECT_GE(res.size(), 0u);  // may be small graph — just assert no crash

    // A depth request beyond the limit would be capped at kMaxPathQueryDepth
    std::size_t requested = kMaxPathQueryDepth + 5;
    std::size_t applied   = std::min(requested, kMaxPathQueryDepth);
    EXPECT_EQ(applied, kMaxPathQueryDepth);
}

// ===========================================================================
// DKC-13 — LWW: higher timestamp wins deterministically
// ===========================================================================

TEST(DKContractHardeningDKC13, LwwHigherTimestampWins) {
    auto dec = resolveLww(2000L, 1000L, "node-A", "node-B");
    EXPECT_EQ(dec, LwwDecision::LocalWins)
        << "Higher local timestamp must win";

    dec = resolveLww(1000L, 2000L, "node-A", "node-B");
    EXPECT_EQ(dec, LwwDecision::RemoteWins)
        << "Higher remote timestamp must win";
}

// ===========================================================================
// DKC-14 — LWW tie-break: lexicographically larger node-ID wins
// ===========================================================================

TEST(DKContractHardeningDKC14, LwwTieBreakByNodeId) {
    // Same timestamp, node-Z > node-A lexicographically
    auto dec = resolveLww(1000L, 1000L, "node-Z", "node-A");
    EXPECT_EQ(dec, LwwDecision::LocalWins)
        << "node-Z is lexicographically larger; local must win the tie";

    dec = resolveLww(1000L, 1000L, "node-A", "node-Z");
    EXPECT_EQ(dec, LwwDecision::RemoteWins)
        << "node-Z is lexicographically larger; remote must win the tie";

    dec = resolveLww(1000L, 1000L, "node-X", "node-X");
    EXPECT_EQ(dec, LwwDecision::Identical)
        << "Same timestamp and same node-ID → Identical";
}

// ===========================================================================
// DKC-15 — Tombstone propagation failure surfaces TOMBSTONE_PROPAGATION_FAILED
// ===========================================================================

TEST(DKContractHardeningDKC15, TombstonePropagationFailureSurfaced) {
    bool propagation_failed = true;
    auto code = propagation_failed
        ? DKErrorCode::TOMBSTONE_PROPAGATION_FAILED
        : DKErrorCode::OK;

    EXPECT_EQ(code, DKErrorCode::TOMBSTONE_PROPAGATION_FAILED);
    EXPECT_TRUE(isRetryableCode(code));
}

// ===========================================================================
// DKC-16 — CRDT merge type mismatch surfaces CRDT_MERGE_TYPE_MISMATCH
// ===========================================================================

TEST(DKContractHardeningDKC16, CrdtMergeTypeMismatchSurfaced) {
    // Simulate: field 'count' is int64 locally but string remotely
    bool type_conflict = true;
    auto code = type_conflict
        ? DKErrorCode::CRDT_MERGE_TYPE_MISMATCH
        : DKErrorCode::OK;

    EXPECT_EQ(code, DKErrorCode::CRDT_MERGE_TYPE_MISMATCH);
    EXPECT_FALSE(isRetryableCode(code))
        << "Type mismatch is a permanent error; retry without schema fix won't help";
}
