/**
 * @file security_functions.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "query/functions/function_registry.h"
#include <nlohmann/json.hpp>
#include <regex>
#include <string>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <functional>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244) // potential narrowing in constants
#pragma warning(disable : 4146) // unary minus on unsigned
#endif

namespace themis {
namespace query {
namespace functions {

// ============================================================================
// VALIDATION FUNCTIONS
// ============================================================================

/**
 * @brief IS_EMAIL(str) - Validate email address format
 * 
 * Uses RFC 5322 simplified regex pattern.
 * 
 * - Example: IS_EMAIL("user@example.com") → true
 * - Example: IS_EMAIL("invalid") → false
 */
class IsEmailFunction : public IFunction {
public:
    ~IsEmailFunction() override = default;
    FunctionSignature signature() const override {
        return FunctionSignature{
            .name = "IS_EMAIL",
            .category = "Security",
            .description = "Validate email address format",
            .arguments = {{"email", ArgType::String, false, nullptr, "Email address to validate"}},
            .return_type = ArgType::Boolean,
            .is_deterministic = true
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                          [[maybe_unused]] const FunctionContext& ctx) const override {
        if (args.empty() || args[0].is_null()) {
          return false;
        }
        if (!args[0].is_string()) {
          return false;
        }
        
        std::string email = args[0].get<std::string>();
        
        // RFC 5322 simplified pattern
        static const std::regex pattern(
            R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})"
        );
        
        return std::regex_match(email, pattern);
    }
};

/**
 * @brief IS_URL(str) - Validate URL format
 * 
 * Supports http, https, ftp protocols.
 * 
 * - Example: IS_URL("https://example.com/path") → true
 * - Example: IS_URL("not-a-url") → false
 */
