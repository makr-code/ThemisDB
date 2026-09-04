/**
 * @file tensor_core_bridge.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=7; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=0, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// PERMANENT FALLBACK NOTE:
// Purpose: TensorCoreStorageBridge defaults to InMemoryTensorBackend when no
//   RocksDB-backed backend is injected.  Data is not persisted across restarts.
// Activation: Any call site that constructs TensorCoreStorageBridge without a
//   backend argument and without THEMIS_HAS_ROCKSDB_TENSOR defined, OR when
//   setDefaultBackendFactory() has not been called by the production bootstrap.
// Production Delta: Data loss on process restart; no compaction or versioning.
// This is the PERMANENT FALLBACK for no-RocksDB builds and for unit tests that
//   don't set a factory.  For production persistence build with
//   -DTHEMIS_HAS_ROCKSDB_TENSOR=ON (Wave-2 guard) or call
//   setDefaultBackendFactory() with a RocksDBTensorBackend factory from the
//   production bootstrap (e.g. server/themis_server.cpp).

#include "tensor/tensor_core_bridge.h"
#include "storage/tensor_network_storage_engine.h"
#include "utils/error_registry.h"
#include "utils/expected.h"
#include <algorithm>
#include <mutex>
#include <stdexcept>
#include <string>

// ── Wave-2: RocksDB tensor backend auto-registration (THEMIS_HAS_ROCKSDB_TENSOR)
// When -DTHEMIS_HAS_ROCKSDB_TENSOR=ON is set, autoRegisterRocksDBBackend() creates
// a RocksDBWrapper at the supplied path and registers a RocksDBTensorBackend factory
// so that TensorCoreStorageBridge uses durable storage automatically.
#ifdef THEMIS_HAS_ROCKSDB_TENSOR
#  include "storage/rocksdb_wrapper.h"
#endif

namespace themis {
namespace tensor {

// ─────────────────────────────────────────────────────────────────────────────
// STUB #269 — default backend factory bridge
// ─────────────────────────────────────────────────────────────────────────────

namespace {
std::mutex& backendFactoryMutex() { static std::mutex m; return m; }
TensorCoreStorageBridge::BackendFactory& backendFactoryStorage() {
    static TensorCoreStorageBridge::BackendFactory fn;
    return fn;
}
} // namespace

/*static*/
void TensorCoreStorageBridge::setDefaultBackendFactory(BackendFactory fn) {
    std::lock_guard<std::mutex> lk(backendFactoryMutex());
    backendFactoryStorage() = std::move(fn);
}

/*static*/
void TensorCoreStorageBridge::clearDefaultBackendFactory() {
    std::lock_guard<std::mutex> lk(backendFactoryMutex());
    backendFactoryStorage() = {};
}

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

