/**
 * @file tensor_core_bridge.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=7; TODO=1, Stub=5, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
    Result<void> write(const ingestion::TensorCoreRecord& record,
               const std::string& tenant_id) override;

    std::size_t writeCount() const override;

    // ─── Diagnostics ──────────────────────────────────────────────────────────

    /// Backend used for storage (for inspection in tests).
    const storage::ITensorStorageBackend& backend() const { return *backend_; }

    /// Retrieve raw bytes for a given tenant + chunk_id (returns empty when absent).
    std::optional<std::vector<uint8_t>> getRaw(const std::string& tenant_id,
                                                const std::string& source_file_id,
                                                const std::string& chunk_id) const;

/**
     * @brief Injectable factory for the default storage backend (STUB #269).
     *
     * Signature: `std::shared_ptr<ITensorStorageBackend> factory()`.
     *
     * When set, the factory is called during `TensorCoreStorageBridge`
     * construction whenever no explicit backend is provided.  This allows
     * the production bootstrap to inject a `RocksDBTensorBackend` without
     * every call site needing to know the concrete type.
     *
     * Typical production bootstrap (main_server.cpp):
     * ```cpp
     * TensorCoreStorageBridge::setDefaultBackendFactory([&] {
     *     return std::make_shared<RocksDBTensorBackend>(db_handle);
     * });
     * ```
     *
     * Tests that need an in-memory backend simply omit this call (factory
     * is null → constructor falls back to `InMemoryTensorBackend`).
     */
    using BackendFactory =
        std::function<std::shared_ptr<storage::ITensorStorageBackend>()>;

    /**
     * @brief Set the process-wide default backend factory (STUB #269 bridge).
     *
     * Thread-safe.  Replaces any previously set factory.
     */
    static void setDefaultBackendFactory(BackendFactory fn);

    /**
     * @brief Clear the default backend factory (STUB #269 bridge).
     *
     * After this call the constructor falls back to `InMemoryTensorBackend`.
     */
    static void clearDefaultBackendFactory();

#ifdef THEMIS_HAS_ROCKSDB_TENSOR
    /**
     * @brief Auto-register a RocksDBTensorBackend as the default factory (Wave-2).
     *
     * Creates a `RocksDBWrapper` at @p db_path and registers a durable backend
     * factory via `setDefaultBackendFactory()`.  Call from the production bootstrap
     * before the first tensor write.  Idempotent.
     *
     * @param db_path  Directory path for the RocksDB database.  Created if absent.
     * @throws std::invalid_argument if @p db_path is empty.
     * @throws std::runtime_error    if RocksDB cannot be opened at @p db_path.
     */
    static void autoRegisterRocksDBBackend(const std::string& db_path);
#endif // THEMIS_HAS_ROCKSDB_TENSOR

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

