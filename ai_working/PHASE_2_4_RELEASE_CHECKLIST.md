# ThemisDB v2.4.0 Release Checklist

**Release**: v2.4.0  
**Release Manager**: TBD  
**Target Date**: July 2026  
**Status**: Pre-Release Checklist

---

## Phase 0: Pre-Release Planning (Week -2)

### 0.1 Feature Freeze & Branch Preparation

- [ ] **Verify feature set complete** — All Phase 2.4 items (Graph Truth Validation, Exception Safety, Performance) merged to `develop`
- [ ] **Code review complete** — All PRs approved by 2+ reviewers
- [ ] **Create RC branch** — `git checkout develop && git pull && git checkout -b release/v2.4-rc1`
- [ ] **Tag RC1** — `git tag v2.4.0-rc1 && git push origin v2.4.0-rc1`
- [ ] **Documentation freeze** — No breaking doc changes after this point
- [ ] **Dependency audit** — Check all third-party versions for security (vcpkg, openssl, rocksdb, etc.)
- [ ] **Notify stakeholders** — Send release plan to PM, marketing, support teams

**Sign-Off**: [ ] Release Manager

---

## Phase 1: Build & Compilation (Week -1)

### 1.1 Full Build Verification

**Command**:
```bash
cmake --preset community-release
cmake --build --preset community-release --parallel 16 --verbose
```

**Verification Checklist**:

- [ ] **Build succeeds** — Exit code 0, no fatal errors
- [ ] **No warnings** — Scan build log: `grep -i "warning" build.log | wc -l` → should be 0
- [ ] **All targets created** — Verify 22 test executables exist
  ```bash
  ls -la build/community-release/bin/test_graph* | wc -l
  # Expected: 22
  ```
- [ ] **Link-time optimization** — Check LTO is enabled: `grep -i "lto\|ltcg" CMakeCache.txt`
- [ ] **No ABI breaks** — Compare symbol signatures with v1.9.0 baseline
- [ ] **Executable signing** — Sign binaries with ThemisDB key (Enterprise only)

**Success Criteria**: Build time < 30 min, zero errors, zero warnings

**Sign-Off**: [ ] Build Engineer

---

### 1.2 Cross-Platform Build Verification

- [ ] **Linux x64** — Build + basic smoke test
- [ ] **Linux ARM64** — Build + basic smoke test (if applicable)
- [ ] **Windows x64** — MSVC 2019+ build
- [ ] **macOS** — Clang build (Intel + Apple Silicon if applicable)

**Command Template**:
```bash
cmake --preset <platform>-release
cmake --build --preset <platform>-release --parallel 16
./build/<platform>-release/bin/test_graph_basic --gtest_filter="*Basic*"
```

**Success Criteria**: All platforms build successfully, smoke test passes

**Sign-Off**: [ ] Platform Lead (or [ ] Release Manager if single-platform)

---

## Phase 2: Test Execution & Coverage (Week -1 to 0)

### 2.1 Full Test Suite Execution

**Command**:
```bash
ctest --preset linux-release -R "graph" \
  --output-on-failure \
  --verbose \
  --timeout 300 \
  -j 8
```

**Verification Checklist**:

- [ ] **Unit tests pass** — 156/156 ✅
- [ ] **Integration tests pass** — 87/87 ✅
- [ ] **Performance tests pass** — 51/51 ✅
- [ ] **Determinism tests pass** — 32/32 ✅
- [ ] **Total: 326/326 tests PASS** ✅
- [ ] **Zero skipped tests** — `ctest --output-on-failure | grep -i "skip"` → empty
- [ ] **Zero flaky tests** — Run 3x, consistent results
- [ ] **Execution time baseline** — Record as reference (target: < 10 min)

**Test Log Analysis**:
```bash
ctest -R "graph" -D ExperimentalTest 2>&1 | tee test-run.log
grep -i "passed\|failed\|error" test-run.log
```

**Success Criteria**: 326 PASS, 0 FAIL, 0 SKIP, execution time consistent ±5%

**Sign-Off**: [ ] QA Lead

---

### 2.2 Code Coverage Analysis

**Command**:
```bash
cmake --preset coverage
cmake --build --preset coverage --parallel 16
ctest --preset coverage -R "graph"
gcovr --root . --exclude='external/.*' --html-details coverage.html
```

**Verification Checklist**:

- [ ] **Line coverage** — ≥ 85% (graph module)
- [ ] **Branch coverage** — ≥ 75%
- [ ] **Function coverage** — ≥ 90%
- [ ] **Critical paths covered** — All truth validation paths exercised
- [ ] **No untested exception paths** — Review exception handling coverage
- [ ] **Coverage report generated** — `coverage.html` exists and readable

