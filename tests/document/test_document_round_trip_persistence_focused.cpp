/**
 * @file test_document_round_trip_persistence_focused.cpp
 * @brief Round-trip persistence diagnostics — store-backed snapshot correctness
 *        and failure-path propagation hardening tests.
 *
 * @details Covers two test categories addressing MODULE_GAPS.md DOC-AUD-02:
 *
 *  - RoundTripPersistence  (RTP-01 … RTP-10): store-backed snapshot correctness
 *  - RoundTripDiagnostics  (RTD-01 … RTD-04): failure-path propagation via a
 *                                              controlled FailingDocumentStore
 *
 * The FailingDocumentStore test double is used exclusively in RTD tests to
 * exercise the error propagation paths in StoreBackedRoundTripEditor without
 * coupling to production storage subsystems.
 *
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Target: Q3–Q4 2026 hardening sprint (DOC-AUD-02)
 * @version 1.0.0
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * ThemisDB — Document Module Round-Trip Persistence Tests
 *
 * File:    test_document_round_trip_persistence_focused.cpp
 * Module:  tests/document/
 * Purpose: Focused regression and diagnostics tests for StoreBackedRoundTripEditor
 *          snapshot storage, ID determinism, field correctness, and error propagation.
 *
 * Version: 1.0.0
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "document/document_store.h"
#include "document/round_trip_editor.h"

#include <iomanip>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

using namespace themis;
using namespace themis::document;

// ─────────────────────────────────────────────────────────────────────────────
// FailingDocumentStore — test double for RTD error-path tests
// ─────────────────────────────────────────────────────────────────────────────

// NON-PRODUCTION PATH (Simulation/Stub/Mockup)
// Reason: Needed to inject controlled storage failures into StoreBackedRoundTripEditor
//         so that failure-propagation paths can be exercised without relying on
//         production storage infrastructure.
// Activation: Only instantiated from RTD test fixtures in this translation unit.
// Production Delta: Always returns ERR_DOC_NOT_FOUND for every operation;
//                  real IDocumentStore implementations perform actual persistence.
// Approved By: Task specification (Document Module Test Hardening, 2026-07-28)
// Removal Target: N/A — test-only fixture; lives exclusively in test code.
/**
 * @brief Test-only IDocumentStore implementation that returns ERR_DOC_NOT_FOUND
 *        for every operation.
 *
 * Used in RTD tests to verify that StoreBackedRoundTripEditor correctly
 * propagates storage-layer errors through its public API.
 *
 * @note This class is a test fixture only.  It must never appear in production
 *       build paths.  See NON-PRODUCTION PATH comment above.
 */
class FailingDocumentStore final : public IDocumentStore {
public:
    [[nodiscard]] Result<DocumentId> put(const DocumentRecord& /*record*/) override {
        return tl::unexpected(Error(errors::ErrorCode::ERR_DOC_NOT_FOUND, "injected"));
    }

    [[nodiscard]] Result<std::optional<DocumentRecord>> get(
        const CollectionId& /*collection*/,
        const DocumentId&   /*id*/) const override
    {
        return tl::unexpected(Error(errors::ErrorCode::ERR_DOC_NOT_FOUND, "injected"));
    }

    [[nodiscard]] Result<void> update(const CollectionId& /*collection*/,
                                      const DocumentId&   /*id*/,
                                      const nlohmann::json& /*body*/) override
    {
        return tl::unexpected(Error(errors::ErrorCode::ERR_DOC_NOT_FOUND, "injected"));
    }

    [[nodiscard]] Result<void> remove(const CollectionId& /*collection*/,
                                      const DocumentId&   /*id*/) override
    {
        return tl::unexpected(Error(errors::ErrorCode::ERR_DOC_NOT_FOUND, "injected"));
    }

    [[nodiscard]] Result<std::vector<DocumentId>> list(
        const CollectionId& /*collection*/) const override
    {
        return tl::unexpected(Error(errors::ErrorCode::ERR_DOC_NOT_FOUND, "injected"));
    }

