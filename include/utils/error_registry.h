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
    
    // LoRA Errors (2100-2199)
    ERR_LORA_NOT_LOADED = 2100,
    ERR_LORA_BATCHING_DISABLED = 2101,
    ERR_LORA_WEIGHT_MISMATCH = 2102,
    ERR_LORA_FUSION_FAILED = 2103,
    ERR_LORA_INVALID_DATA = 2104,
    ERR_LORA_MODEL_MISMATCH = 2105,
    ERR_LORA_GPU_LOAD_FAILED = 2106,
    
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
    
    std::string formatted = fmt::format(metadata.message_template, 
                                       std::forward<Args>(args)...);
    spdlog::error("[{}] {}", static_cast<int>(code), formatted);
}

} // namespace errors
} // namespace themis
