# RUNBOOK: Themis Core — Triage & Recovery

**Author:** ThemisDB Contributors
**Created:** 2026-09-16
**Last Updated:** 2026-09-16
**Status:** active

**Audience:** Database Operators, SREs, Core Platform Team Lead
**Purpose:** Triage and recover from themis core load failures, wire-session stalls, trust violations, and gating timeouts
**Severity:** Critical (themis core failures can halt module loading, gating, and wire-server operation across all modules)
**Estimated Duration:** 5 min – 4 hours (depending on failure class)

---

## Overview

This runbook guides operators through diagnosing and recovering from failures in the ThemisDB themis core module (`src/themis/`). The module has five primary surfaces:

1. **Build identity / edition gating** — edition license, feature-flag evaluation (`ThemisEdition`, `ThemisBuildIdentity`)
2. **Module loader / verifier** — shared-library load, signature verification, trust level assignment
3. **Dependency resolver** — topological sort, circular-dependency detection, version constraint checks
4. **Wire server** — request/response protocol, session lifecycle, heartbeat
5. **Hot-plug / PluginManager integration** — dynamic enable/disable of module adapters

**Key Principles:**
- Module loader is fail-closed; verification failure blocks the module load, not the host process
- Wire sessions have an explicit heartbeat timeout; stalled sessions are detached, not silently leaked
- Dependency gating uses a bounded registry; exceeding the limit returns an explicit error
- Trust levels are clamped to [0, 4]; level 0 means untrusted and must not be used in production paths

---

## Prerequisites Checklist

Before beginning any intervention, verify:

- [ ] Access to server logs with `[THEMIS:*]` tag filtering
- [ ] Prometheus/Grafana dashboard showing `themis_*` metrics
- [ ] Knowledge of module trust level configuration and edition license state
- [ ] Wire server endpoint and heartbeat timeout values

---

## Failure Scenarios

---

### Scenario 1: Module Load Failure

**Symptoms:**
- Log pattern: `[THEMIS:LoadFailed] module=<name> reason=<reason>`
- Plugin or module returns unavailable; dependent features disabled
- `themis_module_load_failures_total` counter rising

**Log patterns:**
```
[THEMIS:LoadFailed] module=whisper reason=signature_verification_failed
[THEMIS:LoadFailed] module=rpc_grpc reason=missing_symbol:THEMIS_GRPC_PLUGIN
[THEMIS:LoadFailed] module=temporal reason=dependency_unresolved:storage
```

#### Step 1: Identify Load Failure
```bash
grep '\[THEMIS:LoadFailed\]' /var/log/themisdb/themisdb.log | tail -20
```

#### Step 2: Resolve by Reason

**signature_verification_failed:** Re-sign the plugin binary:
```bash
themis-admin module re-sign --module <name> --key /etc/themisdb/signing.key
themis-admin module reload --module <name>
```

**missing_symbol:** Rebuild the plugin with the correct export macro:
```bash
# Check plugin manifest
cat plugins/<name>/plugin.json
themis-admin module reload --module <name>
```

**dependency_unresolved:** Ensure dependency module is loaded first:
```bash
themis-admin module status --all
themis-admin module load --module <dependency>
themis-admin module reload --module <name>
```

---

### Scenario 2: Wire Session Stall

**Symptoms:**
- Log pattern: `[THEMIS:WireSessionStall] session_id=<id> elapsed_ms=<ms>`
- Wire server clients waiting indefinitely; heartbeat timeout exceeded
- `themis_wire_session_stalls_total` counter rising

**Log patterns:**
```
[THEMIS:WireSessionStall] session_id=<id> elapsed_ms=30500 heartbeat_timeout_ms=30000
[THEMIS:WireSessionStall] session_id=<id> reason=client_gone waiters=<N>
```

#### Step 1: Identify Stalled Sessions
```bash
grep '\[THEMIS:WireSessionStall\]' /var/log/themisdb/themisdb.log | tail -20
themis-admin wire session-list --state stalled
```

#### Step 2: Detach Stalled Sessions
```bash
themis-admin wire detach-session --session-id <session_id>
# Or detach all stalled sessions
themis-admin wire detach-all-stalled --heartbeat-timeout 30s
```

#### Step 3: Reduce Heartbeat Timeout
```bash
themis-admin config set themis.wire_server.heartbeat_timeout_ms 15000
themis-admin wire reload-config
```

---

### Scenario 3: Trust Violation

