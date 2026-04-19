# AQL Module — Architecture Guide

**Version:** 1.0
**Last Updated:** 2026-04-06
**Module Path:** `src/aql/`

---

## 1. Overview

The AQL module provides the AI-assisted layer on top of ThemisDB's Advanced Query Language.
It bridges natural language and AQL: users can describe what they want in plain text and
receive a validated AQL query in return; conversely, users can submit an AQL query and
receive a natural language explanation. It also hosts documentation assistants, autocompletion,
schema-aware query building, LoRA fine-tuning utilities for the query domain, and migration
tools for query upgrades.

This module does **not** contain the AQL parser or execution engine — those live in
`src/query/`. The AQL module is concerned with LLM-augmented query authoring and assistance.

---

## 2. Design Principles

- **LLM as a Tool, Not a Dependency** – LLM capabilities enhance the developer experience
  but are optional; all core query execution works without them.
- **Prompt Injection Prevention** – all user-supplied input passing through the LLM pipeline
  is sanitized to block instruction overrides, persona hijacking, and system-block markers.
- **Schema-Aware** – query generation and autocompletion use live schema context from the
  `metadata` module to produce valid, executable AQL.
- **Streaming** – AQL explanations are streamed token-by-token (SSE) to minimise perceived
  latency on long queries.
- **Confidence Scoring** – generated queries carry a confidence score so callers can decide
  whether to auto-execute or prompt for human review.

---

## 3. Component Architecture

### 3.1 Key Components

| File | Role |
|---|---|
| `llm_aql_handler.cpp` | LLM command handler: INFER, RAG, EMBED, MODEL, LORA, STATS, CACHE |
| `aql_query_builder.cpp` | Schema-aware programmatic AQL construction |
| `aql_schema_provider.cpp` | Fetches live schema context for LLM-assisted generation |
| `aql_query_validator.cpp` | Structural validation and linting of generated AQL |
| `aql_syntax_highlighter.cpp` | Syntax highlighting and error annotation for LLM responses |
| `aql_autocomplete.cpp` | Token-level autocompletion suggestions |
| `aql_optimizer_advisor.cpp` | Explains query plan; suggests rewrites for performance |
| `aql_confidence_scorer.cpp` | Scores LLM-generated queries for reliability |
| `aql_conversation_context.cpp` | Multi-turn conversation history management |
| `aql_query_template_library.cpp` | Curated query templates for common patterns |
| `aql_lora_finetuner.cpp` | Fine-tunes LoRA adapters on domain-specific AQL corpora |
| `aql_migration_assistant.cpp` | Migrates legacy queries to current AQL syntax |
| `docs_assistant_functions.cpp` | Function lookup, signature display, usage examples |
| `llm_metrics_collector.cpp` | Latency, token counts, cache-hit metrics for LLM operations |

### 3.2 Component Diagram

```
┌──────────────────────────────────────────────────────────────────┐
│                    API/Server Layer                              │
│   POST /query/nl-to-aql  │  POST /query/explain  │  WebSocket   │
└──────────────────────────┬───────────────────────────────────────┘
                           │
┌──────────────────────────▼───────────────────────────────────────┐
│                    LlmAqlHandler                                 │
│  translateNLToAQL() │ streamExplainAQL() │ executeChat()         │
│  LLM INFER │ LLM RAG │ LLM EMBED │ LLM MODEL │ LLM LORA        │
└────┬──────────────┬───────────────┬──────────────────────────────┘
     │              │               │
┌────▼────┐  ┌──────▼───────┐ ┌────▼───────────────┐
│ Schema  │  │  AQL Query   │  │   AQL Validator    │
│Provider │  │   Builder    │  │  + Highlighter     │
└────┬────┘  └──────┬───────┘ └────────────────────┘
     │              │
┌────▼──────────────▼──────────────────────────────────────────────┐
│                  LLM Module (src/llm/)                           │
│            AsyncInferenceEngine / InferenceEngineEnhanced        │
└──────────────────────────────────────────────────────────────────┘
```

---

## 4. Data Flow

### 4.1 Natural Language to AQL

