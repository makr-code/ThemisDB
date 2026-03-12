### Context

This issue implements the roadmap item '`LLMDeploymentPlugin`: RocksDB Model Storage' for the llm domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: `LLMDeploymentPlugin`: RocksDB Model Storage

### Goal

Deliver the scoped changes for `LLMDeploymentPlugin`: RocksDB Model Storage in src/llm/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### `LLMDeploymentPlugin`: RocksDB Model Storage
**Priority:** Medium
**Target Version:** v1.8.0

`llm_deployment_plugin.cpp` line 273 has: "Store in BaseEntity storage (RocksDB) - TODO: Uncomment when `llm_model_storage.cpp` exists". The plugin currently operates in "Filesystem-only mode" (line 136). Model metadata is not persisted to the database, breaking admin query ("list all deployed models") across restarts.

**Implementation Notes:**
- `[ ]` Implement `llm_model_storage.cpp` providing `LLMModelStorage::save(model_id, metadata)` and `load(model_id)` using the existing `StorageEngine` API with key prefix `llm_model::`.
- `[ ]` Uncomment the RocksDB persistence block at line 273; inject `StorageEngine*` into `LLMDeploymentPlugin` constructor.
- `[ ]` Implement `TODO(enhancement)` at line 916: check `model_id` existence before deployment to surface clear errors for unknown model IDs.
- `[ ]` Implement `TODO(feature)` at line 175: propagate authenticated user context from the request JWT into `audit.user` instead of hardcoding `"system"`.

---

### Acceptance Criteria

- [ ] Implement `llm_model_storage.cpp` providing `LLMModelStorage::save(model_id, metadata)` and `load(model_id)` using the existing `StorageEngine` API with key prefix `llm_model::`.
- [ ] Uncomment the RocksDB persistence block at line 273; inject `StorageEngine*` into `LLMDeploymentPlugin` constructor.
- [ ] Implement `TODO(enhancement)` at line 916: check `model_id` existence before deployment to surface clear errors for unknown model IDs.
- [ ] Implement `TODO(feature)` at line 175: propagate authenticated user context from the request JWT into `audit.user` instead of hardcoding `"system"`.

### Relationships

- Roadmap row: #180 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/llm/FUTURE_ENHANCEMENTS.md#llmdeploymentplugin-rocksdb-model-storage
- Source key: roadmap:180:llm:v1.8.0:llmdeploymentplugin-rocksdb-model-storage

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:180:llm:v1.8.0:llmdeploymentplugin-rocksdb-model-storage -->
<!-- roadmap-ref: row=180;module=llm;target=v1.8.0 -->
<!-- roadmap-detail: src/llm/FUTURE_ENHANCEMENTS.md#llmdeploymentplugin-rocksdb-model-storage -->
