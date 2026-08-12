/**
 * @file edge_types.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <functional>
#include <shared_mutex>

namespace themis {

/**
 * @brief Built-in Edge Categories for ThemisDB Graph Subsystem
 * 
 * This module defines a registry of edge categories that Themis manages internally.
 * Each node can have multiple incoming and outgoing edges, and edges can be
 * categorized into different types for efficient filtering and traversal.
 * 
 * ## Design Decisions
 * 
 * ### Multi-Edge Support
 * - Each node supports unlimited incoming and outgoing edges
 * - Edges are stored in adjacency lists (outEdges_, inEdges_) indexed by node PK
 * - Multiple edges between the same pair of nodes are allowed (multi-graph)
 * 
 * ### Edge Category System
 * ThemisDB defines the following built-in edge categories:
 * 
 * 1. **STRUCTURAL** - Core relationship edges (PARENT_OF, CHILD_OF, CONTAINS)
 * 2. **REFERENCE** - Reference/link edges (REFERENCES, LINKS_TO, CITES)
 * 3. **TEMPORAL** - Time-bound relationships (VALID_DURING, SUCCEEDED_BY)
 * 4. **SEMANTIC** - Semantic/meaning relationships (IS_A, SIMILAR_TO, RELATED_TO)
 * 5. **WORKFLOW** - Process/workflow edges (TRIGGERS, DEPENDS_ON, FOLLOWS)
 * 6. **ACCESS** - Access control relationships (CAN_READ, CAN_WRITE, OWNS)
 * 7. **CUSTOM** - User-defined edge types
 * 
 * ### Best Practices
 * - Use built-in categories for common relationship patterns
 * - Edge types within categories can be user-defined strings (e.g., "FOLLOWS", "LIKES")
 * - Category metadata enables optimized traversal (e.g., skip TEMPORAL edges for non-temporal queries)
 * - Type indices provide O(E_type) lookup by edge type
 * 
 * ### Internal Management
 * - EdgeTypeRegistry validates and manages edge type registrations
 * - Categories provide semantic grouping for query optimization
 * - Hooks allow custom behavior per category (e.g., temporal validation)
 */

/**
 * @brief Built-in edge category enumeration
 * 
 * Categories group related edge types and enable category-level optimizations.
 */
enum class EdgeCategory {
    STRUCTURAL,   ///< Core structural relationships (hierarchies, containment)
    REFERENCE,    ///< Reference/linking relationships
    TEMPORAL,     ///< Time-aware relationships with validity periods
    SEMANTIC,     ///< Semantic/meaning-based relationships
    WORKFLOW,     ///< Process and workflow relationships
    ACCESS,       ///< Access control and permission relationships
    CUSTOM        ///< User-defined edge types
};

/**
 * @brief Edge type metadata
 * 
 * Stores information about a registered edge type.
 */
struct EdgeTypeInfo {
    std::string type_name;           ///< The edge type identifier (e.g., "FOLLOWS")
    EdgeCategory category;           ///< Category this type belongs to
    std::string description;         ///< Human-readable description
    bool is_bidirectional = false;   ///< If true, traversal works both directions
    bool requires_temporal = false;  ///< If true, edges must have valid_from/valid_to
    bool is_weighted = false;        ///< If true, edges should have _weight field
    std::optional<std::string> inverse_type;  ///< Inverse type for bidirectional (e.g., PARENT_OF <-> CHILD_OF)
};

/**
 * @brief Edge Type Registry
 * 
 * Central registry for edge types with built-in and custom types.
 * Thread-safe for read operations; write operations should be done at startup.
 */
class EdgeTypeRegistry {
public:
    using ValidationFunc = std::function<bool(const std::string& type, const class BaseEntity& edge)>;
    
    struct Status {
        bool ok = true;
        std::string message;
        static Status OK() { return {}; }
        static Status Error(std::string msg) { return Status{false, std::move(msg)}; }
    };

    /**
     * @brief Get singleton instance
     */
    static EdgeTypeRegistry& instance();

