# AI Agent Review Guide

## Purpose

This guide is specifically designed for AI agents conducting systematic reviews of ThemisDB components. It provides clear methodologies, concrete steps, and expected outputs to ensure AI-driven reviews produce actionable, precise results.

---

## Core Principles for AI Reviews

### 1. **Precision over Breadth**
- Focus on specific, verifiable findings
- Provide concrete evidence (file paths, line numbers, code snippets)
- Avoid generic observations without supporting data

### 2. **Actionable Insights**
- Every finding should include a clear action item
- Prioritize findings by impact and effort
- Provide specific remediation steps

### 3. **Research-Backed Analysis**
- Reference specific papers, standards, or best practices
- Compare implementation against state-of-the-art solutions
- Include DOI/links for all cited research

### 4. **Documentation-Code Consistency**
- Always cross-reference documentation with implementation
- Identify discrepancies with specific examples
- Verify code examples in documentation are correct and runnable

---

## Methodology: Source Code Analysis

### Step 1: Codebase Discovery
```
1. Identify component boundaries
   - List all source files: `find src/<component>/ -type f -name "*.cpp" -o -name "*.h"`
   - Count LOC: `cloc src/<component>/`
   - Map dependencies: Check `#include` statements

2. Understand component architecture
   - Identify main classes/modules
   - Map public API surface
   - Identify internal vs external interfaces
```

### Step 2: Code Quality Analysis
```
For each source file:
1. Analyze design patterns used
   - Document pattern name and location (file:line)
   - Verify correct implementation
   - Identify anti-patterns

2. Check modern C++ usage
   - std::optional/variant/expected usage: grep -r "std::optional" src/
   - RAII compliance: Check resource management
   - Smart pointer usage: grep -r "std::unique_ptr\|std::shared_ptr" src/

3. Review error handling
   - Result<T> usage: grep -r "Result<" src/
   - Exception safety guarantees
   - Error propagation paths

4. Verify memory safety
   - No raw pointer arithmetic: grep -r "new \|delete " src/
   - Buffer overflow protection
   - Use-after-free prevention
