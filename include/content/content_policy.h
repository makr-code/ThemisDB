/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            content_policy.h                                   ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-02-21 11:48:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     94                                             ║
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

#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include <cstdint>

namespace themis {
namespace content {

/// Policy rule for a specific MIME type
struct MimePolicy {
    std::string mime_type;
    uint64_t max_size = 0;  ///< Maximum file size in bytes (0 = unlimited)
    std::string description;
    std::string reason;     ///< Reason for denial (for blacklist entries)
};

/// Category-based policy rule
struct CategoryPolicy {
    std::string category;
    bool action;  ///< true = allow, false = deny
    uint64_t max_size = 0;
    std::string reason;
};

/// Content upload validation policy
struct ContentPolicy {
    uint64_t default_max_size = 104857600;  ///< 100 MB default
    bool default_action = true;   ///< true = allow, false = deny
    
    std::vector<MimePolicy> allowed;
    std::vector<MimePolicy> denied;
    std::map<std::string, CategoryPolicy> category_rules;
    
    /// Check if a MIME type is explicitly allowed
    bool isAllowed(const std::string& mime_type) const;
    
    /// Check if a MIME type is explicitly denied
    bool isDenied(const std::string& mime_type) const;
    
    /// Get max size for a specific MIME type (returns 0 if unlimited)
    uint64_t getMaxSize(const std::string& mime_type) const;
    
    /// Get max size for a category
    uint64_t getCategoryMaxSize(const std::string& category) const;
    
    /// Get denial reason (empty if allowed)
    std::string getDenialReason(const std::string& mime_type) const;
};

/// Validation result for file upload
struct ValidationResult {
    bool allowed = false;
    std::string mime_type;
    uint64_t file_size = 0;
    uint64_t max_allowed_size = 0;
    std::string reason;  ///< Human-readable explanation
    
    // Additional context
    bool size_exceeded = false;
    bool blacklisted = false;
    bool not_whitelisted = false;
};

} // namespace content
} // namespace themis
