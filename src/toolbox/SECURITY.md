---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Toolbox security boundaries

**Scope:** This page describes source-visible behavior, not a security certification or complete incident taxonomy. Project-wide reporting instructions are in [`../../SECURITY.md`](../../SECURITY.md).

## Implemented boundaries

- `ContentToolboxBridge::ingest()` passes raw bytes to `ContentManager::ingestRawBlob()` before retrieving assembled text for toolbox enrichment. The security, authorization, scanning, and storage guarantees of that call belong to the content subsystem and host configuration.
- `enrichExisting()` reads existing metadata and assembled text; it does not repeat raw-blob ingestion. Callers must enforce authorization and decide whether re-enrichment is permitted.
- The registry throws when accessed before initialization. A repeated initialization replaces the previous instance and logs a warning.
- Optional graph and vector writer failures are logged and counted separately. They are soft failures; `BridgeResult::ok` can remain true, so callers requiring durable writes must check the relevant outcome.
- Language and boilerplate outputs are heuristics, not security validation or access-control decisions. SHA-256 fingerprints identify bytes but do not authenticate them or prove provenance.
- Error messages and IDs may be returned or logged. The host must apply its own redaction and sensitive-data handling policies.

## Metrics and coverage limits

- Extraction and bridge objects expose text metrics for counters/failure observations. The global registry misuse counter is appended by the registry free-function metric export when non-zero; this is not an HTTP endpoint.
- Helper metric accessors exist, but ordinary helper input paths do not increment their error counters. Do not interpret zero helper counters as evidence that input was valid or that all helper incidents are covered.
- No complete source-defined set of incident IDs or exhaustive security-test coverage is established by the current toolbox implementation. Test presence does not establish production safety or release certification.

## Integration guidance

Avoid logging raw document content, credentials, or full secret-bearing configuration. Before retrying, establish whether content storage or an optional sink may already have committed data. For operational triage see [`RUNBOOK.md`](./RUNBOOK.md); for dataflow and result semantics see [`ARCHITECTURE.md`](./ARCHITECTURE.md) and [`TRUST.md`](./TRUST.md).

> Exact model backend not verifiable from final document text alone.
