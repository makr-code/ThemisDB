/*
 * ThemisDB | File: tensor_core_bridge.cpp | Version: 1.0.0 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 87/100 | Lines: 187
 * Gap Summary: total=7; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=1, Debt=0, C=1, H=0, M=1, L=0
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

// STUB/SIMULATION NOTE:
// Purpose: TensorCoreStorageBridge defaults to InMemoryTensorBackend when no
//   RocksDB-backed backend is injected.  Data is not persisted across restarts.
// Activation: Any call site that constructs TensorCoreStorageBridge without a
//   backend argument or with an InMemoryTensorBackend instance.
// Production Delta: Data loss on process restart; no compaction or versioning.
// Removal Plan: When RocksDBTensorBackend is implemented (Q4 2026), the
//   production bootstrap should inject it; InMemoryTensorBackend remains for tests.

#include "tensor/tensor_core_bridge.h"
#include "storage/tensor_network_storage_engine.h"
#include "utils/error_registry.h"
#include "utils/expected.h"
#include <algorithm>
#include <mutex>
#include <stdexcept>
#include <string>

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
    std::string key;
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
    std::string key;
    try {
        key = makeKey(tenant_id, source_file_id, chunk_id);
    } catch (...) {
        return std::nullopt;
    }
    return backend_->get(key);
}

} // namespace tensor
} // namespace themis
