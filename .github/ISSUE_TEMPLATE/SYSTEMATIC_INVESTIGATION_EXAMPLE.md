# Systematic Security & Compliance Investigation - Example Usage

**Version:** 1.0.0  
**Created:** 2026-02-01  
**Template:** `security_compliance_systematic_investigation.md`

---

## Übersicht

Dieses Dokument zeigt Beispiele für die Verwendung des systematischen Sicherheits- und Compliance-Untersuchungs-Templates.

## 📋 Verwendungsszenarios

### Szenario 1: Quartalsweise Security Audits

**Ziel:** Regelmäßige, umfassende Sicherheitsüberprüfung der gesamten Codebasis

**Frequenz:** Quartalsweise (Q1, Q2, Q3, Q4)

**Vorgehen:**
1. Issue erstellen mit Template `security_compliance_systematic_investigation.md`
2. Title: `[Security/Compliance] Systematic Investigation: Q1 2026 Security Audit`
3. Alle 12 Analysekategorien durcharbeiten
4. Tools ausführen (CodeQL, clang-tidy, Trivy, etc.)
5. Findings dokumentieren und priorisieren
6. Remediation Plan erstellen
7. Fixes implementieren und validieren

**Beispiel-Command:**
```bash
gh issue create --template security_compliance_systematic_investigation.md \
  --title "[Security/Compliance] Systematic Investigation: Q1 2026 Security Audit" \
  --label "security,compliance,code-audit,systematic-investigation,quarterly,high" \
  --assignee "security-team"
```

**Erwartetes Ergebnis:**
- Vollständiger Audit-Report
- Liste aller Findings mit CVSS-Scores
- Prioritisierter Remediation Plan
- Updated Compliance-Status (BSI C5, ISO 27001, etc.)

---

### Szenario 2: Pre-Release Security Review

**Ziel:** Sicherheitsüberprüfung vor einem Major Release

**Trigger:** 2 Wochen vor geplantem Release

**Fokus-Bereiche:**
- Neue Features seit letztem Release
- Geänderte Security-kritische Komponenten
- Updated Dependencies
- Deployment-Konfigurationen

**Vorgehen:**
1. Issue erstellen mit Fokus auf neue Features
2. Title: `[Security/Compliance] Systematic Investigation: v1.5.0 Pre-Release Security Review`
3. Nur relevante Kategorien auswählen (z.B. neue AI-Features → Kategorie 12)
4. Quick Scan mit Tools
5. Manual Review von kritischen Änderungen
6. Pen-Test beauftragen (optional)

**Beispiel-Command:**
```bash
gh issue create --template security_compliance_systematic_investigation.md \
  --title "[Security/Compliance] Systematic Investigation: v1.5.0 Pre-Release Security Review" \
  --label "security,compliance,code-audit,systematic-investigation,release,critical" \
  --assignee "@me" \
  --milestone "v1.5.0"
```

**Erwartetes Ergebnis:**
- Security Sign-Off für Release
- Liste aller Release-Blocker
- Known Issues dokumentiert
- Security Release Notes

---

### Szenario 3: Compliance Audit (BSI C5, ISO 27001)

**Ziel:** Compliance-Status für Zertifizierung oder Audit prüfen

**Trigger:** 
- Vor Zertifizierungsaudit
- Nach größeren Architektur-Änderungen
- Jährliche Rezertifizierung

**Fokus:**
- Mapping zu Compliance-Anforderungen
- Gap-Analyse (FULL_AUDIT_CHECKLIST.md)
- Dokumentations-Vollständigkeit
- Evidence Collection

**Vorgehen:**
1. Issue erstellen mit Fokus auf Compliance-Framework
2. Title: `[Security/Compliance] Systematic Investigation: BSI C5 Compliance Audit 2026`
3. Alle relevanten Anforderungen durchgehen
4. Evidence sammeln (Logs, Configs, Docs)
5. Gap-Report erstellen
6. Remediation für alle Gaps planen

**Beispiel-Command:**
```bash
gh issue create --template security_compliance_systematic_investigation.md \
  --title "[Security/Compliance] Systematic Investigation: BSI C5 Compliance Audit 2026" \
  --label "security,compliance,code-audit,systematic-investigation,bsi-c5,high" \
  --assignee "compliance-team" \
  --project "Compliance-2026"
```

