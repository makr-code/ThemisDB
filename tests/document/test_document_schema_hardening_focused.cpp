/**
 * @file test_document_schema_hardening_focused.cpp
 * @brief Document module schema, merge, round-trip edge-case and stress hardening tests.
 *
 * @details Covers five test categories that expand regression breadth beyond
 * the baseline test_document_store.cpp suite:
 *
 *  - SchemaEdge   (SE-01 … SE-12): schema validation edge cases
 *  - SchemaSeal   (SS-01 … SS-04): sealing and version transition
 *  - MergeEdge    (ME-01 … ME-10): three-way merge edge cases
 *  - RoundTripEdge (RTE-01 … RTE-08): store-backed round-trip persistence
 *  - StressConflict (SC-01 … SC-04): deterministic high-conflict stress
 *
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Target: Q3–Q4 2026 hardening sprint
 * @version 1.0.0
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * ThemisDB — Document Module Hardening Tests
 *
 * File:    test_document_schema_hardening_focused.cpp
 * Module:  tests/document/
 * Purpose: Expand focused regression coverage for schema validation,
 *          diff/merge semantics, round-trip persistence, and stress paths.
 *
 * Version: 1.0.0
 * SPDX-License-Identifier: Apache-2.0
 */

#include <gtest/gtest.h>

#include "document/document_diff_merge.h"
#include "document/document_schema_evolution.h"
#include "document/document_store.h"
#include "document/round_trip_editor.h"

#include <cstdint>
#include <limits>
#include <string>

using namespace themis;
using namespace themis::document;

// ─────────────────────────────────────────────────────────────────────────────
// Local helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// @brief Schema with all fields optional (no required field).
SchemaDescriptor makeSchemaAllOptional() {
    SchemaDescriptor sd;
    sd.fields.push_back({"alpha", SchemaFieldType::STRING,  false, {}});
    sd.fields.push_back({"beta",  SchemaFieldType::NUMBER,  false, {}});
    sd.fields.push_back({"gamma", SchemaFieldType::BOOLEAN, false, {}});
    return sd;
}

/// @brief Schema with a single required STRING field "name".
SchemaDescriptor makeSchemaSingleRequired(const std::string& field_name = "name",
                                          SchemaFieldType type = SchemaFieldType::STRING) {
    SchemaDescriptor sd;
    sd.fields.push_back({field_name, type, true, {}});
    return sd;
}

/// @brief Schema with three required fields: "name", "email", "active".
SchemaDescriptor makeSchemaThreeRequired() {
    SchemaDescriptor sd;
    sd.fields.push_back({"name",   SchemaFieldType::STRING,  true, {}});
    sd.fields.push_back({"email",  SchemaFieldType::STRING,  true, {}});
    sd.fields.push_back({"active", SchemaFieldType::BOOLEAN, true, {}});
    return sd;
}

