#include <gtest/gtest.h>
#include "storage/key_schema.h"
#include <set>

using namespace themis;

// ============================================================================
// KeySchema Basic Key Construction Tests
// ============================================================================

TEST(KeySchemaTest, MakeRelationalKey) {
    std::string key = KeySchema::makeRelationalKey("users", "123");
    EXPECT_EQ(key, "rel:users:123");
    
    // Test with different inputs
    key = KeySchema::makeRelationalKey("products", "abc-def");
    EXPECT_EQ(key, "rel:products:abc-def");
}

TEST(KeySchemaTest, MakeDocumentKey) {
    std::string key = KeySchema::makeDocumentKey("orders", "456");
    EXPECT_EQ(key, "doc:orders:456");
    
    // Test with different inputs
    key = KeySchema::makeDocumentKey("invoices", "inv-2024-001");
    EXPECT_EQ(key, "doc:invoices:inv-2024-001");
}

TEST(KeySchemaTest, MakeGraphNodeKey) {
    std::string key = KeySchema::makeGraphNodeKey("node123");
    EXPECT_EQ(key, "node:node123");
}

TEST(KeySchemaTest, MakeGraphEdgeKey) {
    std::string key = KeySchema::makeGraphEdgeKey("edge456");
    EXPECT_EQ(key, "edge:edge456");
}

TEST(KeySchemaTest, MakeVectorKey) {
    std::string key = KeySchema::makeVectorKey("embeddings", "vec789");
    EXPECT_EQ(key, "vec:embeddings:vec789");
}

TEST(KeySchemaTest, MakeSecondaryIndexKey) {
    std::string key = KeySchema::makeSecondaryIndexKey("users", "email", "john@example.com", "123");
    EXPECT_EQ(key, "idx:users:email:john@example.com:123");
}

TEST(KeySchemaTest, MakeGraphOutdexKey) {
    std::string key = KeySchema::makeGraphOutdexKey("node1", "edge1");
    EXPECT_EQ(key, "graph:out:node1:edge1");
}

TEST(KeySchemaTest, MakeGraphIndexKey) {
    std::string key = KeySchema::makeGraphIndexKey("node2", "edge2");
    EXPECT_EQ(key, "graph:in:node2:edge2");
}

// ============================================================================
// KeySchema Parsing Tests
// ============================================================================

TEST(KeySchemaTest, ParseKeyTypeRelational) {
    std::string key = "rel:users:123";
    auto type = KeySchema::parseKeyType(key);
    EXPECT_EQ(type, KeySchema::KeyType::RELATIONAL);
}

TEST(KeySchemaTest, ParseKeyTypeDocument) {
    std::string key = "doc:orders:456";
    auto type = KeySchema::parseKeyType(key);
    EXPECT_EQ(type, KeySchema::KeyType::DOCUMENT);
}

TEST(KeySchemaTest, ParseKeyTypeGraphNode) {
    std::string key = "node:node123";
    auto type = KeySchema::parseKeyType(key);
    EXPECT_EQ(type, KeySchema::KeyType::GRAPH_NODE);
}

TEST(KeySchemaTest, ParseKeyTypeGraphEdge) {
    std::string key = "edge:edge456";
    auto type = KeySchema::parseKeyType(key);
    EXPECT_EQ(type, KeySchema::KeyType::GRAPH_EDGE);
}

TEST(KeySchemaTest, ParseKeyTypeVector) {
    std::string key = "vec:embeddings:vec789";
    auto type = KeySchema::parseKeyType(key);
    EXPECT_EQ(type, KeySchema::KeyType::VECTOR);
}

TEST(KeySchemaTest, ParseKeyTypeSecondaryIndex) {
    std::string key = "idx:users:email:john@example.com:123";
    auto type = KeySchema::parseKeyType(key);
    EXPECT_EQ(type, KeySchema::KeyType::SECONDARY_INDEX);
}

TEST(KeySchemaTest, ParseKeyTypeGraphOutdex) {
    std::string key = "graph:out:node1:edge1";
    auto type = KeySchema::parseKeyType(key);
    EXPECT_EQ(type, KeySchema::KeyType::GRAPH_OUTDEX);
}

TEST(KeySchemaTest, ParseKeyTypeGraphIndex) {
    std::string key = "graph:in:node2:edge2";
    auto type = KeySchema::parseKeyType(key);
    EXPECT_EQ(type, KeySchema::KeyType::GRAPH_INDEX);
}

// ============================================================================
// KeySchema Ambiguity Resolution Tests (v1.5.0+)
// ============================================================================

TEST(KeySchemaTest, ParseKeyTypeDistinguishesRelationalAndDocument) {
    // New format with prefixes should be distinguishable
    std::string rel_key = "rel:users:123";
    std::string doc_key = "doc:users:123";
    
    EXPECT_EQ(KeySchema::parseKeyType(rel_key), KeySchema::KeyType::RELATIONAL);
    EXPECT_EQ(KeySchema::parseKeyType(doc_key), KeySchema::KeyType::DOCUMENT);
    EXPECT_NE(KeySchema::parseKeyType(rel_key), KeySchema::parseKeyType(doc_key));
}

TEST(KeySchemaTest, ParseKeyTypeLegacyFormat) {
    // Legacy format without prefix should default to DOCUMENT for backward compatibility
    std::string legacy_key = "users:123";
    auto type = KeySchema::parseKeyType(legacy_key);
    EXPECT_EQ(type, KeySchema::KeyType::DOCUMENT);
}

// ============================================================================
// KeySchema Primary Key Extraction Tests
// ============================================================================

