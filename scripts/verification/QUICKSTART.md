# Documentation TODO Verification - Quick Start Guide

This guide helps you quickly start verifying documentation TODOs using the automated tools.

## Prerequisites

- Python 3.7+
- Access to ThemisDB repository
- grep command (standard on Linux/macOS)

## Quick Start (5 Minutes)

### Step 1: Verify a Single Document

```bash
cd scripts/verification

# Verify the systematic review plan (403 items)
python3 verify_documentation_todos.py \
    --doc=../../docs/SYSTEMATISCHER_REVIEWPLAN.md \
    --output=reviewplan_verification

# View results
less reviewplan_verification.md
```

### Step 2: Review Automated Results

Open `reviewplan_verification.md` to see:
- Summary statistics
- Status breakdown (implemented/gap/partial/doc-only)
- Category breakdown
- Detailed findings with evidence

### Step 3: Manual Verification (for low confidence items)

```bash
# Check JSON report for items needing manual review
cat reviewplan_verification.json | jq '.todos[] | select(.confidence == "low")'

# Verify a specific item in the codebase
grep -r "FeatureName" ../../src/ ../../include/
```

### Step 4: Generate Issues (if gaps found)

```bash
# Generate issue templates from verified gaps
python3 create_issues_from_gaps.py \
    --input=reviewplan_verification.json \
    --output=../../tmp/issues

# Review issues
ls -la ../../tmp/issues/
```

## Common Workflows

### Workflow 1: Verify Top 5 Priority Documents

```bash
cd scripts/verification

# 1. Systematic Review Plan (403 items)
python3 verify_documentation_todos.py \
    --doc=../../docs/SYSTEMATISCHER_REVIEWPLAN.md \
    --output=reports/reviewplan

# 2. TODO List (387 items)
python3 verify_documentation_todos.py \
    --doc=../../docs/de/development/todo.md \
    --output=reports/todo

# 3. Documentation Renewal (220 items)
python3 verify_documentation_todos.py \
    --doc=../../docs/de/development/DOCUMENTATION_RENEWAL_TODO.md \
    --output=reports/doc_renewal

# 4. v1.4 Roadmap
python3 verify_documentation_todos.py \
    --doc=../../docs/v1.4_DEVELOPMENT_ROADMAP.md \
    --output=reports/roadmap

# 5. Enterprise Features (57 items)
python3 verify_documentation_todos.py \
    --doc=../../docs/de/enterprise/ENTERPRISE_FEATURE_ANALYSIS.md \
    --output=reports/enterprise

# Generate comprehensive summary
python3 generate_verification_report.py \
    --input reports/*.json \
    --output=reports/comprehensive_summary.md
```

### Workflow 2: Verify All Documentation

```bash
cd scripts/verification

# Verify all markdown files (may take several minutes)
python3 verify_documentation_todos.py \
    --all \
    --output=reports/all_docs

# Generate summary
python3 generate_verification_report.py \
    --input reports/all_docs.json \
    --output=reports/full_summary.md
```

### Workflow 3: Focus on Specific Category

```bash
# Generate issues for security gaps only
python3 create_issues_from_gaps.py \
    --input=reports/reviewplan.json \
    --category=security \
    --output=../../tmp/issues/security

# Or performance
python3 create_issues_from_gaps.py \
    --input=reports/todo.json \
    --category=performance \
    --output=../../tmp/issues/performance
```

## Understanding the Output

### Status Categories

- **likely_implemented** (99-100% confidence)
  - Feature found in multiple source files
  - Tests likely exist
  - Action: Update documentation to mark complete

- **possible_gap** (0-30% confidence)
  - No code references found
  - May be actual implementation gap
  - Action: Manual verification required

- **partial** (30-70% confidence)
  - Some code references found
  - May need completion
  - Action: Review and assess completion

- **doc-only** (80-100% confidence)
  - Documentation-related keywords detected
  - Not a code implementation task
  - Action: Update or write documentation

### Confidence Levels

