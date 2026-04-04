# ThemisDB - VSCode Context & Remote Development

## VSCode Setup Overview

ThemisDB provides optimized VSCode configurations for:
- Local development (Windows, Linux, macOS)
- Remote development (SSH, WSL, Dev Containers)
- Integrated debugging and testing
- IntelliSense with vcpkg integration

## Quick Start

### Initial Setup

1. **Copy example configuration:**
   ```bash
   cp -r .vscode.example .vscode
   ```

2. **Install recommended extensions:**
   - Open Command Palette (`Ctrl+Shift+P`)
   - Run: "Extensions: Show Recommended Extensions"
   - Install all recommended extensions

3. **Configure CMake:**
   - Open Command Palette
   - Run: "CMake: Select a Kit"
   - Choose appropriate compiler

### Recommended Extensions

Essential extensions (defined in `.vscode/extensions.json`):

```json
{
  "recommendations": [
    "ms-vscode.cpptools",              // C/C++ IntelliSense
    "ms-vscode.cmake-tools",           // CMake integration
    "ms-vscode.cpptools-extension-pack", // C++ tools bundle
    "twxs.cmake",                      // CMake language support
    "ms-vscode-remote.remote-ssh",     // Remote SSH
    "ms-vscode-remote.remote-wsl",     // WSL integration
    "ms-vscode-remote.remote-containers", // Dev Containers
    "github.copilot",                  // GitHub Copilot
    "github.copilot-chat"              // Copilot Chat
  ]
}
```

## Remote Development Modes

### 1. WSL (Windows Subsystem for Linux)

**Best for:** Windows developers targeting Linux

**Setup:**
```bash
# In WSL terminal
cd /path/to/themis
code .
```

**Benefits:**
- Native Linux build environment
- Access to Linux tools
- Windows UI with Linux backend

**Configuration:**
```json
// .vscode/settings.json
{
  "remote.WSL.useShellEnvironment": true,
  "cmake.buildDirectory": "${workspaceFolder}/build-wsl"
}
```

### 2. Remote SSH

**Best for:** Developing on remote Linux servers

**Setup:**
1. Install "Remote - SSH" extension
2. Open Command Palette → "Remote-SSH: Connect to Host"
3. Configure SSH host in `~/.ssh/config`:

```ssh-config
Host themis-dev
    HostName your-server.com
    User youruser
    IdentityFile ~/.ssh/id_rsa
    ForwardAgent yes
```

**Benefits:**
- Full development on powerful remote machines
- Same VSCode experience as local
- GPU/high-memory systems

### 3. Dev Containers

**Best for:** Reproducible development environment

**Setup:**
```bash
# Open in container
code .
# Command Palette → "Dev Containers: Reopen in Container"
```

**Configuration:** `.devcontainer/devcontainer.json`

**Benefits:**
- Isolated environment
- Same setup for all developers
- Pre-configured tools and dependencies

## CMake Integration

### CMake Presets

VSCode automatically detects `cmake/CMakePresets.json`:

```json
// .vscode/settings.json
{
  "cmake.configureOnOpen": true,
  "cmake.useCMakePresets": "always",
  "cmake.buildDirectory": "${workspaceFolder}/build-${presetName}"
}
```

### Preset Selection

1. Open Command Palette
2. "CMake: Select Configure Preset"
3. Choose:
   - `windows-vs2022-release` (Windows)
   - `linux-gcc-release` (Linux)
   - `docker-ninja-release` (Docker)

### Building

- **Configure:** `Ctrl+Shift+P` → "CMake: Configure"
- **Build:** `Ctrl+Shift+P` → "CMake: Build" or `F7`
- **Clean:** `Ctrl+Shift+P` → "CMake: Clean"

## IntelliSense Configuration

### vcpkg Integration

Automatic IntelliSense with vcpkg:

```json
// .vscode/settings.json
{
  "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
  "C_Cpp.default.compileCommands": "${workspaceFolder}/build/compile_commands.json"
}
```

### Include Paths

If IntelliSense doesn't work:

```json
// .vscode/c_cpp_properties.json
{
  "configurations": [
    {
      "name": "Linux",
      "includePath": [
        "${workspaceFolder}/**",
        "${workspaceFolder}/vcpkg_installed/x64-linux/include"
      ],
      "compilerPath": "/usr/bin/g++",
      "cStandard": "c17",
      "cppStandard": "c++17",
      "intelliSenseMode": "linux-gcc-x64"
    }
  ]
}
```

## Debugging

### Launch Configurations

`.vscode/launch.json` provides debug configurations:

**Debug Server:**
```json
{
  "name": "Debug Server (GDB)",
  "type": "cppdbg",
  "request": "launch",
  "program": "${workspaceFolder}/build/themis_server",
  "args": ["--config", "config/dev.json"],
  "cwd": "${workspaceFolder}",
  "MIMode": "gdb"
}
```

