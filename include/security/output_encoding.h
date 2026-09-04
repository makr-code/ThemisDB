/**
 * @file output_encoding.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

namespace themis {
namespace security {

/**
 * @brief Security utilities for output encoding and XSS prevention
 * 
 * Provides functions to safely encode user-generated content for
 * various contexts (HTML, JavaScript, URL, etc.)
 */
class OutputEncoder {
public:
    /**
     * @brief Encode string for safe HTML output
     * 
     * Escapes HTML special characters: & < > " ' /
     * 
     * @param input Raw string that may contain HTML
     * @return HTML-safe encoded string
     */
    static std::string encodeHTML(std::string_view input) {
        std::string output = {};
        output.reserve(input.size() + input.size() / 5);  // Reserve ~20% extra space for encoding
        
        for (char c : input) {
            switch (c) {
                case '&':  output += "&amp;"; break;
                case '<':  output += "&lt;"; break;
                case '>':  output += "&gt;"; break;
                case '"':  output += "&quot;"; break;
                case '\'': output += "&#x27;"; break;
                case '/':  output += "&#x2F;"; break;
                default:   output += c; break;
            }
        }
        
        return output;
    }
    
    /**
     * @brief Encode string for safe JavaScript string literal
     * 
     * Escapes characters that could break out of a JS string.
     * 
     * @param input Raw string
     * @return JavaScript-safe encoded string
     */
    static std::string encodeJavaScript(std::string_view input) {
        std::string output = {};
        output.reserve(input.size() + input.size() / 5);
        
        for (unsigned char c : input) {
            switch (c) {
                case '"':  output += "\\\""; break;
                case '\'': output += "\\'"; break;
                case '\\': output += "\\\\"; break;
                case '\n': output += "\\n"; break;
                case '\r': output += "\\r"; break;
                case '\t': output += "\\t"; break;
                case '\b': output += "\\b"; break;
                case '\f': output += "\\f"; break;
                case '<':  output += "\\x3C"; break;  // Prevent </script>
                case '>':  output += "\\x3E"; break;
                default:
                    // Escape control characters
                    if (c < 0x20) {
                        char buf[8];
                        snprintf(buf, sizeof(buf), "\\x%02X", c);
                        output += buf;
                    } else {
                        output += c;
                    }
                    break;
            }
        }
        
        return output;
    }
    
    /**
     * @brief Encode string for safe URL query parameter
     * 
     * Percent-encodes special characters.
     * 
     * @param input Raw string
     * @return URL-encoded string
     */
    static std::string encodeURL(std::string_view input) {
        std::string output = {};
        output.reserve(input.size() + input.size() / 5);
        
        const char* hex = "0123456789ABCDEF";
        
        for (unsigned char c : input) {
            if (isAlphaNumeric(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                output += c;
            } else {
                output += '%';
                output += hex[c >> 4];
                output += hex[c & 0x0F];
            }
        }
        
        return output;
    }
    
    /**
     * @brief Encode string for safe JSON output
     * 
     * Escapes JSON special characters and control characters.
     * 
     * @param input Raw string
     * @return JSON-safe encoded string
     */
    static std::string encodeJSON(std::string_view input) {
        std::string output = {};
        output.reserve(input.size() + input.size() / 5);
        
        for (unsigned char c : input) {
            switch (c) {
                case '"':  output += "\\\""; break;
                case '\\': output += "\\\\"; break;
                case '/':  output += "\\/"; break;
                case '\b': output += "\\b"; break;
                case '\f': output += "\\f"; break;
                case '\n': output += "\\n"; break;
                case '\r': output += "\\r"; break;
                case '\t': output += "\\t"; break;
                default:
                    if (c < 0x20) {
                        char buf[8];
                        snprintf(buf, sizeof(buf), "\\u%04X", c);
                        output += buf;
                    } else {
                        output += c;
                    }
                    break;
            }
        }
        
        return output;
    }
    
    /**
     * @brief Sanitize string for safe attribute value
     * 
     * Removes or escapes characters dangerous in HTML attributes.
     * 
     * @param input Raw string
     * @return Sanitized string safe for HTML attributes
     */
    static std::string sanitizeAttribute(std::string_view input) {
        std::string output = {};
        output.reserve(input.size());
        
        for (char c : input) {
            // Remove potentially dangerous characters
            if (c == '"' || c == '\'' || c == '<' || c == '>' || c == '`') {
                continue;  // Skip these characters
            }
            output += c;
        }
        
        return output;
    }
    
private:
    static bool isAlphaNumeric(unsigned char c) {
        return (c >= 'A' && c <= 'Z') ||
               (c >= 'a' && c <= 'z') ||
               (c >= '0' && c <= '9');
    }
};

/**
 * @brief Content Security Policy (CSP) header builder
 * 
 * Helps construct CSP headers for XSS protection.
 */
class CSPBuilder {
public:
    CSPBuilder() = default;
    
