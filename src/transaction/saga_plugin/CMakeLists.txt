cmake_minimum_required(VERSION 3.20)

project(saga_orchestrator_plugin VERSION 1.0.0 LANGUAGES CXX)

if(NOT THEMIS_ROOT_DIR)
    get_filename_component(THEMIS_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
endif()

add_library(saga_orchestrator_plugin SHARED
    ${CMAKE_CURRENT_SOURCE_DIR}/saga_orchestrator_plugin.cpp
    ${THEMIS_ROOT_DIR}/src/transaction/saga_orchestrator.cpp
)

target_include_directories(saga_orchestrator_plugin PUBLIC
    ${THEMIS_ROOT_DIR}/include
)

target_compile_features(saga_orchestrator_plugin PUBLIC cxx_std_17)
target_compile_definitions(saga_orchestrator_plugin PRIVATE THEMIS_PLUGIN_EXPORTS)

find_package(nlohmann_json QUIET)
if(nlohmann_json_FOUND)
    target_link_libraries(saga_orchestrator_plugin PUBLIC nlohmann_json::nlohmann_json)
endif()

find_package(fmt CONFIG QUIET)
if(fmt_FOUND)
    target_link_libraries(saga_orchestrator_plugin PRIVATE fmt::fmt)
endif()

set(SAGA_ORCHESTRATOR_MANIFEST_PATH "${CMAKE_CURRENT_BINARY_DIR}/plugin.json")
set(SAGA_ORCHESTRATOR_MANIFEST_SIG_PATH "${CMAKE_CURRENT_BINARY_DIR}/plugin.json.sig")

configure_file(
    ${THEMIS_ROOT_DIR}/plugins/saga/plugin.json.in
    ${SAGA_ORCHESTRATOR_MANIFEST_PATH}
    @ONLY
)

file(SHA256 "${SAGA_ORCHESTRATOR_MANIFEST_PATH}" SAGA_ORCHESTRATOR_MANIFEST_SHA256)
file(WRITE "${SAGA_ORCHESTRATOR_MANIFEST_SIG_PATH}" "${SAGA_ORCHESTRATOR_MANIFEST_SHA256}\n")

add_custom_command(TARGET saga_orchestrator_plugin POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SAGA_ORCHESTRATOR_MANIFEST_PATH}"
        "$<TARGET_FILE_DIR:saga_orchestrator_plugin>/saga_orchestrator.plugin.json"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SAGA_ORCHESTRATOR_MANIFEST_SIG_PATH}"
        "$<TARGET_FILE_DIR:saga_orchestrator_plugin>/saga_orchestrator.plugin.json.sig"
)

add_custom_target(saga_orchestrator_plugin_runtime_assets
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SAGA_ORCHESTRATOR_MANIFEST_PATH}"
        "$<TARGET_FILE_DIR:saga_orchestrator_plugin>/saga_orchestrator.plugin.json"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${SAGA_ORCHESTRATOR_MANIFEST_SIG_PATH}"
        "$<TARGET_FILE_DIR:saga_orchestrator_plugin>/saga_orchestrator.plugin.json.sig"
    DEPENDS
        saga_orchestrator_plugin
        "${SAGA_ORCHESTRATOR_MANIFEST_PATH}"
        "${SAGA_ORCHESTRATOR_MANIFEST_SIG_PATH}"
)

message(STATUS "saga_orchestrator_plugin v${PROJECT_VERSION} configured")