```
User: "Find all users who joined last month"
    │
    ▼
aql_schema_provider.cpp → fetch collection/field names
    │
    ▼
llm_aql_handler.cpp: sanitize input (strip injection patterns)
    │
    ▼
LLM prompt: system_prompt + schema_context + nl_query
    │
    ▼
src/llm/: inference → raw AQL string
    │
    ▼
aql_query_validator.cpp: structural validation
    │
    ▼
aql_confidence_scorer.cpp: compute confidence [0.0, 1.0]
    │
    ▼
Response: { aql: "...", confidence: 0.87, warnings: [] }
```

### 4.2 Streaming AQL Explanation (SSE)

```
Client: POST /query/explain  { aql: "FOR u IN users ..." }
    │
    ▼
llm_aql_handler.cpp: streamExplainAQLAsSSE()
    │
    ▼
Token stream from LLM → SSE frames sent to client in real time
```

---

## 5. Integration Points

| Direction | Module | Interface |
|---|---|---|
| **Uses** | `src/llm/` | Inference engines for NL→AQL and explanations |
| **Uses** | `src/metadata/` | Schema context via `aql_schema_provider.cpp` |
| **Uses** | `src/query/` | AQL validator delegates parsing to the AQL parser |
| **Uses** | `src/index/` | Vector search for RAG context retrieval |
| **Consumed by** | `src/server/` | Exposed via API handlers |

---

## 6. Threading & Concurrency Model

- Each `LlmAqlHandler` call is stateless; concurrency is handled by the caller's thread pool.
- Multi-turn conversations are serialized per session ID in `aql_conversation_context.cpp`.
- SSE streaming uses a dedicated response-writer coroutine per connection.
- `llm_metrics_collector.cpp` uses lock-free atomic counters.

---

## 7. Performance Architecture

| Technique | Detail |
|---|---|
| Prompt caching | `LLM CACHE` command caches prompt prefixes in the LLM module |
| Schema caching | `aql_schema_provider.cpp` caches schema per tenant with TTL |
| Query templates | Pre-validated templates bypass LLM for common patterns |
| Confidence threshold | Low-confidence queries are flagged, avoiding invalid executions |

---

## 8. Security Considerations

- **Prompt injection prevention**: `translateNLToAQL()` strips instruction overrides, persona
  hijacking markers, system-block markers, and null bytes from `nl_query` and `schema_context`.
- **No query auto-execution** below configurable confidence threshold (default: 0.7).
- **LoRA fine-tuning**: training data is validated and sanitized before submission.
- **Schema leakage**: schema context sent to LLM is scoped to the requesting tenant's
  accessible collections only.

---

## 9. Configuration

| Parameter | Default | Description |
|---|---|---|
| `aql.nl_to_aql.confidence_threshold` | 0.7 | Min confidence to auto-execute |
| `aql.nl_to_aql.schema_cache_ttl_s` | 300 | Schema cache TTL in seconds |
| `aql.streaming.sse_chunk_delay_ms` | 0 | Delay between SSE chunks (0 = immediate) |
| `aql.lora.finetuner_enabled` | false | Enable LoRA fine-tuning utilities |

---

## 10. Error Handling

| Error Type | Strategy |
|---|---|
| Prompt injection detected | Reject with 400; log security event |
| LLM inference timeout | Return timeout error; do not execute partial AQL |
| Validation failure on generated AQL | Return AQL + validation errors; confidence = 0 |
| Schema unavailable | Return error; do not attempt generation without context |

---

## 11. Known Limitations & Future Work

- `aql_query_validator.cpp` currently performs structural validation only; semantic
  type-checking is planned.
- LoRA fine-tuner (`aql_lora_finetuner.cpp`) requires a training data corpus to be
  provided externally.
- Migration assistant (`aql_migration_assistant.cpp`) covers v1.3→v1.4 syntax changes;
  earlier versions require manual migration.

---

## 12. References

- `src/aql/README.md` — module overview and LLM command syntax
- `src/query/README.md` — AQL parser and execution engine
- `src/llm/README.md` — inference engine architecture
- `docs/aql_functions_implementation_status.md` — AQL function library status
- `docs/aql_roadmap.md` — AQL language roadmap
- `ARCHITECTURE.md` (root) — full system architecture