**Success Criteria**: Line coverage ≥ 85%, branch coverage ≥ 75%, no critical paths untested

**Sign-Off**: [ ] QA Lead

---

### 2.3 Memory Safety & Sanitizers

**Command**:
```bash
cmake --preset asan-release
cmake --build --preset asan-release --parallel 16
ctest --preset asan-release -R "graph" --output-on-failure
# Check for ASan/UBSan reports
```

**Verification Checklist**:

- [ ] **AddressSanitizer (ASan)** — Zero heap/stack errors
- [ ] **UndefinedBehaviorSanitizer (UBSan)** — Zero undefined behavior
- [ ] **MemorySanitizer (MSan)** — Zero uninitialized reads (if supported)
- [ ] **LeakSanitizer (LSan)** — Zero memory leaks
- [ ] **Thread Sanitizer (TSan)** — Zero data races (multi-threaded tests)
- [ ] **Sanitizer log clean** — No warnings or errors in stderr

**Success Criteria**: Zero sanitizer errors, all checks pass

**Sign-Off**: [ ] Security Lead

---

### 2.4 Valgrind & Static Analysis (Optional for Enterprise)

- [ ] **Valgrind memcheck** — Zero definite leaks
- [ ] **Clang static analyzer** — Zero high/critical issues
- [ ] **Coverity scan** — If applicable, zero high issues
- [ ] **CodeQL** — Zero security alerts

**Sign-Off**: [ ] Security Lead (Enterprise only)

---

## Phase 3: Performance & Stability (Week 0)

### 3.1 Performance Baseline Verification

**Command**:
```bash
./build/community-release/bin/themis-perf-benchmark \
  --suite graph_module \
  --warmup 1000 \
  --iterations 10000 \
  --threads 8 \
  --output v2.4.0-baseline.json
```

**Verification Checklist**:

- [ ] **Single-hop latency** — p99 < 1 ms (baseline from v1.9.0: ~1.2 ms)
- [ ] **5-hop latency** — p99 < 50 ms (baseline: ~60 ms) → 15–20% improvement
- [ ] **Full scan (1M nodes)** — p99 < 200 ms (baseline: ~250 ms)
- [ ] **Memory overhead** — < 5% increase from v1.9.0
- [ ] **CPU efficiency** — Task cycles per operation ≤ v1.9.0
- [ ] **Throughput** — Ops/sec ≥ v1.9.0 (no regression)
- [ ] **Baseline recorded** — `benchmarks/v2.4.0-baseline.json` exists

**Regression Detection Criteria**:
```
If (p99_latency > baseline × 1.10):
  ❌ FAIL — Investigate performance regression
If (throughput < baseline × 0.95):
  ❌ FAIL — Investigate throughput regression
If memory > baseline × 1.08:
  ⚠️ WARNING — Monitor memory scaling
```

**Success Criteria**: No regressions > 10%, baseline established

**Sign-Off**: [ ] Performance Lead

---

### 3.2 Stability Verification (100x Iteration Protocol)

**Command**:
```bash
#!/bin/bash
for i in {1..100}; do
  echo "=== Run $i at $(date) ==="
  ctest --preset linux-release -R "graph" \
    --output-on-failure \
    --timeout 300 \
    -j 8 || {
      echo "FAIL at run $i"
      exit 1
    }
  # Capture metrics
  ps aux | grep themis-server | grep -v grep | awk '{print $2, $3, $4, $6}' >> memory-log.txt
done
echo "All 100 runs completed successfully"
```

**Verification Checklist**:

- [ ] **All 100 runs PASS** — 100/100 successful
- [ ] **Zero flaky tests** — No intermittent failures
- [ ] **Execution time stable** — Max deviation ±5% from mean
- [ ] **Memory usage stable** — No leaks across 100 iterations
- [ ] **CPU usage stable** — No runaway processes
- [ ] **No timeouts** — All tests complete within time limit
- [ ] **Stability report generated** — `stability-100x-run.md` with statistics

**Success Criteria**: 100/100 runs PASS, zero flaky tests, stable metrics

**Sign-Off**: [ ] QA Lead

---

### 3.3 Determinism Verification

**Command**:
```bash
for i in {1..10}; do
  ctest --preset linux-release -R "graph.*determinism" \
    --output-on-failure > determinism-run-$i.log
done

# Compare outputs for consistency
for f in determinism-run-*.log; do
  grep "PASS\|FAIL" $f | sort > ${f}.sorted
done
```

