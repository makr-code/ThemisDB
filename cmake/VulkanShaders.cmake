# VulkanShaders.cmake
# CMake module for compiling Vulkan GLSL shaders to SPIR-V

# Find glslangValidator or glslc compiler
set(_THEMIS_VULKAN_SDK_HINTS
    ENV VULKAN_SDK
    ${Vulkan_GLSLC_EXECUTABLE}
    ${VULKAN_SDK}/bin
    ${VULKAN_SDK}/Bin
    $ENV{VULKAN_SDK}/bin
    $ENV{VULKAN_SDK}/Bin
)

find_program(GLSLANG_VALIDATOR glslangValidator
    HINTS
        ${_THEMIS_VULKAN_SDK_HINTS}
    PATHS
        /usr/bin
        /usr/local/bin
)

find_program(GLSLC glslc
    HINTS
        ${_THEMIS_VULKAN_SDK_HINTS}
    PATHS
        /usr/bin
        /usr/local/bin
)

# Prefer glslc over glslangValidator
if(GLSLC)
    set(VULKAN_SHADER_COMPILER "${GLSLC}")
    set(VULKAN_SHADER_COMPILER_NAME "glslc")
    message(STATUS "Found Vulkan shader compiler: ${GLSLC}")
elseif(GLSLANG_VALIDATOR)
    set(VULKAN_SHADER_COMPILER "${GLSLANG_VALIDATOR}")
    set(VULKAN_SHADER_COMPILER_NAME "glslangValidator")
    message(STATUS "Found Vulkan shader compiler: ${GLSLANG_VALIDATOR}")
else()
    message(WARNING "Vulkan shader compiler not found. Shaders will need to be pre-compiled.")
    message(WARNING "Install Vulkan SDK or glslang-tools package to compile shaders.")
    set(VULKAN_SHADER_COMPILER_FOUND FALSE)
    return()
endif()

set(VULKAN_SHADER_COMPILER_FOUND TRUE)

# Function to compile a single shader
# Usage: compile_vulkan_shader(TARGET shader_file OUTPUT output_file)
function(compile_vulkan_shader TARGET SHADER_FILE)
    cmake_parse_arguments(PARSE_ARGV 2 ARG "" "OUTPUT" "")
    
    get_filename_component(SHADER_NAME ${SHADER_FILE} NAME)
    get_filename_component(SHADER_DIR ${SHADER_FILE} DIRECTORY)
    
    if(ARG_OUTPUT)
        set(OUTPUT_FILE ${ARG_OUTPUT})
    else()
        set(OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/${SHADER_NAME}.spv)
    endif()
    
    # Set compiler flags based on which compiler we're using
    if(VULKAN_SHADER_COMPILER_NAME STREQUAL "glslc")
        set(COMPILER_FLAGS -o ${OUTPUT_FILE})
    else()
        set(COMPILER_FLAGS -V -o ${OUTPUT_FILE})
    endif()
    
    add_custom_command(
        OUTPUT ${OUTPUT_FILE}
        COMMAND ${VULKAN_SHADER_COMPILER} ${COMPILER_FLAGS} ${SHADER_FILE}
        DEPENDS ${SHADER_FILE}
        COMMENT "Compiling Vulkan shader: ${SHADER_NAME}"
        VERBATIM
    )
    
    add_custom_target(${TARGET}
        DEPENDS ${OUTPUT_FILE}
    )
endfunction()

# Function to compile multiple shaders
# Usage: compile_vulkan_shaders(TARGET shader_files...)
function(compile_vulkan_shaders TARGET)
    set(SHADER_FILES ${ARGN})
    set(COMPILED_SHADERS "")
    
    foreach(SHADER_FILE ${SHADER_FILES})
        get_filename_component(SHADER_NAME ${SHADER_FILE} NAME)
        set(OUTPUT_FILE ${CMAKE_CURRENT_BINARY_DIR}/shaders/${SHADER_NAME}.spv)
        
        # Create output directory
        get_filename_component(OUTPUT_DIR ${OUTPUT_FILE} DIRECTORY)
        file(MAKE_DIRECTORY ${OUTPUT_DIR})
        
        # Set compiler flags
        if(VULKAN_SHADER_COMPILER_NAME STREQUAL "glslc")
            set(COMPILER_FLAGS -o ${OUTPUT_FILE})
        else()
            set(COMPILER_FLAGS -V -o ${OUTPUT_FILE})
        endif()
        
        add_custom_command(
            OUTPUT ${OUTPUT_FILE}
            COMMAND ${VULKAN_SHADER_COMPILER} ${COMPILER_FLAGS} ${SHADER_FILE}
            DEPENDS ${SHADER_FILE}
            COMMENT "Compiling Vulkan shader: ${SHADER_NAME}"
            VERBATIM
        )
        
        list(APPEND COMPILED_SHADERS ${OUTPUT_FILE})
    endforeach()
    
    if(THEMIS_EAGER_SHADER_BUILD)
        add_custom_target(${TARGET} ALL
            DEPENDS ${COMPILED_SHADERS}
        )
    else()
        add_custom_target(${TARGET}
            DEPENDS ${COMPILED_SHADERS}
        )
    endif()
endfunction()

# Function to install compiled shaders
# Usage: install_vulkan_shaders(FILES shader_files... DESTINATION dest)
function(install_vulkan_shaders)
    cmake_parse_arguments(ARG "" "DESTINATION" "FILES" ${ARGN})
    
    if(NOT ARG_DESTINATION)
        set(ARG_DESTINATION "share/themis/shaders")
    endif()
    
    install(FILES ${ARG_FILES}
        DESTINATION ${ARG_DESTINATION}
    )
endfunction()
