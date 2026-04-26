# Archive Ingestion Plugin

## Overview

Archive ingestion is implemented as an **optional convenience plugin** for ThemisDB. 

**Important Design Principle**: ThemisDB is primarily a **storage and query engine** that expects **pre-processed, complete data** including:
- Relational structures
- Document representations  
- Graph relationships
- Vector embeddings
- Geospatial data
- Timeline/temporal data
- Process/workflow data

Archive extraction and processing should ideally happen **externally** before data reaches ThemisDB.

## Architecture Philosophy

### Primary API: `importContent()`
The main interface expects **fully processed data**:
```json
{
  "content": {
    "id": "uuid",
    "mime_type": "application/pdf",
    "category": "TEXT",
    "metadata": {...}
  },
  "chunks": [
    {
      "id": "uuid",
      "text": "...",
      "embedding": [0.1, 0.2, ...],
      "metadata": {...}
    }
  ],
  "edges": [
    {
      "from": "content:uuid1",
      "to": "content:uuid2",
      "type": "REFERENCES"
    }
  ]
}
```

### Optional Plugin: `ingestRawBlob()`
A **convenience layer** for simple scenarios:
- Quick testing and development
- Simple file uploads without external pipeline
- **Not recommended for production** with complex requirements

## Plugin Design Philosophy

ThemisDB follows a plugin architecture where:
- **Core functionality** works with pre-processed data
- **Content processors** are optional convenience plugins
- System gracefully degrades when plugins are not available
- **External processing pipelines are preferred** for production

## Recommended Production Architecture

```
┌─────────────────────────────────────────────────┐
│  External Processing Pipeline                   │
│  ─────────────────────────────────────          │
│  1. Archive Extraction (unzip, tar, etc.)      │
│  2. Content Detection & Classification          │
│  3. Text Extraction (PDF, Office, etc.)        │
│  4. Chunking Strategy                           │
│  5. Embedding Generation (LLM/CLIP)            │
│  6. Graph Relationship Discovery               │
│  7. Metadata Enrichment                        │
└─────────────────────────────────────────────────┘
                     │
                     ▼ JSON
┌─────────────────────────────────────────────────┐
│  ThemisDB: importContent()                      │
│  ─────────────────────────                      │
│  • Store content metadata                       │
│  • Store chunks with embeddings                 │
│  • Create graph edges                           │
│  • Index for search (vector, fulltext, etc.)   │
└─────────────────────────────────────────────────┘
```

## When to Use Archive Plugin

### ✅ Good Use Cases
- **Development & Testing**: Quick file uploads during development
- **Simple Deployments**: Small-scale deployments without complex requirements
- **Prototyping**: Rapid prototyping and demos
- **Single-User Systems**: Personal knowledge bases

### ❌ Not Recommended For
- **Production Pipelines**: Use external processing (Apache NiFi, Airflow, etc.)
- **High-Volume Ingestion**: Batch processing should be external
- **Complex Transformations**: Custom logic better in dedicated pipeline
- **Distributed Processing**: Archive extraction doesn't scale horizontally

## External Processing Pipeline Examples

### Example 1: Python Pipeline
```python
import zipfile
from themis_client import ThemisDB

def process_archive(archive_path):
    # 1. Extract archive (external)
    with zipfile.ZipFile(archive_path) as zf:
        zf.extractall('/tmp/extracted')
    
    # 2. Process each file (external)
    for file_path in extracted_files:
        # Extract text, generate embeddings, etc.
        content, chunks, edges = process_file(file_path)
        
        # 3. Import to ThemisDB (structured data)
        themis = ThemisDB('localhost:8080')
        themis.import_content({
            'content': content,
            'chunks': chunks,
            'edges': edges
        })
```

### Example 2: Apache NiFi Flow
```
ExtractZipContent → DetectContentType → ExtractText →
GenerateEmbeddings → CreateGraphEdges → ImportToThemisDB
```

### Example 3: Airflow DAG
```python
from airflow import DAG
from airflow.operators.python import PythonOperator

dag = DAG('archive_processing')

extract = PythonOperator(task_id='extract', python_callable=extract_archive)
process = PythonOperator(task_id='process', python_callable=process_files)
import_db = PythonOperator(task_id='import', python_callable=import_to_themis)

extract >> process >> import_db
```