/// @brief Schema with a single ANY-typed field.
SchemaDescriptor makeSchemaAnyField(const std::string& field_name = "value") {
    SchemaDescriptor sd;
    sd.fields.push_back({field_name, SchemaFieldType::ANY, false, {}});
    return sd;
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// SchemaEdge tests  (SE-01 … SE-12)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Test fixture for schema validation edge cases.
 *
 * Each test constructs a fresh InMemoryDocumentSchemaEvolution to ensure
 * full test isolation.
 */
class SchemaEdgeTest : public ::testing::Test {
protected:
    InMemoryDocumentSchemaEvolution evo_;
};

// SE-01: empty document body against schema with all-optional fields → valid
TEST_F(SchemaEdgeTest, SE01_EmptyBodyAllOptionalSchema_IsValid) {
    ASSERT_TRUE(evo_.registerVersion(1, makeSchemaAllOptional()).has_value());
    const nlohmann::json empty_body = nlohmann::json::object();
    auto r = evo_.validate("doc-se01", empty_body, 1);
    ASSERT_TRUE(r.has_value());
    EXPECT_TRUE(r.value().isValid())
        << "An empty document body is valid against an all-optional schema.";
}

// SE-02: null JSON value for a STRING field → TYPE_MISMATCH
TEST_F(SchemaEdgeTest, SE02_NullValueForStringField_TypeMismatch) {
    ASSERT_TRUE(evo_.registerVersion(1, makeSchemaSingleRequired()).has_value());
    nlohmann::json body{{"name", nullptr}};  // null is not a string
    auto r = evo_.validate("doc-se02", body, 1);
    ASSERT_TRUE(r.has_value());
    EXPECT_FALSE(r.value().isValid());
    ASSERT_FALSE(r.value().violations.empty());
    EXPECT_EQ(r.value().violations[0].kind, FieldViolationKind::TYPE_MISMATCH)
        << "A null JSON value for a STRING field must trigger TYPE_MISMATCH.";
    EXPECT_EQ(r.value().violations[0].field_name, "name");
}

// SE-03: array value for BOOLEAN field → TYPE_MISMATCH
TEST_F(SchemaEdgeTest, SE03_ArrayForBooleanField_TypeMismatch) {
    ASSERT_TRUE(evo_.registerVersion(1, makeSchemaSingleRequired("flag", SchemaFieldType::BOOLEAN)).has_value());
    nlohmann::json body{{"flag", nlohmann::json::array({1, 2, 3})}};
    auto r = evo_.validate("doc-se03", body, 1);
    ASSERT_TRUE(r.has_value());
    EXPECT_FALSE(r.value().isValid());
    bool found = false;
    for (const auto& v : r.value().violations) {
        if (v.field_name == "flag" && v.kind == FieldViolationKind::TYPE_MISMATCH) {
            found = true;
        }
    }
    EXPECT_TRUE(found) << "Array value for BOOLEAN field must produce TYPE_MISMATCH.";
}

// SE-04: object value for NUMBER field → TYPE_MISMATCH
TEST_F(SchemaEdgeTest, SE04_ObjectForNumberField_TypeMismatch) {
    ASSERT_TRUE(evo_.registerVersion(1, makeSchemaSingleRequired("count", SchemaFieldType::NUMBER)).has_value());
    nlohmann::json body{{"count", nlohmann::json::object({{"inner", 1}})}};
    auto r = evo_.validate("doc-se04", body, 1);
    ASSERT_TRUE(r.has_value());
    EXPECT_FALSE(r.value().isValid());
    bool found = false;
    for (const auto& v : r.value().violations) {
        if (v.field_name == "count" && v.kind == FieldViolationKind::TYPE_MISMATCH) {
            found = true;
        }
    }
    EXPECT_TRUE(found) << "Object value for NUMBER field must produce TYPE_MISMATCH.";
}

// SE-05: multiple required fields all missing → report contains all violations
TEST_F(SchemaEdgeTest, SE05_AllRequiredFieldsMissing_ReportsAllViolations) {
    ASSERT_TRUE(evo_.registerVersion(1, makeSchemaThreeRequired()).has_value());
    const nlohmann::json empty_body = nlohmann::json::object();
    auto r = evo_.validate("doc-se05", empty_body, 1);
    ASSERT_TRUE(r.has_value());
    EXPECT_FALSE(r.value().isValid());
    EXPECT_EQ(r.value().violations.size(), 3u)
        << "All three required fields must appear as violations when absent.";
    for (const auto& v : r.value().violations) {
        EXPECT_EQ(v.kind, FieldViolationKind::MISSING_REQUIRED_FIELD);
    }
}

// SE-06: validate() on sealed registry still validates correctly
TEST_F(SchemaEdgeTest, SE06_ValidateOnSealedRegistry_StillWorks) {
    ASSERT_TRUE(evo_.registerVersion(1, makeSchemaAllOptional()).has_value());
    evo_.seal();
    ASSERT_TRUE(evo_.isSealed());

    nlohmann::json body{{"alpha", "hello"}};
    auto r = evo_.validate("doc-se06", body, 1);
    ASSERT_TRUE(r.has_value())
        << "seal() must not affect existing validate() behavior.";
    EXPECT_TRUE(r.value().isValid());
}

// SE-07: registerVersion() with version=0 succeeds (0 is a valid version)
TEST_F(SchemaEdgeTest, SE07_RegisterVersionZero_Succeeds) {
    auto r = evo_.registerVersion(0, makeSchemaAllOptional());
    ASSERT_TRUE(r.has_value())
        << "Version 0 is a valid SchemaVersion; registerVersion() must accept it.";
    auto vs = evo_.registeredVersions();
    ASSERT_EQ(vs.size(), 1u);
    EXPECT_EQ(vs[0], 0u);
}

// SE-08: registerVersion() with MAX_UINT32 version succeeds
TEST_F(SchemaEdgeTest, SE08_RegisterVersionMaxUint32_Succeeds) {
    constexpr SchemaVersion kMaxVer = std::numeric_limits<SchemaVersion>::max();
    auto r = evo_.registerVersion(kMaxVer, makeSchemaAllOptional());
    ASSERT_TRUE(r.has_value())
        << "std::numeric_limits<SchemaVersion>::max() must be a valid version number.";
    auto vs = evo_.registeredVersions();
    ASSERT_EQ(vs.size(), 1u);
    EXPECT_EQ(vs[0], kMaxVer);
}

// SE-09: two consecutive registerVersion() calls return [1,2] in ascending order
TEST_F(SchemaEdgeTest, SE09_TwoConsecutiveVersions_ListedAscending) {
    ASSERT_TRUE(evo_.registerVersion(1, makeSchemaAllOptional()).has_value());
    ASSERT_TRUE(evo_.registerVersion(2, makeSchemaAllOptional()).has_value());
    auto vs = evo_.registeredVersions();
    ASSERT_EQ(vs.size(), 2u);
    EXPECT_EQ(vs[0], 1u);
    EXPECT_EQ(vs[1], 2u);
}

// SE-10: registerVersion() on an independent unsealed copy does not affect a separately sealed instance
TEST_F(SchemaEdgeTest, SE10_IndependentInstancesAreIsolated) {
    InMemoryDocumentSchemaEvolution sealed_evo;
    InMemoryDocumentSchemaEvolution other_evo;

    ASSERT_TRUE(sealed_evo.registerVersion(1, makeSchemaAllOptional()).has_value());
    sealed_evo.seal();
    ASSERT_TRUE(sealed_evo.isSealed());

    // Registering version 2 on other_evo must not affect sealed_evo.
    ASSERT_TRUE(other_evo.registerVersion(1, makeSchemaAllOptional()).has_value());
    ASSERT_TRUE(other_evo.registerVersion(2, makeSchemaAllOptional()).has_value());

    EXPECT_TRUE(sealed_evo.isSealed());
    EXPECT_FALSE(other_evo.isSealed());
    EXPECT_EQ(sealed_evo.registeredVersions().size(), 1u)
        << "sealed_evo must still have exactly one version after operations on other_evo.";
    EXPECT_EQ(other_evo.registeredVersions().size(), 2u);
}

// SE-11: document with extra fields not in schema → valid (extra fields allowed)
TEST_F(SchemaEdgeTest, SE11_ExtraFieldsInDocument_Valid) {
    ASSERT_TRUE(evo_.registerVersion(1, makeSchemaSingleRequired()).has_value());
    nlohmann::json body{{"name", "Alice"}, {"extra_field", "ignored"}, {"another", 99}};
    auto r = evo_.validate("doc-se11", body, 1);
    ASSERT_TRUE(r.has_value());
    EXPECT_TRUE(r.value().isValid())
        << "Extra fields not declared in the schema must not produce violations.";
}

// SE-12: SchemaFieldType::ANY never produces TYPE_MISMATCH for any value type
TEST_F(SchemaEdgeTest, SE12_AnyFieldType_NeverTypeMismatch) {
    ASSERT_TRUE(evo_.registerVersion(1, makeSchemaAnyField("value")).has_value());

    // Test all JSON value types against ANY.
    const std::vector<nlohmann::json> candidates = {
        nlohmann::json("a string"),
        nlohmann::json(42),
        nlohmann::json(3.14),
        nlohmann::json(true),
        nlohmann::json::array({1, 2}),
        nlohmann::json::object({{"k", "v"}}),
        nlohmann::json(nullptr),
    };

    for (const auto& val : candidates) {
        nlohmann::json body{{"value", val}};
        auto r = evo_.validate("doc-se12", body, 1);
        ASSERT_TRUE(r.has_value());
        for (const auto& v : r.value().violations) {
            EXPECT_NE(v.kind, FieldViolationKind::TYPE_MISMATCH)
                << "SchemaFieldType::ANY must never produce TYPE_MISMATCH; got one for value: "
                << val.dump();
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// SchemaSeal tests  (SS-01 … SS-04)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Test fixture for schema sealing and version transition scenarios.
 */
class SchemaSealTest : public ::testing::Test {
protected:
    InMemoryDocumentSchemaEvolution evo_;
};

// SS-01: seal() is idempotent — second call does not change behavior
TEST_F(SchemaSealTest, SS01_SealIsIdempotent) {
    ASSERT_TRUE(evo_.registerVersion(1, makeSchemaAllOptional()).has_value());
    evo_.seal();
    evo_.seal();  // second call — must be harmless
    EXPECT_TRUE(evo_.isSealed());
    auto r = evo_.registerVersion(2, makeSchemaAllOptional());
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code(), errors::ErrorCode::ERR_DOC_SCHEMA_SEALED)
        << "After double seal(), registerVersion() must still return ERR_DOC_SCHEMA_SEALED.";
}

// SS-02: After seal(), registeredVersions() still returns all pre-seal versions
TEST_F(SchemaSealTest, SS02_SealPreservesRegisteredVersions) {
    ASSERT_TRUE(evo_.registerVersion(1, makeSchemaAllOptional()).has_value());
    ASSERT_TRUE(evo_.registerVersion(3, makeSchemaAllOptional()).has_value());
    ASSERT_TRUE(evo_.registerVersion(5, makeSchemaAllOptional()).has_value());
    evo_.seal();

    auto vs = evo_.registeredVersions();
    ASSERT_EQ(vs.size(), 3u);
    EXPECT_EQ(vs[0], 1u);
    EXPECT_EQ(vs[1], 3u);
    EXPECT_EQ(vs[2], 5u);
}

// SS-03: registerVersion() after seal() returns ERR_DOC_SCHEMA_SEALED (not VERSION_EXISTS)
TEST_F(SchemaSealTest, SS03_RegisterAfterSeal_ReturnsSealedError) {
    ASSERT_TRUE(evo_.registerVersion(1, makeSchemaAllOptional()).has_value());
    evo_.seal();
    auto r = evo_.registerVersion(2, makeSchemaAllOptional());
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code(), errors::ErrorCode::ERR_DOC_SCHEMA_SEALED)
        << "A sealed registry must return ERR_DOC_SCHEMA_SEALED, not VERSION_EXISTS.";
    EXPECT_NE(r.error().code(), errors::ErrorCode::ERR_DOC_SCHEMA_VERSION_EXISTS);
}

// SS-04: v1 requires only "name"; v2 also requires "email" — doc valid for v1 fails v2
TEST_F(SchemaSealTest, SS04_VersionTransitionValidation_V1PassesV2Fails) {
    SchemaDescriptor sd_v1;
    sd_v1.fields.push_back({"name", SchemaFieldType::STRING, true, {}});

    SchemaDescriptor sd_v2;
    sd_v2.fields.push_back({"name",  SchemaFieldType::STRING, true, {}});
    sd_v2.fields.push_back({"email", SchemaFieldType::STRING, true, {}});

    ASSERT_TRUE(evo_.registerVersion(1, sd_v1).has_value());
    ASSERT_TRUE(evo_.registerVersion(2, sd_v2).has_value());

    nlohmann::json doc{{"name", "Alice"}};  // no "email" field

    auto r1 = evo_.validate("doc-ss04", doc, 1);
    ASSERT_TRUE(r1.has_value());
    EXPECT_TRUE(r1.value().isValid())
        << "Document with 'name' only must be valid for schema v1.";

    auto r2 = evo_.validate("doc-ss04", doc, 2);
    ASSERT_TRUE(r2.has_value());
    EXPECT_FALSE(r2.value().isValid())
        << "Document missing 'email' must be invalid for schema v2.";
    bool has_email_violation = false;
    for (const auto& v : r2.value().violations) {
        if (v.field_name == "email" &&
            v.kind == FieldViolationKind::MISSING_REQUIRED_FIELD) {
            has_email_violation = true;
        }
    }
    EXPECT_TRUE(has_email_violation);
}

// ─────────────────────────────────────────────────────────────────────────────
// MergeEdge tests  (ME-01 … ME-10)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Test fixture for three-way merge edge cases.
 *
 * Owns both a backing store and the diff/merge engine.  The helper put()
 * inserts documents without boilerplate.
 */
class MergeEdgeTest : public ::testing::Test {
protected:
    InMemoryDocumentStore    store_;
    InMemoryDocumentDiffMerge dm_{store_};
    const CollectionId       kCol{"me-col"};

    void put(const std::string& id, const nlohmann::json& body) {
        (void)store_.put({id, kCol, body});
    }
};

// ME-01: both ours and theirs delete the same field → clean merge, field absent
TEST_F(MergeEdgeTest, ME01_BothBranchesDeleteField_CleanMergeFieldAbsent) {
    put("base",   nlohmann::json{{"x", 1}, {"y", 2}});
    put("ours",   nlohmann::json{{"y", 2}});    // deleted "x"
    put("theirs", nlohmann::json{{"y", 2}});    // deleted "x"

    auto r = dm_.merge(kCol, "base", "ours", "theirs", MergeStrategy::OURS_WINS);
    ASSERT_TRUE(r.has_value());
    EXPECT_TRUE(r.value().conflicts.empty())
        << "Concurrent deletion of the same field must not be a conflict.";
    EXPECT_FALSE(r.value().merged_body.contains("x"))
        << "Field deleted by both branches must be absent from the merged result.";
    EXPECT_EQ(r.value().merged_body["y"], 2);
}

// ME-02: ours deletes a field that theirs modifies → conflict reported
TEST_F(MergeEdgeTest, ME02_OursDeletesTheirsModifies_ConflictReported) {
    put("base",   nlohmann::json{{"x", 1}});
    put("ours",   nlohmann::json::object());    // deleted "x"
    put("theirs", nlohmann::json{{"x", 99}});  // modified "x"

    auto r = dm_.merge(kCol, "base", "ours", "theirs", MergeStrategy::OURS_WINS);
    ASSERT_TRUE(r.has_value());
    EXPECT_FALSE(r.value().conflicts.empty())
        << "Deletion in ours vs modification in theirs must produce a conflict.";
    bool found = false;
    for (const auto& c : r.value().conflicts) {
        if (c.field_name == "x") found = true;
    }
    EXPECT_TRUE(found);
}

// ME-03: theirs adds a new field (not in base, not in ours) → clean merge, field present
TEST_F(MergeEdgeTest, ME03_TheirsAddsNewField_CleanMergeFieldPresent) {
    put("base",   nlohmann::json{{"a", 1}});
    put("ours",   nlohmann::json{{"a", 1}});
    put("theirs", nlohmann::json{{"a", 1}, {"new_field", "T"}});

    auto r = dm_.merge(kCol, "base", "ours", "theirs");
    ASSERT_TRUE(r.has_value());
    EXPECT_TRUE(r.value().conflicts.empty())
        << "Adding a new field in only one branch must not produce a conflict.";
    EXPECT_TRUE(r.value().merged_body.contains("new_field"));
    EXPECT_EQ(r.value().merged_body["new_field"], "T");
}

// ME-04: both ours and theirs add the same field with the same value → clean merge
TEST_F(MergeEdgeTest, ME04_BothAddSameFieldSameValue_CleanMerge) {
    put("base",   nlohmann::json{{"a", 1}});
    put("ours",   nlohmann::json{{"a", 1}, {"new", "X"}});
    put("theirs", nlohmann::json{{"a", 1}, {"new", "X"}});

    auto r = dm_.merge(kCol, "base", "ours", "theirs");
    ASSERT_TRUE(r.has_value());
    EXPECT_TRUE(r.value().conflicts.empty())
        << "Both branches adding the same field with the same value must not conflict.";
    EXPECT_EQ(r.value().merged_body["new"], "X");
}

// ME-05: both ours and theirs add the same field with DIFFERENT values → conflict
TEST_F(MergeEdgeTest, ME05_BothAddSameFieldDifferentValues_Conflict) {
    put("base",   nlohmann::json{{"a", 1}});
    put("ours",   nlohmann::json{{"a", 1}, {"new", "X"}});
    put("theirs", nlohmann::json{{"a", 1}, {"new", "Y"}});

    auto r = dm_.merge(kCol, "base", "ours", "theirs", MergeStrategy::OURS_WINS);
    ASSERT_TRUE(r.has_value());
    EXPECT_FALSE(r.value().conflicts.empty())
        << "Both branches adding the same field with different values must conflict.";
    bool found = false;
    for (const auto& c : r.value().conflicts) {
        if (c.field_name == "new") found = true;
    }
    EXPECT_TRUE(found);
}

// ME-06: THEIRS_WINS strategy resolves conflict with theirs value
TEST_F(MergeEdgeTest, ME06_TheirsWinsResolvesConflictWithTheirsValue) {
    put("base",   nlohmann::json{{"v", 0}});
    put("ours",   nlohmann::json{{"v", 1}});
    put("theirs", nlohmann::json{{"v", 2}});

    auto r = dm_.merge(kCol, "base", "ours", "theirs", MergeStrategy::THEIRS_WINS);
    ASSERT_TRUE(r.has_value());
    ASSERT_FALSE(r.value().conflicts.empty());
    EXPECT_EQ(r.value().merged_body["v"], 2)
        << "THEIRS_WINS must apply theirs value (2) for conflicting field.";
    EXPECT_EQ(r.value().strategy_applied, MergeStrategy::THEIRS_WINS);
}

// ME-07: FAIL strategy accumulates ALL conflicts before returning error
TEST_F(MergeEdgeTest, ME07_FailStrategyAccumulatesAllConflicts) {
    put("base",   nlohmann::json{{"a", 0}, {"b", 0}});
    put("ours",   nlohmann::json{{"a", 1}, {"b", 1}});
    put("theirs", nlohmann::json{{"a", 9}, {"b", 9}});

    auto r = dm_.merge(kCol, "base", "ours", "theirs", MergeStrategy::FAIL);
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code(), errors::ErrorCode::ERR_DOC_MERGE_CONFLICT);
    // Both conflicts must have been accumulated before returning: the context
    // carries the conflict count.
    EXPECT_EQ(r.error().context(), "2 conflict(s)")
        << "FAIL strategy must accumulate all conflicts and report the count.";
}

// ME-08: merge() with empty base_id → ERR_DOC_INVALID_ARGUMENT
TEST_F(MergeEdgeTest, ME08_EmptyBaseId_InvalidArgument) {
    put("ours",   nlohmann::json{{"k", 1}});
    put("theirs", nlohmann::json{{"k", 2}});

    auto r = dm_.merge(kCol, "", "ours", "theirs");
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code(), errors::ErrorCode::ERR_DOC_INVALID_ARGUMENT);
}

