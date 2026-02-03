---
name: 🔗 Cross-Platform Unit Tests
about: Implement or improve cross-platform unit tests (Linux, Windows, macOS)
title: '[CROSS-UNITTEST] '
labels: ['type:testing', 'area:cross-platform', 'area:unit-tests', 'needs-triage']
assignees: ''
---

## 🎯 Cross-Platform Test Objective / Plattformübergreifendes Test-Ziel

**Component to Test:** <!-- z.B. File I/O, Network Layer, Path Handling -->
**Component Path:** <!-- z.B. src/storage/file_io.cpp, src/network/ -->
**Platforms to Support:**
- [ ] Linux (Ubuntu 22.04+, RHEL 8+)
- [ ] Windows (Windows 10+, Windows Server 2019+)
- [ ] macOS (macOS 12+)
- [ ] Other: <!-- specify -->

**Cross-Platform Issues:**
- [ ] Path separators (`/` vs `\`)
- [ ] Line endings (LF vs CRLF)
- [ ] File permissions (POSIX vs Windows ACL)
- [ ] Network byte order (endianness)
- [ ] Threading APIs (pthreads vs Windows threads)
- [ ] Process management
- [ ] Environment variables
- [ ] Other: <!-- specify -->

---

## 📋 Platform-Specific Considerations / Plattformspezifische Überlegungen

### Linux Specifics / Linux-Spezifika

**System Calls:**
- [ ] `open()`, `read()`, `write()`, `close()`
- [ ] `fork()`, `exec()`
- [ ] POSIX file permissions (`chmod`, `chown`)
- [ ] Signal handling (`signal()`, `sigaction()`)

**File System:**
- Case-sensitive file names
- Forward slash path separator (`/`)
- Symbolic links (`symlink()`)
- Extended attributes

**Build/Test:**
```bash
# Linux build and test
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make test_<component>
./test_<component>
```

---

### Windows Specifics / Windows-Spezifika

**Windows APIs:**
- [ ] `CreateFile()`, `ReadFile()`, `WriteFile()`, `CloseHandle()`
- [ ] `CreateProcess()`
- [ ] Windows ACL permissions
- [ ] Structured Exception Handling (SEH)

**File System:**
- Case-insensitive file names (usually)
- Backslash path separator (`\`)
- Drive letters (`C:\`)
- Short path names (8.3 format)

**Build/Test:**
```powershell
# Windows build and test (PowerShell)
mkdir build; cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Debug
ctest -C Debug -R test_<component>
```

```cmd
REM Windows build and test (CMD)
mkdir build && cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Debug
.\Debug\test_<component>.exe
```

---

### macOS Specifics / macOS-Spezifika

**System Calls:**
- [ ] BSD-style system calls
- [ ] Grand Central Dispatch (GCD)
- [ ] Mach ports
- [ ] FSEvents API (file system monitoring)

**File System:**
- Case-insensitive by default (HFS+/APFS)
- Forward slash path separator (`/`)
- Resource forks (legacy)
- Extended attributes

**Build/Test:**
```bash
# macOS build and test
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
make test_<component>
./test_<component>
```

---

## 🔬 Test Implementation / Test-Implementierung

### Platform Abstraction / Plattform-Abstraktion

```cpp
#include <gtest/gtest.h>

#ifdef _WIN32
    #include <windows.h>
    #define PATH_SEPARATOR "\\"
    #define LINE_ENDING "\r\n"
#else
    #include <unistd.h>
    #include <sys/types.h>
    #define PATH_SEPARATOR "/"
    #define LINE_ENDING "\n"
#endif

// Platform-agnostic path helper
std::string JoinPath(const std::string& base, const std::string& rel) {
#ifdef _WIN32
    return base + "\\" + rel;
#else
    return base + "/" + rel;
#endif
}

// Test with platform-specific expectations
TEST(CrossPlatformTest, PathSeparator) {
    auto path = JoinPath("dir", "file.txt");
    
#ifdef _WIN32
    EXPECT_EQ(path, "dir\\file.txt");
#else
    EXPECT_EQ(path, "dir/file.txt");
#endif
}
```

### Platform-Specific Test Cases / Plattformspezifische Testfälle

```cpp
// Test only on Linux
#ifdef __linux__
TEST(LinuxOnlyTest, PosixSignals) {
    // Test POSIX signal handling
}
#endif

// Test only on Windows
#ifdef _WIN32
TEST(WindowsOnlyTest, WindowsACL) {
    // Test Windows ACL permissions
}
#endif

// Test only on macOS
#ifdef __APPLE__
TEST(MacOSOnlyTest, FSEvents) {
    // Test macOS file system events
}
#endif

// Test on all POSIX platforms (Linux + macOS)
#ifndef _WIN32
TEST(PosixTest, FilePermissions) {
    // Test POSIX file permissions
}
#endif
```

### Portable Test Cases / Portable Testfälle

```cpp
// Test that works on all platforms
TEST(CrossPlatformTest, FileOperations) {
    // Use std::filesystem for portable path handling
    namespace fs = std::filesystem;
    
    // Create temp file
    auto temp_path = fs::temp_directory_path() / "test_file.txt";
    
    // Write
    {
        std::ofstream file(temp_path);
        file << "Test data" << std::endl;
    }
    
    // Read
    {
        std::ifstream file(temp_path);
        std::string content;
        std::getline(file, content);
        EXPECT_EQ(content, "Test data");
    }
    
    // Cleanup
    fs::remove(temp_path);
}
```

---

## ✅ Test Cases / Testfälle

### Path Handling Tests / Pfadbehandlungs-Tests

```cpp
TEST(CrossPlatformPathTest, NormalizePath) {
    namespace fs = std::filesystem;
    
    // Test path normalization
    auto normalized = fs::path("dir/./subdir/../file.txt").lexically_normal();
    EXPECT_EQ(normalized, fs::path("dir/file.txt"));
}

TEST(CrossPlatformPathTest, AbsolutePath) {
    namespace fs = std::filesystem;
    
    auto current = fs::current_path();
    auto absolute = fs::absolute("relative/path");
    
    EXPECT_TRUE(absolute.is_absolute());
    EXPECT_TRUE(absolute.string().find(current.string()) != std::string::npos);
}
```

### Line Ending Tests / Zeilenenden-Tests

```cpp
TEST(CrossPlatformTest, LineEndings) {
    // Test reading file with different line endings
    // File should work regardless of native line ending
    std::stringstream ss;
    ss << "line1\n";   // LF (Unix)
    ss << "line2\r\n"; // CRLF (Windows)
    ss << "line3\n";
    
    std::string line;
    std::vector<std::string> lines;
    while (std::getline(ss, line)) {
        lines.push_back(line);
    }
    
    ASSERT_EQ(lines.size(), 3);
    EXPECT_EQ(lines[0], "line1");
    EXPECT_EQ(lines[1], "line2");
    EXPECT_EQ(lines[2], "line3");
}
```

### Endianness Tests / Endianness-Tests

```cpp
TEST(CrossPlatformTest, NetworkByteOrder) {
    uint32_t host_value = 0x12345678;
    uint32_t network_value = htonl(host_value);
    uint32_t back_to_host = ntohl(network_value);
    
    EXPECT_EQ(back_to_host, host_value);
    
    // Big-endian systems: network_value == host_value
    // Little-endian systems: network_value != host_value (usually)
}
```

---

## 🛠️ Build Integration / Build-Integration

### CMakeLists.txt with Platform Detection

```cmake
# Platform-specific sources
if(WIN32)
    set(PLATFORM_SOURCES
        src/platform/windows_specific.cpp
    )
    set(PLATFORM_LIBS ws2_32) # Winsock2
elseif(APPLE)
    set(PLATFORM_SOURCES
        src/platform/macos_specific.cpp
    )
    set(PLATFORM_LIBS "-framework CoreFoundation")
else() # Linux and other Unix
    set(PLATFORM_SOURCES
        src/platform/linux_specific.cpp
    )
    set(PLATFORM_LIBS pthread)
endif()

# Test executable with platform sources
add_executable(test_cross_platform
    tests/test_cross_platform.cpp
    ${PLATFORM_SOURCES}
)

target_link_libraries(test_cross_platform
    PRIVATE
        themisdb_<component>
        GTest::gtest
        GTest::gtest_main
        ${PLATFORM_LIBS}
)

# Platform-specific compiler flags
if(MSVC)
    target_compile_options(test_cross_platform PRIVATE /W4 /WX)
else()
    target_compile_options(test_cross_platform PRIVATE -Wall -Wextra -Werror)
endif()

# Register tests
gtest_discover_tests(test_cross_platform
    PROPERTIES
        LABELS "unit;cross-platform"
)
```

---

## 🧪 CI/CD Testing / CI/CD-Tests

### GitHub Actions Matrix

```yaml
name: Cross-Platform Tests

on: [push, pull_request]

jobs:
  test:
    strategy:
      matrix:
        os: [ubuntu-22.04, windows-2022, macos-12]
        build_type: [Debug, Release]
        
    runs-on: ${{ matrix.os }}
    
    steps:
      - uses: actions/checkout@v3
      
      - name: Install Dependencies (Ubuntu)
        if: runner.os == 'Linux'
        run: |
          sudo apt-get update
          sudo apt-get install -y cmake g++ libgtest-dev
          
      - name: Install Dependencies (Windows)
        if: runner.os == 'Windows'
        run: |
          choco install cmake
          # vcpkg or other package manager for gtest
          
      - name: Install Dependencies (macOS)
        if: runner.os == 'macOS'
        run: |
          brew install cmake googletest
          
      - name: Build
        run: |
          mkdir build && cd build
          cmake .. -DCMAKE_BUILD_TYPE=${{ matrix.build_type }}
          cmake --build . --config ${{ matrix.build_type }}
          
      - name: Test
        run: |
          cd build
          ctest -C ${{ matrix.build_type }} --output-on-failure
```

---

## ✅ Acceptance Criteria / Akzeptanzkriterien

### Cross-Platform Compatibility / Plattformübergreifende Kompatibilität

- [ ] Tests pass on Linux (Ubuntu 22.04, GCC 11+)
- [ ] Tests pass on Windows (Windows 10+, MSVC 2022)
- [ ] Tests pass on macOS (macOS 12+, Clang 14+)
- [ ] No platform-specific `#ifdef` code unless necessary
- [ ] Portable abstractions used (std::filesystem, std::thread, etc.)

### Code Quality / Code-Qualität

- [ ] No memory leaks on any platform (Valgrind/ASAN on Linux, Dr. Memory on Windows)
- [ ] No undefined behavior (UBSAN clean)
- [ ] No data races (TSAN clean on Linux/macOS)
- [ ] Compiler warnings addressed on all platforms

### Test Quality / Test-Qualität

- [ ] Platform differences documented
- [ ] Platform-specific behavior tested explicitly
- [ ] Fallback behavior defined for unsupported platforms
- [ ] Tests are deterministic on all platforms

### CI/CD / CI/CD

- [ ] Tests run automatically on all platforms
- [ ] Platform-specific failures reported clearly
- [ ] Build artifacts archived for debugging
- [ ] Performance tracked across platforms

---

## 📊 Platform Test Matrix / Plattform-Test-Matrix

| Test Case | Linux | Windows | macOS | Notes |
|-----------|-------|---------|-------|-------|
| File I/O | ✅ | ✅ | ✅ | |
| Path handling | ✅ | ✅ | ✅ | Different separators |
| Line endings | ✅ | ✅ | ✅ | CRLF vs LF |
| Permissions | ✅ | ⚠️ | ✅ | Windows uses ACL |
| Threading | ✅ | ✅ | ✅ | |
| Networking | ✅ | ✅ | ✅ | |
| Signals | ✅ | ❌ | ✅ | Windows uses different model |

**Legend:**
- ✅ Fully supported and tested
- ⚠️ Partially supported (platform differences)
- ❌ Not applicable on this platform

---

## 🔗 References / Referenzen

### Cross-Platform Development
- [C++ Standard Library](https://en.cppreference.com/)
- [std::filesystem](https://en.cppreference.com/w/cpp/filesystem)
- [Platform Detection Macros](https://sourceforge.net/p/predef/wiki/OperatingSystems/)

### Platform-Specific Documentation
- [Windows API](https://docs.microsoft.com/en-us/windows/win32/api/)
- [POSIX Standard](https://pubs.opengroup.org/onlinepubs/9699919799/)
- [Apple Developer](https://developer.apple.com/documentation/)

### Internal Documentation
- [Cross-Platform Guidelines](../../docs/development/cross-platform.md)
- [CI/CD Matrix](../../docs/ci-cd/matrix-testing.md)

---

## 🎓 Best Practices / Best Practices

- [ ] **Use Standard Library**: Prefer `std::filesystem`, `std::thread` over platform-specific APIs
- [ ] **Abstract Platform Differences**: Create portable wrappers
- [ ] **Test on Real Hardware**: CI matrix covering all platforms
- [ ] **Document Platform Behavior**: Make differences explicit
- [ ] **Avoid Assumptions**: Don't assume case-sensitivity, path separators, etc.
- [ ] **Use CMake**: Portable build system
- [ ] **Graceful Degradation**: Handle missing features on some platforms

---

**Created:** <!-- YYYY-MM-DD -->
**Owner:** <!-- Team/Person -->
**Priority:** <!-- P0/P1/P2/P3 -->
**Target Platforms:** <!-- Linux, Windows, macOS -->
**Target Version:** <!-- v1.x.x -->

---

**Template Version:** 1.0.0  
**Created:** 2026-02-03  
**Maintained by:** ThemisDB Cross-Platform Team
