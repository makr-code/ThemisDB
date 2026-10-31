# Security — Document Module

<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ARCHITECTURE.md · PRODUCTION_REQUIREMENTS.md · ROADMAP.md -->

Report vulnerabilities through the project-level `SECURITY.md`.

## Scope and implemented behavior

The document APIs provide typed operation boundaries, schema field validation, merge conflict reporting, lifecycle callbacks, and error classification. These behaviors are not a substitute for deployment authorization, durable storage security, or XML schema validation.

| Area | Source-backed behavior | Limit |
|---|---|---|
| Store and manager | Result-based store/manager operations; in-memory store uses a mutex | In-memory implementation is not durable and does not implement an authorization policy |
| Schema | In-memory registry checks required fields and configured JSON types | Round-trip editor does not invoke schema validation automatically |
| Diff/merge | Conflict-aware merge strategies and structured errors | Interface does not define authorization or audit policy |
| Encrypted entities | `encrypted_entities.h` serializes fields when represented as encrypted; the manager helper returns an in-memory entity handle | `createEncrypted()` stores its supplied JSON body as-is in the in-memory store; `InMemoryEncryptedEntity` only records a key ID and has no bytes to re-cipher |
| XDOMEA | Typed import/export and repository interfaces | In-memory XML handling is not XSD validation |
| Round-trip | Snapshot IDs combine relay ID and interaction index; store errors propagate | Persistence and access control depend on the supplied store/integration |

## Deployment follow-ups

- TODO: verify authorization and collection isolation in the concrete store/manager integration.
- TODO: define and validate the XML/XSD boundary for production XDOMEA use.
- TODO: document durable-store encryption, retention, backup, and audit controls in the owning deployment/integration documentation.
- TODO: establish whether key rotation must re-encrypt persisted document bytes; the in-memory helper alone does not do so.

These are integration questions, not claims that a vulnerability has been demonstrated. See [PRODUCTION_REQUIREMENTS.md](PRODUCTION_REQUIREMENTS.md) for the distinction between module behavior and deployment obligations.

Operational reference: [`docs/operability/RUNBOOK_DOCUMENT_STORE.md`](../../docs/operability/RUNBOOK_DOCUMENT_STORE.md). The runbook's presence does not establish that alerts, tracing, or deployment controls are wired or exercised.