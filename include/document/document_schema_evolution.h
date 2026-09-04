/**
 * @file document_schema_evolution.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.3
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * ThemisDB — Document Module
 *
 * File:    document_schema_evolution.h
 * Module:  include/document/
 * Purpose: IDocumentSchemaEvolution — schema-version registry and
 *          point-in-time document validation.  Includes
 *          InMemoryDocumentSchemaEvolution as a reference implementation.
 *
 * Version: 1.3.0
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "document/document_store.h"
#include "utils/expected.h"

#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace document {

// ─────────────────────────────────────────────────────────────────────────────
// SchemaVersion
// ─────────────────────────────────────────────────────────────────────────────

/// @brief Monotonically increasing schema version number (1-based).
using SchemaVersion = std::uint32_t;

// ─────────────────────────────────────────────────────────────────────────────
// SchemaFieldType
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief JSON-level type for a schema field descriptor.
 */
enum class SchemaFieldType {
    STRING,
    NUMBER,
    BOOLEAN,
    OBJECT,
    ARRAY,
    ANY,  ///< No type constraint
};

// ─────────────────────────────────────────────────────────────────────────────
// SchemaFieldDescriptor
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Definition of a single field within a schema version.
 */
struct SchemaFieldDescriptor {
    std::string       name;          ///< JSON key name
    SchemaFieldType   type{SchemaFieldType::ANY}; ///< Expected JSON type
    bool              required{false};            ///< Must be present in every document
    nlohmann::json    default_value;              ///< Suggested default (informational)
};

// ─────────────────────────────────────────────────────────────────────────────
// SchemaDescriptor
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Composable plain-data descriptor for a complete schema version.
 *
 * Immutable after being passed to IDocumentSchemaEvolution::registerVersion().
 */
struct SchemaDescriptor {
    std::vector<SchemaFieldDescriptor> fields; ///< Field definitions
};

// ─────────────────────────────────────────────────────────────────────────────
// FieldViolationKind
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Classification of a schema validation failure.
 */
enum class FieldViolationKind {
    MISSING_REQUIRED_FIELD,  ///< Required field absent from document
    TYPE_MISMATCH,           ///< Field present but has wrong JSON type
};

// ─────────────────────────────────────────────────────────────────────────────
// FieldViolation
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A single field-level schema violation found during validation.
 */
struct FieldViolation {
    std::string        field_name;    ///< Offending JSON key
    FieldViolationKind kind;          ///< Nature of the violation
    std::string        suggested_fix; ///< Human-readable remediation hint
};

// ─────────────────────────────────────────────────────────────────────────────
// ValidationReport
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Result of IDocumentSchemaEvolution::validate().
 *
 * A report with an empty @c violations list indicates the document is fully
 * schema-compliant.
 */
struct ValidationReport {
    DocumentId                 document_id;  ///< Document that was validated
    SchemaVersion              version;      ///< Schema version used
    std::vector<FieldViolation> violations;  ///< Empty ⟹ valid

    bool isValid() const noexcept { return violations.empty(); }
};

// ─────────────────────────────────────────────────────────────────────────────
// IDocumentSchemaEvolution
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Schema-version registry and validation interface.
 *
 * ### Immutability contract
 * - Once seal() is called, no further versions may be registered; any attempt
 *   returns ERR_DOC_SCHEMA_SEALED.
 * - Registered schema versions are themselves immutable; field definitions
 *   cannot be modified post-registration.
 *
 * ### Error codes
 *   - ERR_DOC_SCHEMA_SEALED           — registerVersion() after seal()
 *   - ERR_DOC_SCHEMA_VERSION_EXISTS   — duplicate version number
 *   - ERR_DOC_SCHEMA_VERSION_NOT_FOUND — validate() with unknown version
 *   - ERR_DOC_NOT_FOUND               — validate() document id not in store
 */
class IDocumentSchemaEvolution {
public:
    virtual ~IDocumentSchemaEvolution() = default;

    /**
     * @brief Register a new schema version.
     *
     * @return ERR_DOC_SCHEMA_SEALED         if already sealed.
     * @return ERR_DOC_SCHEMA_VERSION_EXISTS if @p version is already registered.
     */
    [[nodiscard]] virtual Result<void> registerVersion(SchemaVersion           version,
                                         const SchemaDescriptor& descriptor) = 0;

