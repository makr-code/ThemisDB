# Phase 3 PoC Validation Decision Framework

## Objective
Validate that Ollama code generation is **safe and reliable** before unleashing it on the real codebase (67K gaps across 65 modules).

## Decision Matrix

| Quality Score | Ollama Status | Decision | Action |
|--------------|---------------|----------|--------|
| **>80** | Excellent | ✅ APPROVED for production | Execute Phase 3 immediately on all modules |
| **70-80** | Good | ✅ APPROVED with review | Execute Phase 3, require Copilot review before merge |
| **60-70** | Fair | ⚠️ CONDITIONAL | Execute Phase 3 on high-priority modules only, manual code review mandatory |
| **50-60** | Poor | ❌ NOT APPROVED | Refine prompts or use different model, test again |
| **<50** | Very Poor | ❌ BLOCKED | Do not use Ollama, revert to manual coding |

---

## Quality Score Components (Weighted)

```
Total Score = 100 points

Documentation (20 pts)
  - Has Doxygen comments: +20
  - Has inline comments: +10
  - No comments: 0

Modern C++ (20 pts)
  - Uses auto, constexpr, nullptr, concepts: +20
  - Partial modern C++ usage: +10
  - Old C++ style: 0

Memory Safety (20 pts)
  - No raw pointers: +20
  - Smart pointers only: +15
  - Some raw pointers: +5
  - Many raw pointers: 0

Completeness (20 pts)
  - No TODOs/FIXMEs: +20
  - <5% incomplete: +15
  - >5% incomplete: 0

Error Handling (20 pts)
  - Exception-safe + error handling: +20
  - Partial error handling: +10
  - No error handling: 0
```

---

## Validation Checklist

### Stage 1: Environment ✓
- [ ] Ollama installed (`ollama --version` works)
- [ ] Service running (can connect to localhost:11434)
- [ ] Model downloaded (`ollama list` shows deepseek-coder-v2:16b)
- [ ] API responding correctly

### Stage 2: Code Generation ✓
- [ ] Ollama generates valid C++ code
- [ ] Generation completes in <2 minutes
- [ ] Output is substantial (>500 characters)

### Stage 3: Syntax ✓
- [ ] Braces balanced
- [ ] Includes present
- [ ] No obvious syntax errors
- [ ] C++ constructs valid

### Stage 4: Compilation ✓
- [ ] Code compiles (MSVC/clang)
- [ ] No critical errors
- [ ] Warnings acceptable (<5)

### Stage 5: Quality ✓
- [ ] Quality score calculated
- [ ] Score >= 60 (minimum for conditional approval)

---

## Decision Logic

```
IF ollama NOT installed OR running:
  → DECISION: BLOCKED
  → ACTION: Install Ollama first (15 min setup)

ELSE IF code generation fails:
  → DECISION: BLOCKED
  → ACTION: Check Ollama API, try different model

ELSE IF syntax or compilation fails:
  → DECISION: NOT_APPROVED (score < 50)
  → ACTION: Refine prompts, possibly switch to codellama:34b

ELSE IF quality score >= 70:
  → DECISION: APPROVED
  → ACTION: Execute Phase 3 with standard workflow
           (Ollama draft → Copilot review → build → test → merge)

ELSE IF quality score 60-70:
  → DECISION: CONDITIONAL
  → ACTION: Execute Phase 3 with enhanced review
           (Ollama draft → MANDATORY Copilot review → MANDATORY code review → build → test → merge)

ELSE (score < 60):
  → DECISION: NOT_APPROVED
  → ACTION: Do NOT use Ollama, return to manual coding
           OR: Try smaller model (qwen2.5:7b) and re-validate
```

---

## Success Criteria per Stage

### Stage 1: Environment
- **SUCCESS**: All 4 checks pass
- **ACCEPTABLE**: 3/4 checks pass (can install missing components)
- **FAILURE**: <2/4 checks pass (requires setup work)

### Stage 2: Generation
- **SUCCESS**: Code generated, valid syntax, >500 chars
- **FAILURE**: Generation fails or timeout

### Stage 3: Syntax
- **SUCCESS**: All checks pass
- **WARNING**: 1-2 minor issues
- **FAILURE**: 3+ major issues

