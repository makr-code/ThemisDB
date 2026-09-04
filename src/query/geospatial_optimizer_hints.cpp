/**
 * @file geospatial_optimizer_hints.cpp
 * @brief Optimizer hints implementation for Phase 6C
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Phase 6C Q3 2026 Delivery
 *
 * ThemisDB | Query Module Phase 6C: Geospatial Phase 2
 */

#include "query/geospatial_optimizer_hints.h"
#include "utils/logger.h"
#include <algorithm>
#include <sstream>
#include <regex>
#include <cctype>

namespace themis {
namespace query {

// =============================================================================
// SpatialHint
// =============================================================================

bool SpatialHint::isValid() const {
    if (fieldName.empty()) {
      return false;
    }
    
    switch (type) {
        case SpatialHintType::USE_INDEX:
            return !indexName.empty();
        case SpatialHintType::FORCE_SCAN:
            return true;
        case SpatialHintType::INDEX_PRIORITY:
            return priorityFactor >= 0.1 && priorityFactor <= 10.0;
        case SpatialHintType::DISTANCE_ORDER:
            return orderDirection == "ascending" || orderDirection == "descending";
        default:
            return false;
    }
}

std::string SpatialHint::toString() const {
    std::ostringstream oss = {};
    
    switch (type) {
        case SpatialHintType::USE_INDEX:
            oss << "USE_INDEX(" << fieldName << ", \"" << indexName << "\")";
            break;
        case SpatialHintType::FORCE_SCAN:
            oss << "FORCE_SCAN(" << fieldName << ")";
            break;
        case SpatialHintType::INDEX_PRIORITY:
            oss << "INDEX_PRIORITY(" << fieldName << ", " << priorityFactor << ")";
            break;
        case SpatialHintType::DISTANCE_ORDER:
            oss << "DISTANCE_ORDER(" << fieldName << ", \"" << orderDirection << "\")";
            break;
    }
    
    return oss.str();
}

// =============================================================================
// SpatialPlan
// =============================================================================

bool SpatialPlan::hasHint(SpatialHintType type) const {
    for (const auto& hint : hints) {
        if (hint.type == type) {
            return true;
        }
    }
    return false;
}

const SpatialHint* SpatialPlan::getHint(SpatialHintType type) const {
    for (const auto& hint : hints) {
        if (hint.type == type) {
            return &hint;
        }
    }
    return nullptr;
}

void SpatialPlan::addHint(const SpatialHint& hint) {
    if (hint.isValid()) {
        hints.push_back(hint);
    }
}

double SpatialPlan::getCostAdjustmentFactor() const {
    double factor = 1.0;
    
    for (const auto& hint : hints) {
        if (hint.type == SpatialHintType::INDEX_PRIORITY) {
            factor *= hint.priorityFactor;
        }
        // FORCE_SCAN and USE_INDEX modify execution plan, not cost adjustment
        // DISTANCE_ORDER is handled separately in execution
    }
    
    return factor;
}

// =============================================================================
// SpatialHintParser
// =============================================================================

SpatialHint SpatialHintParser::parseHint(const std::string& hintString) {
    SpatialHint hint;
    
    // Normalize whitespace
    std::string normalized = hintString;
    std::regex ws_regex("\\s+");
    normalized = std::regex_replace(normalized, ws_regex, " ");
    
    // Pattern: HINT_TYPE(field, arg)
    std::regex hint_regex("(USE_INDEX|FORCE_SCAN|INDEX_PRIORITY|DISTANCE_ORDER)\\s*\\(([^,]+),?([^)]*)\\)");
    std::smatch match = {};
    
    if (!std::regex_search(normalized, match, hint_regex)) {
        THEMIS_WARN("GeospatialHints: Failed to parse hint: {}", hintString);
        return hint;
    }
    
    std::string hintType = match[1].str();
    std::string fieldArg = match[2].str();
    std::string additionalArg = match[3].str();
    
    // Trim whitespace
    auto trim = [](std::string& s) {
        s.erase(0, s.find_first_not_of(" \t"));
        s.erase(s.find_last_not_of(" \t") + 1);
    };
    
    trim(fieldArg);
    trim(additionalArg);
    
    hint.fieldName = fieldArg;
    
    if (hintType == "USE_INDEX") {
        hint.type = SpatialHintType::USE_INDEX;
        // Remove quotes from index name if present
        if ((additionalArg.length() >= 2 && 
            (additionalArg.front() == '"' && additionalArg.back() == '"' ||
             additionalArg.front() == '\'' && additionalArg.back() == '\''))) {
            hint.indexName = additionalArg.substr(1, additionalArg.length() - 2);
        } else {
            hint.indexName = additionalArg;
        }
    } else if (hintType == "FORCE_SCAN") {
        hint.type = SpatialHintType::FORCE_SCAN;
    } else if (hintType == "INDEX_PRIORITY") {
        hint.type = SpatialHintType::INDEX_PRIORITY;
        try {
            hint.priorityFactor = std::stod(additionalArg);
        } catch (const std::exception&) {
            THEMIS_WARN("GeospatialHints: Invalid priority value: {}", additionalArg);
            hint.priorityFactor = 1.0;
        }
    } else if (hintType == "DISTANCE_ORDER") {
        hint.type = SpatialHintType::DISTANCE_ORDER;
        // Remove quotes
        if ((additionalArg.length() >= 2 && 
            (additionalArg.front() == '"' && additionalArg.back() == '"' ||
             additionalArg.front() == '\'' && additionalArg.back() == '\''))) {
            hint.orderDirection = additionalArg.substr(1, additionalArg.length() - 2);
        } else {
            hint.orderDirection = additionalArg;
        }
        // Normalize to lowercase
        std::transform(hint.orderDirection.begin(), hint.orderDirection.end(),
                      hint.orderDirection.begin(), ::tolower);
    }
    
    return hint;
}

bool SpatialHintParser::validateHint(
    const SpatialHint& hint,
    const std::map<std::string, std::string>& availableIndexes) {
    
    if (!hint.isValid()) {
        return false;
    }
    
    if (hint.type == SpatialHintType::USE_INDEX) {
        auto it = availableIndexes.find(hint.indexName);
        if (it == availableIndexes.end()) {
            THEMIS_WARN("GeospatialHints: Index not found: {}", hint.indexName);
            return false;
        }
        
        // Check if index type is spatial
        const auto& indexType = it->second;
        if (indexType != "SPATIAL" && indexType != "RTREE" && indexType != "QUADTREE") {
            THEMIS_WARN("GeospatialHints: Index type {} does not support spatial predicates", 
                       indexType);
            return false;
        }
    }
    
    return true;
}

std::string SpatialHintParser::getHintWarning(
    const SpatialHint& hint,
    const std::map<std::string, std::string>& availableIndexes) {
    
    if (hint.type == SpatialHintType::FORCE_SCAN) {
        // Check if good index is available
        for (const auto& [indexName, indexType] : availableIndexes) {
            if (((indexType == "SPATIAL" || indexType == "RTREE" || indexType == "QUADTREE") &&
                indexName.find(hint.fieldName) != std::string::npos)) {
                return "FORCE_SCAN specified but spatial index '" + indexName + 
                       "' is available and likely faster";
            }
        }
    }
    
    if (hint.type == SpatialHintType::USE_INDEX) {
        auto it = availableIndexes.find(hint.indexName);
        if (it != availableIndexes.end()) {
            const auto& indexType = it->second;
            if (indexType != "SPATIAL" && indexType != "RTREE" && indexType != "QUADTREE") {
                return "Index '" + hint.indexName + "' type '" + indexType + 
                       "' may not support spatial predicates efficiently";
            }
        }
    }
    
    return "";  // No warning
}

std::vector<SpatialHint> SpatialHintParser::parseHintsFromQuery(
    const std::string& queryText) {
    
    std::vector<SpatialHint> result;
    
    // Find all hint patterns in query text
    std::regex hint_pattern("(USE_INDEX|FORCE_SCAN|INDEX_PRIORITY|DISTANCE_ORDER)\\s*\\([^)]+\\)");
    std::sregex_iterator iter(queryText.begin(), queryText.end(), hint_pattern);
    std::sregex_iterator end = {};
    
    while (iter != end) {
        std::string hintStr = iter->str();
        SpatialHint hint = parseHint(hintStr);
        
        if (hint.isValid()) {
            result.push_back(hint);
        } else {
            THEMIS_WARN("GeospatialHints: Skipped invalid hint: {}", hintStr);
        }
        
        ++iter;
    }
    
    return result;
}

// =============================================================================
// SpatialHintContext
// =============================================================================

const SpatialPlan* SpatialHintContext::getPlanForPredicate(
    const std::string& predicateId) const {
    
    for (const auto& plan : plans) {
        if (plan.predicateId == predicateId) {
            return &plan;
        }
    }
    return nullptr;
}

bool SpatialHintContext::shouldUseIndex(const std::string& predicateId) const {
    const auto* plan = getPlanForPredicate(predicateId);
    if (!plan) return true;  // Default: use index if available
    
    // FORCE_SCAN overrides everything
    if (plan->hasHint(SpatialHintType::FORCE_SCAN)) {
        return false;
    }
    
    // USE_INDEX explicitly enables index
    if (plan->hasHint(SpatialHintType::USE_INDEX)) {
        return true;
    }
    
    // Default: use index if available
    return true;
}

std::string SpatialHintContext::getRecommendedIndex(const std::string& predicateId) const {
    const auto* plan = getPlanForPredicate(predicateId);
    if (!plan) {
      return "";
    }
    
    const auto* useIndexHint = plan->getHint(SpatialHintType::USE_INDEX);
    if (useIndexHint) {
        return useIndexHint->indexName;
    }
    
    return "";  // No specific recommendation
}

double SpatialHintContext::getCostAdjustment(const std::string& predicateId) const {
    const auto* plan = getPlanForPredicate(predicateId);
    if (!plan) return 1.0;  // No adjustment
    
    return plan->getCostAdjustmentFactor();
}

}  // namespace query
}  // namespace themis