```

### Step 3: Security Analysis
```
1. Run static analysis tools
   - CodeQL: analyze with security queries
   - cppcheck: cppcheck --enable=all src/<component>/
   - clang-tidy: clang-tidy src/<component>/*.cpp

2. Manual security review
   - Input validation: Check all external inputs
   - Output encoding: Verify sanitization
   - Authentication/Authorization: Verify RBAC integration

3. Vulnerability scanning
   - Check dependencies: grep -r "^#include" | analyze for known CVEs
   - Review cryptographic usage
   - Verify secrets management
```

### Step 4: Performance Analysis
```
1. Profile the component
   - Run benchmarks: ./benchmarks/<component>_bench
   - Identify hotspots: perf record/report
   - Measure memory usage: valgrind --tool=massif

2. Analyze algorithmic complexity
   - Identify critical paths
   - Calculate time complexity: O(?)
   - Calculate space complexity: O(?)

3. Find optimization opportunities
   - Unnecessary copies
   - Inefficient algorithms
   - Cache misses
```

---

## Methodology: Documentation-Code Consistency

### Step 1: Documentation Discovery
```
1. Find all related documentation
   find docs/ -name "*<component>*"
   find examples/ -name "*<component>*"
   grep -r "<component>" docs/ README.md

2. List code examples in documentation
   Extract all code blocks from markdown files
   Identify runnable examples
```

### Step 2: Consistency Verification
```
For each code example:
1. Extract the code snippet
2. Attempt to compile/run it
3. Verify output matches documentation
4. Document discrepancies with:
   - Documentation location
   - Code example
   - Actual behavior
   - Expected behavior
```

### Step 3: API Documentation Verification
```
1. Generate API documentation: doxygen Doxyfile
2. Compare with actual implementation
3. Verify:
   - Function signatures match
   - Parameter descriptions are accurate
   - Return value documentation is correct
   - Example usage is valid
```

### Step 4: Architecture Documentation Check
```
1. Locate architecture diagrams
2. Verify diagram accuracy:
   - Component relationships
   - Data flow
   - API boundaries
3. Update diagrams if outdated
4. Generate new diagrams if missing using mermaid
```

---

## Methodology: Scientific Paper Research

### Step 1: Identify Relevant Research Areas
```
Based on component functionality, identify:
1. Core algorithms used
2. Data structures implemented
3. Protocols/standards followed
4. Performance characteristics claimed
```

### Step 2: Literature Search
```
For each research area:
1. Search academic databases:
   - Google Scholar: <algorithm name> database
   - arXiv.org: <topic>
   - ACM Digital Library
   - IEEE Xplore

2. Prioritize papers by:
   - Citation count (>100 for foundational, >50 for recent)
   - Recency (last 5 years preferred)
   - Relevance to ThemisDB use case
   - Implementation viability

3. For each relevant paper:
   - Title
   - Authors (Year)
   - DOI/Link
   - Key contribution (1-2 sentences)
   - Relevance to ThemisDB (specific)
   - Implementation status
   - Performance characteristics
   - Algorithmic complexity
```

### Step 3: State-of-the-Art Comparison
```
For each paper:
1. Summarize the approach
2. Compare with ThemisDB implementation
3. Identify gaps or improvements
4. Assess applicability:
   - ✅ Already implemented
   - 🟡 Partially implemented  
   - 🔵 Planned
   - ❌ Not applicable (explain why)
```

### Step 4: Competitive Analysis
```
Analyze similar systems:
1. System name
2. Open source? (link if yes)
3. Approach/architecture
4. Strengths vs ThemisDB
5. Weaknesses vs ThemisDB
6. Specific lessons learned
7. Applicable features
```

---

## Methodology: Implementation Roadmap Derivation

### Step 1: Gap Analysis
```
From code analysis, documentation review, and research:
1. List current capabilities
2. List missing features from papers/competitors
3. List technical debt items
4. List security vulnerabilities
5. List performance bottlenecks
```

### Step 2: Prioritization Matrix
```
For each gap/improvement:
1. Impact (Critical/High/Medium/Low)
   - Critical: Security vulnerability, data loss risk
   - High: Major performance issue, core feature missing
   - Medium: Enhancement, nice-to-have feature
   - Low: Minor optimization, convenience feature

2. Effort (High/Medium/Low)
   - High: >2 weeks, requires architecture changes
   - Medium: 3-10 days, localized changes
   - Low: <3 days, simple fixes

3. Dependencies
   - Blocks other work?
   - Requires other components?
   - External dependencies?

4. Risk (High/Medium/Low)
   - High: Breaking changes, data migration
   - Medium: API changes, behavior changes
   - Low: Internal refactoring, additions
```

### Step 3: Roadmap Construction
```
Short-Term (0-3 months):
- Critical/High impact + Low/Medium effort
- Security fixes
- Blocking issues

Medium-Term (3-6 months):
- High/Medium impact + Medium/High effort
- Feature additions
- Performance optimizations

Long-Term (6-12 months):
- Strategic goals
- Architecture improvements
- Research integrations
```

### Step 4: Implementation Plan
```
For each roadmap item:
1. Task description (specific)
2. Success criteria (measurable)
3. Dependencies (list)
4. Estimated effort (person-days)
5. Owner/team assignment
6. Target version
7. Risks & mitigations
8. Testing requirements
9. Documentation requirements
```

---

## Expected Output Format

### Finding Structure
```markdown
### [Category] Finding: [Short Description]

**Severity:** Critical/High/Medium/Low
**Location:** `src/component/file.cpp:123-145`
**Evidence:**
\`\`\`cpp
// Show actual code with issue
\`\`\`

**Impact:**
- Specific impact on functionality/performance/security
- Quantify if possible (e.g., "20% performance loss")

**Root Cause:**
- Explain why the issue exists

**Recommendation:**
- Specific steps to fix
- Include code example if applicable

**References:**
- Link to paper/standard/best practice
- DOI or URL

**Priority:** P0/P1/P2/P3
**Effort:** XS/S/M/L/XL (hours/days)
```

### Research Paper Summary
```markdown
### Paper: [Title]

**Authors:** [Names] ([Year])
**DOI:** [Link]
**Citations:** [Count from Google Scholar]

**Key Contribution:**
[1-2 sentence summary]

**Relevance to ThemisDB:**
[Specific connection to component/feature]

**Algorithmic Approach:**
- Algorithm: [Name]
- Time Complexity: O(?)
- Space Complexity: O(?)
- Key Parameters: [List]

**Implementation in ThemisDB:**
- ✅ Fully implemented in `src/path/`
- 🟡 Partially implemented: [what's missing]
- 🔵 Planned for v[X.Y]
- ❌ Not applicable: [reason]

**Performance Comparison:**
| Metric | Paper | ThemisDB | Notes |
|--------|-------|----------|-------|
| Throughput | X/s | Y/s | |
| Latency (p99) | Xms | Yms | |

**Lessons for ThemisDB:**
1. [Specific actionable item]
2. [Specific actionable item]
3. [Specific actionable item]
```

### Roadmap Item
```markdown
### [Priority] [Category]: [Task Name]

**Description:**
[Detailed description of what needs to be done]

**Success Criteria:**
- [ ] Measurable criterion 1
- [ ] Measurable criterion 2
- [ ] Measurable criterion 3

**Technical Details:**
- Files affected: [list]
- Dependencies: [list]
- Breaking changes: Yes/No
- API changes: Yes/No

**Implementation Steps:**
1. [Specific step]
2. [Specific step]
3. [Specific step]

**Testing Requirements:**
- [ ] Unit tests
- [ ] Integration tests
- [ ] Performance tests
- [ ] Security tests

**Documentation Requirements:**
- [ ] Code comments/docstrings
- [ ] API documentation
- [ ] User guide updates
- [ ] Architecture documentation

**Estimated Effort:** [X person-days]
**Target Version:** v[X.Y.Z]
**Owner:** [Team/Person]
**Dependencies:** [List blocking items]
**Risks:** [List with mitigations]

**References:**
- [Related papers, issues, PRs]
```

---

## Quality Checklist for AI Reviews

Before submitting a review, verify:

- [ ] **Precision**: Every finding has specific evidence (file:line)
- [ ] **Completeness**: All template sections filled with relevant data
- [ ] **Research**: At least 3-5 relevant papers cited with DOIs
- [ ] **Documentation**: All discrepancies documented with examples
- [ ] **Actionability**: Each finding has clear remediation steps
- [ ] **Prioritization**: Items ranked by impact and effort
- [ ] **Quantification**: Performance/metrics included where possible
- [ ] **References**: All claims backed by evidence (code, papers, benchmarks)
- [ ] **Roadmap**: Clear short/medium/long-term plan
- [ ] **Dependencies**: All dependencies and blockers identified

---

## Example: Good vs Bad Findings

### ❌ Bad Finding (Too Generic)
```
The code has some performance issues and should be optimized.
```

### ✅ Good Finding (Specific and Actionable)
```
### Performance Finding: Inefficient Vector Iteration in Query Path

**Severity:** High
**Location:** `src/query/executor.cpp:234-256`
**Evidence:**
\`\`\`cpp
// Current implementation - O(n²) due to nested loops
for (const auto& row : results) {
    for (const auto& col : columns) {
        process(row, col);  // Inefficient lookup
    }
}
\`\`\`

**Impact:**
- 45% of query execution time spent in this loop (profiled with perf)
- Scales poorly: 2.3s for 10K rows, 23s for 100K rows
- Blocks p99 latency improvements

**Root Cause:**
- Nested iteration creates O(n²) complexity
- `columns` lookup is linear search (no indexing)

**Recommendation:**
1. Pre-build column index map: `std::unordered_map<std::string, size_t>`
2. Single-pass iteration with O(1) column lookup
3. Expected improvement: 70% reduction in this hotspot

\`\`\`cpp
// Proposed implementation - O(n)
auto col_index = build_column_index(columns);
for (const auto& row : results) {
    auto col_pos = col_index[target_column];
    process(row, col_pos);  // O(1) lookup
}
\`\`\`

**References:**
- "Efficient Query Processing in Database Systems" - Graefe (IEEE DE Bulletin 1993)
- Similar optimization in PostgreSQL: [commit link]

**Priority:** P1 (High impact, medium effort)
**Effort:** M (3-5 days including tests)
```

---

## Advanced Techniques

### For Complex Components
```
1. Use call graph analysis: cflow, doxygen
2. Generate UML diagrams: doxygen, PlantUML
3. Trace data flow: ltrace, strace
4. Visualize metrics: cloc, SonarQube
```

### For Distributed Systems
```
1. Analyze consensus protocol: Compare with Raft paper
2. Test failure modes: Use chaos engineering (Jepsen)
3. Verify linearizability: Use model checkers
4. Measure replication lag: Monitor metrics
```

### For AI/LLM Components
```
1. Evaluate model performance: BLEU, ROUGE, perplexity
2. Test for bias: Use fairness metrics
3. Check prompt injection: OWASP LLM Top 10
4. Verify embedding quality: Compare with papers
```

---

## Continuous Improvement

After completing a review:
1. Document lessons learned
2. Update this guide with new techniques
3. Add examples of good findings
4. Refine prioritization criteria

---

**Version:** 1.0.0
**Created:** 2026-02-03
**Maintained by:** ThemisDB Core Team
