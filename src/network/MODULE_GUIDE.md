---
Author: "ThemisDB Contributors"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Network Module Contributor Guide

## Start with the owning paths

- Runtime implementation: `src/network/`
- Public declarations: `include/network/`
- Focused unit/contract tests: `tests/network/`
- Integration soak test: `tests/integration/test_network_protocol_soak.cpp`
- Benchmarks: `benchmarks/network/`

[`README.md`](README.md) provides the existing module overview and examples.
For current release posture, use the root [`ROADMAP.md`](../../ROADMAP.md) and
source evidence rather than the maturity wording in older module summaries.
Read [`ARCHITECTURE.md`](ARCHITECTURE.md) for component boundaries,
[`API.md`](API.md) for caller contracts, and
[`IMPLEMENTATION.md`](IMPLEMENTATION.md) before changing lifecycle or
dispatch behavior.

## Build and run focused tests

The `linux-release` preset defaults to tests disabled. Enable them explicitly,
then build the corresponding focused target and run its registered CTest case:

```sh
cmake --preset linux-release -DTHEMIS_BUILD_TESTS=ON
cmake --build build-gcc-linux-release \
  --target module_network_test_network_contract_hardening_focused_focused
ctest --test-dir build-gcc-linux-release \
  -R '^test_network_contract_hardening_focused_network_FocusedTests$' \
  --output-on-failure
```

`tests/network/CMakeLists.txt` creates each target as
`module_network_<source-stem>_focused` and registers
`<source-stem>_network_FocusedTests`. Choose the source-stem matching the
changed behavior. For example:

| Change area | Focused test source |
|---|---|
| Frame/auth/rate-limit contract | `test_network_contract_hardening_focused.cpp` |
| Transport retry and mixed failures | `test_network_transport_resilience_focused.cpp` |
| Connection lifecycle and QUIC readiness | `test_network_lifecycle_guardrails_focused.cpp` |
| Topology and failover | `test_network_routing_hardening_focused.cpp` |
| UDP packet validation/ACK behavior | `test_udp_server.cpp` |
| QoS command argument safety | `test_qos_manager_command_injection.cpp` |

Build availability depends on configured dependencies and feature flags. If
the preset or binary directory differs, use the actual configured build tree.

## Change checklist

1. Identify whether the change affects a public header, internal implementation,
   or an optional feature guarded by a build definition.
2. Preserve validation-before-dispatch and ownership/thread-safety guarantees.
3. Add or adjust the narrowest regression test that exercises the changed
   behavior, including malformed input or failure paths where relevant.
4. Run the focused test and, for performance-sensitive work, the benchmark
   that exercises the real implementation.
5. Confirm that documentation describes observed behavior rather than
   benchmark targets or optional build features as universal defaults.

The `bench_network_protocol_path.cpp` cases NRG-P1..NRG-P4 are explicitly
marked as stubs; do not use them as proof that a transport implementation
meets a production latency target. See [`RESEARCH.md`](RESEARCH.md).
