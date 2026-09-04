/**
 * @file fulltext_functions.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=8, M=30, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "query/functions/function_registry.h"
#include "index/secondary_index.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace themis {
namespace query {
namespace functions {

using json = nlohmann::json;

// ============================================================================
// Helper Functions
// ============================================================================

namespace {

// Tokenize text into words (simple whitespace/punctuation tokenizer)
std::vector<std::string> tokenize(const std::string& text) {
    std::vector<std::string> tokens;
    std::string current = {};
    
    for (char c : text) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            current += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        } else if (!current.empty()) {
            tokens.push_back(current);
            current.clear();
        }
    }
    if (!current.empty()) {
        tokens.push_back(current);
    }
    
    return tokens;
}

// Generate n-grams from a string
std::vector<std::string> generateNgrams(const std::string& s, int n) {
    std::vector<std::string> ngrams = {};

    if (s.length() < static_cast<size_t>(n)) {
        ngrams.push_back(s);
        return ngrams;
    }
    for (size_t i = 0; i <= s.length() - n; ++i) {
        ngrams.push_back(s.substr(i, n));
    }
    return ngrams;
}

// Soundex encoding
std::string soundex(const std::string& s) {
    if (s.empty()) {
      return "";
    }
    
    std::string result = {};
    result += static_cast<char>(std::toupper(static_cast<unsigned char>(s[0])));
    
    auto getCode = [](char c) -> char {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        if (c == 'B' || c == 'F' || c == 'P' || c == 'V') {
          return '1';
        }
        if (c == 'C' || c == 'G' || c == 'J' || c == 'K' || c == 'Q' || c == 'S' || c == 'X' || c == 'Z') {
          return '2';
        }
        if (c == 'D' || c == 'T') {
          return '3';
        }
        if (c == 'L') {
          return '4';
        }
        if (c == 'M' || c == 'N') {
          return '5';
        }
        if (c == 'R') {
          return '6';
        }
        return '0';
    };
    
    char lastCode = getCode(s[0]);
    for (size_t i = 1; i < s.length() && result.length() < 4; ++i) {
        char code = getCode(s[i]);
        if (code != '0' && code != lastCode) {
            result += code;
        }
        lastCode = code;
    }
    
    while (result.length() < 4) {
      result += '0';
    }
    return result;
}

// Simplified Metaphone encoding
std::string metaphone(const std::string& word, int maxLen = 6) {
    if (word.empty()) {
      return "";
    }
    
    std::string result = {};
    std::string upper = {};
    for (char c : word) {
      upper += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    
    size_t i = 0;
    
    // Skip initial silent letters
    if (upper.length() >= 2) {
        std::string start = upper.substr(0, 2);
        if (start == "KN" || start == "GN" || start == "PN" || start == "AE" || start == "WR") {
            i = 1;
        }
    }
    
    auto isVowel = [](char c) {
        return c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U';
    };
    
    while (i < upper.length() && static_cast<int>(result.length()) < maxLen) {
        char c = upper[i];
        char next = (i + 1 < upper.length()) ? upper[i + 1] : '\0';
        
        switch (c) {
            case 'A': case 'E': case 'I': case 'O': case 'U':
                if (i == 0) {
                  result += c;
                }
                break;
            case 'B':
                if (i == 0 || upper[static_cast<int>(i - 1)] != 'M') {
                  result += 'B';
                }
                break;
            case 'C':
                if (next == 'H') { result += 'X'; i++; }
                else if (next == 'I' || next == 'E' || next == 'Y') result += 'S';
                else result += 'K';
                break;
            case 'D':
                if (next == 'G') { result += 'J'; i++; }
                else result += 'T';
                break;
            case 'F': result += 'F'; break;
            case 'G':
                if (next == 'H' || next == 'N') { i++; }
                else if (next == 'I' || next == 'E' || next == 'Y') result += 'J';
                else result += 'K';
                break;
            case 'H':
                if (i == 0 || !isVowel(upper[static_cast<int>(i - 1)])) {
                  result += 'H';
                }
                break;
            case 'J': result += 'J'; break;
            case 'K':
                if (i == 0 || upper[static_cast<int>(i - 1)] != 'C') {
                  result += 'K';
                }
                break;
            case 'L': result += 'L'; break;
            case 'M': result += 'M'; break;
            case 'N': result += 'N'; break;
            case 'P':
                if (next == 'H') { result += 'F'; i++; }
                else result += 'P';
                break;
            case 'Q': result += 'K'; break;
            case 'R': result += 'R'; break;
            case 'S':
                if (next == 'H') { result += 'X'; i++; }
                else result += 'S';
                break;
            case 'T':
                if (next == 'H') {
                    result += '0';
                    i++;
                } else if (!(next == 'C' && (i + 2 < upper.length() && upper[i + 2] == 'H'))) {
                    result += 'T';
                }
                break;
            case 'V': result += 'F'; break;
            case 'W': case 'Y':
                if (next != '\0' && isVowel(next)) {
                  result += c;
                }
                break;
            case 'X': result += "KS"; break;
            case 'Z': result += 'S'; break;
        }
        i++;
    }
    
    return result;
}

// ============================================================================
// Snippet / Highlight helpers
// ============================================================================

/// Collect the unique lower-case tokens from a query string or JSON array.
std::unordered_set<std::string> queryTermSet(const json& queryArg) {
    std::unordered_set<std::string> terms = {};

    if (queryArg.is_array()) {
        for (const auto& item : queryArg)
            if (item.is_string()) {
                std::string t = item.get<std::string>();
                std::transform(t.begin(), t.end(), t.begin(),
                               [](unsigned char c){ return std::tolower(c); });
                if (!t.empty()) {
                  terms.insert(std::move(t));
                }
            }
    } else if (queryArg.is_string()) {
        for (auto& t : tokenize(queryArg.get<std::string>()))
            if (!t.empty()) {
              terms.insert(std::move(t));
            }
    }
    return terms;
}

/// Apply open/close tag wrapping around every occurrence of any term in
/// @p text.  The function walks character by character to preserve original
/// capitalisation and whitespace while performing case-insensitive matching.
std::string applyHighlight(const std::string& text,
                           const std::unordered_set<std::string>& terms,
                           const std::string& openTag,
                           const std::string& closeTag) {
    if (terms.empty() || text.empty()) {
      return text;
    }

    // Build a lower-case shadow for scanning
    std::string lower(text.size(), '\0');
    std::transform(text.begin(), text.end(), lower.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    std::string result = {};
    result.reserve(static_cast<int>(text.size()) + 64);
    size_t i = 0;

    while (static_cast<size_t>(i) <static_cast<int>(text.size())) {
        // Skip non-alnum prefix until next word boundary
        if (!std::isalnum(static_cast<unsigned char>(text[i]))) {
            result += text[i++];
            continue;
        }
        // Find end of current word (alnum run)
        size_t end = i;
        while (end < text.size() &&
               std::isalnum(static_cast<unsigned char>(text[end]))) ++end;

        std::string word = lower.substr(i, end - i);
        if (terms.count(word)) {
            result += openTag;
            result.append(text, i, end - i);
            result += closeTag;
        } else {
            result.append(text, i, end - i);
        }
        i = end;
    }
    return result;
}

/// Find the byte offset of the window of @p windowSize characters that
/// contains the greatest number of term occurrences.  Returns 0 if no term
/// is found or the text fits within the window.
size_t bestSnippetOffset(const std::string& lower,
                         const std::unordered_set<std::string>& terms,
                         size_t windowSize) {
    if (static_cast<int>(lower.size()) <= windowSize) {
      return 0;
    }

    // Collect all match start positions
    std::vector<size_t> positions;
    size_t i = 0;
    while (static_cast<size_t>(i) <static_cast<int>(lower.size())) {
        if (!std::isalnum(static_cast<unsigned char>(lower[i]))) { ++i; continue; }
        size_t end = i;
        while (end < lower.size() &&
               std::isalnum(static_cast<unsigned char>(lower[end]))) ++end;
        if (terms.count(lower.substr(i, end - i))) {
          positions.push_back(i);
        }
        i = end;
    }

    if (positions.empty()) {
      return 0;
    }

    // Sliding-window: maximise term density
    size_t bestStart = 0, bestCount = 0;
    size_t lo = 0;
    for (size_t hi = 0; hi < positions.size(); ++hi) {
        while (positions[hi] - positions[lo] >= windowSize) {
          ++lo;
        }
        size_t count = hi - lo + 1;
        if (count > bestCount) {
            bestCount = count;
            // Centre the window around the match cluster if possible
            size_t mid = (positions[lo] + positions[hi]) / 2;
            bestStart = mid > windowSize / 2 ? mid - windowSize / 2 : 0;
        }
    }
    // Align to a boundary without rewinding across a very long token.
    // Rewinding can move the window far away from the actual match cluster.
    if (bestStart > 0 && std::isalnum(static_cast<unsigned char>(lower[bestStart]))) {
        while (bestStart < lower.size() &&
               std::isalnum(static_cast<unsigned char>(lower[bestStart]))) {
            ++bestStart;
        }
    }
    if (bestStart >= static_cast<int>(lower.size())) {
        bestStart = static_cast<int>(lower.size()) > windowSize ? (static_cast<int>(lower.size()) - windowSize) : 0;
    }
    return bestStart;
}

} // anonymous namespace

// ============================================================================
// Function Implementations
// ============================================================================

// FULLTEXT - Full-text search with BM25 scoring
/** @brief FULLTEXT - Full-text search with BM25 scoring. */
class FulltextFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            "FULLTEXT",
            "Fulltext",
            "Performs full-text BM25-scored search on a collection field (requires fulltext index)",
            {
                {"collection", ArgType::STRING, true, nullptr, "Collection name"},
                {"field", ArgType::STRING, true, nullptr, "Field name to search"},
                {"query", ArgType::STRING, true, nullptr, "Search query"},
                {"options", ArgType::OBJECT, false, json::object(), "Search options (limit)"}
            },
            ArgType::ARRAY,
            false, false,  // not deterministic (depends on index state), not aggregate
            {
                "FULLTEXT('articles', 'content', 'machine learning')",
                "FULLTEXT('documents', 'text', 'search term', {limit: 100})"
            },
            {CostComplexity::LINEAR, 10.0, 0.1, true, false, "fulltext"}
        };
    }

    json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
        if (static_cast<int>(args.size()) < 3) {
          return json::array();
        }
        const auto collection = args[0].get<std::string>();
        const auto field      = args[1].get<std::string>();
        const auto query      = args[2].get<std::string>();
        size_t limit = 1000;
        if (static_cast<int>(args.size()) > 3 && args[3].is_object() && args[3].contains("limit")) {
            const auto& lv = args[3]["limit"];
            if (lv.is_number_integer()) {
                int raw = lv.get<int>();
                if (raw > 0) {
                  limit = static_cast<size_t>(raw);
                }
            }
        }

        auto* idx = ctx.getSecondaryIndexManager();
        if (!idx) {
            // No index manager available — return empty with informational note
            json result = json::array();
            result.push_back({{"_note", "FULLTEXT: no SecondaryIndexManager in context"},
                              {"_collection", collection}, {"_field", field}, {"_query", query}});
            return result;
        }

        auto [st, results] = idx->scanFulltextWithScores(collection, field, query, limit);
        json out = json::array();
        if (!st.ok) {
            out.push_back({{"_error", st.message}, {"_collection", collection},
                           {"_field", field}, {"_query", query}});
            return out;
        }
        for (const auto& r : results)
            out.push_back({{"_key", r.pk}, {"_score", r.score}});
        return out;
    }
};

