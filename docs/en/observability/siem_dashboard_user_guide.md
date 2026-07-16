# SIEM Dashboard User Guide

**Version:** 1.0  
**Last Updated:** 2026-04-06  
**Target Audience:** SOC Analysts, Security Engineers, IT Operations

---

## Introduction

This guide provides instructions for using the ThemisDB SIEM (Security Information and Event Management) Dashboard for daily security monitoring, incident detection, and compliance verification.

## Dashboard Access

**URL**: `https://grafana.your-domain.com/d/themisdb-siem/themisdb-siem-security-monitoring`

**Default Credentials** (Change immediately):
- Username: `admin`
- Password: `admin`

**Recommended Browser**: Chrome, Firefox, or Edge (latest versions)

---

## Dashboard Overview

The SIEM Dashboard is organized into four main sections:

1. **🔐 Authentication & Authorization** - User access monitoring
2. **📋 Audit & Security Events** - Security-relevant operations tracking
3. **🔍 Query & Performance** - System performance and anomalies
4. **🖥️ Infrastructure Metrics** - System health and availability

---

## Section 1: Authentication & Authorization

### Failed Login Attempts (5m)

**What it shows**: Number of failed authentication attempts in the last 5 minutes.

**Color coding**:
- 🟢 Green (0-4): Normal
- 🟡 Yellow (5-9): Elevated - Monitor
- 🔴 Red (10+): Critical - Investigate immediately

**Action Required**:
- **Green**: No action
- **Yellow**: Review authentication logs for patterns
- **Red**: **IMMEDIATE ACTION** - Possible brute force attack
  1. Identify source IPs in "Failed Login Attempts by User/IP" panel
  2. Block suspicious IPs at firewall
  3. Notify security team
  4. Check if any successful logins occurred from these IPs

**Example Investigation**:
```
If panel shows: 25 failed attempts (RED)
1. Look at "Failed Login Attempts by User/IP" panel
2. See: "admin from 203.0.113.10" = 25 attempts
3. Action:
   - Block 203.0.113.10 immediately
   - Check if admin account had successful logins recently
   - Force password reset for admin account
   - Generate incident report
```

### Authentication Attempts Rate

**What it shows**: Real-time authentication activity with success/failure breakdown.

**Normal patterns**:
- Business hours: 10-50 attempts/sec
- Off-hours: 0-5 attempts/sec
- Spikes during shift changes are normal

**Anomalies to watch for**:
- ⚠️ Sudden spike in failures (possible attack)
- ⚠️ High activity during off-hours (unauthorized access attempt)
- ⚠️ Flat line during business hours (monitoring system issue)

**Investigation steps**:
1. Click on legend to isolate specific authentication methods
2. Hover over spikes to see exact values and timestamps
3. Correlate with "Active Sessions" panel
4. Check "Rate Limiting Events" for related activity

### Active Sessions

**What it shows**: Current number of active user sessions.

**Thresholds**:
- 🟢 Green (0-99): Normal
- 🟡 Yellow (100-499): Elevated usage
- 🔴 Red (500+): Possible session exhaustion or attack

**Action Required**:
- If red and unexpected: Check for session hijacking or connection leak
- Compare with "Authentication Attempts Rate" - should correlate with login activity
- High sessions + high failed logins = potential distributed attack

### Privilege Escalation Events (5m)

**What it shows**: Number of privilege escalation events in last 5 minutes.

**Thresholds**:
- 🟢 Green (0): Normal
- 🟡 Yellow (1-4): Review required
- 🔴 Red (5+): Critical - Investigate immediately

**Action Required**:
- **ANY value > 0**: Review "Admin Actions" panel for details
- Verify authorization workflow was followed
- Check if user had legitimate business need
- Audit similar privilege changes in last 24 hours

**CRITICAL**: Unauthorized privilege escalation is a **Tier 1 security incident**. Follow incident response protocol immediately.

### Rate Limiting Events

**What it shows**: API endpoints where rate limits were exceeded.

**Normal patterns**:
- Occasional spikes for automated tools
- Should be < 10 events/sec normally

**Anomalies**:
- Sustained high rate limiting = DoS attempt or misconfigured client
- Multiple IPs hitting same endpoint = coordinated attack
- Sudden spike to new endpoint = possible exploit attempt

