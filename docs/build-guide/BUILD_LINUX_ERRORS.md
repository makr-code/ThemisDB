# Linux Build Errors & Solutions

This document catalogs common compiler and linker errors encountered when building ThemisDB on Linux with GCC or Clang.

## Table of Contents

1. [Linker Errors](#linker-errors)
2. [Symbol Visibility](#symbol-visibility)
3. [Compiler Warnings](#compiler-warnings)
4. [Dependencies](#dependencies)
5. [CMake Configuration](#cmake-configuration)

## Linker Errors

### Error: Undefined Reference

**Symptom**:
```
/usr/bin/ld: libthemis_base.a(file.cpp.o): undefined reference to `symbol_name`
collect2: error: ld returned 1 exit status
```

**Causes**:
1. Missing library in link command
2. Wrong link order
3. Symbol stripped by `--as-needed`
4. Missing explicit template instantiation

**Solutions**:

#### 1. Add Missing Library
```cmake
target_link_libraries(my_target PRIVATE 
    themis_base
    pthread
    dl
    m
)
```

#### 2. Fix Link Order
```cmake
# Libraries should be listed in dependency order
# Dependencies first, then dependents
target_link_libraries(my_target PRIVATE
    themis_storage    # Depends on themis_base
    themis_base       # Base library
    ${ROCKSDB_LIBRARIES}
)
```

#### 3. Disable --as-needed
```cmake
if(UNIX AND NOT APPLE)
    target_link_options(my_target PRIVATE 
        "LINKER:--no-as-needed"
    )
endif()
```

#### 4. Explicit Template Instantiation
```cpp
// In .cpp file
template class std::vector<MyType>;
template void myFunction<int>(int);
```

### Error: DSO Missing from Command Line

**Symptom**:
```
/usr/bin/ld: warning: libfoo.so, needed by libbar.so, not found
/usr/bin/ld: libbar.so: undefined reference to `symbol@libfoo.so.1'
```

**Cause**: Missing transitive dependency

**Solution**:
```cmake
# Option 1: Make dependency public
target_link_libraries(libbar PUBLIC libfoo)

# Option 2: Link directly
target_link_libraries(my_target PRIVATE
    libbar
    libfoo  # Add transitive dependency explicitly
)
```

### Error: Cannot Find -llib

**Symptom**:
```
/usr/bin/ld: cannot find -lrocksdb
collect2: error: ld returned 1 exit status
```

**Solutions**:

#### 1. Install Missing Library
```bash
# Ubuntu/Debian
sudo apt-get install librocksdb-dev

# Fedora/RHEL
sudo dnf install rocksdb-devel
```

#### 2. Specify Library Path
```cmake
link_directories(/path/to/libs)
# Or better:
find_library(ROCKSDB_LIB rocksdb PATHS /path/to/libs)
target_link_libraries(my_target PRIVATE ${ROCKSDB_LIB})
```

### Error: Multiply Defined Symbols

**Symptom**:
```
/usr/bin/ld: file1.cpp.o: in function `foo()':
file1.cpp:(.text+0x0): multiple definition of `foo()'; file2.cpp.o:file2.cpp:(.text+0x0): first defined here
```

**Solutions**:

#### 1. Use `inline` Keyword
```cpp
// In header file
inline void foo() {
    // Implementation
}
```

#### 2. Use Anonymous Namespace
```cpp
// In .cpp file
namespace {
    void internalHelper() {
        // Only visible in this file
    }
}
```

#### 3. Use `static`
```cpp
// In .cpp file
static void fileLocalFunction() {
    // Only visible in this file
}
```

## Symbol Visibility

### Warning: Hidden Symbol Cannot Be Undefined

**Symptom**:
```
/usr/bin/ld: warning: hidden symbol 'ClassName::method()' in file.o is referenced by DSO
```

**Cause**: Symbol marked as hidden but used across shared library boundaries

**Solution**:
```cpp
// Add visibility attribute
class __attribute__((visibility("default"))) MyClass {
    void publicMethod();
};

// Or use export macro
class THEMIS_BASE_API MyClass {
    void publicMethod();
};
```

### Issue: Weak Symbol Conflicts

**Symptom**:
```
warning: symbol 'foo' has different size in different object files
```

**Solutions**:

#### 1. Use Inline Linkage
```cpp
// Instead of weak symbols, use inline
inline void foo() {
    // ...
}
```

#### 2. Use Proper Visibility
```cmake
if(NOT WIN32)
    set_target_properties(my_target PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
    )
endif()
```

## Compiler Warnings

### Warning: Unused Parameter

**Symptom**:
```
warning: unused parameter 'param' [-Wunused-parameter]
```

**Solutions**:

#### 1. Use `[[maybe_unused]]`
```cpp
void function([[maybe_unused]] int param) {
    // param might be used in some configurations
}
```

#### 2. Use void Cast
```cpp
void function(int param) {
    (void)param;
    // ...
}
```

#### 3. Remove Parameter Name
```cpp
void function(int /*param*/) {
    // ...
}
```

### Warning: Comparison Between Signed and Unsigned

**Symptom**:
```
warning: comparison of integer expressions of different signedness [-Wsign-compare]
```

**Solution**:
```cpp
// Before
for (int i = 0; i < vec.size(); ++i) { }

// After - use correct type
for (size_t i = 0; i < vec.size(); ++i) { }

// Or use range-based for
for (const auto& item : vec) { }

// Or explicit cast if needed
for (int i = 0; i < static_cast<int>(vec.size()); ++i) { }
```

### Warning: Strict Aliasing

**Symptom**:
```
warning: dereferencing type-punned pointer will break strict-aliasing rules [-Wstrict-aliasing]
```

**Solution**:
```cpp
// Before (undefined behavior)
int* p = reinterpret_cast<int*>(&float_value);

// After - use memcpy
int result;
std::memcpy(&result, &float_value, sizeof(result));

// Or use union (C++20)
union {
    float f;
    int i;
} converter;
converter.f = float_value;
int result = converter.i;
```

## Dependencies

### Missing Header Files

**Common Missing Development Packages**:

```bash
# Ubuntu/Debian
sudo apt-get install \
    build-essential \
    cmake \
    libssl-dev \
    librocksdb-dev \
    libgrpc++-dev \
    protobuf-compiler \
    libboost-all-dev

# Fedora/RHEL
sudo dnf install \
    gcc-c++ \
    cmake \
    openssl-devel \
    rocksdb-devel \
    grpc-devel \
    protobuf-compiler \
    boost-devel
```

### PKG-CONFIG Issues

**Symptom**:
```
CMake Error: Could not find a package configuration file provided by "RocksDB"
```

**Solution**:
```cmake
# Add pkg-config support
find_package(PkgConfig REQUIRED)
pkg_check_modules(ROCKSDB REQUIRED rocksdb)

target_include_directories(my_target PRIVATE ${ROCKSDB_INCLUDE_DIRS})
target_link_libraries(my_target PRIVATE ${ROCKSDB_LIBRARIES})
```

## CMake Configuration

### Recommended GCC/Clang Flags

```cmake
if(UNIX AND NOT APPLE)
    # Warning flags
    add_compile_options(
        -Wall
        -Wextra
        -Wpedantic
        -Wno-unused-parameter
    )
    
    # Position-independent code for shared libraries
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    
    # Symbol visibility
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
    
    # Link-time optimization (Release only)
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        add_compile_options(-flto)
        add_link_options(-flto)
    endif()
    
    # Linker options
    add_link_options(
        "LINKER:--no-as-needed"
        "LINKER:--no-undefined"  # Catch missing symbols early
    )
endif()
```

### GCC-Specific Options

```cmake
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    add_compile_options(
        -Wno-maybe-uninitialized  # Too many false positives
    )
    
    # Enable colored diagnostics
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 4.9)
        add_compile_options(-fdiagnostics-color=always)
    endif()
endif()
```

### Clang-Specific Options

```cmake
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(
        -Wno-unused-command-line-argument
    )
    
    # Enable colored diagnostics
    add_compile_options(-fcolor-diagnostics)
    
    # Use libc++ on Linux (optional)
    # add_compile_options(-stdlib=libc++)
    # add_link_options(-stdlib=libc++ -lc++abi)
endif()
```

## Position-Independent Code (PIC)

### Error: Relocation R_X86_64_32

**Symptom**:
```
relocation R_X86_64_32 against `.rodata' can not be used when making a shared object; recompile with -fPIC
```

**Solution**:
```cmake
# Enable globally
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Or per target
set_target_properties(my_target PROPERTIES
    POSITION_INDEPENDENT_CODE ON
)
```

## Sanitizers

### Building with Address Sanitizer (ASan)

```cmake
option(ENABLE_ASAN "Enable AddressSanitizer" OFF)

if(ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()
```

```bash
# Build and run
cmake -B build -DENABLE_ASAN=ON
cmake --build build
./build/my_binary
```

### Building with Undefined Behavior Sanitizer (UBSan)

```cmake
option(ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)

if(ENABLE_UBSAN)
    add_compile_options(-fsanitize=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=undefined)
endif()
```

## Testing

### Build and Test

```bash
# Configure
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build with verbose output
cmake --build build --verbose

# Check for warnings
cmake --build build 2>&1 | grep "warning:"

# Run tests
cd build && ctest --output-on-failure
```

### Check Symbol Visibility

```bash
# List exported symbols
nm -D build/libthemis_base.so | grep " T "

# Check for unresolved symbols
ldd -r build/libthemis_base.so
```

## References

- [GCC Warning Options](https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html)
- [Clang Diagnostics](https://clang.llvm.org/docs/DiagnosticsReference.html)
- [CMake Build Configuration](https://cmake.org/cmake/help/latest/)
- [Export Macro Best Practices](../CONTRIBUTING_PLATFORM_GUIDELINES.md)

## Getting Help

If you encounter an error not covered here:

1. Run the diagnostic scanner:
   ```bash
   python tools/compiler_diagnostics/diagnostic_scanner.py build.log
   ```

2. Check the [Platform Compatibility Matrix](../PLATFORM_COMPATIBILITY_MATRIX.md)

3. See [COMPILER_TROUBLESHOOTING.md](COMPILER_TROUBLESHOOTING.md)
