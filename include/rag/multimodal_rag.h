/**
 * @file multimodal_rag.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "rag/rag_judge.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis::rag::multimodal {

// ─────────────────────────────────────────────────────────────────────────────
// Modality
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Data modality of a retrieved source.
 */
enum class Modality {
    TEXT,   ///< Textual passage from a document
    IMAGE,  ///< Image or chart retrieved via embedding similarity
    TABLE,  ///< Structured table extracted from a document
};

// ─────────────────────────────────────────────────────────────────────────────
// Document types
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief An image document returned by the image retrieval function.
 *
 * The caller is responsible for generating the embedding (e.g. with a CLIP
 * model) and storing it alongside the image path / URI.
 */
struct ImageDocument {
    std::string id;                    ///< Unique document identifier
    std::string image_path;            ///< File path or URI to the image
    std::string caption;               ///< Pre-computed caption (may be empty)
    std::vector<float> embedding;      ///< Pre-computed image embedding
    double relevance_score = 0.0;      ///< Retrieval similarity score [0, 1]
    std::unordered_map<std::string, std::string> metadata; ///< Arbitrary metadata
};

// ─────────────────────────────────────────────────────────────────────────────
// Query
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A multi-modal query combining text and optional image embedding.
 */
struct MultiModalQuery {
    /// Natural-language query string; used for text retrieval.
    std::string text;

    /// Optional pre-computed image embedding (e.g. CLIP).
    /// When non-empty and Modality::IMAGE is in @c modalities, the
    /// image retriever is invoked with this embedding.
    std::vector<float> image_embedding;

    /// Which modalities to retrieve.  Defaults to TEXT only.
    std::vector<Modality> modalities = {Modality::TEXT};

    /// Maximum number of results to request from each retrieval function.
    size_t top_k = 10;
};

// ─────────────────────────────────────────────────────────────────────────────
// Source item
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief A single multi-modal source item in the RAG result.
 *
 * Which fields are populated depends on @c modality:
 *  - TEXT:  @c content is populated.
 *  - IMAGE: @c image_path and @c caption are populated.
 *  - TABLE: @c table_data is populated.
 */
struct MultiModalSource {
    Modality    modality;              ///< Source modality
    std::string document_id;           ///< Identifier of the originating document
    double      relevance_score = 0.0; ///< Fused relevance score [0, 1]

    // TEXT / TABLE fields
    std::string content;    ///< Text passage content
    std::string table_data; ///< Serialised table (CSV / markdown)

    // IMAGE fields
    std::string image_path; ///< Path or URI to the image
    std::string caption;    ///< Natural-language caption for the image

    /// Pass-through metadata from the original document.
    std::unordered_map<std::string, std::string> metadata;
};

// ─────────────────────────────────────────────────────────────────────────────
// Result
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Result of a multi-modal RAG query.
 */
struct MultiModalRAGResult {
    /// Retrieved sources, sorted by relevance_score descending.
    std::vector<MultiModalSource> sources;

    /// Formatted LLM context string built from @c sources.
    /// Suitable for direct injection into an LLM prompt.
    std::string context;

    /// Wall-clock duration for the complete query (milliseconds).
    double elapsed_ms = 0.0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Configuration
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Configuration for MultiModalRAG.
 */
struct MultiModalRAGConfig {
    /// Enable image retrieval.  Requires an ImageRetrievalFn to be set.
    bool enable_image_retrieval = true;

    /// Enable table question-answering retrieval.
    /// Requires a table retrieval function (set via setTextRetriever with
    /// a table-aware backend) and TABLE in the query's modalities.
    bool enable_table_qa = false;

    /// Enable OCR-based document retrieval.
    /// When true, retrieved image sources may include OCR-extracted text
    /// appended to their caption by the caller-supplied ImageCaptionFn.
    bool enable_ocr = false;

    /// Contribution weight for text retrieval results in RRF fusion.
    double text_weight = 0.5;

    /// Contribution weight for image retrieval results in RRF fusion.
    double image_weight = 1.0;

    /// Default top-k per modality (overridden by MultiModalQuery::top_k).
    size_t top_k = 10;

    /// RRF smoothing constant (consistent with MultiModalSearch::Config).
    double rrf_k = 60.0;

