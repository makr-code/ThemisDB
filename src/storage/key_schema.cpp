/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            key_schema.cpp                                     ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 07:42:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     105                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

﻿#include "storage/key_schema.h"
#include <sstream>

namespace themis {

std::string KeySchema::makeRelationalKey(std::string_view table, std::string_view pk) {
    std::ostringstream oss;
    oss << "rel" << SEPARATOR << table << SEPARATOR << pk;
    return oss.str();
}

std::string KeySchema::makeDocumentKey(std::string_view collection, std::string_view pk) {
    std::ostringstream oss;
    oss << "doc" << SEPARATOR << collection << SEPARATOR << pk;
    return oss.str();
}

std::string KeySchema::makeGraphNodeKey(std::string_view pk) {
    std::ostringstream oss;
    oss << "node" << SEPARATOR << pk;
    return oss.str();
}

std::string KeySchema::makeGraphEdgeKey(std::string_view pk) {
    std::ostringstream oss;
    oss << "edge" << SEPARATOR << pk;
    return oss.str();
}

std::string KeySchema::makeVectorKey(std::string_view object_name, std::string_view pk) {
    std::ostringstream oss;
    oss << "vec" << SEPARATOR << object_name << SEPARATOR << pk;
    return oss.str();
}

std::string KeySchema::makeSecondaryIndexKey(
    std::string_view table,
    std::string_view column,
    std::string_view value,
    std::string_view pk
) {
    std::ostringstream oss;
    oss << "idx" << SEPARATOR << table << SEPARATOR << column << SEPARATOR << value << SEPARATOR << pk;
    return oss.str();
}

std::string KeySchema::makeGraphOutdexKey(std::string_view pk_start, std::string_view pk_edge) {
    std::ostringstream oss;
    oss << "graph" << SEPARATOR << "out" << SEPARATOR << pk_start << SEPARATOR << pk_edge;
    return oss.str();
}

std::string KeySchema::makeGraphIndexKey(std::string_view pk_target, std::string_view pk_edge) {
    std::ostringstream oss;
    oss << "graph" << SEPARATOR << "in" << SEPARATOR << pk_target << SEPARATOR << pk_edge;
    return oss.str();
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
