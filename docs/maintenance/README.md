# Maintenance Documentation

**Version:** 1.8.1  
**Last Updated:** May 2026  
**Target Audience:** Database Administrators, SREs, Operations Teams

---

## Overview

This directory contains operational maintenance documentation for ThemisDB. It covers scheduled maintenance procedures, module integration guidance, and orchestration design for automated maintenance tasks.

---

## Contents

| Document | Purpose |
|----------|---------|
| [MAINTENANCE_SCHEDULE.md](MAINTENANCE_SCHEDULE.md) | Daily/Weekly/Monthly/Quarterly automated maintenance schedules and how to enable/configure them |
| [MODULE_INTEGRATION_GUIDE.md](MODULE_INTEGRATION_GUIDE.md) | Guide for integrating modules with the maintenance orchestrator |
| [ORCHESTRATOR_DESIGN.md](ORCHESTRATOR_DESIGN.md) | Architecture and design of the maintenance orchestration system |

---

## Maintenance Schedule Overview

All maintenance schedules are **disabled by default** and must be explicitly enabled by operators.

| Schedule | Cron | Key Tasks |
|----------|------|----------|
| Daily | `0 2 * * *` | Metrics collection, fragmentation monitoring, quota check |
| Weekly | `0 2 * * 0` | Consistency check, replica validation, performance analysis, MVCC cleanup |
| Monthly | `0 2 1 * *` | Full integrity scan, backup verification, capacity trend analysis |
| Quarterly | `0 0 1 1,4,7,10 *` | Disaster recovery drill, performance baseline update |

→ Full schedule details: [MAINTENANCE_SCHEDULE.md](MAINTENANCE_SCHEDULE.md)

### Enabling a Schedule

```bash
# Find the schedule ID
curl -s -H "Authorization: Bearer $TOKEN" \
     https://themisdb/api/v1/maintenance/schedules \
  | jq '.schedules[] | select(.name=="Daily Maintenance") | .id'

# Enable it
curl -X PATCH -H "Authorization: Bearer $TOKEN" \
     -H "Content-Type: application/json" \
     -d '{"enabled": true}' \
     https://themisdb/api/v1/maintenance/schedules/<id>
```

---

## Regular Maintenance Checklist

### Daily (Automated)
- [ ] Metrics collection and Prometheus counter snapshot
- [ ] Storage fragmentation monitoring and alerting
- [ ] Quota threshold validation

### Weekly (Automated)
- [ ] Index consistency check
- [ ] Replica lag validation
- [ ] Slow query and lock contention analysis
- [ ] MVCC garbage collection

### Monthly (Automated + Manual Review)
- [ ] Full database integrity scan
- [ ] Backup restore verification
- [ ] Capacity trend analysis and runway projection
- [ ] Index fragmentation report review

### Quarterly (Human-Supervised)
- [ ] Disaster recovery drill (RTO/RPO validation)
- [ ] Performance baseline refresh
- [ ] Security patching review
- [ ] Documentation update

---

## Related Documentation

- **Operations Hub**: [../OPERATIONS.md](../OPERATIONS.md)
- **Operations Handbook**: [../operations/OPERATIONS_HANDBOOK.md](../operations/OPERATIONS_HANDBOOK.md)
- **Backup & Recovery**: [../backup_recovery_system.md](../backup_recovery_system.md)
- **Disaster Recovery Plan**: [../production/DISASTER_RECOVERY_PLAN.md](../production/DISASTER_RECOVERY_PLAN.md)
- **Monitoring Guide**: [../production/MONITORING.md](../production/MONITORING.md)
- **Operational Runbooks**: [../production/RUNBOOKS.md](../production/RUNBOOKS.md)

---

**Document Classification:** Internal – Operations  
**Review Cycle:** Quarterly  
**Maintained by:** Operations Team
