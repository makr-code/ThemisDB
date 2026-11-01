---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Governance Domain Models

## Purpose and scope

This document describes the runtime data and decision models represented by `include/governance/` and implemented in `src/governance/`. It is intended for developers changing policy evaluation, compliance reporting, tenant inheritance, or governance evidence. These models are software contracts, not a formal proof of regulatory compliance.

## Policy and decision model

`PolicyRule` carries a rule identifier, resource/action patterns, roles, effects (encryption, export, cache, retention, redaction, audit), priority, version, and lifecycle metadata. `PolicyManager` tracks lifecycle as `DRAFT`, `ACTIVE`, `DEPRECATED`, or `RETIRED`; review workflow stages are separate and must not be conflated with that lifecycle enum.

`PolicyEngine` maps request headers and route context to a `PolicyDecision`. Its result contains classification, enforcement/observe mode, logging and content-encryption requirements, redaction, ANN/export/cache permissions, retention, and CCPA opt-out state. A missing explicit classification uses a route mapping when present; otherwise the native path uses `streng-geheim`. An unknown label also resolves to the strictest result. Configured profiles take precedence over native heuristics for known classifications.

An attached OPA evaluator is consulted first. A concrete OPA decision replaces
the native decision, after which the CCPA opt-out export restriction is applied.
An unavailable REST evaluator can result in native evaluation. Explicit WASM
mode uses only its injected evaluator and returns an enforced strict deny if
unavailable; it does not fall through. See [Trust](TRUST.md).

## Compliance and evidence model

Framework-specific rule/control sets evaluate configured requirements and produce structured results. `ComplianceReporter` derives coverage, gaps, rule-evaluation summaries, and (where supplied) bias statistics from policy state and caller-provided evidence. Scores and gap counts therefore depend on the input inventory and policy configuration; they do not represent external certification.

## Tenant composition model

`CrossTenantPolicyInheritance` represents a parent-linked tenant hierarchy and tenant-specific `PolicyManager` instances. Registration must not introduce a cycle. Effective decisions combine ancestor and tenant policy state using restrictive semantics for allow/export/cache decisions; a child cannot turn a denied inherited decision into an allow. A tenant identifier or classification is context, not proof of caller authentication.

## Lineage model

Each `LineageEvent` has a dataset ID, event type, timestamp, actor, optional schemas/metadata, and optional `parent_event_id`. Events are retained chronologically per dataset and indexed for upstream/downstream traversal. Recording is append-oriented, but configured per-dataset/global bounds evict oldest records. Audit forwarding is separate from local recording: an open audit circuit can return an error while the event remains locally recorded.

## Assumptions and trade-offs

- Decision correctness depends on authenticated upstream context, correct route/resource mapping, loaded policy, and callers enforcing returned restrictions.
- Native classification fallback favors restriction over availability for missing or unknown labels.
- OPA allows central policy evaluation but adds network, response-shape, and timeout failure modes.
- Lineage and audit improve traceability but retain sensitive operational identifiers and are subject to configured bounds.
- Policy and report scores are explainability aids; operators must inspect the contributing rules and evidence.

## Source references

- [`policy_manager.h`](../../include/governance/policy_manager.h), [`policy_engine.h`](../../include/governance/policy_engine.h)
- [`cross_tenant_policy_inheritance.h`](../../include/governance/cross_tenant_policy_inheritance.h)
- [`compliance_reporter.h`](../../include/governance/compliance_reporter.h)
- [`data_lineage.h`](../../include/governance/data_lineage.h)
- [Architecture](ARCHITECTURE.md), [API](API.md), and [trust model](TRUST.md)
