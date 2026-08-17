/**
 * @file error_registry.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=6, M=26, L=1
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "utils/error_registry.h"
#include "utils/string_utils.h"
#include <algorithm>
#include <iostream>
#include <mutex>
#include <stdexcept>

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
    try {
        registerDefaultErrors();
    } catch (const std::exception& ex) {
        // Avoid throwing from constructor during static initialization
        // Log to stderr directly to avoid logger initialization issues
        std::cerr << "ERROR: ErrorRegistry initialization failed: " << ex.what() << std::endl;
    } catch (const std::string& ex) {
        std::cerr << "ERROR: ErrorRegistry initialization failed: " << ex << std::endl;
    } catch (const char* ex) {
        std::cerr << "ERROR: ErrorRegistry initialization failed: "
                  << (ex ? ex : "<null>") << std::endl;
    }
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
    
    registerError({
        ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
        "Storage",
        "Error",
        "Transaction failed: {}",
        "A database transaction could not be completed successfully.",
        "1. Check if the database is in read-only mode\n"
        "2. Verify transaction timeout settings\n"
        "3. Check for deadlocks in system logs\n"
        "4. Ensure sufficient resources (memory, disk space)",
        {"/docs/transactions.md", "/docs/troubleshooting.md"},
        {"transaction", "commit", "rollback", "storage", "database"}
    });
    
    registerError({
        ErrorCode::ERR_STORAGE_CACHE_ERROR,
        "Storage",
        "Warning",
        "Cache operation failed: {}",
        "The cache layer encountered an error during read or write operation.",
        "1. Check cache configuration settings\n"
        "2. Verify cache size limits are not exceeded\n"
        "3. Clear cache if corrupted: themis-admin cache clear\n"
        "4. Review cache eviction policies",
        {"/docs/caching.md", "/docs/performance.md"},
        {"cache", "memory", "eviction", "storage"}
    });
    
    registerError({
        ErrorCode::ERR_STORAGE_LOG_FULL,
        "Storage",
        "Critical",
        "Write-ahead log is full: {}",
        "The write-ahead log (WAL) has reached capacity and cannot accept new writes.",
        "1. Trigger log checkpoint: themis-admin checkpoint\n"
        "2. Increase WAL size limit in configuration\n"
        "3. Check if log archiving is working\n"
        "4. Verify disk space is available",
        {"/docs/wal.md", "/docs/configuration.md"},
        {"wal", "log", "full", "checkpoint", "storage"}
    });
    
    registerError({
        ErrorCode::ERR_STORAGE_REDUNDANCY_FAILED,
        "Storage",
        "Error",
        "Redundancy operation failed: {}",
        "Failed to maintain data redundancy across storage backends.",
        "1. Check connectivity to all storage backends\n"
        "2. Verify backend credentials and permissions\n"
        "3. Review redundancy policy configuration\n"
        "4. Check available space on all backends",
        {"/docs/redundancy.md", "/docs/backup.md"},
        {"redundancy", "replication", "backup", "storage", "backend"}
    });
    
    // Backup & Recovery Errors
    registerError({
        ErrorCode::ERR_BACKUP_CREATION_FAILED,
        "Backup",
        "Error",
        "Backup creation failed: {}",
        "Failed to create database backup.",
        "1. Check available disk space\n"
        "2. Verify backup destination permissions\n"
        "3. Ensure database is accessible\n"
        "4. Review backup logs for specific errors",
        {"/docs/backup.md", "/docs/operations.md"},
        {"backup", "creation", "failed"}
    });
    
    registerError({
        ErrorCode::ERR_BACKUP_RESTORATION_FAILED,
        "Backup",
        "Error",
        "Backup restoration failed: {}",
        "Failed to restore database from backup.",
        "1. Verify backup integrity\n"
        "2. Check backup file permissions\n"
        "3. Ensure sufficient disk space\n"
        "4. Try restoring from an earlier backup",
        {"/docs/backup.md", "/docs/recovery.md"},
        {"backup", "restore", "recovery", "failed"}
    });
    
    registerError({
        ErrorCode::ERR_BACKUP_VERIFICATION_FAILED,
        "Backup",
        "Warning",
        "Backup verification failed: {}",
        "Backup integrity check detected issues.",
        "1. Run full backup verification\n"
        "2. Check backup checksums\n"
        "3. Consider creating a new full backup",
        {"/docs/backup.md"},
        {"backup", "verification", "integrity"}
    });
    
    registerError({
        ErrorCode::ERR_BACKUP_NOT_FOUND,
        "Backup",
        "Error",
        "Backup not found: {}",
        "The specified backup does not exist.",
        "1. Verify backup path\n"
        "2. List available backups\n"
        "3. Check backup retention policies",
        {"/docs/backup.md"},
        {"backup", "not found", "missing"}
    });
    
    registerError({
        ErrorCode::ERR_BACKUP_INVALID_TYPE,
        "Backup",
        "Error",
        "Invalid backup type: {}",
        "The backup type is not supported for this operation.",
        "1. Use full backup for restoration\n"
        "2. Restore from full backup then apply incremental/differential",
        {"/docs/backup.md"},
        {"backup", "type", "invalid"}
    });
    
    registerError({
        ErrorCode::ERR_BACKUP_INCOMPLETE,
        "Backup",
        "Critical",
        "Backup incomplete: {}",
        "Backup is missing required components (e.g., RAID shards).",
        "1. Verify all backup components exist\n"
        "2. For RAID configs, ensure all shards are backed up\n"
        "3. Create a new complete backup",
        {"/docs/backup.md", "/docs/raid.md"},
        {"backup", "incomplete", "missing", "raid"}
    });
    
    registerError({
        ErrorCode::ERR_BACKUP_COMPRESSION_FAILED,
        "Backup",
        "Error",
        "Backup compression failed: {}",
        "Failed to compress backup data.",
        "1. Check available disk space\n"
        "2. Verify tar/gzip availability\n"
        "3. Check file permissions",
        {"/docs/backup.md"},
        {"backup", "compression", "failed"}
    });
    
    registerError({
        ErrorCode::ERR_BACKUP_DECOMPRESSION_FAILED,
        "Backup",
        "Error",
        "Backup decompression failed: {}",
        "Failed to decompress backup archive.",
        "1. Verify backup file integrity\n"
        "2. Check for corruption\n"
        "3. Ensure tar/gzip is available",
        {"/docs/backup.md"},
        {"backup", "decompression", "failed"}
    });
    
    registerError({
        ErrorCode::ERR_BACKUP_CHECKSUM_MISMATCH,
        "Backup",
        "Critical",
        "Backup checksum mismatch: {}",
        "Backup file checksum does not match expected value.",
        "1. Do not use this backup - it may be corrupted\n"
        "2. Verify backup source integrity\n"
        "3. Create a new backup\n"
        "4. Check for disk errors",
        {"/docs/backup.md", "/docs/troubleshooting.md"},
        {"backup", "checksum", "corruption", "integrity"}
    });
    
    registerError({
        ErrorCode::ERR_BACKUP_MANIFEST_CORRUPT,
        "Backup",
        "Error",
        "Backup manifest corrupt: {}",
        "Backup metadata file is corrupted or unreadable.",
        "1. Check backup directory structure\n"
        "2. Verify JSON format of MANIFEST.json\n"
        "3. Restore from an earlier backup",
        {"/docs/backup.md"},
        {"backup", "manifest", "corrupt", "metadata"}
    });
    
    registerError({
        ErrorCode::ERR_BACKUP_WAL_ARCHIVE_FAILED,
        "Backup",
        "Error",
        "WAL archive failed: {}",
        "Failed to archive Write-Ahead Log files.",
        "1. Check WAL directory permissions\n"
        "2. Verify available disk space\n"
        "3. Review database logs",
        {"/docs/backup.md", "/docs/wal.md"},
        {"backup", "wal", "archive", "failed"}
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
    
    registerError({
        ErrorCode::ERR_LLM_BATCH_SIZE_EXCEEDED,
        "LLM",
        "Error",
        "Batch size {} exceeds maximum allowed: {}",
        "The requested batch size is larger than the configured maximum.",
        "1. Reduce batch size in the request\n"
        "2. Increase max_batch_size in llm_config.yaml\n"
        "3. Enable continuous batching for dynamic sizing\n"
        "4. Split large batches into multiple smaller requests\n"
        "5. Check available GPU memory if using GPU acceleration",
        {"/docs/llm/batching.md", "/docs/llm/performance.md"},
        {"llm", "batch", "size", "exceeded", "limit"}
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
    
    registerError({
        ErrorCode::ERR_LORA_ADAPTER_CONFLICT,
        "LoRA",
        "Error",
        "LoRA adapter conflict detected: {}",
        "Multiple LoRA adapters are attempting to modify the same model layers causing conflicts.",
        "1. Review adapter layer mappings for overlaps\n"
        "2. Use adapters that target different layers\n"
        "3. Load adapters sequentially instead of simultaneously\n"
        "4. Consider merging adapters offline before loading\n"
        "5. Check adapter metadata for layer compatibility",
        {"/docs/llm/lora_conflicts.md", "/docs/llm/lora_fusion.md"},
        {"lora", "conflict", "adapter", "layers", "overlap"}
    });
    
    registerError({
        ErrorCode::ERR_LORA_TRAINING_DIVERGED,
        "LoRA",
        "Critical",
        "LoRA training diverged: loss={}, gradient_norm={}",
        "Training loss diverged or gradients exploded during LoRA fine-tuning.",
        "1. Reduce learning rate (try 1e-5 or lower)\n"
        "2. Enable gradient clipping (max_grad_norm=1.0)\n"
        "3. Use smaller LoRA rank (r=8 or r=16)\n"
        "4. Check for corrupted training data\n"
        "5. Increase warmup steps\n"
        "6. Use mixed precision training (fp16/bf16)\n"
        "7. Review training loss curves for instability",
        {"/docs/llm/lora_training.md", "/docs/llm/lora_troubleshooting.md"},
        {"lora", "training", "diverged", "loss", "gradient", "explosion"}
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
        {"/research/DETAILED_IMPLEMENTATION_GUIDE.md"},
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
    
    // Index Errors
    registerError({
        ErrorCode::ERR_INDEX_NOT_INITIALIZED,
        "Index",
        "Error",
        "Index manager not initialized",
        "The index manager or index subsystem has not been properly initialized.",
        "1. Ensure database initialization completed successfully\n"
        "2. Check initialization logs for errors\n"
        "3. Verify configuration is correct\n"
        "4. Try restarting the database",
        {"/docs/index/initialization.md"},
        {"index", "manager", "not initialized"}
    });
    
    registerError({
        ErrorCode::ERR_INDEX_CREATION_FAILED,
        "Index",
        "Error",
        "Failed to create index: {}",
        "The system could not create the requested index.",
        "1. Check available disk space\n"
        "2. Verify index type is supported\n"
        "3. Check for duplicate index names\n"
        "4. Review error logs for specific failure reason",
        {"/docs/index/management.md"},
        {"index", "creation", "failed"}
    });
    
    registerError({
        ErrorCode::ERR_INDEX_NOT_FOUND,
        "Index",
        "Error",
        "Index not found: {}",
        "The requested index does not exist.",
        "1. List available indexes\n"
        "2. Check index name spelling\n"
        "3. Verify index was created successfully\n"
        "4. Check if index was dropped",
        {"/docs/index/management.md"},
        {"index", "not found"}
    });
    
    registerError({
        ErrorCode::ERR_INDEX_INVALID_TYPE,
        "Index",
        "Error",
        "Invalid index type: {}",
        "The specified index type is not supported or recognized.",
        "1. Review supported index types in documentation\n"
        "2. Check type name spelling\n"
        "3. Verify database version supports this type",
        {"/docs/index/types.md"},
        {"index", "type", "invalid"}
    });
    
    // Query Errors
    registerError({
        ErrorCode::ERR_QUERY_PARSE_FAILED,
        "Query",
        "Error",
        "Query parsing failed: {}",
        "The query could not be parsed due to syntax or semantic errors.",
        "1. Review query syntax\n"
        "2. Check for typos in keywords\n"
        "3. Verify table/field names exist\n"
        "4. Consult query language documentation",
        {"/docs/query/syntax.md"},
        {"query", "parse", "failed", "syntax"}
    });
    
    registerError({
        ErrorCode::ERR_QUERY_INVALID_SYNTAX,
        "Query",
        "Error",
        "Invalid query syntax at position {}: {}",
        "The query contains invalid syntax.",
        "1. Check query syntax matches AQL/GraphQL specification\n"
        "2. Verify all brackets/quotes are balanced\n"
        "3. Check for reserved keyword usage\n"
        "4. Review query examples in documentation",
        {"/docs/query/syntax.md"},
        {"query", "syntax", "invalid"}
    });
    
    registerError({
        ErrorCode::ERR_QUERY_EXECUTION_FAILED,
        "Query",
        "Error",
        "Query execution failed: {}",
        "The query was parsed successfully but failed during execution.",
        "1. Check if referenced tables/indexes exist\n"
        "2. Verify data types are compatible\n"
        "3. Review constraints and permissions\n"
        "4. Check system resources (memory, disk)",
        {"/docs/query/execution.md"},
        {"query", "execution", "failed"}
    });
    
    registerError({
        ErrorCode::ERR_QUERY_TIMEOUT,
        "Query",
        "Warning",
        "Query execution timeout after {} seconds",
        "The query did not complete within the configured timeout period.",
        "1. Increase query timeout in configuration\n"
        "2. Optimize query (add indexes, reduce data scanned)\n"
        "3. Check for missing indexes on filter columns\n"
        "4. Consider breaking query into smaller operations",
        {"/docs/query/optimization.md"},
        {"query", "timeout", "performance"}
    });
    
    registerError({
        ErrorCode::ERR_QUERY_CTE_CYCLE_DETECTED,
        "Query",
        "Error",
        "Circular CTE reference detected: {}",
        "Common Table Expression contains a circular reference.",
        "1. Review CTE dependencies\n"
        "2. Remove circular references\n"
        "3. Restructure query to avoid cycles",
        {"/docs/query/cte.md"},
        {"query", "cte", "cycle", "circular"}
    });
    
    registerError({
        ErrorCode::ERR_QUERY_SUBQUERY_FAILED,
        "Query",
        "Error",
        "Subquery execution failed: {}",
        "A subquery within the main query failed to execute.",
        "1. Check subquery syntax and logic\n"
        "2. Verify subquery returns expected result type\n"
        "3. Test subquery independently",
        {"/docs/query/subqueries.md"},
        {"query", "subquery", "failed"}
    });
    
    registerError({
        ErrorCode::ERR_QUERY_INVALID_WINDOW_SPEC,
        "Query",
        "Error",
        "Invalid window specification: {}",
        "Window function specification is invalid or malformed.",
        "1. Check window frame bounds (ROWS/RANGE)\n"
        "2. Verify PARTITION BY and ORDER BY clauses\n"
        "3. Review window function documentation",
        {"/docs/query/window_functions.md"},
        {"query", "window", "invalid", "specification"}
    });
    
    registerError({
        ErrorCode::ERR_QUERY_RESOURCE_EXHAUSTED,
        "Query",
        "Error",
        "Query resource limit exceeded: {}",
        "The query exceeded a configured per-query resource limit (rows, memory, or timeout).",
        "1. Reduce result set size with additional FILTER/LIMIT clauses\n"
        "2. Increase the per-query resource limit in configuration\n"
        "3. Break the query into smaller operations\n"
        "4. Add indexes to speed up execution and reduce scanned rows",
        {"/docs/query/resource-limits.md"},
        {"query", "resource", "limit", "exhausted", "timeout", "memory", "rows"}
    });

    registerError({
        ErrorCode::ERR_QUERY_CANCELLED,
        "Query",
        "Warning",
        "Query cancelled by request: {}",
        "The query was explicitly cancelled via its request ID before or during execution.",
        "1. Retry the query if cancellation was unintended\n"
        "2. Check client-side cancellation logic\n"
        "3. Review query timeout and resource settings",
        {"/docs/query/cancellation.md"},
        {"query", "cancel", "request", "abort"}
    });

    registerError({
        ErrorCode::ERR_QUERY_INVALID_INPUT,
        "Query",
        "Error",
        "Invalid input for statistical function: {}",
        "The input parameters for a statistical aggregation function are invalid.",
        "1. Check parameter ranges (e.g., percentile 0-100)\n"
        "2. Verify data types are numeric\n"
        "3. Ensure input is not null or empty\n"
        "4. Review function documentation",
        {"/docs/query/statistical_functions.md"},
        {"query", "statistical", "invalid", "input"}
    });
    
    registerError({
        ErrorCode::ERR_QUERY_INSUFFICIENT_DATA,
        "Query",
        "Error",
        "Insufficient data for statistical function: {}",
        "The statistical function requires more data points than provided.",
        "1. Check minimum data requirements (e.g., variance needs ≥2 values)\n"
        "2. Filter out null or non-numeric values\n"
        "3. Verify data source is not empty\n"
        "4. Consider using different aggregation function",
        {"/docs/query/statistical_functions.md"},
        {"query", "statistical", "insufficient", "data"}
    });
    
    // API Errors
    registerError({
        ErrorCode::ERR_API_INVALID_REQUEST,
        "API",
        "Error",
        "Invalid API request: {}",
        "The API request is malformed or contains invalid parameters.",
        "1. Verify request format (JSON/GraphQL/etc.)\n"
        "2. Check required parameters are provided\n"
        "3. Validate parameter types and values\n"
        "4. Review API documentation",
        {"/docs/api/reference.md"},
        {"api", "request", "invalid"}
    });
    
    registerError({
        ErrorCode::ERR_API_UNAUTHORIZED,
        "API",
        "Error",
        "Unauthorized: {}",
        "The request lacks valid authentication credentials.",
        "1. Verify API key or token is provided\n"
        "2. Check credentials are not expired\n"
        "3. Ensure user has required permissions\n"
        "4. Review authentication configuration",
        {"/docs/security/authentication.md"},
        {"api", "unauthorized", "authentication"}
    });
    
    registerError({
        ErrorCode::ERR_API_RATE_LIMIT,
        "API",
        "Warning",
        "Rate limit exceeded: {} requests per {}",
        "The API rate limit has been exceeded.",
        "1. Reduce request frequency\n"
        "2. Implement exponential backoff\n"
        "3. Request rate limit increase if needed\n"
        "4. Use batch operations where possible",
        {"/docs/api/rate_limits.md"},
        {"api", "rate limit", "throttling"}
    });
    
    registerError({
        ErrorCode::ERR_API_INTERNAL_ERROR,
        "API",
        "Critical",
        "Internal API error: {}",
        "An unexpected internal error occurred while processing the request.",
        "1. Check server logs for details\n"
        "2. Retry the request\n"
        "3. If persistent, report to support\n"
        "4. Verify system health and resources",
        {"/docs/troubleshooting.md"},
        {"api", "internal", "error"}
    });
    
    // Plugin Errors
    registerError({
        ErrorCode::ERR_PLUGIN_NOT_FOUND,
        "Plugin",
        "Error",
        "Plugin not found: {}",
        "The requested plugin does not exist or is not loaded.",
        "1. List available plugins\n"
        "2. Check plugin name spelling\n"
        "3. Verify plugin file exists in plugins directory\n"
        "4. Check plugin is enabled in configuration",
        {"/docs/plugins/management.md"},
        {"plugin", "not found"}
    });
    
    registerError({
        ErrorCode::ERR_PLUGIN_LOAD_FAILED,
        "Plugin",
        "Error",
        "Failed to load plugin: {}",
        "The plugin file exists but could not be loaded.",
        "1. Check plugin file is not corrupted\n"
        "2. Verify plugin is compatible with current version\n"
        "3. Review plugin dependencies\n"
        "4. Check file permissions",
        {"/docs/plugins/development.md"},
        {"plugin", "load", "failed"}
    });
    
    registerError({
        ErrorCode::ERR_PLUGIN_INCOMPATIBLE,
        "Plugin",
        "Error",
        "Plugin incompatible: {} requires version {}, found {}",
        "The plugin is not compatible with the current database version.",
        "1. Update plugin to compatible version\n"
        "2. Check plugin version requirements\n"
        "3. Update database if appropriate\n"
        "4. Contact plugin developer for compatibility",
        {"/docs/plugins/compatibility.md"},
        {"plugin", "incompatible", "version"}
    });
    
    registerError({
        ErrorCode::ERR_PLUGIN_INVALID_SIGNATURE,
        "Plugin",
        "Error",
        "Plugin signature verification failed: {}",
        "The plugin signature is invalid or missing.",
        "1. Re-download plugin from trusted source\n"
        "2. Verify plugin checksum\n"
        "3. Check signature file exists\n"
        "4. If self-signed, add to trusted keys",
        {"/docs/plugins/security.md"},
        {"plugin", "signature", "security"}
    });
    
    // Compression Errors
    registerError({
        ErrorCode::ERR_COMPRESSION_FAILED,
        "Compression",
        "Error",
        "Compression operation failed: {}",
        "Failed to compress data using the specified algorithm.",
        "1. Verify input data is not corrupted\n"
        "2. Check compression algorithm is supported\n"
        "3. Ensure sufficient memory is available\n"
        "4. Try a different compression level",
        {"/docs/compression.md"},
        {"compression", "failed", "algorithm"}
    });
    
    registerError({
        ErrorCode::ERR_COMPRESSION_BUFFER_TOO_SMALL,
        "Compression",
        "Error",
        "Compression buffer too small: {} bytes required, {} bytes available",
        "The output buffer is insufficient for compressed data.",
        "1. Increase buffer size in configuration\n"
        "2. Use dynamic buffer allocation\n"
        "3. Check compression ratio estimates",
        {"/docs/compression.md"},
        {"compression", "buffer", "size"}
    });
    
    registerError({
        ErrorCode::ERR_COMPRESSION_INVALID_FORMAT,
        "Compression",
        "Error",
        "Invalid compression format: {}",
        "The compressed data format is not recognized or corrupted.",
        "1. Verify data was compressed with supported algorithm\n"
        "2. Check for data corruption during transfer\n"
        "3. Ensure compression format version is compatible\n"
        "4. Re-compress data if source is available",
        {"/docs/compression.md"},
        {"compression", "format", "invalid", "corrupted"}
    });
     
    // ── Phase 2 Utils Module Errors (7300-7399) ────────────────────────────────
    // Observability Plane - Audit Logger Errors
    registerError({
        ErrorCode::ERR_AUDIT_BUFFER_OVERFLOW,
        "Audit",
        "Critical",
        "Audit buffer overflow: {} bytes required, {} bytes available",
        "Audit buffer capacity exceeded while writing event.",
        "1. Increase audit buffer size in configuration\n"
        "2. Reduce audit event frequency\n"
        "3. Enable audit event batching\n"
        "4. Consider implementing event filtering",
        {"/docs/audit/configuration.md", "/docs/audit/troubleshooting.md"},
        {"audit", "buffer", "overflow", "capacity"}
    });
     
    registerError({
        ErrorCode::ERR_AUDIT_LOG_WRITE_FAILED,
        "Audit",
        "Error",
        "Failed to write audit event: {}",
        "Failed to persist audit event to storage.",
        "1. Check disk space availability\n"
        "2. Verify audit log file permissions\n"
        "3. Check for I/O errors\n"
        "4. Ensure storage backend is responsive",
        {"/docs/audit/configuration.md"},
        {"audit", "write", "failed", "storage"}
    });
     
    registerError({
        ErrorCode::ERR_AUDIT_SERIALIZATION_FAILED,
        "Audit",
        "Error",
        "Failed to serialize audit event: {}",
        "Audit event serialization to storage format failed.",
        "1. Verify all audit event fields are valid\n"
        "2. Check event data does not contain invalid characters\n"
        "3. Ensure audit schema is up to date\n"
        "4. Review audit configuration",
        {"/docs/audit/schema.md"},
        {"audit", "serialization", "failed", "format"}
    });
     
    registerError({
        ErrorCode::ERR_AUDIT_SERVICE_UNREACHABLE,
        "Audit",
        "Error",
        "External audit service unreachable: {}",
        "Cannot connect to external audit service (e.g., syslog, external service).",
        "1. Verify external service is running\n"
        "2. Check network connectivity\n"
        "3. Verify service endpoint configuration\n"
        "4. Check firewall rules\n"
        "5. Audit will degrade to local logging",
        {"/docs/audit/external-services.md"},
        {"audit", "service", "unreachable", "external"}
    });
     
    registerError({
        ErrorCode::ERR_AUDIT_FORMAT_INVALID,
        "Audit",
        "Error",
        "Invalid audit event format: {}",
        "Audit event format validation failed.",
        "1. Verify audit event structure matches schema\n"
        "2. Check required fields are present\n"
        "3. Validate field data types\n"
        "4. Review audit configuration for format requirements",
        {"/docs/audit/schema.md"},
        {"audit", "format", "invalid", "validation"}
    });
     
    registerError({
        ErrorCode::ERR_AUDIT_PERMISSION_DENIED,
        "Audit",
        "Error",
        "Permission denied for audit operation: {}",
        "Insufficient permissions to perform audit operation.",
        "1. Check process user permissions\n"
        "2. Verify file/directory ownership\n"
        "3. Check SELinux/AppArmor policies\n"
        "4. Ensure audit service is running with appropriate privileges",
        {"/docs/audit/deployment.md", "/docs/security.md"},
        {"audit", "permission", "denied"}
    });
     
    registerError({
        ErrorCode::ERR_AUDIT_DISK_FULL,
        "Audit",
        "Critical",
        "Audit storage disk full: {} bytes required, {} bytes available",
        "Insufficient disk space to continue audit logging.",
        "1. Free up disk space immediately\n"
        "2. Archive old audit logs\n"
        "3. Increase storage capacity\n"
        "4. Configure retention policies\n"
        "5. Consider enabling log rotation",
        {"/docs/audit/retention.md", "/docs/maintenance.md"},
        {"audit", "disk", "full", "storage", "capacity"}
    });
     
    registerError({
        ErrorCode::ERR_AUDIT_ROTATION_FAILED,
        "Audit",
        "Error",
        "Audit log rotation failed: {}",
        "Failed to rotate audit log file.",
        "1. Check audit log file permissions\n"
        "2. Verify disk space for new log file\n"
        "3. Check for file locks\n"
        "4. Retry rotation manually if needed",
        {"/docs/audit/log-management.md"},
        {"audit", "rotation", "failed"}
    });
     
    registerError({
        ErrorCode::ERR_AUDIT_SERVICE_DEGRADED,
        "Audit",
        "Warning",
        "Audit service running in degraded mode: {}",
        "Audit service is operating with reduced functionality.",
        "1. Check system resource availability (CPU, memory)\n"
        "2. Review audit event throughput\n"
        "3. Verify storage backend responsiveness\n"
        "4. Consider enabling sampling or filtering",
        {"/docs/audit/troubleshooting.md"},
        {"audit", "degraded", "performance"}
    });
     
    registerError({
        ErrorCode::ERR_AUDIT_CLEANUP_FAILED,
        "Audit",
        "Error",
        "Audit log cleanup failed: {}",
        "Failed to cleanup old audit log files.",
        "1. Check file permissions for log directory\n"
        "2. Verify disk space for temporary cleanup operations\n"
        "3. Check for file locks or processes holding references\n"
        "4. Review retention configuration",
        {"/docs/audit/retention.md", "/docs/maintenance.md"},
        {"audit", "cleanup", "failed", "retention"}
    });
     
    // Privacy & Detection Errors
    registerError({
        ErrorCode::ERR_PII_DETECTION_FAILED,
        "Privacy",
        "Error",
        "PII detection operation failed: {}",
        "General failure in PII detection processing.",
        "1. Check input data validity\n"
        "2. Verify detection engine is initialized\n"
        "3. Review detection configuration\n"
        "4. Check system resource availability",
        {"/docs/privacy/pii-detection.md"},
        {"pii", "detection", "failed"}
    });
     
    registerError({
        ErrorCode::ERR_PII_ENGINE_INIT_FAILED,
        "Privacy",
        "Error",
        "PII detection engine initialization failed: {}",
        "Failed to initialize PII detection engine.",
        "1. Verify engine configuration is valid\n"
        "2. Check for missing pattern files or models\n"
        "3. Ensure sufficient memory is available\n"
        "4. Review engine logs for specific errors",
        {"/docs/privacy/pii-detection.md", "/docs/deployment.md"},
        {"pii", "engine", "initialization", "failed"}
    });
     
    registerError({
        ErrorCode::ERR_PII_DETECTION_TIMEOUT,
        "Privacy",
        "Error",
        "PII detection exceeded timeout: {} seconds",
        "PII detection operation did not complete within timeout.",
        "1. Increase detection timeout in configuration\n"
        "2. Reduce input data size\n"
        "3. Consider using sampling for large datasets\n"
        "4. Review detection performance characteristics",
        {"/docs/privacy/pii-detection.md", "/docs/configuration.md"},
        {"pii", "detection", "timeout"}
    });
     
    registerError({
        ErrorCode::ERR_PII_UNICODE_HANDLING_ERROR,
        "Privacy",
        "Error",
        "Unicode handling error in PII detector: {}",
        "Failed to handle Unicode characters in input.",
        "1. Verify input encoding is UTF-8\n"
        "2. Check for invalid Unicode sequences\n"
        "3. Consider normalizing input before detection\n"
        "4. Review detector language configuration",
        {"/docs/privacy/pii-detection.md"},
        {"pii", "unicode", "encoding", "character"}
    });
     
    registerError({
        ErrorCode::ERR_PII_MALFORMED_INPUT,
        "Privacy",
        "Error",
        "Malformed input to PII detector: {}",
        "Input data format is invalid or corrupted.",
        "1. Validate input format before detection\n"
        "2. Check for missing required fields\n"
        "3. Verify data encoding\n"
        "4. Review input validation configuration",
        {"/docs/privacy/pii-detection.md"},
        {"pii", "input", "malformed", "validation"}
    });
     
    registerError({
        ErrorCode::ERR_PII_REGEX_COMPILE_FAILED,
        "Privacy",
        "Error",
        "Regex compilation failed in PII engine: {}",
        "Failed to compile regex pattern for PII detection.",
        "1. Verify regex patterns are valid\n"
        "2. Check pattern configuration\n"
        "3. Review regex syntax for errors\n"
        "4. Consider using simpler patterns if applicable",
        {"/docs/privacy/pii-detection.md"},
        {"pii", "regex", "compilation", "pattern"}
    });
     
    registerError({
        ErrorCode::ERR_PII_NER_ENGINE_ERROR,
        "Privacy",
        "Error",
        "NER detection engine error: {}",
        "Named Entity Recognition engine encountered an error.",
        "1. Verify NER model is properly loaded\n"
        "2. Check input format matches model expectations\n"
        "3. Review NER engine configuration\n"
        "4. Ensure sufficient memory is available",
        {"/docs/privacy/ner-detection.md"},
        {"pii", "ner", "engine", "error"}
    });
     
    registerError({
        ErrorCode::ERR_PII_RESOURCE_EXHAUSTED,
        "Privacy",
        "Error",
        "PII detection resource exhausted: {}",
        "Insufficient system resources to complete PII detection.",
        "1. Check available memory\n"
        "2. Reduce detection scope or parallelism\n"
        "3. Enable sampling for large datasets\n"
        "4. Review resource limits in configuration",
        {"/docs/privacy/pii-detection.md"},
        {"pii", "resource", "exhausted", "memory"}
    });
     
    registerError({
        ErrorCode::ERR_PII_POLICY_NOT_FOUND,
        "Privacy",
        "Error",
        "PII detection policy not found: {}",
        "Requested PII detection policy does not exist.",
        "1. Verify policy name in configuration\n"
        "2. Check policy is properly registered\n"
        "3. Review available policies\n"
        "4. Create missing policy if needed",
        {"/docs/privacy/policies.md"},
        {"pii", "policy", "not found"}
    });
     
    registerError({
        ErrorCode::ERR_PII_PSEUDONYMIZATION_FAILED,
        "Privacy",
        "Error",
        "PII pseudonymization operation failed: {}",
        "Failed to pseudonymize detected PII.",
        "1. Verify pseudonymization configuration\n"
        "2. Check for invalid PII values\n"
        "3. Ensure pseudonymization algorithm is available\n"
        "4. Review pseudonymization logs",
        {"/docs/privacy/pseudonymization.md"},
        {"pii", "pseudonymization", "failed"}
    });
     
    // Key Management Errors
    registerError({
        ErrorCode::ERR_HKDF_DERIVATION_FAILED,
        "Crypto",
        "Error",
        "HKDF key derivation failed: {}",
        "Failed to derive key using HKDF algorithm.",
        "1. Verify input key material (IKM) is valid\n"
        "2. Check derivation parameters\n"
        "3. Ensure sufficient memory is available\n"
        "4. Review HKDF configuration",
        {"/docs/crypto/hkdf.md"},
        {"crypto", "hkdf", "derivation", "failed"}
    });
     
    registerError({
        ErrorCode::ERR_HKDF_INVALID_PARAMS,
        "Crypto",
        "Error",
        "Invalid parameters to HKDF derivation: {}",
        "HKDF derivation parameters are invalid.",
        "1. Verify all required parameters are provided\n"
        "2. Check parameter lengths meet requirements\n"
        "3. Validate parameter values\n"
        "4. Review HKDF specification",
        {"/docs/crypto/hkdf.md"},
        {"crypto", "hkdf", "parameters", "invalid"}
    });
     
    registerError({
        ErrorCode::ERR_HKDF_CACHE_MISS,
        "Crypto",
        "Warning",
        "HKDF cache miss for key: {}",
        "Requested derived key not found in cache.",
        "1. This is normal operation; derivation will be performed\n"
        "2. Monitor cache hit rate\n"
        "3. Consider adjusting cache size\n"
        "4. Review cache configuration",
        {"/docs/crypto/caching.md"},
        {"crypto", "cache", "miss", "hkdf"}
    });
     
    registerError({
        ErrorCode::ERR_HKDF_CACHE_EXPIRED,
        "Crypto",
        "Warning",
        "HKDF cache entry expired: {}",
        "Cached derived key exceeded TTL.",
        "1. Cache entry has been evicted as expected\n"
        "2. Derivation will be performed for new request\n"
        "3. Review cache TTL configuration\n"
        "4. Monitor cache performance",
        {"/docs/crypto/caching.md"},
        {"crypto", "cache", "expired", "ttl"}
    });
     
    registerError({
        ErrorCode::ERR_PKI_CERT_LOAD_FAILED,
        "Crypto",
        "Error",
        "PKI certificate loading failed: {}",
        "Failed to load certificate from storage.",
        "1. Verify certificate file path is correct\n"
        "2. Check certificate file format (PEM/DER)\n"
        "3. Ensure file is readable\n"
        "4. Validate certificate structure",
        {"/docs/crypto/pki.md", "/docs/deployment.md"},
        {"crypto", "pki", "certificate", "load", "failed"}
    });
     
    registerError({
        ErrorCode::ERR_PKI_KEY_LOAD_FAILED,
        "Crypto",
        "Error",
        "PKI private key loading failed: {}",
        "Failed to load private key from storage.",
        "1. Verify key file path is correct\n"
        "2. Check key file format (PEM/DER)\n"
        "3. Ensure file is readable and not encrypted without password\n"
        "4. Validate key structure",
        {"/docs/crypto/pki.md", "/docs/deployment.md"},
        {"crypto", "pki", "key", "load", "failed"}
    });
     
    registerError({
        ErrorCode::ERR_PKI_SERVICE_UNAVAILABLE,
        "Crypto",
        "Error",
        "PKI service unavailable: {}",
        "Cannot connect to PKI service.",
        "1. Verify PKI service is running\n"
        "2. Check network connectivity to PKI service\n"
        "3. Verify PKI service endpoint configuration\n"
        "4. Check firewall rules\n"
        "5. Consider implementing local key caching",
        {"/docs/crypto/pki.md"},
        {"crypto", "pki", "service", "unavailable"}
    });
     
    registerError({
        ErrorCode::ERR_PKI_VALIDATION_FAILED,
        "Crypto",
        "Error",
        "PKI validation operation failed: {}",
        "PKI validation (e.g., signature verification) failed.",
        "1. Verify the signed data matches signature\n"
        "2. Check certificate is valid and not expired\n"
        "3. Verify certificate chain of trust\n"
        "4. Review validation parameters",
        {"/docs/crypto/pki.md"},
        {"crypto", "pki", "validation", "failed"}
    });
     
    registerError({
        ErrorCode::ERR_KEY_DERIVATION_TIMEOUT,
        "Crypto",
        "Error",
        "Key derivation exceeded timeout: {} seconds",
        "Key derivation operation did not complete within timeout.",
        "1. Increase derivation timeout in configuration\n"
        "2. Check system resource availability\n"
        "3. Consider using cached keys\n"
        "4. Review derivation parameters complexity",
        {"/docs/crypto/configuration.md"},
        {"crypto", "derivation", "timeout"}
    });
     
    registerError({
        ErrorCode::ERR_KEY_CACHE_REFRESH_FAILED,
        "Crypto",
        "Error",
        "Key cache refresh operation failed: {}",
        "Failed to refresh cached key material.",
        "1. Verify key source is accessible\n"
        "2. Check network connectivity for remote keys\n"
        "3. Review refresh policy configuration\n"
        "4. Ensure cache has sufficient space",
        {"/docs/crypto/caching.md"},
        {"crypto", "cache", "refresh", "failed"}
    });
     
    // Compression/Encoding Errors
    registerError({
        ErrorCode::ERR_COMPRESSION_GENERAL_FAILED,
        "Compression",
        "Error",
        "Compression operation failed: {}",
        "General failure in compression operation.",
        "1. Verify input data is not corrupted\n"
        "2. Check compression algorithm is supported\n"
        "3. Ensure sufficient memory is available\n"
        "4. Try a different compression level",
        {"/docs/compression.md"},
        {"compression", "failed", "algorithm", "general"}
    });
     
    registerError({
        ErrorCode::ERR_COMPRESSION_BUFFER_OVERFLOW,
        "Compression",
        "Error",
        "Compression output buffer overflow: {} bytes required, {} bytes available",
        "Output buffer is insufficient for compressed data.",
        "1. Increase buffer size in configuration\n"
        "2. Use dynamic buffer allocation\n"
        "3. Reduce input data size\n"
        "4. Check compression ratio estimates",
        {"/docs/compression.md"},
        {"compression", "buffer", "overflow", "size"}
    });
     
    registerError({
        ErrorCode::ERR_DECOMPRESSION_FAILED,
        "Compression",
        "Error",
        "Decompression operation failed: {}",
        "Failed to decompress data.",
        "1. Verify compressed data is not corrupted\n"
        "2. Check decompression algorithm matches compression\n"
        "3. Ensure sufficient memory for decompression\n"
        "4. Verify data format is supported",
        {"/docs/compression.md"},
        {"compression", "decompression", "failed"}
    });
     
    registerError({
        ErrorCode::ERR_COMPRESSION_FORMAT_INVALID,
        "Compression",
        "Error",
        "Invalid compression format: {}",
        "Compressed data format is not recognized.",
        "1. Verify data was compressed with supported algorithm\n"
        "2. Check for data corruption during transfer\n"
        "3. Ensure format version is compatible\n"
        "4. Re-compress data if source is available",
        {"/docs/compression.md"},
        {"compression", "format", "invalid"}
    });
     
    registerError({
        ErrorCode::ERR_COMPRESSION_RESOURCE_EXHAUSTED,
        "Compression",
        "Error",
        "Compression resource exhausted: {}",
        "Insufficient system resources for compression.",
        "1. Check available memory\n"
        "2. Reduce input data size\n"
        "3. Use lower compression level\n"
        "4. Review compression configuration",
        {"/docs/compression.md"},
        {"compression", "resource", "exhausted", "memory"}
    });
     
    registerError({
        ErrorCode::ERR_COMPRESSION_TIMEOUT,
        "Compression",
        "Error",
        "Compression operation exceeded timeout: {} seconds",
        "Compression operation did not complete within timeout.",
        "1. Increase compression timeout\n"
        "2. Reduce input data size\n"
        "3. Use faster compression algorithm\n"
        "4. Check system performance",
        {"/docs/compression.md"},
        {"compression", "timeout"}
    });
     
    registerError({
        ErrorCode::ERR_SERIALIZATION_FAILED,
        "Serialization",
        "Error",
        "Serialization operation failed: {}",
        "Failed to serialize data to storage format.",
        "1. Verify object is in valid state for serialization\n"
        "2. Check serialization format specification\n"
        "3. Ensure all required fields are set\n"
        "4. Review serialization configuration",
        {"/docs/serialization.md"},
        {"serialization", "failed", "format"}
    });
     
    registerError({
        ErrorCode::ERR_SERIALIZATION_BUFFER_OVERFLOW,
        "Serialization",
        "Error",
        "Serialization output buffer overflow: {} bytes required, {} bytes available",
        "Output buffer is insufficient for serialized data.",
        "1. Increase buffer size\n"
        "2. Use dynamic buffer allocation\n"
        "3. Reduce object size or complexity\n"
        "4. Review serialization format",
        {"/docs/serialization.md"},
        {"serialization", "buffer", "overflow", "size"}
    });
     
    registerError({
        ErrorCode::ERR_CODEC_INIT_FAILED,
        "Encoding",
        "Error",
        "Codec initialization failed: {}",
        "Failed to initialize encoder/decoder.",
        "1. Verify codec configuration is valid\n"
        "2. Check codec library is properly installed\n"
        "3. Ensure required parameters are provided\n"
        "4. Review codec initialization logs",
        {"/docs/encoding/codecs.md"},
        {"codec", "initialization", "failed"}
    });
     
    registerError({
        ErrorCode::ERR_CODEC_NOT_AVAILABLE,
        "Encoding",
        "Error",
        "Requested codec not available: {}",
        "Specified codec is not supported or not compiled in.",
        "1. Check supported codecs in documentation\n"
        "2. Verify codec library version\n"
        "3. Ensure codec is compiled into build\n"
        "4. Consider using alternative codec",
        {"/docs/encoding/codecs.md"},
        {"codec", "not available", "supported"}
    });
     
    // Runtime Service Errors
    registerError({
        ErrorCode::ERR_THREADPOOL_OVERFLOW,
        "Runtime",
        "Error",
        "Thread pool work queue overflowed: {} items queued, max {} allowed",
        "Thread pool work queue capacity exceeded.",
        "1. Increase thread pool queue depth\n"
        "2. Increase number of threads\n"
        "3. Reduce task submission rate\n"
        "4. Implement backpressure handling",
        {"/docs/runtime/thread-pool.md"},
        {"threadpool", "overflow", "queue", "capacity"}
    });
     
    registerError({
        ErrorCode::ERR_THREADPOOL_TASK_REJECTED,
        "Runtime",
        "Error",
        "Thread pool rejected task: {} (overloaded)",
        "Thread pool cannot accept new tasks due to overload.",
        "1. Wait and retry the task\n"
        "2. Implement exponential backoff\n"
        "3. Reduce concurrent task submissions\n"
        "4. Consider splitting into smaller tasks",
        {"/docs/runtime/thread-pool.md"},
        {"threadpool", "task", "rejected", "overload"}
    });
     
    registerError({
        ErrorCode::ERR_THREADPOOL_TIMEOUT,
        "Runtime",
        "Error",
        "Thread pool operation exceeded timeout: {} seconds",
        "Thread pool task did not complete within timeout.",
        "1. Increase operation timeout\n"
        "2. Check if system is heavily loaded\n"
        "3. Review task implementation for performance\n"
        "4. Consider using async/await pattern",
        {"/docs/runtime/thread-pool.md"},
        {"threadpool", "timeout"}
    });
     
    registerError({
        ErrorCode::ERR_RATE_LIMIT_EXCEEDED,
        "Runtime",
        "Error",
        "Rate limit exceeded: {} requests in {} seconds (limit: {} requests/sec)",
        "Operation rate exceeds configured limit.",
        "1. Reduce operation rate\n"
        "2. Implement request batching\n"
        "3. Use adaptive rate limiting\n"
        "4. Review rate limit configuration",
        {"/docs/runtime/rate-limiting.md"},
        {"rate", "limit", "exceeded"}
    });
     
    registerError({
        ErrorCode::ERR_RATE_LIMITER_ERROR,
        "Runtime",
        "Error",
        "Rate limiter internal error: {}",
        "Rate limiter encountered an internal error.",
        "1. Check rate limiter configuration\n"
        "2. Verify time source is synchronized\n"
        "3. Check for system resource issues\n"
        "4. Review rate limiter logs",
        {"/docs/runtime/rate-limiting.md"},
        {"rate", "limiter", "error", "internal"}
    });
     
    registerError({
        ErrorCode::ERR_CONNECTION_POOL_EXHAUSTED,
        "Runtime",
        "Error",
        "Connection pool exhausted: {} connections in use, max {} allowed",
        "Connection pool capacity exceeded.",
        "1. Increase connection pool size\n"
        "2. Reduce connection hold time\n"
        "3. Implement connection recycling\n"
        "4. Check for connection leaks",
        {"/docs/runtime/connection-pool.md"},
        {"pool", "connection", "exhausted", "capacity"}
    });
     
    registerError({
        ErrorCode::ERR_CONNECTION_POOL_TIMEOUT,
        "Runtime",
        "Error",
        "Connection pool operation exceeded timeout: {} seconds",
        "Could not acquire connection from pool within timeout.",
        "1. Increase connection pool timeout\n"
        "2. Increase connection pool size\n"
        "3. Reduce connection acquisition rate\n"
        "4. Check for connection pool bottlenecks",
        {"/docs/runtime/connection-pool.md"},
        {"pool", "connection", "timeout"}
    });
     
    registerError({
        ErrorCode::ERR_QUEUE_DEPTH_EXCEEDED,
        "Runtime",
        "Error",
        "Queue depth limit exceeded: {} items, max {} allowed",
        "Queue capacity exceeded.",
        "1. Increase queue depth limit\n"
        "2. Implement queue overflow handling\n"
        "3. Increase consumer throughput\n"
        "4. Implement backpressure",
        {"/docs/runtime/queuing.md"},
        {"queue", "depth", "exceeded", "capacity"}
    });
     
    registerError({
        ErrorCode::ERR_RESOURCE_EXHAUSTION,
        "Runtime",
        "Critical",
        "Resource exhaustion: {}",
        "System has exhausted critical resources.",
        "1. Free up system resources\n"
        "2. Identify resource leak\n"
        "3. Increase resource limits\n"
        "4. Check system resource usage",
        {"/docs/runtime/resources.md"},
        {"resource", "exhausted", "critical"}
    });
     
    registerError({
        ErrorCode::ERR_CONCURRENCY_CONFLICT,
        "Runtime",
        "Error",
        "Concurrency conflict detected: {}",
        "Concurrent access conflict occurred.",
        "1. Review concurrent access patterns\n"
        "2. Implement appropriate locking\n"
        "3. Check for race conditions\n"
        "4. Review thread safety guarantees",
        {"/docs/runtime/concurrency.md"},
        {"concurrency", "conflict", "race"}
    });
     
    registerError({
        ErrorCode::ERR_GRPC_POOL_ERROR,
        "Runtime",
        "Error",
        "gRPC channel pool error: {}",
        "gRPC channel pool encountered an error.",
        "1. Check gRPC service is running\n"
        "2. Verify network connectivity\n"
        "3. Check gRPC configuration\n"
        "4. Review gRPC logs",
        {"/docs/runtime/grpc.md"},
        {"grpc", "pool", "channel", "error"}
    });
     
    registerError({
        ErrorCode::ERR_GRPC_ROUTING_FAILED,
        "Runtime",
        "Error",
        "gRPC routing/selection failed: {}",
        "Failed to route gRPC request to appropriate target.",
        "1. Verify target services are available\n"
        "2. Check routing configuration\n"
        "3. Review load balancing policy\n"
        "4. Check service discovery",
        {"/docs/runtime/grpc.md"},
        {"grpc", "routing", "failed", "selection"}
    });
     
    registerError({
        ErrorCode::ERR_TRACING_DEGRADED,
        "Observability",
        "Warning",
        "Tracing system running degraded: {}",
        "Tracing system is operating with reduced functionality.",
        "1. Check tracing backend availability\n"
        "2. Review tracing configuration\n"
        "3. Check system resource availability\n"
        "4. Consider enabling sampling",
        {"/docs/observability/tracing.md"},
        {"tracing", "degraded", "performance"}
    });
     
    registerError({
        ErrorCode::ERR_SAGA_EVENT_LOSS,
        "Observability",
        "Warning",
        "Saga event loss detected: {} events lost",
        "One or more saga events were lost.",
        "1. Review saga event buffering configuration\n"
        "2. Check event store availability\n"
        "3. Verify event persistence\n"
        "4. Monitor event loss rate",
        {"/docs/observability/sagas.md"},
        {"saga", "event", "loss", "buffer"}
    });
     
    // Crypto Errors
    registerError({
        ErrorCode::ERR_CRYPTO_ENCRYPTION_FAILED,
        "Crypto",
        "Error",
        "Encryption operation failed: {}",
        "Failed to encrypt data using the specified algorithm.",
        "1. Verify encryption key is valid\n"
        "2. Check encryption algorithm is supported\n"
        "3. Ensure input data format is correct\n"
        "4. Verify OpenSSL/crypto library is properly initialized",
        {"/docs/security/encryption.md"},
        {"crypto", "encryption", "failed"}
    });
    
    registerError({
        ErrorCode::ERR_CRYPTO_DECRYPTION_FAILED,
        "Crypto",
        "Error",
        "Decryption operation failed: {}",
        "Failed to decrypt data, possibly due to wrong key or corrupted data.",
        "1. Verify decryption key matches encryption key\n"
        "2. Check ciphertext is not corrupted\n"
        "3. Ensure correct algorithm and mode are used\n"
        "4. Verify IV/nonce if applicable",
        {"/docs/security/encryption.md"},
        {"crypto", "decryption", "failed", "key"}
    });
    
    registerError({
        ErrorCode::ERR_CRYPTO_KEY_GENERATION_FAILED,
        "Crypto",
        "Error",
        "Key generation failed: {}",
        "Failed to generate cryptographic key.",
        "1. Verify entropy source is available (/dev/urandom)\n"
        "2. Check key size is supported\n"
        "3. Ensure crypto library is properly initialized\n"
        "4. Verify sufficient system resources",
        {"/docs/security/key_management.md"},
        {"crypto", "key", "generation", "failed"}
    });
    
    registerError({
        ErrorCode::ERR_CRYPTO_INVALID_KEY,
        "Crypto",
        "Error",
        "Invalid cryptographic key: {}",
        "The provided key is invalid or malformed.",
        "1. Verify key format (PEM, DER, etc.)\n"
        "2. Check key size matches algorithm requirements\n"
        "3. Ensure key is not corrupted\n"
        "4. Verify key type (RSA, EC, etc.) is correct",
        {"/docs/security/key_management.md"},
        {"crypto", "key", "invalid"}
    });
    
    // Utility Errors
    registerError({
        ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
        "Utility",
        "Error",
        "Invalid argument: {}",
        "A function was called with an invalid argument.",
        "1. Check function parameters are correct\n"
        "2. Verify input data format\n"
        "3. Review API documentation for parameter constraints\n"
        "4. Check for null or empty values",
        {"/docs/api/utilities.md"},
        {"utility", "invalid", "argument", "parameter"}
    });
    
    registerError({
        ErrorCode::ERR_UTIL_FILE_OPERATION_FAILED,
        "Utility",
        "Error",
        "File operation failed: {}",
        "A file operation (read, write, delete) failed.",
        "1. Check file path is correct\n"
        "2. Verify file permissions\n"
        "3. Ensure parent directory exists\n"
        "4. Check disk space is available",
        {"/docs/troubleshooting.md"},
        {"utility", "file", "operation", "failed"}
    });
    
    registerError({
        ErrorCode::ERR_UTIL_PERMISSION_DENIED,
        "Utility",
        "Error",
        "Permission denied: {}",
        "Insufficient permissions to perform the operation.",
        "1. Check user/process permissions\n"
        "2. Verify file/directory ownership\n"
        "3. Review SELinux/AppArmor policies\n"
        "4. Run with appropriate privileges if needed",
        {"/docs/security.md"},
        {"utility", "permission", "denied", "access"}
    });
    
    registerError({
        ErrorCode::ERR_UTIL_PKI_CERT_LOAD_FAILED,
        "Utility",
        "Error",
        "Failed to load PKI certificate: {}",
        "Could not load or parse the certificate file.",
        "1. Verify certificate file path is correct\n"
        "2. Check certificate format (PEM expected)\n"
        "3. Ensure certificate is not expired\n"
        "4. Verify file is readable and not corrupted",
        {"/docs/security/pki.md"},
        {"utility", "pki", "certificate", "load", "failed"}
    });
    
    registerError({
        ErrorCode::ERR_UTIL_PKI_KEY_LOAD_FAILED,
        "Utility",
        "Error",
        "Failed to load PKI private key: {}",
        "Could not load or parse the private key file.",
        "1. Verify private key file path is correct\n"
        "2. Check key format (PEM expected)\n"
        "3. Ensure passphrase is correct if key is encrypted\n"
        "4. Verify file is readable and not corrupted",
        {"/docs/security/pki.md"},
        {"utility", "pki", "key", "private", "load", "failed"}
    });
    
    registerError({
        ErrorCode::ERR_UTIL_PII_ENGINE_CREATION_FAILED,
        "Utility",
        "Error",
        "Failed to create PII detection engine: {}",
        "Could not instantiate the PII detection engine.",
        "1. Verify engine type is supported (regex, ner, embedding)\n"
        "2. Check engine configuration is valid\n"
        "3. Ensure PKI signature is valid if signed engine\n"
        "4. Review engine initialization logs",
        {"/docs/privacy/pii_detection.md"},
        {"utility", "pii", "detection", "engine", "failed"}
    });
    
    registerError({
        ErrorCode::ERR_UTIL_POLICY_NOT_FOUND,
        "Utility",
        "Error",
        "Retention policy not found: {}",
        "The specified retention policy does not exist.",
        "1. Verify policy name is correct\n"
        "2. Check policy configuration file\n"
        "3. Ensure policy was registered successfully\n"
        "4. Review retention manager logs",
        {"/docs/retention.md"},
        {"utility", "retention", "policy", "not found"}
    });
    
    registerError({
        ErrorCode::ERR_UTIL_COMPRESSION_FAILED,
        "Utility",
        "Error",
        "Compression operation failed: {}",
        "Failed to compress or decompress data using ZSTD.",
        "1. Check if data is corrupted\n"
        "2. Verify input data is valid\n"
        "3. Review compression error logs\n"
        "4. Check available memory",
        {"/docs/utils/compression.md"},
        {"utility", "compression", "zstd", "failed"}
    });
    
    registerError({
        ErrorCode::ERR_UTIL_ALLOCATION_FAILED,
        "Utility",
        "Critical",
        "Memory allocation failed: {}",
        "Failed to allocate memory for compression/decompression operation.",
        "1. Check available system memory\n"
        "2. Reduce input data size\n"
        "3. Check for memory leaks\n"
        "4. Consider increasing system resources",
        {"/docs/utils/compression.md"},
        {"utility", "memory", "allocation", "failed", "oom"}
    });
    
    // Memory Pool Errors
    registerError({
        ErrorCode::ERR_MEMORY_POOL_EXHAUSTED,
        "Memory",
        "Critical",
        "Memory pool exhausted: requested {} bytes, available {} bytes",
        "The memory pool has run out of available memory blocks.",
        "1. Increase pool size in configuration\n"
        "2. Enable dynamic pool expansion\n"
        "3. Check for memory leaks\n"
        "4. Review allocation patterns and optimize",
        {"/docs/memory/pool_allocator.md"},
        {"memory", "pool", "exhausted", "out of memory"}
    });
    
    registerError({
        ErrorCode::ERR_MEMORY_ALLOCATION_FAILED,
        "Memory",
        "Critical",
        "Memory allocation failed: requested {} bytes",
        "Failed to allocate memory from the system.",
        "1. Check available system memory\n"
        "2. Reduce memory usage in configuration\n"
        "3. Enable memory compression\n"
        "4. Check for memory leaks",
        {"/docs/memory/troubleshooting.md"},
        {"memory", "allocation", "failed", "oom"}
    });
    
    registerError({
        ErrorCode::ERR_MEMORY_INVALID_SIZE,
        "Memory",
        "Error",
        "Invalid allocation size: {} bytes",
        "The requested allocation size is invalid or exceeds limits.",
        "1. Verify allocation size is positive and non-zero\n"
        "2. Check size does not exceed max allocation limit\n"
        "3. Ensure size is properly aligned if required",
        {"/docs/memory/pool_allocator.md"},
        {"memory", "invalid", "size", "allocation"}
    });
    
    registerError({
        ErrorCode::ERR_MEMORY_INVALID_ALIGNMENT,
        "Memory",
        "Error",
        "Invalid memory alignment: requested {} bytes, must be power of 2",
        "The requested alignment is not a power of 2.",
        "1. Verify alignment is a power of 2 (e.g., 8, 16, 32, 64)\n"
        "2. Use standard alignment values (16 or 64 bytes)\n"
        "3. Check alignment requirements for your data structure",
        {"/docs/memory/pool_allocator.md"},
        {"memory", "alignment", "invalid", "power of 2"}
    });
    
    registerError({
        ErrorCode::ERR_MEMORY_DOUBLE_FREE,
        "Memory",
        "Critical",
        "Double free detected: address {}",
        "Attempted to free memory that was already freed.",
        "1. Check for double free bugs in code\n"
        "2. Ensure proper ownership semantics\n"
        "3. Use smart pointers or RAII patterns\n"
        "4. Run with AddressSanitizer to detect memory errors",
        {"/docs/memory/debugging.md"},
        {"memory", "double free", "corruption", "bug"}
    });
    
    registerError({
        ErrorCode::ERR_MEMORY_POOL_NOT_INITIALIZED,
        "Memory",
        "Error",
        "Memory pool not initialized",
        "Attempted to use a memory pool before initialization.",
        "1. Ensure pool is initialized before use\n"
        "2. Check initialization order\n"
        "3. Verify pool configuration is valid",
        {"/docs/memory/pool_allocator.md"},
        {"memory", "pool", "not initialized", "initialization"}
    });
    
    registerError({
        ErrorCode::ERR_MEMORY_FRAGMENTATION,
        "Memory",
        "Warning",
        "Memory fragmentation detected: {} free blocks, largest {} bytes",
        "Memory pool is fragmented with many small free blocks.",
        "1. Enable pool defragmentation\n"
        "2. Increase pool size\n"
        "3. Adjust allocation sizes\n"
        "4. Consider pool reset during low-activity periods",
        {"/docs/memory/pool_allocator.md"},
        {"memory", "fragmentation", "performance", "optimization"}
    });
    
    // Security Errors
    registerError({
        ErrorCode::ERR_SECURITY_INJECTION_DETECTED,
        "Security",
        "Critical",
        "AQL injection detected: {}",
        "The query contains patterns that indicate a SQL/AQL injection attempt.",
        "1. Use parameterized queries instead of string concatenation\n"
        "2. Validate all user input before using in queries\n"
        "3. Review the query for suspicious patterns (DROP, DELETE, --, etc.)\n"
        "4. Check security audit logs for the source of the malicious query",
        {"/docs/security/injection_prevention.md"},
        {"security", "injection", "sql", "aql", "validation", "attack"}
    });

    // Graph Errors (6400-6499)
    registerError({
        ErrorCode::ERR_GRAPH_NO_SUCH_VERTEX,
        "Graph",
        "Error",
        "Vertex not found: {}",
        "The referenced vertex does not exist in the graph.",
        "1. Verify the vertex ID is correct\n"
        "2. Ensure the vertex was added before querying\n"
        "3. Check for typos in the vertex identifier",
        {"/docs/graph_roadmap.md"},
        {"graph", "vertex", "not found", "missing"}
    });

    registerError({
        ErrorCode::ERR_GRAPH_NO_SUCH_EDGE,
        "Graph",
        "Error",
        "Edge not found: {}",
        "The referenced edge does not exist in the graph.",
        "1. Verify the edge ID is correct\n"
        "2. Ensure the edge was added before querying\n"
        "3. Check for typos in the edge identifier",
        {"/docs/graph_roadmap.md"},
        {"graph", "edge", "not found", "missing"}
    });

    registerError({
        ErrorCode::ERR_GRAPH_CONSTRAINT_CONFLICT,
        "Graph",
        "Error",
        "Contradictory path constraints: {}",
        "The path constraints are contradictory and cannot be satisfied simultaneously.",
        "1. Review path constraints for conflicting requirements\n"
        "2. Ensure a node/edge is not both required and forbidden\n"
        "3. Check that min_length <= max_length",
        {"/docs/graph_roadmap.md"},
        {"graph", "constraint", "conflict", "path", "contradictory"}
    });

    registerError({
        ErrorCode::ERR_GRAPH_PATH_NOT_FOUND,
        "Graph",
        "Error",
        "No path found from '{}' to '{}'",
        "No path satisfying all constraints exists between the given vertices.",
        "1. Verify the graph is connected between the start and end vertices\n"
        "2. Relax path constraints (increase max_length, remove forbidden nodes)\n"
        "3. Check if there are any isolated subgraphs",
        {"/docs/graph_roadmap.md"},
        {"graph", "path", "not found", "unreachable", "disconnected"}
    });

    registerError({
        ErrorCode::ERR_GRAPH_CYCLE_DETECTED,
        "Graph",
        "Error",
        "Cycle detected at vertex: {}",
        "A cycle was encountered during a traversal that requires an acyclic path.",
        "1. Remove the acyclic constraint if cycles are acceptable\n"
        "2. Use UNIQUE_NODES constraint to avoid revisiting vertices\n"
        "3. Pre-check the graph for cycles before running acyclic traversals",
        {"/docs/graph_roadmap.md"},
        {"graph", "cycle", "acyclic", "path", "loop"}
    });

    registerError({
        ErrorCode::ERR_GRAPH_DEPTH_EXCEEDED,
        "Graph",
        "Warning",
        "Query depth exceeded limit of {}",
        "The graph traversal exceeded the configured maximum depth limit.",
        "1. Increase the max_depth constraint if deeper traversal is needed\n"
        "2. Use a more targeted query starting closer to the target vertex\n"
        "3. Consider using an index for faster traversal",
        {"/docs/graph_roadmap.md"},
        {"graph", "depth", "limit", "traversal", "exceeded"}
    });

    registerError({
        ErrorCode::ERR_GRAPH_RATE_LIMIT_EXCEEDED,
        "Graph",
        "Warning",
        "Graph query rate limit exceeded (max {} queries/second)",
        "The query was rejected because the per-second query budget set by "
        "setMaxQueriesPerSecond() has been exhausted for the current window.",
        "1. Reduce query frequency or batch multiple queries\n"
        "2. Increase the rate limit via setMaxQueriesPerSecond()\n"
        "3. Set max_qps to 0 to disable rate limiting",
        {"/docs/graph_roadmap.md"},
        {"graph", "rate", "limit", "qps", "throttle"}
    });

    // ── Document Errors (9400-9499) ───────────────────────────────────────────

    registerError({
        ErrorCode::ERR_DOC_NOT_FOUND,
        "Document",
        "Error",
        "Document not found: {}",
        "The requested document ID does not exist in the specified collection.",
        "1. Verify the document ID is correct\n"
        "2. Confirm the collection name is correct\n"
        "3. Check if the document was deleted",
        {"/docs/document_module.md"},
        {"document", "not_found", "missing", "id"}
    });

    registerError({
        ErrorCode::ERR_DOC_ALREADY_EXISTS,
        "Document",
        "Error",
        "Document already exists: {}",
        "A document with the given ID already exists in the collection.",
        "1. Use a unique document ID\n"
        "2. Use update() to modify an existing document\n"
        "3. Remove the existing document first if replacement is intended",
        {"/docs/document_module.md"},
        {"document", "duplicate", "exists", "id"}
    });

    registerError({
        ErrorCode::ERR_DOC_INVALID_ID,
        "Document",
        "Error",
        "Invalid document ID: {}",
        "The document ID is empty or contains invalid characters.",
        "1. Provide a non-empty document identifier (UUID recommended)\n"
        "2. Ensure the ID contains only printable characters",
        {"/docs/document_module.md"},
        {"document", "id", "invalid", "empty"}
    });

    registerError({
        ErrorCode::ERR_DOC_SCHEMA_SEALED,
        "Document",
        "Error",
        "Schema registry is sealed; cannot register new versions",
        "The IDocumentSchemaEvolution instance has been sealed via seal(); "
        "no further schema versions can be registered.",
        "1. Register all required schema versions before calling seal()\n"
        "2. Create a new IDocumentSchemaEvolution instance if schema changes are required",
        {"/docs/document_schema_evolution.md"},
        {"document", "schema", "sealed", "version", "evolution"}
    });

    registerError({
        ErrorCode::ERR_DOC_SCHEMA_VERSION_NOT_FOUND,
        "Document",
        "Error",
        "Schema version not found: {}",
        "The requested schema version has not been registered.",
        "1. Call registerVersion() for the required schema version first\n"
        "2. List registered versions to confirm availability",
        {"/docs/document_schema_evolution.md"},
        {"document", "schema", "version", "missing"}
    });

    registerError({
        ErrorCode::ERR_DOC_SCHEMA_VERSION_EXISTS,
        "Document",
        "Error",
        "Schema version already registered: {}",
        "Attempting to register a schema version that already exists.",
        "1. Use a different version number\n"
        "2. Schema versions are immutable after registration",
        {"/docs/document_schema_evolution.md"},
        {"document", "schema", "version", "duplicate"}
    });

    registerError({
        ErrorCode::ERR_DOC_DIFF_NOT_FOUND,
        "Document",
        "Error",
        "One or both documents for diff/merge not found: {}",
        "The document IDs provided to diff() or merge() do not exist in the store.",
        "1. Verify that all document IDs exist before calling diff() or merge()\n"
        "2. Check the collection name is correct",
        {"/docs/document_diff_merge.md"},
        {"document", "diff", "merge", "not_found"}
    });

    registerError({
        ErrorCode::ERR_DOC_MERGE_CONFLICT,
        "Document",
        "Warning",
        "Three-way merge produced unresolvable conflicts for document: {}",
        "Both branches modified the same fields with different values; "
        "conflicts are listed in MergeResult::conflicts.",
        "1. Inspect MergeResult::conflicts to identify conflicting fields\n"
        "2. Resolve conflicts manually and apply the merged result\n"
        "3. Consider using a conflict-resolution strategy",
        {"/docs/document_diff_merge.md"},
        {"document", "merge", "conflict", "three-way"}
    });

    registerError({
        ErrorCode::ERR_DOC_ACCESS_DENIED,
        "Document",
        "Error",
        "Document access denied for collection: {}",
        "The collection-level ACL denied the requested operation.",
        "1. Verify the actor identity is authorised for this collection\n"
        "2. Check the collection access-control configuration",
        {"/docs/document_module.md"},
        {"document", "access", "denied", "acl", "permission"}
    });

    registerError({
        ErrorCode::ERR_DOC_COLLECTION_NOT_FOUND,
        "Document",
        "Error",
        "Collection not found: {}",
        "The specified collection does not exist.",
        "1. Check the collection name\n"
        "2. Create the collection before inserting documents",
        {"/docs/document_module.md"},
        {"document", "collection", "not_found"}
    });

    registerError({
        ErrorCode::ERR_DOC_ENCRYPT_FAILED,
        "Document",
        "Error",
        "Encrypted entity operation failed: {}",
        "An operation on an IEncryptedDocumentEntity failed "
        "(e.g. key rotation, re-encryption).",
        "1. Check the key rotation descriptor for valid key IDs\n"
        "2. Verify the key provider is accessible\n"
        "3. Inspect logs for underlying cryptographic errors",
        {"/docs/document_manager.md"},
        {"document", "encrypt", "key_rotation", "encrypted_entity"}
    });

    registerError({
        ErrorCode::ERR_DOC_INVALID_ARGUMENT,
        "Document",
        "Error",
        "Invalid argument: {}",
        "A required argument passed to a document API method is invalid or missing.",
        "1. Check that required fields are non-empty\n"
        "2. Consult the method documentation for valid argument ranges",
        {"/docs/document_module.md"},
        {"document", "argument", "invalid", "null", "empty"}
    });
}

void ErrorRegistry::registerError(const ErrorMetadata& metadata) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    int code_value = static_cast<int>(metadata.code);
    errors_[code_value] = metadata;
    category_index_[metadata.category].push_back(code_value);
}

ErrorMetadata ErrorRegistry::getError(ErrorCode code) const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
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
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
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
    std::shared_lock<std::shared_mutex> lock(mutex_);
    
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
    std::shared_lock<std::shared_mutex> lock(mutex_);
    std::vector<std::string> categories;
    for (const auto& [category, _] : category_index_) {
        categories.push_back(category);
    }
    return categories;
}

json ErrorRegistry::toJSON() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    json result = {
        {"total_errors", errors_.size()},
        {"categories", json::array()},
        {"errors", json::array()}
    };

    for (const auto& [category, _] : category_index_) {
        result["categories"].push_back(category);
    }
    
    for (const auto& pair : errors_) {
        result["errors"].push_back(pair.second.toJSON());
    }
    
    return result;
}

std::string ErrorRegistry::getRecoveryHint(ErrorCode code) const {
    return getError(code).solution;
}

} // namespace errors
} // namespace themis