    /**
     * @brief Set default-src directive
     */
    CSPBuilder& defaultSrc(const std::string& value) {
        directives_["default-src"] = value;
        return *this;
    }
    
    /**
     * @brief Set script-src directive
     */
    CSPBuilder& scriptSrc(const std::string& value) {
        directives_["script-src"] = value;
        return *this;
    }
    
    /**
     * @brief Set style-src directive
     */
    CSPBuilder& styleSrc(const std::string& value) {
        directives_["style-src"] = value;
        return *this;
    }
    
    /**
     * @brief Set img-src directive
     */
    CSPBuilder& imgSrc(const std::string& value) {
        directives_["img-src"] = value;
        return *this;
    }
    
    /**
     * @brief Set connect-src directive
     */
    CSPBuilder& connectSrc(const std::string& value) {
        directives_["connect-src"] = value;
        return *this;
    }
    
    /**
     * @brief Build the CSP header value
     */
    std::string build() const {
        std::string csp = {};
        bool first = true;
        
        for (const auto& [directive, value] : directives_) {
            if (!first) {
              csp += "; ";
            }
            csp += directive + " " + value;
            first = false;
        }
        
        return csp;
    }
    
    /**
     * @brief Create a strict CSP for API endpoints
     */
    static std::string strictAPI() {
        return CSPBuilder()
            .defaultSrc("'none'")
            .build();
    }
    
    /**
     * @brief Create a standard CSP for web applications
     */
    static std::string standard() {
        return CSPBuilder()
            .defaultSrc("'self'")
            .scriptSrc("'self'")
            .styleSrc("'self'")
            .imgSrc("'self' data:")
            .connectSrc("'self'")
            .build();
    }
    
private:
    std::unordered_map<std::string, std::string> directives_;
};

/**
 * @brief Security headers helper
 * 
 * Provides recommended security headers for HTTP responses.
 */
class SecurityHeaders {
public:
    /**
     * @brief Get standard security headers for API responses
     */
    static std::unordered_map<std::string, std::string> apiHeaders() {
        return {
            {"X-Content-Type-Options", "nosniff"},
            {"X-Frame-Options", "DENY"},
            {"X-XSS-Protection", "1; mode=block"},
            {"Content-Security-Policy", CSPBuilder::strictAPI()},
            {"Strict-Transport-Security", "max-age=31536000; includeSubDomains"},
            {"Referrer-Policy", "no-referrer"}
        };
    }
    
    /**
     * @brief Get standard security headers for web responses
     */
    static std::unordered_map<std::string, std::string> webHeaders() {
        return {
            {"X-Content-Type-Options", "nosniff"},
            {"X-Frame-Options", "SAMEORIGIN"},
            {"X-XSS-Protection", "1; mode=block"},
            {"Content-Security-Policy", CSPBuilder::standard()},
            {"Strict-Transport-Security", "max-age=31536000; includeSubDomains"},
            {"Referrer-Policy", "strict-origin-when-cross-origin"}
        };
    }
};

} // namespace security
} // namespace themis

