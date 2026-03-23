<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Ingestion Module (Public Headers)

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../../SECURITY.md).

## Threat Model

| Threat | Mitigation |
|--------|------------|
| SSRF via web crawler / API connector | URL allow-list enforced in `web_crawler_connector.h` and `api_connector.h` |
| Malicious file content via filesystem ingester | MIME validation + OCR sandboxed; oversized files rejected |
| CDC stream replay | Offset continuity checked; duplicate events deduplicated |
| LLM prompt injection via ingested content | `llm_adapter.h` sanitises content before LLM prompt construction |
| Credential leakage | Connection credentials injected via secret store; not stored in headers |
| Cross-tenant ingestion leakage | Each connector operation scoped to tenant context |

## Known Limitations

- Web crawler depth and domain allow-list are operator-configured.
- LLM adapter Phase 2 (agentic loop) security review pending.
- Implementation-level security details: `../../src/ingestion/SECURITY.md`.
