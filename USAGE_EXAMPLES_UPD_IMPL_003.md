# Patch Ordering Enforcement (UPD-IMPL-003) - Usage Examples

**Component:** Delta Update Engine
**File Reference:** `include/updates/delta_update_engine.h`

## Example 1: Simple Linear Dependency Chain

This example shows how to define patches that depend on each other in a linear chain.

```cpp
using namespace themis::updates;

// Create a manifest for updating from 1.0.0 to 2.0.0
DeltaManifest manifest;
manifest.from_version = "1.0.0";
manifest.to_version = "2.0.0";
manifest.enforce_order = true;  // Enable ordering enforcement

// Base library patch - no dependencies
FileDelta libcore;
libcore.path = "lib/libcore.so";
libcore.base_hash = "hash_of_libcore_v1";
libcore.target_hash = "hash_of_libcore_v2";
libcore.patch_url = "https://releases.example.com/libcore.v2.patch";
libcore.patch_size = 2048;
libcore.target_size = 8192;
libcore.apply_order = 10;  // Apply first

// Dependent library - depends on libcore
FileDelta libutil;
libutil.path = "lib/libutil.so";
libutil.base_hash = "hash_of_libutil_v1";
libutil.target_hash = "hash_of_libutil_v2";
libutil.patch_url = "https://releases.example.com/libutil.v2.patch";
libutil.patch_size = 1024;
libutil.target_size = 4096;
libutil.depends_on = {"lib/libcore.so"};  // MUST apply after libcore
libutil.apply_order = 20;

// Application binary - depends on both libraries
FileDelta app;
app.path = "bin/app";
app.base_hash = "hash_of_app_v1";
app.target_hash = "hash_of_app_v2";
app.patch_url = "https://releases.example.com/app.v2.patch";
app.patch_size = 4096;
app.target_size = 16384;
app.depends_on = {"lib/libcore.so", "lib/libutil.so"};  // Depends on both
app.apply_order = 30;

manifest.deltas = {libcore, libutil, app};

// Apply patches - they will be reordered to: libcore, libutil, app
DeltaUpdateEngine engine("/opt/themis", "/tmp/patches");
DeltaApplyResult result = engine.applyDelta(manifest);

if (result.success) {
    LOG_INFO("Successfully patched {} files", result.files_patched.size());
} else {
    LOG_ERROR("Patch failed: {}", result.error_message);
}
```

## Example 2: Diamond Dependency Pattern

Configuration files are needed before any binary patches can be applied.

```cpp
DeltaManifest manifest;
manifest.from_version = "2.0.0";
manifest.to_version = "2.1.0";
manifest.enforce_order = true;

// Configuration file - has no dependencies
FileDelta config;
config.path = "etc/config.yaml";
config.base_hash = "hash_config_v1";
config.target_hash = "hash_config_v2";
config.patch_url = "https://releases.example.com/config.patch";
config.apply_order = 1;  // Apply first

// Multiple services that all depend on config
FileDelta svc1;
svc1.path = "bin/service1";
svc1.depends_on = {"etc/config.yaml"};
svc1.apply_order = 10;

FileDelta svc2;
svc2.path = "bin/service2";
svc2.depends_on = {"etc/config.yaml"};
svc2.apply_order = 10;

FileDelta svc3;
svc3.path = "bin/service3";
svc3.depends_on = {"etc/config.yaml"};
svc3.apply_order = 10;

manifest.deltas = {svc1, svc2, svc3, config};

// Even though config is last in manifest, it will be applied first
// Then svc1, svc2, svc3 can be applied (potentially in parallel)
DeltaUpdateEngine engine("/opt/themis", "/tmp/patches");
DeltaApplyResult result = engine.applyDelta(manifest);
```

## Example 3: Implicit Global Dependencies

Many patches depend on a common configuration file. Use implicit dependencies.

