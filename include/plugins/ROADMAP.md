---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Public plugin header inventory

This file is a source inventory, not an implementation or release-readiness
roadmap. Project-wide readiness is governed by [`../../ROADMAP.md`](../../ROADMAP.md);
module implementation evidence and unresolved work are tracked in
[`../../src/plugins/ROADMAP.md`](../../src/plugins/ROADMAP.md).

The current public header set includes:

- Core plugin, manager, registry, dependency, API, and contract headers:
  `plugin_interface.h`, `plugin_manager.h`, `plugin_registry.h`,
  `plugin_dependency_resolver.h`, `plugin_api.h`, `plugins_api_contract.h`.
- Monitoring and recovery declarations:
  `plugin_health_monitor.h`, `plugin_hot_plug_monitor.h`, `plugin_metrics.h`,
  `self_healing_plugin.h`.
- Distribution/trust declarations:
  `oci_registry_client.h`, `oci_manifest_signing.h`,
  `signed_plugin_repository.h`.
- Domain and runtime declarations:
  `rpc_plugin_interface.h`, `huggingface_ingestion_plugin.h`,
  `image_analysis_interface.h`, `image_analysis_manager.h`,
  `image_generation_interface.h`, `audio_backend_interface.h`,
  `wasm_component_model.h`, `wasm_host_api.h`,
  `edition_policy_plugin.h`, `tesseract_ocr_plugin.h`, and
  `yolov8_onnx_plugin.h`.

**Implementation/build status is not inferred from header presence.** For
example, a declared interface may have no corresponding implementation in
`src/plugins`; runtime and build wiring must be checked independently. The
current scoped focused-test registration is described in
[`../../tests/plugins/README.md`](../../tests/plugins/README.md), and the
registered plugin benchmark target in
[`../../benchmarks/plugins/README.md`](../../benchmarks/plugins/README.md).

**TODO:** No approved public-API delivery milestone or breaking-change plan was
verified in the sources consulted for this inventory.
