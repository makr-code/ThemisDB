/**
 * @file web_crawler_connector.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=5; TODO=1, Stub=1, Unimpl=0, Mock=3, Sim=0, Debt=0, C=2, H=2, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// When THEMIS_ENABLE_CURL is defined the full libcurl-backed implementation
// is compiled.  When the macro is absent the connector still compiles and:
//   - returns CONNECTOR_NOT_SUPPORTED on any live HTTP call, OR
//   - uses injected mock functions (unit tests).

#include "ingestion/web_crawler_connector.h"

#ifdef THEMIS_ENABLE_CURL
#include <curl/curl.h>
#endif

#include <sstream>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <queue>
#include <unordered_set>
#include <algorithm>
#include <cctype>
#include <cstring>

#ifdef ERROR
#undef ERROR
#endif

namespace themis {
namespace ingestion {

// ---------------------------------------------------------------------------
// Minimal URL helpers (dependency-free)
// ---------------------------------------------------------------------------

namespace {

/// Extracts the scheme+host portion from a URL, e.g. "https://example.com".
/// Returns empty string on parse failure.
static std::string urlOrigin(const std::string& url) {
    // Find "://"
    auto scheme_end = url.find("://");
    if (scheme_end == std::string::npos) return {};
    auto host_start = scheme_end + 3;
    auto host_end   = url.find('/', host_start);
    if (host_end == std::string::npos) return url; // no path
    return url.substr(0, host_end);
}

/// Returns true if the URL scheme is http or https (the only schemes this
/// crawler is permitted to fetch, preventing SSRF via file://, ftp://, etc.).
static bool isAllowedScheme(const std::string& url) {
    if (url.find("http://") == 0)  return true;
    if (url.find("https://") == 0) return true;
    return false;
}

/// Resolves a potentially relative href against a base URL.
/// Returns the absolute URL, or empty string if the href is not usable.
static std::string resolveUrl(const std::string& base,
                               const std::string& href) {
    if (href.empty()) return {};
    // Skip anchors, mailto, javascript, and any non-http/https absolute URI
    if (href[0] == '#') return {};
    if (href.find("mailto:") == 0) return {};
    if (href.find("javascript:") == 0) return {};

    // Already absolute: only allow http/https to prevent SSRF
    if (href.find("://") != std::string::npos) {
        return isAllowedScheme(href) ? href : std::string{};
    }

    // Protocol-relative
    if (href.size() >= 2 && href[0] == '/' && href[1] == '/') {
        auto colon = base.find(':');
        if (colon == std::string::npos) return {};
        std::string resolved = base.substr(0, colon + 1) + href;
        return isAllowedScheme(resolved) ? resolved : std::string{};
    }

    std::string origin = urlOrigin(base);
    if (origin.empty()) return {};

    if (href[0] == '/') {
        return origin + href;
    }

    // Relative path: strip the last path segment from base
    auto q = base.find('?');
    std::string base_path = (q != std::string::npos) ? base.substr(0, q) : base;
    auto slash = base_path.rfind('/');
    if (slash == std::string::npos || slash < origin.size()) {
        return origin + '/' + href;
    }
    return base_path.substr(0, slash + 1) + href;
}

/// Normalises a URL by stripping the fragment part.
static std::string normaliseUrl(const std::string& url) {
    auto hash = url.find('#');
    return (hash == std::string::npos) ? url : url.substr(0, hash);
}

/// Extracts plain text from an HTML body by stripping tags.
/// Handles basic entity decoding for &amp; < > &quot; &apos;
static std::string htmlToText(const std::string& html) {
    std::string text;
    text.reserve(html.size() / 2);
    bool in_tag    = false;
    bool in_script = false;
    bool in_style  = false;

    auto startsWithCI = [&](size_t pos, const char* needle) {
        size_t n = std::strlen(needle);
        if (pos + n > html.size()) return false;
        for (size_t i = 0; i < n; ++i) {
            if (std::tolower(static_cast<unsigned char>(html[pos + i])) !=
                std::tolower(static_cast<unsigned char>(needle[i])))
                return false;
        }
        return true;
    };

    for (size_t i = 0; i < html.size(); ++i) {
        char c = html[i];
        if (c == '<') {
            // Check for <script or <style blocks to skip entirely
            if (!in_tag) {
                if (startsWithCI(i + 1, "script")) in_script = true;
                else if (startsWithCI(i + 1, "/script")) in_script = false;
                else if (startsWithCI(i + 1, "style")) in_style = true;
                else if (startsWithCI(i + 1, "/style")) in_style = false;
            }
            in_tag = true;
            continue;
        }
        if (c == '>') {
            in_tag = false;
            // Insert a space between tags to avoid word-merging
            if (!text.empty() && text.back() != ' ') text += ' ';
            continue;
        }
        if (in_tag || in_script || in_style) continue;

        // Basic entity decoding
        if (c == '&') {
            if (i + 4 < html.size() && html.substr(i, 5) == "&amp;")  { text += '&'; i += 4; continue; }
            if (i + 3 < html.size() && html.substr(i, 4) == "<")   { text += '<'; i += 3; continue; }
            if (i + 3 < html.size() && html.substr(i, 4) == ">")   { text += '>'; i += 3; continue; }
            if (i + 5 < html.size() && html.substr(i, 6) == "&quot;") { text += '"'; i += 5; continue; }
            if (i + 5 < html.size() && html.substr(i, 6) == "&apos;") { text += '\''; i += 5; continue; }
        }

        text += c;
    }

    // Collapse runs of whitespace
    std::string result;
    result.reserve(text.size());
    bool prev_space = true; // skip leading whitespace
    for (char ch : text) {
        bool is_ws = (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r');
        if (is_ws) {
            if (!prev_space) { result += ' '; prev_space = true; }
        } else {
            result += ch;
            prev_space = false;
        }
    }
    // Trim trailing space
    if (!result.empty() && result.back() == ' ') result.pop_back();
    return result;
}

/// Extracts all href values from HTML anchor tags.
static std::vector<std::string> extractHrefs(const std::string& html) {
    std::vector<std::string> hrefs;
    size_t pos = 0;
    while (pos < html.size()) {
        // Find <a (case-insensitive)
        auto a_pos = html.find('<', pos);
        if (a_pos == std::string::npos) break;
        // Check "a " or "a\t" or "a>"
        size_t tag_start = a_pos + 1;
        // Skip whitespace after <
        while (tag_start < html.size() && html[tag_start] == ' ') ++tag_start;
        if (tag_start >= html.size()) { pos = a_pos + 1; continue; }
        char t0 = static_cast<char>(std::tolower(static_cast<unsigned char>(html[tag_start])));
        if (t0 != 'a') { pos = a_pos + 1; continue; }
        size_t after_a = tag_start + 1;
        if (after_a >= html.size()) { pos = a_pos + 1; continue; }
        char delim = html[after_a];
        if (delim != ' ' && delim != '\t' && delim != '\n' && delim != '\r' && delim != '>') {
            pos = a_pos + 1;
            continue;
        }

        // Find end of this tag
        auto tag_end = html.find('>', a_pos);
        if (tag_end == std::string::npos) break;
        std::string tag = html.substr(a_pos, tag_end - a_pos + 1);

        // Find href= in the tag (case-insensitive)
        std::string tag_lc = tag;
        std::transform(tag_lc.begin(), tag_lc.end(), tag_lc.begin(),
                       [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

        auto href_pos = tag_lc.find("href=");
        if (href_pos != std::string::npos) {
            size_t val_start = href_pos + 5;
            if (val_start < tag.size()) {
                char quote = tag[val_start];
                if (quote == '"' || quote == '\'') {
                    ++val_start;
                    auto val_end = tag.find(quote, val_start);
                    if (val_end != std::string::npos) {
                        hrefs.push_back(tag.substr(val_start, val_end - val_start));
                    }
                } else {
                    // Unquoted
                    auto val_end = val_start;
                    while (val_end < tag.size() && tag[val_end] != ' ' &&
                           tag[val_end] != '>' && tag[val_end] != '\t')
                        ++val_end;
                    hrefs.push_back(tag.substr(val_start, val_end - val_start));
                }
            }
        }

        pos = tag_end + 1;
    }
    return hrefs;
}

/// Extracts <loc> URL values from an XML sitemap or sitemap index.
static std::vector<std::string> extractSitemapLocs(const std::string& xml) {
    std::vector<std::string> locs;
    size_t pos = 0;
    const std::string open  = "<loc>";
    const std::string close = "</loc>";
    while (pos < xml.size()) {
        auto start = xml.find(open, pos);
        if (start == std::string::npos) break;
        auto val_start = start + open.size();
        auto end = xml.find(close, val_start);
        if (end == std::string::npos) break;
        std::string loc = xml.substr(val_start, end - val_start);
        // Trim whitespace
        while (!loc.empty() && std::isspace(static_cast<unsigned char>(loc.front()))) loc.erase(loc.begin());
        while (!loc.empty() && std::isspace(static_cast<unsigned char>(loc.back())))  loc.pop_back();
        if (!loc.empty()) locs.push_back(loc);
        pos = end + close.size();
    }
    return locs;
}

/// Checks whether an XML document is a sitemap index (contains <sitemapindex>).
static bool isSitemapIndex(const std::string& xml) {
    return xml.find("<sitemapindex") != std::string::npos;
}

/// Returns the Disallow paths from a robots.txt body for a given user agent.
static std::vector<std::string> parseRobotsTxt(const std::string& body,
                                                const std::string& user_agent) {
    std::vector<std::string> disallowed;
    std::istringstream ss(body);
    std::string line;

    bool in_matching_agent = false;
    bool in_wildcard_agent = false;
    std::vector<std::string> disallowed_specific;
    std::vector<std::string> disallowed_wildcard;

    std::string ua_lc = user_agent;
    std::transform(ua_lc.begin(), ua_lc.end(), ua_lc.begin(),
                   [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

    while (std::getline(ss, line)) {
        // Strip carriage return and comments
        if (!line.empty() && line.back() == '\r') line.pop_back();
        auto hash = line.find('#');
        if (hash != std::string::npos) line = line.substr(0, hash);
        // Trim trailing space
        while (!line.empty() && std::isspace(static_cast<unsigned char>(line.back()))) line.pop_back();
        if (line.empty()) {
            in_matching_agent = false;
            in_wildcard_agent = false;
            continue;
        }

        // Parse "Key: value"
        auto colon = line.find(':');
        if (colon == std::string::npos) continue;
        std::string key = line.substr(0, colon);
        std::string val = line.substr(colon + 1);
        // Trim key and value
        while (!key.empty() && std::isspace(static_cast<unsigned char>(key.back()))) key.pop_back();
        while (!val.empty() && std::isspace(static_cast<unsigned char>(val.front()))) val.erase(val.begin());

        std::string key_lc = key;
        std::transform(key_lc.begin(), key_lc.end(), key_lc.begin(),
                       [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

        if (key_lc == "user-agent") {
            std::string agent_lc = val;
            std::transform(agent_lc.begin(), agent_lc.end(), agent_lc.begin(),
                           [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
            in_matching_agent = (agent_lc == ua_lc || agent_lc.find(ua_lc) != std::string::npos);
            in_wildcard_agent = (agent_lc == "*");
        } else if (key_lc == "disallow") {
            if (in_matching_agent && !val.empty()) disallowed_specific.push_back(val);
            if (in_wildcard_agent && !val.empty()) disallowed_wildcard.push_back(val);
        }
    }

    // Specific user-agent rules take precedence; fall back to wildcard
    disallowed = disallowed_specific.empty() ? disallowed_wildcard : disallowed_specific;
    return disallowed;
}

/// Returns true if `path` is disallowed by any of the given robots.txt rules.
static bool isDisallowedByRobots(const std::string& url,
                                  const std::vector<std::string>& disallow_rules) {
    // Extract the path from the URL
    std::string path;
    auto scheme_end = url.find("://");
    if (scheme_end != std::string::npos) {
        auto path_start = url.find('/', scheme_end + 3);
        if (path_start != std::string::npos) {
            path = url.substr(path_start);
        } else {
            path = "/";
        }
    } else {
        path = url;
    }

    for (const auto& rule : disallow_rules) {
        if (path.find(rule) == 0) return true;
    }
    return false;
}

#ifdef THEMIS_ENABLE_CURL
// libcurl write callback
static size_t webCrawlerWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* buf = static_cast<std::string*>(userdata);
    buf->append(ptr, size * nmemb);
    return size * nmemb;
}
#endif

} // anonymous namespace

// ---------------------------------------------------------------------------
// Pimpl
// ---------------------------------------------------------------------------

class WebCrawlerConnector::Impl {
public:
    Impl() = default;

    bool initialize(const SourceConfig& config) {
        if (config.type != SourceType::WEB_CRAWLER) return false;
        if (config.location.empty()) return false;
        // Only allow http/https seed URLs to prevent SSRF
        if (!isAllowedScheme(config.location)) return false;
        config_   = config;
        seed_url_ = config.location;

        auto opt = [&](const std::string& k, const std::string& def) -> std::string {
            auto it = config.options.find(k);
            return (it != config.options.end()) ? it->second : def;
        };

        try { max_depth_ = std::stoi(opt("max_depth", "3")); }
        catch (...) { max_depth_ = 3; }
        try { max_pages_ = std::stoul(opt("max_pages", "0")); }
        catch (...) { max_pages_ = 0; }

        user_agent_       = opt("user_agent",       "ThemisDB-Crawler/1.0");
        follow_sitemaps_  = (opt("follow_sitemaps",  "true") != "false");
        respect_robots_   = (opt("respect_robots",   "true") != "false");
        same_domain_only_ = (opt("same_domain_only", "true") != "false");

        initialized_ = true;
        return true;
    }

    bool isAvailable() const {
        if (!initialized_ || seed_url_.empty()) return false;
        // Quick reachability check
        auto [status, body] = fetchUrl(seed_url_);
        return (status >= 200 && status < 400);
    }

    size_t getDocumentCount() const {
        return 0; // unknown before crawling
    }

    IngestionStats ingest(const std::string& /*target_collection*/,
                          ProgressCallback progress_callback) {
        IngestionStats stats;
        if (!initialized_) {
            stats.addError(IngestionErrorCode::CONNECTOR_INIT_FAILED,
                           IngestionErrorSeverity::ERROR,
                           "WebCrawlerConnector not initialized");
            return stats;
        }

        std::string origin = urlOrigin(seed_url_);

        // ----- Step 1: load robots.txt -----
        std::vector<std::string> disallow_rules;
        if (respect_robots_ && !origin.empty()) {
            auto [rcode, rbody] = fetchUrl(origin + "/robots.txt");
            if (rcode == 200) {
                disallow_rules = parseRobotsTxt(rbody, user_agent_);
            }
        }

        // ----- Step 2: seed the crawl queue -----
        // BFS queue: {url, depth}
        std::queue<std::pair<std::string, int>> queue;
        std::unordered_set<std::string> visited;

        auto enqueue = [&](const std::string& url, int depth) {
            std::string norm = normaliseUrl(url);
            if (norm.empty()) return;
            if (visited.count(norm)) return;
            if (same_domain_only_ && !origin.empty() &&
                urlOrigin(norm) != origin) return;
            if (respect_robots_ && isDisallowedByRobots(norm, disallow_rules)) return;
            if (max_pages_ > 0 && visited.size() + queue.size() >= max_pages_) return;
            visited.insert(norm);
            queue.push({norm, depth});
        };

        // Sitemap discovery
        if (follow_sitemaps_ && !origin.empty()) {
            std::queue<std::string> sitemap_queue;
            sitemap_queue.push(origin + "/sitemap.xml");
            int sitemap_depth = 0;
            const int max_sitemap_depth = 5;

            while (!sitemap_queue.empty() && sitemap_depth <= max_sitemap_depth) {
                std::string smap_url = sitemap_queue.front();
                sitemap_queue.pop();
                auto [scode, sbody] = fetchUrl(smap_url);
                if (scode != 200) continue;

                if (isSitemapIndex(sbody)) {
                    // Nested sitemaps
                    for (const auto& loc : extractSitemapLocs(sbody)) {
                        sitemap_queue.push(loc);
                    }
                    ++sitemap_depth;
                } else {
                    // Regular sitemap – add all <loc> URLs to crawl queue
                    for (const auto& loc : extractSitemapLocs(sbody)) {
                        enqueue(loc, 0);
                    }
                }
            }
        }

        // Always enqueue the seed URL itself
        enqueue(seed_url_, 0);

        // ----- Step 3: BFS crawl -----
        auto start_time = std::chrono::steady_clock::now();

        while (!queue.empty()) {
            if (max_pages_ > 0 && stats.documents_processed >= max_pages_) break;

            auto [url, depth] = queue.front();
            queue.pop();

            // Exponential back-off retry for individual pages
            std::pair<int, std::string> fetch_result;
            double delay_ms  = retry_config_.initial_delay_ms;
            bool   succeeded = false;
            for (int attempt = 1; attempt <= retry_config_.max_attempts; ++attempt) {
                fetch_result = fetchUrl(url);
                if (fetch_result.first == 200) { succeeded = true; break; }
                if (fetch_result.first == 404 ||
                    fetch_result.first == 401 ||
                    fetch_result.first == 403) break; // non-retryable
                if (attempt < retry_config_.max_attempts) {
                    ++stats.metrics.retry_count;
                    std::this_thread::sleep_for(
                        std::chrono::milliseconds(static_cast<int64_t>(delay_ms)));
                    delay_ms = std::min(delay_ms * retry_config_.backoff_factor,
                                        retry_config_.max_delay_ms);
                }
            }

            if (!succeeded) {
                ++stats.documents_failed;
                IngestionErrorCode code = IngestionErrorCode::HTTP_REQUEST_FAILED;
                if (fetch_result.first == 401 || fetch_result.first == 403)
                    code = IngestionErrorCode::HTTP_UNAUTHORIZED;
                else if (fetch_result.first == 404)
                    code = IngestionErrorCode::HTTP_NOT_FOUND;
                else if (fetch_result.first == 429)
                    code = IngestionErrorCode::HTTP_RATE_LIMITED;
                else if (fetch_result.first == 0)
                    code = IngestionErrorCode::HTTP_TIMEOUT;
                stats.addError(code, IngestionErrorSeverity::WARNING,
                               "Failed to fetch: " + url, config_.source_id);
                continue;
            }

            const std::string& body = fetch_result.second;
            stats.bytes_processed  += body.size();

            // Extract plain text
            std::string text = htmlToText(body);
            if (!text.empty()) {
                ++stats.documents_processed;
                if (progress_callback) {
                    progress_callback(config_.source_id,
                                      stats.documents_processed,
                                      0 /* total unknown */,
                                      "crawling");
                }
            }

            // Discover links for deeper crawling
            if (depth < max_depth_) {
                for (const auto& href : extractHrefs(body)) {
                    auto resolved = normaliseUrl(resolveUrl(url, href));
                    if (!resolved.empty()) {
                        enqueue(resolved, depth + 1);
                    }
                }
            }
        }

        auto end_time = std::chrono::steady_clock::now();
        stats.elapsed_seconds = std::chrono::duration<double>(
            end_time - start_time).count();

        if (stats.elapsed_seconds > 0) {
            stats.metrics.throughput_docs_per_sec =
                static_cast<double>(stats.documents_processed) / stats.elapsed_seconds;
        }

        return stats;
    }

    void setRetryConfig(const RetryConfig& config) {
        retry_config_ = config;
    }

    void setHttpFetchForTesting(WebCrawlerConnector::HttpFetchFn fn) {
        mock_fetch_ = std::move(fn);
    }

