---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-11-13"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# Developer LLM Wiki — Modules and APIs

Datum: 2026-11-13
Status: Active

## Curated module contexts

- [AQL module context](AQL.md) — assistance API boundaries, validation behavior, and test/benchmark registration.
- [Graph module context](GRAPH.md) — graph execution boundaries, RotatE evidence, focused tests, and archived artifacts.
- [Ethics AI module context](ETHICS_AI.md) — in-tree API/runtime boundaries, LDM helper limits, test/benchmark registration, and canonical plugin source boundary.
- [Prompt engineering module context](PROMPT_ENGINEERING.md) — prompt runtime boundaries, trust caveats, and test/benchmark registration.
- [Importer module context](IMPORTERS.md) — shared contracts, connector boundaries, focused test evidence, and operational limits.
- [Updates module context](UPDATES.md) — update lifecycle, rollout and patch contracts, diagnostic evidence, and operator boundaries.
- [Distributed tensor module context](DISTRIBUTED_TENSOR.md) — tensor artifact contracts, planning boundaries, C++ requirements, and verified test/benchmark evidence.
- [Process module context](PROCESS.md) — process model lifecycle, retrieval boundaries, test registration, and benchmark evidence.
- [Temporal module context](TEMPORAL.md) — runtime/API separation, persistence boundaries, test and benchmark evidence limits.
- [Storage module context](STORAGE.md) — component boundaries, contract-vs-evidence gaps, and focused test/benchmark limits.
- [Encrypted user storage module context](USER_STORAGE_ENCRYPTED.md) — runtime architecture, configuration, build/test boundaries, and scanner/Doxygen evidence limits.
- [Maintenance module context](MAINTENANCE.md) — runtime API boundaries, schedule CRUD churn enforcement, persistence behavior, and benchmark evidence status.


## Curated vector-search contract (#8089)

Sources: [server handler](../../src/server/vector_api_handler.cpp),
[public handler contract](../../include/server/vector_api_handler.h),
[synchronous Python SDK](../../clients/python/themis/__init__.py).

- `POST /vector/search` accepts omitted `filter` or `{}` as no-ops. Nonempty
  objects and all nonobjects, including JSON `null`, return HTTP 400. Caller
  metadata filters are not authorization policy or tenant-isolation controls;
  existing authorization/governance checks remain separate.
- Synchronous `ThemisClient.vector_search()` merges successful shard hits into
  global top-k, including when only one endpoint succeeds. Prefer descending
  non-null scores for all hits (zero is valid), else ascending non-null distances
  for all hits; scores/distances are expected to be numeric. No common metric
  means `ValueError` (`Vector search hits lack a common non-null score or distance`), not a
  mixed score/distance comparison; ties retain endpoint and response order.
- HTTP/request shard failures produce `partial=True` and an `errors` list
  (`endpoint`, `status`, `message`; sanitized origins, request status `None`).
  Messages are `HTTP <status>` or `Request failure (<exception class>)`, without
  server response bodies. At least two
  successes produce merged `results` plus original `partials`; one success,
  even with failed shards, preserves response keys and `results`/`items` format
  while ranking/capping hits and updating `count`/`batch_size` when present. The SDK adds
  `partial=True`/`errors` for failures, but no `partials` for one success.
  All failures raise `httpx.HTTPError` with an `errors` attribute; a successful
  empty shard response remains valid.
- Single-endpoint cursor fields are retained; there is no global shard cursor.
  Client truncation of oversized shard responses may skip hits on cursor continuation.
  Previously ignored nonempty filters now error, silent partial success becomes
  explicit, and corrected ranking can change order/top-k membership.
