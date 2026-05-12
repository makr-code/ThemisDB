# FUTURE_ENHANCEMENTS

## core-engine

### Scope

- Add advanced policy engine and safe extension points
- Improve scheduling and self-healing behavior

### Design Constraints

- Must remain deterministic and rollback-safe
- Must not permit bypass of signature/hash verification

### Required Interfaces

- policy::IEvaluationEngine
- workflow::IRecoveryStrategy
- state::IMigrationHandler

### Implementation Notes

- Keep policy evaluation side-effect free
- Use explicit versioned policy documents

### Test Strategy

- Property-based tests for policy evaluation
- Fault-injection tests for recovery strategies

### Performance Targets

- Policy evaluation < 5 ms for standard config size

### Security / Reliability

- Reject ambiguous policy conflicts by default
- Emit signed audit events for policy decisions

## source-providers

### Scope

- Add S3, OCI registry, and enterprise mirror providers

### Design Constraints

- Shared provider contract, provider-specific auth adapters
- No provider may skip integrity/authenticity checks

### Required Interfaces

- source::IReleaseProvider
- source::IAuthProvider

### Implementation Notes

- Keep fetch/parsing logic separated from auth handling
- Support offline mirror fallback chain

### Test Strategy

- Integration tests with mocked remote endpoints
- Contract tests per provider backend

### Performance Targets

- Metadata fetch median < 300 ms on local mirror

### Security / Reliability

- TLS validation mandatory
- Retry with bounded exponential backoff

## packaging-and-delta

### Scope

- Add optional differential update support

### Design Constraints

- Full update remains fallback path
- Delta packages must be verifiable and reversible

### Required Interfaces

- artifact::IDeltaApplier
- artifact::IPackageVerifier

### Implementation Notes

- Delta metadata must include base-version constraints
- Apply delta only after base artifact hash verification

### Test Strategy

- Delta parity tests vs full artifact installation
- Corrupted delta rejection tests

### Performance Targets

- Reduce update download size by >= 40% for minor releases

### Security / Reliability

- Validate both delta metadata and resulting artifact hash

## ui-and-observability

### Scope

- Add optional GUI shell and richer observability hooks

### Design Constraints

- GUI optional, CLI remains first-class
- Observability must not leak secrets

### Required Interfaces

- ui::IProgressSink
- telemetry::IEventSink

### Implementation Notes

- Keep core independent from specific UI toolkit
- Expose structured event stream

### Test Strategy

- Snapshot tests for CLI JSON output
- Integration tests for cancel/resume behavior

### Performance Targets

- Progress update latency < 100 ms in normal operation

### Security / Reliability

- PII redaction in all emitted events
