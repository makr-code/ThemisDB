> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Governance Module

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../../SECURITY.md).

## Security Scope

The Governance module is the compliance and data protection enforcement layer for ThemisDB. It evaluates GDPR, HIPAA, CCPA/CPRA, PCI-DSS, and SOC 2 policies at query time; enforces data masking; manages data retention; tracks data lineage; and integrates with OPA for policy-as-code. It is a critical security boundary for regulated workloads.

## Threat Model

| Threat | Mitigation |
|--------|------------|
| Unauthorized access to PII/PHI fields | Policy engine enforces attribute-based access control at query time; denied queries return no data |
| PII/PHI leakage in query results | `DataMasker` automatically masks configured sensitive fields before returning query results |
| GDPR right-to-erasure non-compliance | Policy-driven data retention and deletion; CCPA/GDPR data subject rights enforcement |
| Cross-tenant policy bypass | `CrossTenantPolicyInheritance` uses most-restrictive-wins merge; tenant policies cannot grant more access than their parent |
| Policy tampering via hot-reload | Policy file watcher validates file integrity before applying; versioned policy manager supports rollback |
| Conflicting policies granting excess access | `PolicyManager::detectConflicts()` identifies and reports overlapping rules; conflict resolution is operator-reviewed |
| OPA policy injection | OPA adapter submits structured policy queries; user input is in the query data payload, not in the Rego policy itself |
| Compliance report falsification | Compliance reporter collects evidence from append-only audit logs; evidence cannot be modified after collection |
| Policy simulation revealing sensitive data | `PolicyEngine::simulateDecision()` returns only allow/deny + reason; no data values are included in simulation output |
| Data lineage graph traversal beyond authorization | Data lineage queries are evaluated against the caller's access policy before returning node information |

## Security Controls

### Policy Enforcement
- `PolicyEngine` evaluates access control decisions at query time; enforcement is synchronous and cannot be bypassed.
- Policy decisions consider RBAC roles, ABAC attributes (data classification, tenant, time, context), and compliance constraints simultaneously.
- Denied queries return no partial data and log a governance enforcement event.

### Data Masking
- `DataMasker` operates in the query result pipeline; masking is applied before results reach the caller.
- Masking strategies: redact (replace with `[REDACTED]`), tokenize (pseudonymize), hash (one-way), or drop.
- Masking rules are defined in policy and enforced regardless of the caller's RBAC role.
- AI/ML model governance (`model_governance.cpp`) applies masking to training data lineage reports.

### Compliance Rules
- GDPR, HIPAA, CCPA/CPRA, PCI-DSS, and SOC 2 rule sets are implemented as structured rule evaluators.
- Compliance rule violations are logged to the audit trail; compliance reports are generated from audit evidence.
- Data subject rights (right-to-delete, right-to-know, opt-out-of-sale) are enforced programmatically.

### Policy Versioning and Rollback
- `PolicyManagerVersioned` maintains policy history with version numbers and timestamps.
- Rollback to a previous policy version is available to operators; requires privileged access.
- Policy hot-reload triggers only on verified file integrity check.

### OPA Integration
- OPA adapter submits structured data queries to the OPA server; Rego policies are managed by operators, not callers.
- OPA server endpoint is configured by operators; TLS connection recommended.

## Data Handling

- Governance module processes query results but does not persist them; it applies masking and returns masked results.
- Audit trail entries (principal, query, decision, timestamp) are append-only; subject to data retention policy.
- Data lineage graph nodes include dataset names and transformation records; no raw document content.
- Compliance reports aggregate audit evidence into summary statistics; no PII appears in report payloads.
- Training data lineage for AI/ML model governance may include dataset metadata; handled under data governance policies.

## Known Limitations

- OPA integration uses HTTP to communicate with OPA server; TLS must be configured by operators.
- Policy conflict detection identifies overlapping rules but automatic resolution is not implemented; operator review is required.
- Cross-tenant policy inheritance is merged at policy evaluation time; policy caching must be invalidated when hierarchy changes.

## Dependency Security

| Dependency | Purpose | Notes |
|------------|---------|-------|
| OPA Server (optional) | Policy-as-code evaluation | TLS-secured HTTP endpoint; operator-managed |
| inotify / kqueue | Policy file watcher | OS-level file change notification; no network exposure |
