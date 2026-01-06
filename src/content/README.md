# Content Module

Content management, ingestion, and processing implementation for ThemisDB.

## Components

- Content manager
- Content type detection
- Text processors
- Image processors
- Geo processors
- **Office document processors (DOCX/XLSX/PPTX)**
- Content ingestion pipeline

## Features

- Multi-format content ingestion (JSON, images, documents)
- MIME type detection
- Text extraction and processing
- Image metadata extraction
- Geospatial data processing
- **Office document extraction (Word, Excel, PowerPoint)**
- Content compression (zstd)

## Office Document Support

ThemisDB supports text extraction and metadata parsing from Microsoft Office documents:

### Supported Formats
- **DOCX** - Word documents (2007+)
- **XLSX** - Excel spreadsheets (2007+)
- **PPTX** - PowerPoint presentations (2007+)
- **ODT/ODS/ODP** - OpenDocument formats
- **RTF** - Rich Text Format (basic support)

### Building with Office Support

Office support requires `libzip` and `pugixml`. Enable with:

```bash
cmake -B build -DTHEMIS_ENABLE_OFFICE=ON
```

Install dependencies via vcpkg:
```bash
vcpkg install libzip pugixml
```

### Features

**DOCX (Word):**
- Paragraph and heading extraction
- Comments and track changes
- Metadata (author, title, dates)
- Word and page count

**XLSX (Excel):**
- Cell data extraction
- Sheet names and count
- Formulas
- Defined names

**PPTX (PowerPoint):**
- Slide text extraction
- Speaker notes
- Slide count
- Presentation metadata
- Per-slide titles and content

### Usage Example

```cpp
#include "content/office_processor.h"

using namespace themis::content;

// Create processor
OfficeProcessor::Config config;
config.extract_text = true;
config.extract_metadata = true;
config.extract_speaker_notes = true;  // PPTX only
auto processor = std::make_unique<OfficeProcessor>(config);

// Extract from document
ContentType content_type;
content_type.mime_type = "application/vnd.openxmlformats-officedocument.presentationml.presentation";
auto result = processor->extract(pptx_blob, content_type);

if (result.ok) {
    std::cout << "Extracted text: " << result.text << std::endl;
    std::cout << "Slide count: " << result.metadata["slide_count"] << std::endl;
}
```

## Documentation

For content documentation, see:
- [Content Manager](../../docs/src/content/content_manager.cpp.md)
- [Content Type](../../docs/src/content/content_type.cpp.md)
- [Text Processor](../../docs/src/content/text_processor.cpp.md)
- [Office Processor](../../include/content/office_processor.h)
- [Content Architecture](../../docs/content_architecture.md)
- [Content Pipeline](../../docs/content_pipeline.md)
- [Content Processors](../../docs/content/)