**Action steps**:
1. Identify endpoint and source IP from graph
2. Check application logs for endpoint behavior
3. If malicious: Block IP and increase rate limits if needed
4. If legitimate: Whitelist IP or adjust rate limits

---

## Section 2: Audit & Security Events

### Audit Events - CRUD Operations

**What it shows**: Database operations (Create, Read, Update, Delete) over time.

**Normal patterns**:
- Steady rate during business hours
- Lower rate during off-hours
- Delete operations should be infrequent

**Anomalies to watch for**:
- ⚠️ Sudden spike in Delete operations (mass deletion attempt)
- ⚠️ Unusually high Read operations (data scraping)
- ⚠️ Create spikes not matching business activity

**Investigation approach**:
1. Click operation in legend to isolate (e.g., "delete")
2. Check "Top 10 User Activities" for user correlation
3. Review "Data Export Events" for related exfiltration
4. Verify business justification for unusual patterns

### Admin Actions (5m)

**What it shows**: Administrative operations by user in last 5 minutes.

**High-risk actions to monitor**:
- `user_delete` - User account deletion
- `config_change` - System configuration changes
- `backup_restore` - Data restore operations
- `role_modify` - Permission changes

**Action Required**:
- Verify each admin action has:
  1. Valid change ticket/approval
  2. Business justification
  3. Authorized approver
  4. Post-change verification

**Red flags**:
- Admin actions from unexpected users
- Actions outside change windows
- Multiple failed admin actions (possible unauthorized attempt)

### Top 10 User Activities (15m)

**What it shows**: Most active users and their actions in last 15 minutes.

**Use this panel to**:
- Identify power users vs. anomalous activity
- Spot users with unusual activity patterns
- Correlate activity across other panels

