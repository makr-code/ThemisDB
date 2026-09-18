/**
 * @file content_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <unordered_map>
#include <atomic>
#include <istream>
#include <nlohmann/json.hpp>
#include "content/content_type.h"
#include "content/content_processor.h"
#include "content/deduplication_checker.h"
#include "content/embedding_pipeline.h"
#include "content/mime_detector.h"
#include "content/processor_chain_config.h"
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"
#include "index/vector_index.h"
#include "index/graph_index.h"
#include "index/secondary_index.h"
#include "security/encryption.h"
#include "security/malware_scanner.h"

namespace themis {
namespace content {

using json = nlohmann::json;

struct ContentMeta {
    std::string id;                  // Content UUID (without "content:" prefix)
    std::string mime_type;           // MIME type
    ContentCategory category;        // Category
    std::string original_filename;   // Original filename
    int64_t size_bytes;              // Original size
    // Compression/Encryption flags for stored blob
    bool compressed = false;         // True if original blob stored compressed
    std::string compression_type;    // e.g., "zstd"
    bool encrypted = false;          // True if blob stored encrypted
    std::string encryption_type;     // e.g., "aes-256-gcm"
    int64_t created_at;              // Upload timestamp
    int64_t modified_at;             // Last modification
    std::string hash_sha256;         // Content hash (for deduplication)
    
    // Processing metadata
    bool text_extracted;             // Text extraction successful
    bool chunked;                    // Content has been chunked
    bool indexed;                    // Chunks indexed in VectorIndex
    int chunk_count;                 // Number of chunks created
    int embedding_dim;               // Embedding dimension (if applicable)
    
    // Type-specific metadata (from extraction)
    json extracted_metadata;         // EXIF, ID3, CAD properties, etc.
    
    // User metadata
    json user_metadata;              // Application-defined metadata
    std::vector<std::string> tags;   // User-defined tags
    
    // Relations
    std::string parent_id;           // Parent content (e.g., archive member)
    std::vector<std::string> child_ids; // Child content (e.g., CAD parts, archive files)
    
    // Virtual Filesystem
    std::string virtual_path;        // Virtual filesystem path (e.g., "/documents/report.pdf")
    bool is_directory = false;       // True if this represents a directory
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    json toJson() const;
    /**
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static ContentMeta fromJson(const json& j);
};

struct ChunkMeta {
    std::string id;                  // Chunk UUID (without "chunk:" prefix)
    std::string content_id;          // Parent content ID (FK to Content)
    int seq_num;                     // Sequence number within content (0-based)
    std::string chunk_type;          // "text", "image_region", "audio_segment", "table_row", etc.
    
    // Chunk data (type-dependent)
    std::string text;                // For text chunks
    json data;                       // For structured chunks (JSON, CSV row, etc.)
    std::string blob_ref;            // Reference to blob storage (for binary chunks)
    
    // Positional metadata
    int start_offset;                // Start position in original content
    int end_offset;                  // End position in original content
    
    // Embedding
    std::vector<float> embedding;    // Optional: Store embedding directly
    bool embedding_indexed;          // True if in VectorIndex
    
    int64_t created_at;              // Creation timestamp
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    json toJson() const;
    /**
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static ChunkMeta fromJson(const json& j);
};

struct ContentAssembly {
    ContentMeta metadata;                    // Content metadata
    std::vector<ChunkMeta> chunks;           // All chunks (ordered by seq_num)
    std::optional<std::string> assembled_text; // Full text (lazy: only if requested)
    int64_t total_size_bytes;                // Total size of all chunks
    
    /**
     * @brief Get Chunk By Seq Num.
     * @param[in] seq_num Input parameter.
     * @return Return value.
     */
    std::optional<ChunkMeta> getChunkBySeqNum(int seq_num) const;
};

// Ingestion wurde entfernt. Stattdessen erwartet der Server bereits
// vorverarbeitete, strukturierte JSON-Objekte über /content/import.

struct Status {
    bool ok = true;
    std::string message;
    /**
     * @brief OK.
     * @return Return value.
     * @details Implements OK without additional internal calls.
     */
    static Status OK() { return {}; }
    /**
     * @brief Error.
     * @param[in] msg Input parameter.
     * @return Return value.
     * @details Calls: std::move().
     */
    static Status Error(std::string msg) { return Status{false, std::move(msg)}; }
};

