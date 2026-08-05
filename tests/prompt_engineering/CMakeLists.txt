cmake_minimum_required(VERSION 3.20)
# prompt_engineering module tests

file(GLOB PROMPT_ENGINEERING_MODULE_TEST_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test_*.cpp"
)

foreach(_src IN LISTS PROMPT_ENGINEERING_MODULE_TEST_SOURCES)
    get_filename_component(_stem "${_src}" NAME_WE)
    set(_target "module_prompt_engineering_${_stem}_focused")
    set(_ctest "${_stem}_prompt_engineering_FocusedTests")

    if(TARGET ${_target})
        continue()
    endif()

    add_executable(${_target} "${_src}")
    # Link against themis_core for prompt_engineering module
    target_link_libraries(${_target} PRIVATE
        ${TEST_LIBS}
        themis_core
        themis_llm
        spdlog::spdlog
        Threads::Threads
    )
    target_include_directories(${_target} PRIVATE
        ${THEMIS_ROOT_DIR}/include
        ${THEMIS_ROOT_DIR}/src
    )
    target_compile_definitions(${_target} PRIVATE THEMIS_TEST_BUILD=1)

    themis_register_module_focused_test(
        MODULE prompt_engineering
        NAME ${_ctest}
        TARGET ${_target}
        TIER unit
        TIMEOUT 120
        LABELS prompt_engineering
    )
endforeach()
