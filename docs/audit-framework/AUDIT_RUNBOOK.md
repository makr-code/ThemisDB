# ThemisDB Audit Runbook

**Version:** 1.0  
**Date:** January 2026  
**Purpose:** Step-by-step guide for conducting ThemisDB security and compliance audits

---

## 📋 Table of Contents

- [1. Introduction](#1-introduction)
- [2. Pre-Audit Setup](#2-pre-audit-setup)
- [3. Audit Execution Steps](#3-audit-execution-steps)
- [4. Tool Integration Guide](#4-tool-integration-guide)
- [5. Evidence Collection](#5-evidence-collection)
- [6. Reporting Templates](#6-reporting-templates)
- [7. KPIs and Metrics](#7-kpis-and-metrics)
- [8. Troubleshooting](#8-troubleshooting)

---

## 1. Introduction

### 1.1 Purpose

This runbook provides detailed, step-by-step procedures for conducting security and compliance audits for ThemisDB. It ensures consistent, repeatable, and efficient audit execution.

### 1.2 Audience

- Lead Auditors
- Security Auditors
- Compliance Auditors
- DevOps Engineers
- Development Team Leads

### 1.3 Prerequisites

**Required Knowledge:**
- Security assessment methodologies
- C++ development and security
- Database security concepts
- Compliance frameworks (ISO 27001, NIST, OWASP)
- CI/CD and automation

**Required Access:**
- GitHub repository (read access)
- CI/CD system (GitHub Actions)
- Security scanning tools
- Documentation repositories
- Audit evidence storage

**Required Tools:**
- Git client
- Docker
- Security scanning tools (see section 4)
- Documentation tools (Markdown editor)

### 1.4 Related Documents

- `audit_charter_planning.md` - Audit framework and governance
- `AUDIT_GATE_TEMPLATE.md` - Master checklist for audits
- `COMPLIANCE_MAPPING.md` - Standards compliance matrix
- `/SECURITY.md` - Security policy
- `/CONTRIBUTING.md` - Development guidelines

---

## 2. Pre-Audit Setup

### 2.1 Planning Phase (Day -7 to -1)

#### Step 1: Review Previous Audit
```bash
# Navigate to audit evidence repository
cd docs/audit-framework/evidence/

# Review previous audit findings
cat [previous_version]/audit-report-detailed.md

# Check open findings
grep -r "Status: Open" [previous_version]/findings/
```

**Action Items:**
- [ ] Review previous audit report
- [ ] Identify open findings to retest
- [ ] Note any recurring issues
- [ ] Document lessons learned

#### Step 2: Define Audit Scope
```bash
# Check release version
cat VERSION

# Review changes since last release
git log --oneline [last_release_tag]..HEAD

# Identify changed files
git diff --stat [last_release_tag]..HEAD > changes.txt
```

**Action Items:**
- [ ] Document release version
- [ ] List changed components
- [ ] Identify high-risk changes
- [ ] Define audit boundaries

#### Step 3: Update Audit Charter
```bash
# Update audit charter with current details
vim docs/audit-framework/audit_charter_planning.md

# Update audit schedule section
# Update team assignments
```

**Action Items:**
- [ ] Update audit dates
- [ ] Assign team roles
- [ ] Confirm availability
- [ ] Set up kickoff meeting

#### Step 4: Prepare Audit Checklist
```bash
# Copy template for this release
cp docs/audit-framework/AUDIT_GATE_TEMPLATE.md \
   docs/audit-framework/evidence/v[VERSION]/audit-checklist.md

# Update version and dates
sed -i 's/\[RELEASE_VERSION\]/v[VERSION]/g' audit-checklist.md
sed -i 's/\[AUDIT_DATE\]/[DATE]/g' audit-checklist.md
```

**Action Items:**
- [ ] Create release-specific checklist
- [ ] Customize for scope
- [ ] Remove N/A items
- [ ] Set up evidence folders

#### Step 5: Set Up Evidence Repository
```bash
# Create directory structure
mkdir -p docs/audit-framework/evidence/v[VERSION]/{scans,test-results,compliance,code-review,findings,reports}

# Initialize findings register
cat > docs/audit-framework/evidence/v[VERSION]/findings/findings-register.md << 'EOF'
# Findings Register - v[VERSION]

| ID | Category | Description | Risk | Status | Owner | Target Date | Evidence |
|----|----------|-------------|------|--------|-------|-------------|----------|
EOF
```

**Action Items:**
- [ ] Create evidence folders
- [ ] Set up tracking documents
- [ ] Configure access permissions
- [ ] Backup previous audit data

### 2.2 Environment Setup

#### Step 1: Clone Repository
```bash
# Clone ThemisDB repository
git clone https://github.com/makr-code/ThemisDB.git
cd ThemisDB

# Checkout release branch
git checkout [release_branch]

# Verify commit
git log -1
```

#### Step 2: Set Up Build Environment
```bash
# Install dependencies (Linux)
sudo apt-get update
sudo apt-get install -y \
  build-essential cmake ninja-build \
  libssl-dev librocksdb-dev \
  clang-tidy cppcheck

# Or use Docker build environment
docker pull themisdb/build-env:latest
```

#### Step 3: Configure Security Tools
```bash
# Verify tool installations
cppcheck --version
clang-tidy --version
docker run --rm owasp/zap2docker-stable zap-cli --version

# Configure .cppcheck file (if needed)
cat > .cppcheck << 'EOF'
--enable=all
--suppress=unusedFunction
--inline-suppr
--std=c++17
EOF
```

---

## 3. Audit Execution Steps

### 3.1 Phase 1: Automated Security Scanning (Day 1-2)

#### 3.1.1 Static Application Security Testing (SAST)

**Step 1: Run cppcheck**
```bash
# Full cppcheck scan
cppcheck --enable=all \
  --std=c++17 \
  --suppress=unusedFunction \
  --xml \
  --xml-version=2 \
  --output-file=audit-evidence/v[VERSION]/scans/sast-cppcheck.xml \
  src/ include/

# Generate human-readable report
cppcheck --enable=all \
  --std=c++17 \
  --suppress=unusedFunction \
  src/ include/ \
  2>&1 | tee audit-evidence/v[VERSION]/scans/sast-cppcheck.txt
```

**Step 2: Run clang-tidy**
```bash
# Generate compile_commands.json
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -B build

# Run clang-tidy
clang-tidy -p build \
  --checks='*,-llvmlibc-*' \
  --header-filter='.*' \
  src/**/*.cpp \
  2>&1 | tee audit-evidence/v[VERSION]/scans/sast-clang-tidy.txt
```

**Step 3: Secret Scanning with Gitleaks**
```bash
# Run Gitleaks
docker run --rm -v $(pwd):/repo \
  zricethezav/gitleaks:latest \
  detect --source /repo \
  --report-format json \
  --report-path /repo/gitleaks-report.json

# Copy to evidence
cp gitleaks-report.json audit-evidence/v[VERSION]/scans/secret-scan-gitleaks.json
```

**Step 4: Dependency Scanning**
```bash
# Scan vcpkg dependencies
vcpkg list | tee audit-evidence/v[VERSION]/scans/dependencies-list.txt

# Check for known vulnerabilities (if tools available)
# Example: Using Snyk
snyk test --file=vcpkg.json \
  --json > audit-evidence/v[VERSION]/scans/dependency-scan-snyk.json || true
```

**Review Criteria:**
- [ ] No P0 (critical) vulnerabilities
- [ ] P1 (high) vulnerabilities documented and triaged
- [ ] False positives marked and suppressed
- [ ] Findings added to register

#### 3.1.2 Dynamic Application Security Testing (DAST)

**Step 1: Build and Run ThemisDB**
```bash
# Build release version
cmake -DCMAKE_BUILD_TYPE=Release -B build -G Ninja
cmake --build build -j $(nproc)

# Start ThemisDB server
./build/themisdb --config config/audit.conf &
THEMISDB_PID=$!

# Wait for startup
sleep 10

# Verify running
curl http://localhost:8080/health
```

**Step 2: Run OWASP ZAP Scan**
```bash
# Run ZAP baseline scan
docker run --rm --network host \
  -v $(pwd):/zap/wrk:rw \
  owasp/zap2docker-stable zap-baseline.py \
  -t http://localhost:8080 \
  -J audit-evidence/v[VERSION]/scans/dast-owasp-zap.json \
  -r audit-evidence/v[VERSION]/scans/dast-owasp-zap.html

# Run ZAP full scan (optional, takes longer)
docker run --rm --network host \
  -v $(pwd):/zap/wrk:rw \
  owasp/zap2docker-stable zap-full-scan.py \
  -t http://localhost:8080 \
  -J audit-evidence/v[VERSION]/scans/dast-owasp-zap-full.json
```

**Step 3: API Security Testing**
```bash
# Test authentication endpoints
curl -X POST http://localhost:8080/auth/login \
  -H "Content-Type: application/json" \
  -d '{"username":"admin","password":"wrong"}' \
  -v 2>&1 | tee audit-evidence/v[VERSION]/scans/api-auth-test.txt

# Test SQL injection resistance
curl "http://localhost:8080/api/query?q=SELECT+*+FROM+users+WHERE+id='1'+OR+'1'='1'" \
  -v 2>&1 | tee -a audit-evidence/v[VERSION]/scans/api-injection-test.txt

# Test rate limiting
for i in {1..100}; do
  curl -s http://localhost:8080/api/status
done | tee audit-evidence/v[VERSION]/scans/api-rate-limit-test.txt
```

**Step 4: TLS/SSL Configuration Testing**
```bash
# Test SSL configuration with testssl.sh
docker run --rm --network host \
  drwetter/testssl.sh:3.0 \
  --jsonfile=/output/tls-scan.json \
  https://localhost:8443

# Or use nmap
nmap --script ssl-enum-ciphers -p 8443 localhost \
  > audit-evidence/v[VERSION]/scans/tls-cipher-scan.txt
```

**Step 5: Clean Up**
```bash
# Stop ThemisDB
kill $THEMISDB_PID
```

**Review Criteria:**
- [ ] No high-severity vulnerabilities found
- [ ] TLS configuration follows best practices
- [ ] API security controls functioning
- [ ] Rate limiting effective

#### 3.1.3 Container Security Scanning

**Step 1: Build Container Image**
```bash
# Build Docker image
docker build -t themisdb:audit-v[VERSION] .
```

**Step 2: Scan with Trivy**
```bash
# Scan for vulnerabilities
docker run --rm \
  -v /var/run/docker.sock:/var/run/docker.sock \
  aquasec/trivy:latest image \
  --format json \
  --output /output/trivy-scan.json \
  themisdb:audit-v[VERSION]

# Copy results
docker cp [container_id]:/output/trivy-scan.json \
  audit-evidence/v[VERSION]/scans/container-scan-trivy.json

# Generate human-readable report
docker run --rm \
  -v /var/run/docker.sock:/var/run/docker.sock \
  aquasec/trivy:latest image \
  themisdb:audit-v[VERSION] \
  | tee audit-evidence/v[VERSION]/scans/container-scan-trivy.txt
```

**Step 3: Check Container Configuration**
```bash
# Inspect Dockerfile
cat Dockerfile | tee audit-evidence/v[VERSION]/scans/dockerfile-review.txt

# Check for best practices
docker run --rm -i hadolint/hadolint < Dockerfile \
  | tee audit-evidence/v[VERSION]/scans/dockerfile-hadolint.txt
```

**Review Criteria:**
- [ ] No critical vulnerabilities in base image
- [ ] Container runs as non-root user
- [ ] Minimal attack surface (only required packages)
- [ ] Dockerfile follows best practices

### 3.2 Phase 2: Testing and Quality Assessment (Day 3)

#### 3.2.1 Run Test Suite

**Step 1: Unit Tests**
```bash
# Build with tests
cmake -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug -B build-test
cmake --build build-test -j $(nproc)

# Run unit tests with coverage
cd build-test
ctest --output-on-failure --verbose \
  | tee ../audit-evidence/v[VERSION]/test-results/unit-tests.txt

# Generate coverage report
gcovr -r .. --html --html-details \
  -o ../audit-evidence/v[VERSION]/test-results/coverage-report.html

# Generate coverage summary
gcovr -r .. | tee ../audit-evidence/v[VERSION]/test-results/coverage-summary.txt
```

**Step 2: Integration Tests**
```bash
# Run integration test suite
ctest -L integration --output-on-failure \
  | tee ../audit-evidence/v[VERSION]/test-results/integration-tests.txt
```

**Step 3: Security Tests**
```bash
# Run security-specific tests
ctest -L security --output-on-failure \
  | tee ../audit-evidence/v[VERSION]/test-results/security-tests.txt
```

**Step 4: Performance Benchmarks**
```bash
# Run benchmarks
./build-test/benchmarks/themisdb_benchmark \
  --benchmark_out=audit-evidence/v[VERSION]/test-results/benchmarks.json \
  --benchmark_out_format=json

# Generate readable report
./build-test/benchmarks/themisdb_benchmark \
  | tee audit-evidence/v[VERSION]/test-results/benchmarks.txt
```

**Review Criteria:**
- [ ] Unit test coverage > 80%
- [ ] All tests passing
- [ ] Security tests comprehensive
- [ ] Performance meets SLA

#### 3.2.2 Fuzz Testing

**Step 1: Build with Fuzzing Support**
```bash
# Run from the project root with AFL++ instrumentation and sanitizers
CC=afl-clang-lto CXX=afl-clang-lto++ cmake -B build-fuzz \
  -DCMAKE_BUILD_TYPE=Debug \
  -DENABLE_FUZZING=ON \
  -DENABLE_SANITIZERS=ON
cmake --build build-fuzz --target fuzz_targets
```

**Step 2: Run Fuzzers**
```bash
# Run from the project root: AQL parser fuzzer (AFL++ mode)
afl-fuzz -i fuzz/corpus/aql \
  -o audit-evidence/v[VERSION]/scans/fuzz-output \
  -x fuzz/dictionaries/aql.dict \
  -V 3600 \
  -- ./build-fuzz/fuzz/bin/aql_parser_harness @@
# Save human-readable AFL++ stats as the audit artifact
cp audit-evidence/v[VERSION]/scans/fuzz-output/default/fuzzer_stats \
  audit-evidence/v[VERSION]/scans/fuzzer_stats.txt
```

**Review Criteria:**
- [ ] No crashes found during fuzzing
- [ ] Memory errors caught by ASAN resolved
- [ ] Corpus coverage adequate

### 3.3 Phase 3: Compliance Verification (Day 4)

#### 3.3.1 ISO 27001 Compliance Check

**Step 1: Review Control Implementation**
```bash
# Generate compliance checklist
cat > audit-evidence/v[VERSION]/compliance/iso27001-checklist.md << 'EOF'
# ISO 27001:2022 Compliance Checklist - v[VERSION]

## Annex A Controls

### A.5 Organizational Controls
- [ ] A.5.1 Information security policies
- [ ] A.5.2 Information security roles
...
EOF
```

**Step 2: Collect Evidence**
- Review `/SECURITY.md` for security policy
- Review `/docs/security/` for procedures
- Check access control implementation in code
- Verify audit logging functionality
- Review encryption implementation
- Check incident response procedures

**Step 3: Document Gaps**
```bash
# Note any compliance gaps
echo "## ISO 27001 Gaps" >> audit-evidence/v[VERSION]/findings/findings-register.md
```

#### 3.3.2 NIST CSF Assessment

**Process each function:**
1. **Identify:** Asset inventory, risk assessment
2. **Protect:** Access control, data protection
3. **Detect:** Security monitoring, detection processes
4. **Respond:** Incident response, communication
5. **Recover:** Recovery planning, improvements

**Document in:** `audit-evidence/v[VERSION]/compliance/nist-csf-assessment.md`

#### 3.3.3 OWASP ASVS Verification

**Step 1: Review ASVS Requirements**
```bash
# Check key ASVS areas
grep -r "authentication" src/ | wc -l
grep -r "authorization" src/ | wc -l
grep -r "encryption" src/ | wc -l
```

**Step 2: Test Security Controls**
- V1: Architecture - Review design docs
- V2: Authentication - Test auth mechanisms
- V3: Session Management - Test session handling
- V4: Access Control - Test authorization
- V5: Input Validation - Test validation
- V6: Cryptography - Review crypto implementation
- V7: Error Handling - Test error responses
- V8: Data Protection - Test data security
- V9: Communications - Test TLS
- V10: Malicious Code - Code review
- V11: Business Logic - Test workflows
- V12: Files - Test file handling
- V13: API - Test API security
- V14: Configuration - Review config

**Document in:** `audit-evidence/v[VERSION]/compliance/owasp-asvs-checklist.md`

#### 3.3.4 BSI C5 Compliance

**Key control categories:**
- OIS: Organization of Information Security
- CHG: Change Management
- DEV: Development
- SEC: Information Security Incident Management
- IDM: Identity and Access Management
- CRY: Cryptography
- LOG: Logging
- DAS: Data Protection

**Document in:** `audit-evidence/v[VERSION]/compliance/bsi-c5-mapping.md`

#### 3.3.5 SOC 2 Trust Services

**Review:**
- CC1: Control Environment
- CC2: Communication and Information
- CC3: Risk Assessment
- CC4: Monitoring Activities
- CC5: Control Activities
- CC6: Logical and Physical Access
- CC7: System Operations
- CC8: Change Management
- CC9: Risk Mitigation

**Document in:** `audit-evidence/v[VERSION]/compliance/soc2-evidence.md`

#### 3.3.6 SLSA Level 3 Verification

**Step 1: Verify Build Process**
```bash
# Check GitHub Actions workflow
cat .github/workflows/release-build.yml

# Verify signed commits
git log --show-signature -1

# Check build provenance
cat build-provenance.json
```

**Step 2: Verify Requirements**
- [ ] Source integrity (version control)
- [ ] Build integrity (reproducible builds)
- [ ] Provenance authenticity (signed)
- [ ] Provenance completeness (all inputs tracked)

**Document in:** `audit-evidence/v[VERSION]/compliance/slsa-provenance.json`

### 3.4 Phase 4: Manual Security Review (Day 4-5)

#### 3.4.1 Code Review

**Focus Areas:**
1. Authentication implementation (`src/auth/`)
2. Authorization logic (`src/rbac/`)
3. Encryption handling (`src/crypto/`)
4. Input validation (`src/parser/`)
5. SQL query handling (`src/query/`)
6. Session management (`src/session/`)
7. Error handling (all modules)

**Review Process:**
```bash
# Review high-risk code
git diff [last_release_tag]..HEAD src/auth/ > audit-evidence/v[VERSION]/code-review/auth-changes.diff
git diff [last_release_tag]..HEAD src/crypto/ > audit-evidence/v[VERSION]/code-review/crypto-changes.diff

# Check for common issues
grep -rn "strcpy\|strcat\|sprintf" src/  # Unsafe functions
grep -rn "TODO\|FIXME\|HACK" src/ > audit-evidence/v[VERSION]/code-review/todos.txt
grep -rn "password\|secret\|key" src/ | grep -v "Hash\|Encrypted" > audit-evidence/v[VERSION]/code-review/sensitive-strings.txt
```

**Review Checklist:**
- [ ] No unsafe C functions (strcpy, etc.)
- [ ] Proper error handling
- [ ] Input validation on all inputs
- [ ] No hardcoded credentials
- [ ] Secure random number generation
- [ ] Proper memory management
- [ ] No information leakage in errors
- [ ] Security headers implemented

**Document in:** `audit-evidence/v[VERSION]/code-review/security-review-notes.md`

#### 3.4.2 Configuration Review

**Step 1: Review Default Configuration**
```bash
# Check default config
cat config/themisdb.conf | tee audit-evidence/v[VERSION]/code-review/default-config.txt

# Check for insecure defaults
grep -i "password\|secret\|debug\|verbose" config/
```

**Step 2: Review Deployment Configuration**
```bash
# Check Docker Compose
cat docker-compose.yml

# Check Kubernetes manifests
cat deploy/kubernetes/*.yaml

# Check Helm values
cat helm/themisdb/values.yaml
```

**Review Criteria:**
- [ ] No default credentials
- [ ] Secure defaults (encryption enabled, etc.)
- [ ] Debug mode disabled in production
- [ ] Resource limits configured
- [ ] Security contexts defined (K8s)

#### 3.4.3 Documentation Review

**Review Documents:**
- README.md
- SECURITY.md
- docs/de/compliance/
- docs/security/
- API documentation
- Deployment guides

**Checklist:**
- [ ] Security policy complete and current
- [ ] Deployment security best practices documented
- [ ] Compliance documentation up to date
- [ ] API security guidelines provided
- [ ] Incident response procedures documented

### 3.5 Phase 5: Findings Analysis (Day 5-6)

#### 3.5.1 Consolidate Findings

**Step 1: Create Master Findings List**
```bash
# Consolidate all findings
cat > audit-evidence/v[VERSION]/findings/findings-register.md << 'EOF'
# Findings Register - v[VERSION]

## Critical (P0)
[List P0 findings]

## High (P1)
[List P1 findings]

## Medium (P2)
[List P2 findings]

## Low (P3)
[List P3 findings]
EOF
```

#### 3.5.2 Risk Assessment

**For each finding:**
1. Assign CVSS score (if vulnerability)
2. Determine likelihood and impact
3. Calculate risk priority (P0-P3)
4. Identify affected components
5. Recommend remediation

**Use Risk Matrix:**
```
Risk = Likelihood (1-5) × Impact (1-5)
P0: Risk ≥ 20 (Critical)
P1: Risk 12-19 (High)
P2: Risk 6-11 (Medium)
P3: Risk ≤ 5 (Low)
```

#### 3.5.3 Root Cause Analysis

**For critical/high findings:**
1. Perform 5 Whys analysis
2. Identify root cause category:
   - Process issue
   - Knowledge gap
   - Tool limitation
   - Time constraint
3. Document in findings register
4. Recommend process improvements

### 3.6 Phase 6: Remediation (Day 6-8)

#### 3.6.1 Create Remediation Plan

**For each P0/P1 finding:**
```markdown
## Remediation Plan: [Finding ID]

**Finding:** [Description]
**Risk:** [P0/P1]
**Root Cause:** [Analysis]

**Remediation Steps:**
1. [Step 1]
2. [Step 2]
...

**Owner:** [Developer name]
**Target Date:** [Date]
**Dependencies:** [Any dependencies]
**Verification:** [How to verify fix]
```

#### 3.6.2 Track Remediation

```bash
# Create GitHub issues for findings
gh issue create \
  --title "[AUDIT] [P1] [Finding description]" \
  --body "$(cat finding-template.md)" \
  --label "security,audit,P1"

# Track progress
gh issue list --label audit --state open
```

#### 3.6.3 Verify Fixes

**For each remediation:**
1. Review code changes
2. Rerun relevant tests
3. Rerun security scans
4. Verify in findings register
5. Update status to "Verified"

### 3.7 Phase 7: Reporting (Day 9)

#### 3.7.1 Generate Executive Summary

**Template:** (See Section 6.1)

**Key Elements:**
- Overall assessment (Pass/Fail/Conditional)
- Critical findings count
- Remediation status
- Risk summary
- Recommendations
- Release decision

#### 3.7.2 Generate Detailed Report

**Template:** (See Section 6.2)

**Sections:**
- Executive summary
- Audit scope and methodology
- Findings summary
- Detailed findings with evidence
- Risk assessment
- Remediation tracking
- Compliance status
- Recommendations
- Appendices (scan reports, test results)

#### 3.7.3 Prepare Metrics Dashboard

**Generate metrics:**
```bash
# Calculate metrics
total_findings=$(wc -l < findings-register.md)
critical_findings=$(grep "P0" findings-register.md | wc -l)
code_coverage=$(grep "lines:" coverage-summary.txt | awk '{print $2}')

# Document in report
cat > audit-evidence/v[VERSION]/reports/metrics-summary.txt << EOF
# Audit Metrics Summary

- Total Findings: $total_findings
- Critical (P0): $critical_findings
- Code Coverage: $code_coverage
EOF
```

### 3.8 Phase 8: Sign-Off (Day 10)

#### 3.8.1 Obtain Approvals

**Process:**
1. Distribute audit report to approvers
2. Schedule sign-off meeting
3. Present findings and remediation status
4. Address questions and concerns
5. Obtain written approvals

**Required Sign-Offs:**
- [ ] Lead Auditor
- [ ] Security Lead
- [ ] Compliance Officer
- [ ] Release Manager

#### 3.8.2 Document Decision

```markdown
## Release Decision

**Release Version:** v[VERSION]
**Audit Completion Date:** [DATE]
**Decision:** ✅ APPROVED / ⚠️ CONDITIONAL / ❌ REJECTED

**Approvals:**
- Lead Auditor: [Name] - [Date]
- Security Lead: [Name] - [Date]
- Compliance Officer: [Name] - [Date]
- Release Manager: [Name] - [Date]

**Conditions (if any):**
[List any conditions]

**Open Items:**
[List any open P2/P3 items for next release]
```

---

## 4. Tool Integration Guide

### 4.1 Static Analysis Tools

#### 4.1.1 cppcheck

**Installation:**
```bash
# Ubuntu/Debian
sudo apt-get install cppcheck

# macOS
brew install cppcheck

# From source
git clone https://github.com/danmar/cppcheck.git
cd cppcheck
make MATCHCOMPILER=yes FILESDIR=/usr/share/cppcheck
sudo make install
```

**Configuration:**
```bash
# Create .cppcheck config
cat > .cppcheck << 'EOF'
--enable=all
--std=c++17
--inline-suppr
--suppress=unusedFunction
--suppress=missingIncludeSystem
--max-configs=30
-j $(nproc)
EOF
```

**Usage:**
```bash
# Basic scan
cppcheck src/

# Full scan with all checks
cppcheck --enable=all --std=c++17 src/ include/

# XML output for CI
cppcheck --enable=all --xml --xml-version=2 src/ 2> cppcheck-report.xml

# With specific checks
cppcheck --enable=warning,style,performance,portability src/
```

#### 4.1.2 clang-tidy

**Installation:**
```bash
# Ubuntu/Debian
sudo apt-get install clang-tidy

# macOS
brew install llvm
```

**Configuration:**
```yaml
# Create .clang-tidy
---
Checks: >
  -*,
  bugprone-*,
  cert-*,
  cppcoreguidelines-*,
  google-*,
  modernize-*,
  performance-*,
  readability-*,
  security-*,
  -google-readability-todo,
  -modernize-use-trailing-return-type

CheckOptions:
  - key: readability-identifier-naming.ClassCase
    value: CamelCase
  - key: readability-identifier-naming.FunctionCase
    value: camelCase
```

**Usage:**
```bash
# Scan single file
clang-tidy src/main.cpp -- -std=c++17

# Scan with compile_commands.json
clang-tidy -p build src/main.cpp

# Scan all files
find src -name "*.cpp" -exec clang-tidy -p build {} \;
```

#### 4.1.3 Gitleaks (Secret Scanning)

**Installation:**
```bash
# Using Docker
docker pull zricethezav/gitleaks:latest

# Using binary
wget https://github.com/zricethezav/gitleaks/releases/download/v8.18.0/gitleaks_8.18.0_linux_x64.tar.gz
tar -xzf gitleaks_8.18.0_linux_x64.tar.gz
sudo mv gitleaks /usr/local/bin/
```

**Configuration:**
```toml
# .gitleaks.toml
title = "ThemisDB Gitleaks Config"

[[rules]]
id = "generic-api-key"
description = "Generic API Key"
regex = '''(?i)(api[_-]?key|apikey)['\"]?\s*[:=]\s*['\"]?[a-zA-Z0-9]{32,}'''
```

**Usage:**
```bash
# Scan repository
gitleaks detect --source . --verbose

# Scan with report
gitleaks detect --source . --report-format json --report-path gitleaks-report.json

# Scan specific commit range
gitleaks detect --source . --log-opts="[last_tag]..HEAD"
```

### 4.2 Dynamic Analysis Tools

#### 4.2.1 OWASP ZAP

**Installation:**
```bash
# Using Docker
docker pull owasp/zap2docker-stable

# Desktop version
wget https://github.com/zaproxy/zaproxy/releases/download/v2.12.0/ZAP_2.12.0_Linux.tar.gz
```

**Usage:**
```bash
# Baseline scan
docker run --rm --network host \
  -v $(pwd):/zap/wrk:rw \
  owasp/zap2docker-stable zap-baseline.py \
  -t http://localhost:8080 \
  -J zap-report.json \
  -r zap-report.html

# API scan
docker run --rm --network host \
  -v $(pwd):/zap/wrk:rw \
  owasp/zap2docker-stable zap-api-scan.py \
  -t http://localhost:8080/openapi.json \
  -f openapi \
  -J zap-api-report.json

# Full scan (authenticated)
docker run --rm --network host \
  -v $(pwd):/zap/wrk:rw \
  owasp/zap2docker-stable zap-full-scan.py \
  -t http://localhost:8080 \
  -z "-config api.addrs.addr.name=.* -config api.addrs.addr.regex=true"
```

#### 4.2.2 Sanitizers (ASAN, MSAN, UBSAN)

**Build Configuration:**
```bash
# Address Sanitizer
cmake -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address -fno-omit-frame-pointer -g" \
  -B build-asan
cmake --build build-asan

# Memory Sanitizer
cmake -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=memory -fno-omit-frame-pointer -g" \
  -B build-msan
cmake --build build-msan

# Undefined Behavior Sanitizer
cmake -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=undefined -fno-omit-frame-pointer -g" \
  -B build-ubsan
cmake --build build-ubsan
```

**Usage:**
```bash
# Run with ASAN
./build-asan/themisdb 2>&1 | tee asan-output.txt

# Run tests with ASAN
cd build-asan && ctest --output-on-failure 2>&1 | tee ../asan-test-output.txt
```

### 4.3 Container Security Tools

#### 4.3.1 Trivy

**Installation:**
```bash
# Using Docker
docker pull aquasec/trivy:latest

# Using binary
wget https://github.com/aquasecurity/trivy/releases/download/v0.40.0/trivy_0.40.0_Linux-64bit.tar.gz
tar -xzf trivy_0.40.0_Linux-64bit.tar.gz
sudo mv trivy /usr/local/bin/
```

**Usage:**
```bash
# Scan Docker image
trivy image themisdb:latest

# Scan with JSON output
trivy image --format json --output trivy-report.json themisdb:latest

# Scan filesystem
trivy fs --security-checks vuln,config .

# Scan with severity filter
trivy image --severity CRITICAL,HIGH themisdb:latest
```

#### 4.3.2 Hadolint (Dockerfile Linter)

**Installation:**
```bash
# Using Docker
docker pull hadolint/hadolint

# Using binary
wget -O /usr/local/bin/hadolint https://github.com/hadolint/hadolint/releases/download/v2.12.0/hadolint-Linux-x86_64
chmod +x /usr/local/bin/hadolint
```

**Usage:**
```bash
# Lint Dockerfile
hadolint Dockerfile

# With custom rules
hadolint --config .hadolint.yaml Dockerfile

# JSON output
hadolint --format json Dockerfile > hadolint-report.json
```

### 4.4 Test Coverage Tools

#### 4.4.1 gcovr

**Installation:**
```bash
pip install gcovr
```

**Usage:**
```bash
# Generate HTML report
gcovr -r . --html --html-details -o coverage.html

# Generate XML for CI
gcovr -r . --xml -o coverage.xml

# Print summary
gcovr -r .

# With branch coverage
gcovr -r . --branches
```

### 4.5 Performance Testing Tools

#### 4.5.1 Apache Benchmark (ab)

**Installation:**
```bash
sudo apt-get install apache2-utils
```

**Usage:**
```bash
# Basic load test
ab -n 10000 -c 100 http://localhost:8080/api/status

# With authentication
ab -n 1000 -c 10 -H "Authorization: Bearer TOKEN" http://localhost:8080/api/data

# POST requests
ab -n 1000 -c 10 -p data.json -T application/json http://localhost:8080/api/insert
```

---

## 5. Evidence Collection

### 5.1 Evidence Categories

#### 5.1.1 Scan Results
- SAST reports (cppcheck, clang-tidy)
- DAST reports (OWASP ZAP)
- Secret scan results (Gitleaks)
- Dependency scans
- Container scans (Trivy)

#### 5.1.2 Test Results
- Unit test reports
- Integration test reports
- Security test reports
- Performance benchmarks
- Coverage reports

#### 5.1.3 Compliance Documentation
- ISO 27001 checklist
- NIST CSF assessment
- OWASP ASVS verification
- BSI C5 mapping
- SOC 2 evidence
- SLSA provenance

#### 5.1.4 Code Review
- PR review comments
- Security review notes
- Configuration review
- Diff files for critical changes

#### 5.1.5 Findings
- Findings register
- Risk assessments
- Remediation plans
- Verification evidence

### 5.2 Evidence Organization

**Directory Structure:**
```
audit-evidence/
├── v1.4.1/
│   ├── scans/
│   │   ├── sast-cppcheck.txt
│   │   ├── sast-clang-tidy.txt
│   │   ├── dast-owasp-zap.json
│   │   ├── secret-scan-gitleaks.json
│   │   ├── dependency-scan.txt
│   │   └── container-scan-trivy.json
│   ├── test-results/
│   │   ├── unit-tests.xml
│   │   ├── integration-tests.xml
│   │   ├── security-tests.xml
│   │   ├── benchmarks.json
│   │   └── coverage-report.html
│   ├── compliance/
│   │   ├── iso27001-checklist.md
│   │   ├── nist-csf-assessment.md
│   │   ├── owasp-asvs-checklist.md
│   │   ├── bsi-c5-mapping.md
│   │   ├── soc2-evidence.md
│   │   └── slsa-provenance.json
│   ├── code-review/
│   │   ├── pr-reviews/
│   │   ├── security-review-notes.md
│   │   ├── auth-changes.diff
│   │   └── crypto-changes.diff
│   ├── findings/
│   │   ├── findings-register.md
│   │   ├── remediation-plans.md
│   │   ├── risk-assessments.md
│   │   └── risk-acceptance-forms/
│   └── reports/
│       ├── audit-report-executive.md
│       ├── audit-report-detailed.md
│       ├── metrics-summary.txt
│       └── sign-off-approvals.md
```

### 5.3 Evidence Retention

**Retention Periods:**
- Audit reports: 7 years
- Evidence files: 3 years
- Scan results: 2 years
- Working papers: 1 year

**Backup:**
```bash
# Archive evidence
tar -czf audit-evidence-v[VERSION].tar.gz audit-evidence/v[VERSION]/

# Upload to secure storage
aws s3 cp audit-evidence-v[VERSION].tar.gz s3://themisdb-audit-archive/
```

---

## 6. Reporting Templates

### 6.1 Executive Summary Template

```markdown
# ThemisDB Security Audit - Executive Summary

**Release Version:** v[VERSION]
**Audit Period:** [START_DATE] to [END_DATE]
**Lead Auditor:** [NAME]
**Overall Assessment:** ✅ PASS / ⚠️ CONDITIONAL PASS / ❌ FAIL

## Key Findings

### Security Posture
- **Critical Issues (P0):** [COUNT] - [STATUS]
- **High Issues (P1):** [COUNT] - [STATUS]
- **Medium Issues (P2):** [COUNT]
- **Low Issues (P3):** [COUNT]

### Compliance Status
- **ISO 27001:** ✅ Compliant / ⚠️ Partially Compliant / ❌ Non-Compliant
- **NIST CSF:** ✅ Tier 3 / ⚠️ Tier 2 / ❌ Tier 1
- **OWASP ASVS:** ✅ Level 2 / ⚠️ Level 1
- **BSI C5:** ✅ Compliant / ⚠️ Gaps Identified
- **SOC 2:** ✅ Ready / ⚠️ Preparation Needed
- **SLSA:** ✅ Level 3 / ⚠️ Level 2

### Quality Metrics
- **Code Coverage:** [PERCENTAGE]%
- **SAST Pass Rate:** [PERCENTAGE]%
- **Test Success Rate:** [PERCENTAGE]%
- **Vulnerability Density:** [NUMBER] per 1000 LOC

## Risk Summary

[High-level risk assessment and key concerns]

## Release Recommendation

**Recommendation:** ✅ APPROVE / ⚠️ CONDITIONAL APPROVAL / ❌ DELAY

**Rationale:** [Brief explanation]

**Conditions (if applicable):**
1. [Condition 1]
2. [Condition 2]

## Key Recommendations

1. [Recommendation 1]
2. [Recommendation 2]
3. [Recommendation 3]

---

**Prepared By:** [AUDITOR NAME]
**Date:** [DATE]
**Distribution:** CTO, CISO, Compliance Officer, Release Manager
```

### 6.2 Detailed Audit Report Template

```markdown
# ThemisDB Security and Compliance Audit Report

**Version:** v[VERSION]
**Audit Period:** [START] to [END]
**Report Date:** [DATE]
**Classification:** Confidential - Internal Use Only

---

## Table of Contents

1. Executive Summary
2. Audit Objectives and Scope
3. Audit Methodology
4. Findings Summary
5. Detailed Findings
6. Risk Assessment
7. Compliance Status
8. Test Results
9. Remediation Tracking
10. Recommendations
11. Conclusion
12. Appendices

---

## 1. Executive Summary

[See template above]

## 2. Audit Objectives and Scope

### 2.1 Objectives
[List primary and secondary objectives]

### 2.2 Scope
**In Scope:**
- [Components audited]

**Out of Scope:**
- [Exclusions]

### 2.3 Standards Applied
- ISO/IEC 27001:2022
- NIST Cybersecurity Framework v1.1
- OWASP ASVS v4.0
- BSI C5
- SOC 2
- SLSA Level 3

## 3. Audit Methodology

### 3.1 Approach
[Description of audit approach]

### 3.2 Phases
1. Planning and Preparation
2. Automated Security Scanning
3. Manual Security Review
4. Compliance Verification
5. Testing and QA Assessment
6. Findings Analysis
7. Remediation Support
8. Reporting and Sign-Off

### 3.3 Tools Used
- Static Analysis: cppcheck, clang-tidy, Gitleaks
- Dynamic Analysis: OWASP ZAP, sanitizers
- Container Security: Trivy, Hadolint
- Test Coverage: gcovr
- Performance: Apache Benchmark

## 4. Findings Summary

| Priority | Count | Resolved | Open | % Resolved |
|----------|-------|----------|------|------------|
| P0 (Critical) | [N] | [N] | [N] | [%] |
| P1 (High) | [N] | [N] | [N] | [%] |
| P2 (Medium) | [N] | [N] | [N] | [%] |
| P3 (Low) | [N] | [N] | [N] | [%] |
| **Total** | **[N]** | **[N]** | **[N]** | **[%]** |

### 4.1 Findings by Category
[Breakdown by security category]

### 4.2 Findings by Component
[Breakdown by system component]

## 5. Detailed Findings

### Finding F-001: [Title]

**ID:** F-001
**Priority:** P1
**Category:** [Authentication/Authorization/etc.]
**CWE:** [CWE-XXX]
**CVSS Score:** [X.X]

**Description:**
[Detailed description of the finding]

**Impact:**
[Security impact and business impact]

**Evidence:**
[References to evidence files, screenshots, logs]

**Affected Components:**
- [Component 1]
- [Component 2]

**Recommendation:**
[Specific remediation steps]

**Status:** Open / In Progress / Resolved / Accepted
**Owner:** [Name]
**Target Date:** [Date]

**Root Cause:**
[Root cause analysis]

[Repeat for each finding]

## 6. Risk Assessment

### 6.1 Overall Risk Posture
[Assessment of overall security risk]

### 6.2 Risk Heat Map
[Visual representation of risk distribution]

### 6.3 Residual Risk
[Assessment of remaining risks after remediation]

## 7. Compliance Status

### 7.1 ISO 27001
**Status:** Compliant / Partially Compliant / Non-Compliant
**Details:** [Summary of compliance status]
**Gaps:** [List any gaps]

### 7.2 NIST CSF
**Maturity Tier:** [1-4]
**Details:** [Assessment per function]

### 7.3 OWASP ASVS
**Level Achieved:** [Level 1/2/3]
**Details:** [Summary of verification]

### 7.4 BSI C5
**Status:** Compliant / Gaps Identified
**Details:** [Control assessment]

### 7.5 SOC 2
**Readiness:** Ready / Preparation Needed
**Details:** [Trust Services Criteria assessment]

### 7.6 SLSA
**Level:** [Level 1/2/3]
**Details:** [Supply chain security assessment]

## 8. Test Results

### 8.1 Unit Tests
- **Total Tests:** [N]
- **Passed:** [N]
- **Failed:** [N]
- **Code Coverage:** [X]%

### 8.2 Integration Tests
- **Total Tests:** [N]
- **Passed:** [N]
- **Failed:** [N]

### 8.3 Security Tests
- **Authentication Tests:** [N/N passed]
- **Authorization Tests:** [N/N passed]
- **Input Validation Tests:** [N/N passed]
- **Encryption Tests:** [N/N passed]

### 8.4 Performance Tests
- **Write Throughput:** [X] ops/sec (Target: 45K)
- **Read Throughput:** [X] ops/sec (Target: 120K)
- **Latency p95:** [X] ms
- **Latency p99:** [X] ms

## 9. Remediation Tracking

### 9.1 Remediation Status
[Summary of remediation progress]

### 9.2 Remediation Timeline
[Gantt chart or timeline of remediation activities]

### 9.3 Verification Status
[Status of fix verification]

## 10. Recommendations

### 10.1 Immediate Actions (Pre-Release)
1. [Action 1]
2. [Action 2]

### 10.2 Short-Term (Next 30 Days)
1. [Action 1]
2. [Action 2]

### 10.3 Long-Term (Next Quarter)
1. [Action 1]
2. [Action 2]

### 10.4 Process Improvements
1. [Improvement 1]
2. [Improvement 2]

## 11. Conclusion

[Summary of audit outcomes and final assessment]

## 12. Appendices

### Appendix A: Scan Reports
- A.1 SAST Reports
- A.2 DAST Reports
- A.3 Dependency Scans
- A.4 Container Scans

### Appendix B: Test Results
- B.1 Unit Test Reports
- B.2 Integration Test Reports
- B.3 Coverage Reports
- B.4 Performance Benchmarks

### Appendix C: Compliance Checklists
- C.1 ISO 27001 Checklist
- C.2 NIST CSF Assessment
- C.3 OWASP ASVS Checklist
- C.4 BSI C5 Mapping
- C.5 SOC 2 Evidence
- C.6 SLSA Provenance

### Appendix D: Sign-Off Records
- D.1 Approval Signatures
- D.2 Risk Acceptance Forms

---

**Report prepared by:** [AUDITOR NAME]
**Reviewed by:** [REVIEWER NAME]
**Approved by:** [LEAD AUDITOR NAME]
**Date:** [DATE]

**Distribution:**
- CTO
- CISO
- Compliance Officer
- Development Lead
- Release Manager

**Confidentiality:** Internal Use Only - Do Not Distribute
```

---

## 7. KPIs and Metrics

### 7.1 Security KPIs

| Metric | Target | Measurement Method |
|--------|--------|-------------------|
| **Mean Time to Detect (MTTD)** | < 24 hours | Time from vulnerability disclosure to detection in codebase |
| **Mean Time to Remediate (MTTR) - Critical** | < 7 days | Time from detection to fix deployment |
| **Mean Time to Remediate (MTTR) - High** | < 30 days | Time from detection to fix deployment |
| **Vulnerability Density** | < 1 per 1000 LOC | Vulnerabilities found / lines of code * 1000 |
| **Security Test Coverage** | > 80% | Security-focused test coverage |
| **Critical Findings per Release** | < 5 | Count of P0/P1 findings in audit |
| **Recurrence Rate** | < 5% | Previously resolved findings that reappear |

### 7.2 Quality KPIs

| Metric | Target | Measurement Method |
|--------|--------|-------------------|
| **Code Coverage** | > 80% | gcovr report |
| **Static Analysis Pass Rate** | > 95% | Clean scans / total scans |
| **Code Complexity** | < 15 | Cyclomatic complexity average |
| **Build Success Rate** | > 99% | Successful builds / total builds |
| **Test Success Rate** | 100% | Passing tests / total tests |

### 7.3 Compliance KPIs

| Metric | Target | Measurement Method |
|--------|--------|-------------------|
| **Standard Compliance Rate** | > 95% | Compliant controls / total controls |
| **Audit Findings Remediation Rate** | > 90% within 30 days | Resolved findings / total findings |
| **Control Effectiveness** | > 85% | Effective controls / total controls |
| **Evidence Completeness** | > 95% | Evidence collected / evidence required |
| **Policy Adherence** | > 98% | Compliant activities / total activities |

### 7.4 Process KPIs

| Metric | Target | Measurement Method |
|--------|--------|-------------------|
| **Audit Completion On Schedule** | > 95% | On-time audits / total audits |
| **Audit Cycle Time** | < 10 days | Time from start to sign-off |
| **Automated Check Coverage** | > 70% | Automated checks / total checks |
| **Finding Recurrence Rate** | < 5% | Recurring findings / total findings |
| **Team Training Hours** | > 40 hrs/year/person | Training hours logged |

### 7.5 Metric Collection Script

```bash
#!/bin/bash
# metrics-collection.sh

VERSION="$1"
EVIDENCE_DIR="audit-evidence/v${VERSION}"

# Security Metrics
TOTAL_FINDINGS=$(grep -c "^| F-" ${EVIDENCE_DIR}/findings/findings-register.md)
CRITICAL_FINDINGS=$(grep -c "| P0 |" ${EVIDENCE_DIR}/findings/findings-register.md)
HIGH_FINDINGS=$(grep -c "| P1 |" ${EVIDENCE_DIR}/findings/findings-register.md)

# Quality Metrics
CODE_COVERAGE=$(grep "lines:" ${EVIDENCE_DIR}/test-results/coverage-summary.txt | awk '{print $2}')
SAST_ISSUES=$(grep -c "error:" ${EVIDENCE_DIR}/scans/sast-cppcheck.txt)

# Test Metrics
TOTAL_TESTS=$(grep "tests" ${EVIDENCE_DIR}/test-results/unit-tests.txt | awk '{print $1}')
PASSED_TESTS=$(grep "passed" ${EVIDENCE_DIR}/test-results/unit-tests.txt | awk '{print $1}')

# Output metrics
cat > ${EVIDENCE_DIR}/reports/metrics-summary.txt << EOF
# Audit Metrics Summary - v${VERSION}

## Security Metrics
- Total Findings: ${TOTAL_FINDINGS}
- Critical (P0): ${CRITICAL_FINDINGS}
- High (P1): ${HIGH_FINDINGS}

## Quality Metrics
- Code Coverage: ${CODE_COVERAGE}
- SAST Issues: ${SAST_ISSUES}

## Test Metrics
- Total Tests: ${TOTAL_TESTS}
- Passed Tests: ${PASSED_TESTS}
- Success Rate: $(echo "scale=2; ${PASSED_TESTS}/${TOTAL_TESTS}*100" | bc)%
EOF
```

---

## 8. Troubleshooting

### 8.1 Common Issues

#### Issue: cppcheck takes too long
**Solution:**
```bash
# Use parallel processing
cppcheck -j $(nproc) src/

# Limit checks
cppcheck --enable=warning,performance src/
```

#### Issue: OWASP ZAP cannot reach local server
**Solution:**
```bash
# Use host network mode
docker run --network host owasp/zap2docker-stable ...

# Or use host.docker.internal on macOS/Windows
docker run ... -t http://host.docker.internal:8080
```

#### Issue: Memory sanitizer build fails
**Solution:**
```bash
# Ensure libc++ is built with MSAN
# May need to rebuild libc++ with MSAN instrumentation
```

#### Issue: Coverage report empty
**Solution:**
```bash
# Ensure built with coverage flags
cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="--coverage" ...

# Run tests before generating report
ctest

# Then generate coverage
gcovr -r .
```

#### Issue: Gitleaks too many false positives
**Solution:**
```bash
# Use .gitleaks.toml to suppress false positives
# Add specific rules or use allowlist
```

### 8.2 Support Resources

- **ThemisDB Documentation:** https://makr-code.github.io/ThemisDB/
- **Security Policy:** `/SECURITY.md`
- **Audit Team Email:** security-audit@themisdb.org
- **GitHub Issues:** https://github.com/makr-code/ThemisDB/issues

---

## Appendix A: Quick Reference Checklists

### A.1 Pre-Audit Checklist
- [ ] Previous audit reviewed
- [ ] Scope defined
- [ ] Team assigned
- [ ] Tools configured
- [ ] Evidence repository set up
- [ ] Stakeholders notified

### A.2 Daily Audit Checklist
- [ ] Review progress against schedule
- [ ] Document findings immediately
- [ ] Update checklist status
- [ ] Communicate blockers
- [ ] Back up evidence

### A.3 Post-Audit Checklist
- [ ] All evidence collected
- [ ] All findings documented
- [ ] Remediation plans created
- [ ] Reports generated
- [ ] Approvals obtained
- [ ] Evidence archived
- [ ] Lessons learned documented

---

## Appendix B: Command Reference

### B.1 Essential Git Commands
```bash
# Check release version
cat VERSION

# Review changes
git log --oneline [last_release]..HEAD
git diff --stat [last_release]..HEAD

# Check for secrets
git log -p | grep -i "password\|secret\|key"
```

### B.2 Essential Audit Commands
```bash
# Quick security scan
cppcheck --enable=warning,performance src/

# Run all tests
ctest --output-on-failure

# Check test coverage
gcovr -r . --branches

# Find TODOs
grep -rn "TODO\|FIXME" src/
```

---

## Appendix C: Glossary

- **SAST:** Static Application Security Testing
- **DAST:** Dynamic Application Security Testing
- **ASAN:** Address Sanitizer
- **MSAN:** Memory Sanitizer
- **UBSAN:** Undefined Behavior Sanitizer
- **CVSS:** Common Vulnerability Scoring System
- **CWE:** Common Weakness Enumeration
- **MVCC:** Multi-Version Concurrency Control
- **RBAC:** Role-Based Access Control
- **TLS:** Transport Layer Security
- **SLSA:** Supply Chain Levels for Software Artifacts

---

**Document Version:** 1.0  
**Last Updated:** April 2026  
**Maintained By:** ThemisDB Security & Compliance Team  
**Next Review:** April 2026

---

*This runbook is a living document. Please submit improvements and updates through the standard change management process.*
