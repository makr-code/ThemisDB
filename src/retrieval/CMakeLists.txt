# EPIC 1 — Hybrid Knowledge Retrieval

set(RETRIEVAL_PUBLIC_HEADERS
    include/lora_package.h
)

set(RETRIEVAL_SOURCES
    src/lora_package.cc
    src/lora_package_config.cc
    ${THEMIS_ROOT_DIR}/src/rag/deployment_gate_controller.cpp
)

add_library(themis_retrieval STATIC ${RETRIEVAL_SOURCES})
if(NOT TARGET yaml-cpp::yaml-cpp)
    find_package(yaml-cpp CONFIG REQUIRED)
    if(NOT TARGET yaml-cpp::yaml-cpp AND TARGET yaml-cpp)
        add_library(yaml-cpp::yaml-cpp ALIAS yaml-cpp)
    endif()
endif()
if(NOT TARGET themis_core)
    message(FATAL_ERROR
        "themis_retrieval requires themis_core for ConfigPathResolver")
endif()
target_include_directories(themis_retrieval
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${THEMIS_ROOT_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(themis_retrieval
    PUBLIC nlohmann_json::nlohmann_json
           ${THEMIS_ROCKSDB_TARGET}
    PRIVATE themis_core
            yaml-cpp::yaml-cpp
            OpenSSL::Crypto
)
target_compile_features(themis_retrieval PUBLIC cxx_std_20)
