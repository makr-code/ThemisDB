# EPIC 1 — Hybrid Knowledge Retrieval: skeleton headers and sources are in place.
# Enable library targets after interface review and acceptance-test sign-off.

set(RETRIEVAL_PUBLIC_HEADERS
    include/ann_frontdoor.h
    include/tensor_midlayer.h
    include/graph_validator.h
    include/lora_package.h
    include/model_switch.h
    include/federated_summaries.h
    include/retrieval_observability.h
)

set(RETRIEVAL_SOURCES
    src/ann_frontdoor.cc
    src/tensor_midlayer.cc
    src/graph_validator.cc
    src/lora_package.cc
    src/model_switch.cc
    src/federated_summaries.cc
    src/retrieval_observability.cc
)

# Uncomment when build targets are approved for activation:
# add_library(themis_retrieval STATIC ${RETRIEVAL_SOURCES})
# target_include_directories(themis_retrieval PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
