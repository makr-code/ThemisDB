#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <fmt/format.h>

namespace themis {
namespace errors {

using json = nlohmann::json;

enum class ErrorCode {
    // Storage Errors (1000-1999)
    ERR_STORAGE_FILE_NOT_FOUND = 1000,
    ERR_STORAGE_PERMISSION_DENIED = 1001,
    ERR_STORAGE_DISK_FULL = 1002,
    ERR_STORAGE_CORRUPTION = 1003,
    ERR_STORAGE_TRANSACTION_FAILED = 1004,
    ERR_STORAGE_CACHE_ERROR = 1005,
    ERR_STORAGE_LOG_FULL = 1006,
    ERR_STORAGE_REDUNDANCY_FAILED = 1007,
    
    // Backup & Recovery Errors (1100-1199)
    ERR_BACKUP_CREATION_FAILED = 1100,
    ERR_BACKUP_RESTORATION_FAILED = 1101,
    ERR_BACKUP_VERIFICATION_FAILED = 1102,
    ERR_BACKUP_NOT_FOUND = 1103,
    ERR_BACKUP_INVALID_TYPE = 1104,
    ERR_BACKUP_INCOMPLETE = 1105,
    ERR_BACKUP_COMPRESSION_FAILED = 1106,
    ERR_BACKUP_DECOMPRESSION_FAILED = 1107,
    ERR_BACKUP_CHECKSUM_MISMATCH = 1108,
    ERR_BACKUP_MANIFEST_CORRUPT = 1109,
    ERR_BACKUP_WAL_ARCHIVE_FAILED = 1110,
    
    // LLM Errors (2000-2099)
    ERR_LLM_MODEL_NOT_FOUND = 2000,
    ERR_LLM_MODEL_LOAD_FAILED = 2001,
    ERR_LLM_CONTEXT_CREATION_FAILED = 2002,
    ERR_LLM_INFERENCE_TIMEOUT = 2003,
    ERR_LLM_GPU_OOM = 2004,
    ERR_LLM_INVALID_HANDLE = 2005,
    ERR_LLM_VISION_INFERENCE_FAILED = 2006,
    ERR_LLM_DRAFT_MODEL_LOAD_FAILED = 2007,
    ERR_LLM_RAM_OOM = 2008,
    ERR_LLM_GPU_NOT_AVAILABLE = 2009,
    ERR_LLM_GPU_ALLOC_FAILED = 2010,
    ERR_LLM_GPU_PEER_ACCESS_FAILED = 2011,
    ERR_LLM_BATCH_SIZE_EXCEEDED = 2012,
    
    // LoRA Errors (2100-2199)
    ERR_LORA_NOT_LOADED = 2100,
    ERR_LORA_BATCHING_DISABLED = 2101,
    ERR_LORA_WEIGHT_MISMATCH = 2102,
    ERR_LORA_FUSION_FAILED = 2103,
    ERR_LORA_INVALID_DATA = 2104,
    ERR_LORA_MODEL_MISMATCH = 2105,
    ERR_LORA_GPU_LOAD_FAILED = 2106,
    ERR_LORA_ADAPTER_CONFLICT = 2107,
    ERR_LORA_TRAINING_DIVERGED = 2108,
    
    // MCP Errors (3000-3999)
    ERR_MCP_TRANSPORT_FAILED = 3000,
    ERR_MCP_INVALID_REQUEST = 3001,
    ERR_MCP_TOOL_NOT_FOUND = 3002,
    ERR_MCP_SCHEMA_UNAVAILABLE = 3003,
    ERR_MCP_STDIO_INIT_FAILED = 3004,
    
    // Schema Errors (4000-4999)
    ERR_SCHEMA_TABLE_NOT_FOUND = 4000,
    ERR_SCHEMA_INVALID_TYPE = 4001,
    ERR_SCHEMA_CACHE_MISS = 4002,
    
    // Network Errors (5000-5999)
    ERR_NET_CONNECTION_REFUSED = 5000,
    ERR_NET_TIMEOUT = 5001,
    ERR_NET_DNS_FAILURE = 5002,
    
    // Index Errors (6000-6099)
    ERR_INDEX_NOT_INITIALIZED = 6000,
    ERR_INDEX_CREATION_FAILED = 6001,
    ERR_INDEX_NOT_FOUND = 6002,
    ERR_INDEX_INVALID_TYPE = 6003,
    ERR_INDEX_MAINTENANCE_FAILED = 6004,
    ERR_INDEX_FRAGMENTATION_HIGH = 6005,
    ERR_INDEX_REBUILD_FAILED = 6006,
    ERR_INDEX_REORGANIZE_FAILED = 6007,
    ERR_INDEX_STATISTICS_UPDATE_FAILED = 6008,
    ERR_INDEX_CONSISTENCY_CHECK_FAILED = 6009,
    ERR_INDEX_MAINTENANCE_IN_PROGRESS = 6010,
    ERR_INDEX_MAINTENANCE_DISABLED = 6011,
    
