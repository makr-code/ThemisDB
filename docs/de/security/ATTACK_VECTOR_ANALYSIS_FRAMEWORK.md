# Systematische Angriffsvektoren-Analyse

**Version:** 1.0.0  
**Status:** Aktiv  
**Kategorie:** 🔒 Security Operations

## Übersicht

ThemisDB verfügt über ein umfassendes Framework für die systematische Analyse von Angriffsvektoren. Dieses Framework ermöglicht wiederholbare, automatisierte und manuelle Sicherheitsanalysen auf Basis etablierter Standards (OWASP, BSI C5, ISO 27001, NIST CSF).

## Komponenten

### 1. Automatisierter Workflow

**Datei:** `.github/workflows/attack-vector-analysis.yml`

Der Workflow führt automatisch jeden Dienstag um 3:00 UTC eine vollständige Analyse durch und kann auch manuell getriggert werden.

**Features:**
- ✅ Konfigurierbare Analyse-Scopes (full, network, authentication, injection, crypto, distributed)
- ✅ Automatische Generierung von Analyse-Dokumenten für jede Kategorie
- ✅ Konsolidierte Berichterstattung mit Compliance-Matrix
- ✅ Optionale GitHub Issue-Erstellung für Findings
- ✅ Integration mit bestehenden Security-Scans (OWASP ZAP, AFL++, CodeQL, Trivy)

**Manuelle Ausführung:**

```bash
# Via GitHub CLI
gh workflow run attack-vector-analysis.yml \
  -f analysis_scope=full \
  -f generate_report=true \
  -f create_issues=false

# Via GitHub Web UI
# Actions → Attack Vector Analysis (Systematic) → Run workflow
```

### 2. Runbook für manuelle Analyse

**Datei:** `docs/de/security/ANGRIFFSVEKTOREN_ANALYSE_RUNBOOK.md`

Umfassender Leitfaden für die Durchführung manueller Sicherheitsanalysen mit:
- ✅ Schritt-für-Schritt-Anleitungen für alle Angriffskategorien
- ✅ Code-Beispiele für Penetrationstests
- ✅ Checklisten für Pre/During/Post-Analyse
- ✅ Integration mit Security-Tools (OWASP ZAP, AFL++, Burp Suite, sqlmap, etc.)
- ✅ Severity Levels und SLA-Definitionen
- ✅ Remediation Workflows

### 3. Basis-Analyse-Dokument

**Datei:** `docs/de/security/ANGRIFFSVEKTOREN_ANALYSE.md`

Umfassende Dokumentation aller bekannten Angriffsvektoren auf ThemisDB mit:
- ✅ Externe und interne Angriffsvektoren
- ✅ Risikobewertung und Priorisierung
- ✅ Implementierte Schutzmaßnahmen
- ✅ Monitoring und Detection
- ✅ Handlungsempfehlungen

### 4. Test-Templates

**Verzeichnis:** `tests/security/attack-vectors/`

Strukturierte Test-Templates für die Validierung von Sicherheitsmaßnahmen gegen bekannte Angriffsvektoren.

## Angriffskategorien

### 1. Netzwerk-Angriffsvektoren

**Geprüfte Vektoren:**
- HTTP Request Smuggling
- SSRF (Server-Side Request Forgery)
- CSRF (Cross-Site Request Forgery)
- CORS Misconfiguration
- WebSocket Injection
- gRPC Protocol Attacks
- MQTT Topic Injection
- PostgreSQL Wire Protocol Manipulation

**Automatisierte Tests:**
- OWASP ZAP Baseline & Full Scans
- Network Protocol Fuzzing
- API Security Scans

### 2. Authentifizierung & Autorisierung

**Geprüfte Vektoren:**
- JWT Token Manipulation
- Session Fixation/Hijacking
- Brute Force Attacks
- API Key Enumeration
- Privilege Escalation (Vertical/Horizontal)
- IDOR (Insecure Direct Object Reference)
- RBAC/ABAC Policy Bypass

**Implementierte Schutzmaßnahmen:**
- JWT mit RS256 Signierung
- Rate Limiting
- Account Lockout
- Apache Ranger Integration
- ABAC für feingranulare Kontrolle

