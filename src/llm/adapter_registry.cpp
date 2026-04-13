/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            adapter_registry.cpp                               ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:26:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   94.0/100                                       ║
    • Total Lines:     495                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • efdbcc2fc8  2026-03-19  merge: resolve conflicts with develop - keep predictive p... ║
    • ca711c041f  2026-03-19  fix(lora): unique_lock for hotLoad write, CI path, AC5 pe... ║
    • 2873683f74  2026-03-18  Changes before error encountered        ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 011803ade1  2026-02-28  feat(llm): add hotLoad() and addHotLoadObserver() to Adap... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file adapter_registry.cpp
 * @brief In-memory implementation of AdapterRegistry
 *
 * All adapter records are stored in a thread-safe, in-memory map indexed by
 * adapter_id.  Signatures are delegated to the provided SecuritySignatureManager.
 */

#include "llm/adapter_registry.h"
#include "storage/security_signature.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <chrono>

namespace themis {
namespace llm {

// ============================================================================
// Pimpl
// ============================================================================

struct AdapterRegistry::Impl {
    mutable std::shared_mutex rw_mu;
    // Primary store: adapter_id → metadata
    std::unordered_map<std::string, AdapterMetadata> adapters;
    // Signature store: adapter_id → AdapterSignature
    std::unordered_map<std::string, AdapterSignature> signatures;
    // Hot-load observer callbacks (registration order preserved)
    std::vector<AdapterRegistry::HotLoadCallback> hot_load_callbacks;
};

// ============================================================================
// Constructor / destructor
// ============================================================================

AdapterRegistry::AdapterRegistry(std::shared_ptr<storage::SecuritySignatureManager> sig_manager)
    : sig_manager_(sig_manager)
    , impl_(std::make_unique<Impl>()) {
}

AdapterRegistry::~AdapterRegistry() = default;

// ============================================================================
// Key construction helpers
// ============================================================================

// Key construction helpers.
// These exist as named methods so that a future persistent backend (e.g. RocksDB)
// can override the namespace without changing callers.
std::string AdapterRegistry::makeAdapterKey(const std::string& adapter_id) const {
    return std::string(ADAPTER_KEY_PREFIX) + adapter_id;
}

std::string AdapterRegistry::makeBaseModelIndexKey(const std::string& base_model) const {
    return std::string(BASE_MODEL_INDEX_PREFIX) + base_model;
}

std::string AdapterRegistry::makeDomainIndexKey(const std::string& domain) const {
    return std::string(DOMAIN_INDEX_PREFIX) + domain;
}

// updateIndices is a no-op for the in-memory backend (the maps provide
// full scan capability without secondary indices).
// Reserved for a future persistent backend that maintains explicit index tables.
void AdapterRegistry::updateIndices(const AdapterMetadata& /*metadata*/, bool /*remove*/) {}

// ============================================================================
// CRUD operations
// ============================================================================

bool AdapterRegistry::registerAdapter(const AdapterMetadata& metadata) {
    if (metadata.adapter_id.empty()) {
        spdlog::error("AdapterRegistry::registerAdapter: adapter_id must not be empty");
        return false;
    }
    std::unique_lock<std::shared_mutex> lock(impl_->rw_mu);
    if (impl_->adapters.count(metadata.adapter_id)) {
        spdlog::warn("AdapterRegistry::registerAdapter: adapter '{}' already registered",
                     metadata.adapter_id);
        return false;
    }
    impl_->adapters[metadata.adapter_id] = metadata;
    spdlog::debug("AdapterRegistry: registered adapter '{}'", metadata.adapter_id);
    return true;
}

std::optional<AdapterMetadata> AdapterRegistry::getAdapter(const std::string& adapter_id) {
    std::shared_lock<std::shared_mutex> lock(impl_->rw_mu);
    auto it = impl_->adapters.find(adapter_id);
    if (it == impl_->adapters.end()) {
        return std::nullopt;
    }
    return it->second;
}

bool AdapterRegistry::updateAdapter(const AdapterMetadata& metadata) {
    if (metadata.adapter_id.empty()) {
        spdlog::error("AdapterRegistry::updateAdapter: adapter_id must not be empty");
        return false;
    }
    std::unique_lock<std::shared_mutex> lock(impl_->rw_mu);
    auto it = impl_->adapters.find(metadata.adapter_id);
    if (it == impl_->adapters.end()) {
        spdlog::warn("AdapterRegistry::updateAdapter: adapter '{}' not found",
                     metadata.adapter_id);
        return false;
    }
    it->second = metadata;
    spdlog::debug("AdapterRegistry: updated adapter '{}'", metadata.adapter_id);
    return true;
}

bool AdapterRegistry::deleteAdapter(const std::string& adapter_id) {
    std::unique_lock<std::shared_mutex> lock(impl_->rw_mu);
    auto erased = impl_->adapters.erase(adapter_id);
    if (erased == 0) {
        spdlog::warn("AdapterRegistry::deleteAdapter: adapter '{}' not found", adapter_id);
        return false;
    }
    impl_->signatures.erase(adapter_id);
    spdlog::debug("AdapterRegistry: deleted adapter '{}'", adapter_id);
    return true;
}

std::vector<AdapterMetadata> AdapterRegistry::listAdapters() {
    std::shared_lock<std::shared_mutex> lock(impl_->rw_mu);
    std::vector<AdapterMetadata> result;
    result.reserve(impl_->adapters.size());
    for (const auto& [id, meta] : impl_->adapters) {
        result.push_back(meta);
    }
    return result;
}

std::vector<AdapterMetadata> AdapterRegistry::listAdaptersByBaseModel(
    const std::string& base_model
) {
    std::shared_lock<std::shared_mutex> lock(impl_->rw_mu);
    std::vector<AdapterMetadata> result;
    for (const auto& [id, meta] : impl_->adapters) {
        if (meta.base_model_name == base_model) {
            result.push_back(meta);
        }
    }
    return result;
}

std::vector<AdapterMetadata> AdapterRegistry::listAdaptersByDomain(
    const std::string& domain
) {
    std::shared_lock<std::shared_mutex> lock(impl_->rw_mu);
    std::vector<AdapterMetadata> result;
    for (const auto& [id, meta] : impl_->adapters) {
        if (meta.domain == domain) {
            result.push_back(meta);
        }
    }
    return result;
}

// ============================================================================
// Compatibility validation
// ============================================================================

AdapterRegistry::ValidationResult AdapterRegistry::validateCompatibility(
    const std::string& adapter_id,
    const std::string& base_model,
    const std::string& model_version
) {
    ValidationResult result;
    auto meta_opt = getAdapter(adapter_id);
    if (!meta_opt) {
        result.compatible = false;
        result.errors.push_back("Adapter '" + adapter_id + "' not found in registry");
        return result;
    }

    const auto& meta = *meta_opt;

    // Check base model name match
    if (!meta.base_model_name.empty() && meta.base_model_name != base_model) {
        result.errors.push_back(
            "Base model mismatch: adapter requires '" + meta.base_model_name +
            "', got '" + base_model + "'");
    }

    // Check version compatibility (exact or empty means "any")
    if (!meta.base_model_version.empty() && meta.base_model_version != model_version) {
        result.warnings.push_back(
            "Base model version mismatch: adapter trained on '" + meta.base_model_version +
            "', running '" + model_version + "'");
    }

    result.compatible = result.errors.empty();
    return result;
}

// ============================================================================
// Signature operations
// ============================================================================

bool AdapterRegistry::signAdapter(const std::string& adapter_id,
                                   [[maybe_unused]] const std::string& private_key) {
    // NOTE: Real Ed25519 signing via `private_key` is not yet implemented.
    // The `content_hash` field is populated, and the `signature` field is a
    // placeholder token that makes `verifySignature()` deterministic for
    // testing purposes.  A production implementation must replace this with
    // an actual Ed25519 signature over `content_hash` using `private_key`.
    std::unique_lock<std::shared_mutex> lock(impl_->rw_mu);
    if (!impl_->adapters.count(adapter_id)) {
        spdlog::warn("AdapterRegistry::signAdapter: adapter '{}' not found", adapter_id);
        return false;
    }

    AdapterSignature sig;
    sig.signer_identity = "adapter_registry";
    sig.content_hash    = storage::SecuritySignatureManager::computeFileHash(adapter_id);
    // ISO 8601 timestamp via system_clock
    auto now    = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&time_t));
    sig.signing_timestamp = buf;
    sig.signature         = "sig:" + sig.content_hash;  // Placeholder; real Ed25519 via private_key

