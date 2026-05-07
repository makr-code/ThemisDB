/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tensor_core_bridge.h                                 ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-05                                         ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready (RocksDB backend: STUB #160)             ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file tensor_core_bridge.h
 * @brief Concrete `ITensorCoreBridge` backed by `ITensorStorageBackend`.
 *
 * `TensorCoreStorageBridge` is the production counterpart of
 * `InMemoryTensorCoreBridge`.  It persists each `TensorCoreRecord` as raw bytes
 * in a `ITensorStorageBackend` (either `InMemoryTensorBackend` for tests or a
 * RocksDB-backed backend for production).
 *
 * ### Key schema
 * ```
 * __ttcore__:<tenant>:<source_file_id>:<chunk_id>   → serialized_train bytes
 * ```
 * The `__ttcore__` prefix separates chunk-level TT-cores from the
 * model/field-level `__ttn__` keys used by `TensorNetworkStorageEngine`.
 *
 * ### Thread safety
 * Delegated to the injected `ITensorStorageBackend`; `write()` may be called
 * concurrently from multiple sink step instances.
 *
 * ### Dependency rule
 * This header lives in `tensor/` and imports:
 *  - `ingestion/ingestion_sinks.h` → `ITensorCoreBridge` (abstract interface)
 *  - `storage/tensor_network_storage_engine.h` → `ITensorStorageBackend`
 *
 * `ingestion/` headers MUST NOT import this header (SoC boundary).  Wiring
 * happens in server bootstrap / `main.cpp`.
 */

#pragma once

#include "ingestion/ingestion_sinks.h"
#include "storage/tensor_network_storage_engine.h"
#include <memory>
#include <atomic>
#include <string>

namespace themis {
namespace tensor {

/**
 * @brief Production `ITensorCoreBridge` backed by `ITensorStorageBackend`.
 *
 * Stores each `TensorCoreRecord::serialized_train` byte-vector under the key:
 * `__ttcore__:<tenant>:<source_file_id>:<chunk_id>`
 *
 * Re-ingesting the same `tenant` + `chunk_id` overwrites the existing entry
 * (upsert semantics via backend `put()`).
 *
 * ### STUB NOTE (STUB_INVENTORY #160)
 * The production path uses `InMemoryTensorBackend` as the default when no
 * RocksDB-backed backend is injected.  Full RocksDB wiring is planned for
 * Q4 2026 (see `FUTURE_ENHANCEMENTS.md` Phase 10).
 *
 * @see `include/storage/tensor_network_storage_engine.h` for `ITensorStorageBackend`
 * @see `include/ingestion/ingestion_sinks.h` for `ITensorCoreBridge`
 */
class TensorCoreStorageBridge : public ingestion::ITensorCoreBridge {
public:
    // ─── Construction ─────────────────────────────────────────────────────────

    /**
     * @brief Construct with an injectable storage backend.
     *
     * @param backend  KV-store backend.  Uses `InMemoryTensorBackend` when nullptr.
     * @throws std::invalid_argument if backend is explicitly constructed but null.
     */
    explicit TensorCoreStorageBridge(
        std::shared_ptr<storage::ITensorStorageBackend> backend = nullptr);

    // ─── ITensorCoreBridge ──────────────────────────────────────────────────────

    /**
     * @brief Persist one `TensorCoreRecord`.
     *
     * The `serialized_train` bytes are stored under the structured key:
     * `__ttcore__:<tenant_id>:<source_file_id>:<chunk_id>`
     *
     * @param record     Pre-computed TT-core record.
     * @param tenant_id  Tenant scope; must be non-empty and free of '/' and '\\0'.
     * @return Error on invalid input or backend write failure.
     */
    ingestion::Result<void> write(const ingestion::TensorCoreRecord& record,
                                  const std::string& tenant_id) override;

    std::size_t writeCount() const override;

    // ─── Diagnostics ──────────────────────────────────────────────────────────

    /// Backend used for storage (for inspection in tests).
    const storage::ITensorStorageBackend& backend() const { return *backend_; }

    /// Retrieve raw bytes for a given tenant + chunk_id (returns empty when absent).
    std::optional<std::vector<uint8_t>> getRaw(const std::string& tenant_id,
                                                const std::string& source_file_id,
                                                const std::string& chunk_id) const;

    // ─── Static helpers ───────────────────────────────────────────────────────

    /**
     * @brief Build the storage key for a TT-core record.
     *
     * Format: `__ttcore__:<tenant_id>:<source_file_id>:<chunk_id>`
     *
     * @throws std::invalid_argument when any argument is empty or contains '/'.
     */
    static std::string makeKey(const std::string& tenant_id,
                               const std::string& source_file_id,
                               const std::string& chunk_id);

private:
    std::shared_ptr<storage::ITensorStorageBackend> backend_;
    std::atomic<std::size_t>                        write_count_{0};

    static void validateTenantId(const std::string& tenant_id);
};

} // namespace tensor
} // namespace themis