// ME-09: base=ours=theirs (all identical) → clean merge, no conflicts, merged==base
TEST_F(MergeEdgeTest, ME09_AllThreeIdentical_CleanMergeEqualsBase) {
    const nlohmann::json body{{"x", 1}, {"y", 2}};
    put("same-base",   body);
    put("same-ours",   body);
    put("same-theirs", body);

    auto r = dm_.merge(kCol, "same-base", "same-ours", "same-theirs");
    ASSERT_TRUE(r.has_value());
    EXPECT_TRUE(r.value().conflicts.empty());
    EXPECT_EQ(r.value().merged_body, body)
        << "When all three versions are identical the merged result must equal the base.";
}

// ME-10: non-object payloads on all three → returns ours body unchanged
TEST_F(MergeEdgeTest, ME10_NonObjectPayloads_ReturnsOursBody) {
    // Store non-object (scalar) payloads.
    put("scalar-base",   nlohmann::json("base-string"));
    put("scalar-ours",   nlohmann::json("ours-string"));
    put("scalar-theirs", nlohmann::json("theirs-string"));

    auto r = dm_.merge(kCol, "scalar-base", "scalar-ours", "scalar-theirs",
                       MergeStrategy::OURS_WINS);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r.value().merged_body, nlohmann::json("ours-string"))
        << "Non-object payloads must result in merged_body == ours body.";
    EXPECT_TRUE(r.value().conflicts.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// RoundTripEdge tests  (RTE-01 … RTE-08)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Test fixture for round-trip persistence edge cases.
 *
 * Uses a StoreBackedRoundTripEditor backed by an in-memory store.
 */
class RoundTripEdgeTest : public ::testing::Test {
protected:
    InMemoryDocumentStore      store_;
    StoreBackedRoundTripEditor editor_{store_};
};

// RTE-01: beginRelay() with same relay_id twice returns ERR_DOC_ALREADY_EXISTS
TEST_F(RoundTripEdgeTest, RTE01_BeginRelayTwiceSameId_AlreadyExists) {
    ASSERT_TRUE(editor_.beginRelay("relay-a", "seed-doc").has_value());
    auto r = editor_.beginRelay("relay-a", "seed-doc-2");
    ASSERT_FALSE(r.has_value());
    EXPECT_EQ(r.error().code(), errors::ErrorCode::ERR_DOC_ALREADY_EXISTS)
        << "A second beginRelay() with the same relay_id must fail with ALREADY_EXISTS.";
}

// RTE-02: loadInteraction() for a non-existent relay_id returns nullopt (not an error)
TEST_F(RoundTripEdgeTest, RTE02_LoadNonExistentRelay_ReturnsNullopt) {
    auto r = editor_.loadInteraction("no-such-relay", 0);
    ASSERT_TRUE(r.has_value())
        << "loadInteraction() for an unknown relay_id must succeed (return nullopt).";
    EXPECT_FALSE(r.value().has_value());
}

// RTE-03: saveInteraction() at index 1 before beginRelay() succeeds (no ordering precondition)
TEST_F(RoundTripEdgeTest, RTE03_SaveBeforeBeginRelay_Succeeds) {
    auto r = editor_.saveInteraction("relay-rte03", 1, "instr", "doc-content");
    EXPECT_TRUE(r.has_value())
        << "saveInteraction() has no precondition requiring a prior beginRelay() call.";
}

// RTE-04: countSnapshots() returns 0 for unknown relay_id
TEST_F(RoundTripEdgeTest, RTE04_CountSnapshotsUnknownRelay_ReturnsZero) {
    auto r = editor_.countSnapshots("unknown-relay-rte04");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, 0u);
}

