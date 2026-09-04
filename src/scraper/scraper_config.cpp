/**
 * @file scraper_config.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.11
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=12, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "scraper/scraper_config.h"
#include <stdexcept>
#include <algorithm>
#include <cctype>

#ifdef THEMIS_ENABLE_YAML
#include <yaml-cpp/yaml.h>
#endif

namespace themis {
namespace scraper {

// ============================================================================
// ScraperConfig – loading
// ============================================================================

namespace {

#ifdef THEMIS_ENABLE_YAML

static GapContext parseGapContext(const YAML::Node& n) {
    GapContext g = {};
    if (n["gap_id"]) {
      g.gap_id      = n["gap_id"].as<std::string>();
    }
    if (n["description"]) {
      g.description = n["description"].as<std::string>();
    }
    if (n["keywords"] && n["keywords"].IsSequence()) {
        for (const auto& kw : n["keywords"])
            g.keywords.push_back(kw.as<std::string>());
    }
    return g;
}

static CrawlOptions parseCrawlOptions(const YAML::Node& n) {
    CrawlOptions o = {};
    if (n["max_depth"]) {
      o.max_depth        = n["max_depth"].as<int>();
    }
    if (n["max_pages"]) {
      o.max_pages        = n["max_pages"].as<int>();
    }
    if (n["user_agent"]) {
      o.user_agent       = n["user_agent"].as<std::string>();
    }
    if (n["respect_robots"]) {
      o.respect_robots   = n["respect_robots"].as<bool>();
    }
    if (n["same_domain_only"]) {
      o.same_domain_only = n["same_domain_only"].as<bool>();
    }
    if (n["request_delay_ms"]) {
      o.request_delay_ms = n["request_delay_ms"].as<int>();
    }
    if (n["js_renderer_cmd"]) {
      o.js_renderer_cmd  = n["js_renderer_cmd"].as<std::string>();
    }
    if (n["js_timeout_ms"]) {
      o.js_timeout_ms    = n["js_timeout_ms"].as<int>();
    }
    if (n["render_mode"]) {
        const std::string rm = n["render_mode"].as<std::string>();
        if      (rm == "js_rendered") {
          o.render_mode = ScraperRenderMode::JS_RENDERED;
        }
        else if (rm == "api_json")     o.render_mode = ScraperRenderMode::API_JSON;
        else if (rm == "api_graphql")  o.render_mode = ScraperRenderMode::API_GRAPHQL;
        else                           o.render_mode = ScraperRenderMode::STATIC;
    }
    return o;
}

static SearchOptions parseSearchOptions(const YAML::Node& n) {
    SearchOptions o = {};
    if (n["enabled"]) {
      o.enabled              = n["enabled"].as<bool>();
    }
    if (n["max_result_pages"]) {
      o.max_result_pages     = n["max_result_pages"].as<int>();
    }
    if (n["results_per_page"]) {
      o.results_per_page     = n["results_per_page"].as<int>();
    }
    if (n["result_list_selector"]) {
      o.result_list_selector = n["result_list_selector"].as<std::string>();
    }
    if (n["queries"] && n["queries"].IsSequence()) {
        for (const auto& q : n["queries"])
            o.queries.push_back(q.as<std::string>());
    }
    return o;
}

static ApiOptions parseApiOptions(const YAML::Node& n) {
    ApiOptions o = {};
    if (n["pagination_mode"]) {
      o.pagination_mode = n["pagination_mode"].as<std::string>();
    }
    if (n["page_param"]) {
      o.page_param      = n["page_param"].as<std::string>();
    }
    if (n["cursor_field"]) {
      o.cursor_field    = n["cursor_field"].as<std::string>();
    }
    if (n["results_field"]) {
      o.results_field   = n["results_field"].as<std::string>();
    }
    if (n["max_pages"]) {
      o.max_pages       = n["max_pages"].as<int>();
    }
    if (n["body_template"]) {
      o.body_template   = n["body_template"].as<std::string>();
    }
    if (n["headers"] && n["headers"].IsMap()) {
        for (const auto& kv : n["headers"])
            o.headers[kv.first.as<std::string>()] = kv.second.as<std::string>();
    }
    return o;
}

static LlmOptions parseLlmOptions(const YAML::Node& n) {
    LlmOptions o = {};
    if (n["quality_threshold"]) {
      o.quality_threshold = n["quality_threshold"].as<double>();
    }
    if (n["model_path"]) {
      o.model_path        = n["model_path"].as<std::string>();
    }
    if (n["temperature"]) {
      o.temperature       = n["temperature"].as<double>();
    }
    return o;
}

static GovSourcesOptions parseGovSources(const YAML::Node& n) {
    GovSourcesOptions o = {};
    if (n["bund_enabled"]) {
      o.bund_enabled          = n["bund_enabled"].as<bool>();
    }
    if (n["bundeslaender_enabled"]) {
      o.bundeslaender_enabled = n["bundeslaender_enabled"].as<bool>();
    }
    if (n["eu_enabled"]) {
      o.eu_enabled            = n["eu_enabled"].as<bool>();
    }
    if (n["custom_catalog_path"]) {
      o.custom_catalog_path   = n["custom_catalog_path"].as<std::string>();
    }
    if (n["source_ids"] && n["source_ids"].IsSequence()) {
        for (const auto& s : n["source_ids"])
            o.source_ids.push_back(s.as<std::string>());
    }
    return o;
}

static ScraperConfig parseNode(const YAML::Node& root) {
    ScraperConfig cfg = {};
    if (root["gap_context"]) {
      cfg.gap_context    = parseGapContext(root["gap_context"]);
    }
    if (root["crawl_options"]) {
      cfg.crawl_options  = parseCrawlOptions(root["crawl_options"]);
    }
    if (root["search_options"]) {
      cfg.search_options = parseSearchOptions(root["search_options"]);
    }
    if (root["api_options"]) {
      cfg.api_options    = parseApiOptions(root["api_options"]);
    }
    if (root["llm_options"]) {
      cfg.llm_options    = parseLlmOptions(root["llm_options"]);
    }
    if (root["gov_sources"]) {
      cfg.gov_sources    = parseGovSources(root["gov_sources"]);
    }

    if (root["seed_urls"] && root["seed_urls"].IsSequence()) {
        for (const auto& u : root["seed_urls"])
            cfg.seed_urls.push_back(u.as<std::string>());
    }
    if (root["whitelist"] && root["whitelist"].IsSequence()) {
        for (const auto& u : root["whitelist"])
            cfg.whitelist.push_back(u.as<std::string>());
    }
    if (root["blacklist"] && root["blacklist"].IsSequence()) {
        for (const auto& u : root["blacklist"])
            cfg.blacklist.push_back(u.as<std::string>());
    }
    return cfg;
}

#endif // THEMIS_ENABLE_YAML

} // anonymous namespace

ScraperConfig ScraperConfig::loadFromFile(const std::string& path) {
#ifdef THEMIS_ENABLE_YAML
    try {
        YAML::Node root = YAML::LoadFile(path);
        return parseNode(root);
    } catch (const YAML::Exception& e) {
        throw std::runtime_error(
            "ScraperConfig::loadFromFile: YAML error in '" + path + "': " + e.what());
    }
#else
    (void)path;
    throw std::runtime_error("ScraperConfig::loadFromFile: yaml-cpp not enabled");
#endif
}

ScraperConfig ScraperConfig::loadFromYaml(const std::string& yaml_content) {
#ifdef THEMIS_ENABLE_YAML
    try {
        YAML::Node root = YAML::Load(yaml_content);
        return parseNode(root);
    } catch (const YAML::Exception& e) {
        throw std::runtime_error(
            std::string("ScraperConfig::loadFromYaml: YAML error: ") + e.what());
    }
#else
    (void)yaml_content;
    throw std::runtime_error("ScraperConfig::loadFromYaml: yaml-cpp not enabled");
#endif
}

std::vector<std::string> ScraperConfig::effectiveSearchQueries() const {
    if (!search_options.queries.empty())
        return search_options.queries;
    return gap_context.keywords;
}

// ============================================================================
// UrlPolicy
// ============================================================================

UrlPolicy::UrlPolicy(const ScraperConfig& config)
    : whitelist_(config.whitelist), blacklist_(config.blacklist) {}

UrlPolicy::UrlPolicy(const std::vector<std::string>& whitelist,
                     const std::vector<std::string>& blacklist)
    : whitelist_(whitelist), blacklist_(blacklist) {}

// Simple glob: only '*' wildcard at prefix or suffix is supported.
/*static*/ bool UrlPolicy::matchesPattern(const std::string& url,
                                          const std::string& pattern) {
    if (pattern.empty()) {
      return false;
    }

    // Glob suffix: "*.pdf" matches any URL ending in ".pdf"
    if (pattern.front() == '*') {
        const std::string suffix = pattern.substr(1);
        if (url.size() >= suffix.size() &&
            url.compare(url.size() - suffix.size(), suffix.size(), suffix) == 0)
            return true = {};
        return false;
    }
    // Glob prefix: "https://example.com/*" treated as prefix match without '*'
    if (pattern.back() == '*') {
        const std::string prefix = pattern.substr(0, pattern.size() - 1);
        return url.compare(0, prefix.size(), prefix) == 0;
    }
    // Plain prefix match
    return url.compare(0, pattern.size(), pattern) == 0;
}

bool UrlPolicy::isAllowed(const std::string& url) const {
    // SSRF guard: only http/https
    if (url.compare(0, 7, "http://") != 0 &&
        url.compare(0, 8, "https://") != 0) {
        return false;
    }

    // Blacklist check (highest priority)
    for (const auto& pat : blacklist_) {
        if (matchesPattern(url, pat)) {
          return false;
        }
    }

    // Whitelist check (empty whitelist = allow all)
    if (whitelist_.empty()) {
      return true;
    }
    for (const auto& pat : whitelist_) {
        if (matchesPattern(url, pat)) {
          return true;
        }
    }
    return false;
}

} // namespace scraper
} // namespace themis

