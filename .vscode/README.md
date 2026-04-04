# VSCode Configuration for ThemisDB

This directory contains enhanced VSCode configuration for ThemisDB development.

## Files Overview

### `c_cpp_properties.json`
Advanced IntelliSense configuration for multiple platforms:
- **Linux (GCC)** - Default Linux development
- **Linux (Clang)** - Alternative with Clang
- **Windows (MSVC)** - Visual Studio 2022
- **WSL (Ubuntu)** - Windows Subsystem for Linux
- **ARM64 (Cross-compile)** - Raspberry Pi / ARM servers

Features:
- Auto-detection of vcpkg include paths
- Platform-specific compiler settings
- Integration with CMake Tools via `configurationProvider`
- Support for `compile_commands.json`

### `cmake-variants.json`
Build type variants for CMake Tools extension:
- **Debug** - Development with symbols, no optimizations
- **Release** - Full optimizations
- **RelWithDebInfo** - Release with debug symbols
- **MinSizeRel** - Minimal binary size
- **Debug-LLM** - Debug with LLM features enabled
- **Release-Full** - All features (LLM + GPU + RPC)

### `cmake-kits.json`
Compiler kit definitions:
- **GCC** - GNU Compiler Collection
- **Clang** - LLVM Clang
- **MSVC 2022** - Microsoft Visual C++
- **WSL GCC** - GCC in Windows Subsystem for Linux
- **ARM64 Cross** - Cross-compilation for ARM64

## Quick Start

### 1. First Time Setup

```bash
# Copy example configuration (if needed)
cp -r .vscode.example .vscode

# Or use the enhanced configuration already in .vscode/
```

### 2. Select Configuration

**IntelliSense:**
- Status bar (bottom right) → Click on the configuration name
- Or: Command Palette → "C/C++: Select a Configuration"

**CMake Kit:**
- Status bar → Click on "No Kit Selected"
- Or: Command Palette → "CMake: Select a Kit"
- Choose based on your platform (GCC, Clang, MSVC, etc.)

**Build Variant:**
- Status bar → Click on build type
- Or: Command Palette → "CMake: Select Variant"
- Choose based on what you're working on

### 3. Build and Run

```bash
# Configure (first time or after CMakeLists.txt changes)
F7 or Ctrl+Shift+P → "CMake: Configure"

# Build
F7 or Ctrl+Shift+P → "CMake: Build"

# Run tests
Ctrl+Shift+P → "CMake: Run Tests"

# Debug
F5 (with launch configuration selected)
```

## Configuration Selection Guide

### For Daily Development

**Platform: Linux**
- Kit: GCC
- Variant: Debug
- Config: Linux (GCC)

**Platform: Windows**
- Kit: MSVC 2022
- Variant: Debug
- Config: Windows (MSVC)

**Platform: WSL**
- Kit: WSL GCC
- Variant: Debug
- Config: WSL (Ubuntu)

### For Testing LLM Features

- Kit: GCC or MSVC (your platform)
- Variant: **Debug-LLM**
- Config: Your platform

### For Performance Testing

- Kit: GCC or MSVC (your platform)
- Variant: **Release** or **Release-Full**
- Config: Your platform

### For ARM/Raspberry Pi Development

- Kit: **ARM64 Cross**
- Variant: Release
- Config: ARM64 (Cross-compile)

## IntelliSense Troubleshooting

### Issue: IntelliSense not working

**Solution 1: Regenerate compile_commands.json**
```bash
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

**Solution 2: Select correct configuration**
- Status bar → Select your platform configuration
- Should match your active build directory

**Solution 3: Restart IntelliSense**
- Command Palette → "C/C++: Reset IntelliSense Database"
- Restart VSCode

### Issue: Wrong includes highlighted as errors

**Check:**
1. `vcpkg_installed/` directory exists
2. Correct triplet in `c_cpp_properties.json`
3. CMake has configured successfully
4. `compile_commands.json` exists in build directory

### Issue: CMake Tools not detecting kits

**Solution:**
1. Command Palette → "CMake: Scan for Kits"
2. Check compiler is installed: `gcc --version` or `cl.exe`
3. Verify paths in `cmake-kits.json`

## Customization

### Add Custom Build Variant

Edit `cmake-variants.json`:

```json
{
  "MyVariant": {
    "short": "MyVar",
    "long": "My custom build configuration",
    "buildType": "Debug",
    "settings": {
      "CMAKE_BUILD_TYPE": "Debug",
      "MY_CUSTOM_FLAG": "ON"
    }
  }
}
```

### Add Custom Compiler Kit

Edit `cmake-kits.json`:

```json
{
  "name": "My Compiler",
  "compilers": {
    "C": "/path/to/my/gcc",
    "CXX": "/path/to/my/g++"
  },
  "preferredGenerator": {
    "name": "Ninja"
  }
}
```

### Adjust IntelliSense Settings

Edit `c_cpp_properties.json`:

```json
{
  "configurations": [{
    "name": "My Config",
    "includePath": [
      "${workspaceFolder}/**",
      "/path/to/custom/includes"
    ],
    "defines": ["MY_DEFINE"],
    "compilerPath": "/path/to/compiler"
  }]
}
```

## Integration with CMake Presets

The configurations in this directory work alongside CMake presets defined in `cmake/CMakePresets.json`.

**Relationship:**
- **CMake Presets** - Define build configurations (configure + build args)
- **CMake Kits** - Define compiler toolchains
- **CMake Variants** - VSCode-specific build type selection
- **c_cpp_properties.json** - IntelliSense configuration (language server)

**Recommended workflow:**
1. Select Kit (compiler) → `cmake-kits.json`
2. Select Variant (build type) → `cmake-variants.json`
3. CMake Tools combines these with presets
4. IntelliSense uses `c_cpp_properties.json` + `compile_commands.json`

## Additional Resources

- [VSCode C++ Docs](https://code.visualstudio.com/docs/languages/cpp)
- [CMake Tools Extension](https://github.com/microsoft/vscode-cmake-tools)
- [ThemisDB VSCode Guide](.github/copilot/VSCODE_CONTEXT.md)
- [ThemisDB Build Guide](.github/copilot/BUILD_GUIDE.md)

---

**Note:** This enhanced configuration is optional. The basic setup in `.vscode.example/` works fine for most users. Use this enhanced config if you:
- Develop on multiple platforms
- Need advanced IntelliSense configuration
- Want fine-grained build variant control
- Do cross-compilation (ARM64)
