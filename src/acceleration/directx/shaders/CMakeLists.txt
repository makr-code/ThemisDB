# DirectX Shaders CMakeLists.txt
# Compiles HLSL compute shaders to DXIL bytecode (.cso) for the DirectX 12 backend.
#
# This file covers the ANN (vector index) and Geospatial shaders.
# LoRA shaders are compiled inline in cmake/CMakeLists.txt alongside the LoRA target.
#
# Requires: DXC_COMPILER — path to dxc (Windows SDK or DirectX Agility SDK).
# The parent CMakeLists.txt sets DXC_COMPILER and then calls add_subdirectory here.
#
# Output directories:
#   ${CMAKE_BINARY_DIR}/shaders/vector_index/*.cso  — ANN distance shaders
#   ${CMAKE_BINARY_DIR}/shaders/geo/*.cso           — Geospatial shaders
#
# Install destinations:
#   bin/shaders/vector_index/   — ANN shaders
#   bin/shaders/geo/            — Geospatial shaders

cmake_minimum_required(VERSION 3.20)

if(NOT DXC_COMPILER)
    message(STATUS "DirectX ANN/Geo shaders: DXC compiler not available — skipping compilation")
    message(STATUS "  Pre-compiled .cso files can be placed in the output shader directories.")
    return()
endif()

# ---------------------------------------------------------------------------
# ANN / Vector Index shaders
# ---------------------------------------------------------------------------

