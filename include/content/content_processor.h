/**
 * @file content_processor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 82/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <nlohmann/json.hpp>
#include "content/content_type.h"

namespace themis {
namespace content {

using json = nlohmann::json;

/**
 * @brief Extraction Result
 * 
 * Result of extracting structured data from content.
 */
struct ExtractionResult {
    bool ok = false;
    std::string text;              // Extracted plain text (for TEXT types)
    json metadata;                 // Structured metadata (EXIF, ID3, CAD properties, etc.)
    std::vector<float> embedding;  // Optional: Pre-computed embedding
    std::string error_message;
    
    // Archive extraction fields
    bool success = false;          // Alias for ok (for backwards compatibility)
    std::vector<std::string> extracted_files;  // List of extracted file paths (for archives)
    std::string temp_directory;    // Temporary extraction directory (for archives)
    
    // Type-specific extracted data
    struct GeoData {
        std::vector<std::pair<double, double>> coordinates; // lat/lon pairs
        std::string projection;    // EPSG code or WKT
        json properties;           // GeoJSON properties
    };
    std::optional<GeoData> geo_data;
    
    struct MediaData {
        int duration_seconds = 0;  ///< Duration in seconds (CON-017)
        int width = 0;             ///< Frame width in pixels (CON-017)
        int height = 0;            ///< Frame height in pixels (CON-017)
        std::string codec;
        int bitrate = 0;           ///< Bit-rate in kbps (CON-017)
    };
    std::optional<MediaData> media_data;
    
    struct CADData {
        std::vector<std::string> part_ids;  // Assembly hierarchy
        json bom;                            // Bill of materials
        json dimensions;                     // Bounding box, volume, etc.
    };
    std::optional<CADData> cad_data;
};

/**
 * @brief Content Processor Interface
 * 
 * Abstract base class for content-type-specific processors.
 * Each processor handles extraction, chunking, and embedding for a category.
 */
class IContentProcessor {
public:
    /**
     * @brief TBD: Describe ~IContentProcessor.
     * @return Return value.
     */
    virtual ~IContentProcessor() = default;
    
    /**
     * @brief Extract structured data from blob
     * 
     * @param blob Binary content
     * @param content_type Content type info
     * @return Extracted data (text, metadata, embeddings)
     */
    [[nodiscard]] virtual ExtractionResult extract(
        const std::string& blob,
        const ContentType& content_type
    ) = 0;
    
    /**
     * @brief Chunk content for RAG/search
     * 
     * @param extraction_result Previously extracted data
     * @param chunk_size Target chunk size (tokens or other unit)
     * @param overlap Overlap between chunks
     * @return Vector of chunks with metadata
     */
    [[nodiscard]] virtual std::vector<json> chunk(
        const ExtractionResult& extraction_result,
        int chunk_size,
        int overlap
    ) = 0;
    
    /**
     * @brief Generate embedding for a chunk
     * 
     * @param chunk_data Chunk data (text or other representation)
     * @return Embedding vector
     */
    [[nodiscard]] virtual std::vector<float> generateEmbedding(const std::string& chunk_data) = 0;
    
    /**
     * @brief Get processor name
     */
    [[nodiscard]] virtual std::string getName() const = 0;
    
    /**
     * @brief Get supported categories
     */
    [[nodiscard]] virtual std::vector<ContentCategory> getSupportedCategories() const = 0;
};

/**
 * @brief Text Content Processor
 * 
 * Handles text documents, code, JSON, XML, Markdown, etc.
 */
class TextProcessor : public IContentProcessor {
public:
    ExtractionResult extract(const std::string& blob, const ContentType& content_type) override;
    std::vector<json> chunk(const ExtractionResult& extraction_result, int chunk_size, int overlap) override;
    std::vector<float> generateEmbedding(const std::string& chunk_data) override;
    std::string getName() const override { return "TextProcessor"; }
    std::vector<ContentCategory> getSupportedCategories() const override {
        return {ContentCategory::TEXT};
    }

