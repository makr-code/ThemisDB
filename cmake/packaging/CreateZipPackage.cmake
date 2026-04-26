cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED THEMIS_BINARY_DIR)
    message(FATAL_ERROR "THEMIS_BINARY_DIR not defined")
endif()
if(NOT DEFINED THEMIS_CONFIG)
    set(THEMIS_CONFIG "Release")
endif()
if(NOT DEFINED THEMIS_RELEASE_DIR)
    message(FATAL_ERROR "THEMIS_RELEASE_DIR not defined")
endif()
if(NOT DEFINED THEMIS_PACKAGE_NAME)
    message(FATAL_ERROR "THEMIS_PACKAGE_NAME not defined")
endif()
if(NOT DEFINED THEMIS_INCLUDE_DEVELOPMENT)
    set(THEMIS_INCLUDE_DEVELOPMENT OFF)
endif()

set(_staging_root "${THEMIS_RELEASE_DIR}/.staging")
set(_package_root "${_staging_root}/${THEMIS_PACKAGE_NAME}")
set(_zip_file "${THEMIS_RELEASE_DIR}/${THEMIS_PACKAGE_NAME}.zip")

file(MAKE_DIRECTORY "${THEMIS_RELEASE_DIR}")
file(REMOVE_RECURSE "${_package_root}")
file(MAKE_DIRECTORY "${_package_root}")

set(_components runtime tools models shaders Unspecified)
if(THEMIS_INCLUDE_DEVELOPMENT)
    list(APPEND _components development)
endif()

foreach(_component IN LISTS _components)
    execute_process(
        COMMAND "${CMAKE_COMMAND}" --install "${THEMIS_BINARY_DIR}" --config "${THEMIS_CONFIG}" --prefix "${_package_root}" --component "${_component}"
        RESULT_VARIABLE _install_result
        OUTPUT_VARIABLE _install_output
        ERROR_VARIABLE _install_error
    )

    if(NOT _install_result EQUAL 0)
        message(FATAL_ERROR
            "Failed to install component '${_component}' for ZIP packaging.\n"
            "stdout:\n${_install_output}\n"
            "stderr:\n${_install_error}")
    endif()
endforeach()

if(EXISTS "${_zip_file}")
    file(REMOVE "${_zip_file}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar cfv "${_zip_file}" --format=zip "${THEMIS_PACKAGE_NAME}"
    WORKING_DIRECTORY "${_staging_root}"
    RESULT_VARIABLE _zip_result
    OUTPUT_VARIABLE _zip_output
    ERROR_VARIABLE _zip_error
)

if(NOT _zip_result EQUAL 0)
    message(FATAL_ERROR
        "Failed to create ZIP package '${_zip_file}'.\n"
        "stdout:\n${_zip_output}\n"
        "stderr:\n${_zip_error}")
endif()

message(STATUS "Created deployable package: ${_zip_file}")