set(VECTOR_SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(VECTOR_SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders/vector_index")
file(MAKE_DIRECTORY "${VECTOR_SHADER_OUTPUT_DIR}")

set(VECTOR_HLSL_SHADERS
    l2_distance.hlsl
    cosine_distance.hlsl
    inner_product_distance.hlsl
    batch_search.hlsl
    topk_selection.hlsl
)

set(COMPILED_VECTOR_SHADERS "")
foreach(_shader IN LISTS VECTOR_HLSL_SHADERS)
    get_filename_component(_stem "${_shader}" NAME_WE)
    set(_src  "${VECTOR_SHADER_SOURCE_DIR}/${_shader}")
    set(_out  "${VECTOR_SHADER_OUTPUT_DIR}/${_stem}.cso")

    add_custom_command(
        OUTPUT  "${_out}"
        COMMAND "${DXC_COMPILER}" -T cs_6_0 -E CSMain "${_src}" -Fo "${_out}"
        DEPENDS "${_src}"
        COMMENT "Compiling DirectX ANN shader: ${_shader}"
        VERBATIM
    )
    list(APPEND COMPILED_VECTOR_SHADERS "${_out}")
endforeach()

if(THEMIS_EAGER_SHADER_BUILD)
    add_custom_target(directx_vector_shaders ALL
        DEPENDS ${COMPILED_VECTOR_SHADERS}
        COMMENT "Building DirectX ANN/Vector-Index shaders"
    )
else()
    add_custom_target(directx_vector_shaders
        DEPENDS ${COMPILED_VECTOR_SHADERS}
        COMMENT "Building DirectX ANN/Vector-Index shaders (on-demand)"
    )
endif()

install(FILES ${COMPILED_VECTOR_SHADERS}
    DESTINATION bin/shaders/vector_index
    COMPONENT shaders
)

# Also install HLSL sources for offline inspection / re-compilation
set(_VECTOR_HLSL_PATHS "")
foreach(_shader IN LISTS VECTOR_HLSL_SHADERS)
    list(APPEND _VECTOR_HLSL_PATHS "${VECTOR_SHADER_SOURCE_DIR}/${_shader}")
endforeach()
install(FILES ${_VECTOR_HLSL_PATHS}
    DESTINATION share/themis/shaders/vector_index/source
    COMPONENT shaders
)

message(STATUS "DirectX ANN shaders → ${VECTOR_SHADER_OUTPUT_DIR}")

# ---------------------------------------------------------------------------
# Geospatial shaders
# ---------------------------------------------------------------------------

set(GEO_SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders/geo")
file(MAKE_DIRECTORY "${GEO_SHADER_OUTPUT_DIR}")

set(GEO_HLSL_SHADERS
    haversine_distance.hlsl
    point_in_polygon.hlsl
)

set(COMPILED_GEO_SHADERS "")
foreach(_shader IN LISTS GEO_HLSL_SHADERS)
    get_filename_component(_stem "${_shader}" NAME_WE)
    set(_src  "${VECTOR_SHADER_SOURCE_DIR}/${_shader}")
    set(_out  "${GEO_SHADER_OUTPUT_DIR}/${_stem}.cso")

    add_custom_command(
        OUTPUT  "${_out}"
        COMMAND "${DXC_COMPILER}" -T cs_6_0 -E CSMain "${_src}" -Fo "${_out}"
        DEPENDS "${_src}"
        COMMENT "Compiling DirectX Geospatial shader: ${_shader}"
        VERBATIM
    )
    list(APPEND COMPILED_GEO_SHADERS "${_out}")
endforeach()

if(THEMIS_EAGER_SHADER_BUILD)
    add_custom_target(directx_geo_shaders ALL
        DEPENDS ${COMPILED_GEO_SHADERS}
        COMMENT "Building DirectX Geospatial shaders"
    )
else()
    add_custom_target(directx_geo_shaders
        DEPENDS ${COMPILED_GEO_SHADERS}
        COMMENT "Building DirectX Geospatial shaders (on-demand)"
    )
endif()

install(FILES ${COMPILED_GEO_SHADERS}
    DESTINATION bin/shaders/geo
    COMPONENT shaders
)

set(_GEO_HLSL_PATHS "")
foreach(_shader IN LISTS GEO_HLSL_SHADERS)
    list(APPEND _GEO_HLSL_PATHS "${VECTOR_SHADER_SOURCE_DIR}/${_shader}")
endforeach()
install(FILES ${_GEO_HLSL_PATHS}
    DESTINATION share/themis/shaders/geo/source
    COMPONENT shaders
)

message(STATUS "DirectX Geo shaders → ${GEO_SHADER_OUTPUT_DIR}")

# ---------------------------------------------------------------------------
# LoRA shaders
# ---------------------------------------------------------------------------

set(LORA_SHADER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/lora")
set(LORA_SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/shaders/lora")
file(MAKE_DIRECTORY "${LORA_SHADER_OUTPUT_DIR}")

set(LORA_HLSL_SHADERS
    elementwise.hlsl
    embedding_lookup.hlsl
    gradient.hlsl
    matmul.hlsl
    sequence_mean.hlsl
    dequantization_nf4.hlsl
    quantization_nf4.hlsl
)

set(COMPILED_LORA_SHADERS "")
foreach(_shader IN LISTS LORA_HLSL_SHADERS)
    get_filename_component(_stem "${_shader}" NAME_WE)
    set(_src  "${LORA_SHADER_SOURCE_DIR}/${_shader}")
    set(_out  "${LORA_SHADER_OUTPUT_DIR}/${_stem}.cso")

    add_custom_command(
        OUTPUT  "${_out}"
        COMMAND "${DXC_COMPILER}" -T cs_6_0 -E CSMain "${_src}" -Fo "${_out}"
        DEPENDS "${_src}"
        COMMENT "Compiling DirectX LoRA shader: ${_shader}"
        VERBATIM
    )
    list(APPEND COMPILED_LORA_SHADERS "${_out}")
endforeach()

if(THEMIS_EAGER_SHADER_BUILD)
    add_custom_target(directx_lora_shaders ALL
        DEPENDS ${COMPILED_LORA_SHADERS}
        COMMENT "Building DirectX LoRA shaders"
    )
else()
    add_custom_target(directx_lora_shaders
        DEPENDS ${COMPILED_LORA_SHADERS}
        COMMENT "Building DirectX LoRA shaders (on-demand)"
    )
endif()

install(FILES ${COMPILED_LORA_SHADERS}
    DESTINATION bin/shaders/lora
    COMPONENT shaders
)

set(_LORA_HLSL_PATHS "")
foreach(_shader IN LISTS LORA_HLSL_SHADERS)
    list(APPEND _LORA_HLSL_PATHS "${LORA_SHADER_SOURCE_DIR}/${_shader}")
endforeach()
install(FILES ${_LORA_HLSL_PATHS}
    DESTINATION share/themis/shaders/lora/source
    COMPONENT shaders
)

message(STATUS "DirectX LoRA shaders → ${LORA_SHADER_OUTPUT_DIR}")
