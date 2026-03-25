# Tests for User Storage Encrypted Plugin
cmake_minimum_required(VERSION 3.20)

find_package(GTest REQUIRED)

# libargon2 (required by key_derivation_service)
find_library(ARGON2_LIB NAMES argon2)
find_path(ARGON2_INCLUDE_DIR NAMES argon2.h)
if(NOT ARGON2_LIB OR NOT ARGON2_INCLUDE_DIR)
    message(FATAL_ERROR "libargon2 not found. Install: apt-get install libargon2-dev")
endif()

# ── Existing test binary ────────────────────────────────────────────────────
add_executable(test_user_storage_encrypted
    test_multi_level_storage.cpp
)

add_executable(test_user_storage_features
    test_user_storage_features.cpp
)

foreach(_target test_user_storage_encrypted test_user_storage_features)
    target_include_directories(${_target}
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/..
            ${CMAKE_SOURCE_DIR}/include
    )
    target_link_libraries(${_target}
        PRIVATE
            themis_user_storage_encrypted
            GTest::GTest
            GTest::Main
            nlohmann_json::nlohmann_json
    )
    target_compile_features(${_target} PRIVATE cxx_std_17)
endforeach()
target_include_directories(test_user_storage_encrypted
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_SOURCE_DIR}/include
        ${GTEST_INCLUDE_DIRS}
)

target_link_libraries(test_user_storage_encrypted
    PRIVATE
        themis_user_storage_encrypted
        GTest::GTest
        GTest::Main
        nlohmann_json::nlohmann_json
)

target_compile_features(test_user_storage_encrypted PRIVATE cxx_std_17)

add_test(NAME UserStorageEncrypted COMMAND test_user_storage_encrypted)
set_tests_properties(UserStorageEncrypted PROPERTIES TIMEOUT 60)

add_test(NAME UserStorageFeatures COMMAND test_user_storage_features)
set_tests_properties(UserStorageFeatures PROPERTIES TIMEOUT 120 LABELS "UserStorageFocusedTests")
# ── New features test binary (items 1–3) ────────────────────────────────────
add_executable(test_user_storage_features
    test_user_storage_features.cpp
)

target_include_directories(test_user_storage_features
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_SOURCE_DIR}/include
        ${GTEST_INCLUDE_DIRS}
        ${ARGON2_INCLUDE_DIR}
)

target_link_libraries(test_user_storage_features
    PRIVATE
        themis_user_storage_encrypted
        GTest::GTest
        GTest::Main
        nlohmann_json::nlohmann_json
        ${ARGON2_LIB}
)

target_compile_features(test_user_storage_features PRIVATE cxx_std_17)

add_test(NAME UserStorageFeatures COMMAND test_user_storage_features)
set_tests_properties(UserStorageFeatures PROPERTIES TIMEOUT 120)
