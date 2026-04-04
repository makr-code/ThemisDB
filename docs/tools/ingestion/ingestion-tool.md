# Themis.IngestionTool - Enterprise Data Ingestion Tool

## Overview

**Themis.IngestionTool** is a comprehensive .NET console application for importing data into ThemisDB. It supports recursive directory scanning, hash-based deduplication, parallel processing, and metadata extraction for ThemisDB's graph, vector, and relational models.

## Use Cases

- **Bulk Data Import:** Import large datasets from file systems
- **Migration:** Migrate data from existing systems
- **Initial Setup:** Populate new ThemisDB instances
- **Incremental Updates:** Add new data while avoiding duplicates
- **Development:** Create test datasets from sample files
- **Metadata Extraction:** Extract structured data from various file formats

## Requirements

- **.NET 8.0 SDK** or later
- **Windows, Linux, or macOS**
- **Access to ThemisDB server** (optional - can generate metadata files)
- **Sufficient disk space** for SQLite tracking database

## Installation

```bash
cd tools/Themis.IngestionTool

# Restore dependencies
dotnet restore

# Build application
dotnet build --configuration Release
```

## Basic Usage

### Ingest from Directory

```bash
# Basic ingestion
dotnet run --project Themis.IngestionTool -- \
  --source /path/to/data

# With output file
dotnet run --project Themis.IngestionTool -- \
  --source /path/to/data \
  --output ingestion_results.json
```

### With Configuration File

```bash
dotnet run --project Themis.IngestionTool -- \
  --config ingest_config.yaml
```

## Command-Line Options

```
usage: Themis.IngestionTool [options]

Options:
  --source <path>           Source directory to scan
  --output <file>           Output JSON file (default: ingestion_output.json)
  --config <file>           YAML/JSON configuration file
  --include-ext <exts>      File extensions to include (comma-separated)
  --exclude-ext <exts>      File extensions to exclude
  --max-size <mb>           Maximum file size in MB (default: 100)
  --no-graph                Disable graph model extraction
  --no-vector               Disable vector model extraction
  --no-relational           Disable relational model extraction
  --verbose                 Enable verbose logging
  --dry-run                 Preview without processing
  --workers <n>             Number of parallel workers (default: 4)
  --tracker-db <path>       SQLite tracking database path
```

## Configuration

Create `ingest_config.yaml`:

```yaml
source:
  path: /data/documents
  recursive: true
  follow_symlinks: false

output:
  file: ingestion_output.json
  format: json  # json or yaml

filters:
  include_extensions:
    - .json
    - .yaml
    - .yml
    - .csv
    - .txt
    - .md
  exclude_extensions:
    - .tmp
    - .log
  max_file_size_mb: 100
  exclude_patterns:
    - "*/temp/*"
    - "*/.git/*"
    - "*/node_modules/*"

processing:
  parallel_workers: 8
  batch_size: 100
  enable_deduplication: true
  tracker_db: ingestion_tracker.db

models:
  graph:
    enabled: true
    extract_entities: true
    extract_relationships: true
    entity_types:
      - User
      - Document
      - Organization
  
  vector:
    enabled: true
    extract_text_content: true
    chunk_size: 512
    overlap: 50
  
  relational:
    enabled: true
    infer_schema: true
    table_name_from_filename: true

logging:
  level: Information  # Trace, Debug, Information, Warning, Error
  file: ingestion.log
  console: true

themis:
  server_url: http://localhost:8080
  api_key: ""
  batch_insert: true
  insert_immediately: false  # If false, only generates metadata
```

## Output Format

The tool generates comprehensive metadata in JSON format:

```json
{
  "metadata": {
    "timestamp": "2026-01-12T15:00:00Z",
    "source_path": "/data/documents",
    "total_files": 1523,
    "processed_files": 1520,
    "skipped_files": 3,
    "total_size_bytes": 458392032,
    "processing_time_seconds": 45.2
  },
  "files": [
    {
      "path": "/data/documents/users.json",
      "size": 15234,
      "hash": "a1b2c3d4e5f6...",
      "mime_type": "application/json",
      "processed_at": "2026-01-12T15:00:15Z",
      "models": {
        "graph": {
          "entities": [
            {
              "type": "User",
              "id": "user_123",
              "properties": {
                "name": "Alice",
                "email": "alice@example.com"
              }
            }
          ],
          "relationships": [
            {
              "type": "WORKS_FOR",
              "from": "user_123",
              "to": "org_456",
              "properties": {
                "since": "2024-01-15"
              }
            }
          ]
        },
        "vector": {
          "content": "User profile data for Alice...",
          "chunks": [
            {
              "text": "Alice is a software engineer...",
              "metadata": {
                "chunk_id": 0,
                "position": 0
              }
            }
          ]
        },
        "relational": {
          "table": "users",
          "schema": [
            {"name": "id", "type": "string"},
            {"name": "name", "type": "string"},
            {"name": "email", "type": "string"}
          ],
          "records": [
            {"id": "123", "name": "Alice", "email": "alice@example.com"}
          ]
        }
      }
    }
  ],
  "summary": {
    "by_extension": {
      ".json": 450,
      ".csv": 320,
      ".txt": 750
    },
    "by_model": {
      "graph": {
        "entities": 2345,
        "relationships": 1823
      },
      "vector": {
        "chunks": 15234
      },
      "relational": {
        "tables": 12,
        "records": 45678
      }
    }
  }
}
```

## Supported File Formats

