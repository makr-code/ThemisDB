# Deployment Documentation

**Version:** 1.8.1  
**Last Updated:** May 2026  
**Target Audience:** DevOps Engineers, System Administrators, Operations Teams

---

## Overview

This directory contains deployment-related documentation for ThemisDB. The primary production deployment guide lives in `docs/production/` and is linked from here for easy navigation.

---

## Quick Navigation

### Installation & Setup

| Document | Purpose |
|----------|---------|
| [Production Deployment Guide](../production/DEPLOYMENT.md) | Step-by-step installation, hardware requirements, GPU setup, multi-node configuration |
| [Port Mapping Reference](PORT_MAPPING.md) | Network port assignments for all ThemisDB services |
| [Load Balancer Integration](../production/LOAD_BALANCER_INTEGRATION.md) | NGINX, AWS ALB, GCP LB, Istio, HAProxy configuration |

### Pre- and Post-Deployment Checklists

| Checklist | When to Use |
|-----------|------------|
| [Pre-Deployment Checklist](../production/CHECKLISTS/pre_deployment.md) | Before starting any deployment |
| [Post-Deployment Checklist](../production/CHECKLISTS/post_deployment.md) | After deployment to validate correctness |
| [Operational Compliance Checklist](../production/CHECKLISTS/operational_compliance.md) | Monthly compliance verification |

### Upgrade & Migration

| Document | Purpose |
|----------|---------|
| [Upgrade Runbook](../production/RUNBOOKS/UPGRADE_RUNBOOK.md) | Zero-downtime rolling, blue-green, and canary upgrades |
| [Migration Guide](../MIGRATION_GUIDE.md) | Cross-version migration procedures |

### Configuration Examples

Located in [`../production/examples/`](../production/examples/):

| File | Scenario |
|------|---------|
| `single_gpu_setup.yaml` | Development / single-GPU inference |
| `multi_gpu_setup.yaml` | Production multi-GPU training |
| `distributed_training.yaml` | Multi-node distributed setup |
| `raid_configuration.yaml` | High-availability storage |
| `k8s_production_values.yaml` | Kubernetes Helm production values |

---

## Deployment Scenarios

### Single-Node (Development / Small-Scale)

```bash
# Docker single-node startup
docker run -d --name themisdb \
  -p 8765:8765 -p 8080:8080 \
  -v $(pwd)/config:/etc/themis/config:ro \
  -v $(pwd)/data:/var/lib/themis/data \
  themisdb:community:v1.8.1 \
  /opt/themis/bin/themis_server --config=/etc/themis/config/config.yaml

# Verify health
themisctl --port 8765 health
```

→ Full guide: [production/DEPLOYMENT.md#single-gpu-setup](../production/DEPLOYMENT.md#single-gpu-setup)

### Multi-Node Cluster (Kubernetes)

```bash
helm install themisdb ./helm/themisdb \
  --namespace production \
  --set autoscaling.enabled=true \
  --set autoscaling.minReplicas=3 \
  --set autoscaling.maxReplicas=10 \
  --values docs/production/examples/k8s_production_values.yaml
```

→ Full guide: [production/DEPLOYMENT.md#multi-node-distributed-setup](../production/DEPLOYMENT.md#multi-node-distributed-setup)

---

## Standard Deployment Workflow

1. **Prepare**: Complete [Pre-Deployment Checklist](../production/CHECKLISTS/pre_deployment.md)
2. **Deploy**: Follow [Deployment Guide](../production/DEPLOYMENT.md)
3. **Configure**: Apply security hardening ([Security Guide](../production/SECURITY.md))
4. **Monitor**: Set up observability ([Monitoring Guide](../production/MONITORING.md))
5. **Validate**: Complete [Post-Deployment Checklist](../production/CHECKLISTS/post_deployment.md)

---

## Related Documentation

- **Operations Hub**: [../OPERATIONS.md](../OPERATIONS.md)
- **Operations Handbook**: [../operations/OPERATIONS_HANDBOOK.md](../operations/OPERATIONS_HANDBOOK.md)
- **Disaster Recovery**: [../production/DISASTER_RECOVERY_PLAN.md](../production/DISASTER_RECOVERY_PLAN.md)
- **Backup & Recovery**: [../backup_recovery_system.md](../backup_recovery_system.md)
- **Security Hardening**: [../production/SECURITY.md](../production/SECURITY.md)
- **CI/CD Pipelines**: [../ci-cd/README.md](../ci-cd/README.md)

---

**Document Classification:** Internal – Operations  
**Review Cycle:** Quarterly  
**Maintained by:** Operations Team
