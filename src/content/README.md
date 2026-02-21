# Content Module

Content management, ingestion, and processing implementation for ThemisDB.

## Module Purpose

Provides multi-format content ingestion and processing for ThemisDB, handling JSON documents, images, geospatial data, and text extraction with MIME detection and zstd compression.

## Subsystem Scope

**In scope:** Multi-format content ingestion (JSON, images, documents), MIME type detection, text extraction and processing, image metadata extraction, geospatial data processing, zstd compression.

**Out of scope:** PDF/binary format parsing (planned), full-text indexing (handled by search module), vector embedding generation (handled by LLM/RAG modules).

## Relevant Interfaces

- `content_manager.cpp` — orchestrates ingestion pipeline
- `content_type.cpp` — MIME detection and type classification
- `text_processor.cpp` — text extraction
- `image_processor.cpp` — image metadata
- `pipeline/` — processing stage pipeline

## Current Delivery Status

**Maturity:** 🟡 Beta — Core content ingestion and processing operational; PDF/OCR pipeline and streaming ingestion planned.

## Components

- Content manager
- Content type detection
- Text processors
- Image processors
- Geo processors
- Content ingestion pipeline

## Features

- Multi-format content ingestion (JSON, images, documents)
- MIME type detection
- Text extraction and processing
- Image metadata extraction
- Geospatial data processing
- Content compression (zstd)

## Documentation

For content documentation, see:
- [Content Manager](../../docs/src/content/content_manager.cpp.md)
- [Content Type](../../docs/src/content/content_type.cpp.md)
- [Text Processor](../../docs/src/content/text_processor.cpp.md)
- [Content Architecture](../../docs/content_architecture.md)
- [Content Pipeline](../../docs/content_pipeline.md)
- [Content Processors](../../docs/content/)
