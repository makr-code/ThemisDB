> **Roadmap-Hinweis:** Vage Bullets ohne Akzeptanzkriterien in Checkbox-Tasks ueberfuehren. Format: `- [ ] <Task> (Target: <Q/Jahr>)`.

<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

# Server Module Roadmap

## Current Status
Production-ready server stack with HTTP/1.1, HTTP/2, HTTP/3, WebSocket, MQTT, PostgreSQL wire protocol, gRPC, GraphQL, and MCP integration. Core API gateway, auth middleware, validation, and observability paths are available in production deployments.

## In Progress
- [~] P0 security/code-quality remediation wave for server paths (Target: Q2 2026)
  - [ ] Finish remaining true-positive triage from gap scan and remove residual high-risk findings from active code paths (Target: Q2 2026)
  - [ ] Consolidate auth enforcement checks for all routing-layer special cases and keep regression tests green (Target: Q2 2026)

## Planned Features

### Short-term (3-6 months)
- [ ] Plugin-based server adapter loading with signature validation and rollback guardrails (Target: Q4 2026)
- [ ] Cluster-wide distributed rate-limit state hardening for mixed-node latency profiles (Target: Q4 2026)
- [ ] GraphQL federation and schema governance hardening for multi-service deployments (Target: Q4 2026)
- [ ] HTTP/3 congestion-control and connection migration tuning under production-like packet loss (Target: Q4 2026)

### Mid-term (6-12 months)
- [ ] Passwordless WebAuthn/FIDO2 auth integration for admin and API scopes (Target: Q1 2027)
- [ ] CPU- and memory-governed WASM execution hardening with stricter runtime policy envelopes (Target: Q1 2027)
- [ ] Service-mesh policy sync hardening and failover behavior validation under partition scenarios (Target: Q1 2027)

## Implementation Phases

### Phase 1: Security and Access Hardening
- [ ] Complete route-by-route auth gate audit for privileged server endpoints (Target: Q2 2026)
- [ ] Close remaining scanner-confirmed high-severity auth/logging findings with regression tests (Target: Q2 2026)

### Phase 2: Protocol and Gateway Hardening
- [ ] Improve HTTP/3 production behavior under migration/retransmit stress (Target: Q4 2026)
- [ ] Extend gateway resilience tests for quorum loss and split-brain protection paths (Target: Q4 2026)

### Phase 3: Validation and Contract Governance
- [ ] Strengthen OpenAPI/JSON-Schema drift detection for handler registration changes (Target: Q4 2026)
- [ ] Add stricter backward-compat checks for gRPC and REST versioning contracts (Target: Q4 2026)

### Phase 4: Tests and Reliability Gates
- [ ] Expand integration and soak coverage for mixed protocol traffic (HTTP/gRPC/WebSocket/MQTT) (Target: Q4 2026)
- [ ] Add deterministic fault-injection tests for distributed rate-limit and fallback behavior (Target: Q4 2026)

### Phase 5: Performance and Operational Hardening
- [ ] Re-baseline server latency/throughput gates with production-like payload mixes (Target: Q1 2027)
- [ ] Add adaptive tuning recommendations for queue/backpressure settings by deployment profile (Target: Q1 2027)

### Phase 6: Documentation and Release Readiness
- [ ] Keep server developer docs aligned with source and routing behavior after each hardening wave (Target: Q2 2026)
- [ ] Ensure completed roadmap items are moved only to CHANGELOG and not retained in roadmap history blocks (Target: ongoing)

## Production Readiness Checklist
- Status: Tracking in progress
- Nachweise: Integration tests, focused protocol tests, and security regression suites
- Hinweis: Abgeschlossene Arbeit wird ausschliesslich in CHANGELOG dokumentiert.

## Known Issues and Limitations
- Plugin-based adapter loading still requires roadmap delivery.
- Some advanced protocol features require additional soak/fault-injection validation before hard SLA commitments.
- Cross-node consistency for globally distributed rate limits needs further hardening evidence.

## Breaking Changes
- REST versioning remains path-based and backward-compatible for v1 clients.
- gRPC schema evolution remains additive-only for active major lines.
