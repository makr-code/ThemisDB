/**
 * @file markdown_processor.cpp
 * @brief Markdown content processor for parsing, validation, and semantic analysis.
 * @version 0.0.15
 * @note Maturity: 🟡 BETA
 * @note Score: 83/100
 * @note Gap Summary: total=6; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=2, C=0, H=1, M=5, L=0
 * @note Status: Production Ready; Markdown parsing complete; advanced extensions deferred
 * @note This block is auto-generated and will be overwritten.
 */
#include "content/markdown_processor.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <sstream>

namespace themis {
namespace content {

// ============================================================================
// Constructors
// ============================================================================

MarkdownProcessor::MarkdownProcessor()
    : MarkdownProcessor(Config{})
{}

MarkdownProcessor::MarkdownProcessor(Config config)
    : config_(std::move(config))
{}

// ============================================================================
// Internal helpers (file-local)
// ============================================================================

static std::string trimCopy(const std::string& s) {
    auto start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return {};
    auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// ============================================================================
// parseFrontmatter
// ============================================================================

json MarkdownProcessor::parseFrontmatter(const std::string& markdown,
                                          std::string& body_out) {
    json fm = json::object();
    body_out = markdown;

    // Must start with "---" on its own line
    if (static_cast<int>(markdown.size()) < 3 || markdown.substr(0, 3) != "---") {
        return fm;
    }
    size_t first_nl = markdown.find('\n');
    if (first_nl == std::string::npos) {
      return fm;
    }

    // Characters between "---" and first newline must be spaces/tabs only
    for (size_t i = 3; i < first_nl; ++i) {
        if (markdown[i] != ' ' && markdown[i] != '\t') {
          return fm;
        }
    }

    // Search for closing delimiter: "---" or "..." on its own line
    size_t search_start = first_nl + 1;
    size_t close_pos    = std::string::npos;
    while (static_cast<size_t>(search_start) <static_cast<int>(markdown.size())) {
        size_t line_end = markdown.find('\n', search_start);
        if (line_end == std::string::npos) {
          line_end = markdown.size();
        }

        std::string trimmed = trimCopy(markdown.substr(search_start, line_end - search_start));
        if (trimmed == "---" || trimmed == "...") {
            close_pos = search_start;
            body_out  = (line_end <static_cast<int>(markdown.size())) ? markdown.substr(line_end + 1) : "";
            break;
        }
        search_start = line_end + 1;
    }

    if (close_pos == std::string::npos) {
        // No closing delimiter — not valid frontmatter
        return fm;
    }

    // Parse lines between opening and closing delimiter
    std::string block = markdown.substr(first_nl + 1, close_pos - (first_nl + 1));
    std::istringstream ss(block);
    std::string line = {};
    while (std::getline(ss, line)) {
        std::string tline = trimCopy(line);
        if (tline.empty() || tline[0] == '#') {
          continue;
        }

        size_t colon = line.find(':');
        if (colon == std::string::npos) {
          continue;
        }

        std::string key   = trimCopy(line.substr(0, colon));
        std::string value = trimCopy(line.substr(colon + 1));

        if (key.empty()) {
          continue;
        }

        // Inline YAML list: key: [a, b, c]
        if (!value.empty() && value.front() == '[' && value.back() == ']') {
            std::string inner = value.substr(1, static_cast<int>(value.size()) - 2);
            json arr = json::array();
            std::istringstream list_ss(inner);
            std::string item = {};
            while (std::getline(list_ss, item, ',')) {
                std::string ti = trimCopy(item);
                if ((static_cast<int>(ti.size()) >= 2 &&
                    ((ti.front() == '"' && ti.back() == '"') ||
                     (ti.front() == '\'' && ti.back() == '\'')))) {
                    ti = ti.substr(1, static_cast<int>(ti.size()) - 2);
                }
                arr.push_back(ti);
            }
            fm[key] = arr;
        } else {
            // Scalar — strip optional surrounding quotes
            if ((static_cast<int>(value.size()) >= 2 &&
                ((value.front() == '"' && value.back() == '"') ||
                 (value.front() == '\'' && value.back() == '\'')))) {
                value = value.substr(1, static_cast<int>(value.size()) - 2);
            }
            fm[key] = value;
        }
    }

    return fm;
}

// ============================================================================
// stripMarkdown
// ============================================================================

std::string MarkdownProcessor::stripMarkdown(const std::string& markdown,
                                              bool preserve_headings,
                                              bool strip_code) {
    std::ostringstream out = {};
    std::istringstream in(markdown);
    std::string line = {};

    bool in_fenced_code = false;
    std::string fence_marker; // "```" or "~~~"

    while (std::getline(in, line)) {
        // ----------------------------------------------------------------
        // Fenced code block boundaries
        // ----------------------------------------------------------------
        {
            std::string sl = trimCopy(line);
            bool is_fence = static_cast<int>(sl.size()) >= 3 &&
                            (sl.substr(0, 3) == "```" || sl.substr(0, 3) == "~~~");

            if (!in_fenced_code && is_fence) {
                in_fenced_code = true;
                fence_marker   = sl.substr(0, 3);
                out << '\n';
                continue;
            }
            if (in_fenced_code) {
                if (is_fence && sl.substr(0,static_cast<int>(fence_marker.size())) == fence_marker) {
                    in_fenced_code = false;
                    fence_marker.clear();
                    out << '\n';
                } else if (!strip_code) {
                    out << line << '\n';
                }
                continue;
            }
        }

        // ----------------------------------------------------------------
        // Horizontal rules: lines of only -, *, or _ (>=3 chars)
        // ----------------------------------------------------------------
        {
            std::string tl = trimCopy(line);
            if (static_cast<int>(tl.size()) >= 3) {
                char c = tl[0];
                if (c == '-' || c == '*' || c == '_') {
                    bool is_hr = true;
                    for (char ch : tl) {
                        if (ch != c && ch != ' ') { is_hr = false; break; }
                    }
                    if (is_hr) { out << '\n'; continue; }
                }
            }
        }

        // ----------------------------------------------------------------
        // Setext heading underlines (=== or ---): emit newline boundary
        // ----------------------------------------------------------------
        {
            std::string tl = trimCopy(line);
            if (static_cast<int>(tl.size()) >= 2) {
                bool all_eq   = tl.find_first_not_of('=') == std::string::npos;
                bool all_dash = tl.find_first_not_of('-') == std::string::npos;
                if (all_eq || all_dash) { out << '\n'; continue; }
            }
        }

        // ----------------------------------------------------------------
        // Blockquote: strip leading ">" chains
        // ----------------------------------------------------------------
        {
            size_t i = 0;
            while (i <static_cast<int>(line.size()) && line[i] == ' ') {
              ++i;
            }
            if (i <static_cast<int>(line.size()) && line[i] == '>') {
                std::string rest = line;
                while (!rest.empty()) {
                    size_t j = 0;
                    while (j <static_cast<int>(rest.size()) && rest[j] == ' ') {
                      ++j;
                    }
                    if (j <static_cast<int>(rest.size()) && rest[j] == '>') {
                        rest = rest.substr(j + 1);
                        if (!rest.empty() && rest[0] == ' ') {
                          rest = rest.substr(1);
                        }
                    } else {
                        break;
                    }
                }
                line = rest;
            }
        }

        // ----------------------------------------------------------------
        // Table rows: strip separator rows, replace | with spaces
        // ----------------------------------------------------------------
        {
            std::string tl = trimCopy(line);
            if (!tl.empty() && tl[0] == '|') {
                bool is_sep = true;
                for (char c : tl) {
                    if (c != '|' && c != '-' && c != ':' && c != ' ') { is_sep = false; break; }
                }
                if (is_sep) {
                  continue;
                }
                std::string row = {};
                for (char c : tl) {
                  row += (c == '|') ? ' ' : c;
                }
                line = trimCopy(row);
            }
        }

        // ----------------------------------------------------------------
        // ATX headings: # H1 … ###### H6
        // ----------------------------------------------------------------
        {
            size_t hashes = 0;
            while (hashes <static_cast<int>(line.size()) && line[hashes] == '#') {
              ++hashes;
            }
            if (hashes >= 1 && hashes <= 6 &&
                hashes <static_cast<int>(line.size()) && line[hashes] == ' ') {
                std::string text = line.substr(hashes + 1);
                // Strip trailing closing hashes
                auto rend = text.find_last_not_of("# \t");
                if (rend != std::string::npos) {
                  text = text.substr(0, rend + 1);
                }

                if (preserve_headings) {
                    out << std::string(hashes, '#') << ' ' << text << '\n';
                } else {
                    out << text << '\n';
                }
                continue;
            }
        }

        // ----------------------------------------------------------------
        // Unordered list markers: "- ", "* ", "+ "
        // ----------------------------------------------------------------
        {
            size_t i = 0;
            while (i <static_cast<int>(line.size()) && line[i] == ' ') {
              ++i;
            }
            if ((i <static_cast<int>(line.size()) &&
                (line[i] == '-' || line[i] == '*' || line[i] == '+') &&
               i + 1 <static_cast<int>(line.size()) && line[i + 1] == ' ')) {
                line = line.substr(i + 2);
            }
        }

        // ----------------------------------------------------------------
        // Ordered list markers: "1. ", "2. ", etc.
        // ----------------------------------------------------------------
        {
            size_t i = 0;
            while (i <static_cast<int>(line.size()) && line[i] == ' ') {
              ++i;
            }
            size_t j = i;
            while (j <static_cast<int>(line.size()) && std::isdigit(static_cast<unsigned char>(line[j]))) {
              ++j;
            }
            if (j > i  && static_cast<size_t>(j) <static_cast<int>(line.size()) && line[j] == '.' &&
                j + 1 <static_cast<int>(line.size()) && line[j + 1] == ' ') {
                line = line.substr(j + 2);
            }
        }

        // ----------------------------------------------------------------
        // Inline Markdown: images, links, code spans, emphasis, strikethrough
        // ----------------------------------------------------------------
        {
            std::string result = {};
            result.reserve(line.size());
            size_t i = 0;
            while (static_cast<size_t>(i) <static_cast<int>(line.size())) {
                // Images: ![alt](url) → alt text
                if (line[i] == '!' && i + 1 <static_cast<int>(line.size()) && line[i + 1] == '[') {
                    size_t alt_start = i + 2;
                    size_t alt_end   = line.find(']', alt_start);
                    if (alt_end != std::string::npos) {
                        size_t po = line.find('(', alt_end);
                        size_t pc = (po != std::string::npos) ? line.find(')', po) : std::string::npos;
                        if (po == alt_end + 1 && pc != std::string::npos) {
                            result += line.substr(alt_start, alt_end - alt_start);
                            i = pc + 1;
                            continue;
                        }
                    }
                    result += line[i++];
                    continue;
                }

                // Links: [text](url) → text
                if (line[i] == '[') {
                    size_t ts = i + 1;
                    size_t te = line.find(']', ts);
                    if (te != std::string::npos) {
                        size_t po = line.find('(', te);
                        size_t pc = (po != std::string::npos) ? line.find(')', po) : std::string::npos;
                        if (po == te + 1 && pc != std::string::npos) {
                            result += line.substr(ts, te - ts);
                            i = pc + 1;
                            continue;
                        }
                    }
                    result += line[i++];
                    continue;
                }

                // Inline code: `...` or ``...``
                if (line[i] == '`') {
                    size_t n = 0;
                    while (i + n <static_cast<int>(line.size()) && line[i + n] == '`') {
                      ++n;
                    }
                    std::string ticks(n, '`');
                    size_t cs = i + n;
                    size_t ce = line.find(ticks, cs);
                    if (ce != std::string::npos) {
                        if (!strip_code) {
                          result += line.substr(cs, ce - cs);
                        }
                        i = ce + n;
                        continue;
                    }
                    result += line[i++];
                    continue;
                }

                // Bold+italic: ***…*** or ___…___
                if (((line[i] == '*' || line[i] == '_') &&
                    i + 2 <static_cast<int>(line.size()) && line[i+1] == line[i] && line[i+2] == line[i])) {
                    char m = line[i];
                    std::string triple(3, m);
                    size_t close = line.find(triple, i + 3);
                    if (close != std::string::npos) {
                        result += line.substr(i + 3, close - (i + 3));
                        i = close + 3;
                        continue;
                    }
                    result += line[i++];
                    continue;
                }

                // Bold: **…** or __…__
                if (((line[i] == '*' || line[i] == '_') &&
                    i + 1 <static_cast<int>(line.size()) && line[i+1] == line[i])) {
                    char m = line[i];
                    std::string pair(2, m);
                    size_t close = line.find(pair, i + 2);
                    if (close != std::string::npos) {
                        result += line.substr(i + 2, close - (i + 2));
                        i = close + 2;
                        continue;
                    }
                    result += line[i++];
                    continue;
                }

                // Italic: *…* or _…_
                if (line[i] == '*' || line[i] == '_') {
                    char m = line[i];
                    size_t close = line.find(m, i + 1);
                    if (close != std::string::npos && close > i + 1) {
                        result += line.substr(i + 1, close - (i + 1));
                        i = close + 1;
                        continue;
                    }
                    result += line[i++];
                    continue;
                }

                // Strikethrough: ~~…~~
                if (line[i] == '~' && i + 1 <static_cast<int>(line.size()) && line[i+1] == '~') {
                    size_t close = line.find("~~", i + 2);
                    if (close != std::string::npos) {
                        result += line.substr(i + 2, close - (i + 2));
                        i = close + 2;
                        continue;
                    }
                    result += line[i++];
                    continue;
                }

                result += line[i++];
            }
            line = result;
        }

        out << line << '\n';
    }

    return out.str();
}

// ============================================================================
// Private helpers
// ============================================================================

std::string MarkdownProcessor::normalizeWhitespace(const std::string& text) {
    std::string result = {};
    result.reserve(text.size());

    bool last_was_space   = false;
    bool last_was_newline = false;
    int  consecutive_nl   = 0;

    for (char c : text) {
        if (c == '\n') {
            ++consecutive_nl;
            if (consecutive_nl <= 2) {
              result += '\n';
            }
            last_was_space   = false;
            last_was_newline = true;
        } else if (c == '\r') {
            // ignore
        } else if (c == ' ' || c == '\t') {
            consecutive_nl = 0;
            if (!last_was_space && !last_was_newline) {
              result += ' ';
            }
            last_was_space = true;
        } else {
            consecutive_nl   = 0;
            result           += c;
            last_was_space   = false;
            last_was_newline = false;
        }
    }

    auto start = result.find_first_not_of(" \t\n\r");
    auto end   = result.find_last_not_of(" \t\n\r");
    if (start == std::string::npos) {
      return "";
    }
    return result.substr(start, end - start + 1);
}

int MarkdownProcessor::countTokens(const std::string& text) {
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

ExtractionResult MarkdownProcessor::extract(
    const std::string& blob,
    const ContentType& content_type
) {
    ExtractionResult result;
    result.ok = true;

    if (blob.empty()) {
        result.ok = false;
        result.error_message = "Empty Markdown blob";
        return result;
    }

    // 1. Parse (and strip) YAML frontmatter
    json frontmatter = json::object();
    std::string body = blob;
    if (config_.parse_frontmatter) {
        frontmatter = parseFrontmatter(blob, body);
    }

    // 2. Strip Markdown syntax
    std::string text = stripMarkdown(body,
                                     config_.preserve_heading_markers,
                                     config_.strip_code_blocks);

    // 3. Normalize whitespace
    text = normalizeWhitespace(text);

    // 4. Enforce max_text_length
    if (config_.max_text_length > 0 && static_cast<int>(text.size()) > config_.max_text_length) {
        text = text.substr(0, config_.max_text_length);
    }

    result.text = std::move(text);

    // Build metadata
    result.metadata = frontmatter;
    result.metadata["original_size_bytes"]  = static_cast<int64_t>(blob.size());
    result.metadata["extracted_size_bytes"] = static_cast<int64_t>(result.text.size());
    result.metadata["mime_type"]            = content_type.mime_type;
    result.metadata["token_count"]          = countTokens(result.text);
    result.metadata["has_frontmatter"]      = !frontmatter.empty();

    return result;
}

std::vector<json> MarkdownProcessor::chunk(
    const ExtractionResult& extraction_result,
    int chunk_size,
    int overlap
) {
    std::vector<json> chunks;

    const std::string& text = extraction_result.text;
    if (text.empty()) {
      return chunks;
    }

    // Split into paragraphs on blank-line and heading boundaries
    std::vector<std::string> paragraphs;
    {
        std::string para = {};
        std::istringstream ss(text);
        std::string line = {};
        while (std::getline(ss, line)) {
            std::string tl = trimCopy(line);
            bool is_heading = (!tl.empty() && tl[0] == '#');
            bool is_blank   = tl.empty();

            if (is_heading) {
                if (!para.empty()) { paragraphs.push_back(para); para.clear(); }
                para = line;
            } else if (is_blank) {
                if (!para.empty()) { paragraphs.push_back(para); para.clear(); }
            } else {
                if (!para.empty()) {
                  para += '\n';
                }
                para += line;
            }
        }
        if (!para.empty()) {
          paragraphs.push_back(para);
        }
    }

    int seq = 0;
    std::string current_chunk = {};
    int current_tokens = 0;

    auto flushChunk = [&]([[maybe_unused]] const std::string& text_chunk) {
        if (text_chunk.empty()) {
          return;
        }
        json c = json::object();
        c["seq_num"]     = seq++;
        c["chunk_type"]  = "text";
        c["text"]        = text_chunk;
        c["token_count"] = countTokens(text_chunk);
        chunks.push_back(std::move(c));
    };

    for (const auto& para : paragraphs) {
        int para_tokens = countTokens(para);

        if (current_tokens + para_tokens <= chunk_size || current_chunk.empty()) {
            if (!current_chunk.empty()) {
              current_chunk += "\n\n";
            }
            current_chunk  += para;
            current_tokens += para_tokens;
        } else {
            flushChunk(current_chunk);

            std::string overlap_text = {};
            if (overlap > 0 && !current_chunk.empty()) {
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

std::vector<float> MarkdownProcessor::generateEmbedding(const std::string& chunk_data) {
    // Deterministic hash-based embedding — compatible with HtmlProcessor / TextProcessor
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

std::unique_ptr<IContentProcessor> createMarkdownProcessor() {
    return std::make_unique<MarkdownProcessor>();
}

std::unique_ptr<IContentProcessor> createMarkdownProcessor(MarkdownProcessor::Config config) {
    return std::make_unique<MarkdownProcessor>(std::move(config));
}

} // namespace content
} // namespace themis

