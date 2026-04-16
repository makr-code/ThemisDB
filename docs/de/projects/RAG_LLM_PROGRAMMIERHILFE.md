# RAG LLM Programmierhilfe - RESPO

**Projektname:** RESPO (RAG-Enhanced Software Programming Optimizer)  
**Version:** 1.0  
**Stand:** 6. April 2026  
**Typ:** Recherche & Umsetzungsplan  
**Projektpfad:** `projects/respo/`

> **Hinweis:** RESPO ist ein **eigenständiges Projekt**, unabhängig von ThemisDB.
> Es unterstützt verschiedene Vector Store Backends (ChromaDB, Qdrant, Weaviate, ThemisDB).

---

## 📋 Executive Summary

Dieses Dokument beschreibt die Konzeption und Implementierung eines **on-premise RAG-basierten LLM-Systems für Programmierhilfe**, das folgende Kernmerkmale bietet:

- **Kein Vendor-Login** - Vollständig lokale Ausführung
- **Pluggable Vector Stores** - ChromaDB (default), Qdrant, Weaviate, ThemisDB
- **vLLM als Inference-Engine** - Hochperformante LLM-Inferenz
- **LoRA Fine-Tuning** - Anpassung an spezifische Programmierdomänen
- **Enterprise-Ready** - DSGVO-konform, air-gapped deployment möglich
- **Unabhängig** - Keine feste Abhängigkeit von spezifischen Datenbanken

---

## 🏗️ Architektur-Konzept

### High-Level Architektur

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           RESPO - RAG LLM Programmierhilfe                   │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌───────────────┐    ┌───────────────┐    ┌───────────────────────────┐   │
│  │   Frontend    │    │   IDE Plugin  │    │   CLI / API Client        │   │
│  │   (Web UI)    │    │   (VSCode)    │    │                           │   │
│  └───────┬───────┘    └───────┬───────┘    └─────────────┬─────────────┘   │
│          │                    │                          │                  │
│          └────────────────────┼──────────────────────────┘                  │
│                               │                                              │
│                               ▼                                              │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │                     RESPO API Gateway (FastAPI)                      │   │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌───────────┐   │   │
│  │  │ /chat       │  │ /complete   │  │ /explain    │  │ /search   │   │   │
│  │  │ /review     │  │ /refactor   │  │ /document   │  │ /ingest   │   │   │
│  │  └─────────────┘  └─────────────┘  └─────────────┘  └───────────┘   │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                               │                                              │
│          ┌────────────────────┼────────────────────┐                        │
│          │                    │                    │                        │
│          ▼                    ▼                    ▼                        │
│  ┌───────────────┐    ┌───────────────┐    ┌───────────────────────────┐   │
│  │ RAG Pipeline  │    │ vLLM Engine   │    │ ThemisDB                  │   │
│  │               │◄──►│               │    │ (Vektor + Graph + Doc)    │   │
│  │ - Retrieval   │    │ - Inference   │◄──►│                           │   │
│  │ - Reranking   │    │ - LoRA        │    │ - Code Embeddings         │   │
│  │ - Context     │    │ - Streaming   │    │ - Dokumentation           │   │
│  │   Assembly    │    │               │    │ - Projekt-Graphen         │   │
│  └───────────────┘    └───────────────┘    └───────────────────────────┘   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### Komponenten-Übersicht

| Komponente | Technologie | Funktion |
|------------|-------------|----------|
| **API Gateway** | FastAPI (Python) | REST/WebSocket API, Request Routing |
| **RAG Pipeline** | LangChain/LlamaIndex | Retrieval, Reranking, Context Assembly |
| **vLLM Engine** | vLLM + LoRA | LLM Inferenz mit Fine-Tuned Adaptern |
| **Vektor Store** | ThemisDB HNSW | Code-Embeddings, Semantic Search |
| **Graph Store** | ThemisDB Graph | Projektstruktur, Code-Dependencies |
| **Document Store** | ThemisDB Entities | Dokumentation, Konfigurationen |
| **Embedding Service** | sentence-transformers | Code-spezifische Embeddings |

---

## 🔬 Technologie-Stack Analyse

