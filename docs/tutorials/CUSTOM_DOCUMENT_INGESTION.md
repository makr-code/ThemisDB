# Tutorial: Custom Document Ingestion for Legal LoRA Training

## Introduction

This tutorial guides you through ingesting your own legal documents into ThemisDB for training a customized LoRA adapter. We'll cover:

1. Preparing your documents
2. Configuring data sources
3. Running ingestion
4. Validating results
5. Troubleshooting common issues

## Prerequisites

- ThemisDB installed and running
- Documents to ingest (PDF, DOCX, TXT, or other supported formats)
- Basic understanding of YAML configuration
- For scanned PDFs: Tesseract OCR installed

## Step 1: Prepare Your Documents

### Organize Your Files

Create a directory structure:

```
/data/legal_documents/
├── federal/
│   ├── BGB_sections.pdf
│   ├── StGB_commentary.docx
│   └── ...
├── state/
│   ├── LandVG_Berlin.pdf
│   └── ...
└── internal/
    ├── verwaltungsvorschrift_2024.pdf
    ├── guidance_modal_verbs.docx
    └── ...
```

### File Requirements

#### Supported Formats
- **PDF**: Text-based or scanned (requires OCR)
- **DOCX**: Microsoft Word documents
- **TXT**: Plain text files
- **HTML/XML**: Structured documents
- **JSON**: Structured legal data

#### File Size Limits
- Default: 100 MB per file
- Adjustable in configuration
- For very large files, consider splitting

### Document Quality

For best results:

1. **Scanned PDFs**:
   - Minimum 300 DPI
   - Clear, high-contrast text
   - Proper orientation (not rotated)

2. **Text Documents**:
   - UTF-8 encoding preferred
   - Consistent formatting
   - Minimal boilerplate (headers/footers)

3. **Metadata**:
   - Include in filename if possible: `BGB_§123_2024.pdf`
   - Helps with categorization and tracking

## Step 2: Configure Data Sources

### Basic Configuration

Create or edit `config/ingestion/my_sources.yaml`:

```yaml
filesystem_sources:
  # Internal administrative regulations
  - source_id: "my_verwaltungsvorschriften"
    enabled: true
    type: "filesystem"
    location: "/data/legal_documents/internal"
    priority: 10  # Higher than public data
    description: "Internal administrative regulations"
    options:
      format: "auto"  # Auto-detect format
      recursive: true  # Include subdirectories
      
      # OCR configuration
      ocr_enabled: true
      ocr_language: "deu"  # German
      ocr_dpi: 300
      skip_text_pdfs: true  # Skip OCR if PDF has text
      
      # File filtering
      extensions:
        - ".pdf"
        - ".docx"
      exclude_patterns:
        - "**/backup/**"
        - "**/draft/**"
      min_size_bytes: 100
      max_size_bytes: 104857600  # 100 MB
      
      # Metadata
      extract_metadata: true
```

### Advanced Configuration

#### Multiple Sources with Priorities

```yaml
filesystem_sources:
  # Priority 10: Most important (internal guidance)
  - source_id: "internal_guidance"
    location: "/data/legal_documents/internal"
    priority: 10
    
  # Priority 8: State-level laws
  - source_id: "state_laws"
    location: "/data/legal_documents/state"
    priority: 8
    
  # Priority 5: Federal laws (public)
  - source_id: "federal_laws"
    location: "/data/legal_documents/federal"
    priority: 5
```

Higher priority = processed first = more weight in training.

#### OCR for Legacy Documents

```yaml
options:
  ocr_enabled: true
  ocr_language: "deu+eng"  # German + English
  ocr_dpi: 600  # Higher quality for poor scans
  skip_text_pdfs: false  # Always OCR (legacy docs)
```

#### Selective Ingestion

Only specific files:

```yaml
options:
  extensions:
    - ".pdf"  # Only PDFs
  include_patterns:
    - "**/*_final_*.pdf"  # Only final versions
    - "**/BGB_*.pdf"      # Only BGB sections
  exclude_patterns:
    - "**/backup/**"
    - "**/archive/**"
    - "**/*_draft_*.pdf"
```

## Step 3: Run Ingestion

### Command Line

