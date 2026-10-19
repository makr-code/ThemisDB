---
Author: ThemisDB Contributors
Created: 2026-10-19
Last Updated: 2026-10-19
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: unknown
Human_Review: true
Human_Author: ThemisDB Contributors
---

# Security Trust and Risk Model

AI provenance note: Exact model backend not verifiable from final document text alone.


This document defines the trust boundaries and risks that matter for the security module. It is grounded in the contract in `include/security/security_api_contract.h`, the enforcement logic in `src/security`, and the active module roadmap.

## Trust boundaries

The module is designed around the assumption that multiple trust boundaries exist at the same time:

- client-to-server trust boundary
- principal-to-role trust boundary
- application-to-key-provider trust boundary
- query-to-execution trust boundary
- evidence-to-operator trust boundary

A request or dataset is not considered trusted just because it has entered the process. Every step that modifies behavior or accesses sensitive data must be re-evaluated against policy, identity, or key state.

## Risk model

The primary risks are:

1. unauthorized access due to missing or conflicting policy state
2. unsafe key material, rotation gaps, or provider failure
3. injection or malicious query payloads
4. unsafe mutation by AI-driven tools
5. incomplete or tamper-prone audit evidence

The module addresses these risks using explicit denial, bounded policy evaluation, versioned key semantics, query validation, and structured evidence capture.

## Controls and invariants

The effective controls currently implemented include:

- deny-by-default authorization and zero-trust verification
- TLS and certificate validation requirements enforced by the contract
- key lifecycle management with explicit status transitions
- encryption bound to versioned metadata and key identifiers
- query suspicion and literal validation before execution
- AI destructive-operation classification and approval windows
- structured security evidence bundles and retention-aware export logic

## Trust assumptions that should not be made

The following assumptions are not safe in the module's current design:

- a missing or stale policy is equivalent to a valid allow
- a key provider fallback is a normal production-safe state
- a query is safe because it passed a loose regex check
- audit logs can be treated as complete without integrity validation
- AI-generated mutation requests can bypass approval or classification

These are explicitly contrary to the contract and the implementation style.

## Incident handling

When trust is broken or a control fails:

- stop or block the offending operation
- preserve evidence and logs
- identify which boundary failed: transport, identity, policy, key, or evidence
- repair the root cause before restoring flow
- revalidate that the deny-by-default posture remains intact

## Current posture

The security module is production-evidenced and actively hardened, but the roadmap and gap tracker still record that full production closure remains pending. That is a trust posture requirement, not a deficiency of the documentation. The module should therefore be treated as strong but not universally closed for all future threat scenarios.