// RTE-05: countSnapshots() returns N+1 after beginRelay() + N saveInteraction() calls
TEST_F(RoundTripEdgeTest, RTE05_CountSnapshotsAfterNInteractions_CorrectCount) {
    constexpr int N = 3;
    ASSERT_TRUE(editor_.beginRelay("relay-rte05", "seed").has_value());
    for (int i = 1; i <= N; ++i) {
        ASSERT_TRUE(editor_.saveInteraction("relay-rte05",
                                            static_cast<std::size_t>(i),
                                            "instr-" + std::to_string(i),
                                            "doc-" + std::to_string(i)).has_value());
    }
    auto r = editor_.countSnapshots("relay-rte05");
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(*r, static_cast<std::size_t>(N + 1))
        << "countSnapshots() must include the seed (index 0) and all N interactions.";
}

// RTE-06: saveInteraction() with the same index twice returns an error
TEST_F(RoundTripEdgeTest, RTE06_SaveSameIndexTwice_ReturnsError) {
    ASSERT_TRUE(editor_.beginRelay("relay-rte06", "seed").has_value());
    ASSERT_TRUE(editor_.saveInteraction("relay-rte06", 1, "instr", "doc").has_value());
    auto r = editor_.saveInteraction("relay-rte06", 1, "instr", "doc-duplicate");
    ASSERT_FALSE(r.has_value())
        << "Saving the same interaction_index twice must return an error.";
    EXPECT_EQ(r.error().code(), errors::ErrorCode::ERR_DOC_ALREADY_EXISTS);
}

