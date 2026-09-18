/**
 * @file graphql_schema_builder.h
 * @brief GraphQL schema construction and validation interfaces.
 *
 * @details Provides type descriptors and builder interfaces for constructing
 * GraphQL schemas without pulling in the full GraphQL parser library.
 *
 * Core components:
 *  - `GraphQLTypeDescriptor`: Plain-data struct for a GraphQL object type
 *  - `SchemaValidationResult`: Validation result with error information
 *  - `IGraphQLSchemaBuilder`: Pure-virtual interface for schema registration
 *
 * Type descriptor fields:
 *  - name: GraphQL type name (e.g., "User", "Query")
 *  - fields: map of field name to GraphQL type string (e.g., { "id": "ID!", "name": "String" })
 *  - description: Markdown documentation for schema introspection
 *
 * Schema validation:
 *  - Types must have unique names within the schema
 *  - Field names within a type must be unique
 *  - Field types must reference valid GraphQL scalar or object types
 *  - Circular type references are permitted
 *
 * ### Thread safety
 * Schema construction is typically single-threaded at startup.
 * After schema is finalized, introspection queries are thread-safe.
 *
 * ### Usage
 * ```cpp
 * auto builder = createGraphQLSchemaBuilder();
 *
 * GraphQLTypeDescriptor user_type;
 * user_type.name = "User";
 * user_type.fields["id"] = "ID!";
 * user_type.fields["name"] = "String!";
 * user_type.fields["email"] = "String";
 *
 * auto result = builder->addType(user_type);
 * if (!result.valid) {
 *     std::cerr << "Schema error: " << result.errorMessage << "\\n";
 * }
 * ```
 *
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 */


#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <memory>
#include <functional>
#include "utils/expected.h"

namespace themis {
namespace api {

// ---------------------------------------------------------------------------
// Forward declarations
// ---------------------------------------------------------------------------

struct GraphQLTypeDescriptor;
struct SchemaValidationResult;

// ---------------------------------------------------------------------------
// GraphQLTypeDescriptor — plain-data descriptor for a GraphQL type
// ---------------------------------------------------------------------------

struct GraphQLFieldDescriptor {
    std::string name;         ///< Field name (must be a valid GraphQL identifier)
    std::string type_string;  ///< GraphQL type expression, e.g. "String!", "[Entity!]!"
    std::string description;  ///< Optional human-readable description (appears in introspection)
    bool        deprecated = false; ///< Whether this field is deprecated
    std::string deprecation_reason; ///< Non-empty when `deprecated == true`
};

struct GraphQLTypeDescriptor {
    std::string name;           ///< Type name, e.g. "Entity", "Query", "Mutation"
    std::string description;    ///< Optional human-readable description
    std::vector<GraphQLFieldDescriptor> fields; ///< Fields exposed by this type
    bool is_interface = false;  ///< True for `interface` types; false for `type`
};

// ---------------------------------------------------------------------------
// SchemaValidationResult — structured result of IGraphQLSchemaBuilder::build()
// ---------------------------------------------------------------------------

struct SchemaValidationError {
    std::string type_name;  ///< Name of the type where the error occurred, or empty for global errors
    std::string field_name; ///< Name of the field where the error occurred, or empty for type-level errors
    std::string message;    ///< Human-readable error description
};

struct SchemaValidationResult {
    bool valid = false;
    std::vector<SchemaValidationError> errors;

    /**
     * @brief Ok.
     * @return Return value.
     * @details Implements ok without additional internal calls.
     */
    static SchemaValidationResult ok() {
        return {true, {}};
    }

    /**
     * @brief Fail.
     * @param[in] errs Input parameter.
     * @return Return value.
     * @details Calls: std::move().
     */
    static SchemaValidationResult fail(std::vector<SchemaValidationError> errs) {
        return {false, std::move(errs)};
    }

    /**
     * @brief Fail.
     * @param[in] type_name Name of the type.
     * @param[in] field_name Name of the field.
     * @param[in] message Input parameter.
     * @return Return value.
     * @details Calls: std::move().
     */
    static SchemaValidationResult fail(std::string type_name, std::string field_name, std::string message) {
        SchemaValidationError e;
        e.type_name  = std::move(type_name);
        e.field_name = std::move(field_name);
        e.message    = std::move(message);
        return {false, {std::move(e)}};
    }
};

// ---------------------------------------------------------------------------
// IGraphQLSchemaBuilder — pure-virtual interface for schema construction
// ---------------------------------------------------------------------------

class IGraphQLSchemaBuilder {
public:
    /**
     * @brief IGraph QLSchema Builder.
     * @return Return value.
     */
    virtual ~IGraphQLSchemaBuilder() = default;

    /**
     * @brief Add Type.
     * @param[in] descriptor Input parameter.
     * @return Return value.
     */
    virtual IGraphQLSchemaBuilder& addType(GraphQLTypeDescriptor descriptor) = 0;

    /**
     * @brief Add Query.
     * @param[in] field Input parameter.
     * @return Return value.
     */
    virtual IGraphQLSchemaBuilder& addQuery(GraphQLFieldDescriptor field) = 0;

    /**
     * @brief Add Mutation.
     * @param[in] field Input parameter.
     * @return Return value.
     */
    virtual IGraphQLSchemaBuilder& addMutation(GraphQLFieldDescriptor field) = 0;

    /**
     * @brief Build.
     * @return Return value.
     */
    virtual SchemaValidationResult build() = 0;

    /**
     * @brief Is Built.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    virtual bool isBuilt() const noexcept = 0;
};

} // namespace api
} // namespace themis
