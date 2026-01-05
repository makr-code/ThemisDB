# P2: Extended Content Processing

**Priorität:** P2 - NICE-TO-HAVE  
**Zeitrahmen:** 6-8 Wochen (Sprint 4-6)  
**Status:** 🟢 Geplant

---

## 🎯 Zielsetzung

Erweitern der Content-Processing-Fähigkeiten um Video, PowerPoint, GeoTIFF/Shapefiles und vollständige PostgreSQL Wire Protocol Unterstützung.

---

## 📊 Feature-Übersicht

| Feature | Priorität | Aufwand | Status | Use Case |
|---------|-----------|---------|--------|----------|
| **Video Processor (FFmpeg)** | P2 | 1 Woche | 🔴 TODO | Media Analysis |
| **Office PPTX Support** | P2 | 3 Tage | 🔴 TODO | Presentations |
| **Geo Processor (GDAL)** | P2 | 1 Woche | 🔴 TODO | GIS Integration |
| **PostgreSQL Wire Protocol** | P2 | 2 Wochen | 🔴 TODO | BI Tools |

**Gesamt:** 4-5 Wochen Development + 2-3 Wochen Testing

---

## 1️⃣ Video Processor (FFmpeg Integration)

### Aktuelle Situation

**Datei:** `src/content/video_processor.cpp`

**Aktueller Code (Simulation):**
```cpp
VideoMetadata VideoProcessor::extractMetadata(const std::vector<uint8_t>& data) {
    VideoMetadata meta;
    
    // This is a simulation - real implementation would use libavformat
    meta.duration_seconds = 120.0;  // Placeholder
    meta.width = 1920;
    meta.height = 1080;
    meta.fps = 30.0;
    meta.codec = "h264";  // Assumed
    
    THEMIS_WARN("VideoProcessor: Using simulated metadata extraction");
    return meta;
}

std::vector<uint8_t> VideoProcessor::extractThumbnail(const std::vector<uint8_t>& data) {
    // Return empty thumbnail placeholder
    return {};
}
```

### Use Cases

1. **Video Content Analysis**
   - Metadata-Extraktion (Dauer, Auflösung, Codec)
   - Thumbnail-Generierung
   - Frame-Extraktion für AI-Analyse

2. **Media Asset Management**
   - Automatische Katalogisierung
   - Qualitätsprüfung
   - Format-Konvertierung

3. **Forensik & Compliance**
   - Video-Hashing
   - Metadata-Integrität
   - Timestamp-Extraktion

### Implementation

**Dependencies:**
```cmake
# CMakeLists.txt
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBAV REQUIRED 
    libavformat 
    libavcodec 
    libavutil 
    libswscale
)

target_link_libraries(themis_content PRIVATE 
    ${LIBAV_LIBRARIES}
)
```

**Neue Implementation:**