    /**
     * @brief Seal the registry; no further versions may be registered after
     *        this call.  Idempotent.
     */
    virtual void seal() noexcept = 0;

    /**
     * @brief Return true iff the registry has been sealed.
     */
    [[nodiscard]] virtual bool isSealed() const noexcept = 0;

    /**
     * @brief List all registered version numbers in ascending order.
     */
    [[nodiscard]] virtual std::vector<SchemaVersion> registeredVersions() const = 0;

    /**
     * @brief Validate @p document_body against @p version.
     *
     * @return ERR_DOC_SCHEMA_VERSION_NOT_FOUND if @p version is unknown.
     */
    [[nodiscard]] virtual Result<ValidationReport> validate(
        const DocumentId&     document_id,
        const nlohmann::json& document_body,
        SchemaVersion         version) const = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// InMemoryDocumentSchemaEvolution
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Thread-safe in-memory implementation of IDocumentSchemaEvolution.
 */
class InMemoryDocumentSchemaEvolution final : public IDocumentSchemaEvolution {
public:
    Result<void> registerVersion(SchemaVersion           version,
                                 const SchemaDescriptor& descriptor) override
    {
        std::lock_guard<std::mutex> lk(mu_);
        if (sealed_) {
            return tl::unexpected(Error(
                errors::ErrorCode::ERR_DOC_SCHEMA_SEALED,
                "schema registry is sealed"));
        }
        if (schemas_.count(version)) {
            return tl::unexpected(Error(
                errors::ErrorCode::ERR_DOC_SCHEMA_VERSION_EXISTS,
                std::to_string(version)));
        }
        schemas_[version] = descriptor;
        return Result<void>{};
    }

    void seal() noexcept override {
        std::lock_guard<std::mutex> lk(mu_);
        sealed_ = true;
    }

    bool isSealed() const noexcept override {
        std::lock_guard<std::mutex> lk(mu_);
        return sealed_;
    }

    std::vector<SchemaVersion> registeredVersions() const override {
        std::lock_guard<std::mutex> lk(mu_);
        std::vector<SchemaVersion> vs = {};

        vs.reserve(schemas_.size());
        for (const auto& [v, _] : schemas_) {
            vs.push_back(v);
        }
        return vs; // std::map iterates in ascending key order
    }

    Result<ValidationReport> validate(
        const DocumentId&     document_id,
        const nlohmann::json& document_body,
        SchemaVersion         version) const override
    {
        std::lock_guard<std::mutex> lk(mu_);
        auto it = schemas_.find(version);
        if (it == schemas_.end()) {
            return tl::unexpected(Error(
                errors::ErrorCode::ERR_DOC_SCHEMA_VERSION_NOT_FOUND,
                std::to_string(version)));
        }

        const SchemaDescriptor& schema = it->second;
        ValidationReport report;
        report.document_id = document_id;
        report.version     = version;

        for (const auto& field : schema.fields) {
            if (!document_body.contains(field.name)) {
                if (field.required) {
                    report.violations.push_back({
                        field.name,
                        FieldViolationKind::MISSING_REQUIRED_FIELD,
                        "Add required field '" + field.name + "' to the document"
                    });
                }
            } else if (field.type != SchemaFieldType::ANY) {
                bool type_ok = checkType(document_body[field.name], field.type);
                if (!type_ok) {
                    report.violations.push_back({
                        field.name,
                        FieldViolationKind::TYPE_MISMATCH,
                        "Field '" + field.name + "' has unexpected type"
                    });
                }
            }
        }

        return report;
    }

private:
    static bool checkType(const nlohmann::json& val, SchemaFieldType expected) {
        switch (expected) {
            case SchemaFieldType::STRING:  return val.is_string();
            case SchemaFieldType::NUMBER:  return val.is_number();
            case SchemaFieldType::BOOLEAN: return val.is_boolean();
            case SchemaFieldType::OBJECT:  return val.is_object();
            case SchemaFieldType::ARRAY:   return val.is_array();
            case SchemaFieldType::ANY:     return true;
        }
        return true;
    }

    mutable std::mutex                    mu_;
    bool                                  sealed_{false};
    std::map<SchemaVersion, SchemaDescriptor> schemas_;
};

} // namespace document
} // namespace themis
