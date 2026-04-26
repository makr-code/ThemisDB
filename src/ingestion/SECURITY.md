> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Ingestion Module

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../../SECURITY.md).

## Security Scope

This document covers the security posture of the Ingestion module, including all data-intake connectors
(FileSystem, HuggingFace, GenericAPI, Kafka, S3/GCS/Azure Blob, CDC, JDBC/ODBC, WebCrawler),
the distributed coordinator, the LLM-driven deontic extraction pipeline, and the admin API.

## Threat Model

| Threat | Attack Vector | Mitigation |
|--------|--------------|------------|
| SSRF via WebCrawler | Attacker-controlled URL targeting internal services | Allow-list restricts schemes to `http`/`https`; private IP ranges (RFC 1918, RFC 4193, loopback) are blocked before connection |
| Path traversal in FileSystem ingester | URL-encoded or literal `../` in configured paths | All paths are normalised and rejected if they contain `..` components after resolution |
| API key / credential leakage in logs | Structured log lines containing auth headers | Credential masking layer redacts `Authorization`, `X-Api-Key`, and bearer token fields before any log sink |
| Prompt injection in LLM pipeline | Malicious text in ingested documents reaching `DeonticExtractor` | Regex-based `SemanticValidator` strips and flags known injection patterns before content reaches the LLM |
| Unvalidated schema data | Malformed records bypassing type checks | Per-source JSON Schema (draft-07) validation is enforced at connector boundary; non-conforming records are quarantined |
| OAuth token leakage | Token written to logs on refresh failure | OAuth token values are never passed to the logger; only HTTP status codes are recorded |
| TLS certificate bypass | Misconfigured CA path allowing MitM | CA bundle path is validated for existence and readability at startup; `CURLOPT_SSL_VERIFYPEER` is always enabled; disabling peer verification is not supported |
| Malicious file content (PDF/DOCX) | Parser exploitation via crafted binary payloads | Binary MIME detection gates parsing; unsupported or unexpected MIME types are quarantined, not parsed |
| Kafka topic enumeration | Unauthenticated broker access | Kafka connector requires SASL/SCRAM or mTLS; plaintext mode is compile-time disabled in production builds |
| CDC slot abuse | Replication slot not released, causing WAL accumulation | Slot lifecycle is tied to connector session; slots are dropped on clean shutdown and on reconnect if detected stale |

## Security Controls

### Network
- WebCrawler: egress restricted to public routable addresses only; DNS resolution result is checked against blocklist before connection is established.
- All HTTP(S) connections use libcurl with peer and host verification enabled.
- Kafka and CDC connectors enforce encrypted transport in production configuration.

### Authentication & Authorisation
- OAuth 2.0 client credentials flow with automatic token refresh; tokens stored only in process memory, never on disk or in logs.
- Admin API (pause/resume/quarantine) requires an authenticated session; unauthenticated requests receive HTTP 401.
- S3/GCS/Azure connectors use short-lived credentials (IAM roles / Workload Identity / Managed Identity) where available.

### Input Validation
- Schema validation is the first processing step after raw record receipt; records failing validation never reach downstream storage.
- `AgenticReferenceValidator` cross-checks LLM-extracted deontic references against a known-good corpus before persistence.

### Secrets Management
- No credentials are hard-coded; all secrets are injected via environment variables or a secrets provider at runtime.
- Credential masking is applied globally to the structured logger and cannot be bypassed by connector code.

## Data Handling

- Ingested records containing PII are subject to the same retention and access controls as the rest of ThemisDB.
- Lineage metadata is stored alongside ingested documents to support auditing and data-subject requests.
- Quarantined records are stored in an isolated queue; access requires elevated privilege.
- Dry-run mode does not persist any data; it is safe to use against production sources for validation purposes.

## Known Limitations

- Phase 2 LLM pipeline (LoRA, SpaCy, agentic verification loop) is not yet complete; the current regex-based prompt-injection mitigation may not cover novel injection techniques introduced by future model integrations.
- WebCrawler private IP blocklist covers IPv4 RFC 1918 and IPv6 RFC 4193 ranges; non-standard private ranges (e.g. carrier-grade NAT 100.64.0.0/10) are blocked but rely on correct DNS resolution — DNS rebinding attacks are not fully mitigated.
- JDBC/ODBC connector relies on the security posture of the underlying driver; driver provenance should be verified before deployment.

## Dependency Security

| Dependency | Purpose | Notes |
|------------|---------|-------|
| libcurl | HTTP transport for GenericAPI and WebCrawler | TLS enforced; version pinned in vcpkg.json |
| pugixml | HTML/XML parsing in FileSystem ingester | No network access; input size limits enforced |
| librdkafka 2.x | Kafka consumer | SASL/mTLS required in production |
| OpenSSL / system TLS | Certificate verification | CA bundle path validated at startup |