### 1. ThemisDB als Wissensspeicher

ThemisDB eignet sich ideal für dieses Projekt aufgrund:

#### Vorteile
- ✅ **Multi-Model-Architektur** - Vektor, Graph und Dokumente in einem System
- ✅ **HNSW Persistence** - Produktionsreife Vektor-Suche (1.800 q/s)
- ✅ **Graph Traversals** - Code-Dependency-Analyse (BFS, Dijkstra)
- ✅ **ACID Transactions** - Konsistente Ingestion
- ✅ **Hybrid Search** - Kombination von Vektor + Metadaten-Filter
- ✅ **On-Premise** - Kein externer Dienst erforderlich
- ✅ **Existierende Adapter** - VCC-Base Library als Grundlage

#### ThemisDB-Nutzung für RAG

```python
# Code-Chunk mit Embedding speichern
await themis.put_entity(
    key=f"code:{repo}:{file_path}:{chunk_id}",
    blob={
        "content": code_chunk,
        "language": "python",
        "file_path": file_path,
        "function_name": "process_data",
        "embedding": embedding_vector,  # 768-dim
        "metadata": {
            "repo": repo,
            "commit": commit_sha,
            "created_at": timestamp
        }
    }
)

# Semantic Search
results = await themis.vector_search(
    vector=query_embedding,
    k=10,
    filter={"language": "python", "repo": "my-project"}
)

# Graph Traversal für Dependencies
dependencies = await themis.graph_traverse(
    start_vertex=f"function:{module}:{function_name}",
    max_depth=3,
    direction="OUTBOUND"
)
```

### 2. vLLM als Inference-Engine

#### Warum vLLM?
- ✅ **PagedAttention** - Effizientes GPU Memory Management
- ✅ **Continuous Batching** - Hoher Durchsatz
- ✅ **LoRA Support** - Fine-Tuning ohne Full-Model-Training
- ✅ **OpenAI-kompatible API** - Einfache Integration
- ✅ **Quantization** - AWQ, GPTQ, INT8 für kleinere GPUs
- ✅ **Streaming** - Token-by-Token Response

#### Hardware-Anforderungen

| Modell | VRAM (Mindestens) | VRAM (Empfohlen) | Quantization |
|--------|-------------------|------------------|--------------|
| CodeLlama-7B | 8 GB | 16 GB | AWQ/GPTQ |
| CodeLlama-13B | 16 GB | 24 GB | AWQ/GPTQ |
| CodeLlama-34B | 48 GB | 80 GB | AWQ |
| DeepSeek-Coder-33B | 48 GB | 80 GB | AWQ |
| StarCoder2-15B | 24 GB | 40 GB | AWQ |

#### vLLM Server Konfiguration

```bash
# vLLM Server starten mit LoRA
python -m vllm.entrypoints.openai.api_server \
    --model codellama/CodeLlama-13b-Instruct-hf \
    --enable-lora \
    --lora-modules respo-python=/models/lora/respo-python \
                   respo-typescript=/models/lora/respo-typescript \
    --max-lora-rank 64 \
    --tensor-parallel-size 2 \
    --gpu-memory-utilization 0.9 \
    --max-model-len 8192 \
    --port 8000
```

### 3. LoRA Fine-Tuning Strategie

#### Trainings-Daten

| Datenquelle | Inhalt | Zweck |
|-------------|--------|-------|
| **Eigener Code** | Interne Repositories | Domänen-Anpassung |
| **Dokumentation** | Projekt-Docs, READMEs | Kontext-Verständnis |
| **Stack Overflow** | Q&A Paare | Problemlösung |
| **GitHub Issues** | Bug Reports + Fixes | Debugging-Fähigkeiten |
| **Code Reviews** | Review-Kommentare | Best Practices |

#### LoRA Hyperparameter

