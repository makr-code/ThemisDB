# Security - Acceleration Module

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via SECURITY.md.

## Threat Model

| Threat | Mitigation surface |
|---|---|
| malicious or tampered plugin loading | plugin security validation and loader controls |
| runtime execution of altered shader/kernels | shader integrity checks and controlled load paths |
| unsafe fallback behavior after backend/runtime errors | deterministic fallback dispatch and error handling |
| resource exhaustion under malformed workloads | validation and bounded resource-management controls |
| cross-device coordination misuse in distributed paths | explicit multi-device coordination and guardrails |

## Security Controls

- plugin loading is gated by dedicated security verification paths
- shader and runtime validation paths enforce integrity checks before execution
- dispatch and fallback logic avoids silent unsafe execution states
- resource-management paths enforce bounded operational behavior

## Known Limitations

- platform-specific verification surfaces may vary by deployment target
- optional plugin paths require ongoing hardening and audit coverage
- some advanced distributed scenarios need broader regression evidence

## Sourcecode Verification (Module: acceleration/security)

- Verified files:
  - src/acceleration/plugin_security.cpp
  - src/acceleration/plugin_loader.cpp
  - src/acceleration/shader_integrity.cpp
  - src/acceleration/backend_registry.cpp
  - src/acceleration/vllm_resource_manager.cpp
  - src/acceleration/multi_gpu_backend.cpp
- Verified controls:
  - plugin and integrity validation behavior
  - fallback/error guard behavior
  - bounded resource and multi-device security-related controls
