/**
 * @file string_functions.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "query/functions/function_registry.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <regex>

namespace themis {
namespace query {
namespace functions {

// ============================================================================
// String Functions
// ============================================================================

/**
 * @brief LENGTH(value) - Length of string, array, or object
 */
class LengthFunction : public IFunction {
public:
    ~LengthFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "LENGTH",
            .category = "String",
            .description = "Returns the length of a string, array, or object",
            .arguments = {
                {"value", ArgType::ANY, true, nullptr, "String, array, or object"}
            },
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {
                R"(LENGTH("hello") // 5)",
                R"(LENGTH([1, 2, 3]) // 3)"
            },
            .cost = FunctionCost{}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        const auto& val = args[0];
        if (val.is_string()) {
          return static_cast<int64_t>(val.get<std::string>().length());
        }
        if (val.is_array()) {
          return static_cast<int64_t>(val.size());
        }
        if (val.is_object()) {
          return static_cast<int64_t>(val.size());
        }
        if (val.is_null()) {
          return 0;
        }
        return 0;
    }
};

/**
 * @brief CONCAT(s1, s2, ...) - Concatenate strings
 */
class ConcatFunction : public IFunction {
public:
    ~ConcatFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "CONCAT",
            .category = "String",
            .description = "Concatenates all arguments into a single string",
            .arguments = {
                {"values", ArgType::ANY, true, nullptr, "Values to concatenate (variadic)"}
            },
            .return_type = ArgType::STRING,
            .is_deterministic = true,
            .examples = {
                R"(CONCAT("Hello", " ", "World") // "Hello World")"
            },
            .cost = FunctionCost{}
        };
    }
    
    void validateArgs(const std::vector<nlohmann::json>&) const override {
        // Accept any number of arguments
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::string result = {};
        for (const auto& arg : args) {
            result += toString(arg);
        }
        return result;
    }
};

/**
 * @brief SUBSTRING(str, start [, length]) - Extract substring
 */
class SubstringFunction : public IFunction {
public:
    ~SubstringFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "SUBSTRING",
            .category = "String",
            .description = "Extracts a substring from a string",
            .arguments = {
                {"str", ArgType::STRING, true, nullptr, "Source string"},
                {"start", ArgType::INTEGER, true, nullptr, "Start position (0-based)"},
                {"length", ArgType::INTEGER, false, nullptr, "Number of characters (optional)"}
            },
            .return_type = ArgType::STRING,
            .is_deterministic = true,
            .examples = {
                R"(SUBSTRING("Hello World", 6) // "World")",
                R"(SUBSTRING("Hello World", 0, 5) // "Hello")"
            },
            .cost = FunctionCost{}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::string str = args[0].get<std::string>();
        // Guard against negative start: clamp to 0 before cast to size_t.
        const int64_t rawStart = args[1].get<int64_t>();
        size_t start = (rawStart <= 0) ? 0 : static_cast<size_t>(rawStart);
        
        if (start >= str.length()) {
          return "";
        }
        
        if (args.size() > 2) {
            const int64_t rawLen = args[2].get<int64_t>();
            size_t len = (rawLen <= 0) ? 0 : static_cast<size_t>(rawLen);
            return str.substr(start, len);
        }
        return str.substr(start);
    }
};

/**
 * @brief UPPER(str) - Convert to uppercase
 */
class UpperFunction : public IFunction {
public:
    ~UpperFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "UPPER",
            .category = "String",
            .description = "Converts a string to uppercase",
            .arguments = {
                {"str", ArgType::STRING, true, nullptr, "String to convert"}
            },
            .return_type = ArgType::STRING,
            .is_deterministic = true,
            .examples = {R"(UPPER("hello") // "HELLO")"},
            .cost = FunctionCost{}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::string str = args[0].get<std::string>();
        std::transform(str.begin(), str.end(), str.begin(), ::toupper);
        return str;
    }
};

/**
 * @brief LOWER(str) - Convert to lowercase
 */
class LowerFunction : public IFunction {
public:
    ~LowerFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "LOWER",
            .category = "String",
            .description = "Converts a string to lowercase",
            .arguments = {
                {"str", ArgType::STRING, true, nullptr, "String to convert"}
            },
            .return_type = ArgType::STRING,
            .is_deterministic = true,
            .examples = {R"(LOWER("HELLO") // "hello")"},
            .cost = FunctionCost{}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::string str = args[0].get<std::string>();
        std::transform(str.begin(), str.end(), str.begin(), ::tolower);
        return str;
    }
};

