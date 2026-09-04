/**
 * @file aql_syntax_highlighter.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.36
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=16, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "aql/aql_syntax_highlighter.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_set>

namespace themis {
namespace aql {

// ---------------------------------------------------------------------------
// ANSI colour codes
// ---------------------------------------------------------------------------

namespace {

std::string toLowerAqlSyntax(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

// Standard AQL keywords (case-insensitive)
const std::unordered_set<std::string> &coreKeywords() {
    static const std::unordered_set<std::string> kw = {
        "for",    "in",        "filter",    "sort",          "limit",  "return", "let",      "asc",
        "desc",   "and",       "or",        "xor",           "not",    "graph",  "outbound", "inbound",
        "any",    "collect",   "aggregate", "true",          "false",  "null",   "with",     "as",
        "all",    "satisfies", "to",        "shortest_path", "insert", "update", "replace",  "remove",
        "upsert", "create",    "drop",      "collection",    "index",  "view",   "distinct",
    };
    return kw;
}

// LLM-extension keywords
const std::unordered_set<std::string> &llmKeywords() {
    static const std::unordered_set<std::string> kw = {
        "llm",     "infer",     "rag",   "embed",   "model",  "lora",    "stats", "cache",
        "load",    "unload",    "list",  "ingest",  "vision", "analyze", "batch", "question",
        "compare", "transform", "using", "options", "top",    "from",
    };
    return kw;
}

// Well-known built-in function names
const std::unordered_set<std::string> &builtinFunctions() {
    static const std::unordered_set<std::string> fn = {
        "similarity", "proximity",      "sum",         "min",          "max",       "avg",
        "count",      "length",         "concat",      "upper",        "lower",     "trim",
        "substring",  "contains",       "starts_with", "ends_with",    "like",      "floor",
        "ceil",       "round",          "abs",         "sqrt",         "pow",       "st_distance",
        "st_within",  "st_intersects",  "date_now",    "date_format",  "date_add",  "push",
        "pop",        "append",         "slice",       "flatten",      "to_string", "to_number",
        "to_bool",    "json_stringify", "json_parse",  "themis_embed",
    };
    return fn;
}

// ANSI colour helpers ---------------------------------------------------------

constexpr const char *RESET        = "\x1b[0m";
constexpr const char *BOLD         = "\x1b[1m";
constexpr const char *FG_CYAN      = "\x1b[36m"; // core keywords
constexpr const char *FG_MAGENTA   = "\x1b[35m"; // LLM keywords
constexpr const char *FG_YELLOW    = "\x1b[33m"; // built-in functions
constexpr const char *FG_GREEN     = "\x1b[32m"; // string literals
constexpr const char *FG_BLUE      = "\x1b[34m"; // numbers
constexpr const char *FG_RED       = "\x1b[31m"; // error annotation marker
constexpr const char *FG_DARK_GREY = "\x1b[90m"; // comments

} // anonymous namespace

// ---------------------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------------------

AQLSyntaxHighlighter::AQLSyntaxHighlighter([[maybe_unused]] bool use_ansi) : use_ansi_(use_ansi) {}

// ---------------------------------------------------------------------------
// Tokenizer
// ---------------------------------------------------------------------------

std::vector<AQLToken> AQLSyntaxHighlighter::tokenize(const std::string &code) const {
    std::vector<AQLToken> tokens;
    std::size_t pos  = 0;
    std::size_t line = 1;
    std::size_t col  = 1;

    auto advance = [&]() -> char {
        char c = code[pos++];
        if (c == '\n') {
            ++line;
            col = 1;
        } else {
            ++col;
        }
        return c;
    };
    auto peek
        = [&]([[maybe_unused]] std::size_t offset = 0) -> char { return (pos + offset < code.size()) ? code[pos + offset] : '\0'; };

    while (static_cast<size_t>(pos) < code.size()) {
        // Skip whitespace (preserve for faithful reconstruction)
        if (std::isspace(static_cast<unsigned char>(code[pos]))) {
            std::string ws = {};
            std::size_t tl = line, tc = col;
            while (pos < code.size() && std::isspace(static_cast<unsigned char>(code[pos]))) {
                ws += advance();
            }
            tokens.push_back({AQLTokenType::UNKNOWN, ws, tl, tc});
            continue;
        }

        std::size_t tl = line, tc = col;

        // Single-line comment //
        if (code[pos] == '/' && peek(1) == '/') {
            std::string comment = {};
            while (pos < code.size() && code[pos] != '\n') {
                comment += advance();
            }
            tokens.push_back({AQLTokenType::COMMENT, comment, tl, tc});
            continue;
        }

        // Block comment /* … */
        if (code[pos] == '/' && peek(1) == '*') {
            std::string comment = {};
            comment += advance();
            comment += advance(); // consume /*
            while (static_cast<size_t>(pos) < code.size()) {
                if (code[pos] == '*' && peek(1) == '/') {
                    comment += advance();
                    comment += advance();
                    break;
                }
                comment += advance();
            }
            tokens.push_back({AQLTokenType::COMMENT, comment, tl, tc});
            continue;
        }

        // String literal (single or double quote)
        if (code[pos] == '"' || code[pos] == '\'') {
            char q = code[pos];
            std::string str = {};
            str += advance(); // opening quote
            while (static_cast<size_t>(pos) < code.size()) {
                char c = advance();
                str += c;
                if (c == '\\' && pos < code.size()) {
                    str += advance(); // escaped char
                } else if (c == q) {
                    break;
                }
            }
            tokens.push_back({AQLTokenType::STRING, str, tl, tc});
            continue;
        }

        // Bind parameter  @name
        if (code[pos] == '@') {
            std::string ident = {};
            ident += advance(); // @
            while (pos < code.size() && (std::isalnum(static_cast<unsigned char>(code[pos])) || code[pos] == '_')) {
                ident += advance();
            }
            tokens.push_back({AQLTokenType::IDENTIFIER, ident, tl, tc});
            continue;
        }

        // Number (integer or float)
        if (std::isdigit(static_cast<unsigned char>(code[pos]))
            || (code[pos] == '-' && pos + 1 < code.size() && std::isdigit(static_cast<unsigned char>(code[pos + 1])))) {
            std::string num = {};
            if (code[pos] == '-') {
                num += advance();
            }
            while (pos < code.size() && std::isdigit(static_cast<unsigned char>(code[pos]))) {
                num += advance();
            }
            if (pos < code.size() && code[pos] == '.') {
                num += advance();
                while (pos < code.size() && std::isdigit(static_cast<unsigned char>(code[pos]))) {
                    num += advance();
                }
            }
            tokens.push_back({AQLTokenType::NUMBER, num, tl, tc});
            continue;
        }

        // Identifier or keyword
        if (std::isalpha(static_cast<unsigned char>(code[pos])) || code[pos] == '_') {
            std::string ident = {};
            while (pos < code.size() && (std::isalnum(static_cast<unsigned char>(code[pos])) || code[pos] == '_')) {
                ident += advance();
            }

            // Peek for '(' to detect function calls
            std::size_t look = pos;
            while (look < code.size() && code[look] == ' ') {
                ++look;
            }
            bool is_call = (look < code.size() && code[look] == '(');

            std::string lower = toLowerAqlSyntax(ident);
            AQLTokenType ttype = {};
            if (is_call && builtinFunctions().count(lower)) {
                ttype = AQLTokenType::FUNCTION;
            } else if (coreKeywords().count(lower)) {
                ttype = AQLTokenType::KEYWORD;
            } else if (llmKeywords().count(lower)) {
                ttype = AQLTokenType::LLM_KEYWORD;
            } else {
                ttype = AQLTokenType::IDENTIFIER;
            }

            tokens.push_back({ttype, ident, tl, tc});
            continue;
        }

        // Two-char operators
        if (pos + 1 < code.size()) {
            std::string two{code[pos], code[pos + 1]};
            if (two == "==" || two == "!=" || two == "<=" || two == ">=" || two == "&&" || two == "||" || two == "|>") {
                advance();
                advance();
                tokens.push_back({AQLTokenType::OPERATOR, two, tl, tc});
                continue;
            }
        }

        // Single-char operators and punctuation
        char c = code[pos];
        std::string s{c};
        if (c == '<' || c == '>' || c == '=' || c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '!'
            || c == '~') {
            advance();
            tokens.push_back({AQLTokenType::OPERATOR, s, tl, tc});
        } else if (c == '(' || c == ')' || c == '{' || c == '}' || c == '[' || c == ']' || c == ',' || c == '.'
                   || c == ':' || c == ';') {
            advance();
            tokens.push_back({AQLTokenType::PUNCTUATION, s, tl, tc});
        } else {
            advance();
            tokens.push_back({AQLTokenType::UNKNOWN, s, tl, tc});
        }
    }

    return tokens;
}