| Format | Extension | Graph | Vector | Relational |
|--------|-----------|-------|--------|------------|
| JSON | .json | ✅ | ✅ | ✅ |
| YAML | .yaml, .yml | ✅ | ✅ | ✅ |
| CSV | .csv | ✅ | ✅ | ✅ |
| Text | .txt, .md | ❌ | ✅ | ❌ |
| XML | .xml | ✅ | ✅ | ✅ |
| TSV | .tsv | ❌ | ✅ | ✅ |

## Advanced Usage

### Incremental Ingestion

```bash
# First run - processes all files
dotnet run -- --source /data --tracker-db tracker.db

# Second run - skips already processed files (based on hash)
dotnet run -- --source /data --tracker-db tracker.db
```

### Parallel Processing

```bash
# Use 16 parallel workers for faster processing
dotnet run -- \
  --source /data \
  --workers 16 \
  --batch-size 200
```

### Selective Model Extraction

```bash
# Only extract vector embeddings
dotnet run -- \
  --source /data \
  --no-graph \
  --no-relational

# Only extract relational data
dotnet run -- \
  --source /data \
  --no-graph \
  --no-vector
```

### Filter by File Type

```bash
# Only process JSON and CSV files
dotnet run -- \
  --source /data \
  --include-ext .json,.csv

# Exclude log files
dotnet run -- \
  --source /data \
  --exclude-ext .log,.tmp
```

### Direct Upload to ThemisDB

```bash
# Process and upload immediately
dotnet run -- \
  --source /data \
  --config config.yaml \
  --insert-immediately

# Config must include themis.server_url
```

## Deduplication

The tool uses SHA-256 hashing for deduplication:

1. **Calculate hash** of each file
2. **Check tracker database** if hash exists
3. **Skip if exists**, process if new
4. **Store hash** in tracker database

### Tracker Database

SQLite database tracking processed files:

```sql
CREATE TABLE processed_files (
  hash TEXT PRIMARY KEY,
  path TEXT NOT NULL,
  processed_at DATETIME NOT NULL,
  size INTEGER,
  status TEXT
);
```

Query tracker:
```bash
sqlite3 ingestion_tracker.db "SELECT COUNT(*) FROM processed_files;"
```

## Performance Tuning

### Optimize for Speed

```yaml
processing:
  parallel_workers: 16  # More workers
  batch_size: 500       # Larger batches
  
models:
  graph:
    enabled: false      # Disable if not needed
  vector:
    enabled: true
    chunk_size: 1024    # Larger chunks
```

### Optimize for Memory

```yaml
processing:
  parallel_workers: 2   # Fewer workers
  batch_size: 50        # Smaller batches
  
models:
  vector:
    chunk_size: 256     # Smaller chunks
```

### Performance Targets

| File Count | Workers | Expected Time |
|------------|---------|---------------|
| 1,000 | 4 | ~30 seconds |
| 10,000 | 8 | ~3 minutes |
| 100,000 | 16 | ~25 minutes |
| 1,000,000 | 32 | ~4 hours |

*Times vary based on file size and complexity*

## Troubleshooting

### Out of Memory

**Symptoms:** Application crashes with OOM error

**Solutions:**
- Reduce `parallel_workers`: `--workers 2`
- Reduce `batch_size` in config
- Process in smaller chunks
- Increase system memory or swap

### Slow Processing

**Symptoms:** Ingestion takes very long

**Solutions:**
- Increase `parallel_workers`: `--workers 16`
- Use SSD for tracker database
- Exclude unnecessary file types
- Disable unused models (graph/vector/relational)

### Database Locked

**Symptoms:** SQLite "database is locked" error

**Solutions:**
```bash
# Reduce workers to avoid SQLite contention
dotnet run -- --source /data --workers 1

# Or use separate tracker per run
dotnet run -- --source /data --tracker-db tracker_$(date +%s).db
```

### Files Skipped

**Symptoms:** Many files shown as "skipped"

**Solutions:**
- Check file size limits: `--max-size 200`
- Verify file extensions: `--include-ext .json,.csv`
- Review exclude patterns in config
- Check file permissions

## Integration

### CI/CD Pipeline

```yaml
# .github/workflows/ingest-data.yml
- name: Ingest Test Data
  run: |
    dotnet run --project tools/Themis.IngestionTool -- \
      --source test-data \
      --output ingestion_results.json \
      --workers 4
      
- name: Upload to ThemisDB
  run: |
    curl -X POST http://themis-server:8080/api/bulk-import \
      -H "Content-Type: application/json" \
      -d @ingestion_results.json
```

### Automated Data Pipeline

```bash
#!/bin/bash
# daily_ingest.sh

DATE=$(date +%Y%m%d)
SOURCE="/mnt/data/incoming"
OUTPUT="ingestion_$DATE.json"

dotnet run --project /opt/themis/tools/Themis.IngestionTool -- \
  --source "$SOURCE" \
  --output "$OUTPUT" \
  --tracker-db /var/lib/themis/tracker.db \
  --workers 16

# Upload results
curl -X POST http://themis-server:8080/api/bulk-import \
  -H "Content-Type: application/json" \
  -d @"$OUTPUT"

# Archive processed files
tar -czf "processed_$DATE.tar.gz" "$SOURCE"
mv "processed_$DATE.tar.gz" /mnt/archive/
rm -rf "$SOURCE"/*
```

## See Also

- [Themis.IngestionTool README](../../../tools/Themis.IngestionTool/README.md) - Detailed tool documentation
- [ingest.py](ingest-py.md) - Python alternative
- [Data Import Guide](../../data/import_guide.md)
- [BaseEntity Principle](../../../BASEENTITY_PRINCIPLE.md)

## License

Part of ThemisDB, licensed under the project's main license.
