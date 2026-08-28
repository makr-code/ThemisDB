# Vulkan Shaders CMakeLists.txt
# Compiles GLSL compute shaders to SPIR-V for Vulkan backend

cmake_minimum_required(VERSION 3.20)

# Include shader compilation module
include(${CMAKE_SOURCE_DIR}/cmake/VulkanShaders.cmake)

if(NOT VULKAN_SHADER_COMPILER_FOUND)
    message(STATUS "Vulkan shader compiler not found - shaders will not be compiled")
    message(STATUS "Pre-compiled SPIR-V shaders can be provided in the shaders directory")
    return()
endif()

# LoRA shader sources
set(LORA_SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lora)
set(LORA_SHADERS
    ${LORA_SHADER_DIR}/matmul.comp
    ${LORA_SHADER_DIR}/elementwise.comp
    ${LORA_SHADER_DIR}/gradient.comp
)

# Vector Index shader sources
set(VECTOR_INDEX_SHADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/l2_distance.comp
    ${CMAKE_CURRENT_SOURCE_DIR}/cosine_distance.comp
    ${CMAKE_CURRENT_SOURCE_DIR}/inner_product_distance.comp
    ${CMAKE_CURRENT_SOURCE_DIR}/batch_search.comp
    ${CMAKE_CURRENT_SOURCE_DIR}/topk_selection.comp
)

# Geospatial shader sources
set(GEO_SHADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/haversine_distance.comp
    ${CMAKE_CURRENT_SOURCE_DIR}/point_in_polygon.comp
)

# Output directory for compiled shaders
set(SHADER_OUTPUT_DIR ${CMAKE_BINARY_DIR}/shaders/lora)
set(VECTOR_SHADER_OUTPUT_DIR ${CMAKE_BINARY_DIR}/shaders/vector_index)
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})
file(MAKE_DIRECTORY ${VECTOR_SHADER_OUTPUT_DIR})

# Compile each shader
set(COMPILED_LORA_SHADERS "")
foreach(SHADER_FILE ${LORA_SHADERS})
    get_filename_component(SHADER_NAME ${SHADER_FILE} NAME_WE)
    set(OUTPUT_FILE ${SHADER_OUTPUT_DIR}/${SHADER_NAME}.comp.spv)
    
    # Set compiler flags based on compiler
    if(VULKAN_SHADER_COMPILER_NAME STREQUAL "glslc")
        set(COMPILER_FLAGS -o ${OUTPUT_FILE})
    else()
        set(COMPILER_FLAGS -V -o ${OUTPUT_FILE})
    endif()
    
    add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND ${VULKAN_SHADER_COMPILER} ${COMPILER_FLAGS} ${SHADER_FILE}
        DEPENDS ${SHADER_FILE}
        COMMENT "Compiling LoRA Vulkan shader: ${SHADER_NAME}.comp"
        VERBATIM
    )
    
    list(APPEND COMPILED_LORA_SHADERS ${OUTPUT_FILE})
endforeach()

# Create target for all LoRA shaders
if(THEMIS_EAGER_SHADER_BUILD)
    add_custom_target(vulkan_lora_shaders ALL
        DEPENDS ${COMPILED_LORA_SHADERS}
        COMMENT "Building Vulkan LoRA shaders"
    )
else()
    add_custom_target(vulkan_lora_shaders
        DEPENDS ${COMPILED_LORA_SHADERS}
        COMMENT "Building Vulkan LoRA shaders (on-demand)"
    )
endif()

# Install compiled shaders
install(FILES ${COMPILED_LORA_SHADERS}
    DESTINATION share/themis/shaders/lora
    COMPONENT shaders
)

# Also install source shaders for reference
install(FILES ${LORA_SHADERS}
    DESTINATION share/themis/shaders/lora/source
    COMPONENT shaders
)

message(STATUS "Vulkan LoRA shaders will be compiled to: ${SHADER_OUTPUT_DIR}")

