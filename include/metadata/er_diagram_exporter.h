/**
 * @file er_diagram_exporter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

#include "metadata/schema_manager.h"
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace themis {

/// ERDiagramExporter - Cross-collection Relationship Graph Export
///
/// Generates Entity-Relationship (ER) diagrams from ThemisDB schema metadata,
/// visualising cross-collection relationships (graph edges and foreign-key-style
/// links discovered by SchemaManager).
///
/// Supported output formats:
/// - Mermaid `erDiagram` – embeddable in Markdown documentation and GitHub
/// - DOT (Graphviz) – renderable with `dot -Tsvg schema.dot -o schema.svg`
/// - JSON graph – machine-readable node/edge representation for custom renderers
///
/// Architecture:
///   ERDiagramExporter
///   ├─→ SchemaManager::TableSchema (nodes)
///   └─→ SchemaManager::RelationshipSchema (edges)
///
/// Thread-Safety:
/// - All export methods are const and stateless; safe to call concurrently.
///
/// Usage:
///   auto tables        = schema_mgr.getAllTables();
///   auto relationships = schema_mgr.getAllRelationships();
///
///   ERDiagramExporter exporter;
///   std::string mermaid = exporter.exportMermaid(tables, relationships);
///   std::string dot     = exporter.exportDOT(tables, relationships);
///   auto        graph   = exporter.exportJSON(tables, relationships);
///
/// Issue: makr-code/ThemisDB#1993
class ERDiagramExporter {
public:
    // =========================================================================
    // Construction
    // =========================================================================

    ERDiagramExporter() = default;
    ~ERDiagramExporter() = default;

    ERDiagramExporter(const ERDiagramExporter&) = default;
    ERDiagramExporter& operator=(const ERDiagramExporter&) = default;
    ERDiagramExporter(ERDiagramExporter&&) noexcept = default;
    ERDiagramExporter& operator=(ERDiagramExporter&&) noexcept = default;

    // =========================================================================
    // Export API
    // =========================================================================

    /// Export as Mermaid erDiagram syntax.
    ///
    /// The output is a valid Mermaid `erDiagram` block that can be embedded in
    /// Markdown fences:
    /// ```mermaid
    /// erDiagram
    ///     users {
    ///         integer id
    ///         string  name
    ///     }
    ///     orders {
    ///         integer id
    ///         integer user_id
    ///     }
    ///     users ||--o{ orders : "has"
    /// ```
    ///
    /// Relationship cardinality notation:
    /// - graph_edge relationships use `||--o{` (one-to-many)
    /// - when from_table == to_table the edge is rendered as `}o--o{` (many-to-many)
    ///
    /// @param tables        All table/collection schemas (nodes)
    /// @param relationships All edge/relationship schemas (edges)
    /// @return Mermaid erDiagram string (UTF-8)
    std::string exportMermaid(
        const std::vector<SchemaManager::TableSchema>& tables,
        const std::vector<SchemaManager::RelationshipSchema>& relationships
    ) const;

    /// Export as Graphviz DOT language.
    ///
    /// Produces a directed graph (`digraph schema { ... }`) with record-shaped
    /// nodes listing each entity's properties and labelled directed edges for
    /// each relationship.
    ///
    /// @param tables        All table/collection schemas (nodes)
    /// @param relationships All edge/relationship schemas (edges)
    /// @return DOT language string (UTF-8)
    std::string exportDOT(
        const std::vector<SchemaManager::TableSchema>& tables,
        const std::vector<SchemaManager::RelationshipSchema>& relationships
    ) const;

    /// Export as a JSON graph (nodes + edges).
    ///
    /// Schema:
    /// ```json
    /// {
    ///   "nodes": [
    ///     { "id": "users", "type": "relational",
    ///       "properties": [ { "name": "id", "type": "integer" }, ... ] }
    ///   ],
    ///   "edges": [
    ///     { "from": "users", "to": "orders", "label": "placed",
    ///       "properties": [] }
    ///   ]
    /// }
    /// ```
    ///
    /// @param tables        All table/collection schemas (nodes)
    /// @param relationships All edge/relationship schemas (edges)
    /// @return JSON object with "nodes" and "edges" arrays
    nlohmann::json exportJSON(
        const std::vector<SchemaManager::TableSchema>& tables,
        const std::vector<SchemaManager::RelationshipSchema>& relationships
    ) const;

private:
    // =========================================================================
    // Internal helpers
    // =========================================================================

    /// Escape a string for safe inclusion inside a Mermaid entity/attribute name.
    static std::string escapeMermaid(const std::string& s);

    /// Escape a string for safe inclusion inside a DOT record label.
    static std::string escapeDOT(const std::string& s);
};

} // namespace themis
