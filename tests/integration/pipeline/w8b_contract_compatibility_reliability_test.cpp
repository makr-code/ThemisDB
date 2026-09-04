/**
 * @file w8b_contract_compatibility_reliability_test.cpp
 * @brief Wave 8B — Contract & Compatibility Reliability Layer (CCR-01..CCR-08).
 *
 * Hardens API/schema/contract tests against unintended breaking changes.
 * Covers forward/backward schema compatibility, required-field enforcement,
 * type contracts, API idempotency, pagination and error-response contracts.
 * All tests are deterministic via kCanonicalSeed = 42.
 */

#include "../test_data_generator.h"
#include "../test_fixture.h"

#include <algorithm>
#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis { namespace test { 

namespace {

// Canonical seed is provided by test_data_generator.h (themis::test::kCanonicalSeed)

// ---------------------------------------------------------------------------
// SchemaField — simple schema descriptor used by CCR tests
// ---------------------------------------------------------------------------

/// @brief Data types recognised by the schema validator.
enum class FieldType { kString, kInteger, kBoolean };

/// @brief Schema field definition.
struct SchemaFieldDef {
    std::string name;
    FieldType   type{FieldType::kString};
    bool        required{false};
};

/// @brief Runtime field value (variant-like, discriminated by active member).
struct FieldValue {
    FieldType   type{FieldType::kString};
    std::string string_val;
    int64_t     int_val{0};
    bool        bool_val{false};
};

using SchemaRecord = std::unordered_map<std::string, FieldValue>;
using Schema       = std::vector<SchemaFieldDef>;

/// @brief Validation result.
struct ValidationResult {
    bool                     ok{false};
    std::vector<std::string> errors;
};

// ---------------------------------------------------------------------------
// SchemaValidator — CCR-02/CCR-05: required fields + type enforcement
// ---------------------------------------------------------------------------

/// @brief Validates a record against a schema definition.
///
/// Enforces required-field presence and field type correctness.
/// Unknown fields in the record are tolerated (forward-compatibility).
class SchemaValidator {
public:
    explicit SchemaValidator(Schema schema) : schema_(std::move(schema)) {}

    /// @brief Validate a record.
    /// @param record  Input record to validate.
    /// @return ValidationResult; ok=false if any required field is absent or
    ///         any present field has the wrong type.
    [[nodiscard]] ValidationResult Validate(const SchemaRecord& record) const {
        ValidationResult result;
        result.ok = true;
        for (const auto& field_def : schema_) {
            const auto it = record.find(field_def.name);
            if (it == record.end()) {
                if (field_def.required) {
                    result.ok = false;
                    result.errors.push_back("missing_required:" + field_def.name);
                }
                continue;
            }
            // Type check
            if (it->second.type != field_def.type) {
                result.ok = false;
                result.errors.push_back("type_mismatch:" + field_def.name);
            }
        }
        return result;
    }

private:
    Schema schema_;
};

// ---------------------------------------------------------------------------
// VersionedSchemaStore — CCR-03/CCR-04: forward/backward compatibility
// ---------------------------------------------------------------------------

/// @brief Record version tag.
enum class RecordVersion : uint32_t { kV1 = 1, kV2 = 2 };

/// @brief V1 record — original schema.
struct RecordV1 {
    std::string id;
    std::string name;
    int64_t     value{0};
};

/// @brief V2 record — adds optional `tags` field (forward-compatible extension).
struct RecordV2 {
    std::string              id;
    std::string              name;
    int64_t                  value{0};
    std::vector<std::string> tags;  ///< New in V2; absent in V1
};

/// @brief Store that can hold records serialised as either V1 or V2.
///        V2 reader must be able to read V1 records without error (backward).
///        V1 reader must not crash on V2 records even if it ignores `tags`.
class VersionedSchemaStore {
public:
    /// @brief Write a V1 record.
    void WriteV1(const RecordV1& rec) {
        std::lock_guard<std::mutex> lk(mutex_);
        // Store as a generic map; V1 has no "tags" key
        auto& entry = store_[rec.id];
        entry["_version"] = "1";
        entry["name"]     = rec.name;
        entry["value"]    = std::to_string(rec.value);
    }

