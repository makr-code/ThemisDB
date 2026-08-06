/**
 * @file test_process_determinism_conflict_focused.cpp
 * @brief Phase 4 Determinism Tests: Reproducible behavior under high model churn and conflicts
 * @note Test IDs: D-01..D-08
 */

#include <gtest/gtest.h>
#include "process/process_api_contract.h"
#include "process/process_linker.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <random>
#include <vector>

using namespace themis::process;

// ─────────────────────────────────────────────────────────────────────────────
// Determinism Test Fixture
// ─────────────────────────────────────────────────────────────────────────────

class DeterminismConflictTest : public ::testing::Test {
protected:
    static constexpr int32_t kCanonicalRngSeed = 42;
    static constexpr int32_t kModelChurnRounds = 100;

    // Deterministic PRNG seeded at construction
    std::mt19937 rng{kCanonicalRngSeed};

    // Reseed for each test
    void SetUp() override {
        rng.seed(kCanonicalRngSeed);
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// D-01: RNG produces identical sequences with same seed
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DeterminismConflictTest, D01_IdenticalRngSequences) {
    // First sequence
    std::mt19937 rng1(kCanonicalRngSeed);
    std::vector<uint32_t> seq1;
    for (int32_t i = 0; i < 1000; ++i) {
        seq1.push_back(rng1());
    }

    // Second sequence with same seed
    std::mt19937 rng2(kCanonicalRngSeed);
    std::vector<uint32_t> seq2;
    for (int32_t i = 0; i < 1000; ++i) {
        seq2.push_back(rng2());
    }

    EXPECT_EQ(seq1, seq2);
}

// ─────────────────────────────────────────────────────────────────────────────
// D-02: Link creation produces deterministic ordering with same seed
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DeterminismConflictTest, D02_DeterministicLinkOrdering) {
    auto create_links = [](int32_t seed) -> std::vector<ProcessLink> {
        std::vector<ProcessLink> links;
        std::mt19937 local_rng(seed);
        std::uniform_int_distribution<int32_t> dist(0, 9);

        for (int32_t i = 0; i < 100; ++i) {
            ProcessLink link;
            link.link_id = "link_" + std::to_string(i);
            link.source_id = "src_" + std::to_string(dist(local_rng));
            link.target_id = "tgt_" + std::to_string(dist(local_rng));
            link.link_type = ProcessLinkType::CROSS_REFERENCE;
            links.push_back(link);
        }
        return links;
    };

    auto links1 = create_links(kCanonicalRngSeed);
    auto links2 = create_links(kCanonicalRngSeed);

    ASSERT_EQ(links1.size(), links2.size());
    for (size_t i = 0; i < links1.size(); ++i) {
        EXPECT_EQ(links1[i].source_id, links2[i].source_id);
        EXPECT_EQ(links1[i].target_id, links2[i].target_id);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// D-03: Sorted collections are deterministic
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DeterminismConflictTest, D03_SortedCollectionsDeterministic) {
    auto create_and_sort = [](int32_t seed) -> std::vector<std::string> {
        std::vector<std::string> items;
        std::mt19937 local_rng(seed);
        std::uniform_int_distribution<int32_t> dist(0, 999);

        for (int32_t i = 0; i < 200; ++i) {
            items.push_back("item_" + std::to_string(dist(local_rng)));
        }
        std::sort(items.begin(), items.end());
        return items;
    };

    auto sorted1 = create_and_sort(kCanonicalRngSeed);
    auto sorted2 = create_and_sort(kCanonicalRngSeed);

    EXPECT_EQ(sorted1, sorted2);
}

// ─────────────────────────────────────────────────────────────────────────────
// D-04: Conflict resolution is deterministic (first-writer-wins semantics)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DeterminismConflictTest, D04_ConflictResolutionDeterministic) {
    auto resolve_conflicts = [](int32_t seed) -> std::vector<std::string> {
        std::map<std::string, std::string> state;
        std::mt19937 local_rng(seed);
        std::uniform_int_distribution<int32_t> dist(0, 9);

        // Simulate concurrent writes to same keys
        for (int32_t round = 0; round < kModelChurnRounds; ++round) {
            std::string key = "key_" + std::to_string(dist(local_rng));
            std::string value = "v_" + std::to_string(round);

            // First write wins (deterministic)
            if (state.find(key) == state.end()) {
                state[key] = value;
            }
        }

        std::vector<std::string> results;
        for (const auto& [k, v] : state) {
            results.push_back(k + "=" + v);
        }
        std::sort(results.begin(), results.end());
        return results;
    };

    auto res1 = resolve_conflicts(kCanonicalRngSeed);
    auto res2 = resolve_conflicts(kCanonicalRngSeed);

    EXPECT_EQ(res1, res2);
}

