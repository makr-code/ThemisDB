# ThemisDB Content Processor Plugin Architecture

**Version:** 2.0  
**Status:** Production-Ready  
**Autor:** ThemisDB Team  
**Datum:** Dezember 2025

---

## 1. Übersicht

ThemisDB verwendet ein Plugin-basiertes System für Content-Verarbeitung. Alle Prozessoren werden als dynamische Bibliotheken (DLL/SO) bereitgestellt und über YAML-Konfiguration gesteuert.

### 1.1 Architektur

```
ThemisDB Core
    ↓
Content Processor Registry
    ↓
Plugin Loader (YAML Config)
    ↓
Plugins (DLL/SO):
    ├── themis_proc_pdf.dll      (PDF Extraction)
    ├── themis_proc_office.dll   (DOCX, XLSX, PPTX)
    ├── themis_proc_geo.dll      (GeoJSON, GPX, Shapefile)
    ├── themis_proc_video.dll    (MP4, MKV, AVI)
    ├── themis_proc_audio.dll    (MP3, WAV, FLAC)
    ├── themis_proc_image.dll    (JPEG, PNG, TIFF)
    ├── themis_proc_cad.dll      (STEP, IGES, STL)
    └── themis_proc_text.dll     (TXT, JSON, XML, MD)
```

### 1.2 Plugin-Verzeichnisse

| Platform | Pfad |
|----------|------|
| Windows | `C:/Program Files/ThemisDB/plugins/processors` |
| Linux | `/usr/local/lib/themis/plugins/processors` |
| macOS | `/usr/local/lib/themis/plugins/processors` |
| Relativ | `./plugins/processors` |

---

## 2. YAML-Konfiguration

### 2.1 Hauptkonfiguration

```yaml
# /etc/themis/content_processors.yaml

content_processors:
  # Globale Einstellungen
  enabled: true
  plugin_directory: "/usr/local/lib/themis/plugins/processors"
  auto_load: true
  signature_verification: true
  sandbox_enabled: true
  
  # Ressourcenlimits (pro Prozess)
  limits:
    max_memory_mb: 512
    max_cpu_time_sec: 60
    max_file_size_mb: 100
    max_concurrent: 4
  
  # Logging
  logging:
    level: info
    include_timing: true
    audit_enabled: true
  
  # Aktivierte Prozessoren
  processors:
    - name: pdf
      enabled: true
      config_file: "processors/pdf.yaml"
    
    - name: office
      enabled: true
      config_file: "processors/office.yaml"
    
    - name: geo
      enabled: true
      config_file: "processors/geo.yaml"
    
    - name: video
      enabled: true
      config_file: "processors/video.yaml"
    
    - name: audio
      enabled: true
      config_file: "processors/audio.yaml"
    
    - name: image
      enabled: true
      config_file: "processors/image.yaml"
    
    - name: cad
      enabled: false  # Optional, benötigt OpenCASCADE
      config_file: "processors/cad.yaml"
    
    - name: text
      enabled: true
      config_file: "processors/text.yaml"
```

### 2.2 PDF Processor Konfiguration

```yaml
# /etc/themis/processors/pdf.yaml

processor:
  name: "PDF Processor"
  version: "1.0.0"
  library: "themis_proc_pdf"
  
  # Unterstützte MIME-Types
  mime_types:
    - "application/pdf"
    - "application/x-pdf"
  
  # Unterstützte Dateiendungen
  extensions:
    - ".pdf"
  
  # Extraction-Einstellungen
  extraction:
    text: true
    metadata: true
    images: false       # Embedded Images extrahieren
    fonts: false        # Font-Informationen
    annotations: true   # PDF-Annotationen
    form_fields: true   # Formularfelder
    bookmarks: true     # Lesezeichen/Outline
    links: true         # Hyperlinks
  
  # OCR-Einstellungen (für gescannte PDFs)
  ocr:
    enabled: false      # Benötigt Tesseract
    language: "deu+eng"
    dpi: 300
    timeout_sec: 120
  
  # Chunking für RAG
  chunking:
    strategy: "page"    # page, paragraph, sentence, fixed
    max_tokens: 512
    overlap_tokens: 50
    preserve_structure: true
  
  # Sicherheit
  security:
    allow_encrypted: true
    max_pages: 1000
    javascript_enabled: false
    external_links_allowed: false
  
  # Backend-Bibliothek
  backend:
    library: "poppler"  # poppler, pdfium, mupdf
    version_min: "22.0"
```

