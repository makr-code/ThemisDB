/**
 * @file content_plugin_interface.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <nlohmann/json.hpp>

// Export macros
#ifdef _WIN32
    #ifdef THEMIS_PLUGIN_EXPORTS
        #define THEMIS_PLUGIN_API __declspec(dllexport)
    #else
        #define THEMIS_PLUGIN_API __declspec(dllimport)
    #endif
#else
    #define THEMIS_PLUGIN_API __attribute__((visibility("default")))
#endif

// Plugin API version - must match between core and plugins
#define THEMIS_PLUGIN_API_VERSION "1.0.0"

namespace themis {
namespace content {

using json = nlohmann::json;

// ============================================================================
// Data Structures
// ============================================================================

/**
 * @brief Plugin Metadata
 */
struct PluginInfo {
    std::string name;
    std::string version;
    std::string description;
    std::string author;
    std::string license;
    std::vector<std::string> mime_types;
    std::vector<std::string> extensions;
    
    // Capabilities
    bool supports_chunking = true;
    bool supports_embedding = false;
    bool supports_streaming = false;
    
    // Resource requirements
    size_t min_memory_mb = 64;
    size_t recommended_memory_mb = 256;
};

/**
 * @brief Geo-specific extraction data
 */
struct GeoExtractionData {
    std::vector<std::pair<double, double>> coordinates;  // lat, lon pairs
    std::string crs;                                      // e.g., "EPSG:4326"
    json properties;
    std::array<double, 4> bounds = {};                    ///< minX, minY, maxX, maxY (zero-initialised; CON-023)
    std::string geometry_type;                            // Point, LineString, Polygon, etc.
};

/**
 * @brief Media-specific extraction data (video/audio)
 */
struct MediaExtractionData {
    int64_t duration_ms = 0;
    int width = 0;
    int height = 0;
    std::string video_codec;
    std::string audio_codec;
    int bitrate_kbps = 0;
    double framerate = 0.0;
    int sample_rate = 0;
    int channels = 0;
    std::string container_format;
    
    // Extended fields for video processing
    std::vector<int64_t> keyframe_timestamps;  // Keyframe positions in ms
    std::vector<int64_t> scene_boundaries;     // Scene change positions in ms
    std::string subtitles;                     // Extracted subtitle text
};

/**
 * @brief CAD-specific extraction data
 */
struct CADExtractionData {
    std::vector<std::string> part_ids;
    json assembly_tree;
    json bill_of_materials;
    std::array<double, 3> bounding_box_min = {};  ///< AABB minimum corner (CON-024)
    std::array<double, 3> bounding_box_max = {};  ///< AABB maximum corner (CON-024)
    double volume = 0.0;
    double surface_area = 0.0;
    int part_count = 0;
};

/**
 * @brief Extraction Result from Content Processor
 */
struct ContentExtractionResult {
    bool success = false;
    std::string error_message;
    
    // Extracted content
    std::string text;                          // Extracted plain text
    json metadata;                             // Structured metadata
    std::vector<uint8_t> thumbnail;            // Optional thumbnail (PNG/JPEG)
    std::string thumbnail_mime_type;           // e.g., "image/png"
    
    // Type-specific data
    std::optional<GeoExtractionData> geo;
    std::optional<MediaExtractionData> media;
    std::optional<CADExtractionData> cad;
    
    // Processing statistics
    int64_t processing_time_ms = 0;
    size_t input_size_bytes = 0;
    size_t output_size_bytes = 0;
};

/**
 * @brief Content Chunk for RAG/Search
 */
struct ContentChunk {
    std::string text;
    int sequence = 0;
    int start_offset = 0;
    int end_offset = 0;
    int token_count = 0;
    json metadata;                             // Page number, section, etc.
    std::vector<float> embedding;              // Optional pre-computed embedding
};

/**
 * @brief Plugin Configuration (loaded from YAML)
 */
class PluginConfig {
public:
    PluginConfig() = default;
    explicit PluginConfig(const json& settings) : settings_(settings) {}
    