class ContentManager {
public:
    ContentManager(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<VectorIndexManager> vector_index,
        std::shared_ptr<GraphIndexManager> graph_index,
        std::shared_ptr<SecondaryIndexManager> secondary_index,
        std::shared_ptr<FieldEncryption> field_encryption = nullptr
    );
    
    ~ContentManager() = default;

    /**
     * @brief Register Processor.
     * @param[in] processor Input parameter.
     */
    void registerProcessor(std::unique_ptr<IContentProcessor> processor);

    // user_context: z.B. Benutzer-ID für kontextabhängige Verschlüsselung
    [[nodiscard]] Status importContent(const json& spec, const std::optional<std::string>& blob = std::nullopt, const std::string& user_context = "");

    struct IngestResult {
        bool success = false;          ///< True when all required pipeline stages succeeded (CON-018)
        std::string error_message;
        std::string primary_content_id;  // Main content ID (archive or single file)
        std::vector<std::string> extracted_content_ids;  // IDs of extracted files (for archives)
        json metadata;  // Additional metadata about the ingestion

        struct StageOutcome {
            std::string stage_name;    ///< Name of the pipeline stage (e.g. "extraction").
            bool succeeded = true;     ///< Whether the stage completed without error.
            bool skipped = false;      ///< True when continue_on_error was used and stage was degraded.
            int attempts = 1;          ///< Total attempts made (1 = first try, no retries).
            std::string error_message; ///< Error description (empty when succeeded).
        };
        std::vector<StageOutcome> stage_outcomes; ///< Ordered per-stage outcomes for this ingestion.
    };
    
    [[nodiscard]] IngestResult ingestRawBlob(
        const std::string& blob,
        const std::string& filename,
        const std::string& mime_type = "",
        const std::string& user_context = "",
        const json& config = json::object()
    );

    [[nodiscard]] IngestResult ingestStream(
        std::istream& stream,
        const std::string& filename,
        const std::string& mime_type = "",
        const std::string& user_context = "",
        const json& config = json::object()
    );

    // Get content metadata; optional user_context can be provided for contextual decryption
    std::optional<ContentMeta> getContentMeta(const std::string& content_id, const std::string& user_context = "");

    std::optional<std::string> getContentBlob(const std::string& content_id, const std::string& user_context = "");

    /**
     * @brief Get Content Chunks.
     * @param[in] content_id Identifier of the content.
     * @return Return value.
     */
    std::vector<ChunkMeta> getContentChunks(const std::string& content_id);

    /**
     * @brief Get Chunk.
     * @param[in] chunk_id Identifier of the chunk.
     * @return Return value.
     */
    std::optional<ChunkMeta> getChunk(const std::string& chunk_id);

    std::optional<ContentAssembly> assembleContent(const std::string& content_id, bool include_text = false);

    /**
     * @brief Get Next Chunk.
     * @param[in] chunk_id Identifier of the chunk.
     * @return Return value.
     */
    std::optional<ChunkMeta> getNextChunk(const std::string& chunk_id);

    /**
     * @brief Get Previous Chunk.
     * @param[in] chunk_id Identifier of the chunk.
     * @return Return value.
     */
    std::optional<ChunkMeta> getPreviousChunk(const std::string& chunk_id);

    /**
     * @brief Get Chunk Range.
     * @param[in] content_id Identifier of the content.
     * @param[in] start_seq Input parameter.
     * @param[in] count Input parameter.
     * @return Return value.
     */
    std::vector<ChunkMeta> getChunkRange(const std::string& content_id, int start_seq, int count);

    std::vector<std::pair<std::string, float>> searchContent(
        const std::string& query_text,
        int k,
        const json& filters = json::object()
    );

    std::vector<std::pair<std::string, float>> searchContentHybrid(
        const std::string& query_text,
        int k,
        const json& filters = json::object(),
        float vector_weight = 0.5f,
        float fulltext_weight = 0.5f,
        float rrf_k = 60.0f
    );