### 2.3 Office Processor Konfiguration

```yaml
# /etc/themis/processors/office.yaml

processor:
  name: "Office Processor"
  version: "1.0.0"
  library: "themis_proc_office"
  
  mime_types:
    # OOXML (Office 2007+)
    - "application/vnd.openxmlformats-officedocument.wordprocessingml.document"
    - "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"
    - "application/vnd.openxmlformats-officedocument.presentationml.presentation"
    # ODF
    - "application/vnd.oasis.opendocument.text"
    - "application/vnd.oasis.opendocument.spreadsheet"
    - "application/vnd.oasis.opendocument.presentation"
    # Legacy
    - "application/msword"
    - "application/vnd.ms-excel"
    - "application/vnd.ms-powerpoint"
    # RTF
    - "application/rtf"
  
  extensions:
    - ".docx"
    - ".xlsx"
    - ".pptx"
    - ".odt"
    - ".ods"
    - ".odp"
    - ".doc"
    - ".xls"
    - ".ppt"
    - ".rtf"
  
  extraction:
    text: true
    metadata: true
    comments: true
    track_changes: true
    headers_footers: true
    embedded_objects: false
    
    # Excel-spezifisch
    excel:
      formulas: true
      named_ranges: true
      charts_metadata: true
      max_rows: 100000
      max_columns: 1000
    
    # PowerPoint-spezifisch
    powerpoint:
      speaker_notes: true
      slide_layout: true
      animations_metadata: true
  
  chunking:
    strategy: "paragraph"
    max_tokens: 512
    overlap_tokens: 50
    
  security:
    allow_macros: false
    allow_encrypted: true
    scan_embedded: true
```

### 2.4 Geo/GIS Processor Konfiguration

```yaml
# /etc/themis/processors/geo.yaml

processor:
  name: "Geo/GIS Processor"
  version: "1.0.0"
  library: "themis_proc_geo"
  
  mime_types:
    - "application/geo+json"
    - "application/vnd.google-earth.kml+xml"
    - "application/vnd.google-earth.kmz"
    - "application/gpx+xml"
    - "application/x-shapefile"
    - "image/tiff"  # GeoTIFF
    - "application/geopackage+sqlite3"
  
  extensions:
    - ".geojson"
    - ".json"
    - ".kml"
    - ".kmz"
    - ".gpx"
    - ".shp"
    - ".shx"
    - ".dbf"
    - ".prj"
    - ".tif"
    - ".tiff"
    - ".gpkg"
  
  extraction:
    coordinates: true
    properties: true
    crs: true           # Coordinate Reference System
    bounds: true        # Bounding Box
    topology: true      # Topologische Beziehungen
    
    # Format-spezifisch
    geojson:
      validate_schema: true
      simplify_tolerance: 0.0001
    
    shapefile:
      encoding: "utf-8"
      include_dbf: true
    
    gpx:
      tracks: true
      waypoints: true
      routes: true
      timestamps: true
    
    geotiff:
      extract_bands: false
      compute_statistics: true
  
  # Spatial Indexing
  indexing:
    enabled: true
    index_type: "rtree"  # rtree, quadtree, h3
    h3_resolution: 9
  
  # Koordinatentransformation
  transformation:
    default_crs: "EPSG:4326"
    allow_reprojection: true
    
  backend:
    library: "gdal"
    version_min: "3.0"
```

### 2.5 Video Processor Konfiguration

