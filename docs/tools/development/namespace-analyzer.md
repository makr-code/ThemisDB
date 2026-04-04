# namespace_analyzer.py - C++ Codebase Structure Analyzer

## Overview

`namespace_analyzer.py` is a powerful analysis tool that examines the ThemisDB C++ codebase to extract and document namespaces, classes, functions, and variables. It supports Git metadata integration to track when components were introduced and modified, making it invaluable for understanding code evolution.

## Use Cases

- **Codebase Understanding:** Get comprehensive view of code structure
- **Documentation Generation:** Auto-generate API structure documentation
- **Refactoring Planning:** Identify dependencies before restructuring
- **Onboarding:** Help new developers understand codebase organization
- **Metrics Tracking:** Monitor code growth and complexity over time
- **Architecture Analysis:** Visualize namespace hierarchies and relationships

## Requirements

- Python 3.8 or later
- Git (for `--include-git` option)
- Read access to ThemisDB source code
- Optional: `tqdm` for progress bars (`pip install tqdm`)

## Installation

No installation required - the script is standalone:

```bash
cd /path/to/ThemisDB
python3 tools/namespace_analyzer.py
```

## Basic Usage

### Quick Analysis

```bash
# Analyze with all output formats
python3 tools/namespace_analyzer.py

# Outputs:
# - namespace_analysis/namespace_analysis.json
# - namespace_analysis/namespace_analysis.md
# - namespace_analysis/namespaces.csv
# - namespace_analysis/classes.csv
# - namespace_analysis/functions.csv
```

### With Git Metadata

```bash
# Include Git history (slower but more detailed)
python3 tools/namespace_analyzer.py --include-git

# Shows:
# - When each component was introduced
# - Last modification date and author
# - Number of times modified
```

### Specific Output Format

```bash
# Only generate Markdown report
python3 tools/namespace_analyzer.py --format markdown

# Only generate JSON
python3 tools/namespace_analyzer.py --format json

# Only generate CSV files
python3 tools/namespace_analyzer.py --format csv
```

## Command-Line Options

```
usage: namespace_analyzer.py [-h] [--output-dir DIR] [--format FORMAT]
                             [--include-git] [--verbose] [--repo-root DIR]

Options:
  --output-dir DIR      Output directory (default: ./namespace_analysis)
  --format FORMAT       Output format: json, markdown, csv, or all (default: all)
  --include-git         Include Git metadata (timestamps, authors)
  --verbose             Enable verbose output
  --repo-root DIR       Repository root directory (default: auto-detect)
```

## Output Formats

### 1. JSON Format

**File:** `namespace_analysis.json`

Complete machine-readable structure:

```json
{
  "metadata": {
    "timestamp": "2026-01-12T12:00:00Z",
    "repo_root": "/path/to/ThemisDB",
    "total_namespaces": 85,
    "total_classes": 245,
    "total_functions": 1823,
    "total_variables": 167,
    "git_metadata_included": true
  },
  "namespaces": {
    "themis": {
      "name": "themis",
      "full_name": "themis",
      "parent": null,
      "classes": [
        {
          "name": "RocksDBWrapper",
          "type": "class",
          "file": "include/storage/rocksdb_wrapper.h",
          "line": 42,
          "base_classes": [],
          "is_template": false,
          "git_info": {
            "first_commit": "a1b2c3d",
            "first_date": "2024-06-15",
            "last_commit": "e4f5g6h",
            "last_date": "2025-12-20",
            "author": "dev@example.com"
          }
        }
      ],
      "functions": [...],
      "variables": [...]
    }
  }
}
```

### 2. Markdown Format

**File:** `namespace_analysis.md`

Human-readable report with:

- Summary statistics
- Top namespaces by size
- Detailed namespace information
- Class hierarchies
- Function signatures

Example output:
```markdown
# ThemisDB Namespace Analysis

**Generated:** 2026-01-12 12:00:00
**Repository:** /path/to/ThemisDB

## Summary

- **Total Namespaces:** 85
- **Total Classes:** 245
- **Total Functions:** 1,823
- **Total Variables:** 167

## Top Namespaces

| Namespace | Classes | Functions | Variables | Total |
|-----------|---------|-----------|-----------|-------|
| themis | 45 | 312 | 28 | 385 |
| themis::llm | 18 | 95 | 12 | 125 |
| themis::storage | 22 | 87 | 8 | 117 |

## Namespace: themis

**Full Name:** `themis`
**Files:** 45 header files

### Classes

#### class RocksDBWrapper
- **File:** `include/storage/rocksdb_wrapper.h:42`
- **Type:** class
- **Introduced:** 2024-06-15 (commit a1b2c3d)
- **Last Modified:** 2025-12-20 by dev@example.com
...
```

### 3. CSV Format

Three CSV files for spreadsheet analysis:

**File:** `namespaces.csv`
```csv
Namespace,Full Name,Class Count,Function Count,Variable Count,Total Entities
themis,themis,45,312,28,385
themis::llm,themis::llm,18,95,12,125
```

**File:** `classes.csv`
```csv
Namespace,Class Name,Type,File,Line,Base Classes,Template
themis,RocksDBWrapper,class,include/storage/rocksdb_wrapper.h,42,"",false
```

**File:** `functions.csv`
```csv
Namespace,Function Name,Return Type,Parameters,File,Line,Modifiers
themis,open,bool,"const std::string&",include/storage/rocksdb_wrapper.h,58,"virtual const"
```

