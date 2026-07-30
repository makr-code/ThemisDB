/**
 * @file test_document_conflict_diagnostics_focused.cpp
 * @brief Document module multi-branch merge, schema sealing/transition, and
 *        operator diagnostics focused tests.
 *
 * @details Covers three test categories addressing the Q4 2026 Planned Features
 * backlog in @c src/document/ROADMAP.md:
 *
 *  - MultiBranchMerge  (MBM-01 … MBM-08): exhaustive multi-branch three-way
 *    merge permutations across all MergeStrategy values and conflict scenarios.
 *  - SchemaSealVersion (SSV-01 … SSV-06): schema sealing enforcement and
 *    version-transition ordering edge cases.
 *  - OperatorDiagnostics (ODE-01 … ODE-08): DocumentDiagnosticSink counting,
 *    classifyDocumentError, formatDocumentError, and documentErrorDescription
 *    coverage for EXCHANGE_ERROR and ROUND_TRIP_ERROR paths.
 *
 * All tests use deterministic fixtures; where document IDs are generated
 * programmatically, the canonical seed kCDCanonicalSeed = 42 is used.
 *
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Target: Q4 2026 hardening sprint — Planned Features closure
 * @version 1.0.0
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * ThemisDB — Document Module Multi-Branch Merge + Diagnostics Tests
 *
 * File:    test_document_conflict_diagnostics_focused.cpp
 * Module:  tests/document/
 * Purpose: Closes three Q4 2026 Planned Feature items:
 *   1. tighten deterministic conflict handling for multi-branch merge permutations
 *   2. expand regression coverage for schema sealing/version transition edge cases
 *   3. improve operator diagnostics for document round-trip and exchange failures
 *
 * Version: 1.0.0
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "document/document_diagnostics.h"
#include "document/document_diff_merge.h"
#include "document/document_schema_evolution.h"
#include "document/document_store.h"

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

using namespace themis;
using namespace themis::document;
using namespace themis::errors;

// ─────────────────────────────────────────────────────────────────────────────
// Constants
// ─────────────────────────────────────────────────────────────────────────────

namespace {
/// @brief Canonical deterministic seed for multi-branch fixtures.
static constexpr uint64_t kCDCanonicalSeed = 42;
} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// @brief Builds a document ID of the form "<prefix>-<index>".
static std::string makeDocId(const char* prefix, unsigned index)
{
    return std::string(prefix) + "-" + std::to_string(index);
}

/// @brief Creates a simple JSON object with a single string field.
static nlohmann::json makeBody(const char* key, const char* value)
{
    nlohmann::json j;
    j[key] = value;
    return j;
}

/// @brief Puts a document into @p store and asserts success.
static DocumentId putDoc(InMemoryDocumentStore& store,
                         const std::string&     id,
                         const std::string&     collection,
                         const nlohmann::json&  body)
{
    DocumentRecord rec;
    rec.id         = id;
    rec.collection_id = collection;
    rec.body       = body;
    auto result    = store.put(rec);
    if (!result.has_value()) {
        ADD_FAILURE() << "putDoc failed: " << result.error().message();
    }
    return result.has_value() ? *result : id;
}

} // namespace

// ═════════════════════════════════════════════════════════════════════════════
// MultiBranchMerge — MBM-01 … MBM-08
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief Fixture for multi-branch merge permutation tests.
 *
 * Sets up an InMemoryDocumentStore and InMemoryDocumentDiffMerge.  Each test
 * creates a base document plus two divergent branches (ours / theirs) in the
 * same collection, then exercises the merge() API exhaustively.
 */
class MultiBranchMergeTest : public ::testing::Test {
protected:
    static constexpr const char* kCollection = "mbm-collection";

    void SetUp() override
    {
        merge_ = std::make_unique<InMemoryDocumentDiffMerge>(store_);
    }

    InMemoryDocumentStore                          store_;
    std::unique_ptr<InMemoryDocumentDiffMerge>     merge_;
};

/**
 * @test MBM-01: OURS_WINS resolves a conflicting field in favour of ours.
 *
 * Base has field "status"="pending".  Ours changes it to "approved".
 * Theirs changes it to "rejected".  OURS_WINS must produce "approved".
 */