**Verification Checklist**:

- [ ] **32 determinism tests** — All pass consistently
- [ ] **Output identical** — All 10 runs produce identical results
- [ ] **Timing deterministic** — Variance < 2% between runs
- [ ] **Graph construction deterministic** — Node IDs, edge ordering consistent
- [ ] **Validation results deterministic** — ACL/multi-hop outcomes always same for same input

**Success Criteria**: 32 determinism tests PASS across 10 runs, identical output

**Sign-Off**: [ ] QA Lead

---

## Phase 4: Documentation & L1 Compliance (Week 0)

### 4.1 Documentation Completeness

**Verification Checklist**:

- [ ] **Release notes complete** — `PHASE_2_4_RELEASE_NOTES_DRAFT.md` ✅
- [ ] **Migration guide** — For v1.9.x → v2.4.0 upgrades
- [ ] **API documentation** — All public APIs documented
  ```bash
  doxygen Doxyfile.html.local
  # Check: docs/generated/html/classthemis_1_1graph_1_1GraphTruthValidator.html exists
  # No "undocumented" warnings for public APIs
  ```
- [ ] **Configuration guide** — New options documented
  - `graph.truth_validation.policy_engine`
  - `graph.truth_validation.max_depth`
- [ ] **Troubleshooting guide** — Common issues & resolutions
- [ ] **Performance tuning guide** — Recommendations for production
- [ ] **Security guide** — ACL validation, fail-closed behavior explained

**Documentation Audit**:
```bash
grep -r "TODO\|FIXME\|XXX" docs/ --include="*.md" | wc -l
# Should be 0 for release
```

**Success Criteria**: All docs complete, no TODOs, release notes ready

**Sign-Off**: [ ] Documentation Lead

---

### 4.2 L1 Audit Re-run (4/4 Criteria)

**1. Documentation Completeness (L1.1)**

- [ ] All public APIs have Doxygen comments
- [ ] Parameters documented with `@param`
- [ ] Return values documented with `@return`
- [ ] Exceptions documented with `@throw`
- [ ] Example usage in headers for complex APIs

**Command**:
```bash
doxygen Doxyfile.html.local 2>&1 | grep -i "warning.*undocumented"
# Should output: 0 warnings
```

**2. Code Organization (L1.2)**

- [ ] Headers properly structured (include guards, forward declarations)
- [ ] Namespaces correctly nested (`themis::graph::*`)
- [ ] Includes properly ordered (std → boost → themis → local)
- [ ] No circular includes
- [ ] File organization follows module layout

**Command**:
```bash
clang-tidy src/graph/*.cpp -- -I. -Iinclude 2>&1 | grep "error"
# Should output: 0 errors
```

**3. Static Analysis (L1.3 — CodeQL)**

- [ ] CodeQL scan completes without errors
- [ ] Zero high-severity security alerts
- [ ] Zero critical bugs detected
- [ ] All actionable items addressed

**Command**:
```bash
# Run as part of CI/CD pipeline (see Phase 5)
# Results available in GitHub Security tab
```

**4. Coverage Threshold (L1.4)**

- [ ] Line coverage ≥ 85%
- [ ] Branch coverage ≥ 75%
- [ ] All critical paths covered
- [ ] Exception paths tested

**Command**:
```bash
gcovr --root . --exclude='external/.*' | tail -5
# Sample output:
# GCC Code Coverage Report
# Lines: 87.3% (...)
# Branches: 76.2% (...)
# Functions: 91.5% (...)
```

**Success Criteria**: All 4/4 L1 criteria met

**Sign-Off**: [ ] L1 Audit Lead

---

## Phase 5: Security & Code Quality (Week 0)

### 5.1 Security Audit

**Verification Checklist**:

- [ ] **CodeQL full scan** — Completed and reviewed
  ```bash
  codeql database create <db> --language=cpp --source-root=.
  codeql database analyze <db> security-and-quality.qls
  ```
- [ ] **Dependency security audit** — No known vulnerabilities
  ```bash
  vcpkg audit
  # Review OpenSSL, RocksDB, gRPC versions for CVEs
  ```
- [ ] **Secret scanning** — No secrets in repository
  ```bash
  git-secrets --scan
  gitleaks detect --verbose
  ```
- [ ] **SBOM generated** — Software Bill of Materials for compliance
  ```bash
  syft packages themisdb:2.4.0 -o cyclonedx > sbom.xml
  ```
- [ ] **License compliance** — All dependencies MIT-compatible
- [ ] **Security review board approval** — Signed off on architecture

**Critical Security Items**:

