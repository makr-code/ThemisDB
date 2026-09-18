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

class IContentProcessor {
public:
    /**
     * @brief IContent Processor.
     * @return Return value.
     */
    virtual ~IContentProcessor() = default;
    
    [[nodiscard]] virtual ExtractionResult extract(
        const std::string& blob,
        const ContentType& content_type
    ) = 0;
    
    [[nodiscard]] virtual std::vector<json> chunk(
        const ExtractionResult& extraction_result,
        int chunk_size,
        int overlap
    ) = 0;
    
    [[nodiscard]] virtual std::vector<float> generateEmbedding(const std::string& chunk_data) = 0;
    
    [[nodiscard]] virtual std::string getName() const = 0;
    
    [[nodiscard]] virtual std::vector<ContentCategory> getSupportedCategories() const = 0;
};

class TextProcessor : public IContentProcessor {
public:
    ExtractionResult extract(const std::string& blob, const ContentType& content_type) override;
    std::vector<json> chunk(const ExtractionResult& extraction_result, int chunk_size, int overlap) override;
    std::vector<float> generateEmbedding(const std::string& chunk_data) override;
    std::string getName() const override { return "TextProcessor"; }
    std::vector<ContentCategory> getSupportedCategories() const override {
        return {ContentCategory::TEXT};
    }

    using EmbeddingFn = std::function<std::vector<float>(const std::string&)>;

    /**
     * @brief Set Embedding Backend.
     * @param[in] fn Input parameter.
     */
    void setEmbeddingBackend(EmbeddingFn fn);

    static std::vector<uint32_t> computeMinHash(
        const std::string& text,
        size_t num_hashes = 128
    );

private:
    /**
     * @brief Normalize Text.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    std::string normalizeText(const std::string& text);
    /**
     * @brief Count Tokens.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    int countTokens(const std::string& text); // Simple whitespace-based tokenizer
    /**
     * @brief Split Into Sentences.
     * @param[in] text Input parameter.
     * @return Return value.
     */
    std::vector<std::string> splitIntoSentences(const std::string& text);

    EmbeddingFn embedding_fn_;
};

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
     * @brief Extract EXIF.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    json extractEXIF(const std::string& blob);
    std::pair<int, int> getImageDimensions(const std::string& blob);
};
#endif

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
     * @brief Parse Geo JSON.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    ExtractionResult::GeoData parseGeoJSON(const std::string& blob);
    /**
     * @brief Parse GPX.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    ExtractionResult::GeoData parseGPX(const std::string& blob);
};

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
     * @brief Parse STEP.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    json parseSTEP(const std::string& blob);
    /**
     * @brief Extract Assembly Hierarchy.
     * @param[in] step_data Input parameter.
     * @return Return value.
     */
    json extractAssemblyHierarchy(const json& step_data);
};

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
     * @brief Extract ID3 Tags.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    json extractID3Tags(const std::string& blob);
    /**
     * @brief Get Duration Seconds.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    int getDurationSeconds(const std::string& blob);
};

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
     * @brief Parse CSV.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    std::vector<std::vector<std::string>> parseCSV(const std::string& blob);
    /**
     * @brief Extract Schema.
     * @param[in] rows Input parameter.
     * @return Return value.
     */
    json extractSchema(const std::vector<std::vector<std::string>>& rows);
};

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
     * @brief Compute Hash.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    std::string computeHash(const std::string& blob);
};

} // namespace content
} // namespace themis