    /**
     * @brief Initialize with built-in edge types
     * 
     * Called automatically on first access. Registers default edge types:
     * - STRUCTURAL: PARENT_OF, CHILD_OF, CONTAINS, PART_OF
     * - REFERENCE: REFERENCES, LINKS_TO, CITES, MENTIONS
     * - TEMPORAL: VALID_DURING, PRECEDED_BY, SUCCEEDED_BY, OVERLAPS
     * - SEMANTIC: IS_A, SIMILAR_TO, RELATED_TO, SYNONYM_OF
     * - WORKFLOW: TRIGGERS, DEPENDS_ON, FOLLOWS, BLOCKS
     * - ACCESS: CAN_READ, CAN_WRITE, CAN_DELETE, OWNS, MANAGES
     */
    void initializeBuiltinTypes();

    /**
     * @brief Register a custom edge type
     * 
     * @param info Edge type information
     * @return Status indicating success or error
     */
    Status registerType(const EdgeTypeInfo& info);

    /**
     * @brief Register a custom edge type with validation callback
     * 
     * @param info Edge type information
     * @param validator Custom validation function
     * @return Status indicating success or error
     */
    Status registerType(const EdgeTypeInfo& info, ValidationFunc validator);

    /**
     * @brief Check if a type is registered
     */
    bool isRegistered(std::string_view type_name) const;

    /**
     * @brief Get type information
     * 
     * @param type_name The edge type to look up
     * @return Optional EdgeTypeInfo if found
     */
    std::optional<EdgeTypeInfo> getTypeInfo(std::string_view type_name) const;

    /**
     * @brief Get all types in a category
     * 
     * @param category The category to query
     * @return Vector of type names in that category
     */
    std::vector<std::string> getTypesByCategory(EdgeCategory category) const;

    /**
     * @brief Get category for a type
     * 
     * @param type_name The edge type to look up
     * @return Optional category if type is registered
     */
    std::optional<EdgeCategory> getCategoryForType(std::string_view type_name) const;

    /**
     * @brief Validate an edge against its registered type constraints
     * 
     * @param type_name The edge type
     * @param edge The edge entity to validate
     * @return Status indicating if edge is valid
     */
    Status validateEdge(std::string_view type_name, const class BaseEntity& edge) const;

    /**
     * @brief Get inverse type if defined
     * 
     * For bidirectional relationships, returns the inverse type.
     * E.g., getInverseType("PARENT_OF") returns "CHILD_OF"
     */
    std::optional<std::string> getInverseType(std::string_view type_name) const;

    /**
     * @brief List all registered type names
     */
    std::vector<std::string> listAllTypes() const;

    /**
     * @brief Get category name as string
     */
    static std::string categoryToString(EdgeCategory category);

    /**
     * @brief Parse category from string
     */
    static std::optional<EdgeCategory> categoryFromString(std::string_view str);

private:
    EdgeTypeRegistry();
    ~EdgeTypeRegistry() = default;
    EdgeTypeRegistry(const EdgeTypeRegistry&) = delete;
    EdgeTypeRegistry& operator=(const EdgeTypeRegistry&) = delete;

    std::unordered_map<std::string, EdgeTypeInfo> types_;
    std::unordered_map<std::string, ValidationFunc> validators_;
    std::unordered_map<EdgeCategory, std::unordered_set<std::string>> category_index_;
    bool initialized_ = false;
    mutable std::shared_mutex registry_mutex_;

    void registerBuiltinType_(const EdgeTypeInfo& info);
};

/**
 * @brief Helper to check if an edge type requires temporal validity
 */
inline bool requiresTemporalValidity(std::string_view type_name) {
    auto info = EdgeTypeRegistry::instance().getTypeInfo(type_name);
    return info.has_value() && info->requires_temporal;
}

/**
 * @brief Helper to check if an edge type is weighted
 */
inline bool isWeightedEdgeType(std::string_view type_name) {
    auto info = EdgeTypeRegistry::instance().getTypeInfo(type_name);
    return info.has_value() && info->is_weighted;
}

/**
 * @brief Helper to check if an edge type is bidirectional
 */
inline bool isBidirectionalEdgeType(std::string_view type_name) {
    auto info = EdgeTypeRegistry::instance().getTypeInfo(type_name);
    return info.has_value() && info->is_bidirectional;
}

} // namespace themis
