/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tensor_core_sink.cpp                               ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-05                                         ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready (RocksDB backend: STUB #160)             ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// STUB/SIMULATION NOTE:
// Purpose: TensorCoreStorageSink defaults to InMemoryTensorBackend when no
//   RocksDB-backed backend is injected.  Data is not persisted across restarts.
// Activation: Any call site that constructs TensorCoreStorageSink without a
//   backend argument or with an InMemoryTensorBackend instance.
// Production Delta: Data loss on process restart; no compaction or versioning.
// Removal Plan: When RocksDBTensorBackend is implemented (Q4 2026), the
//   production bootstrap should inject it; InMemoryTensorBackend remains for tests.

#include "tensor/tensor_core_sink.h"
#include "storage/tensor_network_storage_engine.h"
#include "utils/error_registry.h"
#include "utils/expected.h"
#include <algorithm>
#include <stdexcept>
#include <string>

namespace themis {
namespace tensor {

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

TensorCoreStorageSink::TensorCoreStorageSink(
    std::shared_ptr<storage::ITensorStorageBackend> backend)
    : backend_(std::move(backend))
{
    if (!backend_) {
        // Default: in-memory backend (non-persistent, suitable for tests).
        backend_ = std::make_shared<storage::InMemoryTensorBackend>();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Static helpers
// ─────────────────────────────────────────────────────────────────────────────

void TensorCoreStorageSink::validateTenantId(const std::string& tenant_id) {
    if (tenant_id.empty()) {
        throw std::invalid_argument(
            "TensorCoreStorageSink: tenant_id is empty");
    }
    if (tenant_id.find('/') != std::string::npos) {
        throw std::invalid_argument(
            "TensorCoreStorageSink: tenant_id contains '/' — not allowed");
    }
    // Use std::any_of to scan all bytes including embedded nulls, avoiding any
    // ambiguity with C-string null-terminator semantics.
    if (std::any_of(tenant_id.begin(), tenant_id.end(),
                    [](unsigned char c) { return c == '\0'; })) {
        throw std::invalid_argument(
            "TensorCoreStorageSink: tenant_id contains '\\0' — not allowed");
    }
}

std::string TensorCoreStorageSink::makeKey(const std::string& tenant_id,
                                            const std::string& source_file_id,
                                            const std::string& chunk_id) {
    validateTenantId(tenant_id);
    if (chunk_id.empty()) {
        throw std::invalid_argument(
            "TensorCoreStorageSink: chunk_id is empty");
    }
    // Build: __ttcore__:<tenant>:<file_id>:<chunk_id>
    // source_file_id may be empty (e.g. in-memory documents).
    return "__ttcore__:" + tenant_id + ":" + source_file_id + ":" + chunk_id;
}

// ─────────────────────────────────────────────────────────────────────────────
// ITensorCoreSink::write
// ─────────────────────────────────────────────────────────────────────────────

ingestion::Result<void> TensorCoreStorageSink::write(
    const ingestion::TensorCoreRecord& record,
    const std::string&                 tenant_id)
{
    // --- Input validation ---------------------------------------------------
    if (tenant_id.empty()) {
        return ingestion::ErrVoid(errors::ErrorCode::ERR_DOC_INVALID_ARGUMENT,
                       "TensorCoreStorageSink::write: tenant_id is empty");
    }
    if (tenant_id.find('/') != std::string::npos ||
        std::any_of(tenant_id.begin(), tenant_id.end(),
                    [](unsigned char c) { return c == '\0'; })) {
        return ingestion::ErrVoid(errors::ErrorCode::ERR_DOC_INVALID_ARGUMENT,
                       "TensorCoreStorageSink::write: tenant_id contains "
                       "illegal characters");
    }
    if (record.chunk_id.empty()) {
        return ingestion::ErrVoid(errors::ErrorCode::ERR_DOC_INVALID_ARGUMENT,
                       "TensorCoreStorageSink::write: chunk_id is empty");
    }
    if (record.serialized_train.empty()) {
        return ingestion::ErrVoid(errors::ErrorCode::ERR_DOC_INVALID_ARGUMENT,
                       "TensorCoreStorageSink::write: serialized_train is empty");
    }

    // --- Build key and persist ----------------------------------------------
    std::string key;
    try {
        key = makeKey(tenant_id, record.source_file_id, record.chunk_id);
    } catch (const std::invalid_argument& e) {
        return ingestion::ErrVoid(errors::ErrorCode::ERR_DOC_INVALID_ARGUMENT,
                       std::string("TensorCoreStorageSink::write: ") + e.what());
    }

    const bool ok = backend_->put(key, record.serialized_train);
    if (!ok) {
        return ingestion::ErrVoid(
            errors::ErrorCode::ERR_STORAGE_TRANSACTION_FAILED,
            "TensorCoreStorageSink::write: backend put() failed for key=" + key);
    }

    write_count_.fetch_add(1, std::memory_order_relaxed);
    return {};
}

// ─────────────────────────────────────────────────────────────────────────────
// Diagnostics
// ─────────────────────────────────────────────────────────────────────────────

std::size_t TensorCoreStorageSink::writeCount() const {
    return write_count_.load(std::memory_order_relaxed);
}

std::optional<std::vector<uint8_t>>
TensorCoreStorageSink::getRaw(const std::string& tenant_id,
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
