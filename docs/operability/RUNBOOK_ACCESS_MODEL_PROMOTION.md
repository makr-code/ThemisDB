# RUNBOOK: Access-Model Promotion Workflow & Rollback

**Audience:** Database Operators, SREs  
**Purpose:** Safe promotion of access-control model changes with rollback capability  
**Severity:** Medium (affects authentication/authorization for all connections)  
**Estimated Duration:** 30 min - 2 hours (dry-run + canary)  

---

## Overview

This runbook guides operators through the safe promotion of changes to ThemisDB's access-control model (e.g., new policy rules, role definitions, permission mappings). The procedure includes a dry-run environment, canary rollout to a small shard/replica set, full rollout, and rollback if issues are detected.

**Key Principles:**
- Never promote directly to production without dry-run validation
- Canary on a small subset (one shard or one replica) before full rollout
- Maintain rollback capability until new model is confirmed stable (48 hours)
- Monitor authentication latency and policy evaluation errors during transition

---

## Prerequisites Checklist

Before starting promotion, verify:

- [ ] Access-control model changes code-reviewed and merged to `develop`
- [ ] Changes tested in `release_critical;access-model` CI suite (all tests pass)
- [ ] Backward compatibility verified (old clients connect after promotion)
- [ ] Dry-run environment available and current with production snapshot
- [ ] Canary target shard/replica selected and quorum established
- [ ] Rollback procedure tested in staging (simulated promotion + rollback)
- [ ] Monitoring/alerts configured for authentication latency and policy errors
- [ ] Operator on-call and available for 2+ hours post-promotion
- [ ] Incident response team notified and on standby

---

## Step-by-Step Procedure

### 1. Dry-Run in Staging Environment (15-20 min)

**Objective:** Validate that the new access model works correctly without production impact.

1. Deploy the new model build to the dry-run cluster:
   ```bash
   deploy-to-staging \
     --build-version v2.4.x-access-model-v1 \
     --cluster dry-run-us-east-1 \
     --skip-rollout
   ```

2. Verify the cluster started cleanly:
   ```bash
   kubectl get pods -n themisdb --dry-run cluster
   # All pods should be Running/Ready
   ```

3. Load a representative access-model test workload:
   ```bash
   run-benchmark --suite access-model-promotion-dry-run \
     --duration 5m \
     --client-count 100
   ```

4. Verify results:
   - [ ] All authentication requests succeeded (0% error rate)
   - [ ] Policy evaluation latency **p99 ≤ 150 µs** (GATE-W9-02 compliance)
   - [ ] No policy evaluation errors in logs
   - [ ] Role-based access control functioning for all test cases

5. **Decision Point:**
   - ✅ **PASS:** Proceed to Canary Rollout (Step 2)
   - ❌ **FAIL:** Rollback promotion to previous version, file issue, return to development

### 2. Canary Rollout (Single Shard/Replica)

**Objective:** Validate the new model on 1-2% of production traffic with instant rollback capability.

1. Select canary target:
   ```bash
   # Option A: Single shard in a multi-shard cluster
   canary-target=shard-us-east-1-001
   
   # Option B: Single replica in a quorum set
   canary-target=replica-us-west-2-002
   ```

2. Drain existing connections from canary target (graceful shutdown):
   ```bash
   drain-target --target $canary-target --wait-timeout 5m
   ```

3. Deploy new model to canary target:
   ```bash
   deploy-to-target \
     --build-version v2.4.x-access-model-v1 \
     --target $canary-target \
     --no-orchestration  # Manual, single-target deployment
   ```

4. Wait for canary target to become ready:
   ```bash
   wait-for-target --target $canary-target --ready-timeout 2m
   ```

5. Route a small percentage of traffic to canary (start at 1-5%):
   ```bash
   update-traffic-split \
     --shard $canary-target \
     --weight 0.05  # 5% of traffic
   ```

6. Monitor for 10 minutes:
   - [ ] Canary authentication latency **p99 ≤ 160 µs** (allow 10% margin)
   - [ ] Authentication error rate **≤ 0.1%** (industry acceptable)
   - [ ] Policy evaluation errors **≤ 1 per 10k requests**
   - [ ] No cascading failures (no circuit breaker trips)

7. **Decision Point:**

   **✅ PASS (2/2 metrics green):** Proceed to Full Rollout (Step 3)
   
   **⚠ WARNING (1/2 metrics amber):** Increase monitoring frequency to 1 min, wait 5 more minutes
   - If amber persists: Continue to Full Rollout with caution
   - If red: Proceed to Rollback (Step 4)
   
   **❌ FAIL (1+ metrics red):** Proceed to Rollback (Step 4) immediately

