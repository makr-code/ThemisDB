# CMake Modules Reference

This document provides detailed references for each CMake module file used in ThemisDB project.

## Versions.cmake
### Purpose
Handles version management for the project.
### Key Variables
- `PROJECT_VERSION`: The current version of the project.
- `MINIMUM_REQUIRED_VERSION`: The minimum required version of CMake.
### Usage Example
```cmake
include(Versions)
set(PROJECT_VERSION "1.0.0")
```  

## CompilerOptions.cmake
### Purpose
Contains compiler flags specific to C++20 standards and compiler types.
### Key Variables
- `CXX_STANDARD`: The C++ standard to be used.
- `MSVC_FLAGS`: Flags specific to MSVC compiler.
- `GCC_FLAGS`: Flags specific to GCC compiler.
- `CLANG_FLAGS`: Flags specific to Clang compiler.
### Usage Example
```cmake
include(CompilerOptions)
set(CXX_STANDARD 20)
```  

## PlatformDetection.cmake
### Purpose
Detects the platform and sets relevant variables accordingly.
### Key Variables
- `CMAKE_SYSTEM_NAME`: The name of the system.
- `CMAKE_SYSTEM_PROCESSOR`: The processor architecture.
### Usage Example
```cmake
include(PlatformDetection)
message(STATUS "Detected OS: ${CMAKE_SYSTEM_NAME}")
```  

## ArchitectureOptimizations.cmake
### Purpose
Enables architecture-specific optimizations.
### Key Variables
- `USE_AVX2`: Flag to enable AVX2 optimizations.
- `USE_NEON`: Flag to enable NEON optimizations.
### Usage Example
```cmake
include(ArchitectureOptimizations)
set(USE_AVX2 ON)
```  

## EditionDefaults.cmake
### Purpose
Sets default configurations for different editions of the project.
### Key Variables
- `EDITION`: Current edition being built.
### Usage Example
```cmake
include(EditionDefaults)
set(EDITION "Standard")
```  

## FeatureDefaults.cmake
### Purpose
Defines default values for various feature flags in the project.
### Key Variables
- `ENABLE_FEATURE_X`: Flag to enable/disable feature X.
### Usage Example
```cmake
include(FeatureDefaults)
set(ENABLE_FEATURE_X ON)
```  

## EditionMatrix.cmake
### Purpose
Defines a compatibility matrix for different editions.
### Key Variables
- `COMPATIBILITY_MATRIX`: The compatibility information.
### Usage Example
```cmake
include(EditionMatrix)
message(STATUS "Compatibility matrix loaded.")
```  

## VcpkgConfiguration.cmake
### Purpose
Configures Vcpkg binary cache settings.
### Key Variables
- `VCPKG_BINARY_CACHE`: Path to the binary cache.
### Usage Example
```cmake
include(VcpkgConfiguration)
set(VCPKG_BINARY_CACHE "/path/to/cache")
```  

## Dependencies.cmake
### Purpose
Handles all find_package calls for project dependencies.
### Key Variables
- `FIND_PACKAGE_OPTIONS`: Options for find_package.
### Usage Example
```cmake
include(Dependencies)
find_package(SOME_LIBRARY REQUIRED)
```  

## Validation Modules
### PlatformValidation.cmake
#### Purpose
Validates platform-specific configurations.
\
### EditionValidation.cmake
#### Purpose
Validates edition-specific configurations.
### FeatureValidation.cmake
#### Purpose
Validates feature-specific configurations.
### Key Variables
- `VALIDATION_FLAGS`: Flags for validation checks.
### Usage Example
```cmake
include(PlatformValidation)
include(EditionValidation)
include(FeatureValidation)
```  

## Conclusion
This document should help developers understand the purpose, variables, and usage of each CMake module in ThemisDB. For detailed implementations, refer to the corresponding module files.
