# Automatic Fulltext Index for Ingested Documents

## Overview

ThemisDB now supports automatic creation of fulltext indices for ingested documents, enabling fast keyword-based search on document content without manual index configuration.

## Feature Description

When documents are ingested into ThemisDB through the `ContentManager`, text chunks are automatically indexed in a BM25-based fulltext index. This enables efficient keyword search across all ingested documents.

### Key Benefits

1. **Zero Configuration**: No need to manually create fulltext indices after document ingestion
2. **Fast Search**: BM25 scoring provides relevant results with proper ranking
3. **Language Support**: Configurable language-specific stemming and stopword removal
4. **Automatic Cleanup**: Fulltext index entries are automatically removed when content is deleted
5. **Configurable**: Can be enabled/disabled and fine-tuned per deployment

## Configuration

The automatic fulltext indexing feature is controlled through the `config:content` key in RocksDB:

```json
{
  "auto_fulltext_index": true,
  "fulltext_config": {
    "language": "en",
    "stemming_enabled": true,
    "stopwords_enabled": true,
    "normalize_umlauts": false,
    "stopwords": []
  }
}
```

### Configuration Options

| Option | Type | Default | Description |
|--------|------|---------|-------------|
| `auto_fulltext_index` | boolean | `false` | Enable automatic fulltext index creation |
| `fulltext_config.language` | string | `"none"` | Language for stemming: `"en"`, `"de"`, `"none"` |
| `fulltext_config.stemming_enabled` | boolean | `false` | Enable word stemming (e.g., "running" → "run") |
| `fulltext_config.stopwords_enabled` | boolean | `false` | Filter common stopwords (the, a, an, etc.) |
| `fulltext_config.normalize_umlauts` | boolean | `false` | Normalize German umlauts (ä→a, ö→o, ü→u, ß→ss) |
| `fulltext_config.stopwords` | array | `[]` | Custom stopword list (lowercase) |

## Usage Examples

### 1. Enable Automatic Fulltext Indexing

Store the configuration in RocksDB:

```cpp
#include "storage/rocksdb_wrapper.h"
#include <nlohmann/json.hpp>

json config = {
    {"auto_fulltext_index", true},
    {"fulltext_config", {
        {"language", "en"},
        {"stemming_enabled", true},
        {"stopwords_enabled", true}
    }}
};

std::string config_str = config.dump();
storage->put("config:content", 
             std::vector<uint8_t>(config_str.begin(), config_str.end()));
```

### 2. Ingest Documents

Once enabled, documents are automatically indexed during ingestion:

```cpp
#include "content/content_manager.h"

// Create content specification
json spec = {
    {"content", {
        {"id", "doc-123"},
        {"mime_type", "text/plain"},
        {"original_filename", "report.txt"}
    }},
    {"chunks", {
        {
            {"id", "chunk-1"},
            {"text", "Machine learning algorithms analyze data patterns."},
            {"seq_num", 0}
        },
        {
            {"id", "chunk-2"},
            {"text", "Deep learning uses neural networks for pattern recognition."},
            {"seq_num", 1}
        }
    }}
};

// Import content (fulltext index is automatically updated)
auto status = content_manager->importContent(spec);
```

### 3. Search Indexed Content

Use the `SecondaryIndexManager` to search the fulltext index:

```cpp
#include "index/secondary_index.h"

// Search for documents containing "machine learning"
auto [status, results] = secondary_index->scanFulltextWithScores(
    "chunk",           // table name
    "text",            // column name
    "machine learning", // query
    10                 // limit
);

if (status.ok) {
    for (const auto& result : results) {
        std::cout << "Chunk ID: " << result.pk 
                  << ", Score: " << result.score << std::endl;
    }
}
```

### 4. Advanced Search Features

The fulltext index supports various search capabilities:

**Phrase Search** (exact phrase matching):
```cpp
auto [status, results] = secondary_index->scanFulltextPhrase(
    "chunk", "text", "deep learning algorithms", 10);
```

**Fuzzy Search** (with Levenshtein distance):
```cpp
auto [status, results] = secondary_index->scanFulltextFuzzy(
    "chunk", "text", "machene lerning", 2, 10);  // max distance = 2
```

## Implementation Details

### Index Structure

- **Table**: `chunk`
- **Column**: `text`
- **Index Type**: Inverted index with BM25 scoring
- **Key Schema**: `ftidx:chunk:text:token:chunk_id`

### Processing Flow