class IsUrlFunction : public IFunction {
public:
    ~IsUrlFunction() override = default;
    FunctionSignature signature() const override {
        return FunctionSignature{
            .name = "IS_URL",
            .category = "Security",
            .description = "Validate URL format",
            .arguments = {{"url", ArgType::String, false, nullptr, "URL to validate"}},
            .return_type = ArgType::Boolean
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                          [[maybe_unused]] const FunctionContext& ctx) const override {
        if (args.empty() || args[0].is_null()) {
          return false;
        }
        if (!args[0].is_string()) {
          return false;
        }
        
        std::string url = args[0].get<std::string>();
        
        static const std::regex pattern(
            R"(^(https?|ftp)://[^\s/$.?#].[^\s]*$)",
            std::regex::icase
        );
        
        return std::regex_match(url, pattern);
    }
};

/**
 * @brief IS_UUID(str) - Validate UUID format (v1-v5)
 * 
 * - Example: IS_UUID("550e8400-e29b-41d4-a716-446655440000") → true
 * - Example: IS_UUID("not-a-uuid") → false
 */
class IsUuidFunction : public IFunction {
public:
    ~IsUuidFunction() override = default;
    FunctionSignature signature() const override {
        return FunctionSignature{
            .name = "IS_UUID",
            .category = "Security",
            .description = "Validate UUID format (v1-v5)",
            .arguments = {{"uuid", ArgType::String, false, nullptr, "UUID to validate"}},
            .return_type = ArgType::Boolean
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                          [[maybe_unused]] const FunctionContext& ctx) const override {
        if (args.empty() || args[0].is_null()) {
          return false;
        }
        if (!args[0].is_string()) {
          return false;
        }
        
        std::string uuid = args[0].get<std::string>();
        
        static const std::regex pattern(
            R"(^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[1-5][0-9a-fA-F]{3}-[89abAB][0-9a-fA-F]{3}-[0-9a-fA-F]{12}$)"
        );
        
        return std::regex_match(uuid, pattern);
    }
};

/**
 * @brief IS_IP(str, version?) - Validate IP address format
 * 
 * @param str IP address to validate
 * @param version Optional: 4 for IPv4, 6 for IPv6, omit for both
 * 
 * - Example: IS_IP("192.168.1.1") → true
 * - Example: IS_IP("::1", 6) → true
 */
class IsIpFunction : public IFunction {
public:
    ~IsIpFunction() override = default;
    FunctionSignature signature() const override {
        return FunctionSignature{
            .name = "IS_IP",
            .category = "Security",
            .description = "Validate IP address format",
            .arguments = {
                {"ip", ArgType::String, false, nullptr, "IP address to validate"},
                {"version", ArgType::Number, true, nullptr, "4 for IPv4, 6 for IPv6"}
            },
            .return_type = ArgType::Boolean
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                          [[maybe_unused]] const FunctionContext& ctx) const override {
        if (args.empty() || args[0].is_null()) {
          return false;
        }
        if (!args[0].is_string()) {
          return false;
        }
        
        std::string ip = args[0].get<std::string>();
        int version = args.size() > 1 && args[1].is_number() ? args[1].get<int>() : 0;
        
        // IPv4 pattern
        static const std::regex ipv4Pattern(
            R"(^(?:(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.){3}(?:25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$)"
        );
        
        // Simplified IPv6 pattern
        static const std::regex ipv6Pattern(
            R"(^(?:[0-9a-fA-F]{1,4}:){7}[0-9a-fA-F]{1,4}$|^::$|^::1$|^(?:[0-9a-fA-F]{1,4}:)*:(?:[0-9a-fA-F]{1,4}:)*[0-9a-fA-F]{1,4}$)"
        );
        
        if (version == 4) {
            return std::regex_match(ip, ipv4Pattern);
        } else if (version == 6) {
            return std::regex_match(ip, ipv6Pattern);
        } else {
            return std::regex_match(ip, ipv4Pattern) || std::regex_match(ip, ipv6Pattern);
        }
    }
};

/**
 * @brief IS_PHONE(str, countryCode?) - Validate phone number format
 * 
 * - Example: IS_PHONE("+49 123 456789") → true
 * - Example: IS_PHONE("0123456789", "DE") → true
 */
class IsPhoneFunction : public IFunction {
public:
    ~IsPhoneFunction() override = default;
    FunctionSignature signature() const override {
        return FunctionSignature{
            .name = "IS_PHONE",
            .category = "Security",
            .description = "Validate phone number format",
            .arguments = {
                {"phone", ArgType::String, false, nullptr, "Phone number to validate"},
                {"countryCode", ArgType::String, true, nullptr, "Country code (e.g., DE, US)"}
            },
            .return_type = ArgType::Boolean
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                          [[maybe_unused]] const FunctionContext& ctx) const override {
        if (args.empty() || args[0].is_null()) {
          return false;
        }
        if (!args[0].is_string()) {
          return false;
        }
        
        std::string phone = args[0].get<std::string>();
        
        // Remove common separators for validation
        std::string cleaned;
        for (char c : phone) {
            if (std::isdigit(c) || c == '+') {
                cleaned += c;
            }
        }
        
        // International format: +[country code][number]
        static const std::regex internationalPattern(R"(^\+[1-9]\d{6,14}$)");
        
        // Local format: starts with 0, at least 7 digits
        static const std::regex localPattern(R"(^0\d{6,14}$)");
        
        return std::regex_match(cleaned, internationalPattern) || 
               std::regex_match(cleaned, localPattern);
    }
};

/**
 * @brief IS_IBAN(str) - Validate IBAN format with checksum
 * 
 * - Example: IS_IBAN("DE89370400440532013000") → true
 */
class IsIbanFunction : public IFunction {
public:
    ~IsIbanFunction() override = default;
    FunctionSignature signature() const override {
        return FunctionSignature{
            .name = "IS_IBAN",
            .category = "Security",
            .description = "Validate IBAN format with checksum",
            .arguments = {{"iban", ArgType::String, false, nullptr, "IBAN to validate"}},
            .return_type = ArgType::Boolean
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                          [[maybe_unused]] const FunctionContext& ctx) const override {
        if (args.empty() || args[0].is_null()) {
          return false;
        }
        if (!args[0].is_string()) {
          return false;
        }
        
        std::string iban = args[0].get<std::string>();
        
        // Remove spaces
        iban.erase(std::remove(iban.begin(), iban.end(), ' '), iban.end());
        
        // Convert to uppercase
        std::transform(iban.begin(), iban.end(), iban.begin(), ::toupper);
        
        // Basic format check: 2 letters, 2 digits, up to 30 alphanumeric
        static const std::regex pattern(R"(^[A-Z]{2}[0-9]{2}[A-Z0-9]{1,30}$)");
        if (!std::regex_match(iban, pattern)) {
            return false;
        }
        
        // Move first 4 chars to end
        std::string rearranged = iban.substr(4) + iban.substr(0, 4);
        
        // Convert letters to numbers (A=10, B=11, ..., Z=35)
        std::string numericStr;
        for (char c : rearranged) {
            if (std::isalpha(c)) {
                numericStr += std::to_string(c - 'A' + 10);
            } else {
                numericStr += c;
            }
        }
        
        // Mod 97 calculation (using string arithmetic for large numbers)
        int remainder = 0;
        for (char c : numericStr) {
            remainder = (remainder * 10 + (c - '0')) % 97;
        }
        
        return remainder == 1;
    }
};

/**
 * @brief IS_CREDIT_CARD(str) - Validate credit card number (Luhn algorithm)
 * 
 * - Example: IS_CREDIT_CARD("4532015112830366") → true
 */
class IsCreditCardFunction : public IFunction {
public:
    ~IsCreditCardFunction() override = default;
    FunctionSignature signature() const override {
        return FunctionSignature{
            .name = "IS_CREDIT_CARD",
            .category = "Security",
            .description = "Validate credit card number (Luhn algorithm)",
            .arguments = {{"card", ArgType::String, false, nullptr, "Credit card number to validate"}},
            .return_type = ArgType::Boolean
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                          [[maybe_unused]] const FunctionContext& ctx) const override {
        if (args.empty() || args[0].is_null()) {
          return false;
        }
        if (!args[0].is_string()) {
          return false;
        }
        
        std::string card = args[0].get<std::string>();
        
        // Remove spaces and dashes
        std::string cleaned;
        for (char c : card) {
            if (std::isdigit(c)) {
                cleaned += c;
            }
        }
        
        // Must be 13-19 digits
        if (cleaned.length() < 13 || cleaned.length() > 19) {
            return false;
        }
        
        // Luhn algorithm
        int sum = 0;
        bool alternate = false;
        for (int i = static_cast<int>(cleaned.length()) - 1; i >= 0; --i) {
            int digit = cleaned[i] - '0';
            if (alternate) {
                digit *= 2;
                if (digit > 9) {
                    digit -= 9;
                }
            }
            sum += digit;
            alternate = !alternate;
        }
        
        return (sum % 10) == 0;
    }
};

// ============================================================================
// SANITIZATION FUNCTIONS
// ============================================================================

/**
 * @brief SANITIZE(str, type?) - Sanitize input string
 * 
 * @param str String to sanitize
 * @param type Type of sanitization: "html", "sql", "json", "filename" (default: "html")
 * 
 * - Example: SANITIZE("<script>alert('xss')</script>", "html") → "<script>..."
 */
class SanitizeFunction : public IFunction {
public:
    ~SanitizeFunction() override = default;
    FunctionSignature signature() const override {
        return FunctionSignature{
            .name = "SANITIZE",
            .category = "Security",
            .description = "Sanitize input string",
            .arguments = {
                {"str", ArgType::String, false, nullptr, "String to sanitize"},
                {"type", ArgType::String, true, nullptr, "Type: html, sql, json, filename"}
            },
            .return_type = ArgType::String
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                          [[maybe_unused]] const FunctionContext& ctx) const override {
        if (args.empty() || args[0].is_null()) {
          return "";
        }
        if (!args[0].is_string()) {
          return args[0].dump();
        }
        
        std::string str = args[0].get<std::string>();
        std::string type = args.size() > 1 && args[1].is_string() ? args[1].get<std::string>() : "html";
        
        if (type == "html") {
            return escapeHtml(str);
        } else if (type == "sql") {
            return escapeSql(str);
        } else if (type == "json") {
            return escapeJson(str);
        } else if (type == "filename") {
            return sanitizeFilename(str);
        }
        
        return escapeHtml(str);
    }

private:
    static std::string escapeHtml(const std::string& str) {
        std::string result;
        result.reserve(str.size() * 1.2);
        for (char c : str) {
            switch (c) {
                case '<': result += "<"; break;
                case '>': result += ">"; break;
                case '&': result += "&amp;"; break;
                case '"': result += "&quot;"; break;
                case '\'': result += "&#39;"; break;
                default: result += c;
            }
        }
        return result;
    }
    
    static std::string escapeSql(const std::string& str) {
        std::string result;
        result.reserve(str.size() * 1.2);
        for (char c : str) {
            if (c == '\'') {
                result += "''";
            } else if (c == '\\') {
                result += "\\\\";
            } else {
                result += c;
            }
        }
        return result;
    }
    
    static std::string escapeJson(const std::string& str) {
        std::string result;
        result.reserve(str.size() * 1.2);
        for (char c : str) {
            switch (c) {
                case '"': result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\b': result += "\\b"; break;
                case '\f': result += "\\f"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default: result += c;
            }
        }
        return result;
    }
    
    static std::string sanitizeFilename(const std::string& str) {
        std::string result;
        result.reserve(str.size());
        for (char c : str) {
            // Allow alphanumeric, dot, dash, underscore
            if (std::isalnum(c) || c == '.' || c == '-' || c == '_') {
                result += c;
            } else if (c == ' ') {
                result += '_';
            }
            // Skip other characters
        }
        return result;
    }
};

/**
 * @brief HAS_INJECTION(str, type?) - Check for potential injection patterns
 * 
 * @param str String to check
 * @param type Type: "sql", "xss", "path", "cmd" (default: all)
 * 
 * - Example: HAS_INJECTION("1'; DROP TABLE users--", "sql") → true
 */
class HasInjectionFunction : public IFunction {
public:
    ~HasInjectionFunction() override = default;
    FunctionSignature signature() const override {
        return FunctionSignature{
            .name = "HAS_INJECTION",
            .category = "Security",
            .description = "Check for potential injection patterns",
            .arguments = {
                {"str", ArgType::String, false, nullptr, "String to check"},
                {"type", ArgType::String, true, nullptr, "Type: sql, xss, path, cmd"}
            },
            .return_type = ArgType::Boolean
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                          [[maybe_unused]] const FunctionContext& ctx) const override {
        if (args.empty() || args[0].is_null()) {
          return false;
        }
        if (!args[0].is_string()) {
          return false;
        }
        
        std::string str = args[0].get<std::string>();
        std::string type = args.size() > 1 && args[1].is_string() ? args[1].get<std::string>() : "all";
        
        // Convert to lowercase for case-insensitive matching
        std::string lower = str;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        
        if (type == "sql" || type == "all") {
            // SQL injection patterns
            static const std::vector<std::string> sqlPatterns = {
                "' or ", "' and ", "1=1", "1'='1", "drop table", "delete from",
                "insert into", "update set", "union select", "--", "/*", "*/"
            };
            for (const auto& pattern : sqlPatterns) {
                if (lower.find(pattern) != std::string::npos) {
                    return true;
                }
            }
        }
        
        if (type == "xss" || type == "all") {
            // XSS patterns
            static const std::vector<std::string> xssPatterns = {
                "<script", "javascript:", "onerror=", "onload=", "onclick=",
                "onmouseover=", "onfocus=", "onblur=", "eval(", "expression("
            };
            for (const auto& pattern : xssPatterns) {
                if (lower.find(pattern) != std::string::npos) {
                    return true;
                }
            }
        }
        
        if (type == "path" || type == "all") {
            // Path traversal patterns
            static const std::vector<std::string> pathPatterns = {
                "../", "..\\", "%2e%2e", "%252e"
            };
            for (const auto& pattern : pathPatterns) {
                if (lower.find(pattern) != std::string::npos) {
                    return true;
                }
            }
        }
        
        if (type == "cmd" || type == "all") {
            // Command injection patterns
            static const std::vector<std::string> cmdPatterns = {
                "; ", "| ", "` ", "$(" , "&&", "||"
            };
            for (const auto& pattern : cmdPatterns) {
                if (str.find(pattern) != std::string::npos) {
                    return true;
                }
            }
        }
        
        return false;
    }
};

// ============================================================================
// MASKING FUNCTIONS
// ============================================================================

/**
 * @brief MASK(str, start?, end?, char?) - Mask characters in string
 * 
 * @param str String to mask
 * @param start Characters to show at start (default: 0)
 * @param end Characters to show at end (default: 0)
 * @param char Masking character (default: '*')
 * 
 * - Example: MASK("1234567890", 0, 4) → "******7890"
 * - Example: MASK("secret", 1, 1, '#') → "s####t"
 */
class MaskFunction : public IFunction {
public:
    ~MaskFunction() override = default;
    FunctionSignature signature() const override {
        return FunctionSignature{
            .name = "MASK",
            .category = "Security",
            .description = "Mask characters in string",
            .arguments = {
                {"str", ArgType::String, false, nullptr, "String to mask"},
                {"start", ArgType::Number, true, nullptr, "Characters to show at start"},
                {"end", ArgType::Number, true, nullptr, "Characters to show at end"},
                {"char", ArgType::String, true, nullptr, "Masking character"}
            },
            .return_type = ArgType::String
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                          [[maybe_unused]] const FunctionContext& ctx) const override {
        if (args.empty() || args[0].is_null()) {
          return "";
        }
        if (!args[0].is_string()) {
          return args[0].dump();
        }
        
        std::string str = args[0].get<std::string>();
        int start = args.size() > 1 && args[1].is_number() ? args[1].get<int>() : 0;
        int end = args.size() > 2 && args[2].is_number() ? args[2].get<int>() : 0;
        char maskChar = args.size() > 3 && args[3].is_string() && !args[3].get<std::string>().empty() 
                        ? args[3].get<std::string>()[0] : '*';
        
        int len = static_cast<int>(str.length());
        if (start + end >= len) {
            return str; // Nothing to mask
        }
        
        std::string result;
        result.reserve(len);
        for (int i = 0; i < len; ++i) {
            if (i < start || i >= len - end) {
                result += str[i];
            } else {
                result += maskChar;
            }
        }
        
        return result;
    }
};

/**
 * @brief MASK_EMAIL(email) - Mask email address
 * 
 * - Example: MASK_EMAIL("john.doe@example.com") → "j******e@e*****e.com"
 */
class MaskEmailFunction : public IFunction {
public:
    ~MaskEmailFunction() override = default;
    FunctionSignature signature() const override {
        return FunctionSignature{
            .name = "MASK_EMAIL",
            .category = "Security",
            .description = "Mask email address",
            .arguments = {{"email", ArgType::String, false, nullptr, "Email to mask"}},
            .return_type = ArgType::String
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                          [[maybe_unused]] const FunctionContext& ctx) const override {
        if (args.empty() || args[0].is_null()) {
          return "";
        }
        if (!args[0].is_string()) {
          return "";
        }
        
        std::string email = args[0].get<std::string>();
        size_t atPos = email.find('@');
        if (atPos == std::string::npos) {
            return email; // Not a valid email
        }
        
        // Mask local part (keep first and last char)
        std::string local = email.substr(0, atPos);
        std::string domain = email.substr(atPos + 1);
        
        std::string maskedLocal;
        if (local.length() <= 2) {
            maskedLocal = local;
        } else {
            maskedLocal = local[0] + std::string(local.length() - 2, '*') + local.back();
        }
        
        // Mask domain (keep first and last char of each part)
        size_t dotPos = domain.find('.');
        if (dotPos != std::string::npos) {
            std::string domainName = domain.substr(0, dotPos);
            std::string tld = domain.substr(dotPos);
            
            std::string maskedDomain;
            if (domainName.length() <= 2) {
                maskedDomain = domainName;
            } else {
                maskedDomain = domainName[0] + std::string(domainName.length() - 2, '*') + domainName.back();
            }
            
            return maskedLocal + "@" + maskedDomain + tld;
        }
        
        return maskedLocal + "@" + domain;
    }
};

/**
 * @brief MASK_CREDIT_CARD(card) - Mask credit card number (show last 4 digits)
 * 
 * - Example: MASK_CREDIT_CARD("4532015112830366") → "************0366"
 */
class MaskCreditCardFunction : public IFunction {
public:
    ~MaskCreditCardFunction() override = default;
    FunctionSignature signature() const override {
        return FunctionSignature{
            .name = "MASK_CREDIT_CARD",
            .category = "Security",
            .description = "Mask credit card number",
            .arguments = {{"card", ArgType::String, false, nullptr, "Credit card to mask"}},
            .return_type = ArgType::String
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                          [[maybe_unused]] const FunctionContext& ctx) const override {
        if (args.empty() || args[0].is_null()) {
          return "";
        }
        if (!args[0].is_string()) {
          return "";
        }
        
        std::string card = args[0].get<std::string>();
        
        // Remove non-digits
        std::string cleaned;
        for (char c : card) {
            if (std::isdigit(c)) {
                cleaned += c;
            }
        }
        
        if (cleaned.length() <= 4) {
            return cleaned;
        }
        
        return std::string(cleaned.length() - 4, '*') + cleaned.substr(cleaned.length() - 4);
    }
};

/**
 * @brief MASK_IBAN(iban) - Mask IBAN (show country code and last 4 chars)
 * 
 * - Example: MASK_IBAN("DE89370400440532013000") → "DE**************3000"
 */
class MaskIbanFunction : public IFunction {
public:
    ~MaskIbanFunction() override = default;
    FunctionSignature signature() const override {
        return FunctionSignature{
            .name = "MASK_IBAN",
            .category = "Security",
            .description = "Mask IBAN",
            .arguments = {{"iban", ArgType::String, false, nullptr, "IBAN to mask"}},
            .return_type = ArgType::String
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                          [[maybe_unused]] const FunctionContext& ctx) const override {
        if (args.empty() || args[0].is_null()) {
          return "";
        }
        if (!args[0].is_string()) {
          return "";
        }
        
        std::string iban = args[0].get<std::string>();
        
        // Remove spaces
        iban.erase(std::remove(iban.begin(), iban.end(), ' '), iban.end());
        
        if (iban.length() <= 6) {
            return iban;
        }
        
        // Keep first 2 chars (country) and last 4
        return iban.substr(0, 2) + std::string(iban.length() - 6, '*') + iban.substr(iban.length() - 4);
    }
};

// ============================================================================
// HASHING FUNCTIONS
// ============================================================================

/**
 * @brief HASH(str, algorithm?) - Compute hash of string
 * 
 * Uses FNV-1a for speed. For cryptographic hashing, use SHA256.
 * Note: For production use, integrate with ThemisDB's security module.
 * 
 * @param str String to hash
 * @param algorithm "fnv1a" (default), "djb2"
 * 
 * - Example: HASH("password") → "af63bd4c..."
 */
class HashFunction : public IFunction {
public:
    ~HashFunction() override = default;
    FunctionSignature signature() const override {
        return FunctionSignature{
            .name = "HASH",
            .category = "Security",
            .description = "Compute hash of string (non-cryptographic)",
            .arguments = {
                {"str", ArgType::String, false, nullptr, "String to hash"},
                {"algorithm", ArgType::String, true, nullptr, "Algorithm: fnv1a, djb2"}
            },
            .return_type = ArgType::String
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                          [[maybe_unused]] const FunctionContext& ctx) const override {
        if (args.empty() || args[0].is_null()) {
          return "";
        }
        if (!args[0].is_string()) {
          return "";
        }
        
        std::string str = args[0].get<std::string>();
        std::string algorithm = args.size() > 1 && args[1].is_string() ? args[1].get<std::string>() : "fnv1a";
        
        uint64_t hash;
        if (algorithm == "djb2") {
            hash = djb2Hash(str);
        } else {
            hash = fnv1aHash(str);
        }
        
        std::ostringstream oss;
        oss << std::hex << std::setfill('0') << std::setw(16) << hash;
        return oss.str();
    }

private:
    static uint64_t fnv1aHash(const std::string& str) {
        uint64_t hash = 0xcbf29ce484222325ULL; // FNV-1a offset basis
        for (char c : str) {
            hash ^= static_cast<uint64_t>(static_cast<unsigned char>(c));
            hash *= 0x100000001b3ULL; // FNV-1a prime
        }
        return hash;
    }
    
    static uint64_t djb2Hash(const std::string& str) {
        uint64_t hash = 5381;
        for (char c : str) {
            hash = ((hash << 5) + hash) + static_cast<unsigned char>(c);
        }
        return hash;
    }
};

/**
 * @brief CHECKSUM(data, algorithm?) - Compute checksum
 * 
 * @param data String or array to compute checksum for
 * @param algorithm "crc32" (default), "adler32", "fletcher16"
 */
class ChecksumFunction : public IFunction {
public:
    ~ChecksumFunction() override = default;
    FunctionSignature signature() const override {
        return FunctionSignature{
            .name = "CHECKSUM",
            .category = "Security",
            .description = "Compute checksum of data",
            .arguments = {
                {"data", ArgType::Any, false, nullptr, "Data to checksum"},
                {"algorithm", ArgType::String, true, nullptr, "Algorithm: crc32, adler32"}
            },
            .return_type = ArgType::Number
        };
    }
    
