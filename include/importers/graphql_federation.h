/**
 * @file graphql_federation.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "importers/schema_inference.h"
#include <string>
#include <vector>

namespace themis {
namespace importers {

class GraphQLFederationSupport {
public:
    // ------------------------------------------------------------------
    // Schema generator
    // ------------------------------------------------------------------
    class GraphQLSchemaGenerator {
    public:
        std::string generateFederatedSchema(
            const std::vector<InferenceTableSchema>& schemas,
            const std::string& service_name,
            const std::vector<std::string>& external_entities = {}
        );

        /**
         * @brief Generate Plain Schema.
         * @param[in] schemas Input parameter.
         * @return Return value.
         */
        std::string generatePlainSchema(
            const std::vector<InferenceTableSchema>& schemas
        );

    private:
        /**
         * @brief Pg Type To Graph QL.
         * @param[in] pg_type Input parameter.
         * @return Return value.
         */
        std::string pgTypeToGraphQL(const std::string& pg_type) const;
        /**
         * @brief Table Name To Type Name.
         * @param[in] table Input parameter.
         * @return Return value.
         */
        std::string tableNameToTypeName(const std::string& table) const;
        /**
         * @brief Column To Field.
         * @param[in] col Input parameter.
         * @param[in] pg_type Input parameter.
         * @param[in] nullable Input parameter.
         * @return Return value.
         */
        std::string columnToField(const std::string& col,
                                   const std::string& pg_type,
                                   bool nullable) const;
    };
};

} // namespace importers
} // namespace themis