```cpp
DeltaManifest manifest;
manifest.from_version = "1.5.0";
manifest.to_version = "1.6.0";
manifest.enforce_order = true;

// Global dependency - all patches implicitly depend on this
manifest.implicit_dependencies = {"etc/schema.sql"};

// Schema patch - has no explicit dependencies
FileDelta schema;
schema.path = "etc/schema.sql";
schema.base_hash = "hash_schema_v1";
schema.target_hash = "hash_schema_v2";
schema.patch_url = "https://releases.example.com/schema.patch";
schema.apply_order = 1;

// Multiple data files - implicitly depend on schema
FileDelta data1;
data1.path = "data/initial_data.sql";
data1.base_hash = "hash_data1_v1";
data1.target_hash = "hash_data1_v2";
data1.patch_url = "https://releases.example.com/data1.patch";

FileDelta data2;
data2.path = "data/config_data.sql";
data2.base_hash = "hash_data2_v1";
data2.target_hash = "hash_data2_v2";
data2.patch_url = "https://releases.example.com/data2.patch";

manifest.deltas = {data1, data2, schema};

// Result: schema.sql applied first, then data1 and data2
// (implicit dependencies override manifest order)
DeltaUpdateEngine engine("/opt/themis", "/tmp/patches");
DeltaApplyResult result = engine.applyDelta(manifest);
```

## Example 4: Backward Compatibility (No Ordering)

Legacy code path that ignores all dependency information.

```cpp
DeltaManifest manifest;
manifest.from_version = "1.0.0";
manifest.to_version = "1.1.0";
manifest.enforce_order = false;  // Disable ordering enforcement

// Even though we specify dependencies, they will be ignored
FileDelta f1;
f1.path = "file1";
f1.depends_on = {"file2"};  // This is IGNORED when enforce_order=false

FileDelta f2;
f2.path = "file2";
f2.apply_order = 100;  // This is IGNORED when enforce_order=false

manifest.deltas = {f1, f2};

// Patches are applied in manifest order: f1, then f2
// Dependencies are completely ignored
DeltaUpdateEngine engine("/opt/themis", "/tmp/patches");
DeltaApplyResult result = engine.applyDelta(manifest);
```

## Example 5: Error Handling - Circular Dependency

Shows how to detect and handle circular dependencies.

```cpp
DeltaManifest manifest;
manifest.from_version = "1.0.0";
manifest.to_version = "2.0.0";
manifest.enforce_order = true;

// Create a circular dependency: A -> B -> C -> A
FileDelta a;
a.path = "file_a";
a.depends_on = {"file_b"};

FileDelta b;
b.path = "file_b";
b.depends_on = {"file_c"};

FileDelta c;
c.path = "file_c";
c.depends_on = {"file_a"};  // Back to A - creates cycle

manifest.deltas = {a, b, c};

DeltaUpdateEngine engine("/opt/themis", "/tmp/patches");
DeltaApplyResult result = engine.applyDelta(manifest);

// Error handling
if (!result.success) {
    LOG_ERROR("Patch application failed: {}", result.error_message);
    // Error message will mention ordering failure
    // Log will show: "circular dependency detected (7402)"
    
    // Take fallback action - perhaps full download
    downloadFullRelease("1.0.0", "2.0.0");
}
```

## Example 6: Error Handling - Missing Dependency

Shows how to detect and handle missing dependencies.

```cpp
DeltaManifest manifest;
manifest.from_version = "1.0.0";
manifest.to_version = "2.0.0";
manifest.enforce_order = true;

// This patch depends on a file that's not in the manifest
FileDelta app;
app.path = "bin/app";
app.depends_on = {"lib/libnotfound.so"};  // This file doesn't exist

manifest.deltas = {app};

DeltaUpdateEngine engine("/opt/themis", "/tmp/patches");
DeltaApplyResult result = engine.applyDelta(manifest);

// Error handling
if (!result.success) {
    LOG_ERROR("Patch failed: {}", result.error_message);
    // Error message will mention ordering failure
    // Log will show: "dependency 'lib/libnotfound.so' not found in manifest (7404)"
    
    // Take fallback action
    if (result.error_message.find("Patch ordering failed") != std::string::npos) {
        LOG_WARN("Dependency validation failed - falling back to full update");
        downloadFullRelease("1.0.0", "2.0.0");
    }
}
```

## Example 7: Complex Real-World Scenario

A complete update with multiple dependency levels.

