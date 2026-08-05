# RewriteEngine Phase 2 implementation build configuration
# Builds core rewrite engine, rule types, YAML loader, and observability

cmake_minimum_required(VERSION 3.20)

set(REWRITE_ENGINE_SOURCES
    rewrite_engine.cpp
    rewrite_rule_base.cpp
    rewrite_rule_loader.cpp
    rewrite_metrics.cpp
)

set(REWRITE_ENGINE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/prompt_engineering/rewrite_engine.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/prompt_engineering/rewrite_rule.h
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include/prompt_engineering/rewrite_rule_loader.h
)

# Add rewrite engine to main themis_core library
# (assuming themis_core is the primary library in prompt_engineering module)
# Note: This file should be included from a parent CMakeLists.txt that builds themis_core

# If building as standalone library for testing:
if(NOT TARGET themis_prompt_engineering_rewrite)
    add_library(themis_prompt_engineering_rewrite STATIC ${REWRITE_ENGINE_SOURCES})
    
    target_include_directories(themis_prompt_engineering_rewrite
        PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include
                ${CMAKE_CURRENT_SOURCE_DIR}
    )
    
    target_link_libraries(themis_prompt_engineering_rewrite
        PUBLIC
            nlohmann_json::nlohmann_json
            spdlog::spdlog
            yaml-cpp
    )
    
    target_compile_features(themis_prompt_engineering_rewrite PUBLIC cxx_std_17)
endif()