- Scope/evidence gap: `AsyncThemisClient` is unchanged by this SDK task. Focused
  regression sources are [SDK vector tests](../../clients/python/tests/test_vector_search.py)
  and [HTTP vector tests](../../tests/network/test_http_vector.cpp); reading them
  is not execution evidence. This curated section fills the vector-specific
  contract gap in the fresh 2026-11-13 wiki (8,134 sources); generated hashes and
  manifests are not regenerated. See the [SDK reference](../../clients/python/README.md#synchronous-vector-search-contract-8089)
  for caller-facing compatibility details.

## include/acceleration/ai_hardware_dispatcher.h
- Kategorie: modules_and_api
- Hash: `9ded277956ff475c`
- Titel: (binary or unreadable)

## include/acceleration/batch_validator.h
- Kategorie: modules_and_api
- Hash: `c5acdfb907f27ed0`
- Titel: (binary or unreadable)

## include/acceleration/break_even_validator.h
- Kategorie: modules_and_api
- Hash: `b9cfca072b47a842`
- Titel: (binary or unreadable)

## include/acceleration/compute_backend.h
- Kategorie: modules_and_api
- Hash: `3d2015eecd4b6cf6`
- Titel: (binary or unreadable)

## include/acceleration/compute_future.h
- Kategorie: modules_and_api
- Hash: `bf0a8d611a6da167`
- Titel: (binary or unreadable)

## include/acceleration/compute_graph.h
- Kategorie: modules_and_api
- Hash: `f790afc3b77f058d`
- Titel: (binary or unreadable)

## include/acceleration/cpu_backend.h
- Kategorie: modules_and_api
- Hash: `b862b4b7d05faa81`
- Titel: (binary or unreadable)

## include/acceleration/cuda_backend.h
- Kategorie: modules_and_api
- Hash: `6d444273b95c4937`
- Titel: (binary or unreadable)

## include/acceleration/device_manager.h
- Kategorie: modules_and_api
- Hash: `09ff17cb8fa48e05`
- Titel: (binary or unreadable)

## include/acceleration/error_codes.h
- Kategorie: modules_and_api
- Hash: `8e9f401a9bd61216`
- Titel: (binary or unreadable)

## include/acceleration/error_context.h
- Kategorie: modules_and_api
- Hash: `b51c0f986b3c38bd`
- Titel: (binary or unreadable)

## include/acceleration/faiss_gpu_backend.h
- Kategorie: modules_and_api
- Hash: `68b037b67ecd9946`
- Titel: (binary or unreadable)

## include/acceleration/fp16_vector_kernels.h
- Kategorie: modules_and_api
- Hash: `d967fab8304c8466`
- Titel: (binary or unreadable)

## include/acceleration/geo_acceleration_bridge.h
- Kategorie: modules_and_api
- Hash: `f9d309c844153744`
- Titel: (binary or unreadable)

## include/acceleration/graph_kernel_classification.h
- Kategorie: modules_and_api
- Hash: `004baf5766ec8eda`
- Titel: (binary or unreadable)

## include/acceleration/graphics_backends.h
- Kategorie: modules_and_api
- Hash: `f2b77545cc7ea95a`
- Titel: (binary or unreadable)

## include/acceleration/hip_backend.h
- Kategorie: modules_and_api
- Hash: `f450cfb7a8f77e9f`
- Titel: (binary or unreadable)

## include/acceleration/kernel_fallback_dispatcher.h
- Kategorie: modules_and_api
- Hash: `119a93d70466ce79`
- Titel: (binary or unreadable)

## include/acceleration/kernel_invocation.h
- Kategorie: modules_and_api
- Hash: `e6f0c563429353e5`
- Titel: (binary or unreadable)

## include/acceleration/kernel_registry.h
- Kategorie: modules_and_api
- Hash: `addaf447abf1581b`
- Titel: (binary or unreadable)

## include/acceleration/metrics/backend_metrics.h
- Kategorie: modules_and_api
- Hash: `d4cb5774193a7833`
- Titel: (binary or unreadable)

## include/acceleration/metrics/metrics_collector.h
- Kategorie: modules_and_api
- Hash: `4983ccefff0c28b8`
- Titel: (binary or unreadable)

## include/acceleration/multi_gpu_backend.h
- Kategorie: modules_and_api
- Hash: `d46a0f46d506c24a`
- Titel: (binary or unreadable)

## include/acceleration/nccl_vector_backend.h
- Kategorie: modules_and_api
- Hash: `b5f0a349f28550d3`
- Titel: (binary or unreadable)

## include/acceleration/opencl_backend.h
- Kategorie: modules_and_api
- Hash: `75013ffd8fb584af`
- Titel: (binary or unreadable)

## include/acceleration/plugin_loader.h
- Kategorie: modules_and_api
- Hash: `8d21c5b6d7d0baa2`
- Titel: (binary or unreadable)

## include/acceleration/plugin_security.h
- Kategorie: modules_and_api
- Hash: `4d826abe959e5ef4`
- Titel: (binary or unreadable)

## include/acceleration/quantized_backend.h
- Kategorie: modules_and_api
- Hash: `27add24a15f75b3e`
- Titel: (binary or unreadable)

## include/acceleration/raii/cuda_raii.h
- Kategorie: modules_and_api
- Hash: `4011547e43af20eb`
- Titel: (binary or unreadable)

## include/acceleration/raii/hip_raii.h
- Kategorie: modules_and_api
- Hash: `33a561b709a48dfd`
- Titel: (binary or unreadable)

## include/acceleration/raii/opencl_raii.h
- Kategorie: modules_and_api
- Hash: `030f0ba838c178dc`
- Titel: (binary or unreadable)

## include/acceleration/raii/vulkan_raii.h
- Kategorie: modules_and_api
- Hash: `c69316917e378897`
- Titel: (binary or unreadable)

## include/acceleration/rccl_vector_backend.h
- Kategorie: modules_and_api
- Hash: `e26bc97cdb5c33a0`
- Titel: (binary or unreadable)

## include/acceleration/shader_integrity.h
- Kategorie: modules_and_api
- Hash: `2b551446cd8a622f`
- Titel: (binary or unreadable)

## include/acceleration/tensor_core_matmul.h
- Kategorie: modules_and_api
- Hash: `cb373064a51b6e73`
- Titel: (binary or unreadable)

## include/acceleration/vec_knn.h
- Kategorie: modules_and_api
- Hash: `f883a9f705f215f3`
- Titel: (binary or unreadable)

## include/acceleration/vllm_resource_manager.h
- Kategorie: modules_and_api
- Hash: `d263fca11564dccc`
- Titel: (binary or unreadable)

## include/acceleration/vulkan_backend.h
- Kategorie: modules_and_api
- Hash: `225aa9d1b6a1df23`
- Titel: (binary or unreadable)

## include/acceleration/zluda_backend.h
- Kategorie: modules_and_api
- Hash: `87f78bd45bceb0c8`
- Titel: (binary or unreadable)

## include/access_model/access_coordinator.h
- Kategorie: modules_and_api
- Hash: `c54a9132104d28ec`
- Titel: (binary or unreadable)

## include/access_model/access_metrics.h
- Kategorie: modules_and_api
- Hash: `1826165720ef8612`
- Titel: (binary or unreadable)

## include/access_model/access_model_logging.h
- Kategorie: modules_and_api
- Hash: `bb64851d41e1ce5e`
- Titel: (binary or unreadable)

## include/access_model/access_model_trace.h
- Kategorie: modules_and_api
- Hash: `2ea837b9839f0ba0`
- Titel: (binary or unreadable)

## include/access_model/access_tier_interface.h
- Kategorie: modules_and_api
- Hash: `b9926ccd1179b1ce`
- Titel: (binary or unreadable)

## include/access_model/age_based_policy.h
- Kategorie: modules_and_api
- Hash: `a1b18a500a9c3afe`
- Titel: (binary or unreadable)

## include/access_model/promotion_demotion.h
- Kategorie: modules_and_api
- Hash: `8e9e412e779eac35`
- Titel: (binary or unreadable)

## include/ai/ai_plugin_generator.h
- Kategorie: modules_and_api
- Hash: `46ee3aed0f3a7026`
- Titel: (binary or unreadable)

## include/ai/cai_ethics_integration.h
- Kategorie: modules_and_api
- Hash: `d78630a0551e6439`
- Titel: (binary or unreadable)

## include/analytics/aggregation.h
- Kategorie: modules_and_api
- Hash: `849a2e1ad977cd5e`
- Titel: (binary or unreadable)

## include/analytics/analytics_api_contract.h
- Kategorie: modules_and_api
- Hash: `c482d11eba0cc2ac`
- Titel: (binary or unreadable)

## include/analytics/analytics_engine.h
- Kategorie: modules_and_api
- Hash: `6b9945c33e50987a`
- Titel: (binary or unreadable)

## include/analytics/analytics_export.h
- Kategorie: modules_and_api
- Hash: `06195b731a43f68b`
- Titel: (binary or unreadable)

## include/analytics/anomaly_detection.h
- Kategorie: modules_and_api
- Hash: `df30ab9f72cfe3f2`
- Titel: (binary or unreadable)

## include/analytics/arrow_export.h
- Kategorie: modules_and_api
- Hash: `6388988c127d80c9`
- Titel: (binary or unreadable)

## include/analytics/arrow_flight.h
- Kategorie: modules_and_api
- Hash: `3aca4b0d04e35ce6`
- Titel: (binary or unreadable)

## include/analytics/automl.h
- Kategorie: modules_and_api
- Hash: `b91ddce2abeb7b0e`
- Titel: (binary or unreadable)

## include/analytics/cep_engine.h
- Kategorie: modules_and_api
- Hash: `f6bd2b0a2e239e4d`
- Titel: (binary or unreadable)

## include/analytics/columnar_execution.h
- Kategorie: modules_and_api
- Hash: `03fde147aeac6c39`
- Titel: (binary or unreadable)

## include/analytics/connection_guard.h
- Kategorie: modules_and_api
- Hash: `08cec65cc24f0d3f`
- Titel: (binary or unreadable)

## include/analytics/detail/lru_cache.h
- Kategorie: modules_and_api
- Hash: `fa20c9fdc1cb6a30`
- Titel: (binary or unreadable)

## include/analytics/detail/memory_pool.h
- Kategorie: modules_and_api
- Hash: `a337162b10884d52`
- Titel: (binary or unreadable)

## include/analytics/detail/ring_buffer.h
- Kategorie: modules_and_api
- Hash: `23447572ceeabdba`
- Titel: (binary or unreadable)

## include/analytics/detail/stats.h
- Kategorie: modules_and_api
- Hash: `61120425903874a6`
- Titel: (binary or unreadable)

## include/analytics/diff_engine.h
- Kategorie: modules_and_api
- Hash: `87ac07d7fb7425a6`
- Titel: (binary or unreadable)

## include/analytics/distributed_analytics.h
- Kategorie: modules_and_api
- Hash: `90e6778622f259ba`
- Titel: (binary or unreadable)

## include/analytics/expert_system_engine.h
- Kategorie: modules_and_api
- Hash: `78121c564dd71def`
- Titel: (binary or unreadable)

## include/analytics/forecasting.h
- Kategorie: modules_and_api
- Hash: `839be65d1e032360`
- Titel: (binary or unreadable)

## include/analytics/incremental_view.h
- Kategorie: modules_and_api
- Hash: `d2a9d078af8a1270`
- Titel: (binary or unreadable)

## include/analytics/jit_aggregation.h
- Kategorie: modules_and_api
- Hash: `44e0ab6c6374258a`
- Titel: (binary or unreadable)

## include/analytics/knowledge_base.h
- Kategorie: modules_and_api
- Hash: `3175752c4f7a0b6e`
- Titel: (binary or unreadable)

## include/analytics/llm_process_analyzer.h
- Kategorie: modules_and_api
- Hash: `ec8e1e44fadb2ee0`
- Titel: (binary or unreadable)

## include/analytics/lora_pattern_classifier.h
- Kategorie: modules_and_api
- Hash: `b745144de43a1310`
- Titel: (binary or unreadable)

## include/analytics/ml_serving.h
- Kategorie: modules_and_api
- Hash: `9c3f5abc96a99b08`
- Titel: (binary or unreadable)

## include/analytics/model_serving.h
- Kategorie: modules_and_api
- Hash: `579b76e7ce760072`
- Titel: (binary or unreadable)

## include/analytics/nlp_text_analyzer.h
- Kategorie: modules_and_api
- Hash: `7cf645626f75630c`
- Titel: (binary or unreadable)

## include/analytics/olap.h
- Kategorie: modules_and_api
- Hash: `73b84887e37722c1`
- Titel: (binary or unreadable)

## include/analytics/process_mining.h
- Kategorie: modules_and_api
- Hash: `eaa1f7ecbcb09b3e`
- Titel: (binary or unreadable)

## include/analytics/process_pattern_matcher.h
- Kategorie: modules_and_api
- Hash: `6f1474086e2a0fde`
- Titel: (binary or unreadable)

## include/analytics/result_aggregator.h
- Kategorie: modules_and_api
- Hash: `c41b72f169c6fc5c`
- Titel: (binary or unreadable)

## include/analytics/streaming_join.h
- Kategorie: modules_and_api
- Hash: `86f5d96c701010d4`
- Titel: (binary or unreadable)