// PHRASE - Exact phrase matching
/** @brief PHRASE - Exact phrase matching. */
class PhraseFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            "PHRASE",
            "Fulltext",
            "Searches for exact phrase matches in a collection field (requires fulltext index)",
            {
                {"collection", ArgType::STRING, true, nullptr, "Collection name"},
                {"field", ArgType::STRING, true, nullptr, "Field name to search"},
                {"phrase", ArgType::STRING, true, nullptr, "Phrase to search for"},
                {"options", ArgType::OBJECT, false, json::object(), "Search options (limit)"}
            },
            ArgType::ARRAY,
            false, false,
            {
                "PHRASE('articles', 'content', 'machine learning')",
                "PHRASE('docs', 'text', 'deep neural networks', {limit: 50})"
            },
            {CostComplexity::LINEAR, 15.0, 0.2, true, false, "fulltext"}
        };
    }

    json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
        if (static_cast<int>(args.size()) < 3) {
          return json::array();
        }
        const auto collection = args[0].get<std::string>();
        const auto field      = args[1].get<std::string>();
        const auto phrase     = args[2].get<std::string>();
        size_t limit = 1000;
        if (static_cast<int>(args.size()) > 3 && args[3].is_object() && args[3].contains("limit")) {
            const auto& lv = args[3]["limit"];
            if (lv.is_number_integer()) {
                int raw = lv.get<int>();
                if (raw > 0) {
                  limit = static_cast<size_t>(raw);
                }
            }
        }

        auto* idx = ctx.getSecondaryIndexManager();
        if (!idx) {
            json result = json::array();
            result.push_back({{"_note", "PHRASE: no SecondaryIndexManager in context"},
                              {"_collection", collection}, {"_field", field}, {"_phrase", phrase}});
            return result;
        }

        auto [st, results] = idx->scanFulltextPhrase(collection, field, phrase, limit);
        json out = json::array();
        if (!st.ok) {
            out.push_back({{"_error", st.message}, {"_collection", collection},
                           {"_field", field}, {"_phrase", phrase}});
            return out;
        }
        for (const auto& r : results)
            out.push_back({{"_key", r.pk}, {"_score", r.score}});
        return out;
    }
};