// RTE-07: same interaction_index in different relay_ids do not collide
TEST_F(RoundTripEdgeTest, RTE07_SameIndexDifferentRelays_NoCollision) {
    ASSERT_TRUE(editor_.beginRelay("relay-X", "seed-X").has_value());
    ASSERT_TRUE(editor_.beginRelay("relay-Y", "seed-Y").has_value());

    ASSERT_TRUE(editor_.saveInteraction("relay-X", 1, "instr-X", "doc-X").has_value());
    ASSERT_TRUE(editor_.saveInteraction("relay-Y", 1, "instr-Y", "doc-Y").has_value());

    auto rx = editor_.loadInteraction("relay-X", 1);
    auto ry = editor_.loadInteraction("relay-Y", 1);

    ASSERT_TRUE(rx.has_value() && rx.value().has_value());
    ASSERT_TRUE(ry.has_value() && ry.value().has_value());
    EXPECT_EQ(rx.value().value().document, "doc-X");
    EXPECT_EQ(ry.value().value().document, "doc-Y");
}

// RTE-08: loadInteraction() returns correct instruction and document fields
TEST_F(RoundTripEdgeTest, RTE08_LoadInteraction_CorrectFields) {
    ASSERT_TRUE(editor_.beginRelay("relay-rte08", "seed-content").has_value());
    ASSERT_TRUE(editor_.saveInteraction("relay-rte08", 1,
                                        "my instruction", "my document").has_value());

    auto r = editor_.loadInteraction("relay-rte08", 1);
    ASSERT_TRUE(r.has_value());
    ASSERT_TRUE(r.value().has_value());
    const auto& snap = r.value().value();
    EXPECT_EQ(snap.relay_id, "relay-rte08");
    EXPECT_EQ(snap.interaction_index, 1u);
    EXPECT_EQ(snap.instruction, "my instruction");
    EXPECT_EQ(snap.document, "my document");
}