```bash
# Using example executable
cd examples/legal_lora_training
./train_legal_lora --config my_sources.yaml --phase ingestion

# Or using Python client
python ingest_documents.py --config my_sources.yaml
```

### Programmatic (C++)

```cpp
#include "ingestion/ingestion_manager.h"
#include "ingestion/filesystem_ingester.h"

int main() {
    std::string db = "http://localhost:8529/_db/legal_training";
    
    // Create ingestion manager
    ingestion::IngestionManager mgr(db);
    
    // Configure source
    ingestion::SourceConfig config;
    config.source_id = "my_docs";
    config.type = ingestion::SourceType::FILESYSTEM;
    config.location = "/data/legal_documents/internal";
    config.priority = 10;
    config.options["ocr_enabled"] = "true";
    config.options["ocr_language"] = "deu";
    
    // Register and ingest
    mgr.registerSource(config);
    mgr.setTargetCollection("legal_documents");
    
    auto report = mgr.ingestAll([](const std::string& source, 
                                    size_t processed, 
                                    size_t total,
                                    const std::string& status) {
        std::cout << "[" << source << "] " 
                  << processed << "/" << total 
                  << " - " << status << std::endl;
    });
    
    // Check results
    std::cout << "Documents ingested: " << report.total_documents << std::endl;
    std::cout << "Failures: " << report.total_failures << std::endl;
    
    return 0;
}
```

### Monitoring Progress

The ingestion provides real-time feedback:

```
[my_docs] 0/127 - Starting ingestion...
[my_docs] 10/127 - Processing: verwaltungsvorschrift_2024.pdf
[my_docs] 20/127 - OCR in progress: legacy_regulation_1995.pdf
[my_docs] 50/127 - Halfway complete
[my_docs] 100/127 - Almost done...
[my_docs] 127/127 - Ingestion complete!

Ingestion complete:
  Total documents: 127
  Total failures: 3
  Total time: 45.2s
```

## Step 4: Validate Results

### Check Ingested Documents

Using AQL query:

```sql
FOR doc IN legal_documents
    FILTER doc.source_id == "my_docs"
    COLLECT source = doc.document_type WITH COUNT INTO count
    RETURN {type: source, count: count}
```

Expected output:
```json
[
  {"type": "regulation", "count": 85},
  {"type": "guidance", "count": 32},
  {"type": "case_law", "count": 10}
]
```

### Inspect Document Content

```sql
FOR doc IN legal_documents
    FILTER doc.source_id == "my_docs"
    LIMIT 5
    RETURN {
        title: doc.title,
        type: doc.document_type,
        content_length: LENGTH(doc.content),
        has_embedding: doc.embedding != null
    }
```

### Check for Errors

```sql
FOR log IN ingestion_logs
    FILTER log.source_id == "my_docs"
    FILTER log.status == "failed"
    RETURN {
        document: log.document_path,
        error: log.error_message
    }
```

## Step 5: Advanced Features

### OCR Quality Assessment

After OCR, check quality:

```sql
FOR doc IN legal_documents
    FILTER doc.source_id == "my_docs"
    FILTER doc.metadata.ocr_performed == true
    SORT doc.metadata.ocr_confidence ASC
    LIMIT 10
    RETURN {
        title: doc.title,
        confidence: doc.metadata.ocr_confidence,
        word_count: doc.metadata.word_count
    }
```

Low confidence (<0.7)? Try:
- Increase DPI: `ocr_dpi: 600`
- Pre-process images (contrast, deskew)
- Manual review and correction

### Metadata Enrichment

Extract additional metadata:

```cpp
filesystem_ingester.setMetadataExtraction(true);

// Extracted metadata includes:
// - Author
// - Creation date
// - Document type (from filename or content)
// - Keywords
// - References to other documents
```

### Chunking Large Documents

For documents >10 pages:

```yaml
text_processing:
  enable_chunking: true
  chunk_size: 2000  # characters
  chunk_overlap: 200  # for context continuity
```

This creates:
```
Document → Chunk 1 (chars 0-2000)
        → Chunk 2 (chars 1800-3800)  # 200 overlap
        → Chunk 3 (chars 3600-5600)
        → ...
```

### Deduplication

Avoid duplicate documents:

