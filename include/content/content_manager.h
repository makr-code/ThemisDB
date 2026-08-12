/**
 * @file content_manager.h
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

/**
 * @brief Content Metadata (Universal)
 * 
 * Unified metadata structure for all content types.
 * Primary Key: content:<uuid>
 */
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
    
    json toJson() const;
    static ContentMeta fromJson(const json& j);
};

/**
 * @brief Chunk Metadata (Universal)
 * 
 * Represents a chunk from any content type.
 * Primary Key: chunk:<uuid>
 */
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
    
    json toJson() const;
    static ChunkMeta fromJson(const json& j);
};

/**
 * @brief Content Assembly Result
 * 
 * Efficient container for reconstructed content with lazy-loaded chunks.
 */
struct ContentAssembly {
    ContentMeta metadata;                    // Content metadata
    std::vector<ChunkMeta> chunks;           // All chunks (ordered by seq_num)
    std::optional<std::string> assembled_text; // Full text (lazy: only if requested)
    int64_t total_size_bytes;                // Total size of all chunks
    
    // Helper: Get chunk by sequence number
    std::optional<ChunkMeta> getChunkBySeqNum(int seq_num) const;
};

// Ingestion wurde entfernt. Stattdessen erwartet der Server bereits
// vorverarbeitete, strukturierte JSON-Objekte über /content/import.

/**
 * @brief Generic status type for ContentManager operations
 */
struct Status {
    bool ok = true;
    std::string message;
    static Status OK() { return {}; }
    static Status Error(std::string msg) { return Status{false, std::move(msg)}; }
};

