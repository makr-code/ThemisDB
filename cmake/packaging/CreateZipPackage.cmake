cmake_minimum_required(VERSION 3.20)

if(NOT DEFINED THEMIS_BINARY_DIR)
    message(FATAL_ERROR "THEMIS_BINARY_DIR not defined")
endif()
if(NOT DEFINED THEMIS_SOURCE_DIR)
    message(FATAL_ERROR "THEMIS_SOURCE_DIR not defined")
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
if(NOT DEFINED THEMIS_PACKAGE_INCLUDE_TESTS)
    set(THEMIS_PACKAGE_INCLUDE_TESTS OFF)
endif()
if(NOT DEFINED THEMIS_PACKAGE_INCLUDE_BENCHMARKS)
    set(THEMIS_PACKAGE_INCLUDE_BENCHMARKS OFF)
endif()

set(_runtime_layout_file "${THEMIS_SOURCE_DIR}/cmake/packaging/RuntimeLayout.cmake")
if(EXISTS "${_runtime_layout_file}")
    include("${_runtime_layout_file}")
endif()

set(_staging_root "${THEMIS_RELEASE_DIR}/.staging")
set(_package_root "${_staging_root}/${THEMIS_PACKAGE_NAME}")
set(_zip_file "${THEMIS_RELEASE_DIR}/${THEMIS_PACKAGE_NAME}.zip")

file(MAKE_DIRECTORY "${THEMIS_RELEASE_DIR}")
file(REMOVE_RECURSE "${_package_root}")
file(MAKE_DIRECTORY "${_package_root}")

if(DEFINED THEMIS_RELEASE_COMPONENTS_ZIP)
    set(_components ${THEMIS_RELEASE_COMPONENTS_ZIP})
else()
    set(_components runtime tools models shaders documentation)
endif()
if(THEMIS_INCLUDE_DEVELOPMENT)
    list(APPEND _components development)
endif()
if(THEMIS_PACKAGE_INCLUDE_TESTS)
    list(APPEND _components tests)
endif()
if(THEMIS_PACKAGE_INCLUDE_BENCHMARKS)
    list(APPEND _components benchmarks)
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

# Ensure all built binaries (server/tools/tests/benchmarks + runtime DLLs)
# are present in the deployable ZIP even if individual install components lag.
set(_build_bin_dir "${THEMIS_BINARY_DIR}/bin")
if(EXISTS "${_build_bin_dir}")
    file(MAKE_DIRECTORY "${_package_root}/bin")
    execute_process(
        COMMAND "${CMAKE_COMMAND}" -E copy_directory "${_build_bin_dir}" "${_package_root}/bin"
        RESULT_VARIABLE _copy_bin_result
        OUTPUT_VARIABLE _copy_bin_output
        ERROR_VARIABLE _copy_bin_error
    )
    if(NOT _copy_bin_result EQUAL 0)
        message(FATAL_ERROR
            "Failed to copy build bin directory into ZIP staging.\n"
            "stdout:\n${_copy_bin_output}\n"
            "stderr:\n${_copy_bin_error}")
    endif()
endif()

set(_runtime_sync_script "${THEMIS_SOURCE_DIR}/cmake/CopyRuntimeDlls.cmake")
if(EXISTS "${_runtime_sync_script}")
    set(_runtime_sync_args
        -DBIN_DIR=${_build_bin_dir}
        -DDST_DIR=${_package_root}/bin
    )
    if(DEFINED THEMIS_VCPKG_ROOT AND DEFINED VCPKG_TARGET_TRIPLET AND NOT "${THEMIS_VCPKG_ROOT}" STREQUAL "" AND NOT "${VCPKG_TARGET_TRIPLET}" STREQUAL "")
        list(APPEND _runtime_sync_args
            -DVCPKG_BIN_DIR=${THEMIS_VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/bin
        )
    endif()

    execute_process(
        COMMAND "${CMAKE_COMMAND}" ${_runtime_sync_args} -P "${_runtime_sync_script}"
        RESULT_VARIABLE _sync_runtime_result
        OUTPUT_VARIABLE _sync_runtime_output
        ERROR_VARIABLE _sync_runtime_error
    )
    if(NOT _sync_runtime_result EQUAL 0)
        message(FATAL_ERROR
            "Failed to sync runtime DLLs into ZIP staging.\n"
            "stdout:\n${_sync_runtime_output}\n"
            "stderr:\n${_sync_runtime_error}")
    endif()
endif()

# Keep tests/benchmarks in deploy package only when explicitly enabled.
if(NOT THEMIS_PACKAGE_INCLUDE_TESTS)
    file(GLOB _themis_test_bins "${_package_root}/bin/test*.exe")
    if(_themis_test_bins)
        file(REMOVE ${_themis_test_bins})
    endif()
endif()

if(NOT THEMIS_PACKAGE_INCLUDE_BENCHMARKS)
    file(GLOB _themis_bench_bins
        "${_package_root}/bin/bench*.exe"
        "${_package_root}/bin/llm_bench.exe")
    if(_themis_bench_bins)
        file(REMOVE ${_themis_bench_bins})
    endif()
endif()

# Copy runtime directory skeleton/content declared in central runtime layout.
if(DEFINED THEMIS_RELEASE_RUNTIME_DIRS)
    foreach(_runtime_dir IN LISTS THEMIS_RELEASE_RUNTIME_DIRS)
        if(EXISTS "${THEMIS_SOURCE_DIR}/${_runtime_dir}")
            file(COPY "${THEMIS_SOURCE_DIR}/${_runtime_dir}" DESTINATION "${_package_root}")
        endif()
    endforeach()
endif()

# Copy top-level release docs into package root.
if(DEFINED THEMIS_RELEASE_ROOT_DOC_FILES)
    foreach(_doc_file IN LISTS THEMIS_RELEASE_ROOT_DOC_FILES)
        if(EXISTS "${THEMIS_SOURCE_DIR}/${_doc_file}")
            file(COPY "${THEMIS_SOURCE_DIR}/${_doc_file}" DESTINATION "${_package_root}")
        endif()
    endforeach()
endif()

# Hard requirements for release handoff docs and setup helper.
set(_required_root_docs
    README.md
    CHANGELOG.md
    RELEASE_STRATEGY.md
    QUICKSTART.md
    SETUP.md
    VERSION
    VERSIONING.md
)
foreach(_doc IN LISTS _required_root_docs)
    if(EXISTS "${THEMIS_SOURCE_DIR}/${_doc}")
        file(COPY "${THEMIS_SOURCE_DIR}/${_doc}" DESTINATION "${_package_root}")
    endif()
endforeach()

if(EXISTS "${THEMIS_SOURCE_DIR}/docs/packaging/runtime-layout.md")
    file(MAKE_DIRECTORY "${_package_root}/docs")
    file(COPY "${THEMIS_SOURCE_DIR}/docs/packaging/runtime-layout.md" DESTINATION "${_package_root}/docs")
endif()

if(EXISTS "${THEMIS_SOURCE_DIR}/scripts/setup-runtime-env.ps1")
    file(MAKE_DIRECTORY "${_package_root}/bin")
    file(COPY "${THEMIS_SOURCE_DIR}/scripts/setup-runtime-env.ps1" DESTINATION "${_package_root}/bin")
endif()

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
