---
Author: "ThemisDB Team"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

Exact model backend not verifiable from final document text alone.

# ThemisDB AQL Public Headers

> **See also:** [Public header map](ARCHITECTURE.md) ·
> [AQL implementation overview](../../src/aql/README.md) ·
> [Assistance API notes](../../src/aql/API.md)

## Scope

`include/aql/` declares public interfaces for AQL query authoring and
validation, LLM-assisted translation, context/example helpers, and related
adapters. The core query parser and execution services are in `include/query/`
and `src/query/`; the AQL assistance headers do not implement or replace those
services.

The header map in [`ARCHITECTURE.md`](ARCHITECTURE.md) lists the current header
groups. Consult each header for exact signatures, namespace, ownership and
configuration.

## Main interfaces

| Interface | Source-supported role |
|---|---|
| `LLMAQLHandler` | LLM operation and NL-to-AQL assistance entry points; exposes parser-service and validation-pipeline configuration |
| `AQLQueryValidator` | Checks query strings or builders and can accept collection metadata; it is not the core parser |
| `AQLConversationContext` | Conversation history with configurable `max_turns` and `max_history_tokens` (defaults: 50 and 8192) |
| `AQLFewShotExampleLibrary` | Filters/ranks examples; lexical Jaccard ranking is available without embeddings, with optional embedding-based ranking |
| `LLMAQLEmbeddingBridge` | Adapts the handler's embedding operation to `IEmbeddingProvider`; the handler must outlive the bridge |
| `ReActAgent` | Agent composition around explicitly registered application-provided tools |

These are interface descriptions, not claims that a provider or optional
integration is configured in every build.

## Translation and safety boundary

The translation implementation sanitizes/bounds inputs, requests a generated
candidate, strips Markdown fences, validates it, and applies configured
validation/retry behavior and collection-scope checks. Parser-service validation
is used when available; the fallback is string-level validation and is not
equivalent to parsing. A valid parse does not execute a query or grant access.
The host application remains responsible for authorization at the execution
and tool-call boundaries.

The contract review in [`../../src/aql/AQL_ASSISTANCE_CONTRACT.md`](../../src/aql/AQL_ASSISTANCE_CONTRACT.md)
distinguishes former intended semantics from current implementation behavior.
Use it with [`../../src/aql/API.md`](../../src/aql/API.md); neither replaces the
public header declarations.

## Test and benchmark build configuration

- Focused test sources are in `tests/aql/`. Its CMake file globs
  `test_aql_*.cpp`, creates `module_aql_<test-stem>_focused` targets, and
  registers corresponding CTest entries with the `aql` label and a 120-second
  timeout. Reconfigure CMake after changing the glob's file set.
- The focused tests are conditional on the repository test configuration;
  root `CMakeLists.txt` defines `THEMIS_BUILD_TESTS`.
- `benchmarks/aql/CMakeLists.txt` returns without registering its local targets
  unless `THEMIS_BUILD_BENCHMARKS` is enabled. It registers function, mutation,
  translation, helper-path, assistance-gate, and consistency-gate targets.
- These statements describe registration in source, not a build or test result.

For a focused test, configure a supported build with `THEMIS_BUILD_TESTS=ON`
(see the root [`CMAKE.md`](../../CMAKE.md)), then build and run the registered
validator test:

```bash
cmake --build <build-dir> --target module_aql_test_aql_query_validator_focused
ctest --test-dir <build-dir> --output-on-failure \
  -R '^test_aql_query_validator_AqlFocusedTests$'
```

The target/test names follow the stem in `tests/aql/CMakeLists.txt`; the command
does not imply that the test was run or passed.

## Source references

- [`llm_aql_handler.h`](llm_aql_handler.h)
- [`aql_query_validator.h`](aql_query_validator.h)
- [`aql_conversation_context.h`](aql_conversation_context.h)
- [`aql_fewshot_example_library.h`](aql_fewshot_example_library.h)
- [`llm_aql_embedding_bridge.h`](llm_aql_embedding_bridge.h)
- [`aql_agent.h`](aql_agent.h)
- [`../../tests/aql/CMakeLists.txt`](../../tests/aql/CMakeLists.txt)
- [`../../benchmarks/aql/CMakeLists.txt`](../../benchmarks/aql/CMakeLists.txt)