    // Query Errors (6100-6199)
    ERR_QUERY_PARSE_FAILED = 6100,
    ERR_QUERY_INVALID_SYNTAX = 6101,
    ERR_QUERY_EXECUTION_FAILED = 6102,
    ERR_QUERY_TIMEOUT = 6103,
    ERR_QUERY_CTE_CYCLE_DETECTED = 6104,
    ERR_QUERY_SUBQUERY_FAILED = 6105,
    ERR_QUERY_INVALID_WINDOW_SPEC = 6106,
    ERR_QUERY_INVALID_INPUT = 6150,
    ERR_QUERY_INSUFFICIENT_DATA = 6151,
    
    // API Errors (6200-6299)
    ERR_API_INVALID_REQUEST = 6200,
    ERR_API_UNAUTHORIZED = 6201,
    ERR_API_RATE_LIMIT = 6202,
    ERR_API_INTERNAL_ERROR = 6203,
    
    // Plugin Errors (6300-6399)
    ERR_PLUGIN_NOT_FOUND = 6300,
    ERR_PLUGIN_LOAD_FAILED = 6301,
    ERR_PLUGIN_INCOMPATIBLE = 6302,
    ERR_PLUGIN_INVALID_SIGNATURE = 6303,
    
    // Compression Errors (7000-7099)
    ERR_COMPRESSION_FAILED = 7000,
    ERR_COMPRESSION_BUFFER_TOO_SMALL = 7001,
    ERR_COMPRESSION_INVALID_FORMAT = 7002,
    
    // Crypto Errors (8000-8099)
    ERR_CRYPTO_ENCRYPTION_FAILED = 8000,
    ERR_CRYPTO_DECRYPTION_FAILED = 8001,
    ERR_CRYPTO_KEY_GENERATION_FAILED = 8002,
    ERR_CRYPTO_INVALID_KEY = 8003,
    
    // Utility Errors (9000-9099)
    ERR_UTIL_INVALID_ARGUMENT = 9000,
    ERR_UTIL_FILE_OPERATION_FAILED = 9001,
    ERR_UTIL_PERMISSION_DENIED = 9002,
    ERR_UTIL_PKI_CERT_LOAD_FAILED = 9003,
    ERR_UTIL_PKI_KEY_LOAD_FAILED = 9004,
    ERR_UTIL_PII_ENGINE_CREATION_FAILED = 9005,
    ERR_UTIL_POLICY_NOT_FOUND = 9006,
    
    // Memory Pool Errors (9100-9199)
    ERR_MEMORY_POOL_EXHAUSTED = 9100,
    ERR_MEMORY_ALLOCATION_FAILED = 9101,
    ERR_MEMORY_INVALID_SIZE = 9102,
    ERR_MEMORY_INVALID_ALIGNMENT = 9103,
    ERR_MEMORY_DOUBLE_FREE = 9104,
    ERR_MEMORY_POOL_NOT_INITIALIZED = 9105,
    ERR_MEMORY_FRAGMENTATION = 9106,
    
    // Unknown
    ERR_UNKNOWN = 9999
};

struct ErrorMetadata {
    ErrorCode code;
    std::string category;           // "Storage", "LLM", "LoRA", etc.
    std::string severity;           // "Critical", "Error", "Warning"
    std::string message_template;   // Template with {} placeholders
    std::string cause;              // Detailed cause description
    std::string solution;           // Step-by-step solution
    std::vector<std::string> related_docs;  // Documentation links
    std::vector<std::string> keywords;      // For searching
    
    json toJSON() const;
};

class ErrorRegistry {
public:
    static ErrorRegistry& getInstance();
    
    void registerError(const ErrorMetadata& metadata);
    ErrorMetadata getError(ErrorCode code) const;
    std::vector<ErrorMetadata> getErrorsByCategory(const std::string& category) const;
    std::vector<ErrorMetadata> searchErrors(const std::string& query) const;
    std::vector<std::string> getAllCategories() const;
    
    json toJSON() const;
    
private:
    ErrorRegistry();
    void registerDefaultErrors();
    
    std::unordered_map<int, ErrorMetadata> errors_;
    std::unordered_map<std::string, std::vector<int>> category_index_;
};

// Helper function to log errors with error code
template<typename... Args>
void logError(ErrorCode code, Args&&... args) {
    auto& registry = ErrorRegistry::getInstance();
    auto metadata = registry.getError(code);
    
    // Use fmt::format with fmt::runtime() to avoid constexpr issues
    std::string formatted;
    try {
        formatted = fmt::vformat(metadata.message_template, 
                                fmt::make_format_args(args...));
    } catch (...) {
        formatted = metadata.message_template;
    }
    spdlog::error("[{}] {}", static_cast<int>(code), formatted);
}

} // namespace errors
} // namespace themis
