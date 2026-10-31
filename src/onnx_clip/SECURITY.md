---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Security — ONNX CLIP Reference Backend

Exact model backend not verifiable from final document text alone.

## Scope and trust boundaries

This assessment describes the live implementation in
[`onnx_clip_plugin.cpp`](./onnx_clip_plugin.cpp), not a future ONNX inference
runtime. The plugin accepts config and caller-controlled byte/text inputs,
returns deterministic hash/PRNG vectors, and may read or map a configured file.
It does not decode images, execute model operators, authenticate callers,
authorize index access, enforce rate limits, persist vectors, or export audit
events.

The host application remains responsible for authorization, request-size
limits, vector-store access, retention, privacy review, and incident logging.
See [`TRUST.md`](./TRUST.md) for a boundary-by-boundary trust model.

## Current risks and controls

| Risk | Current behavior | Required control / limitation |
|---|---|---|
| Large/untrusted payloads | The image path accepts opaque byte vectors; text is tokenized in memory. Empty inputs fail, but there is no maximum byte length, total batch count, or text length in this plugin. | Enforce per-request byte and count limits in the host/API layer. |
| Oversized output allocation | Positive `model.embedding_dim` values and configured `max_batch_size` have no upper clamp. Batch chunking does not limit total result allocation. | Validate maximum dimension and request size before calling the plugin. |
| Model path substitution | A configured file may be read for hashing or mapped read-only. Mapping does not make the file trusted and its bytes are not used in inference. | Use immutable, access-controlled paths and an artifact digest from a trusted deployment source. |
| Digest check bypass | Digest checking requires both path and expected digest. OpenSSL builds compute SHA-256; non-OpenSSL builds call the injected `ModelHashFn`, but skip checking if no callback exists. | Hardened builds must ensure an active verifier; test the actual build configuration. |
| Misleading model/backend labels | `model.name` is reported as a label. `AUTO` selects CPU; other backend enum values are stored but not used to select inference. | Do not infer model loading, provider use, or semantic capability from labels or `isReady()`. |
| Sensitive input-derived data | Outputs are deterministic and depend on input bytes/text; repeated inputs yield repeatable vectors. | Treat derived vectors and inputs under the host's data-classification, access, encryption, and retention policies. Do not claim anonymity or non-biometric status. |
| Reload timeout ambiguity | `reloadModel()` publishes a valid new state before waiting for old requests. A drain timeout returns false after the new state is active. | On false, inspect statistics/config before deciding whether to roll back. |

## Integrity and file handling

`model.expected_sha256` is compared byte-for-byte with the computed or injected
digest. When OpenSSL is enabled, unreadable files or mismatches fail
initialization/reload. Without OpenSSL, a registered `ModelHashFn` can provide
the digest; if none is registered, the code proceeds without verification.
There is no signature, trusted manifest, format validation, or provenance
check.

`enable_mmap_loading` requests a read-only mapping for a non-empty path on the
implemented POSIX/Windows code paths. Mapping failures are ignored and
initialization can still become ready. The mapped pointer is not connected to
an inference session. Do not treat mmap success as a model-validity or security
check.

## Incident handling

If an expected digest fails, keep the plugin out of traffic and verify the
artifact and digest through the trusted deployment pipeline; do not turn off
verification as an availability workaround. If a non-OpenSSL deployment has
no callback, classify that as a configuration-control failure when integrity
verification is required.

For suspected path replacement, preserve deployment metadata and host
filesystem/audit evidence, then replace the file only through the approved
artifact deployment process. For possible input/vector exposure, follow the
host service's privacy/security incident process; the plugin itself cannot
identify callers or determine whether stored vectors were accessed.

On `reloadModel()` returning false, check `getStatistics()` because a
post-publication drain timeout leaves the new state active. To revert, use the
saved prior config in another reload and verify the resulting model label,
dimension, and readiness.

## Changes requiring security re-review

Introducing actual ONNX model execution changes the risk surface. Reassess
model provenance and operator execution, graph/resource limits, image
decoder/preprocessing behavior, tokenizer bounds, provider isolation,
concurrency, reload rollback, and output sensitivity. Require tests against
real model files and update [`TRUST.md`](./TRUST.md),
[`RUNBOOK.md`](./RUNBOOK.md), and [`OPERATIONS.md`](./OPERATIONS.md) with the
implementation.
