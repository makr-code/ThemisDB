---
name: 🔍 AI Review - Code Quality & Technical Debt
about: Systematische Code-Qualitäts- und Technical-Debt-Analyse / Systematic code quality and technical debt analysis
title: '[CODE-QUALITY-REVIEW] '
labels: ['type:systematic-review', 'area:code-quality', 'area:technical-debt', 'needs-triage']
assignees: ''
---

<!-- 
Wiederholbare Template für Code Quality & Technical Debt Reviews
Repeatable template for code quality and technical debt reviews
Empfohlene Häufigkeit: Quartalsweise / Recommended frequency: Quarterly
-->

## 🎯 Component / Komponente

**Component Name:** <!-- z.B. Query Engine, Storage Layer, API Server -->
**Component Path:** <!-- z.B. src/query/, src/storage/, src/api/ -->
**Review Period:** <!-- z.B. Q1 2026, Version 1.4.x -->
**Reviewer(s):** <!-- Namen der Reviewer -->
**Previous Review:** <!-- Datum des letzten Reviews -->

---

## 📊 Code Metrics / Code-Metriken

### Quantitative Metrics / Quantitative Metriken
- **Total Lines of Code:** 
- **Lines of Code (LOC) - Source:** 
- **Lines of Code - Comments:** 
- **Comment Ratio:** <!-- % -->
- **Number of Files:** 
- **Average File Size:** <!-- Lines -->

### Complexity Metrics / Komplexitäts-Metriken
- **Cyclomatic Complexity (avg):** 
- **Cyclomatic Complexity (max):** 
- **Cognitive Complexity (avg):** 
- **Cognitive Complexity (max):** 
- **Maintainability Index:** <!-- 0-100 scale -->
- **Halstead Complexity:** 

**Complexity Hotspots:** <!-- Files/functions with high complexity -->
1. 
2. 
3. 

---

## 🔍 Code Quality Analysis / Code-Qualitäts-Analyse

### Code Smells / Code-Gerüche
- [ ] **Long Methods** - Methods exceeding 50 lines
- [ ] **Large Classes** - Classes exceeding 500 lines
- [ ] **God Objects** - Classes doing too much
- [ ] **Feature Envy** - Methods using other classes' data excessively
- [ ] **Data Clumps** - Repeated groups of parameters
- [ ] **Primitive Obsession** - Overuse of primitives vs objects
- [ ] **Switch Statements** - Could be replaced with polymorphism
- [ ] **Duplicate Code** - Code duplication detected

**Code Smells Found:**
1. **Smell Type:**
   - Location: 
   - Severity: <!-- High/Medium/Low -->
   - Refactoring Recommendation: 

2. **Smell Type:**
   - Location: 
   - Severity: 
   - Refactoring Recommendation: 

### Design Issues / Design-Probleme
- [ ] **Tight Coupling** - Excessive dependencies between modules
- [ ] **Low Cohesion** - Unrelated functionality in same class
- [ ] **Violation of SRP** - Single Responsibility Principle
- [ ] **Violation of OCP** - Open/Closed Principle
- [ ] **Violation of LSP** - Liskov Substitution Principle
- [ ] **Violation of ISP** - Interface Segregation Principle
- [ ] **Violation of DIP** - Dependency Inversion Principle
- [ ] **Circular Dependencies** - Module A depends on B depends on A

**Design Issues Found:**
1. 
2. 
3. 

---

## 💰 Technical Debt / Technische Schulden

### Technical Debt Inventory / Technical-Debt-Inventar
- **Total Technical Debt:** <!-- Hours/Days/Story Points -->
- **Critical Debt Items:** 
- **High Priority Debt Items:** 
- **Medium Priority Debt Items:** 
- **Low Priority Debt Items:** 

### Debt Categories / Schulden-Kategorien
- **Architecture Debt:** <!-- Issues with system architecture -->
- **Design Debt:** <!-- Poor design decisions -->
- **Code Debt:** <!-- Code quality issues -->
- **Test Debt:** <!-- Missing or inadequate tests -->
- **Documentation Debt:** <!-- Missing or outdated docs -->
- **Infrastructure Debt:** <!-- Build, deployment, tooling issues -->