**Erwartetes Ergebnis:**
- Vollständiger Compliance-Report
- Evidence Package für Auditor
- Gap-Remediation Plan
- Updated Compliance-Dokumentation

---

### Szenario 4: Post-Incident Security Review

**Ziel:** Nach Security-Incident systematisch prüfen, was schief gelaufen ist

**Trigger:** Nach Security-Incident (Breach, Vulnerability Exploit, etc.)

**Fokus:**
- Root Cause Analysis
- Affected Components
- Similar Vulnerabilities in anderen Komponenten
- Prevention Measures

**Vorgehen:**
1. Issue erstellen mit Referenz auf Incident
2. Title: `[Security/Compliance] Systematic Investigation: Post-Incident Review (INC-2026-001)`
3. Fokus auf betroffene und ähnliche Komponenten
4. Code-Patterns suchen, die zu Incident geführt haben
5. Lessons Learned dokumentieren
6. Prevention Measures implementieren

**Beispiel-Command:**
```bash
gh issue create --template security_compliance_systematic_investigation.md \
  --title "[Security/Compliance] Systematic Investigation: Post-Incident Review (INC-2026-001)" \
  --label "security,compliance,code-audit,systematic-investigation,incident,critical" \
  --assignee "incident-response-team" \
  --body "Related Incident: #1234"
```

**Erwartetes Ergebnis:**
- Post-Incident Report
- Root Cause Analysis
- Similar Vulnerabilities Fixed
- Updated Security Measures
- Lessons Learned Documentation

---

### Szenario 5: Pre-Penetration Test Assessment

**Ziel:** Vor Pen-Test möglichst viele Schwachstellen selbst finden und fixen

**Trigger:** 4-6 Wochen vor geplantem Pen-Test

**Fokus:**
- OWASP Top 10
- Common Vulnerability Patterns
- Attack Surface Reduction
- Security Hardening

**Vorgehen:**
1. Issue erstellen mit Fokus auf Pen-Test-Vorbereitung
2. Title: `[Security/Compliance] Systematic Investigation: Pre-Pen-Test Assessment 2026`
3. Automated Scanning Tools ausführen
4. Manual Code Review für High-Risk-Bereiche
5. Findings priorisieren und fixen
6. Attack Surface dokumentieren für Pen-Tester

**Beispiel-Command:**
```bash
gh issue create --template security_compliance_systematic_investigation.md \
  --title "[Security/Compliance] Systematic Investigation: Pre-Pen-Test Assessment 2026" \
  --label "security,compliance,code-audit,systematic-investigation,pentest,high" \
  --assignee "security-team"
```

**Erwartetes Ergebnis:**
- Reduced Vulnerability Count
- Attack Surface Documentation
- Security Hardening Complete
- Better Pen-Test Results

---

### Szenario 6: Dependency Security Audit

**Ziel:** Systematische Überprüfung aller Dependencies auf Schwachstellen

**Trigger:**
- Quartalsweise
- Nach großen Dependency-Updates
- Vor Major Releases

**Fokus:**
- Known CVEs in Dependencies
- Outdated Packages
- License Compliance
- Supply Chain Security

**Vorgehen:**
1. Issue erstellen mit Fokus auf Kategorie 8 (Dependency Security)
2. Title: `[Security/Compliance] Systematic Investigation: Dependency Security Audit Q1 2026`
3. Trivy, OWASP Dependency-Check ausführen
4. SBOM aktualisieren
5. Vulnerable Dependencies updaten
6. License-Compliance prüfen

**Beispiel-Command:**
```bash
gh issue create --template security_compliance_systematic_investigation.md \
  --title "[Security/Compliance] Systematic Investigation: Dependency Security Audit Q1 2026" \
  --label "security,compliance,code-audit,systematic-investigation,dependencies,medium" \
  --assignee "devops-team"
```

**Erwartetes Ergebnis:**
- Updated Dependencies
- No Critical CVEs
- Updated SBOM
- License Compliance Confirmed

---

## 🛠️ Tool-Commands Cheat Sheet