    std::vector<std::pair<std::string, float>> searchWithExpansion(
        const std::string& query_text,
        int k,
        int expansion_hops,
        const json& filters = json::object()
    );

    [[nodiscard]] Status deleteContent(const std::string& content_id);

    /**
     * @brief Resolve Path.
     * @param[in] virtual_path Path to the virtual.
     * @return Return value.
     */
    std::optional<std::string> resolvePath(const std::string& virtual_path);

    /**
     * @brief List Directory.
     * @param[in] virtual_path Path to the virtual.
     * @return Return value.
     */
    std::vector<ContentMeta> listDirectory(const std::string& virtual_path);

    [[nodiscard]] Status createDirectory(const std::string& virtual_path, bool recursive = false);

    [[nodiscard]] Status registerPath(const std::string& content_id, const std::string& virtual_path);

    /**
     * @brief Get Processor.
     * @param[in] category Input parameter.
     * @return Pointer to the result.
     */
    IContentProcessor* getProcessor(ContentCategory category);

    struct Stats {
        int total_content_items = 0;   ///< CON-018
        int total_chunks = 0;          ///< CON-018
        int total_embeddings = 0;      ///< CON-018
        std::unordered_map<ContentCategory, int> items_by_category;
        int64_t total_storage_bytes = 0; ///< CON-018
    };
    /**
     * @brief Get Stats.
     * @return Return value.
     */
    Stats getStats();

    struct Metrics {
        std::atomic<uint64_t> compressed_bytes_total{0};
        std::atomic<uint64_t> uncompressed_bytes_total{0};
        std::atomic<uint64_t> compression_skipped_total{0};
        std::atomic<uint64_t> compression_skipped_image_total{0};
        std::atomic<uint64_t> compression_skipped_video_total{0};
        std::atomic<uint64_t> compression_skipped_zip_total{0};

        // Compression ratio histogram-like buckets (per-upload)
        std::atomic<uint64_t> comp_ratio_le_1{0};
        std::atomic<uint64_t> comp_ratio_le_1_5{0};
        std::atomic<uint64_t> comp_ratio_le_2{0};
        std::atomic<uint64_t> comp_ratio_le_3{0};
        std::atomic<uint64_t> comp_ratio_le_5{0};
        std::atomic<uint64_t> comp_ratio_le_10{0};
        std::atomic<uint64_t> comp_ratio_le_100{0};
        std::atomic<uint64_t> comp_ratio_le_inf{0};

        // Sum/count for average compression ratio (sum stored as milli * 1000)
        std::atomic<uint64_t> comp_ratio_sum_milli{0};
        std::atomic<uint64_t> comp_ratio_count{0};

        // Deduplication counters (content_dedup_checks_total / content_dedup_hits_total)
        std::atomic<uint64_t> dedup_checks_total{0};  ///< Number of dedup checks performed
        std::atomic<uint64_t> dedup_hits_total{0};    ///< Number of near-duplicates detected
    };

    /**
     * @brief Get Metrics.
     * @return Return value.
     */
    const Metrics& getMetrics() const;

    /**
     * @brief Set Malware Filter.
     * @param[in] malware_filter Input parameter.
     */
    void setMalwareFilter(std::shared_ptr<themis::security::MalwareFilterManager> malware_filter);

    /**
     * @brief Get Malware Filter.
     * @return Return value.
     */
    std::shared_ptr<themis::security::MalwareFilterManager> getMalwareFilter() const;

    // =========================================================================
    // Embedding Pipeline
    // =========================================================================

    /**
     * @brief Set Embedding Pipeline.
     * @param[in] pipeline Input parameter.
     */
    void setEmbeddingPipeline(std::shared_ptr<EmbeddingPipeline> pipeline);

    std::vector<float> generateEmbedding(const std::string& text,
                                          const std::string& model_name = "");

    // =========================================================================
    // LLM-assisted content analysis
    // =========================================================================

    /**
     * @brief Analyze Content.
     * @param[in] content_id Identifier of the content.
     * @return Return value.
     */
    json analyzeContent(const std::string& content_id);

    std::vector<std::string> generateTags(
        const std::string& content_id,
        int max_tags = 10
    );