- [ ] **ACL validation fail-closed** — Verified in code review (GraphTruthValidator)
- [ ] **Multi-hop depth bounded** — `max_depth` enforced
- [ ] **No SQL injection** — Graph query parameters sanitized
- [ ] **Authentication enforced** — API key or cert validation enabled
- [ ] **Encryption in transit** — TLS 1.3+ required
- [ ] **Encryption at rest** — Optional, documented

**Success Criteria**: Zero high/critical security findings, SBOM generated

**Sign-Off**: [ ] Security Lead

---

### 5.2 Code Quality Gate

**Verification Checklist**:

- [ ] **Clang-Format compliance** — All code formatted
  ```bash
  clang-format --dry-run --Werror src/graph/*.cpp src/graph/*.hpp
  # Should exit with 0
  ```
- [ ] **Clang-Tidy warnings** — Zero critical issues
  ```bash
  clang-tidy src/graph/*.cpp -- -I. | grep -i "error"
  # Should output empty
  ```
- [ ] **CMake best practices** — Reviewed by CMake expert
- [ ] **No compiler warnings** — Build clean on GCC, Clang, MSVC
- [ ] **Code review completed** — 2+ approvals per file

**Success Criteria**: Zero critical code quality issues

**Sign-Off**: [ ] Code Quality Lead

---

## Phase 6: Release Build & Artifacts (Week 0–1)

### 6.1 Release Binaries

**Build Commands**:

```bash
# Community Edition
cmake --preset community-release
cmake --build --preset community-release --parallel 16 --config Release
cmake --install build/community-release \
  --component runtime \
  --prefix /tmp/themisdb-2.4.0-community

# Minimal Edition
cmake --preset minimal-release
cmake --build --preset minimal-release --parallel 16 --config Release
cmake --install build/minimal-release \
  --component runtime \
  --prefix /tmp/themisdb-2.4.0-minimal

# Enterprise (if applicable)
git checkout enterprise
cmake --preset enterprise-release
cmake --build --preset enterprise-release --parallel 16 --config Release
cmake --install build/enterprise-release \
  --component runtime \
  --prefix /tmp/themisdb-2.4.0-enterprise
```

**Verification Checklist**:

- [ ] **Binaries created** — All edition binaries present
  - `themisdb-2.4.0-community-binary-x64`
  - `themisdb-2.4.0-minimal-binary-x64`
  - `themisdb-2.4.0-enterprise-binary-x64` (if applicable)
- [ ] **Binary signed** — GPG signature generated for enterprise editions
- [ ] **Binary tested** — Run with `--version` and `--help`
  ```bash
  ./themisdb-server --version
  # Output: ThemisDB v2.4.0 [RELEASE]
  ```
- [ ] **Dependencies bundled** — All runtime dependencies included
- [ ] **License files included** — LICENSE and third-party notices
- [ ] **Checksums generated** — SHA-256 for each binary

**Success Criteria**: All binaries created, tested, signed, checksummed

**Sign-Off**: [ ] Release Engineer

---

### 6.2 Docker Images

**Build Commands**:

```bash
# Community
docker build -f Dockerfile -t themisdb/themisdb:2.4.0-community-binary-x64 .
docker push themisdb/themisdb:2.4.0-community-binary-x64
docker tag themisdb/themisdb:2.4.0-community-binary-x64 themisdb/themisdb:latest

# Minimal
docker build -f Dockerfile.minimal -t themisdb/themisdb-minimal:2.4.0-minimal-binary-x64 .
docker push themisdb/themisdb-minimal:2.4.0-minimal-binary-x64
```

**Verification Checklist**:

- [ ] **Docker images build** — Both community and minimal
- [ ] **Image size reasonable** — Community < 1 GB, Minimal < 500 MB
- [ ] **Image runs** — `docker run -it themisdb/themisdb:2.4.0-community-binary-x64 themisdb-server --version`
- [ ] **Images pushed** — Available on Docker Hub
- [ ] **Latest tag updated** — `latest` points to v2.4.0

**Success Criteria**: Docker images built, tested, pushed

**Sign-Off**: [ ] DevOps Lead

---

### 6.3 Package Managers

**Verification Checklist** (if applicable):

- [ ] **vcpkg port** — Updated for v2.4.0
  ```bash
  vcpkg install themisdb:2.4.0
  ```
- [ ] **Homebrew formula** — macOS support (if applicable)
- [ ] **APT/RPM packages** — Linux distributions
- [ ] **Windows MSI** — Installer package
- [ ] **Package tests** — Installation verification for each package manager

**Success Criteria**: All applicable package managers updated

**Sign-Off**: [ ] Package Lead

