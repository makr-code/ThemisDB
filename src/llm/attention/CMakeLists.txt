# Flash Attention v3 Integration for ThemisDB LLM
# Supports CUDA, Vulkan, HIP backends

cmake_minimum_required(VERSION 3.20)

# Flash Attention core sources
set(FLASH_ATTENTION_SOURCES
    flash_attention.cpp
    kv_cache_manager.cpp
)

# CUDA backend
if(THEMIS_ENABLE_CUDA OR CUDA_FOUND)
    message(STATUS "Flash Attention: CUDA backend enabled")
    
    enable_language(CUDA)
    
    list(APPEND FLASH_ATTENTION_SOURCES
        cuda/flash_attention_cuda.cu
    )
    
    set_source_files_properties(
        cuda/flash_attention_cuda.cu
        PROPERTIES
            LANGUAGE CUDA
            CUDA_SEPARABLE_COMPILATION ON
    )
    
    # Set CUDA architecture
    if(NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
        set(CMAKE_CUDA_ARCHITECTURES "80;86;90")  # A100, RTX 4090, H100
    endif()
    
    add_compile_definitions(THEMIS_ENABLE_CUDA)
endif()

# Vulkan backend (TODO)
if(THEMIS_ENABLE_VULKAN)
    message(STATUS "Flash Attention: Vulkan backend enabled")
    # list(APPEND FLASH_ATTENTION_SOURCES
    #     vulkan/flash_attention_vulkan.cpp
    # )
endif()

# HIP backend (TODO)
if(THEMIS_ENABLE_HIP OR HIP_FOUND)
    message(STATUS "Flash Attention: HIP backend enabled")
    # list(APPEND FLASH_ATTENTION_SOURCES
    #     hip/flash_attention_hip.cpp
    # )
endif()

# Create Flash Attention library
add_library(themis_flash_attention STATIC ${FLASH_ATTENTION_SOURCES})

target_include_directories(themis_flash_attention
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(themis_flash_attention
    PUBLIC
        # No external dependencies for now
)

# CUDA-specific settings
if(THEMIS_ENABLE_CUDA OR CUDA_FOUND)
    target_link_libraries(themis_flash_attention
        PUBLIC
            CUDA::cudart
            CUDA::cuda_driver
    )
    
    # Set CUDA flags
    target_compile_options(themis_flash_attention
        PRIVATE
            $<$<COMPILE_LANGUAGE:CUDA>:
                --expt-relaxed-constexpr
                --use_fast_math
                -Xptxas=-v
            >
    )
endif()

# C++ standard
target_compile_features(themis_flash_attention PUBLIC cxx_std_17)

# Install targets
install(TARGETS themis_flash_attention
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)

# Install headers
install(DIRECTORY ${CMAKE_SOURCE_DIR}/include/llm/attention
    DESTINATION include/llm
    FILES_MATCHING PATTERN "*.h"
)

message(STATUS "Flash Attention v3 module configured")
