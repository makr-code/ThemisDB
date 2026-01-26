# ThemisDB Audit Framework - Quick Start Guide

**Version:** 1.0  
**Date:** January 2026  
**For:** ThemisDB v1.4.1+

---

## 📋 What is the Audit Framework?

The ThemisDB Audit Framework is a comprehensive, repeatable system for conducting security and compliance audits. It ensures ThemisDB maintains best-practice conformance across multiple international standards while supporting automated audit gates in the CI/CD pipeline.

## 🎯 Key Standards Covered

- ✅ **ISO 27001:2022** - Information Security Management (95.4% compliant)
- ✅ **NIST CSF v1.1** - Cybersecurity Framework (Tier 3)
- ✅ **OWASP ASVS v4.0** - Application Security (Level 2)
- ✅ **BSI C5** - Cloud Computing Compliance (98.9% compliant)
- ✅ **SOC 2 Type II** - Trust Services Criteria (98.1% compliant)
- ✅ **SLSA Level 3** - Supply Chain Security (100% achieved ⭐)

**Aggregate Compliance: 95.3%** across all standards (428 controls assessed)

---

## 📚 Documentation Structure

### Core Documents

```
docs/audit-framework/
├── audit_charter_planning.md      # Framework governance & methodology
├── AUDIT_GATE_TEMPLATE.md         # 113-point audit checklist
├── AUDIT_RUNBOOK.md               # Step-by-step execution guide
└── COMPLIANCE_MAPPING.md          # 400+ controls mapped to features
```

#### 1. Audit Charter & Planning (20KB)
**Purpose:** Establishes audit governance, objectives, and methodology

**Contents:**
- Audit objectives and scope
- 6 primary + 8 supporting standards
- Team roles and responsibilities
- Risk assessment framework
- KPIs and continuous improvement

**When to use:** Setup new audit program, annual review, team onboarding

#### 2. Audit Gate Template (35KB)
**Purpose:** Master checklist for conducting release audits

**Contents:**
- **13 audit dimensions** with 113 checkpoints
- Evidence tracking structure
- Findings management workflow
- Risk assessment matrix
- Sign-off and approval gates

**When to use:** Every release, quarterly audits, compliance verification

**13 Audit Dimensions:**
1. Governance & Planning
2. Risk Assessment (NIST CSF)
3. Security Controls & Implementation
4. Compliance & Standards Mapping
5. Code Quality & Static Analysis (SAST)
6. Testing & Quality Assurance
7. Performance & Reliability
8. Documentation & Evidence
9. Deployment & Infrastructure Hardening
10. Findings Management & Root Cause Analysis
11. Reporting & Communication
12. Sign-Off & Approval
13. Post-Audit & Continuous Improvement

#### 3. Audit Runbook (43KB)
**Purpose:** Detailed step-by-step guide for audit execution

**Contents:**
- 8-phase audit process
- Tool integration guide (10+ security tools)
- Command-line examples and scripts
- Report templates (executive & detailed)
- 20+ KPIs and metrics definitions
- Troubleshooting guide

**When to use:** Executing audits, training new auditors, tool setup

**8 Audit Phases:**
1. Planning and Preparation
2. Automated Security Scanning (SAST/DAST)
3. Testing and Quality Assessment
4. Compliance Verification
5. Manual Security Review
6. Findings Analysis
7. Remediation
8. Reporting and Sign-Off

#### 4. Compliance Mapping Matrix (64KB)
**Purpose:** Maps ThemisDB features to standards requirements

**Contents:**
- **428 controls** across 6 standards
- **692 detailed tables** with status
- Evidence paths to code/docs
- Coverage heat maps
- Gap analysis (Zero P0 gaps!)
- Remediation roadmap

**When to use:** Compliance verification, external audits, certification prep

---

## 🤖 Automated Workflows

### 1. Audit Check Workflow (`.github/workflows/audit-check.yml`)

**Triggers:**
- Every pull request
- Release tags (v*.*.*)
- Manual dispatch

**Jobs:**
1. **SAST** - Static analysis (cppcheck, clang-tidy, Gitleaks)
2. **Dependency Scan** - Vulnerability scanning
3. **Test Coverage** - Code coverage analysis (>80% target)
4. **Container Scan** - Docker image security (Trivy)
5. **DAST** - Dynamic analysis (OWASP ZAP) [on release]
6. **Compliance Check** - Verify documentation
7. **Audit Report** - Generate summary
8. **Audit Gate** - Go/No-Go decision

