---
Author: "ThemisDB Contributors"
Created: "2026-08-07"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

> Exact model backend not verifiable from final document text alone.

# Tensor Fingerprint Graph Stress Coverage

This page describes the executable suite in
`test_tensor_stress_suite_focused.cpp`. Earlier workload-size, memory-growth,
and throughput targets in the prior version of this document were design goals,
not the current test contract.

## What the suite runs

The source defines 20 `TSTRESS` cases and eight workload profiles covering
query-heavy, mixed, store-heavy, saturated-read, concurrent, chaos, churn, and
sustained-load patterns. Random generators use seed 42, with a stable
adapter-key hash mixed into TT-train generation.

Although individual cases assign operation counts from 10,000 to 500,000, the
workload mixer clamps actual execution to at most 1,500 operations per case.
This is a focused stress test, not a 500,000-operation or 48-hour soak.

Current assertions include:

- throughput floors of 100 operations/second for normal cases and 50 for chaos
  and extreme-churn cases;
- query-heavy p99 below 100 ms, mixed p95 below 500 ms and p99 below 1,000 ms,
  store-heavy p99 below 2,000 ms, and chaos p99 below 5,000 ms;
- bounded graph entry counts for selected mixed/churn cases and an injected
  failure-rate ceiling below 10%.

These are local test assertions, not representative-hardware performance
baselines. The memory-stability cases check graph entry counts; they do not
measure process RSS or heap growth.

## Registration and execution

`tests/tensor/CMakeLists.txt` registers target
`module_tensor_test_tensor_stress_suite_focused` with CTest name
`test_tensor_stress_suite_focused_tensor_FocusedTests`, labels `tensor` and
`stress` (plus the long-run label), and a 300-second timeout.

```sh
cmake --build <build-dir> --target module_tensor_test_tensor_stress_suite_focused
ctest --test-dir <build-dir> -R '^test_tensor_stress_suite_focused_tensor_FocusedTests$' --output-on-failure
```

Registration does not establish that the target has built or passed; record the
actual command output when reporting validation.
