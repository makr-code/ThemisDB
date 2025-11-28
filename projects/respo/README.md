# RESPO - RAG-Enhanced Software Programming Optimizer

[![Status](https://img.shields.io/badge/status-development-yellow)](.)
[![Python](https://img.shields.io/badge/python-3.10+-blue)](.)
[![License](https://img.shields.io/badge/license-MIT-green)](LICENSE)

## 🎯 Übersicht

RESPO ist ein **eigenständiges, on-premise RAG LLM Programmierhilfe-System**:

- **Unabhängig** - Keine Abhängigkeit von spezifischen Datenbanken
- **Pluggable Vector Stores** - ChromaDB, Qdrant, Weaviate, ThemisDB, etc.
- **vLLM** - Hochperformante LLM-Inferenz mit LoRA Support
- **Ohne Vendor-Login** - Vollständig lokale Ausführung
- **Air-Gapped Deployment** - Läuft komplett offline

## 🏗️ Architektur

```
┌─────────────────────────────────────────────────────────────────┐
│                        RESPO API (FastAPI)                       │
├─────────────────────────────────────────────────────────────────┤
│  /chat  │  /complete  │  /explain  │  /review  │  /search      │
└─────────────────────────────────────────────────────────────────┘
                              │
          ┌───────────────────┼───────────────────┐
          ▼                   ▼                   ▼
   ┌─────────────┐     ┌─────────────┐     ┌─────────────┐
   │ RAG Pipeline│     │ vLLM Engine │     │Vector Store │
   │             │     │             │     │ (Pluggable) │
   │ - Retrieval │     │ - Inference │     │             │
   │ - Reranking │     │ - LoRA      │     │ - ChromaDB  │
   │ - Context   │     │ - Streaming │     │ - Qdrant    │
   └─────────────┘     └─────────────┘     │ - Weaviate  │
                                           │ - ThemisDB  │
                                           └─────────────┘
```

## 🚀 Quick Start

### Installation

```bash
# Clone the repository
git clone https://github.com/makr-code/respo.git
cd respo

# Install dependencies
pip install -e .

# Or with optional vector stores
pip install -e ".[qdrant]"    # With Qdrant
pip install -e ".[all]"       # All optional dependencies

# Copy configuration
cp .env.example .env
# Edit .env with your settings

# Start RESPO API
respo server --port 8080
```

### Mit Docker Compose

```bash
cd docker
docker compose up -d
```

## 📡 API Endpoints

| Endpoint | Methode | Beschreibung |
|----------|---------|--------------|
| `/chat` | POST | Interaktiver Chat mit Code-Kontext |
| `/complete` | POST | Code Completion |
| `/explain` | POST | Code Explanation |
| `/review` | POST | Code Review |
| `/search` | POST | Semantic Code Search |
| `/ingest` | POST | Code Repository Indexing |

### Beispiel: Chat

```bash
curl -X POST http://localhost:8080/chat \
  -H "Content-Type: application/json" \
  -d '{
    "message": "Wie implementiere ich einen LRU Cache in Python?",
    "context": {
      "repo": "my-project",
      "language": "python"
    }
  }'
```

## 🔍 GitHub Scraper

RESPO enthält einen leistungsfähigen GitHub Scraper zum Sammeln von Trainingsdaten.

### CLI Befehle

```bash
# Repository scrapen
respo scrape owner/repo -o ./output

# Mit GitHub Token (höhere Rate Limits)
respo scrape owner/repo -t $GITHUB_TOKEN -o ./output

# Repository suchen
respo search "machine learning" --language python --min-stars 1000

# Batch-Scraping aus Datei
respo batch-scrape repos.txt -o ./data -j metadata.json
```

### Beispiele

```bash
# Python-Repositories scrapen
respo scrape python/cpython -o ./python-src -e py

# Top TypeScript Repos finden
respo search "typescript framework" -l typescript -s 5000 -n 50

# Batch-Scraping
cat > repos.txt << EOF
facebook/react
microsoft/TypeScript
rust-lang/rust
EOF
respo batch-scrape repos.txt -o ./training-data -j metadata.json
```

## 📁 Projektstruktur

```
respo/
├── respo/                  # Python Package
│   ├── api/                # FastAPI Endpoints
│   ├── rag/                # RAG Pipeline
│   ├── embedding/          # Embedding Service
│   ├── ingestion/          # Code Ingestion + GitHub Scraper
│   ├── llm/                # vLLM Integration
│   ├── vectorstore/        # Pluggable Vector Stores
│   │   ├── base.py         # Abstract Interface
│   │   ├── chroma.py       # ChromaDB (default)
│   │   ├── qdrant.py       # Qdrant
│   │   ├── weaviate.py     # Weaviate
│   │   └── themis.py       # ThemisDB
│   ├── graph/              # Code Graph Analysis
│   └── utils/              # Utilities
├── training/               # LoRA Training
├── tests/                  # Tests
├── docker/                 # Docker Configs
└── docs/                   # Dokumentation
```

## 🔧 Konfiguration

### Umgebungsvariablen

```bash
# Vector Store (choose one)
VECTOR_STORE=chroma                    # chroma, qdrant, weaviate, themis
CHROMA_PERSIST_DIR=./data/chroma
# QDRANT_URL=http://localhost:6333
# WEAVIATE_URL=http://localhost:8080
# THEMIS_URL=http://localhost:8765

# vLLM
VLLM_URL=http://localhost:8000
VLLM_MODEL=codellama/CodeLlama-13b-Instruct-hf

# Embedding
EMBEDDING_MODEL=microsoft/codebert-base

# Logging
LOG_LEVEL=INFO
```

## 🧪 LoRA Training

```bash
cd training
python train_lora.py --config configs/python.yaml
```

Siehe [training/README.md](training/README.md) für Details.

## 📊 Performance

| Operation | Latenz | Anmerkung |
|-----------|--------|-----------|
| Vector Search | 5-20 ms | ChromaDB/Qdrant |
| Reranking | 100-200 ms | CPU |
| LLM First Token | 500-1000 ms | A100 |
| LLM Total (500 Token) | 3-8 s | A100 |

## 🔒 Security

- ✅ **On-Premise** - Keine Cloud-Abhängigkeiten
- ✅ **Kein Vendor-Login** - Vollständig lokal
- ✅ **DSGVO-konform** - Alle Daten bleiben lokal
- ✅ **Air-Gapped Deployment** - Läuft komplett offline
- ✅ **Unabhängig** - Kein Lock-in zu spezifischen Backends

## 🔌 Vector Store Backends

RESPO unterstützt verschiedene Vector Store Backends:

| Backend | Status | Beschreibung |
|---------|--------|--------------|
| ChromaDB | ✅ Default | Lokale Embedded-Datenbank |
| Qdrant | ✅ Supported | Hochperformante Vector-DB |
| ThemisDB | ✅ Supported | Multi-Model DB mit Graph & Hybrid Search |
| Weaviate | 🔧 Planned | GraphQL-basierte Vector-DB |

## 🔗 ThemisDB Integration

Bei Verwendung von ThemisDB als Backend werden zusätzliche Features freigeschaltet:

### Graph-basierte Code-Analyse

```python
from respo.ingestion import IngestionPipeline

# Ingestion mit Graph-Extraktion
pipeline = IngestionPipeline(
    vector_store=themis_store,
    embedder=embedder,
)

# Repository indexieren - Graph wird automatisch aufgebaut
await pipeline.ingest_github_repo("owner", "repo")

# Dependency-Analyse
deps = await pipeline.analyze_dependencies("module.function_name")
print(f"Dependencies: {deps['dependencies']}")
print(f"Used by: {deps['usages']}")

# Call Graph abrufen
call_graph = await pipeline.get_call_graph("main.process_data", depth=3)
```

### Hybrid Search mit Graph Expansion

```python
from respo.vectorstore.themis import ThemisVectorStore

store = ThemisVectorStore()

# Standard Vector Search
results = await store.search(query_embedding, k=10)

# Hybrid Search: Vector + Keyword + Graph
results = await store.hybrid_search(
    query_embedding=embedding,
    query_text="database connection",
    k=10,
    expand_graph=True,  # Findet auch verwandte Code-Entitäten
    graph_depth=2,
)

# Graph Traversal
callees = await store.graph_traverse(
    start_id="utils.db.connect",
    edge_types=["calls"],
    direction="outgoing",
    depth=3,
)
```

### Extrahierte Beziehungen

| Beziehungstyp | Beschreibung |
|---------------|--------------|
| `imports` | Modul-Import-Beziehungen |
| `calls` | Funktionsaufrufe |
| `inherits` | Klassen-Vererbung |
| `implements` | Interface-Implementierung |
| `uses` | Variablen-/Typen-Nutzung |
| `contains` | Modul/Klasse enthält Funktion |
| `defines` | Klasse definiert Methode |

## 🛠️ Development

```bash
# Tests ausführen
pytest tests/

# Linting
ruff check respo/

# Type Checking
mypy respo/

# Formatierung
black respo/
```

## 📄 Lizenz

MIT License

---

**Status:** Development  
**Version:** 0.1.0  
**Maintainer:** RESPO Team
