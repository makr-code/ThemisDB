# Image Analysis Module — Architecture

<!-- Status: PRODUCTION_READY | validated: 2026-08-10 -->

## Overview

The image analysis module provides computer vision capabilities through a pluggable backend abstraction, enabling multiple vision engines (OCR, object detection) to be used interchangeably within ThemisDB's content indexing and querying pipeline.

## Design Principles

1. **Backend Abstraction:** Multiple vision engines supported through plugin interface
2. **Feature Composition:** Results from different engines combined into unified feature vectors
3. **Caching Strategy:** Frequently-analyzed images cached to avoid redundant processing
4. **Fail-Graceful:** Backend unavailability doesn't block document indexing
5. **Async Processing:** Long-running image analysis can be decoupled from document ingestion

## Architecture Diagram

```
┌─────────────────────────────────────────────────────────────┐
│  ImageProcessor (Main API)                                  │
│  • processImage(image_path) → Result<ImageAnalysisResult>   │
│  • Routes to appropriate backend based on image format      │
└──────────────────────┬──────────────────────────────────────┘
                       │
        ┌──────────────┼──────────────┐
        │              │              │
        ▼              ▼              ▼
   ┌─────────┐  ┌─────────┐  ┌──────────────┐
   │Tesseract│  │YOLOv8   │  │FeatureExtr.  │
   │ OCR     │  │Detection│  │(Embeddings)  │
   │Plugin   │  │Plugin   │  │              │
   └────┬────┘  └────┬────┘  └──────┬───────┘
        │            │              │
        └────────────┼──────────────┘
                     │
                     ▼
        ┌────────────────────────┐
        │  ImageCache (LRU)      │
        │  • Key: image_hash     │
        │  • Value: Results      │
        │  • TTL: configurable   │
        └────────────────────────┘
```

## Core Components

### ImageProcessor (Main Entry Point)

**Purpose:** Unified interface for image analysis with backend routing and result composition.

**Responsibilities:**
- Accept image data (path or buffer)
- Validate image format and integrity
- Route to appropriate analysis backend(s)
- Compose results from multiple backends
- Manage caching layer
- Handle errors and timeouts

**Public API:**
```cpp
class ImageProcessor {
  Result<ImageAnalysisResult> processImage(const std::string& path);
  Result<OCRResult> extractText(const Image& image);
  Result<DetectionResult> detectObjects(const Image& image);
};
```

### OCR Backend (Tesseract)

**Purpose:** Extract text from images with layout analysis and confidence scoring.

**Capabilities:**
- Multi-language text recognition
- Layout analysis (blocks, paragraphs, lines)
- Word-level confidence scores
- Script detection and validation
- Timeout-bounded processing

**Configuration:**
- Languages to support (default: English, French, German, Spanish)
- Confidence threshold for accepting text
- Processing timeout (default: 5 seconds)

### Object Detection Backend (YOLOv8 ONNX)

**Purpose:** Detect and localize objects in images with class labels and confidence.

**Capabilities:**
- ~80 object classes (COCO dataset)
- Bounding box coordinates (normalized 0-1 range)
- Confidence scores per detection
- Non-maximum suppression (NMS) for filtering overlaps
- Batch inference support

**Configuration:**
- Confidence threshold (default: 0.5)
- NMS IoU threshold (default: 0.45)
- Max detections per image (default: 100)
- Processing timeout (default: 10 seconds)

### Feature Extraction

**Purpose:** Generate embeddings from image content for similarity search.

**Approach:**
- Combines features from OCR and detection backends
- Text embeddings (from extracted text)
- Object class embeddings
- Spatial layout embeddings
- Unified vector representation for similarity queries

**Output:**
- Fixed-dimension embedding vector (~384-dim default)
- Suitable for vector search (FAISS, HNSW, etc.)
- Normalized L2 distance for similarity

### Image Cache

**Purpose:** Cache analysis results to avoid reprocessing identical images.

**Strategy:**
- LRU eviction with configurable capacity
- TTL-based expiration (configurable per cache tier)
- Keyed by image content hash (SHA-256)
- Supports fallback to reprocessing on miss

**Configuration:**
- Max cache size (default: 1 GB)
- TTL per result (default: 24 hours)
- Enable/disable per backend

## Data Flow

### Image Ingestion Pipeline

```
Document with Image Attachment
  │
  ├─► ImageProcessor.processImage()
  │
  ├─► Check ImageCache (L1 fast)
  │
  ├─► Backend Dispatch:
  │   ├─► OCR: Tesseract → text + confidence
  │   └─► Detection: YOLOv8 → objects + boxes
  │
  ├─► Feature Extraction:
  │   ├─► Text embeddings
  │   ├─► Object embeddings
  │   └─► Spatial embeddings
  │
  ├─► ImageCache.put() (L1 store)
  │
  └─► ImageAnalysisResult
       ├─► extracted_text
       ├─► detected_objects[]
       │   ├─► class_name
       │   ├─► confidence
       │   └─► bbox {x, y, w, h}
       └─► feature_vector[]
```

## Performance Characteristics

### Target Latencies (P99)

- **OCR Text Extraction:** < 100 ms per image
- **Object Detection:** < 200 ms per image
- **Feature Extraction:** < 50 ms per image
- **Cache Lookup:** < 1 ms
- **End-to-End (cache miss):** < 300 ms

### Resource Consumption

- **Memory per Image:** < 50 MB during processing
- **Model Cache:** ~200 MB (Tesseract + YOLOv8 models)
- **Cache Overhead:** ~100 KB per cached result

### Throughput

- **Batch Processing:** 5-10 images/sec (single-threaded)
- **Concurrent Processing:** Scales with thread pool workers

## Error Handling

### Graceful Degradation

1. **OCR Failure** → Document indexed without text extraction; detection proceeds
2. **Detection Failure** → Document indexed without objects; text extraction proceeds
3. **Cache Failure** → Bypass cache; process normally
4. **Timeout** → Return partial results with timeout flag
5. **Backend Unavailable** → Queue for async processing; return error

### Error Codes (E6200–E6299)

- E6200: Unsupported image format
- E6201: Image corrupted or invalid
- E6202: Backend initialization failed
- E6203: Processing timeout exceeded
- E6204: Insufficient memory for processing

## Integration Points

### Content Indexing Pipeline

Image analysis results are integrated into the document indexing pipeline:
- Extracted text added to document term index
- Detected objects add to faceted navigation
- Feature vectors enable similarity search

### Query Processing

Image queries can use analysis results:
- "Find documents with cats and dogs"
- "Find similar images to this one"
- "Find documents mentioning 'John' in extracted text"

## See Also

- [`ROADMAP.md`](ROADMAP.md) — Implementation phases and deliverables
- [`FUTURE_ENHANCEMENTS.md`](FUTURE_ENHANCEMENTS.md) — Planned features
- [`../../include/image_analysis/image_processor.h`](../../include/image_analysis/image_processor.h) — Public API
