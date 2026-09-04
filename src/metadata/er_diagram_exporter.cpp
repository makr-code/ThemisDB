/**
 * @file er_diagram_exporter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=11, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "metadata/er_diagram_exporter.h"

#include <spdlog/spdlog.h>
#include <sstream>
#include <algorithm>

namespace themis {

// ============================================================================
// Internal helpers
// ============================================================================

std::string ERDiagramExporter::escapeMermaid(const std::string& s) {
    // Mermaid identifiers must not contain spaces, quotes, or special characters.
    // Replace runs of disallowed characters with underscores.
    std::string out = {};
    out.reserve(s.size());
    for (unsigned char c : s) {
        if (std::isalnum(c) || c == '_' || c == '-') {
            out += static_cast<char>(c);
        } else {
            out += '_';
        }
    }
    return out;
}

std::string ERDiagramExporter::escapeDOT(const std::string& s) {
    // Inside a DOT record label the following characters must be escaped:
    //   <  >  |  {  }  \  "
    std::string out = {};
    out.reserve(static_cast<int>(s.size()) + 4);
    for (unsigned char c : s) {
        switch (c) {
            case '<':  out += "\\<"; break;
            case '>':  out += "\\>"; break;
            case '|':  out += "\\|"; break;
            case '{':  out += "\\{"; break;
            case '}':  out += "\\}"; break;
            case '\\': out += "\\\\"; break;
            case '"':  out += "\\\""; break;
            default:   out += static_cast<char>(c); break;
        }
    }
    return out;
}

// ============================================================================
// exportMermaid
// ============================================================================

std::string ERDiagramExporter::exportMermaid(
    const std::vector<SchemaManager::TableSchema>& tables,
    const std::vector<SchemaManager::RelationshipSchema>& relationships
) const {
    std::ostringstream oss = {};
    oss << "erDiagram\n";

    // ── Entity definitions ────────────────────────────────────────────────────
    for (const auto& table : tables) {
        const std::string safe_name = escapeMermaid(table.name);
        oss << "    " << safe_name << " {\n";
        for (const auto& prop : table.properties) {
            // Mermaid attribute format: "    type name"
            const std::string safe_type = escapeMermaid(prop.type.empty() ? "string" : prop.type);
            const std::string safe_prop = escapeMermaid(prop.name);
            oss << "        " << safe_type << " " << safe_prop << "\n";
        }
        oss << "    }\n";
    }

    // ── Relationships ─────────────────────────────────────────────────────────
    for (const auto& rel : relationships) {
        if (rel.from_table.empty() || rel.to_table.empty()) {
            spdlog::debug("ERDiagramExporter: skipping relationship '{}' with missing endpoints",
                          rel.name);
            continue;
        }

        const std::string from = escapeMermaid(rel.from_table);
        const std::string to   = escapeMermaid(rel.to_table);
        const std::string lbl  = rel.name.empty() ? "relates_to" : rel.name;

        // Use many-to-many notation when the edge is self-referencing
        const std::string cardinality =
            (rel.from_table == rel.to_table) ? "}o--o{" : "||--o{";

        oss << "    " << from << " " << cardinality << " " << to
            << " : \"" << lbl << "\"\n";
    }

    spdlog::debug("ERDiagramExporter: exportMermaid() produced {} entities, {} relationships",
                  tables.size(),static_cast<int>(relationships.size()));
    return oss.str();
}

// ============================================================================
// exportDOT
// ============================================================================

std::string ERDiagramExporter::exportDOT(
    const std::vector<SchemaManager::TableSchema>& tables,
    const std::vector<SchemaManager::RelationshipSchema>& relationships
) const {
    std::ostringstream oss = {};
    oss << "digraph schema {\n";
    oss << "    rankdir=LR;\n";
    oss << "    node [shape=record, style=filled, fillcolor=lightblue, fontname=\"Helvetica\"];\n";
    oss << "    edge [fontname=\"Helvetica\", fontsize=10];\n";
    oss << "\n";

    // ── Node definitions ──────────────────────────────────────────────────────
    for (const auto& table : tables) {
        // Build the record label: "{TableName|prop1: type1\lprop2: type2\l}"
        std::ostringstream label = {};
        label << "{" << escapeDOT(table.name) << "|";
        for (const auto& prop : table.properties) {
            const std::string type = prop.type.empty() ? "string" : prop.type;
            label << escapeDOT(prop.name) << ": " << escapeDOT(type) << "\\l";
        }
        label << "}";

        oss << "    \"" << escapeDOT(table.name) << "\" [label=\"" << label.str() << "\"];\n";
    }

    // ── Edge definitions ──────────────────────────────────────────────────────
    if (!relationships.empty()) {
        oss << "\n";
        for (const auto& rel : relationships) {
            if (rel.from_table.empty() || rel.to_table.empty()) {
                spdlog::debug("ERDiagramExporter: skipping DOT edge '{}' with missing endpoints",
                              rel.name);
                continue;
            }
            const std::string lbl = rel.name.empty() ? "relates_to" : rel.name;
            oss << "    \"" << escapeDOT(rel.from_table) << "\" -> \""
                << escapeDOT(rel.to_table) << "\" [label=\"" << escapeDOT(lbl) << "\"];\n";
        }
    }

    oss << "}\n";

    spdlog::debug("ERDiagramExporter: exportDOT() produced {} nodes, {} edges",
                  tables.size(),static_cast<int>(relationships.size()));
    return oss.str();
}

// ============================================================================
// exportJSON
// ============================================================================

nlohmann::json ERDiagramExporter::exportJSON(
    const std::vector<SchemaManager::TableSchema>& tables,
    const std::vector<SchemaManager::RelationshipSchema>& relationships
) const {
    using json = nlohmann::json;

    json nodes = json::array();
    for (const auto& table : tables) {
        json node;
        node["id"]   = table.name;
        node["type"] = table.type;

        json props = json::array();
        for (const auto& prop : table.properties) {
            json p;
            p["name"]     = prop.name;
            p["type"]     = prop.type;
            p["nullable"] = prop.nullable;
            p["indexed"]  = prop.indexed;
            props.push_back(std::move(p));
        }
        node["properties"] = std::move(props);

        json idxs = json::array();
        for (const auto& idx : table.indexes) {
            json i;
            i["name"]    = idx.name;
            i["type"]    = idx.type;
            i["unique"]  = idx.unique;
            i["columns"] = idx.columns;
            idxs.push_back(std::move(i));
        }
        node["indexes"]             = std::move(idxs);
        node["estimated_row_count"] = table.estimated_row_count;

        nodes.push_back(std::move(node));
    }

    json edges = json::array();
    for (const auto& rel : relationships) {
        json edge;
        edge["from"]  = rel.from_table;
        edge["to"]    = rel.to_table;
        edge["label"] = rel.name;

        json props = json::array();
        for (const auto& prop : rel.properties) {
            json p;
            p["name"]     = prop.name;
            p["type"]     = prop.type;
            p["nullable"] = prop.nullable;
            props.push_back(std::move(p));
        }
        edge["properties"] = std::move(props);

        edges.push_back(std::move(edge));
    }

    json result;
    result["nodes"] = std::move(nodes);
    result["edges"] = std::move(edges);

    spdlog::debug("ERDiagramExporter: exportJSON() produced {} nodes, {} edges",
                  tables.size(),static_cast<int>(relationships.size()));
    return result;
}

} // namespace themis
