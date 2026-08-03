/**
 * @file age_based_policy.cpp
 * @brief Implementation of age-based migration policy.
 *
 * ThemisDB | File: age_based_policy.cpp | Version: 1.0.0
 * Maturity: 🟡 ALPHA (Phase 2 Implementation) | Status: In Progress
 * Author: Copilot | Date: 2026-08-03
 */

#include "access_model/age_based_policy.h"

#include <sstream>

namespace themis {
namespace access_model {

std::string AgeBasedPolicy::describe() const {
    std::ostringstream oss;
    
    oss << "AgeBasedPolicy {\n";
    oss << "  Cache Tier Thresholds:\n";
    oss << "    l1_zero_access_days: " << l1_zero_access_days << " days\n";
    oss << "    l2_zero_access_days: " << l2_zero_access_days << " days\n";
    oss << "    l3_to_storage_days: " << l3_to_storage_days << " days\n";
    oss << "  Storage Tier Thresholds:\n";
    oss << "    hot_zero_access_days: " << hot_zero_access_days << " days\n";
    oss << "    hot_to_warm_days: " << hot_to_warm_days << " days\n";
    oss << "    warm_zero_access_days: " << warm_zero_access_days << " days\n";
    oss << "    warm_to_cold_days: " << warm_to_cold_days << " days\n";
    oss << "  Promotion Thresholds:\n";
    oss << "    l1_promotion_threshold: " << l1_promotion_threshold << " accesses\n";
    oss << "    l2_promotion_threshold: " << l2_promotion_threshold << " accesses\n";
    oss << "    storage_promotion_threshold: " << storage_promotion_threshold
        << " accesses\n";
    oss << "    storage_promotion_window: "
        << storage_promotion_window.count() << " seconds\n";
    oss << "}\n";
    
    return oss.str();
}

}  // namespace access_model
}  // namespace themis