### 3. Full Rollout (All Shards/Replicas)

**Objective:** Roll out the new access model to all production instances.

1. Increase canary traffic to 100%:
   ```bash
   update-traffic-split \
     --shard $canary-target \
     --weight 1.0  # 100% traffic
   ```

2. Monitor for 5 minutes (canary now receiving full load):
   - [ ] No increase in authentication latency (p99 stable)
   - [ ] No increase in error rate
   - [ ] No new policy evaluation errors

3. If canary metrics stable, proceed to orchestrated rollout:
   ```bash
   orchestrate-rollout \
     --build-version v2.4.x-access-model-v1 \
     --strategy rolling \
     --batch-size 2 \
     --health-check-interval 30s \
     --rollback-threshold latency_p99:160ms,error_rate:0.5%
   ```

4. Monitor orchestrated rollout (typically 30-60 min for full cluster):
   - [ ] Rollout progress (watch pod counts):
     ```bash
     watch kubectl get pods -n themisdb
     ```
   - [ ] Cluster health metrics (latency, errors) — should remain stable
   - [ ] No services degradation or cascading failures

5. Once orchestrated rollout completes (all pods updated):
   ```bash
   verify-rollout-complete \
     --build-version v2.4.x-access-model-v1 \
     --cluster production
   ```

### 4. Post-Rollout Validation (30 min - 48 hours)

**Objective:** Prove that the new access model is stable under production load.

1. **Immediate validation (first 30 min):**
   ```bash
   run-benchmark --suite access-model-production-validation \
     --duration 30m \
     --client-count 500  # Realistic production-like load
   ```
   - [ ] Authentication latency p99 ≤ 150 µs (gate compliance)
   - [ ] Error rate ≤ 0.01%
   - [ ] No policy evaluation errors
   - [ ] All role-based access control checks passing

2. **Extended monitoring (48 hours):**
   ```bash
   # Query metrics for the past 48 hours
   query-metrics \
     --metric access_model_auth_latency_p99 \
     --range 48h \
     --data-point-threshold 1000  # At least 1k samples
   ```
   - [ ] **Authentication Latency Trend:** No increase over 48 hours
   - [ ] **Policy Evaluation Errors:** Trending to zero
   - [ ] **Rollback Rate:** Zero forced rollbacks due to this promotion
   - [ ] **Incident Count:** Zero incidents attributed to access-model change

3. **Decision Point:**

   - ✅ **PASS (all 48-hour metrics green):** Promotion complete, disable rollback and clean up canary
   - ⚠ **WARNING (some metrics amber):** Keep rollback capability, continue monitoring
   - ❌ **FAIL (metrics degrade):** Initiate Rollback immediately (Step 5)

---

## Rollback Procedure

### When to Rollback

Rollback is **automatic** if:
- Authentication latency p99 > 160 µs for 2+ consecutive 1-minute windows
- Authentication error rate > 1% for 2+ consecutive 1-minute windows
- Circuit breakers trip in access-control path (cascading failures)

Rollback is **manual** if operator initiates due to incident (Step 5 below).

### Step 5: Rollback to Previous Version

**Objective:** Quickly return to the previous, known-good access model.

1. **Issue the rollback command:**
   ```bash
   initiate-rollback \
     --to-version previous \
     --access-model \
     --strategy rolling \
     --batch-size 2
   ```

2. **Monitor rollback progress:**
   ```bash
   watch kubectl get pods -n themisdb
   # Pods should cycle to previous version
   ```

3. **Verify rollback completion (5-10 min):**
   ```bash
   verify-version \
     --pod-selector app=themisdb \
     --expected-version v2.4.x-access-model-v0  # Previous version
   ```

4. **Validate metrics after rollback:**
   - [ ] Authentication latency returns to previous baseline (p99 ≤ 150 µs)
   - [ ] Error rate drops to near-zero
   - [ ] Policy evaluation errors stop
   - [ ] All client applications resume normal operations

5. **Post-Incident Actions:**
   - [ ] File incident report (template below)
   - [ ] Assign root cause analysis (access-model team lead)
   - [ ] Block PR/promotion until fix verified in dry-run
   - [ ] Schedule post-mortem within 24 hours

---

## Troubleshooting & Decision Trees

### Issue: Authentication Latency Spike (p99 > 160 µs)

**Symptom:** Canary or full-rollout shows increased auth latency  
**Likely Causes:**

