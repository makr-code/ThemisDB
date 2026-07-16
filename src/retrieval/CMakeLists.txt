# EPIC 1 — Hybrid Knowledge Retrieval

set(RETRIEVAL_PUBLIC_HEADERS
    include/lora_package.h
)

set(RETRIEVAL_SOURCES
    src/lora_package.cc
)

add_library(themis_retrieval STATIC ${RETRIEVAL_SOURCES})
target_include_directories(themis_retrieval
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(themis_retrieval
    PUBLIC nlohmann_json::nlohmann_json
)
target_compile_features(themis_retrieval PUBLIC cxx_std_17)