**Symptoms:**
- Log pattern: `[THEMIS:TrustViolation] module=<name> claimed_level=<N> reason=<reason>`
- Module attempting to access a capability above its trust level
- `themis_trust_violations_total` counter non-zero

**Log patterns:**
```
[THEMIS:TrustViolation] module=<name> claimed_level=5 max_level=4
[THEMIS:TrustViolation] module=<name> claimed_level=0 reason=untrusted_module_access_attempt
```

#### Step 1: Identify Violating Module
```bash
grep '\[THEMIS:TrustViolation\]' /var/log/themisdb/themisdb.log | tail -20
```

#### Step 2: Quarantine Module
```bash
themis-admin module quarantine --module <name>
# Prevent further load until investigated
```

#### Step 3: Review and Re-certify
If violation is from a known-good module with misconfigured trust level:
```bash
# Update trust level in module manifest
themis-admin module set-trust-level --module <name> --level 2
themis-admin module reload --module <name>
```

---

### Scenario 4: Gating Timeout

**Symptoms:**
- Log pattern: `[THEMIS:GatingTimeout] dependency=<name> waited_ms=<ms>`
- Module initialization blocked waiting for dependency gate to open
- `themis_gating_timeout_total` counter rising

**Log patterns:**
```
[THEMIS:GatingTimeout] dependency=storage waited_ms=10200 timeout_ms=10000
[THEMIS:GatingTimeout] dependency=rpc_grpc waited_ms=15000 reason=circular_wait
```

#### Step 1: Identify Gate Timeout
```bash
grep '\[THEMIS:GatingTimeout\]' /var/log/themisdb/themisdb.log | tail -20
```

#### Step 2: Check Dependency Load Order
```bash
themis-admin dependency graph --format dot | dot -Tpng > /tmp/dep_graph.png
# Look for circular dependencies
themis-admin dependency check --circular
```

#### Step 3: Override Gate for Emergency Recovery
```bash
# Force-open gate for named dependency (use with caution)
themis-admin dependency force-open --dependency <name>
# After recovery, reload full dependency resolution
themis-admin module reload-all
```

---

### Scenario 5: Edition Gating Failure

**Symptoms:**
- Log pattern: `[THEMIS:EditionGateFailed] feature=<name> required_edition=<E> current=<C>`
- Premium features returning "not available" even when licensed
- `themis_edition_gate_failures_total` counter rising

**Log patterns:**
```
[THEMIS:EditionGateFailed] feature=distributed_tracing required_edition=ENTERPRISE current=COMMUNITY
[THEMIS:EditionGateFailed] feature=vector_search required_edition=STANDARD current=COMMUNITY
```

#### Step 1: Verify License State
```bash
themis-admin license status
grep '\[THEMIS:EditionGateFailed\]' /var/log/themisdb/themisdb.log | tail -10
```

#### Step 2: Reload License
```bash
themis-admin license reload --license-path /etc/themisdb/license.key
```

#### Step 3: Downgrade Feature Requirements (if no valid license)
If a valid license is not available, configure the system to use community-tier features only:
```bash
themis-admin config set themis.edition COMMUNITY
themis-admin reload-all
```

---

## Alert → Runbook Mapping

| Alert Name | Log Pattern | Runbook Scenario |
|------------|-------------|------------------|
| `themis_module_load_failed` | `[THEMIS:LoadFailed]` | Scenario 1 |
| `themis_wire_session_stall` | `[THEMIS:WireSessionStall]` | Scenario 2 |
| `themis_trust_violation` | `[THEMIS:TrustViolation]` | Scenario 3 |
| `themis_gating_timeout` | `[THEMIS:GatingTimeout]` | Scenario 4 |
| `themis_edition_gate_failed` | `[THEMIS:EditionGateFailed]` | Scenario 5 |

---

## Escalation Path

1. **L1 (Operator):** Apply runbook steps; resolve within 30 min
2. **L2 (SRE):** Escalate if module load failure or trust violation persists after 3 attempts
3. **L3 (Core Platform / Security):** Engage for trust violation or signature verification failures that suggest tampering

---

## Related Documentation

- `src/themis/ROADMAP.md` — Wave D operability items
- `tests/integration/test_themis_core_soak.cpp` — Wave D soak tests
- `tests/themis/test_themis_highcardinality_stress.cpp` — Wave D stress tests
- `benchmarks/themis/bench_themis_dedicated_gates.cpp` — Dedicated performance gates
- `benchmarks/themis/bench_themis_release_gates.cpp` — Release gate benchmarks
