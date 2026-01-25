# Compiler Troubleshooting Guide

This guide provides common error patterns, their root causes, and systematic solutions for cross-compiler issues in ThemisDB.

## Quick Reference

| Error Type | Typical Message | Quick Fix |
|------------|----------------|-----------|
| Missing Export | `undefined reference` / `LNK2019` | Add `THEMIS_*_API` macro |
| Template Instantiation | `undefined reference to template` | Add explicit instantiation |
| Link Order | `undefined reference` (linker) | Reorder dependencies |
| Platform Guard | Compilation fails on specific platform | Add `#ifdef` guards |
| Intrinsic | `__builtin_* not found` | Add fallback implementation |
| Alignment | `SIGBUS` / misaligned access | Use `alignas()` or `memcpy` |
| Endianness | Data corruption | Use byte-order conversion |
| Symbol Visibility | `hidden symbol referenced by DSO` | Change visibility to default |

## Systematic Debugging Process

### Step 1: Identify the Error Category

Run the diagnostic scanner to categorize the error:

```bash
python tools/compiler_diagnostics/diagnostic_scanner.py build.log --output errors.db
```

The scanner will categorize errors into:
- **SYMBOL_VISIBILITY**: Export/import issues
- **LINKER**: Undefined references, missing libraries
- **TEMPLATE**: Template instantiation problems
- **INTRINSICS**: Compiler-specific intrinsics
- **ABI**: Calling convention, mangling issues
- **PLATFORM_SPECIFIC**: OS-specific code without guards
- **STANDARD_LIBRARY**: STL compatibility
- **WARNING**: Compiler warnings

### Step 2: Locate the Problem

Use the source audit tool to scan your code:

```bash
python tools/compiler_diagnostics/source_audit.py --root . --output audit_report.md
```

This identifies:
- Missing export macros
- Unguarded platform-specific code
- Intrinsics without fallbacks
- Template issues

### Step 3: Apply Platform-Specific Solutions

Refer to platform-specific guides:
- [Windows (MSVC)](build-guide/BUILD_WINDOWS_ERRORS.md)
- [Linux (GCC/Clang)](build-guide/BUILD_LINUX_ERRORS.md)
- [ARM Cross-Compilation](build-guide/BUILD_ARM_ERRORS.md)

### Step 4: Verify the Fix

After applying fixes:

1. **Rebuild**: `cmake --build build`
2. **Check symbols**: `python tools/compiler_diagnostics/symbol_checker.py build/libthemis_base.so`
3. **Run tests**: `cd build && ctest`
4. **Update matrix**: Update [PLATFORM_COMPATIBILITY_MATRIX.md](PLATFORM_COMPATIBILITY_MATRIX.md)

## Common Error Patterns & Solutions

### Pattern 1: Undefined Reference to Class Member

**Error**:
```
undefined reference to `ClassName::publicMethod()'
LNK2019: unresolved external symbol "public: void __cdecl ClassName::publicMethod(void)"
```

**Root Causes**:
1. Missing export macro on class
2. Method declared but not defined
3. Template not instantiated

**Debug Steps**:
```bash
# 1. Check if symbol is exported
nm -D libthemis_base.so | grep ClassName
dumpbin /EXPORTS themis_base.dll | findstr ClassName

# 2. Check if method is defined
grep -r "ClassName::publicMethod" src/
```

**Solutions**:

```cpp
// Solution 1: Add export macro
class THEMIS_BASE_API ClassName {
public:
    void publicMethod();
};

// Solution 2: Ensure definition exists
void ClassName::publicMethod() {
    // Implementation
}

// Solution 3: For templates
template class ClassName<int>;  // In .cpp file
```

### Pattern 2: Multiple Definition

**Error**:
```
multiple definition of `functionName()'
LNK1169: one or more multiply defined symbols found
```

**Root Cause**: Function defined in header without `inline`

**Solution**:
```cpp
// Before (causes error)
// In header.h
void helper() {
    // ...
}

// After (correct)
// Option 1: inline
inline void helper() {
    // ...
}

// Option 2: Move to .cpp
// In header.h
void helper();

// In source.cpp
void helper() {
    // ...
}

// Option 3: static (deprecated)
static void helper() {
    // File-scope only
}

// Option 4: anonymous namespace (modern C++)
namespace {
    void helper() {
        // Translation-unit scope
    }
}
```