TensorCoreStorageBridge::TensorCoreStorageBridge(
    std::shared_ptr<storage::ITensorStorageBackend> backend)
    : backend_(std::move(backend))
{
    if (!backend_) {
        // STUB #269: try the process-wide factory first (injected by production
        // bootstrap to provide a RocksDBTensorBackend); fall back to
        // InMemoryTensorBackend for tests that don't set a factory.
        BackendFactory factory_copy;
        {
            std::lock_guard<std::mutex> lk(backendFactoryMutex());
            factory_copy = backendFactoryStorage();
        }
        if (factory_copy) {
            backend_ = factory_copy();
        }
        if (!backend_) {
            backend_ = std::make_shared<storage::InMemoryTensorBackend>();
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Static helpers
// ─────────────────────────────────────────────────────────────────────────────

void TensorCoreStorageBridge::validateTenantId(const std::string& tenant_id) {
    if (tenant_id.empty()) {
        throw std::invalid_argument(
            "TensorCoreStorageBridge: tenant_id is empty");
    }
    if (tenant_id.find('/') != std::string::npos) {
        throw std::invalid_argument(
            "TensorCoreStorageBridge: tenant_id contains '/' — not allowed");
    }
    // Use std::any_of to scan all bytes including embedded nulls, avoiding any
    // ambiguity with C-string null-terminator semantics.
    if (std::any_of(tenant_id.begin(), tenant_id.end(),
                    [](unsigned char c) { return c == '\0'; })) {
        throw std::invalid_argument(
            "TensorCoreStorageBridge: tenant_id contains '\\0' — not allowed");
    }
}

std::string TensorCoreStorageBridge::makeKey(const std::string& tenant_id,
                                            const std::string& source_file_id,
                                            const std::string& chunk_id) {
    validateTenantId(tenant_id);
    if (chunk_id.empty()) {
        throw std::invalid_argument(
            "TensorCoreStorageBridge: chunk_id is empty");
    }
    // Build: __ttcore__:<tenant>:<file_id>:<chunk_id>
    // source_file_id may be empty (e.g. in-memory documents).
    return "__ttcore__:" + tenant_id + ":" + source_file_id + ":" + chunk_id;
}

// ─────────────────────────────────────────────────────────────────────────────
// ITensorCoreBridge::write
// ─────────────────────────────────────────────────────────────────────────────

Result<void> TensorCoreStorageBridge::write(
    const ingestion::TensorCoreRecord& record,
    const std::string&                 tenant_id)
{
    // --- Input validation ---------------------------------------------------
    if (tenant_id.empty()) {
        return ErrVoid(errors::ErrorCode::ERR_DOC_INVALID_ARGUMENT,
                   "TensorCoreStorageBridge::write: tenant_id is empty");
    }
    if (tenant_id.find('/') != std::string::npos ||
        std::any_of(tenant_id.begin(), tenant_id.end(),
                    [](unsigned char c) { return c == '\0'; })) {
        return ErrVoid(errors::ErrorCode::ERR_DOC_INVALID_ARGUMENT,
                   "TensorCoreStorageBridge::write: tenant_id contains "
                   "illegal characters");
    }
    if (record.chunk_id.empty()) {
        return ErrVoid(errors::ErrorCode::ERR_DOC_INVALID_ARGUMENT,
                   "TensorCoreStorageBridge::write: chunk_id is empty");
    }
    if (record.serialized_train.empty()) {
        return ErrVoid(errors::ErrorCode::ERR_DOC_INVALID_ARGUMENT,
                   "TensorCoreStorageBridge::write: serialized_train is empty");
    }

    // --- Build key and persist ----------------------------------------------
    std::string key = {};
    try {
        key = makeKey(tenant_id, record.source_file_id, record.chunk_id);
    } catch (const std::invalid_argument& e) {
        return ErrVoid(errors::ErrorCode::ERR_DOC_INVALID_ARGUMENT,
                   std::string("TensorCoreStorageBridge::write: ") + e.what());
    }

    const bool ok = backend_->put(key, record.serialized_train);
    if (!ok) {
        return ErrVoid(
            errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
            "TensorCoreStorageBridge::write: backend put() failed for key=" + key);
    }

    write_count_.fetch_add(1, std::memory_order_relaxed);
    return {};
}

// ─────────────────────────────────────────────────────────────────────────────
// Diagnostics
// ─────────────────────────────────────────────────────────────────────────────

std::size_t TensorCoreStorageBridge::writeCount() const {
    return write_count_.load(std::memory_order_relaxed);
}

std::optional<std::vector<uint8_t>>
TensorCoreStorageBridge::getRaw(const std::string& tenant_id,
                               const std::string& source_file_id,
                               const std::string& chunk_id) const {
    std::string key = {};
    try {
        key = makeKey(tenant_id, source_file_id, chunk_id);
    } catch (...) {
        return std::nullopt;
    }
    return backend_->get(key);
}

#ifdef THEMIS_HAS_ROCKSDB_TENSOR
/**
 * @brief Auto-register a RocksDBTensorBackend as the default factory (Wave-2).
 *
 * Creates a `RocksDBWrapper` at @p db_path with a minimal configuration suitable
 * for tensor blob storage and registers a factory via `setDefaultBackendFactory()`.
 * Any subsequent construction of `TensorCoreStorageBridge` without an explicit
 * backend will use this durable RocksDB-backed store.
 *
 * This method is idempotent: calling it again replaces the previously registered
 * factory.  Call it from the production bootstrap (e.g. `server/themis_server.cpp`)
 * before the first tensor write.
 *
 * @param db_path  Directory path for the RocksDB database.  Created if absent.
 * @throws std::runtime_error if RocksDB cannot be opened at @p db_path.
 */
/*static*/
void TensorCoreStorageBridge::autoRegisterRocksDBBackend(const std::string& db_path) {
    if (db_path.empty())
        throw std::invalid_argument(
            "TensorCoreStorageBridge::autoRegisterRocksDBBackend: db_path is empty");

    // Build a minimal RocksDB configuration for tensor blobs.
    ::themis::RocksDBWrapper::Config cfg;
    cfg.db_path                 = db_path;
    cfg.create_if_missing       = true;
    cfg.db_write_buffer_size_mb = 64; // 64 MB total write buffer

    // Share the RocksDB wrapper across all tensors opened from this path.
    auto db = std::make_shared<::themis::RocksDBWrapper>(cfg);

    setDefaultBackendFactory([db]() -> std::shared_ptr<storage::ITensorStorageBackend> {
        return std::make_shared<::themis::storage::RocksDBTensorBackend>(db);
    });
}
#endif // THEMIS_HAS_ROCKSDB_TENSOR

} // namespace tensor
} // namespace themis

