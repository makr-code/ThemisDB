# RESPO Quick Start Guide

Get started with RESPO in 5 minutes.

## Prerequisites

- Python 3.10+
- Docker (optional, for vLLM)

## Installation

```bash
cd projects/respo
pip install -e .
```

## Start the Server

```bash
respo server --port 8080
```

## Basic Usage

### Chat with RAG

```bash
curl -X POST http://localhost:8080/chat \
  -H "Content-Type: application/json" \
  -d '{"message": "How do I implement an LRU cache in Python?"}'
```

### Index a Repository

```bash
curl -X POST http://localhost:8080/ingest \
  -H "Content-Type: application/json" \
  -d '{"source_type": "github", "source": "owner/repo"}'
```

### Search Code

```bash
curl -X POST http://localhost:8080/search \
  -H "Content-Type: application/json" \
  -d '{"query": "database connection", "limit": 10}'
```

## Web UI

```bash
respo ui --port 7860
```

Open http://localhost:7860 in your browser.

## Configuration

Create a `.env` file:

```bash
# Vector Store
VECTOR_STORE=chroma  # or: qdrant, themis

# vLLM
VLLM_URL=http://localhost:8000
VLLM_MODEL=codellama/CodeLlama-7b-Instruct-hf

# Embeddings
EMBEDDING_MODEL=microsoft/codebert-base
```

## Next Steps

- [Architecture Overview](ARCHITECTURE.md)
- [API Reference](API.md)
- [Training Sources](TRAINING_SOURCES.md)
