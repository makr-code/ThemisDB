cmake_minimum_required(VERSION 3.20)

# Compatibility shim: runtime plugin implementation has been integrated at
# src/user_storage_encrypted. Keep this file for legacy paths under plugins/user_storage_encrypted.

if(NOT THEMIS_ROOT_DIR)
    get_filename_component(THEMIS_ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
endif()

set(_storage_src_dir "${THEMIS_ROOT_DIR}/src/user_storage_encrypted")

if(EXISTS "${_storage_src_dir}/CMakeLists.txt")
    message(STATUS "plugins/user_storage_encrypted: delegating build to src/user_storage_encrypted")
    if(NOT TARGET themis_user_storage_encrypted)
        add_subdirectory("${_storage_src_dir}" "${CMAKE_CURRENT_BINARY_DIR}/src_user_storage_encrypted")
    endif()
else()
    message(FATAL_ERROR "plugins/user_storage_encrypted: expected integrated source at ${_storage_src_dir}")
endif()