```yaml
# LoRA Training Config
lora_config:
  r: 64                    # LoRA rank
  lora_alpha: 128          # Alpha scaling
  lora_dropout: 0.05       # Dropout rate
  target_modules:          # Welche Module trainiert werden
    - q_proj
    - k_proj
    - v_proj
    - o_proj
    - gate_proj
    - up_proj
    - down_proj

training_config:
  learning_rate: 2e-4
  num_epochs: 3
  batch_size: 4
  gradient_accumulation_steps: 8
  warmup_ratio: 0.03
  max_seq_length: 4096
  bf16: true               # Falls GPU unterstützt
```

#### Trainings-Pipeline

```
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│   Data Prep     │────►│   LoRA Train    │────►│   Evaluation    │
│                 │     │                 │     │                 │
│ - Sammeln       │     │ - HuggingFace   │     │ - HumanEval     │
│ - Bereinigen    │     │ - PEFT Library  │     │ - MBPP          │
│ - Formatieren   │     │ - DeepSpeed     │     │ - Custom Tests  │
└─────────────────┘     └─────────────────┘     └─────────────────┘
                               │
                               ▼
                        ┌─────────────────┐
                        │   vLLM Deploy   │
                        │                 │
                        │ - LoRA Adapter  │
                        │ - Hot-Reload    │
                        └─────────────────┘
```

---

## 📊 Embedding-Strategie für Code

### Code-spezifische Embedding-Modelle

| Modell | Dimensionen | Besonderheiten |
|--------|-------------|----------------|
| **CodeBERT** | 768 | Microsoft, gutes Code-Verständnis |
| **StarEncoder** | 1024 | BigCode, multi-lingual |
| **UniXcoder** | 768 | Microsoft, Code-Doc aligned |
| **CodeT5+** | 768 | Salesforce, encoder-decoder |
| **Nomic-Embed-Code** | 768 | Optimiert für Code |

### Chunking-Strategie für Code

```python
class CodeChunker:
    """Intelligentes Chunking für Quellcode."""
    
    CHUNK_STRATEGIES = {
        "function": FunctionChunker,      # Jede Funktion = 1 Chunk
        "class": ClassChunker,            # Jede Klasse = 1 Chunk
        "semantic": SemanticChunker,      # AST-basiert
        "sliding": SlidingWindowChunker,  # Überlappende Fenster
    }
    
    def chunk(self, code: str, language: str) -> List[CodeChunk]:
        # 1. AST parsen
        tree = parse_code(code, language)
        
        # 2. Semantische Einheiten extrahieren
        units = extract_semantic_units(tree)
        
        # 3. Chunks mit Kontext erstellen
        chunks = []
        for unit in units:
            chunks.append(CodeChunk(
                content=unit.source,
                type=unit.type,  # function, class, method
                name=unit.name,
                signature=unit.signature,
                docstring=unit.docstring,
                imports=unit.imports,
                dependencies=unit.dependencies,
            ))
        
        return chunks
```

### Embedding-Pipeline

```
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│   Source Code   │────►│   AST Parser    │────►│   Chunker       │
│                 │     │   (tree-sitter) │     │                 │
└─────────────────┘     └─────────────────┘     └────────┬────────┘
                                                         │
                                                         ▼
┌─────────────────┐     ┌─────────────────┐     ┌─────────────────┐
│   ThemisDB      │◄────│   Embedder      │◄────│   Code Chunks   │
│   Vector Store  │     │   (CodeBERT)    │     │                 │
└─────────────────┘     └─────────────────┘     └─────────────────┘
```

---

## 🔄 RAG Pipeline Design

### Query-Flow