// ─────────────────────────────────────────────────────────────────────────────
// StressConflict tests  (SC-01 … SC-04)
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Test fixture for deterministic high-conflict stress scenarios.
 *
 * Builds 50-field documents where every field is a conflict between branches.
 */
class StressConflictTest : public ::testing::Test {
protected:
    InMemoryDocumentStore    store_;
    InMemoryDocumentDiffMerge dm_{store_};
    const CollectionId       kCol{"sc-col"};

    /// @brief Build a 50-field JSON object: {"f0": base_val, "f1": base_val+1, …}
    static nlohmann::json makeDoc50(int base_val) {
        nlohmann::json obj = nlohmann::json::object();
        for (int i = 0; i < 50; ++i) {
            obj["f" + std::to_string(i)] = base_val + i;
        }
        return obj;
    }

    void put(const std::string& id, const nlohmann::json& body) {
        (void)store_.put({id, kCol, body});
    }
};

// SC-01: 50 conflicting fields, OURS_WINS → merged contains all ours values
TEST_F(StressConflictTest, SC01_FiftyConflicts_OursWins_AllOursValues) {
    put("sc-base",   makeDoc50(0));    // {"f0":0,  "f1":1,  …, "f49":49}
    put("sc-ours",   makeDoc50(100));  // {"f0":100,"f1":101,…, "f49":149}
    put("sc-theirs", makeDoc50(200));  // {"f0":200,"f1":201,…, "f49":249}

    auto r = dm_.merge(kCol, "sc-base", "sc-ours", "sc-theirs",
                       MergeStrategy::OURS_WINS);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r.value().conflicts.size(), 50u);

    for (int i = 0; i < 50; ++i) {
        const std::string key = "f" + std::to_string(i);
        ASSERT_TRUE(r.value().merged_body.contains(key));
        EXPECT_EQ(r.value().merged_body[key], 100 + i)
            << "OURS_WINS: field " << key << " must have ours value.";
    }
}

