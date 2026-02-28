// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

// Tests: ERDiagramExporter – Mermaid, DOT, and JSON graph export
// All tests are pure-logic unit tests; no database access is required.

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

#include "metadata/er_diagram_exporter.h"
#include "metadata/schema_manager.h"

using namespace themis;
using json = nlohmann::json;

// ============================================================================
// Helpers
// ============================================================================

static SchemaManager::TableSchema makeTable(
    const std::string& name,
    const std::string& type = "relational"
) {
    SchemaManager::TableSchema t;
    t.name  = name;
    t.type  = type;
    t.estimated_row_count = 42;

    SchemaManager::PropertyInfo id_prop;
    id_prop.name     = "id";
    id_prop.type     = "integer";
    id_prop.nullable = false;
    id_prop.indexed  = true;
    t.properties.push_back(id_prop);

    SchemaManager::PropertyInfo name_prop;
    name_prop.name     = "name";
    name_prop.type     = "string";
    name_prop.nullable = true;
    t.properties.push_back(name_prop);

    return t;
}

static SchemaManager::RelationshipSchema makeRelationship(
    const std::string& edge_name,
    const std::string& from_table,
    const std::string& to_table
) {
    SchemaManager::RelationshipSchema rel;
    rel.name       = edge_name;
    rel.from_table = from_table;
    rel.to_table   = to_table;
    return rel;
}

// ============================================================================
// Fixture
// ============================================================================

class ERDiagramExporterTest : public ::testing::Test {
protected:
    ERDiagramExporter exporter;

    std::vector<SchemaManager::TableSchema> two_tables() {
        return { makeTable("users"), makeTable("orders") };
    }

    std::vector<SchemaManager::RelationshipSchema> one_rel() {
        return { makeRelationship("placed", "users", "orders") };
    }
};

// ============================================================================
// exportMermaid tests
// ============================================================================

TEST_F(ERDiagramExporterTest, MermaidEmptyProducesHeader) {
    const std::string out = exporter.exportMermaid({}, {});
    EXPECT_NE(out.find("erDiagram"), std::string::npos)
        << "Output must start with 'erDiagram'";
}

TEST_F(ERDiagramExporterTest, MermaidContainsEntityNames) {
    const std::string out = exporter.exportMermaid(two_tables(), {});
    EXPECT_NE(out.find("users"), std::string::npos);
    EXPECT_NE(out.find("orders"), std::string::npos);
}

TEST_F(ERDiagramExporterTest, MermaidContainsPropertyTypesAndNames) {
    const std::string out = exporter.exportMermaid(two_tables(), {});
    // Should list "integer id" and "string name" inside entity blocks
    EXPECT_NE(out.find("integer"), std::string::npos);
    EXPECT_NE(out.find("id"), std::string::npos);
    EXPECT_NE(out.find("string"), std::string::npos);
    EXPECT_NE(out.find("name"), std::string::npos);
}

TEST_F(ERDiagramExporterTest, MermaidContainsRelationship) {
    const std::string out = exporter.exportMermaid(two_tables(), one_rel());
    // Relationship line must reference both tables and the edge label
    EXPECT_NE(out.find("users"), std::string::npos);
    EXPECT_NE(out.find("orders"), std::string::npos);
    EXPECT_NE(out.find("placed"), std::string::npos);
}

TEST_F(ERDiagramExporterTest, MermaidRelationshipCardinality) {
    const std::string out = exporter.exportMermaid(two_tables(), one_rel());
    // Non-self-referencing edge must use ||--o{ notation
    EXPECT_NE(out.find("||--o{"), std::string::npos);
}

TEST_F(ERDiagramExporterTest, MermaidSelfReferenceCardinalityManyToMany) {
    auto rels = std::vector<SchemaManager::RelationshipSchema>{
        makeRelationship("links_to", "users", "users")
    };
    const std::string out = exporter.exportMermaid(two_tables(), rels);
    EXPECT_NE(out.find("}o--o{"), std::string::npos)
        << "Self-referencing edge should use }o--o{ cardinality";
}

