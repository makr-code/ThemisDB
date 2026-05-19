# Phase 3 Complete Workflow: Local Ollama → GitHub PRs → Close Issues

## Overview

This document describes the manual, module-by-module workflow to convert Ollama-generated code into GitHub PRs that close related issues.

**Key Principle:** Every module has:
- 1 GitHub Issue (#5245-#5309)
- 1 Phase 2 Plan (cost/effort estimate)
- 1 Phase 3 Result (generated code via Ollama)
- 1 Git Feature Branch
- 1 Draft PR
- 1 Issue Close (via PR)

---

## Current Status

### Phase 3 Execution (Completed)
- ✅ **INDEX** - 5/5 tasks valid (315.2s)
- ✅ **ANALYTICS** - 4/5 tasks valid (281.1s)
- ✅ **STORAGE** - 5/5 tasks valid (318.8s)
- ⏳ **Remaining 62 modules** - Can be executed incrementally

### GitHub Issues
- Total Issues: 65 (#5245-#5309, one per module)
- Status Tracking: See `python issue_tracker.py --list`

---

## Step-by-Step Workflow for ONE Module

### Example: INDEX (Module #5270)

#### 1. Verify Phase 3 Results Exist
```bash
cat ai_working/phase3_index_results.json | jq '.summary'
```
Expected output:
```json
{
  "module": "index",
  "model": "codellama:latest",
  "tasks_generated": 5,
  "syntax_ok": 5,
  "execution_time": 315.2
}
```

#### 2. Review Generated Code Quality
```bash
# Show code samples for each task
cat ai_working/phase3_index_results.json | jq '.results[] | {id:.task_id, size:.code_length, valid:.syntax_ok}'
```

#### 3. Authenticate with GitHub
```bash
gh auth login
gh auth status
```

#### 4. Check Issue Status
```bash
gh issue view #5270 --repo makr-code/ThemisDB
```

#### 5. Create Feature Branch
```bash
git checkout -b feature/phase3-index-codegen
git branch -u origin/develop
```

#### 6. Commit Generated Code
```bash
# If there are files to commit:
git add <modified_files>
git commit -m 'Phase 3: Ollama code generation for index'
git commit -m 'Implements #5270: Code generation using codellama:latest'

# If no actual file changes (results stored in JSON):
git commit --allow-empty -m 'Phase 3: Ollama generation complete for index (#5270)'
```

#### 7. Create Draft PR
```bash
gh pr create \
  --draft \
  --title "Phase 3: Code Generation - INDEX" \
  --base develop \
  --body-file ai_working/pr_body_index.md
```

**Store PR number from output (e.g., #1234)** as `$PR_NUM`

#### 8. Link Issue to PR
```bash
export PR_NUM=1234  # Replace with actual PR number
gh pr comment $PR_NUM --body "Closes #5270"
```

#### 9. Update Issue with PR Link
```bash
gh issue comment #5270 --body "Phase 3 code generation complete. See PR: github.com/makr-code/ThemisDB/pull/$PR_NUM"
```

#### 10. Add Labels
```bash
gh issue edit #5270 --add-label "phase-3,generated-code,review"
```

#### 11. Verify Setup
```bash
# View PR with linked issue
gh pr view $PR_NUM

# Should show in output: Closes #5270
```

---

## Repeat for Each Module

### Quick Reference Commands

List all modules ready for workflow:
```bash
python issue_tracker.py --list
```

Show workflow for specific module:
```bash
python workflow_gh_cli.py analytics
python workflow_gh_cli.py storage
```

Check GitHub for existing Phase 3 issues:
```bash
python issue_tracker.py --check-gh
```

---

## Workflow for Multiple Modules (Sequential)

For each module in order:
```bash
for MODULE in index analytics storage security content rag
do
  echo "=== Processing $MODULE ==="
  python workflow_gh_cli.py $MODULE
  # Follow manual steps from output
  # Wait for user to complete git/gh commands
done
```

---

## File Organization

All Phase 3 artifacts are in `ai_working/`:

```
ai_working/
├── phase2_batch_results.json           # Phase 2 plans (65 modules)
├── phase3_index_results.json           # Generated code for INDEX
├── phase3_analytics_results.json       # Generated code for ANALYTICS
├── phase3_storage_results.json         # Generated code for STORAGE
├── pr_body_index.md                    # PR description template
├── pr_body_analytics.md                # PR description template
└── ... (more as modules complete)
```

---

## Key Files

**To show workflow:**
```bash
python issue_tracker.py <module>           # Show issue status
python workflow_gh_cli.py <module>         # Show all gh CLI commands
```

**To list issues:**
```bash
python issue_tracker.py --list             # Local issue list
python issue_tracker.py --check-gh         # GitHub actual issues
```

---

## Success Criteria

For each module:
- ✅ Phase 3 code generated (JSON with results)
- ✅ GitHub issue exists (#5245-#5309)
- ✅ Feature branch created
- ✅ Draft PR created with Issue link
- ✅ PR body includes generated code samples
- ✅ Issue updated with PR reference
- ✅ Ready for Copilot review + merge

---

## Troubleshooting

**gh cli not found:**
```bash
# Install gh
# macOS: brew install gh
# Windows: winget install github.cli
# Linux: sudo apt install gh
```

**Authentication failed:**
```bash
gh auth logout
gh auth login --web
gh auth status
```

**PR creation failed:**
```bash
# Check branch is pushed
git push -u origin feature/phase3-<module>-codegen

# Retry PR creation
gh pr create --draft --title "..." --base develop --body-file pr_body_<module>.md
```

**Issue not linking:**
Ensure PR body or first comment contains: `Closes #5270`

---

## Next Phase

Once PR is merged:
1. Run Phase 4: Build verification
   ```bash
   cmake --build --preset windows-release
   ```
2. Run Phase 5: Test verification
   ```bash
   ctest --preset windows-release --output-on-failure
   ```
3. Close issue when merge complete
   ```bash
   gh issue close #5270
   ```

---

**Generated:** 2026-05-19  
**Status:** Phase 3 Manual Integration Ready  
**Modules Completed:** 3 (INDEX, ANALYTICS, STORAGE)  
**Modules Pending:** 62