    /**
     * @brief Callback type for a real embedding backend.
     *
     * Receives the chunk text and returns the embedding vector (e.g.
     * all-mpnet-base-v2 / ONNXClipPlugin / Sentence-BERT).  The returned
     * vector must be L2-normalised and non-empty.
     */
    using EmbeddingFn = std::function<std::vector<float>(const std::string&)>;

    /**
     * @brief Inject a real semantic embedding backend.
     *
     * When set, `generateEmbedding()` delegates to @p fn instead of the
     * built-in hash-projection fallback.  Pass `nullptr` to revert to the
     * hash-projection path.
     *
     * Roadmap ref: src/content/ROADMAP.md §Phase 5; src/content/FUTURE_ENHANCEMENTS.md
     * @param[in] fn Input parameter.
     */
    void setEmbeddingBackend(EmbeddingFn fn);

    /**
     * @brief Compute a MinHash signature for near-duplicate text detection.
     *
     * Uses `num_hashes` independent hash functions over 3-word shingles
     * (trigrams) of the input text.  The resulting signature can be used with
     * band-LSH (16 bands × 8 rows) to find documents whose estimated Jaccard
     * similarity exceeds 0.85.
     *
     * @param text       Input text (UTF-8).
     * @param num_hashes Number of MinHash permutations (default: 128).
     * @return Vector of `num_hashes` uint32_t values; all UINT32_MAX if text
     *         is empty.
     */
    static std::vector<uint32_t> computeMinHash(
        const std::string& text,
        size_t num_hashes = 128
    );

private:
    /**
     * @brief TBD: Describe normalizeText.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    std::string normalizeText(const std::string& text);
    /**
     * @brief TBD: Describe countTokens.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    int countTokens(const std::string& text); // Simple whitespace-based tokenizer
    /**
     * @brief TBD: Describe splitIntoSentences.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    std::vector<std::string> splitIntoSentences(const std::string& text);

    /// Injected real embedding backend (null → hash-projection fallback).
    EmbeddingFn embedding_fn_;
};

/**
 * @brief Image Content Processor
 * 
 * Handles photos, diagrams, screenshots.
 * Extracts EXIF metadata, generates image embeddings (e.g., CLIP).
 */
#ifndef THEMIS_CONTENT_PLUGIN_IMAGE_PROCESSOR_DEFINED
class LegacyImageProcessor : public IContentProcessor {
public:
    ExtractionResult extract(const std::string& blob, const ContentType& content_type) override;
    std::vector<json> chunk(const ExtractionResult& extraction_result, int chunk_size, int overlap) override;
    std::vector<float> generateEmbedding(const std::string& chunk_data) override;
    std::string getName() const override { return "LegacyImageProcessor"; }
    std::vector<ContentCategory> getSupportedCategories() const override {
        return {ContentCategory::IMAGE};
    }

private:
    /**
     * @brief TBD: Describe extractEXIF.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    json extractEXIF(const std::string& blob);
    std::pair<int, int> getImageDimensions(const std::string& blob);
};
#endif

/**
 * @brief Geo Content Processor
 * 
 * Handles GeoJSON, GPX, Shapefiles, GeoTIFF.
 * Extracts coordinates, creates spatial indices.
 */
class LegacyGeoProcessor : public IContentProcessor {
public:
    ExtractionResult extract(const std::string& blob, const ContentType& content_type) override;
    std::vector<json> chunk(const ExtractionResult& extraction_result, int chunk_size, int overlap) override;
    std::vector<float> generateEmbedding(const std::string& chunk_data) override;
    std::string getName() const override { return "LegacyGeoProcessor"; }
    std::vector<ContentCategory> getSupportedCategories() const override {
        return {ContentCategory::GEO};
    }

private:
    /**
     * @brief TBD: Describe parseGeoJSON.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    ExtractionResult::GeoData parseGeoJSON(const std::string& blob);
    /**
     * @brief TBD: Describe parseGPX.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    ExtractionResult::GeoData parseGPX(const std::string& blob);
};

/**
 * @brief CAD Content Processor
 * 
 * Handles STEP, IGES, STL, DXF.
 * Extracts geometry, assemblies, bill of materials.
 */
class LegacyCADProcessor : public IContentProcessor {
public:
    ExtractionResult extract(const std::string& blob, const ContentType& content_type) override;
    std::vector<json> chunk(const ExtractionResult& extraction_result, int chunk_size, int overlap) override;
    std::vector<float> generateEmbedding(const std::string& chunk_data) override;
    std::string getName() const override { return "LegacyCADProcessor"; }
    std::vector<ContentCategory> getSupportedCategories() const override {
        return {ContentCategory::CAD};
    }

private:
    /**
     * @brief TBD: Describe parseSTEP.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    json parseSTEP(const std::string& blob);
    /**
     * @brief TBD: Describe extractAssemblyHierarchy.
     * @param[in] step_data Input parameter.
     * @return Return value.
     */
    json extractAssemblyHierarchy(const json& step_data);
};

/**
 * @brief Audio Content Processor
 * 
 * Handles MP3, WAV, FLAC.
 * Extracts ID3 tags, transcribes speech (optional), generates audio embeddings.
 */
class LegacyAudioProcessor : public IContentProcessor {
public:
    ExtractionResult extract(const std::string& blob, const ContentType& content_type) override;
    std::vector<json> chunk(const ExtractionResult& extraction_result, int chunk_size, int overlap) override;
    std::vector<float> generateEmbedding(const std::string& chunk_data) override;
    std::string getName() const override { return "LegacyAudioProcessor"; }
    std::vector<ContentCategory> getSupportedCategories() const override {
        return {ContentCategory::AUDIO};
    }

private:
    /**
     * @brief TBD: Describe extractID3Tags.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    json extractID3Tags(const std::string& blob);
    /**
     * @brief TBD: Describe getDurationSeconds.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    int getDurationSeconds(const std::string& blob);
};

/**
 * @brief Structured Data Processor
 * 
 * Handles CSV, Parquet, Arrow tables.
 * Creates row-level chunks, column embeddings.
 */
class StructuredProcessor : public IContentProcessor {
public:
    ExtractionResult extract(const std::string& blob, const ContentType& content_type) override;
    std::vector<json> chunk(const ExtractionResult& extraction_result, int chunk_size, int overlap) override;
    std::vector<float> generateEmbedding(const std::string& chunk_data) override;
    std::string getName() const override { return "StructuredProcessor"; }
    std::vector<ContentCategory> getSupportedCategories() const override {
        return {ContentCategory::STRUCTURED};
    }

private:
    /**
     * @brief TBD: Describe parseCSV.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    std::vector<std::vector<std::string>> parseCSV(const std::string& blob);
    /**
     * @brief TBD: Describe extractSchema.
     * @param[in] rows Input parameter.
     * @return Return value.
     */
    json extractSchema(const std::vector<std::vector<std::string>>& rows);
};

/**
 * @brief Generic Binary Processor (Fallback)
 * 
 * Handles unknown binary types.
 * Stores blob, extracts minimal metadata (size, hash).
 */
class BinaryProcessor : public IContentProcessor {
public:
    ExtractionResult extract(const std::string& blob, const ContentType& content_type) override;
    std::vector<json> chunk(const ExtractionResult& extraction_result, int chunk_size, int overlap) override;
    std::vector<float> generateEmbedding(const std::string& chunk_data) override;
    std::string getName() const override { return "BinaryProcessor"; }
    std::vector<ContentCategory> getSupportedCategories() const override {
        return {ContentCategory::BINARY, ContentCategory::ARCHIVE, ContentCategory::UNKNOWN};
    }

private:
    /**
     * @brief TBD: Describe computeHash.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    std::string computeHash(const std::string& blob);
};

} // namespace content
} // namespace themis
