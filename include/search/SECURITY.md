<!-- Status: current | validated: 2026-03-22 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Search Module

## Scope

Covers all public headers in `include/search/`. Implementation hardening in `../../src/search/`.

## Threat Model

| Threat | Impact | Mitigation |
|---|---|---|
| Search highlight HTML injection | High — XSS | `SearchHighlighter` HTML-escapes all document content before inserting highlight spans |
| LLM query rewriting prompt injection | High — query manipulation | `LlmQueryRewriter` wraps user queries in a fixed system prompt; injection patterns screened |
| Distributed search shard MITM | High — result tampering | `DistributedHybridSearch` requires mTLS for all shard connections |
| Query-based data exfiltration via facets | Medium — PII exposure | `FacetedSearch` enforces RBAC field-level visibility before including fields in facet results |
| Cross-lingual search mistranslation abuse | Low — relevance gaming | `CrossLingualSearch` translation is logged and auditable; no direct user control over translation model |
| LTR model poisoning | Medium — ranking manipulation | `LearningToRank` model updates require operator authentication; training data validated |
| Zero-result information disclosure | Low — index structure enumeration | `SearchAnalytics` zero-result logs are access-controlled; not exposed via public API |
| Negative keyword filter bypass | Low — unwanted results | `NegativeKeywordFilter` parses at tokenization level; cannot be bypassed by URL encoding |

## Security Controls

1. **HTML escaping in highlights** — `SearchHighlighter` escapes `<`, `>`, `&`, `"` in all document content.
2. **mTLS on distributed search** — `DistributedHybridSearch` enforces mutual TLS for all inter-shard connections.
3. **RBAC on facets** — `FacetedSearch` checks field-level read permissions before including in results.
4. **Prompt injection screening** — `LlmQueryRewriter` applies `PromptInjectionDetector` (shared with prompt_engineering module) before LLM call.
5. **Authenticated LTR updates** — `LearningToRank` model updates require signed operator token.
6. **Audit log for cross-lingual** — `CrossLingualSearch` logs all translation operations for audit.

## Known Limitations

- `MultiModalSearch` image and audio modalities are not yet scanned for embedded adversarial content — tracked for Q4 2026.
- Federated search (planned Q4 2026) across isolated tenant indexes will require new trust boundary review.
