# VS Code Configuration

This directory contains recommended VS Code settings for ThemisDB development.

## Installation

Copy these files to your `.vscode/` directory:

```bash
cp -r .vscode.example/* .vscode/
```

Or on Windows:
```powershell
Copy-Item -Recurse .vscode.example\* .vscode\
```

## What's Included

### `settings.json`
- File associations for AQL, CMake, and Protocol Buffers
- Editor formatting and code style settings
- C++ IntelliSense configuration
- Search and file exclusions for build artifacts

### `launch.json`
- **Debug ThemisDB Server**: Debug the main server process
- **Debug Tests**: Debug test suite
- **Attach to ThemisDB Process**: Attach debugger to running process

### `tasks.json`
- **cmake-configure**: Configure CMake build
- **cmake-build-debug**: Build server in debug mode
- **cmake-build-tests**: Build test suite
- **run-tests**: Run all tests
- **clean**: Clean build directory

### `extensions.json`
- Recommended VS Code extensions for C++ development
- CMake tools
- Markdown support
- Git integration
- Docker support

## Platform Note for `cmake.buildDirectory`

The template uses a platform-agnostic build directory pattern:

```json
{
  "cmake.buildDirectory": "${workspaceFolder}/build/${buildType}"
}
```

If you use custom preset names or multiple local configurations, override it in your local `.vscode/settings.json`.

## Note on `cmake.configureOnOpen`

`cmake.configureOnOpen` is enabled in the template for quick onboarding. If you prefer manual configuration, override locally:

```json
{
  "cmake.configureOnOpen": false
}
```

## Usage

### Build and Debug
1. Press `Ctrl+Shift+B` (or `Cmd+Shift+B` on macOS) to build
2. Press `F5` to start debugging
3. Set breakpoints by clicking in the gutter

### Run Tests
1. Open Command Palette (`Ctrl+Shift+P` or `Cmd+Shift+P`)
2. Type "Run Task"
3. Select "run-tests"

### Install Recommended Extensions
VS Code will prompt you to install recommended extensions when you open the project.

## Customization

Feel free to customize these settings for your development workflow. The `.vscode/` directory is gitignored to allow personal preferences.
