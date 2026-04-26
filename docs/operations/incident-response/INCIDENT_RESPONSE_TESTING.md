# Incident Response Testing & Drills

**Version:** 1.5.0  
**Last Updated:** 2026-04-06  
**Addresses:** FIND-030 - Formalize and Test Incident Response Drills  
**Standards:** ISO 27001 A.16.1.5, BSI C5 OIS-02

---

## Overview

This document describes the automated incident response testing framework for ThemisDB. Regular drills ensure the incident response team is prepared to handle security incidents effectively.

---

## Testing Objectives

- **Validate** incident response procedures
- **Train** team members on IR processes
- **Identify** gaps in procedures and tools
- **Measure** response time metrics
- **Improve** team coordination and communication
- **Ensure** compliance with ISO 27001 and BSI C5

---

## Testing Schedule

### Regular Drills

| Drill Type | Frequency | Duration | Participants |
|------------|-----------|----------|--------------|
| **Tabletop Exercise** | Monthly | 1-2 hours | IR Team |
| **Simulation Drill** | Quarterly | 2-4 hours | IR Team + Leadership |
| **Full-Scale Exercise** | Annually | 4-8 hours | All stakeholders |
| **Red Team Exercise** | Annually | 1-2 weeks | Security team + External |

### Drill Calendar

**Monthly Tabletop Exercises:**
- Focus on specific scenario types
- Rotate through all major scenarios
- Document lessons learned

**Quarterly Simulation Drills:**
- Realistic incident simulation
- Test complete IR workflow
- Involve all response teams

**Annual Full-Scale Exercise:**
- Complete incident simulation
- Include external stakeholders
- Test DR procedures
- Media/PR involvement

---

## Automated Drill System

### Script: `incident-drill.sh`

**Location:** `scripts/operations/incident-drill.sh`

**Usage:**
```bash
# Run specific scenario drill
./scripts/operations/incident-drill.sh --scenario data-breach

# List available scenarios
./scripts/operations/incident-drill.sh --list-scenarios

# Run drill with specific difficulty
./scripts/operations/incident-drill.sh --scenario ransomware --difficulty hard

# Generate drill report
./scripts/operations/incident-drill.sh --report --drill-id DR-2026-02-01

# Schedule future drill
./scripts/operations/incident-drill.sh --schedule --date "2026-03-01 10:00" --scenario ddos

# Dry-run mode
./scripts/operations/incident-drill.sh --scenario insider-threat --dry-run
```

**Options:**
- `--scenario <name>` - Drill scenario to run
- `--difficulty <level>` - easy, medium, hard (default: medium)
- `--list-scenarios` - List all available scenarios
- `--report` - Generate drill report
- `--drill-id <id>` - Drill identifier for reporting
- `--schedule` - Schedule future drill
- `--date <datetime>` - Drill date/time
- `--dry-run` - Test mode without actual changes
- `--notify` - Send drill notifications to team

---

## Drill Scenarios

### 1. Data Breach Scenario

**Difficulty:** Medium  
**Duration:** 90 minutes  
**Focus:** Data exposure, containment, notification

**Scenario Injects:**
```yaml
scenario: data-breach
description: "Unauthorized access to customer database detected"
injects:
  - time: "T+0"
    event: "SIEM alert: Unusual database access from external IP"
    expected_action: "Acknowledge alert, begin investigation"
    
  - time: "T+15"
    event: "Evidence of data exfiltration found"
    expected_action: "Activate incident response, notify IC"
    
  - time: "T+30"
    event: "Scope identified: 10,000 customer records accessed"
    expected_action: "Containment measures, preserve evidence"
    
  - time: "T+60"
    event: "Attacker still has active session"
    expected_action: "Terminate session, block IP, revoke credentials"
    
  - time: "T+90"
    event: "Legal team asks about notification requirements"
    expected_action: "Assess GDPR requirements, prepare notification"
```

**Success Criteria:**
- [ ] Alert acknowledged within 5 minutes
- [ ] Incident Commander activated within 15 minutes
- [ ] Containment actions completed within 30 minutes
- [ ] Evidence preserved properly
- [ ] Legal/compliance notified
- [ ] Notification requirements assessed

### 2. Ransomware Attack Scenario

**Difficulty:** Hard  
**Duration:** 120 minutes  
**Focus:** Rapid response, isolation, recovery

**Scenario Injects:**
```yaml
scenario: ransomware
description: "Systems encrypted by ransomware, ransom note detected"
injects:
  - time: "T+0"
    event: "Multiple systems showing encryption activity"
    expected_action: "Immediately isolate affected systems"
    
  - time: "T+5"
    event: "Ransomware spreading to additional systems"
    expected_action: "Network segmentation, disable network shares"
    
  - time: "T+10"
    event: "Backup systems also targeted"
    expected_action: "Isolate backups, verify offline backups"
    
  - time: "T+30"
    event: "Ransom note demanding payment"
    expected_action: "Do NOT pay, consult legal, verify backups"
    
  - time: "T+90"
    event: "Systems ready for restoration"
    expected_action: "Begin restore from clean backups"
```

### 3. DDoS Attack Scenario

