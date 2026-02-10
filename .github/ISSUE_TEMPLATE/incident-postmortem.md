---
name: 🚨 Incident Post-Mortem
about: Strukturierte Analyse eines Produktions-Incidents / Structured analysis of a production incident
title: '[INCIDENT] '
labels: ['type:post-mortem', 'area:incident', 'priority:high']
assignees: ''
---

<!-- 
Template für Incident Post-Mortem Analysen
Template for incident post-mortem analyses
Verwendung: Nach jedem signifikanten Produktions-Incident / Use after any significant production incident
-->

## 📋 Incident Summary / Incident-Zusammenfassung

**Incident ID:** <!-- z.B. INC-2026-001 -->
**Incident Date:** <!-- YYYY-MM-DD -->
**Detection Time:** <!-- HH:MM UTC -->
**Resolution Time:** <!-- HH:MM UTC -->
**Duration:** <!-- Total incident duration -->
**Severity:** <!-- P0 (Critical) / P1 (High) / P2 (Medium) / P3 (Low) -->
**Status:** <!-- Resolved / Ongoing / Investigating -->

**One-Line Summary:**
<!-- Brief description of what happened -->

---

## 🎯 Impact Assessment / Impact-Bewertung

### User Impact / Benutzer-Impact
- **Users Affected:** <!-- Number or percentage -->
- **Geographic Scope:** <!-- Global, Regional, Specific location -->
- **Service Degradation:** <!-- Complete outage, partial, slow performance -->
- **Customer-Facing Impact:** 
  - [ ] Complete service unavailable
  - [ ] Degraded performance
  - [ ] Intermittent errors
  - [ ] Data loss/corruption
  - [ ] Other: _______

**User Impact Description:**


### Business Impact / Geschäfts-Impact
- **Revenue Impact:** <!-- $ amount or estimate -->
- **SLA Violations:** <!-- Yes/No, which SLA -->
- **Customer Complaints:** <!-- Count -->
- **Support Tickets Created:** <!-- Count -->
- **Reputation Impact:** <!-- High/Medium/Low -->

### Technical Impact / Technischer Impact
- **Systems Affected:**
  - [ ] Database
  - [ ] API Services
  - [ ] LLM Inference
  - [ ] Storage Layer
  - [ ] Network
  - [ ] Authentication
  - [ ] Other: _______

**Affected Components:**
- 
- 
- 

---

## ⏱️ Timeline / Zeitlinie

### Incident Timeline / Incident-Zeitlinie

**All times in UTC**

| Time | Event | Action Taken |
|------|-------|--------------|
| HH:MM | Incident began | |
| HH:MM | First alert triggered | |
| HH:MM | Incident acknowledged | |
| HH:MM | Investigation started | |
| HH:MM | Root cause identified | |
| HH:MM | Fix deployed | |
| HH:MM | Service restored | |
| HH:MM | Incident closed | |

### Detailed Timeline / Detaillierte Zeitlinie

**[HH:MM] Event:**
- What happened: 
- Who was involved: 
- Actions taken: 

**[HH:MM] Event:**
- What happened: 
- Who was involved: 
- Actions taken: 

**[HH:MM] Event:**
- What happened: 
- Who was involved: 
- Actions taken: 

---

## 🔍 Root Cause Analysis / Ursachen-Analyse

### What Happened? / Was ist passiert?
<!-- Detailed description of the incident -->


### Why Did It Happen? / Warum ist es passiert?
<!-- 5 Whys analysis recommended -->

**Why #1:**

**Why #2:**

**Why #3:**

**Why #4:**

**Why #5:**

### Root Cause / Grundursache
<!-- The fundamental reason the incident occurred -->


### Contributing Factors / Beitragende Faktoren
1. 
2. 
3. 

---

## 🛡️ Detection & Response / Erkennung & Reaktion

### How Was It Detected? / Wie wurde es erkannt?
- [ ] **Automated monitoring** alert
- [ ] **User report**
- [ ] **Internal team** discovery
- [ ] **External partner** notification
- [ ] **Health check** failure
- [ ] Other: _______

**Detection Details:**


### Response Quality / Reaktions-Qualität
- **Time to Detect:** <!-- From incident start to detection -->
- **Time to Acknowledge:** <!-- From detection to acknowledgment -->
- **Time to Mitigate:** <!-- From acknowledgment to mitigation -->
- **Time to Resolve:** <!-- From acknowledgment to full resolution -->

**Response Effectiveness:** <!-- Excellent/Good/Fair/Poor -->

**What Went Well:**
1. 
2. 
3. 

**What Could Be Improved:**
1. 
2. 
3. 

---

## 🔧 Resolution / Lösung

### Immediate Mitigation / Sofortige Abmilderung
<!-- What was done to quickly reduce impact -->


### Permanent Fix / Dauerhafte Lösung
<!-- What was done to permanently resolve the issue -->


### Verification / Überprüfung
- [ ] Fix verified in staging
- [ ] Fix verified in production
- [ ] Monitoring confirms resolution
- [ ] No regression detected

**Verification Details:**


---

## 📊 Incident Metrics / Incident-Metriken

### Availability Impact / Verfügbarkeits-Impact
- **Uptime This Month (before):** <!-- % -->
- **Uptime This Month (after):** <!-- % -->
- **SLA Target:** <!-- % -->
- **SLA Status:** <!-- Met/Breached -->

### Performance Metrics / Performance-Metriken
**During Incident:**
- Average Response Time: 
- Error Rate: 
- Throughput: 