### Pattern 3: Template Instantiation Error

**Error**:
```
undefined reference to `std::vector<MyClass, std::allocator<MyClass>>::push_back(MyClass const&)'
```

**Root Cause**: Template implementation in .cpp file or not instantiated

**Solutions**:

```cpp
// Solution 1: Move template to header
// mytemplate.h
template<typename T>
class MyTemplate {
public:
    void method() {
        // Implementation in header
    }
};

// Solution 2: Explicit instantiation
// In .cpp file
template class MyTemplate<int>;
template class MyTemplate<std::string>;

// Solution 3: Use separate .tpp/.inl file
// mytemplate.h
template<typename T>
class MyTemplate {
    void method();
};

#include "mytemplate.tpp"  // Include implementation

// mytemplate.tpp
template<typename T>
void MyTemplate<T>::method() {
    // Implementation
}
```

### Pattern 4: Platform-Specific Code Without Guards

**Error**: Compilation fails on one platform but works on another

**Example**:
```cpp
// Wrong - no guards
#include <windows.h>
void doWindowsStuff() {
    // Windows-only code
}
```

**Solution**:
```cpp
// Correct - with guards
#ifdef _WIN32
    #include <windows.h>
    
    void doWindowsStuff() {
        // Windows-only code
    }
#else
    void doWindowsStuff() {
        // Fallback or empty implementation
    }
#endif
```

**Better - Feature detection**:
```cpp
// Check for specific features
#if defined(_WIN32)
    #define THEMIS_HAS_WINDOWS_API 1
    #include <windows.h>
#elif defined(__unix__) || defined(__APPLE__)
    #define THEMIS_HAS_POSIX_API 1
    #include <unistd.h>
#endif

void doPlatformSpecificStuff() {
    #if THEMIS_HAS_WINDOWS_API
        // Windows implementation
    #elif THEMIS_HAS_POSIX_API
        // POSIX implementation
    #else
        #error "Unsupported platform"
    #endif
}
```

### Pattern 5: Intrinsics Without Fallbacks

**Error**:
```
error: '_mm_add_ps' was not declared
error: '__builtin_popcount' is not supported on this target
```

**Solution**:
```cpp
// Portable intrinsic wrapper
inline int popcount(uint32_t x) {
    #if defined(__GNUC__) || defined(__clang__)
        return __builtin_popcount(x);
    #elif defined(_MSC_VER)
        return __popcnt(x);
    #else
        // Software fallback
        int count = 0;
        while (x) {
            count += x & 1;
            x >>= 1;
        }
        return count;
    #endif
}

// SIMD with fallback
void vectorAdd(float* result, const float* a, const float* b, size_t n) {
    #if defined(__SSE__)
        // SSE implementation
        for (size_t i = 0; i < n; i += 4) {
            __m128 va = _mm_loadu_ps(&a[i]);
            __m128 vb = _mm_loadu_ps(&b[i]);
            _mm_storeu_ps(&result[i], _mm_add_ps(va, vb));
        }
    #elif defined(__ARM_NEON)
        // NEON implementation
        for (size_t i = 0; i < n; i += 4) {
            float32x4_t va = vld1q_f32(&a[i]);
            float32x4_t vb = vld1q_f32(&b[i]);
            vst1q_f32(&result[i], vaddq_f32(va, vb));
        }
    #else
        // Scalar fallback
        for (size_t i = 0; i < n; ++i) {
            result[i] = a[i] + b[i];
        }
    #endif
}
```

### Pattern 6: Linker Order Issues

**Error**:
```
undefined reference to `libA::function()'
```
But library is in link command!

**Root Cause**: Wrong link order - library listed before the object that needs it

**Solution**:
```cmake
# Wrong order
target_link_libraries(my_exe
    dependency_lib      # Listed first
    main_lib            # Uses dependency_lib
)

# Correct order (dependencies last)
target_link_libraries(my_exe
    main_lib            # Uses dependency_lib
    dependency_lib      # Dependency
)

# Or use PRIVATE/PUBLIC to manage transitively
target_link_libraries(main_lib PUBLIC dependency_lib)
target_link_libraries(my_exe PRIVATE main_lib)  # Gets dependency_lib automatically
```

### Pattern 7: Hidden Symbol Issues (Linux)

**Error**:
```
/usr/bin/ld: warning: hidden symbol 'foo' is referenced by DSO
```

