/**
 * @file schema_context_builder.h
 * @brief Header-only schema context builder for AQL Phase 5 unified testing.
 *
 * Provides a fluent builder for constructing schema context strings and
 * objects for use in AQL validation and translation tests. Includes:
 * - Builder pattern for dynamic schema construction
 * - Preset schemas (users, products, orders)
 * - Invalid schema variants (missing fields, type mismatches)
 *
 * Usage:
 * @code
 *   auto schema = SchemaContextBuilder::preset_users();
 *   std::string ctx_str = schema.buildContextString();
 *
 *   // Custom schema
 *   auto custom = SchemaContextBuilder()
 *       .collection("events")
 *       .field("id",        "string",  true)
 *       .field("timestamp", "int",     true)
 *       .field("payload",   "object",  false)
 *       .buildContextString();
 * @endcode
 *
 * @note Header-only, no infrastructure dependencies.
 */


#pragma once

#include <sstream>
#include <string>
#include <vector>

namespace themis {
namespace aql {
namespace testing {

// ============================================================================
// Field Descriptor
// ============================================================================

struct SchemaField {
    std::string name;
    std::string type;       // "string", "int", "float", "bool", "array", "object"
    bool        required = false;
    std::string description;
};

// ============================================================================
// Schema Context Builder
// ============================================================================

class SchemaContextBuilder {
public:
    SchemaContextBuilder() = default;

    /// Set the collection name
    SchemaContextBuilder& collection(const std::string& name) {
        collection_name_ = name;
        return *this;
    }

    /// Add a field to the schema
    SchemaContextBuilder& field(const std::string& name, const std::string& type,
                                 bool required = false, const std::string& desc = "") {
        fields_.push_back({name, type, required, desc});
        return *this;
    }

    /// Build a schema context string in the format used by ThemisDB validation
    std::string buildContextString() const {
        std::ostringstream oss;
        oss << "collection:" << collection_name_ << " fields:[";
        for (std::size_t i = 0; i < fields_.size(); ++i) {
            if (i > 0) oss << ",";
            const auto& f = fields_[i];
            oss << f.name << ":" << f.type;
            if (f.required) oss << "(required)";
        }
        oss << "]";
        return oss.str();
    }

    /// Build a JSON-like schema context string
    std::string buildJSONContextString() const {
        std::ostringstream oss;
        oss << "{\"collection\":\"" << collection_name_ << "\",\"fields\":[";
        for (std::size_t i = 0; i < fields_.size(); ++i) {
            if (i > 0) oss << ",";
            const auto& f = fields_[i];
            oss << "{\"name\":\"" << f.name << "\",\"type\":\"" << f.type << "\""
                << ",\"required\":" << (f.required ? "true" : "false") << "}";
        }
        oss << "]}";
        return oss.str();
    }

    const std::string& getCollectionName() const { return collection_name_; }
    const std::vector<SchemaField>& getFields() const { return fields_; }

    // ------------------------------------------------------------------
    // Preset Schemas
    // ------------------------------------------------------------------

    /// @brief Preset: users collection (id, name, email, age, created_at)
    static SchemaContextBuilder preset_users() {
        return SchemaContextBuilder()
            .collection("users")
            .field("_id",        "string", true,  "Document key")
            .field("name",       "string", true,  "Full name")
            .field("email",      "string", true,  "Email address")
            .field("age",        "int",    false, "User age")
            .field("created_at", "int",    false, "Unix timestamp");
    }

    /// @brief Preset: products collection (id, name, price, category, stock)
    static SchemaContextBuilder preset_products() {
        return SchemaContextBuilder()
            .collection("products")
            .field("_id",      "string", true,  "Document key")
            .field("name",     "string", true,  "Product name")
            .field("price",    "float",  true,  "Unit price")
            .field("category", "string", false, "Product category")
            .field("stock",    "int",    false, "Available stock");
    }

    /// @brief Preset: orders collection (id, user_id, items, total, status)
    static SchemaContextBuilder preset_orders() {
        return SchemaContextBuilder()
            .collection("orders")
            .field("_id",       "string", true,  "Document key")
            .field("user_id",   "string", true,  "Reference to users._id")
            .field("items",     "array",  true,  "Array of ordered items")
            .field("total",     "float",  true,  "Total order value")
            .field("status",    "string", false, "Order status (pending/shipped/delivered)");
    }

    // ------------------------------------------------------------------
    // Invalid / Pathological Variants
    // ------------------------------------------------------------------

    /// @brief Invalid: empty collection name
    static std::string invalid_empty_collection() {
        return SchemaContextBuilder()
            .collection("")
            .field("id", "string", true)
            .buildContextString();
    }

    /// @brief Invalid: no fields registered
    static std::string invalid_no_fields() {
        return SchemaContextBuilder()
            .collection("empty_collection")
            .buildContextString();
    }

    /// @brief Invalid: field with unknown type (triggers TypeMismatch)
    static SchemaContextBuilder invalid_type_mismatch() {
        return SchemaContextBuilder()
            .collection("bad_types")
            .field("amount", "UNKNOWN_TYPE", false, "Field with undefined type");
    }

    /// @brief Invalid: required fields missing (partial schema)
    static SchemaContextBuilder invalid_missing_required() {
        return SchemaContextBuilder()
            .collection("partial_schema")
            .field("description", "string", false);
        // '_id' and other required fields are intentionally absent
    }

    /// @brief Large schema: collection with N fields (configurable)
    static SchemaContextBuilder large_schema(int field_count = 200) {
        SchemaContextBuilder builder;
        builder.collection("large_collection");
        builder.field("_id", "string", true);
        for (int i = 0; i < field_count; ++i) {
            const std::string type = (i % 4 == 0) ? "string"
                                   : (i % 4 == 1) ? "int"
                                   : (i % 4 == 2) ? "float"
                                   : "bool";
            builder.field("attr_" + std::to_string(i), type, false);
        }
        return builder;
    }

    /// @brief Multi-collection context string (simulates join-like queries)
    static std::string multi_collection_context(
        const std::vector<std::string>& collection_names)
    {
        std::ostringstream oss;
        for (std::size_t i = 0; i < collection_names.size(); ++i) {
            if (i > 0) oss << " ";
            oss << "collection:" << collection_names[i];
        }
        return oss.str();
    }

private:
    std::string             collection_name_;
    std::vector<SchemaField> fields_;
};

}  // namespace testing
}  // namespace aql
}  // namespace themis
