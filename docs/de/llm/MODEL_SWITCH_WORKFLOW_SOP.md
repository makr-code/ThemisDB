# Model-Switch Workflow — SOP / Runbook

> **Audience:** LLM platform operators, ML engineers, on-call SREs  
> **Module:** `include/llm/model_switch_workflow.h` — `src/llm/model_switch_workflow.cpp`  
> **Phase:** EPIC 1.5, Phase 6 — LLM Resilienz / Rebuild / Evaluierung  
> **Status:** ✅ Production Ready

---

## 1. Purpose

This runbook describes the end-to-end procedure for safely switching the base
model for one or more LoRA packages in ThemisDB.  It covers:

- Pre-switch compatibility assessment
- Ratchet-matrix verification
- Adapter invalidation and rebuild decisions
- Policy-safe migration (prompt, tokenizer, layer checks)
- Post-switch validation and rollback

---

## 2. Core Concepts

### 2.1 Ratchet Compatibility Matrix

A *ratchet matrix* maps `(adapter_id, model_family)` to a versioned range
`[min_version, max_version)`.  "Ratchet" means the minimum floor can only be
advanced, never silently lowered.  This prevents accidental deployment of an
adapter on an older model after the team has certified it for a newer one.

```
RatchetCompatibilityMatrix schema_version="1.0.0"
  adapter_id="legal-general"  model_family="llama"
    min_model_version = 3.0
    max_model_version_excl = 4.0   # exclusive upper bound; 0.0.0 = unbounded
```

The matrix is serialisable to JSON for version-controlled storage alongside
package manifests.

### 2.2 Rebuild Policy

The `RebuildPolicy` struct lists which changes mandate a rebuild:

| Trigger | Condition |
|---|---|
| `ARCHITECTURE_CHANGE` | Model architecture (llama → mistral) changes |
| `TOKENIZER_CHANGE`    | Tokenizer family or vocabulary differs |
| `LAYER_DIMENSION_CHANGE` | Hidden size or layer count differs |
| `VERSION_OUT_OF_RANGE`   | Target version falls outside the ratchet range |

When `fail_closed_on_rebuild = true` a triggered rebuild requirement blocks
the switch with `BLOCKED` outcome rather than surfacing a `REBUILD_REQUIRED`
warning.  Use `fail_closed_on_rebuild = true` in production environments.

### 2.3 Switch Outcomes

| Outcome | Meaning | Adapter safe to serve? |
|---|---|---|
| `COMPATIBLE`         | All checks passed; package promoted to STAGING | ✅ Yes |
| `REBUILD_REQUIRED`   | Rebuild recommended but not blocking | ⚠️ No (rebuild first) |
| `BLOCKED`            | Rebuild required and fail-closed policy active | ❌ No |
| `INCOMPATIBLE`       | Hard incompatibility (e.g. ratchet floor violation) | ❌ No |

---

## 3. Pre-Switch Checklist

Before initiating a model switch, verify:

- [ ] Target model binary is available on all serving nodes
- [ ] Ratchet-matrix entry for the adapter covers the target version
- [ ] Training data and LoRA config are available in case a rebuild is needed
- [ ] Rollback target package (`PREVIOUS_KNOWN_GOOD`) exists and is verified
- [ ] Monitoring dashboards are open; alerting thresholds reviewed
- [ ] Change window is approved

---

## 4. Step-by-Step Execution

### Step 1 — Construct the workflow

```cpp
#include "llm/model_switch_workflow.h"
using namespace themis::llm;

// 1a. Load or build the ratchet matrix from a versioned JSON file.
auto matrix_json  = load_json("/etc/themisdb/compat_matrix.json");
auto matrix       = RatchetCompatibilityMatrix::fromJson(matrix_json);

// 1b. Configure the rebuild policy for production (fail-closed).
RebuildPolicy policy;
policy.fail_closed_on_rebuild = true;
policy.triggers = {
    RebuildTrigger::ARCHITECTURE_CHANGE,
    RebuildTrigger::TOKENIZER_CHANGE,
    RebuildTrigger::VERSION_OUT_OF_RANGE,
};

// 1c. Wire up registry and orchestrator (from your DI container / service locator).
ModelSwitchWorkflow workflow(registry, orchestrator, std::move(matrix), policy);
```

