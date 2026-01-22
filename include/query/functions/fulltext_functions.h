/**
 * @file fulltext_functions.h
 * @brief Fulltext Search Functions for ThemisDB AQL
 * 
 * Provides comprehensive fulltext search capabilities including:
 * - Full-text search with scoring
 * - Phrase matching
 * - N-gram similarity
 * - Phonetic matching (Soundex, Metaphone)
 * - Text tokenization
 */

#pragma once

#include "function_registry.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <sstream>

namespace themisdb {
namespace query {
namespace functions {

// ============================================================================
// FULLTEXT - Full-text search with scoring
// ============================================================================

class FulltextFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            "FULLTEXT",
            {ParamType::STRING, ParamType::STRING, ParamType::STRING},  // collection, field, query
            ParamType::ARRAY,
            2, 4,  // min/max args (options optional)
            "Performs full-text search on a collection field",
            FunctionCost{CostComplexity::LINEAR, 10.0, 0.1, true, false, "fulltext"}
        };
    }
    
    JsonValue execute(const std::vector<JsonValue>& args, ExecutionContext& ctx) const override {
        // Returns array of {doc, score} objects
        // Implementation would use inverted index
        return JsonValue::array();
    }
};

// ============================================================================
// PHRASE - Exact phrase matching
// ============================================================================

class PhraseFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            "PHRASE",
            {ParamType::STRING, ParamType::STRING, ParamType::STRING},
            ParamType::ARRAY,
            3, 4,
            "Searches for exact phrase matches in a collection field",
            FunctionCost{CostComplexity::LINEAR, 15.0, 0.2, true, false, "fulltext"}
        };
    }
    
    JsonValue execute(const std::vector<JsonValue>& args, ExecutionContext& ctx) const override {
        // Note: Actual implementation would query the SecondaryIndexManager
        // This is a placeholder that documents the expected interface
        // Real implementation should call ctx.indexManager->scanFulltextPhrase()
        return JsonValue::array();
    }
};

// ============================================================================
// FUZZY - Fuzzy matching with Levenshtein distance
// ============================================================================

class FuzzyFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            "FUZZY",
            {ParamType::STRING, ParamType::STRING, ParamType::STRING, ParamType::NUMBER, ParamType::NUMBER},
            ParamType::ARRAY,
            3, 5,  // collection, field, query, optional maxDistance (INTEGER), optional limit (INTEGER)
            "Performs fuzzy search using Levenshtein distance",
            FunctionCost{CostComplexity::LINEAR, 20.0, 0.3, true, false, "fulltext"}
        };
    }
    
    JsonValue execute(const std::vector<JsonValue>& args, ExecutionContext& ctx) const override {
        // Note: Actual implementation would query the SecondaryIndexManager
        // This is a placeholder that documents the expected interface
        // Real implementation should call ctx.indexManager->scanFulltextFuzzy()
        return JsonValue::array();
    }
};

// ============================================================================
// NGRAM_MATCH - N-gram based similarity matching
// ============================================================================

class NgramMatchFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            "NGRAM_MATCH",
            {ParamType::STRING, ParamType::STRING},
            ParamType::NUMBER,
            2, 3,  // optional n parameter (default 2)
            "Calculates n-gram similarity between two strings (0.0-1.0)",
            FunctionCost{CostComplexity::LINEAR, 1.0, 0.01, false, false, ""}
        };
    }
    
    JsonValue execute(const std::vector<JsonValue>& args, ExecutionContext& ctx) const override {
        if (args.size() < 2) return JsonValue(0.0);
        
        std::string s1 = args[0].as_string();
        std::string s2 = args[1].as_string();
        int n = args.size() > 2 ? static_cast<int>(args[2].as_number()) : 2;
        
        if (s1.empty() || s2.empty()) return JsonValue(0.0);
        if (n < 1) n = 2;
        
        auto getNgrams = [n](const std::string& s) -> std::vector<std::string> {
            std::vector<std::string> ngrams;
            if (s.length() < static_cast<size_t>(n)) {
                ngrams.push_back(s);
                return ngrams;
            }
            for (size_t i = 0; i <= s.length() - n; ++i) {
                ngrams.push_back(s.substr(i, n));
            }
            return ngrams;
        };
        
        auto ngrams1 = getNgrams(s1);
        auto ngrams2 = getNgrams(s2);
        
        std::unordered_map<std::string, int> count1, count2;
        for (const auto& ng : ngrams1) count1[ng]++;
        for (const auto& ng : ngrams2) count2[ng]++;
        
        int intersection = 0;
        for (const auto& [ng, c] : count1) {
            if (count2.count(ng)) {
                intersection += std::min(c, count2[ng]);
            }
        }
        
        int total = ngrams1.size() + ngrams2.size();
        if (total == 0) return JsonValue(0.0);
        
        return JsonValue(2.0 * intersection / total);
    }
};

// ============================================================================
// TOKENS - Tokenize text
// ============================================================================

class TokensFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            "TOKENS",
            {ParamType::STRING},
            ParamType::ARRAY,
            1, 2,  // optional analyzer parameter
            "Tokenizes text into an array of tokens",
            FunctionCost{CostComplexity::LINEAR, 1.0, 0.005, false, true, ""}
        };
    }
    
    JsonValue execute(const std::vector<JsonValue>& args, ExecutionContext& ctx) const override {
        if (args.empty()) return JsonValue::array();
        
        std::string text = args[0].as_string();
        std::string analyzer = args.size() > 1 ? args[1].as_string() : "text_en";
        
        // Simple whitespace/punctuation tokenizer
        std::vector<JsonValue> tokens;
        std::string current;
        
        for (char c : text) {
            if (std::isalnum(c)) {
                current += std::tolower(c);
            } else if (!current.empty()) {
                tokens.push_back(JsonValue(current));
                current.clear();
            }
        }
        if (!current.empty()) {
            tokens.push_back(JsonValue(current));
        }
        
        return JsonValue(tokens);
    }
};

// ============================================================================
// SOUNDEX - Phonetic encoding (Soundex algorithm)
// ============================================================================

class SoundexFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            "SOUNDEX",
            {ParamType::STRING},
            ParamType::STRING,
            1, 1,
            "Returns Soundex phonetic encoding of a string",
            FunctionCost{CostComplexity::CONSTANT, 1.0, 0.0, false, true, ""}
        };
    }
    
    JsonValue execute(const std::vector<JsonValue>& args, ExecutionContext& ctx) const override {
        if (args.empty()) return JsonValue("");
        
        std::string s = args[0].as_string();
        if (s.empty()) return JsonValue("");
        
        // Soundex algorithm
        std::string result;
        result += std::toupper(s[0]);
        
        auto getCode = [](char c) -> char {
            c = std::toupper(c);
            if (c == 'B' || c == 'F' || c == 'P' || c == 'V') return '1';
            if (c == 'C' || c == 'G' || c == 'J' || c == 'K' || c == 'Q' || c == 'S' || c == 'X' || c == 'Z') return '2';
            if (c == 'D' || c == 'T') return '3';
            if (c == 'L') return '4';
            if (c == 'M' || c == 'N') return '5';
            if (c == 'R') return '6';
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
        
        while (result.length() < 4) result += '0';
        return JsonValue(result);
    }
};

// ============================================================================
// METAPHONE - Phonetic encoding (Metaphone algorithm)
// ============================================================================

class MetaphoneFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            "METAPHONE",
            {ParamType::STRING},
            ParamType::STRING,
            1, 2,  // optional max_length
            "Returns Metaphone phonetic encoding of a string",
            FunctionCost{CostComplexity::LINEAR, 1.0, 0.01, false, true, ""}
        };
    }
    
    JsonValue execute(const std::vector<JsonValue>& args, ExecutionContext& ctx) const override {
        if (args.empty()) return JsonValue("");
        
        std::string word = args[0].as_string();
        int maxLen = args.size() > 1 ? static_cast<int>(args[1].as_number()) : 6;
        
        if (word.empty()) return JsonValue("");
        
        // Simplified Metaphone implementation
        std::string result;
        std::string upper;
        for (char c : word) upper += std::toupper(c);
        
        size_t i = 0;
        
        // Skip initial silent letters
        if (upper.length() >= 2) {
            std::string start = upper.substr(0, 2);
            if (start == "KN" || start == "GN" || start == "PN" || start == "AE" || start == "WR") {
                i = 1;
            }
        }
        
        while (i < upper.length() && static_cast<int>(result.length()) < maxLen) {
            char c = upper[i];
            char next = (i + 1 < upper.length()) ? upper[i + 1] : '\0';
            
            switch (c) {
                case 'A': case 'E': case 'I': case 'O': case 'U':
                    if (i == 0) result += c;
                    break;
                case 'B':
                    if (i == 0 || upper[i - 1] != 'M') result += 'B';
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
                    if (i == 0 || !isVowel(upper[i - 1])) result += 'H';
                    break;
                case 'J': result += 'J'; break;
                case 'K':
                    if (i == 0 || upper[i - 1] != 'C') result += 'K';
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
                    if (next == 'H') { result += '0'; i++; }  // TH sound
                    else if (next != 'C' || (i + 2 < upper.length() && upper[i + 2] == 'H')) result += 'T';
                    break;
                case 'V': result += 'F'; break;
                case 'W': case 'Y':
                    if (next != '\0' && isVowel(next)) result += c;
                    break;
                case 'X': result += "KS"; break;
                case 'Z': result += 'S'; break;
            }
            i++;
        }
        
        return JsonValue(result);
    }
    
private:
    static bool isVowel(char c) {
        return c == 'A' || c == 'E' || c == 'I' || c == 'O' || c == 'U';
    }
};

// ============================================================================
// DOUBLE_METAPHONE - Enhanced phonetic encoding
// ============================================================================

class DoubleMetaphoneFunction : public IFunction {
public:
    FunctionSignature signature() const override {
        return {
            "DOUBLE_METAPHONE",
            {ParamType::STRING},
            ParamType::OBJECT,  // Returns {primary, secondary}
            1, 1,
            "Returns Double Metaphone encoding (primary and secondary codes)",
            FunctionCost{CostComplexity::LINEAR, 2.0, 0.02, false, true, ""}
        };
    }
    
    JsonValue execute(const std::vector<JsonValue>& args, ExecutionContext& ctx) const override {
        if (args.empty()) {
            return JsonValue::object({{"primary", ""}, {"secondary", ""}});
        }
        
        // Simplified - returns same as Metaphone for both
        MetaphoneFunction mf;
        std::string primary = mf.execute(args, ctx).as_string();
        
        return JsonValue::object({
            {"primary", primary},
            {"secondary", primary}
        });
    }
};

// ============================================================================
// Registration
// ============================================================================

inline void registerFulltextFunctions(FunctionRegistry& registry) {
    registry.registerFunction(std::make_unique<FulltextFunction>());
    registry.registerFunction(std::make_unique<PhraseFunction>());
    registry.registerFunction(std::make_unique<FuzzyFunction>());
    registry.registerFunction(std::make_unique<NgramMatchFunction>());
    registry.registerFunction(std::make_unique<TokensFunction>());
    registry.registerFunction(std::make_unique<SoundexFunction>());
    registry.registerFunction(std::make_unique<MetaphoneFunction>());
    registry.registerFunction(std::make_unique<DoubleMetaphoneFunction>());
}

} // namespace functions
} // namespace query
} // namespace themisdb