**Top 5 Technical Debt Items:**
1. **Item 1:**
   - Category: 
   - Impact: <!-- High/Medium/Low -->
   - Effort to Fix: <!-- Hours/Days -->
   - Priority: <!-- P0/P1/P2 -->

2. **Item 2:**
   - Category: 
   - Impact: 
   - Effort to Fix: 
   - Priority: 

3. **Item 3:**
   - Category: 
   - Impact: 
   - Effort to Fix: 
   - Priority: 

4. **Item 4:**
   - Category: 
   - Impact: 
   - Effort to Fix: 
   - Priority: 

5. **Item 5:**
   - Category: 
   - Impact: 
   - Effort to Fix: 
   - Priority: 

### Debt Trends / Schulden-Trends
- **Debt Growth Rate:** <!-- Increasing, Stable, Decreasing -->
- **Debt Paydown Rate:** <!-- How much debt paid per quarter -->
- **Debt to Development Ratio:** <!-- % of time spent on debt vs features -->

---

## 🧹 Code Cleanliness / Code-Sauberkeit

### Naming Conventions / Namenskonventionen
- [ ] **Consistent naming** across codebase
- [ ] **Meaningful names** (not x, tmp, data)
- [ ] **Consistent case** (camelCase, snake_case, PascalCase)
- [ ] **Abbreviations** documented and consistent
- [ ] **Magic numbers** replaced with named constants

**Naming Issues:**


### Code Organization / Code-Organisation
- [ ] **Logical file structure**
- [ ] **Related code** grouped together
- [ ] **Public/private** separation clear
- [ ] **Namespaces** used appropriately
- [ ] **Header/implementation** separation (C++)

**Organization Issues:**


### Code Formatting / Code-Formatierung
- [ ] **Consistent indentation**
- [ ] **Consistent brace style**
- [ ] **Line length** reasonable (< 120 chars)
- [ ] **Whitespace** used consistently
- [ ] **Automated formatter** (clang-format, prettier) used

**Formatting Issues:**


---

## 📝 Comments & Documentation / Kommentare & Dokumentation

### Code Comments / Code-Kommentare
- [ ] **Comments** explain why, not what
- [ ] **No commented-out code**
- [ ] **No obsolete comments**
- [ ] **Complex algorithms** explained
- [ ] **API docs** (Doxygen, JSDoc) complete

**Comment Quality:** <!-- Good/Needs Improvement/Poor -->

**Comment Issues:**


### TODO/FIXME Tracking / TODO/FIXME-Tracking
- **Total TODOs:** 
- **Total FIXMEs:** 
- **Total HACKs:** 
- **Old TODOs (> 6 months):** 

**Critical TODOs:**
1. 
2. 
3. 

---

## 🔒 Error Handling / Fehlerbehandlung

### Error Handling Quality / Fehlerbehandlungs-Qualität
- [ ] **Exceptions** used appropriately
- [ ] **Error codes** checked
- [ ] **Resources** cleaned up properly (RAII, try-finally)
- [ ] **Error messages** clear and actionable
- [ ] **Logging** on errors
- [ ] **No swallowed exceptions**

**Error Handling Issues:**
1. 
2. 
3. 

---

## 🔄 Refactoring Opportunities / Refactoring-Möglichkeiten

### High-Impact Refactorings / High-Impact-Refactorings
1. **Refactoring 1:**
   - Type: <!-- Extract Method, Extract Class, Inline, etc. -->
   - Location: 
   - Benefit: 
   - Effort: <!-- Hours/Days -->
   - Risk: <!-- High/Medium/Low -->

2. **Refactoring 2:**
   - Type: 
   - Location: 
   - Benefit: 
   - Effort: 
   - Risk: 

3. **Refactoring 3:**
   - Type: 
   - Location: 
   - Benefit: 
   - Effort: 
   - Risk: 

### Quick Wins / Schnelle Erfolge
Low-effort, high-impact refactorings (< 1 day):
1. 
2. 
3. 

---

## 🛠️ Static Analysis / Statische Analyse

### Static Analysis Tools / Statische Analyse-Tools
- **Tool:** <!-- cppcheck, SonarQube, ESLint, etc. -->
- **Version:** 
- **Last Run:** 