## Advanced Usage

### Custom Output Directory

```bash
# Output to specific directory
python3 tools/namespace_analyzer.py \
  --output-dir /tmp/analysis_$(date +%Y%m%d) \
  --include-git
```

### Scheduled Analysis

```bash
#!/bin/bash
# weekly_analysis.sh

DATE=$(date +%Y%m%d)
OUTPUT_DIR="analysis_reports/$DATE"

python3 tools/namespace_analyzer.py \
  --output-dir "$OUTPUT_DIR" \
  --include-git \
  --verbose

# Archive results
tar -czf "analysis_$DATE.tar.gz" "$OUTPUT_DIR"

# Upload to documentation site
scp "analysis_$DATE.tar.gz" docs@example.com:/var/www/docs/analysis/
```

### Comparison Between Versions

```bash
# Analyze current state
git checkout main
python3 tools/namespace_analyzer.py \
  --output-dir analysis_main \
  --format json

# Analyze previous release
git checkout v1.3.0
python3 tools/namespace_analyzer.py \
  --output-dir analysis_v1.3.0 \
  --format json

# Compare
python3 scripts/compare_namespaces.py \
  analysis_main/namespace_analysis.json \
  analysis_v1.3.0/namespace_analysis.json
```

## Analyzed Components

### Namespaces

- Hierarchical namespace structure
- Parent-child relationships
- Files contributing to each namespace

### Classes

For each class/struct/enum:
- Name and type
- Template parameters
- Base classes (inheritance)
- File and line number
- Git history (optional)

### Functions

For each function:
- Name and signature
- Return type
- Parameters
- Modifiers (static, virtual, const, constexpr)
- File and line number
- Git history (optional)

### Variables

For each variable/constant:
- Name and type
- Modifiers (const, constexpr, static)
- File and line number
- Git history (optional)

## Git Integration

With `--include-git`, the analyzer tracks:

| Metric | Description | Use Case |
|--------|-------------|----------|
| **First Commit** | When component introduced | Find newest additions |
| **Last Commit** | Most recent modification | Identify active areas |
| **Author** | Who last modified | Know who to ask |
| **Commit Count** | Number of changes | Spot frequently changed code |

### Example Queries

```bash
# Find components added in last 3 months
jq '.namespaces[].classes[] | 
    select(.git_info.first_date > "2025-10-01") | 
    {name, file, date: .git_info.first_date}' \
    namespace_analysis.json

# Find most frequently modified functions
jq '.namespaces[].functions[] | 
    select(.git_info.commit_count > 10) | 
    {name, file, count: .git_info.commit_count}' \
    namespace_analysis.json
```

## Performance

| Mode | Files | Time | Memory |
|------|-------|------|--------|
| Without Git | 314 headers | ~5 seconds | ~50 MB |
| With Git | 314 headers | ~2 minutes | ~150 MB |

**Tip:** For quick analysis, omit `--include-git`

## Limitations

- **Simplified Parser:** May not recognize all complex C++ constructs
- **Macro Expansion:** Preprocessor macros not fully resolved
- **Inline Code:** Treats inline implementations as declarations
- **Git Performance:** Slow on very large repositories
- **C++ Only:** Analyzes header files (.h, .hpp, .hh)

## Troubleshooting

### "Could not find repository root"

```bash
# Specify root explicitly
python3 tools/namespace_analyzer.py \
  --repo-root /path/to/ThemisDB
```

### Git commands too slow

```bash
# Disable Git integration
python3 tools/namespace_analyzer.py  # without --include-git

# Or analyze subset of files
python3 tools/namespace_analyzer.py \
  --include-git \
  --file-pattern "include/storage/*.h"
```

### Missing entities

```bash
# Run in verbose mode to see what's analyzed
python3 tools/namespace_analyzer.py --verbose

# Check if files are excluded
grep -r "namespace themis" include/
```

## Integration

### Documentation Generation

```bash
# Generate API structure docs
python3 tools/namespace_analyzer.py \
  --format markdown \
  --output-dir docs/api/structure

# Add to documentation build
mkdocs build
```

### CI/CD Pipeline

```yaml
# .github/workflows/analyze.yml
- name: Analyze Codebase
  run: |
    python3 tools/namespace_analyzer.py \
      --format json \
      --output-dir artifacts/

- name: Upload Analysis
  uses: actions/upload-artifact@v3
  with:
    name: namespace-analysis
    path: artifacts/namespace_analysis.json
```

### Metrics Tracking

```bash
# Track metrics over time
python3 tools/namespace_analyzer.py \
  --format json \
  --output-dir "metrics/$(date +%Y%m%d)"

# Extract metrics for dashboard
jq '.metadata | {
  namespaces: .total_namespaces,
  classes: .total_classes,
  functions: .total_functions
}' metrics/*/namespace_analysis.json
```

## See Also

- [NAMESPACE_ANALYZER_README.md](../../../tools/NAMESPACE_ANALYZER_README.md) - Original documentation
- [Architecture Documentation](../../architecture/) - High-level architecture
- [Contributing Guide](../../../CONTRIBUTING.md) - Code contribution guidelines
- [Coding Standards](../../CODING_STANDARDS.md) - C++ coding conventions

## License

Part of ThemisDB, licensed under the project's main license.