**Example patterns**:
- Normal: `bob | SELECT | 150` (Bob running reports)
- Suspicious: `alice | DELETE | 5000` (Alice not authorized for mass deletes)
- Concerning: `service_account | config_change | 100` (Service account shouldn't modify config)

**Investigation steps**:
1. Click user name to filter all panels
2. Check user's normal baseline from historical data
3. Verify user role and permissions
4. If suspicious: Suspend account and review all actions

### Security Policy Checks

**What it shows**: Real-time policy enforcement (RBAC, data access, encryption, retention).

**Normal patterns**:
- Steady rate of allowed checks
- Very few denied checks (< 1% of total)

**Anomalies**:
- High denial rate = misconfigured policies or unauthorized access attempts
- Spike in specific policy type = targeted probing
- Zero checks = monitoring system failure

**Action Required**:
- Denied checks: Review user permissions
- Pattern of denials from same user: Possible unauthorized access attempt
- Unexpected allowed checks: Verify policy configuration

### Security Incidents (5m)

**What it shows**: Detected security incidents in last 5 minutes.

**Incident Types**:
- `brute_force` - Multiple authentication failures
- `data_exfiltration` - Unauthorized data export
- `unauthorized_access` - Failed authorization attempts
- `malware` - Malicious code detection

**CRITICAL**: ANY security incident requires immediate investigation

**Incident Response Protocol**:
1. **Assess**: Click incident to see details
2. **Contain**: Isolate affected systems/accounts
3. **Investigate**: Collect logs and evidence
4. **Remediate**: Fix vulnerability and restore normal operations
5. **Report**: Document incident and notify stakeholders

### Data Export Events (5m)

**What it shows**: Data export operations in last 5 minutes.

**Thresholds**:
- 🟢 Green (0-9): Normal
- 🟡 Yellow (10-49): Elevated - Monitor
- 🔴 Red (50+): Critical - Investigate

**High-risk scenarios**:
- Export of sensitive/restricted data
- Exports to external destinations
- Exports during off-hours
- High-volume exports by single user

**Action Required**:
- Review "Top 10 User Activities" for export user details
- Verify export authorization and business need
- Check destination (file, API, email) for data leakage risk
- If unauthorized: **INITIATE DATA BREACH PROTOCOL**

### Configuration Changes (5m)

**What it shows**: System configuration modifications in last 5 minutes.

**Critical components to watch**:
- `security` - Security settings changes
- `network` - Network configuration
- `logging` - Audit log settings
- `database` - Database parameters

**Action Required**:
- ALL configuration changes should have:
  - Change ticket reference
  - Pre-approval from change board
  - Back-out plan
  - Post-change testing
- Unauthorized changes: **IMMEDIATE ROLLBACK** and incident investigation

### Policy Violations (5m)

**What it shows**: Security policy violations in last 5 minutes.

**Severity levels**:
- Low: User education required
- Medium: Policy review and user warning
- High: Mandatory user training and account review
- Critical: Account suspension and investigation

**Action Required**:
- Document all violations
- Notify user and manager
- Apply remediation per policy
- Escalate critical violations to security team

---

## Section 3: Query & Performance

### Query Success Rate

**What it shows**: Percentage of successful queries (gauge).

**Thresholds**:
- 🟢 Green (99-100%): Excellent
- 🟡 Yellow (95-98.9%): Acceptable
- 🔴 Red (<95%): Critical - System issue

**Low success rate causes**:
- Permission errors (check authorization metrics)
- Syntax errors (user training needed)
- System overload (check infrastructure metrics)
- Attack attempt (check error types)

**Action steps**:
1. Check "Query Errors by Type" panel for error breakdown
2. Review "Query Request Rate" for load correlation
3. Examine "Infrastructure Metrics" for resource constraints
4. If attack suspected: Check authentication and rate limiting panels

### Query Request Rate

**What it shows**: Total query requests vs. errors over time.

**Normal patterns**:
- Request rate follows business activity
- Error rate stays near zero
- Spikes correlate with batch jobs or report generation

**Anomalies**:
- Flat request rate + high active sessions = query bottleneck
- High request rate from single user = possible scraping
- Error rate tracking request rate = system issue

### Cache Hit Rate

**What it shows**: Percentage of queries served from cache (gauge).

**Thresholds**:
- 🟢 Green (80-100%): Excellent
- 🟡 Yellow (50-79%): Acceptable
- 🔴 Red (<50%): Poor - Performance impact

**Low cache hit rate causes**:
- Cache invalidation spike (check config changes)
- New query patterns (normal after feature release)
- Cache size too small (scale cache)
- Attack generating unique queries (check authentication)

### Slow Queries (5m)

**What it shows**: Number of queries exceeding performance threshold in last 5 minutes.

**Thresholds**:
- 🟢 Green (0-9): Normal
- 🟡 Yellow (10-49): Elevated - Monitor
- 🔴 Red (50+): Critical - Performance issue

**Action Required**:
- Check "Query Latency Percentiles" for severity
- Review query logs for slow query details
- If sudden spike: Check infrastructure metrics
- If persistent: Query optimization needed

### Query Latency Percentiles

**What it shows**: Query response times at 50th, 95th, and 99th percentiles.

**Target SLAs**:
- P50: < 50ms (median user experience)
- P95: < 100ms (95% of users)
- P99: < 500ms (worst-case acceptable)

**Threshold breaches**:
- Yellow: P95 > 100ms or P99 > 500ms
- Red: P95 > 500ms or P99 > 1000ms

**Investigation approach**:
1. If all percentiles high: System-wide issue (check infrastructure)
2. If only P99 high: Specific slow queries (check slow query logs)
3. Sudden spike: Check for new deployments or load changes
4. Gradual increase: Capacity planning needed

### Query Errors by Type (5m)

**What it shows**: Breakdown of error types in last 5 minutes.

**Error Types**:
- `syntax` - Query syntax errors (user education)
- `permission` - Authorization failures (check access control)
- `timeout` - Query timeouts (performance issue)
- `resource` - Resource exhaustion (scale system)

**Action steps**:
1. Identify dominant error type
2. Check corresponding mitigation panel:
   - syntax → User training
   - permission → Authorization panel
   - timeout → Infrastructure panel
   - resource → Infrastructure panel
3. If attack suspected: Check authentication failures

---

## Section 4: Infrastructure Metrics

### CPU Usage

**What it shows**: Current CPU utilization percentage (gauge).

**Thresholds**:
- 🟢 Green (0-69%): Normal
- 🟡 Yellow (70-89%): Elevated
- 🔴 Red (90-100%): Critical

**Action Required**:
- Yellow: Monitor, prepare to scale
- Red: **IMMEDIATE ACTION**
  1. Check process list for CPU hogs
  2. Review query load
  3. Scale horizontally or vertically
  4. If sudden spike: Check for DoS attack

### Memory Usage

**What it shows**: Current memory utilization percentage (gauge).

**Thresholds**:
- 🟢 Green (0-69%): Normal
- 🟡 Yellow (70-89%): Elevated
- 🔴 Red (90-100%): Critical - OOM risk

**Action Required**:
- Yellow: Review memory consumers
- Red: **IMMEDIATE ACTION**
  1. Identify memory leaks
  2. Clear caches if safe
  3. Scale memory capacity
  4. Prepare for potential restart

### Storage Usage

**What it shows**: Disk space utilization percentage (gauge).

**Thresholds**:
- 🟢 Green (0-79%): Normal
- 🟡 Yellow (80-94%): Elevated
- 🔴 Red (95-100%): Critical

**Action Required**:
- Yellow: Plan storage expansion
- Red: **IMMEDIATE ACTION**
  1. Clear temporary files
  2. Archive old logs
  3. Move large files to archive storage
  4. Expand disk capacity
- If rapid growth: Check for log flooding attack

### Network Traffic

**What it shows**: Network receive (RX) and transmit (TX) rates by interface.

**Normal patterns**:
- RX and TX rates roughly balanced for database traffic
- Spikes correlate with batch jobs and replication
- Low traffic during off-hours

**Anomalies**:
- Very high TX with low RX = data exfiltration
- Sustained high RX = DoS attack
- No traffic = network issue or monitoring failure

**Action steps**:
1. Identify which interface has anomaly
2. Check corresponding panels:
   - High TX → Data Export Events
   - High RX → Query Request Rate
3. If attack: Check authentication and rate limiting

### Replication Status

**What it shows**: Current replication lag for each replica (table).

**Thresholds**:
- 🟢 Green (0-1 sec): Normal
- 🟡 Yellow (1-2 sec): Elevated
- 🔴 Red (>2 sec): Critical

**Action Required**:
- Yellow: Monitor, check replica load
- Red: **CHECK REPLICA HEALTH**
  1. Verify replica connectivity
  2. Check replica resources (CPU, memory, disk)
  3. Review replication logs for errors
  4. If persistent: Consider failover

### Replication Lag

**What it shows**: Replication lag over time by replica.

**Use this panel to**:
- Identify which replica is lagging
- Spot trends (increasing lag = capacity issue)
- Correlate with other metrics (high query load → high lag)

**Investigation approach**:
1. If one replica lagging: Replica-specific issue
2. If all replicas lagging: Primary overload
3. If lag spikes: Correlate with infrastructure metrics
4. If persistent: Capacity planning needed

---

## Common Scenarios and Workflows

### Scenario 1: Brute Force Attack Detection

**Indicators**:
- Failed Login Attempts panel: RED (10+ attempts)
- Failed Login Attempts by User/IP: Spike from single IP
- Rate Limiting Events: High activity

**Workflow**:
1. **Identify**: Note attacking IP from "Failed Login Attempts by User/IP"
2. **Contain**:
   ```bash
   # Block IP at firewall
   sudo iptables -A INPUT -s 203.0.113.10 -j DROP
   ```
3. **Investigate**:
   - Check if any successful logins from this IP
   - Review target accounts
   - Check for similar attacks from different IPs
4. **Remediate**:
   - Force password reset for targeted accounts
   - Enable MFA if not already enabled
   - Update rate limiting rules
5. **Report**: Generate incident report with timeline

### Scenario 2: Unauthorized Data Export

**Indicators**:
- Data Export Events panel: RED (50+ exports)
- Top User Activities: User with high export count
- Network Traffic: Spike in transmit rate

**Workflow**:
1. **Assess**: Identify user and export destination
2. **Contain**: **IMMEDIATELY**
   ```bash
   # Suspend user account
   themisdb-cli user suspend --username=alice
   # Block export destination
   sudo iptables -A OUTPUT -d export.suspicious.com -j DROP
   ```
3. **Investigate**:
   - Review all exports by this user in last 7 days
   - Identify data classification of exported data
   - Check export authorization logs
4. **Determine Breach Impact**:
   - If restricted/sensitive data: **INITIATE BREACH PROTOCOL**
   - Notify DPO, legal, and compliance
   - Prepare breach notifications (GDPR 72-hour rule)
5. **Remediate**:
   - Revoke user access
   - Review and restrict export permissions
   - Implement additional export controls
6. **Report**: Full incident report with evidence chain

### Scenario 3: Privilege Escalation

**Indicators**:
- Privilege Escalation Events panel: Non-zero value
- Admin Actions panel: Unusual role modifications
- Security Policy Checks: Spike in authorization checks

**Workflow**:
1. **Identify**: Review Admin Actions panel for escalation details
2. **Verify Authorization**:
   - Check change management system for ticket
   - Verify approver authorization
   - Confirm business justification
3. **If Unauthorized**:
   - Suspend user account immediately
   - Revert privilege escalation
   - Audit all actions taken with elevated privileges
   - Check for lateral movement attempts
4. **If Authorized but Unusual**:
   - Verify with user and approver via secondary channel
   - Monitor elevated account activity closely
   - Set temporary elevated access expiration
5. **Document**: Record in audit log with justification

### Scenario 4: Performance Degradation

**Indicators**:
- Query Success Rate: Yellow/Red
- Query Latency Percentiles: P95 > 100ms
- Infrastructure: High CPU or memory usage

**Workflow**:
1. **Assess Severity**:
   - Green metrics: No immediate action
   - Yellow metrics: Monitor and prepare
   - Red metrics: Immediate investigation
2. **Identify Cause**:
   - Check "Query Request Rate" for load spike
   - Review "Infrastructure Metrics" for resource bottlenecks
   - Check "Slow Queries" for problematic queries
3. **Immediate Mitigation**:
   - If high load: Enable query rate limiting
   - If resource constrained: Scale resources
   - If slow queries: Kill long-running queries
4. **Long-term Resolution**:
   - Optimize slow queries
   - Implement query caching
   - Review capacity planning
5. **Prevention**: Set up predictive alerts for capacity planning

### Scenario 5: Suspicious Admin Activity

**Indicators**:
- Admin Actions panel: High frequency from single user
- Configuration Changes: Multiple security changes
- Audit Events: Admin accessing sensitive data

**Workflow**:
1. **Identify Admin**: Note username from Admin Actions panel
2. **Verify Legitimacy**:
   - Check if admin is on-duty
   - Verify actions match change schedule
   - Contact admin via secondary channel
3. **If Suspicious**:
   - **DO NOT ALERT SUSPECT**
   - Preserve evidence (take dashboard screenshots)
   - Involve security team immediately
   - Prepare to suspend account
4. **If Compromised Admin Account**:
   - Suspend account
   - Force logout all sessions
   - Review all actions taken
   - Revert malicious changes
   - Forensic investigation
5. **Recovery**:
   - Issue new credentials
   - Review access controls
   - Update security procedures

---

## Alert Response

### Alert Priority Levels

| Priority | Response Time | Actions |
|----------|---------------|---------|
| **Critical** | < 5 minutes | Immediate investigation, possible system shutdown |
| **High** | < 15 minutes | Prioritized investigation, escalation if unresolved in 30 min |
| **Medium** | < 1 hour | Standard investigation, ticket creation |
| **Low** | < 4 hours | Monitor and review during business hours |

### Alert Acknowledgment

When an alert fires:

1. **Acknowledge** in Grafana to stop repeat notifications
2. **Document** acknowledgment in ticketing system
3. **Investigate** per alert priority
4. **Escalate** if unable to resolve within time window
5. **Resolve** and document resolution

### Alert Muting

Alerts can be muted during:
- Scheduled maintenance windows
- Known system changes
- False positive investigation

**NEVER mute**:
- Security incident alerts
- Data exfiltration alerts
- Privilege escalation alerts

---

## Best Practices for SOC Analysts

### Daily Routine

**Start of Shift** (First 15 minutes):
1. Open SIEM Dashboard
2. Review last 24 hours for anomalies
3. Check all alert statuses
4. Review handover notes from previous shift
5. Verify all monitoring systems operational

**During Shift** (Every 30 minutes):
1. Quick scan of all dashboard sections
2. Acknowledge and triage new alerts
3. Follow up on in-progress investigations
4. Update tickets and incident reports

**End of Shift** (Last 15 minutes):
1. Document all findings and actions taken
2. Update shift handover notes
3. Escalate unresolved issues
4. Brief incoming analyst

### Weekly Reviews

1. Review alert fatigue metrics
2. Tune alert thresholds as needed
3. Update baseline for anomaly detection
4. Review incident response effectiveness
5. Update runbooks based on lessons learned

### Dashboard Customization

**Personal Views**:
- Create custom time ranges for your timezone
- Add annotations for shift changes
- Star frequently used dashboards
- Set up custom alert notification preferences

**Team Views**:
- Create role-specific dashboards (Analyst, Manager, Engineer)
- Share dashboard snapshots for briefings
- Export data for compliance reports
- Set up automated screenshot emails for morning briefings

---

## Troubleshooting

### Dashboard Not Loading

**Symptoms**: White screen, "No data", or timeout errors

**Solutions**:
1. Check browser console for errors (F12)
2. Verify Prometheus is running:
   ```bash
   curl http://localhost:9090/api/v1/targets
   ```
3. Check Grafana datasource connection
4. Clear browser cache
5. Try incognito/private mode

### Missing Data in Panels

**Symptoms**: Some panels show "No data"

**Solutions**:
1. Check time range (top right corner)
2. Verify instance filter (top left variables)
3. Check Prometheus scraping:
   ```bash
   curl http://localhost:9091/metrics | grep themis_
   ```
4. Review Prometheus logs for scrape errors
5. Verify ThemisDB metrics are being generated

### Slow Dashboard Performance

**Symptoms**: Long load times, laggy interactions

**Solutions**:
1. Reduce time range (use 1h instead of 24h)
2. Disable auto-refresh temporarily
3. Increase Prometheus resources
4. Use recording rules for complex queries
5. Enable Grafana query caching

### Incorrect Alert Notifications

**Symptoms**: Not receiving alerts or receiving duplicates

**Solutions**:
1. Check Alertmanager status:
   ```bash
   curl http://localhost:9093/api/v1/status
   ```
2. Verify alert rules syntax:
   ```bash
   promtool check rules alerts/siem_security_alerts.yaml
   ```
3. Check email/Slack webhook configuration
4. Review Alertmanager inhibition rules
5. Verify notification channel settings in Grafana

---

## Keyboard Shortcuts

| Shortcut | Action |
|----------|--------|
| `d` + `s` | Open search/navigation |
| `d` + `h` | Go to home dashboard |
| `d` + `k` | Toggle kiosk mode |
| `d` + `d` | Open dashboard settings |
| `e` | Expand time range |
| `t` + `l` | Toggle legend |
| `Ctrl/Cmd` + `s` | Save dashboard (if edit mode) |
| `Esc` | Exit edit mode |

---

## Additional Resources

### Documentation
- **Full SIEM Integration Guide**: `docs/en/observability/siem_integration.md`
- **Alert Configuration**: `grafana/alerts/siem_security_alerts.yaml`
- **Compliance Mapping**: See SIEM Integration Guide, Section 6

### Training
- **Grafana Basics**: https://grafana.com/tutorials/
- **Prometheus Queries**: https://prometheus.io/docs/prometheus/latest/querying/basics/
- **Security Monitoring**: Internal SOC training portal

### Support
- **SOC Team**: soc@example.com
- **Security Team**: security@example.com
- **IT Support**: helpdesk@example.com
- **Emergency**: security-oncall@example.com

---

## Glossary

- **SIEM**: Security Information and Event Management
- **SOC**: Security Operations Center
- **Brute Force**: Attack method using many password attempts
- **Privilege Escalation**: Gaining higher access level than authorized
- **Data Exfiltration**: Unauthorized data export/theft
- **Anomaly**: Deviation from normal behavior baseline
- **Audit Trail**: Complete record of security-relevant events
- **Compliance**: Adherence to regulatory requirements (SOC2, GDPR, HIPAA)
- **P50/P95/P99**: 50th, 95th, 99th percentile (median, 95%, worst-case)
- **Rate Limiting**: Restricting request frequency per user/IP
- **OOM**: Out Of Memory
- **DoS**: Denial of Service attack
- **RX/TX**: Network receive/transmit
- **RBAC**: Role-Based Access Control
- **CRUD**: Create, Read, Update, Delete operations

---

**Document Version**: 1.0  
**Last Review**: 2026-01-27  
**Next Review**: 2026-02-27  
**Owner**: SOC Team Lead  
**Feedback**: soc-feedback@example.com