**Output:**
- Comprehensive audit report
- PR comment with results
- Artifacts for 90 days
- Audit gate decision

### 2. Audit Gate Issue Workflow (`.github/workflows/audit-gate-issue.yml`)

**Triggers:**
- Release tags (v*.*.*)
- Manual dispatch

**Purpose:**
- Automatically creates audit tracking issues
- Pre-fills version and date information
- Labels and assigns to audit team

---

## 🚀 Quick Start Guide

### For Developers: Running Audit Checks

#### On Pull Request
Audit checks run automatically on every PR. Review the results in:
1. PR comment (audit summary)
2. Workflow run details (full logs)
3. Artifacts (detailed reports)

#### Locally (Manual)
```bash
# 1. Run static analysis
cppcheck --enable=all src/ include/

# 2. Run clang-tidy
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B build
clang-tidy -p build src/**/*.cpp

# 3. Run tests with coverage
cmake -DCMAKE_CXX_FLAGS="--coverage" -B build-coverage
cmake --build build-coverage
cd build-coverage && ctest
gcovr -r .. --html-details coverage.html

# 4. Scan for secrets
docker run --rm -v $(pwd):/repo zricethezav/gitleaks:latest detect --source /repo
```

### For Release Managers: Conducting Release Audits

#### Pre-Release Checklist
1. **T-14 days:** Review audit charter, define scope
2. **T-10 days:** Trigger audit-check workflow
3. **T-7 days:** Review automated results
4. **T-5 days:** Execute manual security review
5. **T-3 days:** Document findings, begin remediation
6. **T-1 day:** Verify all P0/P1 findings resolved
7. **T-day:** Obtain sign-offs, approve release

#### Step-by-Step Process
```bash
# 1. Create audit tracking issue (automatic on tag push)
git tag -a v1.4.1 -m "Release v1.4.1"
git push origin v1.4.1
# → Creates audit issue automatically

# 2. Copy audit template for this release
cp docs/audit-framework/AUDIT_GATE_TEMPLATE.md \
   docs/audit-framework/evidence/v1.4.1/audit-checklist.md

# 3. Set up evidence directory
mkdir -p docs/audit-framework/evidence/v1.4.1/{scans,test-results,compliance,findings,reports}

# 4. Execute audit steps (see AUDIT_RUNBOOK.md)
# Follow 8-phase process...

# 5. Generate audit report
# Collect evidence from workflow artifacts
# Document findings in register
# Create executive summary

# 6. Obtain sign-offs
# Lead Auditor → Security Team → Compliance Officer → Release Manager
```

### For Auditors: Using the Framework

#### Initial Setup
```bash
# 1. Clone repository
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# 2. Review audit charter
cat docs/audit-framework/audit_charter_planning.md

# 3. Install audit tools
sudo apt-get install cppcheck clang-tidy
docker pull owasp/zap2docker-stable
docker pull aquasec/trivy:latest

# 4. Set up evidence repository
mkdir -p docs/audit-framework/evidence/v${VERSION}
```

#### Conducting an Audit
Follow the **AUDIT_RUNBOOK.md** for detailed instructions:

1. **Planning** (Day -7 to -1)
   - Review previous audit
   - Define scope
   - Update charter
   - Prepare checklist

2. **Execution** (Day 1-8)
   - Run automated scans
   - Execute manual reviews
   - Verify compliance
   - Document findings

3. **Reporting** (Day 9-10)
   - Generate reports
   - Present to stakeholders
   - Obtain approvals

4. **Follow-up** (Ongoing)
   - Track remediation
   - Monitor metrics
   - Update framework

---

## 📊 Key Metrics & KPIs

### Security Metrics
- **MTTD (Mean Time to Detect):** < 24 hours
- **MTTR (Mean Time to Remediate):**
  - Critical (P0): < 7 days
  - High (P1): < 30 days
- **Vulnerability Density:** < 1 per 1000 LOC
- **Security Test Coverage:** > 80%
- **Critical Findings per Release:** < 5

### Quality Metrics
- **Code Coverage:** > 80%
- **SAST Pass Rate:** > 95%
- **Build Success Rate:** > 99%
- **Test Success Rate:** 100%

### Compliance Metrics
- **Standard Compliance Rate:** > 95%
- **Control Effectiveness:** > 85%
- **Evidence Completeness:** > 95%