```cpp
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>

class VideoProcessorFFmpeg {
public:
    VideoProcessorFFmpeg() {
        // Initialize FFmpeg (thread-safe in modern versions)
        av_log_set_level(AV_LOG_ERROR);
    }
    
    VideoMetadata extractMetadata(const std::vector<uint8_t>& data) {
        VideoMetadata meta;
        
        // 1. Create custom AVIOContext for memory buffer
        AVIOContext* avio_ctx = createAVIOContext(data);
        if (!avio_ctx) {
            throw std::runtime_error("Failed to create AVIO context");
        }
        
        // 2. Open input format
        AVFormatContext* fmt_ctx = avformat_alloc_context();
        fmt_ctx->pb = avio_ctx;
        
        if (avformat_open_input(&fmt_ctx, nullptr, nullptr, nullptr) < 0) {
            av_free(avio_ctx->buffer);
            avio_context_free(&avio_ctx);
            throw std::runtime_error("Failed to open input");
        }
        
        // 3. Read stream info
        if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
            avformat_close_input(&fmt_ctx);
            throw std::runtime_error("Failed to find stream info");
        }
        
        // 4. Find video stream
        int video_stream_idx = -1;
        for (unsigned i = 0; i < fmt_ctx->nb_streams; i++) {
            if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                video_stream_idx = i;
                break;
            }
        }
        
        if (video_stream_idx == -1) {
            avformat_close_input(&fmt_ctx);
            throw std::runtime_error("No video stream found");
        }
        
        // 5. Extract metadata
        AVStream* video_stream = fmt_ctx->streams[video_stream_idx];
        AVCodecParameters* codecpar = video_stream->codecpar;
        
        meta.codec = avcodec_get_name(codecpar->codec_id);
        meta.width = codecpar->width;
        meta.height = codecpar->height;
        meta.bitrate = codecpar->bit_rate;
        
        // Duration
        if (fmt_ctx->duration != AV_NOPTS_VALUE) {
            meta.duration_seconds = static_cast<double>(fmt_ctx->duration) / AV_TIME_BASE;
        } else if (video_stream->duration != AV_NOPTS_VALUE) {
            meta.duration_seconds = static_cast<double>(video_stream->duration) * 
                                   av_q2d(video_stream->time_base);
        }
        
        // Frame rate
        AVRational fps = av_guess_frame_rate(fmt_ctx, video_stream, nullptr);
        meta.fps = av_q2d(fps);
        
        // Audio streams
        for (unsigned i = 0; i < fmt_ctx->nb_streams; i++) {
            if (fmt_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
                meta.has_audio = true;
                meta.audio_codec = avcodec_get_name(fmt_ctx->streams[i]->codecpar->codec_id);
                break;
            }
        }
        
        // Container format
        meta.container_format = fmt_ctx->iformat->name;
        
        // Cleanup
        avformat_close_input(&fmt_ctx);
        av_free(avio_ctx->buffer);
        avio_context_free(&avio_ctx);
        
        return meta;
    }
    
    std::vector<uint8_t> extractThumbnail(const std::vector<uint8_t>& data, 
                                          double timestamp_seconds = 0.0) {
        // 1. Setup decoder
        AVIOContext* avio_ctx = createAVIOContext(data);
        AVFormatContext* fmt_ctx = avformat_alloc_context();
        fmt_ctx->pb = avio_ctx;
        
        if (avformat_open_input(&fmt_ctx, nullptr, nullptr, nullptr) < 0) {
            throw std::runtime_error("Failed to open input");
        }
        
        avformat_find_stream_info(fmt_ctx, nullptr);
        
        // 2. Find video stream
        int video_stream_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
        if (video_stream_idx < 0) {
            avformat_close_input(&fmt_ctx);
            throw std::runtime_error("No video stream");
        }
        
        AVStream* video_stream = fmt_ctx->streams[video_stream_idx];
        
        // 3. Setup decoder
        const AVCodec* codec = avcodec_find_decoder(video_stream->codecpar->codec_id);
        AVCodecContext* codec_ctx = avcodec_alloc_context3(codec);
        avcodec_parameters_to_context(codec_ctx, video_stream->codecpar);
        avcodec_open2(codec_ctx, codec, nullptr);
        
        // 4. Seek to timestamp
        int64_t seek_ts = static_cast<int64_t>(timestamp_seconds / av_q2d(video_stream->time_base));
        av_seek_frame(fmt_ctx, video_stream_idx, seek_ts, AVSEEK_FLAG_BACKWARD);
        
        // 5. Decode frame
        AVPacket* packet = av_packet_alloc();
        AVFrame* frame = av_frame_alloc();
        
        bool got_frame = false;
        while (av_read_frame(fmt_ctx, packet) >= 0) {
            if (packet->stream_index == video_stream_idx) {
                if (avcodec_send_packet(codec_ctx, packet) >= 0) {
                    if (avcodec_receive_frame(codec_ctx, frame) >= 0) {
                        got_frame = true;
                        av_packet_unref(packet);
                        break;
                    }
                }
            }
            av_packet_unref(packet);
        }
        
        if (!got_frame) {
            throw std::runtime_error("Failed to decode frame");
        }
        
        // 6. Convert to JPEG
        std::vector<uint8_t> thumbnail = frameToJPEG(frame, 800, 600);
        
        // 7. Cleanup
        av_frame_free(&frame);
        av_packet_free(&packet);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&fmt_ctx);
        av_free(avio_ctx->buffer);
        avio_context_free(&avio_ctx);
        
        return thumbnail;
    }
    
    std::vector<std::vector<uint8_t>> extractKeyFrames(const std::vector<uint8_t>& data, 
                                                        int max_frames = 10) {
        // Extract key frames for video preview
        std::vector<std::vector<uint8_t>> frames;
        
        // Similar to extractThumbnail but iterate and filter key frames
        // Implementation details...
        
        return frames;
    }

private:
    static AVIOContext* createAVIOContext(const std::vector<uint8_t>& data) {
        size_t buffer_size = data.size();
        uint8_t* buffer = static_cast<uint8_t*>(av_malloc(buffer_size));
        std::memcpy(buffer, data.data(), buffer_size);
        
        AVIOContext* avio_ctx = avio_alloc_context(
            buffer, buffer_size, 0, nullptr,
            readPacket, nullptr, seekPacket
        );
        
        return avio_ctx;
    }
    
    static int readPacket(void* opaque, uint8_t* buf, int buf_size) {
        // Custom read function for memory buffer
        // Implementation...
        return 0;
    }
    
    static int64_t seekPacket(void* opaque, int64_t offset, int whence) {
        // Custom seek function
        // Implementation...
        return 0;
    }
    
    std::vector<uint8_t> frameToJPEG(AVFrame* frame, int target_width, int target_height) {
        // Convert AVFrame to JPEG
        // 1. Scale frame
        SwsContext* sws_ctx = sws_getContext(
            frame->width, frame->height, static_cast<AVPixelFormat>(frame->format),
            target_width, target_height, AV_PIX_FMT_RGB24,
            SWS_BILINEAR, nullptr, nullptr, nullptr
        );
        
        AVFrame* rgb_frame = av_frame_alloc();
        rgb_frame->format = AV_PIX_FMT_RGB24;
        rgb_frame->width = target_width;
        rgb_frame->height = target_height;
        av_frame_get_buffer(rgb_frame, 0);
        
        sws_scale(sws_ctx, frame->data, frame->linesize, 0, frame->height,
                  rgb_frame->data, rgb_frame->linesize);
        
        // 2. Encode to JPEG using libjpeg or similar
        std::vector<uint8_t> jpeg_data;
        // ... JPEG encoding ...
        
        // Cleanup
        av_frame_free(&rgb_frame);
        sws_freeContext(sws_ctx);
        
        return jpeg_data;
    }
};
```

