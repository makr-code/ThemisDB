# Production Operations Documentation

This directory contains comprehensive operational documentation for running ThemisDB in production environments.

## 📋 Core Documentation

### Deployment & Configuration
- **[DEPLOYMENT.md](DEPLOYMENT.md)** - Complete deployment guide for production environments
- **[LOAD_BALANCER_INTEGRATION.md](LOAD_BALANCER_INTEGRATION.md)** - Load balancer configuration (NGINX, AWS ALB, GCP, Istio, HAProxy)

### Security & Posture
- **[SECURITY_POSTURE.md](SECURITY_POSTURE.md)** - Security defaults, unsafe modes, and hardening guide (integrators start here)
- **[SECURITY.md](SECURITY.md)** - Security hardening (GPU, TLS, audit logging, HSM, key rotation)

### Operational Procedures
- **[RUNBOOKS.md](RUNBOOKS.md)** - Overview of all operational runbooks
- **Detailed Runbooks:**
  - [CORE_MODULE_RUNBOOK.md](RUNBOOKS/CORE_MODULE_RUNBOOK.md) - Required environment variables, failure modes, mitigations
  - [UPGRADE_RUNBOOK.md](RUNBOOKS/UPGRADE_RUNBOOK.md) - Zero-downtime upgrade procedures (Rolling, Blue-Green, Canary)
  - [RESTORE_RUNBOOK.md](RUNBOOKS/RESTORE_RUNBOOK.md) - Backup restoration and PITR
  - [FAILOVER_RUNBOOK.md](RUNBOOKS/FAILOVER_RUNBOOK.md) - Failover and recovery procedures
  - [SCALING_RUNBOOK.md](RUNBOOKS/SCALING_RUNBOOK.md) - Horizontal/vertical scaling and HPA

### Disaster Recovery
- **[DISASTER_RECOVERY_PLAN.md](DISASTER_RECOVERY_PLAN.md)** - Complete DR plan with RTO/RPO targets, recovery procedures, and testing schedules

### Monitoring & Performance
- **[MONITORING.md](MONITORING.md)** - Observability setup with Prometheus and Grafana
- **[PERFORMANCE_TUNING.md](PERFORMANCE_TUNING.md)** - Optimization techniques and best practices

### Security & Troubleshooting
- **[SECURITY.md](SECURITY.md)** - Security hardening and best practices
- **[TROUBLESHOOTING.md](TROUBLESHOOTING.md)** - Common issues and solutions

## ✅ Checklists

Pre-planned checklists for common operational scenarios:

- **[pre_deployment.md](CHECKLISTS/pre_deployment.md)** - Pre-deployment validation checklist
- **[post_deployment.md](CHECKLISTS/post_deployment.md)** - Post-deployment validation checklist
- **[incident_response.md](CHECKLISTS/incident_response.md)** - Structured incident handling
- **[operational_compliance.md](CHECKLISTS/operational_compliance.md)** - Monthly compliance verification

## 🎯 Service Level Objectives (SLOs)

### Availability Target: 99.9%
- **Maximum Downtime**: 43 minutes per month
- **MTTD**: < 2 minutes (Mean Time To Detect)
- **MTTR**: < 30 minutes (Mean Time To Recover)

### Performance Targets
- **P95 Latency**: < 200ms
- **P99 Latency**: < 500ms
- **Error Rate**: < 0.1%

### Recovery Objectives

| Scenario | RTO | RPO |
|----------|-----|-----|
| Single Node Failure | 5 min | 0 sec |
| Data Center Outage | 30 min | 5 sec |
| Regional Disaster | 2 hours | 1 min |
| Complete Data Loss | 4 hours | 15 min |
| Ransomware Attack | 6 hours | 1 hour |

## 🚀 Quick Start Examples

### Deploy with Auto-Scaling (Kubernetes)

```bash
helm install themisdb ./helm/themisdb \
  --namespace production \
  --set autoscaling.enabled=true \
  --set autoscaling.minReplicas=3 \
  --set autoscaling.maxReplicas=10 \
  --set autoscaling.targetCPUUtilizationPercentage=75
```

