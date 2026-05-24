# Enhanced Gap Scanner & AI-Agent Ready Issues

## Overview

This toolset generates detailed GitHub issues from gap analysis results, specifically designed for AI-agent execution with concrete acceptance criteria, scope boundaries, and implementation instructions.

## Components

### 1. Enhanced Template Generator
**File:** `gap_issue_enhanced_template_generator.py`

Transforms gap analysis results into detailed issue templates with:
- **Specific patterns** to fix (not generic guidance)
- **Acceptance criteria** per gap category (Security, Memory, Reliability, etc.)
- **Test requirements** (unit, integration, sanitizer tests)
- **Scope definitions** (what's IN scope, what's OUT)
- **AI-agent execution instructions** (step-by-step fix procedures)

**Usage:**
```bash
python tools/gap_issue_enhanced_template_generator.py
```

**Output:** `ai_working/enhanced_issues/*.md` (one file per module)

### 2. Integrated Pipeline
**File:** `gap_scanner_enhanced_pipeline.py`

Orchestrates the complete workflow:
1. Load gap analysis results
2. Generate enhanced templates
3. Prepare GitHub issues
4. Optionally create issues on GitHub

**Usage:**
```bash
# Generate templates only (no GitHub changes)
python tools/gap_scanner_enhanced_pipeline.py

# Create issues on GitHub (dry run first!)
python tools/gap_scanner_enhanced_pipeline.py --dry-run
python tools/gap_scanner_enhanced_pipeline.py --github
```

## Issue Template Structure

Each generated issue includes:

### Executive Summary
- Total gaps, severity breakdown (CRITICAL/HIGH/MEDIUM)
- Estimated effort in hours/days/weeks
- Priority level

### Gap Breakdown by Category
For each category (Security, Memory, Reliability, etc.):
- **Patterns:** Specific vulnerable code patterns
- **Acceptance Criteria:** What must be true after fix
- **Test Requirements:** How to verify the fix
- **Scope Definition:** IN scope (fix here) vs OUT of scope (handle separately)

### High-Impact Files
Priority-ordered list of files to fix, with:
- Number and severity of gaps per file
- Category breakdown
- Remediation checkbox

### AI Agent Execution Instructions
Step-by-step procedure for implementation:
1. Locate all instances
2. Implement fixes (category-specific)
3. Verify fixes (tests, sanitizers)
4. Submit PR (checklist)

### Code Review Checklist
Verification points before merge:
- Acceptance criteria met
- Tests cover error cases
- No compiler warnings
- Memory/thread safety verified
- Documentation updated

## Gap Categories with Detailed Guidance

### 🔒 Security (CWE-78/89/79)
- Unsafe string functions
- Hardcoded secrets
- SQL/command injection
- Input validation gaps

### 💾 Memory (CWE-401/416/119)
- Memory leaks
- Use-after-free
- Buffer overflows
- Smart pointer migrations

### ⚡ Reliability (CWE-252/391)
- Ignored error codes
- Missing timeouts
- Incomplete retry logic
- Error propagation

### 🔄 Concurrency (CWE-362/366)
- Data races
- Deadlock risks
- Missing synchronization
- Lock ordering issues

### 📦 RAII (CWE-404/460)
- Missing destructors
- Non-virtual cleanup
- Exception-unsafe paths

### 📋 Container (CWE-1104/831)
- O(n²) patterns
- Missing reserves
- Inefficient operations

### 🌍 Platform (CWE-758/1007)
- Portability gaps
- Hardcoded paths
- Endianness issues

### ⚙️ Performance (CWE-1104)
- String concat loops
- Regex recompilation
- Lock contention

## Example: Enhanced Issue for Security Category