**Testing:**

```cpp
// tests/test_video_processor_ffmpeg.cpp
TEST(VideoProcessorFFmpeg, ExtractMetadata) {
    VideoProcessorFFmpeg processor;
    
    // Load test video
    std::vector<uint8_t> video_data = loadTestVideo("sample.mp4");
    
    auto meta = processor.extractMetadata(video_data);
    
    EXPECT_EQ(meta.codec, "h264");
    EXPECT_EQ(meta.width, 1920);
    EXPECT_EQ(meta.height, 1080);
    EXPECT_GT(meta.duration_seconds, 0.0);
    EXPECT_GT(meta.fps, 0.0);
}

TEST(VideoProcessorFFmpeg, ExtractThumbnail) {
    VideoProcessorFFmpeg processor;
    std::vector<uint8_t> video_data = loadTestVideo("sample.mp4");
    
    auto thumbnail = processor.extractThumbnail(video_data, 5.0);  // 5 seconds
    
    EXPECT_GT(thumbnail.size(), 0);
    // Verify JPEG header
    EXPECT_EQ(thumbnail[0], 0xFF);
    EXPECT_EQ(thumbnail[1], 0xD8);
}
```

**Aufwand:** 1 Woche

---

## 2️⃣ Office PPTX Support

### Aktuelle Situation

**Datei:** `src/content/office_processor.cpp`

**Status:** DOCX/XLSX ✅ fertig, PPTX ❌ Placeholder

```cpp
if (content_type.mime_type.find("presentationml") != std::string::npos) {
    // 🟡 PPTX - Placeholder
    result.text = "[PPTX extraction not yet implemented]";
    THEMIS_WARN("PPTX extraction is a placeholder");
}
```

### Use Cases

1. **Presentation Search** - Volltext in Folien
2. **Slide Analysis** - Titel, Inhalte, Notizen
3. **Asset Extraction** - Bilder, Diagramme
4. **Metadata** - Autor, Erstellungsdatum, Revisionen

### Implementation

**PPTX Structure:**
```
presentation.pptx (ZIP)
├── [Content_Types].xml
├── _rels/
├── ppt/
│   ├── presentation.xml
│   ├── slides/
│   │   ├── slide1.xml
│   │   ├── slide2.xml
│   ├── slideLayouts/
│   ├── slideMasters/
│   ├── notesSlides/
│   ├── media/  (images, videos)
│   └── _rels/
└── docProps/
    ├── app.xml
    └── core.xml
```

**Code:**