**Debug Tests:**
```json
{
  "name": "Debug Tests (GDB)",
  "type": "cppdbg",
  "request": "launch",
  "program": "${workspaceFolder}/build/tests/storage_tests",
  "args": ["--gtest_filter=RocksDB*"],
  "MIMode": "gdb"
}
```

### Debugging Steps

1. Set breakpoints in code (click left margin)
2. Press `F5` or select debug configuration
3. Use debug toolbar:
   - Continue: `F5`
   - Step Over: `F10`
   - Step Into: `F11`
   - Step Out: `Shift+F11`

### Conditional Breakpoints

Right-click breakpoint → "Edit Breakpoint":
```cpp
// Break only when condition is true
id == 42
count > 1000
user_name == "admin"
```

## Testing Integration

### Running Tests

**Via CMake Tools:**
- Press `Ctrl+Shift+P`
- "CMake: Run Tests"

**Via Test Explorer:**
- Install "C++ TestMate" extension
- View → Testing
- Run individual tests

### Test Output

```json
// .vscode/settings.json
{
  "cmake.testExplorerIntegrationEnabled": true,
  "testExplorer.useNativeTesting": true
}
```

## Tasks

`.vscode/tasks.json` defines common tasks:

**Build Task** (`Ctrl+Shift+B`):
```json
{
  "label": "Build (CMake)",
  "type": "cmake",
  "command": "build",
  "group": {
    "kind": "build",
    "isDefault": true
  }
}
```

**Format Code:**
```json
{
  "label": "Format (clang-format)",
  "type": "shell",
  "command": "clang-format -i ${file}",
  "group": "none"
}
```

## Live Development

### File Watcher

Auto-rebuild on file changes:

```json
// .vscode/settings.json
{
  "files.watcherExclude": {
    "**/build-*/**": true,
    "**/vcpkg_installed/**": true
  }
}
```

### Auto-save

```json
{
  "files.autoSave": "onFocusChange",
  "files.autoSaveDelay": 1000
}
```

## Code Formatting

### Clang-Format Integration

```json
// .vscode/settings.json
{
  "editor.formatOnSave": true,
  "C_Cpp.formatting": "clangFormat",
  "C_Cpp.clang_format_path": "/usr/bin/clang-format",
  "C_Cpp.clang_format_style": "file"  // Uses .clang-format
}
```

### Format on Save

Automatically format C++ files on save:
```json
{
  "[cpp]": {
    "editor.formatOnSave": true
  }
}
```

## Git Integration

### Built-in Git Support

- **View Changes:** Source Control panel (`Ctrl+Shift+G`)
- **Stage Files:** Click `+` icon
- **Commit:** Enter message and press `Ctrl+Enter`
- **Push:** Click `...` → Push

### GitLens Extension

Enhanced Git capabilities:
```json
{
  "recommendations": [
    "eamodio.gitlens"
  ]
}
```

## Performance Optimization

### Large Workspace

```json
{
  "files.exclude": {
    "**/build-*": true,
    "**/vcpkg_installed": true,
    "**/.git": true
  },
  "search.exclude": {
    "**/build-*": true,
    "**/vcpkg_installed": true
  }
}
```

### IntelliSense Performance

```json
{
  "C_Cpp.intelliSenseCachePath": "${workspaceFolder}/.vscode/ipch",
  "C_Cpp.intelliSenseCacheSize": 5120  // 5GB
}
```

## Remote Development Best Practices

### 1. Use Remote Extensions

Install extensions on remote host, not locally:
- Extensions panel → Filter by "Remote"
- Install extensions in remote environment

### 2. Port Forwarding

Forward ports for web services:
```json
// .devcontainer/devcontainer.json
{
  "forwardPorts": [8080, 9090],
  "portsAttributes": {
    "8080": {"label": "ThemisDB Server"},
    "9090": {"label": "Metrics"}
  }
}
```

### 3. Persistent Settings

Use workspace settings for team:
```json
// .vscode/settings.json (checked in)
{
  "cmake.buildDirectory": "${workspaceFolder}/build-${presetName}"
}
```

Use user settings for personal preferences:
- `Ctrl+,` → Settings → User

## Troubleshooting

### IntelliSense Not Working

1. Check compile_commands.json exists:
   ```bash
   ls build/compile_commands.json
   ```

2. Regenerate CMake cache:
   - Command Palette → "CMake: Delete Cache and Reconfigure"

3. Restart IntelliSense:
   - Command Palette → "C/C++: Reset IntelliSense Database"

### CMake Preset Not Found

Ensure `cmake/CMakePresets.json` exists and is valid:
```bash
cmake --list-presets
```

### Build Errors

Check CMake output:
- View → Output → CMake/Build

Enable verbose output:
```json
{
  "cmake.buildArgs": ["-v"]
}
```

## Additional Resources

- VSCode C++ Documentation: https://code.visualstudio.com/docs/languages/cpp
- CMake Tools: https://github.com/microsoft/vscode-cmake-tools
- Remote Development: https://code.visualstudio.com/docs/remote/remote-overview
- Example Configuration: `.vscode.example/` in repository
