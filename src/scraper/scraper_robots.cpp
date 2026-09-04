/**
 * @file scraper_robots.cpp
 * @brief Implementation of RobotsTxtCache — fetch, parse, and cache robots.txt rules.
 * @version 1.0.0
 */

#include "scraper/scraper_robots.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace themis {
namespace scraper {

// ============================================================================
// Construction
// ============================================================================

RobotsTxtCache::RobotsTxtCache(FetchFn fetch_fn)
    : fetch_fn_(std::move(fetch_fn)) {}

// ============================================================================
// Helpers
// ============================================================================

namespace {

/// Convert a string to lower-case in-place.
std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

/// Strip leading and trailing whitespace from a string.
std::string trim(const std::string& s) {
    const auto begin = s.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return {};
    const auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(begin, end - begin + 1);
}

} // anonymous namespace

/*static*/ std::string RobotsTxtCache::extractDomain(const std::string& url) {
    // Find scheme separator "://"
    const auto sep = url.find("://");
    if (sep == std::string::npos) {
      return url;
    }
    const auto after_scheme = sep + 3;
    // Find end of authority (first '/', '?', '#', or end-of-string)
    auto end = url.find_first_of("/?#", after_scheme);
    if (end == std::string::npos) {
      end = url.size();
    }
    return url.substr(after_scheme, end - after_scheme);
}

/*static*/ std::string RobotsTxtCache::extractPath(const std::string& url) {
    const auto sep = url.find("://");
    if (sep == std::string::npos) {
      return "/";
    }
    const auto after_scheme = sep + 3;
    const auto path_start = url.find('/', after_scheme);
    if (path_start == std::string::npos) {
      return "/";
    }
    // Strip query/fragment
    const auto q = url.find_first_of("?#", path_start);
    if (q == std::string::npos) {
      return url.substr(path_start);
    }
    return url.substr(path_start, q - path_start);
}

/*static*/ std::string extractScheme(const std::string& url) {
    const auto sep = url.find("://");
    if (sep == std::string::npos) {
      return "https";
    }
    return url.substr(0, sep);
}

// ============================================================================
// Parser
// ============================================================================

/*static*/ RobotsTxtRules RobotsTxtCache::parse(const std::string& content) {
    RobotsTxtRules result;

    bool in_wildcard_block = false;
    std::istringstream ss(content);
    std::string line;

    while (std::getline(ss, line)) {
        // Strip carriage-return in CRLF files
        if (!line.empty() && line.back() == '\r') {
          line.pop_back();
        }

        line = trim(line);

        // Blank line ends a user-agent block
        if (line.empty()) {
            in_wildcard_block = false;
            continue;
        }
        // Comment line
        if (line[0] == '#') {
          continue;
        }

        // Find colon separator
        const auto colon = line.find(':');
        if (colon == std::string::npos) {
          continue;
        }

        const std::string key   = toLower(trim(line.substr(0, colon)));
        const std::string value = trim(line.substr(colon + 1));

        if (key == "user-agent") {
            // Enter wildcard block only for "*"
            in_wildcard_block = (value == "*");
        } else if (in_wildcard_block) {
            if (key == "disallow") {
                if (!value.empty()) {
                    result.disallow.push_back(value);
                }
                // Empty Disallow means "allow all" — we simply don't add a rule.
            } else if (key == "allow") {
                if (!value.empty()) {
                    result.allow.push_back(value);
                }
            }
        }
    }

    return result;
}

// ============================================================================
// Cache operations
// ============================================================================

void RobotsTxtCache::injectRobots(const std::string& domain,
                                   const std::string& content) {
    std::lock_guard<std::mutex> lk(mutex_);
    cache_[domain] = parse(content);
}

void RobotsTxtCache::clear() {
    std::lock_guard<std::mutex> lk(mutex_);
    cache_.clear();
}

void RobotsTxtCache::fetchAndCache(const std::string& domain,
                                    const std::string& scheme,
                                    const std::string& user_agent) {
    // cache_ lock is already held by the caller
    const std::string robots_url = scheme + "://" + domain + "/robots.txt";
    std::string content;
    try {
        if (fetch_fn_) {
            content = fetch_fn_(robots_url, user_agent);
        }
    } catch (...) {
        // Network error → treat as "allow all" (no rules)
    }
    cache_[domain] = content.empty() ? RobotsTxtRules{} : parse(content);
}

// ============================================================================
// isAllowed()
// ============================================================================

bool RobotsTxtCache::isAllowed(const std::string& url,
                                const std::string& user_agent) {
    const std::string domain = extractDomain(url);
    const std::string path   = extractPath(url);
    const std::string scheme = extractScheme(url);

    std::lock_guard<std::mutex> lk(mutex_);

    // Populate cache on first access for this domain
    if (cache_.find(domain) == cache_.end()) {
        fetchAndCache(domain, scheme, user_agent);
    }

    const RobotsTxtRules& rules = cache_.at(domain);

    // Allow takes precedence over Disallow (RFC 9309 §2.2.2)
    for (const auto& allow_prefix : rules.allow) {
        if (path.find(allow_prefix) == 0) {
            return true;
        }
    }

    for (const auto& disallow_prefix : rules.disallow) {
        if (path.find(disallow_prefix) == 0) {
            return false;
        }
    }

    return true; // Default: allowed
}

} // namespace scraper
} // namespace themis