```cpp
#include <zip.h>  // libzip
#include <pugixml.hpp>

class PPTXProcessor {
public:
    ExtractionResult extract(const std::vector<uint8_t>& pptx_data) {
        ExtractionResult result;
        
        // 1. Open PPTX as ZIP
        zip_source_t* src = zip_source_buffer_create(pptx_data.data(), pptx_data.size(), 0, nullptr);
        zip_t* archive = zip_open_from_source(src, ZIP_RDONLY, nullptr);
        
        if (!archive) {
            result.text = "[Failed to open PPTX archive]";
            return result;
        }
        
        // 2. Extract metadata
        result.metadata = extractMetadata(archive);
        
        // 3. Get slide count
        int num_slides = countSlides(archive);
        
        // 4. Extract text from each slide
        std::ostringstream text_stream;
        for (int i = 1; i <= num_slides; i++) {
            std::string slide_path = "ppt/slides/slide" + std::to_string(i) + ".xml";
            std::string slide_text = extractSlideText(archive, slide_path);
            
            text_stream << "=== Slide " << i << " ===\n";
            text_stream << slide_text << "\n\n";
            
            // Extract speaker notes
            std::string notes_path = "ppt/notesSlides/notesSlide" + std::to_string(i) + ".xml";
            std::string notes = extractNotesText(archive, notes_path);
            if (!notes.empty()) {
                text_stream << "Notes: " << notes << "\n\n";
            }
        }
        
        result.text = text_stream.str();
        
        // 5. Extract embedded media
        result.embedded_files = extractEmbeddedMedia(archive);
        
        zip_close(archive);
        return result;
    }

private:
    ContentMetadata extractMetadata(zip_t* archive) {
        ContentMetadata meta;
        
        // Read core.xml
        std::string core_xml = readZipFile(archive, "docProps/core.xml");
        pugi::xml_document doc;
        doc.load_string(core_xml.c_str());
        
        auto root = doc.child("cp:coreProperties");
        meta.author = root.child_value("dc:creator");
        meta.title = root.child_value("dc:title");
        meta.created = root.child_value("dcterms:created");
        meta.modified = root.child_value("dcterms:modified");
        
        // Read app.xml
        std::string app_xml = readZipFile(archive, "docProps/app.xml");
        doc.load_string(app_xml.c_str());
        
        auto props = doc.child("Properties");
        meta.slide_count = std::stoi(props.child_value("Slides"));
        
        return meta;
    }
    
    int countSlides(zip_t* archive) {
        int count = 0;
        for (int i = 0; i < zip_get_num_entries(archive, 0); i++) {
            const char* name = zip_get_name(archive, i, 0);
            if (std::string(name).find("ppt/slides/slide") == 0 &&
                std::string(name).find(".xml") != std::string::npos) {
                count++;
            }
        }
        return count;
    }
    
    std::string extractSlideText(zip_t* archive, const std::string& path) {
        std::string xml = readZipFile(archive, path);
        if (xml.empty()) return "";
        
        pugi::xml_document doc;
        doc.load_string(xml.c_str());
        
        std::ostringstream text;
        
        // Extract all <a:t> elements (text runs)
        for (auto node : doc.select_nodes("//a:t")) {
            text << node.node().child_value() << " ";
        }
        
        return text.str();
    }
    
    std::string extractNotesText(zip_t* archive, const std::string& path) {
        std::string xml = readZipFile(archive, path);
        if (xml.empty()) return "";
        
        pugi::xml_document doc;
        doc.load_string(xml.c_str());
        
        std::ostringstream text;
        for (auto node : doc.select_nodes("//a:t")) {
            text << node.node().child_value() << " ";
        }
        
        return text.str();
    }
    
    std::vector<EmbeddedFile> extractEmbeddedMedia(zip_t* archive) {
        std::vector<EmbeddedFile> files;
        
        // Find all files in ppt/media/
        for (int i = 0; i < zip_get_num_entries(archive, 0); i++) {
            const char* name = zip_get_name(archive, i, 0);
            std::string path(name);
            
            if (path.find("ppt/media/") == 0) {
                EmbeddedFile file;
                file.filename = path.substr(10);  // Remove "ppt/media/"
                file.data = readZipFileBinary(archive, path);
                file.mime_type = guessMimeType(file.filename);
                files.push_back(file);
            }
        }
        
        return files;
    }
    
    std::string readZipFile(zip_t* archive, const std::string& path) {
        zip_file_t* file = zip_fopen(archive, path.c_str(), 0);
        if (!file) return "";
        
        zip_stat_t stat;
        zip_stat(archive, path.c_str(), 0, &stat);
        
        std::string content(stat.size, '\0');
        zip_fread(file, &content[0], stat.size);
        zip_fclose(file);
        
        return content;
    }
};
```

**Aufwand:** 3 Tage

---

