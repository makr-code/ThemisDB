# Building Legal Training Data Database

This guide explains how to build and use the legal training data database, which automatically downloads and imports the HuggingFace `legal-bert-de` dataset into ThemisDB.

## Overview

The legal training data ingestion system downloads German legal text data from HuggingFace and imports it into a RocksDB database for use with ThemisDB's legal document features and LoRA fine-tuning capabilities.

**IMPORTANT**: The original dataset `joelito/legal_mc_de` is no longer available. This system now supports multiple alternative datasets with automatic fallback.

**Primary Dataset**: `joelNiklaus/MultiLegalPile` (German subset)  
**HuggingFace URL**: https://huggingface.co/datasets/joelNiklaus/MultiLegalPile  
**Alternative Sources**: See [Alternative Datasets](#alternative-datasets) below  
**Default samples**: 10,000  
**Language**: German (de)  
**Domain**: Legal

## Quick Start

### Basic Build

```bash
# Enable legal training data build
cmake -B build \
    -DTHEMIS_ENABLE_LLM=ON \
    -DTHEMIS_BUILD_LEGAL_TRAINING_DATA=ON

# Build the legal training data
cmake --build build --target legal_training_data

# Check output
ls -lh build/data/legal_training.db/
```

### Build Options

| CMake Option | Default | Description |
|--------------|---------|-------------|
| `THEMIS_BUILD_LEGAL_TRAINING_DATA` | `OFF` | Enable automatic HuggingFace dataset ingestion |
| `THEMIS_ENABLE_LLM` | `OFF` | Enable LLM features (recommended for legal data usage) |

## Prerequisites

### Required Dependencies

- **Python 3.7+** with pip
- **HuggingFace datasets library**:
  ```bash
  pip install datasets huggingface-hub
  ```
- **RocksDB** (for database import)
- **nlohmann_json** (C++ JSON library)

### Optional

- **HuggingFace Hub token** (for private datasets or rate limiting):
  ```bash
  export HF_TOKEN=your_token_here
  ```

## Alternative Datasets

**⚠️ IMPORTANT**: The original dataset `joelito/legal_mc_de` is no longer available on HuggingFace. The ingestion script now supports multiple alternative sources with automatic fallback.

### Available Datasets (in fallback order)

#### 1. **joelNiklaus/MultiLegalPile** (German subset) - **RECOMMENDED**
- **URL**: https://huggingface.co/datasets/joelNiklaus/MultiLegalPile
- **Description**: Large-scale multilingual legal corpus with comprehensive German legal texts
- **Advantages**: Most comprehensive, actively maintained, large dataset
- **Usage**: Default (no flags needed) or `--dataset joelNiklaus/MultiLegalPile`

#### 2. **joelito/legal_mc_de** - **DEPRECATED**
- **URL**: https://huggingface.co/datasets/joelito/legal_mc_de
- **Status**: ⚠️ No longer available (kept for fallback attempt)
- **Description**: German legal multiple choice questions

#### 3. **elenanereiss/german-ler**
- **URL**: https://huggingface.co/datasets/elenanereiss/german-ler
- **Description**: German legal entity recognition dataset
- **Advantages**: Focused on legal entities, good for NER tasks

### Using Alternative Datasets

**List available datasets**:
```bash
python3 scripts/ingest_legal_training_data.py --list-datasets
```

**Specify a specific dataset**:
```bash
python3 scripts/ingest_legal_training_data.py \
    --dataset joelNiklaus/MultiLegalPile \
    --max-samples 10000
```

**Use a custom local dataset**:
```bash
python3 scripts/ingest_legal_training_data.py \
    --local-file /path/to/your/legal_data.json \
    --max-samples 10000
```

### Custom Dataset Format

If using `--local-file`, your JSON should be in one of these formats:

**Format 1 - Simple list**:
```json
[
  {"text": "Legal text 1..."},
  {"text": "Legal text 2..."}
]
```

**Format 2 - With documents key**:
```json
{
  "documents": [
    {"text": "Legal text 1..."},
    {"text": "Legal text 2..."}
  ]
}
```

**Format 3 - ThemisDB format** (preferred):
```json
{
  "metadata": {"source": "custom", "count": 2},
  "documents": [
    {
      "_key": "legal_001",
      "source": "custom",
      "text": "Legal text...",
      "metadata": {"language": "de", "domain": "legal"}
    }
  ]
}
```

## Build Process

The build process consists of 4 automated steps:

### 1. Download & Ingest HuggingFace Dataset

```bash
# This step is automated by CMake
python3 scripts/ingest_legal_training_data.py \
    --output build/data/legal_training_data.json \
    --max-samples 10000
```

**Duration**: 5-10 minutes (depending on network speed)  
**Output**: JSON file (~50-100 MB)

### 2. Generate RocksDB Importer

```bash
# Automated by CMake
python3 scripts/generate_legal_rocksdb.py \
    --method cpp \
    --output build/data/legal_training.db
```

**Output**: C++ source file for RocksDB import

### 3. Compile Importer

```bash
# Automated by CMake
g++ -std=c++17 build/data/import_legal_rocksdb.cpp \
    -o import_legal_rocksdb \
    -lrocksdb -lnlohmann_json -lpthread
```

### 4. Import to RocksDB

```bash
# Automated by CMake
./import_legal_rocksdb \
    build/data/legal_training_data.json \
    build/data/legal_training.db
```

**Duration**: 1-2 minutes  
**Output**: RocksDB database directory

## Manual Build

If you prefer to run the steps manually:

```bash
# 1. Create data directory
mkdir -p build/data

# 2. Ingest from HuggingFace
python3 scripts/ingest_legal_training_data.py \
    --output build/data/legal_training_data.json \
    --max-samples 10000

# 3. Generate RocksDB importer
python3 scripts/generate_legal_rocksdb.py \
    --method cpp \
    --output build/data/legal_training.db

# 4. Compile importer
g++ -std=c++17 build/data/import_legal_rocksdb.cpp \
    -o build/import_legal_rocksdb \
    -lrocksdb -lnlohmann_json -lpthread

# 5. Import data
./build/import_legal_rocksdb \
    build/data/legal_training_data.json \
    build/data/legal_training.db
```

## Docker Integration

To include legal training data in a Docker image:

```dockerfile
FROM ubuntu:24.04 AS legal-data-builder

# Install dependencies
RUN apt-get update && apt-get install -y \
    python3 python3-pip cmake g++ \
    librocksdb-dev nlohmann-json3-dev

# Install Python dependencies
RUN pip3 install datasets huggingface-hub

# Copy source
COPY . /build

# Build legal training data
WORKDIR /build
RUN cmake -B build -DTHEMIS_BUILD_LEGAL_TRAINING_DATA=ON
RUN cmake --build build --target legal_training_data

# Final image
FROM themisdb/themisdb:latest

# Copy legal training database
COPY --from=legal-data-builder /build/data/legal_training.db /app/data/

# Set environment variable
ENV THEMIS_LEGAL_TRAINING_DB_PATH=/app/data/legal_training.db
```

## Usage in ThemisDB

### Query Legal Training Data

```sql
-- View sample documents
SELECT * FROM :legal_training LIMIT 10;

-- Filter by metadata
SELECT text, metadata 
FROM :legal_training 
WHERE metadata.language = 'de' 
AND metadata.domain = 'legal';

-- Count documents
SELECT COUNT(*) FROM :legal_training;
```

### LoRA Fine-Tuning

```sql
-- Extract training data for LoRA fine-tuning
SELECT text, metadata 
FROM :legal_training 
WHERE metadata.language = 'de' 
FOR TRAINING;
```

### Legal RAG (Retrieval Augmented Generation)

```python
# Use with ThemisDB Python SDK
from themisdb import ThemisClient

client = ThemisClient()
result = client.query("""
    SELECT text 
    FROM :legal_training 
    WHERE text CONTAINS 'Vertragsrecht'
    LIMIT 5
""")
```

## Configuration

Create a configuration file for legal training data:

**File**: `config/legal_training.yaml`

```yaml
legal_training:
  enabled: true
  database:
    path: "data/legal_training.db"
    type: "rocksdb"
    read_only: true
  
  ingestion:
    auto_update: false
    update_interval_hours: 168  # 1 week
    source: "huggingface:joelNiklaus/MultiLegalPile"  # Updated to new source
    fallback_sources:
      - "huggingface:elenanereiss/german-ler"
      - "local_file:custom_legal_data.json"
    max_samples: 10000
  
  usage:
    lora_training: true
    prompt_augmentation: true
    legal_rag: true
```

## Troubleshooting

### Dataset Not Available / 404 Error

**Problem**: The dataset `joelito/legal_mc_de` is no longer available on HuggingFace.

**Solution**: The script now automatically tries alternative datasets in fallback order.

```bash
# Check which datasets are available
python3 scripts/ingest_legal_training_data.py --list-datasets

# Use the recommended dataset explicitly
python3 scripts/ingest_legal_training_data.py \
    --dataset joelNiklaus/MultiLegalPile \
    --max-samples 10000

# Or provide your own dataset
python3 scripts/ingest_legal_training_data.py \
    --local-file custom_data.json
```

### All HuggingFace Datasets Fail

**Problem**: All HuggingFace datasets fail to load.

**Solutions**:
1. **Check internet connection**
2. **Use a local dataset**:
   ```bash
   python3 scripts/ingest_legal_training_data.py --local-file your_data.json
   ```
3. **Try a specific dataset**:
   ```bash
   python3 scripts/ingest_legal_training_data.py --dataset joelNiklaus/german_court_decisions
   ```
4. **Set HuggingFace token** (if rate-limited):
   ```bash
   export HF_TOKEN=your_token
   ```

### Python Dependencies Not Found

```bash
# Install required packages
pip install datasets huggingface-hub
```

### HuggingFace Rate Limiting

```bash
# Set authentication token
export HF_TOKEN=your_huggingface_token

# Or login via CLI
huggingface-cli login
```

### RocksDB Not Found

```bash
# Ubuntu/Debian
sudo apt-get install librocksdb-dev

# macOS
brew install rocksdb

# Windows (vcpkg)
vcpkg install rocksdb
```

### Slow Download

The first download may be slow due to network speed. The dataset is cached locally by HuggingFace, so subsequent builds will be faster.

### Out of Memory

If you encounter memory issues during ingestion:

```bash
# Reduce the number of samples
cmake -B build \
    -DTHEMIS_BUILD_LEGAL_TRAINING_DATA=ON \
    -DLEGAL_MAX_SAMPLES=5000
```

## Performance Notes

- **First build**: 5-10 minutes (dataset download)
- **Subsequent builds**: 2-3 minutes (uses cached data)
- **Storage**: ~50-100 MB (compressed in RocksDB)
- **Memory**: ~500 MB peak during import

## Data Format

### JSON Structure

```json
{
  "metadata": {
    "source": "huggingface:joelNiklaus/MultiLegalPile",
    "version": "2.0.0",
    "count": 10000,
    "max_samples": 10000
  },
  "documents": [
    {
      "_key": "legal_de_000001",
      "source": "huggingface:joelNiklaus/MultiLegalPile",
      "text": "Legal text content...",
      "metadata": {
        "index": 1,
        "dataset": "joelNiklaus/MultiLegalPile",
        "language": "de",
        "domain": "legal"
      }
    }
  ]
}
```

## Comparison with docs.db

| Feature | docs.db | legal_training.db |
|---------|---------|-------------------|
| **Source** | Local `docs/` + `compendium/` | HuggingFace (multiple sources with fallback) |
| **Primary Dataset** | N/A | `joelNiklaus/MultiLegalPile` |
| **Build Flag** | `THEMIS_BUILD_DOCS_DB` | `THEMIS_BUILD_LEGAL_TRAINING_DATA` |
| **Default** | ON | OFF (opt-in) |
| **Size** | ~2-3 MB | ~50-100 MB |
| **Documents** | 1151 | 10000 |
| **Language** | Multi-language | German |
| **Purpose** | Documentation | Legal training data |

## License & Attribution

### Primary Dataset: joelNiklaus/MultiLegalPile
- **Dataset**: `joelNiklaus/MultiLegalPile` (German subset)
- **HuggingFace URL**: https://huggingface.co/datasets/joelNiklaus/MultiLegalPile
- **License**: Check HuggingFace dataset page for license information
- **Citation**: Please cite the original dataset when using this data

### Alternative Datasets
- **elenanereiss/german-ler**: https://huggingface.co/datasets/elenanereiss/german-ler
- **joelito/legal_mc_de** (DEPRECATED): https://huggingface.co/datasets/joelito/legal_mc_de (no longer available)

### Important Notes
⚠️ The original dataset `joelito/legal_mc_de` referenced in earlier versions is no longer available on HuggingFace. This system now uses `joelNiklaus/MultiLegalPile` as the primary source with automatic fallback to alternative datasets.

## See Also

- [Documentation Database Build](../../../README.md)
- [LLM Integration Guide](../../llm/README.md)
- [LoRA Fine-Tuning](../../llm/lora.md)
