/**
 * @file cursor.h
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
#include <vector>
#include <optional>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace themis {
namespace utils {

/**
 * Configuration for pagination behavior
 */
struct PaginationConfig {
    size_t default_page_size = 100;     // Default items per page
    size_t min_page_size = 1;           // Minimum allowed page size
    size_t max_page_size = 10000;       // Maximum allowed page size
    int64_t cursor_ttl_seconds = 3600;  // Cursor validity duration (1 hour)
    bool enable_total_count = false;     // Whether to compute total count (expensive, future feature)
};

/**
 * Decoded cursor information
 */
struct CursorInfo {
    std::string pk;                      // Last primary key
    std::string collection;              // Collection name
    std::optional<std::string> order_value; // Last ORDER BY value (for keyset pagination)
    int64_t created_at = 0;              // Unix timestamp when cursor was created
    int version = 1;                     // Cursor format version
};

/**
 * Cursor encoding/decoding utilities for pagination.
 * 
 * Cursors encode the last seen primary key or index position to enable
 * stateless pagination. Supports multiple pagination strategies:
 * - Basic cursor pagination (pk-based)
 * - Keyset pagination (ORDER BY value + pk)
 * - Cursor expiration and versioning
 * 
 * Format: base64(json({pk, collection, order_value?, created_at, version}))
 * 
 * Security Note: Cursor tokens use Base64 encoding (not encryption).
 * This is appropriate for position markers but should not contain sensitive data.
 */
class Cursor {
public:
    /**
     * Encode a cursor from the last primary key and collection name.
     * 
     * @param last_pk The primary key of the last item in the current page
     * @param collection The collection name
     * @param order_value Optional ORDER BY value for keyset pagination
     * @return Base64-encoded cursor token
     */
    static std::string encode(
        const std::string& last_pk, 
        const std::string& collection,
        const std::optional<std::string>& order_value = std::nullopt
    );
    
    /**
     * Decode a cursor token to extract cursor information.
     * 
     * @param cursor_token Base64-encoded cursor string
     * @return Optional CursorInfo, nullopt if invalid
     */
    static std::optional<CursorInfo> decodeDetailed(const std::string& cursor_token);
    
    /**
     * Decode a cursor token to extract the primary key and collection (legacy).
     * 
     * @param cursor_token Base64-encoded cursor string
     * @return Optional pair of (pk, collection), nullopt if invalid/expired
     */
    static std::optional<std::pair<std::string, std::string>> decode(const std::string& cursor_token);
    
    /**
     * Validate if a cursor is still valid (not expired).
     * 
     * @param cursor_token Base64-encoded cursor string
     * @param ttl_seconds Time-to-live in seconds (0 = no expiration check)
     * @return true if cursor is valid and not expired
     */
    static bool isValid(const std::string& cursor_token, int64_t ttl_seconds = 0);
    
    /**
     * Normalize page size to be within configured limits.
     * 
     * @param requested_size Requested page size
     * @param config Pagination configuration
     * @return Normalized page size within [min, max]
     */
    static size_t normalizePageSize(size_t requested_size, const PaginationConfig& config);
    
    // Base64 encode/decode helpers
    static std::string base64Encode(const std::string& input);
    static std::optional<std::string> base64Decode(const std::string& input);

private:
    
    // Get current Unix timestamp
    static int64_t getCurrentTimestamp();
};

/**
 * Pagination method types
 */
enum class PaginationMethod {
    CURSOR,      // Cursor-based pagination (stateless, efficient)
    OFFSET,      // Offset-based pagination (page_number * page_size)
    KEYSET       // Keyset/seek pagination (ORDER BY value + pk)
};

/**
 * Pagination metadata for responses
 */
struct PageInfo {
    size_t page_size = 0;              // Items in current page
    bool has_next_page = false;        // More results available
    bool has_prev_page = false;        // Previous page available (offset-based only)
    std::optional<size_t> total_count; // Total items (if computed)
    std::optional<size_t> current_page; // Current page number (offset-based only)
    std::optional<size_t> total_pages;  // Total pages (offset-based only)
    
    nlohmann::json toJSON() const {
        nlohmann::json result = {
            {"page_size", page_size},
            {"has_next_page", has_next_page},
            {"has_prev_page", has_prev_page}
        };
        
        if (total_count.has_value()) {
            result["total_count"] = *total_count;
        }
        if (current_page.has_value()) {
            result["current_page"] = *current_page;
        }
        if (total_pages.has_value()) {
            result["total_pages"] = *total_pages;
        }
        
        return result;
    }
};

/**
 * Paginated response structure for AQL queries.
 */
struct PaginatedResponse {
    nlohmann::json items;              // Array of result items
    bool has_more = false;             // True if there are more results (legacy)
    std::string next_cursor;           // Cursor for next page (empty if no more)
    size_t batch_size = 0;             // Number of items in current batch (legacy)
    
    // Enhanced pagination metadata
    PageInfo page_info;                // Detailed pagination information
    PaginationMethod method = PaginationMethod::CURSOR;  // Pagination method used
    
    nlohmann::json toJSON() const {
        nlohmann::json result = {
            {"items", items},
            {"has_more", has_more},         // Legacy field
            {"batch_size", batch_size},     // Legacy field
            {"page_info", page_info.toJSON()},
            {"pagination_method", 
                method == PaginationMethod::CURSOR ? "cursor" :
                method == PaginationMethod::OFFSET ? "offset" : "keyset"}
        };
        
        if (has_more && !next_cursor.empty()) {
            result["next_cursor"] = next_cursor;
        }
        
        return result;
    }
};

} // namespace utils
} // namespace themis
