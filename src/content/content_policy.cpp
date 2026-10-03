/** @file content_policy.cpp @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata. @author makr-code @version 0.0.1 @date 2026-09-18 10:42:58 @note Maturity: 🟢 PRODUCTION-READY @note Score: 100/100 @note Lines: 52 @note Module Context: src/content @note Ownership Scope: production-code @note Primary Symbols: none-detected @note PR History (last 5): #5950 [CP-1] Content Module Batch... (2026-08-15) | #4331 feat(content): perceptual h... (2026-03-19) @note Governance: BranchModel=develop-first; CanonicalBranches=develop,community,military @note Release Context: GateModel=WaveA→B→C→D on develop @note Status: Production Ready @note Generator: .github/scripts/code_maturity_header_writer.py @note This block is auto-generated and will be overwritten. */

#include "content/content_policy.h"
#include <algorithm>

namespace themis {
namespace content {

bool ContentPolicy::isAllowed(const std::string& mime_type) const {
    return std::any_of(allowed.begin(), allowed.end(),
        [&mime_type](const MimePolicy& p) { return p.mime_type == mime_type; });
}

bool ContentPolicy::isDenied(const std::string& mime_type) const {
    return std::any_of(denied.begin(), denied.end(),
        [&mime_type](const MimePolicy& p) { return p.mime_type == mime_type; });
}

uint64_t ContentPolicy::getMaxSize(const std::string& mime_type) const {
    // Check explicit allowed list
    auto it = std::find_if(allowed.begin(), allowed.end(),
        [&mime_type](const MimePolicy& p) { return p.mime_type == mime_type; });
    
    if (it != allowed.end() && it->max_size > 0) {
        return it->max_size;
    }
    
    // Return default
    return default_max_size;
}

uint64_t ContentPolicy::getCategoryMaxSize(const std::string& category) const {
    auto it = category_rules.find(category);
    if (it != category_rules.end() && it->second.max_size > 0) {
        return it->second.max_size;
    }
    return default_max_size;
}

std::string ContentPolicy::getDenialReason(const std::string& mime_type) const {
    auto it = std::find_if(denied.begin(), denied.end(),
        [&mime_type](const MimePolicy& p) { return p.mime_type == mime_type; });
    
    if (it != denied.end()) {
        return it->reason.empty() ? "MIME type is blacklisted" : it->reason;
    }
    
    return "";
}

} // namespace content
} // namespace themis