### Stage 4: Compilation
- **SUCCESS**: Compiles with 0-1 warnings
- **WARNING**: Compiles with 2-5 warnings
- **FAILURE**: Compilation fails or >5 warnings

### Stage 5: Quality
- **EXCELLENT** (>80): Production-ready
- **GOOD** (70-80): Needs light review
- **FAIR** (60-70): Needs thorough review
- **POOR** (<60): Needs significant work or different approach

---

## What This Means for Phase 3 Rollout

### If APPROVED (score ≥ 70):

```
PHASE 3 WORKFLOW:

Task 1: Generate Code (Ollama)
  - Time: <5 min
  - Output: Draft implementation

Task 2: Review (Copilot)
  - Time: 5-15 min
  - Check: Logic, patterns, edge cases

Task 3: Validate (Build + Test)
  - Time: 10-20 min
  - Check: No regressions, tests pass

Task 4: Integrate (Commit)
  - Time: <5 min
  - Output: Merged to develop

TOTAL per task: 30-45 minutes
TOTAL for 65 modules: ~35-50 hours (vs 2,600+ hours manual)
SAVINGS: 98%
```

### If CONDITIONAL (score 60-70):

```
SAME AS ABOVE, but:
  - Require GitHub code review (2 approvers)
  - Run full test suite (not just module tests)
  - Manual inspection of critical sections
  - Each commit requires documented rationale

TOTAL per task: 45-60 minutes
TOTAL for 65 modules: ~50-65 hours
SAVINGS: 97%
```

### If NOT APPROVED (score <60):

```
DO NOT PROCEED with Ollama

OPTIONS:
1. Try qwen2.5-coder:7b (smaller, may be more careful)
2. Switch to codellama:34b (larger, but slower)
3. Use Copilot exclusively (cloud, no local model)
4. Return to manual coding + GitHub Copilot chat

Recommendation: Either improve prompts or use manual coding
with Copilot assist for lower risk.
```

---

## Running the PoC Validation

### Quick Start (If Ollama Ready)

```bash
# 1. Verify Ollama running
ollama list

# 2. Run validator
python tools/auto_phase3_validator_poc.py

# 3. Wait for results (2-3 minutes)

# 4. Check reports
cat ai_working/phase3_validation_report.json

# 5. Review generated code
cat ai_working/phase3_poc_generated.cpp
```

### If Ollama Not Ready

```bash
# 1. Install Ollama
winget install ollama
# OR: https://ollama.com/download/windows

# 2. Start service
ollama serve

# 3. Download model (in another terminal, ~10 min)
ollama pull deepseek-coder-v2:16b

# 4. Verify
ollama list

# 5. Then run validator (see above)
```

---

## Escalation Path

### If validator returns BLOCKED or NOT_APPROVED:

1. **Check environment**
   ```bash
   ollama --version
   ollama list
   curl http://localhost:11434/api/tags
   ```

2. **Review generated code**
   ```bash
   cat ai_working/phase3_poc_generated.cpp
   ```

3. **Check validation report**
   ```bash
   cat ai_working/phase3_validation_report.json | jq '.stages'
   ```

4. **Options**:
   - Fix Ollama setup issues
   - Try different model (`ollama pull codellama:34b`)
   - Refine prompts in validator
   - Escalate to manual review

---

## Recommendation

✅ **Run the PoC validator FIRST**

Only proceed to Phase 3 production if:
- Stage 1: ALL checks pass ✓
- Stage 2: Code generation works ✓
- Stage 3: Syntax valid ✓
- Stage 5: Quality score ≥ 60 ✓

This ensures we don't waste time or create technical debt by deploying low-quality AI-generated code.

**Safety first, scale second.** 🛡️

---

## Timeline

1. **Today** (May 19)
   - Run PoC validator
   - Get quality score
   - Make go/no-go decision

2. **If APPROVED** (May 19-20)
   - Execute Phase 3 on LLM module (PoC)
   - Review + test results
   - Decide on batch execution

3. **If APPROVED for batch** (May 20-21)
   - Execute on Server, Query, Sharding modules
   - Validate results
   - Begin integration

4. **If needs work** (May 19+)
   - Refine prompts
   - Try alternative models
   - Re-validate before production

---

**Next Step**: Execute `python tools/auto_phase3_validator_poc.py` and check results.
