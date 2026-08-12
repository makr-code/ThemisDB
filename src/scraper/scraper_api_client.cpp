/**
 * @file scraper_api_client.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.11
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "scraper/scraper_api_client.h"
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <algorithm>

#ifdef THEMIS_ENABLE_CURL
#include <curl/curl.h>
#endif

#include <nlohmann/json.hpp>

namespace themis {
namespace scraper {

using json = nlohmann::json;

// ============================================================================
// Construction
// ============================================================================

HttpScraperApiClient::HttpScraperApiClient(HttpFetchFn fetch_fn)
    : fetch_fn_(std::move(fetch_fn)) {
    if (!fetch_fn_) {
        fetch_fn_ = &HttpScraperApiClient::curlFetch;
    }
}

// ============================================================================
// URL / body helpers
// ============================================================================

namespace {
std::string urlEncodeComponent(const std::string& s) {
    std::ostringstream out;
    for (unsigned char c : s) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            out << c;
        } else {
            out << '%'
                << std::hex << std::uppercase
                << std::setw(2) << std::setfill('0')
                << static_cast<int>(c);
        }
    }
    return out.str();
}

std::string applyTemplate(const std::string& tmpl,
                           const std::string& query,
                           int page,
                           const std::string& cursor) {
    std::string out = tmpl;
    auto replace = [&](const std::string& key, const std::string& val) {
        std::size_t pos = 0;
        while ((pos = out.find(key, pos)) != std::string::npos) {
            out.replace(pos, key.size(), val);
            pos += val.size();
        }
    };
    replace("{{QUERY}}",  query);
    replace("{{PAGE}}",   std::to_string(page));
    replace("{{CURSOR}}", cursor);
    return out;
}
} // anonymous namespace

std::string HttpScraperApiClient::buildGetUrl(
        const ApiEndpointConfig& cfg,
        const std::string& query,
        int page, int offset,
        const std::string& cursor) const {
    std::ostringstream url;
    url << cfg.url;
    bool first = (cfg.url.find('?') == std::string::npos);
    auto app = [&](const std::string& k, const std::string& v) {
        url << (first ? "?" : "&") << urlEncodeComponent(k)
            << "=" << urlEncodeComponent(v);
        first = false;
    };
    if (!cfg.search_param.empty() && !query.empty())
        app(cfg.search_param, query);
    if (cfg.pagination_mode == "page" && page > 1)
        app(cfg.page_param, std::to_string(page));
    if (cfg.pagination_mode == "offset" && offset > 0)
        app("offset", std::to_string(offset));
    if (cfg.pagination_mode == "cursor" && !cursor.empty())
        app(cfg.page_param, cursor);
    if (cfg.page_size > 0)
        app(cfg.page_size_param, std::to_string(cfg.page_size));
    return url.str();
}

std::string HttpScraperApiClient::buildBody(
        const ApiEndpointConfig& cfg,
        const std::string& query,
        int page,
        const std::string& cursor) const {
    if (!cfg.body_template.empty())
        return applyTemplate(cfg.body_template, query, page, cursor);
    // Default: JSON body with query
    json body;
    if (!query.empty())     body["query"] = query;
    if (page > 1)           body["page"]  = page;
    if (!cursor.empty())    body["cursor"] = cursor;
    return body.dump();
}

// ============================================================================
// JSON parsing
// ============================================================================

/*static*/ std::string HttpScraperApiClient::flattenJson(const std::string& json_text) {
    try {
        const json j = json::parse(json_text);
        std::ostringstream out;
        std::function<void(const json&)> walk = [&](const json& v) {
            if (v.is_string()) { out << v.get<std::string>() << ' '; }
            else if (v.is_number()) { out << v.dump() << ' '; }
            else if (v.is_array()) { for (const auto& e : v) walk(e); }
            else if (v.is_object()) { for (const auto& [k, val] : v.items()) { (void)k; walk(val); } }
        };
        walk(j);
        return out.str();
    } catch (...) {
        return json_text; // return raw on parse error
    }
}

/*static*/ std::vector<ApiResult> HttpScraperApiClient::parseResultsArray(
        const std::string& json_text,
        const std::string& results_field,
        const std::string& source_url) {
    std::vector<ApiResult> out;
    try {
        const json root = json::parse(json_text);
        const json* arr = nullptr;
        if (!results_field.empty() && root.contains(results_field))
            arr = &root[results_field];
        else if (root.is_array())
            arr = &root;

        if (!arr || !arr->is_array()) return out;
        for (const auto& item : *arr) {
            ApiResult r;
            r.url        = source_url;
            r.raw_json   = item.dump();
            r.extracted_text = flattenJson(r.raw_json);
            // Common field extraction
            for (const auto& key : {"url","link","href","uri"}) {
                if (item.contains(key) && item[key].is_string()) {
                    r.url = item[key].get<std::string>(); break;
                }
            }
            for (const auto& key : {"title","name","label","heading"}) {
                if (item.contains(key) && item[key].is_string()) {
                    r.title = item[key].get<std::string>(); break;
                }
            }
            for (const auto& key : {"date","created_at","published","datum"}) {
                if (item.contains(key) && item[key].is_string()) {
                    r.date = item[key].get<std::string>(); break;
                }
            }
            // Collect all string fields
            if (item.is_object()) {
                for (const auto& [k, v] : item.items()) {
                    if (v.is_string()) r.fields[k] = v.get<std::string>();
                }
            }
            out.push_back(std::move(r));
        }
    } catch (...) {}
    return out;
}

