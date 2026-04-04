# ingest.py - Python Data Ingestion Script

## Overview

Lightweight Python script for recursive directory scanning and data ingestion into ThemisDB. Supports hash-based deduplication, progress tracking, and metadata extraction for graph, vector, and relational models.

## Use Cases

- Quick data imports without .NET dependencies
- Scripted/automated data ingestion
- CI/CD pipeline integration
- Development environment setup
- Cross-platform data loading

## Requirements

- Python 3.8+
- Optional: `pyyaml` for YAML config (`pip install pyyaml`)
- Optional: `tqdm` for progress bars (`pip install tqdm`)

## Installation

```bash
# No installation required - standalone script
cd /path/to/ThemisDB

# Install optional dependencies
pip install pyyaml tqdm
```

## Basic Usage

```bash
# Basic ingestion
python3 tools/ingest.py --source /path/to/data

# With configuration file
python3 tools/ingest.py --config tools/ingest_config.example.yaml

# With custom options
python3 tools/ingest.py --source /path/to/data \
  --output results.json \
  --include-ext .json .yaml .txt \
  --max-size 50 \
  --verbose
```

## Command-Line Options

```
usage: ingest.py [-h] --source SOURCE [--output OUTPUT]
                 [--config CONFIG] [--include-ext EXTS]
                 [--exclude-ext EXTS] [--max-size MB]
                 [--no-graph] [--no-vector] [--no-relational]
                 [--verbose]

Options:
  --source SOURCE       Source directory to scan
  --output OUTPUT       Output JSON file (default: ingestion_output.json)
  --config CONFIG       YAML configuration file
  --include-ext EXTS    File extensions to include
  --exclude-ext EXTS    File extensions to exclude
  --max-size MB         Maximum file size in MB (default: 100)
  --no-graph            Disable graph metadata extraction
  --no-vector           Disable vector metadata extraction
  --no-relational       Disable relational metadata extraction
  --verbose             Enable verbose logging
```

## Configuration

Create `ingest_config.yaml`:

```yaml
source:
  path: /data/documents
  include_extensions: [.json, .yaml, .csv, .txt]
  max_size_mb: 100

output:
  file: ingestion_output.json

tracking:
  database: ingestion_tracker.db
  skip_processed: true

models:
  graph: true
  vector: true
  relational: true
```

Use with:
```bash
python3 tools/ingest.py --config ingest_config.yaml
```

## Features

- **Hash-Based Deduplication:** SHA-256 hashing, skip already processed files
- **Progress Tracking:** Visual progress bar with `tqdm`
- **SQLite Tracking:** Persistent database of processed files
- **Multi-Format Support:** JSON, YAML, CSV, TXT
- **Metadata Extraction:** Graph entities/relationships, vector content, relational schema
- **Detailed Logging:** `ingestion.log` file with all events

## Output Format

```json
{
  "metadata": {
    "timestamp": "2026-01-12T16:00:00Z",
    "source": "/data/documents",
    "total_files": 1250,
    "processed": 1248,
    "skipped": 2
  },
  "files": [
    {
      "path": "/data/documents/users.json",
      "hash": "a1b2c3d4...",
      "size": 15234,
      "models": {
        "graph": {...},
        "vector": {...},
        "relational": {...}
      }
    }
  ]
}
```

## Performance

| File Count | Typical Time | Memory Usage |
|------------|--------------|--------------|
| 1,000 | ~30 seconds | ~50 MB |
| 10,000 | ~4 minutes | ~150 MB |
| 100,000 | ~35 minutes | ~500 MB |

## Comparison with Themis.IngestionTool

| Feature | ingest.py | Themis.IngestionTool |
|---------|-----------|----------------------|
| **Runtime** | Python 3.8+ | .NET 8.0 |
| **Performance** | Good | Excellent |
| **Dependencies** | Minimal | .NET SDK |
| **Parallel Processing** | Single-threaded | Multi-threaded |
| **Configuration** | YAML | YAML/JSON |
| **Use Case** | Quick scripts | Enterprise import |

**When to use ingest.py:**
- Lightweight scripting
- Python-based pipelines
- No .NET runtime available
- Small to medium datasets

**When to use Themis.IngestionTool:**
- Large datasets (>100k files)
- Need parallel processing
- .NET environment available
- Enterprise deployments

## See Also

- [Themis.IngestionTool](ingestion-tool.md) - .NET enterprise tool
- [INGESTION_TOOL_COMPARISON.md](../../../tools/INGESTION_TOOL_COMPARISON.md)
- [Data Import Guide](../../data/import_guide.md)