```yaml
# /etc/themis/processors/video.yaml

processor:
  name: "Video Processor"
  version: "1.0.0"
  library: "themis_proc_video"
  
  mime_types:
    - "video/mp4"
    - "video/x-matroska"
    - "video/webm"
    - "video/x-msvideo"
    - "video/quicktime"
    - "video/x-ms-wmv"
    - "video/mpeg"
  
  extensions:
    - ".mp4"
    - ".mkv"
    - ".webm"
    - ".avi"
    - ".mov"
    - ".wmv"
    - ".mpg"
    - ".mpeg"
  
  extraction:
    metadata: true
    thumbnail: true
    keyframes: false
    subtitles: true
    audio_tracks: true
    
    # Metadata-Details
    metadata_fields:
      - duration
      - resolution
      - codec
      - bitrate
      - framerate
      - aspect_ratio
      - creation_date
      - gps_location
    
    # Thumbnail-Generierung
    thumbnails:
      count: 1
      position: "middle"  # start, middle, end, percentage
      size: "320x240"
      format: "jpeg"
      quality: 85
    
    # Keyframe-Extraction
    keyframes:
      enabled: false
      interval_sec: 30
      max_count: 10
  
  # Transcription (optional)
  transcription:
    enabled: false
    engine: "whisper"
    language: "auto"
    model: "base"
  
  # Scene Detection (optional)
  scene_detection:
    enabled: false
    threshold: 30
    min_scene_length_sec: 2
  
  backend:
    library: "ffmpeg"
    version_min: "5.0"
    hardware_accel: "auto"  # auto, cuda, vaapi, none
```

### 2.6 Audio Processor Konfiguration

```yaml
# /etc/themis/processors/audio.yaml

processor:
  name: "Audio Processor"
  version: "1.0.0"
  library: "themis_proc_audio"
  
  mime_types:
    - "audio/mpeg"
    - "audio/mp3"
    - "audio/wav"
    - "audio/x-wav"
    - "audio/flac"
    - "audio/x-flac"
    - "audio/aac"
    - "audio/ogg"
    - "audio/opus"
    - "audio/x-m4a"
  
  extensions:
    - ".mp3"
    - ".wav"
    - ".flac"
    - ".aac"
    - ".ogg"
    - ".opus"
    - ".m4a"
    - ".wma"
  
  extraction:
    metadata: true
    waveform: false
    spectrum: false
    
    # ID3/Metadata Tags
    tags:
      - title
      - artist
      - album
      - year
      - genre
      - track_number
      - duration
      - bitrate
      - sample_rate
      - channels
      - cover_art
    
    # Waveform-Generierung
    waveform:
      enabled: false
      samples: 1000
      format: "json"  # json, png
    
    # Spektralanalyse
    spectrum:
      enabled: false
      type: "mel"  # mel, linear, bark
      n_mels: 128
  
  # Speech-to-Text (optional)
  transcription:
    enabled: false
    engine: "whisper"
    language: "auto"
    model: "base"
    timestamps: true
    speaker_diarization: false
  
  # Music Analysis (optional)
  music_analysis:
    enabled: false
    bpm_detection: true
    key_detection: true
    mood_classification: false
  
  backend:
    library: "ffmpeg"
    version_min: "5.0"
```

### 2.7 Image Processor Konfiguration