private:
    std::pair<int, std::string> fetchUrl(const std::string& url) const {
        // Use mock if injected
        if (mock_fetch_) return mock_fetch_(url);

#ifdef THEMIS_ENABLE_CURL
        CURL* curl = curl_easy_init();
        if (!curl) return {0, {}};

        std::string response_body;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_USERAGENT, user_agent_.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, webCrawlerWriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS,
                         static_cast<long>(retry_config_.timeout_ms));
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 10000L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 5L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);

        CURLcode res = curl_easy_perform(curl);
        long http_code = 0;
        if (res == CURLE_OK) {
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        }
        curl_easy_cleanup(curl);

        if (res != CURLE_OK) return {0, {}};
        return {static_cast<int>(http_code), std::move(response_body)};
#else
        return {0, {}};
#endif
    }

    SourceConfig   config_;
    std::string    seed_url_;
    int            max_depth_        = 3;
    size_t         max_pages_        = 0;
    std::string    user_agent_       = "ThemisDB-Crawler/1.0";
    bool           follow_sitemaps_  = true;
    bool           respect_robots_   = true;
    bool           same_domain_only_ = true;
    bool           initialized_      = false;
    RetryConfig    retry_config_;
    WebCrawlerConnector::HttpFetchFn mock_fetch_;
};

// ---------------------------------------------------------------------------
// WebCrawlerConnector public interface (delegates to Pimpl)
// ---------------------------------------------------------------------------

WebCrawlerConnector::WebCrawlerConnector()
    : impl_(std::make_unique<Impl>()) {}

WebCrawlerConnector::~WebCrawlerConnector() = default;

bool WebCrawlerConnector::initialize(const SourceConfig& config) {
    return impl_->initialize(config);
}

bool WebCrawlerConnector::isAvailable() const {
    return impl_->isAvailable();
}

size_t WebCrawlerConnector::getDocumentCount() const {
    return impl_->getDocumentCount();
}

IngestionStats WebCrawlerConnector::ingest(const std::string& target_collection,
                                            ProgressCallback progress_callback) {
    return impl_->ingest(target_collection, progress_callback);
}

void WebCrawlerConnector::setRetryConfig(const RetryConfig& config) {
    impl_->setRetryConfig(config);
}

void WebCrawlerConnector::setHttpFetchForTesting(HttpFetchFn fn) {
    impl_->setHttpFetchForTesting(std::move(fn));
}

} // namespace ingestion
} // namespace themis