    /// @brief Write a V2 record.
    void WriteV2(const RecordV2& rec) {
        std::lock_guard<std::mutex> lk(mutex_);
        auto& entry = store_[rec.id];
        entry["_version"] = "2";
        entry["name"]     = rec.name;
        entry["value"]    = std::to_string(rec.value);
        // Serialise tags as comma-joined list
        std::string tags_str = {};
        for (size_t i = 0; i < rec.tags.size(); ++i) {
            if (i > 0) { tags_str += ','; }
            tags_str += rec.tags[i];
        }
        entry["tags"] = tags_str;
    }

    /// @brief Read as V2 — tolerates missing "tags" field (backward compat).
    [[nodiscard]] std::optional<RecordV2> ReadAsV2(const std::string& id) const {
        std::lock_guard<std::mutex> lk(mutex_);
        const auto it = store_.find(id);
        if (it == store_.end()) { return std::nullopt; }
        RecordV2 rec;
        rec.id    = id;
        rec.name  = it->second.at("name");
        rec.value = std::stoll(it->second.at("value"));
        const auto tags_it = it->second.find("tags");
        if (tags_it != it->second.end() && !tags_it->second.empty()) {
            // Split by comma
            std::string token = {};
            for (const char ch : tags_it->second) {
                if (ch == ',') {
                    if (!token.empty()) {
                        rec.tags.push_back(std::move(token));
                        token.clear();
                    }
                } else {
                    token += ch;
                }
            }
            if (!token.empty()) { rec.tags.push_back(std::move(token)); }
        }
        return rec;
    }

    /// @brief Read as V1 — ignores unknown fields (forward compat).
    [[nodiscard]] std::optional<RecordV1> ReadAsV1(const std::string& id) const {
        std::lock_guard<std::mutex> lk(mutex_);
        const auto it = store_.find(id);
        if (it == store_.end()) { return std::nullopt; }
        RecordV1 rec;
        rec.id    = id;
        rec.name  = it->second.at("name");
        rec.value = std::stoll(it->second.at("value"));
        // "tags" field is silently ignored — forward compatibility
        return rec;
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> store_;
};

// ---------------------------------------------------------------------------
// IdempotentApiEndpoint — CCR-06: API idempotency contract
// ---------------------------------------------------------------------------

/// @brief Models an API endpoint that guarantees idempotent creation via a
///        client-supplied idempotency key.
class IdempotentApiEndpoint {
public:
    struct CreateRequest {
        std::string idempotency_key;
        std::string resource_name;
        int64_t     resource_value{0};
    };

    struct CreateResponse {
        bool        created{false};  ///< true if newly created; false if duplicate
        std::string resource_id;
        std::string resource_name;
        int64_t     resource_value{0};
        std::string error = {};
    };

    /// @brief Process a creation request.
    /// Duplicate requests (same idempotency_key) return the original response.
    [[nodiscard]] CreateResponse Create(const CreateRequest& req) {
        std::lock_guard<std::mutex> lk(mutex_);
        const auto it = idempotency_map_.find(req.idempotency_key);
        if (it != idempotency_map_.end()) {
            // Return cached response unchanged
            return it->second;
        }
        // New creation
        CreateResponse resp;
        resp.created        = true;
        resp.resource_id    = "res-" + req.idempotency_key;
        resp.resource_name  = req.resource_name;
        resp.resource_value = req.resource_value;
        idempotency_map_[req.idempotency_key] = resp;
        return resp;
    }

