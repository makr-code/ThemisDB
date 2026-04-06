# Incident Response Checklist

**Version:** 1.8.0-rc1  
**Last Updated:** April 2026  
**Purpose:** Structured approach to handle production incidents

---

## Incident Classification

### Severity Levels

**P0 - Critical** (Response time: Immediate)
- Complete system outage
- Data loss or corruption
- Security breach
- All users affected

**P1 - High** (Response time: <30 minutes)
- Major feature unavailable
- Severe performance degradation
- Multiple users affected
- GPU failures

**P2 - Medium** (Response time: <2 hours)
- Minor feature unavailable
- Moderate performance issues
- Limited user impact
- Non-critical warnings

**P3 - Low** (Response time: <24 hours)
- Cosmetic issues
- Minor degradation
- Single user affected
- Enhancement requests

---

## Incident Response Phases

```
1. DETECT → 2. ASSESS → 3. CONTAIN → 4. INVESTIGATE → 5. RESOLVE → 6. REVIEW
```

---

## Phase 1: Detection & Initial Response

### Immediate Actions (First 5 Minutes)

- [ ] **Incident detected**
  - Source: [ ] Monitoring alert  [ ] User report  [ ] Team member
  - Time detected: _______________
  - Detection method: _______________

- [ ] **Alert team**
  - On-call engineer notified
  - Incident channel created (#incident-YYYYMMDD-NNN)
  - Stakeholders notified (for P0/P1)
  - War room established (for P0)

- [ ] **Capture initial information**
  - Symptoms observed: _______________
  - Affected services: _______________
  - User impact estimate: _______________
  - Severity level: P0 / P1 / P2 / P3

- [ ] **Start incident log**
  ```
  Incident ID: INC-YYYYMMDD-NNN
  Start time: _______________
  Severity: _______________
  On-call: _______________
  ```

---

## Phase 2: Assessment

### Assess Impact (5-10 Minutes)

- [ ] **Determine scope**
  - Number of affected users: _______________
  - Affected components: _______________
  - Geographic region: _______________
  - Business impact: _______________

- [ ] **Check system health**
  ```bash
  # Service status
  sudo systemctl status themisdb
  
  # GPU status
  nvidia-smi
  
  # Recent logs
  sudo journalctl -u themisdb --since "10 minutes ago"
  
  # Resource usage
  top
  df -h
  free -h
  
  # Network connectivity
  ping -c 4 <critical_host>
  ```

- [ ] **Review monitoring**
  - Check Grafana dashboards
  - Review Prometheus alerts
  - Check log aggregation (ELK/Loki)
  - Identify anomalies in metrics

- [ ] **Classify incident type**
  - [ ] GPU failure
  - [ ] Out of memory
  - [ ] Network issue
  - [ ] Storage problem
  - [ ] Software bug
  - [ ] Security incident
  - [ ] Configuration error
  - [ ] External dependency
  - [ ] Other: _______________

---

## Phase 3: Containment

### Immediate Containment (10-20 Minutes)

- [ ] **Prevent escalation**
  - [ ] Stop auto-scaling (if applicable)
  - [ ] Pause new job submissions
  - [ ] Enable read-only mode (if data corruption suspected)
  - [ ] Isolate affected nodes (if multi-node)
  - [ ] Block suspicious traffic (if security incident)

- [ ] **Save forensic data**
  ```bash
  # Capture system state
  themisdb-cli debug dump --output /tmp/forensics-$(date +%s).tar.gz
  
  # Save logs
  sudo journalctl -u themisdb --since "1 hour ago" > /tmp/incident-logs-$(date +%s).log
  
  # GPU state
  nvidia-smi -q > /tmp/gpu-state-$(date +%s).txt
  
  # Network state
  ss -tunap > /tmp/network-state-$(date +%s).txt
  
  # Process state
  ps auxf > /tmp/process-state-$(date +%s).txt
  ```

- [ ] **Implement workaround (if known)**
  - Documented workaround: _______________
  - Applied successfully: [ ] Yes  [ ] No
  - User impact reduced: [ ] Yes  [ ] No

---

## Phase 4: Investigation

### Root Cause Analysis (Ongoing)

- [ ] **Check recent changes**
  - Recent deployments: _______________
  - Configuration changes: _______________
  - Infrastructure changes: _______________
  - External events: _______________

- [ ] **Analyze logs**
  ```bash
  # Error messages
  sudo journalctl -u themisdb -p err --since "1 hour ago"
  
  # GPU errors
  nvidia-smi --query-gpu=gpu_name,ecc.errors.corrected.aggregate.total --format=csv
  
  # Application errors
  grep ERROR /var/log/themisdb/app.log | tail -100
  ```

- [ ] **Check for patterns**
  - Similar past incidents: _______________
  - Known issues: _______________
  - Correlation with metrics: _______________

- [ ] **Test hypothesis**
  - Hypothesis #1: _______________
    - Test: _______________
    - Result: _______________
  - Hypothesis #2: _______________
    - Test: _______________
    - Result: _______________

---

## Phase 5: Resolution

### GPU-Specific Issues

#### GPU Failure
- [ ] Identify failed GPU
- [ ] Check `nvidia-smi` output
- [ ] Attempt GPU reset: `sudo nvidia-smi --gpu-reset --id <gpu_id>`
- [ ] If reset fails, disable GPU: `themisdb-cli gpu disable --device <id>`
- [ ] Redistribute workload
- [ ] Schedule hardware replacement
- [ ] Document in incident log

#### Out of Memory (OOM)
- [ ] Identify OOM source (GPU/system)
- [ ] Clear GPU cache: `themisdb-cli gpu clear-cache`
- [ ] Reduce batch size temporarily
- [ ] Enable gradient checkpointing
- [ ] Kill memory-intensive processes if needed
- [ ] Restart service: `sudo systemctl restart themisdb`
- [ ] Monitor memory usage
- [ ] Document root cause

#### CUDA Errors
- [ ] Check CUDA version compatibility
- [ ] Verify driver version
- [ ] Check for CUDA kernel errors
- [ ] Review recent code changes
- [ ] Enable CUDA debugging: `export CUDA_LAUNCH_BLOCKING=1`
- [ ] Restart with safe configuration
- [ ] Apply fix or rollback

### Network Issues
- [ ] Check network connectivity
- [ ] Verify firewall rules
- [ ] Test bandwidth: `iperf3`
- [ ] Check for packet loss: `ping`
- [ ] Restart network services if needed
- [ ] Check load balancer (if applicable)
- [ ] Verify DNS resolution
- [ ] Test from different locations

### Storage Issues
- [ ] Check disk space: `df -h`
- [ ] Check disk I/O: `iostat -x 1`
- [ ] Check RAID status (if applicable): `cat /proc/mdstat`
- [ ] Clean up old files
- [ ] Rotate logs
- [ ] Expand storage if needed
- [ ] Check filesystem errors: `sudo fsck`

### Application Issues
- [ ] Identify buggy code
- [ ] Apply hotfix if available
- [ ] Rollback to previous version
- [ ] Disable problematic feature
- [ ] Restart with safe configuration
- [ ] Deploy permanent fix

---

## Phase 6: Recovery & Verification

### Service Recovery

- [ ] **Restore service**
  ```bash
  # Start service
  sudo systemctl start themisdb
  
  # Verify health
  themisdb-cli health --wait
  
  # Run smoke tests
  themisdb-cli test inference --quick
  themisdb-cli test training --quick
  ```

- [ ] **Verify functionality**
  - [ ] API endpoints responding
  - [ ] GPU operations working
  - [ ] Data integrity verified
  - [ ] Backups accessible
  - [ ] Monitoring active
  - [ ] Alerts firing correctly

- [ ] **Gradual rollout**
  - [ ] Resume job submissions (low priority first)
  - [ ] Enable auto-scaling
  - [ ] Monitor closely for 1 hour
  - [ ] Resume normal operations

- [ ] **Monitor for recurrence**
  - Watch for same symptoms
  - Monitor key metrics
  - Check logs periodically
  - Keep team on standby

---

## Communication Templates

### Initial Notification (P0/P1)

```
Subject: [P0 INCIDENT] ThemisDB Production Issue - <Brief Description>

We are currently investigating a production issue affecting ThemisDB.

Impact: <User impact description>
Start Time: <YYYY-MM-DD HH:MM UTC>
Status: Investigating
ETA: TBD

We will provide updates every 30 minutes.

Incident Channel: #incident-YYYYMMDD-NNN
On-Call Engineer: <Name>
```

### Status Update

```
Subject: [UPDATE] [P0 INCIDENT] ThemisDB Production Issue

Update: <What we've learned, what we're doing>

Current Status: Investigating / Mitigating / Resolved
Impact: <Current impact>
Next Update: <Time>

Incident Channel: #incident-YYYYMMDD-NNN
```

### Resolution Notification

```
Subject: [RESOLVED] ThemisDB Production Issue

The incident affecting ThemisDB has been resolved.

Root Cause: <Brief explanation>
Resolution: <What was done>
Start Time: <YYYY-MM-DD HH:MM UTC>
End Time: <YYYY-MM-DD HH:MM UTC>
Duration: <HH:MM>

Post-Incident Review: <Date and time>

Thank you for your patience.
```

---

## Post-Incident Review

### Schedule PIR (Within 48 Hours)

- [ ] **PIR meeting scheduled**
  - Date/Time: _______________
  - Attendees: _______________
  - Agenda prepared

### PIR Template

**Incident Summary:**
- Incident ID: _______________
- Severity: _______________
- Duration: _______________
- Impact: _______________

**Timeline:**
```
HH:MM - Event 1
HH:MM - Event 2
HH:MM - Event 3
...
```

**Root Cause:**
```
[Detailed explanation]
```

**What Went Well:**
- Item 1
- Item 2

**What Went Wrong:**
- Item 1
- Item 2

**Action Items:**
- [ ] Action 1 - Owner: ___ - Due: ___
- [ ] Action 2 - Owner: ___ - Due: ___
- [ ] Action 3 - Owner: ___ - Due: ___

---

## Incident Metrics

### Key Metrics to Track

```
Metric                          Value
──────────────────────────────────────
Time to detect (TTD)            ___ min
Time to acknowledge (TTA)       ___ min
Time to mitigate (TTM)          ___ min
Time to resolve (TTR)           ___ min
Total duration                  ___ min
Users affected                  ___
Revenue impact                  $ ___
Similar incidents in past 30d   ___
```

---

## Escalation Paths

### P0 Escalation
1. On-call engineer (immediate)
2. Engineering manager (+15 min)
3. Director of Engineering (+30 min)
4. CTO (+1 hour)

### P1 Escalation
1. On-call engineer (immediate)
2. Engineering manager (+30 min)
3. Director of Engineering (+2 hours)

### Contacts
```
On-Call Engineer:    <phone>
Engineering Manager: <phone>
Security Team:       <phone>
Infrastructure Team: <phone>
PagerDuty:          <service_key>
Slack Channel:      #incident-response
```

---

## Incident Closure

### Checklist

- [ ] Service fully restored
- [ ] Root cause identified
- [ ] Permanent fix deployed
- [ ] Monitoring updated
- [ ] Documentation updated
- [ ] Runbooks updated
- [ ] All stakeholders notified
- [ ] Post-incident review completed
- [ ] Action items assigned
- [ ] Incident report published
- [ ] Metrics recorded
- [ ] Lessons learned documented

### Sign-Off

**Incident Commander:** _________________ Date: _______

**Engineering Lead:** _________________ Date: _______

**Operations Lead:** _________________ Date: _______

---

## Reference Documents

- [Troubleshooting Guide](../TROUBLESHOOTING.md)
- [Runbooks](../RUNBOOKS.md)
- [Monitoring Guide](../MONITORING.md)
- [Emergency Procedures](../RUNBOOKS.md#emergency-procedures)

---

**Checklist Version:** 1.0  
**Last Updated:** April 2026
