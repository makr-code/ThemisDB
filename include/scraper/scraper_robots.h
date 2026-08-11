/**
 * @file scraper_robots.h
 * @brief robots.txt fetching, parsing, and per-domain access cache.
 * @version 1.0.0
 *
 * ## Purpose
 *
 * Provides `RobotsTxtCache` — a per-domain cache that fetches and parses a
 * site's `robots.txt` on first access and answers `isAllowed()` queries for
 * subsequent URLs on the same domain.  The cache is thread-safe.
 *
 * ## Robots.txt grammar supported
 *
 * A simplified subset of RFC 9309 is parsed:
 *   - `User-agent: *`   (only the wildcard agent is matched)
 *   - `Disallow: /path` (prefix-match; empty `Disallow:` means allow all)
 *   - `Allow: /path`    (prefix-match; evaluated before Disallow rules)
 *   - Blank lines end a user-agent block.
 *   - Lines starting with `#` are comments.
 *   - Field names are case-insensitive.
 *
 * ## Injection for tests
 *
 * `injectRobots(domain, content)` bypasses network access so that unit tests
 * can verify crawl-policy enforcement without live HTTP calls.
 *
 * @see src/scraper/ROADMAP.md — v1.2 feature: robots.txt respect
 */

#pragma once

#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace scraper {

// ============================================================================
// Parsed robots.txt representation
// ============================================================================

/**
 * @brief In-memory representation of the rules for the wildcard user-agent.
 *
 * Only `User-agent: *` rules are stored; site-specific agent rules are
 * ignored because the scraper sends a generic user-agent string.
 */
struct RobotsTxtRules {
    /// Disallow prefixes (evaluated after Allow prefixes).
    std::vector<std::string> disallow;
    /// Allow prefixes (take precedence over matching Disallow entries).
    std::vector<std::string> allow;
};

// ============================================================================
// RobotsTxtCache
// ============================================================================

/**
 * @brief Thread-safe per-domain cache for parsed robots.txt rules.
 *
 * On the first call to `isAllowed()` for a domain, the cache fetches
 * `<scheme>://<host>/robots.txt` using the injected or default HTTP function,
 * parses the wildcard-agent block, and stores the result.  Subsequent calls
 * return from cache without any network I/O.
 *
 * ### Failure handling
 * If the robots.txt fetch fails (network error, 4xx/5xx), the cache stores
 * an empty `RobotsTxtRules` (all paths allowed) — the convention for domains
 * that do not publish a robots.txt.
 *
 * ### Thread safety
 * All public methods are guarded by an internal mutex and are safe to call
 * from multiple threads concurrently.
 */
class RobotsTxtCache {
public:
    /**
     * @brief Injectable HTTP fetch function.
     *
     * Signature: `(url, user_agent) -> response_body_string`.
     * Throws `std::runtime_error` on network errors (treated as "allow all").
     */
    using FetchFn = std::function<std::string(
        const std::string& url,
        const std::string& user_agent)>;

    RobotsTxtCache() = default;

    /**
     * @brief Construct with a custom fetch function.
     * @param fetch_fn  HTTP GET implementation (default: libcurl via ScraperPlugin).
     */
    explicit RobotsTxtCache(FetchFn fetch_fn);

    /**
     * @brief Test helper: pre-populate the cache for a domain.
     *
     * @param domain   Bare hostname (e.g. "example.com").
     * @param content  Raw robots.txt content to parse and cache.
     *
     * After this call, `isAllowed()` for any URL on @p domain will use the
     * injected rules without making any HTTP calls.
     */
    void injectRobots(const std::string& domain, const std::string& content);

    /**
     * @brief Check whether a URL is allowed to be fetched.
     *
     * Fetches and caches the domain's robots.txt on first call for that domain.
     *
     * @param url        Full URL to check (e.g. "https://example.com/path").
     * @param user_agent User-agent string sent in the robots.txt fetch.
     * @return true  The path is allowed by the wildcard rules (or no rules apply).
     * @return false A Disallow rule matches and no Allow rule overrides it.
     */
    [[nodiscard]] bool isAllowed(const std::string& url,
                                 const std::string& user_agent);

    /**
     * @brief Clear all cached entries (e.g. between test cases).
     */
    void clear();

    /**
     * @brief Parse raw robots.txt content into a `RobotsTxtRules` struct.
     *
     * Static utility exposed for unit testing the parser in isolation.
     *
     * @param content  Raw robots.txt text.
     * @return         Wildcard-agent rules extracted from @p content.
     */
    [[nodiscard]] static RobotsTxtRules parse(const std::string& content);

private:
    /// Extract the bare hostname from a full URL.
    static std::string extractDomain(const std::string& url);

    /// Extract the path component from a full URL.
    static std::string extractPath(const std::string& url);

    /// Fetch and parse the robots.txt for @p domain; store in cache_.
    void fetchAndCache(const std::string& domain,
                       const std::string& scheme,
                       const std::string& user_agent);

    FetchFn                             fetch_fn_;
    std::map<std::string, RobotsTxtRules> cache_;
    mutable std::mutex                  mutex_;
};

} // namespace scraper
} // namespace themis