## Archive Plugin (Convenience Layer)

If you still want to use the built-in archive plugin for simple cases:

### Configuration

### Build-Time (CMake)
```cmake
# Enable archive processing (default: ON for convenience)
option(THEMIS_ENABLE_ARCHIVES "Enable archive processing plugin" ON)

# Archive support requires libzip
if(THEMIS_ENABLE_ARCHIVES)
    find_package(libzip REQUIRED)
    target_compile_definitions(themisdb PRIVATE THEMIS_ENABLE_ARCHIVES=1)
endif()
```

### Runtime (Configuration)
Archive behavior is controlled via `ContentManager::ingestRawBlob()` config parameter:

```json
{
  "archive_strategy": "EXTRACT_AND_INGEST",  // or "METADATA_ONLY" or "REJECT"
  "encrypted_policy": "REJECT",              // or "METADATA_ONLY" or "REQUIRE_PASSWORD"
  "password": "optional_password_for_encrypted_archives"
}
```

### Strategies

#### 1. EXTRACT_AND_INGEST (Default)
- Extracts all files from archive
- Ingests each file individually
- Creates graph edges: `archive --[CONTAINS]--> extracted_file`
- Supports recursive extraction (nested archives)

#### 2. METADATA_ONLY
- Stores archive as blob without extraction
- Extracts only metadata (file list, sizes, etc.)
- No individual file ingestion
- Lower resource usage

#### 3. REJECT
- Rejects all archive uploads
- Returns error to client
- Use for security-sensitive deployments

## Security Features

### Zip Bomb Protection
- Maximum compression ratio limit (default: 100:1)
- Total size limit (default: 10 GB)
- Individual file size limit (default: 1 GB)
- File count limit (default: 10,000 files)

### Path Traversal Prevention
- Sanitizes all file paths
- Rejects paths containing ".."
- Path depth limit (default: 20 levels)
- Path length limit (default: 4096 characters)

### Encrypted Archives
Three policies available:
1. **REJECT** (default): Reject encrypted archives
2. **METADATA_ONLY**: Store encrypted archive without extraction
3. **REQUIRE_PASSWORD**: Accept with password parameter

## Supported Formats

### Currently Implemented
- **ZIP** - Full support via libzip
  - Standard ZIP archives
  - Encrypted ZIP (with password)
  - ZIP64 (large files >4GB)

### Planned (Requires libarchive)
- **TAR** - Uncompressed tar archives
- **TAR.GZ** - Gzip-compressed tar
- **TAR.BZ2** - Bzip2-compressed tar
- **TAR.XZ** - XZ-compressed tar
- **7Z** - 7-Zip archives

## Usage Examples

### HTTP API
```bash
# Upload and extract archive
curl -X POST http://localhost:8080/content/import \
  -H "Content-Type: application/json" \
  -d '{
    "blob_base64": "UEsDBBQAAA...",
    "filename": "documents.zip",
    "config": {
      "archive_strategy": "EXTRACT_AND_INGEST"
    }
  }'

# Upload archive with metadata only
curl -X POST http://localhost:8080/content/import \
  -H "Content-Type: application/json" \
  -d '{
    "blob_base64": "UEsDBBQAAA...",
    "filename": "large_dataset.zip",
    "config": {
      "archive_strategy": "METADATA_ONLY"
    }
  }'

# Upload encrypted archive
curl -X POST http://localhost:8080/content/import \
  -H "Content-Type: application/json" \
  -d '{
    "blob_base64": "UEsDBBQAAA...",
    "filename": "sensitive.zip",
    "config": {
      "archive_strategy": "EXTRACT_AND_INGEST",
      "encrypted_policy": "REQUIRE_PASSWORD",
      "password": "secret123"
    }
  }'
```