TEST_F(ERDiagramExporterTest, MermaidSkipsRelationshipWithMissingEndpoints) {
    SchemaManager::RelationshipSchema bad_rel;
    bad_rel.name       = "broken";
    bad_rel.from_table = "";   // missing
    bad_rel.to_table   = "orders";

    const std::string out = exporter.exportMermaid(two_tables(), {bad_rel});
    // "broken" should not appear as a relationship line
    EXPECT_EQ(out.find("broken"), std::string::npos);
}

TEST_F(ERDiagramExporterTest, MermaidEscapesSpecialCharactersInName) {
    auto tables = std::vector<SchemaManager::TableSchema>{ makeTable("my-table.v2") };
    const std::string out = exporter.exportMermaid(tables, {});
    // Dots and hyphens may be replaced; ensure no crash and output is produced
    EXPECT_FALSE(out.empty());
    EXPECT_NE(out.find("erDiagram"), std::string::npos);
}

// ============================================================================
// exportDOT tests
// ============================================================================

TEST_F(ERDiagramExporterTest, DOTContainsDigraphKeyword) {
    const std::string out = exporter.exportDOT(two_tables(), one_rel());
    EXPECT_NE(out.find("digraph"), std::string::npos);
}

TEST_F(ERDiagramExporterTest, DOTContainsNodeDefinitions) {
    const std::string out = exporter.exportDOT(two_tables(), {});
    EXPECT_NE(out.find("users"), std::string::npos);
    EXPECT_NE(out.find("orders"), std::string::npos);
}

TEST_F(ERDiagramExporterTest, DOTContainsEdgeArrow) {
    const std::string out = exporter.exportDOT(two_tables(), one_rel());
    // Edge must use DOT arrow syntax: "users" -> "orders"
    EXPECT_NE(out.find("->"), std::string::npos);
    EXPECT_NE(out.find("placed"), std::string::npos);
}

TEST_F(ERDiagramExporterTest, DOTRecordLabelContainsProperties) {
    const std::string out = exporter.exportDOT(two_tables(), {});
    EXPECT_NE(out.find("integer"), std::string::npos);
    EXPECT_NE(out.find("string"), std::string::npos);
}

TEST_F(ERDiagramExporterTest, DOTEmptyTablesProducesValidGraph) {
    const std::string out = exporter.exportDOT({}, {});
    EXPECT_NE(out.find("digraph schema"), std::string::npos);
    EXPECT_NE(out.find('}'), std::string::npos);
}

TEST_F(ERDiagramExporterTest, DOTSkipsEdgeWithMissingEndpoints) {
    SchemaManager::RelationshipSchema bad_rel;
    bad_rel.name       = "ghost";
    bad_rel.from_table = "users";
    bad_rel.to_table   = "";  // missing

    const std::string out = exporter.exportDOT(two_tables(), {bad_rel});
    // "ghost" should not appear as an edge label
    EXPECT_EQ(out.find("ghost"), std::string::npos);
}

// ============================================================================
// exportJSON tests
// ============================================================================

TEST_F(ERDiagramExporterTest, JSONContainsNodesAndEdgesKeys) {
    auto result = exporter.exportJSON(two_tables(), one_rel());
    ASSERT_TRUE(result.contains("nodes")) << "'nodes' key must be present";
    ASSERT_TRUE(result.contains("edges")) << "'edges' key must be present";
}

TEST_F(ERDiagramExporterTest, JSONNodeCount) {
    auto result = exporter.exportJSON(two_tables(), {});
    EXPECT_EQ(result["nodes"].size(), 2u);
    EXPECT_EQ(result["edges"].size(), 0u);
}

TEST_F(ERDiagramExporterTest, JSONEdgeCount) {
    auto result = exporter.exportJSON(two_tables(), one_rel());
    EXPECT_EQ(result["edges"].size(), 1u);
}

TEST_F(ERDiagramExporterTest, JSONNodeHasExpectedFields) {
    auto result = exporter.exportJSON({ makeTable("users") }, {});
    const auto& node = result["nodes"][0];
    EXPECT_EQ(node["id"].get<std::string>(), "users");
    EXPECT_EQ(node["type"].get<std::string>(), "relational");
    ASSERT_TRUE(node.contains("properties"));
    EXPECT_EQ(node["properties"].size(), 2u);  // id + name
}

