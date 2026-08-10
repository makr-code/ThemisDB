cmake_minimum_required(VERSION 3.20)
project(themis_accel_cuda_plugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ─────────────────────────────────────────────────────────────────────────────
# Guard: only build when CUDA is enabled
# ─────────────────────────────────────────────────────────────────────────────
if(NOT THEMIS_ENABLE_CUDA)
    message(STATUS "themis_accel_cuda_plugin: THEMIS_ENABLE_CUDA=OFF — skipping")
    return()
endif()

find_package(CUDAToolkit QUIET)
if(NOT CUDAToolkit_FOUND)
    message(STATUS "themis_accel_cuda_plugin: CUDAToolkit not found — skipping plugin build")
    return()
endif()

# ─────────────────────────────────────────────────────────────────────────────
# Shared library target
# ─────────────────────────────────────────────────────────────────────────────
add_library(themis_accel_cuda_plugin SHARED
    cuda_plugin.cpp
)

# Root directory resolution (may be provided by the parent CMake context;
# fall back to a relative path for standalone builds).
if(NOT THEMIS_ROOT_DIR)
    get_filename_component(THEMIS_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
endif()

target_include_directories(themis_accel_cuda_plugin PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"   # cuda_plugin_config.h lives here
    "${THEMIS_ROOT_DIR}/include"
    "${THEMIS_ROOT_DIR}/src"
    "${CUDAToolkit_INCLUDE_DIRS}"
)

target_compile_definitions(themis_accel_cuda_plugin PRIVATE
    THEMIS_ENABLE_CUDA=1
)

# themis_acceleration provides all compiled CUDA backend object code
# (CUDAVectorBackend, CUDAGraphBackend, CUDAGeoBackend, MultiGPUVectorBackend,
#  FaissGPUVectorBackend, fp16_vector_kernels.o).
target_link_libraries(themis_accel_cuda_plugin PRIVATE
    themis_acceleration
    CUDA::cudart
)

# ─────────────────────────────────────────────────────────────────────────────
# Plugin naming convention
#   Output: themis_accel_cuda.plugin.so  (no 'lib' prefix)
# ─────────────────────────────────────────────────────────────────────────────
set_target_properties(themis_accel_cuda_plugin PROPERTIES
    OUTPUT_NAME              "themis_accel_cuda"
    PREFIX                   ""
    SUFFIX                   ".plugin${CMAKE_SHARED_LIBRARY_SUFFIX}"
    POSITION_INDEPENDENT_CODE ON
    CXX_VISIBILITY_PRESET    hidden
    VISIBILITY_INLINES_HIDDEN ON
)

# ─────────────────────────────────────────────────────────────────────────────
# Install rules
# ─────────────────────────────────────────────────────────────────────────────
install(TARGETS themis_accel_cuda_plugin
    LIBRARY DESTINATION lib/themis/plugins
    RUNTIME DESTINATION lib/themis/plugins
)
install(FILES cuda_plugin.json DESTINATION lib/themis/plugins)
