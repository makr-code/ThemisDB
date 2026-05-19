# Phase 3 Complete Pipeline: Gap Remediation via Code Generation

## Executive Summary

The scan results (`gap_scan_v2_*.json`) already map **370+ gaps per module to concrete files with line numbers**. Phase 3 should:

1. **Load scan data** for a module
2. **Group gaps by severity** (CRITICAL, HIGH first)
3. **Generate code using Ollama** targeting each gap's specific context
4. **Apply patches** to source files using snippet context
5. **Test compilation** and **commit** changes

## Current State

✓ **Phase 1** (Audit): Identified 109,601 gaps across 65 modules
✓ **Phase 2** (Analysis): Categorized gaps by file + severity + category
✓ **Scan Output**: `ai_working/gap_scan_v2_*.json` contains:
  - Concrete file paths (e.g., `src/index/ann_index.cpp`)
  - Line numbers and snippet context
  - Severity (CRITICAL, HIGH, MEDIUM, LOW)
  - Category (unimplemented, incomplete, stub_documented, platform_fallback, etc.)
  - Metadata (is_test, is_platform_specific, has_documentation)

✗ **Phase 3 (NOW)**: Not yet implemented properly

---

## The Right Workflow (Phase 3)

### Step 1: Load Gap Data
```python
module = "index"
with open(f'ai_working/gap_scan_v2_{module}.json') as f:
    gaps = json.load(f)
```

**Output:** Structured data with files + line numbers + gaps

### Step 2: Filter Gaps by Severity
- **CRITICAL**: 95 gaps for INDEX
- **HIGH**: 96 gaps for INDEX
- Start with CRITICAL only (manageable batch size)

### Step 3: Per-Gap Code Generation

For each CRITICAL gap:

```
Gap: src/index/ann_index.cpp:293 - unimplemented
  Snippet: "if (flat_ids_.empty()) return {};"
  Context: Load surrounding lines (5 before, 5 after)
  
  → Ollama: "Complete this function given the context"
  ← Generated code: Implementation for the gap
```

### Step 4: Patch Source Files

Use the snippet + line number to:
1. Find insertion point in actual source file
2. Insert generated code
3. Preserve formatting/indentation

### Step 5: Compile + Test

```bash
cmake --build --preset windows-release
ctest --preset windows-release --output-on-failure
```

### Step 6: Commit Changes

```bash
git add src/index/*.cpp src/index/*.hpp
git commit -m "Phase 3: Gap remediation for INDEX (CRITICAL severity)

- Fixed 95 CRITICAL gaps in INDEX module
- Implemented missing functions and complete stubs
- All unit tests pass
- See gap_scan_v2_index.json for details"
```

---

## Concrete Example: INDEX Module

### Current Gaps:
```json
{
  "module": "index",
  "stats": {
    "total": 370,
    "unimplemented": 95,      ← START HERE
    "incomplete": 96,          ← THEN HERE
    "severity_critical": 95,
    "severity_high": 96
  },
  "gaps_by_file": {
    "src/index/ann_index.cpp": [
      {
        "file": "src/index/ann_index.cpp",
        "line": 293,
        "category": "unimplemented",
        "severity": "critical",
        "snippet": "if (flat_ids_.empty()) return {};",
        "has_documentation": false
      },
      ...
    ]
  }
}
```

### Process for this gap:
1. Read `src/index/ann_index.cpp`
2. Extract lines 288-298 (context)
3. Send to Ollama:
   ```
   "Complete the function at line 293 given this context:
   [context here]
   
   The gap is: if (flat_ids_.empty()) return {};
   Generate the missing implementation using modern C++20."
   ```
4. Get generated code → validate syntax
5. Insert into file at line 293
6. Continue with next gap

---

## Pipeline Scripts Needed

### 1. `phase3_gap_loader.py`
- Load `gap_scan_v2_*.json`
- Group by severity
- Return structured gap list with file paths + contexts

### 2. `phase3_codegen_targeted.py`
- Per-gap code generation
- Send concrete context to Ollama
- Validate syntax
- Return: `{file, line, original_code, generated_code}`

### 3. `phase3_patcher.py`
- Apply patches to source files
- Preserve formatting
- Create backup
- Rollback on error

### 4. `phase3_validate_build.py`
- Run `cmake --build`
- Parse compiler output
- Report which gaps caused errors
- Update gap status

### 5. `phase3_commit_changes.py`
- Stage changed files
- Create commit message with gap stats
- Push to feature branch

---

## Recommended Execution Order

### Phase 3a: Core Gaps (INDEX, ANALYTICS, STORAGE)
- These 3 modules have the most well-defined structure
- Each: ~95-100 CRITICAL gaps
- Estimated: 2-3 hours per module (Ollama + validation + testing)

### Phase 3b: Query, LLM, RAG (High-Impact Modules)
- Query engine: 14,255 gaps (but many are low-severity)
- LLM integration: 46,775 gaps (platform-specific, many fallbacks)
- RAG pipeline: 14,150 gaps

### Phase 3c: Security, Server (Complex Modules)
- Security: 43,080 gaps (many platform-specific)
- Server: 11,192 gaps

### Phase 3d: Remaining 55 Gap-Remediation Modules
- Distributed across all systems
- Highest success rate with batch automation

---

## Error Handling

### If compilation fails:
1. Identify which gap caused the error
2. Mark gap as "needs_review"
3. Use Ollama to refine: "Fix this compilation error: [error message]"
4. Retry patch + compile

### If Ollama fails to generate valid C++:
1. Manually write the code (flag gap for review)
2. Mark as "manual_fix"
3. Continue with next gap
4. Return to manual gaps later

---

## Success Criteria

✓ **Per-Module Completion:**
- [ ] All CRITICAL gaps fixed (or marked for manual review)
- [ ] Compilation passes (no errors)
- [ ] All unit tests pass
- [ ] Code review completed
- [ ] PR merged

✓ **Quality Gates:**
- Syntax: 95%+ valid on first try
- Compilation: 100% success after fixes
- Tests: 100% pass rate
- Code review: 2 approvals minimum

---

## Next Actions (Priority)

1. **Write `phase3_gap_loader.py`** - Extract gaps from scan files
2. **Refactor `phase3_codegen_targeted.py`** - Per-gap generation with context
3. **Implement `phase3_patcher.py`** - Safe file patching
4. **Run INDEX module pilot** - Test full pipeline
5. **Scale to ANALYTICS, STORAGE**
6. **Batch execute remaining 62 modules**

---

## Timeline Estimate

| Task | Effort | Owner |
|------|--------|-------|
| Pipeline setup (scripts 1-5) | 1-2 hours | Automation |
| INDEX pilot execution | 2-3 hours | Phase 3 |
| ANALYTICS + STORAGE | 4-6 hours | Phase 3 |
| Remaining 62 modules | 12-16 hours | Batch automation |
| **Total** | **19-27 hours** | |

---

## Open Questions

1. Ollama timeout for large context windows?
2. How do we handle multi-line patches (gaps spanning 5+ lines)?
3. Should we group related gaps or process independently?
4. Rollback strategy if compilation fails mid-batch?

