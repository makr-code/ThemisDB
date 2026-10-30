---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Distributed Knowledge Trust Model

## Purpose and scope

For security reviewers and operators assessing data crossing shard boundaries.
This page describes trust controls exposed by the module and the limits of
those controls. The canonical production requirements and
[SECURITY.md](./SECURITY.md) remain authoritative for mandatory controls.

## Boundaries and assets

The module receives JSON-derived capability announcements, gradient
contributions, retrieval results, feedback summaries, and soft labels from
callers. These records can contain model updates, ranking information, vector
data, identifiers, and policy-sensitive metadata. The module's in-process
validation and callbacks do not authenticate the remote sender or secure the
network connection. Transport authentication, authorization, encryption,
tenant isolation, and access to persisted copies must be enforced by the
integrating service.

`EncryptedGradient` is named as an encrypted contribution but stores an opaque
JSON `data` value; this module does not perform cryptographic encryption of
that value. JSON conversion is serialization, not a trust check. Validate
sender identity and payload schema before accepting or forwarding it.

## Available controls and caveats

- `IFederationTrustPolicy` exposes a trust-gate decision and the API contract
  assigns `TRUST_GATE_REJECTED` to rejected operations. The
  `AlwaysPermitTrustPolicy` implementation permits every announcement; it
  must not be used where peer trust needs enforcement.
- Feedback sync supports inbound policy and zero-trust callbacks, dimension
  validation, origin anonymization, and summary-ID deduplication. These do not
  prove that feedback content or embeddings are anonymous or non-sensitive.
- LoRA and distillation coordinators expose differential-privacy parameters,
  round/budget accounting, and callbacks for audit or policy. Privacy
  guarantees depend on correctly calibrated inputs, mechanisms, composition,
  and durable budget ownership.
- LoRA supports an optional outlier filter. It is an input-quality control,
  not a substitute for authorization or a poisoning-resistance proof.
- Cross-border policy and shard-location configuration are exposed by the
  LoRA coordinator. The deployment must supply accurate locations and a
  policy appropriate to the data.
- Diagnostic events may contain shard IDs, operation IDs, causes, and metadata.
  Limit access and retention and avoid placing secrets or raw user content in
  those fields.
- Erasure methods operate on component-local state or counters; do not treat
  them as proof that remote shards, backups, logs, or consumer stores have been
  erased. Coordinate subject-rights workflows across all owning systems.

## Secure operation and incident handling

Use explicit deny-by-default policy callbacks where the deployment requires
authorization. Keep privacy and trust checks enabled; the production
requirements prohibit disabling security checks and require audit logging for
security-relevant operations. Test rejected, malformed, replayed, and
out-of-policy inputs before deployment.

On suspected compromise or privacy leakage, stop the affected propagation
path, preserve minimal diagnostic evidence, identify recipients and
potentially exposed fields, and escalate through the project security and
privacy incident processes. Resume only after peer identity, policy, payload
handling, and any required erasure or notification workflow have been
reviewed. Operational triage details are in [RUNBOOK.md](./RUNBOOK.md).