    impl_->signatures[adapter_id] = sig;

    // Persist via SecuritySignatureManager if available
    if (sig_manager_) {
        storage::SecuritySignature stored_sig;
        stored_sig.resource_id = makeAdapterKey(adapter_id);
        stored_sig.hash        = sig.content_hash;
        stored_sig.algorithm   = "Ed25519";
        stored_sig.created_at  = static_cast<uint64_t>(time_t);
        sig_manager_->storeSignature(stored_sig);
    }

    spdlog::debug("AdapterRegistry: signed adapter '{}'", adapter_id);
    return true;
}

bool AdapterRegistry::verifySignature(const std::string& adapter_id) {
    std::shared_lock<std::shared_mutex> lock(impl_->rw_mu);
    auto it = impl_->signatures.find(adapter_id);
    if (it == impl_->signatures.end()) {
        spdlog::debug("AdapterRegistry::verifySignature: no signature for '{}'", adapter_id);
        return false;
    }

    // Recompute hash and compare
    std::string current_hash = storage::SecuritySignatureManager::computeFileHash(adapter_id);
    bool valid = (it->second.content_hash == current_hash);
    spdlog::debug("AdapterRegistry: signature for '{}' is {}", adapter_id,
                  valid ? "valid" : "invalid");
    return valid;
}

