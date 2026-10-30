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

Exact model backend not verifiable from final document text alone.

# AQL Assistance Operations

## Purpose and operating boundary

This page describes operational checks for the AQL assistance components in
`src/aql/`. Translation, explanation, and semantic example ranking depend on
configured LLM/parser/embedding integrations. Their availability is separate
from direct AQL execution; operators should monitor each dependency and path
independently.

## Deployment readiness

Before enabling an assistance route, confirm that:

- The intended LLM client and parser service are wired into the
  `LLMAQLHandler`. If no parser service is configured, translation uses a
  string-level `AQLQueryValidator` fallback; do not treat it as equivalent to
  parser-backed validation.
- The validation policy, retry count, timeout, output limits, and conversation
  context bounds are explicitly reviewed for the deployment.
- Access checks and query-safety controls remain enforced at the execution
  boundary. Generated AQL and schema context are not authorization tokens.
- Optional embedding provider and bridge lifetimes are valid; missing
  embeddings must not be mistaken for semantic matches.
- Logs and metrics avoid retaining sensitive prompt content and have
  deployment-appropriate access and retention controls.

Relevant configuration surfaces are declared in
[`include/aql/llm_aql_handler.h`](../../include/aql/llm_aql_handler.h) and
[`include/aql/aql_conversation_context.h`](../../include/aql/aql_conversation_context.h).
Consult the deployed version's configuration and service documentation before
changing runtime values; this module page does not define a universal admin
CLI or production configuration.

## Health and observability

Monitor:

- Translation success/failure, retry counts, confidence diagnostics, and
  latency percentiles.
- Parser validation failures and their categories.
- Provider readiness and circuit-breaker states exposed by the handler.
- `[BRIDGE:ExecutionFailed]` events and the expected lexical fallback when
  embeddings are unavailable.
- Context token/turn bounds and eviction frequency.

Use the application's configured metrics exporter and dashboards. Metric
names and availability vary by integration; do not assume a metric exists
solely because it appears in an older roadmap or recovery matrix. Logs should
identify operation and failure category without exposing raw prompts,
credentials, or sensitive query values.

## Routine change and release checks

1. Validate configuration and provider connectivity before routing production
   traffic.
2. Run focused AQL test targets for translation, validation, context,
   provider degradation, and embedding fallback.
3. Compare p95/p99 and failure rates with the versioned baselines in
   [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md), using the
   documented hardware and benchmark path.
4. Roll out through the service's normal staged deployment and monitor the
   metrics above; stop promotion if validation failures, retries, or
   latency regress beyond the approved budget.
5. Roll back the last deployment/configuration change using the established
   release procedure. Preserve validation and authorization controls during
   rollback.

## Incident handling

For provider outage, repeated parser rejection, breaker transitions, context
pressure, or embedding failures, follow [`RUNBOOK.md`](RUNBOOK.md) and
[`ERROR_RECOVERY_MATRIX.md`](ERROR_RECOVERY_MATRIX.md). The older
[`docs/operability/RUNBOOK_AQL_ASSISTANCE.md`](../../docs/operability/RUNBOOK_AQL_ASSISTANCE.md)
has recommendations that have not been reconciled with current module
behavior; do not treat its runtime configuration commands or `WARN_ONLY`
recommendation as production guidance. Escalate suspected exposure or
authorization bypass through the project security process.
