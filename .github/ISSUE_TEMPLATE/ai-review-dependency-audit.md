---
name: 📦 AI Review - Dependency Audit & License Compliance
about: Systematische Abhängigkeits- und Lizenz-Compliance-Prüfung / Systematic dependency and license compliance audit
title: '[DEPENDENCY-AUDIT] '
labels: ['type:systematic-review', 'area:dependencies', 'area:security', 'area:compliance', 'needs-triage']
assignees: ''
---

<!-- 
Wiederholbare Template für Dependency Audit & License Compliance Reviews
Repeatable template for dependency audit and license compliance reviews
Empfohlene Häufigkeit: Quartalsweise oder vor Releases / Recommended: Quarterly or before releases
-->

## 🎯 Scope / Umfang

**Review Scope:** <!-- z.B. All dependencies, Production only, Specific module -->
**Review Period:** <!-- z.B. Q1 2026, Version 1.4.x -->
**Reviewer(s):** <!-- Namen der Reviewer -->
**Previous Review:** <!-- Datum des letzten Reviews -->

---

## 📊 Dependency Inventory / Abhängigkeits-Inventar

### Direct Dependencies / Direkte Abhängigkeiten
- **Total Direct Dependencies:** 
- **Production Dependencies:** 
- **Development Dependencies:** 
- **Optional Dependencies:** 

### Transitive Dependencies / Transitive Abhängigkeiten
- **Total Transitive Dependencies:** 
- **Dependency Tree Depth:** <!-- Max depth -->
- **Largest Dependency Chain:** 

### By Ecosystem / Nach Ökosystem
- **C++ Libraries:** <!-- vcpkg, conan -->
- **npm Packages:** 
- **Python Packages:** <!-- pip -->
- **Go Modules:** 
- **Maven/Gradle:** <!-- Java -->
- **Other:** 

---

## 🔒 Security Vulnerabilities / Sicherheits-Schwachstellen

### CVE Analysis / CVE-Analyse
- **Critical Vulnerabilities:** <!-- CVSS >= 9.0 -->
- **High Severity:** <!-- CVSS 7.0-8.9 -->
- **Medium Severity:** <!-- CVSS 4.0-6.9 -->
- **Low Severity:** <!-- CVSS 0.1-3.9 -->

### Vulnerability Details / Schwachstellen-Details
**Critical CVEs:**
1. **CVE-YYYY-XXXXX:**
   - Dependency: 
   - Current Version: 
   - Fixed Version: 
   - Impact: 
   - Mitigation Plan: 
   - Timeline: 

2. **CVE-YYYY-XXXXX:**
   - Dependency: 
   - Current Version: 
   - Fixed Version: 
   - Impact: 
   - Mitigation Plan: 
   - Timeline: 

### Security Scanning Tools / Sicherheits-Scan-Tools
- [ ] **GitHub Dependabot** enabled
- [ ] **Snyk** scanning
- [ ] **npm audit** / **pip-audit** run
- [ ] **OWASP Dependency Check** run
- [ ] **Trivy** container scanning
- [ ] **Grype** vulnerability scanning

**Last Scan Date:** 
**Scan Results:** <!-- Link to report -->

---

## 📜 License Compliance / Lizenz-Compliance

### License Inventory / Lizenz-Inventar
- **Total Unique Licenses:** 
- **Permissive Licenses:** <!-- MIT, Apache 2.0, BSD -->
- **Weak Copyleft:** <!-- LGPL, MPL -->
- **Strong Copyleft:** <!-- GPL, AGPL -->
- **Proprietary/Commercial:** 
- **Unknown/Unlicensed:** 

### License Distribution / Lizenz-Verteilung
| License | Count | % | Compliant? |
|---------|-------|---|------------|
| MIT | | | ✅/⚠️/❌ |
| Apache 2.0 | | | ✅/⚠️/❌ |
| BSD | | | ✅/⚠️/❌ |
| GPL | | | ✅/⚠️/❌ |
| LGPL | | | ✅/⚠️/❌ |
| Other | | | ✅/⚠️/❌ |

### License Compliance Issues / Lizenz-Compliance-Probleme
- [ ] **License conflicts** detected
- [ ] **Copyleft requirements** unmet
- [ ] **Attribution** missing
- [ ] **License file** missing in distribution
- [ ] **Dual licensing** conflicts