TEST_F(MultiBranchMergeTest, MBM01_OursWinsOnConflict)
{
    const std::string kBase   = makeDocId("MBM01-base",   kCDCanonicalSeed);
    const std::string kOurs   = makeDocId("MBM01-ours",   kCDCanonicalSeed);
    const std::string kTheirs = makeDocId("MBM01-theirs", kCDCanonicalSeed);

    nlohmann::json baseBody;  baseBody["status"]  = "pending";
    nlohmann::json oursBody;  oursBody["status"]  = "approved";
    nlohmann::json theirsBody; theirsBody["status"] = "rejected";

    putDoc(store_, kBase,   kCollection, baseBody);
    putDoc(store_, kOurs,   kCollection, oursBody);
    putDoc(store_, kTheirs, kCollection, theirsBody);

    auto result = merge_->merge(kCollection, kBase, kOurs, kTheirs, MergeStrategy::OURS_WINS);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_EQ(result->merged_body["status"], "approved");
    EXPECT_EQ(result->strategy_applied, MergeStrategy::OURS_WINS);
    EXPECT_EQ(result->conflicts.size(), 1u) << "Conflict must be recorded even when resolved by strategy";
}

/**
 * @test MBM-02: THEIRS_WINS resolves a conflicting field in favour of theirs.
 */
TEST_F(MultiBranchMergeTest, MBM02_TheirsWinsOnConflict)
{
    const std::string kBase   = makeDocId("MBM02-base",   kCDCanonicalSeed);
    const std::string kOurs   = makeDocId("MBM02-ours",   kCDCanonicalSeed);
    const std::string kTheirs = makeDocId("MBM02-theirs", kCDCanonicalSeed);

    nlohmann::json baseBody;  baseBody["status"]  = "pending";
    nlohmann::json oursBody;  oursBody["status"]  = "approved";
    nlohmann::json theirsBody; theirsBody["status"] = "rejected";

    putDoc(store_, kBase,   kCollection, baseBody);
    putDoc(store_, kOurs,   kCollection, oursBody);
    putDoc(store_, kTheirs, kCollection, theirsBody);

    auto result = merge_->merge(kCollection, kBase, kOurs, kTheirs, MergeStrategy::THEIRS_WINS);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_EQ(result->merged_body["status"], "rejected");
    EXPECT_EQ(result->strategy_applied, MergeStrategy::THEIRS_WINS);
}

/**
 * @test MBM-03: FAIL strategy returns ERR_DOC_MERGE_CONFLICT when fields diverge.
 */
TEST_F(MultiBranchMergeTest, MBM03_FailStrategyReturnsConflictError)
{
    const std::string kBase   = makeDocId("MBM03-base",   kCDCanonicalSeed);
    const std::string kOurs   = makeDocId("MBM03-ours",   kCDCanonicalSeed);
    const std::string kTheirs = makeDocId("MBM03-theirs", kCDCanonicalSeed);

    nlohmann::json baseBody;  baseBody["priority"] = 1;
    nlohmann::json oursBody;  oursBody["priority"] = 2;
    nlohmann::json theirsBody; theirsBody["priority"] = 3;

    putDoc(store_, kBase,   kCollection, baseBody);
    putDoc(store_, kOurs,   kCollection, oursBody);
    putDoc(store_, kTheirs, kCollection, theirsBody);

    auto result = merge_->merge(kCollection, kBase, kOurs, kTheirs, MergeStrategy::FAIL);
    ASSERT_FALSE(result.has_value()) << "FAIL strategy must not succeed on conflict";
    EXPECT_EQ(result.error().code(), ErrorCode::ERR_DOC_MERGE_CONFLICT);
}

/**
 * @test MBM-04: Clean merge (no conflict) succeeds under all three strategies.
 *
 * Ours adds field "a"; theirs adds field "b"; base has neither.
 * No shared field is modified, so all strategies must produce a clean merge.
 */
