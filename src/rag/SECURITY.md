> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — RAG Module

> Report vulnerabilities via the project-level [SECURITY.md](../../../SECURITY.md).

## Threat Model

| Threat | Mitigation |
|--------|-----------|
| Prompt injection via retrieved documents | Input sanitization; retrieved content treated as untrusted data |
| Data exfiltration via LLM | Output filtering; PII detection before returning answers |
| Unauthorized document access | Auth checks before retrieval; collection-level ACLs |
| Poisoned embeddings | Embedding validation; source provenance tracking |
| Model extraction via repeated queries | Rate limiting via server/rate_limiter_v2 |

## Security Controls

- All retrieval operations enforce collection-level authorization
- Source documents are attributed in responses to enable auditing
- PII filtering applied to retrieved context before LLM processing
- Audit logging for all RAG query operations

## Known Limitations

- Cross-encoder reranking may expose partial document content to re-ranking models
- Streaming responses cannot be recalled once delivered

## Dependency Security

- Depends on `llm` module for generation (see llm/SECURITY.md)
- Depends on `index` module for vector retrieval
