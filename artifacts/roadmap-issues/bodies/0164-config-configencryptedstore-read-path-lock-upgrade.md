### Context

This issue implements the roadmap item '`ConfigEncryptedStore` Read-Path Lock Upgrade' for the config domain. It is sourced from the consolidated roadmap under 🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0) and targets milestone v1.8.0.

Primary detail section: `ConfigEncryptedStore` Read-Path Lock Upgrade

### Goal

Deliver the scoped changes for `ConfigEncryptedStore` Read-Path Lock Upgrade in src/config/ and complete the linked detail section in a release-ready state for v1.8.0.

### Detailed Scope

### `ConfigEncryptedStore` Read-Path Lock Upgrade
**Priority:** Medium
**Target Version:** v1.8.0

`config_encrypted_store.cpp` uses `std::lock_guard<std::mutex>` (exclusive) for both read (`get`, `list`, `contains`, `size`) and write (`set`, `remove`, `rotate_key`) operations. All read calls serialize with each other unnecessarily.

**Implementation Notes:**
- `[ ]` Replace `std::mutex mutex_` with `std::shared_mutex` in `ConfigEncryptedStore`; upgrade `get`, `list`, `contains`, `size` to `std::shared_lock`.
- `[ ]` Keep `set`, `remove`, `rotate_key`, and `re_encrypt_all_locked()` on `std::unique_lock`.
- `[ ]` Re-encryption (line 192, "perform full re-encrypt before swapping") requires `unique_lock` for its full duration to maintain atomicity — do not split it.

---

### Acceptance Criteria

- [ ] Replace `std::mutex mutex_` with `std::shared_mutex` in `ConfigEncryptedStore`; upgrade `get`, `list`, `contains`, `size` to `std::shared_lock`.
- [ ] Keep `set`, `remove`, `rotate_key`, and `re_encrypt_all_locked()` on `std::unique_lock`.
- [ ] Re-encryption (line 192, "perform full re-encrypt before swapping") requires `unique_lock` for its full duration to maintain atomicity — do not split it.

### Relationships

- Roadmap row: #164 (🟡 Medium Priority — Near-term (v1.5.0 – v1.8.0))
- Depends on: none identified during generation.
- Part of: consolidated roadmap delivery tracking.

### References

- src/ROADMAP.md
- src/config/FUTURE_ENHANCEMENTS.md#configencryptedstore-read-path-lock-upgrade
- Source key: roadmap:164:config:v1.8.0:configencryptedstore-read-path-lock-upgrade

Generated from the consolidated source roadmap. Keep the roadmap and issue in sync when scope changes.

<!-- roadmap-source-key: roadmap:164:config:v1.8.0:configencryptedstore-read-path-lock-upgrade -->
<!-- roadmap-ref: row=164;module=config;target=v1.8.0 -->
<!-- roadmap-detail: src/config/FUTURE_ENHANCEMENTS.md#configencryptedstore-read-path-lock-upgrade -->