// ---------------------------------------------------------------------------
// ANSI colorize helper
// ---------------------------------------------------------------------------

std::string AQLSyntaxHighlighter::ansiReset() const {
    return use_ansi_ ? RESET : "";
}

std::string AQLSyntaxHighlighter::colorize(const AQLToken &tok) const {
    if (!use_ansi_) {
        return tok.value;
    }

    switch (tok.type) {
        case AQLTokenType::KEYWORD:
            return std::string(BOLD) + FG_CYAN + tok.value + RESET;
        case AQLTokenType::LLM_KEYWORD:
            return std::string(BOLD) + FG_MAGENTA + tok.value + RESET;
        case AQLTokenType::FUNCTION:
            return std::string(FG_YELLOW) + tok.value + RESET;
        case AQLTokenType::STRING:
            return std::string(FG_GREEN) + tok.value + RESET;
        case AQLTokenType::NUMBER:
            return std::string(FG_BLUE) + tok.value + RESET;
        case AQLTokenType::COMMENT:
            return std::string(FG_DARK_GREY) + tok.value + RESET;
        default:
            return tok.value;
    }
}

// ---------------------------------------------------------------------------
// highlightBlock
// ---------------------------------------------------------------------------

std::string AQLSyntaxHighlighter::highlightBlock(const std::string &aql_code) const {
    auto tokens = tokenize(aql_code);
    std::string out = {};
    out.reserve(aql_code.size() * 2);
    for (const auto &tok : tokens) {
        out += colorize(tok);
    }
    return out;
}