```yaml
# /etc/themis/processors/image.yaml

processor:
  name: "Image Processor"
  version: "1.0.0"
  library: "themis_proc_image"
  
  mime_types:
    - "image/jpeg"
    - "image/png"
    - "image/gif"
    - "image/webp"
    - "image/tiff"
    - "image/bmp"
    - "image/svg+xml"
    - "image/heic"
    - "image/heif"
    - "image/avif"
  
  extensions:
    - ".jpg"
    - ".jpeg"
    - ".png"
    - ".gif"
    - ".webp"
    - ".tif"
    - ".tiff"
    - ".bmp"
    - ".svg"
    - ".heic"
    - ".heif"
    - ".avif"
  
  extraction:
    metadata: true
    exif: true
    iptc: true
    xmp: true
    icc_profile: true
    thumbnail: true
    dominant_colors: true
    
    # EXIF-Details
    exif_fields:
      - camera_make
      - camera_model
      - lens
      - focal_length
      - aperture
      - shutter_speed
      - iso
      - exposure_compensation
      - flash
      - white_balance
      - gps_latitude
      - gps_longitude
      - gps_altitude
      - date_taken
      - orientation
    
    # Thumbnail-Generierung
    thumbnails:
      sizes:
        - name: "small"
          width: 150
          height: 150
          crop: true
        - name: "medium"
          width: 300
          height: 300
          crop: false
        - name: "large"
          width: 800
          height: 800
          crop: false
      format: "webp"
      quality: 85
    
    # Farbanalyse
    colors:
      palette_size: 5
      algorithm: "kmeans"  # kmeans, median_cut
  
  # Computer Vision (optional)
  vision:
    enabled: false
    
    # Object Detection
    object_detection:
      enabled: false
      model: "yolov8"
      confidence_threshold: 0.5
    
    # Face Detection
    face_detection:
      enabled: false
      blur_faces: false  # DSGVO
    
    # OCR (Text in Bildern)
    ocr:
      enabled: false
      language: "deu+eng"
    
    # CLIP Embeddings
    clip_embeddings:
      enabled: false
      model: "openai/clip-vit-base-patch32"
  
  backend:
    library: "libvips"  # libvips, imagemagick, pillow
    version_min: "8.12"
```

### 2.8 CAD Processor Konfiguration

```yaml
# /etc/themis/processors/cad.yaml

processor:
  name: "CAD Processor"
  version: "1.0.0"
  library: "themis_proc_cad"
  
  mime_types:
    - "application/step"
    - "application/x-step"
    - "application/iges"
    - "application/x-iges"
    - "model/stl"
    - "application/sla"
    - "image/vnd.dxf"
    - "application/x-autocad"
  
  extensions:
    - ".step"
    - ".stp"
    - ".iges"
    - ".igs"
    - ".stl"
    - ".dxf"
    - ".dwg"
    - ".3ds"
    - ".obj"
    - ".gltf"
    - ".glb"
  
  extraction:
    metadata: true
    geometry_summary: true
    bom: true           # Bill of Materials
    assembly_tree: true
    thumbnail: true
    
    # Geometrie-Details
    geometry:
      bounding_box: true
      volume: true
      surface_area: true
      center_of_mass: true
      part_count: true
    
    # BOM-Extraktion
    bom:
      include_properties: true
      include_materials: true
      hierarchical: true
    
    # Thumbnail/Preview
    preview:
      enabled: true
      format: "png"
      size: "800x600"
      view: "isometric"  # isometric, front, top, right
  
  # Konvertierung
  conversion:
    enabled: false
    output_formats:
      - "stl"
      - "obj"
      - "gltf"
  
  backend:
    library: "opencascade"
    version_min: "7.6"
```

---

## 3. Plugin-Interface

### 3.1 C++ Plugin API

