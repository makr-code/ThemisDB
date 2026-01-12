---
name: Documentation - Plugin System Status and Architecture
about: Clarify plugin system development status and document actual architecture
title: '[DOCS] Plugin System: Clarify Development Status and Document Architecture'
labels: 'type:documentation, priority:P1, area:plugins, effort:small'
assignees: ''
---

## Problem Statement

The documentation in `plugins/README.md` describes a production-ready runtime plugin loading system with DLL/SO plugins. However, the actual implementation appears to be **template/example files only**, creating confusion about the system's maturity and capabilities.

## Documentation vs. Reality Gap

### Documented System (plugins/README.md)

**Claims:**
- Runtime-loadable plugins (DLL/SO)
- Production-ready plugin architecture
- Backend registry with auto-detection
- Multiple backend plugins available:
  - themis_accel_cuda.dll/.so (NVIDIA CUDA)
  - themis_accel_vulkan.dll/.so (Cross-Platform)
  - themis_accel_directx.dll (Windows)
  - themis_accel_hip.dll/.so (AMD)
  - themis_accel_metal.dylib (Apple Metal)

**Documentation gives impression:** Fully functional plugin system in production

### Actual Implementation

```
plugins/
├── README.md (describes production system)
├── CMakeLists.txt
├── blob_storage/ (actual plugin category)
│   ├── azure/
│   └── s3/
├── cuda/ (examples/templates only)
│   ├── CMakeLists.txt.example
│   ├── cuda_plugin.cpp.example
│   └── cuda_plugin.json
├── exporters/ (actual plugin category)
│   └── jsonl_llm/
├── image_analysis/ (actual plugin category)
│   └── onnx_clip/
├── importers/ (actual plugin category)
│   └── postgres/
└── rpc/ (actual plugin category)
    └── grpc/
```

**Actual Status:**
- CUDA plugin: Example/template files only (.example extensions)
- Other acceleration plugins: Not implemented
- Different plugin types exist: blob_storage, exporters, image_analysis, importers, rpc
- Plugin system may be partially implemented but not as documented

## Impact

**Severity:** HIGH

**Affected Users:**
- Developers trying to create plugins (following wrong documentation)
- Users expecting hardware acceleration (may think it's ready)
- Contributors evaluating project maturity
- Technical decision-makers assessing ThemisDB capabilities

**Consequences:**
- Developer time wasted following incorrect guides
- False expectations about feature availability
- Difficulty understanding actual plugin architecture
- Potential misrepresentation of product capabilities

## Proposed Solution

### Required Actions

1. **Clarify Development Status**
   - Update plugins/README.md with accurate status indicators
   - Mark documented features as "Planned" / "In Development" / "Production"
   - Add "Current Status" section at top of document

2. **Document Actual Plugin Architecture**
   - Document the actual plugin types that exist:
     - blob_storage plugins (Azure, S3)
     - exporters plugins (JSONL LLM)
     - image_analysis plugins (ONNX CLIP)
     - importers plugins (PostgreSQL)
     - rpc plugins (gRPC)
   - Explain how these differ from documented acceleration plugins
   - Describe the actual plugin loading mechanism (if exists)

3. **Separate Documentation**
   - Create separate docs for different plugin types:
     - PLANNED_ACCELERATION_PLUGINS.md (for CUDA/Vulkan/etc.)
     - CURRENT_PLUGIN_SYSTEM.md (for blob_storage/exporters/etc.)
   - Update main README to reference both

4. **Update Examples**
   - Ensure example files are clearly marked
   - Provide working example for at least one plugin type
   - Add step-by-step guide to create a plugin

## Acceptance Criteria

### Minimum Requirements
- [ ] plugins/README.md updated with accurate status indicators
- [ ] Clear "Status" badges for each documented feature:
  - ✅ Production / 🚧 In Development / 📋 Planned / ❌ Not Implemented
- [ ] "Current Status" section added to document top
- [ ] Documentation covers actual plugin types (blob_storage, exporters, etc.)
- [ ] False claims removed or clearly marked as future plans

### Recommended Additions
- [ ] Separate documentation files for different plugin subsystems
- [ ] Working example plugin with step-by-step guide
- [ ] Plugin development guide updated for actual architecture
- [ ] CMakeLists.txt documentation for plugin building
- [ ] Architecture diagram showing actual plugin system

### Quality Checks
- [ ] All code examples tested and working
- [ ] Links verified
- [ ] Reviewed by plugin system maintainer
- [ ] No misleading claims about production readiness

## Investigation Checklist

Before implementing the fix, investigate:

- [ ] Is there a plugin loading system in src/plugins/?
- [ ] Do the .example files work if renamed?
- [ ] What is the build process for plugins?
- [ ] Are any plugins actually used in production?
- [ ] What is the roadmap for acceleration plugins?
- [ ] Which plugin types are stable vs. experimental?

## Recommended Documentation Structure

```markdown
# ThemisDB Plugin System

## Current Status

⚠️ **Development Status:** The plugin system is under active development.

### Production-Ready Plugin Types
- ✅ Blob Storage Plugins (Azure, S3)
- ✅ Image Analysis Plugins (ONNX CLIP)
- 🚧 Exporters Plugins (JSONL LLM) - Beta
- 🚧 Importers Plugins (PostgreSQL) - Beta
- 🚧 RPC Plugins (gRPC) - Beta

### Planned Plugin Types
- 📋 Hardware Acceleration Plugins (CUDA, Vulkan, DirectX, HIP, Metal)
- 📋 Protocol Plugins (Custom protocols)

## Working with Production Plugins

[Actual documentation for working plugin types]

## Developing Custom Plugins

[Guide based on actual plugin system]

## Future Plans

[Roadmap for acceleration plugins and other planned types]
```

## Additional Context

**Source:** Documentation-Source Code Gap Analysis  
**Gap Analysis Document:** docs/DOCUMENTATION_SOURCE_CODE_GAP_ANALYSIS.md

**Files to Review:**
- plugins/README.md (needs major update)
- plugins/cuda/*.example (understand if functional)
- src/plugins/ (check for plugin loading code)
- src/acceleration/ (check for backend abstraction)
- CMakeLists.txt (check for plugin build logic)

**Related Documentation:**
- docs/architecture/HARDWARE_ACCELERATION.md (if exists)
- docs/development/plugin_development.md (if exists)

## Timeline

**Estimated Effort:** 1-2 days (8-16 hours)

**Breakdown:**
- Investigation: 2-4 hours (understand actual system)
- Documentation update: 4-8 hours (write accurate docs)
- Example creation: 2-4 hours (create working example)
- Review and polish: 1-2 hours

**Priority:** P1 (High) - Affects perception of project maturity and developer trust
