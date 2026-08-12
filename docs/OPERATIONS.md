# Operations Guide

**Version:** 1.8.0-rc1  
**Last Updated:** April 2026  
**Purpose:** Master index for production operations documentation

---

## Overview

This operations guide provides comprehensive documentation for deploying, managing, and troubleshooting ThemisDB in production environments with GPU acceleration.

### Target Audience

- **DevOps Engineers**: Deployment and infrastructure management
- **Site Reliability Engineers**: Monitoring, incident response, and reliability
- **Security Engineers**: Security hardening and compliance
- **System Administrators**: Day-to-day operations and maintenance
- **ML Engineers**: Training and inference workload optimization

---

## Quick Links

### Getting Started
- [Pre-Deployment Checklist](production/CHECKLISTS/pre_deployment.md) - Verify readiness before deployment
- [Deployment Guide](production/DEPLOYMENT.md) - Step-by-step deployment instructions
- [Deployment Documentation Index](deployment/README.md) - All deployment-related docs
- [Post-Deployment Checklist](production/CHECKLISTS/post_deployment.md) - Validation after deployment

### Auto-Scaling & High Availability
- [Kubernetes HPA Configuration](../helm/themisdb/templates/hpa.yaml) - Horizontal Pod Autoscaler setup
- [Scaling Runbook](production/RUNBOOKS/SCALING_RUNBOOK.md) - Horizontal and vertical scaling procedures
- [Load Balancer Integration](production/LOAD_BALANCER_INTEGRATION.md) - NGINX, AWS ALB, GCP LB, Istio, HAProxy

### Day-to-Day Operations
- [Operations Documentation Index](operations/README.md) - Handbooks, runbooks, admin guides
- [Operational Runbooks](production/RUNBOOKS.md) - Standard operational procedures
  - [Upgrade Runbook](production/RUNBOOKS/UPGRADE_RUNBOOK.md) - Zero-downtime upgrade procedures
  - [Restore Runbook](production/RUNBOOKS/RESTORE_RUNBOOK.md) - Backup restoration procedures
  - [Failover Runbook](production/RUNBOOKS/FAILOVER_RUNBOOK.md) - Failover and recovery procedures
  - [Scaling Runbook](production/RUNBOOKS/SCALING_RUNBOOK.md) - Horizontal and vertical scaling
- [Monitoring Guide](production/MONITORING.md) - Metrics, dashboards, and alerting
- [Troubleshooting Guide](production/TROUBLESHOOTING.md) - Common issues and solutions
- [CDC Operations Runbook](CDC_OPERATIONS_RUNBOOK.md) - Change Data Capture operational procedures
- [Backup & Recovery System](backup_recovery_system.md) - Backup types, restore, PITR

### Operational Excellence
- [Disaster Recovery Plan](production/DISASTER_RECOVERY.md) - DR procedures, RTO/RPO, backup strategies
- [SLA Monitoring](production/SLA_MONITORING.md) - Service level agreements, Prometheus alerts, Grafana dashboards
- [Auto-Scaling Guide](production/AUTOSCALING.md) - Kubernetes HPA/VPA, load balancer integration

### Security & Compliance
- [Security Documentation Index](security/README.md) - All security-related docs and quick reference
- [Security Hardening](production/SECURITY.md) - Security best practices and configuration
- [Compliance Checklists](production/CHECKLISTS/compliance.md) - SOC2, GDPR, HIPAA compliance
- [Incident Response](production/CHECKLISTS/incident_response.md) - Structured incident handling
- [Operational Compliance Checklist](production/CHECKLISTS/operational_compliance.md) - Monthly compliance verification

### CI/CD & Release
- [CI/CD Documentation](ci-cd/README.md) - Workflow architecture, release process
- [CI/CD Architecture](ci-cd/ci-architecture.md) - Pipeline design and workflow details
- [Release Workflows](ci-cd/workflows/04-release/) - Release automation documentation
- [Maintenance Schedule](maintenance/README.md) - Automated maintenance tasks

### Performance
- [Performance Tuning](production/PERFORMANCE_TUNING.md) - Optimization techniques and best practices
- [Load Balancer Integration](production/LOAD_BALANCER_INTEGRATION.md) - Load balancer configuration and setup

### Disaster Recovery
- [Disaster Recovery Plan](production/DISASTER_RECOVERY_PLAN.md) - Complete DR plan with RTO/RPO targets