```cpp
// include/content/plugin_interface.h

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <nlohmann/json.hpp>

namespace themis {
namespace content {

using json = nlohmann::json;

/**
 * @brief Plugin Metadata
 */
struct PluginInfo {
    std::string name;
    std::string version;
    std::string description;
    std::string author;
    std::vector<std::string> mime_types;
    std::vector<std::string> extensions;
};

/**
 * @brief Extraction Result
 */
struct ExtractionResult {
    bool success;
    std::string text;
    json metadata;
    std::vector<uint8_t> thumbnail;
    std::string error_message;
    
    // Optionale spezifische Daten
    struct GeoData {
        std::vector<std::pair<double, double>> coordinates;
        std::string crs;
        json properties;
    };
    std::optional<GeoData> geo;
    
    struct MediaData {
        int duration_ms;
        int width;
        int height;
        std::string codec;
    };
    std::optional<MediaData> media;
};

/**
 * @brief Chunk für RAG/Search
 */
struct ContentChunk {
    std::string text;
    int sequence;
    int start_offset;
    int end_offset;
    int token_count;
    json metadata;
};

/**
 * @brief Plugin Configuration (from YAML)
 */
struct PluginConfig {
    json settings;
    
    template<typename T>
    T get(const std::string& path, T default_value) const;
    
    bool has(const std::string& path) const;
};

/**
 * @brief Content Processor Plugin Interface
 * 
 * Alle Content Processor Plugins müssen dieses Interface implementieren.
 */
class IContentProcessorPlugin {
public:
    virtual ~IContentProcessorPlugin() = default;
    
    /**
     * @brief Get plugin information
     */
    virtual PluginInfo getInfo() const = 0;
    
    /**
     * @brief Initialize plugin with configuration
     */
    virtual bool initialize(const PluginConfig& config) = 0;
    
    /**
     * @brief Shutdown plugin
     */
    virtual void shutdown() = 0;
    
    /**
     * @brief Check if plugin can process given content
     */
    virtual bool canProcess(const std::string& mime_type) const = 0;
    
    /**
     * @brief Extract content from blob
     */
    virtual ExtractionResult extract(
        const std::vector<uint8_t>& blob,
        const std::string& mime_type,
        const json& options = {}
    ) = 0;
    
    /**
     * @brief Chunk extracted content for RAG
     */
    virtual std::vector<ContentChunk> chunk(
        const ExtractionResult& result,
        int max_tokens,
        int overlap
    ) = 0;
    
    /**
     * @brief Generate embedding for chunk (optional)
     */
    virtual std::vector<float> generateEmbedding(
        const std::string& text
    ) {
        return {};  // Default: keine Embeddings
    }
    
    /**
     * @brief Health check
     */
    virtual bool healthCheck() const = 0;
};

/**
 * @brief Plugin Entry Point Macro
 */
#define THEMIS_CONTENT_PLUGIN(PluginClass) \
    extern "C" { \
        THEMIS_EXPORT IContentProcessorPlugin* themis_create_plugin() { \
            return new PluginClass(); \
        } \
        THEMIS_EXPORT void themis_destroy_plugin(IContentProcessorPlugin* plugin) { \
            delete plugin; \
        } \
        THEMIS_EXPORT const char* themis_plugin_version() { \
            return THEMIS_PLUGIN_API_VERSION; \
        } \
    }

} // namespace content
} // namespace themis
```

### 3.2 Plugin Registry

```cpp
// include/content/plugin_registry.h

#pragma once

#include "content/plugin_interface.h"
#include <map>
#include <memory>
#include <filesystem>

namespace themis {
namespace content {

/**
 * @brief Content Processor Plugin Registry
 * 
 * Verwaltet das Laden und Lifecycle aller Content Processor Plugins.
 */
class ContentProcessorRegistry {
public:
    static ContentProcessorRegistry& instance();
    
    /**
     * @brief Load configuration from YAML
     */
    bool loadConfig(const std::filesystem::path& config_path);
    
    /**
     * @brief Load all plugins from directory
     */
    int loadPlugins(const std::filesystem::path& plugin_dir);
    
    /**
     * @brief Load single plugin
     */
    bool loadPlugin(const std::filesystem::path& plugin_path);
    
    /**
     * @brief Get processor for MIME type
     */
    IContentProcessorPlugin* getProcessor(const std::string& mime_type);
    
    /**
     * @brief Get all registered processors
     */
    std::vector<PluginInfo> getRegisteredProcessors() const;
    
    /**
     * @brief Unload all plugins
     */
    void unloadAll();
    
    /**
     * @brief Process content (auto-detect processor)
     */
    ExtractionResult process(
        const std::vector<uint8_t>& blob,
        const std::string& mime_type,
        const json& options = {}
    );

private:
    ContentProcessorRegistry() = default;
    
    struct LoadedPlugin {
        void* handle;
        std::unique_ptr<IContentProcessorPlugin> plugin;
        PluginConfig config;
    };
    
    std::map<std::string, std::shared_ptr<LoadedPlugin>> plugins_;
    std::map<std::string, std::string> mime_to_plugin_;  // MIME -> Plugin Name
    json global_config_;
    
    bool verifyPluginSignature(const std::filesystem::path& plugin_path);
};

} // namespace content
} // namespace themis
```