**Difficulty:** Medium  
**Duration:** 60 minutes  
**Focus:** Service availability, mitigation, communication

**Scenario Injects:**
```yaml
scenario: ddos
description: "High volume of requests causing service degradation"
injects:
  - time: "T+0"
    event: "Service response time degraded, error rate increasing"
    expected_action: "Confirm DDoS attack, activate mitigation"
    
  - time: "T+15"
    event: "Attack intensifies, service unavailable"
    expected_action: "Implement rate limiting, contact DDoS provider"
    
  - time: "T+30"
    event: "Customers reporting outage"
    expected_action: "Update status page, communicate with customers"
    
  - time: "T+45"
    event: "Mitigation working, traffic normalizing"
    expected_action: "Monitor closely, prepare recovery"
```

### 4. Insider Threat Scenario

**Difficulty:** Hard  
**Duration:** 120 minutes  
**Focus:** Investigation, legal considerations, HR coordination

**Scenario Injects:**
```yaml
scenario: insider-threat
description: "Employee suspected of unauthorized data access"
injects:
  - time: "T+0"
    event: "DLP alert: Large data transfer to personal email"
    expected_action: "Investigate alert, verify user identity"
    
  - time: "T+20"
    event: "User accessed sensitive customer data outside normal pattern"
    expected_action: "Suspend account, notify manager and HR"
    
  - time: "T+40"
    event: "Evidence of data copied to USB device"
    expected_action: "Preserve evidence, involve legal team"
    
  - time: "T+90"
    event: "User claims legitimate business need"
    expected_action: "HR investigation, legal review"
```

### 5. Service Outage Scenario

**Difficulty:** Easy  
**Duration:** 45 minutes  
**Focus:** Diagnosis, recovery, communication

**Scenario Injects:**
```yaml
scenario: service-outage
description: "Database service unavailable"
injects:
  - time: "T+0"
    event: "Health checks failing, service unavailable"
    expected_action: "Confirm outage, check infrastructure"
    
  - time: "T+10"
    event: "Recent deployment identified as potential cause"
    expected_action: "Decision: rollback or fix forward"
    
  - time: "T+25"
    event: "Rollback completed, service still down"
    expected_action: "Deeper investigation, check dependencies"
    
  - time: "T+35"
    event: "Root cause found: database connection pool exhausted"
    expected_action: "Restart service, implement monitoring"
```

---

## Drill Execution Process

### Pre-Drill Phase

**1 Week Before:**
- [ ] Select scenario for drill
- [ ] Notify participants
- [ ] Prepare scenario injects
- [ ] Set up monitoring/logging
- [ ] Brief observers

**Day Before:**
- [ ] Confirm participant availability
- [ ] Test drill infrastructure
- [ ] Prepare communication channels
- [ ] Review scenario timeline

