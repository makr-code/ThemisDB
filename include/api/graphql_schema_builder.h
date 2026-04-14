/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            graphql_schema_builder.h                           ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-13 20:20:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     205                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a4bbe03b5e  2026-03-10  feat(api): add header-only interface definitions — IHttpH... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
