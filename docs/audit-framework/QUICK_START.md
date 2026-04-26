# Audit Framework - Developer Quick Start Guide

**For:** Developers, Contributors, Release Managers  
**Time to Complete:** 10 minutes  
**Prerequisites:** Git access, basic Docker knowledge

---

## 🎯 What is This?

The ThemisDB Audit Framework ensures every release meets security and compliance standards. As a developer, you'll interact with this framework when:
- Creating pull requests
- Preparing releases
- Addressing security findings
- Contributing new features

---

## 🚀 Quick Start for Developers

### Your PR Gets Automatically Audited

When you create a PR, the `audit-check` workflow automatically runs:

```
✓ SAST (Static Analysis) - cppcheck, clang-tidy
✓ Secret Scanning - gitleaks
✓ Test Coverage - gcov
✓ Dependency Scan - GitHub Security Advisories
```

**What You Need to Do:**
1. Wait for checks to complete (~10-15 minutes)
2. Review results in PR comments
3. Fix any findings marked as 🔴 CRITICAL or 🟠 HIGH
4. Re-push to trigger re-scan

### Running Audits Locally

Before pushing, run checks locally to catch issues early:

```bash
# 1. Static Analysis (quick, ~2 minutes)
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B build
clang-tidy -p build src/your_file.cpp

# 2. Secret Scanning (quick, ~30 seconds)
docker run --rm -v $(pwd):/repo zricethezav/gitleaks:latest detect --source /repo

# 3. Code Coverage (slower, ~10 minutes)
cmake -DCMAKE_CXX_FLAGS="--coverage" -B build-coverage
cmake --build build-coverage
cd build-coverage && ctest
gcovr -r .. --html coverage.html
```

### Understanding Findings

Findings are categorized by severity:

| Severity | Must Fix? | Timeframe | Example |
|----------|-----------|-----------|---------|
| 🔴 CRITICAL | ✅ Yes (before merge) | Immediate | SQL injection, hardcoded keys |
| 🟠 HIGH | ✅ Yes (before release) | 1 week | Missing encryption, weak auth |
| 🟡 MEDIUM | ⚠️ Maybe | 2 weeks | Missing tests, code complexity |
| 🟢 LOW | ❌ Nice to have | 1 month | Code style, minor optimizations |

### Fixing Findings

**Step 1:** Understand the finding
```bash
# Read the detailed report
cat build/.audit-reports/clang-tidy-output.txt | grep "your_file.cpp"
```

**Step 2:** Fix the issue
```cpp
// ❌ Before (insecure)
std::string password = "hardcoded123";

// ✅ After (secure)
std::string password = getenv("DB_PASSWORD");
```

**Step 3:** Verify the fix
```bash
# Re-run the specific check
clang-tidy -p build src/your_file.cpp
```

**Step 4:** Push and verify CI passes
```bash
git add .
git commit -m "fix: remove hardcoded password (FIND-001)"
git push
```

---

## 📋 For Release Managers

### Pre-Release Checklist (T-14 days)

```bash
# 1. Create release branch
git checkout -b release/v1.4.1

# 2. Update version
echo "1.4.1" > VERSION

# 3. Update changelog
vim CHANGELOG.md

# 4. Push release tag (triggers audit issue creation)
git tag -a v1.4.1 -m "Release v1.4.1"
git push origin v1.4.1
```

This automatically:
- ✅ Creates audit tracking issue
- ✅ Runs comprehensive audit-check workflow
- ✅ Generates reports in build/.audit-reports/

### During Audit (T-7 to T-1 days)

**Monitor audit progress:**
```bash
# Check audit issue
https://github.com/makr-code/ThemisDB/issues?q=is%3Aissue+label%3Aaudit-gate

# Review findings
cat docs/audit-framework/evidence/v1.4.1/findings/findings-register.md
```

**Address findings:**
1. Assign critical findings to developers
2. Track progress in audit issue
3. Verify fixes with re-scans
4. Document decisions (fix or accept risk)

### Final Sign-Off (T-day)

**Requirements:**
- [ ] All CRITICAL findings resolved
- [ ] All HIGH findings resolved or risk accepted
- [ ] Test coverage ≥ 80%
- [ ] Documentation updated
- [ ] Changelog complete

**Get approvals:**
```bash
# Sign-off checklist in audit issue:
- [ ] Audit Lead approval
- [ ] Security Team approval
- [ ] Compliance Officer approval
- [ ] Release Manager approval
```

**Release:**
```bash
git checkout main
git merge release/v1.4.1
git push origin main
```

---

## 🔍 Common Scenarios

### Scenario 1: "My PR has a CRITICAL finding"

**Problem:** CI found hardcoded credentials

**Solution:**
```bash
# 1. Check the finding
grep -r "password\|secret\|key" src/

# 2. Move to environment variable
# Before: string pwd = "secret123";
# After:  string pwd = getenv("DB_PASSWORD");

# 3. Update .gitignore
echo "config/secrets.env" >> .gitignore

# 4. Re-run gitleaks
docker run --rm -v $(pwd):/repo zricethezav/gitleaks:latest detect --source /repo

# 5. Push fix
git add .
git commit -m "fix(security): remove hardcoded credentials [CRITICAL]"
git push
```