```
User Query: "Wie implementiere ich einen LRU Cache in Python?"
                │
                ▼
┌─────────────────────────────────────────────────────────────────┐
│ 1. Query Understanding                                          │
│    - Intent Detection (explain, implement, debug, review)       │
│    - Entity Extraction (LRU Cache, Python)                      │
│    - Query Expansion (synonyme: Least Recently Used, cache)     │
└───────────────────────────────┬─────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│ 2. Retrieval (ThemisDB)                                         │
│    a) Vector Search: Top-50 ähnliche Code-Chunks                │
│    b) Keyword Search: "LRU", "cache", "OrderedDict"             │
│    c) Graph Traversal: Verwandte Module/Funktionen              │
│    d) Hybrid Score = α*vector + β*keyword + γ*graph             │
└───────────────────────────────┬─────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│ 3. Reranking                                                    │
│    - Cross-Encoder Reranking (ms-marco-MiniLM)                  │
│    - Recency Boost (neuere Commits höher gewichten)             │
│    - Quality Score (Docstrings, Tests vorhanden)                │
│    - Top-K Selection (k=5-10)                                   │
└───────────────────────────────┬─────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│ 4. Context Assembly                                             │
│    - Prompt Template Selection                                  │
│    - Context Window Management (8K tokens)                      │
│    - Code Formatting (Syntax Highlighting)                      │
│    - Dependency Context (Imports, Types)                        │
└───────────────────────────────┬─────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│ 5. LLM Inference (vLLM)                                         │
│    - Model: CodeLlama-13B + respo-python LoRA                   │
│    - Streaming Response                                         │
│    - Token-by-Token Output                                      │
└───────────────────────────────┬─────────────────────────────────┘
                                │
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│ 6. Post-Processing                                              │
│    - Code Extraction                                            │
│    - Syntax Validation                                          │
│    - Citation/Reference Linking                                 │
│    - Response Formatting                                        │
└─────────────────────────────────────────────────────────────────┘
```

### Prompt Templates

```python
SYSTEM_PROMPTS = {
    "explain": """Du bist ein erfahrener Software-Entwickler.
Erkläre den folgenden Code klar und verständlich.
Nutze die bereitgestellten Kontext-Informationen aus der Codebasis.

Kontext:
{retrieved_context}
""",

    "implement": """Du bist ein erfahrener Software-Entwickler.
Implementiere die gewünschte Funktionalität basierend auf dem Stil und 
den Konventionen der existierenden Codebasis.

Relevante Code-Beispiele aus der Codebasis:
{retrieved_context}

Beachte:
- Verwende konsistente Namenskonventionen
- Füge Docstrings und Typ-Hints hinzu
- Berücksichtige Error-Handling
""",

    "review": """Du bist ein erfahrener Code-Reviewer.
Analysiere den folgenden Code auf:
- Bugs und potenzielle Fehler
- Performance-Probleme
- Sicherheitslücken
- Best-Practice-Verletzungen

Vergleiche mit Best Practices aus der Codebasis:
{retrieved_context}
""",

    "debug": """Du bist ein Debugging-Experte.
Analysiere das Problem und schlage eine Lösung vor.

Ähnliche gelöste Probleme aus der Codebasis:
{retrieved_context}

Fehlermeldung: {error_message}
Stack Trace: {stack_trace}
"""
}
```

---

## 📁 Projektstruktur für RESPO Adapter

