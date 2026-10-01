find_package(GTest CONFIG REQUIRED)

add_executable(themis_docs_builder_validator_test
    test_validator.cpp
    ${PROJECT_SOURCE_DIR}/src/validator.cpp
)

target_include_directories(themis_docs_builder_validator_test PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(themis_docs_builder_validator_test
    PRIVATE
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(themis_docs_builder_validator_test)