---

## Phase 7: Release Announcement & Deployment (Week 1)

### 7.1 Release Tagging

**Verification Checklist**:

- [ ] **Git tag created** — `git tag v2.4.0 && git push origin v2.4.0`
- [ ] **Tag signed** — GPG signature on tag (for security)
  ```bash
  git tag -s v2.4.0 -m "Release v2.4.0" <GPG_KEY_ID>
  ```
- [ ] **GitHub Release created** — Release page published with notes
- [ ] **Release pinned** — v2.4.0 visible on GitHub releases page

**Success Criteria**: Git tag exists, GitHub release published

**Sign-Off**: [ ] Release Manager

---

### 7.2 Announcement & Communication

**Verification Checklist**:

- [ ] **Changelog published** — `CHANGELOG.md` entry for v2.4.0
- [ ] **Blog post published** — Announcement on ThemisDB blog
- [ ] **Social media announced** — Twitter, LinkedIn, community channels
- [ ] **Stakeholders notified** — PM, marketing, support, partners
- [ ] **Email sent** — Release notification to mailing list
- [ ] **Forum post** — Announcement in community forum

**Success Criteria**: Release announced across all channels

**Sign-Off**: [ ] Marketing Lead

---

### 7.3 Deployment to Production

**Verification Checklist**:

- [ ] **Staging deployment** — Test in staging environment first
- [ ] **Health checks pass** — API responses, database queries work
- [ ] **Monitoring active** — Prometheus, Grafana, logs streaming
- [ ] **Production deployment** — Blue-green deployment initiated
- [ ] **Canary validation** — 5% traffic to v2.4.0, monitor for errors
- [ ] **Full rollout** — 100% traffic to v2.4.0
- [ ] **Metrics reviewed** — CPU, memory, latency baseline vs v1.9.0
- [ ] **User feedback** — Monitor support channel for issues

**Success Criteria**: Production deployment complete, no critical issues

**Sign-Off**: [ ] DevOps Lead

---

## Phase 8: Post-Release (Week 1+)

### 8.1 Support & Monitoring

**Verification Checklist**:

- [ ] **Support team trained** — v2.4.0 features, known issues, upgrade path
- [ ] **Monitoring active** — Dashboards tracking key metrics
- [ ] **Alert thresholds set** — CPU, memory, latency alerts configured
- [ ] **Issue triage** — Rapid response protocol for critical bugs
- [ ] **Patch release plan** — If critical issues found (v2.4.1 process)

**Success Criteria**: Support team ready, monitoring active

**Sign-Off**: [ ] Support Lead

---

### 8.2 Post-Release Retrospective (Week 2)

**Verification Checklist**:

- [ ] **Incident review** — Any issues during release process
- [ ] **Metrics review** — Performance, stability, adoption metrics
- [ ] **Feedback collected** — User feedback on v2.4.0
- [ ] **Lessons learned** — Process improvements for next release
- [ ] **Documentation updates** — Incorporate feedback into docs
- [ ] **Retrospective meeting** — Team discussion & action items

**Success Criteria**: Retrospective completed, action items assigned

**Sign-Off**: [ ] Release Manager

---

## Summary Checklist

**Pre-Release (Week -2 to -1)**:
- [ ] Phase 0: Pre-Release Planning
- [ ] Phase 1: Build & Compilation
- [ ] Phase 2: Test Execution & Coverage

**Release Week (Week 0)**:
- [ ] Phase 3: Performance & Stability
- [ ] Phase 4: Documentation & L1 Compliance
- [ ] Phase 5: Security & Code Quality
- [ ] Phase 6: Release Build & Artifacts

**Post-Release (Week 1+)**:
- [ ] Phase 7: Release Announcement & Deployment
- [ ] Phase 8: Post-Release Support & Retrospective

---

## Sign-Off

| Role | Name | Date | Signature |
|------|------|------|-----------|
| Release Manager | _______________ | ___/___/____ | ___________ |
| QA Lead | _______________ | ___/___/____ | ___________ |
| Security Lead | _______________ | ___/___/____ | ___________ |
| Build Engineer | _______________ | ___/___/____ | ___________ |
| DevOps Lead | _______________ | ___/___/____ | ___________ |
| Documentation Lead | _______________ | ___/___/____ | ___________ |

---

**Release Status**: [ ] READY FOR RELEASE [ ] BLOCKED (document reason below)

**Blocking Issues** (if any):
```
<Describe any blockers preventing release>
```

---

*Last Updated: 2026-07-01*  
*Next Review: v2.5.0 Release Cycle (September 2026)*
