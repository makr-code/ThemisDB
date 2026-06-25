
## Gap Scanner Pipeline (Phase 5)

### Pipeline Stages

```
[Phase 1: File Existence Check]
  ↓ Removes FILE_NOT_FOUND false-positives (2,837 removed)
  ↓
[Phase 2: Multi-Factor Classification]
  ↓ Classifies gaps as: test_mock, guarded_stub, placeholder, real_gap
  ↓
[Phase 3-4: Enrichment & Metadata]
  ↓ Adds source location, severity, type, recommended fixes
  ↓
[Phase 5: External Submodule Filtering] ← NEW
  ↓ Removes findings from: llama.cpp, whisper.cpp, vcpkg, onnx-clip
  ↓ _resolve_gaps_to_repo_paths(): Reconstructs absolute paths
  ↓ is_external_submodule(): Identifies external boundaries
  ↓ filter_external_submodules(): Removes 0 external findings
  ↓
[Output: L0 JSON]
  → gap_scan_L0_full_phase5.json
  → 131,230 verified gaps (100% themis_core)
  → 0 gaps from external submodules
  ↓
[L1: MODULE_GAPS.md Generation]
  → 32 module files with Phase 5 verification notes
  ↓
[L2: Aggregation]
  → MODULE_SNAPSHOT_AGGREGATE_L2.md (summary statistics)
  ↓
[L3: Root Documentation]
  → CHANGELOG.md, ARCHITECTURE.md, README.md (updates)
```

### Phase 5: External Submodule Filtering

**Purpose**: Ensure that L0-L3 documentation boundaries are explicit (code-based) rather than implicit (directory-based).

**Implementation**:
```python
EXTERNAL_SUBMODULES = {
    'llama.cpp',
    'whisper.cpp', 
    'vcpkg',
    'vcpkg_installed',
    'vcpkg_installed_linux',
    'onnx-clip',
}

def is_external_submodule(file_path: str) -> bool:
    # Explicit boundary check
    normalized = _normalize_path(file_path)
    return any(sub.lower() in normalized for sub in EXTERNAL_SUBMODULES)
```

**Path Resolution**:
- Scanner returns: "explain_plan.cpp" (relative to source_dir)
- Resolver reconstructs: "src/graph/explain_plan.cpp" (repo-absolute)
- Classifier receives correct path for scope determination
- Result: 100% themis_core (not FALSE 0%)

### Key Metrics (Phase 5)

- **Input gaps**: 134,067 (raw scanner)
- **FILE_NOT_FOUND filtered**: -2,837
- **EXTERNAL_SUBMODULE filtered**: -0 (none found — correct!)
- **Downgraded (re-assessed)**: -5,021
- **Output gaps**: 131,230 verified (themis_core only)

### Validation

✅ Scope breakdown: 100% themis_core | 0% third_party
✅ External submodule names only in documentation
✅ All 32 MODULE_GAPS.md files include Phase 5 verification note
✅ Path resolution reconstructs 1,652+ absolute paths

### Future Phases

- **Phase 6**: Performance bottleneck analysis
- **Phase 7**: Security vulnerability detection
- **Phase 8**: Dependency analysis & external API mapping
- **Phase 9-11**: Automated remediation & Q3 2026 roadmap
