# Incident Response Playbook

**Version:** 1.5.0  
**Last Updated:** 2026-04-06  
**Addresses:** FIND-030 - Formalize and Test Incident Response Drills  
**Standards:** ISO 27001 A.16, BSI C5 OIS-01 to OIS-04

---

## Overview

This Incident Response Playbook provides comprehensive procedures for detecting, responding to, and recovering from security incidents affecting ThemisDB. It aligns with ISO 27001 Annex A.16 (Information security incident management) and BSI C5 requirements.

---

## Table of Contents

1. [Incident Response Framework](#incident-response-framework)
2. [Incident Classification](#incident-classification)
3. [Response Team Structure](#response-team-structure)
4. [Response Procedures](#response-procedures)
5. [Incident Scenarios](#incident-scenarios)
6. [Communication Protocols](#communication-protocols)
7. [Post-Incident Activities](#post-incident-activities)
8. [Drill Testing](#drill-testing)

---

## Incident Response Framework

### IR Lifecycle

```
Detection → Triage → Containment → Eradication → Recovery → Lessons Learned
```

### Phase Descriptions

**1. Detection**
- Identify potential security incident
- Collect initial evidence
- Determine incident type and scope

**2. Triage**
- Assess incident severity
- Classify incident category
- Assign response team
- Initiate incident ticket

**3. Containment**
- Isolate affected systems
- Prevent incident spread
- Preserve evidence
- Implement temporary controls

**4. Eradication**
- Remove root cause
- Eliminate threat actor access
- Patch vulnerabilities
- Verify threat removal

**5. Recovery**
- Restore systems to normal operation
- Validate system integrity
- Monitor for recurrence
- Update security controls

**6. Lessons Learned**
- Conduct post-incident review
- Document findings and improvements
- Update procedures and controls
- Share knowledge with team

---

## Incident Classification

### Severity Levels

| Level | Response Time | Description | Examples |
|-------|---------------|-------------|----------|
| **P0 - Critical** | 15 minutes | Severe impact on operations, data breach | Active data breach, ransomware, complete service outage |
| **P1 - High** | 1 hour | Significant impact, security vulnerability | Critical vulnerability, partial service disruption, attempted breach |
| **P2 - Medium** | 4 hours | Moderate impact, potential security issue | Policy violation, minor service degradation, suspicious activity |
| **P3 - Low** | 24 hours | Minimal impact, informational | False positive alerts, minor configuration issues |

### Incident Categories

**Security Incidents:**
- Data breach / data exfiltration
- Unauthorized access
- Malware / ransomware
- DDoS attack
- Insider threat
- Social engineering

**Operational Incidents:**
- Service outage
- Performance degradation
- Data corruption
- Hardware failure

**Compliance Incidents:**
- Policy violation
- Regulatory non-compliance
- Audit finding
- Privacy breach

---

## Response Team Structure

### Incident Response Team (IRT)

**Incident Commander (IC)**
- Overall incident coordination
- Decision-making authority
- Stakeholder communication
- Resource allocation

**Security Lead**
- Technical security analysis
- Threat investigation
- Containment strategy
- Security tool operation

**Operations Lead**
- System operations
- Infrastructure management
- Service restoration
- Performance monitoring

**Communications Lead**
- Internal communications
- External notifications
- Media relations
- Customer communication

**Legal/Compliance Lead**
- Regulatory requirements
- Legal implications
- Data breach notifications
- Evidence preservation

### Contact Information

**Emergency Contacts:**
```yaml
incident_commander:
  name: "Security Director"
  phone: "+1-555-0100"
  email: "ic@example.com"
  
security_lead:
  name: "Security Team Lead"
  phone: "+1-555-0101"
  email: "security-lead@example.com"
  
operations_lead:
  name: "Operations Manager"
  phone: "+1-555-0102"
  email: "ops-lead@example.com"
  
legal_compliance:
  name: "Compliance Officer"
  phone: "+1-555-0103"
  email: "compliance@example.com"
```

---

## Response Procedures

### Initial Response (First 15 Minutes)

**Step 1: Incident Detection**
```bash
# Automated alerting triggers incident
# Manual detection via monitoring or user report
```

**Step 2: Initial Assessment**
- [ ] Confirm incident is genuine (not false positive)
- [ ] Identify affected systems/data
- [ ] Estimate impact and scope
- [ ] Determine severity level

**Step 3: Activate Response**
- [ ] Create incident ticket
- [ ] Notify Incident Commander
- [ ] Assemble response team
- [ ] Establish communication channel (Slack #incident-YYYYMMDD)

**Step 4: Initial Containment**
- [ ] Isolate affected systems (if P0/P1)
- [ ] Preserve evidence
- [ ] Begin logging all actions

### Triage Phase (15-60 Minutes)

**Investigation:**
- [ ] Analyze logs and alerts
- [ ] Identify attack vector
- [ ] Assess data exposure
- [ ] Determine root cause
- [ ] Map affected assets

**Classification:**
- [ ] Confirm severity level
- [ ] Assign incident category
- [ ] Identify compliance requirements
- [ ] Determine notification obligations

**Resource Allocation:**
- [ ] Assign team members to tasks
- [ ] Request additional resources if needed
- [ ] Set up incident war room
- [ ] Establish shift schedule (for P0 incidents)

### Containment Phase

**Short-term Containment:**
- [ ] Block malicious IP addresses
- [ ] Revoke compromised credentials
- [ ] Disable compromised accounts
- [ ] Isolate affected network segments
- [ ] Apply emergency patches

**Long-term Containment:**
- [ ] Implement monitoring on affected systems
- [ ] Deploy temporary security controls
- [ ] Create backup of compromised systems (forensics)
- [ ] Maintain business operations (if possible)

### Eradication Phase

**Threat Removal:**
- [ ] Remove malware/backdoors
- [ ] Close vulnerability exploited
- [ ] Reset all compromised credentials
- [ ] Rebuild compromised systems
- [ ] Validate threat removal

**Vulnerability Remediation:**
- [ ] Patch vulnerable systems
- [ ] Update security configurations
- [ ] Strengthen access controls
- [ ] Deploy additional monitoring

### Recovery Phase

**System Restoration:**
- [ ] Restore systems from clean backups
- [ ] Verify system integrity
- [ ] Re-enable services gradually
- [ ] Monitor for anomalies
- [ ] Confirm business operations restored

**Validation:**
- [ ] Security scan completed
- [ ] Penetration test (if applicable)
- [ ] Data integrity verified
- [ ] Performance baseline restored

---

## Incident Scenarios

### Scenario 1: Data Breach

**Detection Indicators:**
- Unusual data access patterns
- Large data transfers
- Unauthorized API calls
- Alert from DLP system

**Response Steps:**
1. **Immediate (0-15 min):**
   - Isolate affected database
   - Identify compromised accounts
   - Block data exfiltration paths
   
2. **Short-term (15-60 min):**
   - Analyze access logs
   - Identify breach scope
   - Preserve forensic evidence
   - Notify legal team
   
3. **Recovery:**
   - Revoke all access tokens
   - Reset database credentials
   - Implement enhanced monitoring
   - Notify affected parties (if required)

**See also:** [Data Breach Response](INCIDENT_RESPONSE_DATA_BREACH.md)

### Scenario 2: Ransomware Attack

**Detection Indicators:**
- Files encrypted with unknown extension
- Ransom note displayed
- Unusual process activity
- Backup systems targeted

**Response Steps:**
1. **Immediate (0-5 min):**
   - Isolate infected systems from network
   - Shut down affected servers
   - Prevent spread to backups
   
2. **Short-term (5-60 min):**
   - Identify ransomware variant
   - Assess encryption scope
   - Verify backup integrity
   - Do NOT pay ransom
   
3. **Recovery:**
   - Rebuild affected systems
   - Restore from clean backups
   - Deploy enhanced endpoint protection
   - Update security awareness training

**See also:** [Ransomware Response](INCIDENT_RESPONSE_RANSOMWARE.md)

### Scenario 3: DDoS Attack

**Detection Indicators:**
- Service unavailable
- Network bandwidth saturation
- High volume of requests from multiple IPs
- Alert from DDoS protection service

**Response Steps:**
1. **Immediate (0-15 min):**
   - Activate DDoS mitigation service
   - Implement rate limiting
   - Block attacking IP ranges
   
2. **Short-term (15-60 min):**
   - Analyze attack pattern
   - Scale infrastructure (if possible)
   - Notify ISP/hosting provider
   - Update DNS/CDN configuration
   
3. **Recovery:**
   - Monitor traffic patterns
   - Gradually restore service
   - Implement enhanced DDoS protection
   - Document attack characteristics

### Scenario 4: Insider Threat

**Detection Indicators:**
- Unauthorized access to sensitive data
- Data copied to external devices
- Policy violation alerts
- Suspicious user behavior

**Response Steps:**
1. **Immediate (0-15 min):**
   - Suspend user account
   - Revoke access credentials
   - Preserve user activity logs
   - Notify HR and legal
   
2. **Investigation (1-7 days):**
   - Analyze user access logs
   - Identify accessed/copied data
   - Interview user (HR/Legal present)
   - Assess damage
   
3. **Recovery:**
   - Revoke all user access
   - Rotate credentials for accessed systems
   - Implement enhanced monitoring
   - Update security policies

### Scenario 5: Service Outage

**Detection Indicators:**
- Service health check failures
- Customer reports
- Monitoring alerts
- Error rate spike

**Response Steps:**
1. **Immediate (0-15 min):**
   - Confirm outage scope
   - Check infrastructure status
   - Review recent changes
   - Initiate status page update
   
2. **Short-term (15-60 min):**
   - Identify root cause
   - Implement failover (if applicable)
   - Rollback problematic changes
   - Scale resources if needed
   
3. **Recovery:**
   - Restore service operation
   - Verify data consistency
   - Monitor service stability
   - Communicate resolution to customers

---

## Communication Protocols

### Internal Communication

**Incident Slack Channel:** `#incident-YYYYMMDD-ID`

**Update Frequency:**
- P0: Every 15 minutes
- P1: Every hour
- P2: Every 4 hours
- P3: Daily

**Update Template:**
```
**Incident Update [HH:MM UTC]**
Status: [Detection/Containment/Recovery/Resolved]
Impact: [Brief description]
Actions Taken: [Key actions since last update]
Next Steps: [Planned actions]
ETA: [Estimated resolution time]
```

### External Communication

**Customer Notification:**
- P0/P1: Within 1 hour (if customer impact)
- P2: Within 4 hours (if customer impact)
- P3: No notification unless requested

**Regulatory Notification:**
- Data breach: Within 72 hours (GDPR)
- Follow regulatory requirements
- Legal team approval required

**Status Page Updates:**
```
# Status page: https://status.example.com

# P0/P1 incidents
- Initial: "Investigating - We are aware of an issue affecting [service]"
- Update: "Identified - We have identified the issue and are working on a fix"
- Resolved: "Resolved - Service has been restored. We will provide a post-mortem."
```

---

## Post-Incident Activities

### Incident Review Meeting

**Timing:** Within 5 business days of incident resolution

**Attendees:**
- Incident Response Team
- Affected team leads
- Management (for P0/P1)

**Agenda:**
1. Incident timeline review
2. Root cause analysis
3. Response effectiveness assessment
4. Improvement opportunities
5. Action items assignment

### Post-Incident Report

**Report Sections:**
1. **Executive Summary**
   - Incident overview
   - Impact assessment
   - Key findings
   
2. **Timeline**
   - Detection to resolution
   - Key decision points
   - Response actions
   
3. **Root Cause Analysis**
   - Technical root cause
   - Contributing factors
   - Systemic issues
   
4. **Response Assessment**
   - What went well
   - What could be improved
   - Gaps identified
   
5. **Action Items**
   - Corrective actions
   - Preventive measures
   - Process improvements
   
6. **Lessons Learned**
   - Key takeaways
   - Best practices
   - Knowledge sharing

**Report Template:** `templates/post-incident-report.md`

### Follow-up Actions

- [ ] Implement corrective actions
- [ ] Update documentation
- [ ] Conduct security awareness training
- [ ] Update detection rules
- [ ] Test improved procedures
- [ ] Share lessons learned with team

---

## Drill Testing

### Automated Incident Response Drills

**Frequency:** Monthly

**Script:** `scripts/operations/incident-drill.sh`

```bash
# Run incident response drill
./scripts/operations/incident-drill.sh --scenario data-breach

# List available scenarios
./scripts/operations/incident-drill.sh --list-scenarios

# Generate drill report
./scripts/operations/incident-drill.sh --report
```

**Available Scenarios:**
1. `data-breach` - Simulated data breach
2. `ransomware` - Ransomware attack simulation
3. `ddos` - DDoS attack simulation
4. `insider-threat` - Insider threat scenario
5. `service-outage` - Service outage scenario

### Drill Process

**1. Pre-Drill:**
- [ ] Schedule drill date/time
- [ ] Notify participants
- [ ] Prepare scenario details
- [ ] Set up monitoring

**2. During Drill:**
- [ ] Inject scenario
- [ ] Observe team response
- [ ] Take notes on response time
- [ ] Document actions taken

**3. Post-Drill:**
- [ ] Conduct debrief
- [ ] Collect feedback
- [ ] Identify improvement areas
- [ ] Generate drill report

### Drill Metrics

| Metric | Target | Description |
|--------|--------|-------------|
| Time to detect | < 5 minutes | Scenario to detection |
| Time to triage | < 15 minutes | Detection to response activation |
| Time to contain | < 30 minutes | Activation to containment |
| Team response rate | 100% | Team members responding |
| Procedure adherence | > 90% | Following documented procedures |

**See also:** [Incident Response Testing](INCIDENT_RESPONSE_TESTING.md)

---

## Tools & Resources

### Incident Response Tools

- **SIEM:** Centralized log analysis
- **EDR:** Endpoint detection and response
- **Network Monitor:** Traffic analysis
- **Forensics Tools:** Evidence collection
- **Communication:** Slack, PagerDuty

### Reference Documentation

- ISO 27001 A.16 - Information security incident management
- BSI C5 OIS-01 to OIS-04 - Operational security
- NIST SP 800-61 - Computer Security Incident Handling Guide
- SANS Incident Handler's Handbook

### External Resources

- **Security Team:** security@example.com
- **Legal Team:** legal@example.com
- **PR/Communications:** pr@example.com
- **External IR Firm:** (retained for P0 incidents)

---

## Appendix

### Incident Response Checklist

**Detection Phase:**
- [ ] Incident identified and confirmed
- [ ] Initial evidence collected
- [ ] Severity assessed
- [ ] Incident ticket created

**Triage Phase:**
- [ ] Response team assembled
- [ ] Communication channel established
- [ ] Affected systems identified
- [ ] Incident classified

**Containment Phase:**
- [ ] Affected systems isolated
- [ ] Evidence preserved
- [ ] Spread prevented
- [ ] Temporary controls implemented

**Eradication Phase:**
- [ ] Root cause identified
- [ ] Threat removed
- [ ] Vulnerabilities patched
- [ ] Systems hardened

**Recovery Phase:**
- [ ] Systems restored
- [ ] Integrity verified
- [ ] Services re-enabled
- [ ] Monitoring enhanced

**Post-Incident Phase:**
- [ ] Review meeting conducted
- [ ] Post-incident report completed
- [ ] Action items assigned
- [ ] Lessons learned documented

---

## Related Documentation

- [Incident Response Testing](INCIDENT_RESPONSE_TESTING.md)
- [Data Breach Response](INCIDENT_RESPONSE_DATA_BREACH.md)
- [Ransomware Response](INCIDENT_RESPONSE_RANSOMWARE.md)
- [Operations Handbook](../OPERATIONS_HANDBOOK.md)
- [Security Deployment Guide](../../security/SECURITY_DEPLOYMENT_GUIDE.md)
- [Audit Findings Report](../../../audit-reports/v1.4.1/FINDINGS_AND_RISKS.md)

---

**Document Version:** 1.5.0  
**Compliance:** ISO 27001 A.16, BSI C5 OIS-01 to OIS-04  
**Last Reviewed:** 2026-02-03  
**Next Review:** 2026-05-03 (Quarterly)