/**
 * @brief TRIM(str [, chars]) - Remove leading/trailing whitespace or chars
 */
class TrimFunction : public IFunction {
public:
    ~TrimFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "TRIM",
            .category = "String",
            .description = "Removes leading and trailing whitespace or specified characters",
            .arguments = {
                {"str", ArgType::STRING, true, nullptr, "String to trim"},
                {"chars", ArgType::STRING, false, " \t\n\r", "Characters to remove"}
            },
            .return_type = ArgType::STRING,
            .is_deterministic = true,
            .examples = {
                R"(TRIM("  hello  ") // "hello")",
                R"(TRIM("xxhelloxx", "x") // "hello")"
            },
            .cost = FunctionCost{}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::string str = args[0].get<std::string>();
        std::string chars = args.size() > 1 ? args[1].get<std::string>() : " \t\n\r";
        
        size_t start = str.find_first_not_of(chars);
        if (start == std::string::npos) {
          return "";
        }
        
        size_t end = str.find_last_not_of(chars);
        return str.substr(start, end - start + 1);
    }
};

/**
 * @brief LTRIM(str [, chars]) - Remove leading whitespace
 */
class LTrimFunction : public IFunction {
public:
    ~LTrimFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "LTRIM",
            .category = "String",
            .description = "Removes leading whitespace or specified characters",
            .arguments = {
                {"str", ArgType::STRING, true, nullptr, "String to trim"},
                {"chars", ArgType::STRING, false, " \t\n\r", "Characters to remove"}
            },
            .return_type = ArgType::STRING,
            .is_deterministic = true,
            .examples = {R"(LTRIM("  hello") // "hello")"},
            .cost = FunctionCost{}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::string str = args[0].get<std::string>();
        std::string chars = args.size() > 1 ? args[1].get<std::string>() : " \t\n\r";
        
        size_t start = str.find_first_not_of(chars);
        if (start == std::string::npos) {
          return "";
        }
        return str.substr(start);
    }
};

/**
 * @brief RTRIM(str [, chars]) - Remove trailing whitespace
 */
class RTrimFunction : public IFunction {
public:
    ~RTrimFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "RTRIM",
            .category = "String",
            .description = "Removes trailing whitespace or specified characters",
            .arguments = {
                {"str", ArgType::STRING, true, nullptr, "String to trim"},
                {"chars", ArgType::STRING, false, " \t\n\r", "Characters to remove"}
            },
            .return_type = ArgType::STRING,
            .is_deterministic = true,
            .examples = {R"(RTRIM("hello  ") // "hello")"},
            .cost = FunctionCost{}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::string str = args[0].get<std::string>();
        std::string chars = args.size() > 1 ? args[1].get<std::string>() : " \t\n\r";
        
        size_t end = str.find_last_not_of(chars);
        if (end == std::string::npos) {
          return "";
        }
        return str.substr(0, end + 1);
    }
};

/**
 * @brief SPLIT(str, separator [, limit]) - Split string into array
 */
class SplitFunction : public IFunction {
public:
    ~SplitFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "SPLIT",
            .category = "String",
            .description = "Splits a string into an array by a separator",
            .arguments = {
                {"str", ArgType::STRING, true, nullptr, "String to split"},
                {"separator", ArgType::STRING, true, nullptr, "Separator string"},
                {"limit", ArgType::INTEGER, false, nullptr, "Maximum number of splits"}
            },
            .return_type = ArgType::ARRAY,
            .is_deterministic = true,
            .examples = {R"(SPLIT("a,b,c", ",") // ["a", "b", "c"])"},
            .cost = FunctionCost{}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::string str = args[0].get<std::string>();
        std::string sep = args[1].get<std::string>();
        int64_t limit = args.size() > 2 ? args[2].get<int64_t>() : -1;
        
        nlohmann::json result = nlohmann::json::array();
        size_t start = 0;
        size_t end = {};
        int64_t count = 0;
        
        while ((end = str.find(sep, start)) != std::string::npos) {
            if (limit >= 0 && count >= limit - 1) {
              break;
            }
            result.push_back(str.substr(start, end - start));
            start = end + sep.length();
            count++;
        }
        result.push_back(str.substr(start));
        
        return result;
    }
};

/**
 * @brief CONTAINS(str, search) - Check if string contains substring
 */
