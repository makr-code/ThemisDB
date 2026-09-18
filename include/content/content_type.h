/**
 * @file content_type.h
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
#include <functional>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis {
namespace content {

using json = nlohmann::json;

enum class ContentCategory {
    TEXT,        // Documents, code, JSON, XML, etc.
    IMAGE,       // Photos, diagrams, screenshots
    AUDIO,       // Music, speech, podcasts
    VIDEO,       // Movies, tutorials, recordings
    GEO,         // GIS data, maps, GPS tracks
    CAD,         // 3D models, technical drawings
    ARCHIVE,     // ZIP, TAR, etc.
    STRUCTURED,  // CSV, Parquet, Arrow tables
    BINARY,      // Generic binary data
    UNKNOWN
};

struct ContentType {
    std::string mime_type;           // IANA MIME type (e.g., "text/plain")
    ContentCategory category;         // High-level category
    std::vector<std::string> extensions; // File extensions (e.g., [".txt", ".md"])
    bool supports_text_extraction;    // Can extract searchable text
    bool supports_embedding;          // Can generate embeddings
    bool supports_chunking;           // Should be chunked for RAG
    bool supports_metadata_extraction; // Can extract structured metadata
    bool binary_storage_required;     // Needs blob storage vs. can be stored as text
    
    // Feature flags for advanced processing
    struct Features {
        bool geospatial = false;      // Has lat/lon coordinates
        bool temporal = false;        // Has timestamps/duration
        bool hierarchical = false;    // Tree structure (e.g., CAD assemblies)
        bool versioned = false;       // Supports version history
        bool multimodal = false;      // Multiple data types (e.g., video = audio + images)
    } features{};
    
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
    static ContentType fromJson(const json& j);
};

class ContentTypeRegistry {
public:
    /**
     * @brief Instance.
     * @return Return value.
     */
    static ContentTypeRegistry& instance();
    
    /**
     * @brief Register Type.
     * @param[in] type Input parameter.
     */
    void registerType(const ContentType& type);
    
    /**
     * @brief Get By Mime Type.
     * @param[in] mime_type Input parameter.
     * @return Return value.
     */
    std::optional<ContentType> getByMimeType(const std::string& mime_type) const;
    
    /**
     * @brief Get By Extension.
     * @param[in] extension Input parameter.
     * @return Return value.
     */
    std::optional<ContentType> getByExtension(const std::string& extension) const;
    
    /**
     * @brief Detect From Blob.
     * @param[in] blob Input parameter.
     * @return Return value.
     */
    std::optional<ContentType> detectFromBlob(const std::string& blob) const;
    
    /**
     * @brief Get By Category.
     * @param[in] category Input parameter.
     * @return Return value.
     */
    std::vector<const ContentType*> getByCategory(ContentCategory category) const;
    
    /**
     * @brief Get All Types.
     * @return Return value.
     */
    std::vector<const ContentType*> getAllTypes() const;

private:
    ContentTypeRegistry();
    /**
     * @brief Register Default Types.
     */
    void registerDefaultTypes();
    
    std::vector<ContentType> types_;
};

/**
 * @brief Initialize Default Content Types.
 */
void initializeDefaultContentTypes();

} // namespace content
} // namespace themis
