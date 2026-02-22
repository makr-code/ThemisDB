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

## Scientific References

1. Robertson, S., & Zaragoza, H. (2009). **The Probabilistic Relevance Framework: BM25 and Beyond**. *Foundations and Trends in Information Retrieval*, 3(4), 333–389. https://doi.org/10.1561/1500000019

2. Salton, G., & McGill, M. J. (1983). **Introduction to Modern Information Retrieval**. McGraw-Hill. ISBN: 978-0-070-54484-0

3. Dublin Core Metadata Initiative. (2012). **DCMI Metadata Terms**. DCMI Recommendation. https://www.dublincore.org/specifications/dublin-core/dcmi-terms/

4. W3C. (2013). **PROV-O: The PROV Ontology**. W3C Recommendation. https://www.w3.org/TR/prov-o/