### Enable SLA Monitoring

```bash
# Deploy SLA monitoring dashboard
kubectl apply -f ../../grafana/dashboards/sla-monitoring.json

# Apply SLA alerting rules
kubectl apply -f ../../prometheus/rules/sla-rules.yml
```

### Run Compliance Check

```bash
# Monthly operational compliance check
./operational-compliance-check.sh
```

### Execute DR Drill

```bash
# Quarterly disaster recovery test
themisdb-cli test dr-drill --scenario datacenter-failure
```

## 📊 Monitoring Dashboards

Located in [../../grafana/dashboards/](../../grafana/dashboards/):
- **sla-monitoring.json** - SLA compliance, availability, error budgets

## 🔔 Alerting Rules

Located in [../../prometheus/rules/](../../prometheus/rules/):
- **sla-rules.yml** - SLA breach detection, error budget monitoring

## 📦 Example Configurations

Located in [examples/](examples/):
- **[k8s_production_values.yaml](examples/k8s_production_values.yaml)** - Production Helm values with TLS, env vars, probes, and autoscaling
- **single_gpu_setup.yaml** - Single GPU development/testing configuration
- **multi_gpu_setup.yaml** - Multi-GPU production configuration
- **distributed_training.yaml** - Distributed training setup
- **raid_configuration.yaml** - High-availability storage configuration

## 🔐 Security Resources

- **[SECURITY_POSTURE.md](SECURITY_POSTURE.md)** - Explicit security defaults vs. production-hardened settings
- **[RUNBOOKS/CORE_MODULE_RUNBOOK.md](RUNBOOKS/CORE_MODULE_RUNBOOK.md)** - Required environment variables and failure modes
- **[deploy/systemd/](../../deploy/systemd/)** - systemd service unit and production drop-in

## 🔄 Operational Workflows

### Standard Deployment Workflow
1. Review [Pre-Deployment Checklist](CHECKLISTS/pre_deployment.md)
2. Follow [Deployment Guide](DEPLOYMENT.md)
3. Configure [Load Balancer](LOAD_BALANCER_INTEGRATION.md)
4. Set up [Monitoring](MONITORING.md)
5. Complete [Post-Deployment Checklist](CHECKLISTS/post_deployment.md)

### Upgrade Workflow
1. Review [Upgrade Runbook](RUNBOOKS/UPGRADE_RUNBOOK.md)
2. Create full backup
3. Test in staging
4. Execute rolling/blue-green/canary upgrade
5. Validate and monitor

### Incident Response Workflow
1. Follow [Incident Response Checklist](CHECKLISTS/incident_response.md)
2. Consult [Troubleshooting Guide](TROUBLESHOOTING.md)
3. Execute appropriate runbook (Failover/Restore)
4. Document and conduct post-incident review

### Monthly Operations Review
1. Complete [Operational Compliance Checklist](CHECKLISTS/operational_compliance.md)
2. Review SLA metrics and error budgets
3. Conduct capacity planning
4. Update documentation as needed

## 📞 Emergency Contacts

- **On-Call Engineer**: [PagerDuty / Phone]
- **Operations Lead**: [Email / Phone]
- **Engineering Manager**: [Email / Phone]
- **CTO**: [Email / Phone]

## 📚 Additional Resources

- **Main Operations Guide**: [../OPERATIONS.md](../OPERATIONS.md)
- **Architecture Documentation**: [../de/architecture/](../de/architecture/)
- **API Reference**: [../api/](../api/)
- **Contributing Guide**: [../../CONTRIBUTING.md](../../CONTRIBUTING.md)
- **Security Policy**: [../../SECURITY.md](../../SECURITY.md)

## 🔐 Document Classification

**Classification**: Internal - Operations  
**Audience**: Operations Teams, SREs, DevOps Engineers  
**Review Cycle**: Quarterly  
**Version**: 1.0  
**Last Updated**: 2026-04-06

---

**For questions or feedback**, please:
- Open an issue: https://github.com/makr-code/ThemisDB/issues
- Start a discussion: https://github.com/makr-code/ThemisDB/discussions
- Contact operations team: ops@themisdb.com
