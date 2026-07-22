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
        cuda/infini_attention_cuda.cu
    )
    
    set_source_files_properties(
        cuda/flash_attention_cuda.cu
        cuda/infini_attention_cuda.cu
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

# Vulkan backend
if(THEMIS_ENABLE_VULKAN)
    message(STATUS "Flash Attention: Vulkan backend enabled")
    
    find_package(Vulkan REQUIRED)
    
    # Compile GLSL compute shaders to SPIR-V
    set(VULKAN_SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vulkan/shaders")
    set(VULKAN_SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/vulkan/spirv")
    
    file(MAKE_DIRECTORY "${VULKAN_SHADER_OUTPUT_DIR}")
    
    # Find glslc compiler
    find_program(GLSLC_COMPILER glslc HINTS
        "$ENV{VULKAN_SDK}/bin"
        "/usr/bin"
        "/usr/local/bin"
    )
    
    if(GLSLC_COMPILER)
        message(STATUS "Found glslc at ${GLSLC_COMPILER}")
        
        # Compile infini_attention shader
        set(VULKAN_SHADER_SRC "${VULKAN_SHADER_DIR}/infini_attention.glsl")
        set(VULKAN_SHADER_SPV "${VULKAN_SHADER_OUTPUT_DIR}/infini_attention.spv")
        
        if(EXISTS "${VULKAN_SHADER_SRC}")
            add_custom_command(
                OUTPUT "${VULKAN_SHADER_SPV}"
                COMMAND "${GLSLC_COMPILER}" -fshader-stage=compute -O "${VULKAN_SHADER_SRC}" -o "${VULKAN_SHADER_SPV}"
                DEPENDS "${VULKAN_SHADER_SRC}"
                COMMENT "Compiling Vulkan compute shader: ${VULKAN_SHADER_SRC}"
            )
            
            list(APPEND VULKAN_SHADER_OUTPUTS "${VULKAN_SHADER_SPV}")
        endif()
    else()
        message(WARNING "glslc not found; Vulkan shaders will not be compiled. Install Vulkan SDK to enable.")
    endif()
    
    list(APPEND FLASH_ATTENTION_SOURCES
        vulkan/infini_attention_vulkan.cpp
        ${VULKAN_SHADER_OUTPUTS}
    )
    
    add_compile_definitions(THEMIS_ENABLE_VULKAN)
endif()

# HIP backend (TODO)
if(THEMIS_ENABLE_HIP OR HIP_FOUND)
    message(STATUS "Flash Attention: HIP backend enabled")
    
    enable_language(HIP)
    
    list(APPEND FLASH_ATTENTION_SOURCES
        hip/flash_attention_hip.hip
        hip/infini_attention_hip.hip
        hip/infini_attention_hip.cpp
    )
    
    set_source_files_properties(
        hip/flash_attention_hip.hip
        hip/infini_attention_hip.hip
        PROPERTIES
            LANGUAGE HIP
    )
    
    # Set HIP architecture
    if(NOT DEFINED HIP_ARCHITECTURES)
        set(HIP_ARCHITECTURES "gfx906;gfx908;gfx90a;gfx940;gfx941;gfx942")  # CDNA1, CDNA2, CDNA3
    endif()
    
    add_compile_definitions(THEMIS_ENABLE_HIP)
    
    # HIP-specific linker and compiler settings
    set(HIP_HIPCC_FLAGS "${HIP_HIPCC_FLAGS} -ffast-math")
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

# HIP-specific settings
if(THEMIS_ENABLE_HIP OR HIP_FOUND)
    target_link_libraries(themis_flash_attention
        PUBLIC
            hip::host
            rocm::rocsolver
    )
    
    target_compile_options(themis_flash_attention
        PRIVATE
            $<$<COMPILE_LANGUAGE:HIP>:
                -ffast-math
                -O3
            >
    )
endif()

# Vulkan-specific settings
if(THEMIS_ENABLE_VULKAN)
    target_link_libraries(themis_flash_attention
        PUBLIC
            Vulkan::Vulkan
    )
    
    target_include_directories(themis_flash_attention
        PRIVATE
            ${VULKAN_SHADER_OUTPUT_DIR}
    )
    
    # Add shader outputs as dependencies
    if(VULKAN_SHADER_OUTPUTS)
        add_custom_target(vulkan_shaders ALL DEPENDS ${VULKAN_SHADER_OUTPUTS})
        add_dependencies(themis_flash_attention vulkan_shaders)
    endif()
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
