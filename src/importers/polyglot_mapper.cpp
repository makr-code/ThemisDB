/**
 * @file polyglot_mapper.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "importers/polyglot_mapper.h"
#include <algorithm>
#include <set>

namespace themis {
namespace importers {

// ---------------------------------------------------------------------------
// dataModelToString
// ---------------------------------------------------------------------------

std::string PolyglotPersistenceMapper::dataModelToString(DataModel m) {
    switch (m) {
        case DataModel::RELATIONAL:  return "RELATIONAL";
        case DataModel::DOCUMENT:    return "DOCUMENT";
        case DataModel::GRAPH:       return "GRAPH";
        case DataModel::TIMESERIES:  return "TIMESERIES";
        case DataModel::VECTORSPACE: return "VECTORSPACE";
        case DataModel::KEYVALUE:    return "KEYVALUE";
        default:                     return "UNKNOWN";
    }
}

// ---------------------------------------------------------------------------
// Private inference logic
// ---------------------------------------------------------------------------

PolyglotPersistenceMapper::DataModel
PolyglotPersistenceMapper::inferModelFromSchema(
    const InferenceTableSchema& schema,
    const std::vector<QueryPattern>& queries) const
{
    // Time-series: table has timestamp-like columns and no FKs
    static const std::set<std::string> ts_hints{
        "timestamp", "created_at", "event_time", "recorded_at", "measured_at"
    };
    for (const auto& col : schema.columns) {
        if (ts_hints.count(col) && schema.foreign_keys.empty()) {
            return DataModel::TIMESERIES;
        }
    }

    // Graph: many self-referential or bidirectional FKs
    size_t self_fk = 0;
    for (const auto& fk : schema.foreign_keys) {
        std::string ref_table = fk.second.substr(0, fk.second.find('.'));
        if (ref_table == schema.name) {
          ++self_fk;
        }
    }
    if (self_fk >= 2 || static_cast<int>(schema.foreign_keys.size()) >= 3) {
        return DataModel::GRAPH;
    }

    // Key-value: exactly one PK and one or two value columns
    if (static_cast<int>(schema.primary_keys.size()) == 1 &&
        schema.columns.size() <= 3 &&
        schema.foreign_keys.empty()) {
        return DataModel::KEYVALUE;
    }

    // Document: has JSON/JSONB columns or many nullable columns
    for (const auto& [col, type] : schema.column_types) {
        if (type.find("json") != std::string::npos) {
            return DataModel::DOCUMENT;
        }
    }

    // Query-driven: prefer DOCUMENT for aggregation-heavy tables
    for (const auto& q : queries) {
        if (q.table_name == schema.name && q.pattern_type == "aggregation" &&
            q.frequency > 0.5) {
            return DataModel::DOCUMENT;
        }
    }

    // Default: normalised relational
    return DataModel::RELATIONAL;
}

// ---------------------------------------------------------------------------
// recommendDataModels
// ---------------------------------------------------------------------------

std::vector<PolyglotPersistenceMapper::DataModelMapping>
PolyglotPersistenceMapper::recommendDataModels(
    const std::vector<InferenceTableSchema>& schemas,
    const std::vector<QueryPattern>& observed_queries)
{
    std::vector<DataModelMapping> result = {};

    result.reserve(schemas.size());

    for (const auto& schema : schemas) {
        DataModelMapping mapping;
        mapping.source_table       = schema.name;
        mapping.recommended_model  = inferModelFromSchema(schema, observed_queries);
        mapping.confidence_score   = 0.75; // default heuristic confidence

        mapping.rationale.push_back(
            "Recommended model: " + dataModelToString(mapping.recommended_model));

        mapping.transformation_rules = json{
            {"source",  schema.name},
            {"target_model", dataModelToString(mapping.recommended_model)},
            {"pk_columns", schema.primary_keys},
            {"fk_count",static_cast<int>(schema.foreign_keys.size())}
        };

        result.push_back(std::move(mapping));
    }

    return result;
}

// ---------------------------------------------------------------------------
// ModelTransformer
// ---------------------------------------------------------------------------

json PolyglotPersistenceMapper::ModelTransformer::tableToDocument(
    const json& row,
    const InferenceTableSchema& schema)
{
    json doc = row; // start with flat copy

    // Promote FK columns into a nested object
    for (const auto& fk : schema.foreign_keys) {
        const std::string& local_col = fk.first;
        std::string ref_table = fk.second.substr(0, fk.second.find('.'));

        if (doc.contains(local_col)) {
            // Nest the FK value under the referenced table name
            doc[ref_table] = json{{"_ref", doc.at(local_col)}};
        }
    }

    doc["_schema"] = schema.name;
    return doc;
}

std::pair<std::vector<PolyglotPersistenceMapper::ModelTransformer::GraphNode>,
          std::vector<PolyglotPersistenceMapper::ModelTransformer::GraphEdge>>
PolyglotPersistenceMapper::ModelTransformer::tableToGraph(
    const std::vector<json>& rows,
    const InferenceTableSchema& schema)
{
    std::vector<GraphNode> nodes;
    std::vector<GraphEdge> edges;

    // Determine which columns are FK columns
    std::set<std::string> fk_cols = {};

    for (const auto& fk : schema.foreign_keys) {
      fk_cols.insert(fk.first);
    }

    for (const auto& row : rows) {
        // Build node id from primary key
        std::string node_id = {};
        for (const auto& pk : schema.primary_keys) {
            if (row.contains(pk)) {
                node_id += row.at(pk).dump();
            }
        }
        if (node_id.empty()) {
          node_id = row.dump().substr(0, 16);
        }

        GraphNode node;
        node.id    = node_id;
        node.label = schema.name;

        // Non-FK columns become node properties
        for (auto it = row.begin(); it != row.end(); ++it) {
            if (!fk_cols.count(it.key())) {
                node.properties[it.key()] = it.value();
            }
        }
        nodes.push_back(std::move(node));

        // FK columns become edges
        for (const auto& fk : schema.foreign_keys) {
            if (row.contains(fk.first) && !row.at(fk.first).is_null()) {
                GraphEdge edge;
                edge.from_id           = node_id;
                edge.to_id             = row.at(fk.first).dump();
                edge.relationship_type = "FK_" + fk.second;
                edges.push_back(std::move(edge));
            }
        }
    }

    return {std::move(nodes), std::move(edges)};
}

} // namespace importers
} // namespace themis