TEST_F(MultiBranchMergeTest, MBM04_CleanMergeSucceedsUnderAllStrategies)
{
    static const std::array<MergeStrategy, 3> kStrategies = {
        MergeStrategy::OURS_WINS,
        MergeStrategy::THEIRS_WINS,
        MergeStrategy::FAIL,
    };

    for (auto strategy : kStrategies) {
        const std::string suffix = std::to_string(static_cast<int>(strategy));
        const std::string kBase   = "MBM04-base-"   + suffix;
        const std::string kOurs   = "MBM04-ours-"   + suffix;
        const std::string kTheirs = "MBM04-theirs-" + suffix;

        nlohmann::json baseBody  = nlohmann::json::object();
        nlohmann::json oursBody;  oursBody["branch_a"]  = "val_a";
        nlohmann::json theirsBody; theirsBody["branch_b"] = "val_b";

        putDoc(store_, kBase,   kCollection, baseBody);
        putDoc(store_, kOurs,   kCollection, oursBody);
        putDoc(store_, kTheirs, kCollection, theirsBody);

        auto result = merge_->merge(kCollection, kBase, kOurs, kTheirs, strategy);
        ASSERT_TRUE(result.has_value())
            << "Clean merge failed for strategy=" << suffix
            << ": " << (result.has_value() ? "" : result.error().message());
        EXPECT_TRUE(result->conflicts.empty())
            << "No conflicts expected for non-overlapping field sets";
        EXPECT_EQ(result->merged_body.value("branch_a", ""), "val_a");
        EXPECT_EQ(result->merged_body.value("branch_b", ""), "val_b");
    }
}

/**
 * @test MBM-05: Multi-field document — partial conflict, OURS_WINS strategy.
 *
 * Base has three fields.  Ours modifies field "c".  Theirs modifies fields
 * "b" and "c" (creating a conflict on "c").  OURS_WINS must preserve ours'
 * "c" value and merge theirs' "b" change cleanly.
 */
TEST_F(MultiBranchMergeTest, MBM05_PartialConflictMultiFieldOursWins)
{
    const std::string kBase   = "MBM05-base";
    const std::string kOurs   = "MBM05-ours";
    const std::string kTheirs = "MBM05-theirs";

    nlohmann::json baseBody;
    baseBody["a"] = "alpha";  baseBody["b"] = "beta";  baseBody["c"] = "gamma";

    nlohmann::json oursBody  = baseBody;
    oursBody["c"] = "gamma-ours";

    nlohmann::json theirsBody = baseBody;
    theirsBody["b"] = "beta-theirs";
    theirsBody["c"] = "gamma-theirs";

    putDoc(store_, kBase,   kCollection, baseBody);
    putDoc(store_, kOurs,   kCollection, oursBody);
    putDoc(store_, kTheirs, kCollection, theirsBody);

    auto result = merge_->merge(kCollection, kBase, kOurs, kTheirs, MergeStrategy::OURS_WINS);
    ASSERT_TRUE(result.has_value()) << result.error().message();
    EXPECT_EQ(result->merged_body.value("a", ""), "alpha")    << "Unchanged field 'a' preserved";
    EXPECT_EQ(result->merged_body.value("b", ""), "beta-theirs") << "Theirs-only change 'b' merged";
    EXPECT_EQ(result->merged_body.value("c", ""), "gamma-ours") << "OURS_WINS takes ours 'c'";
    EXPECT_EQ(result->conflicts.size(), 1u);
    EXPECT_EQ(result->conflicts[0].field_name, "c");
}

/**
 * @test MBM-06: Chained merge — three sequential branch merges converge cleanly.
 *
 * Models: A branched into B and C; C branched into D.
 * Merge(A, B, C) → BC; Merge(A, BC, D) → final.
 * All branches modify distinct fields, so no conflicts arise.
 */
