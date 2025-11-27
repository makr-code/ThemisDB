# RESPO Architecture

## System Overview

```
┌─────────────────────────────────────────────────────────────┐
│                      RESPO System                            │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────────────┐ │
│  │ VS Code │  │ Web UI  │  │   CLI   │  │  External APIs  │ │
│  │  (MCP)  │  │(Gradio) │  │         │  │                 │ │
│  └────┬────┘  └────┬────┘  └────┬────┘  └────────┬────────┘ │
│       │            │            │                 │          │
│       └────────────┴────────────┴─────────────────┘          │
│                            │                                 │
│  ┌─────────────────────────▼─────────────────────────────┐  │
│  │                   FastAPI Server                       │  │
│  │  /chat  /complete  /explain  /review  /search         │  │
│  │  /ingest  /mcp  /tasks  /capabilities  /metrics       │  │
│  └─────────────────────────┬─────────────────────────────┘  │
│                            │                                 │
│  ┌─────────────────────────▼─────────────────────────────┐  │
│  │                    Core Modules                        │  │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────────────────┐ │  │
│  │  │   RAG    │  │  Agents  │  │     Evaluation       │ │  │
│  │  │ Pipeline │  │ Planner  │  │    LLM-as-Judge      │ │  │
│  │  └────┬─────┘  └────┬─────┘  └──────────────────────┘ │  │
│  └───────┼─────────────┼────────────────────────────────┘   │
│          │             │                                    │
│  ┌───────▼─────────────▼────────────────────────────────┐  │
│  │              Infrastructure Layer                     │  │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐            │  │
│  │  │  Cache   │  │ Metrics  │  │  Tasks   │            │  │
│  │  │(LRU/Redis│  │Prometheus│  │ Manager  │            │  │
│  │  └──────────┘  └──────────┘  └──────────┘            │  │
│  └──────────────────────────────────────────────────────┘  │
│                            │                                │
│  ┌─────────────────────────▼─────────────────────────────┐ │
│  │                  External Services                     │ │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────────────────┐ │ │
│  │  │  vLLM    │  │ Vector   │  │     Scrapers         │ │ │
│  │  │ Server   │  │  Store   │  │  GitHub, GitLab...   │ │ │
│  │  └──────────┘  └──────────┘  └──────────────────────┘ │ │
│  └───────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
```

## Component Details

### RAG Pipeline

1. **Retriever**: Hybrid search (vector + keyword + graph)
2. **Reranker**: Cross-encoder for precision
3. **Context Assembly**: Top-K selection with deduplication
4. **Generation**: vLLM with LoRA adapters

### Vector Stores

| Store | Features |
|-------|----------|
| ChromaDB | Default, embedded, no server |
| Qdrant | High-performance, distributed |
| ThemisDB | Graph + Hybrid search |

### Agents

- **AgenticPlanner**: Decomposes complex problems
- **StepExecutor**: Executes plan steps
- **DeepResearchAgent**: Iterative research synthesis

### MCP Integration

JSON-RPC 2.0 protocol for VS Code:
- `respo_chat`: Chat with context
- `respo_search`: Semantic search
- `respo_research`: Deep research
- `respo_implement`: Code implementation

## Data Flow

```
User Query → API → RAG Pipeline → Vector Search
                                        ↓
                              Context Retrieval
                                        ↓
                              LLM Generation
                                        ↓
                              Response + Sources
```

## Deployment Modes

1. **Development**: Single process
2. **Production**: Docker Compose with scaling
3. **Air-Gapped**: Offline with pre-downloaded models
