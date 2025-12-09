# Content Module

**Stand:** 5. Dezember 2025  
**Version:** 1.0.0  
**Kategorie:** Content

---

## Übersicht

Das Content-Modul bietet eine vollständige Content-Ingestion-Pipeline für ThemisDB, einschließlich Text-Extraktion, Chunking, Embedding-Generierung und Content-Prozessoren für verschiedene Dateitypen.

## Source-Code Referenz

| Komponente | Header | Source | Beschreibung |
|------------|--------|--------|--------------|
| ContentManager | `content_manager.h` | `content_manager.cpp` | Zentrale Content-Verwaltung |
| ContentTypeRegistry | `content_type.h` | `content_type.cpp` | MIME-Type-Erkennung |
| ContentProcessor | `content_processor.h` | - | Basis-Interface |
| TextProcessor | `text_processor.h` | `text_processor.cpp` | Text-Extraktion |
| PDFProcessor | `pdf_processor.h` | `pdf_processor.cpp` | PDF-Verarbeitung |
| OfficeProcessor | `office_processor.h` | `office_processor.cpp` | Office-Dokumente |
| ImageProcessor | `image_processor.h` | `image_processor.cpp` | Bildverarbeitung |
| GeoProcessor | `geo_processor.h` | `geo_processor.cpp` | Geo-Daten |
| VideoProcessor | `video_processor.h` | `video_processor.cpp` | Video-Metadaten |
| AudioProcessor | `audio_processor.h` | `audio_processor.cpp` | Audio-Metadaten |
| CADProcessor | `cad_processor.h` | `cad_processor.cpp` | CAD-Dateien |

**Gesamt:** 16 Header, 15 Source-Dateien, ~9,000 LOC

## Datenstrukturen

### ContentMeta

Universelle Metadaten-Struktur für alle Content-Typen:

```cpp
struct ContentMeta {
    std::string id;                  // Content UUID
    std::string mime_type;           // MIME type
    ContentCategory category;        // Category
    std::string original_filename;   // Original filename
    int64_t size_bytes;              // Original size
    bool compressed;                 // Stored compressed (zstd)
    bool encrypted;                  // Stored encrypted (aes-256-gcm)
    std::string hash_sha256;         // Deduplizierungs-Hash
    bool text_extracted;             // Text-Extraktion erfolgreich
    bool chunked;                    // Content wurde gechunkt
    bool indexed;                    // In VectorIndex indexiert
    int chunk_count;                 // Anzahl Chunks
    json extracted_metadata;         // EXIF, ID3, CAD-Properties etc.
    std::string virtual_path;        // Virtual Filesystem Path
};
```

### ChunkMeta

Chunk-Metadaten für Text-Segmente:

```cpp
struct ChunkMeta {
    std::string id;                  // Chunk UUID
    std::string content_id;          // Parent Content ID
    int32_t seq_num;                 // Sequenznummer
    int64_t start_byte;              // Byte-Offset Start
    int64_t end_byte;                // Byte-Offset Ende
    std::string text;                // Extrahierter Text
    std::vector<float> embedding;    // Vektor-Embedding
    json chunk_metadata;             // Zusätzliche Metadaten
};
```

## Unterstützte Content-Typen

| Kategorie | MIME Types | Prozessor |
|-----------|------------|-----------|
| **Text** | text/plain, text/markdown, text/csv | TextProcessor |
| **PDF** | application/pdf | PDFProcessor (poppler) |
| **Office** | application/vnd.openxmlformats-* | OfficeProcessor |
| **Image** | image/jpeg, image/png, image/webp | ImageProcessor (libvips) |
| **Video** | video/mp4, video/webm | VideoProcessor (FFmpeg) |
| **Audio** | audio/mpeg, audio/wav | AudioProcessor (FFmpeg) |
| **Geo** | application/geo+json, application/gpx+xml | GeoProcessor (GDAL) |
| **CAD** | model/step, model/stl | CADProcessor (OpenCASCADE) |

## API

### Content Import

```cpp
ContentManager manager(db, vector_index, graph_index);

// Import mit automatischer Typ-Erkennung
auto result = manager.import("/path/to/document.pdf", {
    .extract_text = true,
    .chunk_strategy = ChunkStrategy::Semantic,
    .generate_embeddings = true,
    .encrypt = true
});

// Zugriff auf Chunks
for (const auto& chunk : result.chunks) {
    // Vektor-Suche über Chunks möglich
}
```

### Virtual Filesystem

```cpp
// Virtuelle Verzeichnisstruktur
manager.mkdir("/documents/reports");
manager.import("/path/to/report.pdf", {.virtual_path = "/documents/reports/q4.pdf"});

// Dateien auflisten
auto files = manager.ls("/documents/reports");
```

## Verwandte Dokumentation

- [Content Ingestion](content_ingestion.md) - Ingestion-Pipeline Details
- [Content Processor Plugins](content_processor_plugins.md) - Plugin-Architektur
- [Content Search API](content_search_api.md) - Such-API
- [Content Test Report](content_test_report.md) - Test-Ergebnisse
