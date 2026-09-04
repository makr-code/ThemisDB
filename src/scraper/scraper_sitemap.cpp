/**
 * @file scraper_sitemap.cpp
 * @brief Implementation of SitemapCrawler — fetch and parse XML sitemaps.
 * @version 1.0.0
 */

#include "scraper/scraper_sitemap.h"

#include <algorithm>
#include <stdexcept>

namespace themis {
namespace scraper {

// ============================================================================
// Construction
// ============================================================================

SitemapCrawler::SitemapCrawler(FetchFn fetch_fn,
                                std::size_t max_urls,
                                std::string user_agent)
    : fetch_fn_(std::move(fetch_fn))
    , max_urls_(max_urls)
    , user_agent_(std::move(user_agent)) {}

// ============================================================================
// Parser helpers
// ============================================================================

/// Extract all `<loc>text</loc>` values from @p xml_content.
/*static*/ std::vector<std::string> SitemapCrawler::parseLocEntries(
        const std::string& xml_content) {
    std::vector<std::string> urls;
    const std::string open_tag  = "<loc>";
    const std::string close_tag = "</loc>";
    const std::size_t open_len  = open_tag.size();
    const std::size_t close_len = close_tag.size();

    std::size_t pos = 0;
    while (pos < xml_content.size()) {
        const std::size_t open_pos = xml_content.find(open_tag, pos);
        if (open_pos == std::string::npos) {
          break;
        }

        const std::size_t value_start = open_pos + open_len;
        const std::size_t close_pos   = xml_content.find(close_tag, value_start);
        if (close_pos == std::string::npos) {
          break;
        }

        std::string url = xml_content.substr(value_start, close_pos - value_start);
        // Trim whitespace around the URL
        const auto begin = url.find_first_not_of(" \t\r\n");
        if (begin != std::string::npos) {
            const auto end = url.find_last_not_of(" \t\r\n");
            url = url.substr(begin, end - begin + 1);
        } else {
            url.clear();
        }
        if (!url.empty()) {
            urls.push_back(std::move(url));
        }
        pos = close_pos + close_len;
    }
    return urls;
}

/*static*/ bool SitemapCrawler::isSitemapIndex(const std::string& xml_content) {
    return xml_content.find("<sitemapindex") != std::string::npos;
}

// ============================================================================
// fetchUrls()
// ============================================================================

std::vector<std::string> SitemapCrawler::fetchUrls(
        const std::string& sitemap_url) const {
    if (!fetch_fn_) return {};

    std::string xml;
    try {
        xml = fetch_fn_(sitemap_url, user_agent_);
    } catch (...) {
        return {};
    }
    if (xml.empty()) return {};

    std::vector<std::string> result;

    if (isSitemapIndex(xml)) {
        // Child sitemap URLs are listed as <loc> inside <sitemap> elements.
        // parseLocEntries extracts all <loc> values regardless of parent element.
        const auto child_urls = parseLocEntries(xml);
        for (const auto& child_url : child_urls) {
            if (result.size() >= max_urls_) {
              break;
            }
            std::string child_xml;
            try {
                child_xml = fetch_fn_(child_url, user_agent_);
            } catch (...) {
                continue;
            }
            if (child_xml.empty()) {
              continue;
            }
            const auto child_locs = parseLocEntries(child_xml);
            for (const auto& loc : child_locs) {
                if (result.size() >= max_urls_) {
                  break;
                }
                result.push_back(loc);
            }
        }
    } else {
        const auto locs = parseLocEntries(xml);
        const auto take = std::min(locs.size(), max_urls_);
        result.assign(locs.begin(), locs.begin() + static_cast<std::ptrdiff_t>(take));
    }

    // De-duplicate while preserving order
    std::vector<std::string> seen;
    std::vector<std::string> unique;
    unique.reserve(result.size());
    for (auto& u : result) {
        if (std::find(seen.begin(), seen.end(), u) == seen.end()) {
            seen.push_back(u);
            unique.push_back(std::move(u));
        }
    }
    return unique;
}

} // namespace scraper
} // namespace themis