# Compile Vector Index shaders
set(COMPILED_VECTOR_SHADERS "")
foreach(SHADER_FILE ${VECTOR_INDEX_SHADERS})
    get_filename_component(SHADER_NAME ${SHADER_FILE} NAME_WE)
    set(OUTPUT_FILE ${VECTOR_SHADER_OUTPUT_DIR}/${SHADER_NAME}.comp.spv)
    
    # Set compiler flags based on compiler
    if(VULKAN_SHADER_COMPILER_NAME STREQUAL "glslc")
        set(COMPILER_FLAGS -o ${OUTPUT_FILE})
    else()
        set(COMPILER_FLAGS -V -o ${OUTPUT_FILE})
    endif()
    
    add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND ${VULKAN_SHADER_COMPILER} ${COMPILER_FLAGS} ${SHADER_FILE}
        DEPENDS ${SHADER_FILE}
        COMMENT "Compiling Vector Index Vulkan shader: ${SHADER_NAME}.comp"
        VERBATIM
    )
    
    list(APPEND COMPILED_VECTOR_SHADERS ${OUTPUT_FILE})
endforeach()

# Create target for Vector Index shaders
if(THEMIS_EAGER_SHADER_BUILD)
    add_custom_target(vulkan_vector_index_shaders ALL
        DEPENDS ${COMPILED_VECTOR_SHADERS}
        COMMENT "Building Vulkan Vector Index shaders"
    )
else()
    add_custom_target(vulkan_vector_index_shaders
        DEPENDS ${COMPILED_VECTOR_SHADERS}
        COMMENT "Building Vulkan Vector Index shaders (on-demand)"
    )
endif()

# Install compiled Vector Index shaders
install(FILES ${COMPILED_VECTOR_SHADERS}
    DESTINATION share/themis/shaders/vector_index
    COMPONENT shaders
)

# Install Vector Index shader sources
install(FILES ${VECTOR_INDEX_SHADERS}
    DESTINATION share/themis/shaders/vector_index/source
    COMPONENT shaders
)

message(STATUS "Vulkan Vector Index shaders will be compiled to: ${VECTOR_SHADER_OUTPUT_DIR}")

# Compile Geospatial shaders
set(GEO_SHADER_OUTPUT_DIR ${CMAKE_BINARY_DIR}/shaders/geo)
file(MAKE_DIRECTORY ${GEO_SHADER_OUTPUT_DIR})

set(COMPILED_GEO_SHADERS "")
foreach(SHADER_FILE ${GEO_SHADERS})
    get_filename_component(SHADER_NAME ${SHADER_FILE} NAME_WE)
    set(OUTPUT_FILE ${GEO_SHADER_OUTPUT_DIR}/${SHADER_NAME}.comp.spv)

    # Set compiler flags based on compiler
    if(VULKAN_SHADER_COMPILER_NAME STREQUAL "glslc")
        set(COMPILER_FLAGS -o ${OUTPUT_FILE})
    else()
        set(COMPILER_FLAGS -V -o ${OUTPUT_FILE})
    endif()

    add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND ${VULKAN_SHADER_COMPILER} ${COMPILER_FLAGS} ${SHADER_FILE}
        DEPENDS ${SHADER_FILE}
        COMMENT "Compiling Geospatial Vulkan shader: ${SHADER_NAME}.comp"
        VERBATIM
    )

    list(APPEND COMPILED_GEO_SHADERS ${OUTPUT_FILE})
endforeach()

# Create target for Geospatial shaders
if(THEMIS_EAGER_SHADER_BUILD)
    add_custom_target(vulkan_geo_shaders ALL
        DEPENDS ${COMPILED_GEO_SHADERS}
        COMMENT "Building Vulkan Geospatial shaders"
    )
else()
    add_custom_target(vulkan_geo_shaders
        DEPENDS ${COMPILED_GEO_SHADERS}
        COMMENT "Building Vulkan Geospatial shaders (on-demand)"
    )
endif()

# Install compiled Geospatial shaders
install(FILES ${COMPILED_GEO_SHADERS}
    DESTINATION share/themis/shaders/geo
    COMPONENT shaders
)

# Install Geospatial shader sources
install(FILES ${GEO_SHADERS}
    DESTINATION share/themis/shaders/geo/source
    COMPONENT shaders
)

message(STATUS "Vulkan Geospatial shaders will be compiled to: ${GEO_SHADER_OUTPUT_DIR}")