---

## 4. Sicherheit

### 4.1 Plugin-Signierung

Alle Plugins müssen digital signiert sein:

```yaml
# Plugin Signature Verification
security:
  signature_verification:
    enabled: true
    trusted_keys:
      - "/etc/themis/keys/plugin_signing_public.pem"
    algorithm: "RSA-SHA256"
    reject_unsigned: true
```

### 4.2 Sandbox-Isolation

```yaml
# Sandbox Configuration
sandbox:
  enabled: true
  type: "seccomp"  # seccomp, apparmor, namespace
  
  # Erlaubte Syscalls
  allowed_syscalls:
    - read
    - write
    - mmap
    - munmap
    - brk
    - close
    - fstat
    - lseek
    
  # Verbotene Aktionen
  deny:
    - network_access
    - filesystem_write
    - process_spawn
    - ipc
```

### 4.3 Ressourcenlimits

```yaml
# Resource Limits (per Plugin)
limits:
  memory:
    max_mb: 512
    oom_score_adj: 500
  
  cpu:
    max_percent: 50
    nice: 10
  
  time:
    max_seconds: 60
    grace_seconds: 5
  
  io:
    max_read_mb_sec: 100
    max_write_mb_sec: 50
```

---

## 5. Entwicklung eines Plugins

### 5.1 Beispiel: Custom Video Processor

```cpp
// plugins/video/video_processor.cpp

#include "content/plugin_interface.h"
#include <ffmpeg/avcodec.h>
#include <ffmpeg/avformat.h>

namespace themis {
namespace content {

class VideoProcessor : public IContentProcessorPlugin {
public:
    PluginInfo getInfo() const override {
        return {
            .name = "Video Processor",
            .version = "1.0.0",
            .description = "Extract metadata and thumbnails from video files",
            .author = "ThemisDB Team",
            .mime_types = {"video/mp4", "video/x-matroska", "video/webm"},
            .extensions = {".mp4", ".mkv", ".webm"}
        };
    }
    
    bool initialize(const PluginConfig& config) override {
        config_ = config;
        
        // FFmpeg initialisieren
        av_register_all();
        avcodec_register_all();
        
        return true;
    }
    
    void shutdown() override {
        // Cleanup
    }
    
    bool canProcess(const std::string& mime_type) const override {
        return mime_type.find("video/") == 0;
    }
    
    ExtractionResult extract(
        const std::vector<uint8_t>& blob,
        const std::string& mime_type,
        const json& options
    ) override {
        ExtractionResult result;
        result.success = false;
        
        // AVFormatContext für Blob erstellen
        AVFormatContext* fmt_ctx = nullptr;
        // ... FFmpeg-Verarbeitung ...
        
        if (fmt_ctx) {
            result.metadata["duration_ms"] = 
                fmt_ctx->duration / (AV_TIME_BASE / 1000);
            result.metadata["format"] = fmt_ctx->iformat->name;
            
            // Streams analysieren
            for (unsigned i = 0; i < fmt_ctx->nb_streams; i++) {
                AVStream* stream = fmt_ctx->streams[i];
                if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                    result.metadata["width"] = stream->codecpar->width;
                    result.metadata["height"] = stream->codecpar->height;
                    result.metadata["codec"] = 
                        avcodec_get_name(stream->codecpar->codec_id);
                }
            }
            
            // Thumbnail generieren
            if (config_.get("extraction.thumbnail", true)) {
                result.thumbnail = generateThumbnail(fmt_ctx);
            }
            
            result.success = true;
            avformat_close_input(&fmt_ctx);
        }
        
        return result;
    }
    
    std::vector<ContentChunk> chunk(
        const ExtractionResult& result,
        int max_tokens,
        int overlap
    ) override {
        // Videos werden nicht in Text-Chunks aufgeteilt
        // Stattdessen: Scene-basierte Chunks
        std::vector<ContentChunk> chunks;
        
        ContentChunk chunk;
        chunk.text = result.metadata.dump();
        chunk.sequence = 0;
        chunk.token_count = 100;  // Geschätzt
        chunks.push_back(chunk);
        
        return chunks;
    }
    
    bool healthCheck() const override {
        // Prüfen ob FFmpeg verfügbar
        return av_version_info() != nullptr;
    }

private:
    PluginConfig config_;
    
    std::vector<uint8_t> generateThumbnail(AVFormatContext* fmt_ctx) {
        std::vector<uint8_t> thumbnail;
        // ... Thumbnail-Generierung mit FFmpeg ...
        return thumbnail;
    }
};

// Plugin exportieren
THEMIS_CONTENT_PLUGIN(VideoProcessor)

} // namespace content
} // namespace themis
```

