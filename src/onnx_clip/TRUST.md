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

# ONNX CLIP Trust Boundaries

Exact model backend not verifiable from final document text alone.

## Trust model

`ONNXClipPlugin` receives configuration and opaque image/text input from its
host. The implementation returns deterministic derived vectors and local
statistics. It does not authenticate callers, enforce authorization, rate-limit
requests, decode image formats, load ONNX weights, or persist results. The host
application is responsible for caller authorization, request-size limits,
storage policy, and access control on any index containing derived vectors.

## Boundaries and risks

| Boundary | Risk | Current control / operator responsibility |
|---|---|---|
| Caller payload → plugin | Large byte vectors or batches can consume memory; payloads are not decoded or semantically validated. | Empty values are rejected, but callers must cap byte size, batch count, and total request allocation. |
| Configuration → plugin | `model.name` and backend values can create misleading operational labels; `model.embedding_dim` can increase output allocation. | Validate config in deployment. Non-positive dimensions reset to 512; positive dimensions and batch size have no upper clamp in this source. |
| Model path/digest → process | A configured path is read for digest calculation and may be mapped read-only. Filesystem replacement/permissions remain host concerns. | Use access-controlled immutable model directories and a trusted expected digest. Hash comparison is only enforced when both path and expected digest are set and a verifier is active. |
| Non-OpenSSL build → digest check | When no injected `ModelHashFn` is installed, the configured check is skipped. | Hardened deployments must verify build features and callback registration; do not interpret a successful initialize as verification when no verifier exists. |
| Plugin → embedding consumer | Deterministic vectors may be mistaken for semantic or privacy-preserving CLIP output. | Label this implementation as a reference backend; do not use it to claim model quality or to infer that vectors are anonymous. Apply the data policy required by the host index. |
| Host/plugin process → operators | This plugin has no authentication, secrets store, rate limiter, audit log, or Prometheus exporter. | Use host security controls and incident process; keep credentials out of configuration captured in diagnostics. |

## Artifact integrity is not inference

`model.expected_sha256` causes a digest comparison only when `model.path` is
also non-empty. In OpenSSL builds, source computes the file digest. Otherwise,
the process-wide callback may supply it; absent callback means the check is
skipped. A successful digest check proves only that bytes match the supplied
digest. It does not prove model provenance, validity, safety, or that those
bytes are used for inference.

Similarly, `enable_mmap_loading` can create a read-only mapping on supported
platforms, but the mapped pointer is not connected to an inference session.
Mapping is not signature verification or model loading.

## Embedding data handling

Image bytes and text influence deterministic hash/PRNG vectors. The returned
vector is reproducible for identical inputs/configuration and may reveal input
equality patterns to parties who can compare outputs. Do not describe outputs
as anonymous or non-sensitive. Apply the host's data minimization, encryption,
retention, and access-control rules, especially for image/text data concerning
people or other regulated content.

## Incident handling

If an unexpected model-path or digest change is detected, stop initialization
or reload, keep the affected plugin out of traffic, preserve the expected
digest and deployment audit evidence, and restore the artifact only through the
trusted deployment process. If a reload returns false, inspect current
statistics: a drain timeout is reported after the new implementation has
already been published. Treat suspected payload/vector exposure through the
host service's privacy and security incident process.

## Change boundary

Adding actual ONNX inference changes the trust model. Before that change is
enabled, review provider/operator execution, model provenance and format
validation, preprocessing/tokenizer attacks, memory exhaustion, concurrency,
and the host's access controls. Add model-backed security tests and update
[`SECURITY.md`](./SECURITY.md), [`RUNBOOK.md`](./RUNBOOK.md), and
[`OPERATIONS.md`](./OPERATIONS.md).