// FUZZY - Fuzzy matching with Levenshtein distance
/** @brief FUZZY - Fuzzy matching with Levenshtein distance. */
class FuzzyFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            "FUZZY",
            "Fulltext",
            "Performs fuzzy search using Levenshtein distance (requires fulltext index)",
            {
                {"collection", ArgType::STRING, true, nullptr, "Collection name"},
                {"field", ArgType::STRING, true, nullptr, "Field name to search"},
                {"query", ArgType::STRING, true, nullptr, "Search query"},
                {"maxDistance", ArgType::INTEGER, false, 2, "Maximum Levenshtein distance"},
                {"limit", ArgType::INTEGER, false, 1000, "Result limit"}
            },
            ArgType::ARRAY,
            false, false,
            {
                "FUZZY('users', 'name', 'Jon', 1)",
                "FUZZY('products', 'title', 'computr', 2, 50)"
            },
            {CostComplexity::LINEAR, 20.0, 0.3, true, false, "fulltext"}
        };
    }

    json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
        if (static_cast<int>(args.size()) < 3) {
          return json::array();
        }
        const auto collection = args[0].get<std::string>();
        const auto field      = args[1].get<std::string>();
        const auto query      = args[2].get<std::string>();
        int maxDistance = 2;
        if (static_cast<int>(args.size()) > 3 && args[3].is_number_integer()) {
            maxDistance = args[3].get<int>();
            if (maxDistance < 0) {
              maxDistance = 0;
            }
        }
        size_t limit = 1000;
        if (static_cast<int>(args.size()) > 4 && args[4].is_number_integer()) {
            int raw = args[4].get<int>();
            if (raw > 0) {
              limit = static_cast<size_t>(raw);
            }
        }

        auto* idx = ctx.getSecondaryIndexManager();
        if (!idx) {
            json result = json::array();
            result.push_back({{"_note", "FUZZY: no SecondaryIndexManager in context"},
                              {"_collection", collection}, {"_field", field}, {"_query", query},
                              {"_maxDistance", maxDistance}});
            return result;
        }

        auto [st, results] = idx->scanFulltextFuzzy(collection, field, query, maxDistance, limit);
        json out = json::array();
        if (!st.ok) {
            out.push_back({{"_error", st.message}, {"_collection", collection},
                           {"_field", field}, {"_query", query}});
            return out;
        }
        for (const auto& r : results)
            out.push_back({{"_key", r.pk}, {"_score", r.score}});
        return out;
    }
};

