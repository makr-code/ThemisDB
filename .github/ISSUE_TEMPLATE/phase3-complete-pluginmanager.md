---
name: Phase 3 - Complete PluginManager Migration
about: Complete the remaining PluginManager methods migration to Result<T>
title: '[Phase 3] Complete PluginManager Migration to Result<T>'
labels: ['enhancement', 'error-handling', 'phase-3', 'plugin-system']
assignees: ''
---

## 📋 Overview

Complete the migration of remaining PluginManager methods from legacy error patterns to `Result<T>`.

**Current Status:** 25% complete (3 of 12 methods)  
**Target:** 100% complete (12 of 12 methods)  
**Remaining:** 9 methods

## 🎯 Goals

Migrate all remaining PluginManager methods to use `tl::expected`-based error handling for consistent, type-safe error propagation in the plugin system.

## ✅ Already Complete (PR #XXX)

- [x] `loadPlugin()` - Returns `Result<IThemisPlugin*>`
- [x] `loadPluginFromPath()` - Returns `Result<IThemisPlugin*>`
- [x] `getPlugin()` - Returns `Result<IThemisPlugin*>`

## 🔨 Remaining Work

### Methods to Migrate (9 remaining)

1. **Plugin Registry Operations:**
   - [ ] `registerPlugin()` - Currently returns `bool`, migrate to `Result<void>`
   - [ ] `unregisterPlugin()` - Currently returns `bool`, migrate to `Result<void>`

2. **Plugin Discovery:**
   - [ ] `discoverPlugins()` - Review return type, consider `Result<size_t>` or `Result<std::vector<...>>`
   - [ ] `scanPluginDirectory()` - Review return type

3. **Plugin Validation:**
   - [ ] `verifyPlugin()` - Currently returns `bool` with error message, migrate to `Result<void>`
   - [ ] `validateManifest()` - Review return type

4. **Plugin Configuration:**
   - [ ] `configurePlugin()` - Review return type, likely `Result<void>`
   - [ ] `reconfigurePlugin()` - Review return type

5. **Plugin Metadata:**
   - [ ] `getManifest()` - Currently returns `std::optional<PluginManifest>`, migrate to `Result<PluginManifest>`

## 📝 Implementation Checklist

For each method:
- [ ] Update method signature in `include/plugins/plugin_manager.h`
- [ ] Update implementation in `src/plugins/plugin_manager.cpp`
- [ ] Replace legacy error patterns (`nullptr`, `false`, `std::nullopt`) with proper error codes
- [ ] Use appropriate error codes:
  - `ERR_PLUGIN_NOT_FOUND`
  - `ERR_PLUGIN_LOAD_FAILED`
  - `ERR_PLUGIN_INVALID_SIGNATURE`
  - `ERR_PLUGIN_INCOMPATIBLE`
  - `ERR_API_INVALID_REQUEST`
- [ ] Update all call sites (search for method usage)
- [ ] Update test files
- [ ] Update mock implementations if any
- [ ] Add error context with `fmt::format` for detailed messages

## 🧪 Testing Requirements

- [ ] Update `tests/integration/test_cross_functional_plugin_query_metrics.cpp`
- [ ] Update any other plugin-related tests
- [ ] Verify error codes are properly checked in tests
- [ ] Ensure backward compatibility (existing code should compile)

## 📚 Documentation Updates

- [ ] Update method documentation in header files
- [ ] Update error handling examples in `examples/migration/`
- [ ] Update ERROR_HANDLING_MIGRATION_STATUS.md with new progress

## 🎯 Success Criteria

- [ ] All 12 PluginManager methods use `Result<T>`
- [ ] All tests pass
- [ ] All call sites updated
- [ ] Zero breaking changes
- [ ] Security check (CodeQL) passes
- [ ] Code review approved

## 📊 Progress Tracking

**Expected Effort:** 2-3 days  
**Priority:** High (user-facing plugin system)

## 🔗 Related

- **Parent Issue:** #XXX (Error Handling Migration - Master Tracking)
- **Previous PR:** #XXX (Phase 1-2 Verification + Phase 3 Start)
- **Documentation:** ERROR_HANDLING_MIGRATION_STATUS.md
- **Migration Guide:** examples/migration/README.md

## 💡 Notes

- Maintain consistent error code usage across all methods
- Consider plugin state when returning errors
- Provide detailed error context for debugging
- Follow patterns established in loadPlugin() and getPlugin()
