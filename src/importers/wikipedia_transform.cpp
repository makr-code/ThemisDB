/**
 * @file wikipedia_transform.cpp
 * @brief Wikipedia transformation stage implementation.
 *
 * Implements WikipediaTransformer: wikitext parsing, section extraction,
 * category mapping, and outbound link resolution.
 */

#include "importers/wikipedia_pipeline.hpp"

#include "importers/wikipedia_transform.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <regex>
#include <sstream>

namespace themis::importers {

namespace {
uint64_t fnv1a64(std::string_view text) {
    uint64_t hash = 1469598103934665603;
    for (const unsigned char ch : text) {
        hash ^= ch;
        hash *= 1099511628211;
    }
    return hash;
}

std::string hex64([[maybe_unused]] uint64_t value) {
    std::ostringstream output = {};
    output << std::hex << value;
    return output.str();
}

std::string trim(std::string text) {
    const auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    text.erase(text.begin(), std::find_if(text.begin(), text.end(), not_space));
    text.erase(std::find_if(text.rbegin(), text.rend(), not_space).base(), text.end());
    return text;
}
} // namespace

std::string WikipediaTransform::normalizeTitle(std::string_view title) {
    std::string normalized(title);
    std::replace(normalized.begin(), normalized.end(), '_', ' ');
    normalized = trim(normalized);
    bool previous_space = false;
    std::string compact = {};
    compact.reserve(normalized.size());
    for (const char ch : normalized) {
        const bool is_space = std::isspace(static_cast<unsigned char>(ch)) != 0;
        if (is_space) {
            if (!previous_space) {
                compact.push_back(' ');
            }
            previous_space = true;
            continue;
        }
        compact.push_back(ch);
        previous_space = false;
    }
    if (!compact.empty()) {
        compact.front() = static_cast<char>(std::toupper(static_cast<unsigned char>(compact.front())));
    }
    return compact;
}

std::string WikipediaTransform::decodeXmlEntities(std::string_view text) {
    std::string output(text);
    const std::pair<const char*, const char*> entities[] = {
        {"&amp;", "&"},
        {"&lt;", "<"},
        {"&gt;", ">"},
        {"&quot;", "\""},
        {"&apos;", "'"}
    };
    for (const auto& [needle, replacement] : entities) {
        std::string::size_type pos = 0;
        while ((pos = output.find(needle, pos)) != std::string::npos) {
            output.replace(pos, std::strlen(needle), replacement);
            pos += std::strlen(replacement);
        }
    }
    return output;
}

std::string WikipediaTransform::canonicalizeText(std::string_view text) {
    auto normalized = decodeXmlEntities(text);
    normalized = trim(std::move(normalized));
    return normalized;
}

std::string WikipediaTransform::checksumHex(std::string_view text) {
    return hex64(fnv1a64(text));
}

std::vector<WikipediaLinkRecord> WikipediaTransform::extractLinks(
    const WikipediaPageRecord& page,
    const WikipediaRevisionRecord& revision) {
    std::vector<WikipediaLinkRecord> links;
    static const std::regex link_regex(R"(\[\[([^\]|#]+)(?:#[^\]|]+)?(?:\|[^\]]*)?\]\])");
    auto begin = std::sregex_iterator(revision.text.begin(), revision.text.end(), link_regex);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        const std::string target = normalizeTitle((*it)[1].str());
        if (target.rfind("Category:", 0) == 0 || target.rfind("File:", 0) == 0) {
            continue;
        }
        links.push_back({page.page_id, target, 0, "LINKS_TO"});
    }
    return links;
}

std::vector<WikipediaCategoryRecord> WikipediaTransform::extractCategories(
    const WikipediaPageRecord& page,
    const WikipediaRevisionRecord& revision) {
    std::vector<WikipediaCategoryRecord> categories;
    static const std::regex category_regex(R"(\[\[Category:([^\]|]+))", std::regex::icase);
    auto begin = std::sregex_iterator(revision.text.begin(), revision.text.end(), category_regex);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        categories.push_back({page.page_id, normalizeTitle((*it)[1].str())});
    }
    return categories;
}

std::optional<WikipediaRedirectRecord> WikipediaTransform::extractRedirect(
    const WikipediaPageRecord& page,
    const WikipediaRevisionRecord& /*revision*/) {
    if (!page.is_redirect || page.redirect_title.empty()) {
        return std::nullopt;
    }
    return WikipediaRedirectRecord{page.page_id, normalizeTitle(page.redirect_title)};
}