### Static Analysis
```bash
# CodeQL Full Scan
codeql database create --language=cpp codeql-db
codeql database analyze codeql-db --format=sarif-latest --output=results.sarif

# clang-tidy
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON .
clang-tidy src/**/*.cpp -- -std=c++20

# cppcheck
cppcheck --enable=all --inconclusive --xml --xml-version=2 src/ include/ 2> cppcheck-report.xml

# Semgrep
semgrep --config=auto src/
```

### Dynamic Analysis
```bash
# AFL++ Fuzzing
cd fuzz
afl-fuzz -i input -o output -- ./aql_parser_fuzzer @@

# ASAN
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON .
make && ./tests/run_tests

# Valgrind
valgrind --leak-check=full --show-leak-kinds=all ./themisdb
```

### Security Scanning
```bash
# Trivy (Container + Dependencies)
trivy image themisdb/themisdb:latest
trivy fs --security-checks vuln,config .

# Gitleaks (Secret Detection)
gitleaks detect --source . --verbose

# OWASP ZAP (DAST)
docker run -v $(pwd):/zap/wrk/:rw -t ghcr.io/zaproxy/zaproxy:stable zap-baseline.py -t http://localhost:8080
```

### Compliance Checks
```bash
# Comprehensive Code Audit
./scripts/comprehensive-code-audit.sh

# SBOM Generation
syft . -o cyclonedx-json > sbom.json
```

---

## 📊 Timeline-Beispiel

### Quartalsweiser Security Audit (8 Wochen)

**Woche 1-2: Preparation & Automated Scanning**
- [ ] Issue erstellen
- [ ] Tools ausführen (CodeQL, Trivy, Gitleaks)
- [ ] Initial Findings sammeln
- [ ] Team briefen

**Woche 3-4: Manual Code Review**
- [ ] High-Risk-Bereiche reviewen (Auth, Crypto, Network)
- [ ] Code-Patterns analysieren
- [ ] Findings dokumentieren mit CVSS-Scores

**Woche 5-6: Remediation**
- [ ] Fixes implementieren (Critical/High)
- [ ] Tests schreiben
- [ ] Code Reviews

**Woche 7: Validation**
- [ ] Re-Scan mit Tools
- [ ] Regression Tests
- [ ] Security Tests

**Woche 8: Documentation & Reporting**
- [ ] Audit Report finalisieren
- [ ] Compliance-Dokumentation updaten
- [ ] Stakeholder Briefing
- [ ] Issue schließen

---

## 🎯 Best Practices

### 1. Regelmäßigkeit
- Quartalsweise Full Audits
- Monatliche Dependency Scans
- Wöchentliche Automated Scans in CI/CD

### 2. Priorisierung
- Focus auf High-Risk-Bereiche zuerst
- CVSS-Scores für Priorisierung nutzen
- Business Impact berücksichtigen

### 3. Automatisierung
- CI/CD Integration für kontinuierliches Scanning
- Automated Reporting
- Trend Analysis

### 4. Dokumentation
- Alle Findings dokumentieren
- Remediation tracked in Issues
- Lessons Learned festhalten

### 5. Team-Kommunikation
- Security Champions in jedem Team
- Regular Security Briefings
- Shared Knowledge Base

---

## 📚 Referenzen

### Interne Dokumente
- [Security Templates Guide](SECURITY_TEMPLATES_GUIDE.md)
- [SECURITY.md](../../SECURITY.md)
- [Full Audit Checklist](../../docs/de/compliance/compliance_full_checklist.md)
- [Compliance Audit TODO](../../docs/de/compliance/compliance_audit_todo.md)

### Externe Standards
- [BSI C5 Catalogue](https://www.bsi.bund.de/EN/Topics/CloudComputing/Compliance_Controls_Catalogue/)
- [ISO/IEC 27001:2022](https://www.iso.org/standard/27001)
- [OWASP Top 10](https://owasp.org/www-project-top-ten/)
- [CWE Top 25](https://cwe.mitre.org/top25/)

---

**Maintained by:** ThemisDB Security Team  
**Last Updated:** 2026-02-01  
**Version:** 1.0.0