## 3️⃣ Geo Processor (GDAL Integration)

### Use Cases

1. **GIS Data Import** - Shapefiles, GeoTIFF
2. **Spatial Analysis** - Koordinaten-Extraktion
3. **Map Rendering** - Raster/Vector Data
4. **Geo-Coding** - Address → Coordinates

### Implementation

**Dependencies:**
```cmake
find_package(GDAL REQUIRED)
target_link_libraries(themis_content PRIVATE GDAL::GDAL)
```

**Code:**

```cpp
#include <gdal/gdal.h>
#include <gdal/ogrsf_frmts.h>
#include <gdal/gdal_priv.h>

class GeoProcessor {
public:
    GeoProcessor() {
        GDALAllRegister();
    }
    
    ExtractionResult extractShapefile(const std::vector<uint8_t>& data) {
        // Save to temp file (GDAL requires file path)
        std::string temp_path = saveTempFile(data, ".shp");
        
        GDALDataset* dataset = static_cast<GDALDataset*>(
            GDALOpenEx(temp_path.c_str(), GDAL_OF_VECTOR, nullptr, nullptr, nullptr)
        );
        
        if (!dataset) {
            throw std::runtime_error("Failed to open shapefile");
        }
        
        ExtractionResult result;
        std::ostringstream text;
        
        // Iterate layers
        for (int i = 0; i < dataset->GetLayerCount(); i++) {
            OGRLayer* layer = dataset->GetLayer(i);
            text << "Layer: " << layer->GetName() << "\n";
            text << "Features: " << layer->GetFeatureCount() << "\n";
            
            // Spatial reference
            OGRSpatialReference* srs = layer->GetSpatialRef();
            if (srs) {
                char* wkt;
                srs->exportToWkt(&wkt);
                text << "SRS: " << wkt << "\n";
                CPLFree(wkt);
            }
            
            // Extract features
            layer->ResetReading();
            OGRFeature* feature;
            int count = 0;
            while ((feature = layer->GetNextFeature()) != nullptr && count < 100) {
                text << extractFeatureText(feature);
                OGRFeature::DestroyFeature(feature);
                count++;
            }
        }
        
        result.text = text.str();
        GDALClose(dataset);
        
        return result;
    }
    
    ExtractionResult extractGeoTIFF(const std::vector<uint8_t>& data) {
        std::string temp_path = saveTempFile(data, ".tif");
        
        GDALDataset* dataset = static_cast<GDALDataset*>(
            GDALOpen(temp_path.c_str(), GA_ReadOnly)
        );
        
        if (!dataset) {
            throw std::runtime_error("Failed to open GeoTIFF");
        }
        
        ExtractionResult result;
        std::ostringstream text;
        
        // Raster metadata
        text << "Size: " << dataset->GetRasterXSize() << "x" << dataset->GetRasterYSize() << "\n";
        text << "Bands: " << dataset->GetRasterCount() << "\n";
        
        // Geotransform
        double geotransform[6];
        if (dataset->GetGeoTransform(geotransform) == CE_None) {
            text << "Origin: (" << geotransform[0] << ", " << geotransform[3] << ")\n";
            text << "Pixel Size: (" << geotransform[1] << ", " << geotransform[5] << ")\n";
        }
        
        // Projection
        const char* projection = dataset->GetProjectionRef();
        if (projection) {
            text << "Projection: " << projection << "\n";
        }
        
        result.text = text.str();
        GDALClose(dataset);
        
        return result;
    }

private:
    std::string extractFeatureText(OGRFeature* feature) {
        std::ostringstream text;
        
        // Geometry
        OGRGeometry* geom = feature->GetGeometryRef();
        if (geom) {
            char* wkt;
            geom->exportToWkt(&wkt);
            text << "Geometry: " << wkt << "\n";
            CPLFree(wkt);
        }
        
        // Attributes
        for (int i = 0; i < feature->GetFieldCount(); i++) {
            OGRFieldDefn* field_defn = feature->GetFieldDefnRef(i);
            text << field_defn->GetNameRef() << ": " 
                 << feature->GetFieldAsString(i) << "\n";
        }
        
        text << "---\n";
        return text.str();
    }
};
```

**Aufwand:** 1 Woche

---

## 4️⃣ PostgreSQL Wire Protocol Completion

### Aktuelle Situation

**Datei:** `src/server/postgres_session.cpp`

**Status:** Multiple Stubs für Parse, Bind, Execute, Describe, Close