### Scenario 2: "Test coverage dropped below 80%"

**Problem:** New feature lacks tests

**Solution:**
```bash
# 1. Identify uncovered code
gcovr -r . --html coverage.html
open coverage.html  # View red areas

# 2. Add tests
# Create tests/test_new_feature.cpp

# 3. Verify coverage improved
cmake --build build-coverage
cd build-coverage && ctest
gcovr -r .. --txt

# 4. Push with tests
git add tests/test_new_feature.cpp
git commit -m "test: add coverage for new feature"
git push
```

### Scenario 3: "OWASP ZAP found vulnerabilities"

**Problem:** API endpoint vulnerable to injection

**Solution:**
```cpp
// 1. Review finding
// "SQL Injection in /api/query endpoint"

// 2. Add input validation
bool validateQuery(const string& query) {
    // Whitelist allowed characters
    regex safe_pattern("^[a-zA-Z0-9 _,().]+$");
    return regex_match(query, safe_pattern);
}

// 3. Use parameterized queries
PreparedStatement stmt = conn.prepareStatement(
    "SELECT * FROM users WHERE id = ?"
);
stmt.setInt(1, userId);

// 4. Test with attack vectors
// Test: ?query=1' OR '1'='1
// Expected: Rejected by validation

// 5. Document fix in finding register
```

---

## 🛠️ Useful Commands

### Quick Health Check
```bash
# Run all local checks
./scripts/run-local-audit.sh  # (if created)

# Or manually:
cppcheck --enable=all src/ include/ 2>&1 | tee audit-local.txt
docker run --rm -v $(pwd):/repo zricethezav/gitleaks:latest detect --source /repo
```

### View Reports
```bash
# SAST reports
cat build/.audit-reports/cppcheck-report.xml
cat build/.audit-reports/clang-tidy-output.txt

# Coverage
open build/.audit-reports/coverage-report.html

# Security scans
cat build/.audit-reports/gitleaks-report.json | jq
```

### Archive Evidence
```bash
# For releases, archive reports
VERSION="v1.4.1"
mkdir -p docs/audit-framework/evidence/${VERSION}/scans/
cp build/.audit-reports/*.{xml,json,txt} \
   docs/audit-framework/evidence/${VERSION}/scans/
```

---

## 📚 Learn More

### Essential Reading (5 minutes each)
- [Audit Charter](audit_charter_planning.md) - Overview and objectives
- [Compliance Mapping](COMPLIANCE_MAPPING.md) - Standards we follow
- [Audit Issue Template](.github/ISSUE_TEMPLATE/audit_review.md) - What gets checked

### When You Need More Detail (30 minutes)
- [Audit Runbook](AUDIT_RUNBOOK.md) - Complete step-by-step guide
- [OWASP ASVS](https://owasp.org/www-project-application-security-verification-standard/) - Security standards
- [ISO 27001](https://www.iso.org/standard/27001) - Information security

### Tools Documentation
- [cppcheck](http://cppcheck.sourceforge.net/) - Static analyzer
- [clang-tidy](https://clang.llvm.org/extra/clang-tidy/) - C++ linter
- [gitleaks](https://github.com/zricethezav/gitleaks) - Secret scanner
- [OWASP ZAP](https://www.zaproxy.org/) - Security scanner

---

## ❓ FAQ

**Q: Do I need to run audits for every small change?**  
A: No. CI runs automatically. Only run locally for:
- Security-sensitive changes (auth, crypto, validation)
- Before major PRs
- When you want early feedback

**Q: What if I disagree with a finding?**  
A: Comment on the audit issue with:
1. Why you think it's a false positive
2. Evidence supporting your view
3. Alternative risk mitigation (if applicable)
The audit team will review and respond.

**Q: Can I get an exception for a HIGH finding?**  
A: Yes, but:
1. Document business justification
2. Identify compensating controls
3. Get senior management approval
4. Add to risk acceptance register

**Q: How long do audits take?**  
A: Automated checks: 10-15 minutes
Full release audit: 2-3 days (with findings remediation)

**Q: Who can I ask for help?**  
A: 
- **Technical:** security@themisdb.org
- **Process:** audit-team@themisdb.org
- **GitHub:** Open issue with label `audit-framework`

---

## 🎓 Next Steps

**New to the project?**
1. Read [CONTRIBUTING.md](../../CONTRIBUTING.md)
2. Review [Security Policy](../../SECURITY.md)
3. Check [Coding Standards](../CODING_STANDARDS.md)

**Ready to contribute?**
1. Fork the repo
2. Create feature branch
3. Make changes with tests
4. Run local checks
5. Submit PR
6. Wait for audit-check ✅

**Preparing a release?**
1. Review [Audit Runbook](AUDIT_RUNBOOK.md)
2. Use [Release Checklist](templates/AUDIT_REPORT_EXECUTIVE_TEMPLATE.md)
3. Follow [Git Flow](../ci-cd/branching-release-history/BRANCHING_STRATEGY.md)

---

**Happy Coding! 🚀**

Questions? Contact: audit-team@themisdb.org
