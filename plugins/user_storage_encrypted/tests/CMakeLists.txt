# Tests for User Storage Encrypted Plugin
cmake_minimum_required(VERSION 3.20)

find_package(GTest REQUIRED)

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

add_test(NAME UserStorageEncrypted COMMAND test_user_storage_encrypted)
set_tests_properties(UserStorageEncrypted PROPERTIES TIMEOUT 60)

add_test(NAME UserStorageFeatures COMMAND test_user_storage_features)
set_tests_properties(UserStorageFeatures PROPERTIES TIMEOUT 120 LABELS "UserStorageFocusedTests")