// SC-02: 50 conflicting fields, THEIRS_WINS → merged contains all theirs values
TEST_F(StressConflictTest, SC02_FiftyConflicts_TheirsWins_AllTheirsValues) {
    put("sc2-base",   makeDoc50(0));
    put("sc2-ours",   makeDoc50(100));
    put("sc2-theirs", makeDoc50(200));

    auto r = dm_.merge(kCol, "sc2-base", "sc2-ours", "sc2-theirs",
                       MergeStrategy::THEIRS_WINS);
    ASSERT_TRUE(r.has_value());
    EXPECT_EQ(r.value().conflicts.size(), 50u);

    for (int i = 0; i < 50; ++i) {
        const std::string key = "f" + std::to_string(i);
        ASSERT_TRUE(r.value().merged_body.contains(key));
        EXPECT_EQ(r.value().merged_body[key], 200 + i)
            << "THEIRS_WINS: field " << key << " must have theirs value.";
    }
}

// SC-03: 50 conflicting fields, FAIL strategy reports exactly 50 conflicts
TEST_F(StressConflictTest, SC03_FiftyConflicts_FailStrategy_ExactlyFiftyConflicts) {
    put("sc3-base",   makeDoc50(0));
    put("sc3-ours",   makeDoc50(100));
    put("sc3-theirs", makeDoc50(200));

    auto r = dm_.merge(kCol, "sc3-base", "sc3-ours", "sc3-theirs",
                       MergeStrategy::FAIL);
    ASSERT_FALSE(r.has_value())
        << "FAIL strategy with 50 conflicts must return an error.";
    EXPECT_EQ(r.error().code(), errors::ErrorCode::ERR_DOC_MERGE_CONFLICT);
    EXPECT_EQ(r.error().context(), "50 conflict(s)")
        << "All 50 conflicts must be accumulated before the error is returned.";
}