// ---------------------------------------------------------------------------
// annotateErrors  (lightweight structural validation)
// ---------------------------------------------------------------------------

std::vector<AQLAnnotation> AQLSyntaxHighlighter::annotateErrors(const std::string &aql_code) const {
    std::vector<AQLAnnotation> errors;
    auto tokens = tokenize(aql_code);

    // --- 1. Balanced brackets check ---
    struct BracketEntry {
        char ch = {};
        std::size_t line = {};
        std::size_t col = {};
    };
    std::vector<BracketEntry> stack;

    for (const auto &tok : tokens) {
        if (tok.type != AQLTokenType::PUNCTUATION) {
            continue;
        }
        char c = tok.value.empty() ? '\0' : tok.value[0];
        if (c == '(' || c == '{' || c == '[') {
            stack.push_back({c, tok.line, tok.column});
        } else if (c == ')' || c == '}' || c == ']') {
            if (stack.empty()) {
                errors.push_back({tok.line, tok.column, std::string("Unmatched closing '") + c + "'"});
            } else {
                char open = stack.back().ch;
                bool ok   = (c == ')' && open == '(') || (c == '}' && open == '{') || (c == ']' && open == '[');
                if (!ok) {
                    errors.push_back({tok.line, tok.column,
                                      std::string("Mismatched bracket: expected closing for '") + open
                                          + "' opened at line " + std::to_string(stack.back().line) + ", col "
                                          + std::to_string(stack.back().col) + ", but found '" + c + "'"});
                }
                stack.pop_back();
            }
        }
    }
    for (const auto &entry : stack) {
        errors.push_back({entry.line, entry.col, std::string("Unclosed '") + entry.ch + "'"});
    }

    // --- 2. Unterminated string literals ---
    for (const auto &tok : tokens) {
        if (tok.type != AQLTokenType::STRING) {
            continue;
        }
        if (tok.value.size() < 2) {
            errors.push_back({tok.line, tok.column, "Unterminated string literal"});
            continue;
        }
        char q = tok.value.front();
        if (tok.value.back() != q) {
            errors.push_back({tok.line, tok.column, "Unterminated string literal"});
        }
    }

    // --- 3. FOR without IN ---
    for (std::size_t i = 0; i < tokens.size(); ++i) {
        const auto &tok = tokens[i];
        if (tok.type != AQLTokenType::KEYWORD) {
            continue;
        }
        if (toLowerAqlSyntax(tok.value) != "for") {
            continue;
        }

        // Skip whitespace/identifier tokens and look for IN
        bool found_in = false;
        for (std::size_t j = i + 1; j < tokens.size() && j < i + 8; ++j) {
            const auto &t = tokens[j];
            if (t.type == AQLTokenType::KEYWORD && toLowerAqlSyntax(t.value) == "in") {
                found_in = true;
                break;
            }
            // Stop if we hit another statement-level keyword
            if (t.type == AQLTokenType::KEYWORD) {
                std::string lv = toLowerAqlSyntax(t.value);
                if (lv == "for" || lv == "filter" || lv == "return" || lv == "sort" || lv == "limit"
                    || lv == "collect") {
                    break;
                }
            }
        }
        if (!found_in) {
            errors.push_back({tok.line, tok.column, "FOR clause is missing IN keyword"});
        }
    }

    // Sort by (line, column) for a deterministic order
    std::stable_sort(errors.begin(), errors.end(), [](const AQLAnnotation &a, const AQLAnnotation &b) {
        return a.line < b.line || (a.line == b.line && a.column < b.column);
    });

    return errors;
}

