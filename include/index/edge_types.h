/**
 * @file edge_types.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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


enum class EdgeCategory {
    STRUCTURAL,   ///< Core structural relationships (hierarchies, containment)
    REFERENCE,    ///< Reference/linking relationships
    TEMPORAL,     ///< Time-aware relationships with validity periods
    SEMANTIC,     ///< Semantic/meaning-based relationships
    WORKFLOW,     ///< Process and workflow relationships
    ACCESS,       ///< Access control and permission relationships
    CUSTOM        ///< User-defined edge types
};

struct EdgeTypeInfo {
    std::string type_name;           ///< The edge type identifier (e.g., "FOLLOWS")
    EdgeCategory category;           ///< Category this type belongs to
    std::string description;         ///< Human-readable description
    bool is_bidirectional = false;   ///< If true, traversal works both directions
    bool requires_temporal = false;  ///< If true, edges must have valid_from/valid_to
    bool is_weighted = false;        ///< If true, edges should have _weight field
    std::optional<std::string> inverse_type;  ///< Inverse type for bidirectional (e.g., PARENT_OF <-> CHILD_OF)
};

class EdgeTypeRegistry {
public:
    using ValidationFunc = std::function<bool(const std::string& type, const class BaseEntity& edge)>;
    
    struct Status {
        bool ok = true;
        std::string message;
        /**
         * @brief OK.
         * @return Return value.
         * @details Implements OK without additional internal calls.
         */
        static Status OK() { return {}; }
        /**
         * @brief Error.
         * @param[in] msg Input parameter.
         * @return Return value.
         * @details Calls: std::move().
         */
        static Status Error(std::string msg) { return Status{false, std::move(msg)}; }
    };

    /**
     * @brief Instance.
     * @return Return value.
     */
    static EdgeTypeRegistry& instance();

    /**
     * @brief Initialize Builtin Types.
     */
    void initializeBuiltinTypes();

    /**
     * @brief Register Type.
     * @param[in] info Input parameter.
     * @return Return value.
     */
    Status registerType(const EdgeTypeInfo& info);

    /**
     * @brief Register Type.
     * @param[in] info Input parameter.
     * @param[in] validator Input parameter.
     * @return Return value.
     */
    Status registerType(const EdgeTypeInfo& info, ValidationFunc validator);

    /**
     * @brief Is Registered.
     * @param[in] type_name Name of the type.
     * @return True when the operation succeeds.
     */
    bool isRegistered(std::string_view type_name) const;

    /**
     * @brief Get Type Info.
     * @param[in] type_name Name of the type.
     * @return Return value.
     */
    std::optional<EdgeTypeInfo> getTypeInfo(std::string_view type_name) const;

    /**
     * @brief Get Types By Category.
     * @param[in] category Input parameter.
     * @return Return value.
     */
    std::vector<std::string> getTypesByCategory(EdgeCategory category) const;

    /**
     * @brief Get Category For Type.
     * @param[in] type_name Name of the type.
     * @return Return value.
     */
    std::optional<EdgeCategory> getCategoryForType(std::string_view type_name) const;

    /**
     * @brief Validate Edge.
     * @param[in] type_name Name of the type.
     * @param[in] edge Input parameter.
     * @return Return value.
     */
    Status validateEdge(std::string_view type_name, const class BaseEntity& edge) const;

    /**
     * @brief Get Inverse Type.
     * @param[in] type_name Name of the type.
     * @return Return value.
     */
    std::optional<std::string> getInverseType(std::string_view type_name) const;

    /**
     * @brief List All Types.
     * @return Return value.
     */
    std::vector<std::string> listAllTypes() const;

    /**
     * @brief Category To String.
     * @param[in] category Input parameter.
     * @return Return value.
     */
    static std::string categoryToString(EdgeCategory category);

    /**
     * @brief Category From String.
     * @param[in] str Input parameter.
     * @return Return value.
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

    /**
     * @brief Register Builtin Type.
     * @param[in] info Input parameter.
     */
    void registerBuiltinType_(const EdgeTypeInfo& info);
};

/**
 * @brief Requires Temporal Validity.
 * @param[in] type_name Name of the type.
 * @return True when the operation succeeds.
 * @details Calls: EdgeTypeRegistry::instance(), getTypeInfo(), has_value().
 */
inline bool requiresTemporalValidity(std::string_view type_name) {
    auto info = EdgeTypeRegistry::instance().getTypeInfo(type_name);
    return info.has_value() && info->requires_temporal;
}

/**
 * @brief Is Weighted Edge Type.
 * @param[in] type_name Name of the type.
 * @return True when the operation succeeds.
 * @details Calls: EdgeTypeRegistry::instance(), getTypeInfo(), has_value().
 */
inline bool isWeightedEdgeType(std::string_view type_name) {
    auto info = EdgeTypeRegistry::instance().getTypeInfo(type_name);
    return info.has_value() && info->is_weighted;
}

/**
 * @brief Is Bidirectional Edge Type.
 * @param[in] type_name Name of the type.
 * @return True when the operation succeeds.
 * @details Calls: EdgeTypeRegistry::instance(), getTypeInfo(), has_value().
 */
inline bool isBidirectionalEdgeType(std::string_view type_name) {
    auto info = EdgeTypeRegistry::instance().getTypeInfo(type_name);
    return info.has_value() && info->is_bidirectional;
}

} // namespace themis