| Cause | Detection Method | Resolution |
|-------|------------------|-----------|
| Policy evaluation inefficiency | Query logs for slow policy decisions | Optimize policy rule ordering in access model |
| Role lookup cache miss | Check role cache hit rate (<50% = problem) | Verify cache warming completed post-deployment |
| Certificate rotation timing | Check TLS handshake latency spikes | Align promotion with off-peak certificate operations |
| Database load contention | Query resource metrics (CPU/memory spike) | Migrate other workloads away from auth shard |
| Backward compat issue | Compare latency by client version | Roll back to v0, test client upgrade path |

**Decision:** If spike persists beyond canary phase → **Rollback immediately** (Step 5)

### Issue: Policy Evaluation Errors Appearing

**Symptom:** Logs show "policy evaluation failed" or "role not found" errors  
**Likely Causes:**

| Cause | Detection Method | Resolution |
|-------|------------------|-----------|
| Missing role migration | Verify all roles in old model exist in new | Update access model to include missing roles |
| Permission mapping mismatch | Compare old vs. new permission grants | Re-map permissions; test in dry-run first |
| Backward compat break | Check if old client format still supported | Add compatibility layer for old client requests |
| Race condition in role sync | Check sync process logs for delays | Increase sync timeout or reduce batch size |

**Decision:** If errors exceed threshold (> 1 per 10k req) → **Rollback immediately**

### Issue: Client Connection Failures After Rollout

**Symptom:** Applications report "authentication failed" to ThemisDB  
**Likely Causes:**

| Cause | Detection Method | Resolution |
|-------|------------------|-----------|
| API contract change | Review migration guide for breaking changes | Coordinate client version upgrade with rollout |
| TLS certificate mismatch | Check certificate expiry and trust chain | Verify certificates installed on all nodes |
| Credential format change | Test with sample credentials from all apps | Update client SDK if format changed |
| Firewall/network change | Check network ACL logs and connectivity | Update network policies for new auth service ports |

**Decision:** If widespread (>10% of clients) → **Rollback immediately**

---

## Incident Report Template

Use this template if rollback was triggered:

```markdown
# Promotion Incident Report

## Promotion Details
- **Access Model Version:** v2.4.x-access-model-v1
- **Promotion Date/Time:** YYYY-MM-DD HH:MM:SS UTC
- **Rollback Date/Time:** YYYY-MM-DD HH:MM:SS UTC
- **Time to Rollback:** X minutes

## Incident Summary
[Brief description of what went wrong]

## Detection Method
- [ ] Automatic threshold breach (specify metric)
- [ ] Manual operator observation (describe)

## Impact Assessment
- **Clients Affected:** [% or number]
- **Duration of Impact:** [minutes]
- **Data Loss:** [yes/no]
- **Cascading Failures:** [yes/no, describe]

## Root Cause (Assigned to access-model team)
[To be filled after investigation]

## Remediation Steps
1. [Step taken to resolve]
2. [Follow-up actions]

## Prevention
[What will prevent this in future promotions]

## Sign-Off
- **Operator:** [name]
- **Access-Model Lead:** [name]
- **Date:** YYYY-MM-DD
```

---

## Evidence & Logging Checklist

After successful promotion (48-hour mark), collect evidence:

- [ ] `auth_latency_baseline.json` — Latency p50/p95/p99 pre- and post-promotion
- [ ] `policy_error_logs.json` — Zero policy evaluation errors for 48-hour window
- [ ] `rollout_progress.log` — Orchestration progress from deployment tool
- [ ] `canary_metrics.csv` — Canary phase metrics (time, latency, error rate)
- [ ] `production_metrics_48h.csv` — Production metrics over 48 hours
- [ ] `access_model_schema_v1.json` — Schema of promoted access model (for audit)

Archive in: `evidence/promotions/access-model-v1-2026-08-15/`

---

## Quick Reference: Command Cheat Sheet

```bash
# Dry-run validation
deploy-to-staging --build-version <ver> --cluster dry-run-us-east-1

# Canary deployment
deploy-to-target --build-version <ver> --target <shard-or-replica>

# Traffic split
update-traffic-split --shard <target> --weight 0.05  # 5%

# Full rollout
orchestrate-rollout --build-version <ver> --strategy rolling

# Rollback
initiate-rollback --to-version previous --access-model

# Verification
verify-rollout-complete --build-version <ver>
verify-version --pod-selector app=themisdb --expected-version <ver>

# Monitoring
query-metrics --metric access_model_auth_latency_p99 --range 48h
watch kubectl get pods -n themisdb
```

---

**Runbook Version:** 1.0  
**Last Updated:** 2026-08-15  
**Owner:** Operations Team  
**Next Review:** 2026-12-15 (post-Phase 5 UAT)
