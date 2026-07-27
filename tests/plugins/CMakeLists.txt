# plugins module tests
# Auto-discovered by tests/CMakeLists.txt via the */CMakeLists.txt glob.
# Follows the same pattern as tests/auth/CMakeLists.txt and
# tests/importers/CMakeLists.txt.

file(GLOB PLUGINS_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS PLUGINS_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_plugins_${_stem}_focused")
    set(_ctest  "${_stem}_plugins_FocusedTests")
    set(_extra_sources)

    if(TARGET ${_target})
        continue()
    endif()

    if(_stem STREQUAL "test_plugin_registrar_fail_closed")
        list(APPEND _extra_sources
            ${THEMIS_ROOT_DIR}/src/llama_cpp/llama_cpp_plugin.cpp
            ${THEMIS_ROOT_DIR}/src/llama_cpp/llama_cpp_registrar.cpp
            ${THEMIS_ROOT_DIR}/src/rag/rag_context_assembler.cpp
            ${THEMIS_ROOT_DIR}/src/stable_diffusion/sd_plugin.cpp
            ${THEMIS_ROOT_DIR}/src/stable_diffusion/sd_plugin_registrar.cpp
            ${THEMIS_ROOT_DIR}/src/stable_diffusion/sd_config.cpp
            ${THEMIS_ROOT_DIR}/src/stable_diffusion/sd_prompt_sanitizer.cpp
            ${THEMIS_ROOT_DIR}/src/whisper/whisper_plugin.cpp
            ${THEMIS_ROOT_DIR}/src/whisper/whisper_plugin_registrar.cpp
            ${THEMIS_ROOT_DIR}/src/whisper/whisper_config.cpp
            ${THEMIS_ROOT_DIR}/src/whisper/audio_chunk_reader.cpp
        )
    endif()

    add_executable(${_target}
        "${_src}"
        ${_extra_sources}
    )
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    if(DEFINED _themis_vcpkg_include_dir AND _themis_vcpkg_include_dir)
        target_include_directories(${_target} SYSTEM PRIVATE
            ${_themis_vcpkg_include_dir}
        )
    endif()
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        themis_core
        spdlog::spdlog
        Threads::Threads
    )
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE plugins
        NAME   ${_ctest}
        TARGET ${_target}
        TIER   unit
        TIMEOUT 120
        LABELS  plugins
    )
endforeach()
