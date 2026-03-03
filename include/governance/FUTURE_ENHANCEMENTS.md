# Governance Module - Future Header Enhancements

## Scope

- `IGovernancePolicy` interface extensions for policy hot-reload and inheritance
- Data masking hook API exposed via `IDataMaskingHook` header
- OPA policy evaluation interface (compile-time optional via feature flag)
- AI/ML model governance API for model lifecycle and usage controls
- Data subject rights request interface for CCPA/CPRA/GDPR compliance
- Cross-tenant policy inheritance API for hierarchical policy composition

## Design Constraints

- [ ] Policy evaluation is synchronous and `noexcept`; implementations must not throw or block on I/O
- [ ] Masking hooks are stateless; key material and configuration are injected at construction, not during `mask()` calls
- [ ] OPA integration is optional via `THEMIS_ENABLE_OPA`; absent symbols produce a clear `static_assert` at link time
- [ ] Data subject rights requests are always audit-logged; the audit sink is injected via `IGovernancePolicy::setAuditSink()`
- [ ] Cross-tenant policy inheritance is expressed as a DAG; cycles detected at `build()` time and returned as `PolicyError::CYCLE_DETECTED`
- [ ] AI/ML model governance hooks are called synchronously before inference dispatch; rejection is surfaced as `ModelGovernanceError`

## Required Interfaces

| Interface | Consumer | Notes |
|---|---|---|
| `IGovernancePolicy` | Query engine, REST API gateway, data pipeline | Base policy interface; all policy types derive from this |
| `IDataMaskingHook` | Query result serializer, export pipeline, audit trail | Stateless; receives field name + value, returns masked value |
| `IOPAPolicyEvaluator` | Policy engine, access control middleware | Compile-time optional; guarded by `THEMIS_ENABLE_OPA` |
| `IDataSubjectRightsHandler` | CCPA/CPRA/GDPR compliance endpoint, admin API | Handles `ACCESS`, `DELETE`, `PORTABILITY`, `OPT_OUT` requests |
| `IModelGovernancePolicy` | LLM inference dispatch, ML pipeline, audit trail | Called pre-inference; can block or annotate the request |
| `ICrossTenantPolicyNode` | Multi-tenant policy planner, tenant admin API | Node in a policy inheritance DAG; resolved top-down |

## Planned Features

### Policy Hot-Reload Interface

- [ ] Define `IGovernancePolicy::reload(PolicyBundle)` returning `PolicyReloadResult`
- [ ] Hot-reload must be atomic: either the new policy is fully active or the old one remains
- [ ] `PolicyBundle` carries a version hash; reload rejected if hash matches the currently active bundle
- [ ] Reload notification delivered to registered `IPolicyReloadObserver` instances after successful swap

### Cross-Tenant Policy Inheritance API

- [ ] Define `ICrossTenantPolicyNode` with `parentPolicies()`, `ownRules()`, and `resolvedPolicy()` methods
- [ ] Policy resolution traverses the DAG from root to leaf; leaf rules override parent rules
- [ ] Cycle detection runs at `PolicyDAG::build()` time; returns `PolicyError::CYCLE_DETECTED` with the offending node path
- [ ] `resolvedPolicy()` result is cached and invalidated on any ancestor hot-reload

### Cross-Border Data Transfer Control Interface

- [ ] Define `IDataTransferPolicy` with `isTransferPermitted(srcRegion, dstRegion, DataCategory)` returning `TransferDecision`
- [ ] Transfer decisions are logged to the audit sink regardless of outcome
- [ ] Policy supports `ALLOW`, `DENY`, and `CONDITIONAL` decisions; `CONDITIONAL` carries a list of required safeguards
- [ ] Region codes follow ISO 3166-1 alpha-2; unknown codes return `TransferDecision::DENY` by default

### AI/ML Model Governance Hook

- [ ] Define `IModelGovernancePolicy` with `evaluate(ModelRequest) -> ModelGovernanceDecision`
- [ ] Decision types: `ALLOW`, `BLOCK`, `ALLOW_WITH_ANNOTATION`
- [ ] Hook receives model ID, caller identity, and input token count; must not receive raw prompt text
- [ ] `IModelGovernancePolicy` composed with `IGovernancePolicy` via `GovernancePolicyComposer`

### Data Subject Rights Request API

- [ ] Define `IDataSubjectRightsHandler` with `handleRequest(DataSubjectRequest) -> DataSubjectRightsResult`
- [ ] Request types: `ACCESS`, `DELETE`, `PORTABILITY`, `OPT_OUT`, `CORRECT`
- [ ] Every call to `handleRequest()` is synchronously logged to the configured audit sink before processing begins
- [ ] `DataSubjectRightsResult` includes a `requestId` for downstream tracking and a `completionStatus`

## Test Strategy

- Unit-test `IGovernancePolicy::reload()` atomicity: verify that concurrent reads during a reload always see either the old or the new policy, never a partial state
- Test `ICrossTenantPolicyNode` cycle detection with DAGs of depth 1, 10, and a deliberate cycle; assert correct error codes
- Integration-test `IDataSubjectRightsHandler` for all five request types against a mock audit sink; verify every call produces exactly one audit record
- Test `IDataMaskingHook` with empty, single-character, and maximum-length field values; verify no plaintext leakage in return value
- Compile-flag test: build with and without `THEMIS_ENABLE_OPA`; assert no linker errors and correct `static_assert` message
- Fuzz-test `IDataTransferPolicy` with random region codes and data categories; verify `DENY` is returned for all unknown inputs

## Performance Targets

- Policy evaluation (`IGovernancePolicy::evaluate()`) ≤ 1 ms p99 under 10,000 concurrent evaluations
- Data masking hook (`IDataMaskingHook::mask()`) ≤ 500 µs per field for fields up to 64 KB
- OPA policy query (`IOPAPolicyEvaluator::query()`) ≤ 5 ms p99 for bundles with up to 10,000 rules
- Data subject rights request handling (`IDataSubjectRightsHandler::handleRequest()`) ≤ 100 ms p95 including audit log write
- Cross-tenant policy resolution (`ICrossTenantPolicyNode::resolvedPolicy()`) ≤ 2 ms for DAGs with depth ≤ 10
- Policy hot-reload swap (`IGovernancePolicy::reload()`) completes in ≤ 50 ms; zero dropped evaluations during swap

## Security / Reliability

- Policy engine operates in read-only mode during evaluation; no policy decision can mutate collection data
- Masking hooks cannot read the original unmasked value after returning; the interface contract prohibits storing references to input spans beyond the call
- All governance decisions (allow and deny) are audit-logged with caller identity, timestamp, and policy version hash
- OPA bundle integrity is verified via signature before loading; bundles with invalid signatures are rejected with `PolicyError::BUNDLE_SIGNATURE_INVALID`
- Cross-border transfer control defaults to `DENY` for unknown or missing region metadata; fail-closed behavior is not overridable at the interface level
- Data subject rights requests are idempotent by `requestId`; duplicate submissions return the original result without re-executing the operation
