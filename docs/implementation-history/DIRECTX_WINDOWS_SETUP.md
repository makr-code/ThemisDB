# DirectX 12 Backend - Windows Setup Guide

## Prerequisites

### Required Software
- **Windows 10** version 1809+ or **Windows 11**
- **Windows 10 SDK** 10.0.19041.0 or later
- **Visual Studio 2019** or later (or Build Tools for Visual Studio)
- **CMake** 3.20 or later

### DirectX 12 Requirements
- DirectX 12 compatible GPU (NVIDIA, AMD, or Intel)
- DirectX 12 runtime (included in Windows 10 1809+)
- Shader Model 6.0+ support

## Installation Steps

### 1. Install Windows 10 SDK

Download and install the Windows 10 SDK from:
https://developer.microsoft.com/en-us/windows/downloads/windows-sdk/

**Recommended SDK versions:**
- Windows 10 SDK 10.0.22621.0 (Windows 11)
- Windows 10 SDK 10.0.19041.0 (Windows 10)

During installation, ensure you select:
- ✅ Windows SDK for Desktop C++ Apps
- ✅ Windows SDK Signing Tools for Desktop Apps

### 2. Verify DXC Compiler

The DirectX Shader Compiler (DXC) is included with the Windows SDK.

Verify installation by running:
```cmd
where dxc
```

**Typical locations:**
- `C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64\dxc.exe`
- `C:\Program Files (x86)\Windows Kits\10\bin\10.0.19041.0\x64\dxc.exe`

### 3. Build ThemisDB with DirectX Support

```bash
# Configure with DirectX enabled
cmake -B build -G "Visual Studio 17 2022" -A x64 ^
  -DTHEMIS_ENABLE_DIRECTX=ON ^
  -DTHEMIS_ENABLE_LLM=ON ^
  -DTHEMIS_BUILD_TESTS=ON

# Build
cmake --build build --config Release

# Run tests
cd build
ctest -C Release -R directx --output-on-failure
```

### 4. Verify Shader Compilation

After building, check that shaders were compiled:

```cmd
dir build\shaders\lora\*.cso
```

You should see:
- `matmul.cso`
- `elementwise.cso`
- `gradient.cso`

## GPU Compatibility

### Supported GPUs

**NVIDIA:**
- GeForce GTX 900 series and newer
- GeForce RTX series (all)
- Quadro/Tesla with Maxwell architecture or newer

**AMD:**
- Radeon RX 400 series and newer
- Radeon RX 5000/6000/7000 series
- Radeon Pro with GCN 3.0 or newer

**Intel:**
- Intel HD Graphics 500 series and newer
- Intel Iris Xe Graphics
- Intel Arc Graphics (all)

### Check GPU Compatibility

Run the following PowerShell script to check your GPU:

```powershell
# Check DirectX 12 support
dxdiag /t dxdiag_output.txt
notepad dxdiag_output.txt
```

Look for:
- **Feature Level**: Should be 12_0 or higher
- **Driver Model**: Should be WDDM 2.0 or higher

## Troubleshooting

### DXC Compiler Not Found

**Symptoms:**
```
CMake Warning: DXC compiler not found - DirectX shaders will not be compiled
```

**Solution:**
1. Install Windows 10 SDK (see step 1)
2. Or manually set DXC path in CMake:
   ```bash
   cmake -B build -DDXC_COMPILER="C:\Path\To\dxc.exe" -DTHEMIS_ENABLE_DIRECTX=ON
   ```

### Shader Compilation Failed

**Symptoms:**
```
Error: Compiling matmul.hlsl to DXIL bytecode failed
```

**Solutions:**
1. Check shader syntax errors in build log
2. Verify Windows SDK version supports Shader Model 6.0+
3. Update GPU drivers to latest version

### DirectX Device Initialization Failed

**Symptoms:**
```
DirectX: Failed to initialize context
Failed to create D3D12 device for adapter 0
```

**Solutions:**
1. Update GPU drivers from manufacturer website:
   - NVIDIA: https://www.nvidia.com/download/index.aspx
   - AMD: https://www.amd.com/en/support
   - Intel: https://www.intel.com/content/www/us/en/download-center/home.html

2. Enable DirectX 12 in Windows:
   ```cmd
   # Run as Administrator
   dism /online /add-capability /capabilityname:DirectX.Configuration.Database~~~~0.0.1.0
   ```

3. Check GPU compatibility (see section above)

### Shader Files Not Found at Runtime

**Symptoms:**
```
Failed to load shader: matmul.cso from C:\path\to\shaders\lora\matmul.cso
```

