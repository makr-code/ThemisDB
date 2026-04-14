/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            graphql_federation.h                               ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-04-13 20:22:14                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     87                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 9efa3acd76  2026-03-11  feat(importers): add PostgreSQL Importer v2.1+ with 12 ne... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
 * Language), emitting @key, @shareable, and @requires directives for
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
         *  - Maps each table to a GraphQL type with @key(fields: "<pk>")
         *  - Emits scalar overrides for semantic types (UUID, DateTime, …)
         *  - Adds @relationship annotations on FK columns
         *  - Wraps list fields as non-null arrays where appropriate
         *
         * @param schemas          Table schemas to convert.
         * @param service_name     Name of this subgraph service.
         * @param external_entities  Types owned by other subgraphs (@external).
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