```yaml
deduplication:
  enabled: true
  method: "content_hash"  # SHA-256 of content
  
  # Or use title matching:
  method: "title"
  fuzzy_threshold: 0.9
```

## Troubleshooting

### Issue 1: OCR Fails

**Symptom**: Error "Tesseract not found" or empty content

**Solution**:
```bash
# Install Tesseract
sudo apt-get install tesseract-ocr tesseract-ocr-deu

# Verify
tesseract --version
tesseract --list-langs
```

### Issue 2: Out of Memory

**Symptom**: Process crashes during ingestion

**Solution**:
```yaml
# Reduce parallel processing
parallel_processing:
  enabled: true
  max_threads: 2  # Instead of 4

# Process in smaller batches
batch_size: 50  # Instead of 100
```

### Issue 3: Slow Ingestion

**Symptom**: Takes >1 minute per document

**Causes & Solutions**:
- **Large PDFs**: Enable chunking
- **Many scanned pages**: Disable OCR for text PDFs
- **Network storage**: Copy to local disk first
- **CPU-bound**: Increase `max_threads`

### Issue 4: Encoding Errors

**Symptom**: Garbled text, special characters broken

**Solution**:
```yaml
text_processing:
  encoding: "utf-8"
  fallback_encodings:
    - "iso-8859-1"
    - "windows-1252"
  normalize_unicode: true
```

### Issue 5: Missing Documents

**Symptom**: Expected 200 files, only 150 ingested

**Debug**:
```cpp
// Check file filter
auto count = ingester.getDocumentCount();
std::cout << "Matching files: " << count << std::endl;

// List filtered files
auto ingester_impl = ingester.getImpl();
for (const auto& file : ingester_impl->listMatchingFiles()) {
    std::cout << file << std::endl;
}
```

## Best Practices

### 1. Start Small
Ingest a small subset first (10-20 documents) to validate:
- Format compatibility
- OCR quality
- Metadata extraction
- Processing time

### 2. Use Priorities
Assign priorities based on importance:
- 10: Critical internal documents
- 7-9: Important references
- 5-6: General legal corpus
- 1-4: Background/context

### 3. Monitor Quality
After ingestion, check:
- Content length distribution
- OCR confidence scores
- Missing metadata fields
- Duplicate detection

### 4. Incremental Updates
Don't re-ingest everything:
```yaml
incremental:
  enabled: true
  check_modified_time: true
  skip_existing: true
```

### 5. Backup First
Before large ingestion:
```bash
# Backup database
themisdb-backup --database legal_training --output backup/

# Or use snapshot
curl -X POST http://localhost:8529/_db/legal_training/_api/snapshot
```

## Next Steps

After ingestion:

1. **Auto-Labeling**: Generate training samples
   ```bash
   ./train_legal_lora --phase labeling
   ```

2. **Graph Enrichment**: Add context
   ```bash
   ./train_legal_lora --phase enrichment
   ```

3. **Training**: Create LoRA adapter
   ```bash
   ./train_legal_lora --phase training
   ```

See main documentation: [LEGAL_LORA_TRAINING_PIPELINE.md](../llm_orchestration/LEGAL_LORA_TRAINING_PIPELINE.md)

## Example: Complete Workflow

```yaml
# 1. Configure sources
filesystem_sources:
  - source_id: "my_internal_docs"
    location: "/data/verwaltung"
    priority: 10
    options:
      ocr_enabled: true
      ocr_language: "deu"

# 2. Run ingestion
./train_legal_lora --config my_config.yaml --phase ingestion

# 3. Validate
FOR doc IN legal_documents 
    FILTER doc.source_id == "my_internal_docs" 
    RETURN COUNT(doc)
# Expected: ~127 documents

# 4. Continue to training
./train_legal_lora --config my_config.yaml --phase all
```

## Resources

- **Main Documentation**: [LEGAL_LORA_TRAINING_PIPELINE.md](../llm_orchestration/LEGAL_LORA_TRAINING_PIPELINE.md)
- **Configuration Reference**: `config/ingestion/sources.yaml`
- **Code Examples**: `examples/legal_lora_training/`
- **API Reference**: See header files in `include/ingestion/`

## Support

Questions? Open an issue:
- GitHub: https://github.com/makr-code/ThemisDB/issues
- Tag: `legal-training` or `ingestion`