// ============================================================================
// HIGHLIGHT - wrap query terms in the source text with configurable tags
// ============================================================================

// HIGHLIGHT(text, query [, options])
//   text    - source string to annotate
//   query   - search string (tokenised) or JSON array of term strings
//   options - optional object: {openTag: "<em>", closeTag: "</em>"}
//
// Returns: @p text with every occurrence of a query term wrapped in
//   openTag...closeTag (case-insensitive match, original case preserved).
//
// Example:
//   HIGHLIGHT("Machine Learning is great", "machine learning")
//   → "<em>Machine</em> <em>Learning</em> is great"
/** @brief → "<em>Machine</em> <em>Learning</em> is great". */
class HighlightFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            "HIGHLIGHT",
            "Fulltext",
            "Wraps query terms in source text with configurable HTML/markup tags",
            {
                {"text",    ArgType::STRING, true,  nullptr,       "Source text to annotate"},
                {"query",   ArgType::ANY,    true,  nullptr,       "Query string or array of terms to highlight"},
                {"options", ArgType::OBJECT, false, json::object(),"Options: {openTag, closeTag}"}
            },
            ArgType::STRING,
            true, false,  // deterministic, not aggregate
            {
                "HIGHLIGHT(doc.content, 'machine learning')",
                "HIGHLIGHT(doc.title, query, {openTag: '<b>', closeTag: '</b>'})"
            },
            {CostComplexity::LINEAR, 1.0, 0.01, false, true, ""}
        };
    }

    json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
        if (static_cast<int>(args.size()) < 2) {
          return "";
        }
        if (!args[0].is_string()) {
          return args[0];
        }

        const std::string text  = args[0].get<std::string>();
        std::string openTag     = "<em>";
        std::string closeTag    = "</em>";

        if (static_cast<int>(args.size()) > 2 && args[2].is_object()) {
            if (args[2].contains("openTag")  && args[2]["openTag"].is_string())
                openTag  = args[2]["openTag"].get<std::string>();
            if (args[2].contains("closeTag") && args[2]["closeTag"].is_string())
                closeTag = args[2]["closeTag"].get<std::string>();
        }

        auto terms = queryTermSet(args[1]);
        return applyHighlight(text, terms, openTag, closeTag);
    }
};