std::vector<WikipediaProcessEvent> WikipediaTransform::buildProcessEvents(
    const WikipediaPageRecord& page,
    const std::vector<WikipediaRevisionRecord>& revisions) {
    std::vector<WikipediaProcessEvent> events = {};

    if (!revisions.empty()) {
        events.push_back({
            page.page_id,
            "page_created",
            revisions.front().timestamp,
            json{{"title", page.title}, {"revision_id", revisions.front().revision_id}}
        });
    }
    for (const auto& revision : revisions) {
        events.push_back({
            page.page_id,
            "revision_ingested",
            revision.timestamp,
            json{{"revision_id", revision.revision_id}, {"sha1", revision.sha1}}
        });
    }
    if (page.is_redirect) {
        events.push_back({
            page.page_id,
            "page_redirected",
            page.touched_at,
            json{{"target_title", page.redirect_title}}
        });
    }
    return events;
}

std::vector<WikipediaTimeSeriesMetric> WikipediaTransform::buildTimeSeriesMetrics(
    const WikipediaPageRecord& page,
    const std::vector<WikipediaRevisionRecord>& revisions) {
    std::vector<WikipediaTimeSeriesMetric> metrics = {};

    metrics.reserve(revisions.size());
    for (const auto& revision : revisions) {
        const std::string bucket = revision.timestamp.size() >= 10
            ? revision.timestamp.substr(0, 10)
            : revision.timestamp;
        metrics.push_back({page.page_id, "revisions/day", bucket, 1.0});
    }
    return metrics;
}

std::vector<WikipediaVectorRecord> WikipediaTransform::buildVectorRecords(
    const WikipediaPageRecord& page,
    const WikipediaRevisionRecord& revision,
    const std::string& embedding_model,
    bool embedding_enabled) {
    std::string content = page.title;
    if (!revision.text.empty()) {
        content.append("\n\n");
        content.append(canonicalizeText(revision.text.substr(0, 512)));
    }
    return {WikipediaVectorRecord{page.page_id, content, embedding_model, !embedding_enabled}};
}

void WikipediaIngestionPipeline::applyParsedPage(
    const WikipediaParsedPage& parsed_page,
    ImportStats& stats,
    const ImportOptions& options,
    bool incremental) {
    ++stats.total_records;

    const auto existing_page = snapshot_.pages.find(parsed_page.page.page_id);
    const bool same_revision = snapshot_.revisions.count(parsed_page.revision.revision_id) > 0;
    const bool unchanged_page = existing_page != snapshot_.pages.end() &&
        existing_page->second.checksum == parsed_page.page.checksum;

    if ((same_revision || unchanged_page) && options.skip_duplicates) {
        ++stats.skipped_records;
        return;
    }

    removeExistingPageDerivedRows(parsed_page.page.page_id);

    snapshot_.pages[parsed_page.page.page_id] = parsed_page.page;
    snapshot_.revisions[parsed_page.revision.revision_id] = parsed_page.revision;

    if ([[maybe_unused]] options.streaming_row_callback) {
        options.streaming_row_callback("wiki_page", parsed_page.page.toJson());
        options.streaming_row_callback("wiki_revision", parsed_page.revision.toJson());
    }

    auto links = WikipediaTransform::extractLinks(parsed_page.page, parsed_page.revision);
    for (auto& link : links) {
        snapshot_.links.push_back(link);
        if ([[maybe_unused]] options.streaming_row_callback) {
            options.streaming_row_callback("wiki_link", link.toJson());
        }
    }

    auto categories = WikipediaTransform::extractCategories(parsed_page.page, parsed_page.revision);
    for (auto& category : categories) {
        snapshot_.categories.push_back(category);
        if ([[maybe_unused]] options.streaming_row_callback) {
            options.streaming_row_callback("wiki_category", category.toJson());
        }
    }

    if (auto redirect = WikipediaTransform::extractRedirect(parsed_page.page, parsed_page.revision)) {
        snapshot_.redirects.push_back(*redirect);
        if ([[maybe_unused]] options.streaming_row_callback) {
            options.streaming_row_callback("wiki_redirect", redirect->toJson());
        }
    }

    markDirtyPage(parsed_page.page.page_id, incremental ? "delta-upsert" : "full-import");
    checkpoint_state_.last_page_id = parsed_page.page.page_id;
    checkpoint_state_.last_page_title = parsed_page.page.title;
    ++stats.imported_records;
    stats.tables_processed = 6;
    stats.relationships_processed = static_cast<int>(snapshot_.links.size()) + static_cast<int>(snapshot_.categories.size()) + static_cast<int>(snapshot_.redirects.size()) ;
}

} // namespace themis::importers
