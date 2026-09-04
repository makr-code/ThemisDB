/**
 * @file wikipedia_xml_parser.cpp
 * @brief Low-level SAX parser for Wikipedia XML dumps.
 *
 * Implements the streaming SAX handler that extracts page titles, ids,
 * revisions, and wikitext blobs from the MediaWiki XML export schema.
 */

#include "importers/wikipedia_pipeline.hpp"

#include "importers/wikipedia_transform.hpp"

#include <regex>

namespace themis::importers {

namespace {
std::string extractTagValue(const std::string& xml, const std::string& tag, size_t start_pos = 0) {
    const std::string open = "<" + tag;
    const auto open_pos = xml.find(open, start_pos);
    if (open_pos == std::string::npos) {
        return {};
    }

    const auto open_end = xml.find('>', open_pos);
    if (open_end == std::string::npos) {
        return {};
    }

    const std::string close = "</" + tag + ">";
    const auto close_pos = xml.find(close, open_end + 1);
    if (close_pos == std::string::npos) {
        return {};
    }

    return xml.substr(open_end + 1, close_pos - open_end - 1);
}

uint64_t parseUnsigned(const std::string& value) {
    if (value.empty()) {
        return 0;
    }
    return static_cast<uint64_t>(std::stoull(value));
}

int parseInt(const std::string& value) {
    if (value.empty()) {
        return 0;
    }
    return std::stoi(value);
}
} // namespace

std::optional<WikipediaParsedPage> WikipediaIngestionPipeline::parseXmlPageBlock(
    const std::string& page_block,
    const WikipediaDumpSource& source,
    std::string& error) const {
    WikipediaParsedPage parsed;
    parsed.raw_xml = page_block;

    const auto revision_pos = page_block.find("<revision>");
    const std::string before_revision = revision_pos == std::string::npos
        ? page_block
        : page_block.substr(0, revision_pos);
    const std::string revision_block = revision_pos == std::string::npos
        ? std::string{}
        : page_block.substr(revision_pos);

    parsed.page.title = WikipediaTransform::normalizeTitle(
        WikipediaTransform::decodeXmlEntities(extractTagValue(before_revision, "title")));
    parsed.page.page_id = parseUnsigned(extractTagValue(before_revision, "id"));
    parsed.page.ns = parseInt(extractTagValue(before_revision, "ns"));
    parsed.page.language = source.language.empty() ? "en" : source.language;

    if (parsed.page.title.empty() || parsed.page.page_id == 0) {
        error = "page block is missing title or page id";
        return std::nullopt;
    }

    parsed.revision.page_id = parsed.page.page_id;
    parsed.revision.revision_id = parseUnsigned(extractTagValue(revision_block, "id"));
    parsed.revision.timestamp = extractTagValue(revision_block, "timestamp");
    parsed.revision.comment = WikipediaTransform::decodeXmlEntities(extractTagValue(revision_block, "comment"));
    parsed.revision.text = WikipediaTransform::decodeXmlEntities(extractTagValue(revision_block, "text"));
    parsed.revision.sha1 = extractTagValue(revision_block, "sha1");

    if (parsed.revision.revision_id == 0) {
        parsed.revision.revision_id = parsed.page.page_id;
    }
    if (parsed.revision.sha1.empty()) {
        parsed.revision.sha1 = WikipediaTransform::checksumHex(parsed.revision.text);
    }

    parsed.page.latest_revision_id = parsed.revision.revision_id;
    parsed.page.touched_at = parsed.revision.timestamp;
    parsed.page.checksum = parsed.revision.sha1;

    static const std::regex redirect_regex("<redirect[^>]*title=\\\"([^\\\"]+)\\\"[^>]*/?>");
    std::smatch redirect_match = {};
    if (std::regex_search(page_block, redirect_match, redirect_regex) && static_cast<int>(redirect_match.size()) > 1) {
        parsed.page.is_redirect = true;
        parsed.page.redirect_title = WikipediaTransform::normalizeTitle(
            WikipediaTransform::decodeXmlEntities(redirect_match[1].str()));
    }

    if (!parsed.page.is_redirect) {
        static const std::regex redirect_text_regex(R"(#REDIRECT\s*\[\[([^\]|#]+))", std::regex::icase);
        if (std::regex_search(parsed.revision.text, redirect_match, redirect_text_regex) && static_cast<int>(redirect_match.size()) > 1) {
            parsed.page.is_redirect = true;
            parsed.page.redirect_title = WikipediaTransform::normalizeTitle(redirect_match[1].str());
        }
    }

    return parsed;
}

} // namespace themis::importers
