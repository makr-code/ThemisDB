---
Author: "ThemisDB Team"
Created: "2026-10-18"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: unknown
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References: "include/exporters/*.h; src/exporters/*.cpp"
Related: "Issue #7346; DOC-WEEKLY-2026-44"
---

# Exporters capabilities

Exact model backend not verifiable from final document text alone.

The exporters module serializes caller-provided `BaseEntity` records; it does
not query source collections or generate embeddings. Its public interfaces
provide:

- JSONL formats and named LLM-data templates, with configurable field mapping,
  quality checks, and optional PII detection/redaction.
- Parquet and Arrow IPC file/stream paths, subject to build-time Arrow support.
- Local Hugging Face dataset-directory generation and a separate Hub upload
  client.
- Cursor-based streaming, sequence-watermark incremental output, and a
  memory-budgeted in-process join.
- Optional encryption, AQL predicate filtering on supported export paths,
  format registration, and in-process metrics.

Streaming consumes cursor pages through a buffered writer; its configured
`max_buffer_bytes` value triggers flushes but is not a total-process-memory cap.
Incremental export receives an entity vector from its caller. Join export holds
its right-side records in memory. These distinctions matter when sizing input
and resource limits.

See [README.md](./README.md), [ARCHITECTURE.md](./ARCHITECTURE.md),
[API.md](./API.md), and [PRODUCTION_REQUIREMENTS.md](./PRODUCTION_REQUIREMENTS.md).
