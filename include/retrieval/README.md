> **Build:** `cmake --preset release && cmake --build build/release`

# Retrieval Module — Public Headers

**Module Path:** `include/retrieval/`
**Implementation:** `../../src/retrieval/`

## Purpose

Public interfaces and declarations for ThemisDB's information retrieval and semantic search subsystem, providing unified retrieval orchestration across multiple backend strategies.

## Canonical Module Documentation

`include/retrieval/` contains public header contracts. Canonical module behavior, architecture, and operations docs live in `src/retrieval/`:

- [`../../src/retrieval/README.md`](../../src/retrieval/README.md)
- [`../../src/retrieval/ARCHITECTURE.md`](../../src/retrieval/ARCHITECTURE.md)
- [`../../src/retrieval/ROADMAP.md`](../../src/retrieval/ROADMAP.md)
- [`../../src/retrieval/FUTURE_ENHANCEMENTS.md`](../../src/retrieval/FUTURE_ENHANCEMENTS.md)

## Header Files

| Header | Primary Class / Interface |
|--------|--------------------------|
| `retrieval.h` | `Retriever` — core retrieval orchestration interface |
| `retrieval_config.h` | `RetrievalConfig` — configuration and strategy selection |
| `bm25_retriever.h` | `BM25Retriever` — BM25 ranking implementation |
| `semantic_retriever.h` | `SemanticRetriever` — embedding-based semantic retrieval |
| `hybrid_retriever.h` | `HybridRetriever` — combined lexical and semantic strategies |
| `retrieval_metrics.h` | `RetrievalMetrics` — precision, recall, and ranking quality metrics |
| `result_reranker.h` | `ResultReranker` — re-ranking and score normalization |
| `cross_encoder_reranker.h` | `CrossEncoderReranker` — neural re-ranking interface |

## Usage

```cpp
#include "retrieval/hybrid_retriever.h"

auto retriever = themis::retrieval::createHybridRetriever({
    .lexical_weight = 0.3,
    .semantic_weight = 0.7,
    .reranker_enabled = true
});

auto results = retriever->retrieve("query", 20);
for (const auto& result : results) {
    std::cout << result.document << " (score: " << result.score << ")" << std::endl;
}
```

For full runtime usage examples (retrieval strategies, re-ranking, metrics), see [`../../src/retrieval/README.md`](../../src/retrieval/README.md).

## Key Configuration Surface

Important configuration entry points are declared in:

- `retrieval.h` (`Retriever::Config` for strategy selection)
- `retrieval_config.h` (backend-specific tuning parameters)
- `hybrid_retriever.h` (`HybridRetriever::Config` for lexical/semantic weighting)
- `result_reranker.h` (re-ranking and normalization configuration)

## Build

```cmake
cmake --preset release && cmake --build build/release --target themis-retrieval
```

## See Also

- [`../../src/retrieval/README.md`](../../src/retrieval/README.md) — implementation details
- [`../../src/search/README.md`](../../src/search/README.md) — search module integration
- [`../../src/rag/README.md`](../../src/rag/README.md) — RAG system integration

## Installation

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