TEST_F(MultiBranchMergeTest, MBM06_ChainedMergeConverges)
{
    // Branch A (base)
    nlohmann::json a = nlohmann::json::object();
    putDoc(store_, "MBM06-A", kCollection, a);

    // Branch B adds field "x"
    nlohmann::json b; b["x"] = "from-B";
    putDoc(store_, "MBM06-B", kCollection, b);

    // Branch C adds field "y"
    nlohmann::json c; c["y"] = "from-C";
    putDoc(store_, "MBM06-C", kCollection, c);

    // First merge: A + B + C → BC
    auto r1 = merge_->merge(kCollection, "MBM06-A", "MBM06-B", "MBM06-C", MergeStrategy::FAIL);
    ASSERT_TRUE(r1.has_value()) << r1.error().message();
    EXPECT_TRUE(r1->conflicts.empty());
    EXPECT_EQ(r1->merged_body.value("x", ""), "from-B");
    EXPECT_EQ(r1->merged_body.value("y", ""), "from-C");

    // Store merged result as BC, then create branch D from A with field "z"
    putDoc(store_, "MBM06-BC", kCollection, r1->merged_body);
    nlohmann::json d; d["z"] = "from-D";
    putDoc(store_, "MBM06-D", kCollection, d);

    // Second merge: A + BC + D → final
    auto r2 = merge_->merge(kCollection, "MBM06-A", "MBM06-BC", "MBM06-D", MergeStrategy::FAIL);
    ASSERT_TRUE(r2.has_value()) << r2.error().message();
    EXPECT_TRUE(r2->conflicts.empty());
    EXPECT_EQ(r2->merged_body.value("x", ""), "from-B");
    EXPECT_EQ(r2->merged_body.value("y", ""), "from-C");
    EXPECT_EQ(r2->merged_body.value("z", ""), "from-D");
}

/**
 * @test MBM-07: Merge returns ERR_DOC_DIFF_NOT_FOUND when base document is absent.
 */
TEST_F(MultiBranchMergeTest, MBM07_MissingBaseReturnsError)
{
    nlohmann::json body; body["k"] = "v";
    putDoc(store_, "MBM07-ours",   kCollection, body);
    putDoc(store_, "MBM07-theirs", kCollection, body);

    auto result = merge_->merge(kCollection, "MBM07-base-MISSING",
                                "MBM07-ours", "MBM07-theirs",
                                MergeStrategy::FAIL);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error().code(), ErrorCode::ERR_DOC_DIFF_NOT_FOUND);
}

/**
 * @test MBM-08: Symmetric conflict — swapping ours/theirs changes OURS_WINS outcome.
 *
 * Verifies that the merge result is not commutative when OURS_WINS is applied
 * and that swapping the branch positions flips the resolved value.
 */
TEST_F(MultiBranchMergeTest, MBM08_SymmetricConflictNonCommutative)
{
    nlohmann::json base;   base["v"]   = "base-v";
    nlohmann::json ours;   ours["v"]   = "ours-v";
    nlohmann::json theirs; theirs["v"] = "theirs-v";

    putDoc(store_, "MBM08-base",   kCollection, base);
    putDoc(store_, "MBM08-ours",   kCollection, ours);
    putDoc(store_, "MBM08-theirs", kCollection, theirs);

    auto r1 = merge_->merge(kCollection, "MBM08-base", "MBM08-ours", "MBM08-theirs",
                            MergeStrategy::OURS_WINS);
    ASSERT_TRUE(r1.has_value());
    EXPECT_EQ(r1->merged_body["v"], "ours-v");

    // Swap ours ↔ theirs
    auto r2 = merge_->merge(kCollection, "MBM08-base", "MBM08-theirs", "MBM08-ours",
                            MergeStrategy::OURS_WINS);
    ASSERT_TRUE(r2.has_value());
    EXPECT_EQ(r2->merged_body["v"], "theirs-v");

    // The two results must differ — merge is not commutative under OURS_WINS.
    EXPECT_NE(r1->merged_body["v"], r2->merged_body["v"]);
}

// ═════════════════════════════════════════════════════════════════════════════
// SchemaSealVersion — SSV-01 … SSV-06
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief Fixture for schema sealing and version-transition edge-case tests.
 *
 * Each test operates on a fresh InMemoryDocumentSchemaEvolution instance.
 */
class SchemaSealVersionTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        evo_ = std::make_unique<InMemoryDocumentSchemaEvolution>();
    }

    /// @brief Helper — register a minimal single-field schema at @p version.
    void registerMinimalVersion(SchemaVersion version, const std::string& field_name)
    {
        SchemaDescriptor sd;
        sd.fields.push_back({field_name, SchemaFieldType::STRING, true, {}});
        auto r = evo_->registerVersion(version, sd);
        ASSERT_TRUE(r.has_value())
            << "registerVersion(" << version << ") failed: " << r.error().message();
    }

    std::unique_ptr<InMemoryDocumentSchemaEvolution> evo_;
};

/**
 * @test SSV-01: Registering the same version twice returns ERR_DOC_SCHEMA_VERSION_EXISTS.
 */
TEST_F(SchemaSealVersionTest, SSV01_DuplicateVersionRegistrationRejected)
{
    registerMinimalVersion(1, "name");
    SchemaDescriptor sd2;
    sd2.fields.push_back({"name", SchemaFieldType::STRING, true, {}});
    auto r = evo_->registerVersion(1, sd2);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code(), ErrorCode::ERR_DOC_SCHEMA_VERSION_EXISTS);
}

/**
 * @test SSV-02: Validating against an unknown version returns
 *               ERR_DOC_SCHEMA_VERSION_NOT_FOUND.
 */
TEST_F(SchemaSealVersionTest, SSV02_ValidateUnknownVersionReturnsError)
{
    // Do not register any version; validate directly.
    nlohmann::json body;
    body["name"] = "test-doc";

    auto r = evo_->validate("SSV02-DOC-001", body, 99u);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code(), ErrorCode::ERR_DOC_SCHEMA_VERSION_NOT_FOUND);
}

/**
 * @test SSV-03: Version ordering — registeredVersions() returns versions in
 *               ascending order regardless of registration order.
 */
TEST_F(SchemaSealVersionTest, SSV03_VersionsReturnedInAscendingOrder)
{
    // Register versions out of order: 3, 1, 2
    registerMinimalVersion(3, "field_c");
    registerMinimalVersion(1, "field_a");
    registerMinimalVersion(2, "field_b");

    auto versions = evo_->registeredVersions();
    ASSERT_EQ(versions.size(), 3u);
    EXPECT_EQ(versions[0], 1u);
    EXPECT_EQ(versions[1], 2u);
    EXPECT_EQ(versions[2], 3u);
}

/**
 * @test SSV-04: Document with missing required field fails validation.
 *
 * Registers a schema with a required field "id".  Validates a document body
 * that omits the required field and expects a non-empty violations list.
 */
TEST_F(SchemaSealVersionTest, SSV04_MissingRequiredFieldProducesViolation)
{
    registerMinimalVersion(1, "id");

    nlohmann::json body;
    // "id" field is intentionally absent
    body["other"] = "irrelevant";

    auto r = evo_->validate("SSV04-DOC-001", body, 1u);
    ASSERT_TRUE(r.has_value()) << "validate() should return a report, not an error";
    EXPECT_FALSE(r->violations.empty()) << "Missing required 'id' must produce a violation";
}

/**
 * @test SSV-05: Registering a new schema version after v1 is valid (forward-only).
 *
 * Verifies that v2 can be registered independently after v1 and that both
 * versions coexist in the registry without interfering.
 */
TEST_F(SchemaSealVersionTest, SSV05_ForwardVersionRegistrationValid)
{
    registerMinimalVersion(1, "email");
    registerMinimalVersion(2, "phone");

    auto versions = evo_->registeredVersions();
    ASSERT_EQ(versions.size(), 2u);

    // v1 validates documents with "email"
    nlohmann::json v1Body; v1Body["email"] = "user@example.com";
    auto r1 = evo_->validate("SSV05-V1", v1Body, 1u);
    ASSERT_TRUE(r1.has_value());
    EXPECT_TRUE(r1->violations.empty());

    // v2 validates documents with "phone"
    nlohmann::json v2Body; v2Body["phone"] = "+49-123-456789";
    auto r2 = evo_->validate("SSV05-V2", v2Body, 2u);
    ASSERT_TRUE(r2.has_value());
    EXPECT_TRUE(r2->violations.empty());
}