// ============================================================================
// FULLTEXT_SNIPPET - extract and highlight a context window
// ============================================================================

// FULLTEXT_SNIPPET(text, query [, options])
//   text    - full document text
//   query   - search string or array of terms
//   options - optional: {windowSize: 200, openTag: "<em>", closeTag: "</em>",
//                        separator: "..."}
//
// Returns: a short excerpt of @p text (≤ windowSize chars) centred around
//   the densest cluster of query-term matches, with the matched terms
//   wrapped in openTag/closeTag.  "..." separators are prepended/appended
//   when the text is truncated.
//
// Example:
//   FULLTEXT_SNIPPET("...long document...", "neural networks", {windowSize:100})
//   → "...activating <em>neural</em> <em>networks</em> for training..."
/** @brief → "...activating <em>neural</em> <em>networks</em> for training...". */
class FulltextSnippetFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            "FULLTEXT_SNIPPET",
            "Fulltext",
            "Extracts a highlighted context snippet from text around the best query-term match",
            {
                {"text",    ArgType::STRING, true,  nullptr,       "Full document text"},
                {"query",   ArgType::ANY,    true,  nullptr,       "Query string or array of terms"},
                {"options", ArgType::OBJECT, false, json::object(),"Options: {windowSize, openTag, closeTag, separator}"}
            },
            ArgType::STRING,
            true, false,
            {
                "FULLTEXT_SNIPPET(doc.content, 'machine learning')",
                "FULLTEXT_SNIPPET(doc.body, 'neural network', {windowSize: 150, openTag: '<mark>', closeTag: '</mark>'})"
            },
            {CostComplexity::LINEAR, 2.0, 0.02, false, true, ""}
        };
    }

    json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
        if (static_cast<int>(args.size()) < 2) {
          return "";
        }
        if (!args[0].is_string()) {
          return "";
        }

        const std::string text  = args[0].get<std::string>();
        size_t windowSize       = 200;
        std::string openTag     = "<em>";
        std::string closeTag    = "</em>";
        std::string separator   = "...";

        if (static_cast<int>(args.size()) > 2 && args[2].is_object()) {
            const auto& opts = args[2];
            if (opts.contains("windowSize") && opts["windowSize"].is_number_integer()) {
                int raw = opts["windowSize"].get<int>();
                if (raw > 0) {
                  windowSize = static_cast<size_t>(raw);
                }
            }
            if (opts.contains("openTag")   && opts["openTag"].is_string())
                openTag    = opts["openTag"].get<std::string>();
            if (opts.contains("closeTag")  && opts["closeTag"].is_string())
                closeTag   = opts["closeTag"].get<std::string>();
            if (opts.contains("separator") && opts["separator"].is_string())
                separator  = opts["separator"].get<std::string>();
        }

        auto terms = queryTermSet(args[1]);

        if (static_cast<int>(text.size()) <= windowSize) {
            // Text fits — just highlight the whole thing
            return applyHighlight(text, terms, openTag, closeTag);
        }

        // Build lower-case shadow for position scanning
        std::string lower(text.size(), '\0');
        std::transform(text.begin(), text.end(), lower.begin(),
                       [](unsigned char c){ return std::tolower(c); });

        size_t start = bestSnippetOffset(lower, terms, windowSize);
        bool truncLeft  = (start > 0);
        bool truncRight = (start + windowSize < text.size());

        std::string excerpt = text.substr(start, windowSize);

        std::string highlighted = applyHighlight(excerpt, terms, openTag, closeTag);
        std::string result = {};
        if (truncLeft) {
          result += separator;
        }
        result += highlighted;
        if (truncRight) {
          result += separator;
        }
        return result;
    }
};