    /**
     * @brief Get configuration value with default
     */
    template<typename T>
    T get(const std::string& path, T default_value) const {
        try {
            json::json_pointer ptr("/" + path);
            // Replace dots with slashes for JSON pointer
            std::string fixed_path = path;
            std::replace(fixed_path.begin(), fixed_path.end(), '.', '/');
            ptr = json::json_pointer("/" + fixed_path);
            
            if (settings_.contains(ptr)) {
                return settings_.at(ptr).get<T>();
            }
        } catch (...) {}
        return default_value;
    }
    
    /**
     * @brief Check if configuration path exists
     */
    bool has(const std::string& path) const {
        try {
            std::string fixed_path = path;
            std::replace(fixed_path.begin(), fixed_path.end(), '.', '/');
            json::json_pointer ptr("/" + fixed_path);
            return settings_.contains(ptr);
        } catch (...) {
            return false;
        }
    }
    
    /**
     * @brief Get raw JSON settings
     */
    const json& raw() const { return settings_; }
    
    /**
     * @brief Set configuration value
     */
    template<typename T>
    void set(const std::string& path, T value) {
        try {
            std::string fixed_path = path;
            std::replace(fixed_path.begin(), fixed_path.end(), '.', '/');
            json::json_pointer ptr("/" + fixed_path);
            settings_[ptr] = value;
        } catch (...) {}
    }
    
private:
    json settings_;
};

/**
 * @brief Extraction Options (per-request)
 */
struct ExtractionOptions {
    bool extract_text = true;
    bool extract_metadata = true;
    bool generate_thumbnail = false;
    bool generate_embedding = false;
    bool extract_keyframes = false;   // Extract keyframe timestamps
    bool extract_scenes = false;      // Detect scene boundaries
    bool extract_subtitles = true;    // Extract embedded subtitles
    
    // Chunking options
    int chunk_max_tokens = 512;
    int chunk_overlap = 50;
    std::string chunk_strategy = "paragraph";  // paragraph, sentence, page, fixed
    
    // Format-specific options
    json format_options;
    
    // Spatial filtering (for geospatial data)
    bool use_spatial_filter = false;
    double filter_minx = 0.0;
    double filter_miny = 0.0;
    double filter_maxx = 0.0;
    double filter_maxy = 0.0;
    
    // Timeout
    int timeout_seconds = 60;
};

// ============================================================================
// Plugin Interface
// ============================================================================

/**
 * @brief Content Processor Plugin Interface
 * 
 * All Content Processor plugins must implement this interface.
 * Plugins are loaded as dynamic libraries (DLL/SO/DYLIB).
 */
class IContentProcessorPlugin {
public:
    virtual ~IContentProcessorPlugin() = default;
    
    /**
     * @brief Get plugin information
     */
    [[nodiscard]] virtual PluginInfo getInfo() const = 0;
    
    /**
     * @brief Initialize plugin with configuration
     * 
     * Called once when plugin is loaded.
     * 
     * @param config Configuration from YAML
     * @return true if initialization successful
     */
    [[nodiscard]] virtual bool initialize(const PluginConfig& config) = 0;
    
    /**
     * @brief Shutdown plugin
     * 
     * Called when plugin is unloaded. Clean up resources.
     */
    virtual void shutdown() = 0;
    
    /**
     * @brief Check if plugin can process given MIME type
     * 
     * @param mime_type MIME type string (e.g., "application/pdf")
     * @return true if plugin can handle this type
     */
    [[nodiscard]] virtual bool canProcess(const std::string& mime_type) const = 0;
    
    /**
     * @brief Extract content from binary blob
     * 
     * Main extraction method. Extracts text, metadata, and other
     * content from the input blob.
     * 
     * @param blob Input binary data
     * @param mime_type MIME type of the content
     * @param options Extraction options
     * @return Extraction result with text, metadata, etc.
     */
    [[nodiscard]] virtual ContentExtractionResult extract(
        const std::vector<uint8_t>& blob,
        const std::string& mime_type,
        const ExtractionOptions& options = {}
    ) = 0;
    
    /**
     * @brief Chunk extracted content for RAG/search
     * 
     * Splits extracted content into smaller chunks suitable for
     * embedding and retrieval.
     * 
     * @param result Extraction result from extract()
     * @param max_tokens Maximum tokens per chunk
     * @param overlap Token overlap between chunks
     * @return Vector of content chunks
     */
    [[nodiscard]] virtual std::vector<ContentChunk> chunk(
        const ContentExtractionResult& result,
        int max_tokens,
        int overlap
    ) = 0;
    