class ContainsFunction : public IFunction {
public:
    ~ContainsFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "CONTAINS",
            .category = "String",
            .description = "Checks if a string contains a substring",
            .arguments = {
                {"str", ArgType::STRING, true, nullptr, "String to search in"},
                {"search", ArgType::STRING, true, nullptr, "Substring to find"}
            },
            .return_type = ArgType::BOOLEAN,
            .is_deterministic = true,
            .examples = {R"(CONTAINS("Hello World", "World") // true)"},
            .cost = FunctionCost{}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::string str = args[0].get<std::string>();
        std::string search = args[1].get<std::string>();
        return str.find(search) != std::string::npos;
    }
};

/**
 * @brief STARTS_WITH(str, prefix) - Check if string starts with prefix
 */
class StartsWithFunction : public IFunction {
public:
    ~StartsWithFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "STARTS_WITH",
            .category = "String",
            .description = "Checks if a string starts with a prefix",
            .arguments = {
                {"str", ArgType::STRING, true, nullptr, "String to check"},
                {"prefix", ArgType::STRING, true, nullptr, "Prefix to find"}
            },
            .return_type = ArgType::BOOLEAN,
            .is_deterministic = true,
            .examples = {R"(STARTS_WITH("Hello World", "Hello") // true)"},
            .cost = FunctionCost{}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::string str = args[0].get<std::string>();
        std::string prefix = args[1].get<std::string>();
        return str.compare(0, prefix.length(), prefix) == 0;
    }
};

/**
 * @brief ENDS_WITH(str, suffix) - Check if string ends with suffix
 */
class EndsWithFunction : public IFunction {
public:
    ~EndsWithFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "ENDS_WITH",
            .category = "String",
            .description = "Checks if a string ends with a suffix",
            .arguments = {
                {"str", ArgType::STRING, true, nullptr, "String to check"},
                {"suffix", ArgType::STRING, true, nullptr, "Suffix to find"}
            },
            .return_type = ArgType::BOOLEAN,
            .is_deterministic = true,
            .examples = {R"(ENDS_WITH("Hello World", "World") // true)"},
            .cost = FunctionCost{}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::string str = args[0].get<std::string>();
        std::string suffix = args[1].get<std::string>();
        if (suffix.length() > str.length()) {
          return false;
        }
        return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
    }
};

/**
 * @brief REPLACE(str, search, replace) - Replace all occurrences
 */
class ReplaceFunction : public IFunction {
public:
    ~ReplaceFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "REPLACE",
            .category = "String",
            .description = "Replaces all occurrences of a search string",
            .arguments = {
                {"str", ArgType::STRING, true, nullptr, "Source string"},
                {"search", ArgType::STRING, true, nullptr, "String to find"},
                {"replace", ArgType::STRING, true, nullptr, "Replacement string"}
            },
            .return_type = ArgType::STRING,
            .is_deterministic = true,
            .examples = {R"(REPLACE("Hello World", "World", "ThemisDB") // "Hello ThemisDB")"},
            .cost = FunctionCost{}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::string str = args[0].get<std::string>();
        std::string search = args[1].get<std::string>();
        std::string replace = args[2].get<std::string>();
        
        if (search.empty()) {
          return str;
        }
        
        size_t pos = 0;
        while ((pos = str.find(search, pos)) != std::string::npos) {
            str.replace(pos, search.length(), replace);
            pos += replace.length();
        }
        return str;
    }
};

/**
 * @brief REVERSE(str) - Reverse a string
 */
class ReverseStringFunction : public IFunction {
public:
    ~ReverseStringFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "REVERSE",
            .category = "String",
            .description = "Reverses a string",
            .arguments = {
                {"str", ArgType::STRING, true, nullptr, "String to reverse"}
            },
            .return_type = ArgType::STRING,
            .is_deterministic = true,
            .examples = {R"(REVERSE("hello") // "olleh")"},
            .cost = FunctionCost{}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::string str = args[0].get<std::string>();
        std::reverse(str.begin(), str.end());
        return str;
    }
};

/**
 * @brief REGEX_TEST(str, pattern) - Test if string matches regex
 */
class RegexTestFunction : public IFunction {
public:
    ~RegexTestFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "REGEX_TEST",
            .category = "String",
            .description = "Tests if a string matches a regular expression",
            .arguments = {
                {"str", ArgType::STRING, true, nullptr, "String to test"},
                {"pattern", ArgType::STRING, true, nullptr, "Regular expression pattern"}
            },
            .return_type = ArgType::BOOLEAN,
            .is_deterministic = true,
            .examples = {R"(REGEX_TEST("hello123", "\\d+") // true)"},
            .cost = FunctionCost{}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::string str = args[0].get<std::string>();
        std::string pattern = args[1].get<std::string>();
        std::regex re(pattern);
        return std::regex_search(str, re);
    }
};

