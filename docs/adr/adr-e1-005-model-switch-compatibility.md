# ADR E1-005: Model-switch compatibility

## Status

Accepted — implemented 2026-07-16, closes #5419

## Context

EPIC 1.5 requires a safe, auditable mechanism for switching the base model of
a deployed LoRA package.  The core tension is between:

1. **Compatibility assurance** — adapters trained on one model may not work
   on a different model (architecture, tokenizer, layer dimensions).
2. **Ratchet safety** — once a package is certified for a given minimum model
   version, it must not silently regress to an older one.
3. **Rebuild clarity** — operators must be told explicitly whether a switch
   requires a rebuild and why, rather than discovering failures at inference time.

## Decision

### Ratchet Compatibility Matrix

A `RatchetCompatibilityMatrix` maps `(adapter_id, model_family)` pairs to an
allowed model-version range `[min_version, max_version)`.  The minimum floor
can only be advanced by normal API calls; lowering the floor requires an
explicit `allow_downgrade = true` operator override.  The matrix is
serializable to JSON for version-controlled storage.

This gives:
- Explicit, auditable compatibility claims at the package level
- Prevention of silent downgrade to an older model
- Human-readable JSON for change management and CI diff review

### Six-gate compatibility check sequence

`ModelSwitchWorkflow::executeSwitch()` evaluates six ordered gate categories:

1. Ratchet matrix — target version within registered floor
2. Architecture — adapter and target model share the same architecture family
3. Tokenizer — same architectural family implies compatible tokenizer vocabulary
4. Layer dimensions — base model name identity check flags potential size changes
5. Quantization — known quantization format conflicts surfaced as rebuild signals
6. Prompt format — chat-template alignment for instruction-tuned adapters

Any hard failure (gate `passed = false`) returns `INCOMPATIBLE` immediately.
Rebuild signals are accumulated across all gates.

### Rebuild policy

A configurable `RebuildPolicy` maps check results to rebuild triggers.  When
`fail_closed_on_rebuild = true` (recommended for production) a required rebuild
blocks the switch with `BLOCKED` outcome rather than a warning.

### Orchestrator integration

On `COMPATIBLE` outcome the workflow calls
`FinalLayerOrchestrator::promotePackage()` to advance the package to STAGING,
enforcing the existing governance state-machine.  This keeps the model-switch
logic decoupled from the lifecycle management logic.

## Consequences

- All model-switch attempts produce a structured `ModelSwitchResult` with a
  JSON audit record suitable for package-event logging.
- The ratchet matrix is a separate artefact that must be maintained alongside
  adapter manifests.  Advancing the floor requires a code or config change,
  which is intentional (governance trace).
- Cross-architecture switches (e.g., llama → mistral) will consistently result
  in `REBUILD_REQUIRED` or `BLOCKED`; callers must not assume cross-family
  compatibility without an explicit retrain.

## Rejected alternatives

- **Implicit compatibility inference from model metadata alone**: rejected
  because metadata may be absent or stale at switch time; explicit matrix
  entries are more reliable.
- **Single validation call without rebuild policy**: rejected because it
  conflates compatibility detection with action policy; separating them allows
  different environments (dev vs. prod) to use different policies.

## Follow-up

- [ ] Integrate rebuild trigger into the LoRA training pipeline scheduler
- [ ] Add Prometheus metrics for `model_switch_outcome` by `package_id`
- [ ] Wire matrix JSON into the existing SBOM / artifact-signing pipeline