// ---------------------------------------------------------------------------
// formatLLMResponse
// ---------------------------------------------------------------------------

HighlightedResponse AQLSyntaxHighlighter::formatLLMResponse(const std::string &llm_response) const {
    HighlightedResponse result;
    result.text.reserve(llm_response.size() * 2);

    std::size_t pos       = 0;
    const std::size_t len = llm_response.size();

    // Helper: find ```  possibly followed by "aql" on the same line
    auto findFence = [&]([[maybe_unused]] std::size_t start) -> std::size_t { return llm_response.find("```", start); };

    while (pos < len) {
        std::size_t fence_open = findFence(pos);
        if (fence_open == std::string::npos) {
            // No more code fences — append the remainder verbatim
            result.text.append(llm_response, pos, len - pos);
            break;
        }

        // Append prose before the fence
        result.text.append(llm_response, pos, fence_open - pos);

        // Skip the opening ``` and optional language tag (e.g. "aql")
        std::size_t after_fence = fence_open + 3;
        // consume optional language tag until newline
        std::size_t tag_end = after_fence;
        while (tag_end < len && llm_response[tag_end] != '\n' && llm_response[tag_end] != '`') {
            ++tag_end;
        }

        std::string lang_tag = llm_response.substr(after_fence, tag_end - after_fence);
        // Lowercase for comparison
        std::string lang_lower = toLowerAqlSyntax(lang_tag);
        bool is_aql            = (lang_lower.empty() || lang_lower == "aql");

        // Skip to the beginning of the code content (next line after the tag)
        std::size_t code_start = tag_end;
        if (code_start < len && llm_response[code_start] == '\n') {
            ++code_start;
        }

        // Find closing ```
        std::size_t fence_close = findFence(code_start);
        std::string code_content = {};
        if (fence_close == std::string::npos) {
            // Unterminated block — treat everything to end as code
            code_content = llm_response.substr(code_start);
            pos          = len;
        } else {
            code_content = llm_response.substr(code_start, fence_close - code_start);
            pos          = fence_close + 3; // skip closing ```
        }

        if (is_aql) {
            // Highlight and annotate
            auto block_errors = annotateErrors(code_content);
            // Offset line numbers if the block doesn't start at line 1 of the response
            // (We do not track absolute line offsets here; annotations are relative
            // to the code block itself, which is the most useful for the caller.)
            result.annotations.insert(result.annotations.end(), block_errors.begin(), block_errors.end());

            // Reconstruct the fence with highlighted content
            result.text += "```aql\n";
            result.text += highlightBlock(code_content);
            if (!code_content.empty() && code_content.back() != '\n') {
                result.text += '\n';
            }
            result.text += "```";
        } else {
            // Non-AQL block — pass through unchanged
            result.text += "```";
            result.text += lang_tag;
            result.text += '\n';
            result.text += code_content;
            result.text += "```";
        }
    }

    return result;
}

} // namespace aql
} // namespace themis