/**
 * @brief REGEX_REPLACE(str, pattern, replacement) - Replace with regex
 */
class RegexReplaceFunction : public IFunction {
public:
    ~RegexReplaceFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "REGEX_REPLACE",
            .category = "String",
            .description = "Replaces matches of a regular expression",
            .arguments = {
                {"str", ArgType::STRING, true, nullptr, "Source string"},
                {"pattern", ArgType::STRING, true, nullptr, "Regular expression pattern"},
                {"replacement", ArgType::STRING, true, nullptr, "Replacement string"}
            },
            .return_type = ArgType::STRING,
            .is_deterministic = true,
            .examples = {R"(REGEX_REPLACE("hello123world", "\\d+", "-") // "hello-world")"},
            .cost = FunctionCost{}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::string str = args[0].get<std::string>();
        std::string pattern = args[1].get<std::string>();
        std::string replacement = args[2].get<std::string>();
        std::regex re(pattern);
        return std::regex_replace(str, re, replacement);
    }
};

/**
 * @brief LEVENSHTEIN_DISTANCE(str1, str2) - Edit distance between strings
 */
class LevenshteinDistanceFunction : public IFunction {
public:
    ~LevenshteinDistanceFunction() override = default;
    FunctionSignature signature() const override {
        return {
            .name = "LEVENSHTEIN_DISTANCE",
            .category = "String",
            .description = "Calculates the Levenshtein (edit) distance between two strings",
            .arguments = {
                {"str1", ArgType::STRING, true, nullptr, "First string"},
                {"str2", ArgType::STRING, true, nullptr, "Second string"}
            },
            .return_type = ArgType::INTEGER,
            .is_deterministic = true,
            .examples = {R"(LEVENSHTEIN_DISTANCE("hello", "hallo") // 1)"},
            .cost = FunctionCost{}
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                           const FunctionContext&) const override {
        std::string s1 = args[0].get<std::string>();
        std::string s2 = args[1].get<std::string>();
        return static_cast<int64_t>(levenshtein(s1, s2));
    }
    
private:
    static size_t levenshtein(const std::string& s1, const std::string& s2) {
        const size_t m = s1.length();
        const size_t n = s2.length();
        
        if (m == 0) {
          return n;
        }
        if (n == 0) {
          return m;
        }
        
        std::vector<size_t> d(n + 1);
        for (size_t i = 0; i <= n; i++) {
          d[i] = i;
        }
        
        for (size_t i = 1; i <= m; i++) {
            size_t prev = d[0];
            d[0] = i;
            for (size_t j = 1; j <= n; j++) {
                size_t temp = d[j];
                if (s1[i-1] == s2[j-1]) {
                    d[j] = prev;
                } else {
                    d[j] = 1 + std::min({prev, d[j], d[j-1]});
                }
                prev = temp;
            }
        }
        return d[n];
    }
};

// ============================================================================
// Register String Functions
// ============================================================================

inline void registerStringFunctions(FunctionRegistry& reg) {
    reg.registerFunction(std::make_unique<LengthFunction>());
    reg.registerFunction(std::make_unique<ConcatFunction>());
    reg.registerFunction(std::make_unique<SubstringFunction>());
    reg.registerFunction(std::make_unique<UpperFunction>());
    reg.registerFunction(std::make_unique<LowerFunction>());
    reg.registerFunction(std::make_unique<TrimFunction>());
    reg.registerFunction(std::make_unique<LTrimFunction>());
    reg.registerFunction(std::make_unique<RTrimFunction>());
    reg.registerFunction(std::make_unique<SplitFunction>());
    reg.registerFunction(std::make_unique<ContainsFunction>());
    reg.registerFunction(std::make_unique<StartsWithFunction>());
    reg.registerFunction(std::make_unique<EndsWithFunction>());
    reg.registerFunction(std::make_unique<ReplaceFunction>());
    reg.registerFunction(std::make_unique<ReverseStringFunction>());
    reg.registerFunction(std::make_unique<RegexTestFunction>());
    reg.registerFunction(std::make_unique<RegexReplaceFunction>());
    reg.registerFunction(std::make_unique<LevenshteinDistanceFunction>());
}

} // namespace functions
} // namespace query
} // namespace themis