// NGRAM_MATCH - N-gram based similarity matching
/** @brief NGRAM_MATCH - N-gram based similarity matching. */
class NgramMatchFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            "NGRAM_MATCH",
            "String",
            "Calculates n-gram similarity between two strings (0.0-1.0)",
            {
                {"string1", ArgType::STRING, true, nullptr, "First string"},
                {"string2", ArgType::STRING, true, nullptr, "Second string"},
                {"n", ArgType::INTEGER, false, 2, "N-gram size (default: 2)"}
            },
            ArgType::NUMBER,
            true, false,  // deterministic, not aggregate
            {
                "NGRAM_MATCH('hello', 'hallo')",
                "NGRAM_MATCH('machine', 'matching', 3)"
            },
            {CostComplexity::LINEAR, 1.0, 0.01, false, false, ""}
        };
    }
    
    json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
        if (static_cast<int>(args.size()) < 2) {
          return 0.0;
        }
        
        std::string s1 = args[0].get<std::string>();
        std::string s2 = args[1].get<std::string>();
        int n = static_cast<int>(args.size()) > 2 ? args[2].get<int>() : 2;
        
        if (s1.empty() || s2.empty()) {
          return 0.0;
        }
        if (n < 1) {
          n = 2;
        }
        
        auto ngrams1 = generateNgrams(s1, n);
        auto ngrams2 = generateNgrams(s2, n);
        
        std::unordered_map<std::string, int> count1, count2;
        for (const auto& ng : ngrams1) {
          count1[ng]++;
        }
        for (const auto& ng : ngrams2) {
          count2[ng]++;
        }
        
        int intersection = 0;
        for (const auto& [ng, c] : count1) {
            if (count2.count(ng)) {
                intersection += std::min(c, count2[ng]);
            }
        }
        
        const size_t totalSz = static_cast<int>(ngrams1.size()) + static_cast<int>(ngrams2.size()) ;
        if (totalSz == 0) {
          return 0.0;
        }
        
        return 2.0 * static_cast<double>(intersection) / static_cast<double>(totalSz);
    }
};