    [[nodiscard]] size_t ResourceCount() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return idempotency_map_.size();
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, CreateResponse> idempotency_map_;
};

// ---------------------------------------------------------------------------
// PaginatedQueryEngine — CCR-07: pagination contract
// ---------------------------------------------------------------------------

/// @brief Simple deterministic in-memory query engine with cursor-based
///        pagination.
class PaginatedQueryEngine {
public:
    /// @brief Populate the store with N entries (key = "item-{i}").
    void Populate(size_t count) {
        std::lock_guard<std::mutex> lk(mutex_);
        items_.clear();
        items_.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            items_.push_back("item-" + std::to_string(i));
        }
    }

    struct PageResult {
        std::vector<std::string> items;
        size_t                   next_cursor{0};  ///< 0 == end of results
        bool                     has_more{false};
    };

    /// @brief Fetch a page starting at cursor with the given page_size.
    /// @param cursor     Inclusive start offset (0-based).
    /// @param page_size  Maximum items per page.
    /// @return PageResult with items and next_cursor.
    [[nodiscard]] PageResult Fetch(size_t cursor, size_t page_size) const {
        std::lock_guard<std::mutex> lk(mutex_);
        PageResult result = {};
        if (cursor >= items_.size()) {
            return result;
        }
        const size_t end = std::min(cursor + page_size, items_.size());
        result.items.assign(items_.begin() + static_cast<ptrdiff_t>(cursor),
                            items_.begin() + static_cast<ptrdiff_t>(end));
        if (end < items_.size()) {
            result.next_cursor = end;
            result.has_more    = true;
        }
        return result;
    }

    [[nodiscard]] size_t TotalCount() const {
        std::lock_guard<std::mutex> lk(mutex_);
        return items_.size();
    }

private:
    mutable std::mutex mutex_;
    std::vector<std::string> items_;
};

// ---------------------------------------------------------------------------
// ErrorContractValidator — CCR-08: structured error contract
// ---------------------------------------------------------------------------

/// @brief Error severity.
enum class ErrorSeverity { kInfo, kWarning, kError, kFatal };

/// @brief Structured API error response.
struct ApiError {
    std::string   code;      ///< machine-readable code (e.g. "not_found")
    std::string   message;   ///< human-readable description
    ErrorSeverity severity{ErrorSeverity::kError};
    std::string   context;   ///< optional additional context
};

/// @brief Validates that an error response conforms to the API error contract.
struct ErrorContractChecker {
    /// @brief All required fields must be non-empty; severity must be in range.
    /// @param error  Error to validate.
    /// @return true if the error conforms to the contract.
    [[nodiscard]] static bool IsConformant(const ApiError& error) noexcept {
        if (error.code.empty())    { return false; }
        if (error.message.empty()) { return false; }
        // severity must be a valid enum value
        const auto sv = static_cast<int>(error.severity);
        if (sv < 0 || sv > static_cast<int>(ErrorSeverity::kFatal)) { return false; }
        return true;
    }
};

/// @brief Produces typed API errors for common failure cases, used to assert
///        the error contract across different failure paths.
class ApiErrorFactory {
public:
    /// @brief Produce a "not_found" error.
    [[nodiscard]] static ApiError NotFound(const std::string& detail) {
        return {"not_found", "Resource not found: " + detail,
                ErrorSeverity::kError, detail};
    }

    /// @brief Produce a "validation_error" for a bad field.
    [[nodiscard]] static ApiError ValidationError(const std::string& field) {
        return {"validation_error", "Validation failed for field: " + field,
                ErrorSeverity::kWarning, field};
    }

    /// @brief Produce an "auth_denied" error.
    [[nodiscard]] static ApiError AuthDenied(const std::string& reason) {
        return {"auth_denied", "Authorization denied: " + reason,
                ErrorSeverity::kError, reason};
    }

    /// @brief Produce an "internal_error" with no context (edge case).
    [[nodiscard]] static ApiError InternalError() {
        return {"internal_error", "An unexpected internal error occurred",
                ErrorSeverity::kFatal, ""};
    }
};

}  // namespace