/**
 * @brief Content Manager
 * 
 * Universal content ingestion, storage, and retrieval system.
 * Handles all content types via pluggable processors.
 * 
 * Architecture:
 * 1. ContentTypeRegistry: MIME type → Category mapping
 * 2. ProcessorRegistry: Category → Processor mapping
 * 3. Storage: RocksDB (metadata + blobs), VectorIndex (embeddings), GraphIndex (relations)
 * 4. Unified API: ingestContent(), getContent(), searchContent(), etc.
 */
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
     * @brief Register a content processor
     * 
     * @param processor Processor instance (takes ownership)
     */
    void registerProcessor(std::unique_ptr<IContentProcessor> processor);

    /**
     * @brief Importiert bereits vorverarbeitete Inhalte (ohne Extraktion/Chunking/Embedding).
     * Erwartet ein strukturiertes JSON-Schema: { content: {...}, chunks: [...], edges?: [...] }.
     * Optional kann der Binärblob separat geliefert und gespeichert werden.
     * 
     * @param spec JSON-Objekt mit Content/Chunks/Edges
     * @param blob Optionaler Binärblob (wird unter content_blob:<id> gespeichert)
     * @return Status mit message; bei Erfolg ist message="ok"
     */
    // user_context: z.B. Benutzer-ID für kontextabhängige Verschlüsselung
    [[nodiscard]] Status importContent(const json& spec, const std::optional<std::string>& blob = std::nullopt, const std::string& user_context = "");

    /**
     * @brief Ingest raw blob with automatic processor selection and archive handling
     * 
     * High-level API that:
     * 1. Detects content type from blob/MIME type/filename
     * 2. For archives: Extracts contents, ingests each file, creates graph relationships
     * 3. For other types: Uses appropriate processor
     * 4. Returns content ID(s) of ingested content
     * 
     * @param blob Binary content data
     * @param filename Original filename (used for type detection)
     * @param mime_type Optional MIME type override
     * @param user_context User context for encryption/authorization
     * @param config Optional JSON configuration (archive strategy, etc.)
     * @return Status with message and JSON result containing content_id(s)
     */
    struct IngestResult {
        bool success = false;          ///< True when all required pipeline stages succeeded (CON-018)
        std::string error_message;
        std::string primary_content_id;  // Main content ID (archive or single file)
        std::vector<std::string> extracted_content_ids;  // IDs of extracted files (for archives)
        json metadata;  // Additional metadata about the ingestion

        /// Per-stage diagnostic record for observability and debugging.
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

    /**
     * @brief Ingest content from a stream with chunked processing for large files
     *
     * Reads the stream in configurable chunks to avoid full in-memory buffering.
     * Streaming-capable types (text/plain, text/csv, NDJSON, Markdown) are
     * processed incrementally, storing each chunk directly to RocksDB.
     * Other types are buffered up to `max_buffered_bytes` before processing.
     *
     * Supported config keys:
     *   - chunk_size_bytes  (size_t): read chunk size in bytes (default: 4 MB)
     *   - max_buffered_bytes (size_t): max buffer for non-streaming types (default: 256 MB)
     *   - chunk_size (int): text segment size in characters (default: 512)
     *   - chunk_overlap (int): not used in streaming path; reserved for future
     *
     * @param stream       Input stream positioned at the start of the content
     * @param filename     Original filename used for MIME type detection
     * @param mime_type    Optional MIME type override (empty = auto-detect)
     * @param user_context User context for encryption / authorization
     * @param config       Optional JSON configuration overrides
     * @return IngestResult with success flag and content_id on success
     */
    [[nodiscard]] IngestResult ingestStream(
        std::istream& stream,
        const std::string& filename,
        const std::string& mime_type = "",
        const std::string& user_context = "",
        const json& config = json::object()
    );

    /**
     * @brief Get content metadata
     * 
     * @param content_id Content UUID (with or without "content:" prefix)
     * @return ContentMeta if found
     */
    // Get content metadata; optional user_context can be provided for contextual decryption
    std::optional<ContentMeta> getContentMeta(const std::string& content_id, const std::string& user_context = "");

    /**
     * @brief Get content blob (original binary)
     * 
     * @param content_id Content UUID
     * @return Blob as string if found
     */
    std::optional<std::string> getContentBlob(const std::string& content_id, const std::string& user_context = "");

    /**
     * @brief Get all chunks for content (ordered by seq_num)
     * 
     * @param content_id Content UUID
     * @return Vector of ChunkMeta
     */
    std::vector<ChunkMeta> getContentChunks(const std::string& content_id);

    /**
     * @brief Get chunk metadata
     * 
     * @param chunk_id Chunk UUID
     * @return ChunkMeta if found
     */
    std::optional<ChunkMeta> getChunk(const std::string& chunk_id);

    /**
     * @brief Assemble complete content from chunks
     * 
     * Efficiently reconstructs content from stored chunks.
     * 
     * @param content_id Content UUID
     * @param include_text If true, concatenate all chunk texts into assembled_text
     * @return ContentAssembly with metadata and chunks
     */
    std::optional<ContentAssembly> assembleContent(const std::string& content_id, bool include_text = false);

    /**
     * @brief Get next chunk in sequence
     * 
     * @param chunk_id Current chunk UUID
     * @return Next chunk (seq_num + 1) if exists
     */
    std::optional<ChunkMeta> getNextChunk(const std::string& chunk_id);

    /**
     * @brief Get previous chunk in sequence
     * 
     * @param chunk_id Current chunk UUID
     * @return Previous chunk (seq_num - 1) if exists
     */
    std::optional<ChunkMeta> getPreviousChunk(const std::string& chunk_id);

    /**
     * @brief Get chunk range (pagination)
     * 
     * @param content_id Content UUID
     * @param start_seq Starting sequence number (inclusive)
     * @param count Number of chunks to retrieve
     * @return Vector of ChunkMeta in sequence order
     */
    std::vector<ChunkMeta> getChunkRange(const std::string& content_id, int start_seq, int count);

    /**
     * @brief Search content by semantic similarity
     * 
     * @param query_text Query text (will be embedded)
     * @param k Number of results
     * @param filters Optional filters (category, mime_type, tags)
     * @return Vector of (chunk_id, score) pairs
     */
    std::vector<std::pair<std::string, float>> searchContent(
        const std::string& query_text,
        int k,
        const json& filters = json::object()
    );

    /**
     * @brief Hybrid Search: Vector (HNSW) + Fulltext (BM25) + RRF
     * 
     * Combines vector similarity search and fulltext search using
     * Reciprocal Rank Fusion (RRF) for optimal ranking.
     * 
     * @param query_text Query text (used for both embedding and fulltext)
     * @param k Number of results to return
     * @param filters Optional filters (category, mime_type, tags, date_from, date_to)
     * @param vector_weight Weight for vector search (default 0.5)
     * @param fulltext_weight Weight for fulltext search (default 0.5)
     * @param rrf_k RRF constant (default 60)
     * @return Vector of (chunk_id, combined_score) pairs, sorted by score descending
     */
    std::vector<std::pair<std::string, float>> searchContentHybrid(
        const std::string& query_text,
        int k,
        const json& filters = json::object(),
        float vector_weight = 0.5f,
        float fulltext_weight = 0.5f,
        float rrf_k = 60.0f
    );

    /**
     * @brief Search with graph expansion (RAG-style)
     * 
     * Finds top-K chunks, then expands to neighbors (prev/next, siblings, parents).
     * 
     * @param query_text Query text
     * @param k Number of initial chunks
     * @param expansion_hops Graph expansion depth (1 = direct neighbors)
     * @param filters Optional filters
     * @return Vector of expanded chunk IDs with scores
     */
    std::vector<std::pair<std::string, float>> searchWithExpansion(
        const std::string& query_text,
        int k,
        int expansion_hops,
        const json& filters = json::object()
    );

    /**
     * @brief Delete content and all chunks (cascade)
     * 
     * @param content_id Content UUID
     * @return Status
     */
    [[nodiscard]] Status deleteContent(const std::string& content_id);

    /**
     * @brief Virtual Filesystem: Resolve path to content ID
     * 
     * @param virtual_path Path like "/documents/report.pdf"
     * @return Content ID if found, nullopt otherwise
     */
    std::optional<std::string> resolvePath(const std::string& virtual_path);

    /**
     * @brief Virtual Filesystem: List directory contents
     * 
     * @param virtual_path Directory path like "/documents"
     * @return Vector of ContentMeta for children (files and subdirectories)
     */
    std::vector<ContentMeta> listDirectory(const std::string& virtual_path);

    /**
     * @brief Virtual Filesystem: Create directory
     * 
     * @param virtual_path Directory path
     * @param recursive Create parent directories if needed
     * @return Status
     */
    [[nodiscard]] Status createDirectory(const std::string& virtual_path, bool recursive = false);

    /**
     * @brief Virtual Filesystem: Register path for existing content
     * 
     * @param content_id Existing content UUID
     * @param virtual_path Path to register
     * @return Status
     */
    [[nodiscard]] Status registerPath(const std::string& content_id, const std::string& virtual_path);

    /**
     * @brief Get processor for a category
     */
    IContentProcessor* getProcessor(ContentCategory category);

    /**
     * @brief Get statistics
     */
    struct Stats {
        int total_content_items = 0;   ///< CON-018
        int total_chunks = 0;          ///< CON-018
        int total_embeddings = 0;      ///< CON-018
        std::unordered_map<ContentCategory, int> items_by_category;
        int64_t total_storage_bytes = 0; ///< CON-018
    };
    Stats getStats();

    /// Metrics for Prometheus exposition
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

    const Metrics& getMetrics() const;

    /**
     * @brief Set malware filter for content scanning (Audit Compliance)
     * 
     * When set, all imported content will be scanned for malware before storage.
     * Threats exceeding the configured threshold will block the import.
     * 
     * Compliance:
     * - BSI C5 (OPS-12): Malware Protection
     * - ISO 27001 A.12.2.1: Controls against malware
     * - NIS2 Art. 21(2)(d): Supply chain security
     * 
     * @param malware_filter Malware filter manager instance
     */
    void setMalwareFilter(std::shared_ptr<themis::security::MalwareFilterManager> malware_filter);

    /**
     * @brief Get malware filter (for status/metrics)
     */
    std::shared_ptr<themis::security::MalwareFilterManager> getMalwareFilter() const;

    // =========================================================================
    // Embedding Pipeline
    // =========================================================================

    /**
     * @brief Attach an embedding pipeline to the content manager.
     *
     * When set and the pipeline is enabled (non-empty model_name), every
     * text chunk without a pre-computed embedding that is imported via
     * importContent() will automatically receive an embedding.
     *
     * @param pipeline  Shared pipeline instance (nullptr removes the pipeline).
     */
    void setEmbeddingPipeline(std::shared_ptr<EmbeddingPipeline> pipeline);

    /**
     * @brief Generate an embedding vector for arbitrary text.
     *
     * Delegates to the attached EmbeddingPipeline if present and enabled;
     * otherwise falls back to the processor's generateEmbedding() for the
     * TEXT category.  Returns an empty vector when no model is available.
     *
     * @param text        Input text (UTF-8).
     * @param model_name  Optional model name override (forwarded to the
     *                    pipeline configuration; ignored when the pipeline is
     *                    not attached).
     * @return Normalised embedding vector, or empty on failure.
     */
    std::vector<float> generateEmbedding(const std::string& text,
                                          const std::string& model_name = "");

    // =========================================================================
    // LLM-assisted content analysis
    // =========================================================================

    json analyzeContent(const std::string& content_id);

    std::vector<std::string> generateTags(
        const std::string& content_id,
        int max_tags = 10
    );

    std::string summarizeContent(
        const std::string& content_id,
        int max_words = 100
    );

    std::string classifyContent(const std::string& content_id);

    json extractEntities(const std::string& content_id);

    /**
     * @brief Attach a deduplication checker for near-duplicate detection.
     *
     * When set and `ContentPolicy::enable_deduplication` is true for a given
     * ingestion call, `ingestRawBlob()` will:
     *  - Compute a pHash for IMAGE content and call `isDuplicateImage()`.
     *  - Compute a MinHash for TEXT content and call `isDuplicateText()`.
     * Detected near-duplicates are rejected before storage and the existing
     * content ID is returned in `IngestResult::primary_content_id`.
     *
     * @param checker  DeduplicationChecker instance (nullptr disables dedup).
     */
    void setDeduplicationChecker(std::shared_ptr<DeduplicationChecker> checker);

    /**
     * @brief Get the attached deduplication checker (may be nullptr).
     */
    std::shared_ptr<DeduplicationChecker> getDeduplicationChecker() const;

    /**
     * @brief Set the processor chain configuration.
     *
     * Controls which processing stages (extraction, chunking, embedding,
     * deduplication) run for each content type during `ingestRawBlob()`.
     * Stages are matched by MIME type first, then by category, then by the
     * global default.  All stages are enabled by default, preserving
     * backward-compatible behaviour when no configuration is set.
     *
     * @param config  ProcessorChainConfig to apply.
     */
    void setProcessorChainConfig(const ProcessorChainConfig& config);

    /**
     * @brief Get the current processor chain configuration.
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

    /// MIME type detector used for OCR routing via ContentPolicy::ocrEnabled().
    /// Initialized once on construction (YAML config loaded from default path).
    /// Call mime_detector_.enableOcr(true/false) before shouldTriggerOcr() to
    /// apply the per-ingestion ContentPolicy::ocr_enabled flag.
    MimeDetector mime_detector_;
    
    // Processor registry (Category → Processor)
    std::unordered_map<ContentCategory, std::unique_ptr<IContentProcessor>> processors_;

    // Metrics instance (atomics) for Prometheus exposition
    mutable Metrics metrics_;

    // Helper methods
    std::string generateUuid();
    std::string normalizeId(const std::string& id, const std::string& prefix);
    std::string computeSHA256(const std::string& blob);
    std::optional<std::string> checkDuplicateByHash(const std::string& hash);
    
    void createChunkGraph(
        const std::vector<std::string>& chunk_ids,
        const std::string& content_id,
        const std::string& chunk_type
    );
    
    void createHierarchicalGraph(
        const std::string& parent_id,
        const std::vector<std::string>& child_ids,
        const std::string& edge_type
    );

    json parseAnalysisResult(const std::string& analysis_text, const ContentMeta& meta);
    std::vector<std::string> parseTags(const std::string& tags_text);
    json parseEntities(const std::string& entities_text);
    std::string getExtractedText(const std::string& content_id);
};

} // namespace content
} // namespace themis