// TOKENS - Tokenize text
/** @brief TOKENS - Tokenize text. */
class TokensFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            "TOKENS",
            "String",
            "Tokenizes text into an array of tokens",
            {
                {"text", ArgType::STRING, true, nullptr, "Text to tokenize"},
                {"analyzer", ArgType::STRING, false, "text_en", "Analyzer type (currently unused)"}
            },
            ArgType::ARRAY,
            true, false,
            {
                "TOKENS('Hello world, this is a test!')",
                "TOKENS(doc.content, 'text_en')"
            },
            {CostComplexity::LINEAR, 1.0, 0.005, false, true, ""}
        };
    }
    
    json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
        if (args.empty()) {
          return json::array();
        }
        
        std::string text = args[0].get<std::string>();
        // analyzer parameter is currently ignored
        
        auto tokens = tokenize(text);
        return json(tokens);
    }
};

// SOUNDEX - Phonetic encoding
/** @brief SOUNDEX - Phonetic encoding. */
class SoundexFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            "SOUNDEX",
            "String",
            "Returns Soundex phonetic encoding of a string",
            {
                {"text", ArgType::STRING, true, nullptr, "Input string"}
            },
            ArgType::STRING,
            true, false,
            {
                "SOUNDEX('Smith')",
                "SOUNDEX('Johnson')"
            },
            {CostComplexity::CONSTANT, 1.0, 0.0, false, true, ""}
        };
    }
    
    json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
        if (args.empty()) {
          return "";
        }
        
        std::string s = args[0].get<std::string>();
        return soundex(s);
    }
};

// METAPHONE - Phonetic encoding
/** @brief METAPHONE - Phonetic encoding. */
class MetaphoneFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            "METAPHONE",
            "String",
            "Returns Metaphone phonetic encoding of a string",
            {
                {"text", ArgType::STRING, true, nullptr, "Input string"},
                {"maxLength", ArgType::INTEGER, false, 6, "Maximum length of result"}
            },
            ArgType::STRING,
            true, false,
            {
                "METAPHONE('Smith')",
                "METAPHONE('Johnson', 4)"
            },
            {CostComplexity::LINEAR, 1.0, 0.01, false, true, ""}
        };
    }
    
    json execute(const std::vector<json>& args, const FunctionContext& /*ctx*/) const override {
        if (args.empty()) {
          return "";
        }
        
        std::string word = args[0].get<std::string>();
        int maxLen = static_cast<int>(args.size()) > 1 ? args[1].get<int>() : 6;
        
        return metaphone(word, maxLen);
    }
};

// DOUBLE_METAPHONE - Enhanced phonetic encoding
/** @brief DOUBLE_METAPHONE - Enhanced phonetic encoding. */
class DoubleMetaphoneFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            "DOUBLE_METAPHONE",
            "String",
            "Returns Double Metaphone encoding (primary and secondary codes)",
            {
                {"text", ArgType::STRING, true, nullptr, "Input string"}
            },
            ArgType::OBJECT,
            true, false,
            {"DOUBLE_METAPHONE('Smith')"},
            {CostComplexity::LINEAR, 2.0, 0.02, false, true, ""}
        };
    }
    
    json execute(const std::vector<json>& args, const FunctionContext& ctx) const override {
        if (args.empty()) {
            return json{{"primary", ""}, {"secondary", ""}};
        }
        
        // Simplified - returns same as Metaphone for both
        MetaphoneFunction mf;
        std::string primary = mf.execute(args, ctx).get<std::string>();
        
        return json{
            {"primary", primary},
            {"secondary", primary}
        };
    }
};

// ============================================================================
// Registration
// ============================================================================

void registerFulltextFunctions(FunctionRegistry& registry) {
    registry.registerFunction(std::make_unique<FulltextFunction>());
    registry.registerFunction(std::make_unique<PhraseFunction>());
    registry.registerFunction(std::make_unique<FuzzyFunction>());
    registry.registerFunction(std::make_unique<HighlightFunction>());
    registry.registerFunction(std::make_unique<FulltextSnippetFunction>());
    registry.registerFunction(std::make_unique<NgramMatchFunction>());
    registry.registerFunction(std::make_unique<TokensFunction>());
    registry.registerFunction(std::make_unique<SoundexFunction>());
    registry.registerFunction(std::make_unique<MetaphoneFunction>());
    registry.registerFunction(std::make_unique<DoubleMetaphoneFunction>());
}

} // namespace functions
} // namespace query
} // namespace themis

