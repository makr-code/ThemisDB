/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            polyglot_mapper.h                                  ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-04-15 05:34:40                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     120                                            ║
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
