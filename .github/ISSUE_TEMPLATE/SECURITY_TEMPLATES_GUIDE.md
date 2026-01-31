# Security Attack Vector Issue Templates Guide

**Version:** 1.0.0  
**Category:** 🔒 Security  
**Status:** Active

---

## Übersicht

Diese Anleitung beschreibt die Verwendung der GitHub Issue Templates für Sicherheits-Angriffsvektoren in ThemisDB. Die Templates sind Teil des systematischen Angriffsvektoren-Analyse-Frameworks.

## 📋 Verfügbare Templates

### 1. 🌐 Network Attack Vector (`security_network_attack.md`)

**Verwendung:** Für alle netzwerkbasierten Angriffsvektoren

**Abgedeckte Vektoren (23 Typen):**
- HTTP/REST: Request Smuggling, Response Splitting, SSRF, CSRF, CORS, XXE, Path Traversal, Parameter Pollution
- WebSocket: Injection, Message Tampering, Connection Hijacking, DoS, Origin Bypass, Downgrade
- gRPC: Metadata Injection, Service Enumeration, Reflection Abuse, Stream Exhaustion
- MQTT: Topic Injection, Subscribe/Publish Abuse, QoS Manipulation
- PostgreSQL Wire: Protocol Injection, Wire Format Manipulation, Pool Exhaustion

**Wann verwenden:**
- Vulnerability in HTTP/REST API gefunden
- WebSocket-Sicherheitsproblem identifiziert
- gRPC-Protokoll-Schwachstelle entdeckt
- MQTT-Topic-Injection möglich
- PostgreSQL Wire Protocol-Problem

**Beispiel:**
```bash
# Issue erstellen für HTTP Request Smuggling
gh issue create --template security_network_attack.md \
  --title "[Security] Network Attack: HTTP Request Smuggling in REST API" \
  --label "security,attack-vector,network,critical"
```

---

### 2. 🔐 Authentication/Authorization Attack Vector (`security_authentication_attack.md`)

**Verwendung:** Für alle Authentifizierungs- und Autorisierungs-Angriffsvektoren

**Abgedeckte Vektoren (17 Typen):**
- Authentifizierung: Brute Force, Credential Stuffing, Session Fixation/Hijacking, JWT Manipulation, API Key Enumeration, OAuth/OIDC Vulnerabilities, MFA Bypass, Password Reset Poisoning
- Autorisierung: Privilege Escalation (Vertical/Horizontal), IDOR, Missing Function Level Access Control, Path Traversal via Authorization, Role/Policy Manipulation, Apache Ranger Misconfiguration, ABAC Policy Injection

**Wann verwenden:**
- JWT Token kann manipuliert werden
- Privilege Escalation möglich
- IDOR-Schwachstelle gefunden
- Session-Angriff möglich
- RBAC/ABAC-Policy kann umgangen werden
- API Key-Enumeration möglich

**Beispiel:**
```bash
# Issue erstellen für JWT Algorithm Confusion
gh issue create --template security_authentication_attack.md \
  --title "[Security] Auth Attack: JWT Algorithm Confusion allows signature bypass" \
  --label "security,attack-vector,authentication,high"
```

---

### 3. 💉 Injection Attack Vector (`security_injection_attack.md`)

**Verwendung:** Für alle Injection-Angriffsvektoren

**Abgedeckte Vektoren (14 Typen):**
- Query Injection: AQL, NoSQL, SQL (PostgreSQL Wire), GraphQL, XPath
- Code/Command Injection: OS Command, Template, Expression Language, LDAP, XML
- LLM-spezifisch: Prompt Injection, Model Poisoning, Training Data Extraction, Output Manipulation

**Wann verwenden:**
- AQL Injection möglich
- NoSQL Injection gefunden
- Command Injection identifiziert
- LLM Prompt Injection möglich
- Template Injection vorhanden

**Beispiel:**
```bash
# Issue erstellen für AQL Injection
gh issue create --template security_injection_attack.md \
  --title "[Security] Injection Attack: AQL Injection in FILTER clause" \
  --label "security,attack-vector,injection,critical"
```

**AFL++ Integration:**
Findings aus AFL++ Fuzzing können direkt in ein Issue übertragen werden:
```bash
# Nach AFL++ Crash
CRASH_FILE="fuzz/output/aql_parser/crashes/id:000000"
gh issue create --template security_injection_attack.md \
  --title "[Security] Injection Attack: AQL Parser Crash from AFL++ Fuzzing" \
  --body "Crash File: ${CRASH_FILE}"
```

