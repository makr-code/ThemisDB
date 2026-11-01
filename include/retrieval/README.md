---
Author: "ThemisDB Contributors"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "ThemisDB documentation orchestration assistant"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References:
  - "retrieval.h"
  - "dual_read_validator.h"
  - "../../src/retrieval/CMakeLists.txt"
  - "../../src/retrieval/include/lora_package.h"
  - "../../src/retrieval/README.md"
---

Exact model backend not verifiable from final document text alone.

# Retrieval public headers

`include/retrieval/` currently contains two headers:

- [`retrieval.h`](retrieval.h) is an umbrella placeholder. It contains no includes or declarations.
- [`dual_read_validator.h`](dual_read_validator.h) declares `RetrievalResult`, `RetrievalMetrics`, and `DualReadValidator`.

The LoRA artifact declarations are in `src/retrieval/include/lora_package.h`, not this directory. They are exposed through the public include directory of the `themis_retrieval` target. See the [module README](../../src/retrieval/README.md) for target details and limitations.

## Canonical Module Documentation

`include/retrieval/` contains public header contracts. Canonical module behavior, architecture, and operations docs live in `src/retrieval/`:

- [`../../src/retrieval/README.md`](../../src/retrieval/README.md)
- [`../../src/retrieval/ARCHITECTURE.md`](../../src/retrieval/ARCHITECTURE.md)
- [`../../src/retrieval/ROADMAP.md`](../../src/retrieval/ROADMAP.md)
- [`../../src/retrieval/FUTURE_ENHANCEMENTS.md`](../../src/retrieval/FUTURE_ENHANCEMENTS.md)

## Include and build

The only module build option is `THEMIS_BUILD_RETRIEVAL` (default `ON`); it
controls the static `themis_retrieval` library. That target adds
`src/retrieval/include` to its public include paths. The artifact header is
included as `lora_package.h` with that target's include path, not as an
`include/retrieval/` header. The dual-read validator is declared in this
directory, but its implementation is not built into `themis_retrieval`.

## See Also

- [`../../src/retrieval/README.md`](../../src/retrieval/README.md) — implementation details
- [`../../src/search/README.md`](../../src/search/README.md) — search module integration
- [`../../src/rag/README.md`](../../src/rag/README.md) — RAG system integration

## Installation

```sh
cmake -S . -B build
cmake --build build --target themis_retrieval
```

There is no retrieval runtime configuration API in these headers.
