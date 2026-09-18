/**
 * @file polyglot_mapper.h
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
#include <nlohmann/json.hpp>

namespace themis {
namespace importers {

class PolyglotPersistenceMapper {
public:
    enum class DataModel {
        RELATIONAL,  ///< Normalised tabular
        DOCUMENT,    ///< JSON/BSON with nesting
        GRAPH,       ///< Vertices & edges (RDF / Property Graph)
        TIMESERIES,  ///< Chronological events
        VECTORSPACE, ///< Embeddings / similarity search
        KEYVALUE     ///< Simple key-value store
    };

    /**
     * @brief Data Model To String.
     * @param[in] m Input parameter.
     * @return Return value.
     */
    static std::string dataModelToString(DataModel m);

    struct QueryPattern {
        std::string table_name;
        std::string pattern_type; ///< "point_lookup" | "range_scan" | "join" | "aggregation"
        double frequency{0.0};    ///< Relative frequency [0,1]
    };

    struct DataModelMapping {
        std::string source_table;
        DataModel recommended_model;
        double confidence_score{0.0};   ///< [0,1]
        json transformation_rules;       ///< How to convert
        std::vector<std::string> rationale;
    };

    std::vector<DataModelMapping> recommendDataModels(
        const std::vector<InferenceTableSchema>& schemas,
        const std::vector<QueryPattern>& observed_queries = {}
    );

    // ------------------------------------------------------------------
    // Model transformers
    // ------------------------------------------------------------------
    class ModelTransformer {
    public:
        /**
         * @brief Table To Document.
         * @param[in] row Input parameter.
         * @param[in] schema Input parameter.
         * @return Return value.
         */
        json tableToDocument(const json& row,
                             const InferenceTableSchema& schema);

        struct GraphNode {
            std::string id;
            std::string label;
            json properties;
        };
        struct GraphEdge {
            std::string from_id;
            std::string to_id;
            std::string relationship_type;
            json properties;
        };

        std::pair<std::vector<GraphNode>, std::vector<GraphEdge>>
        tableToGraph(const std::vector<json>& rows,
                     const InferenceTableSchema& schema);
    };

private:
    /**
     * @brief Infer Model From Schema.
     * @param[in] schema Input parameter.
     * @param[in] queries Input parameter.
     * @return Return value.
     */
    DataModel inferModelFromSchema(
        const InferenceTableSchema& schema,
        const std::vector<QueryPattern>& queries
    ) const;
};

} // namespace importers
} // namespace themis