    /// Maximum total sources to include in the result.
    size_t max_sources = 20;
};

// ─────────────────────────────────────────────────────────────────────────────
// Caller-supplied function types
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Caller-supplied text retrieval function.
 *
 * @param query  Natural-language query string.
 * @param top_k  Maximum number of documents to return.
 * @return       Retrieved text documents; may be empty.
 */
using TextRetrievalFn = std::function<
    std::vector<judge::RetrievedDocument>(const std::string& query, size_t top_k)>;

/**
 * @brief Caller-supplied image retrieval function.
 *
 * @param embedding  Pre-computed query embedding (e.g. CLIP).
 * @param top_k      Maximum number of images to return.
 * @return           Retrieved image documents; may be empty.
 */
using ImageRetrievalFn = std::function<
    std::vector<ImageDocument>(const std::vector<float>& embedding, size_t top_k)>;

/**
 * @brief Optional caller-supplied image caption generator.
 *
 * Called for each image source that has no pre-computed caption.
 *
 * @param img  The image document to caption.
 * @return     A natural-language caption for the image.
 */
using ImageCaptionFn = std::function<std::string(const ImageDocument& img)>;

// ─────────────────────────────────────────────────────────────────────────────
// MultiModalRAG
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Orchestrates multi-modal retrieval-augmented generation.
 *
 * Retrieves text and image documents using caller-supplied functions, fuses
 * results via RRF, generates captions for image sources, and builds a
 * formatted context string for downstream LLM consumption.
 *
 * Usage:
 * @code
 * MultiModalRAGConfig cfg;
 * cfg.enable_image_retrieval = true;
 *
 * MultiModalRAG mm_rag(cfg);
 * mm_rag.setTextRetriever([&](const std::string& q, size_t k) { ... });
 * mm_rag.setImageRetriever([&](const std::vector<float>& e, size_t k) { ... });
 * mm_rag.setImageCaptioner([&](const ImageDocument& img) { ... });
 *
 * auto result = mm_rag.query({"What is this chart showing?",
 *                             clip_embedding,
 *                             {Modality::TEXT, Modality::IMAGE}});
 * @endcode
 */
class MultiModalRAG {
public:
    /**
     * @brief Construct with default configuration.
     */
    MultiModalRAG();

    /**
     * @brief Construct with custom configuration.
     * @param config  Configuration for retrieval and fusion.
     */
    explicit MultiModalRAG(const MultiModalRAGConfig& config);

    /**
     * @brief Destructor.
     */
    ~MultiModalRAG();

    // Non-copyable, movable
    MultiModalRAG(const MultiModalRAG&)            = delete;
    MultiModalRAG& operator=(const MultiModalRAG&) = delete;
    MultiModalRAG(MultiModalRAG&&)                 noexcept = default;
    MultiModalRAG& operator=(MultiModalRAG&&)      noexcept = default;

    // ── Retrieval backend configuration ──────────────────────────────────────

    /**
     * @brief Set the text retrieval function.
     * @param fn  Callable that retrieves text documents for a query.
     */
    void setTextRetriever(TextRetrievalFn fn);

    /**
     * @brief Set the image retrieval function.
     * @param fn  Callable that retrieves image documents for an embedding.
     */
    void setImageRetriever(ImageRetrievalFn fn);

    /**
     * @brief Set the optional image caption generator.
     *
     * When set, images without a pre-computed caption have their caption
     * populated by this function before the result is returned.
     *
     * @param fn  Callable that generates a caption for an ImageDocument.
     */
    void setImageCaptioner(ImageCaptionFn fn);

    // ── Query ─────────────────────────────────────────────────────────────────

    /**
     * @brief Execute a multi-modal RAG query.
     *
     * Retrieves from all enabled modalities specified in @p query.modalities,
     * fuses results via RRF, and builds a formatted LLM context string.
     *
     * @param query  Multi-modal query with text and/or image embedding.
     * @return       Result containing mixed-modality sources and LLM context.
     */
    MultiModalRAGResult query(const MultiModalQuery& query) const;

    // ── Context building (exposed for testing) ────────────────────────────────

    /**
     * @brief Build an LLM-ready context string from a list of sources.
     *
     * Format:
     * @code
     * Text passages:
     * [1] (score=0.92) <content>
     * [2] (score=0.85) <content>
     *
     * Image captions:
     * [1] (score=0.87) <caption>
     *
     * Question: <question>
     * Answer:
     * @endcode
     *
     * @param sources   Mixed-modality sources.
     * @param question  The original query text.
     * @return          Formatted context string.
     */
    std::string buildContext(const std::vector<MultiModalSource>& sources,
                             const std::string& question) const;

    // ── Configuration ─────────────────────────────────────────────────────────

    /**
     * @brief Return a copy of the current configuration.
     */
    MultiModalRAGConfig getConfig() const;

    /**
     * @brief Replace the current configuration.
     * @param config  New configuration.
     */
    void setConfig(const MultiModalRAGConfig& config);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Factory
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Convenience factory for common MultiModalRAG configurations.
 */
class MultiModalRAGFactory {
public:
    /**
     * @brief Text-only retriever: image retrieval disabled.
     *
     * Suitable as a drop-in for standard RAG pipelines that need
     * multi-modal support in the future.
     */
    static std::unique_ptr<MultiModalRAG> createTextOnly();

    /**
     * @brief Text + image retriever with equal modality weights.
     *
     * Suitable for document corpora that mix text passages and images.
     */
    static std::unique_ptr<MultiModalRAG> createTextAndImage();

    /**
     * @brief Full multi-modal retriever (text + image, OCR and table-QA ready).
     *
     * All flags enabled; callers must supply the appropriate retrieval
     * functions before calling query().
     */
    static std::unique_ptr<MultiModalRAG> createFull();
};

} // namespace themis::rag::multimodal
