---
Author: "ThemisDB Contributors"
Created: "2026-10-21"
Last Updated: "2026-10-21"
Status: "active"
AI_Used: true
AI_Models: "GitHub Copilot"
AI_Contributor: "GitHub Copilot"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Retrieval Trust and Security Boundary

## Purpose

This document captures the current trust boundary of the retrieval module as implemented in source. The module's trust responsibilities are narrow but important: it governs adapter integrity, lifecycle state, and retrieval regression evaluation.

## Trust boundaries

### 1. Artifact boundary

The principal trust boundary is the `LoRAPackage` / `PortableAdapterProduct` lifecycle. These objects are not treated as neutral data; they encode policy, provenance, and integrity. The implementation treats them as trust-bearing assets.

The boundary breaks down as follows:

- package metadata is inserted into a manifest store,
- integrity is checked via hashing and verification helpers,
- deployment status is controlled by lifecycle state,
- a package is not considered safe to use without a verified state and policy context.

### 2. Retrieval evaluation boundary

The second boundary is the metric comparison path in `DualReadValidator`. Here the critical trust question is simple: is the current retrieval state meaningfully better or worse than the prior known-good state?

The implementation avoids ungrounded claims by requiring explicit ground truth and metric computation. It does not treat a new retrieval score as authoritative in the absence of evaluation data.

## Risk model

The main risks are:

- tampered artifact metadata,
- invalid or missing hash chains,
- unauthorized adapter deployment,
- retrieval regression hidden by silhouette metrics or stale ground truth,
- silent trust drift between package status and runtime deployment state.

These are precisely the risks that the current design tries to surface with explicit state transitions and verification logic.

## Controls in source

The current source-level controls include:

- deterministic canonical JSON serialization,
- SHA-256 integrity checking,
- explicit status enums for package and product lifecycle states,
- mutex-protected manifest storage,
- ground-truth-aware metric evaluation,
- fail-closed behavior when no valid evaluation signal exists.

## Incident handling

If a package integrity or retrieval regression incident occurs:

1. quarantine the impacted package or index,
2. compare with the last known-good artifact or retrieval baseline,
3. preserve evidence from the manifest store and evaluation logs,
4. prevent further deployment until the cause is understood,
5. document the remediation and rollback action in the runbook.

## Trust posture

The module is intentionally conservative. It is designed to prefer explicit failure and clear evidence over optimistic permissiveness. That trust posture is appropriate for an early-stage retrieval foundation used to guard model adaptation and evaluation quality.

## Conclusion

The retrieval module's trust model is operationally simple: packages and retrieval quality are treated as evidence-bearing artifacts that must be verified and measured before they are trusted. That is the correct boundary until broader retrieval rollout and full multi-shard deployment are ready.