    [[nodiscard]] Result<std::size_t> count(
        const CollectionId& /*collection*/) const override
    {
        return tl::unexpected(Error(errors::ErrorCode::ERR_DOC_NOT_FOUND, "injected"));
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Local helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/**
 * @brief Replicate the StoreBackedRoundTripEditor snapshot ID format for
 *        direct store-level verification.
 *
 * Format: relay_id + ':' + zero-padded 10-digit interaction_index
 * (matches round_trip_editor.cpp makeSnapshotId()).
 *
 * @param relay_id          Relay identifier.
 * @param interaction_index Snapshot index.
 * @return Deterministic snapshot document ID string.
 */
std::string makeExpectedSnapshotId(const std::string& relay_id,
                                   std::size_t interaction_index)
{
    std::ostringstream oss;
    oss << relay_id << ':' << std::setw(10) << std::setfill('0') << interaction_index;
    return oss.str();
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// RoundTripPersistence tests  (RTP-01 … RTP-10)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Test fixture for store-backed snapshot persistence correctness.
 *
 * Owns a real InMemoryDocumentStore so that both the editor API and the
 * underlying store can be probed for correctness.
 */
class RoundTripPersistenceTest : public ::testing::Test {
protected:
    InMemoryDocumentStore      store_;
    StoreBackedRoundTripEditor editor_{store_};
    const CollectionId         kCol{kDefaultRoundTripCollection};
};

// RTP-01: beginRelay() stores the seed snapshot with interaction_index == 0
TEST_F(RoundTripPersistenceTest, RTP01_BeginRelayStoresSeedAtIndexZero) {
    ASSERT_TRUE(editor_.beginRelay("rtp01", "seed-content").has_value());

    auto loaded = editor_.loadInteraction("rtp01", 0);
    ASSERT_TRUE(loaded.has_value());
    ASSERT_TRUE(loaded.value().has_value())
        << "Seed snapshot (interaction_index=0) must be retrievable after beginRelay().";
    EXPECT_EQ(loaded.value().value().interaction_index, 0u);
    EXPECT_EQ(loaded.value().value().relay_id, "rtp01");
}

// RTP-02: saveInteraction() at index 1 stores a snapshot distinct from the seed
TEST_F(RoundTripPersistenceTest, RTP02_SaveInteractionAtIndex1_DistinctFromSeed) {
    ASSERT_TRUE(editor_.beginRelay("rtp02", "seed-doc").has_value());
    ASSERT_TRUE(editor_.saveInteraction("rtp02", 1, "edit-1", "v1-doc").has_value());

    auto seed = editor_.loadInteraction("rtp02", 0);
    auto snap = editor_.loadInteraction("rtp02", 1);

    ASSERT_TRUE(seed.has_value() && seed.value().has_value());
    ASSERT_TRUE(snap.has_value() && snap.value().has_value());

    EXPECT_EQ(seed.value().value().document, "seed-doc");
    EXPECT_EQ(snap.value().value().document, "v1-doc");
    EXPECT_NE(seed.value().value().document, snap.value().value().document)
        << "Seed and interaction 1 must have different document bodies.";
}

// RTP-03: countSnapshots() is consistent with N saveInteraction() calls including seed
TEST_F(RoundTripPersistenceTest, RTP03_CountSnapshotsConsistentWithSaves) {
    ASSERT_TRUE(editor_.beginRelay("rtp03", "seed").has_value());
    ASSERT_TRUE(editor_.saveInteraction("rtp03", 1, "i1", "d1").has_value());
    ASSERT_TRUE(editor_.saveInteraction("rtp03", 2, "i2", "d2").has_value());

    auto r = editor_.countSnapshots("rtp03");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 3u)
        << "countSnapshots() must return 3: seed (0) + interaction 1 + interaction 2.";
}

// RTP-04: snapshot IDs follow relay_id + ':' + zero-padded 10-digit index format
TEST_F(RoundTripPersistenceTest, RTP04_SnapshotIdsDeterministic) {
    const std::string relay_id = "rtp04";
    ASSERT_TRUE(editor_.beginRelay(relay_id, "seed").has_value());
    ASSERT_TRUE(editor_.saveInteraction(relay_id, 1, "instr", "doc").has_value());

    const std::string expected_seed_id  = makeExpectedSnapshotId(relay_id, 0);
    const std::string expected_snap1_id = makeExpectedSnapshotId(relay_id, 1);

    EXPECT_EQ(expected_seed_id,  relay_id + ":0000000000");
    EXPECT_EQ(expected_snap1_id, relay_id + ":0000000001");

    // Verify both IDs are directly retrievable from the backing store.
    auto seed_rec = store_.get(kCol, expected_seed_id);
    auto snap_rec = store_.get(kCol, expected_snap1_id);
    ASSERT_TRUE(seed_rec.has_value() && seed_rec.value().has_value());
    ASSERT_TRUE(snap_rec.has_value() && snap_rec.value().has_value());
}

// RTP-05: store confirms snapshot stored with correct document ID key
TEST_F(RoundTripPersistenceTest, RTP05_StoreDirectlyConfirmsSnapshotKey) {
    ASSERT_TRUE(editor_.beginRelay("rtp05", "my-seed").has_value());

    const std::string expected_id = makeExpectedSnapshotId("rtp05", 0);
    auto rec = store_.get(kCol, expected_id);
    ASSERT_TRUE(rec.has_value());
    ASSERT_TRUE(rec.value().has_value())
        << "Backing store must contain a document at the deterministic seed ID.";
    EXPECT_EQ(rec.value().value().id, expected_id);
    EXPECT_EQ(rec.value().value().collection_id, kCol);
}

// RTP-06: loadInteraction(index=0) returns the seed document exactly
TEST_F(RoundTripPersistenceTest, RTP06_LoadIndex0_ReturnsSeedExactly) {
    ASSERT_TRUE(editor_.beginRelay("rtp06", "exact-seed-content").has_value());

    auto r = editor_.loadInteraction("rtp06", 0);
    ASSERT_TRUE(r.has_value());
    ASSERT_TRUE(r.value().has_value());
    EXPECT_EQ(r.value().value().document, "exact-seed-content");
    EXPECT_EQ(r.value().value().interaction_index, 0u);
    EXPECT_EQ(r.value().value().relay_id, "rtp06");
}

// RTP-07: loadInteraction(index=N) returns the Nth interaction document exactly
TEST_F(RoundTripPersistenceTest, RTP07_LoadIndexN_ReturnsNthInteractionExactly) {
    ASSERT_TRUE(editor_.beginRelay("rtp07", "seed").has_value());
    ASSERT_TRUE(editor_.saveInteraction("rtp07", 1, "inst-1", "doc-1").has_value());
    ASSERT_TRUE(editor_.saveInteraction("rtp07", 2, "inst-2", "doc-2").has_value());
    ASSERT_TRUE(editor_.saveInteraction("rtp07", 3, "inst-3", "doc-3").has_value());

    auto r = editor_.loadInteraction("rtp07", 3);
    ASSERT_TRUE(r.has_value());
    ASSERT_TRUE(r.value().has_value());
    const auto& snap = r.value().value();
    EXPECT_EQ(snap.interaction_index, 3u);
    EXPECT_EQ(snap.instruction, "inst-3");
    EXPECT_EQ(snap.document, "doc-3");
}

// RTP-08: relay_id containing ':' and digits produces valid snapshot IDs
TEST_F(RoundTripPersistenceTest, RTP08_NonAlphanumericRelayId_ValidSnapshotIds) {
    const std::string relay_id = "relay:42:session";
    ASSERT_TRUE(editor_.beginRelay(relay_id, "seed-rtp08").has_value());

    const std::string expected_id = makeExpectedSnapshotId(relay_id, 0);
    auto r = store_.get(kCol, expected_id);
    ASSERT_TRUE(r.has_value())
        << "store_.get() must succeed for relay_id containing ':' and digits.";
    ASSERT_TRUE(r.value().has_value());

    // loadInteraction() must also resolve correctly.
    auto loaded = editor_.loadInteraction(relay_id, 0);
    ASSERT_TRUE(loaded.has_value() && loaded.value().has_value());
    EXPECT_EQ(loaded.value().value().relay_id, relay_id);
}

// RTP-09: large interaction_index (999999999) produces a correct 10-digit ID
TEST_F(RoundTripPersistenceTest, RTP09_LargeInteractionIndex_CorrectTenDigitId) {
    constexpr std::size_t kLargeIndex = 999999999u;
    ASSERT_TRUE(editor_.beginRelay("rtp09", "seed").has_value());
    ASSERT_TRUE(editor_.saveInteraction("rtp09", kLargeIndex,
                                        "large-instr", "large-doc").has_value());

    const std::string expected_id = makeExpectedSnapshotId("rtp09", kLargeIndex);
    EXPECT_EQ(expected_id, "rtp09:0999999999")
        << "999999999 is 9 digits; zero-padded to 10 must yield '0999999999'.";

    auto r = store_.get(kCol, expected_id);
    ASSERT_TRUE(r.has_value() && r.value().has_value());
}

// RTP-10: snapshot body JSON contains all required fields
TEST_F(RoundTripPersistenceTest, RTP10_SnapshotBodyContainsAllRequiredFields) {
    ASSERT_TRUE(editor_.beginRelay("rtp10", "seed-body").has_value());

    const std::string seed_id = makeExpectedSnapshotId("rtp10", 0);
    auto rec = store_.get(kCol, seed_id);
    ASSERT_TRUE(rec.has_value() && rec.value().has_value());
    const nlohmann::json& body = rec.value().value().body;

    EXPECT_TRUE(body.contains("relay_id"))
        << "Snapshot body must contain 'relay_id'.";
    EXPECT_TRUE(body.contains("interaction_index"))
        << "Snapshot body must contain 'interaction_index'.";
    EXPECT_TRUE(body.contains("instruction"))
        << "Snapshot body must contain 'instruction'.";
    EXPECT_TRUE(body.contains("document"))
        << "Snapshot body must contain 'document'.";
    EXPECT_TRUE(body.contains("created_at_ms"))
        << "Snapshot body must contain 'created_at_ms'.";

    EXPECT_EQ(body["relay_id"], "rtp10");
    EXPECT_EQ(body["interaction_index"], 0);
    EXPECT_EQ(body["document"], "seed-body");
}

// ─────────────────────────────────────────────────────────────────────────────
// RoundTripDiagnostics tests  (RTD-01 … RTD-04)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Test fixture for failure-path diagnostics.
 *
 * Uses a FailingDocumentStore to inject ERR_DOC_NOT_FOUND into every storage
 * call, verifying that StoreBackedRoundTripEditor correctly propagates errors
 * rather than swallowing them.
 */
class RoundTripDiagnosticsTest : public ::testing::Test {
protected:
    FailingDocumentStore       failing_store_;
    StoreBackedRoundTripEditor editor_{failing_store_};
};

// RTD-01: beginRelay() failure (backing store returns error) propagates the error Result
TEST_F(RoundTripDiagnosticsTest, RTD01_BeginRelayStoreFailure_PropagatesError) {
    auto r = editor_.beginRelay("rtd01-relay", "seed");
    ASSERT_FALSE(r.has_value())
        << "beginRelay() must propagate a storage-layer error when store.put() fails.";
    EXPECT_EQ(r.error().code(), errors::ErrorCode::ERR_DOC_NOT_FOUND);
}

// RTD-02: saveInteraction() failure propagates the error Result
TEST_F(RoundTripDiagnosticsTest, RTD02_SaveInteractionStoreFailure_PropagatesError) {
    auto r = editor_.saveInteraction("rtd02-relay", 1, "instr", "doc");
    ASSERT_FALSE(r.has_value())
        << "saveInteraction() must propagate a storage-layer error when store.put() fails.";
    EXPECT_EQ(r.error().code(), errors::ErrorCode::ERR_DOC_NOT_FOUND);
}

// RTD-03: loadInteraction() store failure propagates the error Result
TEST_F(RoundTripDiagnosticsTest, RTD03_LoadInteractionStoreFailure_PropagatesError) {
    auto r = editor_.loadInteraction("rtd03-relay", 0);
    ASSERT_FALSE(r.has_value())
        << "loadInteraction() must propagate a storage-layer error when store.get() fails.";
    EXPECT_EQ(r.error().code(), errors::ErrorCode::ERR_DOC_NOT_FOUND);
}

// RTD-04: countSnapshots() store.list() failure propagates the error Result
TEST_F(RoundTripDiagnosticsTest, RTD04_CountSnapshotsStoreListFailure_PropagatesError) {
    auto r = editor_.countSnapshots("rtd04-relay");
    ASSERT_FALSE(r.has_value())
        << "countSnapshots() must propagate a storage-layer error when store.list() fails.";
    EXPECT_EQ(r.error().code(), errors::ErrorCode::ERR_DOC_NOT_FOUND);
}