### Issues Found / Gefundene Probleme
- **Critical Issues:** 
- **High Severity:** 
- **Medium Severity:** 
- **Low Severity:** 
- **Code Smells:** 
- **Bugs:** 
- **Security Vulnerabilities:** 

**Top Static Analysis Issues:**
1. 
2. 
3. 

---

## 📈 Code Duplication / Code-Duplikation

### Duplication Analysis / Duplikations-Analyse
- **Duplication Percentage:** <!-- % of code that is duplicated -->
- **Number of Duplicated Blocks:** 
- **Total Duplicated Lines:** 
- **Largest Duplicate:** <!-- Lines -->

**Duplication Hotspots:**
1. **Location 1:**
   - Files: 
   - Lines: 
   - Refactoring Opportunity: 

2. **Location 2:**
   - Files: 
   - Lines: 
   - Refactoring Opportunity: 

---

## 🔬 Code Review Findings / Code-Review-Ergebnisse

### Recent Code Reviews / Aktuelle Code-Reviews
- **Code Reviews Conducted:** <!-- Last quarter -->
- **Average Review Time:** 
- **Common Issues Found:** 
- **Review Coverage:** <!-- % of changes reviewed -->

**Recurring Issues:**
1. 
2. 
3. 

---

## 🎯 Code Quality Goals / Code-Qualitäts-Ziele

### Current Quality Level / Aktuelles Qualitäts-Niveau
- **Overall Quality:** <!-- Excellent/Good/Fair/Poor -->
- **Maintainability:** <!-- Excellent/Good/Fair/Poor -->
- **Readability:** <!-- Excellent/Good/Fair/Poor -->
- **Testability:** <!-- Excellent/Good/Fair/Poor -->

### Quality Improvement Goals / Qualitäts-Verbesserungs-Ziele
- [ ] Reduce cyclomatic complexity to < 15 (avg)
- [ ] Reduce code duplication to < 5%
- [ ] Increase comment ratio to > 15%
- [ ] Eliminate all critical static analysis issues
- [ ] Pay down 50% of critical technical debt
- [ ] 

---

## 🗺️ Roadmap / Roadmap

### Short-Term (Next 3 Months)
- [ ] Address critical technical debt
- [ ] Eliminate code smells in high-traffic areas
- [ ] Refactor top 3 complexity hotspots
- [ ] 

### Medium-Term (3-6 Months)
- [ ] Systematic refactoring plan
- [ ] Improve test coverage for legacy code
- [ ] Modernize outdated patterns
- [ ] 

### Long-Term (6-12 Months)
- [ ] Architecture improvements
- [ ] Major refactoring initiatives
- [ ] Establish code quality gates
- [ ] 

---

## ✅ Action Items / Aktionspunkte

### Critical (P0)
1. [ ] **Action 1:**
   - Owner: 
   - Due Date: 
   - Description: 

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

### Internal Resources
- [Coding Standards](../CODING_STANDARDS.md)
- [Architecture Documentation](docs/architecture/)
- [Refactoring Guide](docs/development/refactoring.md)

### External Resources
- [Clean Code](https://www.amazon.com/Clean-Code-Handbook-Software-Craftsmanship/dp/0132350882)
- [Refactoring](https://martinfowler.com/books/refactoring.html)
- [Code Smells](https://refactoring.guru/refactoring/smells)
- [Technical Debt](https://martinfowler.com/bliki/TechnicalDebt.html)

---

## 📋 Review Checklist / Review-Checkliste

- [ ] Code metrics collected and analyzed
- [ ] Code smells identified
- [ ] Design issues documented
- [ ] Technical debt inventory updated
- [ ] Code cleanliness assessed
- [ ] Error handling reviewed
- [ ] Refactoring opportunities identified
- [ ] Static analysis run
- [ ] Code duplication measured
- [ ] Quality goals established
- [ ] Action items created and assigned
- [ ] Sign-offs obtained from development team

---

**Review Date:** <!-- YYYY-MM-DD -->
**Next Review:** <!-- YYYY-MM-DD (empfohlen: +3 Monate) -->
**Sign-Off:** <!-- Development Lead, Architecture Team -->

---

**Template Version:** 1.0.0  
**Created:** 2026-02-02  
**Maintained by:** ThemisDB Development Team