---

## Documentation Structure

```
docs/
├── OPERATIONS.md (this file)
└── production/
    ├── DEPLOYMENT.md                  # Installation and configuration
    ├── DISASTER_RECOVERY_PLAN.md      # Complete DR plan with RTO/RPO
    ├── LOAD_BALANCER_INTEGRATION.md   # Load balancer configuration
    ├── PERFORMANCE_TUNING.md          # Optimization guide
    ├── MONITORING.md                  # Observability setup
    ├── TROUBLESHOOTING.md             # Problem resolution
    ├── RUNBOOKS.md                    # Operational procedures
    ├── SECURITY.md                    # Security hardening
    ├── RUNBOOKS/
    │   ├── UPGRADE_RUNBOOK.md         # Upgrade procedures
    │   ├── RESTORE_RUNBOOK.md         # Backup restoration
    │   ├── FAILOVER_RUNBOOK.md        # Failover & recovery
    │   └── SCALING_RUNBOOK.md         # Scaling operations
    ├── CHECKLISTS/
    │   ├── pre_deployment.md          # Pre-deployment validation
    │   ├── post_deployment.md         # Post-deployment validation
    │   ├── incident_response.md       # Incident handling
    │   └── operational_compliance.md  # Monthly compliance check
    └── examples/
        ├── single_gpu_setup.yaml      # Single GPU configuration
        ├── multi_gpu_setup.yaml       # Multi-GPU configuration
        ├── distributed_training.yaml  # Distributed training
        └── raid_configuration.yaml    # High-availability storage

grafana/
└── dashboards/
    └── sla-monitoring.json            # SLA monitoring dashboard

prometheus/
└── rules/
    └── sla-rules.yml                  # SLA alerting rules

helm/themisdb/
└── templates/
    ├── hpa.yaml                       # Horizontal Pod Autoscaler
    └── servicemonitor.yaml            # Prometheus ServiceMonitor
```

---

## Deployment Scenarios

### Single GPU Development

**Use Case:** Development, testing, small-scale inference

