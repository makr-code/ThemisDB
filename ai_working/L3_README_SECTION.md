
## Documentation Governance (Phase 5)

ThemisDB documentation is organized in 4 levels with explicit Phase 5 external submodule filtering:

### L0: Gap Scanning (Semantic Analysis)
- **Input**: ThemisDB C++ codebase + documentation
- **Output**: gap_scan_L0_full_phase5.json (131,230 verified gaps)
- **Phase 5**: External submodules (llama.cpp, whisper.cpp, vcpkg, onnx-clip) explicitly filtered
- **Scope**: 100% themis_core (verified via path resolution + classification)

### L1: Module Documentation
- **32 MODULE_GAPS.md files** (one per module)
- **Phase 5 Verification Notes** in each file
- **Distribution**: llm (12,474 gaps), index (7,712), sharding (7,257), etc.
- **Location**: `src/<module>/MODULE_GAPS.md`

### L2: Aggregate Statistics
- **MODULE_SNAPSHOT_AGGREGATE_L2.md** (cross-module summary)
- **Risk Analysis**: CRITICAL | HIGH | MEDIUM | LOW
- **Top Modules**: Ranked by gap count and severity
- **Location**: `ai_working/MODULE_SNAPSHOT_AGGREGATE_L2.md`

### L3: Root Documentation
- **CHANGELOG.md**: Phase 5 implementation notes
- **ARCHITECTURE.md**: Updated with gap scanner pipeline
- **README.md**: Links to all governance docs
- **Status**: ✅ Phase 5 complete and validated

### External Submodules (Phase 5)

The following external GitHub submodules are **explicitly excluded** from L0-L3 gap analysis:

| Submodule | Scope | Reason |
|-----------|-------|--------|
| llama.cpp | `llama.cpp/` | Third-party AI model inference engine |
| whisper.cpp | `whisper.cpp/` | Third-party speech recognition |
| vcpkg | `vcpkg/` `vcpkg_installed/` | Third-party C++ package manager |
| onnx-clip | referenced | Third-party ML model format |

**Phase 5 Filtering**: These modules are identified via is_external_submodule() function and removed before L0 JSON export, ensuring 100% scope accuracy.
