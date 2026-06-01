# Documentation-first CMake scaffold for planned tests.

set(EPIC1_RETRIEVAL_PLANNED_TESTS
    ann_frontdoor_test.cc
    tensor_midlayer_test.cc
    graph_validator_test.cc
    lora_package_test.cc
    model_switch_test.cc
    federated_summaries_test.cc
    observability_test.cc
)

# Test targets will be registered incrementally as implementation files land.
