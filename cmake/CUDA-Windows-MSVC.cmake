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

# Hardcode CUDA Toolkit Pfad (besser als Registry-Suche)
set(_CUDA_TOOLKIT_ROOT "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v13.1")

# Windows CUDA Library Structure
set(_CUDA_LIB_DIR "${_CUDA_TOOLKIT_ROOT}/lib/x64")

# Set CUDA Toolkit locations BEFORE project() is called
set(CUDAToolkit_ROOT "${_CUDA_TOOLKIT_ROOT}")
set(CUDAToolkit_INCLUDE_DIR "${_CUDA_TOOLKIT_ROOT}/include")
set(CUDAToolkit_LIBRARY_DIR "${_CUDA_LIB_DIR}")
set(CUDAToolkit_BIN_DIR "${_CUDA_TOOLKIT_ROOT}/bin")

# For FindCUDAToolkit.cmake module
set(CUDAToolkit_FOUND ON CACHE INTERNAL "CUDA Toolkit Found")
set(CUDAToolkit_VERSION "13.1" CACHE STRING "CUDA Toolkit Version")
set(CUDAToolkit_VERSION_MAJOR 13 CACHE INTERNAL "CUDA Major Version")
set(CUDAToolkit_VERSION_MINOR 1 CACHE INTERNAL "CUDA Minor Version")

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
    message(STATUS "[CUDA Toolchain] Found CUDA 13.1 at ${_CUDA_TOOLKIT_ROOT}")
endif()