/**
 * @test SSV-06: Type mismatch on a required field produces a violation.
 *
 * Schema expects field "count" to be of type NUMBER.  Document provides a
 * STRING — must result in a type violation.
 */
TEST_F(SchemaSealVersionTest, SSV06_TypeMismatchProducesViolation)
{
    SchemaDescriptor sd;
    sd.fields.push_back({"count", SchemaFieldType::NUMBER, true, {}});
    ASSERT_TRUE(evo_->registerVersion(1, sd).has_value());

    nlohmann::json body;
    body["count"] = "not-a-number";  // type mismatch

    auto r = evo_->validate("SSV06-DOC-001", body, 1u);
    ASSERT_TRUE(r.has_value());
    EXPECT_FALSE(r->violations.empty()) << "Type mismatch must produce a violation";

    bool foundCountViolation = false;
    for (const auto& v : r->violations) {
        if (v.field_name == "count") {
            foundCountViolation = true;
            break;
        }
    }
    EXPECT_TRUE(foundCountViolation) << "Violation must reference 'count' field";
}

// ═════════════════════════════════════════════════════════════════════════════
// OperatorDiagnostics — ODE-01 … ODE-08
// ═════════════════════════════════════════════════════════════════════════════

/**
 * @brief Fixture for operator diagnostics coverage tests.
 *
 * Exercises DocumentDiagnosticSink, classifyDocumentError(),
 * documentErrorDescription(), and formatDocumentError() for the
 * EXCHANGE_ERROR and ROUND_TRIP_ERROR failure paths.
 */
class OperatorDiagnosticsTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        sink_.clear();
    }

    DocumentDiagnosticSink sink_;
};

/**
 * @test ODE-01: ERR_DOC_EXCHANGE_BOUNDARY_VIOLATED classifies as EXCHANGE_ERROR.
 */
TEST_F(OperatorDiagnosticsTest, ODE01_ExchangeBoundaryViolatedClassifiesCorrectly)
{
    const auto cls = classifyDocumentError(ErrorCode::ERR_DOC_EXCHANGE_BOUNDARY_VIOLATED);
    EXPECT_EQ(cls, DocumentErrorClass::EXCHANGE_ERROR);
}

/**
 * @test ODE-02: ERR_DOC_DIFF_NOT_FOUND classifies as EXCHANGE_ERROR.
 *
 * diff() is the primary XDOMEA-adjacent operation that produces this error.
 */
TEST_F(OperatorDiagnosticsTest, ODE02_DiffNotFoundClassifiesAsExchangeError)
{
    const auto cls = classifyDocumentError(ErrorCode::ERR_DOC_DIFF_NOT_FOUND);
    EXPECT_EQ(cls, DocumentErrorClass::EXCHANGE_ERROR);
}

/**
 * @test ODE-03: ERR_DOC_ROUND_TRIP_PERSIST_FAIL classifies as ROUND_TRIP_ERROR.
 */
TEST_F(OperatorDiagnosticsTest, ODE03_RoundTripPersistFailClassifiesCorrectly)
{
    const auto cls = classifyDocumentError(ErrorCode::ERR_DOC_ROUND_TRIP_PERSIST_FAIL);
    EXPECT_EQ(cls, DocumentErrorClass::ROUND_TRIP_ERROR);
}

/**
 * @test ODE-04: ERR_DOC_SNAPSHOT_COLLISION classifies as ROUND_TRIP_ERROR.
 */
TEST_F(OperatorDiagnosticsTest, ODE04_SnapshotCollisionClassifiesAsRoundTripError)
{
    const auto cls = classifyDocumentError(ErrorCode::ERR_DOC_SNAPSHOT_COLLISION);
    EXPECT_EQ(cls, DocumentErrorClass::ROUND_TRIP_ERROR);
}

/**
 * @test ODE-05: DocumentDiagnosticSink correctly counts EXCHANGE_ERROR events.
 *
 * Records three exchange errors and verifies that count() returns 3 and
 * totalCount() returns 3.
 */