// ============================================================================
// Main fetch loop
// ============================================================================

std::vector<ApiResult> HttpScraperApiClient::fetchAll(
        const ApiEndpointConfig& cfg,
        const std::string& query) {
    std::vector<ApiResult> all;
    int  page   = 1;
    int  offset = 0;
    std::string cursor;
    const int max_pages = cfg.max_pages > 0 ? cfg.max_pages : 20;

    for (int pg = 0; pg < max_pages; ++pg) {
        // Build request
        const std::string url = (cfg.method == "POST")
            ? cfg.url
            : buildGetUrl(cfg, query, page, offset, cursor);
        const std::string body = (cfg.method == "POST")
            ? buildBody(cfg, query, page, cursor)
            : std::string{};

        std::string response;
        try {
            response = fetch_fn_(url, cfg.method, cfg.headers, body);
        } catch (const std::exception& e) {
            // Network error: stop pagination
            break;
        }
        if (response.empty()) break;

        auto batch = parseResultsArray(response, cfg.results_field, url);
        if (batch.empty()) break;
        all.insert(all.end(), batch.begin(), batch.end());

        // Advance pagination
        if (cfg.pagination_mode == "page") {
            ++page;
        } else if (cfg.pagination_mode == "offset") {
            offset += static_cast<int>(batch.size());
        } else if (cfg.pagination_mode == "cursor") {
            try {
                const json root = json::parse(response);
                if (!cfg.cursor_field.empty() && root.contains(cfg.cursor_field)) {
                    const auto& cv = root[cfg.cursor_field];
                    cursor = cv.is_string() ? cv.get<std::string>() : cv.dump();
                } else if (!cfg.next_url_field.empty() && root.contains(cfg.next_url_field)) {
                    const auto& nv = root[cfg.next_url_field];
                    const std::string next = nv.is_string() ? nv.get<std::string>() : std::string{};
                    if (next.empty()) break;
                    // Replace the URL for next iteration
                    // (use const_cast-friendly approach via local copy)
                    ApiEndpointConfig next_cfg = cfg;
                    next_cfg.url = next;
                    auto rest = fetchAll(next_cfg, query);
                    all.insert(all.end(), rest.begin(), rest.end());
                    break;
                } else {
                    break; // no cursor returned
                }
            } catch (...) { break; }
            if (cursor.empty()) break;
        } else {
            break; // "none"
        }

        // Check total: stop when we have everything
        try {
            const json root = json::parse(response);
            if (!cfg.total_field.empty() && root.contains(cfg.total_field)) {
                const int total = root[cfg.total_field].get<int>();
                if (static_cast<int>(all.size()) >= total) break;
            }
        } catch (...) {}
    }

    return all;
}

// ============================================================================
// libcurl HTTP backend
// ============================================================================

#ifdef THEMIS_ENABLE_CURL
namespace {
struct CurlWriteBuffer {
    std::string data;
    static std::size_t write(char* ptr, std::size_t size,
                             std::size_t nmemb, void* userdata) {
        auto* buf = static_cast<CurlWriteBuffer*>(userdata);
        buf->data.append(ptr, size * nmemb);
        return size * nmemb;
    }
};
} // anonymous namespace
#endif

/*static*/ std::string HttpScraperApiClient::curlFetch(
        const std::string& url,
        const std::string& method,
        const std::map<std::string, std::string>& headers,
        const std::string& body) {
#ifdef THEMIS_ENABLE_CURL
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("curl_easy_init failed");

    CurlWriteBuffer buf;
    curl_easy_setopt(curl, CURLOPT_URL,            url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  CurlWriteBuffer::write);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &buf);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT,        30L);

    curl_slist* hlist = nullptr;
    for (const auto& kv : headers)
        hlist = curl_slist_append(hlist, (kv.first + ": " + kv.second).c_str());
    if (hlist) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hlist);

    if (method == "POST") {
        curl_easy_setopt(curl, CURLOPT_POST,       1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE,
                         static_cast<long>(body.size()));
    }

    const CURLcode rc = curl_easy_perform(curl);
    if (hlist) curl_slist_free_all(hlist);
    curl_easy_cleanup(curl);

    if (rc != CURLE_OK)
        throw std::runtime_error(std::string("curl error: ") + curl_easy_strerror(rc));
    return buf.data;
#else
    (void)url; (void)method; (void)headers; (void)body;
    return {}; // curl not enabled
#endif
}

} // namespace scraper
} // namespace themis