---

## 🔍 Common Use Cases

### Use Case 1: Pre-Release Audit
**Scenario:** You're preparing for v1.4.1 release

**Steps:**
1. Push release tag → Automatic audit issue created
2. Review automated audit-check workflow results
3. Use AUDIT_GATE_TEMPLATE.md as checklist
4. Document findings in issue
5. Obtain sign-offs
6. Approve release

### Use Case 2: Quarterly Compliance Review
**Scenario:** Quarterly audit for ISO 27001/SOC 2

**Steps:**
1. Use COMPLIANCE_MAPPING.md to review all controls
2. Collect evidence per Annex A requirements
3. Update compliance status in mapping matrix
4. Document gaps and remediation plans
5. Generate compliance report
6. Present to management

### Use Case 3: External Certification Prep
**Scenario:** Preparing for ISO 27001 certification audit

**Steps:**
1. Review COMPLIANCE_MAPPING.md ISO 27001 section
2. Collect evidence for all 93 controls
3. Organize evidence repository
4. Document control effectiveness
5. Prepare audit evidence packages
6. Schedule external auditor

### Use Case 4: Security Incident Investigation
**Scenario:** Security issue found in production

**Steps:**
1. Use risk assessment section of runbook
2. Perform targeted security audit of affected component
3. Document findings and root cause
4. Update risk register
5. Implement remediation
6. Verify fix through re-audit

---

## 🛠️ Tool Reference

### Static Analysis
- **cppcheck** - C++ static analyzer
- **clang-tidy** - Clang-based linter
- **Gitleaks** - Secret scanning

### Dynamic Analysis
- **OWASP ZAP** - Web application scanner
- **ASAN/MSAN/UBSAN** - Memory sanitizers

### Container Security
- **Trivy** - Container vulnerability scanner
- **Hadolint** - Dockerfile linter

### Coverage & Testing
- **gcovr** - Code coverage reporting
- **CTest** - Test runner

### Performance
- **Apache Bench** - HTTP load testing
- **Custom benchmarks** - Database-specific tests

---

## 📖 Additional Resources

### Internal Documentation
- [Security Policy](../../SECURITY.md)
- [Compliance Documentation](../de/compliance/)
- [Build Guide](../de/guides/guides_build_strategy.md)
- [Architecture Docs](../de/architecture/)

### External Standards
- [ISO 27001:2022](https://www.iso.org/standard/27001)
- [NIST Cybersecurity Framework](https://www.nist.gov/cyberframework)
- [OWASP ASVS](https://owasp.org/www-project-application-security-verification-standard/)
- [BSI C5](https://www.bsi.bund.de/EN/Topics/CloudComputing/Compliance_Criteria_Catalogue/Compliance_Criteria_Catalogue_node.html)
- [SOC 2](https://www.aicpa.org/interestareas/frc/assuranceadvisoryservices/aicpasoc2report.html)
- [SLSA](https://slsa.dev/)

### Training & Best Practices
- [NIST SSDF](https://csrc.nist.gov/Projects/ssdf)
- [OWASP Top 10](https://owasp.org/Top10/)
- [CWE Top 25](https://cwe.mitre.org/top25/)
- [IIA Standards](https://www.theiia.org/en/standards/)

---

## 🤝 Support & Contact

### Questions?
- **Documentation:** Read the detailed runbook and charter
- **GitHub Issues:** Open an issue with label `audit-framework`
- **Email:** security-audit@themisdb.org

### Contributing
Improvements to the audit framework are welcome!
1. Open an issue to discuss proposed changes
2. Submit PR with updates to framework documents
3. Ensure changes align with standards
4. Update this quick start guide if needed

---

## 📝 Document Updates

This quick start guide is maintained alongside the audit framework documents. Last updated: January 2026.

**Version History:**
- v1.0 (January 2026) - Initial release for ThemisDB v1.4.1+

---

**Ready to get started?** 
1. Read the [Audit Charter](audit_charter_planning.md) for overview
2. Review the [Audit Runbook](AUDIT_RUNBOOK.md) for step-by-step guide
3. Use the [Audit Gate Template](AUDIT_GATE_TEMPLATE.md) for your first audit
4. Check [Compliance Mapping](COMPLIANCE_MAPPING.md) for standards alignment

**Questions?** Open an issue or contact the security team.

**Happy Auditing! 🔍**