// ===========================================================================
// Test Fixture
// ===========================================================================

class ContractCompatibilityReliabilityTest : public IntegrationTestFixture {
protected:
    void SetUp() override {
        IntegrationTestFixture::SetUp();
        rng_ = std::mt19937{kCanonicalSeed};
    }

    std::mt19937 rng_;
};

// ===========================================================================
// CCR-01 — Forward compatibility: unknown fields in record are tolerated
// ===========================================================================

TEST_F(ContractCompatibilityReliabilityTest,
       CCR01_UnknownFieldsInRecordAreTolerated) {
    SCOPED_TRACE("CCR-01: forward compatibility — unknown fields tolerated");

    // Define a V1 schema with two known fields
    Schema schema_v1 = {
        {"name",  FieldType::kString,  true},
        {"value", FieldType::kInteger, true},
    };
    SchemaValidator validator(schema_v1);

    // V1-compliant record — must pass
    SchemaRecord v1_record;
    v1_record["name"]  = {FieldType::kString, "Alice", 0, false};
    v1_record["value"] = {FieldType::kInteger, "", 42, false};

    const auto r1 = validator.Validate(v1_record);
    EXPECT_TRUE(r1.ok) << "V1-compliant record must pass V1 schema";

    // V2 record adds "tags" (unknown to V1 validator) — must still pass
    SchemaRecord v2_record = v1_record;
    v2_record["tags"] = {FieldType::kString, "alpha,beta", 0, false};

    const auto r2 = validator.Validate(v2_record);
    EXPECT_TRUE(r2.ok) << "V2 record with unknown field must still pass V1 schema";
    EXPECT_TRUE(r2.errors.empty());
}

// ===========================================================================
// CCR-02 — Required-field contract: absent required field returns error
// ===========================================================================

TEST_F(ContractCompatibilityReliabilityTest,
       CCR02_AbsentRequiredFieldProducesStructuredError) {
    SCOPED_TRACE("CCR-02: required-field contract");

    Schema schema = {
        {"id",    FieldType::kString,  true},
        {"name",  FieldType::kString,  true},
        {"score", FieldType::kInteger, false},  // optional
    };
    SchemaValidator validator(schema);

    // Record missing required "name"
    SchemaRecord record;
    record["id"]    = {FieldType::kString, "doc-1", 0, false};
    record["score"] = {FieldType::kInteger, "", 99, false};

    const auto result = validator.Validate(record);
    EXPECT_FALSE(result.ok) << "missing required field must fail validation";
    ASSERT_FALSE(result.errors.empty());
    EXPECT_EQ(result.errors[0], "missing_required:name");

    // Record with all required fields present — must pass
    record["name"] = {FieldType::kString, "Alice", 0, false};
    const auto ok_result = validator.Validate(record);
    EXPECT_TRUE(ok_result.ok);
    EXPECT_TRUE(ok_result.errors.empty());
}

// ===========================================================================
// CCR-03 — Backward compatibility: V2 reader tolerates V1 records
// ===========================================================================

TEST_F(ContractCompatibilityReliabilityTest,
       CCR03_V2ReaderToleratesV1RecordsMissingTagsField) {
    SCOPED_TRACE("CCR-03: backward compatibility — V2 reader handles V1 record");

    VersionedSchemaStore store;

    // Write a V1 record (no "tags" field)
    store.WriteV1({"id-v1", "AliceV1", 100});

    // V2 reader must read it without error; tags must be empty vector
    const auto result = store.ReadAsV2("id-v1");
    ASSERT_TRUE(result.has_value()) << "V2 reader must find V1 record";
    EXPECT_EQ(result->id,    "id-v1");
    EXPECT_EQ(result->name,  "AliceV1");
    EXPECT_EQ(result->value, 100);
    EXPECT_TRUE(result->tags.empty())
        << "V2 tags must be empty vector when reading a V1 record";
}

