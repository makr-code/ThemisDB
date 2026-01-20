# Plugin Hot-Plug Filesystem Monitoring

## Overview

The Plugin Hot-Plug Monitoring system provides automatic detection and management of plugins based on filesystem events. This feature enables:

- **Automatic plugin discovery**: New plugins are detected when added to the watch directory
- **Hot-reload**: Modified plugins can be automatically reloaded without server restart
- **Automatic cleanup**: Deleted plugins are automatically unloaded

## Architecture

### Components

1. **PluginHotPlugMonitor**: Platform-specific filesystem watcher
   - Linux: Uses `inotify` API
   - Windows: Uses `ReadDirectoryChangesW` API
   - macOS: Uses `kqueue` API

2. **Integration with PluginManager**: Seamless integration with existing plugin management infrastructure

### Event Types

- `FileEvent::CREATED`: New plugin file detected
- `FileEvent::MODIFIED`: Existing plugin file modified
- `FileEvent::DELETED`: Plugin file removed

## Configuration

```cpp
struct HotPlugConfig {
    bool enabled = false;        // Enable/disable hot-plug monitoring
    bool auto_load = true;       // Auto-load new plugins
    bool auto_reload = true;     // Auto-reload modified plugins
    bool auto_unload = true;     // Auto-unload deleted plugins
    int watch_interval_ms = 100; // Polling interval (fallback mode)
};
```

## Usage

### Basic Usage

```cpp
#include "plugins/plugin_manager.h"
#include "plugins/plugin_hot_plug_monitor.h"

using namespace themis::plugins;

// Get plugin manager instance
auto& manager = PluginManager::instance();

// Configure hot-plug monitoring
HotPlugConfig config;
config.enabled = true;
config.auto_load = true;
config.auto_reload = true;
config.auto_unload = true;

// Enable monitoring for plugin directory
bool success = manager.enableHotPlug("./plugins", config);
if (success) {
    std::cout << "Hot-plug monitoring enabled" << std::endl;
}

// ... application runs ...

// Disable monitoring on shutdown
manager.disableHotPlug();
```

### Advanced Configuration

```cpp
// Only auto-load, no reload or unload
HotPlugConfig config;
config.enabled = true;
config.auto_load = true;
config.auto_reload = false;
config.auto_unload = false;

manager.enableHotPlug("./plugins", config);
```

### Check Status

```cpp
if (manager.isHotPlugEnabled()) {
    std::cout << "Hot-plug monitoring is active" << std::endl;
}
```

## Platform-Specific Behavior

### Linux (inotify)

- Monitors: `IN_CREATE`, `IN_MODIFY`, `IN_DELETE` events
- Non-blocking I/O with `poll()` for interruptibility
- Efficient handling of multiple events in single buffer

### Windows (ReadDirectoryChangesW)

- Monitors: `FILE_ACTION_ADDED`, `FILE_ACTION_MODIFIED`, `FILE_ACTION_REMOVED`
- Asynchronous directory change notifications
- Handles subdirectories recursively

### macOS (kqueue)

- Uses `EVFILT_VNODE` with `NOTE_WRITE`, `NOTE_EXTEND`, `NOTE_DELETE`
- Periodic directory scanning to detect specific file changes
- File descriptor-based monitoring

## File Detection

The monitor detects the following plugin-related files:

- `.dll` (Windows shared libraries)
- `.so` (Linux shared libraries)
- `.dylib` (macOS dynamic libraries)
- `plugin.json` (Plugin manifests)

## Thread Safety

The hot-plug monitor is fully thread-safe:

- Uses `std::atomic<bool>` for running state
- Background thread for filesystem monitoring
- Proper synchronization with PluginManager's mutex
- Clean shutdown with thread join

## Event Handling Flow

1. **File Created**
   ```
   New file detected
   → Wait 500ms (ensure file is fully written)
   → Scan plugin directory
   → Load plugin if auto_load enabled
   → Log success/failure
   ```

2. **File Modified**
   ```
   Modification detected
   → Wait 500ms (ensure write is complete)
   → Check if plugin is loaded
   → Reload plugin if auto_reload enabled
   → Log success/failure
   ```

3. **File Deleted**
   ```
   Deletion detected
   → Check if plugin is loaded
   → Unload plugin if auto_unload enabled
   → Log completion
   ```

## Performance Considerations

- **Minimal overhead**: Platform-native APIs with efficient event delivery
- **Non-blocking**: Background thread doesn't block main application
- **Debouncing**: 500ms wait after file events to handle incomplete writes
- **Selective monitoring**: Only processes plugin-related file types

## Error Handling

The monitor gracefully handles:

- Non-existent watch directories (fails during `start()`)
- Permission errors (logged as errors)
- Multiple start attempts (returns false on second attempt)
- Interrupted system calls (EINTR)
- File read/write races (debouncing)

## Testing

Comprehensive test suite in `tests/test_plugin_hot_plug.cpp`:

- Enable/disable monitoring
- Double-enable prevention
- Invalid directory handling
- New plugin detection
- Plugin modification detection
- Plugin deletion detection
- Thread safety with concurrent operations

## Example Scenarios

### Development Workflow

```cpp
// Enable hot-reload for development
HotPlugConfig config;
config.auto_load = true;
config.auto_reload = true;
manager.enableHotPlug("./dev_plugins", config);

// Now you can:
// 1. Add new plugin → automatically loaded
// 2. Modify plugin → automatically reloaded
// 3. Remove plugin → automatically unloaded
```

### Production Deployment

```cpp
// Only auto-load new plugins in production
HotPlugConfig config;
config.auto_load = true;
config.auto_reload = false;  // Manual control for safety
config.auto_unload = false;  // Manual control for safety
manager.enableHotPlug("/var/lib/themisdb/plugins", config);

// New plugins are loaded automatically
// But modifications/deletions require manual intervention
```

### Testing Environment

```cpp
// Disable auto-actions for controlled testing
HotPlugConfig config;
config.enabled = true;
config.auto_load = false;
config.auto_reload = false;
config.auto_unload = false;
manager.enableHotPlug("./test_plugins", config);

// Monitor detects changes but doesn't take action
// Test code controls plugin lifecycle manually
```

## Limitations

1. **Partial Write Detection**: The 500ms debounce helps but doesn't guarantee complete writes for very large files
2. **Platform Differences**: Behavior may vary slightly between platforms due to API differences
3. **File System Events**: Some network filesystems may not support change notifications
4. **Resource Usage**: Each monitor thread consumes system resources

## Best Practices

1. **Use appropriate auto-actions**: Enable auto-reload only in development environments
2. **Monitor plugin directory only**: Don't watch entire application directory
3. **Proper cleanup**: Always call `disableHotPlug()` before application shutdown
4. **Error logging**: Monitor logs for any plugin loading failures
5. **Test thoroughly**: Use the test suite to validate behavior on your platform

## Future Enhancements

Potential future improvements:

- Configurable debounce delay
- File content hashing for change detection
- Plugin dependency-aware reload ordering
- Event throttling for rapid changes
- Custom event handlers/callbacks
- Metrics and monitoring integration

## References

- Plugin System Documentation: `docs/de/plugins/PLUGIN_SYSTEM_INTEGRATION.md`
- Plugin Consistency Analysis: `docs/de/plugins/PLUGIN_SYSTEM_CONSISTENCY_ANALYSIS.md`
- PluginManager API: `include/plugins/plugin_manager.h`
- Test Suite: `tests/test_plugin_hot_plug.cpp`
