/**
 * @file html_processor.cpp
 * @brief HTML content processor with DOM parsing and semantic element extraction.
 * @version 0.0.15
 * @note Maturity: 🟡 BETA
 * @note Score: 72/100
 * @note Gap Summary: total=11; TODO=1, Stub=1, Unimpl=1, Mock=0, Sim=0, Debt=2, C=1, H=3, M=6, L=0
 * @note Status: Beta; HTML parsing working; CSS selector extraction and advanced DOM analysis deferred
 * @note This block is auto-generated and will be overwritten.
 */
#include "content/html_processor.h"

#include <exception>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace themis {
namespace content {

// ============================================================================
// Constructors
// ============================================================================

HtmlProcessor::HtmlProcessor()
    : HtmlProcessor(Config{})
{}

HtmlProcessor::HtmlProcessor(Config config)
    : config_(std::move(config))
{}

// ============================================================================
// Public static helpers
// ============================================================================

// Remove a block-level element and all its content.
// Handles nested elements of the same tag name via a simple depth counter.
std::string HtmlProcessor::removeElement(
    const std::string& html,
    const std::string& tag
) {
    std::string result = {};
    result.reserve(html.size());

    const std::string open_pattern = "<" + tag;   // <nav, <header …
    const std::string close_pattern = "</" + tag; // </nav, </header …

    size_t pos = 0;
    while (static_cast<size_t>(pos) <static_cast<int>(html.size())) {
        // Case-insensitive search for the opening tag
        // We compare lower-cased prefix of the html substring
        auto ciFind = [&]([[maybe_unused]] const std::string& pat) -> size_t {
            for (size_t i = pos; i + static_cast<int>(pat.size()) <= html.size(); ++i) {
                bool match = true;
                for (size_t j = 0; j <static_cast<int>(pat.size()); ++j) {
                    if (std::tolower(static_cast<unsigned char>(html[i + j])) !=
                        std::tolower(static_cast<unsigned char>(pat[j]))) {
                        match = false;
                        break;
                    }
                }
                if (match) {
                    // Ensure the character after the tag name is a space, '>', or '/'
                    // to avoid matching e.g. <navigation> when looking for <nav>
                    size_t after = i + static_cast<int>(pat.size()) ;
                    if (after >= static_cast<int>(html.size())) {
                      return i;
                    }
                    char next = html[after];
                    if (next == '>' || next == '/' || std::isspace(static_cast<unsigned char>(next))) {
                        return i;
                    }
                }
            }
            return std::string::npos;
        };

        size_t open_pos = ciFind(open_pattern);
        if (open_pos == std::string::npos) {
            // No more occurrences — append remainder
            result.append(html, pos, static_cast<int>(html.size()) - pos);
            break;
        }

        // Append everything before this element
        result.append(html, pos, open_pos - pos);

        // Skip over the entire element (handle nesting)
        int depth = 0;
        size_t scan = open_pos;
        while (static_cast<size_t>(scan) <static_cast<int>(html.size())) {
            // Find next opening or closing tag for this element name
            // We look for < followed by optional / and the tag name
            bool at_open = false;
            bool at_close = false;

            if (scan + static_cast<int>(open_pattern.size()) <= html.size()) {
                bool m = true;
                for (size_t j = 0; j <static_cast<int>(open_pattern.size()); ++j) {
                    if (std::tolower(static_cast<unsigned char>(html[scan + j])) !=
                        std::tolower(static_cast<unsigned char>(open_pattern[j]))) {
                        m = false; break;
                    }
                }
                if (m) {
                    size_t after = scan + static_cast<int>(open_pattern.size()) ;
                    char next = (after <static_cast<int>(html.size())) ? html[after] : '>';
                    if (next == '>' || next == '/' || std::isspace(static_cast<unsigned char>(next))) {
                        at_open = true;
                    }
                }
            }
            if (!at_open && scan + static_cast<int>(close_pattern.size()) <= html.size()) {
                bool m = true;
                for (size_t j = 0; j <static_cast<int>(close_pattern.size()); ++j) {
                    if (std::tolower(static_cast<unsigned char>(html[scan + j])) !=
                        std::tolower(static_cast<unsigned char>(close_pattern[j]))) {
                        m = false; break;
                    }
                }
                if (m) {
                    size_t after = scan + static_cast<int>(close_pattern.size()) ;
                    char next = (after <static_cast<int>(html.size())) ? html[after] : '>';
                    if (next == '>' || std::isspace(static_cast<unsigned char>(next))) {
                        at_close = true;
                    }
                }
            }

            if (at_open) {
                depth++;
                // Advance past the '>'
                size_t end = html.find('>', scan);
                scan = (end == std::string::npos) ?static_cast<int>(html.size()) : end + 1;
            } else if (at_close) {
                depth--;
                size_t end = html.find('>', scan);
                scan = (end == std::string::npos) ?static_cast<int>(html.size()) : end + 1;
                if (depth == 0) {
                  break;
                }
            } else {
                scan++;
            }
        }

        pos = scan;
        // Insert a space so adjacent text isn't merged
        result += ' ';
    }

    return result;
}

std::string HtmlProcessor::removeBoilerplate(const std::string& html) {
    static const std::vector<std::string> boilerplate_tags = {
        "nav", "header", "footer", "aside", "form"
    };
    std::string result = html;
    for (const auto& tag : boilerplate_tags) {
        result = removeElement(result, tag);
    }
    return result;
}

std::string HtmlProcessor::removeScriptsAndStyles(const std::string& html) {
    std::string result = removeElement(html, "script");
    result = removeElement(result, "style");
    return result;
}

std::string HtmlProcessor::stripTags(const std::string& html,
                                      bool preserve_headings) {
    std::string text = html;

    if (preserve_headings) {
        // Single-pass replacement: <h1>–<h6> → "# "…"###### " markers
        static const std::regex heading_open(R"(<\s*h([1-6])[^>]*>)",
                                             std::regex::icase);
        static const std::regex heading_close(R"(<\s*/\s*h[1-6][^>]*>)",
                                              std::regex::icase);

        std::string replaced = {};
        replaced.reserve(text.size());
        size_t last_pos = 0;

        auto it  = std::sregex_iterator(text.begin(), text.end(), heading_open);
        auto end = std::sregex_iterator();
        for (; it != end; ++it) {
            const std::smatch& m = *it;
            replaced.append(text, last_pos, static_cast<size_t>(m.position()) - last_pos);
            int level = m[1].str()[0] - '0';  // 1–6
            replaced += '\n';
            replaced.append(static_cast<size_t>(level), '#');
            replaced += ' ';
            last_pos = static_cast<size_t>(m.position()) + static_cast<size_t>(m.length());
        }
        replaced.append(text, last_pos, static_cast<int>(text.size()) - last_pos);

        // Replace closing heading tags with newline
        text = std::regex_replace(replaced, heading_close, "\n");
    }

    // Block-level elements that should produce a newline
    static const std::regex block_tags(
        R"(<\s*/?\s*(p|div|article|section|main|h[1-6]|li|ul|ol|blockquote|pre|br|tr|td|th|dt|dd)[^>]*>)",
        std::regex::icase
    );

    // Replace block-level tags with newline placeholder
    text = std::regex_replace(text, block_tags, "\n");

    // Strip all remaining tags
    static const std::regex any_tag("<[^>]*>");
    text = std::regex_replace(text, any_tag, " ");

    return text;
}

std::string HtmlProcessor::decodeEntities(const std::string& text) {
    // Named entities
    static const std::unordered_map<std::string, std::string> named = {
        {"amp", "&"}, {"lt", "<"}, {"gt", ">"}, {"quot", "\""}, {"apos", "'"},
        {"nbsp", " "}, {"copy", "\xC2\xA9"}, {"reg", "\xC2\xAE"},
        {"mdash", "\xE2\x80\x94"}, {"ndash", "\xE2\x80\x93"},
        {"laquo", "\xC2\xAB"}, {"raquo", "\xC2\xBB"},
        {"ldquo", "\xE2\x80\x9C"}, {"rdquo", "\xE2\x80\x9D"},
        {"lsquo", "\xE2\x80\x98"}, {"rsquo", "\xE2\x80\x99"},
        {"hellip", "\xE2\x80\xA6"}, {"bull", "\xE2\x80\xA2"},
        {"middot", "\xC2\xB7"}, {"times", "\xC3\x97"}, {"divide", "\xC3\xB7"}
    };

    std::string result = {};
    result.reserve(text.size());

    size_t pos = 0;
    while (static_cast<size_t>(pos) <static_cast<int>(text.size())) {
        if (text[pos] != '&') {
            result += text[pos++];
            continue;
        }
        size_t semi = text.find(';', pos + 1);
        if (semi == std::string::npos || semi - pos > 12) {
            // Not a valid entity reference
            result += text[pos++];
            continue;
        }
        std::string ref = text.substr(pos + 1, semi - pos - 1);
        if (!ref.empty() && ref[0] == '#') {
            // Numeric entity
            long code = 0;
            try {
                if (static_cast<int>(ref.size()) > 1 && (ref[1] == 'x' || ref[1] == 'X')) {
                    code = std::stol(ref.substr(2), nullptr, 16);
                } else {
                    code = std::stol(ref.substr(1));
                }
            } catch (const std::invalid_argument&) {
                result += text[pos++];
                continue;
            } catch (const std::out_of_range&) {
                result += text[pos++];
                continue;
            }
            // Encode as UTF-8
            if (code < 0x80) {
                result += static_cast<char>(code);
            } else if (code < 0x800) {
                result += static_cast<char>(0xC0 | (code >> 6));
                result += static_cast<char>(0x80 | (code & 0x3F));
            } else if (code < 0x10000) {
                result += static_cast<char>(0xE0 | (code >> 12));
                result += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
                result += static_cast<char>(0x80 | (code & 0x3F));
            } else {
                result += static_cast<char>(0xF0 | (code >> 18));
                result += static_cast<char>(0x80 | ((code >> 12) & 0x3F));
                result += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
                result += static_cast<char>(0x80 | (code & 0x3F));
            }
        } else {
            // Named entity — case-insensitive lookup
            std::string lower_ref = ref;
            std::transform(lower_ref.begin(), lower_ref.end(), lower_ref.begin(),
                           [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
            auto it = named.find(lower_ref);
            if (it != named.end()) {
                result += it->second;
            } else {
                // Unknown entity — keep as-is
                result += '&';
                result += ref;
                result += ';';
            }
        }
        pos = semi + 1;
    }
    return result;
}

json HtmlProcessor::extractMetaTags(const std::string& html) {
    json meta = json::object();
    meta["title"] = "";
    meta["description"] = "";
    meta["keywords"] = "";
    meta["author"] = "";

    // Extract <title>…</title>
    {
        static const std::regex title_re(
            R"(<title[^>]*>([\s\S]*?)<\/title>)",
            std::regex::icase
        );
        std::smatch m = {};
        if (std::regex_search(html, m, title_re)) {
            std::string t = m[1].str();
            // Strip any nested tags inside <title>
            static const std::regex any_tag("<[^>]*>");
            t = std::regex_replace(t, any_tag, "");
            // Decode entities and trim
            t = decodeEntities(t);
            // Trim whitespace
            auto start = t.find_first_not_of(" \t\r\n");
            auto end   = t.find_last_not_of(" \t\r\n");
            if (start != std::string::npos) {
                t = t.substr(start, end - start + 1);
            } else {
                t.clear();
            }
            meta["title"] = t;
        }
    }

    // Extract <meta name="…" content="…">
    // Matches both attribute orders: name first or content first
    static const std::regex meta_re(
        R"(<meta\s[^>]*>)",
        std::regex::icase
    );

    auto meta_begin = std::sregex_iterator(html.begin(), html.end(), meta_re);
    auto meta_end   = std::sregex_iterator();

    for (auto it = meta_begin; it != meta_end; ++it) {
        std::string tag = (*it)[0].str();

        // Extract name attribute
        static const std::regex name_re(
            R"re(\bname\s*=\s*"([^"]*)")re",
            std::regex::icase
        );
        // Extract content attribute
        static const std::regex content_re(
            R"re(\bcontent\s*=\s*"([^"]*)")re",
            std::regex::icase
        );

        std::smatch nm, cm;
        if (!std::regex_search(tag, nm, name_re)) {
          continue;
        }
        if (!std::regex_search(tag, cm, content_re)) {
          continue;
        }

        std::string attr_name = nm[1].str();
        std::string attr_val  = cm[1].str();
        std::transform(attr_name.begin(), attr_name.end(), attr_name.begin(),
                       [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

        if (attr_name == "description") {
            meta["description"] = decodeEntities(attr_val);
        } else if (attr_name == "keywords") {
            meta["keywords"] = decodeEntities(attr_val);
        } else if (attr_name == "author") {
            meta["author"] = decodeEntities(attr_val);
        }
    }

    return meta;
}

// ============================================================================
// Private helpers
// ============================================================================

std::string HtmlProcessor::normalizeWhitespace(const std::string& text) {
    std::string result = {};
    result.reserve(text.size());

    bool last_was_newline = false;
    bool last_was_space   = false;
    int  consecutive_nl   = 0;

    for (char c : text) {
        if (c == '\n' || c == '\r') {
            if (consecutive_nl < 2) {
                result += '\n';
            }
            consecutive_nl++;
            last_was_newline = true;
            last_was_space   = false;
        } else if (c == ' ' || c == '\t') {
            if (!last_was_space && !last_was_newline) {
                result += ' ';
            }
            last_was_space = true;
        } else {
            result += c;
            last_was_space   = false;
            last_was_newline = false;
            consecutive_nl   = 0;
        }
    }

    // Trim leading/trailing whitespace
    auto start = result.find_first_not_of(" \t\n\r");
    auto end   = result.find_last_not_of(" \t\n\r");
    if (start == std::string::npos) {
      return "";
    }
    return result.substr(start, end - start + 1);
}

int HtmlProcessor::countTokens(const std::string& text) {
    if (text.empty()) {
      return 0;
    }
    std::istringstream iss(text);
    std::string tok = {};
    int n = 0;
    while (iss >> tok) {
      ++n;
    }
    return n;
}

// ============================================================================
// IContentProcessor interface
// ============================================================================

ExtractionResult HtmlProcessor::extract(
    const std::string& blob,
    const ContentType& content_type
) {
    ExtractionResult result;
    result.ok = true;

    if (blob.empty()) {
        result.ok = false;
        result.error_message = "Empty HTML blob";
        return result;
    }

    // 1. Extract metadata before removing any tags
    json meta = extractMetaTags(blob);

    // 2. Remove scripts / styles
    std::string working = config_.remove_scripts_styles
        ? removeScriptsAndStyles(blob)
        : blob;

    // 3. Remove boilerplate blocks
    if (config_.remove_boilerplate) {
        working = removeBoilerplate(working);
    }

    // 4. Strip remaining tags
    std::string text = stripTags(working, config_.preserve_heading_markers);

    // 5. Decode HTML entities
    if (config_.decode_entities) {
        text = decodeEntities(text);
    }

    // 6. Normalize whitespace
    text = normalizeWhitespace(text);

    // 7. Enforce max_text_length
    if (config_.max_text_length > 0 && static_cast<int>(text.size()) > config_.max_text_length) {
        text = text.substr(0, config_.max_text_length);
    }

    result.text = std::move(text);

    // Build metadata
    result.metadata = meta;
    result.metadata["original_size_bytes"] = static_cast<int64_t>(blob.size());
    result.metadata["extracted_size_bytes"] = static_cast<int64_t>(result.text.size());
    result.metadata["mime_type"]            = content_type.mime_type;
    result.metadata["token_count"]          = countTokens(result.text);

    return result;
}

std::vector<json> HtmlProcessor::chunk(
    const ExtractionResult& extraction_result,
    int chunk_size,
    int overlap
) {
    std::vector<json> chunks;

    const std::string& text = extraction_result.text;
    if (text.empty()) {
      return chunks;
    }

    // Split into paragraphs (double-newline boundaries)
    std::vector<std::string> paragraphs;
    {
        std::string para = {};
        for (size_t i = 0; i <static_cast<int>(text.size()); ) {
            if (i + 1 <static_cast<int>(text.size()) && text[i] == '\n' && text[i + 1] == '\n') {
                if (!para.empty()) {
                    paragraphs.push_back(para);
                    para.clear();
                }
                i += 2;
            } else if (text[i] == '\n') {
                // Single newline: treat as paragraph separator too
                if (!para.empty()) {
                    paragraphs.push_back(para);
                    para.clear();
                }
                ++i;
            } else {
                para += text[i++];
            }
        }
        if (!para.empty()) {
          paragraphs.push_back(para);
        }
    }

    // Merge paragraphs into chunks of roughly chunk_size tokens
    int seq = 0;
    std::string current_chunk = {};
    int current_tokens = 0;

    auto flushChunk = [&]([[maybe_unused]] const std::string& text_chunk) {
        if (text_chunk.empty()) {
          return;
        }
        json c = json::object();
        c["seq_num"]    = seq++;
        c["chunk_type"] = "text";
        c["text"]       = text_chunk;
        c["token_count"] = countTokens(text_chunk);
        chunks.push_back(std::move(c));
    };

    for (const auto& para : paragraphs) {
        int para_tokens = countTokens(para);

        if (current_tokens + para_tokens <= chunk_size || current_chunk.empty()) {
            // Fits in current chunk
            if (!current_chunk.empty()) {
              current_chunk += "\n\n";
            }
            current_chunk += para;
            current_tokens += para_tokens;
        } else {
            // Flush current chunk
            flushChunk(current_chunk);

            // Build overlap text from end of flushed chunk
            std::string overlap_text = {};
            if (overlap > 0 && !current_chunk.empty()) {
                // Take last `overlap` tokens from current_chunk
                std::istringstream iss(current_chunk);
                std::vector<std::string> tokens;
                std::string tok = {};
                while (iss >> tok) {
                  tokens.push_back(tok);
                }
                int take = std::min(overlap, static_cast<int>(tokens.size()));
                for (int i = static_cast<int>(tokens.size()) - take;
                     i < static_cast<int>(tokens.size()); ++i) {
                    if (!overlap_text.empty()) {
                      overlap_text += ' ';
                    }
                    overlap_text += tokens[i];
                }
            }

            current_chunk  = overlap_text.empty() ? para : (overlap_text + "\n\n" + para);
            current_tokens = countTokens(current_chunk);
        }
    }

    flushChunk(current_chunk);

    return chunks;
}

std::vector<float> HtmlProcessor::generateEmbedding(const std::string& chunk_data) {
    // Deterministic hash-based embedding compatible with TextProcessor
    const int DIM = 768;
    std::vector<float> embedding(DIM, 0.0f);

    if (chunk_data.empty()) {
      return embedding;
    }

    std::hash<std::string> hasher;
    std::istringstream iss(chunk_data);
    std::vector<std::string> tokens;
    std::string token = {};
    while (iss >> token) {
      tokens.push_back(token);
    }

    if (tokens.empty()) {
      return embedding;
    }

    for (size_t i = 0; i <static_cast<int>(tokens.size()); ++i) {
        size_t token_hash = hasher(tokens[i]);
        for (int seed = 0; seed < 3; ++seed) {
            size_t combined = token_hash ^ (i * 31) ^ (static_cast<size_t>(seed) * 97);
            for (int d = 0; d < 10; ++d) {
                int dim = static_cast<int>((combined + static_cast<size_t>(d) * 73) % DIM);
                float weight = 1.0f / (1.0f + static_cast<float>(i) * 0.1f);
                float phase  = static_cast<float>((combined + static_cast<size_t>(dim)) % 360)
                               * 3.14159f / 180.0f;
                embedding[dim] += std::sin(phase) * weight;
            }
        }
    }

    // L2 normalize
    float norm = 0.0f;
    for (float v : embedding) {
      norm += v * v;
    }
    norm = std::sqrt(norm);
    if (norm > 1e-6f) {
        for (float& v : embedding) {
          v /= norm;
        }
    }

    return embedding;
}

// ============================================================================
// Factory functions
// ============================================================================

std::unique_ptr<IContentProcessor> createHtmlProcessor() {
    return std::make_unique<HtmlProcessor>();
}

std::unique_ptr<IContentProcessor> createHtmlProcessor(HtmlProcessor::Config config) {
    return std::make_unique<HtmlProcessor>(std::move(config));
}

} // namespace content
} // namespace themis
