/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tensor/adapter_repository.cpp                      ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-06                                         ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🟡 EXPERIMENTAL — Phase 3 (Q2 2027)                         ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file tensor/adapter_repository.cpp
 * @brief AdapterRepository implementation (Adapter Sovereignty).
 *
 * ### Stub log
 * - AR-01  loadAdapter() copies TT-core data from the backend into a heap
 *          allocation instead of using mmap(MAP_SHARED) for zero-copy.
 *          See STUB #172 in STUB_INVENTORY.md.
 *
 * STUB/SIMULATION NOTE (AR-01):
 * Purpose: Provide a fully functional adapter store/load cycle so that
 *          upstream callers (e.g. GgmlTensorBridge) can integrate with
 *          AdapterRepository before the mmap path is ready.
 * Activation: Always — no compile-time flag required.
 * Production Delta: GgmlCoreDescriptor::train.cores[k].data is a heap copy,
 *                   not a pointer into a mmap'd page.  This means one extra
 *                   memcpy per loadAdapter() call and no mlock() protection.
 *                   The adapter switch latency is O(totalParams) CPU copy vs.
 *                   the ≤ 5 ms page-pin target.
 * Removal Plan: Q1 2027 — replace backend->get() + deserialize() with
 *               mmap(MAP_SHARED) on the RocksDB SST backing file + mlock()
 *               to pin the pages; return raw float* into the mmap region.
 */

#include "tensor/adapter_repository.h"

#include <algorithm>
#include <shared_mutex>
#include <sstream>
#include <stdexcept>

namespace themis {
namespace tensor {

// ============================================================================
// Constructor
// ============================================================================

AdapterRepository::AdapterRepository(
    std::shared_ptr<storage::ITensorStorageBackend> backend,
    std::string                                     tenant_id)
    : backend_(std::move(backend))
    , tenant_id_(std::move(tenant_id))
{
    if (!backend_) {
        throw std::invalid_argument(
            "AdapterRepository: backend must not be null");
    }
}

// ============================================================================
// makeKey()
// ============================================================================

std::string AdapterRepository::makeKey(const std::string& domain,
                                        const std::string& base_model_id) const {
    return "__adapters__:" + tenant_id_ + ":" + domain + ":" + base_model_id;
}

// ============================================================================
// store()
// ============================================================================

bool AdapterRepository::store(const std::string&      domain,
                               const std::string&      base_model_id,
                               const storage::TTTrain& adapter_train,
                               const AdapterMetadata&  /*meta*/) {
    if (domain.empty() || base_model_id.empty()) {
        return false;
    }

    // Serialise the TTTrain to bytes.
    const std::vector<uint8_t> bytes = adapter_train.serialize();
    if (bytes.empty()) {
        return false;
    }

    const std::string key = makeKey(domain, base_model_id);
    const bool ok = backend_->put(key, bytes);

    if (ok) {
        std::unique_lock lock(stats_mutex_);
        ++stats_.total_adapters;
        stats_.total_param_bytes += adapter_train.totalParams() * sizeof(float);
    }
    return ok;
}

// ============================================================================
// remove()
// ============================================================================

bool AdapterRepository::remove(const std::string& domain,
                                const std::string& base_model_id) {
    const std::string key = makeKey(domain, base_model_id);
    const bool existed = backend_->get(key).has_value();
    if (!existed) return false;

    const bool ok = backend_->del(key);
    if (ok) {
        std::unique_lock lock(stats_mutex_);
        if (stats_.total_adapters > 0) --stats_.total_adapters;
    }
    return ok;
}

// ============================================================================
// loadAdapter()
// ============================================================================

GgmlCoreDescriptor
AdapterRepository::loadAdapter(const std::string& domain,
                                const std::string& base_model_id) const {
    GgmlCoreDescriptor desc;
    desc.tenant_id      = tenant_id_;
    desc.domain         = domain;
    desc.base_model_id  = base_model_id;
    desc.adapter_key    = makeKey(domain, base_model_id);

    // STUB/SIMULATION NOTE (AR-01):
    // Retrieves bytes from the backend and deserialises into desc.train.
    // Production path: mmap() + mlock() for zero-copy page-pinning.
    const auto raw = backend_->get(desc.adapter_key);
    if (!raw.has_value() || raw->empty()) {
        std::unique_lock lock(stats_mutex_);
        ++stats_.load_misses;
        return desc;  // valid = false
    }

    auto train_opt = storage::TTTrain::deserialize(*raw);
    if (!train_opt.has_value()) {
        std::unique_lock lock(stats_mutex_);
        ++stats_.load_misses;
        return desc;
    }

    desc.train = std::move(*train_opt);
    desc.valid = true;

    {
        std::unique_lock lock(stats_mutex_);
        ++stats_.load_hits;
    }
    return desc;
}

// ============================================================================
// listDomains()
// ============================================================================

std::vector<std::string> AdapterRepository::listDomains() const {
    const std::string prefix = "__adapters__:" + tenant_id_ + ":";
    const auto keys = backend_->listKeys(prefix);

    std::vector<std::string> domains;
    domains.reserve(keys.size());

    for (const auto& key : keys) {
        // Key format: __adapters__:<tenant>:<domain>:<base_model_id>
        // Strip the prefix and extract the domain part.
        if (key.size() <= prefix.size()) continue;
        const std::string rest = key.substr(prefix.size());
        const auto sep = rest.find(':');
        if (sep == std::string::npos) continue;
        domains.push_back(rest.substr(0, sep));
    }

    // Sort and deduplicate.
    std::sort(domains.begin(), domains.end());
    domains.erase(std::unique(domains.begin(), domains.end()), domains.end());
    return domains;
}

// ============================================================================
// listAdapters()
// ============================================================================

std::vector<std::pair<std::string, std::string>>
AdapterRepository::listAdapters() const {
    const std::string prefix = "__adapters__:" + tenant_id_ + ":";
    const auto keys = backend_->listKeys(prefix);

    std::vector<std::pair<std::string, std::string>> result;
    result.reserve(keys.size());

    for (const auto& key : keys) {
        if (key.size() <= prefix.size()) continue;
        const std::string rest = key.substr(prefix.size());
        const auto sep = rest.find(':');
        if (sep == std::string::npos || sep + 1 >= rest.size()) continue;
        result.emplace_back(rest.substr(0, sep), rest.substr(sep + 1));
    }

    std::sort(result.begin(), result.end());
    return result;
}

// ============================================================================
// stats()
// ============================================================================

AdapterRepository::RepositoryStats
AdapterRepository::stats() const noexcept {
    std::shared_lock lock(stats_mutex_);
    return stats_;
}

} // namespace tensor
} // namespace themis