### C++ API
```cpp
// Register archive processor (optional - plugin design)
auto archive_processor = std::make_unique<ArchiveProcessor>();
content_manager->registerProcessor(std::move(archive_processor));

// Ingest archive
auto result = content_manager->ingestRawBlob(
    blob,
    "documents.zip",
    "application/zip",
    "user_context",
    json{
        {"archive_strategy", "EXTRACT_AND_INGEST"}
    }
);

if (result.success) {
    std::cout << "Archive ID: " << result.primary_content_id << std::endl;
    std::cout << "Extracted files: " << result.extracted_content_ids.size() << std::endl;
}
```

## Graph Relationships

When `EXTRACT_AND_INGEST` strategy is used:

```
Archive (content:abc123)
  ├─[CONTAINS]→ File1 (content:def456)
  │              └─ metadata: {original_path: "docs/readme.txt", extraction_order: 0}
  ├─[CONTAINS]→ File2 (content:ghi789)  
  │              └─ metadata: {original_path: "data/info.csv", extraction_order: 1}
  └─[CONTAINS]→ NestedArchive (content:jkl012)
                 └─[CONTAINS]→ NestedFile (content:mno345)
```

Query extracted files:
```cypher
MATCH (a:content {id: 'abc123'})-[r:CONTAINS]->(f:content)
RETURN f.id, r.original_path
```

## Performance Considerations

### Memory Usage
- Extraction uses temporary disk storage (not RAM)
- Temporary files cleaned up after ingestion
- Configurable size limits prevent resource exhaustion

### Disk I/O
- Sequential extraction minimizes disk seeks
- Temporary directory on fast storage recommended
- Parallel processing not used (simplicity over performance)

### Scalability
- Large archives (>1GB) supported
- ZIP64 format for files >4GB
- Streaming decompression (no full decompression in memory)

## Plugin Registration

Archive processing must be explicitly registered (plugin design):

```cpp
// In HttpServer initialization or ContentManager setup
if (config.enable_archive_processing) {
    ArchiveProcessorConfig arch_config;
    arch_config.strategy = ArchiveStrategy::EXTRACT_AND_INGEST;
    arch_config.max_total_size = 1024ULL * 1024 * 1024 * 10;  // 10 GB
    
    auto processor = std::make_unique<ArchiveProcessor>(arch_config);
    content_manager->registerProcessor(std::move(processor));
    
    THEMIS_INFO("Archive processing plugin enabled");
}
```

## Dependencies

### Required for Archive Support
- **libzip** - ZIP archive handling (already in vcpkg.json)

### Optional (Future)
- **libarchive** - TAR, 7Z support
- **zlib** - Built-in with libzip
- **bz2** - Bzip2 compression
- **lzma** - XZ compression

## Disabling Archive Processing

### At Build Time
```cmake
cmake -DTHEMIS_ENABLE_ARCHIVES=OFF ..
```

### At Runtime
Simply don't register the ArchiveProcessor:
```cpp
// Archive processor NOT registered
// Archives will be stored as blobs with metadata only
```

## Testing

### Unit Tests
```bash
# Run archive processor tests
./build/test/content_test --gtest_filter="ArchiveProcessor*"
```

### Integration Tests
```bash
# Test archive ingestion pipeline
./build/test/integration_test --gtest_filter="ArchiveIngestion*"
```

## Troubleshooting

### "No processor available for archives"
- Archive processor not registered
- Check if `THEMIS_ENABLE_ARCHIVES` was ON during build
- Verify processor registration in initialization code

### "Archive exceeds maximum total size"
- Increase `max_total_size` in ArchiveProcessorConfig
- Or use `METADATA_ONLY` strategy for large archives

### "Archive has suspicious compression ratio"
- Possible zip bomb detected
- Increase `max_compression_ratio` if legitimate
- Default 100:1 ratio limit prevents zip bombs

## Future Enhancements

1. **Streaming Extraction** - Process archives without full download
2. **Parallel Extraction** - Extract multiple files concurrently
3. **Incremental Updates** - Re-extract only changed files
4. **Smart Caching** - Cache extraction results for repeated uploads
5. **Format Auto-Detection** - Better magic byte detection
6. **Libarchive Integration** - Support TAR, 7Z, RAR formats
7. **Cloud Storage Integration** - Extract directly from S3/Azure
8. **Extraction Filters** - Extract only specific file patterns