TEST_F(ERDiagramExporterTest, JSONNodePropertyFields) {
    auto result = exporter.exportJSON({ makeTable("users") }, {});
    const auto& props = result["nodes"][0]["properties"];
    ASSERT_GE(props.size(), 1u);
    EXPECT_TRUE(props[0].contains("name"));
    EXPECT_TRUE(props[0].contains("type"));
    EXPECT_TRUE(props[0].contains("nullable"));
    EXPECT_TRUE(props[0].contains("indexed"));
}

TEST_F(ERDiagramExporterTest, JSONEdgeHasExpectedFields) {
    auto result = exporter.exportJSON(two_tables(), one_rel());
    const auto& edge = result["edges"][0];
    EXPECT_EQ(edge["from"].get<std::string>(), "users");
    EXPECT_EQ(edge["to"].get<std::string>(), "orders");
    EXPECT_EQ(edge["label"].get<std::string>(), "placed");
    ASSERT_TRUE(edge.contains("properties"));
}

TEST_F(ERDiagramExporterTest, JSONGraphNodeHasIndexes) {
    auto tbl = makeTable("users");
    SchemaManager::IndexInfo idx;
    idx.name    = "id_idx";
    idx.type    = "regular";
    idx.unique  = true;
    idx.columns = {"id"};
    tbl.indexes.push_back(idx);

    auto result = exporter.exportJSON({ tbl }, {});
    const auto& node = result["nodes"][0];
    ASSERT_TRUE(node.contains("indexes"));
    EXPECT_EQ(node["indexes"].size(), 1u);
    EXPECT_EQ(node["indexes"][0]["name"].get<std::string>(), "id_idx");
}

TEST_F(ERDiagramExporterTest, JSONGraphNodeEstimatedRowCount) {
    auto result = exporter.exportJSON({ makeTable("users") }, {});
    EXPECT_TRUE(result["nodes"][0].contains("estimated_row_count"));
    EXPECT_EQ(result["nodes"][0]["estimated_row_count"].get<size_t>(), 42u);
}

TEST_F(ERDiagramExporterTest, JSONEmptyInputs) {
    auto result = exporter.exportJSON({}, {});
    EXPECT_EQ(result["nodes"].size(), 0u);
    EXPECT_EQ(result["edges"].size(), 0u);
}

// ============================================================================
// Round-trip / consistency tests
// ============================================================================

TEST_F(ERDiagramExporterTest, MultipleRelationshipsAllExported) {
    auto rels = std::vector<SchemaManager::RelationshipSchema>{
        makeRelationship("placed",   "users",  "orders"),
        makeRelationship("contains", "orders", "products"),
    };
    auto tbls = std::vector<SchemaManager::TableSchema>{
        makeTable("users"),
        makeTable("orders"),
        makeTable("products"),
    };

    const std::string mermaid = exporter.exportMermaid(tbls, rels);
    EXPECT_NE(mermaid.find("placed"),   std::string::npos);
    EXPECT_NE(mermaid.find("contains"), std::string::npos);

    const std::string dot = exporter.exportDOT(tbls, rels);
    EXPECT_NE(dot.find("placed"),   std::string::npos);
    EXPECT_NE(dot.find("contains"), std::string::npos);

    auto json_graph = exporter.exportJSON(tbls, rels);
    EXPECT_EQ(json_graph["edges"].size(), 2u);
    EXPECT_EQ(json_graph["nodes"].size(), 3u);
}

TEST_F(ERDiagramExporterTest, GraphEdgeTableTypeIsHandled) {
    auto edge_table = makeTable("follows", "graph_edge");
    SchemaManager::RelationshipSchema rel;
    rel.name       = "follows";
    rel.from_table = "users";
    rel.to_table   = "users";

    const std::string mermaid = exporter.exportMermaid({ edge_table }, { rel });
    // Self-referencing relationship from a graph_edge table type
    EXPECT_NE(mermaid.find("follows"), std::string::npos);
    EXPECT_NE(mermaid.find("}o--o{"), std::string::npos);
}
