/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            edge_types.cpp                                     ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:08:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     443                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Edge Type Registry Implementation

#include "index/edge_types.h"
#include "storage/base_entity.h"

#include <mutex>

namespace themis {

namespace {
    std::once_flag init_flag;
}

EdgeTypeRegistry& EdgeTypeRegistry::instance() {
    static EdgeTypeRegistry registry;
    std::call_once(init_flag, &EdgeTypeRegistry::initializeBuiltinTypes, &registry);
    return registry;
}

EdgeTypeRegistry::EdgeTypeRegistry() = default;

void EdgeTypeRegistry::initializeBuiltinTypes() {
    if (initialized_) return;

    // ===== STRUCTURAL Category =====
    // Core hierarchical and containment relationships
    registerBuiltinType_({
        .type_name = "PARENT_OF",
        .category = EdgeCategory::STRUCTURAL,
        .description = "Hierarchical parent relationship",
        .is_bidirectional = true,
        .requires_temporal = false,
        .is_weighted = false,
        .inverse_type = "CHILD_OF"
    });
    registerBuiltinType_({
        .type_name = "CHILD_OF",
        .category = EdgeCategory::STRUCTURAL,
        .description = "Hierarchical child relationship",
        .is_bidirectional = true,
        .requires_temporal = false,
        .is_weighted = false,
        .inverse_type = "PARENT_OF"
    });
    registerBuiltinType_({
        .type_name = "CONTAINS",
        .category = EdgeCategory::STRUCTURAL,
        .description = "Containment relationship",
        .is_bidirectional = true,
        .requires_temporal = false,
        .is_weighted = false,
        .inverse_type = "PART_OF"
    });
    registerBuiltinType_({
        .type_name = "PART_OF",
        .category = EdgeCategory::STRUCTURAL,
        .description = "Part-of containment relationship",
        .is_bidirectional = true,
        .requires_temporal = false,
        .is_weighted = false,
        .inverse_type = "CONTAINS"
    });

    // ===== REFERENCE Category =====
    // Linking and citation relationships
    registerBuiltinType_({
        .type_name = "REFERENCES",
        .category = EdgeCategory::REFERENCE,
        .description = "General reference relationship",
        .is_bidirectional = false,
        .requires_temporal = false,
        .is_weighted = false,
        .inverse_type = std::nullopt
    });
    registerBuiltinType_({
        .type_name = "LINKS_TO",
        .category = EdgeCategory::REFERENCE,
        .description = "Hyperlink-style relationship",
        .is_bidirectional = false,
        .requires_temporal = false,
        .is_weighted = false,
        .inverse_type = std::nullopt
    });
    registerBuiltinType_({
        .type_name = "CITES",
        .category = EdgeCategory::REFERENCE,
        .description = "Citation relationship",
        .is_bidirectional = false,
        .requires_temporal = false,
        .is_weighted = false,
        .inverse_type = std::nullopt
    });
    registerBuiltinType_({
        .type_name = "MENTIONS",
        .category = EdgeCategory::REFERENCE,
        .description = "Mention/reference in content",
        .is_bidirectional = false,
        .requires_temporal = false,
        .is_weighted = false,
        .inverse_type = std::nullopt
    });

    // ===== TEMPORAL Category =====
    // Time-aware relationships
    registerBuiltinType_({
        .type_name = "VALID_DURING",
        .category = EdgeCategory::TEMPORAL,
        .description = "Time-bounded validity",
        .is_bidirectional = false,
        .requires_temporal = true,
        .is_weighted = false,
        .inverse_type = std::nullopt
    });
    registerBuiltinType_({
        .type_name = "PRECEDED_BY",
        .category = EdgeCategory::TEMPORAL,
        .description = "Temporal predecessor",
        .is_bidirectional = true,
        .requires_temporal = true,
        .is_weighted = false,
        .inverse_type = "SUCCEEDED_BY"
    });
    registerBuiltinType_({
        .type_name = "SUCCEEDED_BY",
        .category = EdgeCategory::TEMPORAL,
        .description = "Temporal successor",
        .is_bidirectional = true,
        .requires_temporal = true,
        .is_weighted = false,
        .inverse_type = "PRECEDED_BY"
    });
    registerBuiltinType_({
        .type_name = "OVERLAPS",
        .category = EdgeCategory::TEMPORAL,
        .description = "Temporal overlap",
        .is_bidirectional = true,
        .requires_temporal = true,
        .is_weighted = false,
        .inverse_type = "OVERLAPS"
    });

    // ===== SEMANTIC Category =====
    // Meaning and similarity relationships
    registerBuiltinType_({
        .type_name = "IS_A",
        .category = EdgeCategory::SEMANTIC,
        .description = "Type/instance relationship",
        .is_bidirectional = false,
        .requires_temporal = false,
        .is_weighted = false,
        .inverse_type = std::nullopt
    });
    registerBuiltinType_({
        .type_name = "SIMILAR_TO",
        .category = EdgeCategory::SEMANTIC,
        .description = "Similarity relationship",
        .is_bidirectional = true,
        .requires_temporal = false,
        .is_weighted = true,  // Weight = similarity score
        .inverse_type = "SIMILAR_TO"
    });
    registerBuiltinType_({
        .type_name = "RELATED_TO",
        .category = EdgeCategory::SEMANTIC,
        .description = "General semantic relationship",
        .is_bidirectional = true,
        .requires_temporal = false,
        .is_weighted = true,  // Weight = relevance score
        .inverse_type = "RELATED_TO"
    });
    registerBuiltinType_({
        .type_name = "SYNONYM_OF",
        .category = EdgeCategory::SEMANTIC,
        .description = "Synonymy relationship",
        .is_bidirectional = true,
        .requires_temporal = false,
        .is_weighted = false,
        .inverse_type = "SYNONYM_OF"
    });

    // ===== WORKFLOW Category =====
    // Process and dependency relationships
    registerBuiltinType_({
        .type_name = "TRIGGERS",
        .category = EdgeCategory::WORKFLOW,
        .description = "Event trigger relationship",
        .is_bidirectional = false,
        .requires_temporal = false,
        .is_weighted = false,
        .inverse_type = std::nullopt
    });
    registerBuiltinType_({
        .type_name = "DEPENDS_ON",
        .category = EdgeCategory::WORKFLOW,
        .description = "Dependency relationship",
        .is_bidirectional = false,
        .requires_temporal = false,
        .is_weighted = true,  // Weight = dependency strength
        .inverse_type = std::nullopt
    });
    registerBuiltinType_({
        .type_name = "FOLLOWS",
        .category = EdgeCategory::WORKFLOW,
        .description = "Sequence/follows relationship",
        .is_bidirectional = false,
        .requires_temporal = false,
        .is_weighted = false,
        .inverse_type = std::nullopt
    });
    registerBuiltinType_({
        .type_name = "BLOCKS",
        .category = EdgeCategory::WORKFLOW,
        .description = "Blocking dependency",
        .is_bidirectional = false,
        .requires_temporal = false,
        .is_weighted = false,
        .inverse_type = std::nullopt
    });

    // ===== ACCESS Category =====
    // Permission and ownership relationships
    registerBuiltinType_({
        .type_name = "CAN_READ",
        .category = EdgeCategory::ACCESS,
        .description = "Read permission",
        .is_bidirectional = false,
        .requires_temporal = false,
        .is_weighted = false,
        .inverse_type = std::nullopt
    });
    registerBuiltinType_({
        .type_name = "CAN_WRITE",
        .category = EdgeCategory::ACCESS,
        .description = "Write permission",
        .is_bidirectional = false,
        .requires_temporal = false,
        .is_weighted = false,
        .inverse_type = std::nullopt
    });
    registerBuiltinType_({
        .type_name = "CAN_DELETE",
        .category = EdgeCategory::ACCESS,
        .description = "Delete permission",
        .is_bidirectional = false,
        .requires_temporal = false,
        .is_weighted = false,
        .inverse_type = std::nullopt
    });
    registerBuiltinType_({
        .type_name = "OWNS",
        .category = EdgeCategory::ACCESS,
        .description = "Ownership relationship",
        .is_bidirectional = true,
        .requires_temporal = false,
        .is_weighted = false,
        .inverse_type = "OWNED_BY"
    });
    registerBuiltinType_({
        .type_name = "OWNED_BY",
        .category = EdgeCategory::ACCESS,
        .description = "Inverse ownership",
        .is_bidirectional = true,
        .requires_temporal = false,
        .is_weighted = false,
        .inverse_type = "OWNS"
    });
    registerBuiltinType_({
        .type_name = "MANAGES",
        .category = EdgeCategory::ACCESS,
        .description = "Management relationship",
        .is_bidirectional = true,
        .requires_temporal = false,
        .is_weighted = false,
        .inverse_type = "MANAGED_BY"
    });
    registerBuiltinType_({
        .type_name = "MANAGED_BY",
        .category = EdgeCategory::ACCESS,
        .description = "Inverse management",
        .is_bidirectional = true,
        .requires_temporal = false,
        .is_weighted = false,
        .inverse_type = "MANAGES"
    });

    initialized_ = true;
}

void EdgeTypeRegistry::registerBuiltinType_(const EdgeTypeInfo& info) {
    types_[info.type_name] = info;
    category_index_[info.category].insert(info.type_name);
}

EdgeTypeRegistry::Status EdgeTypeRegistry::registerType(const EdgeTypeInfo& info) {
    if (info.type_name.empty()) {
        return Status::Error("Edge type name cannot be empty");
    }
    
    if (types_.count(info.type_name)) {
        return Status::Error("Edge type '" + info.type_name + "' is already registered");
    }

    types_[info.type_name] = info;
    category_index_[info.category].insert(info.type_name);
    
    return Status::OK();
}

EdgeTypeRegistry::Status EdgeTypeRegistry::registerType(const EdgeTypeInfo& info, ValidationFunc validator) {
    auto st = registerType(info);
    if (!st.ok) return st;
    
    validators_[info.type_name] = std::move(validator);
    return Status::OK();
}

bool EdgeTypeRegistry::isRegistered(std::string_view type_name) const {
    return types_.count(std::string(type_name)) > 0;
}

std::optional<EdgeTypeInfo> EdgeTypeRegistry::getTypeInfo(std::string_view type_name) const {
    auto it = types_.find(std::string(type_name));
    if (it != types_.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<std::string> EdgeTypeRegistry::getTypesByCategory(EdgeCategory category) const {
    std::vector<std::string> result;
    auto it = category_index_.find(category);
    if (it != category_index_.end()) {
        result.insert(result.end(), it->second.begin(), it->second.end());
    }
    return result;
}

std::optional<EdgeCategory> EdgeTypeRegistry::getCategoryForType(std::string_view type_name) const {
    auto info = getTypeInfo(type_name);
    if (info.has_value()) {
        return info->category;
    }
    return std::nullopt;
}

EdgeTypeRegistry::Status EdgeTypeRegistry::validateEdge(std::string_view type_name, const BaseEntity& edge) const {
    auto info = getTypeInfo(type_name);
    if (!info.has_value()) {
        // Unregistered types are allowed (CUSTOM category assumed)
        return Status::OK();
    }

    // Check temporal requirements
    if (info->requires_temporal) {
        if (!edge.hasField("valid_from") && !edge.hasField("valid_to")) {
            return Status::Error("Edge type '" + std::string(type_name) + 
                "' requires temporal fields (valid_from or valid_to)");
        }
    }

    // Check weight requirements
    if (info->is_weighted) {
        if (!edge.hasField("_weight")) {
            // Weight is recommended but not strictly required
            // Could log a warning here
        }
    }

    // Run custom validator if registered
    auto validatorIt = validators_.find(std::string(type_name));
    if (validatorIt != validators_.end()) {
        if (!validatorIt->second(std::string(type_name), edge)) {
            return Status::Error("Custom validation failed for edge type '" + std::string(type_name) + "'");
        }
    }

    return Status::OK();
}

std::optional<std::string> EdgeTypeRegistry::getInverseType(std::string_view type_name) const {
    auto info = getTypeInfo(type_name);
    if (info.has_value()) {
        return info->inverse_type;
    }
    return std::nullopt;
}

std::vector<std::string> EdgeTypeRegistry::listAllTypes() const {
    std::vector<std::string> result;
    result.reserve(types_.size());
    for (const auto& [name, _] : types_) {
        result.push_back(name);
    }
    return result;
}

std::string EdgeTypeRegistry::categoryToString(EdgeCategory category) {
    switch (category) {
        case EdgeCategory::STRUCTURAL: return "STRUCTURAL";
        case EdgeCategory::REFERENCE:  return "REFERENCE";
        case EdgeCategory::TEMPORAL:   return "TEMPORAL";
        case EdgeCategory::SEMANTIC:   return "SEMANTIC";
        case EdgeCategory::WORKFLOW:   return "WORKFLOW";
        case EdgeCategory::ACCESS:     return "ACCESS";
        case EdgeCategory::CUSTOM:     return "CUSTOM";
    }
    return "UNKNOWN";
}

std::optional<EdgeCategory> EdgeTypeRegistry::categoryFromString(std::string_view str) {
    if (str == "STRUCTURAL") return EdgeCategory::STRUCTURAL;
    if (str == "REFERENCE")  return EdgeCategory::REFERENCE;
    if (str == "TEMPORAL")   return EdgeCategory::TEMPORAL;
    if (str == "SEMANTIC")   return EdgeCategory::SEMANTIC;
    if (str == "WORKFLOW")   return EdgeCategory::WORKFLOW;
    if (str == "ACCESS")     return EdgeCategory::ACCESS;
    if (str == "CUSTOM")     return EdgeCategory::CUSTOM;
    return std::nullopt;
}

} // namespace themis