---

### 4. 🔒 Cryptography Attack Vector (`security_crypto_attack.md`)

**Verwendung:** Für alle kryptographischen Angriffsvektoren

**Abgedeckte Vektoren (16 Typen):**
- Verschlüsselung: Weak Cipher Suites, ECB Mode, CBC Padding Oracle, IV Reuse, Key Derivation Weaknesses, Side-Channel Attacks, Downgrade Attacks, MITM
- Key Management: Hardcoded Keys, Weak Key Generation, Insecure Storage, Key Leakage, Insufficient Rotation, Unauthorized Access, HSM Bypass, Vault Misconfiguration

**Wann verwenden:**
- Schwache Cipher Suite erkannt
- Padding Oracle Attack möglich
- Key Management Problem identifiziert
- TLS/SSL Konfigurationsfehler
- HSM/Vault Misconfiguration
- Certificate Validation Issue

**Beispiel:**
```bash
# Issue erstellen für Weak Cipher Suite
gh issue create --template security_crypto_attack.md \
  --title "[Security] Crypto Attack: Weak cipher TLS_RSA_WITH_AES_128_CBC_SHA enabled" \
  --label "security,attack-vector,cryptography,medium"
```

**testssl.sh Integration:**
```bash
# Nach testssl.sh Scan
./testssl.sh --full https://localhost:8765 > tls-scan-results.txt
# Findings manuell in Issue übertragen
```

---

### 5. 🔄 Distributed System Attack Vector (`security_distributed_attack.md`)

**Verwendung:** Für alle verteilten System-Angriffsvektoren

**Abgedeckte Vektoren (18 Typen):**
- Consensus/Sharding: Shard Key Enumeration, Cross-Shard Injection, Distributed Transaction Manipulation, Consensus Protocol Attacks (Raft), Split-Brain, Network Partition Exploitation, Leader Election Manipulation, Replication Lag Exploitation, Clock Skew
- Data Integrity: MVCC Bypass, Write Skew Anomalies, Phantom Reads, Dirty Reads, Lost Updates, Stale Read Exploitation, Audit Log Tampering, Signature Verification Bypass, Snapshot Isolation Violation

**Wann verwenden:**
- Shard-Key kann enumeriert werden
- Cross-Shard Injection möglich
- Consensus-Protokoll kann angegriffen werden
- MVCC Bypass gefunden
- Data Integrity Violation möglich
- Audit Log kann manipuliert werden

**Beispiel:**
```bash
# Issue erstellen für MVCC Bypass
gh issue create --template security_distributed_attack.md \
  --title "[Security] Distributed Attack: MVCC Snapshot Isolation Bypass via Clock Skew" \
  --label "security,attack-vector,distributed-systems,high"
```

---

## 🚀 Verwendung

### Automatische Erstellung via Workflow

Die `attack-vector-analysis.yml` Workflow kann automatisch Issues erstellen:

```bash
# Workflow mit automatischer Issue-Erstellung starten
gh workflow run attack-vector-analysis.yml \
  -f analysis_scope=full \
  -f generate_report=true \
  -f create_issues=true  # Aktiviert automatische Issue-Erstellung
```

### Manuelle Erstellung via GitHub CLI

```bash
# 1. Liste verfügbare Templates
gh issue create --help

# 2. Wähle Template und erstelle Issue
gh issue create --template security_network_attack.md

# 3. Mit vorausgefüllten Werten
gh issue create \
  --template security_injection_attack.md \
  --title "[Security] Injection Attack: AQL Injection in FILTER" \
  --label "security,attack-vector,injection,critical" \
  --assignee "@me"
```

### Manuelle Erstellung via GitHub Web UI

1. Navigiere zu `Issues` → `New Issue`
2. Wähle eines der Security Attack Vector Templates
3. Fülle alle relevanten Felder aus
4. Verwende die Checklisten zur strukturierten Dokumentation
5. Füge Labels hinzu (wird automatisch vorausgefüllt)
6. Erstelle Issue

---

## 📊 Severity Levels

Alle Templates verwenden einheitliche Severity Levels:

| Severity | CVSS Score | SLA | Beispiel |
|----------|------------|-----|----------|
| **CRITICAL** | 9.0 - 10.0 | 24h | RCE, Authentication Bypass, Data Breach |
| **HIGH** | 7.0 - 8.9 | 7 Tage | SQL/AQL Injection, Privilege Escalation, Weak Crypto |
| **MEDIUM** | 4.0 - 6.9 | 30 Tage | XSS, CSRF, Information Disclosure |
| **LOW** | 0.1 - 3.9 | 90 Tage | Minor Config Issues, Low-impact Information Leaks |

