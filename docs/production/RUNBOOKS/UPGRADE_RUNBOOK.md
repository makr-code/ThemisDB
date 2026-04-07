# Upgrade Runbook

**Version:** 1.0  
**Last Updated:** April 2026  
**Target Audience:** Operations Teams, SREs

---

## Overview

This runbook provides step-by-step procedures for upgrading ThemisDB in production with zero or minimal downtime.

### Upgrade Strategies

| Strategy | Downtime | Risk | Duration | Use Case |
|----------|----------|------|----------|----------|
| Rolling | Zero | Low | 30-60 min | Standard upgrades |
| Blue-Green | Zero | Low | 45-90 min | Instant rollback needed |
| Canary | Zero | Very Low | 2-4 hours | High-risk upgrades |

---

## Pre-Upgrade Checklist

### 24 Hours Before

- [ ] Create verified backup
- [ ] Test upgrade in staging
- [ ] Review release notes for breaking changes
- [ ] Notify stakeholders
- [ ] Verify monitoring is operational

### 1 Hour Before

- [ ] Final system health check
- [ ] Verify backup completion
- [ ] Enable enhanced monitoring
- [ ] Prepare rollback plan

---

## Rolling Upgrade Procedure (Recommended)

**Duration**: 30-60 minutes  
**Downtime**: Zero

### Step 1: Pre-Flight Checks

```bash
# Verify cluster health
themisdb-cli cluster health

# Check load balancer
themisdb-cli lb status

# Record baseline metrics
themisdb-cli metrics snapshot --output /tmp/baseline.json
```

### Step 2: Upgrade First Node

```bash
# Drain node
themisdb-cli node drain node-1 --graceful-timeout 300s --wait

# Update container image (Kubernetes)
kubectl set image deployment/themisdb themisdb=themisdb/themisdb:1.5.0

# Wait for pod ready
kubectl wait --for=condition=ready pod/themisdb-0 --timeout=300s

# Run smoke tests
themisdb-cli test smoke --node node-1

# Return to service
themisdb-cli node undrain node-1
```

### Step 3: Monitor and Repeat

```bash
# Monitor first node for 10 minutes
themisdb-cli monitor --node node-1 --duration 10m

# Repeat for remaining nodes
for NODE in node-2 node-3; do
  themisdb-cli node drain $NODE --wait
  kubectl set image deployment/themisdb-$NODE themisdb=themisdb/themisdb:1.5.0
  kubectl wait --for=condition=ready pod/themisdb-$NODE-0 --timeout=300s
  themisdb-cli test smoke --node $NODE
  themisdb-cli node undrain $NODE
  themisdb-cli monitor --node $NODE --duration 5m
done
```

### Step 4: Post-Upgrade Validation

```bash
# Verify all nodes upgraded
themisdb-cli cluster version

# Run integration tests
themisdb-cli test integration --suite post-upgrade

# Compare performance
themisdb-cli metrics compare --before /tmp/baseline.json --after now
```

---

## Rollback Procedure

### Immediate Rollback (< 5 minutes)

```bash
# Switch traffic to old version
kubectl patch service themisdb --patch '{"spec":{"selector":{"version":"stable"}}}'

# Scale down new version
kubectl scale deployment themisdb-new --replicas=0

# Verify rollback
themisdb-cli cluster health
```

---

## Troubleshooting

### Node Fails to Start

```bash
# Check logs
kubectl logs themisdb-0 --previous

# Run migration if needed
kubectl exec themisdb-0 -- themisdb migrate --from v1.4 --to v1.5

# Restart pod
kubectl delete pod themisdb-0
```

### High Latency After Upgrade

```bash
# Check for index rebuild
themisdb-cli index status

# Warm up cache
themisdb-cli cache warmup --size 80%

# Trigger compaction if needed
themisdb-cli storage compact
```

---

## Success Criteria

- [ ] All nodes running target version
- [ ] Cluster health: HEALTHY
- [ ] All tests passing
- [ ] Performance within baseline ±5%
- [ ] No critical errors

---

**Next Steps**: Update documentation, notify stakeholders of successful upgrade
