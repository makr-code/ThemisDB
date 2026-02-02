---
name: 🧪 AI Review - Testing & Quality Assurance
about: Systematische Test-Coverage- und Qualitäts-Review / Systematic testing and quality assurance review
title: '[TEST-REVIEW] '
labels: ['type:systematic-review', 'area:testing', 'area:quality', 'needs-triage']
assignees: ''
---

<!-- 
Wiederholbare Template für Test-Coverage und Quality-Reviews
Repeatable template for test coverage and quality reviews
Empfohlene Häufigkeit: Quartalsweise / Recommended frequency: Quarterly
-->

## 🎯 Component / Komponente

**Component Name:** <!-- z.B. Query Engine, Vector Index, HTTP API -->
**Component Path:** <!-- z.B. src/query/, src/index/, src/api/ -->
**Review Period:** <!-- z.B. Q1 2026, Version 1.4.x -->
**Reviewer(s):** <!-- Namen der Reviewer -->
**Previous Review:** <!-- Datum des letzten Reviews -->

---

## 📊 Test Coverage Metrics / Test-Coverage-Metriken

### Overall Coverage / Gesamt-Coverage
- **Line Coverage:** <!-- z.B. 85% -->
- **Branch Coverage:** <!-- z.B. 78% -->
- **Function Coverage:** <!-- z.B. 92% -->
- **Statement Coverage:** <!-- z.B. 87% -->

### Coverage by Test Type / Coverage nach Test-Typ
- **Unit Test Coverage:** 
- **Integration Test Coverage:** 
- **End-to-End Test Coverage:** 
- **API Test Coverage:** 

### Coverage Trends / Coverage-Trends
- **Previous Review Coverage:** <!-- z.B. 80% -->
- **Current Coverage:** <!-- z.B. 85% -->
- **Trend:** <!-- ↗️ Increasing, ↘️ Decreasing, → Stable -->

---

## 🧪 Test Suite Analysis / Test-Suite-Analyse

### Test Inventory / Test-Inventar
- **Total Tests:** 
- **Unit Tests:** 
- **Integration Tests:** 
- **End-to-End Tests:** 
- **Performance Tests:** 
- **Security Tests:** 
- **Fuzz Tests:** 
- **Property-based Tests:** 

### Test Execution / Test-Ausführung
- **Average Test Duration:** 
- **Slowest Tests:** <!-- Top 5 -->
  1. 
  2. 
  3. 
  4. 
  5. 
- **Flaky Tests:** <!-- Count and list -->
- **Skipped/Ignored Tests:** <!-- Count and reason -->

---

## 🔍 Test Quality Assessment / Test-Qualitäts-Bewertung

### Unit Tests / Unit-Tests
- [ ] **Test independence** - Tests don't depend on each other
- [ ] **Test readability** - Clear, well-named tests
- [ ] **Proper assertions** - Not just checking for no exceptions
- [ ] **Edge cases** covered
- [ ] **Error conditions** tested
- [ ] **Mock usage** appropriate
- [ ] **Test setup** clean and minimal

**Issues Found:**
1. 
2. 
3. 

### Integration Tests / Integrations-Tests
- [ ] **Critical paths** covered
- [ ] **Component interactions** tested
- [ ] **External dependencies** properly mocked or containerized
- [ ] **Database transactions** handled correctly
- [ ] **API contracts** validated
- [ ] **Error propagation** tested

**Issues Found:**
1. 
2. 
3. 

### End-to-End Tests / End-to-End-Tests
- [ ] **User scenarios** covered
- [ ] **Complete workflows** tested
- [ ] **Multi-component interactions** validated
- [ ] **Performance** acceptable
- [ ] **Cleanup** after tests
- [ ] **Environment setup** automated

**Issues Found:**
1. 
2. 
3. 

---

## 🚨 Test Gaps / Test-Lücken

### Critical Gaps (Must Address) / Kritische Lücken
1. **Gap 1:**
   - Description: 
   - Impact: <!-- High/Critical -->
   - Coverage: <!-- z.B. 0%, untested -->

2. **Gap 2:**
   - Description: 
   - Impact: 
   - Coverage: 

3. **Gap 3:**
   - Description: 
   - Impact: 
   - Coverage: 

### High Priority Gaps / Hochprioritäts-Lücken
1. 
2. 
3. 

### Medium Priority Gaps / Mittelprioritäts-Lücken
1. 
2. 
3. 

---

## 🐛 Bug Detection & Prevention / Bug-Erkennung & -Prävention

### Bug Analysis / Bug-Analyse
- **Bugs found in review period:** 
- **Bugs caught by tests:** <!-- Percentage -->
- **Bugs escaped to production:** 
- **Average time to detect:** 

### Common Bug Categories / Häufige Bug-Kategorien
- [ ] Null pointer/reference errors
- [ ] Off-by-one errors
- [ ] Race conditions
- [ ] Memory leaks
- [ ] Resource leaks (file handles, connections)
- [ ] Logic errors
- [ ] API contract violations
- [ ] Data validation errors

**Most Common Bug Type:**


---

## 🏗️ Test Infrastructure / Test-Infrastruktur

### Test Frameworks & Tools / Test-Frameworks & -Tools
- **Unit Test Framework:** <!-- z.B. Google Test, Catch2 -->
- **Integration Test Framework:** 
- **Mocking Framework:** <!-- z.B. Google Mock -->
- **Property Testing:** <!-- z.B. RapidCheck -->
- **Fuzzing:** <!-- z.B. AFL++, libFuzzer -->
- **Load Testing:** <!-- z.B. k6, JMeter -->
- **Code Coverage Tool:** <!-- z.B. lcov, gcov -->

