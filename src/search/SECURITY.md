> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Search Module

> Report vulnerabilities via the project-level [SECURITY.md](../../../SECURITY.md).

## Threat Model

| Threat | Mitigation |
|--------|-----------|
| Search query injection | Query sanitization and parameterized search |
| Information disclosure via search | Result filtering based on caller permissions <!-- TODO: verify --> |
| DoS via expensive queries | Resource limits enforced per-component (see below) |
| Index poisoning | Write-path authorization checks <!-- TODO: verify --> |
| Cross-tenant data leakage | Tenant-isolated `FederatedSearch` indexes |

## Security Controls

### Verified Resource Limits (from headers)

| Control | Location | Default |
|---------|----------|---------|
| `max_k` — hard upper bound on final result count | `include/search/hybrid_search.h:97` | 10,000 |
| `max_candidates` — hard upper bound on BM25/vector candidates | `include/search/hybrid_search.h:98` | 10,000 |
| `max_scan` — posting-list scan limit for negative keyword filter | `include/search/negative_keyword_filter.h` | configured per instance |
| `max_rewrite_length` — max character length of LLM-rewritten queries | `include/search/llm_query_rewriter.h:106` | 256 |

### Tenant Isolation

`FederatedSearch` (`include/search/federated_search.h`) enforces complete result isolation between tenant indexes: a result from tenant A cannot be boosted or suppressed by data from tenant B. Each result carries an explicit `tenant_id` field. A weight of 0 excludes a tenant from merged results entirely.

### Unverified Controls

- Search results filtered by document-level ACLs <!-- TODO: verify -->
- Query rate limiting enforced per principal <!-- TODO: verify -->
- Audit logging for all search operations <!-- TODO: verify -->

## Known Limitations

None critical.
