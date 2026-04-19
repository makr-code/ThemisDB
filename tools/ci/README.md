> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# CI/CD Tools

This directory contains tools and scripts for analyzing and managing GitHub Actions workflows.

## Available Tools

### analyze_workflows.py

Analyzes all GitHub Actions workflows in the repository and generates a comprehensive inventory.

**Features:**
- Parses all workflow YAML files
- Extracts metadata (triggers, jobs, runners, actions, permissions)
- Categorizes workflows by purpose
- Identifies common patterns and duplications
- Generates detailed markdown documentation

**Usage:**
```bash
# Install dependencies
pip install -r requirements.txt

# Run from repository root
python3 tools/ci/analyze_workflows.py

# Output will be generated at:
# docs/ci-cd/workflows-inventory.md
```

**Requirements:**
- Python 3.7 or higher
- PyYAML 6.0.1 or higher

**Output:**
The script generates `docs/ci-cd/workflows-inventory.md` with:
- Summary statistics
- Workflows grouped by category
- Detailed workflow information
- Common patterns analysis
- Actions usage summary

### parse_build_errors.py

Parses compiler output from build logs and generates structured error reports.

**Features:**
- Parses GCC, Clang, and MSVC compiler output
- Extracts errors and warnings with file locations
- Categorizes errors by type (linking, syntax, missing files, etc.)
- Generates JSON and Markdown reports
- Used by the nightly build workflow for automated issue creation

**Usage:**
```bash
# Parse a build log
python3 tools/ci/parse_build_errors.py build.log --compiler gcc \
  --json error_summary.json \
  --markdown error_summary.md

# Supported compilers: gcc, clang, msvc
```

**Requirements:**
- Python 3.7 or higher
- No external dependencies

**Output:**
- JSON file with structured error data
- Markdown file with formatted error summary
- Exit code 1 if errors found, 0 otherwise

**Error Categories:**
- `linking` - Undefined references, unresolved externals
- `syntax` - Syntax errors, missing semicolons
- `missing_file` - Missing headers or files
- `ambiguity` - Ambiguous calls, overload resolution
- `deprecated` - Deprecated features
- `other` - All other errors

See [Nightly Build documentation](../../docs/ci/NIGHTLY_BUILD.md) for more details.

## Installation

```bash
# Install dependencies
pip install -r requirements.txt

# Or install globally
pip install PyYAML>=6.0.1
```

## Development

### Adding New Analysis Features

To add new analysis capabilities to `analyze_workflows.py`:

1. Add extraction methods to the `WorkflowAnalyzer` class
2. Update the `analyze_workflow()` method to collect new data
3. Add reporting methods for the new data
4. Update the markdown generation in `generate_inventory_markdown()`

### Running Tests

```bash
# Test the script
python3 analyze_workflows.py

# Verify output
cat ../../docs/ci-cd/workflows-inventory.md
```

## Future Enhancements

Potential improvements for the tools:

- [ ] Add validation for workflow syntax
- [ ] Generate workflow dependency graphs
- [ ] Compare workflows for duplication detection
- [ ] Suggest consolidation opportunities automatically
- [ ] Generate workflow templates
- [ ] Integration with GitHub API for runtime metrics
- [ ] HTML report generation
- [ ] CI/CD cost estimation

## Contributing

When contributing to the CI/CD tools:

1. Maintain Python 3.7+ compatibility
2. Keep external dependencies minimal
3. Document new features in this README
4. Update requirements.txt if adding dependencies
5. Follow PEP 8 style guidelines

## Support

For questions or issues with these tools:

1. Check the [CI/CD documentation](../../docs/ci-cd/)
2. Review the [consolidation plan](../../docs/ci-cd/consolidation-plan.md)
3. Open an issue with details about the problem

---

*Last updated: 2026-02-10*