**Tool Issues:**


### CI/CD Integration / CI/CD-Integration
- [ ] **Tests run on every commit**
- [ ] **Test results** reported clearly
- [ ] **Failing tests** block merges
- [ ] **Coverage reports** generated
- [ ] **Performance regression** tests
- [ ] **Test parallelization** enabled
- [ ] **Test result caching** implemented

**CI/CD Issues:**


---

## 🎯 Test Best Practices / Test-Best-Practices

### Code Review / Code-Review
- [ ] **Tests reviewed** with code changes
- [ ] **Test quality** checked in reviews
- [ ] **Coverage requirements** enforced
- [ ] **Test naming conventions** followed

### Test Maintenance / Test-Wartung
- [ ] **Old tests** refactored when needed
- [ ] **Duplicate tests** removed
- [ ] **Test data** managed properly
- [ ] **Test fixtures** reusable
- [ ] **Test documentation** up-to-date

**Maintenance Issues:**


---

## 🔒 Security Testing / Sicherheitstests

### Security Test Coverage / Sicherheitstest-Coverage
- [ ] **Input validation** tests
- [ ] **SQL/NoSQL injection** tests
- [ ] **XSS** prevention tests
- [ ] **CSRF** protection tests
- [ ] **Authentication** tests
- [ ] **Authorization** tests
- [ ] **Encryption** tests
- [ ] **Secret management** tests

**Security Testing Gaps:**
1. 
2. 
3. 

### Fuzzing / Fuzzing
- [ ] **Fuzz tests** implemented
- [ ] **Input corpus** maintained
- [ ] **Crashes** tracked and fixed
- [ ] **Coverage-guided** fuzzing

**Fuzzing Status:**


---

## ⚡ Performance Testing / Performance-Tests

### Performance Test Coverage / Performance-Test-Coverage
- [ ] **Load tests** for critical paths
- [ ] **Stress tests** for limits
- [ ] **Endurance tests** for memory leaks
- [ ] **Spike tests** for sudden load
- [ ] **Scalability tests** for horizontal scaling

**Performance Testing Status:**


### Performance Regression Detection / Performance-Regressions-Erkennung
- [ ] **Baseline metrics** established
- [ ] **Automated regression** detection
- [ ] **Performance budgets** defined
- [ ] **Alerts** on regressions

**Performance Regression Issues:**


---

## 📈 Quality Metrics / Qualitätsmetriken

### Code Quality / Code-Qualität
- **Cyclomatic Complexity (avg):** 
- **Code Churn:** <!-- Lines changed recently -->
- **Technical Debt:** <!-- Hours/Days estimate -->
- **Code Smells:** <!-- Count from SonarQube/similar -->

### Defect Metrics / Defekt-Metriken
- **Defect Density:** <!-- Bugs per KLOC -->
- **Defect Removal Efficiency:** <!-- % found in testing vs production -->
- **Mean Time to Detect (MTTD):** 
- **Mean Time to Repair (MTTR):** 

---

## 🗺️ Testing Roadmap / Test-Roadmap

### Short-Term (Next 3 Months)
- [ ] Close critical test gaps
- [ ] Improve flaky test stability
- [ ] Add missing integration tests
- [ ] Increase coverage to: <!-- Target % -->

### Medium-Term (3-6 Months)
- [ ] Implement property-based testing
- [ ] Add comprehensive fuzzing
- [ ] Improve test performance
- [ ] Automate more E2E scenarios

### Long-Term (6-12 Months)
- [ ] Achieve 90%+ coverage goal
- [ ] Implement chaos engineering
- [ ] Add performance testing pipeline
- [ ] Full security test automation

---

## ✅ Action Items / Aktionspunkte

### Critical (P0) - Must Address Immediately
1. [ ] **Action 1:**
   - Description: 
   - Owner: 
   - Due Date: 
   - Expected Coverage Improvement: 

2. [ ] **Action 2:**
   - Description: 
   - Owner: 
   - Due Date: 
   - Expected Coverage Improvement: 

### High Priority (P1)
1. [ ] **Action 1:**
   - Description: 
   - Owner: 
   - Due Date: 

2. [ ] **Action 2:**
   - Description: 
   - Owner: 
   - Due Date: 

### Medium Priority (P2)
1. [ ] **Action 1:**
   - Description: 
   - Owner: 
   - Due Date: 

---

## 📚 References / Referenzen

### Internal Documentation
- [Testing Guidelines](docs/testing/)
- [Test Coverage Report](coverage/)
- [CI/CD Pipeline](docs/ci-cd/)

### External Resources
- [Test Pyramid](https://martinfowler.com/articles/practical-test-pyramid.html)
- [Google Testing Blog](https://testing.googleblog.com/)
- [Effective Testing](https://www.effectivetestdesign.com/)

---

## 📋 Review Checklist / Review-Checkliste

- [ ] Coverage metrics collected and analyzed
- [ ] Test quality assessed across all test types
- [ ] Critical test gaps identified
- [ ] Flaky and slow tests documented
- [ ] Security testing coverage reviewed
- [ ] Performance testing coverage reviewed
- [ ] Action items created with owners and deadlines
- [ ] Roadmap updated with testing priorities
- [ ] Sign-offs obtained from QA and development teams

---

**Review Date:** <!-- YYYY-MM-DD -->
**Next Review:** <!-- YYYY-MM-DD (empfohlen: +3 Monate) -->
**Sign-Off:** <!-- QA Lead, Development Lead, CI/CD Team -->

---

**Template Version:** 1.0.0  
**Created:** 2026-02-02  
**Maintained by:** ThemisDB QA Team
