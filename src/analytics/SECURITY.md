> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Analytics Module

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../../SECURITY.md).

## Security Scope

The Analytics module processes query result data for OLAP aggregations, CEP pattern matching, anomaly detection, and ML inference. Security concerns center on: tenant data isolation, protection of ML model serving endpoints, safe event processing under backpressure, and secure external ML tool integration.

## Threat Model

| Threat | Mitigation |
|--------|------------|
| Cross-tenant data leakage in OLAP queries | All query operations are scoped to a `tenant_id`; `DistributedOLAPEngine` enforces per-tenant data isolation at `SourceRegistry` boundary |
| CEP rule injection via malformed EPL | EPL parser uses a structured grammar with explicit token validation; untrusted rule strings are rejected before NFA compilation |
| Denial of service via large event streams | CEP engine enforces queue depth limit, configurable drop policy, and backpressure signal; Prometheus metrics expose saturation |
| Model poisoning via ONNX/TF Serving endpoint | `MLServingClient` uses TLS connections to ONNX Runtime and TensorFlow Serving; no model files are loaded from user-controlled paths |
| Memory exhaustion via unbounded aggregation | Per-window size limits enforced in `TumblingWindow`, `SlidingWindow`, `SessionWindow`; watermark-based eviction removes stale state |
| Exfiltration via Arrow Flight RPC | Arrow Flight server is bound to configured listen address; authentication delegated to the auth module |
| NLP/LLM prompt injection | LLM process analyzer sends structured prompts with user-data in clearly separated context fields; outputs are parsed, not executed |

## Security Controls

### Tenant Isolation
- All OLAP and analytics operations receive a `tenant_id` parameter; cross-tenant query dispatch is blocked at the `DistributedOLAPEngine::SourceRegistry` boundary.
- Partial shard failure tolerance: results are returned if fewer than 20% of shards fail; PERMISSION_DENIED errors are never masked as partial failures.

### CEP Engine
- EPL rules are parsed and validated against a strict grammar before compilation into NFA states.
- Stateful pattern checkpointing (`serializeState()`/`restoreState()`) writes to protected storage; deserialized state is schema-validated before use.
- Backpressure signals prevent unbounded queuing under high ingest rates.

### External ML Integration
- `MLServingClient` communicates with ONNX Runtime and TensorFlow Serving over TLS.
- Model version pinning prevents silent model replacement.
- Graceful degradation when backends are absent — no fallback to user-supplied code paths.

### Arrow and Parquet Export
- All export operations within analytics go through the exporters module, which enforces `PolicyEngine` authorization.
- Columnar Arrow data in-process never bypasses RBAC checks.

## Data Handling

- Analytics processes aggregated or anonymized query results; raw documents are not buffered in analytics state.
- LLM process analyzer sends process event summaries (not document content) to configured LLM providers; opt-in only.
- CEP pattern state (partial matches) is held in memory only; checkpoint serialization is encrypted at rest when configured.
- Anomaly detection models are trained on per-tenant data and are never shared across tenants.

## Known Limitations

- Federated analytics across clusters has not yet been hardened for cross-cluster mTLS authentication.
- Arrow Flight RPC authentication relies on the calling layer; unauthenticated in-process transport is permitted in test mode.
- AutoML model explainability output (SHAP values) may reveal training data distribution; restrict access to admin roles.

## Dependency Security

| Dependency | Purpose | Notes |
|------------|---------|-------|
| Apache Arrow | Columnar data, IPC, Flight RPC | Well-maintained; keep patched |
| Apache Parquet | Columnar export | Uses Arrow Parquet writer |
| ONNX Runtime | Local ML inference | TLS connection; model files from trusted storage only |
| TensorFlow Serving | Remote ML inference | TLS REST endpoint; version-pinned |
| spdlog | Internal logging | No user data in log messages |
