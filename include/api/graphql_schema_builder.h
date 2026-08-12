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
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Plain-data struct describing a single GraphQL object type.
 *
 * Intended for use with `IGraphQLSchemaBuilder::addType()` so that callers
 * can register types without a dependency on the full `graphql.h` schema library.
 *
 * ### Field types
 * Field types are expressed as GraphQL type strings, e.g.:
 *  - `"String"` — non-null string scalar
 *  - `"String!"` — non-null string (required field)
 *  - `"[Entity]"` — list of Entity objects
 *  - `"[Entity!]!"` — non-null list of non-null Entity objects
 */
struct GraphQLFieldDescriptor {
    std::string name;         ///< Field name (must be a valid GraphQL identifier)
    std::string type_string;  ///< GraphQL type expression, e.g. "String!", "[Entity!]!"
    std::string description;  ///< Optional human-readable description (appears in introspection)
    bool        deprecated = false; ///< Whether this field is deprecated
    std::string deprecation_reason; ///< Non-empty when `deprecated == true`
};

/**
 * @brief Plain-data struct describing a single GraphQL object type for registration.
 *
 * Passed to `IGraphQLSchemaBuilder::addType()`.  The builder validates the
 * descriptor and returns a `SchemaValidationResult` on `build()`.
 */
struct GraphQLTypeDescriptor {
    std::string name;           ///< Type name, e.g. "Entity", "Query", "Mutation"
    std::string description;    ///< Optional human-readable description
    std::vector<GraphQLFieldDescriptor> fields; ///< Fields exposed by this type
    bool is_interface = false;  ///< True for `interface` types; false for `type`
};

// ---------------------------------------------------------------------------
// SchemaValidationResult — structured result of IGraphQLSchemaBuilder::build()
// ---------------------------------------------------------------------------

/**
 * @brief A single validation error produced during schema building.
 */
struct SchemaValidationError {
    std::string type_name;  ///< Name of the type where the error occurred, or empty for global errors
    std::string field_name; ///< Name of the field where the error occurred, or empty for type-level errors
    std::string message;    ///< Human-readable error description
};

/**
 * @brief Structured result returned by `IGraphQLSchemaBuilder::build()`.
 *
 * A successful build has `valid == true` and an empty `errors` list.
 * A failed build has `valid == false` and one or more entries in `errors`.
 *
 * Usage:
 * ```cpp
 * auto result = builder.build();
 * if (!result.valid) {
 *     for (const auto& err : result.errors) {
 *         LOG_ERROR("{}.{}: {}", err.type_name, err.field_name, err.message);
 *     }
 * }
 * ```
 */
struct SchemaValidationResult {
    bool valid = false;
    std::vector<SchemaValidationError> errors;

    static SchemaValidationResult ok() {
        return {true, {}};
    }

    static SchemaValidationResult fail(std::vector<SchemaValidationError> errs) {
        return {false, std::move(errs)};
    }

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

/**
 * @brief Pure-virtual interface for constructing a GraphQL schema at server
 *        initialization time.
 *
 * ### Contract
 * - `addType()` may be called multiple times before `build()`.
 * - `addQuery()` and `addMutation()` register root-level fields.
 * - `build()` is callable exactly once.  A second call returns a
 *   `Result::error` with `SchemaAlreadyBuilt` to prevent post-init mutation.
 * - After `build()` returns, the schema is immutable; further `add*()` calls
 *   must throw `std::logic_error` or be no-ops (implementation choice).
 * - `build()` validates all registered types for: duplicate names, unknown
 *   field type references, and injection-vulnerable field name patterns.
 *
 * ### Thread safety
 * `add*()` calls are NOT thread-safe; all registrations must happen on a
 * single thread before `build()`.  After `build()` returns successfully, the
 * schema is safe to read from any thread.
 */
class IGraphQLSchemaBuilder {
public:
    virtual ~IGraphQLSchemaBuilder() = default;

    /**
     * @brief Register an object or interface type.
     *
     * @param descriptor  Description of the type and its fields.
     * @return `*this` for fluent chaining.
     * @throws std::logic_error if `build()` has already been called.
     */
    virtual IGraphQLSchemaBuilder& addType(GraphQLTypeDescriptor descriptor) = 0;

    /**
     * @brief Register a root Query field (i.e. a query entry point).
     *
     * @param field  Field descriptor for the root query field.
     * @return `*this` for fluent chaining.
     * @throws std::logic_error if `build()` has already been called.
     */
    virtual IGraphQLSchemaBuilder& addQuery(GraphQLFieldDescriptor field) = 0;

    /**
     * @brief Register a root Mutation field.
     *
     * @param field  Field descriptor for the root mutation field.
     * @return `*this` for fluent chaining.
     * @throws std::logic_error if `build()` has already been called.
     */
    virtual IGraphQLSchemaBuilder& addMutation(GraphQLFieldDescriptor field) = 0;

    /**
     * @brief Validate and freeze the schema.
     *
     * Validates all registered types and fields.  On success returns
     * `SchemaValidationResult::ok()`.  On failure returns a result with
     * `valid == false` and one or more structured errors.
     *
     * Calling `build()` a second time returns an error with the message
     * `"Schema already built"`.
     *
     * @return Structured validation result.
     */
    virtual SchemaValidationResult build() = 0;

    /**
     * @brief Return `true` after a successful `build()` call.
     */
    virtual bool isBuilt() const noexcept = 0;
};

} // namespace api
} // namespace themis