### 3. Injection-Angriffsvektoren

**Geprüfte Vektoren:**
- AQL Injection (ThemisDB Query Language)
- NoSQL Injection
- SQL Injection (PostgreSQL Wire)
- Command Injection
- LLM Prompt Injection
- Template Injection

**Automatisierte Tests:**
- AFL++ Fuzzing auf AQL Parser
- AFL++ Fuzzing auf JSON Parser
- Input Validation Tests

### 4. Kryptographie-Angriffsvektoren

**Geprüfte Vektoren:**
- Weak Cipher Suites
- CBC Padding Oracle Attacks
- IV Reuse
- Key Derivation Weaknesses
- Side-Channel Attacks
- Man-in-the-Middle
- Key Management Issues
- HSM/Vault Misconfiguration

**Implementierte Schutzmaßnahmen:**
- AES-256-GCM (Authenticated Encryption)
- TLS 1.2/1.3
- RSA-4096/ECC für Key Exchange
- Argon2 für Password Hashing
- HashiCorp Vault Integration
- HSM Support (PKCS#11)

### 5. Verteilte System-Angriffsvektoren

**Geprüfte Vektoren:**
- Shard Key Enumeration
- Cross-Shard Injection
- Distributed Transaction Manipulation
- Consensus Protocol Attacks (Raft)
- Split-Brain Scenarios
- MVCC Bypass
- Data Integrity Violations
- Audit Log Tampering

**Implementierte Schutzmaßnahmen:**
- gRPC mTLS für Shard-Kommunikation
- Raft Consensus
- Snapshot Isolation
- MVCC (Multi-Version Concurrency Control)
- Audit Log mit digitalen Signaturen

## Workflow-Ausführung

### Automatisch (Scheduled)

Der Workflow läuft automatisch jeden **Dienstag um 3:00 UTC**.

**Scope:** Vollständige Analyse aller Kategorien  
**Output:** Analyse-Artefakte für jede Kategorie + Gesamtbericht

### Manuell (On-Demand)

#### Via GitHub CLI

```bash
# Vollständige Analyse
gh workflow run attack-vector-analysis.yml \
  -f analysis_scope=full \
  -f generate_report=true \
  -f create_issues=false

# Nur Netzwerk-Vektoren
gh workflow run attack-vector-analysis.yml \
  -f analysis_scope=network \
  -f generate_report=true

# Mit automatischer Issue-Erstellung
gh workflow run attack-vector-analysis.yml \
  -f analysis_scope=full \
  -f generate_report=true \
  -f create_issues=true
```

#### Via GitHub Web UI

1. Navigiere zu `Actions` → `Attack Vector Analysis (Systematic)`
2. Klicke auf `Run workflow`
3. Wähle Optionen:
   - **analysis_scope**: `full`, `network`, `authentication`, `injection`, `crypto`, oder `distributed`
   - **generate_report**: `true` für detaillierten Bericht
   - **create_issues**: `true` für automatische Issue-Erstellung bei Findings
4. Klicke `Run workflow`

### Monitoring

```bash
# Workflow-Status überwachen
gh run watch

# Liste aktuelle Runs
gh run list --workflow=attack-vector-analysis.yml --limit 5

# Zeige Details eines Runs
gh run view <RUN_ID>

# Download Artefakte
gh run download <RUN_ID>
```

## Artefakte

Nach Workflow-Completion werden folgende Artefakte generiert:

| Artefakt | Inhalt | Retention |
|----------|--------|-----------|
| `network-vector-analysis` | HTTP, WebSocket, gRPC, MQTT Analysen | 90 Tage |
| `auth-vector-analysis` | Authentication/Authorization Analysen | 90 Tage |
| `injection-vector-analysis` | AQL, NoSQL, Command Injection Analysen | 90 Tage |
| `crypto-vector-analysis` | Verschlüsselung und Key Management Analysen | 90 Tage |
| `distributed-vector-analysis` | Sharding, Consensus, Data Integrity Analysen | 90 Tage |
| `attack-vector-analysis-report` | Konsolidierter Gesamtbericht | 90 Tage |
| `compliance-matrix` | Compliance-Matrix für Standards | 90 Tage |

## Integration mit bestehenden Security-Scans

Der Attack Vector Analysis Workflow integriert sich mit:

### OWASP ZAP (DAST)
- **Workflow:** `.github/workflows/owasp-zap.yml`
- **Frequenz:** Wöchentlich (Montag 2:00 UTC) + bei Code-Änderungen
- **Scan-Typen:** Baseline, Full, API Scans

### AFL++ Fuzzing
- **Workflow:** `.github/workflows/fuzzing.yml`
- **Frequenz:** Wöchentlich (Sonntag 0:00 UTC)
- **Targets:** AQL Parser, JSON Parser, Crypto Operations, Network Protocol, Storage Engine, Auth Handler

### CodeQL (SAST)
- **Workflow:** `.github/workflows/security-scan.yml` (Job: codeql-analysis)
- **Frequenz:** Bei jedem PR + wöchentlich (Sonntag 2:00 UTC)
- **Queries:** security-extended, security-and-quality

### Trivy (Dependency Scanning)
- **Workflow:** `.github/workflows/security-scan.yml` (Job: dependency-scan)
- **Frequenz:** Bei jedem PR + wöchentlich
- **Scanners:** Vulnerabilities, Secrets, Misconfigurations

### Gitleaks (Secret Scanning)
- **Workflow:** `.github/workflows/security-scan.yml` (Job: secret-scanning)
- **Frequenz:** Bei jedem PR + wöchentlich
- **Output:** SARIF Report

### cppcheck (Static Analysis)
- **Workflow:** `.github/workflows/security-scan.yml` (Job: cpp-analysis)
- **Frequenz:** Bei jedem PR + wöchentlich
- **Checks:** Warning, Style, Performance, Portability

## Compliance

Das Framework unterstützt Compliance mit:

### BSI C5 (Cloud Computing Compliance Criteria Catalogue)
- **DEV-01:** Sichere Softwareentwicklung
- **OPS-10:** Vulnerability Management
- **Evidence:** Automatisierte Scans, Dokumentierte Prozesse, Audit Trail

### ISO 27001
- **A.12.6.1:** Management of technical vulnerabilities
- **A.14.2.5:** Secure system engineering principles
- **Evidence:** Systematic Analysis, Security Testing, Documentation

### OWASP ASVS (Application Security Verification Standard)
- **V1:** Architecture, Design and Threat Modeling
- **V4:** Access Control
- **V5:** Validation, Sanitization and Encoding
- **V8:** Data Protection
- **Evidence:** Test Coverage, Security Controls, Threat Model

### NIST Cybersecurity Framework
- **DE.CM-8:** Vulnerability scans are performed
- **PR.DS-5:** Protections against data leaks are implemented
- **Evidence:** Automated Workflows, Regular Scans, Findings Tracking

## Manuelle Validierung

Für detaillierte manuelle Tests siehe:
- **Runbook:** `docs/de/security/ANGRIFFSVEKTOREN_ANALYSE_RUNBOOK.md`
- **Basis-Analyse:** `docs/de/security/ANGRIFFSVEKTOREN_ANALYSE.md`

### Quick Reference

```bash
# 1. Netzwerk-Tests
nmap -sV -sC -p- localhost
./testssl.sh --full https://localhost:8765

# 2. Authentication Tests
themis-cli auth login -u testuser -p testpass
# Test JWT manipulation

# 3. Injection Tests
themis-cli query "FOR doc IN users FILTER doc.name == '1' OR '1'=='1' RETURN doc"

# 4. Krypto-Tests
./testssl.sh --full https://localhost:8765
gitleaks detect --source . --verbose

# 5. Distributed System Tests
# Simuliere Network Partition
iptables -A OUTPUT -d <NODE2_IP> -j DROP
```

## Reporting und Remediation

### Severity Levels

| Severity | CVSS Score | SLA | Beispiel |
|----------|------------|-----|----------|
| **CRITICAL** | 9.0 - 10.0 | 24h | RCE, Authentication Bypass |
| **HIGH** | 7.0 - 8.9 | 7 Tage | SQL Injection, Privilege Escalation |
| **MEDIUM** | 4.0 - 6.9 | 30 Tage | XSS, CSRF |
| **LOW** | 0.1 - 3.9 | 90 Tage | Information Disclosure |

### Issue Creation

Für Findings mit Severity >= MEDIUM werden automatisch GitHub Issues erstellt (wenn aktiviert):

```bash
gh workflow run attack-vector-analysis.yml \
  -f analysis_scope=full \
  -f generate_report=true \
  -f create_issues=true  # Aktiviert automatische Issue-Erstellung
```

**Issue-Template:**
```
Title: [Security] <Finding Titel>

## Beschreibung
<Detaillierte Beschreibung>

## Severity
CRITICAL|HIGH|MEDIUM|LOW

## Reproduktion
<Schritte zur Reproduktion>

## Empfohlene Remediation
<Maßnahmen zur Behebung>

## Referenzen
- CWE: <CWE-ID>
- OWASP: <OWASP Reference>
- Workflow Run: <RUN_ID>

Labels: security, attack-vector-analysis, needs-triage
```

## Best Practices

### Für Entwickler

1. **Vor jedem Release:** Manuellen Full-Scope Scan durchführen
2. **Bei Security-relevanten Änderungen:** Betroffene Kategorie analysieren
3. **Findings dokumentieren:** Alle Findings in `ANGRIFFSVEKTOREN_ANALYSE.md` eintragen
4. **Tests erweitern:** Neue Tests für neue Features hinzufügen

### Für Security Engineers

1. **Wöchentliche Review:** Automatische Scan-Ergebnisse überprüfen
2. **Monatliche Deep-Dive:** Manuelle Penetrationstests durchführen
3. **Quartalsweise:** Threat Model aktualisieren
4. **Bei Incidents:** Ad-hoc Analyse der betroffenen Bereiche

### Für DevOps

1. **CI/CD Integration:** Sicherstellen, dass Security-Scans bei jedem Build laufen
2. **Alerting konfigurieren:** Bei kritischen Findings sofort benachrichtigen
3. **Artefakte archivieren:** Scan-Ergebnisse für Audit-Zwecke aufbewahren
4. **Metrics tracken:** Security Metrics in Dashboards integrieren

## Troubleshooting

### Workflow-Fehler

```bash
# 1. Workflow-Logs anzeigen
gh run view <RUN_ID> --log

# 2. Spezifischen Job-Log anzeigen
gh run view <RUN_ID> --log --job=<JOB_ID>

# 3. Artefakte prüfen
gh run download <RUN_ID>
```

### False Positives

Bei False Positives:
1. Dokumentiere im entsprechenden Analyse-Dokument
2. Update Scanning-Rules in `.github/workflows/`
3. Füge Suppression-Comments im Code hinzu (wenn gerechtfertigt)

### Missing Artefakte

Falls Artefakte fehlen:
1. Prüfe ob Job erfolgreich war: `gh run view <RUN_ID>`
2. Prüfe Artifact Retention Policy (Standard: 90 Tage)
3. Re-run Workflow wenn nötig: `gh run rerun <RUN_ID>`

## Support

### Fragen oder Probleme

- **GitHub Issues:** [ThemisDB/issues](https://github.com/makr-code/ThemisDB/issues)
- **Security Disclosure:** Siehe `SECURITY.md` für Responsible Disclosure
- **Dokumentation:** `docs/de/security/`

### Weitere Ressourcen

- [Attack Vector Analysis Runbook](ANGRIFFSVEKTOREN_ANALYSE_RUNBOOK.md)
- [Basis-Analyse-Dokument](ANGRIFFSVEKTOREN_ANALYSE.md)
- [Threat Model](security_threat_model.md)
- [Security Policies](security_policies.md)
- [SECURITY.md](../../../SECURITY.md)

---

**Version History:**

| Version | Datum | Änderung | Autor |
|---------|-------|----------|-------|
| 1.0.0 | 2026-01-31 | Initial Release | Security Team |