---

## 🏷️ Labels

Standard-Labels für alle Security Issues:

### Erforderliche Labels
- `security` - Markiert als Security Issue
- `attack-vector` - Teil des Attack Vector Analysis Frameworks
- `[category]` - Eine der 5 Kategorien: `network`, `authentication`, `injection`, `cryptography`, `distributed-systems`
- `needs-triage` - Automatisch gesetzt, wird nach Triage entfernt

### Optionale Labels
- `critical` / `high` / `medium` / `low` - Severity Level
- `owasp-top-10` - OWASP Top 10 relevant
- `compliance-bsi-c5` - BSI C5 relevant
- `compliance-iso27001` - ISO 27001 relevant
- `fuzzing` - Via AFL++ gefunden
- `pentest` - Via Penetrationstest gefunden

---

## 📝 Best Practices

### 1. Vollständige Dokumentation
- Alle relevanten Felder ausfüllen
- Proof of Concept bereitstellen
- Reproduktionsschritte detailliert beschreiben
- Evidence anhängen (Screenshots, Logs, Payloads)

### 2. Severity Assessment
- CVSS Score berechnen
- Impact realistisch einschätzen
- Exploitability bewerten
- Compliance-Impact berücksichtigen

### 3. Remediation Planning
- Immediate Actions definieren (< 24h)
- Short-term Actions planen (< 1 Woche)
- Long-term Actions identifizieren (< 1 Monat)
- Code/Config Changes spezifizieren

### 4. Testing & Validation
- Test Cases für Regression definieren
- Validation Steps dokumentieren
- Fuzzing Tests hinzufügen (wo relevant)
- Security Test Scripts bereitstellen

### 5. Referenzen
- CWE-IDs angeben
- OWASP-Referenzen verlinken
- Related CVEs listen
- Interne Dokumentation verlinken

---

## 🔗 Integration mit Workflows

### Attack Vector Analysis Workflow
```yaml
# .github/workflows/attack-vector-analysis.yml
# Generiert automatisch Findings und kann Issues erstellen
```

**Workflow-Output:**
- Analyse-Artefakte für jede Kategorie
- Konsolidierter Bericht
- Compliance-Matrix
- Optional: Automatisch erstellte Issues

### Security Scan Workflow
```yaml
# .github/workflows/security-scan.yml
# OWASP ZAP, CodeQL, Trivy, Gitleaks, cppcheck
```

**Integration:**
- Findings aus OWASP ZAP → `security_network_attack.md`
- CodeQL Alerts → Passende Kategorie
- Trivy Findings → `security_crypto_attack.md` (für schwache Crypto)

### Fuzzing Workflow
```yaml
# .github/workflows/fuzzing.yml
# AFL++ Fuzzing für Injection-Vektoren
```

**Integration:**
- AFL++ Crashes → `security_injection_attack.md`
- Crash-Reports direkt in Issue übertragen
- CASR-Analyse hinzufügen

---

## 📚 Referenzen

### Interne Dokumentation
- [Attack Vector Analysis Runbook](../../docs/de/security/ANGRIFFSVEKTOREN_ANALYSE_RUNBOOK.md)
- [Framework Documentation](../../docs/de/security/ATTACK_VECTOR_ANALYSIS_FRAMEWORK.md)
- [Threat Model](../../docs/de/security/security_threat_model.md)

### Workflows
- `.github/workflows/attack-vector-analysis.yml` - Hauptworkflow
- `.github/workflows/security-scan.yml` - Security Scans
- `.github/workflows/fuzzing.yml` - Fuzzing Tests

### Externe Standards
- [OWASP Top 10](https://owasp.org/www-project-top-ten/)
- [OWASP ASVS](https://owasp.org/www-project-application-security-verification-standard/)
- [CWE Top 25](https://cwe.mitre.org/top25/)
- [BSI C5](https://www.bsi.bund.de/EN/Topics/CloudComputing/Compliance_Controls_Catalogue/)

---

## 🆘 Support

Bei Fragen zu den Templates:
- **GitHub Issues:** [ThemisDB/issues](https://github.com/makr-code/ThemisDB/issues)
- **Security Disclosure:** Siehe `SECURITY.md`
- **Dokumentation:** `docs/de/security/`

---

**Version History:**

| Version | Datum | Änderung | Autor |
|---------|-------|----------|-------|
| 1.0.0 | 2026-01-31 | Initial Release | Security Team |