    nlohmann::json execute(const std::vector<nlohmann::json>& args,
                          [[maybe_unused]] const FunctionContext& ctx) const override {
        if (args.empty() || args[0].is_null()) {
          return 0;
        }
        
        std::string data;
        if (args[0].is_string()) {
            data = args[0].get<std::string>();
        } else {
            data = args[0].dump();
        }
        
        std::string algorithm = args.size() > 1 && args[1].is_string() ? args[1].get<std::string>() : "crc32";
        
        if (algorithm == "adler32") {
            return adler32(data);
        } else {
            return crc32(data);
        }
    }

private:
    static uint32_t crc32(const std::string& data) {
        uint32_t crc = 0xFFFFFFFF;
        for (unsigned char c : data) {
            crc ^= c;
            for (int i = 0; i < 8; ++i) {
                crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
            }
        }
        return ~crc;
    }
    
    static uint32_t adler32(const std::string& data) {
        uint32_t a = 1, b = 0;
        for (unsigned char c : data) {
            a = (a + c) % 65521;
            b = (b + a) % 65521;
        }
        return (b << 16) | a;
    }
};

// ============================================================================
// REGISTRATION
// ============================================================================

/**
 * @brief Register all security functions
 */
inline void registerSecurityFunctions() {
    auto& registry = FunctionRegistry::instance();
    
    // Validation
    registry.registerFunction(std::make_unique<IsEmailFunction>());
    registry.registerFunction(std::make_unique<IsUrlFunction>());
    registry.registerFunction(std::make_unique<IsUuidFunction>());
    registry.registerFunction(std::make_unique<IsIpFunction>());
    registry.registerFunction(std::make_unique<IsPhoneFunction>());
    registry.registerFunction(std::make_unique<IsIbanFunction>());
    registry.registerFunction(std::make_unique<IsCreditCardFunction>());
    
    // Sanitization
    registry.registerFunction(std::make_unique<SanitizeFunction>());
    registry.registerFunction(std::make_unique<HasInjectionFunction>());
    
    // Masking
    registry.registerFunction(std::make_unique<MaskFunction>());
    registry.registerFunction(std::make_unique<MaskEmailFunction>());
    registry.registerFunction(std::make_unique<MaskCreditCardFunction>());
    registry.registerFunction(std::make_unique<MaskIbanFunction>());
    
    // Hashing
    registry.registerFunction(std::make_unique<HashFunction>());
    registry.registerFunction(std::make_unique<ChecksumFunction>());
}

} // namespace functions
} // namespace query
} // namespace themis

#ifdef _MSC_VER
#pragma warning(pop)
#endif

