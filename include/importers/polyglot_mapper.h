/**
 * @file polyglot_mapper.h
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
#include <nlohmann/json.hpp>

namespace themis {
namespace importers {

/**
 * @brief Polyglot Persistence pattern recommender.
 *
 * Analyses relational schemas and suggests the optimal data model for each
 * table (Relational, Document, Graph, TimeSeries, VectorSpace, KeyValue).
 *
 * References:
 *   - Marcus et al. (2016) "Polyglot Persistence in Enterprise Applications"
 *   - CAP Theorem & PACELC Framework
 */
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

    /**
     * @brief Analyse schemas and observed query patterns to recommend the
     *        most appropriate data model for each table.
     */
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
         * @brief Flatten a relational row into a nested JSON document.
         * Nested fields are created for FK-referenced columns.
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

        /**
         * @brief Convert a set of rows to graph nodes and edges.
         * Each FK column produces an edge; non-FK columns become node properties.
         */
        std::pair<std::vector<GraphNode>, std::vector<GraphEdge>>
        tableToGraph(const std::vector<json>& rows,
                     const InferenceTableSchema& schema);
    };

private:
    DataModel inferModelFromSchema(
        const InferenceTableSchema& schema,
        const std::vector<QueryPattern>& queries
    ) const;
};

} // namespace importers
} // namespace themis