```cpp
// PostgreSQL Parse message handler (stub)
void PostgresSession::handleParse(const std::vector<uint8_t>& message) {
    // TODO: Parse SQL statement, create prepared statement
}

// PostgreSQL Bind message handler (stub)
void PostgresSession::handleBind(const std::vector<uint8_t>& message) {
    // TODO: Bind parameters to prepared statement
}
```

### Use Cases

1. **BI Tool Compatibility** - Tableau, Power BI, Metabase
2. **JDBC/ODBC Drivers**
3. **psql CLI Support**
4. **ORM Compatibility** - Django, SQLAlchemy

### Implementation Overview

**Features to Implement:**

1. **Prepared Statements** (Parse/Bind/Execute/Describe)
2. **Extended Query Protocol**
3. **Parameter Binding** (Binary/Text Formats)
4. **Result Set Streaming**
5. **Transaction Control**
6. **COPY Protocol**

**Aufwand:** 2 Wochen

---

## 📋 Implementation Timeline

### Sprint 4 (Woche 7-8)

#### Woche 7
- [ ] **Tag 1-3:** Video Processor - Metadata
  - FFmpeg Integration
  - Metadata Extraction
  - Unit Tests

- [ ] **Tag 4-5:** Video Processor - Thumbnails
  - Frame Decoding
  - JPEG Encoding
  - Key Frame Extraction

#### Woche 8
- [ ] **Tag 1-3:** Office PPTX Support
  - ZIP/XML Parsing
  - Text Extraction
  - Slide/Notes Handling
  - Unit Tests

### Sprint 5 (Woche 9-10)

#### Woche 9
- [ ] **Tag 1-3:** Geo Processor - Shapefiles
  - GDAL Integration
  - Shapefile Parsing
  - Feature Extraction

- [ ] **Tag 4-5:** Geo Processor - GeoTIFF
  - Raster Data Handling
  - Metadata Extraction
  - Unit Tests

#### Woche 10
- [ ] **Tag 1-5:** PostgreSQL Protocol - Teil 1
  - Prepared Statements (Parse/Describe)
  - Parameter Binding (Bind)
  - Basic Execute

### Sprint 6 (Woche 11-12)

#### Woche 11
- [ ] **Tag 1-5:** PostgreSQL Protocol - Teil 2
  - Extended Query Protocol
  - Binary Formats
  - Result Set Streaming
  - Transaction Control

#### Woche 12
- [ ] **Tag 1-2:** PostgreSQL Protocol - COPY
  - COPY IN/OUT Support
  - Bulk Data Transfer

- [ ] **Tag 3-5:** Integration Testing & Bug Fixes
  - BI Tool Testing (Metabase, Tableau)
  - psql Compatibility
  - Performance Tests

---

## 🧪 Testing Strategy

### Unit Tests
- Video: Metadata extraction, Thumbnail generation
- PPTX: Slide parsing, Notes extraction
- Geo: Shapefile features, GeoTIFF metadata
- PostgreSQL: Protocol messages, Binary formats

### Integration Tests
- FFmpeg: Test with various video formats (MP4, AVI, MKV)
- PPTX: Real presentations from Office 365
- GDAL: Shapefiles from QGIS, GeoTIFF from satellite data
- PostgreSQL: psql client, JDBC driver, Tableau connection

### Performance Tests
- Video: Thumbnail generation < 1s for 1080p
- PPTX: 100-slide deck < 5s
- Geo: 10k feature shapefile < 10s
- PostgreSQL: Query latency comparable to PostgreSQL

---

## 📊 Success Metrics

| Feature | Metric | Target | Status |
|---------|--------|--------|--------|
| Video Processor | Metadata Accuracy | > 95% | 🔴 TODO |
| Video Processor | Thumbnail Generation | < 1s | 🔴 TODO |
| PPTX Support | Text Extraction | 100% | 🔴 TODO |
| Geo Processor | Format Support | Shapefile + GeoTIFF | 🔴 TODO |
| PostgreSQL Protocol | psql Compatibility | 100% | 🔴 TODO |
| PostgreSQL Protocol | BI Tool Support | Tableau + Metabase | 🔴 TODO |

---

## 🔗 Dependencies

- **P0 & P1** müssen nicht abgeschlossen sein (unabhängig)
- FFmpeg für Video
- libzip + pugixml für PPTX
- GDAL für Geo
- PostgreSQL Protocol Specs

---

**Erstellt:** 4. Januar 2026  
**Start:** 17. Februar 2026 (nach P1)  
**Ende:** 28. März 2026  
**Status:** 🟢 Geplant
