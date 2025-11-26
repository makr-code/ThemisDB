# RESPO - RAG-Enhanced Software Programming Optimizer

[![Status](https://img.shields.io/badge/status-development-yellow)](.)
[![Python](https://img.shields.io/badge/python-3.10+-blue)](.)
[![License](https://img.shields.io/badge/license-MIT-green)](../../LICENSE)

## 🎯 Übersicht

RESPO ist ein **on-premise RAG LLM Programmierhilfe-System** basierend auf:

- **ThemisDB** - Multi-Model Datenbank für Vektor, Graph und Dokumente
- **vLLM** - Hochperformante LLM-Inferenz mit LoRA Support
- **Ohne Vendor-Login** - Vollständig lokale Ausführung

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
   │ RAG Pipeline│     │ vLLM Engine │     │  ThemisDB   │
   │             │     │             │     │             │
   │ - Retrieval │     │ - Inference │     │ - Vectors   │
   │ - Reranking │     │ - LoRA      │     │ - Graphs    │
   │ - Context   │     │ - Streaming │     │ - Docs      │
   └─────────────┘     └─────────────┘     └─────────────┘
```

## 🚀 Quick Start

### Prerequisites

- Python 3.10+
- ThemisDB Server running
- vLLM Server with GPU (optional für CPU-Fallback)
- NVIDIA GPU mit 16+ GB VRAM (empfohlen)

### Installation

```bash
# 1. Dependencies installieren
cd adapters/respo
pip install -e .

# 2. Konfiguration kopieren
cp .env.example .env
# Edit .env with your settings

# 3. ThemisDB starten (falls nicht läuft)
docker compose -f ../../docker-compose.yml up -d themisdb

# 4. RESPO API starten
uvicorn respo.api.app:app --host 0.0.0.0 --port 8080
```

### Mit Docker Compose

```bash
cd adapters/respo/docker
docker compose up -d
```

## 📡 API Endpoints

### Core Endpoints

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

### Beispiel: Code Search

```bash
curl -X POST http://localhost:8080/search \
  -H "Content-Type: application/json" \
  -d '{
    "query": "database connection pooling",
    "language": "python",
    "limit": 10
  }'
```

## 📁 Projektstruktur

```
respo/
├── respo/                  # Python Package
│   ├── api/                # FastAPI Endpoints
│   ├── rag/                # RAG Pipeline
│   ├── embedding/          # Embedding Service
│   ├── ingestion/          # Code Ingestion
│   ├── llm/                # vLLM Integration
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
# ThemisDB
THEMIS_URL=http://localhost:8765

# vLLM
VLLM_URL=http://localhost:8000
VLLM_MODEL=codellama/CodeLlama-13b-Instruct-hf

# Embedding
EMBEDDING_MODEL=microsoft/codebert-base

# Logging
LOG_LEVEL=INFO
```

## 🧪 LoRA Training

Siehe [training/README.md](training/README.md) für Details zum Fine-Tuning.

```bash
cd training
python train_lora.py --config configs/python.yaml
```

## 📊 Performance

| Operation | Latenz | Hardware |
|-----------|--------|----------|
| Vector Search | 5-20 ms | ThemisDB |
| Reranking | 100-200 ms | CPU |
| LLM First Token | 500-1000 ms | A100 |
| LLM Total (500 Token) | 3-8 s | A100 |

## 🔒 Security

- ✅ **On-Premise** - Keine Cloud-Abhängigkeiten
- ✅ **Kein Vendor-Login** - Vollständig lokal
- ✅ **DSGVO-konform** - Alle Daten bleiben lokal
- ✅ **Air-Gapped Deployment** - Möglich

## 📚 Dokumentation

- [Architektur](docs/architecture.md)
- [API Reference](docs/api.md)
- [Deployment Guide](docs/deployment.md)
- [LoRA Training Guide](docs/training.md)

## 🛠️ Development

```bash
# Tests ausführen
pytest tests/

# Linting
ruff check respo/

# Type Checking
mypy respo/
```

## 📄 Lizenz

MIT License - siehe [LICENSE](../../LICENSE)

---

**Status:** Development  
**Version:** 0.1.0  
**Maintainer:** ThemisDB Team