// ===========================================================================
// CCR-04 — Forward compatibility: V1 reader ignores extra V2 fields
// ===========================================================================

TEST_F(ContractCompatibilityReliabilityTest,
       CCR04_V1ReaderIgnoresExtraFieldsFromV2Record) {
    SCOPED_TRACE("CCR-04: forward compatibility — V1 reader ignores V2 tags");

    VersionedSchemaStore store;

    // Write a V2 record with tags
    store.WriteV2({"id-v2", "BobV2", 200, {"tag-x", "tag-y"}});

    // V1 reader must read it without error; "tags" field is silently ignored
    const auto result = store.ReadAsV1("id-v2");
    ASSERT_TRUE(result.has_value()) << "V1 reader must find V2 record";
    EXPECT_EQ(result->id,    "id-v2");
    EXPECT_EQ(result->name,  "BobV2");
    EXPECT_EQ(result->value, 200);
    // No crash, no error — forward compatibility achieved
}

// ===========================================================================
// CCR-05 — Type contract: numeric field rejects string-typed value
// ===========================================================================

TEST_F(ContractCompatibilityReliabilityTest,
       CCR05_NumericFieldRejectsStringTypedValue) {
    SCOPED_TRACE("CCR-05: type-contract enforcement");

    Schema schema = {
        {"count", FieldType::kInteger, true},
        {"label", FieldType::kString,  true},
    };
    SchemaValidator validator(schema);

    // Correct types
    SchemaRecord correct;
    correct["count"] = {FieldType::kInteger, "", 7, false};
    correct["label"] = {FieldType::kString, "ok", 0, false};

    const auto r_ok = validator.Validate(correct);
    EXPECT_TRUE(r_ok.ok);

    // "count" provided as string — type mismatch
    SchemaRecord wrong_type;
    wrong_type["count"] = {FieldType::kString, "seven", 0, false};  // wrong type!
    wrong_type["label"] = {FieldType::kString, "label", 0, false};

    const auto r_err = validator.Validate(wrong_type);
    EXPECT_FALSE(r_err.ok) << "string value for integer field must fail";
    ASSERT_FALSE(r_err.errors.empty());
    EXPECT_EQ(r_err.errors[0], "type_mismatch:count");

    // "label" provided as integer — type mismatch
    SchemaRecord wrong_label;
    wrong_label["count"] = {FieldType::kInteger, "", 1, false};
    wrong_label["label"] = {FieldType::kInteger, "", 99, false};  // wrong type!

    const auto r_err2 = validator.Validate(wrong_label);
    EXPECT_FALSE(r_err2.ok);
    ASSERT_FALSE(r_err2.errors.empty());
    EXPECT_EQ(r_err2.errors[0], "type_mismatch:label");
}

// ===========================================================================
// CCR-06 — API idempotency contract: repeated calls return identical result
// ===========================================================================

TEST_F(ContractCompatibilityReliabilityTest,
       CCR06_RepeatedApiCallWithSameIdempotencyKeyReturnsSameResult) {
    SCOPED_TRACE("CCR-06: API idempotency contract");

    IdempotentApiEndpoint endpoint;

    IdempotentApiEndpoint::CreateRequest req;
    req.idempotency_key  = "idem-key-001";
    req.resource_name    = "my-resource";
    req.resource_value   = 42;

    // First call — must create
    const auto first = endpoint.Create(req);
    EXPECT_TRUE(first.created);
    EXPECT_EQ(first.resource_name, "my-resource");
    EXPECT_EQ(first.resource_value, 42);

    // Second call — must NOT create; must return identical fields
    const auto second = endpoint.Create(req);
    EXPECT_FALSE(second.created) << "second call must not create a new resource";
    EXPECT_EQ(second.resource_id,    first.resource_id);
    EXPECT_EQ(second.resource_name,  first.resource_name);
    EXPECT_EQ(second.resource_value, first.resource_value);

    // Third call — same contract
    const auto third = endpoint.Create(req);
    EXPECT_FALSE(third.created);
    EXPECT_EQ(third.resource_id, first.resource_id);

    // Resource count must remain 1
    EXPECT_EQ(endpoint.ResourceCount(), 1u);

    // Different idempotency key — creates independently
    req.idempotency_key = "idem-key-002";
    const auto other = endpoint.Create(req);
    EXPECT_TRUE(other.created);
    EXPECT_EQ(endpoint.ResourceCount(), 2u);
}