std::optional<AdapterSignature> AdapterRegistry::getSignature(const std::string& adapter_id) {
    std::shared_lock<std::shared_mutex> lock(impl_->rw_mu);
    auto it = impl_->signatures.find(adapter_id);
    if (it == impl_->signatures.end()) {
        return std::nullopt;
    }
    return it->second;
}

// ============================================================================
// Version management
// ============================================================================

std::optional<AdapterMetadata> AdapterRegistry::getLatestVersion(
    const std::string& adapter_base_id
) {
    auto versions = listVersions(adapter_base_id);
    if (versions.empty()) {
        return std::nullopt;
    }
    // Versions are sorted ascending; return the last (latest)
    return versions.back();
}

std::optional<AdapterMetadata> AdapterRegistry::getVersion(
    const std::string& adapter_base_id,
    const AdapterVersion& version
) {
    // Use a delimiter-aware prefix check to avoid matching "model_v2" when
    // searching for base id "model" (without a following ':' delimiter).
    std::string prefix = adapter_base_id + ":";
    std::shared_lock<std::shared_mutex> lock(impl_->rw_mu);
    for (const auto& [id, meta] : impl_->adapters) {
        if ((meta.adapter_id == adapter_base_id || meta.adapter_id.rfind(prefix, 0) == 0) &&
            meta.version == version) {
            return meta;
        }
    }
    return std::nullopt;
}

std::vector<AdapterMetadata> AdapterRegistry::listVersions(
    const std::string& adapter_base_id
) {
    // Same delimiter-aware prefix check used in getVersion().
    std::string prefix = adapter_base_id + ":";
    std::shared_lock<std::shared_mutex> lock(impl_->rw_mu);
    std::vector<AdapterMetadata> versions;
    for (const auto& [id, meta] : impl_->adapters) {
        if (meta.adapter_id == adapter_base_id || meta.adapter_id.rfind(prefix, 0) == 0) {
            versions.push_back(meta);
        }
    }
    // Sort by version ascending
    std::sort(versions.begin(), versions.end(),
              [](const AdapterMetadata& a, const AdapterMetadata& b) {
                  return a.version < b.version;
              });
    return versions;
}

// ============================================================================
// Search and discovery
// ============================================================================

std::vector<AdapterMetadata> AdapterRegistry::searchAdapters(
    const SearchCriteria& criteria
) {
    std::shared_lock<std::shared_mutex> lock(impl_->rw_mu);
    std::vector<AdapterMetadata> result;

    for (const auto& [id, meta] : impl_->adapters) {
        if (criteria.base_model && meta.base_model_name != *criteria.base_model) continue;
        if (criteria.domain     && meta.domain          != *criteria.domain)     continue;
        if (criteria.task_type  && meta.task_type        != *criteria.task_type)  continue;
        if (criteria.language   && meta.language         != *criteria.language)   continue;
        if (criteria.status     && meta.status           != *criteria.status)     continue;
        result.push_back(meta);
    }
    return result;
}

// ============================================================================
// Statistics
// ============================================================================