```
adapters/
└── respo/                              # RAG LLM Programmierhilfe
    ├── README.md
    ├── requirements.txt
    ├── pyproject.toml
    │
    ├── respo/                          # Python Package
    │   ├── __init__.py
    │   ├── config.py                   # Konfiguration
    │   │
    │   ├── api/                        # FastAPI Endpoints
    │   │   ├── __init__.py
    │   │   ├── app.py                  # FastAPI App
    │   │   ├── routes/
    │   │   │   ├── chat.py             # Chat/Conversation
    │   │   │   ├── complete.py         # Code Completion
    │   │   │   ├── explain.py          # Code Explanation
    │   │   │   ├── review.py           # Code Review
    │   │   │   ├── refactor.py         # Refactoring
    │   │   │   ├── document.py         # Documentation Generation
    │   │   │   ├── search.py           # Semantic Search
    │   │   │   └── ingest.py           # Code Ingestion
    │   │   └── websocket.py            # Streaming über WebSocket
    │   │
    │   ├── rag/                        # RAG Pipeline
    │   │   ├── __init__.py
    │   │   ├── retriever.py            # Hybrid Retrieval
    │   │   ├── reranker.py             # Cross-Encoder Reranking
    │   │   ├── context_builder.py      # Context Assembly
    │   │   └── prompts.py              # Prompt Templates
    │   │
    │   ├── embedding/                  # Embedding Service
    │   │   ├── __init__.py
    │   │   ├── code_embedder.py        # Code-spezifische Embeddings
    │   │   └── models.py               # Embedding Model Wrapper
    │   │
    │   ├── ingestion/                  # Code Ingestion
    │   │   ├── __init__.py
    │   │   ├── chunker.py              # Code Chunking (AST-basiert)
    │   │   ├── parser.py               # Multi-Language Parser
    │   │   ├── indexer.py              # ThemisDB Indexer
    │   │   └── watcher.py              # File System Watcher
    │   │
    │   ├── llm/                        # LLM Integration
    │   │   ├── __init__.py
    │   │   ├── vllm_client.py          # vLLM API Client
    │   │   ├── lora_manager.py         # LoRA Adapter Management
    │   │   └── streaming.py            # Streaming Handler
    │   │
    │   ├── graph/                      # Code Graph Analysis
    │   │   ├── __init__.py
    │   │   ├── dependency_graph.py     # Dependency Analysis
    │   │   ├── call_graph.py           # Call Graph
    │   │   └── import_graph.py         # Import Graph
    │   │
    │   └── utils/                      # Utilities
    │       ├── __init__.py
    │       ├── code_utils.py           # Code Manipulation
    │       ├── ast_utils.py            # AST Helpers
    │       └── metrics.py              # Telemetry
    │
    ├── training/                       # LoRA Training
    │   ├── README.md
    │   ├── requirements.txt
    │   ├── train_lora.py               # Training Script
    │   ├── prepare_data.py             # Data Preparation
    │   ├── evaluate.py                 # Evaluation
    │   └── configs/
    │       ├── base.yaml
    │       ├── python.yaml
    │       └── typescript.yaml
    │
    ├── tests/                          # Tests
    │   ├── __init__.py
    │   ├── test_retriever.py
    │   ├── test_chunker.py
    │   ├── test_embedder.py
    │   └── test_api.py
    │
    ├── docker/                         # Docker
    │   ├── Dockerfile.api              # RESPO API
    │   ├── Dockerfile.vllm             # vLLM Server
    │   └── docker-compose.yml          # Full Stack
    │
    └── docs/                           # Dokumentation
        ├── api.md                      # API Reference
        ├── deployment.md               # Deployment Guide
        ├── training.md                 # LoRA Training Guide
        └── architecture.md             # Architecture Deep Dive
```

---

## 🚀 Umsetzungsplan

### Phase 1: Foundation (2-3 Wochen)

#### Woche 1-2: Basis-Setup
- [ ] **Projektstruktur** erstellen
  - [ ] `adapters/respo/` Verzeichnis anlegen
  - [ ] Python Package-Struktur
  - [ ] `pyproject.toml` mit Dependencies
  - [ ] `.env.example` für Konfiguration

- [ ] **ThemisDB Integration**
  - [ ] VCC-Base Library erweitern für Code-spezifische Operationen
  - [ ] Code-Entity Schema definieren
  - [ ] Vector Search Wrapper
  - [ ] Graph Traversal für Code-Dependencies

- [ ] **Embedding Service**
  - [ ] CodeBERT Integration
  - [ ] Batch-Embedding Pipeline
  - [ ] Caching-Strategie

#### Woche 2-3: Code Ingestion
- [ ] **AST-basierter Parser**
  - [ ] tree-sitter Integration
  - [ ] Python, JavaScript, TypeScript Support
  - [ ] Go, Rust Support (optional)

- [ ] **Chunking Pipeline**
  - [ ] Function-Level Chunking
  - [ ] Class-Level Chunking
  - [ ] Semantic Chunking

- [ ] **Graph Builder**
  - [ ] Import Graph
  - [ ] Call Graph
  - [ ] Dependency Graph

### Phase 2: RAG Pipeline (2-3 Wochen)

#### Woche 4-5: Retrieval & Reranking
- [ ] **Hybrid Retriever**
  - [ ] Vector Search (ThemisDB HNSW)
  - [ ] Keyword Search (Full-Text Index)
  - [ ] Graph-based Retrieval
  - [ ] Score Fusion

- [ ] **Reranker**
  - [ ] Cross-Encoder Integration
  - [ ] Recency Scoring
  - [ ] Quality Scoring

