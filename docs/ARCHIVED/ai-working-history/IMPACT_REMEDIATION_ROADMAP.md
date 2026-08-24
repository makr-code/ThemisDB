# ThemisDB Impact-Based Remediation Roadmap

**Date**: 2026-01-XX  
**Objective**: Prioritize and remediate findings across 5 AI-Vibe scanners using dual-axis classification (Severity × Impact)

---

## 1. Executive Summary

### Classification Framework
- **Severity**: How bad is the finding? (CRITICAL, HIGH, MEDIUM, LOW)
- **Impact**: Where does it occur? (CRITICAL modules, HIGH, MEDIUM, LOW, THIRD_PARTY)

### Prioritization Matrix
```
Worst-First Approach:
Priority 1 (🔴 Fix First):    CRITICAL Severity × CRITICAL Impact
Priority 2 (🟠 Fix Soon):     CRITICAL Severity × HIGH Impact  
Priority 3 (🟡 Fix Next):     HIGH Severity × CRITICAL Impact
Priority 4 (🟡 Fix Later):    HIGH Severity × HIGH Impact
Priority 5 (⚪ Fix Eventually): MEDIUM/LOW × Any Impact
```

---

## 2. Module Impact Tiers

### CRITICAL Impact (Core Engine & Security)
- **core/**: Database engine, transaction logic
- **auth/**: Authentication, access control
- **security/**: Encryption, crypto primitives
- **distributed/\*consensus\***: Consensus protocols, distributed transactions
- **include/auth/**, **include/security/**: Public API headers for security
- **Remediation Priority**: Highest - affects all workloads

### HIGH Impact (LLM Integration, Networking, Graph)
- **llm/**: Model loading, inference, prompt handling
- **ai_\**/: AI-related modules
- **network/**: Network protocols, RPC, WebSocket
- **graph/**: Graph query, traversal, indexes
- **model/**: Model management
- **Remediation Priority**: High - affects feature completeness

### MEDIUM Impact (Observability, Protocols)
- **monitoring/**: Metrics, logging, observability
- **multi/**: Multi-GPU support, parallel execution
- **mqtt/**, **kafka/**, **mongo/**: Protocol adapters
- **module/**: Module system, loading
- **Remediation Priority**: Medium - affects operational visibility

### LOW Impact (Utilities, Testing)
- **utils/**, **helper/**: Utilities, string handling
- **logging/**, **format/**: Logging, formatting
- **benchmarks/**: Performance testing
- **Remediation Priority**: Low - affects development velocity

### THIRD_PARTY
- **third_party/**, **external/**, **vendor/**: External dependencies
- **Remediation Priority**: Document and monitor

---

## 3. AI-Vibe Scanner Coverage

### Scanner 1: TODO Production Logic
**Location**: `tools/scanners/gs3_step01_ai_todo_productionlogic.py`

**Detects**: 
- TODO markers in control flow (if, while, for, return)
- TODO_CHECK macros
- Incomplete implementations

**Expected Findings**: ~26-50 per scan  
**Typical Severity**: HIGH (code path not executed) / CRITICAL (control flow)  

**Remediation Strategy**:
```
Priority 1: CRITICAL severity + CRITICAL impact
├─ Fix immediately (blocks release)
├─ Add complete implementation
└─ Add unit test to verify

Priority 2: HIGH severity + HIGH impact
├─ Fix in next sprint
├─ Replace TODO with working code
└─ Test against module integration suite
```

**Example Findings**:
- `distributed_graph.cpp:7` - TODO in arithmetic (HIGH/graph)
- `core/engine.cpp:NNN` - TODO in transaction commit (CRITICAL/core)

---

### Scanner 2: Simulation/Stub Leaks
**Location**: `tools/scanners/gs3_step01_ai_simulation_stub_leak.py`

**Detects**:
- STUB::, MOCK macros in production code
- Mock return patterns
- #ifdef SIMULATION blocks

**Expected Findings**: ~5-15 per scan  
**Typical Severity**: CRITICAL (wrong behavior in production)  

**Remediation Strategy**:
```
ALL FINDINGS = Priority 1 (must fix immediately)
├─ Remove stub/mock from production
├─ Add real implementation or conditional compilation
└─ Ensure no test-only code in release builds
```

**Critical Rule**: No simulation/stub code in production paths  
**Action**: Create GitHub issues for each finding

---

### Scanner 3: LLM Prompt Injection
**Location**: `tools/scanners/gs3_step01_ai_llm_prompt_injection.py`

**Detects**:
- Hardcoded prompts without parameterization
- User input concatenated into prompts
- SQL-like prompt building (string concatenation)

**Expected Findings**: ~10-30 per scan  
**Typical Severity**: HIGH (security vulnerability) / CRITICAL (if exposed to untrusted input)  

**Remediation Strategy**:
```
Priority 1: CRITICAL + CRITICAL/HIGH
├─ Sanitize user input
├─ Use parameterized prompt templates
└─ Add input validation tests

Priority 2: HIGH + CRITICAL/HIGH
├─ Refactor to template-based prompts
├─ Review for injection vectors
└─ Add security tests
```

---

### Scanner 4: Error Handling Consistency
**Location**: `tools/scanners/gs3_step01_ai_error_handling_consistency.py`

**Detects**:
- Unhandled critical operations (CUDA, file I/O)
- Catch(...) without re-throw
- Unchecked function results

**Expected Findings**: ~20-40 per scan  
**Typical Severity**: HIGH (resource leak) / CRITICAL (data corruption)  

**Remediation Strategy**:
```
Priority 1: CRITICAL/HIGH operations in CRITICAL impact
├─ Add proper error handling
├─ Log or propagate errors
└─ Add integration tests

Priority 2: High/Medium in High/Medium impact
├─ Review failure modes
├─ Add context to error handling
└─ Test error paths
```

---

### Scanner 5: Header/Implementation Drift
**Location**: `tools/scanners/gs3_step01_ai_header_drift.py`

**Detects**:
- Missing Doxygen API documentation
- Header/implementation signature mismatches
- Functions declared but not documented

**Expected Findings**: ~50-100 per scan  
**Typical Severity**: MEDIUM (documentation) / HIGH (missing API documentation)  

**Remediation Strategy**:
```
Priority 1: CRITICAL impact + HIGH severity
├─ Add comprehensive Doxygen comments
├─ Document parameters, return values, exceptions
└─ Add code examples

Priority 2: HIGH/MEDIUM impact
├─ Add @brief, @param, @return tags
└─ Generate API documentation
```

---

## 4. Actionable Remediation Plan

### Phase 1: Critical Path (Week 1)
**Goal**: Fix all CRITICAL Severity × CRITICAL Impact findings

1. **Stub/Mock Leaks in Core**
   - Scan for findings with `scanner=Uniform::phase1_ai_simulation_stub_leak` AND `subsystem=core`
   - Action: Remove all STUB/MOCK from production paths
   - Create issue per finding
   - Estimate: 1-2 findings, 2-4 hours fix time

2. **TODO in Critical Control Flow**
   - Find findings with `type=todo_as_productionlogic` AND `severity=CRITICAL` AND `subsystem=core/auth/security`
   - Action: Implement complete logic, add tests
   - Estimate: 3-5 findings, 4-8 hours

### Phase 2: Very High Risk (Week 2)
**Goal**: Fix CRITICAL Severity × HIGH Impact + HIGH Severity × CRITICAL Impact

1. **LLM Prompt Injection in High Impact**
   - Find findings with `type=llm_prompt_injection` AND `severity=CRITICAL` AND `subsystem=llm`
   - Action: Parameterize prompts, sanitize input
   - Estimate: 5-10 findings, 8-12 hours

2. **Error Handling in Network/Graph**
   - Find findings with `type=unchecked_result` AND `subsystem=network/graph`
   - Action: Add error propagation, logging
   - Estimate: 5-15 findings, 6-10 hours

### Phase 3: High Risk (Week 3-4)
**Goal**: Fix remaining HIGH Severity + any impact, MEDIUM Severity × CRITICAL/HIGH Impact

1. **Comprehensive Error Handling**
   - Action: Add try/catch blocks, validate critical operations
   - Estimate: 20-40 findings, 12-20 hours

2. **LLM Security Hardening**
   - Action: Review all prompt handling, add rate limiting
   - Estimate: 10-20 findings, 10-15 hours

### Phase 4: Medium/Low (Month 2)
**Goal**: Documentation, consistency, code quality

1. **API Documentation (Header Drift)**
   - Action: Add Doxygen comments to all public APIs
   - Estimate: 50-100 findings, 20-40 hours

2. **Code Quality**
   - Action: Fix MEDIUM severity issues
   - Estimate: 100-200 findings, 30-50 hours

---

## 5. Metrics & Success Criteria

### Metrics to Track
- **Critical Path Closure**: % of CRITICAL×CRITICAL fixed
- **Time to Fix**: Average hours per finding by impact tier
- **False Positive Rate**: % of findings that are not real issues
- **Regression Rate**: % of findings that re-appear

### Success Criteria
- ✅ 0 CRITICAL Severity × CRITICAL Impact findings in production
- ✅ 0 Stub/Mock leaks in production code
- ✅ 100% error handling on critical operations
- ✅ All public APIs documented with Doxygen
- ✅ All LLM prompt handling parameterized

---

## 6. Governance & Sign-Off

**Approvers**:
- **Security**: Review prompt injection + auth findings
- **Core**: Review database engine + consensus findings  
- **LLM**: Review model/inference findings
- **Network**: Review protocol + distributed findings

**Review Checklist**:
- [ ] Finding is real (not false positive)
- [ ] Root cause identified
- [ ] Solution tested locally
- [ ] No new findings introduced
- [ ] Documentation updated

---

## 7. Follow-Up Actions

1. **Run full codebase scan** with impact classification
   - Command: `python tools/gs3_orchestrator.py src include tests benchmarks --output scan_all_impact.json`
   - Time: 30-60 minutes

2. **Generate impact analysis report**
   - Command: `python generate_impact_report.py`
   - Output: Statistics by subsystem, finding type, priority

3. **Create GitHub issues**
   - One issue per subsystem × priority tier
   - Example: "CRITICAL: Fix 12 TODO production logic findings in core/"

4. **Assign to teams**
   - Security team: Prompt injection + auth findings
   - Core team: Database + consensus findings
   - LLM team: Model + inference findings

---

## Appendix: Finding Types Reference

| Type | Scanner | Severity | Remediation |
|------|---------|----------|-------------|
| todo_as_productionlogic | AI-TODO | HIGH/CRITICAL | Implement complete logic |
| simulation_stub_leak | AI-Stub | CRITICAL | Remove stub/mock, add real impl |
| hardcoded_llm_prompt | AI-LLM | HIGH | Parameterize prompt |
| llm_prompt_injection_risk | AI-LLM | CRITICAL | Sanitize input |
| unchecked_result | AI-Error | HIGH | Add error handling |
| missing_doxygen_brief | AI-Header | MEDIUM | Add @brief tag |
| missing_doxygen_param | AI-Header | MEDIUM | Add @param tags |

