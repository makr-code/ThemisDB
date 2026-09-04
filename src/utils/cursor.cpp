/**
 * @file cursor.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=7, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "utils/cursor.h"
#include <sstream>
#include <iomanip>
#include <cstring>
#include <chrono>

namespace themis {
namespace utils {

// Simple Base64 encoding table
// Note: Custom implementation for lightweight encoding without external dependencies.
// For production systems requiring cryptographic operations, consider using
// established libraries (boost::beast::detail::base64, OpenSSL BIO).
static const char base64_chars[] = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

std::string Cursor::base64Encode(const std::string& input) {
    std::string output;
    int val = 0;
    int valb = -6;
    
    for (unsigned char c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            output.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    
    if (valb > -6) {
        output.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    
    while (output.size() % 4) {
        output.push_back('=');
    }
    
    return output;
}

std::optional<std::string> Cursor::base64Decode(const std::string& input) {
    if (input.empty()) {
        return std::nullopt;
    }
    
    std::string output;
    std::vector<int> T(256, -1);
    
    for (int i = 0; i < 64; i++) {
        T[base64_chars[i]] = i;
    }
    
    int val = 0;
    int valb = -8;
    
    for (unsigned char c : input) {
        if (T[c] == -1) {
          break;
        }
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            output.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    
    return output;
}

int64_t Cursor::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
}

std::string Cursor::encode(
    const std::string& last_pk, 
    const std::string& collection,
    const std::optional<std::string>& order_value
) {
    nlohmann::json cursor_data = {
        {"pk", last_pk},
        {"collection", collection},
        {"version", 1},
        {"created_at", getCurrentTimestamp()}
    };
    
    if (order_value.has_value()) {
        cursor_data["order_value"] = *order_value;
    }
    
    std::string json_str = cursor_data.dump();
    return base64Encode(json_str);
}

std::optional<CursorInfo> Cursor::decodeDetailed(const std::string& cursor_token) {
    auto decoded = base64Decode(cursor_token);
    if (!decoded.has_value()) {
        return std::nullopt;
    }
    
    try {
        auto cursor_data = nlohmann::json::parse(*decoded);
        
        // Validate cursor structure
        if (!cursor_data.contains("pk") || !cursor_data.contains("collection")) {
            return std::nullopt;
        }
        
        CursorInfo info;
        info.pk = cursor_data["pk"].get<std::string>();
        info.collection = cursor_data["collection"].get<std::string>();
        
        // Optional fields
        if (cursor_data.contains("order_value") && !cursor_data["order_value"].is_null()) {
            info.order_value = cursor_data["order_value"].get<std::string>();
        }
        
        if (cursor_data.contains("created_at")) {
            info.created_at = cursor_data["created_at"].get<int64_t>();
        }
        
        if (cursor_data.contains("version")) {
            info.version = cursor_data["version"].get<int>();
        }
        
        return info;
    } catch (const nlohmann::json::exception&) {
        return std::nullopt;
    }
}

std::optional<std::pair<std::string, std::string>> Cursor::decode(const std::string& cursor_token) {
    auto info = decodeDetailed(cursor_token);
    if (!info.has_value()) {
        return std::nullopt;
    }
    
    return std::make_pair(info->pk, info->collection);
}

bool Cursor::isValid(const std::string& cursor_token, int64_t ttl_seconds) {
    auto info = decodeDetailed(cursor_token);
    if (!info.has_value()) {
        return false;
    }
    
    // If no TTL specified, just check if cursor decodes
    if (ttl_seconds <= 0) {
        return true;
    }
    
    // Check expiration
    int64_t now = getCurrentTimestamp();
    int64_t age = now - info->created_at;
    
    return age <= ttl_seconds;
}

size_t Cursor::normalizePageSize(size_t requested_size, const PaginationConfig& config) {
    if (requested_size < config.min_page_size) {
        return config.min_page_size;
    }
    if (requested_size > config.max_page_size) {
        return config.max_page_size;
    }
    return requested_size;
}

} // namespace utils
} // namespace themis