#### Woche 5-6: Context Building
- [ ] **Context Builder**
  - [ ] Prompt Templates
  - [ ] Context Window Management
  - [ ] Code Formatting

- [ ] **Intent Detection**
  - [ ] Query Classification
  - [ ] Entity Extraction

### Phase 3: LLM Integration (2-3 Wochen)

#### Woche 7-8: vLLM Setup
- [ ] **vLLM Server**
  - [ ] Docker Container
  - [ ] GPU Configuration
  - [ ] API Wrapper

- [ ] **Streaming**
  - [ ] WebSocket Handler
  - [ ] Token-by-Token Streaming

#### Woche 8-9: LoRA Training
- [ ] **Data Preparation**
  - [ ] Training Data Sammlung
  - [ ] Data Cleaning
  - [ ] Format Conversion

- [ ] **Training Pipeline**
  - [ ] LoRA Config
  - [ ] Training Script
  - [ ] Evaluation (HumanEval, MBPP)

### Phase 4: API & Frontend (2-3 Wochen)

#### Woche 10-11: FastAPI Endpoints
- [ ] **Core Endpoints**
  - [ ] `/chat` - Conversation
  - [ ] `/complete` - Code Completion
  - [ ] `/explain` - Explanation
  - [ ] `/review` - Code Review
  - [ ] `/search` - Semantic Search

- [ ] **Ingestion Endpoints**
  - [ ] `/ingest/repo` - Repository Import
  - [ ] `/ingest/file` - File Upload
  - [ ] `/ingest/webhook` - Git Webhook

#### Woche 11-12: Clients
- [ ] **VSCode Extension** (optional)
  - [ ] Inline Completion
  - [ ] Chat Panel
  - [ ] Code Actions

- [ ] **CLI Tool**
  - [ ] `respo chat`
  - [ ] `respo explain`
  - [ ] `respo index`

### Phase 5: Deployment & Testing (1-2 Wochen)

#### Woche 13-14: Production Readiness
- [ ] **Docker Compose**
  - [ ] Full Stack Setup
  - [ ] Volume Management
  - [ ] Networking

- [ ] **Testing**
  - [ ] Unit Tests
  - [ ] Integration Tests
  - [ ] Load Tests

- [ ] **Dokumentation**
  - [ ] API Reference
  - [ ] Deployment Guide
  - [ ] User Guide

---

## 🐳 Docker Deployment

### docker-compose.yml

```yaml
version: '3.8'

services:
  # ThemisDB Backend
  themisdb:
    image: themisdb/themisdb:latest
    ports:
      - "8765:8765"
    volumes:
      - themis-data:/data
      - ./config/themis.yaml:/etc/themis/config.yaml
    environment:
      - LOG_LEVEL=INFO
    healthcheck:
      test: ["CMD", "curl", "-f", "http://localhost:8765/health"]
      interval: 30s
      timeout: 10s
      retries: 3

  # vLLM Server
  vllm:
    build:
      context: ./docker
      dockerfile: Dockerfile.vllm
    ports:
      - "8000:8000"
    volumes:
      - ./models:/models
    environment:
      - MODEL_NAME=codellama/CodeLlama-13b-Instruct-hf
      - LORA_MODULES=respo-python=/models/lora/respo-python
      - GPU_MEMORY_UTILIZATION=0.9
      - MAX_MODEL_LEN=8192
    deploy:
      resources:
        reservations:
          devices:
            - driver: nvidia
              count: all
              capabilities: [gpu]
    depends_on:
      - themisdb

  # RESPO API
  respo-api:
    build:
      context: .
      dockerfile: docker/Dockerfile.api
    ports:
      - "8080:8080"
    environment:
      - THEMIS_URL=http://themisdb:8765
      - VLLM_URL=http://vllm:8000
      - EMBEDDING_MODEL=microsoft/codebert-base
      - LOG_LEVEL=INFO
    depends_on:
      - themisdb
      - vllm

  # Embedding Service (optional, für Batch)
  embedding:
    build:
      context: .
      dockerfile: docker/Dockerfile.embedding
    environment:
      - MODEL_NAME=microsoft/codebert-base
      - BATCH_SIZE=32
    deploy:
      resources:
        reservations:
          devices:
            - driver: nvidia
              count: 1
              capabilities: [gpu]

volumes:
  themis-data:
```

