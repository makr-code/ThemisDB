> Author: ThemisDB Team
> Created: 2026-10-30
> Last Updated: 2026-10-30
> Status: active
> AI_Used: true
> AI_Models: unknown
> AI_Contributor: GitHub Copilot
> Human_Review: false
> Human_Author: ThemisDB Team
> Source level: level1
> SOT domain: security
> Canonical source references: [graph security guidance](SECURITY.md), [path constraints](../../include/graph/path_constraints.h), [graph index interface](../../include/index/graph_index.h), [root security policy](../../SECURITY.md)
> Related issue: #7122
>
> Exact model backend not verifiable from final document text alone.

# Graph trust boundaries and security

## Trust boundaries and ownership

| Boundary | Graph module responsibility | Owner outside this module |
|---|---|---|
| Caller input → graph query | Validate supported graph inputs/constraints and return explicit errors where implemented. | Request parsing, caller identity, API authentication and authorization, and tenant entitlement belong to the API/server boundary. `QueryConstraints` and `PathConstraints` are not authorization decisions. Verify the effective policy in the server/access-control integration. |
| Graph algorithm → graph records | Use graph-index interfaces for adjacency, paths, and graph record access. | Canonical graph persistence, database transactions, raw blocks, WAL, durability, and backup/restore belong to index/storage. |
| Graph module → API transport | Provide C++ graph behavior consumed by callers. | HTTP/wire serialization and request lifecycle are in the server API layer, including [graph_api_handler.h](../../include/server/graph_api_handler.h). The handler stores an `AuthMiddleware`, but no handler method calls `auth_` in [graph_api_handler.cpp](../../src/server/graph_api_handler.cpp). Verify enforcement at the server/router boundary; constructor wiring does not establish an authorization policy. |
| Graph module → acceleration/shards | Report fallback, truncation, and errors; coordinate registered executors. | Device drivers, backend availability, shard health, network/RPC service operation, and infrastructure remediation belong to their respective owners. |
| Learned/reasoned output → application | Return ranked candidates, inferred facts, and explanations. | The application/domain owner decides whether to review, persist, expose, or act on those outputs. Scores and inferences are not automatically authoritative facts. |

## Risks and controls

- **Unbounded or adversarial traversal:** depth/result limits, timeouts, frontier controls, and constraints reduce expansion risk where callers supply and enforce them. Set bounds explicitly and test high fan-out cases.
- **Malformed identifiers and fields:** `PathConstraints` checks null bytes and length/field-name limits, but invalid values in several `void` mutators are ignored rather than returned as errors. Validate at the caller boundary and verify the resulting constraint set.
- **Constraint confusion:** required/forbidden nodes, properties, and ontology rules constrain a graph operation; they do not prove that the caller may see or traverse a tenant's data. Enforce authorization before invoking graph operations.
- **Acceleration or shard degradation:** use the exposed fallback/error outcomes and verify result completeness. Never treat a partial distributed result as a full graph answer.
- **Semantic conflicts or model hallucination:** reasoning conflicts must be surfaced for review. RotatE rankings are distances, not calibrated probabilities; do not automatically promote model predictions to trusted facts without application policy.
- **Sensitive metadata leakage:** graph IDs, tenant labels, node IDs, query context, and error diagnostics may be sensitive. The integrating service must apply its logging and redaction policy.
- **Refresh mutation risk:** honor scheduled refresh safety gates and review its audit trail before continuing after an abort or anomalous removal rate.
- **Watermark scope:** watermark/fingerprint metadata can help identify a marked snapshot but does not provide confidentiality, integrity protection, tenant isolation, or access control.

## Incident handling

For suspected unauthorized access, cross-tenant exposure, corrupted graph state, or unsafe inference exposure, preserve request/error/audit evidence and escalate through the root [SECURITY.md](../../SECURITY.md) process and the responsible server/storage owner. Do not use graph-level constraints or watermark metadata as incident containment.

For non-security graph failures, use [RUNBOOK.md](RUNBOOK.md), inspect the error class in [graph_error_taxonomy.h](../../include/graph/graph_error_taxonomy.h), and reproduce with a focused [graph test](../../tests/graph/). Escalate infrastructure failures to the owning server, storage, shard, or accelerator team; the graph module cannot repair those dependencies.

## Canonical references

[Graph SECURITY.md](SECURITY.md) · [Graph architecture](ARCHITECTURE.md) · [Graph API](API.md) · [Root SECURITY.md](../../SECURITY.md) · [Graph error taxonomy](../../include/graph/graph_error_taxonomy.h)

## Graph developer pages

[RESEARCH](RESEARCH.md) · [MODELING](MODELING.md) · [EMBEDDING](EMBEDDING.md) · [IMPLEMENTATION](IMPLEMENTATION.md) · [API](API.md) · [MODULE_GUIDE](MODULE_GUIDE.md) · [RUNBOOK](RUNBOOK.md) · [OPERATIONS](OPERATIONS.md) · [TRUST](TRUST.md)