### 5.2 CMake Build

```cmake
# plugins/video/CMakeLists.txt

cmake_minimum_required(VERSION 3.20)
project(themis_proc_video VERSION 1.0.0)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FFMPEG REQUIRED libavcodec libavformat libavutil libswscale)

add_library(themis_proc_video SHARED
    video_processor.cpp
)

target_include_directories(themis_proc_video PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${FFMPEG_INCLUDE_DIRS}
)

target_link_libraries(themis_proc_video PRIVATE
    ${FFMPEG_LIBRARIES}
)

# Plugin-Signierung
if(THEMIS_SIGN_PLUGINS)
    add_custom_command(TARGET themis_proc_video POST_BUILD
        COMMAND ${CMAKE_SOURCE_DIR}/scripts/sign_plugin.sh
                $<TARGET_FILE:themis_proc_video>
                ${THEMIS_SIGNING_KEY}
    )
endif()

install(TARGETS themis_proc_video
    LIBRARY DESTINATION lib/themis/plugins/processors
)

install(FILES video.yaml
    DESTINATION etc/themis/processors
)
```

---

## 6. Verfügbare Plugins

| Plugin | Status | Bibliothek | Features |
|--------|--------|------------|----------|
| `themis_proc_pdf` | ✅ Ready | poppler | Text, Metadata, OCR |
| `themis_proc_office` | ✅ Ready | libzip, pugixml | DOCX, XLSX, PPTX, ODF |
| `themis_proc_geo` | ✅ Ready | GDAL | GeoJSON, GPX, Shapefile |
| `themis_proc_video` | ✅ Ready | FFmpeg | Metadata, Thumbnails |
| `themis_proc_audio` | ✅ Ready | FFmpeg | Metadata, Transcription |
| `themis_proc_image` | ✅ Ready | libvips | EXIF, Thumbnails, Colors |
| `themis_proc_cad` | 🚧 Beta | OpenCASCADE | STEP, IGES, STL |
| `themis_proc_text` | ✅ Ready | Built-in | TXT, JSON, XML, MD |

---

## Anhang: Migration von Built-in zu Plugin

Bestehende Content Processors (`text_processor.cpp`, etc.) werden in Plugins umgewandelt:

```bash
# Migration Script
./scripts/migrate_content_processors.sh

# Ergebnis:
# src/content/text_processor.cpp -> plugins/text/text_processor.cpp
# src/content/pdf_processor.cpp  -> plugins/pdf/pdf_processor.cpp
# ...
```

---

**Weitere Informationen:**
- [Plugin Development Guide](../development/plugin_development.md)
- [Security: Plugin Signing](../security/PLUGIN_SECURITY.md)
- [API Reference](../api/content_api.md)
