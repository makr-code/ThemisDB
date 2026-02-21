/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            content_policy.cpp                                 ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-02-21 11:48:42                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     76                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
