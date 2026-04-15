/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            key_schema.h                                       ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:05:42                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     114                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace themis {

/// Key schema definitions for multi-model storage
/// All data models (relational, document, graph, vector) map to key-value pairs
/// 
/// Key Format (v1.5.0+):
/// - RELATIONAL:      rel:table_name:pk_value
/// - DOCUMENT:        doc:collection_name:pk_value  
/// - GRAPH_NODE:      node:pk_value
/// - GRAPH_EDGE:      edge:pk_value
/// - VECTOR:          vec:object_name:pk_value
/// - SECONDARY_INDEX: idx:table:column:value:pk
/// - GRAPH_OUTDEX:    graph:out:pk_start:pk_edge
/// - GRAPH_INDEX:     graph:in:pk_target:pk_edge
///
/// IMPORTANT: All key components (table_name, collection_name, object_name, column,
/// value, pk_value, pk_start, pk_edge, pk_target) MUST NOT contain the separator
/// character ':' (see SEPARATOR below). Keys containing ':' in any component are
/// not supported and will result in incorrect parsing. Applications must validate
/// or sanitize input to ensure no ':' characters appear in key components.
///
/// Legacy Format (pre-1.5.0, deprecated):
/// - table_name:pk_value or collection_name:pk_value (ambiguous, assumed DOCUMENT)
///   The same restriction on ':' in components applies to legacy keys.
class KeySchema {
public:
    /// Key types for different data models
    enum class KeyType : uint8_t {
        RELATIONAL,   // table_name:pk_value
        DOCUMENT,     // collection_name:pk_value
        GRAPH_NODE,   // node:pk_value
        GRAPH_EDGE,   // edge:pk_value
        VECTOR,       // object_name:pk_value
        
        // Index keys
        SECONDARY_INDEX,  // idx:table:column:value:pk
        GRAPH_OUTDEX,     // graph:out:pk_start:pk_edge
        GRAPH_INDEX,      // graph:in:pk_target:pk_edge
    };

    /// Construct key for relational table row
    static std::string makeRelationalKey(std::string_view table, std::string_view pk);
    
    /// Construct key for document
    static std::string makeDocumentKey(std::string_view collection, std::string_view pk);
    
    /// Construct key for graph node
    static std::string makeGraphNodeKey(std::string_view pk);
    
    /// Construct key for graph edge
    static std::string makeGraphEdgeKey(std::string_view pk);
    
    /// Construct key for vector object
    static std::string makeVectorKey(std::string_view object_name, std::string_view pk);
    
    /// Construct key for secondary index entry
    static std::string makeSecondaryIndexKey(
        std::string_view table,
        std::string_view column,
        std::string_view value,
        std::string_view pk
    );
    
    /// Construct key for graph outdex (outgoing edges)
    static std::string makeGraphOutdexKey(
        std::string_view pk_start,
        std::string_view pk_edge
    );
    
    /// Construct key for graph index (incoming edges)
    static std::string makeGraphIndexKey(
        std::string_view pk_target,
        std::string_view pk_edge
    );
    
    /// Parse key type from key string
    static KeyType parseKeyType(std::string_view key);
    
    /// Extract primary key from any key type
    static std::string extractPrimaryKey(std::string_view key);
    
private:
    static constexpr char SEPARATOR = ':';
};

} // namespace themis