**Solutions:**
1. Ensure shaders are copied to executable directory:
   ```cmd
   cmake --install build --config Release
   ```

2. Or manually copy shaders:
   ```cmd
   mkdir bin\shaders\lora
   copy build\shaders\lora\*.cso bin\shaders\lora\
   ```

### Debug Layer Messages

**Enable DirectX Debug Layer** (Debug builds only):

1. Install Graphics Tools:
   - Open **Settings** → **Apps** → **Optional Features**
   - Click **Add a feature**
   - Search for **Graphics Tools**
   - Install

2. Debug layer is automatically enabled in Debug builds
3. Check console output for DirectX warnings and errors

### Performance Issues

**Symptoms:** Slower than expected performance

**Solutions:**
1. Build in Release mode (not Debug):
   ```bash
   cmake --build build --config Release
   ```

2. Update GPU drivers to latest version

3. Check GPU usage with Task Manager (Performance tab)

4. Profile with PIX for Windows:
   - Download from: https://devblogs.microsoft.com/pix/download/

## Environment Variables

### Optional Configuration

```cmd
# Force specific GPU adapter (0-based index)
set THEMIS_DIRECTX_ADAPTER=0

# Enable verbose DirectX logging
set THEMIS_DIRECTX_VERBOSE=1

# Shader cache directory (default: current directory)
set THEMIS_SHADER_PATH=C:\path\to\shaders
```

## Testing

### Run DirectX Backend Tests

```bash
# All DirectX tests
ctest -R directx --output-on-failure

# Specific test
ctest -R DirectXBackendTest.MatMulKernel --output-on-failure

# Verbose output
ctest -R directx --output-on-failure -V
```

### Manual Testing

Create a simple test program:

```cpp
#include "llm/lora_framework/directx_kernels.h"
#include <vector>
#include <iostream>

int main() {
    using namespace themis::lora::directx;
    
    // Check availability
    if (!is_directx_available()) {
        std::cerr << "DirectX 12 not available\n";
        return 1;
    }
    
    // Initialize
    if (!initialize_directx_lora(0)) {
        std::cerr << "Failed to initialize DirectX\n";
        return 1;
    }
    
    // Simple matmul test
    std::vector<float> A(16, 1.0f);
    std::vector<float> B(16, 2.0f);
    std::vector<float> C(16, 0.0f);
    
    launch_matmul_shader(A.data(), B.data(), C.data(), 4, 4, 4, 1.0f);
    
    std::cout << "DirectX backend working! Result[0] = " << C[0] << "\n";
    
    cleanup_directx_lora();
    return 0;
}
```

## Performance Benchmarking

### Expected Performance

| Operation | Size | Target Time | Speedup vs CPU |
|-----------|------|-------------|----------------|
| MatMul | 768×768 | 0.1 ms | 100x |
| Element-wise | 1M elements | 0.02 ms | 100x |
| Training step | Full | 3.5 ms | 45x |

### Run Benchmarks

```bash
# Build benchmarks
cmake --build build --config Release --target bench_lora_gpu

# Run
build\Release\bench_lora_gpu.exe
```

## Advanced Configuration

### Custom Shader Compilation

Manually compile shaders with custom options:

```cmd
dxc -T cs_6_0 -E main ^
    -O3 ^
    -Zi ^
    -Qembed_debug ^
    matmul.hlsl ^
    -Fo matmul.cso
```

**Options:**
- `-O3`: Maximum optimization
- `-Zi`: Debug info
- `-Qembed_debug`: Embed debug info in bytecode
- `-T cs_6_0`: Target Shader Model 6.0

### PIX for Windows Profiling

1. Download PIX: https://devblogs.microsoft.com/pix/download/
2. Launch PIX
3. Select **GPU Capture**
4. Set target executable to ThemisDB
5. Start capture
6. Analyze shader performance

## Support

### Reporting Issues

When reporting DirectX issues, include:
1. Windows version (`winver`)
2. GPU model and driver version
3. Windows SDK version
4. CMake configuration output
5. Error messages from console
6. DXC compiler version (`dxc --version`)

### Additional Resources

- **DirectX 12 Programming Guide**: https://docs.microsoft.com/en-us/windows/win32/direct3d12/
- **DirectX Shader Compiler**: https://github.com/microsoft/DirectXShaderCompiler
- **PIX for Windows**: https://devblogs.microsoft.com/pix/
- **Windows Graphics Documentation**: https://docs.microsoft.com/en-us/windows/win32/direct3d12/directx-12-programming-guide

---

**Last Updated**: 2026-04-06  
**DirectX Version**: 12 (Shader Model 6.0)  
**Status**: Production Ready