**Root Cause**: Symbol marked as hidden but used across shared library boundaries

**Solution**:
```cpp
// Wrong - hidden symbol used publicly
class __attribute__((visibility("hidden"))) MyClass {
    // ...
};

// Correct - use default visibility for public APIs
class __attribute__((visibility("default"))) MyClass {
    // ...
};

// Or use export macro
class THEMIS_BASE_API MyClass {
    // ...
};
```

```cmake
# In CMakeLists.txt - set default visibility
set_target_properties(my_lib PROPERTIES
    CXX_VISIBILITY_PRESET default
    VISIBILITY_INLINES_HIDDEN OFF
)
```

## Debugging Tools

### 1. Compiler Diagnostics

```bash
# Verbose compiler output
cmake --build build --verbose

# Save all warnings
cmake --build build 2>&1 | tee build.log

# Parse errors
python tools/compiler_diagnostics/diagnostic_scanner.py build.log
```

### 2. Symbol Inspection

```bash
# Linux/macOS - list symbols
nm -D libthemis_base.so
nm -C libthemis_base.so | grep "ClassName"  # Demangled

# Check for undefined symbols
ldd -r libthemis_base.so

# Windows - list exports
dumpbin /EXPORTS themis_base.dll
```

### 3. Preprocessor Output

```bash
# See what the preprocessor actually generates
g++ -E myfile.cpp -o myfile.i
clang++ -E myfile.cpp -o myfile.i
cl /E myfile.cpp > myfile.i
```

### 4. Assembly Output

```bash
# Check generated code
g++ -S myfile.cpp -o myfile.s
```

## CI Integration

### Automated Error Tracking

```bash
# In CI pipeline
python tools/compiler_diagnostics/issue_tracker.py track \
    --ci-log $BUILD_LOG \
    --build-id $CI_BUILD_ID \
    --compiler gcc \
    --platform linux

# Generate weekly report
python tools/compiler_diagnostics/issue_tracker.py report \
    --output docs/WEEKLY_ERROR_REPORT.md
```

### Pre-Commit Checks

```bash
#!/bin/bash
# .git/hooks/pre-commit

# Run source audit
python tools/compiler_diagnostics/source_audit.py \
    --root . \
    --output /tmp/audit.md

# Check for high-priority issues
if grep -q "severity: high" /tmp/audit.md; then
    echo "ERROR: High-priority issues found. Please fix before committing."
    exit 1
fi
```

## Best Practices Checklist

When writing cross-platform code:

- [ ] Use export macros for all public APIs
- [ ] Guard platform-specific includes with `#ifdef`
- [ ] Provide fallbacks for compiler intrinsics
- [ ] Test on at least 2 different compilers
- [ ] Use `inline` for header-only functions
- [ ] Avoid `static` in headers (use anonymous namespaces in .cpp)
- [ ] Be explicit with symbol visibility
- [ ] Use `std::memcpy` for potentially unaligned data
- [ ] Handle endianness in serialization
- [ ] Document platform-specific requirements
- [ ] Update platform compatibility matrix

## Getting Help

1. **Check existing documentation**:
   - [Platform Compatibility Matrix](PLATFORM_COMPATIBILITY_MATRIX.md)
   - [Windows Errors](build-guide/BUILD_WINDOWS_ERRORS.md)
   - [Linux Errors](build-guide/BUILD_LINUX_ERRORS.md)
   - [ARM Errors](build-guide/BUILD_ARM_ERRORS.md)

2. **Run diagnostic tools**:
   ```bash
   python tools/compiler_diagnostics/diagnostic_scanner.py build.log
   python tools/compiler_diagnostics/source_audit.py
   ```

3. **Search error database**:
   ```bash
   sqlite3 tools/compiler_diagnostics/compiler_diagnostics.db
   SELECT * FROM errors WHERE message LIKE '%your error%';
   ```

4. **Ask for help**: Create an issue with:
   - Error message
   - Platform and compiler version
   - Minimal reproducible example
   - Output from diagnostic tools

## References

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
- [GCC Documentation](https://gcc.gnu.org/onlinedocs/)
- [Clang Documentation](https://clang.llvm.org/docs/)
- [MSVC Documentation](https://docs.microsoft.com/en-us/cpp/)
- [CMake Documentation](https://cmake.org/documentation/)
