> **Build:** `cmake --preset linux-ninja-release && cmake --build --preset linux-ninja-release`

# Compiler Diagnostics Tools

A comprehensive suite of tools for analyzing, tracking, and resolving cross-compiler and linker errors in ThemisDB.

## Overview

These tools help systematically debug compilation issues across multiple platforms (Windows/MSVC, Linux/GCC, Linux/Clang, macOS, ARM) by:

1. **Parsing** compiler error logs and categorizing errors
2. **Auditing** source code for common cross-compiler issues
3. **Validating** symbol visibility in compiled binaries
4. **Tracking** error trends over time in CI/CD
5. **Generating** actionable reports and recommendations

## Tools

### 1. diagnostic_scanner.py

Parses compiler error logs and categorizes errors into actionable groups.

**Features:**
- Supports MSVC, GCC, and Clang error formats
- Auto-detects compiler and platform
- Categorizes errors (symbol visibility, linker, templates, intrinsics, etc.)
- Stores results in SQLite database
- Exports to JSON

**Usage:**
```bash
# Parse a build log
python tools/compiler_diagnostics/diagnostic_scanner.py build.log

# Specify compiler and platform
python tools/compiler_diagnostics/diagnostic_scanner.py build.log \
    --compiler msvc \
    --platform windows \
    --output errors.db

# Export to JSON
python tools/compiler_diagnostics/diagnostic_scanner.py build.log \
    --json errors.json
```

**Output:**
```
Found 245 errors/warnings in build.log

=== Error Statistics ===

By Category:
  LINKER: 89
  SYMBOL_VISIBILITY: 45
  TEMPLATE: 32
  INTRINSICS: 18
  WARNING: 61

By Platform:
  windows: 245

Top Problematic Files:
  src/llm/lora_framework.cpp: 23
  src/storage/rocksdb_wrapper.cpp: 18
  ...
```

### 2. source_audit.py

Scans source code for common cross-compiler issues.

**Features:**
- Checks for missing export macros on public APIs
- Detects platform-specific code without guards
- Finds compiler intrinsics without fallbacks
- Identifies template instantiation issues
- Generates detailed reports with suggestions

**Usage:**
```bash
# Audit entire codebase
python tools/compiler_diagnostics/source_audit.py \
    --root . \
    --output docs/SOURCE_AUDIT_REPORT.md

# Audit specific files
python tools/compiler_diagnostics/source_audit.py \
    --root . \
    --include src/storage/*.cpp \
    --json audit.json
```

**Output:**
```
Auditing 1523 source files...
  Progress: 100/1523
  Progress: 200/1523
  ...
Audit complete. Found 342 issues.
Report written to docs/SOURCE_AUDIT_REPORT.md
```

### 3. symbol_checker.py

Validates symbol visibility in compiled binaries.

**Features:**
- Uses `nm` (Unix) or `dumpbin` (Windows)
- Checks exported symbols
- Verifies expected exports
- Generates symbol reports

**Usage:**
```bash
# Check symbols in a binary
python tools/compiler_diagnostics/symbol_checker.py \
    build/libthemis_base.so \
    --output symbols.md

# Verify expected exports
python tools/compiler_diagnostics/symbol_checker.py \
    build/libthemis_base.so \
    --verify expected_exports.txt
```

**Output:**
```
=== Export Verification ===
Total exported: 1247
Expected: 1250

Missing 3 exports:
  - DatabaseConfig::getAdvancedOption
  - QueryExecutor::optimize
  - StorageEngine::checkpoint

✅ Most symbols are correctly exported
```

### 4. issue_tracker.py

Tracks compiler errors from CI runs over time.

**Features:**
- Tracks errors from CI builds
- Generates weekly reports
- Shows error trends
- Can integrate with GitHub Issues API

**Usage:**
```bash
# Track a CI run
python tools/compiler_diagnostics/issue_tracker.py track \
    --ci-log build.log \
    --build-id "2024-01-25-001" \
    --compiler gcc \
    --platform linux

# Generate weekly report
python tools/compiler_diagnostics/issue_tracker.py report \
    --output docs/WEEKLY_ERROR_REPORT.md

# Show trends
python tools/compiler_diagnostics/issue_tracker.py trends --days 30
```

## Integration with CI/CD

### GitHub Actions

The tools are integrated into CI workflows:

1. **ci-windows-full.yml**: Full Windows build with error tracking
2. **ci-linux-full.yml**: Linux builds (GCC + Clang) with error tracking
3. **ci-arm-cross.yml**: ARM cross-compilation with error tracking
4. **ci-sanitizers.yml**: Sanitizer coverage (ASan, UBSan, TSan, MSan)

### Automated Workflow

1. **Build fails** → Error logs captured
2. **diagnostic_scanner.py** → Parses logs, categorizes errors
3. **issue_tracker.py** → Tracks in database
4. **Report generation** → Creates summary
5. **PR comment** → Posts results (if PR)
6. **Artifact upload** → Stores logs and databases

### Example CI Integration

