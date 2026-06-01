# Documentation-first CMake scaffold for planned epic work.

set(RETRIEVAL_PLANNED_PUBLIC_HEADERS
    include/ann_frontdoor.h
    include/tensor_midlayer.h
    include/graph_validator.h
    include/lora_package.h
    include/model_switch.h
    include/federated_summaries.h
    include/retrieval_observability.h
)

set(RETRIEVAL_PLANNED_SOURCES
    src/ann_frontdoor.cc
    src/tensor_midlayer.cc
    src/graph_validator.cc
    src/lora_package.cc
    src/model_switch.cc
    src/federated_summaries.cc
    src/retrieval_observability.cc
)

# Targets intentionally remain disabled until interface review and implementation approval are complete.