```markdown
# 🔴 CRITICAL — SECURITY Module Gap Analysis

## Executive Summary
| Total Gaps | 156 |
| 🔴 CRITICAL | 12 (~8%) |
| 🟠 HIGH | 89 (~57%) |
| 🟡 MEDIUM | 55 (~35%) |
| Estimated Effort | 3.2 weeks |

## Security Vulnerabilities (156 gaps)

### Patterns to Fix
- [ ] Unsafe string functions (strcpy, sprintf, gets)
- [ ] Hardcoded credentials/API keys/secrets
- [ ] SQL injection via unsanitized queries
- [ ] Command injection via system calls
- [ ] Unchecked user input validation

### Acceptance Criteria
- [ ] All unsafe C functions replaced with safe alternatives
- [ ] Secrets moved to environment variables
- [ ] User input validated against whitelist
- [ ] SQL queries use parameterized statements
- [ ] Command execution uses vector<string> argv

### Test Requirements
- [ ] Unit tests for input validation (normal, boundary, malicious)
- [ ] Integration tests for secret handling
- [ ] Fuzzing harness for injection vectors
- [ ] Security code review completed

### Scope Definition
**IN SCOPE:**
- Direct function calls in user-facing APIs
- Input processing paths (HTTP, CLI, config files)
- Credential/API key storage
- Database query construction

**OUT OF SCOPE:**
- Third-party library vulnerabilities
- Cryptographic algorithms
- Network protocol security

## High-Impact Files
1. `src/security/auth.cpp` — 34 gaps (🔴 5, 🟠 12)
2. `src/security/validator.cpp` — 28 gaps (🔴 3, 🟠 18)
...

## 🤖 AI Agent Instructions

### Phase 1: Locate All Instances
- Use detailed gap output: `ai_working/gap_scan_v3_security.json`
- Grep for patterns: strcpy, sprintf, hardcoded paths

### Phase 2: Implement Fixes
- [ ] Replace unsafe functions with std::string alternatives
- [ ] Inject secrets via environment variables
- [ ] Use parameterized SQL queries
- [ ] Add input validation layer
- [ ] Write unit test for each fix

### Phase 3: Verify
- [ ] All tests passing
- [ ] No new compiler warnings
- [ ] No AddressSanitizer errors
- [ ] Security code review passed

### Phase 4: Submit PR
- Title: "Fix: Security hardening in security module — <specifics>"
- Link to this issue
- Include before/after code samples
```

## Integration with Gap Scanner Pipeline

The enhanced templates work with the existing gap scanner:

```bash
# 1. Run full gap analysis
python tools/gap_scanner_v3.py

# 2. Generate enhanced issue templates
python tools/gap_issue_enhanced_template_generator.py

# 3. Create GitHub issues with AI-agent ready details
python tools/gap_scanner_enhanced_pipeline.py --github
```

## Benefits for AI Agents

1. **Concrete Tasks:** Not "fix security gaps" but specific patterns to address
2. **Measurable Success:** Acceptance criteria clearly defined
3. **Clear Scope:** What's included, what's out of scope
4. **Implementation Guide:** Step-by-step procedure
5. **Verification Checklist:** How to ensure fix is correct
6. **Test Requirements:** What tests must pass

## CLI Usage

```bash
# Generate enhanced templates only (no GitHub changes)
python tools/gap_scanner_enhanced_pipeline.py

# Dry run (preview GitHub issues without creating)
python tools/gap_scanner_enhanced_pipeline.py --dry-run

# Create issues on GitHub (requires 'gh' CLI + auth)
python tools/gap_scanner_enhanced_pipeline.py --github

# Verbose output
python tools/gap_scanner_enhanced_pipeline.py --github --verbose
```

## Output Artifacts

### Templates
- `ai_working/enhanced_issues/<module>_enhanced_issues.md` — Detailed issue template

### Logs
- `ai_working/github_issues_created.csv` — Issue creation log (if --github used)
- `ai_working/github_issues_creation_errors.log` — Error logs

## Next Steps

1. Review generated enhanced templates in `ai_working/enhanced_issues/`
2. Adjust category descriptions if needed (edit `gap_issue_enhanced_template_generator.py`)
3. Create GitHub issues with `--github` flag
4. Assign to AI agents or teams
5. Track execution with issue labels (gap-scanner, ai-agent-ready)

## Customization

To modify acceptance criteria or add new categories:

1. Edit `gap_issue_enhanced_template_generator.py`
2. Add/update entries in `GAP_CATEGORIES` dict
3. Regenerate templates
4. Re-create GitHub issues

Example:
```python
'custom_category': GapCategory(
    name='Custom Category Name',
    cwe='CWE-123',
    description='...',
    patterns=[...],
    acceptance_criteria=[...],
    # ... etc
)
```

## Troubleshooting

**Q: Templates not generated?**  
A: Ensure `gap_scan_v3_aggregate.json` exists in `ai_working/`

**Q: Can't create GitHub issues?**  
A: Verify `gh` CLI is authenticated: `gh auth status`

**Q: Issues too long?**  
A: GitHub issue body limit is 65,536 characters. Split large modules if needed.

---

**Status:** ✅ Ready for AI-Agent Execution  
**Last Updated:** 2026-05-19  
**Contact:** Code Quality Team
