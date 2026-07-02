#include "security/safe_format.h"
#include <algorithm>
#include <cctype>
#include <spdlog/spdlog.h>

namespace themis::security {

int SafeFormat::print_string(const std::string& text) {
    // Print string without interpreting any format specifiers
    return std::fputs(text.c_str(), stdout);
}

std::string SafeFormat::escape_for_display(const std::string& text) {
    std::string escaped;
    escaped.reserve(text.length());
    
    for (unsigned char c : text) {
        if (c == '\n') {
            escaped += "\\n";
        } else if (c == '\r') {
            escaped += "\\r";
        } else if (c == '\t') {
            escaped += "\\t";
        } else if (c == '\\') {
            escaped += "\\\\";
        } else if (c == '"') {
            escaped += "\\\"";
        } else if (c < 32 || c >= 127) {
            // Escape non-printable characters as \xHH
            char buf[5];
            std::snprintf(buf, sizeof(buf), "\\x%02x", static_cast<unsigned int>(c));
            escaped += buf;
        } else {
            escaped += c;
        }
    }
    
    return escaped;
}

void SafeFormat::log_user_message(const std::string& message, 
                                   const std::string& context) {
    std::string escaped = escape_for_display(message);
    
    try {
        auto logger = spdlog::get("themis");
        if (logger) {
            logger->info("[{}] {}", context, escaped);
        }
    } catch (const std::exception& e) {
        // Fallback: direct stderr output
        std::fprintf(stderr, "[%s] %s\n", context.c_str(), escaped.c_str());
    }
}

}  // namespace themis::security