1. **Import Content**: `ContentManager::importContent()` is called
2. **Load Config**: Read `config:content` to check if `auto_fulltext_index` is enabled
3. **Create Index**: If enabled and index doesn't exist, create fulltext index on `chunk.text`
4. **Store Chunks**: Each chunk is stored in RocksDB under `chunk:<id>`
5. **Index Text**: For each chunk with non-empty text, create a `BaseEntity` and add to fulltext index
6. **Tokenization**: Text is tokenized, stemmed (if enabled), and stored in inverted index

### Cleanup on Deletion

When content is deleted via `ContentManager::deleteContent()`:

1. Load all chunks for the content
2. Delete chunk records from RocksDB
3. Remove chunks from vector index (if present)
4. **Remove chunks from fulltext index** (if fulltext index exists)

## Performance Considerations

### Indexing Performance

- **Token Extraction**: O(n) where n is text length
- **Index Update**: O(log m) where m is index size (RocksDB tree)
- **Batch Operations**: Use `SecondaryIndexManager::putBatch()` for bulk ingestion

### Search Performance

- **Query**: O(k * log n) where k is number of tokens, n is vocabulary size
- **Ranking**: BM25 scoring computed for all matching documents
- **Result Limit**: Use reasonable limits (10-100) for best performance

### Storage Overhead

- **Index Size**: ~20-30% of original text size (varies with language and stemming)
- **Compression**: RocksDB compression reduces storage overhead
- **Sparse Index**: Can use sparse index to skip NULL values

## Migration Guide

### Existing Deployments

For existing deployments with already-ingested documents:

1. **Enable Feature**: Update `config:content` with `auto_fulltext_index: true`
2. **Rebuild Index**: Use `SecondaryIndexManager::rebuildIndex()` to index existing content:

```cpp
secondary_index->rebuildIndex("chunk", "text", 
    [](size_t done, size_t total) {
        std::cout << "Progress: " << done << "/" << total << std::endl;
        return true;  // continue
    }
);
```

3. **Verify**: Check index stats and test searches

## Troubleshooting

### Index Not Created

**Symptom**: Documents are ingested but search returns no results

**Solutions**:
1. Verify `config:content` has `auto_fulltext_index: true`
2. Check logs for index creation messages
3. Manually verify index exists: `secondary_index->hasFulltextIndex("chunk", "text")`

### Poor Search Quality

**Symptom**: Search returns irrelevant results or misses relevant documents

**Solutions**:
1. Enable stemming: `stemming_enabled: true`
2. Configure correct language: `language: "en"` or `"de"`
3. Enable stopword filtering: `stopwords_enabled: true`
4. Review BM25 scoring parameters

### Slow Indexing

**Symptom**: Document ingestion is slower after enabling feature

**Solutions**:
1. Use batch operations for bulk ingestion
2. Disable stemming if not needed
3. Consider sparse index for documents with many empty chunks
4. Monitor RocksDB compaction settings

## API Reference

### ContentManager

```cpp
// Automatic fulltext indexing happens in importContent()
Status importContent(const json& spec, 
                     const std::optional<std::string>& blob = std::nullopt,
                     const std::string& user_context = "");

// Cleanup also removes from fulltext index
Status deleteContent(const std::string& content_id);
```

### SecondaryIndexManager

```cpp
// Check if fulltext index exists
bool hasFulltextIndex(std::string_view table, std::string_view column) const;

// Search fulltext index
std::pair<Status, std::vector<FulltextResult>> scanFulltextWithScores(
    std::string_view table,
    std::string_view column,
    std::string_view query,
    size_t limit = 1000) const;

// Rebuild index for existing data
void rebuildIndex(const std::string& table, const std::string& column,
                  std::function<bool(size_t,size_t)> progress);
```

## Testing

Comprehensive tests are available in `tests/test_content_fulltext_index.cpp`:

```bash
# Build and run tests
cd build
cmake .. -DTHEMIS_BUILD_TESTS=ON -DTHEMIS_ENABLE_CONTENT=ON
make test_content_fulltext_index
./test_content_fulltext_index
```

Test coverage includes:
- Auto-index creation when enabled
- Index search functionality
- Multi-chunk document search
- Stemming support
- Deletion cleanup
- Language support (English, German)
- Empty chunk handling
- Multi-document indexing

## Related Features

- **Vector Search**: Combine with vector embeddings for hybrid search
- **Graph Search**: Use graph relationships for document navigation
- **Content Processing**: Automatic text extraction from various formats (PDF, DOCX, etc.)
- **Hybrid Search**: Combine fulltext (BM25) and vector (HNSW) with RRF fusion

## References

- BM25 Algorithm: https://en.wikipedia.org/wiki/Okapi_BM25
- SecondaryIndexManager: `include/index/secondary_index.h`
- ContentManager: `include/content/content_manager.h`
- Fulltext Search Tests: `tests/test_fulltext_phrase_fuzzy.cpp`