---

## 🔒 On-Premise Security

### Keine externe Abhängigkeiten

| Komponente | Status | Anmerkung |
|------------|--------|-----------|
| ThemisDB | ✅ Local | Vollständig on-premise |
| vLLM | ✅ Local | Keine Cloud-API |
| Embedding Model | ✅ Local | Lokale Modelle |
| LLM Model | ✅ Local | HuggingFace Download |
| LoRA Adapters | ✅ Local | Selbst trainiert |

### Air-Gapped Deployment

```bash
# 1. Modelle vorher herunterladen
huggingface-cli download codellama/CodeLlama-13b-Instruct-hf --local-dir ./models/base
huggingface-cli download microsoft/codebert-base --local-dir ./models/embedding

# 2. Docker Images exportieren
docker save themisdb/themisdb:latest > themis.tar
docker save respo-api:latest > respo-api.tar
docker save respo-vllm:latest > respo-vllm.tar

# 3. Auf Air-Gapped System übertragen
scp *.tar airgapped:/opt/respo/

# 4. Images laden
docker load < themis.tar
docker load < respo-api.tar
docker load < respo-vllm.tar

# 5. Starten
docker compose up -d
```

### DSGVO-Konformität

- ✅ **Datenlokalität** - Alle Daten bleiben on-premise
- ✅ **Keine Telemetrie** - Kein Phone-Home
- ✅ **Audit Logging** - ThemisDB Audit Trail
- ✅ **Verschlüsselung** - Field-Level Encryption verfügbar
- ✅ **Zugriffskontrolle** - RBAC über ThemisDB

---

## 📈 Erwartete Performance

### Latenz (typisch)

| Operation | Latenz | Anmerkung |
|-----------|--------|-----------|
| Embedding (single) | 50-100 ms | CodeBERT |
| Vector Search | 5-20 ms | ThemisDB HNSW |
| Reranking (Top-50) | 100-200 ms | Cross-Encoder |
| LLM First Token | 500-1000 ms | vLLM |
| LLM Total (500 Token) | 3-8 s | vLLM Streaming |

### Durchsatz

| Metrik | Wert | Hardware |
|--------|------|----------|
| Concurrent Users | 10-50 | 1x A100 |
| Requests/Minute | 30-100 | 1x A100 |
| Embeddings/Second | 50-100 | 1x A10 |

---

## 🎯 Nächste Schritte

### Sofort (Diese Woche)
1. [ ] `adapters/respo/` Verzeichnis erstellen
2. [ ] Basis-Requirements definieren
3. [ ] ThemisDB Code-Schema entwerfen
4. [ ] Embedding Model evaluieren

### Kurzfristig (2 Wochen)
1. [ ] Code Ingestion Pipeline
2. [ ] Basic RAG Retrieval
3. [ ] vLLM Integration (ohne LoRA)

### Mittelfristig (4-6 Wochen)
1. [ ] LoRA Training Pipeline
2. [ ] FastAPI Endpoints
3. [ ] Docker Deployment

### Langfristig (2-3 Monate)
1. [ ] VSCode Extension
2. [ ] Advanced Features (Review, Refactor)
3. [ ] Multi-Language Support

---

## 📚 Referenzen

### Technologie
- [vLLM Documentation](https://docs.vllm.ai/)
- [LoRA Paper](https://arxiv.org/abs/2106.09685)
- [CodeLlama Paper](https://arxiv.org/abs/2308.12950)
- [ThemisDB Dokumentation](https://makr-code.github.io/ThemisDB/)

### Best Practices
- [RAG Best Practices](https://www.pinecone.io/learn/retrieval-augmented-generation/)
- [Code Embeddings](https://huggingface.co/blog/code-search)
- [Production LLM Serving](https://vllm.ai/blog/)

---

**Erstellt:** November 2025  
**Status:** Recherche & Planung  
**Nächstes Review:** Nach Phase 1 Abschluss
