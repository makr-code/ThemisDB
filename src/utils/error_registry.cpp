#include "utils/error_registry.h"
#include "utils/string_utils.h"
#include <algorithm>

namespace themis {
namespace errors {

json ErrorMetadata::toJSON() const {
    return {
        {"code", static_cast<int>(code)},
        {"category", category},
        {"severity", severity},
        {"message_template", message_template},
        {"cause", cause},
        {"solution", solution},
        {"related_docs", related_docs},
        {"keywords", keywords}
    };
}

ErrorRegistry& ErrorRegistry::getInstance() {
    static ErrorRegistry instance;
    return instance;
}

ErrorRegistry::ErrorRegistry() {
    registerDefaultErrors();
}

void ErrorRegistry::registerDefaultErrors() {
    // Storage Errors
    registerError({
        ErrorCode::ERR_STORAGE_FILE_NOT_FOUND,
        "Storage",
        "Error",
        "File not found: {}",
        "The specified file path does not exist or is not accessible.",
        "1. Verify the file path in configuration\n"
        "2. Check file system permissions\n"
        "3. Ensure the file was not moved or deleted",
        {"/docs/configuration.md", "/docs/troubleshooting.md"},
        {"file", "not found", "storage", "path"}
    });
    
    registerError({
        ErrorCode::ERR_STORAGE_DISK_FULL,
        "Storage",
        "Critical",
        "Disk full: {} bytes required, {} bytes available",
        "Insufficient disk space to complete the operation.",
        "1. Free up disk space by removing old logs or backups\n"
        "2. Configure retention policies\n"
        "3. Consider expanding storage capacity",
        {"/docs/maintenance.md", "/docs/retention.md"},
        {"disk", "full", "space", "storage"}
    });
    
    registerError({
        ErrorCode::ERR_STORAGE_PERMISSION_DENIED,
        "Storage",
        "Error",
        "Permission denied: {}",
        "Insufficient permissions to access the requested resource.",
        "1. Check file/directory permissions\n"
        "2. Ensure the process user has appropriate access rights\n"
        "3. Verify SELinux/AppArmor policies if applicable",
        {"/docs/security.md", "/docs/deployment.md"},
        {"permission", "denied", "access", "storage"}
    });
    
    registerError({
        ErrorCode::ERR_STORAGE_CORRUPTION,
        "Storage",
        "Critical",
        "Data corruption detected: {}",
        "The storage layer has detected corrupted data that cannot be read.",
        "1. Stop all write operations immediately\n"
        "2. Restore from the most recent backup\n"
        "3. Run integrity checks: themis-admin check --repair\n"
        "4. Check system logs for hardware errors",
        {"/docs/backup.md", "/docs/recovery.md"},
        {"corruption", "data", "integrity", "storage"}
    });
    
    // LLM Errors
    registerError({
        ErrorCode::ERR_LLM_MODEL_NOT_FOUND,
        "LLM",
        "Error",
        "Model file not found: {}",
        "The specified LLM model file does not exist at the given path.",
        "1. Verify model path in llm_config.yaml\n"
        "2. Download the model from official sources\n"
        "3. Check file permissions and ownership\n"
        "4. Ensure model format is compatible (GGUF for llama.cpp)",
        {"/docs/llm/model_loading.md", "/docs/configuration.md"},
        {"llm", "model", "not found", "path"}
    });
    
    registerError({
        ErrorCode::ERR_LLM_MODEL_LOAD_FAILED,
        "LLM",
        "Error",
        "Failed to load model from file: {}",
        "The model file exists but could not be loaded due to format or compatibility issues.",
        "1. Verify model file is not corrupted (check file size and hash)\n"
        "2. Ensure model format is GGUF for llama.cpp backend\n"
        "3. Check that model architecture is supported\n"
        "4. Review model metadata with: llama-cli --model <path> --check",
        {"/docs/llm/model_loading.md", "/docs/llm/supported_models.md"},
        {"llm", "model", "load", "failed", "gguf"}
    });
    
    registerError({
        ErrorCode::ERR_LLM_CONTEXT_CREATION_FAILED,
        "LLM",
        "Error",
        "Failed to create context for model: {}",
        "Could not initialize the inference context for the loaded model.",
        "1. Reduce context window size in configuration\n"
        "2. Free up system memory (RAM/VRAM)\n"
        "3. Check GPU drivers if using GPU acceleration\n"
        "4. Try CPU-only mode as fallback",
        {"/docs/llm/configuration.md", "/docs/llm/troubleshooting.md"},
        {"llm", "context", "creation", "failed"}
    });
    
    registerError({
        ErrorCode::ERR_LLM_INFERENCE_TIMEOUT,
        "LLM",
        "Warning",
        "Inference timeout after {} seconds",
        "Model inference took longer than the configured timeout period.",
        "1. Increase timeout in configuration\n"
        "2. Use a smaller or quantized model\n"
        "3. Enable GPU acceleration if available\n"
        "4. Reduce batch size or prompt length",
        {"/docs/llm/performance.md", "/docs/llm/quantization.md"},
        {"llm", "inference", "timeout", "performance"}
    });
    
    registerError({
        ErrorCode::ERR_LLM_GPU_OOM,
        "LLM",
        "Critical",
        "GPU out of memory: {} MB required, {} MB available",
        "Insufficient GPU VRAM to load the model or process the request.",
        "1. Use a smaller model or quantized version (Q4_K_M, Q5_K_M)\n"
        "2. Reduce context window size\n"
        "3. Enable model offloading to CPU\n"
        "4. Close other GPU applications\n"
        "5. Use multiple GPUs with tensor parallelism",
        {"/docs/llm/gpu_management.md", "/docs/llm/quantization.md"},
        {"gpu", "oom", "out of memory", "vram", "llm"}
    });
    
    registerError({
        ErrorCode::ERR_LLM_INVALID_HANDLE,
        "LLM",
        "Error",
        "Invalid model or context handle: {}",
        "The model or context handle is null, indicating improper initialization.",
        "1. Verify model was loaded successfully before use\n"
        "2. Check for previous load errors\n"
        "3. Ensure context creation completed without errors\n"
        "4. Review model initialization sequence",
        {"/docs/llm/initialization.md"},
        {"llm", "handle", "null", "invalid"}
    });
    
    registerError({
        ErrorCode::ERR_LLM_VISION_INFERENCE_FAILED,
        "LLM",
        "Error",
        "Vision inference failed: {}",
        "The vision/multimodal inference operation failed.",
        "1. Verify image format is supported\n"
        "2. Check image size and resolution limits\n"
        "3. Ensure vision model is properly loaded\n"
        "4. Review CLIP integration status",
        {"/docs/llm/vision.md"},
        {"llm", "vision", "inference", "failed", "clip"}
    });
    
    registerError({
        ErrorCode::ERR_LLM_DRAFT_MODEL_LOAD_FAILED,
        "LLM",
        "Error",
        "Failed to load draft model: {}",
        "The draft model for speculative decoding could not be loaded.",
        "1. Verify draft model path in configuration\n"
        "2. Ensure draft model is compatible with base model\n"
        "3. Check draft model file is not corrupted\n"
        "4. Verify sufficient memory for draft model",
        {"/docs/llm/speculative_decoding.md"},
        {"llm", "draft", "model", "speculative"}
    });
    
    registerError({
        ErrorCode::ERR_LLM_RAM_OOM,
        "LLM",
        "Critical",
        "Out of RAM: {} bytes required, {} bytes available",
        "Insufficient system RAM to load the model or complete the operation.",
        "1. Use a smaller model or quantized version\n"
        "2. Close other applications to free RAM\n"
        "3. Increase system swap space\n"
        "4. Consider GPU offloading if available\n"
        "5. Enable model streaming from disk",
        {"/docs/llm/memory_management.md"},
        {"ram", "oom", "out of memory", "llm"}
    });
    
    registerError({
        ErrorCode::ERR_LLM_GPU_NOT_AVAILABLE,
        "LLM",
        "Error",
        "GPU {} is not available",
        "The specified GPU device is not available or accessible.",
        "1. Verify GPU is detected: nvidia-smi or rocm-smi\n"
        "2. Check GPU drivers are installed\n"
        "3. Ensure CUDA/ROCm runtime is available\n"
        "4. Verify GPU device ID is correct\n"
        "5. Check GPU is not in exclusive compute mode",
        {"/docs/llm/gpu_setup.md"},
        {"gpu", "not available", "device", "llm"}
    });
    
    registerError({
        ErrorCode::ERR_LLM_GPU_ALLOC_FAILED,
        "LLM",
        "Critical",
        "GPU memory allocation failed: {}",
        "cudaMalloc or hipMalloc failed to allocate GPU memory.",
        "1. Check GPU has available VRAM\n"
        "2. Reduce model size or batch size\n"
        "3. Close other GPU applications\n"
        "4. Reset GPU: nvidia-smi -r\n"
        "5. Check for GPU hardware issues",
        {"/docs/llm/gpu_troubleshooting.md"},
        {"gpu", "allocation", "failed", "cudamalloc", "llm"}
    });
    
    registerError({
        ErrorCode::ERR_LLM_GPU_PEER_ACCESS_FAILED,
        "LLM",
        "Warning",
        "Failed to enable peer access from GPU {} to GPU {}: {}",
        "Could not enable direct GPU-to-GPU memory access (peer access).",
        "1. Verify GPUs support peer access\n"
        "2. Check GPUs are on same PCIe root complex\n"
        "3. Review NVIDIA/AMD documentation for peer access requirements\n"
        "4. Model will still work but with reduced performance\n"
        "5. Consider using single GPU or NVLINK/Infinity Fabric",
        {"/docs/llm/multi_gpu.md"},
        {"gpu", "peer access", "failed", "multi-gpu"}
    });
    
    // LoRA Errors
    registerError({
        ErrorCode::ERR_LORA_NOT_LOADED,
        "LoRA",
        "Error",
        "LoRA adapter not loaded: {}",
        "The requested LoRA adapter has not been loaded into memory.",
        "1. Verify LoRA adapter path in configuration\n"
        "2. Load the adapter using Multi-LoRA Manager API\n"
        "3. Check adapter compatibility with base model\n"
        "4. Verify adapter format (safetensors or GGUF)",
        {"/docs/llm/lora_management.md"},
        {"lora", "adapter", "not loaded", "multi-lora"}
    });
    
    registerError({
        ErrorCode::ERR_LORA_BATCHING_DISABLED,
        "LoRA",
        "Warning",
        "Multi-LoRA batching is disabled",
        "Multi-LoRA batching feature is not enabled in the current configuration.",
        "1. Enable multi-LoRA batching in llm_config.yaml\n"
        "2. Ensure llama.cpp backend supports multi-LoRA\n"
        "3. Check that base model supports LoRA adapters",
        {"/docs/llm/lora_configuration.md"},
        {"lora", "batching", "disabled", "configuration"}
    });
    
    registerError({
        ErrorCode::ERR_LORA_WEIGHT_MISMATCH,
        "LoRA",
        "Error",
        "Number of LoRAs ({}) doesn't match number of weights ({})",
        "LoRA fusion requires equal number of adapters and weight values.",
        "1. Ensure weights array length matches number of LoRA adapters\n"
        "2. Verify fusion configuration\n"
        "3. Check that all adapters are properly loaded",
        {"/docs/llm/lora_fusion.md"},
        {"lora", "fusion", "weights", "mismatch"}
    });
    
    registerError({
        ErrorCode::ERR_LORA_FUSION_FAILED,
        "LoRA",
        "Error",
        "LoRA fusion operation failed: {}",
        "Could not fuse multiple LoRA adapters together.",
        "1. Verify all adapters are compatible with each other\n"
        "2. Check that adapters are from the same base model\n"
        "3. Reduce number of adapters being fused\n"
        "4. Review adapter dimensions and parameters",
        {"/docs/llm/lora_fusion.md", "/docs/llm/lora_troubleshooting.md"},
        {"lora", "fusion", "failed", "compatibility"}
    });
    
    registerError({
        ErrorCode::ERR_LORA_INVALID_DATA,
        "LoRA",
        "Error",
        "Invalid LoRA data format: {}",
        "The LoRA adapter file contains invalid or corrupted data.",
        "1. Re-download the LoRA adapter file\n"
        "2. Verify file integrity with checksums\n"
        "3. Ensure adapter format is supported (safetensors/GGUF)\n"
        "4. Check adapter was trained for the correct base model",
        {"/docs/llm/lora_formats.md"},
        {"lora", "invalid", "data", "corrupted"}
    });
    
    registerError({
        ErrorCode::ERR_LORA_MODEL_MISMATCH,
        "LoRA",
        "Error",
        "Cannot fuse LoRAs from different base models: {} vs {}",
        "LoRA adapters must be trained on the same base model to be fused.",
        "1. Verify all LoRA adapters are from the same base model\n"
        "2. Check LoRA metadata for base model information\n"
        "3. Re-train adapters if necessary\n"
        "4. Use adapters separately instead of fusing",
        {"/docs/llm/lora_fusion.md"},
        {"lora", "model", "mismatch", "fusion"}
    });
    
    registerError({
        ErrorCode::ERR_LORA_GPU_LOAD_FAILED,
        "LoRA",
        "Error",
        "Failed to load LoRA on GPU {}: {}",
        "Could not load LoRA adapter onto the specified GPU.",
        "1. Verify GPU has sufficient VRAM\n"
        "2. Check LoRA adapter file is valid\n"
        "3. Ensure GPU is available and accessible\n"
        "4. Try loading on different GPU or CPU",
        {"/docs/llm/lora_gpu.md"},
        {"lora", "gpu", "load", "failed"}
    });
    
    // MCP Errors
    registerError({
        ErrorCode::ERR_MCP_TRANSPORT_FAILED,
        "MCP",
        "Error",
        "MCP transport layer failed",
        "Communication with the Model Context Protocol transport layer failed.",
        "1. Check network connectivity\n"
        "2. Verify MCP server is running\n"
        "3. Check firewall rules\n"
        "4. Review MCP configuration settings",
        {"/docs/mcp/configuration.md", "/docs/mcp/troubleshooting.md"},
        {"mcp", "transport", "failed", "network"}
    });
    
    registerError({
        ErrorCode::ERR_MCP_INVALID_REQUEST,
        "MCP",
        "Error",
        "Invalid MCP request format",
        "The MCP request does not conform to the protocol specification.",
        "1. Verify request JSON structure\n"
        "2. Check required fields are present\n"
        "3. Validate parameter types\n"
        "4. Review MCP protocol documentation",
        {"/docs/mcp/protocol.md", "/docs/mcp/api_reference.md"},
        {"mcp", "invalid", "request", "protocol"}
    });
    
    registerError({
        ErrorCode::ERR_MCP_TOOL_NOT_FOUND,
        "MCP",
        "Error",
        "MCP tool not found: {}",
        "The requested MCP tool is not registered or available.",
        "1. List available tools with: GET /api/v1/mcp/tools\n"
        "2. Verify tool name spelling\n"
        "3. Check if tool is enabled in configuration\n"
        "4. Ensure required plugins are loaded",
        {"/docs/mcp/tools.md"},
        {"mcp", "tool", "not found"}
    });
    
    registerError({
        ErrorCode::ERR_MCP_SCHEMA_UNAVAILABLE,
        "MCP",
        "Error",
        "Schema discovery requires full query engine integration",
        "MCP schema introspection is not fully implemented.",
        "1. Implement SchemaManager (see DETAILED_IMPLEMENTATION_GUIDE.md)\n"
        "2. Replace MCP stub implementations\n"
        "3. Enable schema caching\n"
        "4. Verify database is initialized",
        {"/docs/research/DETAILED_IMPLEMENTATION_GUIDE.md"},
        {"mcp", "schema", "unavailable", "stub"}
    });
    
    registerError({
        ErrorCode::ERR_MCP_STDIO_INIT_FAILED,
        "MCP",
        "Error",
        "Failed to initialize MCP stdio transport: {}",
        "Could not initialize stdin/stdout transport for MCP communication.",
        "1. Check if stdin/stdout are available\n"
        "2. Verify process has proper file descriptor access\n"
        "3. Ensure not running in detached/daemon mode\n"
        "4. Check platform-specific stdio requirements",
        {"/docs/mcp/stdio_transport.md"},
        {"mcp", "stdio", "init", "failed", "transport"}
    });
    
    // Schema Errors
    registerError({
        ErrorCode::ERR_SCHEMA_TABLE_NOT_FOUND,
        "Schema",
        "Error",
        "Table not found: {}",
        "The requested table does not exist in the database schema.",
        "1. Verify table name spelling\n"
        "2. Check if table was created\n"
        "3. Refresh schema cache\n"
        "4. Use SchemaManager.getAllTables() to list available tables",
        {"/docs/schema_management.md"},
        {"schema", "table", "not found"}
    });
    
    registerError({
        ErrorCode::ERR_SCHEMA_INVALID_TYPE,
        "Schema",
        "Error",
        "Invalid schema type: {}",
        "The specified data type is not valid for this schema operation.",
        "1. Review supported data types in documentation\n"
        "2. Check type spelling and case\n"
        "3. Verify type is available in current database version\n"
        "4. Use type aliases if applicable (e.g., 'int' vs 'integer')",
        {"/docs/schema/data_types.md"},
        {"schema", "invalid", "type", "data type"}
    });
    
    registerError({
        ErrorCode::ERR_SCHEMA_CACHE_MISS,
        "Schema",
        "Warning",
        "Schema cache miss for: {}",
        "The requested schema information is not in cache and requires database query.",
        "1. This is informational - no action required\n"
        "2. Consider preloading frequently used schemas\n"
        "3. Adjust cache size if misses are frequent\n"
        "4. Review schema caching strategy",
        {"/docs/performance/caching.md"},
        {"schema", "cache", "miss", "performance"}
    });
    
    // Network Errors
    registerError({
        ErrorCode::ERR_NET_CONNECTION_REFUSED,
        "Network",
        "Error",
        "Connection refused: {}",
        "The remote host actively refused the connection attempt.",
        "1. Verify the target service is running\n"
        "2. Check host and port are correct\n"
        "3. Review firewall rules\n"
        "4. Ensure service is listening on the correct interface",
        {"/docs/networking.md", "/docs/troubleshooting.md"},
        {"network", "connection", "refused"}
    });
    
    registerError({
        ErrorCode::ERR_NET_TIMEOUT,
        "Network",
        "Error",
        "Network timeout after {} seconds",
        "The network operation did not complete within the timeout period.",
        "1. Increase timeout value in configuration\n"
        "2. Check network latency and bandwidth\n"
        "3. Verify remote service is responsive\n"
        "4. Consider using retry logic for transient failures",
        {"/docs/networking.md", "/docs/configuration.md"},
        {"network", "timeout", "latency"}
    });
    
    registerError({
        ErrorCode::ERR_NET_DNS_FAILURE,
        "Network",
        "Error",
        "DNS resolution failed for: {}",
        "Could not resolve the hostname to an IP address.",
        "1. Verify hostname spelling\n"
        "2. Check DNS server configuration\n"
        "3. Test resolution with: nslookup <hostname>\n"
        "4. Consider using IP address directly",
        {"/docs/networking.md"},
        {"network", "dns", "resolution", "hostname"}
    });
}

void ErrorRegistry::registerError(const ErrorMetadata& metadata) {
    int code_value = static_cast<int>(metadata.code);
    errors_[code_value] = metadata;
    category_index_[metadata.category].push_back(code_value);
}

ErrorMetadata ErrorRegistry::getError(ErrorCode code) const {
    int code_value = static_cast<int>(code);
    auto it = errors_.find(code_value);
    if (it != errors_.end()) {
        return it->second;
    }
    
    // Return default error metadata
    return {
        ErrorCode::ERR_UNKNOWN,
        "Unknown",
        "Error",
        "Unknown error",
        "Error details not available",
        "Contact support for assistance",
        {},
        {"unknown"}
    };
}

std::vector<ErrorMetadata> ErrorRegistry::getErrorsByCategory(
    const std::string& category) const {
    
    std::vector<ErrorMetadata> result;
    auto it = category_index_.find(category);
    if (it != category_index_.end()) {
        for (int code : it->second) {
            result.push_back(errors_.at(code));
        }
    }
    return result;
}

std::vector<ErrorMetadata> ErrorRegistry::searchErrors(
    const std::string& query) const {
    
    std::vector<ErrorMetadata> result;
    
    for (const auto& pair : errors_) {
        // Search in keywords (case-insensitive)
        for (const auto& keyword : pair.second.keywords) {
            if (utils::containsCaseInsensitive(keyword, query)) {
                result.push_back(pair.second);
                break;
            }
        }
    }
    
    return result;
}

std::vector<std::string> ErrorRegistry::getAllCategories() const {
    std::vector<std::string> categories;
    for (const auto& [category, _] : category_index_) {
        categories.push_back(category);
    }
    return categories;
}

json ErrorRegistry::toJSON() const {
    json result = {
        {"total_errors", errors_.size()},
        {"categories", getAllCategories()},
        {"errors", json::array()}
    };
    
    for (const auto& pair : errors_) {
        result["errors"].push_back(pair.second.toJSON());
    }
    
    return result;
}

} // namespace errors
} // namespace themis