TEST(KeySchemaTest, ExtractPrimaryKeyRelational) {
    std::string key = "rel:users:123";
    std::string pk = KeySchema::extractPrimaryKey(key);
    EXPECT_EQ(pk, "123");
}

TEST(KeySchemaTest, ExtractPrimaryKeyDocument) {
    std::string key = "doc:orders:order-456";
    std::string pk = KeySchema::extractPrimaryKey(key);
    EXPECT_EQ(pk, "order-456");
}

TEST(KeySchemaTest, ExtractPrimaryKeyGraphNode) {
    std::string key = "node:node123";
    std::string pk = KeySchema::extractPrimaryKey(key);
    EXPECT_EQ(pk, "node123");
}

TEST(KeySchemaTest, ExtractPrimaryKeyVector) {
    std::string key = "vec:embeddings:vec789";
    std::string pk = KeySchema::extractPrimaryKey(key);
    EXPECT_EQ(pk, "vec789");
}

TEST(KeySchemaTest, ExtractPrimaryKeySecondaryIndex) {
    std::string key = "idx:users:email:john@example.com:123";
    std::string pk = KeySchema::extractPrimaryKey(key);
    // For secondary index, the PK is the last component
    EXPECT_EQ(pk, "123");
}

TEST(KeySchemaTest, ExtractPrimaryKeyGraphOutdex) {
    std::string key = "graph:out:node1:edge1";
    std::string pk = KeySchema::extractPrimaryKey(key);
    // For graph outdex, last component is edge PK
    EXPECT_EQ(pk, "edge1");
}

TEST(KeySchemaTest, ExtractPrimaryKeyNoSeparator) {
    std::string key = "simplekey";
    std::string pk = KeySchema::extractPrimaryKey(key);
    EXPECT_EQ(pk, "simplekey");
}

// ============================================================================
// KeySchema Edge Cases and Special Characters
// ============================================================================

TEST(KeySchemaTest, KeysWithSpecialCharacters) {
    // Test keys with special characters that might appear in real data
    std::string key = KeySchema::makeRelationalKey("users", "user@example.com");
    EXPECT_EQ(key, "rel:users:user@example.com");
    EXPECT_EQ(KeySchema::extractPrimaryKey(key), "user@example.com");
}

TEST(KeySchemaTest, KeysWithColonsInValue) {
    // Test keys where the value contains colons
    std::string key = KeySchema::makeDocumentKey("logs", "2024:01:15:error");
    EXPECT_EQ(key, "doc:logs:2024:01:15:error");
    // extractPrimaryKey should return everything after the last separator
    EXPECT_EQ(KeySchema::extractPrimaryKey(key), "error");
}

TEST(KeySchemaTest, EmptyComponents) {
    // Test with empty strings
    std::string key = KeySchema::makeRelationalKey("", "123");
    EXPECT_EQ(key, "rel::123");
    
    key = KeySchema::makeRelationalKey("users", "");
    EXPECT_EQ(key, "rel:users:");
}

TEST(KeySchemaTest, RoundTripRelationalKey) {
    // Create a key, extract the PK, verify it matches
    std::string original_pk = "user-12345";
    std::string key = KeySchema::makeRelationalKey("users", original_pk);
    std::string extracted_pk = KeySchema::extractPrimaryKey(key);
    EXPECT_EQ(extracted_pk, original_pk);
}

TEST(KeySchemaTest, RoundTripDocumentKey) {
    // Create a key, extract the PK, verify it matches
    std::string original_pk = "doc-67890";
    std::string key = KeySchema::makeDocumentKey("documents", original_pk);
    std::string extracted_pk = KeySchema::extractPrimaryKey(key);
    EXPECT_EQ(extracted_pk, original_pk);
}

// ============================================================================
// KeySchema Integration Tests
// ============================================================================

TEST(KeySchemaTest, MultipleKeyTypesDistinct) {
    // Verify that different key types for the same PK are distinct
    std::string pk = "123";
    std::string rel_key = KeySchema::makeRelationalKey("data", pk);
    std::string doc_key = KeySchema::makeDocumentKey("data", pk);
    std::string vec_key = KeySchema::makeVectorKey("data", pk);
    
    // All keys should be different
    EXPECT_NE(rel_key, doc_key);
    EXPECT_NE(rel_key, vec_key);
    EXPECT_NE(doc_key, vec_key);
    
    // But all should extract the same PK
    EXPECT_EQ(KeySchema::extractPrimaryKey(rel_key), pk);
    EXPECT_EQ(KeySchema::extractPrimaryKey(doc_key), pk);
    EXPECT_EQ(KeySchema::extractPrimaryKey(vec_key), pk);
}

TEST(KeySchemaTest, AllKeyTypesUnique) {
    // Create keys for all types and ensure they're all different
    std::string pk = "test123";
    
    std::string rel = KeySchema::makeRelationalKey("tbl", pk);
    std::string doc = KeySchema::makeDocumentKey("col", pk);
    std::string node = KeySchema::makeGraphNodeKey(pk);
    std::string edge = KeySchema::makeGraphEdgeKey(pk);
    std::string vec = KeySchema::makeVectorKey("obj", pk);
    std::string idx = KeySchema::makeSecondaryIndexKey("tbl", "col", "val", pk);
    std::string outdex = KeySchema::makeGraphOutdexKey("n1", pk);
    std::string index = KeySchema::makeGraphIndexKey("n2", pk);
    
    // Collect all keys in a set to ensure uniqueness
    std::set<std::string> key_set = {rel, doc, node, edge, vec, idx, outdex, index};
    EXPECT_EQ(key_set.size(), 8) << "All key types should be unique";
}