**Normal Baseline:**
- Average Response Time: 
- Error Rate: 
- Throughput: 

---

## 🎓 Lessons Learned / Gelernte Lektionen

### What Went Well / Was gut lief
1. 
2. 
3. 

### What Didn't Go Well / Was nicht gut lief
1. 
2. 
3. 

### Where We Got Lucky / Wo wir Glück hatten
1. 
2. 
3. 

---

## ✅ Action Items / Aktionspunkte

### Prevent Recurrence / Wiederholung verhindern
1. [ ] **Action 1:**
   - Description: 
   - Owner: 
   - Due Date: 
   - Priority: P0/P1/P2
   - Status: 

2. [ ] **Action 2:**
   - Description: 
   - Owner: 
   - Due Date: 
   - Priority: 
   - Status: 

3. [ ] **Action 3:**
   - Description: 
   - Owner: 
   - Due Date: 
   - Priority: 
   - Status: 

### Improve Detection / Erkennung verbessern
1. [ ] **Action 1:**
   - Description: 
   - Owner: 
   - Due Date: 
   - Priority: 
   - Status: 

2. [ ] **Action 2:**
   - Description: 
   - Owner: 
   - Due Date: 
   - Priority: 
   - Status: 

### Improve Response / Reaktion verbessern
1. [ ] **Action 1:**
   - Description: 
   - Owner: 
   - Due Date: 
   - Priority: 
   - Status: 

2. [ ] **Action 2:**
   - Description: 
   - Owner: 
   - Due Date: 
   - Priority: 
   - Status: 

### Improve Documentation / Dokumentation verbessern
1. [ ] **Action 1:**
   - Description: 
   - Owner: 
   - Due Date: 
   - Priority: 
   - Status: 

---

## 📚 Supporting Information / Unterstützende Informationen

### Logs & Traces / Logs & Traces
- **Log Files:** <!-- Links to relevant logs -->
- **Traces:** <!-- Links to distributed traces -->
- **Metrics:** <!-- Links to metric dashboards -->
- **Screenshots:** <!-- Links to screenshots -->

### Related Incidents / Verwandte Incidents
- **Similar Incidents:** <!-- INC-YYYY-XXX -->
- **Related Outages:** 
- **Known Issues:** 

### Communication / Kommunikation
- **Status Page Updates:** <!-- Links -->
- **Customer Communications:** <!-- Links to emails, notifications -->
- **Internal Communications:** <!-- Slack threads, emails -->
- **External Communications:** <!-- Partner notifications -->

---

## 🔄 Follow-Up / Nachverfolgung

### Post-Mortem Meeting / Post-Mortem-Besprechung
- **Meeting Date:** <!-- YYYY-MM-DD -->
- **Attendees:** 
- **Meeting Notes:** <!-- Link or summary -->

### Action Item Tracking / Aktionspunkt-Verfolgung
- **Tracking Location:** <!-- Jira, GitHub, etc. -->
- **Review Cadence:** <!-- Weekly, bi-weekly -->
- **Next Review Date:** <!-- YYYY-MM-DD -->

### Long-Term Improvements / Langfristige Verbesserungen
1. 
2. 
3. 

---

## 📋 Review & Sign-Off / Review & Freigabe

### Review Status / Review-Status
- [ ] Post-mortem draft completed
- [ ] Technical review completed
- [ ] Management review completed
- [ ] Affected teams reviewed
- [ ] Action items assigned
- [ ] Post-mortem published

### Sign-Off / Freigabe
- **Incident Commander:** <!-- Name, Date -->
- **Engineering Lead:** <!-- Name, Date -->
- **Operations Lead:** <!-- Name, Date -->
- **Management:** <!-- Name, Date -->

---

## 🎯 Incident Classification / Incident-Klassifizierung

### Incident Category / Incident-Kategorie
- [ ] **Infrastructure** (Hardware, Network, Cloud)
- [ ] **Software Bug** (Code defect)
- [ ] **Configuration** (Misconfiguration)
- [ ] **Capacity** (Resource exhaustion)
- [ ] **Security** (Attack, vulnerability)
- [ ] **Human Error** (Operational mistake)
- [ ] **Third-Party** (External service)
- [ ] **Unknown** (Unexplained)

### Preventability / Vermeidbarkeit
- **Could this have been prevented?** <!-- Yes/No -->
- **How?** 

### Similar Incidents / Ähnliche Incidents
- **Has this happened before?** <!-- Yes/No -->
- **When?** 
- **Were previous action items completed?** <!-- Yes/No/Partial -->

---

## 📖 References / Referenzen

### Internal Documentation
- [Incident Response Plan](docs/operations/incident-response.md)
- [Runbooks](docs/operations/runbooks/)
- [Architecture Documentation](docs/architecture/)

### External Resources
- [Google SRE Book - Postmortem Culture](https://sre.google/sre-book/postmortem-culture/)
- [Atlassian Incident Postmortem Template](https://www.atlassian.com/incident-management/postmortem/templates)
- [PagerDuty Postmortem Guide](https://postmortems.pagerduty.com/)

---

**Post-Mortem Created:** <!-- YYYY-MM-DD -->
**Post-Mortem Published:** <!-- YYYY-MM-DD -->
**Last Updated:** <!-- YYYY-MM-DD -->

---

**Template Version:** 1.0.0  
**Created:** 2026-02-02  
**Maintained by:** ThemisDB Operations Team