// ===========================================================================
// CCR-07 — Pagination contract: sequential pages are non-overlapping
// ===========================================================================

TEST_F(ContractCompatibilityReliabilityTest,
       CCR07_SequentialPagesAreNonOverlappingAndCoverAllItems) {
    SCOPED_TRACE("CCR-07: pagination contract — non-overlapping sequential pages");

    PaginatedQueryEngine engine;
    engine.Populate(25);

    constexpr size_t kPageSize = 10;
    std::unordered_set<std::string> seen;
    size_t cursor = 0;
    size_t page_count = 0;

    while (true) {
        const auto page = engine.Fetch(cursor, kPageSize);
        ASSERT_FALSE(page.items.empty() && cursor == 0)
            << "first page must not be empty for a populated store";

        for (const auto& item : page.items) {
            EXPECT_EQ(seen.count(item), 0u)
                << "item '" << item << "' appeared in multiple pages (overlap)";
            seen.insert(item);
        }
        ++page_count;

        if (!page.has_more) { break; }
        cursor = page.next_cursor;

        // Guard against infinite loop
        ASSERT_LT(page_count, 10u) << "too many pages — pagination loop detected";
    }

    // All 25 items must appear across pages
    EXPECT_EQ(seen.size(), engine.TotalCount())
        << "all items must appear in exactly one page";
    EXPECT_EQ(seen.size(), 25u);

    // Exactly 3 pages for 25 items at page_size=10: [0..9], [10..19], [20..24]
    EXPECT_EQ(page_count, 3u);
}

// ===========================================================================
// CCR-08 — Error contract: all API errors have consistent structure
// ===========================================================================

TEST_F(ContractCompatibilityReliabilityTest,
       CCR08_AllApiErrorPathsProduceConformantErrorStructures) {
    SCOPED_TRACE("CCR-08: error contract — all error paths produce valid structure");

    // Every error factory path must produce a conformant error
    const std::vector<ApiError> errors = {
        ApiErrorFactory::NotFound("doc-42"),
        ApiErrorFactory::ValidationError("field_name"),
        ApiErrorFactory::AuthDenied("token-expired"),
        ApiErrorFactory::InternalError(),
    };

    for (const auto& err : errors) {
        EXPECT_TRUE(ErrorContractChecker::IsConformant(err))
            << "non-conformant error for code='" << err.code
            << "' message='" << err.message << "'";
        EXPECT_FALSE(err.code.empty())
            << "error.code must never be empty";
        EXPECT_FALSE(err.message.empty())
            << "error.message must never be empty";
    }

    // Specific field checks
    const auto not_found = ApiErrorFactory::NotFound("my-doc");
    EXPECT_EQ(not_found.code, "not_found");
    EXPECT_EQ(not_found.severity, ErrorSeverity::kError);
    EXPECT_FALSE(not_found.context.empty());

    const auto val_err = ApiErrorFactory::ValidationError("my_field");
    EXPECT_EQ(val_err.code, "validation_error");
    EXPECT_EQ(val_err.severity, ErrorSeverity::kWarning);

    const auto internal = ApiErrorFactory::InternalError();
    EXPECT_EQ(internal.code, "internal_error");
    EXPECT_EQ(internal.severity, ErrorSeverity::kFatal);
    // context is optional — empty is allowed for internal errors
}
} } // namespace themis::test