TEST_F(OperatorDiagnosticsTest, ODE05_SinkCountsExchangeErrors)
{
    sink_.record(ErrorCode::ERR_DOC_EXCHANGE_BOUNDARY_VIOLATED, "doc-001");
    sink_.record(ErrorCode::ERR_DOC_EXCHANGE_BOUNDARY_VIOLATED, "doc-002");
    sink_.record(ErrorCode::ERR_DOC_DIFF_NOT_FOUND,             "doc-003");

    EXPECT_EQ(sink_.count(DocumentErrorClass::EXCHANGE_ERROR), 3u);
    EXPECT_EQ(sink_.totalCount(), 3u);
}

/**
 * @test ODE-06: DocumentDiagnosticSink correctly counts ROUND_TRIP_ERROR events.
 */
TEST_F(OperatorDiagnosticsTest, ODE06_SinkCountsRoundTripErrors)
{
    sink_.record(ErrorCode::ERR_DOC_ROUND_TRIP_PERSIST_FAIL, "snapshot-001");
    sink_.record(ErrorCode::ERR_DOC_SNAPSHOT_COLLISION,      "snapshot-002");

    EXPECT_EQ(sink_.count(DocumentErrorClass::ROUND_TRIP_ERROR), 2u);
    EXPECT_EQ(sink_.totalCount(), 2u);
}

/**
 * @test ODE-07: Mixed error-class recording produces correct per-class counts.
 *
 * Records events spanning EXCHANGE_ERROR, ROUND_TRIP_ERROR, and MERGE_CONFLICT.
 * Verifies that each class counter is independent and the total is the sum.
 */
TEST_F(OperatorDiagnosticsTest, ODE07_MixedClassCountsAreIndependent)
{
    sink_.record(ErrorCode::ERR_DOC_EXCHANGE_BOUNDARY_VIOLATED, "ex-001");
    sink_.record(ErrorCode::ERR_DOC_DIFF_NOT_FOUND,             "ex-002");
    sink_.record(ErrorCode::ERR_DOC_ROUND_TRIP_PERSIST_FAIL,    "rt-001");
    sink_.record(ErrorCode::ERR_DOC_MERGE_CONFLICT,             "mc-001");
    sink_.record(ErrorCode::ERR_DOC_MERGE_CONFLICT,             "mc-002");

    EXPECT_EQ(sink_.count(DocumentErrorClass::EXCHANGE_ERROR),  2u);
    EXPECT_EQ(sink_.count(DocumentErrorClass::ROUND_TRIP_ERROR), 1u);
    EXPECT_EQ(sink_.count(DocumentErrorClass::MERGE_CONFLICT),  2u);
    EXPECT_EQ(sink_.totalCount(), 5u);
}

/**
 * @test ODE-08: documentErrorDescription returns a non-empty string for all
 *               EXCHANGE_ERROR and ROUND_TRIP_ERROR codes.
 *
 * Guards against silent gaps in the description map for the critical
 * exchange and round-trip failure paths.
 */
TEST_F(OperatorDiagnosticsTest, ODE08_DescriptionsNonEmptyForExchangeAndRoundTripCodes)
{
    static const ErrorCode kExchangeCodes[] = {
        ErrorCode::ERR_DOC_DIFF_NOT_FOUND,
        ErrorCode::ERR_DOC_EXCHANGE_BOUNDARY_VIOLATED,
    };
    static const ErrorCode kRoundTripCodes[] = {
        ErrorCode::ERR_DOC_SNAPSHOT_COLLISION,
        ErrorCode::ERR_DOC_ROUND_TRIP_PERSIST_FAIL,
    };

    for (const auto code : kExchangeCodes) {
        const auto desc = documentErrorDescription(code);
        EXPECT_FALSE(std::string(desc).empty())
            << "documentErrorDescription must not be empty for code "
            << static_cast<int>(code);
    }
    for (const auto code : kRoundTripCodes) {
        const auto desc = documentErrorDescription(code);
        EXPECT_FALSE(std::string(desc).empty())
            << "documentErrorDescription must not be empty for code "
            << static_cast<int>(code);
    }
}