**Day Of:**
- [ ] Final readiness check
- [ ] Open incident channel (#drill-YYYYMMDD)
- [ ] Brief participants on drill scope

### During Drill Phase

**Drill Facilitator Actions:**
1. **Inject scenario events** according to timeline
2. **Observe team response** and take notes
3. **Provide hints** if team is stuck (note this)
4. **Track response times** for each action
5. **Document deviations** from procedures
6. **Record questions/issues** raised

**Participant Actions:**
1. **Respond as if real incident**
2. **Follow documented procedures**
3. **Communicate in incident channel**
4. **Document all actions taken**
5. **Escalate as per procedures**
6. **Ask questions if unclear**

### Post-Drill Phase

**Immediately After:**
- [ ] Stop drill and notify participants
- [ ] Conduct hot debrief (15-30 minutes)
- [ ] Collect immediate feedback
- [ ] Document initial observations

**Within 3 Days:**
- [ ] Generate comprehensive drill report
- [ ] Identify improvement areas
- [ ] Create action items
- [ ] Assign owners and deadlines

**Within 1 Week:**
- [ ] Conduct lessons learned session
- [ ] Update procedures/documentation
- [ ] Schedule follow-up drills if needed
- [ ] Share findings with leadership

---

## Drill Metrics

### Key Performance Indicators

| Metric | Target | Description |
|--------|--------|-------------|
| **Detection Time** | < 5 minutes | Time from event to detection |
| **Activation Time** | < 15 minutes | Time to activate response |
| **Containment Time** | < 30 minutes | Time to contain incident |
| **Communication Delay** | < 10 minutes | Time to notify stakeholders |
| **Procedure Adherence** | > 90% | Following documented procedures |
| **Team Response Rate** | 100% | Team members responding to drill |

### Metrics Collection

**Automated Metrics:**
```bash
# Drill metrics exported to Prometheus
incident_drill_detection_time_seconds{scenario="data-breach"} 180
incident_drill_activation_time_seconds{scenario="data-breach"} 720
incident_drill_containment_time_seconds{scenario="data-breach"} 1620
incident_drill_procedure_adherence_rate{scenario="data-breach"} 0.92
incident_drill_response_rate{scenario="data-breach"} 1.0
```

**Grafana Dashboard:** `Incident Response Drill Metrics`

**Tracked Metrics:**
- Response time trends
- Procedure adherence over time
- Team performance by scenario
- Improvement tracking

---

## Drill Report Template

### Executive Summary

**Drill ID:** DR-2026-02-03  
**Date:** 2026-02-03  
**Scenario:** Data Breach  
**Participants:** 8 IR team members  
**Duration:** 90 minutes  
**Overall Rating:** ⭐⭐⭐⭐ (Good)

### Timeline

| Time | Event | Response | Time to Respond |
|------|-------|----------|-----------------|
| T+0 | Alert triggered | Acknowledged | 3 minutes ✅ |
| T+15 | IC activated | Team assembled | 12 minutes ✅ |
| T+30 | Containment started | Systems isolated | 18 minutes ✅ |
| T+60 | Evidence preserved | Forensics collected | 28 minutes ✅ |
| T+90 | Legal notified | Notification prepared | 35 minutes ⚠️ |

### Performance Assessment

**Strengths:**
- ✅ Quick detection and acknowledgment
- ✅ Effective team coordination
- ✅ Proper containment procedures followed
- ✅ Good communication throughout

**Areas for Improvement:**
- ⚠️ Legal notification took longer than target
- ⚠️ Some confusion about evidence preservation
- ⚠️ Documentation not always up to date

### Findings

| ID | Finding | Severity | Action Required |
|----|---------|----------|-----------------|
| DR-01 | Legal contact list outdated | Medium | Update contact list |
| DR-02 | Evidence preservation procedure unclear | Medium | Clarify documentation |
| DR-03 | SIEM alert rules need tuning | Low | Review alert thresholds |

### Action Items

| Action | Owner | Due Date | Status |
|--------|-------|----------|--------|
| Update legal contact list | Security Lead | 2026-02-10 | Open |
| Revise evidence preservation procedure | Security Lead | 2026-02-15 | Open |
| Schedule SIEM tuning | SOC Team | 2026-02-20 | Open |
| Schedule follow-up drill | IR Lead | 2026-03-03 | Open |

### Metrics Summary

- Detection Time: 3 minutes ✅ (Target: < 5 min)
- Activation Time: 12 minutes ✅ (Target: < 15 min)
- Containment Time: 18 minutes ✅ (Target: < 30 min)
- Procedure Adherence: 92% ✅ (Target: > 90%)
- Team Response Rate: 100% ✅ (Target: 100%)

### Recommendations

1. **Immediate:** Update legal contact list
2. **Short-term:** Conduct tabletop on evidence preservation
3. **Long-term:** Implement automated legal notification system

---

## Continuous Improvement

### Drill Evolution

**Quarterly Review:**
- Analyze drill metrics trends
- Update scenarios based on threat landscape
- Incorporate lessons learned
- Adjust difficulty levels

**Annual Review:**
- Comprehensive IR program assessment
- External evaluation (if applicable)
- Benchmark against industry standards
- Update IR framework

### Feedback Loop

**After Each Drill:**
1. Collect participant feedback
2. Analyze metrics and observations
3. Identify improvement opportunities
4. Update procedures and documentation
5. Schedule follow-up activities

---

## CI/CD Integration

### GitHub Actions Workflow

**File:** `.github/workflows/incident-drill.yml`

```yaml
name: Incident Response Drill

on:
  schedule:
    - cron: '0 10 1 * *'  # Monthly on 1st at 10:00 UTC
  workflow_dispatch:
    inputs:
      scenario:
        description: 'Drill scenario'
        required: true
        type: choice
        options:
          - data-breach
          - ransomware
          - ddos
          - insider-threat
          - service-outage

jobs:
  incident-drill:
    runs-on: ubuntu-latest
    steps:
      - name: Checkout
        uses: actions/checkout@v3
      
      - name: Notify team
        run: |
          ./scripts/operations/notify-drill.sh --scenario ${{ inputs.scenario }}
      
      - name: Run drill
        run: |
          ./scripts/operations/incident-drill.sh --scenario ${{ inputs.scenario }}
      
      - name: Generate report
        run: |
          ./scripts/operations/incident-drill.sh --report
      
      - name: Upload report
        uses: actions/upload-artifact@v3
        with:
          name: drill-report
          path: reports/incident-drill-*.md
```

---

## Compliance Documentation

### ISO 27001 Requirements

**A.16.1.5 - Response to information security incidents**

- ✅ IR procedures documented and tested
- ✅ Regular drills conducted
- ✅ Team training maintained
- ✅ Continuous improvement implemented

### BSI C5 Requirements

**OIS-02 - Incident Management**

- ✅ Incident management process defined
- ✅ Regular testing of IR procedures
- ✅ Documentation of test results
- ✅ Continuous improvement process

---

## Related Documentation

- [Incident Response Playbook](INCIDENT_RESPONSE_PLAYBOOK.md)
- [Operations Handbook](../OPERATIONS_HANDBOOK.md)
- [Audit Findings Report](../../../audit-reports/v1.4.1/FINDINGS_AND_RISKS.md)

---

**Document Version:** 1.5.0  
**Compliance:** ISO 27001 A.16.1.5, BSI C5 OIS-02  
**Last Reviewed:** 2026-02-03
