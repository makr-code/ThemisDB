# Windows Build Errors & Solutions

This document catalogs common compiler and linker errors encountered when building ThemisDB on Windows with MSVC, along with their solutions.

## Table of Contents

1. [Symbol Visibility Issues](#symbol-visibility-issues)
2. [Linker Errors](#linker-errors)
3. [Compiler Warnings](#compiler-warnings)
4. [Platform-Specific Issues](#platform-specific-issues)
5. [CMake Configuration](#cmake-configuration)

## Symbol Visibility Issues

### Error: LNK2019 - Unresolved External Symbol

**Symptom**:
```
error LNK2019: unresolved external symbol "public: __cdecl ClassName::method(void)" 
(?method@ClassName@@QEAAXXZ) referenced in function main
```

**Causes**:
1. Missing DLL export on a public class/function
2. Template not explicitly instantiated
3. Missing library in linker dependencies

**Solutions**:

#### 1. Add Export Macro
```cpp
// Before (in header)
class MyClass {
    void publicMethod();
};

// After
class THEMIS_BASE_API MyClass {
    void publicMethod();
};
```

#### 2. Explicit Template Instantiation
```cpp
// In .cpp file
template class std::vector<MyType>;
template void myTemplateFunction<int>();
```

#### 3. Add Library Dependency
```cmake
# In CMakeLists.txt
target_link_libraries(my_target PRIVATE themis_base)
```

### Error: LNK2001 - Unresolved External Symbol (Data)

**Symptom**:
```
error LNK2001: unresolved external symbol "public: static int MyClass::staticVar"
```

**Cause**: Static member variable declared but not defined

**Solution**:
```cpp
// In header
class MyClass {
    static int staticVar;
};

// In .cpp file - ADD THIS
int MyClass::staticVar = 0;
```

### Warning: C4251 - DLL Interface Warning

**Symptom**:
```
warning C4251: 'ClassName::member' : class 'std::vector<T>' needs to have 
dll-interface to be used by clients of class 'ClassName'
```

**Solution**:
```cpp
// Option 1: Suppress warning (if member is private/protected)
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251)
#endif

class THEMIS_API MyClass {
private:
    std::vector<int> data_;  // Won't cause issues if private
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif

// Option 2: Use PIMPL idiom for complex types
class THEMIS_API MyClass {
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};
```

## Linker Errors

### Error: LNK1169 - Multiply Defined Symbols

**Symptom**:
```
error LNK1169: one or more multiply defined symbols found
```

**Causes**:
1. Header-only function without `inline`
2. Same symbol defined in multiple translation units
3. Static library linked multiple times

**Solutions**:

#### 1. Use `inline` for Header Functions
```cpp
// Before
void helperFunction() {
    // ...
}

// After
inline void helperFunction() {
    // ...
}
```

#### 2. Use Anonymous Namespace for Internal Functions
```cpp
// In .cpp file
namespace {
    void internalHelper() {
        // Only visible in this translation unit
    }
}
```

#### 3. Check CMake Link Dependencies
```cmake
# Use PRIVATE to prevent transitive linking
target_link_libraries(my_target PRIVATE dependency)
```

### Error: LNK4098 - Conflicting Library

**Symptom**:
```
warning LNK4098: defaultlib 'MSVCRT' conflicts with use of other libs; use /NODEFAULTLIB:library
```

**Cause**: Mixing debug and release runtime libraries

**Solution**:
```cmake
# Ensure consistent runtime library
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()
```

## Compiler Warnings

### Warning: C4996 - Deprecated Function

**Symptom**:
```
warning C4996: 'strcpy': This function or variable may be unsafe. 
Consider using strcpy_s instead.
```

**Solutions**:

#### 1. Use Safe Alternatives
```cpp
// Before
char buffer[100];
strcpy(buffer, source);

// After
strcpy_s(buffer, sizeof(buffer), source);
// Or use C++ strings
std::string str = source;
```

#### 2. Suppress Warning (if unavoidable)
```cpp
#define _CRT_SECURE_NO_WARNINGS  // At top of file or in CMake
```

```cmake
# In CMakeLists.txt
if(MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
endif()
```

### Warning: C4800 - Implicit Conversion to bool

**Symptom**:
```
warning C4800: 'int': forcing value to bool 'true' or 'false' (performance warning)
```

**Solution**:
```cpp
// Before
bool result = some_int_value;

// After
bool result = (some_int_value != 0);
```

### Warning: C4267 - Conversion from size_t to int

**Symptom**:
```
warning C4267: 'argument': conversion from 'size_t' to 'int', possible loss of data
```

**Solution**:
```cpp
// Before
int count = vec.size();

// After - use correct type
size_t count = vec.size();

// Or if int is required
int count = static_cast<int>(vec.size());
```

## Platform-Specific Issues

### Issue: char8_t Type Conflicts

**Symptom**:
```
error C2440: cannot convert from 'const char [N]' to 'const char8_t *'
```

**Solution**:
```cmake
# In CMakeLists.txt - disable char8_t
if(MSVC)
    add_compile_options(/Zc:char8_t-)
endif()
```

### Issue: Windows.h Conflicts

**Symptom**:
```
error C2039: 'min': is not a member of 'std'
```

**Cause**: Windows.h defines macros for min/max

**Solutions**:

#### 1. Define NOMINMAX
```cpp
#define NOMINMAX
#include <windows.h>
```

```cmake
# In CMakeLists.txt
if(WIN32)
    add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)
endif()
```

#### 2. Use Parentheses
```cpp
// Instead of
auto value = std::min(a, b);

// Use
auto value = (std::min)(a, b);
```

### Issue: Parallel Build PDB Conflicts

**Symptom**:
```
fatal error C1041: cannot open program database; if multiple CL.EXE write to the same .PDB file, please use /FS
```

**Solution**:
```cmake
if(MSVC)
    add_compile_options(/FS)
endif()
```

## CMake Configuration

### Recommended MSVC Flags

```cmake
if(MSVC)
    # Use multiple processors for compilation
    add_compile_options(/MP)
    
    # Enable parallel PDB writes
    add_compile_options(/FS)
    
    # Set warning level
    add_compile_options(/W4)
    
    # Disable specific warnings
    add_compile_options(
        /wd4251  # DLL interface warning
        /wd4275  # Non-DLL-interface class used as base
    )
    
    # Enable standards conformance
    add_compile_options(/permissive-)
    
    # Disable char8_t
    add_compile_options(/Zc:char8_t-)
    
    # Set exception handling model
    add_compile_options(/EHsc)
    
    # Optimize for speed in Release
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(/O2 /Ob2)
    endif()
endif()
```

### Export All Symbols (Temporary Workaround)

```cmake
# NOT RECOMMENDED - causes symbol bloat
# Use explicit export macros instead
if(MSVC)
    set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()
```

## Testing

To test Windows builds:

1. **Configure**:
   ```powershell
   cmake -B build -G "Visual Studio 17 2022" -A x64
   ```

2. **Build**:
   ```powershell
   cmake --build build --config Release
   ```

3. **Check for warnings**:
   ```powershell
   cmake --build build --config Release 2>&1 | findstr /I "warning error"
   ```

## References

- [MSVC Compiler Options](https://docs.microsoft.com/en-us/cpp/build/reference/compiler-options)
- [MSVC Linker Options](https://docs.microsoft.com/en-us/cpp/build/reference/linker-options)
- [CMake MSVC Documentation](https://cmake.org/cmake/help/latest/variable/MSVC.html)
- [Export Macro Best Practices](../CONTRIBUTING_PLATFORM_GUIDELINES.md)

## Getting Help

If you encounter an error not covered here:

1. Run the diagnostic scanner:
   ```powershell
   python tools/compiler_diagnostics/diagnostic_scanner.py build.log
   ```

2. Check the [Platform Compatibility Matrix](PLATFORM_COMPATIBILITY_MATRIX.md)

3. See [COMPILER_TROUBLESHOOTING.md](COMPILER_TROUBLESHOOTING.md)
