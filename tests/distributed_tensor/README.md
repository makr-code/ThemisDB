---
Author: ThemisDB Contributors
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot Coding Agent
Human_Review: false
Human_Author: unknown
---

Exact model backend not verifiable from final document text alone.

# Distributed Tensor Tests

`CMakeLists.txt` registers three focused local tests:

| Source | Build target | CTest name |
|---|---|---|
| `test_tensor_allocation.cpp` | `module_distributed_tensor_test_tensor_allocation_focused` | `test_tensor_allocation_distributed_tensor_FocusedTests` |
| `test_tensor_operations.cpp` | `module_distributed_tensor_test_tensor_operations_focused` | `test_tensor_operations_distributed_tensor_FocusedTests` |
| `test_tensor_performance.cpp` | `module_distributed_tensor_test_tensor_performance_focused` | `test_tensor_performance_distributed_tensor_FocusedTests` |

These registrations use the `distributed_tensor` label and a 120-second
timeout. After configuring the repository with tests enabled, build the targets
and run the registered tests:

```sh
cmake --build <build-dir> --target \
  module_distributed_tensor_test_tensor_allocation_focused \
  module_distributed_tensor_test_tensor_operations_focused \
  module_distributed_tensor_test_tensor_performance_focused
ctest --test-dir <build-dir> -R \
  '^(test_tensor_allocation|test_tensor_operations|test_tensor_performance)_distributed_tensor_FocusedTests$'
```

`test_distributed_tensor_highcardinality_stress.cpp` is present in this
directory but is not listed by this CMake file, so it has no local target
registered here. The separate `../epic3_distributed_tensor/` suite is
configured by its own CMake file and has additional test sources and evidence
requirements. Source registration alone does not establish a test pass.