- **High**: Automated assessment very likely correct
- **Medium**: Automated assessment probably correct, spot check recommended
- **Low**: Manual verification required

### Categories

- `security` - Security, encryption, PKI, HSM
- `performance` - Optimization, caching, benchmarks
- `llm-ai` - LLM, AI models, inference
- `analytics` - Analytics, process mining, graphs
- `enterprise` - Enterprise features, licensing
- `testing` - Tests, test infrastructure
- `documentation` - Documentation tasks
- `general` - Other items

## Tips for Manual Review

### 1. Check for Alternative Names

```bash
# Feature might use different naming
grep -ri "alternative_name" ../../src/
```

### 2. Check Git History

```bash
# Feature might have been completed
git log --all --grep="feature name" --oneline
```

### 3. Check Tests

```bash
# Tests indicate implementation
find ../../tests -name "*feature*"
grep -r "test.*feature" ../../tests/
```

### 4. Check Related Issues

```bash
# Check if GitHub issue exists
gh issue list --search "feature name"
```

## Interpreting Evidence

Good evidence (likely implemented):
- `feature_name found in src/module/feature.cpp`
- `FeatureClass found in include/module/feature.h`
- `test_feature found in tests/test_feature.cpp`

Weak evidence (manual review needed):
- Generic keywords like "implement", "check", "verify"
- German/non-English word matches
- Partial matches in comments

## Next Steps After Verification

### 1. Update Documentation

For "likely_implemented" items:
```bash
# Edit the documentation file
vim ../../docs/SYSTEMATISCHER_REVIEWPLAN.md

# Change: - [ ] Feature X
# To:     - [x] Feature X
```

### 2. Create GitHub Issues

For "possible_gap" items (after manual verification):
```bash
# Use generated templates
cat ../../tmp/issues/issue_001_security.md

# Create issue via GitHub CLI or web interface
gh issue create --title "..." --body-file "../../tmp/issues/issue_001_security.md"
```

### 3. Archive Outdated Items

For outdated TODOs:
```bash
# Move to archive section or remove
# Add comment explaining why no longer relevant
```

## Troubleshooting

### Issue: Script takes too long

Solution: Process documents individually instead of using `--all`

### Issue: Too many false positives

Solution: Focus on items with "medium" or "high" confidence first

### Issue: Cannot find implementation

Solution: 
1. Check alternative naming conventions
2. Check git history
3. Ask team members
4. Check external dependencies

### Issue: Unclear if implemented

Solution: Use the VERIFICATION_TEMPLATE.md to document findings and get peer review

## Time Estimates

- Single document (100 items): 5-10 minutes automated + 1-2 hours manual review
- Top 5 documents: 30 minutes automated + 1-2 days manual review
- All documentation: 2 hours automated + 1-2 weeks manual review

## Getting Help

- Read full documentation: `README.md`
- Check verification template: `VERIFICATION_TEMPLATE.md`
- Review main issue: GitHub Issue #8
- Ask in project discussions or team chat

## Example Session

```bash
# Complete example workflow
cd /home/runner/work/ThemisDB/ThemisDB/scripts/verification

# Create output directory
mkdir -p reports

# Verify top document
python3 verify_documentation_todos.py \
    --doc=../../docs/SYSTEMATISCHER_REVIEWPLAN.md \
    --output=reports/reviewplan

# Check results
echo "Total TODOs found:"
cat reports/reviewplan.json | jq '.total_todos'

echo "Likely implemented:"
cat reports/reviewplan.json | jq '.statistics.by_status.likely_implemented'

echo "Possible gaps:"
cat reports/reviewplan.json | jq '.statistics.by_status.possible_gap'

# Review markdown report
less reports/reviewplan.md

# If gaps found, generate issues
python3 create_issues_from_gaps.py \
    --input=reports/reviewplan.json \
    --output=../../tmp/issues \
    --min-confidence=medium

# Review issue templates
ls -la ../../tmp/issues/
cat ../../tmp/issues/SUMMARY.md
```

That's it! You're now ready to start verifying documentation TODOs.
