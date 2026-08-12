# Documentation TODO Verification Tools

This directory contains tools for verifying documentation TODO markers against the codebase implementation status.

## Overview

The verification process helps distinguish between:
1. **Already Implemented** - Features that exist in code but aren't marked as complete in docs
2. **Actual Gaps** - Features that need implementation
3. **Partial** - Features with some implementation but incomplete
4. **Documentation-only** - Tasks that are about writing/updating docs, not code
5. **Outdated** - Markers that are no longer relevant

## Tools

### verify_documentation_todos.py

Main verification script that analyzes markdown files and cross-references with the codebase.

**Usage:**

```bash
# Verify a specific document
python3 verify_documentation_todos.py --doc=docs/SYSTEMATISCHER_REVIEWPLAN.md

# Verify all documentation files
python3 verify_documentation_todos.py --all

# Specify custom output location
python3 verify_documentation_todos.py --doc=docs/todo.md --output=todo_verification
```

**Features:**
- Extracts TODO markers (checkboxes, TODO:, TBD:, FIXME:)
- Searches codebase for implementation evidence
- Categorizes items by type (security, performance, llm-ai, etc.)
- Generates both Markdown and JSON reports
- Provides confidence levels for automated assessments

### generate_verification_report.py

Generates comprehensive verification reports from processed data.

### create_issues_from_gaps.py

Creates GitHub issues from verified implementation gaps.

### check_maturity_exit_criteria.py

Checks hard 100%-maturity exit criteria directly from roadmap and maturity-report artifacts.

**Usage:**

```bash
python3 check_maturity_exit_criteria.py \
  --repo-root=/home/runner/work/ThemisDB/ThemisDB \
  --maturity-report=audit/MATURITY_REPORT_2026-08.md \
  --output-json=artifacts/maturity_exit_criteria.json
```

**Current checks:**
- GA blocker metrics (technical and governance) must be `0`
- `Module mit 0 Tests` and `Module mit 0 Benchmarks` must be `0`
- No placeholder/scaffold D1 module rows (`⬛`) in the maturity matrix
- No open `[ ]` or in-progress `[~]` checkbox items in root + `src/*/ROADMAP.md`
- No open compliance gap markers (`fehlend`/`offen`/`ausstehend`) in compliance section

## Workflow

### Phase 1: Initial Verification

Run automated verification on high-priority documents:

```bash
cd scripts/verification

# Verify top priority documents
python3 verify_documentation_todos.py \
    --doc=../../docs/SYSTEMATISCHER_REVIEWPLAN.md \
    --output=reviewplan_verification

python3 verify_documentation_todos.py \
    --doc=../../docs/de/development/todo.md \
    --output=todo_verification

python3 verify_documentation_todos.py \
    --doc=../../docs/v1.4_DEVELOPMENT_ROADMAP.md \
    --output=roadmap_verification
```

### Phase 2: Manual Review

Review the generated reports and update the status:
1. Open `*_verification.json`
2. For each item marked as 'possible_gap' or 'partial':
   - Manually verify in codebase
   - Update status to: 'implemented', 'gap', 'outdated', or 'doc-only'
   - Add notes with file paths or reasoning

### Phase 3: Generate Issues

Create GitHub issues from verified gaps:

```bash
python3 create_issues_from_gaps.py \
    --input=todo_verification.json \
    --create-issues
```

### Phase 4: Update Documentation

Update documentation to reflect current state:
- Mark implemented items as complete
- Remove outdated items
- Create new issues for verified gaps

## Report Format

### Markdown Report Structure

```markdown
# Documentation TODO Verification Report

## Summary Statistics
- Status breakdown by category
- Category breakdown by type
- Confidence levels

## Detailed Findings
For each file:
- List of TODOs by status
- Evidence from codebase
- Categorization and confidence
```

### JSON Report Structure

```json
{
  "generated": "2026-01-11T12:00:00",
  "total_todos": 403,
  "statistics": {
    "by_status": {
      "likely_implemented": 120,
      "possible_gap": 150,
      "partial": 80,
      "doc-only": 53
    },
    "by_category": {
      "security": 45,
      "performance": 30,
      ...
    }
  },
  "todos": [
    {
      "file_path": "docs/SYSTEMATISCHER_REVIEWPLAN.md",
      "line_number": 49,
      "content": "- [ ] Check RocksDB Wrapper implementation",
      "marker_type": "checkbox",
      "status": "likely_implemented",
      "evidence": ["RocksDB found in src/storage/rocksdb_wrapper.cpp"],
      "confidence": "medium",
      "category": "storage",
      "notes": "Found 3 code references"
    }
  ]
}
```

## Configuration

The verifier searches in these directories by default:
- `src/` - Main source code
- `include/` - Header files
- `tests/` - Test files
- `plugins/` - Plugin implementations

## Tips for Manual Review

When manually reviewing automated results:

1. **High Confidence Items** - Likely accurate, spot check a few
2. **Medium Confidence Items** - Review more carefully
3. **Low Confidence Items** - Requires thorough manual verification

### Checking for Implementation

```bash
# Search for specific feature
grep -r "FeatureName" src/ include/

# Check git history
git log --all --grep="FeatureName"

# Check for tests
find tests/ -name "*feature_name*"
```

### Common False Positives

- Documentation mentions existing features (not TODOs)
- Planning items that are deliberately future work
- Aspirational goals vs concrete tasks

### Common False Negatives

- Features implemented but with different naming
- Features in non-standard locations (tools/, scripts/)
- Features in external dependencies

## Output Files

Generated files (should be added to .gitignore):
- `*_verification.md` - Human-readable report
- `*_verification.json` - Machine-readable data
- `verification_summary_*.md` - Aggregated reports

## Integration with CI/CD

This verification can be integrated into CI/CD pipelines:

```yaml
# Example GitHub Action
- name: Verify Documentation TODOs
  run: |
    python3 scripts/verification/verify_documentation_todos.py --all
    # Upload reports as artifacts
```

## Contributing

When adding new verification logic:
1. Update `TodoVerifier` class for extraction logic
2. Update `categorize_todo()` for new categories
3. Update `search_codebase()` for new search patterns
4. Add tests for new functionality

## Related Documentation

- [Issue #8: Verify Documentation TODOs](https://github.com/makr-code/ThemisDB/issues/8)
- Main project CONTRIBUTING.md
- Documentation standards in docs/