```cpp
json manifest_json = nlohmann::json::parse(R"({
  "from_version": "1.4.0",
  "to_version": "1.5.0",
  "enforce_order": true,
  "implicit_dependencies": ["etc/build.properties"],
  "deltas": [
    {
      "path": "etc/build.properties",
      "base_hash": "abc123...",
      "target_hash": "def456...",
      "patch_url": "https://releases.example.com/build.patch",
      "patch_size": 512,
      "target_size": 1024,
      "algorithm": "zstd_dict",
      "apply_order": 1
    },
    {
      "path": "lib/libcore.so",
      "base_hash": "abc123...",
      "target_hash": "def456...",
      "patch_url": "https://releases.example.com/libcore.patch",
      "patch_size": 2048,
      "target_size": 8192,
      "algorithm": "zstd_dict",
      "apply_order": 10
    },
    {
      "path": "lib/libutil.so",
      "base_hash": "abc123...",
      "target_hash": "def456...",
      "patch_url": "https://releases.example.com/libutil.patch",
      "patch_size": 1024,
      "target_size": 4096,
      "algorithm": "zstd_dict",
      "depends_on": ["lib/libcore.so"],
      "apply_order": 20
    },
    {
      "path": "bin/server",
      "base_hash": "abc123...",
      "target_hash": "def456...",
      "patch_url": "https://releases.example.com/server.patch",
      "patch_size": 4096,
      "target_size": 16384,
      "algorithm": "zstd_dict",
      "depends_on": ["lib/libcore.so", "lib/libutil.so"],
      "apply_order": 30
    }
  ]
})");

// Deserialize and apply
auto manifest = DeltaManifest::fromJson(manifest_json);
if (!manifest) {
    LOG_ERROR("Failed to parse manifest");
    return;
}

DeltaUpdateEngine engine("/opt/themis", "/tmp/patches");

// Set progress callback for UI updates
engine.setProgressCallback([](int pct, const std::string& msg) {
    std::cout << "[" << pct << "%] " << msg << std::endl;
});

// Apply patches - will be reordered to:
// 1. etc/build.properties (implicit dep, apply_order=1)
// 2. lib/libcore.so (apply_order=10)
// 3. lib/libutil.so (depends on libcore, apply_order=20)
// 4. bin/server (depends on both libs, apply_order=30)
DeltaApplyResult result = engine.applyDelta(*manifest);

if (result.success) {
    LOG_INFO("Update successful - {} files patched", result.files_patched.size());
    if (!result.files_fallback.empty()) {
        LOG_WARN("{} files fell back to full download", result.files_fallback.size());
    }
} else {
    LOG_ERROR("Update failed: {}", result.error_message);
}
```

## JSON Format Reference

### FileDelta with Ordering Fields

```json
{
  "path": "bin/app",
  "base_hash": "sha256...",
  "target_hash": "sha256...",
  "patch_url": "https://...",
  "patch_size": 4096,
  "target_size": 16384,
  "algorithm": "zstd_dict",
  "depends_on": ["lib/libcore.so", "lib/libutil.so"],
  "apply_order": 30
}
```

### DeltaManifest with Ordering Fields

```json
{
  "from_version": "1.0.0",
  "to_version": "2.0.0",
  "enforce_order": true,
  "implicit_dependencies": ["etc/config.yaml"],
  "deltas": [
    { ... patch 1 ... },
    { ... patch 2 ... }
  ]
}
```

## Best Practices

1. **Always set enforce_order = true** for new releases unless you have specific reasons not to
2. **Use explicit dependencies** when ordering is critical for correctness
3. **Use implicit dependencies** when many patches share a common prerequisite
4. **Set apply_order hints** to ensure deterministic ordering even when there are no dependencies
5. **Keep dependency graphs simple** - linear chains and diamonds are preferred over complex graphs
6. **Validate manifests** - circular and missing dependencies will cause the update to fail
7. **Test with real patches** - use generatePatch() from CI/CD to create actual patches

## Troubleshooting

### Issue: "Circular dependency detected (7402)"
**Solution:** Review the `depends_on` fields. Use a graph visualization tool to find the cycle.

### Issue: "Dependency file missing in manifest (7404)"
**Solution:** Check that all files in `depends_on` and `implicit_dependencies` are actually in the `deltas` list.

### Issue: Patches still applying in manifest order
**Solution:** Verify that `enforce_order = true` in the manifest.

### Issue: Unexpected patch order
**Solution:** Check `apply_order` hints and `depends_on` lists. Topological sort may order patches differently than expected if ordering is under-specified.