**Documentation:**
- [Deployment Guide - Single GPU](production/DEPLOYMENT.md#single-gpu-setup)
- [Example Configuration](production/examples/single_gpu_setup.yaml)

**Hardware:**
- 1x RTX 3090/4090 or A100
- 64GB RAM
- 500GB NVMe SSD

### Multi-GPU Production

**Use Case:** Training workloads, high-throughput inference

**Documentation:**
- [Deployment Guide - Multi-GPU](production/DEPLOYMENT.md#multi-gpu-setup-data-parallel)
- [Example Configuration](production/examples/multi_gpu_setup.yaml)
- [Performance Tuning - Multi-GPU](production/PERFORMANCE_TUNING.md#multi-gpu-tuning)

**Hardware:**
- 4-8x A100 or H100 GPUs
- 256GB+ RAM
- 2TB+ NVMe RAID

### Distributed Multi-Node

**Use Case:** Large-scale distributed training, enterprise deployments

**Documentation:**
- [Deployment Guide - Multi-Node](production/DEPLOYMENT.md#multi-node-distributed-setup)
- [Example Configuration](production/examples/distributed_training.yaml)
- [Runbooks - Scaling Operations](production/RUNBOOKS.md#scaling-operations)

**Hardware:**
- Multiple nodes with 4-8 GPUs each
- InfiniBand or 100 GbE networking
- Shared storage (Lustre, BeeGFS)

---

## Common Tasks

### Initial Deployment

1. **Pre-Deployment**
   - [ ] Complete [Pre-Deployment Checklist](production/CHECKLISTS/pre_deployment.md)
   - [ ] Verify hardware compatibility
   - [ ] Install GPU drivers and CUDA
   - [ ] Configure networking and storage

2. **Deployment**
   - [ ] Follow [Deployment Guide](production/DEPLOYMENT.md)
   - [ ] Apply appropriate configuration (see [examples](production/examples/))
   - [ ] Configure security settings ([Security Guide](production/SECURITY.md))
   - [ ] Set up monitoring ([Monitoring Guide](production/MONITORING.md))

3. **Post-Deployment**
   - [ ] Complete [Post-Deployment Checklist](production/CHECKLISTS/post_deployment.md)
   - [ ] Run validation tests
   - [ ] Verify monitoring and alerting
   - [ ] Document deployment

### Job Submission

**Training Job:**
```bash
# See detailed procedure in Runbooks
themisdb-cli job submit --config training-job.yaml
```

- [Training Job Submission](production/RUNBOOKS.md#training-job-submission)
- [Performance Optimization](production/PERFORMANCE_TUNING.md#batch-size-selection)

**Inference Deployment:**
```bash
# See detailed procedure in Runbooks
themisdb-cli inference deploy --model llama-2-7b
```

- [Multi-Shard Inference Setup](production/RUNBOOKS.md#multi-shard-inference-setup)
- [Latency Optimization](production/PERFORMANCE_TUNING.md#latency-optimization)

### Checkpoint Management

```bash
# Save checkpoint
themisdb-cli checkpoint save --job-id <job-id>

# Restore from checkpoint
themisdb-cli job restore --checkpoint <checkpoint-id>
```

- [Checkpoint Management Runbook](production/RUNBOOKS.md#model-checkpoint-management)

### LoRA Adapter Deployment

```bash
# Deploy LoRA adapter
themisdb-cli lora deploy --adapter custom-adapter
```

- [LoRA Deployment Runbook](production/RUNBOOKS.md#lora-adapter-deployment)

### Monitoring

**View GPU Metrics:**
- Grafana Dashboard: http://localhost:3000
- Prometheus: http://localhost:9090
- GPU Status: `nvidia-smi dmon`

- [Monitoring Setup](production/MONITORING.md)
- [Key Metrics](production/MONITORING.md#key-metrics)
- [Alerting Rules](production/MONITORING.md#alerting-rules)

**SLA Monitoring:**
- [SLA Dashboard](../grafana/dashboards/sla-monitoring.json) - Track availability, latency, and error budgets
- [SLA Alerting Rules](../prometheus/rules/sla-rules.yml) - Prometheus alerts for SLA breaches
- Target: 99.9% availability, P95 < 200ms, < 0.1% error rate

### Troubleshooting

**GPU Issues:**
- [GPU Errors](production/TROUBLESHOOTING.md#gpu-errors)
- [Out of Memory](production/TROUBLESHOOTING.md#out-of-memory-oom)
- [CUDA Compatibility](production/TROUBLESHOOTING.md#cuda-compatibility)

**Performance Issues:**
- [Performance Degradation](production/TROUBLESHOOTING.md#performance-degradation)
- [GPU Throttling](production/TROUBLESHOOTING.md#gpu-throttling)

**Training Issues:**
- [Loss is NaN](production/TROUBLESHOOTING.md#loss-is-nan-or-inf)
- [Training Not Converging](production/TROUBLESHOOTING.md#training-not-converging)

---

## Incident Response

### Quick Response

For production incidents:

1. **Assess Severity** (P0-Critical, P1-High, P2-Medium, P3-Low)
2. **Follow Incident Response Checklist**: [incident_response.md](production/CHECKLISTS/incident_response.md)
3. **Use Troubleshooting Guide**: [TROUBLESHOOTING.md](production/TROUBLESHOOTING.md)
4. **Execute Emergency Procedures**: [RUNBOOKS.md#emergency-procedures](production/RUNBOOKS.md#emergency-procedures)

### Common Incidents

| Incident | Response Guide |
|----------|----------------|
| GPU Failure | [Troubleshooting - GPU Errors](production/TROUBLESHOOTING.md#gpu-errors) |
| Out of Memory | [Troubleshooting - OOM](production/TROUBLESHOOTING.md#out-of-memory-oom) |
| Service Down | [Runbooks - Emergency](production/RUNBOOKS.md#emergency-procedures) |
| Security Breach | [Runbooks - Security Incident](production/RUNBOOKS.md#security-incident-response) |
| Data Corruption | [Troubleshooting - Data Corruption](production/TROUBLESHOOTING.md#data-corruption) |

---

## Maintenance

### Regular Maintenance

**Daily:**
- Monitor GPU health and utilization
- Review logs for errors
- Check disk space
- Verify backups completed

**Weekly:**
- Review performance metrics
- Check for GPU driver updates
- Review security logs
- Update documentation

**Monthly:**
- RAID scrub (if applicable)
- Security patching
- Performance tuning review
- Disaster recovery test

See [Runbooks - Maintenance Windows](production/RUNBOOKS.md#maintenance-windows)

### Planned Maintenance

1. Schedule maintenance window
2. Follow [Maintenance Procedures](production/RUNBOOKS.md#planned-maintenance-procedure)
3. Complete [Post-Deployment Checklist](production/CHECKLISTS/post_deployment.md)

---

## Security

### Security Checklist

- [ ] TLS 1.3 configured
- [ ] mTLS enabled for inter-node communication
- [ ] Disk encryption enabled
- [ ] Audit logging configured
- [ ] GPU access controls configured
- [ ] Key rotation automated
- [ ] Security monitoring active

See [Security Hardening Guide](production/SECURITY.md)

### Compliance

**Supported Standards:**
- SOC 2
- GDPR
- HIPAA

See [Security - Compliance](production/SECURITY.md#compliance)

---

## Performance Optimization

### Quick Wins

1. **Enable Mixed Precision**: 2-3x speedup
   - See [Performance Tuning - Mixed Precision](production/PERFORMANCE_TUNING.md#mixed-precision-training)

2. **Optimize Batch Size**: Maximize GPU utilization
   - See [Performance Tuning - Batch Size](production/PERFORMANCE_TUNING.md#batch-size-selection)

3. **Enable Gradient Checkpointing**: 60-80% memory savings
   - See [Performance Tuning - VRAM Optimization](production/PERFORMANCE_TUNING.md#vram-optimization)

4. **Use Flash Attention**: 15-25% speedup, 30% memory reduction
   - See [Performance Tuning - Flash Attention](production/PERFORMANCE_TUNING.md#flash-attention)

### Performance Targets

| Metric | Target |
|--------|--------|
| GPU Utilization (Training) | >85% |
| Inference P95 Latency | <100ms |
| Training Throughput | >1000 samples/sec |
| GPU Memory Usage | <90% |
| Uptime | >99.9% |

---

## Support

### Getting Help

1. **Check Documentation**: Search this operations guide
2. **Review Logs**: `sudo journalctl -u themisdb -f`
3. **Run Diagnostics**: `themisdb-cli debug dump`
4. **Search Issues**: https://github.com/makr-code/ThemisDB/issues
5. **Community Forum**: https://github.com/makr-code/ThemisDB/discussions

### Support Channels

- **GitHub Issues**: Bug reports and feature requests
- **Discussions**: Questions and community support
- **Documentation**: This operations guide
- **Emergency**: Follow on-call procedures

### Creating Support Bundle

```bash
# Collect diagnostic information
themisdb-cli support-bundle --output /tmp/support-bundle.tar.gz

# Include in support request
```

---

## Additional Resources

### Related Documentation
- [Architecture Overview](de/architecture/README.md)
- [API Reference](de/apis/HTTP_API_REFERENCE.md)
- [AQL Language Guide](de/aql/README.md)
- [Client SDKs](clients/README.md)

### External Resources
- [NVIDIA GPU Documentation](https://docs.nvidia.com/)
- [CUDA Programming Guide](https://docs.nvidia.com/cuda/)
- [NCCL Documentation](https://docs.nvidia.com/deeplearning/nccl/)
- [RocksDB Wiki](https://github.com/facebook/rocksdb/wiki)

---

## Changelog

### Version 1.8.1 (May 2026)
- Updated documentation structure and cross-links
- Added Operations, Deployment, Maintenance, and Security directory indices
- Aligned version references across all operations documents
- Added CI/CD release process links

### Version 1.0 (January 2026)
- Initial production operations documentation
- Comprehensive deployment guides
- Performance tuning guidelines
- Security hardening procedures
- Operational runbooks
- Troubleshooting guides
- Example configurations

---

## Feedback

We welcome feedback on this documentation:
- Submit issues: https://github.com/makr-code/ThemisDB/issues
- Contribute improvements: [CONTRIBUTING.md](../CONTRIBUTING.md)
- Discuss: https://github.com/makr-code/ThemisDB/discussions

---

**Document Version:** 1.8.1  
**Last Updated:** May 2026  
**Next Review:** August 2026

---

**Quick Reference:**

```bash
# Health check
themisdb-cli health

# GPU status
nvidia-smi

# Submit job
themisdb-cli job submit --config job.yaml

# Monitor job
themisdb-cli job status <job-id>

# View logs
sudo journalctl -u themisdb -f

# Backup
themisdb-cli backup create --type full

# Emergency stop
sudo systemctl stop themisdb
```
