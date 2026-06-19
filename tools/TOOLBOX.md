# ThemisDB Tools Toolbox

**Purpose**: Specialized diagnostic and analysis tools for build, test, and code quality validation.

---

## Tools Inventory

### 1. Gap Scanner Orchestrator (`gs3_orchestrator.py`)
**Purpose**: Unified orchestrator for gap analysis across codebase  
**Type**: Wrapper/Orchestrator  
**Usage**:
```bash
python tools/gs3_orchestrator.py [source_dir] [--output <file>] [--verbose]
```
**Capabilities**:
- Delegates to specialized scanners
- Aggregates results
- Produces unified reports

**Status**: ✅ Production

---

### 2. Gap Scanner v3 (`gap_scanner_v3.py`)
**Purpose**: Legacy compatibility wrapper for gap_scanner_v3 invocations  
**Type**: Compatibility Shim  
**Usage**:
```bash
python tools/gap_scanner_v3.py [repo_root] [output_dir]
# Or delegate to gs3_orchestrator:
python tools/gs3_orchestrator.py [source_dir] [--output <file>]
```
**Capabilities**:
- Backward-compatible interface
- Forwards to gs3_orchestrator

**Status**: ✅ Production (Wrapper Only)

---

### 3. Unity Build Validator (`unity_build_validator.py`)
**Purpose**: Detect namespace closure and brace balance issues in CMake Unity builds  
**Type**: Specialized Diagnostic  
**Usage**:
```bash
# Single module analysis
python tools/unity_build_validator.py --module themis_graph --verbose

# All modules
python tools/unity_build_validator.py --all-modules --verbose

# JSON output for CI/tooling
python tools/unity_build_validator.py --module themis_graph \
  --output ai_working/unity_build_analysis.json
```

**Capabilities**:
- ✅ Parse CMake source lists from `cmake/ModularBuild.cmake`
- ✅ Analyze individual .cpp files for brace balance
- ✅ Simulate Unity build concatenation sequence
- ✅ Detect cumulative namespace/brace imbalances
- ✅ Report exact file/line for issues
- ✅ Generate JSON reports for CI/CD integration

**Errors Detected**:
| Error Type | Severity | Description |
|-----------|----------|-------------|
| `CLOSES_WITHOUT_OPEN` | 🔴 ERROR | Unmatched closing braces |
| `UNCLOSED_BRACES_AT_EOF` | 🟡 WARNING | File ends with open braces |
| `NAMESPACE_REOPENING_AFTER_IMBALANCE` | 🟡 WARNING | Namespace re-opened after cumulative imbalance |

**Example Output**:
```
Module: themis_graph
Status: CRITICAL

Files analyzed: 21
Cumulative balance by file:
  After file 11: opens=2752, closes=2752, balance=0
  After file 12: opens=2913, closes=2914, balance=-1  ⚠️ PROBLEM HERE!
  After file 21: opens=3274, closes=3275, balance=-1

Problems detected: 11
  [ERROR] CLOSES_WITHOUT_OPEN (file #12, line 702)
    → File has closing braces without opening (line 702)
```

**Technical Details**: [ERROR_CLASS_UNITY_NAMESPACE_IMBALANCE.md](docs/ERROR_CLASS_UNITY_NAMESPACE_IMBALANCE.md)

**Status**: ✅ **NEW** (2026-06-19)  
**Module Status**: themis_graph - 🔴 CRITICAL (extra closing brace at L702)

---

## Error Classes Documented

### Unity Build Namespace Closure Imbalance
- **ID**: UNITY_NAMESPACE_IMBALANCE
- **Detector**: unity_build_validator.py
- **Documentation**: docs/ERROR_CLASS_UNITY_NAMESPACE_IMBALANCE.md
- **Symptom**: MSVC C2143/C2065/C2923 errors in later files within Unity build
- **Root Cause**: Redundant namespace closure/reopening in .cpp files concatenated by Unity build
- **Detection**: Cumulative brace analysis across file sequences
- **Prevention**: Pre-build validation gate

---

## Integration Points

### CI/CD Pipeline
```yaml
# Pre-build validation
- name: Validate Unity Build Structure
  run: python tools/unity_build_validator.py --all-modules --verbose
    --output reports/unity_build_analysis.json

# Gate: Fail if CRITICAL status
- name: Gate Build on Validation
  run: |
    python -c "
    import json
    with open('reports/unity_build_analysis.json') as f:
        results = json.load(f)
    status = [r['status'] for r in results.values()]
    if 'CRITICAL' in status:
        exit(1)
    "
```

### CMake Pre-Configure Check (Planned)
```cmake
# In cmake/CMakeLists.txt (add_custom_target)
add_custom_target(validate_unity_build
  COMMAND python tools/unity_build_validator.py --all-modules
    --output ${CMAKE_BINARY_DIR}/reports/unity_analysis.json
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# Make build depend on validation
add_dependencies(themis_graph validate_unity_build)
```

---

## Tool Development Guidelines

### Adding a New Specialized Tool
1. **Create tool file** in `tools/` directory (e.g., `my_validator.py`)
2. **Document error class** in `docs/ERROR_CLASS_*.md` if detecting new issues
3. **Register in toolbox** by updating `tools/TOOLBOX.md` (this file)
4. **Integrate with orchestrator** (gs3_orchestrator.py) OR run standalone
5. **Add CI/CD gate** if tool detects build blockers

### Tool Naming Convention
- Analysis tools: `{domain}_analyzer.py`
- Validators: `{domain}_validator.py`
- Scanners: `{domain}_scanner.py`
- Fixers: `{domain}_fixer.py`

---

## Tool Output Formats

### Standard JSON Report Format
```json
{
  "tool": "unity_build_validator",
  "timestamp": "2026-06-19T...",
  "status": "CRITICAL|OK|WARNING",
  "modules": {
    "themis_graph": {
      "status": "CRITICAL",
      "error_count": 1,
      "warning_count": 10,
      "problems": [
        {
          "severity": "ERROR",
          "type": "CLOSES_WITHOUT_OPEN",
          "file": "src/graph/ontology_manager.cpp",
          "line": 702,
          "detail": "..."
        }
      ]
    }
  }
}
```

---

## Version History

| Version | Date | Changes |
|---------|------|---------|
| 3.0 | 2026-06-19 | Added unity_build_validator.py (NEW ERROR CLASS: UNITY_NAMESPACE_IMBALANCE) |
| 2.0 | 2026-06-xx | gs3_orchestrator unified interface |
| 1.0 | Earlier | gap_scanner_v3.py legacy interface |

---

## See Also
- [ERROR_CLASS_UNITY_NAMESPACE_IMBALANCE.md](docs/ERROR_CLASS_UNITY_NAMESPACE_IMBALANCE.md) - Detailed error documentation
- [CLAUDE.md](CLAUDE.md) - Development workflow contract
- [.github/copilot-instructions.md](.github/copilot-instructions.md) - AI delivery rules
