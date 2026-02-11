# Tests for User Storage Encrypted Plugin
cmake_minimum_required(VERSION 3.20)

# Find GTest
find_package(GTest REQUIRED)

# Test executable
add_executable(test_user_storage_encrypted
    test_multi_level_storage.cpp
)

# Include directories
target_include_directories(test_user_storage_encrypted
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/..
        ${CMAKE_SOURCE_DIR}/include
        ${GTEST_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(test_user_storage_encrypted
    PRIVATE
        themis_user_storage_encrypted
        GTest::GTest
        GTest::Main
        nlohmann_json::nlohmann_json
)

# C++17 required
target_compile_features(test_user_storage_encrypted PRIVATE cxx_std_17)

# Add test to CTest
add_test(NAME UserStorageEncrypted COMMAND test_user_storage_encrypted)

# Set test timeout
set_tests_properties(UserStorageEncrypted PROPERTIES TIMEOUT 60)