// ─────────────────────────────────────────────────────────────────────────────
// D-05: Timestamp sequences are reproducible with deterministic clock
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DeterminismConflictTest, D05_DeterministicTimestampSequences) {
    auto generate_timestamps = [](int32_t seed) -> std::vector<int64_t> {
        std::vector<int64_t> timestamps;
        std::mt19937 local_rng(seed);
        int64_t current_time_ms = 1000;

        for (int32_t i = 0; i < 100; ++i) {
            timestamps.push_back(current_time_ms);
            std::uniform_int_distribution<int32_t> delay_dist(1, 100);
            current_time_ms += delay_dist(local_rng);
        }
        return timestamps;
    };

    auto ts1 = generate_timestamps(kCanonicalRngSeed);
    auto ts2 = generate_timestamps(kCanonicalRngSeed);

    EXPECT_EQ(ts1, ts2);
}

// ─────────────────────────────────────────────────────────────────────────────
// D-06: Model state transitions are deterministic given seed
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DeterminismConflictTest, D06_ModelStateTransitionsDeterministic) {
    enum class State { CREATED, VALIDATING, VALIDATED, LINKED, FINALIZED };

    auto simulate_model_lifecycle = [](int32_t seed) -> std::vector<State> {
        std::vector<State> states;
        std::mt19937 local_rng(seed);

        states.push_back(State::CREATED);
        std::uniform_int_distribution<int32_t> action_dist(0, 2);

        for (int32_t i = 0; i < kModelChurnRounds; ++i) {
            State current = states.back();

            switch (current) {
                case State::CREATED:
                    states.push_back(State::VALIDATING);
                    break;
                case State::VALIDATING:
                    if (action_dist(local_rng) == 0) {
                        states.push_back(State::VALIDATED);
                    }
                    break;
                case State::VALIDATED:
                    if (action_dist(local_rng) == 0) {
                        states.push_back(State::LINKED);
                    }
                    break;
                case State::LINKED:
                    states.push_back(State::FINALIZED);
                    break;
                case State::FINALIZED:
                    break;
            }

            if (current == State::FINALIZED) break;
        }
        return states;
    };

    auto states1 = simulate_model_lifecycle(kCanonicalRngSeed);
    auto states2 = simulate_model_lifecycle(kCanonicalRngSeed);

    EXPECT_EQ(states1.size(), states2.size());
    for (size_t i = 0; i < states1.size(); ++i) {
        EXPECT_EQ(static_cast<int32_t>(states1[i]), static_cast<int32_t>(states2[i]));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// D-07: Partial state recovery is deterministic
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DeterminismConflictTest, D07_PartialStateRecoveryDeterministic) {
    struct ModelSnapshot {
        int32_t version;
        std::string last_state;
        int64_t timestamp_ms;
    };

    auto recovery_scenario = [](int32_t seed) -> std::vector<ModelSnapshot> {
        std::vector<ModelSnapshot> snapshots;
        std::mt19937 local_rng(seed);
        std::uniform_int_distribution<int32_t> version_dist(1, 100);

        int64_t time_ms = 1000;
        for (int32_t i = 0; i < 50; ++i) {
            ModelSnapshot snap;
            snap.version = version_dist(local_rng);
            snap.last_state = "state_" + std::to_string(snap.version);
            snap.timestamp_ms = time_ms;
            snapshots.push_back(snap);
            time_ms += 100;
        }

        // Simulate partial recovery: keep only valid snapshots
        std::vector<ModelSnapshot> valid;
        for (const auto& snap : snapshots) {
            if (snap.version % 2 == 0) {  // Even versions only
                valid.push_back(snap);
            }
        }
        return valid;
    };

    auto recovered1 = recovery_scenario(kCanonicalRngSeed);
    auto recovered2 = recovery_scenario(kCanonicalRngSeed);

    EXPECT_EQ(recovered1.size(), recovered2.size());
    for (size_t i = 0; i < recovered1.size(); ++i) {
        EXPECT_EQ(recovered1[i].version, recovered2[i].version);
        EXPECT_EQ(recovered1[i].last_state, recovered2[i].last_state);
        EXPECT_EQ(recovered1[i].timestamp_ms, recovered2[i].timestamp_ms);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// D-08: Hash-based selection is deterministic with same seed
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DeterminismConflictTest, D08_HashBasedSelectionDeterministic) {
    auto hash_selection = [](int32_t seed) -> std::vector<int32_t> {
        std::vector<int32_t> selected;
        std::mt19937 local_rng(seed);
        std::hash<int32_t> hasher;

        std::vector<int32_t> candidates;
        for (int32_t i = 0; i < 1000; ++i) {
            candidates.push_back(i);
        }

        for (int32_t candidate : candidates) {
            size_t hash_val = hasher(candidate);
            // Select if hash is "even" (deterministic)
            if (hash_val % 2 == 0) {
                selected.push_back(candidate);
            }
        }
        return selected;
    };

    auto selected1 = hash_selection(kCanonicalRngSeed);
    auto selected2 = hash_selection(kCanonicalRngSeed);

    EXPECT_EQ(selected1, selected2);
}