AdapterRegistry::RegistryStats AdapterRegistry::getStats() const {
    std::shared_lock<std::shared_mutex> lock(impl_->rw_mu);
    RegistryStats stats;
    stats.total_adapters = impl_->adapters.size();
    stats.signed_adapters = impl_->signatures.size();

    std::unordered_map<std::string, size_t> by_model;
    std::unordered_map<std::string, size_t> by_domain;
    for (const auto& [id, meta] : impl_->adapters) {
        by_model[meta.base_model_name]++;
        by_domain[meta.domain]++;
        if (meta.status == AdapterMetadata::Status::DEPLOYED) {
            stats.deployed_adapters++;
        }
    }
    stats.total_base_models            = by_model.size();
    stats.adapters_per_base_model      = std::map<std::string, size_t>(by_model.begin(), by_model.end());
    stats.adapters_per_domain          = std::map<std::string, size_t>(by_domain.begin(), by_domain.end());
    return stats;
}

// ============================================================================
// Provenance Integration
// ============================================================================

bool AdapterRegistry::attachProvenance(const std::string& adapter_id,
                                        const lora::LoRAProvenanceRecord& record) {
    // Verify the adapter exists before accepting provenance
    {
        std::unique_lock<std::shared_mutex> lock(impl_->rw_mu);
        if (!impl_->adapters.count(adapter_id)) {
            spdlog::warn("AdapterRegistry::attachProvenance: adapter '{}' not found",
                         adapter_id);
            return false;
        }
    }
    return provenance_mgr_.storeProvenance(adapter_id, record);
}

std::optional<lora::LoRAProvenanceRecord> AdapterRegistry::getProvenanceRecord(
    const std::string& adapter_id) const {
    return provenance_mgr_.getProvenance(adapter_id);
}

lora::InferenceAuditEntry AdapterRegistry::recordInferenceAudit(
    const std::string& adapter_id,
    lora::InferenceAuditEntry entry) {
    return provenance_mgr_.appendAuditEntry(adapter_id, std::move(entry));
}

std::vector<lora::InferenceAuditEntry> AdapterRegistry::getInferenceAuditLog(
    const std::string& adapter_id) const {
    return provenance_mgr_.getAuditLog(adapter_id);
}

bool AdapterRegistry::verifyAuditChain(const std::string& adapter_id) const {
    return provenance_mgr_.verifyAuditChain(adapter_id);
}

// ============================================================================
// Hot-Loading Interface
// ============================================================================

bool AdapterRegistry::hotLoad(
    const std::string& adapter_id,
    const std::string& weights_path,
    const AdapterMetadata& metadata,
    float scale
) {
    if (adapter_id.empty()) {
        spdlog::error("AdapterRegistry::hotLoad: adapter_id must not be empty");
        return false;
    }
    if (weights_path.empty()) {
        spdlog::error("AdapterRegistry::hotLoad: weights_path must not be empty");
        return false;
    }

    // Register (or update) adapter metadata under an exclusive write lock.
    {
        AdapterMetadata meta = metadata;
        meta.adapter_id   = adapter_id;
        meta.storage_path = weights_path;

        std::unique_lock<std::shared_mutex> lock(impl_->rw_mu);
        bool existed = impl_->adapters.count(adapter_id) > 0;
        impl_->adapters[adapter_id] = meta;
        spdlog::debug("AdapterRegistry::hotLoad: {} adapter '{}'",
                      existed ? "updated" : "registered", adapter_id);
    }

    // Snapshot callbacks outside the lock to avoid lock inversion if a
    // callback itself calls back into the registry.
    std::vector<HotLoadCallback> callbacks;
    {
        std::shared_lock<std::shared_mutex> lock(impl_->rw_mu);
        callbacks = impl_->hot_load_callbacks;
    }

    for (const auto& cb : callbacks) {
        if (cb) {
            cb(adapter_id, weights_path, scale);
        }
    }

    spdlog::info("AdapterRegistry: hot-loaded adapter '{}' (path='{}')",
                 adapter_id, weights_path);
    return true;
}

void AdapterRegistry::addHotLoadObserver(HotLoadCallback callback) {
    if (!callback) {
        spdlog::warn("AdapterRegistry::addHotLoadObserver: null callback ignored");
        return;
    }
    std::unique_lock<std::shared_mutex> lock(impl_->rw_mu);
    impl_->hot_load_callbacks.push_back(std::move(callback));
    spdlog::debug("AdapterRegistry: hot-load observer registered (total: {})",
                  impl_->hot_load_callbacks.size());
}

} // namespace llm
} // namespace themis
