/**
 * @file graphql_federation.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "importers/schema_inference.h"
#include <string>
#include <vector>

namespace themis {
namespace importers {

/**
 * @brief Apollo Federation v2 GraphQL schema generation from PostgreSQL.
 *
 * Converts relational schemas to federated GraphQL SDL (Schema Definition
 * Language), emitting \@key, \@shareable, and \@requires directives for
 * multi-subgraph gateway setups.
 *
 * Standard: Apollo Federation v2 (GraphQL)
 */
class GraphQLFederationSupport {
public:
    // ------------------------------------------------------------------
    // Schema generator
    // ------------------------------------------------------------------
    class GraphQLSchemaGenerator {
    public:
        /**
         * @brief Generate a Federation v2-compatible GraphQL SDL for the
         *        given table schemas.
         *
         * The generator:
         *  - Maps each table to a GraphQL type with \@key(fields: "<pk>")
         *  - Emits scalar overrides for semantic types (UUID, DateTime, …)
         *  - Adds \@relationship annotations on FK columns
         *  - Wraps list fields as non-null arrays where appropriate
         *
         * @param schemas          Table schemas to convert.
         * @param service_name     Name of this subgraph service.
         * @param external_entities  Types owned by other subgraphs (\@external).
         * @return Apollo Federation v2 SDL string.
         */
        std::string generateFederatedSchema(
            const std::vector<InferenceTableSchema>& schemas,
            const std::string& service_name,
            const std::vector<std::string>& external_entities = {}
        );

        /**
         * @brief Generate a plain (non-federated) GraphQL SDL.
         * Useful for single-service setups.
         */
        std::string generatePlainSchema(
            const std::vector<InferenceTableSchema>& schemas
        );

    private:
        std::string pgTypeToGraphQL(const std::string& pg_type) const;
        std::string tableNameToTypeName(const std::string& table) const;
        std::string columnToField(const std::string& col,
                                   const std::string& pg_type,
                                   bool nullable) const;
    };
};

} // namespace importers
} // namespace themis
