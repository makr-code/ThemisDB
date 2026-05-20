/*
 * ThemisDB | File: key_schema.cpp | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=14, M=12, L=0
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "storage/key_schema.h"

namespace themis {

std::string KeySchema::makeRelationalKey(std::string_view table, std::string_view pk) {
    std::string key;
    key.reserve(4 + table.size() + pk.size());
    key += "rel";
    key += SEPARATOR;
    key += table;
    key += SEPARATOR;
    key += pk;
    return key;
}

std::string KeySchema::makeDocumentKey(std::string_view collection, std::string_view pk) {
    std::string key;
    key.reserve(4 + collection.size() + pk.size());
    key += "doc";
    key += SEPARATOR;
    key += collection;
    key += SEPARATOR;
    key += pk;
    return key;
}

std::string KeySchema::makeGraphNodeKey(std::string_view pk) {
    std::string key;
    key.reserve(5 + pk.size());
    key += "node";
    key += SEPARATOR;
    key += pk;
    return key;
}

std::string KeySchema::makeGraphEdgeKey(std::string_view pk) {
    std::string key;
    key.reserve(5 + pk.size());
    key += "edge";
    key += SEPARATOR;
    key += pk;
    return key;
}

std::string KeySchema::makeVectorKey(std::string_view object_name, std::string_view pk) {
    std::string key;
    key.reserve(4 + object_name.size() + pk.size());
    key += "vec";
    key += SEPARATOR;
    key += object_name;
    key += SEPARATOR;
    key += pk;
    return key;
}

std::string KeySchema::makeSecondaryIndexKey(
    std::string_view table,
    std::string_view column,
    std::string_view value,
    std::string_view pk
) {
    std::string key;
    key.reserve(5 + table.size() + column.size() + value.size() + pk.size());
    key += "idx";
    key += SEPARATOR;
    key += table;
    key += SEPARATOR;
    key += column;
    key += SEPARATOR;
    key += value;
    key += SEPARATOR;
    key += pk;
    return key;
}

std::string KeySchema::makeGraphOutdexKey(std::string_view pk_start, std::string_view pk_edge) {
    std::string key;
    key.reserve(10 + pk_start.size() + pk_edge.size());
    key += "graph";
    key += SEPARATOR;
    key += "out";
    key += SEPARATOR;
    key += pk_start;
    key += SEPARATOR;
    key += pk_edge;
    return key;
}

std::string KeySchema::makeGraphIndexKey(std::string_view pk_target, std::string_view pk_edge) {
    std::string key;
    key.reserve(9 + pk_target.size() + pk_edge.size());
    key += "graph";
    key += SEPARATOR;
    key += "in";
    key += SEPARATOR;
    key += pk_target;
    key += SEPARATOR;
    key += pk_edge;
    return key;
}

KeySchema::KeyType KeySchema::parseKeyType(std::string_view key) {
    // Check for specific prefixed key types
    if (key.starts_with("idx:")) return KeyType::SECONDARY_INDEX;
    if (key.starts_with("graph:out:")) return KeyType::GRAPH_OUTDEX;
    if (key.starts_with("graph:in:")) return KeyType::GRAPH_INDEX;
    if (key.starts_with("node:")) return KeyType::GRAPH_NODE;
    if (key.starts_with("edge:")) return KeyType::GRAPH_EDGE;
    if (key.starts_with("rel:")) return KeyType::RELATIONAL;
    if (key.starts_with("doc:")) return KeyType::DOCUMENT;
    if (key.starts_with("vec:")) return KeyType::VECTOR;
    
    // Fallback for legacy keys without prefixes
    // Assume DOCUMENT for backward compatibility (was more common in early versions)
    return KeyType::DOCUMENT;
}

std::string KeySchema::extractPrimaryKey(std::string_view key) {
    // For keys with prefixes (rel:, doc:, vec:, node:, edge:, idx:, graph:),
    // the PK is always the last component after the final separator
    auto last_sep = key.rfind(SEPARATOR);
    if (last_sep != std::string_view::npos) {
        return std::string(key.substr(last_sep + 1));
    }
    // If no separator, return the entire key (edge case/legacy)
    return std::string(key);
}

} // namespace themis