    /**
     * @brief Generate embedding for text chunk (optional)
     * 
     * Default implementation returns empty vector.
     * Override if plugin provides embedding generation.
     * 
     * @param text Text to embed
     * @return Embedding vector
     */
    [[nodiscard]] virtual std::vector<float> generateEmbedding([[maybe_unused]] const std::string& text) {
        return {};
    }
    
    /**
     * @brief Health check
     * 
     * Verify that plugin is operational and all dependencies are available.
     * 
     * @return true if healthy
     */
    [[nodiscard]] virtual bool healthCheck() const = 0;
    
    /**
     * @brief Get plugin statistics
     * 
     * Returns statistics about plugin usage (documents processed, errors, etc.)
     */
    [[nodiscard]] virtual json getStatistics() const {
        return json::object();
    }
};

// ============================================================================
// Plugin Entry Points
// ============================================================================

/**
 * @brief Plugin creation function type
 */
using CreatePluginFunc = IContentProcessorPlugin* (*)();

/**
 * @brief Plugin destruction function type
 */
using DestroyPluginFunc = void (*)(IContentProcessorPlugin*);

/**
 * @brief Plugin version function type
 */
using GetVersionFunc = const char* (*)();

/**
 * @brief Plugin Entry Point Macro
 * 
 * Use this macro to export plugin entry points:
 * 
 * @code
 * class MyProcessor : public IContentProcessorPlugin {
 *     // ... implementation ...
 * };
 * 
 * THEMIS_CONTENT_PLUGIN(MyProcessor)
 * @endcode
 * 
 * NOTE: Only used when building standalone plugins. When building monolithic
 * themis_core, these functions are not exported to avoid duplicate symbols.
 */
#ifdef THEMIS_BUILD_STANDALONE_PLUGINS
#define THEMIS_CONTENT_PLUGIN(PluginClass) \
    extern "C" { \
        THEMIS_PLUGIN_API IContentProcessorPlugin* themis_create_plugin() { \
            return new PluginClass(); \
        } \
        THEMIS_PLUGIN_API void themis_destroy_plugin(IContentProcessorPlugin* plugin) { \
            delete plugin; \
        } \
        THEMIS_PLUGIN_API const char* themis_get_plugin_api_version() { \
            return THEMIS_PLUGIN_API_VERSION; \
        } \
    }
#else
#define THEMIS_CONTENT_PLUGIN(PluginClass) // No-op when building monolithic core
#endif

// ============================================================================
// Helper Functions
// ============================================================================

/**
 * @brief Simple token counter (whitespace-based)
 */
inline int countTokens(const std::string& text) {
    if (text.empty()) {
      return 0;
    }
    
    int count = 0;
    bool in_token = false;
    
    for (char c : text) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (in_token) {
                count++;
                in_token = false;
            }
        } else {
            in_token = true;
        }
    }
    
    if (in_token) {
      count++;
    }
    return count;
}

/**
 * @brief Split text into sentences
 */
inline std::vector<std::string> splitSentences(const std::string& text) {
    std::vector<std::string> sentences;
    std::string current;
    
    for (size_t i = 0; i < text.size(); ++i) {
        current += text[i];
        
        if (text[i] == '.' || text[i] == '!' || text[i] == '?') {
            // Check for abbreviations, etc.
            if (i + 1 < text.size() && std::isspace(static_cast<unsigned char>(text[i + 1]))) {
                // Trim and add
                while (!current.empty() && std::isspace(static_cast<unsigned char>(current.front()))) {
                    current.erase(0, 1);
                }
                if (!current.empty()) {
                    sentences.push_back(current);
                    current.clear();
                }
            }
        }
    }
    
    // Add remaining text
    while (!current.empty() && std::isspace(static_cast<unsigned char>(current.front()))) {
        current.erase(0, 1);
    }
    if (!current.empty()) {
        sentences.push_back(current);
    }
    
    return sentences;
}

} // namespace content
} // namespace themis