// SC-04: sequential merge chain: merge(base, v1, v2) → merged1; merge(merged1, v3, v4) → merged2
TEST_F(StressConflictTest, SC04_SequentialMergeChain_ResultDeterministic) {
    // Round 1: each branch adds a distinct field to base.
    put("chain-base", nlohmann::json{{"root", 0}});
    put("chain-v1",   nlohmann::json{{"root", 0}, {"b_field", 10}});
    put("chain-v2",   nlohmann::json{{"root", 0}, {"c_field", 20}});

    auto r1 = dm_.merge(kCol, "chain-base", "chain-v1", "chain-v2");
    ASSERT_TRUE(r1.has_value());
    EXPECT_TRUE(r1.value().conflicts.empty());
    EXPECT_EQ(r1.value().merged_body["root"],    0);
    EXPECT_EQ(r1.value().merged_body["b_field"], 10);
    EXPECT_EQ(r1.value().merged_body["c_field"], 20);

    // Store merged1 for the second round.
    const nlohmann::json merged1 = r1.value().merged_body;
    put("chain-merged1", merged1);

    // Round 2: each branch adds another distinct field to merged1.
    nlohmann::json v3 = merged1; v3["d_field"] = 30;
    nlohmann::json v4 = merged1; v4["e_field"] = 40;
    put("chain-v3", v3);
    put("chain-v4", v4);

    auto r2 = dm_.merge(kCol, "chain-merged1", "chain-v3", "chain-v4");
    ASSERT_TRUE(r2.has_value());
    EXPECT_TRUE(r2.value().conflicts.empty());
    EXPECT_EQ(r2.value().merged_body["root"],    0);
    EXPECT_EQ(r2.value().merged_body["b_field"], 10);
    EXPECT_EQ(r2.value().merged_body["c_field"], 20);
    EXPECT_EQ(r2.value().merged_body["d_field"], 30);
    EXPECT_EQ(r2.value().merged_body["e_field"], 40);
}