**Compliance Issues:**
1. **Issue 1:**
   - Dependency: 
   - License: 
   - Problem: 
   - Resolution: 

2. **Issue 2:**
   - Dependency: 
   - License: 
   - Problem: 
   - Resolution: 

### ThemisDB License Compatibility / ThemisDB-Lizenz-Kompatibilität
- **ThemisDB License:** <!-- MIT, Apache 2.0, GPL, etc. -->
- **Compatible Dependencies:** <!-- % -->
- **Incompatible Dependencies:** <!-- Count -->

**Incompatible Dependencies:**
1. 
2. 
3. 

---

## 📈 Dependency Health / Abhängigkeits-Gesundheit

### Version Freshness / Versions-Aktualität
- **Up-to-date Dependencies:** <!-- % -->
- **Minor Updates Available:** 
- **Major Updates Available:** 
- **Dependencies > 2 years old:** 
- **Deprecated Dependencies:** 

**Outdated Dependencies:**
| Dependency | Current | Latest | Age | Risk |
|------------|---------|--------|-----|------|
| | | | | High/Medium/Low |
| | | | | |
| | | | | |

### Maintenance Status / Wartungs-Status
- [ ] **Actively maintained** dependencies
- [ ] **Archived/unmaintained** dependencies
- [ ] **Abandoned** dependencies (no commits > 2 years)
- [ ] **Single maintainer** risk
- [ ] **Lack of community** support

**At-Risk Dependencies:**
1. **Dependency:**
   - Status: 
   - Last Update: 
   - Alternative: 
   - Migration Plan: 

---

## 🔄 Dependency Updates / Abhängigkeits-Updates

### Update Strategy / Update-Strategie
- [ ] **Automated updates** (Dependabot, Renovate)
- [ ] **Manual update** process
- [ ] **Update policy** defined
- [ ] **Breaking change** testing
- [ ] **Rollback plan** in place

### Pending Updates / Ausstehende Updates
**High Priority Updates:**
1. **Dependency:**
   - Current: 
   - Target: 
   - Reason: <!-- Security fix, bug fix, new features -->
   - Effort: <!-- Hours/Days -->
   - Breaking Changes: <!-- Yes/No -->

2. **Dependency:**
   - Current: 
   - Target: 
   - Reason: 
   - Effort: 
   - Breaking Changes: 

### Update Risks / Update-Risiken
- **Breaking Changes:** <!-- Count -->
- **API Changes:** <!-- Count -->
- **Performance Impact:** <!-- Positive/Negative/None -->
- **Compatibility Issues:** <!-- Count -->

---

## 🗑️ Unused Dependencies / Ungenutzte Abhängigkeiten

### Dependency Usage Analysis / Abhängigkeits-Nutzungs-Analyse
- **Unused Dependencies:** <!-- Count -->
- **Rarely Used Dependencies:** <!-- < 5 imports -->
- **Potential for Removal:** <!-- Count -->

**Candidates for Removal:**
1. **Dependency:**
   - Usage: <!-- Never/Rarely used -->
   - Reason for Inclusion: 
   - Removal Risk: <!-- High/Medium/Low -->

2. **Dependency:**
   - Usage: 
   - Reason for Inclusion: 
   - Removal Risk: 

---

## 📦 Dependency Size & Performance / Größe & Performance

### Size Analysis / Größen-Analyse
- **Total Dependency Size:** <!-- MB/GB -->
- **Largest Dependency:** <!-- Name, size -->
- **Dependencies > 10 MB:** <!-- Count -->

**Large Dependencies:**
| Dependency | Size | Essential? | Alternative? |
|------------|------|------------|--------------|
| | | Yes/No | |
| | | | |

### Build Time Impact / Build-Zeit-Impact
- **Total Build Time:** 
- **Dependency Installation Time:** 
- **Slowest Dependency:** 

---

## 🔐 Supply Chain Security / Supply-Chain-Sicherheit

### Supply Chain Risks / Supply-Chain-Risiken
- [ ] **Compromised packages** in history
- [ ] **Typosquatting** risks assessed
- [ ] **Dependency confusion** risks
- [ ] **Malicious packages** checked
- [ ] **Package integrity** verified (checksums, signatures)

