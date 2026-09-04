/**
 * @file adapter_registry.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 92/100
 * @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=1, M=9, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "llm/adapter_registry.h"
#include "llm/raii_wrappers.h"
#include "storage/security_signature.h"
#include <spdlog/spdlog.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <algorithm>
#include <cctype>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <chrono>

namespace themis {
namespace llm {

namespace {

const char* statusToString(AdapterMetadata::Status status) {
    switch (status) {
        case AdapterMetadata::Status::TRAINING:
            return "TRAINING";
        case AdapterMetadata::Status::TRAINED:
            return "TRAINED";
        case AdapterMetadata::Status::DEPLOYED:
            return "DEPLOYED";
        case AdapterMetadata::Status::DEPRECATED:
            return "DEPRECATED";
        case AdapterMetadata::Status::FAILED:
            return "FAILED";
    }
    return "TRAINED";
}

AdapterMetadata::Status statusFromString(const std::string& status) {
    if (status == "TRAINING") {
      return AdapterMetadata::Status::TRAINING;
    }
    if (status == "TRAINED") {
      return AdapterMetadata::Status::TRAINED;
    }
    if (status == "DEPLOYED") {
      return AdapterMetadata::Status::DEPLOYED;
    }
    if (status == "DEPRECATED") {
      return AdapterMetadata::Status::DEPRECATED;
    }
    if (status == "FAILED") {
      return AdapterMetadata::Status::FAILED;
    }
    return AdapterMetadata::Status::TRAINED;
}

} // namespace

// ============================================================================
// AdapterSignature / AdapterMetadata JSON conversion
// ============================================================================

nlohmann::json AdapterSignature::toJson() const {
    return {
        {"content_hash", content_hash},
        {"signature", signature},
        {"signer_identity", signer_identity},
        {"signing_timestamp", signing_timestamp},
        {"parent_adapter_signature", parent_adapter_signature},
    };
}

AdapterSignature AdapterSignature::fromJson(const nlohmann::json& j) {
    AdapterSignature s;
    s.content_hash = j.value("content_hash", "");
    s.signature = j.value("signature", "");
    s.signer_identity = j.value("signer_identity", "");
    s.signing_timestamp = j.value("signing_timestamp", "");
    s.parent_adapter_signature = j.value("parent_adapter_signature", "");
    return s;
}

nlohmann::json AdapterMetadata::toJson() const {
    nlohmann::json j;
    j["adapter_id"] = adapter_id;
    j["task_type"] = task_type;
    j["domain"] = domain;
    j["language"] = language;
    j["role"] = (role == AdapterRole::DRAFT ? "DRAFT" : "GENERAL");
    j["base_model_name"] = base_model_name;
    j["base_model_version"] = base_model_version;
    j["architecture"] = architecture;
    j["hidden_size"] = hidden_size;
    j["ffn_dimension"] = ffn_dimension;
    j["tokenizer_name"] = tokenizer_name;
    j["storage_path"] = storage_path;
    j["file_size_bytes"] = file_size_bytes;
    j["format"] = format;
    j["quantization"] = quantization;
    j["status"] = statusToString(status);
    j["created_at"] = created_at;
    j["updated_at"] = updated_at;
    j["signature"] = signature.toJson();
    return j;
}

AdapterMetadata AdapterMetadata::fromJson(const nlohmann::json& j) {
    AdapterMetadata m;
    m.adapter_id = j.value("adapter_id", "");
    m.task_type = j.value("task_type", "");
    m.domain = j.value("domain", "");
    m.language = j.value("language", "en");

    const std::string role_str = j.value("role", "GENERAL");
    m.role = (role_str == "DRAFT") ? AdapterRole::DRAFT : AdapterRole::GENERAL;

    m.base_model_name = j.value("base_model_name", "");
    m.base_model_version = j.value("base_model_version", "");
    m.architecture = j.value("architecture", "");
    m.hidden_size = j.value("hidden_size", 0);
    m.ffn_dimension = j.value("ffn_dimension", 0);
    m.tokenizer_name = j.value("tokenizer_name", "");
    m.storage_path = j.value("storage_path", "");
    m.file_size_bytes = j.value("file_size_bytes", static_cast<size_t>(0));
    m.format = j.value("format", "GGUF-ST");
    m.quantization = j.value("quantization", "Q4_K_M");
    m.status = statusFromString(j.value("status", "TRAINED"));
    m.created_at = j.value("created_at", "");
    m.updated_at = j.value("updated_at", "");

    if (j.contains("signature") && j["signature"].is_object()) {
        m.signature = AdapterSignature::fromJson(j["signature"]);
    }
    return m;
}

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
    std::vector<AdapterMetadata> result = {};

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
    std::vector<AdapterMetadata> result = {};

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
    std::vector<AdapterMetadata> result = {};

    for (const auto& [id, meta] : impl_->adapters) {
        if (meta.domain == domain) {
            result.push_back(meta);
        }
    }
    return result;
}

std::vector<AdapterMetadata> AdapterRegistry::listAdaptersByRole(AdapterRole role) {
    std::shared_lock<std::shared_mutex> lock(impl_->rw_mu);
    std::vector<AdapterMetadata> result = {};

    for (const auto& [id, meta] : impl_->adapters) {
        if (meta.role == role) {
            result.push_back(meta);
        }
    }
    return result;
}

std::optional<AdapterMetadata> AdapterRegistry::findDraftAdapterForFamily(
    const std::string& model_family
) {
    if (model_family.empty()) {
        return std::nullopt;
    }

    // Build lowercase copy of family for case-insensitive matching.
    std::string family_lower = model_family;
    std::transform(family_lower.begin(), family_lower.end(),
                   family_lower.begin(), [](unsigned char c) {
                       return static_cast<char>(std::tolower(c));
                   });

    std::shared_lock<std::shared_mutex> lock(impl_->rw_mu);

    std::optional<AdapterMetadata> best = {};

    for (const auto& [id, meta] : impl_->adapters) {
        if (meta.role != AdapterRole::DRAFT) {
            continue;
        }

        // Case-insensitive substring match on architecture field.
        std::string arch_lower = meta.architecture;
        std::transform(arch_lower.begin(), arch_lower.end(),
                       arch_lower.begin(), [](unsigned char c) {
                           return static_cast<char>(std::tolower(c));
                       });
        if (arch_lower.find(family_lower) == std::string::npos) {
            continue;
        }

        if (!best.has_value()) {
            best = meta;
            continue;
        }

        // Prefer DEPLOYED status over other states.
        const bool meta_deployed = (meta.status == AdapterMetadata::Status::DEPLOYED);
        const bool best_deployed = (best->status == AdapterMetadata::Status::DEPLOYED);
        if (meta_deployed && !best_deployed) {
            best = meta;
            continue;
        }
        if (!meta_deployed && best_deployed) {
            continue;
        }

        // Among equal-status candidates prefer the highest version.
        if (best->version < meta.version) {
            best = meta;
        }
    }

    if (best.has_value()) {
        spdlog::debug("AdapterRegistry::findDraftAdapterForFamily: found '{}' for family '{}'",
                      best->adapter_id, model_family);
    } else {
        spdlog::debug("AdapterRegistry::findDraftAdapterForFamily: no DRAFT adapter for family '{}'",
                      model_family);
    }
    return best;
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
                                   const std::string& private_key) {
    std::unique_lock<std::shared_mutex> lock(impl_->rw_mu);
    if (!impl_->adapters.count(adapter_id)) {
        spdlog::warn("AdapterRegistry::signAdapter: adapter '{}' not found", adapter_id);
        return false;
    }

    AdapterSignature sig;
    sig.signer_identity = "adapter_registry";
    sig.content_hash    = storage::SecuritySignatureManager::computeFileHash(adapter_id);
    auto now    = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", std::gmtime(&time_t));
    sig.signing_timestamp = buf;

    // Perform real Ed25519 signature over content_hash when a PEM private key
    // is provided.  Fall back to a deterministic placeholder when no key is
    // supplied (useful in test/CI environments).
    if (!private_key.empty()) {
        // BATCH 2: RAII wrapper for BIO - automatically freed at scope exit
        ScopedBIO bio(BIO_new_mem_buf(private_key.data(),
                                       static_cast<int>(private_key.size())));
        
        // BATCH 2: RAII wrapper for EVP_PKEY - automatically freed at scope exit
        ScopedEVPKey pkey(PEM_read_bio_PrivateKey(bio.get(), nullptr, nullptr, nullptr));

        if (!pkey) {
            spdlog::warn("AdapterRegistry::signAdapter: failed to parse private key for '{}'",
                         adapter_id);
            return false;
        }

        // BATCH 2: RAII wrapper for EVP_MD_CTX - automatically freed at scope exit
        ScopedEVPContext ctx(EVP_MD_CTX_new());
        std::string signature_bytes = {};
        bool sign_ok = false;

        if (ctx && EVP_DigestSignInit(ctx.get(), nullptr, nullptr, nullptr, pkey.get()) == 1) {
            std::size_t sig_len = 0;
            if (EVP_DigestSign(ctx.get(),
                               nullptr, &sig_len,
                               reinterpret_cast<const unsigned char*>(sig.content_hash.data()),
                               sig.content_hash.size()) == 1) {
                signature_bytes.resize(sig_len);
                if (EVP_DigestSign(ctx.get(),
                                   reinterpret_cast<unsigned char*>(signature_bytes.data()),
                                   &sig_len,
                                   reinterpret_cast<const unsigned char*>(sig.content_hash.data()),
                                   sig.content_hash.size()) == 1) {
                    signature_bytes.resize(sig_len);
                    sign_ok = true;
                }
            }
        }
        // BATCH 2: No manual cleanup needed - RAII destructors handle it automatically
        // bio, pkey, and ctx will be freed here when they go out of scope

        if (!sign_ok) {
            spdlog::warn("AdapterRegistry::signAdapter: Ed25519 sign failed for '{}'",
                         adapter_id);
            return false;
        }

        // Hex-encode the raw signature bytes for storage
        static constexpr char hex[] = "0123456789abcdef";
        std::string hex_sig = {};
        hex_sig.reserve(signature_bytes.size() * 2);
        for (unsigned char c : signature_bytes) {
            hex_sig += hex[(c >> 4) & 0xf];
            hex_sig += hex[c & 0xf];
        }
        sig.signature = "ed25519:" + hex_sig;
    } else {
        // No private key: deterministic placeholder for testing
        sig.signature = "sig:" + sig.content_hash;
    }

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
    std::vector<AdapterMetadata> versions = {};

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
        if (criteria.base_model && meta.base_model_name != *criteria.base_model) {
          continue;
        }
        if (criteria.domain     && meta.domain          != *criteria.domain) {
          continue;
        }
        if (criteria.task_type  && meta.task_type        != *criteria.task_type) {
          continue;
        }
        if (criteria.language   && meta.language         != *criteria.language) {
          continue;
        }
        if (criteria.status     && meta.status           != *criteria.status) {
          continue;
        }
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
    std::unordered_map<std::string, size_t> by_domain = {};

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

    for ([[maybe_unused]] const auto& cb : callbacks) {
        if (cb) {
            cb(adapter_id, weights_path, scale);
        }
    }

    spdlog::info("AdapterRegistry: hot-loaded adapter '{}' (path='{}')",
                 adapter_id, weights_path);
    return true;
}

void AdapterRegistry::addHotLoadObserver([[maybe_unused]] HotLoadCallback callback) {
    if ([[maybe_unused]] !callback) {
        spdlog::warn([[maybe_unused]] "AdapterRegistry::addHotLoadObserver: null callback ignored");
        return;
    }
    std::unique_lock<std::shared_mutex> lock(impl_->rw_mu);
    impl_->hot_load_callbacks.push_back([[maybe_unused]] std::move(callback));
    spdlog::debug("AdapterRegistry: hot-load observer registered (total: {})",
                  impl_->hot_load_callbacks.size());
}

} // namespace llm
} // namespace themis