### Step 2 — Submit the switch request

```cpp
ModelSwitchRequest req;
req.package_id           = "pkg-legal-qa";
req.source_model_name    = "llama-7b";
req.source_model_version = "3.0";
req.target_model_name    = "llama-7b";
req.target_model_version = "3.1";
req.target_model_family  = "llama";
req.correlation_id       = generate_uuid();

ModelSwitchResult result = workflow.executeSwitch(req);
```

### Step 3 — Evaluate the outcome

```cpp
switch (result.outcome) {
    case ModelSwitchOutcome::COMPATIBLE:
        // Package is now at STAGING; continue promotion workflow.
        log_info("Switch compatible — proceed with canary promotion");
        break;

    case ModelSwitchOutcome::REBUILD_REQUIRED:
        // Trigger adapter rebuild, then re-run switch.
        log_warn("Rebuild required before serving");
        trigger_rebuild(req.package_id, req.target_model_name);
        break;

    case ModelSwitchOutcome::BLOCKED:
        // Fail-closed: stop here, escalate to engineering.
        log_error("Switch blocked — fail-closed rebuild policy active");
        send_alert("LLM_SWITCH_BLOCKED", result.toJson());
        break;

    case ModelSwitchOutcome::INCOMPATIBLE:
        // Hard failure: check errors for root cause.
        for (const auto& e : result.errors) {
            log_error("Incompatibility: " + e);
        }
        initiate_rollback(req.package_id);
        break;
}
```

### Step 4 — Inspect the audit trail

The `ModelSwitchResult::toJson()` method produces a structured audit record
that should be appended to the package-event log:

```json
{
  "outcome": 0,
  "can_serve": true,
  "needs_rebuild": false,
  "correlation_id": "4b3a-...",
  "checks": [
    { "kind": 0, "passed": true,  "rebuild_required": false, "message": "Ratchet matrix: compatible" },
    { "kind": 1, "passed": true,  "rebuild_required": false, "message": "Architecture: compatible" },
    { "kind": 2, "passed": true,  "rebuild_required": false, "message": "Tokenizer: compatible (same architectural family)" },
    ...
  ],
  "active_rebuild_triggers": [],
  "errors": [],
  "warnings": []
}
```

Store this JSON alongside the package manifest for compliance traceability.

---

## 5. Rebuild Runbook

### 5.1 Detection

A switch with `REBUILD_REQUIRED` or `BLOCKED` outcome carries populated
`active_rebuild_triggers` in the result.  Each trigger corresponds to a
specific incompatibility:

| Trigger | Root Cause | Resolution |
|---|---|---|
| `ARCHITECTURE_CHANGE` | Target model uses a different architecture | Full retrain from training data |
| `TOKENIZER_CHANGE`    | Tokenizer vocabulary diverged            | Full retrain with new tokenizer |
| `LAYER_DIMENSION_CHANGE` | Hidden size / layer count changed     | Full retrain or dimension adaptation |
| `VERSION_OUT_OF_RANGE`   | Target version below ratchet floor    | Advance ratchet matrix, then retrain |

### 5.2 Adapter Invalidation

Before triggering a rebuild, invalidate the existing adapter to prevent serving
an incompatible artifact:

```cpp
// Mark package as DISABLED so it cannot be promoted
orchestrator->setPackageStatus(package_id, FinalLayerPackageStatus::DISABLED);
```

If the package was serving production traffic, first roll back to
`PREVIOUS_KNOWN_GOOD`:

```cpp
orchestrator->rollbackToPackage(
    "pkg-legal-qa",      // current serving package
    "pkg-legal-stable",  // previous known-good
    target_model_name,
    target_model_version);
```

### 5.3 Rebuild Execution

After invalidation, execute the retraining pipeline for the affected package.
This is external to the ThemisDB workflow layer; consult the fine-tuning
runbook at `docs/de/llm/LORA_TRAINING_FRAMEWORK_INTEGRATION.md`.

