---
Author: "ThemisDB Team"
Created: "2026-05-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "unknown"
Source_Level: "level1"
SOT_Domain: "security"
Canonical_References: "src/timeseries/TRUST.md; include/timeseries/tsstore.h; include/timeseries/encrypted_chunk_store.h; include/timeseries/prometheus_remote_write.h"
---

> Exact model backend not verifiable from final document text alone.

# Security - Timeseries Module

Report vulnerabilities through the project-level [`SECURITY.md`](../../SECURITY.md).
This page describes module boundaries; it is not a security certification or
deployment checklist.

## Relevant boundaries

- `TSStore` accepts caller-provided data and a non-owned RocksDB
  `TransactionDB`. The module does not establish caller identity or
  authorization.
- `PromWriteRequest` parses untrusted protobuf payloads. Parsing does not
  provide HTTP authentication, TLS, endpoint authorization, or request
  rate/size policy.
- `EncryptedChunkStore` relies on caller-supplied key and audit dependencies.
  An audit sink is not guaranteed unless the integrating application configures
  and verifies one.
- Retention operations delete data. The timeseries APIs do not define an
  undelete operation.
- Tags, metadata, metric/entity names, and diagnostics may contain sensitive
  or high-cardinality values; apply application-level access and redaction
  policy before export.

For code-specific trust boundaries, encryption behavior, and incident handling,
see [TRUST.md](TRUST.md). Deployment obligations are summarized in
[PRODUCTION_REQUIREMENTS.md](PRODUCTION_REQUIREMENTS.md); verify enforcement in
the owning server and platform sources. This documentation pass did not
perform a security audit or execute security tests.
