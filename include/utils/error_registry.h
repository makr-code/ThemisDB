/**
 * @file error_registry.h
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
    ERR_QUERY_TYPE_MISMATCH = 6107,
    ERR_QUERY_RESOURCE_EXHAUSTED = 6108,
    ERR_QUERY_INVALID = 6109,
    ERR_QUERY_CANCELLED = 6112,  ///< Query was explicitly cancelled via request ID
    ERR_CACHE_ENTRY_TOO_LARGE = 6110,
    ERR_CACHE_FULL = 6111,
    ERR_QUERY_INVALID_INPUT = 6150,
    ERR_QUERY_INSUFFICIENT_DATA = 6151,
    ERR_QUERY_ACCESS_DENIED = 6152,  ///< Caller not permitted to access the requested collection (QE-2)
    
    // API Errors (6200-6299)
    ERR_API_INVALID_REQUEST = 6200,
    ERR_API_UNAUTHORIZED = 6201,
    ERR_API_RATE_LIMIT = 6202,
    ERR_API_INTERNAL_ERROR = 6203,
    ERR_API_RESOURCE_EXHAUSTED = 6204,
    
    // Plugin Errors (6300-6399)
    ERR_PLUGIN_NOT_FOUND = 6300,
    ERR_PLUGIN_LOAD_FAILED = 6301,
    ERR_PLUGIN_INCOMPATIBLE = 6302,
    ERR_PLUGIN_INVALID_SIGNATURE = 6303,
    ERR_PLUGIN_CIRCULAR_DEPENDENCY = 6304,   // Circular dependency detected in plugin graph
    ERR_PLUGIN_MISSING_DEPENDENCY = 6305,    // Required plugin dependency not registered
    ERR_PLUGIN_DEPENDENCY_CONFLICT = 6306,   // Cannot reload plugin: other loaded plugins depend on it
    ERR_PLUGIN_OCI_PULL_FAILED = 6307,       // Failed to pull plugin from OCI registry
    ERR_PLUGIN_OCI_MANIFEST_NOT_FOUND = 6308, // OCI manifest not found or registry returned 404
    ERR_PLUGIN_OCI_HASH_MISMATCH = 6309,     // Downloaded plugin blob digest does not match manifest
    ERR_PLUGIN_OCI_INVALID_REFERENCE = 6310, // Malformed OCI image reference string
    ERR_PLUGIN_CAPABILITY_ESCALATION = 6311, // Plugin attempted to escalate capabilities beyond manifest declaration
    ERR_PLUGIN_EDITION_CEILING_EXCEEDED = 6312, // Policy plugin claim exceeds compile-time edition ceiling
    ERR_PLUGIN_LICENSE_CLAIM_REJECTED   = 6313, // Policy plugin license claim rejected by RuntimeLicenseGate

    // Graph Errors (6400-6499)
    ERR_GRAPH_NO_SUCH_VERTEX = 6400,    // Referenced vertex does not exist in the graph
    ERR_GRAPH_NO_SUCH_EDGE = 6401,      // Referenced edge does not exist in the graph
    ERR_GRAPH_CONSTRAINT_CONFLICT = 6402, // Contradictory path constraints (e.g. node both required and forbidden)
    ERR_GRAPH_PATH_NOT_FOUND = 6403,    // No path satisfies all constraints
    ERR_GRAPH_CYCLE_DETECTED = 6404,    // Cycle encountered in acyclic-required traversal
    ERR_GRAPH_DEPTH_EXCEEDED = 6405,    // Query depth exceeded the configured limit
    ERR_GRAPH_RATE_LIMIT_EXCEEDED = 6406, // Query rejected: per-second rate budget exhausted
    
    // Compression Errors (7000-7099)
    ERR_COMPRESSION_FAILED = 7000,
    ERR_COMPRESSION_BUFFER_TOO_SMALL = 7001,
    ERR_COMPRESSION_INVALID_FORMAT = 7002,

    // Time Series Errors (7100-7199)
    ERR_TIMESERIES_LATE_ARRIVAL = 7100,  // Data point outside the late-arrival window
     
    // ── Phase 2 Utils Module Errors (7300-7399) ────────────────────────────────
    // Observability Plane Errors (7300-7309)
    ERR_AUDIT_BUFFER_OVERFLOW = 7300,       ///< Audit buffer overflow during write
    ERR_AUDIT_LOG_WRITE_FAILED = 7301,      ///< Failed to write audit event to storage
    ERR_AUDIT_SERIALIZATION_FAILED = 7302,  ///< Audit event serialization failed
    ERR_AUDIT_SERVICE_UNREACHABLE = 7303,   ///< External audit service unreachable
    ERR_AUDIT_FORMAT_INVALID = 7304,        ///< Audit event format validation failed
    ERR_AUDIT_PERMISSION_DENIED = 7305,     ///< Permission denied for audit operation
    ERR_AUDIT_DISK_FULL = 7306,             ///< Audit storage disk full
    ERR_AUDIT_ROTATION_FAILED = 7307,       ///< Audit log rotation operation failed
    ERR_AUDIT_SERVICE_DEGRADED = 7308,      ///< Audit service running in degraded mode
    ERR_AUDIT_CLEANUP_FAILED = 7309,        ///< Audit log cleanup operation failed
     
    // Privacy & Detection Errors (7310-7329)
    ERR_PII_DETECTION_FAILED = 7310,        ///< PII detection operation failed (general)
    ERR_PII_ENGINE_INIT_FAILED = 7311,      ///< PII detection engine initialization failed
    ERR_PII_DETECTION_TIMEOUT = 7312,       ///< PII detection exceeded timeout
    ERR_PII_UNICODE_HANDLING_ERROR = 7313,  ///< Unicode handling error in PII detector
    ERR_PII_MALFORMED_INPUT = 7314,         ///< Malformed input to PII detector
    ERR_PII_REGEX_COMPILE_FAILED = 7315,    ///< Regex compilation failed in PII engine
    ERR_PII_NER_ENGINE_ERROR = 7316,        ///< NER detection engine error
    ERR_PII_RESOURCE_EXHAUSTED = 7317,      ///< PII detection resource exhausted
    ERR_PII_POLICY_NOT_FOUND = 7318,        ///< PII detection policy not found
    ERR_PII_PSEUDONYMIZATION_FAILED = 7319, ///< PII pseudonymization operation failed
     
    // Key Management Errors (7330-7339)
    ERR_HKDF_DERIVATION_FAILED = 7330,      ///< HKDF key derivation failed
    ERR_HKDF_INVALID_PARAMS = 7331,         ///< Invalid parameters to HKDF derivation
    ERR_HKDF_CACHE_MISS = 7332,             ///< HKDF cache miss
    ERR_HKDF_CACHE_EXPIRED = 7333,          ///< HKDF cache entry expired (TTL exceeded)
    ERR_PKI_CERT_LOAD_FAILED = 7334,        ///< PKI certificate loading failed
    ERR_PKI_KEY_LOAD_FAILED = 7335,         ///< PKI private key loading failed
    ERR_PKI_SERVICE_UNAVAILABLE = 7336,     ///< PKI service unavailable
    ERR_PKI_VALIDATION_FAILED = 7337,       ///< PKI validation operation failed
    ERR_KEY_DERIVATION_TIMEOUT = 7338,      ///< Key derivation exceeded timeout
    ERR_KEY_CACHE_REFRESH_FAILED = 7339,    ///< Key cache refresh operation failed
     
    // Compression/Encoding Errors (7340-7349)
    ERR_COMPRESSION_GENERAL_FAILED = 7340,  ///< Compression operation failed (general)
    ERR_COMPRESSION_BUFFER_OVERFLOW = 7341, ///< Compression output buffer overflow
    ERR_DECOMPRESSION_FAILED = 7342,        ///< Decompression operation failed
    ERR_COMPRESSION_FORMAT_INVALID = 7343,  ///< Invalid or unrecognized compression format
    ERR_COMPRESSION_RESOURCE_EXHAUSTED = 7344, ///< Compression resource exhausted
    ERR_COMPRESSION_TIMEOUT = 7345,         ///< Compression operation exceeded timeout
    ERR_SERIALIZATION_FAILED = 7346,        ///< Serialization operation failed
    ERR_SERIALIZATION_BUFFER_OVERFLOW = 7347, ///< Serialization output buffer overflow
    ERR_CODEC_INIT_FAILED = 7348,           ///< Codec initialization failed
    ERR_CODEC_NOT_AVAILABLE = 7349,         ///< Requested codec not available
     
    // Runtime Service Errors (7350-7369)
    ERR_THREADPOOL_OVERFLOW = 7350,         ///< Thread pool work queue overflowed
    ERR_THREADPOOL_TASK_REJECTED = 7351,    ///< Thread pool rejected task (overloaded)
    ERR_THREADPOOL_TIMEOUT = 7352,          ///< Thread pool operation exceeded timeout
    ERR_RATE_LIMIT_EXCEEDED = 7353,         ///< Rate limit was exceeded
    ERR_RATE_LIMITER_ERROR = 7354,          ///< Rate limiter internal error
    ERR_CONNECTION_POOL_EXHAUSTED = 7355,   ///< Connection pool exhausted
    ERR_CONNECTION_POOL_TIMEOUT = 7356,     ///< Connection pool operation exceeded timeout
    ERR_QUEUE_DEPTH_EXCEEDED = 7357,        ///< Queue depth limit exceeded
    ERR_RESOURCE_EXHAUSTION = 7358,         ///< General resource exhaustion
    ERR_CONCURRENCY_CONFLICT = 7359,        ///< Concurrency conflict detected
    ERR_GRPC_POOL_ERROR = 7360,             ///< gRPC channel pool error
    ERR_GRPC_ROUTING_FAILED = 7361,         ///< gRPC routing/selection failed
    ERR_TRACING_DEGRADED = 7362,            ///< Tracing system running degraded
    ERR_SAGA_EVENT_LOSS = 7363,             ///< Saga event loss detected
     
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
    ERR_UTIL_COMPRESSION_FAILED = 9007,
    ERR_UTIL_ALLOCATION_FAILED = 9008,
    ERR_UTIL_UNSUPPORTED_OPERATION = 9009,
    
    // Memory Pool Errors (9100-9199)
    ERR_MEMORY_POOL_EXHAUSTED = 9100,
    ERR_MEMORY_ALLOCATION_FAILED = 9101,
    ERR_MEMORY_INVALID_SIZE = 9102,
    ERR_MEMORY_INVALID_ALIGNMENT = 9103,
    ERR_MEMORY_DOUBLE_FREE = 9104,
    ERR_MEMORY_POOL_NOT_INITIALIZED = 9105,
    ERR_MEMORY_FRAGMENTATION = 9106,
    
    // Security Errors (9200-9299)
    ERR_SECURITY_PATH_TRAVERSAL = 9200,
    ERR_SECURITY_INJECTION_DETECTED = 9201,
    
    // Exporter Errors (9300-9399)
    ERR_EXPORT_SCHEMA_VALIDATION_FAILED = 9300,
    ERR_EXPORT_IO_ERROR = 9301,
    ERR_EXPORT_SIZE_LIMIT_EXCEEDED = 9302,
    ERR_EXPORT_TENANT_UNAUTHORIZED = 9303,
    ERR_EXPORT_PII_VIOLATION = 9304,
    ERR_EXPORT_QUALITY_FILTER_FAILED = 9305,
    ERR_EXPORT_DUPLICATE_DETECTED = 9306,
    ERR_EXPORT_WEIGHT_CALCULATION_FAILED = 9307,
    ERR_EXPORT_FORMAT_INVALID = 9308,
    ERR_EXPORT_CONFIG_INVALID = 9309,
    ERR_EXPORT_POLICY_DENIED  = 9310,  ///< PolicyEngine::checkExportPermission() denied the request
    ERR_EXPORT_JOIN_COLLECTION_NOT_FOUND = 9311,  ///< Left or right collection not found
    ERR_EXPORT_JOIN_PREDICATE_INVALID    = 9312,  ///< Join predicate could not be parsed
    ERR_EXPORT_JOIN_AMBIGUOUS_FIELD      = 9313,  ///< Field present in both collections without alias
    ERR_EXPORT_JOIN_MEMORY_LIMIT         = 9314,  ///< Right-side hash table exceeded memory budget

    // Document Errors (9400-9499)
    ERR_DOC_NOT_FOUND             = 9400,  ///< Document not found in the store
    ERR_DOC_ALREADY_EXISTS        = 9401,  ///< Document with same ID already exists
    ERR_DOC_INVALID_ID            = 9402,  ///< Document ID is empty or malformed
    ERR_DOC_SCHEMA_SEALED         = 9403,  ///< Schema registry is sealed; cannot register new versions
    ERR_DOC_SCHEMA_VERSION_NOT_FOUND = 9404, ///< Requested schema version does not exist
    ERR_DOC_SCHEMA_VERSION_EXISTS = 9405,  ///< Schema version already registered
    ERR_DOC_DIFF_NOT_FOUND        = 9406,  ///< One or both documents for diff/merge not found
    ERR_DOC_MERGE_CONFLICT        = 9407,  ///< Three-way merge produced unresolvable conflicts
    ERR_DOC_ACCESS_DENIED         = 9408,  ///< Collection ACL denied the requested operation
    ERR_DOC_COLLECTION_NOT_FOUND  = 9409,  ///< Collection does not exist
    ERR_DOC_ENCRYPT_FAILED        = 9410,  ///< Encrypted entity operation failed
    ERR_DOC_INVALID_ARGUMENT      = 9411,  ///< A required argument is invalid or missing

    // ── Document module: extended error codes (Phase 2 taxonomy) ─────────────────
    ERR_DOC_SCHEMA_TRANSITION_INVALID  = 9412,  ///< Schema version transition violates ordering or compatibility rules
    ERR_DOC_SNAPSHOT_COLLISION         = 9413,  ///< Round-trip snapshot ID already exists (relay/index collision)
    ERR_DOC_ROUND_TRIP_PERSIST_FAIL    = 9414,  ///< Round-trip persistence failed at store level
    ERR_DOC_EXCHANGE_BOUNDARY_VIOLATED = 9415,  ///< XDOMEA/exchange boundary enforcement failed
    ERR_DOC_STORE_UNAVAILABLE          = 9416,  ///< Backing document store is unavailable or unresponsive
    ERR_DOC_LIFECYCLE_HOOK_FAILED      = 9417,  ///< Lifecycle hook signaled a terminal failure (rare; hooks are noexcept)
    ERR_DOC_VALIDATION_ABORTED         = 9418,  ///< Schema validation aborted due to structural document error (non-object body)
    ERR_DOC_VERSION_CONFLICT           = 9419,  ///< Concurrent version update conflict detected

    // -------------------------------------------------------------------------
    // Tool errors (ERR_TOOL_*) — 9500–9509
    // -------------------------------------------------------------------------
    ERR_TOOL_NOT_FOUND            = 9500,  ///< Named tool is not registered
    ERR_TOOL_NOT_PERMITTED        = 9501,  ///< Tool not in mode's allowlist or in denylist
    ERR_TOOL_EXECUTION_FAILED     = 9502,  ///< Tool execute() threw or returned error JSON
    ERR_TOOL_INVALID_INPUT        = 9503,  ///< Input JSON does not satisfy the tool's inputSchema
    ERR_TOOL_PLUGIN_NOT_A_TOOL    = 9504,  ///< Loaded plugin does not implement IThemisTool
    ERR_TOOL_ALREADY_REGISTERED   = 9505,  ///< A tool with this name is already registered

    // -------------------------------------------------------------------------
    // Workflow / Ingestion-Step errors (ERR_WORKFLOW_*) — 9600–9619
    // -------------------------------------------------------------------------
    ERR_WORKFLOW_PROFILE_NOT_FOUND    = 9600,  ///< YAML workflow profile could not be loaded
    ERR_WORKFLOW_PROFILE_INVALID      = 9601,  ///< YAML profile fails schema validation
    ERR_WORKFLOW_NO_MATCHING_PROFILE  = 9602,  ///< No loaded profile matches the file's MIME/name
    ERR_WORKFLOW_STEP_NOT_REGISTERED  = 9603,  ///< Step references plugin name not in StepRegistry
    ERR_WORKFLOW_STEP_EXECUTION_FAILED= 9604,  ///< A workflow step returned an error
    ERR_WORKFLOW_STEP_NOT_A_STEP      = 9605,  ///< Loaded plugin does not implement IIngestionStep
    ERR_WORKFLOW_STEP_ALREADY_REGISTERED = 9606, ///< A step with this name is already registered
    ERR_WORKFLOW_CONTEXT_INVALID      = 9607,  ///< ExtractionContext is in an invalid state
    ERR_WORKFLOW_MANIFEST_INVALID     = 9608,  ///< FileManifest is missing required fields
    ERR_WORKFLOW_QUALITY_GATE_FAILED  = 9609,  ///< Output failed minimum quality gate
    ERR_WORKFLOW_DECOMPRESS_FAILED    = 9610,  ///< Archive decompression step failed
    ERR_WORKFLOW_OCR_FAILED           = 9611,  ///< OCR extraction step failed
    ERR_WORKFLOW_EMBED_FAILED         = 9612,  ///< Embedding step failed
    ERR_WORKFLOW_ASSEMBLE_FAILED      = 9613,  ///< Base-entity assembler step failed
    ERR_WORKFLOW_SINK_FAILED          = 9614,  ///< Writing to graph/vector/document sink failed
    ERR_WORKFLOW_STEP_CONDITION_ERROR = 9615,  ///< Step condition expression evaluation failed
    ERR_WORKFLOW_CIRCULAR_DEPENDENCY  = 9616,  ///< Workflow step graph has a circular dependency
    ERR_WORKFLOW_TIMEOUT              = 9617,  ///< Workflow or individual step exceeded timeout
    ERR_WORKFLOW_QUARANTINED          = 9618,  ///< File quarantined after step failure with on_failure=quarantine
    ERR_WORKFLOW_PLUGIN_LOAD_FAILED   = 9619,  ///< Dynamic step plugin (.so/.dll) could not be loaded

    // -------------------------------------------------------------------------
    // Tensor module errors (ERR_TENSOR_*) — 9510–9599 [Phase 2 A2 Remediation]
    // -------------------------------------------------------------------------
    // Graph Errors (9510-9514): Fingerprint graph, dependency resolution
    ERR_TENSOR_GRAPH_INVALID_SELF_IP          = 9510,  ///< Fingerprint self-inner-product invalid (NaN/Inf/≤0)
    ERR_TENSOR_GRAPH_EXCEPTION_IN_SIMILARITY  = 9511,  ///< Exception thrown during similarity computation
    ERR_TENSOR_GRAPH_INVALID_SCORE            = 9512,  ///< Computed similarity score is NaN/Inf
    ERR_TENSOR_GRAPH_OTHER_TRAIN_NOT_FOUND    = 9513,  ///< Referenced tensor train entry not found
    ERR_TENSOR_GRAPH_INVALID_CROSS_IP         = 9514,  ///< Cross inner-product computation failed

    // Index Errors (9520-9529): Index construction, lookup, routing
    ERR_TENSOR_INDEX_CONSTRUCTION_FAILED      = 9520,  ///< Graph construction failed (TensorIndex creation)
    ERR_TENSOR_INDEX_LOOKUP_FAILED            = 9521,  ///< Index lookup failed (record not found)
    ERR_TENSOR_INDEX_ROUTING_FAILED           = 9522,  ///< Adapter routing decision failed
    ERR_TENSOR_INDEX_INVALID_DIMENSION        = 9523,  ///< Incompatible or invalid tensor dimension
    ERR_TENSOR_INDEX_CAPACITY_EXCEEDED        = 9524,  ///< Index capacity limit exceeded

    // Adapter Errors (9530-9539): Adapter verification, communication
    ERR_TENSOR_ADAPTER_VERIFICATION_FAILED    = 9530,  ///< Adapter verification failed
    ERR_TENSOR_ADAPTER_NOT_FOUND              = 9531,  ///< Referenced adapter not found
    ERR_TENSOR_ADAPTER_COMMUNICATION_ERROR    = 9532,  ///< Communication with adapter failed
    ERR_TENSOR_ADAPTER_INVALID_RESPONSE       = 9533,  ///< Adapter returned invalid response

    // Fingerprint Errors (9540-9549): Fingerprint computation, validation
    ERR_TENSOR_FINGERPRINT_COMPUTATION_FAILED = 9540,  ///< Fingerprint computation failed
    ERR_TENSOR_FINGERPRINT_VERIFICATION_FAILED= 9541,  ///< Fingerprint verification failed
    ERR_TENSOR_FINGERPRINT_COLLISION_DETECTED = 9542,  ///< Fingerprint collision detected (unexpected duplicates)

    // Routing Errors (9550-9559): Route selection, fallback
    ERR_TENSOR_ROUTING_DECISION_FAILED        = 9550,  ///< Route selection algorithm failed
    ERR_TENSOR_ROUTING_NO_VIABLE_ROUTE        = 9551,  ///< No viable routing target available
    ERR_TENSOR_ROUTING_FALLBACK_EXHAUSTED     = 9552,  ///< Fallback routing options exhausted

    // Recovery Errors (9560-9569): Recovery, persistence
    ERR_TENSOR_RECOVERY_FAILED                = 9560,  ///< Tensor recovery operation failed
    ERR_TENSOR_PERSISTENCE_FAILED             = 9561,  ///< Failed to persist tensor state
    ERR_TENSOR_ROCKSDB_OPERATION_FAILED       = 9562,  ///< RocksDB operation (put/get/delete) failed

    // Concurrency & Threading (9570-9579): Concurrency issues
    ERR_TENSOR_CONCURRENT_MODIFICATION        = 9570,  ///< Concurrent modification detected
    ERR_TENSOR_LOCK_ACQUISITION_FAILED        = 9571,  ///< Failed to acquire lock (timeout/contention)

    // Core Bridge (9580-9589): Core bridge operations
    ERR_TENSOR_CORE_BRIDGE_WRITE_FAILED       = 9580,  ///< Core bridge write operation failed
    ERR_TENSOR_CORE_BRIDGE_READ_FAILED        = 9581,  ///< Core bridge read operation failed

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

/** @brief Error registry for. */
class ErrorRegistry {
public:
    static ErrorRegistry& getInstance();
    
    void registerError(const ErrorMetadata& metadata);
    ErrorMetadata getError(ErrorCode code) const;
    std::vector<ErrorMetadata> getErrorsByCategory(const std::string& category) const;
    std::vector<ErrorMetadata> searchErrors(const std::string& query) const;
    std::vector<std::string> getAllCategories() const;

    /**
     * @brief Get the recovery hint (solution) for a given error code.
     *
     * Convenience shorthand for `getError(code).solution`.
     * Returns an empty string for unknown error codes.
     */
    std::string getRecoveryHint(ErrorCode code) const;

    /**
     * @brief Format an error message using its template and the provided args.
     *
     * Safe wrapper around fmt::vformat.  Falls back to the raw template if
     * formatting fails (e.g., mismatched argument count).
     */
    template<typename... Args>
    std::string formatError(ErrorCode code, Args&&... args) const {
        auto metadata = getError(code);
        try {
            return fmt::vformat(metadata.message_template,
                                fmt::make_format_args(args...));
        } catch (...) {
            return metadata.message_template;
        }
    }

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
