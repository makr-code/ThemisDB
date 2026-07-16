# CUDA Toolchain File für Windows MSVC + CUDA 13.1
# Löst das Registry-Lookup Problem auf VS2022
#
# Verwendung:
#   cmake -S . -B build-msvc \
#     -G "Visual Studio 17 2022" \
#     -DCMAKE_TOOLCHAIN_FILE=cmake/CUDA-Windows-MSVC.cmake \
#     -DTHEMIS_ENABLE_CUDA=ON

# CRITICAL: Setze CUDA Toolkit Pfad VOR any project() oder enable_language() call
if(NOT CMAKE_SYSTEM_NAME)
    set(CMAKE_SYSTEM_NAME Windows)
endif()

# Resolve CUDA Toolkit root from cache/environment first, then common install roots.
set(_CUDA_TOOLKIT_ROOT "${CUDAToolkit_ROOT}")
if(NOT _CUDA_TOOLKIT_ROOT AND DEFINED ENV{CUDA_PATH})
    set(_CUDA_TOOLKIT_ROOT "$ENV{CUDA_PATH}")
endif()
if(NOT _CUDA_TOOLKIT_ROOT AND DEFINED ENV{CUDA_HOME})
    set(_CUDA_TOOLKIT_ROOT "$ENV{CUDA_HOME}")
endif()
if(NOT _CUDA_TOOLKIT_ROOT)
    set(_themis_cuda_toolkits)
    foreach(_themis_program_files_root IN ITEMS "$ENV{ProgramFiles}" "$ENV{ProgramFiles\(x86\)}" "$ENV{ProgramW6432}")
        if(_themis_program_files_root)
            file(GLOB _themis_cuda_toolkits_glob "${_themis_program_files_root}/NVIDIA GPU Computing Toolkit/CUDA/v*")
            list(APPEND _themis_cuda_toolkits ${_themis_cuda_toolkits_glob})
        endif()
    endforeach()
    list(REMOVE_DUPLICATES _themis_cuda_toolkits)
    list(SORT _themis_cuda_toolkits ORDER DESCENDING)
    foreach(_cuda_root IN LISTS _themis_cuda_toolkits)
        if(EXISTS "${_cuda_root}/include/cuda.h")
            set(_CUDA_TOOLKIT_ROOT "${_cuda_root}")
            break()
        endif()
    endforeach()
endif()

if(NOT _CUDA_TOOLKIT_ROOT)
    message(FATAL_ERROR "CUDA Toolkit root could not be resolved from CUDAToolkit_ROOT, CUDA_PATH, CUDA_HOME, or standard install roots")
endif()

# Windows CUDA Library Structure
set(_CUDA_LIB_DIR "${_CUDA_TOOLKIT_ROOT}/lib/x64")

# Set CUDA Toolkit locations BEFORE project() is called
set(CUDAToolkit_ROOT "${_CUDA_TOOLKIT_ROOT}")
set(CUDAToolkit_INCLUDE_DIR "${_CUDA_TOOLKIT_ROOT}/include")
set(CUDAToolkit_LIBRARY_DIR "${_CUDA_LIB_DIR}")
set(CUDAToolkit_BIN_DIR "${_CUDA_TOOLKIT_ROOT}/bin")

# For FindCUDAToolkit.cmake module
set(CUDAToolkit_FOUND ON CACHE INTERNAL "CUDA Toolkit Found")
if(EXISTS "${_CUDA_TOOLKIT_ROOT}/version.json")
    file(READ "${_CUDA_TOOLKIT_ROOT}/version.json" _themis_cuda_version_json)
    string(JSON _themis_cuda_version_string ERROR_VARIABLE _themis_cuda_version_error GET "${_themis_cuda_version_json}" cuda version)
endif()
if(NOT _themis_cuda_version_string AND EXISTS "${_CUDA_TOOLKIT_ROOT}/version.txt")
    file(READ "${_CUDA_TOOLKIT_ROOT}/version.txt" _themis_cuda_version_txt)
    string(REGEX MATCH "([0-9]+)\\.([0-9]+)" _themis_cuda_version_match "${_themis_cuda_version_txt}")
    set(_themis_cuda_version_string "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}")
endif()
if(NOT _themis_cuda_version_string)
    set(_themis_cuda_version_string "unknown")
endif()
string(REGEX MATCH "^([0-9]+)\\.([0-9]+)" _themis_cuda_version_match "${_themis_cuda_version_string}")
set(CUDAToolkit_VERSION "${_themis_cuda_version_string}" CACHE STRING "CUDA Toolkit Version")
if(CMAKE_MATCH_1)
    set(CUDAToolkit_VERSION_MAJOR ${CMAKE_MATCH_1} CACHE INTERNAL "CUDA Major Version")
    set(CUDAToolkit_VERSION_MINOR ${CMAKE_MATCH_2} CACHE INTERNAL "CUDA Minor Version")
endif()

# Set CUDA specific variables
set(CUDA_TOOLKIT_ROOT_DIR "${_CUDA_TOOLKIT_ROOT}" CACHE PATH "CUDA Toolkit Root")
set(CUDA_SDK_ROOT_DIR "${_CUDA_TOOLKIT_ROOT}" CACHE PATH "CUDA SDK Root")

# Libraries
set(CUDA_CUDART_LIBRARY "${_CUDA_LIB_DIR}/cudart.lib" CACHE FILEPATH "CUDA Runtime Library")
set(CUDA_CUDA_LIBRARY "${_CUDA_LIB_DIR}/cuda.lib" CACHE FILEPATH "CUDA Driver Library")
set(CUDA_CUDADEVRT_LIBRARY "${_CUDA_LIB_DIR}/cudadevrt.lib" CACHE FILEPATH "CUDA Device Runtime")

# Compiler
set(CMAKE_CUDA_COMPILER "${_CUDA_TOOLKIT_ROOT}/bin/nvcc.exe" CACHE FILEPATH "CUDA Compiler")
set(CMAKE_CUDA_HOST_COMPILER "cl.exe" CACHE STRING "CUDA Host Compiler")

# Include directories
set(CMAKE_CUDA_TOOLKIT_INCLUDE_DIRECTORIES 
    "${_CUDA_TOOLKIT_ROOT}/include" 
    CACHE PATH "CUDA Toolkit Include Directories" FORCE)

# NVCC Flags
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -Xcompiler /MD -gencode arch=compute_70,code=sm_70" CACHE STRING "CUDA Compiler Flags")

# Verify CUDA exists
if(NOT EXISTS "${CUDAToolkit_INCLUDE_DIR}/cuda.h")
    message(FATAL_ERROR "CUDA Toolkit not found at ${_CUDA_TOOLKIT_ROOT}")
else()
    message(STATUS "[CUDA Toolchain] Found CUDA at ${_CUDA_TOOLKIT_ROOT}")
endif()