    std::string summarizeContent(
        const std::string& content_id,
        int max_words = 100
    );

    /**
     * @brief Classify Content.
     * @param[in] content_id Identifier of the content.
     * @return Return value.
     */
    std::string classifyContent(const std::string& content_id);

    /**
     * @brief Extract Entities.
     * @param[in] content_id Identifier of the content.
     * @return Return value.
     */
    json extractEntities(const std::string& content_id);

    /**
     * @brief Set Deduplication Checker.
     * @param[in] checker Input parameter.
     */
    void setDeduplicationChecker(std::shared_ptr<DeduplicationChecker> checker);

    /**
     * @brief Get Deduplication Checker.
     * @return Return value.
     */
    std::shared_ptr<DeduplicationChecker> getDeduplicationChecker() const;

    /**
     * @brief Set Processor Chain Config.
     * @param[in] config Input parameter.
     */
    void setProcessorChainConfig(const ProcessorChainConfig& config);

    /**
     * @brief Get Processor Chain Config.
     * @return Return value.
     */
    const ProcessorChainConfig& getProcessorChainConfig() const;

private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<VectorIndexManager> vector_index_;
    std::shared_ptr<GraphIndexManager> graph_index_;
    std::shared_ptr<SecondaryIndexManager> secondary_index_;
    std::shared_ptr<FieldEncryption> field_encryption_;
    std::shared_ptr<themis::security::MalwareFilterManager> malware_filter_;
    std::shared_ptr<EmbeddingPipeline> embedding_pipeline_;
    std::shared_ptr<DeduplicationChecker> dedup_checker_;
    ProcessorChainConfig processor_chain_config_;  ///< Configurable stage chain.

    MimeDetector mime_detector_;
    
    // Processor registry (Category → Processor)
    std::unordered_map<ContentCategory, std::unique_ptr<IContentProcessor>> processors_;

    // Metrics instance (atomics) for Prometheus exposition
    mutable Metrics metrics_;

    // Helper methods
    /**
     * @brief Generate Uuid.
     * @return Return value.
     */
    std::string generateUuid();
    /**
     * @brief Normalize Id.
     * @param[in] id Input parameter.
     * @param[in] prefix Input parameter.
     * @return Return value.
     */
    std::string normalizeId(const std::string& id, const std::string& prefix);
    /**
     * @brief Compute SHA256.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    std::string computeSHA256(const std::string& blob);
    /**
     * @brief Check Duplicate By Hash.
     * @param[in] hash Input parameter.
     * @return Return value.
     */
    std::optional<std::string> checkDuplicateByHash(const std::string& hash);
    
    /**
     * @brief Create Chunk Graph.
     * @param[in] chunk_ids Input parameter.
     * @param[in] content_id Identifier of the content.
     * @param[in] chunk_type Input parameter.
     */
    void createChunkGraph(
        const std::vector<std::string>& chunk_ids,
        const std::string& content_id,
        const std::string& chunk_type
    );
    
    /**
     * @brief Create Hierarchical Graph.
     * @param[in] parent_id Identifier of the parent.
     * @param[in] child_ids Input parameter.
     * @param[in] edge_type Input parameter.
     */
    void createHierarchicalGraph(
        const std::string& parent_id,
        const std::vector<std::string>& child_ids,
        const std::string& edge_type
    );

    /**
     * @brief Parse Analysis Result.
     * @param[in] analysis_text Input parameter.
     * @param[in] meta Input parameter.
     * @return Return value.
     */
    json parseAnalysisResult(const std::string& analysis_text, const ContentMeta& meta);
    /**
     * @brief Parse Tags.
     * @param[in] tags_text Input parameter.
     * @return Return value.
     */
    std::vector<std::string> parseTags(const std::string& tags_text);
    /**
     * @brief Parse Entities.
     * @param[in] entities_text Input parameter.
     * @return Return value.
     */
    json parseEntities(const std::string& entities_text);
    /**
     * @brief Get Extracted Text.
     * @param[in] content_id Identifier of the content.
     * @return Return value.
     */
    std::string getExtractedText(const std::string& content_id);
};

} // namespace content
} // namespace themis