### Trusted Sources / Vertrauenswürdige Quellen
- [ ] **Official repositories** only (npm, PyPI, etc.)
- [ ] **Private registry** for internal packages
- [ ] **Package verification** enabled
- [ ] **Reproducible builds**

**Supply Chain Issues:**


---

## 📋 Dependency Documentation / Abhängigkeits-Dokumentation

### Documentation Status / Dokumentations-Status
- [ ] **Dependency list** maintained
- [ ] **License attribution** file
- [ ] **Third-party notices** generated
- [ ] **Dependency rationale** documented
- [ ] **Update procedures** documented

**Documentation Gaps:**


---

## 🎯 Dependency Management Best Practices / Best Practices

### Current Practices / Aktuelle Praktiken
- [ ] **Lock files** used (package-lock.json, Cargo.lock)
- [ ] **Dependency ranges** appropriate (not too loose)
- [ ] **Dependency pinning** for critical deps
- [ ] **Automated scanning** in CI/CD
- [ ] **Security alerts** monitored
- [ ] **Update schedule** followed

**Best Practices Not Followed:**
1. 
2. 
3. 

---

## 🗺️ Roadmap / Roadmap

### Short-Term (Next 3 Months)
- [ ] Address critical security vulnerabilities
- [ ] Update outdated dependencies
- [ ] Remove unused dependencies
- [ ] 

### Medium-Term (3-6 Months)
- [ ] Migrate from deprecated dependencies
- [ ] Resolve license compliance issues
- [ ] Implement automated dependency updates
- [ ] 

### Long-Term (6-12 Months)
- [ ] Reduce dependency footprint
- [ ] Improve supply chain security
- [ ] Establish dependency policies
- [ ] 

---

## ✅ Action Items / Aktionspunkte

### Critical (P0) - Security Issues
1. [ ] **CVE-YYYY-XXXXX:**
   - Owner: 
   - Due Date: 
   - Fix: Update to version X.Y.Z

### High Priority (P1)
1. [ ] **Action 1:**
   - Owner: 
   - Due Date: 
   - Description: 

2. [ ] **Action 2:**
   - Owner: 
   - Due Date: 
   - Description: 

### Medium Priority (P2)
1. [ ] **Action 1:**
   - Owner: 
   - Due Date: 
   - Description: 

---

## 📚 References / Referenzen

### Internal Documentation
- [Dependency Management Policy](docs/policies/dependency-management.md)
- [Security Guidelines](docs/security/)
- [License Compliance](docs/legal/license-compliance.md)

### External Resources
- [OWASP Dependency Check](https://owasp.org/www-project-dependency-check/)
- [Snyk Vulnerability Database](https://snyk.io/vuln/)
- [CVE Database](https://cve.mitre.org/)
- [SPDX License List](https://spdx.org/licenses/)
- [choosealicense.com](https://choosealicense.com/)

### Tools
- [Dependabot](https://github.com/dependabot)
- [Renovate](https://renovatebot.com/)
- [npm audit](https://docs.npmjs.com/cli/v8/commands/npm-audit)
- [pip-audit](https://github.com/pypa/pip-audit)
- [Trivy](https://aquasecurity.github.io/trivy/)

---

## 📋 Review Checklist / Review-Checkliste

- [ ] Complete dependency inventory collected
- [ ] Security vulnerabilities scanned
- [ ] CVEs assessed and prioritized
- [ ] License compliance verified
- [ ] License conflicts resolved
- [ ] Dependency health evaluated
- [ ] Outdated dependencies identified
- [ ] Unused dependencies found
- [ ] Update plan created
- [ ] Supply chain risks assessed
- [ ] Documentation updated
- [ ] Action items created and assigned
- [ ] Sign-offs obtained from security and legal teams

---

**Review Date:** <!-- YYYY-MM-DD -->
**Next Review:** <!-- YYYY-MM-DD (empfohlen: +3 Monate oder vor Release) -->
**Sign-Off:** <!-- Security Team, Legal/Compliance Team, Development Lead -->

---

**Template Version:** 1.0.0  
**Created:** 2026-02-02  
**Maintained by:** ThemisDB Security Team