Minimum rebuild checklist:

- [ ] Training data snapshot matches the intended domain
- [ ] LoRA config (rank, alpha, target modules) updated for the new model
- [ ] Quantization format verified against the target model quantisation scheme
- [ ] Rebuilt adapter signed and registered in `AdapterRegistry`
- [ ] Provenance metadata updated (`parent_adapter_id`, `created_at`)

### 5.4 Post-Rebuild Re-Evaluation

After rebuild, re-run `workflow.executeSwitch()` with the same request.
Expect `COMPATIBLE` outcome.  If the outcome is still `REBUILD_REQUIRED` or
`BLOCKED`, escalate to the LLM platform team.

---

## 6. Ratchet Matrix Governance

### 6.1 Advancing the Floor

When a new model version is certified, update the matrix:

```cpp
// Advance the ratchet floor for 'legal-general' to require llama >= 3.1
matrix.registerEntry("legal-general", "llama",
                     SemVer::parse("3.1"),
                     SemVer{} /* unbounded upper */);
```

Persist the updated matrix to the versioned JSON file and commit it to the
configuration repository.  The schema version in the JSON should be bumped
whenever the semantics of the matrix change (not for individual entry updates).

### 6.2 Operator Override (Downgrade)

Lowering the minimum floor is an exceptional operation and requires explicit
approval.  Pass `allow_downgrade = true` only after obtaining a signed change
request:

```cpp
// OPERATOR OVERRIDE — requires change ticket #XXXX
matrix.registerEntry("legal-general", "llama",
                     SemVer::parse("3.0"),  // lower than current floor
                     SemVer{},
                     /*allow_downgrade=*/true);
```

Document the override in the change ticket and package event log.

---

## 7. Policy-Safe Migration Gates

The workflow evaluates six gate categories per adapter:

| Check | What is verified | Rebuild trigger |
|---|---|---|
| Ratchet Matrix     | Target version within registered range      | `VERSION_OUT_OF_RANGE` |
| Architecture       | Adapter architecture matches target model   | `ARCHITECTURE_CHANGE` |
| Tokenizer          | Adapter family implies same tokenizer       | `TOKENIZER_CHANGE` |
| Layer Dimensions   | Base model name identity check              | `LAYER_DIMENSION_CHANGE` |
| Quantization       | No known quantization format conflicts      | — (warning only) |
| Prompt Format      | Chat-template alignment for instruct models | — (warning only) |

All gates are evaluated in sequence.  A hard failure on any gate returns
`INCOMPATIBLE` immediately; rebuild signals are accumulated across all gates
before the final outcome is determined.

---

## 8. Rollback Procedure

If post-switch validation reveals a regression, roll back immediately:

```cpp
bool ok = orchestrator->rollbackToPackage(
    "pkg-legal-qa",      // source (broken) package
    "pkg-legal-stable",  // rollback target
    current_model_name,
    current_model_version);

if (!ok) {
    // Rollback failed — declare incident and escalate
    send_pager("LLM_ROLLBACK_FAILED");
}
```

After rollback, file a post-mortem incident with the `result.toJson()` audit
record from the failed switch attempt.

---

## 9. Key References

| Resource | Location |
|---|---|
| `ModelSwitchWorkflow` API | `include/llm/model_switch_workflow.h` |
| `FinalLayerOrchestrator` | `include/llm/final_layer_orchestrator.h` |
| `AdapterRegistry` | `include/llm/adapter_registry.h` |
| LoRA Training Framework | `docs/de/llm/LORA_TRAINING_FRAMEWORK_INTEGRATION.md` |
| ADR E1-005 | `docs/adr/adr-e1-005-model-switch-compatibility.md` |
| EPIC 1.5 model switch design | `docs/EPIC1_MODEL_SWITCH.md` |
| Phase 6 QA test suite | `tests/model/test_model_switch_workflow.cpp` |

---

*Last reviewed: 2026-07-16*  
*Owner: LLM Platform Team*  
*Status: Production Ready*