```yaml
- name: Parse Compiler Errors
  if: always()
  run: |
    python tools/compiler_diagnostics/diagnostic_scanner.py \
      build.log \
      --output errors.db \
      --compiler gcc \
      --platform linux

- name: Track Errors
  if: always()
  run: |
    python tools/compiler_diagnostics/issue_tracker.py track \
      --ci-log build.log \
      --build-id "${{ github.run_id }}"

- name: Upload Error Database
  uses: actions/upload-artifact@v3
  with:
    name: error-database
    path: errors.db
```

## Error Categories

| Category | Description | Priority |
|----------|-------------|----------|
| **SYMBOL_VISIBILITY** | Missing/incorrect export macros | High |
| **LINKER** | Undefined references, link order | High |
| **TEMPLATE** | Template instantiation issues | High |
| **INTRINSICS** | Compiler intrinsics without fallbacks | Medium |
| **ABI** | Calling convention, mangling issues | Medium |
| **PLATFORM_SPECIFIC** | OS-specific code without guards | Medium |
| **STANDARD_LIBRARY** | STL compatibility issues | Low |
| **WARNING** | Compiler warnings | Low |

## Database Schema

The SQLite database stores errors with this schema:

```sql
CREATE TABLE errors (
    id INTEGER PRIMARY KEY,
    file_path TEXT,
    line_number INTEGER,
    column_number INTEGER,
    severity TEXT,
    message TEXT,
    category TEXT,
    compiler TEXT,
    platform TEXT,
    full_context TEXT,
    timestamp TEXT,
    resolved BOOLEAN DEFAULT 0
);
```

## Querying the Database

```bash
# Connect to database
sqlite3 tools/compiler_diagnostics/compiler_diagnostics.db

# Count errors by category
SELECT category, COUNT(*) as count 
FROM errors 
GROUP BY category 
ORDER BY count DESC;

# Find errors in specific file
SELECT * FROM errors 
WHERE file_path LIKE '%lora_framework.cpp%';

# Get recent unresolved errors
SELECT * FROM errors 
WHERE resolved = 0 
ORDER BY timestamp DESC 
LIMIT 10;
```

## Pre-Commit Hook

Add to `.git/hooks/pre-commit`:

```bash
#!/bin/bash

# Run source audit
python tools/compiler_diagnostics/source_audit.py \
    --root . \
    --output /tmp/audit.md

# Check for high-priority issues
if grep -q "severity: high" /tmp/audit.md; then
    echo "ERROR: High-priority issues found"
    grep "severity: high" /tmp/audit.md
    exit 1
fi

echo "✅ Source audit passed"
exit 0
```

## Reports Generated

### Source Audit Report

Location: `docs/SOURCE_AUDIT_REPORT.md`

Contains:
- Summary statistics
- Issues grouped by severity
- Issues grouped by type
- Top problematic files
- Specific line numbers and suggestions

### Weekly Error Report

Location: `docs/WEEKLY_ERROR_REPORT.md`

Contains:
- Error count trends
- Errors by category
- Errors by platform
- Top problematic files
- Recommendations

### Platform Compatibility Matrix

Location: `docs/PLATFORM_COMPATIBILITY_MATRIX.md`

Contains:
- Per-file compilation status across platforms
- Known platform-specific issues
- Testing instructions

## Best Practices

### For Developers

1. **Before committing:**
   ```bash
   python tools/compiler_diagnostics/source_audit.py --root .
   ```

2. **After build failure:**
   ```bash
   python tools/compiler_diagnostics/diagnostic_scanner.py build.log
   ```

3. **Check specific file:**
   ```bash
   python tools/compiler_diagnostics/source_audit.py \
       --include src/myfile.cpp
   ```

### For CI Maintainers

1. **Monitor error trends**
2. **Review weekly reports**
3. **Update platform compatibility matrix**
4. **Track unresolved errors**

## Documentation

- [Platform Compatibility Matrix](../../docs/PLATFORM_COMPATIBILITY_MATRIX.md)
- [Compiler Troubleshooting Guide](../../docs/COMPILER_TROUBLESHOOTING.md)
- [Contributing Platform Guidelines](../../docs/CONTRIBUTING_PLATFORM_GUIDELINES.md)
- [Windows Build Errors](../../docs/build-guide/BUILD_WINDOWS_ERRORS.md)
- [Linux Build Errors](../../docs/build-guide/BUILD_LINUX_ERRORS.md)
- [ARM Build Errors](../../docs/build-guide/BUILD_ARM_ERRORS.md)

## Troubleshooting

### Tool doesn't detect my compiler

Add detection logic to `diagnostic_scanner.py`:

```python
def detect_compiler(self, log_content: str) -> str:
    if "my-compiler" in log_content:
        return "my-compiler"
    # ...
```

### False positives in source audit

Adjust patterns in `source_audit.py` or add file exclusions:

```python
# Skip certain directories
if any(x in str(file_path) for x in ['test', 'vendor', 'third_party']):
    return []
```

### Database locked error

Close all connections:

```python
db.close()
```

Or use WAL mode:

```python
cursor.execute("PRAGMA journal_mode=WAL")
```

## Examples

See the `examples/` directory for:
- Parsing different compiler formats
- Custom error categorization
- Generating custom reports
- Integration with other tools

## License

Same as ThemisDB project.

## Contributing

When improving these tools:

1. Add tests for new error patterns
2. Update documentation
3. Test on multiple platforms
4. Consider backward compatibility
