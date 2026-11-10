/**
 * @file lora_orchestrator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/lora_orchestrator.h"
#include "llm/lora_framework/lora_safetensors_package.h"
#include "llm/lora_framework/lora_dataset_loader.h"
#include "llm/decision_record_yaml_processor.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <chrono>
#include <condition_variable>
#include <ctime>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <mutex>
#include <shared_mutex>
#include <spdlog/spdlog.h>
#include <sstream>
#include <thread>
#include <unordered_map>
#include <utility>

namespace themis {
namespace llm {
namespace lora {

using json = nlohmann::json;
using Clock = std::chrono::system_clock;

namespace {
std::atomic<uint64_t> g_job_counter{0};

/**
 * @brief Make Job Id.
 * @param[in] prefix Input parameter.
 * @return Return value.
 * @details Calls: std::to_string().
 */
std::string makeJobId(const std::string& prefix) {
    return prefix + "-" + std::to_string(++g_job_counter);
}

/**
 * @brief Make Metadata.
 * @param[in] adapter_id Identifier of the adapter.
 * @param[in] version Input parameter.
 * @return Return value.
 * @details Calls: Clock::now().
 */
AdapterMetadata makeMetadata(const std::string& adapter_id, const std::string& version) {
    AdapterMetadata metadata;
    metadata.adapter_id = adapter_id;
    metadata.version = version;
    metadata.base_model = "";
    metadata.description = "";
    metadata.created_at = Clock::now();
    metadata.updated_at = metadata.created_at;
    return metadata;
}

/**
 * @brief Make Adapter Info.
 * @param[in] adapter_id Identifier of the adapter.
 * @param[in] version Input parameter.
 * @return Return value.
 * @details Calls: makeMetadata().
 */
AdapterInfo makeAdapterInfo(const std::string& adapter_id, const std::string& version) {
    AdapterInfo info;
    info.adapter_id = adapter_id;
    info.version = version;
    info.base_model = "";
    info.description = "";
    info.is_loaded = false;
    info.is_pinned = false;
    info.metadata = makeMetadata(adapter_id, version);
    return info;
}

namespace fs = std::filesystem;
using JobStatusT = LoRAOrchestrator::JobStatus;

constexpr const char* kStoredSafetensors = "adapter_model.safetensors";
constexpr const char* kStoredAdapterConfig = "adapter_config.json";
constexpr const char* kStoredGguf = "adapter.gguf";
constexpr const char* kStoredManifest = "adapter.manifest.json";

/// Adapter ids become directory names: restrict to a safe charset.
bool isValidAdapterId(const std::string& id) {
    if (id.empty() || id.size() > 128 || id == "." || id == "..") {
        return false;
    }
    return std::all_of(id.begin(), id.end(), [](unsigned char c) {
        return std::isalnum(c) != 0 || c == '_' || c == '.' || c == '-';
    });
}

bool isTerminal(JobStatusT s) {
    return s == JobStatusT::Completed || s == JobStatusT::Failed || s == JobStatusT::Cancelled;
}

std::string toLowerCopy(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::string isoUtcNow() {
    const std::time_t t = Clock::to_time_t(Clock::now());
    std::tm tm{};
#ifdef _WIN32
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    std::ostringstream os;
    os << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return os.str();
}

std::string metaValue(const std::map<std::string, std::string>& m, const std::string& key) {
    const auto it = m.find(key);
    return it == m.end() ? std::string{} : it->second;
}

bool hasGgufMagic(const fs::path& p) {
    std::ifstream in(p, std::ios::binary);
    std::array<char, 4> magic{};
    return in.read(magic.data(), 4) && magic[0] == 'G' && magic[1] == 'G' && magic[2] == 'U' &&
           magic[3] == 'F';
}

/**
 * Base-model compatibility of an adapter (embedded themis.base_model.*) with
 * the configured serving base model. A matching sha256 is authoritative;
 * otherwise GGUF file name and model name must agree where both are known.
 * @return Empty string if compatible, otherwise the reason.
 */
std::string checkBaseCompatibility(const std::map<std::string, std::string>& meta,
                                   const LoRAOrchestrator::TrainingDefaults& d) {
    const std::string name = metaValue(meta, "themis.base_model.name");
    const std::string gguf = metaValue(meta, "themis.base_model.gguf");
    const std::string sha = metaValue(meta, "themis.base_model.sha256");
    if (name.empty() && gguf.empty() && sha.empty()) {
        return "adapter metadata lacks base model identity (themis.base_model.name/gguf/sha256)";
    }
    if (!sha.empty() && !d.base_model_gguf_sha256.empty()) {
        return toLowerCopy(sha) == toLowerCopy(d.base_model_gguf_sha256)
                   ? std::string{}
                   : "base model sha256 mismatch (adapter " + sha + ", configured " +
                         d.base_model_gguf_sha256 + ")";
    }
    if (!gguf.empty() && !d.base_model_gguf.empty() &&
        fs::path(gguf).filename() != fs::path(d.base_model_gguf).filename()) {
        return "base model GGUF mismatch (adapter " + fs::path(gguf).filename().string() +
               ", configured " + fs::path(d.base_model_gguf).filename().string() + ")";
    }
    if (!name.empty() && !d.base_model.empty() && name != d.base_model) {
        return "base model mismatch (adapter " + name + ", configured " + d.base_model + ")";
    }
    return {};
}

/// A safetensors package that passed structural, identity and integrity checks.
struct VerifiedPackage {
    bool ok = false;
    std::string error;
    AdapterPackagePaths paths;
    SafetensorsInspection inspection;
    std::string tensors_sha256;
};

VerifiedPackage verifyPackage(const std::string& source,
                              const std::string& adapter_id,
                              const LoRAOrchestrator::TrainingDefaults& defaults) {
    VerifiedPackage v;
    v.paths = SafetensorsPackageValidator::resolvePackage(source);
    if (!v.paths.valid) {
        v.error = "invalid adapter package: " + v.paths.error;
        return v;
    }
    v.inspection = SafetensorsPackageValidator::inspectFile(v.paths.safetensors);
    if (!v.inspection.valid) {
        v.error = "invalid safetensors file: " + v.inspection.error;
        return v;
    }
    const auto& meta = v.inspection.metadata;
    const std::string embedded_id = metaValue(meta, "themis.adapter.id");
    if (embedded_id.empty()) {
        v.error = "safetensors __metadata__ lacks themis.adapter.id";
        return v;
    }
    if (embedded_id != adapter_id) {
        v.error = "themis.adapter.id '" + embedded_id + "' does not match adapter id '" + adapter_id + "'";
        return v;
    }
    const std::string method = metaValue(meta, "themis.adapter.method");
    if (method != "lora" && method != "adalora") {
        v.error = "themis.adapter.method must be 'lora' or 'adalora'";
        return v;
    }
    const std::string expected_sha = toLowerCopy(metaValue(meta, "themis.adapter.sha256_tensors"));
    if (expected_sha.empty()) {
        v.error = "safetensors __metadata__ lacks themis.adapter.sha256_tensors";
        return v;
    }
    v.tensors_sha256 = SafetensorsPackageValidator::sha256OfTensorData(v.paths.safetensors, v.inspection);
    if (v.tensors_sha256.empty()) {
        v.error = "cannot hash safetensors tensor data";
        return v;
    }
    if (v.tensors_sha256 != expected_sha) {
        v.error = "tensor data sha256 mismatch (embedded " + expected_sha + ", computed " +
                  v.tensors_sha256 + ")";
        return v;
    }
    const std::string compat = checkBaseCompatibility(meta, defaults);
    if (!compat.empty()) {
        v.error = compat;
        return v;
    }
    v.ok = true;
    return v;
}

/// Published artifact files of one adapter version.
struct StoredArtifact {
    bool ok = false;
    std::string error;
    fs::path version_dir;
    fs::path safetensors;
    fs::path gguf;
    std::uint64_t gguf_bytes = 0;
    json adapter_meta;  ///< themis.* metadata + "artifact" block.
};

/// Creates <root>/<adapter_id>/<version>/ and copies the verified package into it.
StoredArtifact storePackage(const std::string& artifact_root,
                            const std::string& adapter_id,
                            const std::string& version,
                            const VerifiedPackage& pkg) {
    StoredArtifact art;
    std::error_code ec;
    art.version_dir = fs::path(artifact_root) / adapter_id / version;
    if (fs::exists(art.version_dir, ec)) {
        art.error = "artifact version directory already exists: " + art.version_dir.string();
        return art;
    }
    fs::create_directories(art.version_dir, ec);
    if (ec) {
        art.error = "cannot create artifact directory: " + ec.message();
        return art;
    }
    art.safetensors = art.version_dir / kStoredSafetensors;
    fs::copy_file(pkg.paths.safetensors, art.safetensors, fs::copy_options::none, ec);
    if (!ec) {
        fs::copy_file(pkg.paths.adapter_config, art.version_dir / kStoredAdapterConfig,
                      fs::copy_options::none, ec);
    }
    if (ec) {
        art.error = "cannot store adapter package: " + ec.message();
        fs::remove_all(art.version_dir, ec);
        return art;
    }
    art.adapter_meta = json::object();
    for (const auto& [k, val] : pkg.inspection.metadata) {
        if (k.rfind("themis.", 0) == 0) {
            art.adapter_meta[k] = val;
        }
    }
    art.ok = true;
    return art;
}

/**
 * Verifies the GGUF at art.gguf, hashes it and writes the convenience manifest.
 * On failure the version directory is removed.
 */
bool finalizeArtifact(StoredArtifact& art, const VerifiedPackage& pkg, const json& extra) {
    std::error_code ec;
    const auto size = fs::file_size(art.gguf, ec);
    if (ec || size == 0 || !hasGgufMagic(art.gguf)) {
        art.error = "GGUF adapter missing or not a GGUF file: " + art.gguf.string();
        fs::remove_all(art.version_dir, ec);
        return false;
    }
    const std::string gguf_sha = SafetensorsPackageValidator::sha256OfFile(art.gguf);
    if (gguf_sha.empty()) {
        art.error = "cannot hash GGUF adapter";
        fs::remove_all(art.version_dir, ec);
        return false;
    }
    art.gguf_bytes = size;
    art.adapter_meta["artifact"] = {
        {"version_dir", art.version_dir.string()},
        {"safetensors_path", art.safetensors.string()},
        {"gguf_path", art.gguf.string()},
        {"gguf_sha256", gguf_sha},
        {"gguf_bytes", size},
        {"tensors_sha256", pkg.tensors_sha256},
        {"stored_at", isoUtcNow()}};
    for (auto it = extra.begin(); extra.is_object() && it != extra.end(); ++it) {
        art.adapter_meta[it.key()] = it.value();
    }
    json manifest = art.adapter_meta;
    manifest["note"] =
        "Convenience copy. Source of truth: themis.* keys embedded in the safetensors "
        "__metadata__ header and in the GGUF KV store.";
    std::ofstream out(art.version_dir / kStoredManifest, std::ios::trunc);
    out << manifest.dump(2);
    if (!out) {
        art.error = "cannot write adapter manifest";
        out.close();
        fs::remove_all(art.version_dir, ec);
        return false;
    }
    return true;
}

} // namespace

class LoRAOrchestrator::Impl {
public:
    /// Queued training work.
    struct WorkItem {
        std::string job_id;
        TrainingJobRequest request;
        std::shared_ptr<std::atomic<bool>> cancel;
        std::string version;
        TrainingDefaults defaults;  ///< Effective defaults (config.training or regeneration target).
        std::optional<TrainingSnapshotFingerprint> snapshot;  ///< Training-data snapshot (WP H).
        std::optional<std::string> snapshot_lease;            ///< Pin lease held for this job.
    };

    explicit Impl(Config cfg) : config(std::move(cfg)) {}
    ~Impl() { shutdown(); }
    Impl(const Impl&) = delete;
    Impl& operator=(const Impl&) = delete;

    Config config;
    std::atomic<bool> is_initialized{false};
    bool advanced_enabled = false;
    mutable std::shared_mutex state_mutex;
    std::condition_variable_any job_cv;  ///< Signalled on every job state change (with state_mutex).
    std::unordered_map<std::string, AdapterInfo> adapters;
    std::unordered_map<std::string, std::vector<std::string>> versions;
    std::unordered_map<std::string, int> version_counter;
    std::unordered_map<std::string, JobInfo> jobs;
    std::unordered_map<std::string, std::shared_ptr<std::atomic<bool>>> cancel_flags;
    std::vector<EventCallback> callbacks;

    // Training / dynamic loading integration (guarded by state_mutex)
    std::shared_ptr<ILoRATrainingBackend> backend;
    AdapterRegistrar registrar;
    AdapterConverter converter;
    std::shared_ptr<MultiLoRAManager> lora_manager;

    // Training-data snapshot provenance & signing (guarded by state_mutex)
    std::shared_ptr<TrainingSnapshotReader> snapshot_reader;
    std::shared_ptr<AdapterProvenanceSigner> signer;
    std::shared_ptr<AdapterProvenanceVerifier> verifier;
    AdapterSignaturePolicy signature_policy;

    // Component instances for cross-shard sync
    std::shared_ptr<LoRAStorageService> storage_service;
    std::shared_ptr<AdapterConsistencyChecker> consistency_checker;

    // Provenance manager for cryptographic audit and MVCC snapshots
    LoRAProvenanceManager provenance_mgr;

    // Decision traceability (optional, non-blocking)
    std::shared_ptr<themis::llm::DecisionRecordYamlProcessor> dr_processor;

    // Worker pool (guarded by queue_mutex)
    std::mutex queue_mutex;
    std::condition_variable queue_cv;
    std::deque<WorkItem> queue;
    std::vector<std::thread> workers;
    bool stopping = false;

    /// Invoke event callbacks without holding any lock.
    void fire(EventType type, const std::string& adapter_id, const json& payload) {
        std::vector<EventCallback> cbs;
        {
            std::shared_lock<std::shared_mutex> lock(state_mutex);
            cbs = callbacks;
        }
        for (const auto& cb : cbs) {
            try {
                if (cb) {
                    cb(type, adapter_id, payload);
                }
            } catch (const std::exception& e) {
                spdlog::warn("LoRAOrchestrator: event callback threw: {}", e.what());
            } catch (...) {
                spdlog::warn("LoRAOrchestrator: event callback threw a non-standard exception");
            }
        }
    }

    /// Reserve the next version name for @p adapter_id. Caller holds state_mutex exclusively.
    std::string reserveVersion(const std::string& adapter_id) {
        int& counter = version_counter[adapter_id];
        const auto& existing = versions[adapter_id];
        counter = std::max<int>(counter, static_cast<int>(existing.size()));
        std::string candidate;
        std::error_code ec;
        do {
            ++counter;
            candidate = "v" + std::to_string(counter);
        } while (std::find(existing.begin(), existing.end(), candidate) != existing.end() ||
                 fs::exists(fs::path(config.training.artifact_root) / adapter_id / candidate, ec));
        return candidate;
    }

    /// Set a terminal state for @p job_id and wake waiters.
    void finishJob(const std::string& job_id, JobStatus status, const std::string& error,
                   const json& extra_metadata) {
        {
            std::unique_lock<std::shared_mutex> lock(state_mutex);
            auto it = jobs.find(job_id);
            if (it != jobs.end()) {
                it->second.status = status;
                it->second.error_message = error;
                it->second.updated_at = Clock::now();
                if (status == JobStatus::Completed) {
                    it->second.progress = 1.0f;
                }
                for (auto e = extra_metadata.begin(); extra_metadata.is_object() && e != extra_metadata.end(); ++e) {
                    it->second.metadata[e.key()] = e.value();
                }
            }
            cancel_flags.erase(job_id);
        }
        job_cv.notify_all();
    }

    void failTraining(const std::string& job_id, const std::string& adapter_id, const std::string& error) {
        spdlog::warn("LoRAOrchestrator: training job {} for adapter '{}' failed: {}", job_id, adapter_id, error);
        finishJob(job_id, JobStatus::Failed, error, json::object());
        const json payload = {{"job_id", job_id}, {"error", error}};
        fire(EventType::TrainingFailed, adapter_id, payload);
        fire(EventType::JobFailed, adapter_id, payload);
    }

    /// Release a snapshot pin lease (no-op without reader/lease).
    void releaseLease(const std::optional<std::string>& lease) {
        if (!lease) {
            return;
        }
        std::shared_ptr<TrainingSnapshotReader> reader;
        {
            std::shared_lock<std::shared_mutex> lock(state_mutex);
            reader = snapshot_reader;
        }
        if (reader && !reader->releasePin(*lease)) {
            spdlog::warn("LoRAOrchestrator: failed to release snapshot pin {}", *lease);
        }
    }

    /// Record a training job that is rejected before any training happens.
    std::string rejectJob(const std::string& adapter_id, const std::string& error, json metadata) {
        JobInfo job;
        job.job_id = makeJobId("train");
        job.type = JobType::Training;
        job.status = JobStatus::Failed;
        job.adapter_id = adapter_id;
        job.started_at = Clock::now();
        job.updated_at = job.started_at;
        job.error_message = error;
        job.metadata = metadata.is_object() ? std::move(metadata) : json::object();
        {
            std::unique_lock<std::shared_mutex> lock(state_mutex);
            jobs[job.job_id] = job;
        }
        job_cv.notify_all();
        spdlog::warn("LoRAOrchestrator: training job {} for adapter '{}' rejected: {}", job.job_id,
                     adapter_id, error);
        const json payload = {{"job_id", job.job_id}, {"error", error}};
        fire(EventType::TrainingFailed, adapter_id, payload);
        fire(EventType::JobFailed, adapter_id, payload);
        return job.job_id;
    }

    /**
     * Verify provenance of safetensors metadata under the current verifier.
     * Without a verifier, signed metadata is Invalid (cannot be checked) and
     * unsigned metadata is Unsigned.
     */
    ProvenanceVerification verifyMetadataProvenance(const std::map<std::string, std::string>& md) {
        std::shared_ptr<AdapterProvenanceVerifier> v;
        {
            std::shared_lock<std::shared_mutex> lock(state_mutex);
            v = verifier;
        }
        if (v) {
            return v->verifyMetadata(md);
        }
        ProvenanceVerification r;
        const bool has_provenance = std::any_of(md.begin(), md.end(), [](const auto& kv) {
            return kv.first.rfind(provenance_keys::kPrefix, 0) == 0;
        });
        r.status = has_provenance ? ProvenanceVerification::Status::Invalid
                                  : ProvenanceVerification::Status::Unsigned;
        r.error = has_provenance ? "adapter is signed but no provenance verifier is configured"
                                 : "adapter metadata carries no provenance signature";
        return r;
    }

    /**
     * Install the GGUF hot-load provenance check on @p mgr from the current
     * verifier/policy (caller holds state_mutex).  Captures by value so the
     * hook stays valid if the manager outlives the orchestrator.
     */
    void installHotLoadVerifier(const std::shared_ptr<MultiLoRAManager>& mgr) {
        if (!mgr) {
            return;
        }
        std::shared_ptr<AdapterProvenanceVerifier> v = verifier;
        const bool require = signature_policy.require_signed_hot_load;
        mgr->setAdapterMetadataVerifier([v, require](const std::string& lora_id, const json& kv,
                                                     std::string& error) {
            ProvenanceVerification r;
            if (v) {
                r = v->verifyGgufKv(kv);
            } else {
                bool has_provenance = false;
                for (auto it = kv.begin(); kv.is_object() && it != kv.end(); ++it) {
                    has_provenance = has_provenance || it.key().rfind(provenance_keys::kPrefix, 0) == 0;
                }
                r.status = has_provenance ? ProvenanceVerification::Status::Invalid
                                          : ProvenanceVerification::Status::Unsigned;
                r.error = has_provenance ? "adapter is signed but no provenance verifier is configured"
                                         : "adapter GGUF carries no provenance signature";
            }
            if (!r.acceptable(require)) {
                error = "adapter provenance rejected for '" + lora_id + "': " + r.error;
                return false;
            }
            return true;
        });
    }

    /// Call the registrar outside locks. @return registered flag.
    bool registerArtifact(const std::string& adapter_id, const StoredArtifact& art, std::string& error) {
        AdapterRegistrar reg;
        {
            std::shared_lock<std::shared_mutex> lock(state_mutex);
            reg = registrar;
        }
        if (!reg) {
            error = "no adapter registrar configured (adapter stored but not loaded)";
            return false;
        }
        try {
            return reg(adapter_id, art.gguf.string(), art.adapter_meta, error);
        } catch (const std::exception& e) {
            error = std::string("adapter registrar threw: ") + e.what();
            return false;
        }
    }

    /// Record a published version in the adapter registry.
    void commitAdapter(const std::string& adapter_id, const std::string& version,
                       const StoredArtifact& art, const LoRAHyperparameters& hp,
                       std::size_t samples, bool registered, const std::string& description) {
        std::unique_lock<std::shared_mutex> lock(state_mutex);
        auto existing = adapters.find(adapter_id);
        AdapterInfo info = existing != adapters.end() ? existing->second : makeAdapterInfo(adapter_id, version);
        const auto now = Clock::now();
        info.version = version;
        info.hyperparameters = hp;
        info.is_loaded = registered;
        info.memory_bytes = static_cast<size_t>(art.gguf_bytes);
        info.base_model = art.adapter_meta.value("themis.base_model.name", config.training.base_model);
        if (!description.empty()) {
            info.description = description;
            info.metadata.description = description;
        }
        info.metadata.adapter_id = adapter_id;
        info.metadata.version = version;
        info.metadata.base_model = info.base_model;
        info.metadata.training_samples = static_cast<int>(samples);
        info.metadata.updated_at = now;
        if (const auto a = art.adapter_meta.find("artifact"); a != art.adapter_meta.end() && a->is_object()) {
            info.metadata.checksum = a->value("gguf_sha256", std::string{});
        }
        info.metadata.custom_metadata = art.adapter_meta;
        adapters[adapter_id] = std::move(info);
        versions[adapter_id].push_back(version);
    }

    void runJob(WorkItem& item);
    void enqueue(WorkItem item);
    void workerLoop();
    void shutdown();
    std::string submitTraining(const std::string& adapter_id, const TrainingData& data,
                               const LoRAHyperparameters& hp, bool async, json job_metadata,
                               const TrainingDefaults* defaults_override = nullptr,
                               std::optional<TrainingSnapshotFingerprint> snapshot = std::nullopt,
                               std::optional<std::string> snapshot_lease = std::nullopt);
    std::string finalizeProvenance(WorkItem& item, const VerifiedPackage& pkg, StoredArtifact& art,
                                   const TrainingJobResult& result);
};

void LoRAOrchestrator::Impl::enqueue(WorkItem item) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        if (!stopping) {
            if (workers.empty()) {
                const int n = std::max(1, config.max_concurrent_jobs);
                workers.reserve(static_cast<size_t>(n));
                for (int i = 0; i < n; ++i) {
                    workers.emplace_back([this] { workerLoop(); });
                }
            }
            queue.push_back(std::move(item));
            queue_cv.notify_one();
            return;
        }
    }
    finishJob(item.job_id, JobStatus::Cancelled, "orchestrator shutting down", json::object());
}

void LoRAOrchestrator::Impl::workerLoop() {
    for (;;) {
        WorkItem item;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            queue_cv.wait(lock, [this] { return stopping || !queue.empty(); });
            if (stopping) {
                return;
            }
            item = std::move(queue.front());
            queue.pop_front();
        }
        runJob(item);
    }
}

void LoRAOrchestrator::Impl::shutdown() {
    std::deque<WorkItem> pending;
    std::vector<std::thread> to_join;
    {
        std::lock_guard<std::mutex> lock(queue_mutex);
        stopping = true;
        pending.swap(queue);
        to_join.swap(workers);
    }
    {
        std::unique_lock<std::shared_mutex> lock(state_mutex);
        for (auto& [id, flag] : cancel_flags) {
            flag->store(true);
        }
        for (const auto& w : pending) {
            if (w.snapshot_lease && snapshot_reader) {
                (void)snapshot_reader->releasePin(*w.snapshot_lease);
            }
            auto it = jobs.find(w.job_id);
            if (it != jobs.end() && !isTerminal(it->second.status)) {
                it->second.status = JobStatus::Cancelled;
                it->second.error_message = "orchestrator shutting down";
                it->second.updated_at = Clock::now();
            }
        }
    }
    queue_cv.notify_all();
    job_cv.notify_all();
    for (auto& t : to_join) {
        if (t.joinable()) {
            t.join();
        }
    }
}

void LoRAOrchestrator::Impl::runJob(WorkItem& item) {
    const std::string& job_id = item.job_id;
    const std::string& adapter_id = item.request.adapter_id;
    const std::atomic<bool>& cancel = *item.cancel;

    // A snapshot pin is kept only for a published adapter; every other exit
    // (failure, cancellation) releases it.
    struct LeaseGuard {
        Impl* impl;
        const std::optional<std::string>& lease;
        bool keep = false;
        ~LeaseGuard() {
            if (!keep) {
                impl->releaseLease(lease);
            }
        }
    } lease_guard{this, item.snapshot_lease};

    std::shared_ptr<ILoRATrainingBackend> be;
    {
        std::unique_lock<std::shared_mutex> lock(state_mutex);
        auto it = jobs.find(job_id);
        if (it == jobs.end() || isTerminal(it->second.status)) {
            cancel_flags.erase(job_id);
            return;
        }
        if (cancel.load()) {
            it->second.status = JobStatus::Cancelled;
            it->second.error_message = "cancelled";
            it->second.updated_at = Clock::now();
            cancel_flags.erase(job_id);
            lock.unlock();
            job_cv.notify_all();
            return;
        }
        it->second.status = JobStatus::Running;
        it->second.updated_at = Clock::now();
        be = backend;
    }
    job_cv.notify_all();
    if (!be) {
        failTraining(job_id, adapter_id, "no training backend configured");
        return;
    }
    const json started = {{"job_id", job_id}, {"version", item.version}, {"backend", be->name()}};
    fire(EventType::JobStarted, adapter_id, started);
    fire(EventType::TrainingStarted, adapter_id, started);

    auto on_progress = [this, &job_id](const TrainingProgress& p) {
        {
            std::unique_lock<std::shared_mutex> lock(state_mutex);
            auto it = jobs.find(job_id);
            if (it != jobs.end() && it->second.status == JobStatus::Running) {
                it->second.progress = std::clamp(p.progress, 0.0f, 1.0f);
                it->second.updated_at = Clock::now();
                json prog = {{"phase", p.phase},
                             {"step", p.step},
                             {"total_steps", p.total_steps},
                             {"train_loss", p.train_loss}};
                if (p.eval_loss >= 0.0) {
                    prog["eval_loss"] = p.eval_loss;
                }
                it->second.metadata["progress"] = std::move(prog);
            }
        }
        job_cv.notify_all();
    };

    TrainingJobResult result;
    try {
        result = be->train(item.request, on_progress, cancel);
    } catch (const std::exception& e) {
        result = TrainingJobResult{};
        result.error = std::string("training backend threw: ") + e.what();
    } catch (...) {
        result = TrainingJobResult{};
        result.error = "training backend threw a non-standard exception";
    }

    if (cancel.load() || (!result.success && result.error == "cancelled")) {
        finishJob(job_id, JobStatus::Cancelled, "cancelled", json::object());
        return;
    }
    if (!result.success) {
        failTraining(job_id, adapter_id, result.error.empty() ? "training failed (no error reported)" : result.error);
        return;
    }

    // ── Publish: a job is Completed only with a validated, stored artifact ──
    if (result.gguf_adapter_path.empty()) {
        failTraining(job_id, adapter_id, "backend reported success without a GGUF adapter");
        return;
    }
    std::string safetensors_src;
    if (result.raw_report.is_object()) {
        safetensors_src = result.raw_report.value("safetensors_path", std::string{});
    }
    if (safetensors_src.empty()) {
        failTraining(job_id, adapter_id,
                     "backend reported success without a safetensors distribution package "
                     "(raw_report.safetensors_path)");
        return;
    }
    const VerifiedPackage pkg = verifyPackage(safetensors_src, adapter_id, item.defaults);
    if (!pkg.ok) {
        failTraining(job_id, adapter_id, "trained package rejected: " + pkg.error);
        return;
    }
    StoredArtifact art = storePackage(item.defaults.artifact_root, adapter_id, item.version, pkg);
    if (!art.ok) {
        failTraining(job_id, adapter_id, art.error);
        return;
    }
    art.gguf = art.version_dir / kStoredGguf;
    {
        const std::string prov_error = finalizeProvenance(item, pkg, art, result);
        if (!prov_error.empty()) {
            std::error_code ec;
            fs::remove_all(art.version_dir, ec);
            failTraining(job_id, adapter_id, prov_error);
            return;
        }
    }
    const json metrics = {{"train_loss", result.train_loss},
                          {"eval_loss", result.eval_loss},
                          {"eval_perplexity", result.eval_perplexity},
                          {"baseline_eval_loss", result.baseline_eval_loss},
                          {"train_samples", result.train_samples},
                          {"eval_samples", result.eval_samples},
                          {"rank_pattern", result.rank_pattern}};
    json artifact_extra = {{"metrics", metrics}, {"backend", be->name()}};
    if (item.snapshot_lease) {
        artifact_extra["snapshot_lease_id"] = *item.snapshot_lease;
    }
    if (!finalizeArtifact(art, pkg, artifact_extra)) {
        failTraining(job_id, adapter_id, art.error);
        return;
    }
    lease_guard.keep = true;

    std::string reg_error;
    const bool registered = registerArtifact(adapter_id, art, reg_error);
    if (!registered) {
        spdlog::warn("LoRAOrchestrator: adapter '{}' {} published but not registered: {}",
                     adapter_id, item.version, reg_error);
    }
    commitAdapter(adapter_id, item.version, art, item.request.hyperparameters,
                  item.request.data.samples.size(), registered, {});

    json done = {{"version", item.version},
                 {"metrics", metrics},
                 {"artifact", art.adapter_meta["artifact"]},
                 {"registered", registered}};
    if (!registered) {
        done["registration_error"] = reg_error;
    }
    finishJob(job_id, JobStatus::Completed, {}, done);
    done["job_id"] = job_id;
    fire(EventType::TrainingCompleted, adapter_id, done);
    fire(EventType::JobCompleted, adapter_id, done);
    if (registered) {
        fire(EventType::AdapterLoaded, adapter_id, done);
    }
}

/**
 * Embed the training-snapshot fingerprint and the provenance signature into the
 * stored safetensors package and produce the GGUF at art.gguf.
 *
 * Without snapshot and signer the backend's GGUF is copied unchanged.
 * Otherwise: re-read the snapshot (fail if the Merkle root changed since
 * capture) → merge themis.training.snapshot.* → sign (canonical JSON incl.
 * themis.adapter.sha256_tensors) → rewrite the stored safetensors header →
 * self-verify → regenerate the GGUF from the signed package via the converter
 * (which passes all themis.* header keys through to the GGUF KV store).
 * @return Empty string on success, otherwise the error.
 */
std::string LoRAOrchestrator::Impl::finalizeProvenance(WorkItem& item, const VerifiedPackage& pkg,
                                                      StoredArtifact& art,
                                                      const TrainingJobResult& result) {
    std::shared_ptr<TrainingSnapshotReader> reader;
    std::shared_ptr<AdapterProvenanceSigner> sig;
    std::shared_ptr<AdapterProvenanceVerifier> ver;
    AdapterSignaturePolicy policy;
    AdapterConverter conv;
    {
        std::shared_lock<std::shared_mutex> lock(state_mutex);
        reader = snapshot_reader;
        sig = signer;
        ver = verifier;
        policy = signature_policy;
        conv = converter;
    }
    if (!sig && policy.require_signed_training) {
        return "adapter signing is required by policy but no signer is configured";
    }
    std::error_code ec;
    if (!item.snapshot && !sig) {
        fs::copy_file(result.gguf_adapter_path, art.gguf, fs::copy_options::none, ec);
        return ec ? "cannot store GGUF adapter: " + result.gguf_adapter_path : std::string{};
    }

    std::map<std::string, std::string> md = pkg.inspection.metadata;
    if (item.snapshot) {
        if (!reader) {
            return "training snapshot reader no longer configured; cannot re-verify snapshot";
        }
        // Detects writes that were in flight (HLC ts <= snapshot ts) at capture time.
        const TrainingSnapshotRead check = reader->verify(*item.snapshot, /*acquire_pin=*/false, {});
        if (!check.ok) {
            return "training snapshot changed during the job; adapter not published: " + check.error;
        }
        for (const auto& [k, v] : item.snapshot->toMetadata()) {
            md[k] = v;
        }
    }
    std::string err;
    if (sig) {
        if (!sig->sign(md, &err)) {
            return "adapter provenance signing failed: " + err;
        }
    }
    if (!rewriteSafetensorsMetadata(art.safetensors, md, &err)) {
        return "cannot embed provenance into safetensors package: " + err;
    }
    if (sig) {
        if (!ver) {
            if (policy.require_signed_training) {
                return "no provenance verifier configured to self-check the signed adapter";
            }
        } else {
            const ProvenanceVerification self = ver->verifySafetensorsFile(art.safetensors);
            if (self.status != ProvenanceVerification::Status::Verified) {
                return "signed adapter failed provenance self-verification: " + self.error;
            }
        }
    }
    if (!conv) {
        return "no safetensors->GGUF converter configured; cannot embed provenance into GGUF";
    }
    json themis_md = json::object();
    for (const auto& [k, v] : md) {
        if (k.rfind("themis.", 0) == 0) {
            themis_md[k] = v;
        }
    }
    TrainingJobResult conv_result;
    try {
        conv_result = conv(art.version_dir.string(), themis_md, art.gguf.string());
    } catch (const std::exception& e) {
        conv_result = TrainingJobResult{};
        conv_result.error = std::string("converter threw: ") + e.what();
    }
    if (conv_result.success && !conv_result.gguf_adapter_path.empty() &&
        fs::path(conv_result.gguf_adapter_path) != art.gguf) {
        fs::copy_file(conv_result.gguf_adapter_path, art.gguf, fs::copy_options::overwrite_existing, ec);
    }
    if (!conv_result.success || ec) {
        return "GGUF regeneration from signed package failed: " +
               (conv_result.error.empty() ? (ec ? ec.message() : std::string("unknown error"))
                                          : conv_result.error);
    }
    for (auto it = themis_md.begin(); it != themis_md.end(); ++it) {
        art.adapter_meta[it.key()] = it.value();
    }
    if (item.snapshot) {
        art.adapter_meta["training_snapshot"] = {{"verified_at", isoUtcNow()},
                                                 {"pinned", item.snapshot->pinned}};
    }
    return {};
}

std::string LoRAOrchestrator::Impl::submitTraining(const std::string& adapter_id,
                                                  const TrainingData& data,
                                                  const LoRAHyperparameters& hp,
                                                  bool async,
                                                  json job_metadata,
                                                  const TrainingDefaults* defaults_override,
                                                  std::optional<TrainingSnapshotFingerprint> snapshot,
                                                  std::optional<std::string> snapshot_lease) {
    JobInfo job;
    job.job_id = makeJobId("train");
    job.type = JobType::Training;
    job.status = JobStatus::Pending;
    job.adapter_id = adapter_id;
    job.started_at = Clock::now();
    job.updated_at = job.started_at;
    job.metadata = job_metadata.is_object() ? std::move(job_metadata) : json::object();
    job.metadata["dataset_id"] = data.dataset_name;
    job.metadata["samples"] = data.samples.size();

    std::string error;
    if (!isValidAdapterId(adapter_id)) {
        error = "invalid adapter_id: must match [A-Za-z0-9_.-]{1,128} and not be '.' or '..'";
    } else if (data.samples.size() < 2) {
        error = "at least 2 training samples are required (train/holdout split)";
    }

    WorkItem item;
    {
        std::unique_lock<std::shared_mutex> lock(state_mutex);
        if (error.empty() && !backend) {
            error = "no training backend configured";
        }
        if (error.empty() && signature_policy.require_signed_training && !signer) {
            error = "adapter signing is required by policy but no signer is configured";
        }
        if (error.empty() && (snapshot || signer) && !converter) {
            error = "a safetensors->GGUF converter is required to embed snapshot/provenance metadata";
        }
        if (error.empty()) {
            item.version = reserveVersion(adapter_id);
            item.cancel = std::make_shared<std::atomic<bool>>(false);
            cancel_flags[job.job_id] = item.cancel;
            job.metadata["version"] = item.version;
            job.metadata["method"] = config.training.method == AdapterTrainingMethod::AdaLoRA ? "adalora" : "lora";
        } else {
            job.status = JobStatus::Failed;
            job.error_message = error;
        }
        jobs[job.job_id] = job;
    }
    job_cv.notify_all();

    if (!error.empty()) {
        releaseLease(snapshot_lease);
        spdlog::warn("LoRAOrchestrator: training job {} for adapter '{}' rejected: {}", job.job_id,
                     adapter_id, error);
        const json payload = {{"job_id", job.job_id}, {"error", error}};
        fire(EventType::TrainingFailed, adapter_id, payload);
        fire(EventType::JobFailed, adapter_id, payload);
        return job.job_id;
    }

    item.defaults = defaults_override ? *defaults_override : config.training;
    item.snapshot = std::move(snapshot);
    item.snapshot_lease = std::move(snapshot_lease);
    const auto& d = item.defaults;
    item.job_id = job.job_id;
    TrainingJobRequest& req = item.request;
    req.job_id = job.job_id;
    req.adapter_id = adapter_id;
    req.data = data;
    if (!req.data.metadata.is_object()) {
        req.data.metadata = json::object();
    }
    // Passed to the backend for embedding into safetensors __metadata__ / GGUF KV.
    req.data.metadata["themis.adapter.version"] = item.version;
    req.data.metadata["themis.training.dataset_id"] = data.dataset_name;
    req.data.metadata["themis.base_model.sha256"] = d.base_model_gguf_sha256;
    req.hyperparameters = hp;
    req.method = d.method;
    req.adalora = d.adalora;
    req.base_model = d.base_model;
    req.base_model_gguf = d.base_model_gguf;
    req.output_dir = (fs::path(d.work_root) / adapter_id / job.job_id).string();
    req.holdout_fraction = d.holdout_fraction;
    req.seed = d.seed;
    req.require_improvement = d.require_improvement;

    fire(EventType::JobQueued, adapter_id, json{{"job_id", job.job_id}, {"version", item.version}});
    if (async) {
        enqueue(std::move(item));
    } else {
        runJob(item);
    }
    return job.job_id;
}

LoRAOrchestrator::LoRAOrchestrator(const Config& config) : impl_(std::make_unique<Impl>(config)) {
    // Initialize storage service using provided config
    impl_->storage_service = std::make_shared<LoRAStorageService>(config.storage_config);

    // Initialize consistency checker
    AdapterConsistencyChecker::Config checker_config;
    checker_config.enable_checksums = true;
    checker_config.enable_signatures = true;
    impl_->consistency_checker = std::make_shared<AdapterConsistencyChecker>(checker_config);

    impl_->is_initialized = true;
    spdlog::info("LoRA Orchestrator initialized with storage and consistency checker");
}

LoRAOrchestrator::LoRAOrchestrator() : LoRAOrchestrator(Config{}) {}

LoRAOrchestrator::~LoRAOrchestrator() = default;

const LoRAOrchestrator::Config& LoRAOrchestrator::getConfig() const noexcept {
    return impl_->config;
}

void LoRAOrchestrator::setTrainingBackend(std::shared_ptr<ILoRATrainingBackend> backend) {
    std::unique_lock<std::shared_mutex> lock(impl_->state_mutex);
    impl_->backend = std::move(backend);
}

void LoRAOrchestrator::setAdapterRegistrar(AdapterRegistrar registrar) {
    std::unique_lock<std::shared_mutex> lock(impl_->state_mutex);
    impl_->registrar = std::move(registrar);
}

void LoRAOrchestrator::setMultiLoRAManager(std::shared_ptr<MultiLoRAManager> manager) {
    std::unique_lock<std::shared_mutex> lock(impl_->state_mutex);
    impl_->lora_manager = manager;
    if (!manager) {
        impl_->registrar = nullptr;
        return;
    }
    impl_->installHotLoadVerifier(manager);
    const std::string configured_base = impl_->config.training.base_model_gguf;
    std::weak_ptr<MultiLoRAManager> weak = manager;
    impl_->registrar = [weak, configured_base](const std::string& adapter_id,
                                               const std::string& gguf_path,
                                               const json& adapter_metadata,
                                               std::string& error) {
        auto mgr = weak.lock();
        if (!mgr) {
            error = "MultiLoRAManager no longer available";
            return false;
        }
        // Replace a previously loaded version: loadLoRA() treats an existing id as a cache hit.
        (void)mgr->unloadLoRA(adapter_id, /*force=*/true);
        const std::string base = adapter_metadata.value("themis.base_model.gguf", configured_base);
        if (!mgr->loadLoRA(adapter_id, gguf_path, base)) {
            error = "MultiLoRAManager::loadLoRA failed for " + gguf_path;
            return false;
        }
        return true;
    };
}

void LoRAOrchestrator::setAdapterConverter(AdapterConverter converter) {
    std::unique_lock<std::shared_mutex> lock(impl_->state_mutex);
    impl_->converter = std::move(converter);
}

void LoRAOrchestrator::setTrainingSnapshotReader(std::shared_ptr<TrainingSnapshotReader> reader) {
    std::unique_lock<std::shared_mutex> lock(impl_->state_mutex);
    impl_->snapshot_reader = std::move(reader);
}

void LoRAOrchestrator::setAdapterSigner(std::shared_ptr<AdapterProvenanceSigner> signer) {
    std::unique_lock<std::shared_mutex> lock(impl_->state_mutex);
    impl_->signer = std::move(signer);
}

void LoRAOrchestrator::setAdapterProvenanceVerifier(std::shared_ptr<AdapterProvenanceVerifier> verifier) {
    std::unique_lock<std::shared_mutex> lock(impl_->state_mutex);
    impl_->verifier = std::move(verifier);
    impl_->installHotLoadVerifier(impl_->lora_manager);
}

void LoRAOrchestrator::setAdapterSignaturePolicy(const AdapterSignaturePolicy& policy) {
    std::unique_lock<std::shared_mutex> lock(impl_->state_mutex);
    impl_->signature_policy = policy;
    impl_->installHotLoadVerifier(impl_->lora_manager);
}

AdapterSignaturePolicy LoRAOrchestrator::getAdapterSignaturePolicy() const {
    std::shared_lock<std::shared_mutex> lock(impl_->state_mutex);
    return impl_->signature_policy;
}

bool LoRAOrchestrator::releaseSnapshotPin(const std::string& lease_id) {
    std::shared_ptr<TrainingSnapshotReader> reader;
    {
        std::shared_lock<std::shared_mutex> lock(impl_->state_mutex);
        reader = impl_->snapshot_reader;
    }
    return reader && reader->releasePin(lease_id);
}

namespace {
/// Convert snapshot rows into TrainingData (same rules as the AQL dataset path).
DatasetLoadResult snapshotRowsToData(const TrainingSnapshotRead& snap) {
    return LoRADatasetLoader::fromRows(snap.rows, snap.fingerprint.collection,
                                       static_cast<std::size_t>(snap.fingerprint.record_count));
}

json snapshotJobMetadata(const TrainingSnapshotFingerprint& fp, const std::optional<std::string>& lease) {
    json j = {{"collection", fp.collection},
              {"hlc_ts", fp.hlc_ts},
              {"record_count", fp.record_count},
              {"merkle_root", fp.merkle_root},
              {"query_sha256", fp.query_sha256},
              {"pinned", fp.pinned}};
    if (lease) {
        j["snapshot_lease_id"] = *lease;
    }
    return j;
}
} // namespace

std::string LoRAOrchestrator::createAdapterFromSnapshot(
    const std::string& adapter_id,
    const std::string& collection,
    std::size_t limit,
    const std::optional<LoRAHyperparameters>& hyperparameters,
    bool async) {
    json md = {{"operation", "create_from_snapshot"}, {"collection", collection}};
    if (!isValidAdapterId(adapter_id)) {
        return impl_->rejectJob(adapter_id, "invalid adapter_id", md);
    }
    std::shared_ptr<TrainingSnapshotReader> reader;
    TrainingDefaults defaults;
    {
        std::shared_lock<std::shared_mutex> lock(impl_->state_mutex);
        reader = impl_->snapshot_reader;
        defaults = impl_->config.training;
    }
    if (!reader) {
        return impl_->rejectJob(adapter_id, "no training snapshot reader configured", md);
    }
    SnapshotSelector sel;
    sel.collection = collection;
    sel.limit = limit;
    TrainingSnapshotRead snap =
        reader->capture(sel, defaults.seed, defaults.holdout_fraction, "lora:" + adapter_id);
    if (!snap.ok) {
        return impl_->rejectJob(adapter_id, "training snapshot capture failed: " + snap.error, md);
    }
    DatasetLoadResult data = snapshotRowsToData(snap);
    if (!data.success) {
        impl_->releaseLease(snap.lease_id);
        return impl_->rejectJob(adapter_id, "training snapshot unusable: " + data.error, md);
    }
    md["snapshot"] = snapshotJobMetadata(snap.fingerprint, snap.lease_id);
    return impl_->submitTraining(adapter_id, data.data, hyperparameters.value_or(LoRAHyperparameters{}),
                                 async, md, &defaults, snap.fingerprint, snap.lease_id);
}

std::string LoRAOrchestrator::regenerateAdapter(
    const std::string& source_adapter_id,
    const RegenerationTarget& target,
    const std::optional<LoRAHyperparameters>& hyperparameters,
    bool async) {
    json md = {{"operation", "regenerate"}, {"source_adapter_id", source_adapter_id},
               {"base_model", target.base_model}, {"base_model_gguf", target.base_model_gguf}};
    const std::string& adapter_id = target.adapter_id;
    if (!isValidAdapterId(adapter_id)) {
        return impl_->rejectJob(adapter_id, "invalid target adapter_id", md);
    }
    std::shared_ptr<TrainingSnapshotReader> reader;
    TrainingDefaults defaults;
    AdapterSignaturePolicy policy;
    std::string source_safetensors;
    {
        std::shared_lock<std::shared_mutex> lock(impl_->state_mutex);
        reader = impl_->snapshot_reader;
        defaults = impl_->config.training;
        policy = impl_->signature_policy;
        auto it = impl_->adapters.find(source_adapter_id);
        if (it != impl_->adapters.end()) {
            const json& cm = it->second.metadata.custom_metadata;
            if (cm.is_object() && cm.contains("artifact") && cm["artifact"].is_object()) {
                source_safetensors = cm["artifact"].value("safetensors_path", std::string{});
            }
        }
    }
    if (source_safetensors.empty()) {
        return impl_->rejectJob(adapter_id, "source adapter '" + source_adapter_id +
                                                "' has no stored safetensors package", md);
    }
    if (!reader) {
        return impl_->rejectJob(adapter_id, "no training snapshot reader configured", md);
    }
    const SafetensorsInspection insp = SafetensorsPackageValidator::inspectFile(source_safetensors);
    if (!insp.valid) {
        return impl_->rejectJob(adapter_id, "source adapter package invalid: " + insp.error, md);
    }
    const std::string tensors_sha = SafetensorsPackageValidator::sha256OfTensorData(source_safetensors, insp);
    if (tensors_sha.empty() || tensors_sha != toLowerCopy(metaValue(insp.metadata, provenance_keys::kTensorSha256))) {
        return impl_->rejectJob(adapter_id, "source adapter tensor data does not match its metadata", md);
    }
    const ProvenanceVerification prov = impl_->verifyMetadataProvenance(insp.metadata);
    if (!prov.acceptable(policy.require_signed_imports)) {
        return impl_->rejectJob(adapter_id, "source adapter provenance rejected: " + prov.error, md);
    }
    std::string err;
    auto fp = TrainingSnapshotFingerprint::fromMetadata(insp.metadata, &err);
    if (!fp) {
        return impl_->rejectJob(adapter_id, "source adapter has no usable training snapshot reference: " + err, md);
    }
    md["source_provenance"] = prov.status == ProvenanceVerification::Status::Verified ? "verified" : "unsigned";
    // Pin first, then re-read at hlc_ts and compare: abort BEFORE training on any difference.
    TrainingSnapshotRead snap = reader->verify(*fp, /*acquire_pin=*/true, "lora:" + adapter_id);
    if (!snap.ok) {
        return impl_->rejectJob(adapter_id, "regeneration aborted before training: " + snap.error, md);
    }
    DatasetLoadResult data = snapshotRowsToData(snap);
    if (!data.success) {
        impl_->releaseLease(snap.lease_id);
        return impl_->rejectJob(adapter_id, "training snapshot unusable: " + data.error, md);
    }
    defaults.base_model = target.base_model;
    defaults.base_model_gguf = target.base_model_gguf;
    defaults.base_model_gguf_sha256 = target.base_model_gguf_sha256;
    defaults.seed = fp->split_seed;
    defaults.holdout_fraction = fp->holdout_fraction;
    TrainingSnapshotFingerprint carried = *fp;
    carried.pinned = snap.lease_id.has_value();
    md["snapshot"] = snapshotJobMetadata(carried, snap.lease_id);
    return impl_->submitTraining(adapter_id, data.data, hyperparameters.value_or(LoRAHyperparameters{}),
                                 async, md, &defaults, carried, snap.lease_id);
}

json LoRAOrchestrator::JobInfo::toJSON() const {
    json j;
    j["job_id"] = job_id;
    j["adapter_id"] = adapter_id;
    j["progress"] = progress;
    j["error_message"] = error_message;
    j["type"] = static_cast<int>(type);
    j["status"] = static_cast<int>(status);
    j["started_at"] = std::chrono::duration_cast<std::chrono::seconds>(started_at.time_since_epoch()).count();
    j["updated_at"] = std::chrono::duration_cast<std::chrono::seconds>(updated_at.time_since_epoch()).count();
    j["metadata"] = metadata;
    return j;
}

/**
 * @brief Submit a real training job for a new adapter (see header).
 */
std::string LoRAOrchestrator::createAdapter(
    const std::string& adapter_id,
    const TrainingData& training_data,
    const std::optional<LoRAHyperparameters>& hyperparameters,
    bool async) {
    return impl_->submitTraining(adapter_id, training_data,
                                 hyperparameters.value_or(LoRAHyperparameters{}), async,
                                 json{{"operation", "create"}});
}

/**
 * @brief Create Adapter Batch.
 * @param[in] adapter_id Identifier of the adapter.
 * @param[in] datasets Input parameter.
 * @param[in] hyperparameters Input parameter.
 * @param[in] async Input parameter.
 * @return Return value.
 * @details Calls: insert(), end(), begin(), createAdapter().
 */
std::string LoRAOrchestrator::createAdapterBatch(
    const std::string& adapter_id,
    const std::vector<TrainingData>& datasets,
    const std::optional<LoRAHyperparameters>& hyperparameters,
    bool async) {
    TrainingData combined;
    for (const auto& ds : datasets) {
        combined.samples.insert(combined.samples.end(), ds.samples.begin(), ds.samples.end());
        if (!ds.dataset_name.empty()) {
            combined.dataset_name += (combined.dataset_name.empty() ? "" : ",") + ds.dataset_name;
        }
    }
    return createAdapter(adapter_id, combined, hyperparameters, async);
}

/**
 * @brief Import an external safetensors adapter package (see header).
 */
bool LoRAOrchestrator::importAdapter(
    const std::string& adapter_id,
    const std::string& source_path,
    const AdapterMetadata& metadata) {
    const ImportResult r = importAdapterPackage(adapter_id, source_path);
    if (!r.success) {
        spdlog::warn("LoRAOrchestrator::importAdapter('{}') rejected: {}", adapter_id, r.error);
        return false;
    }
    if (!metadata.description.empty()) {
        std::unique_lock<std::shared_mutex> lock(impl_->state_mutex);
        auto it = impl_->adapters.find(adapter_id);
        if (it != impl_->adapters.end()) {
            it->second.description = metadata.description;
            it->second.metadata.description = metadata.description;
        }
    }
    return true;
}

LoRAOrchestrator::ImportResult LoRAOrchestrator::importAdapterPackage(
    const std::string& adapter_id,
    const std::string& source_path) {
    ImportResult r;
    if (!isValidAdapterId(adapter_id)) {
        r.error = "invalid adapter_id";
        return r;
    }
    AdapterConverter conv;
    {
        std::shared_lock<std::shared_mutex> lock(impl_->state_mutex);
        conv = impl_->converter;
    }
    if (!conv) {
        r.error = "no safetensors->GGUF converter configured";
        return r;
    }
    const VerifiedPackage pkg = verifyPackage(source_path, adapter_id, impl_->config.training);
    if (!pkg.ok) {
        r.error = pkg.error;
        return r;
    }
    {
        // Provenance: signature over the canonical themis.* payload incl. the
        // (already re-hashed) tensor digest; fail closed per policy.
        bool require_signed = true;
        {
            std::shared_lock<std::shared_mutex> lock(impl_->state_mutex);
            require_signed = impl_->signature_policy.require_signed_imports;
        }
        const ProvenanceVerification prov = impl_->verifyMetadataProvenance(pkg.inspection.metadata);
        if (!prov.acceptable(require_signed)) {
            r.error = "adapter provenance rejected: " + prov.error;
            return r;
        }
        const bool has_snapshot = pkg.inspection.metadata.count(snapshot_keys::kMerkleRoot) != 0;
        std::string snap_err;
        if (has_snapshot && !TrainingSnapshotFingerprint::fromMetadata(pkg.inspection.metadata, &snap_err)) {
            r.error = "malformed training snapshot metadata: " + snap_err;
            return r;
        }
    }
    {
        std::unique_lock<std::shared_mutex> lock(impl_->state_mutex);
        r.version = impl_->reserveVersion(adapter_id);
    }
    StoredArtifact art = storePackage(impl_->config.training.artifact_root, adapter_id, r.version, pkg);
    if (!art.ok) {
        r.error = art.error;
        return r;
    }
    art.gguf = art.version_dir / kStoredGguf;

    TrainingJobResult conv_result;
    try {
        conv_result = conv(art.version_dir.string(), art.adapter_meta, art.gguf.string());
    } catch (const std::exception& e) {
        conv_result = TrainingJobResult{};
        conv_result.error = std::string("converter threw: ") + e.what();
    }
    std::error_code ec;
    if (conv_result.success && !conv_result.gguf_adapter_path.empty() &&
        fs::path(conv_result.gguf_adapter_path) != art.gguf) {
        fs::copy_file(conv_result.gguf_adapter_path, art.gguf, fs::copy_options::overwrite_existing, ec);
    }
    if (!conv_result.success || ec) {
        r.error = "GGUF conversion failed: " +
                  (conv_result.error.empty() ? (ec ? ec.message() : std::string("unknown error"))
                                             : conv_result.error);
        fs::remove_all(art.version_dir, ec);
        return r;
    }
    const std::string source_version = metaValue(pkg.inspection.metadata, "themis.adapter.version");
    if (!finalizeArtifact(art, pkg, json{{"import", {{"source_path", source_path},
                                                     {"source_version", source_version}}}})) {
        r.error = art.error;
        return r;
    }

    r.registered = impl_->registerArtifact(adapter_id, art, r.registration_error);
    LoRAHyperparameters hp;
    const std::string rank = metaValue(pkg.inspection.metadata, "themis.adapter.rank");
    const std::string alpha = metaValue(pkg.inspection.metadata, "themis.adapter.alpha");
    try {
        if (!rank.empty()) {
            hp.rank = std::stoi(rank);
        }
        if (!alpha.empty()) {
            hp.alpha = std::stof(alpha);
        }
    } catch (const std::exception&) {
        spdlog::warn("LoRAOrchestrator: non-numeric rank/alpha metadata in imported adapter '{}'", adapter_id);
    }
    std::size_t samples = 0;
    try {
        const std::string s = metaValue(pkg.inspection.metadata, "themis.training.train_samples");
        samples = s.empty() ? 0 : static_cast<std::size_t>(std::stoull(s));
    } catch (const std::exception&) {
        samples = 0;
    }
    impl_->commitAdapter(adapter_id, r.version, art, hp, samples, r.registered, {});
    r.success = true;
    const json payload = {{"version", r.version}, {"registered", r.registered},
                          {"artifact", art.adapter_meta["artifact"]}};
    impl_->fire(EventType::VersionCreated, adapter_id, payload);
    if (r.registered) {
        impl_->fire(EventType::AdapterLoaded, adapter_id, payload);
    }
    return r;
}

std::optional<AdapterInfo> LoRAOrchestrator::getAdapter(const std::string& adapter_id) const {
    std::shared_lock<std::shared_mutex> lock(impl_->state_mutex);
    auto it = impl_->adapters.find(adapter_id);
    if (it != impl_->adapters.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<AdapterInfo> LoRAOrchestrator::listAdapters(
    const std::optional<std::string>& filter) const {
    std::shared_lock<std::shared_mutex> lock(impl_->state_mutex);
    std::vector<AdapterInfo> out = {};

    out.reserve(impl_->adapters.size());
    for (const auto& kv : impl_->adapters) {
        if (!filter || kv.second.base_model == *filter) {
            out.push_back(kv.second);
        }
    }
    return out;
}

bool LoRAOrchestrator::exists(const std::string& adapter_id) const {
    std::shared_lock<std::shared_mutex> lock(impl_->state_mutex);
    return impl_->adapters.count(adapter_id) > 0;
}

bool LoRAOrchestrator::isLoaded(const std::string& adapter_id) const {
    std::shared_lock<std::shared_mutex> lock(impl_->state_mutex);
    auto it = impl_->adapters.find(adapter_id);
    return it != impl_->adapters.end() && it->second.is_loaded;
}

std::vector<std::string> LoRAOrchestrator::getVersions(const std::string& adapter_id) const {
    std::shared_lock<std::shared_mutex> lock(impl_->state_mutex);
    auto it = impl_->versions.find(adapter_id);
    if (it != impl_->versions.end()) {
        return it->second;
    }
    return {};
}

std::string LoRAOrchestrator::getCurrentVersion(const std::string& adapter_id) const {
    std::shared_lock<std::shared_mutex> lock(impl_->state_mutex);
    auto it = impl_->adapters.find(adapter_id);
    if (it != impl_->adapters.end()) {
        return it->second.version;
    }
    return {};
}

std::vector<AdapterInfo> LoRAOrchestrator::searchAdapters(const json& /*criteria*/) const {
    return listAdapters();
}

/**
 * @brief Update Adapter.
 * @param[in] adapter_id Identifier of the adapter.
 * @param[in] training_data Input parameter.
 * @param[in] bool Input parameter.
 * @param[in] async Input parameter.
 * @return Return value.
 * @details Calls: lock(), count(), makeAdapterInfo(), push_back(), size(), Clock::now(), makeJobId().
 */
std::string LoRAOrchestrator::updateAdapter(
    const std::string& adapter_id,
    const TrainingData& training_data,
    bool /*incremental*/,
    bool async) {
    std::unique_lock<std::shared_mutex> lock(impl_->state_mutex);
    if (!impl_->adapters.count(adapter_id)) {
        impl_->adapters[adapter_id] = makeAdapterInfo(adapter_id, "v1.0");
        impl_->versions[adapter_id].push_back("v1.0");
    }

    AdapterInfo& info = impl_->adapters[adapter_id];
    info.metadata.training_samples += training_data.samples.size();
    info.metadata.updated_at = Clock::now();

    JobInfo job;
    job.job_id = makeJobId("update");
    job.type = JobType::Training;
    job.status = async ? JobStatus::Running : JobStatus::Completed;
    job.adapter_id = adapter_id;
    job.progress = async ? 0.1f : 1.0f;
    job.started_at = Clock::now();
    job.updated_at = job.started_at;
    impl_->jobs[job.job_id] = job;

    return job.job_id;
}

/**
 * @brief Update Metadata.
 * @param[in] adapter_id Identifier of the adapter.
 * @param[in] metadata Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: lock(), find(), end(), push_back().
 */
bool LoRAOrchestrator::updateMetadata(const std::string& adapter_id, const AdapterMetadata& metadata) {
    std::unique_lock<std::shared_mutex> lock(impl_->state_mutex);
    auto it = impl_->adapters.find(adapter_id);
    if (it == impl_->adapters.end()) {
        return false;
    }
    it->second.metadata = metadata;
    it->second.version = metadata.version;
    impl_->versions[adapter_id].push_back(metadata.version);
    return true;
}

/**
 * @brief Create Version.
 * @param[in] adapter_id Identifier of the adapter.
 * @param[in] description Input parameter.
 * @return Return value.
 * @details Calls: lock(), find(), end(), size(), empty(), std::to_string(), count(), makeAdapterInfo().
 */
std::string LoRAOrchestrator::createVersion(const std::string& adapter_id, const std::string& description) {
    std::unique_lock<std::shared_mutex> lock(impl_->state_mutex);
    auto it = impl_->versions.find(adapter_id);
    const size_t next_version = (it != impl_->versions.end()) ? (it->second.size() + 1) : 1;
    const std::string version = description.empty() ? "v" + std::to_string(next_version) : description;

    if (!impl_->adapters.count(adapter_id)) {
        impl_->adapters[adapter_id] = makeAdapterInfo(adapter_id, version);
    }

    impl_->versions[adapter_id].push_back(version);
    impl_->adapters[adapter_id].version = version;
    impl_->adapters[adapter_id].metadata.version = version;
    impl_->adapters[adapter_id].metadata.updated_at = Clock::now();

    return version;
}

/**
 * @brief Switch Version.
 * @param[in] adapter_id Identifier of the adapter.
 * @param[in] version Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: lock(), find(), end(), std::find(), begin(), Clock::now().
 */
bool LoRAOrchestrator::switchVersion(const std::string& adapter_id, const std::string& version) {
    std::unique_lock<std::shared_mutex> lock(impl_->state_mutex);
    auto it = impl_->adapters.find(adapter_id);
    auto ver_it = impl_->versions.find(adapter_id);
    if (it == impl_->adapters.end() || ver_it == impl_->versions.end()) {
        return false;
    }

    const auto& versions = ver_it->second;
    if (std::find(versions.begin(), versions.end(), version) == versions.end()) {
        return false;
    }

    it->second.version = version;
    it->second.metadata.version = version;
    it->second.metadata.updated_at = Clock::now();
    return true;
}

/**
 * @brief Rollback.
 * @param[in] adapter_id Identifier of the adapter.
 * @return True when the operation succeeds.
 * @details Calls: lock(), find(), end(), size(), pop_back(), back(), Clock::now().
 */
bool LoRAOrchestrator::rollback(const std::string& adapter_id) {
    std::unique_lock<std::shared_mutex> lock(impl_->state_mutex);
    auto it = impl_->versions.find(adapter_id);
    if (it == impl_->versions.end() || it->second.size() < 2) {
        return false;
    }

    it->second.pop_back();
    const std::string& previous = it->second.back();
    impl_->adapters[adapter_id].version = previous;
    impl_->adapters[adapter_id].metadata.version = previous;
    impl_->adapters[adapter_id].metadata.updated_at = Clock::now();
    return true;
}

/**
 * @brief Delete Adapter.
 * @param[in] adapter_id Identifier of the adapter.
 * @param[in] bool Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: lock(), erase().
 */
bool LoRAOrchestrator::deleteAdapter(const std::string& adapter_id, bool /*delete_all_versions*/) {
    std::unique_lock<std::shared_mutex> lock(impl_->state_mutex);
    size_t removed = impl_->adapters.erase(adapter_id);
    impl_->versions.erase(adapter_id);
    return removed > 0;
}

/**
 * @brief Delete Version.
 * @param[in] adapter_id Identifier of the adapter.
 * @param[in] version Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: lock(), find(), end(), std::remove(), begin(), erase(), empty(), back().
 */
bool LoRAOrchestrator::deleteVersion(const std::string& adapter_id, const std::string& version) {
    std::unique_lock<std::shared_mutex> lock(impl_->state_mutex);
    auto it = impl_->versions.find(adapter_id);
    if (it == impl_->versions.end()) {
        return false;
    }

    auto& v = it->second;
    const auto new_end = std::remove(v.begin(), v.end(), version);
    const bool removed = new_end != v.end();
    v.erase(new_end, v.end());
    if (removed && !v.empty()) {
        impl_->adapters[adapter_id].version = v.back();
        impl_->adapters[adapter_id].metadata.version = v.back();
    }
    return removed;
}

/**
 * @brief Unload Adapter.
 * @param[in] adapter_id Identifier of the adapter.
 * @param[in] bool Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: lock(), find(), end().
 */
bool LoRAOrchestrator::unloadAdapter(const std::string& adapter_id, bool /*force*/) {
    std::unique_lock<std::shared_mutex> lock(impl_->state_mutex);
    auto it = impl_->adapters.find(adapter_id);
    if (it == impl_->adapters.end()) {
        return false;
    }
    it->second.is_loaded = false;
    return true;
}

/**
 * @brief Load Adapter.
 * @param[in] adapter_id Identifier of the adapter.
 * @param[in] async Input parameter.
 * @return Return value.
 * @details Calls: lock(), count(), makeAdapterInfo(), push_back(), Clock::now(), makeJobId(), submit(), std::move().
 */
std::string LoRAOrchestrator::loadAdapter(const std::string& adapter_id, bool async) {
    std::unique_lock<std::shared_mutex> lock(impl_->state_mutex);
    if (!impl_->adapters.count(adapter_id)) {
        impl_->adapters[adapter_id] = makeAdapterInfo(adapter_id, "v1.0");
        impl_->versions[adapter_id].push_back("v1.0");
    }

    AdapterInfo& info = impl_->adapters[adapter_id];
    info.is_loaded = true;
    info.metadata.updated_at = Clock::now();

    JobInfo job;
    job.job_id = makeJobId("load");
    job.type = JobType::Loading;
    job.status = async ? JobStatus::Running : JobStatus::Completed;
    job.adapter_id = adapter_id;
    job.progress = async ? 0.1f : 1.0f;
    job.started_at = Clock::now();
    job.updated_at = job.started_at;
    impl_->jobs[job.job_id] = job;

    // Emit LOOP_TRIGGER decision record (non-blocking)
    if (impl_->dr_processor) {
        themis::llm::DecisionRecord rec;
        rec.decision_type = "LOOP_TRIGGER";
        rec.component     = "LoRAOrchestrator";
        rec.outcome       = async ? "RUNNING" : "SUCCESS";
        rec.parameters["adapter_id"] = adapter_id;
        rec.parameters["job_id"]     = job.job_id;
        rec.parameters["async"]      = async ? "true" : "false";
        impl_->dr_processor->submit(std::move(rec));
    }

    return job.job_id;
}

std::optional<LoRAOrchestrator::JobInfo> LoRAOrchestrator::getJob(const std::string& job_id) const {
    std::shared_lock<std::shared_mutex> lock(impl_->state_mutex);
    auto it = impl_->jobs.find(job_id);
    if (it != impl_->jobs.end()) {
        return it->second;
    }
    return std::nullopt;
}

std::vector<LoRAOrchestrator::JobInfo> LoRAOrchestrator::listJobs(const std::optional<JobStatus>& status) const {
    std::shared_lock<std::shared_mutex> lock(impl_->state_mutex);
    std::vector<JobInfo> jobs = {};

    for (const auto& kv : impl_->jobs) {
        if (!status || kv.second.status == *status) {
            jobs.push_back(kv.second);
        }
    }
    return jobs;
}

/**
 * @brief Cancel Job.
 * @param[in] job_id Identifier of the job.
 * @return True when the operation succeeds.
 * @details Calls: lock(), find(), end(), Clock::now().
 */
bool LoRAOrchestrator::cancelJob(const std::string& job_id) {
    std::unique_lock<std::shared_mutex> lock(impl_->state_mutex);
    auto it = impl_->jobs.find(job_id);
    if (it == impl_->jobs.end()) {
        return false;
    }
    it->second.status = JobStatus::Cancelled;
    it->second.updated_at = Clock::now();
    return true;
}

/**
 * @brief Wait For Job.
 * @param[in] job_id Identifier of the job.
 * @param[in] int Input parameter.
 * @return Return value.
 * @details Calls: lock(), find(), end().
 */
LoRAOrchestrator::JobInfo LoRAOrchestrator::waitForJob(const std::string& job_id, int /*timeout_seconds*/) {
    std::shared_lock<std::shared_mutex> lock(impl_->state_mutex);
    auto it = impl_->jobs.find(job_id);
    if (it != impl_->jobs.end()) {
        return it->second;
    }

    JobInfo missing;
    missing.job_id = job_id;
    missing.status = JobStatus::Failed;
    missing.error_message = "Job not found";
    return missing;
}

/**
 * @brief Register Event Callback.
 * @param[in] callback Input parameter.
 * @details Calls: lock(), push_back(), std::move().
 */
void LoRAOrchestrator::registerEventCallback(EventCallback callback) {
    std::unique_lock<std::shared_mutex> lock(impl_->state_mutex);
    impl_->callbacks.push_back(std::move(callback));
}

json LoRAOrchestrator::getStats() const {
    std::shared_lock<std::shared_mutex> lock(impl_->state_mutex);
    size_t loaded = 0;
    for (const auto& kv : impl_->adapters) {
        if (kv.second.is_loaded) {
            loaded++;
        }
    }

    json stats;
    stats["adapters_total"] = impl_->adapters.size();
    stats["adapters_loaded"] = loaded;
    stats["cache_size"] = impl_->adapters.size();
    stats["jobs"] = impl_->jobs.size();
    return stats;
}

json LoRAOrchestrator::getHealth() const {
    json health;
    health["status"] = impl_->is_initialized.load(std::memory_order_acquire) ? "ok" : "uninitialized";
    health["adapters"] = impl_->adapters.size();
    return health;
}

json LoRAOrchestrator::getMemoryUsage() const {
    std::shared_lock<std::shared_mutex> lock(impl_->state_mutex);
    size_t bytes = 0;
    for (const auto& kv : impl_->adapters) {
        bytes += kv.second.memory_bytes;
    }
    json usage;
    usage["bytes"] = bytes;
    usage["megabytes"] = bytes / (1024.0 * 1024.0);
    return usage;
}

bool LoRAOrchestrator::healthCheck() const {
    std::shared_lock<std::shared_mutex> lock(impl_->state_mutex);
    return impl_->is_initialized.load(std::memory_order_acquire);
}

/**
 * @brief Clear Cache.
 * @details Calls: lock().
 */
void LoRAOrchestrator::clearCache() {
    std::unique_lock<std::shared_mutex> lock(impl_->state_mutex);
    for (auto& kv : impl_->adapters) {
        kv.second.is_loaded = false;
    }
}

/**
 * @brief Get Multi Lo RAManager.
 * @return Pointer to the result.
 * @details Implements getMultiLoRAManager without additional internal calls.
 */
MultiLoRAManager* LoRAOrchestrator::getMultiLoRAManager() {
    return nullptr;
}

void LoRAOrchestrator::enableAdvancedFeatures([[maybe_unused]] bool enable) {
    impl_->advanced_enabled = enable;
}

std::shared_ptr<LoRAStorageService> LoRAOrchestrator::getStorageService() const {
    return impl_->storage_service;
}

std::shared_ptr<AdapterConsistencyChecker> LoRAOrchestrator::getConsistencyChecker() const {
    return impl_->consistency_checker;
}

/**
 * @brief ============================================================================ Provenance, Snapshots, and Audit Log ============================================================================
 * @param[in] adapter_id Identifier of the adapter.
 * @param[in] record Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: lock(), count(), spdlog::warn(), storeProvenance().
 */

bool LoRAOrchestrator::attachProvenance(const std::string& adapter_id,
                                         const LoRAProvenanceRecord& record) {
    {
        std::shared_lock<std::shared_mutex> lock(impl_->state_mutex);
        if (!impl_->adapters.count(adapter_id)) {
            spdlog::warn("LoRAOrchestrator::attachProvenance: adapter '{}' not registered",
                         adapter_id);
            return false;
        }
    }
    return impl_->provenance_mgr.storeProvenance(adapter_id, record);
}

std::optional<LoRAProvenanceRecord> LoRAOrchestrator::getProvenanceRecord(
    const std::string& adapter_id) const {
    return impl_->provenance_mgr.getProvenance(adapter_id);
}

/**
 * @brief Create Adapter Snapshot.
 * @param[in] adapter_id Identifier of the adapter.
 * @param[in] version Input parameter.
 * @param[in] weights_hash Input parameter.
 * @return Return value.
 * @details Calls: getProvenance(), createSnapshot().
 */
AdapterSnapshot LoRAOrchestrator::createAdapterSnapshot(
    const std::string& adapter_id,
    const std::string& version,
    const std::string& weights_hash) {
    // Load current provenance (if any) as the snapshot's provenance
    LoRAProvenanceRecord prov;
    auto opt_prov = impl_->provenance_mgr.getProvenance(adapter_id);
    if (opt_prov) {
      prov = *opt_prov;
    }
    return impl_->provenance_mgr.createSnapshot(adapter_id, version, weights_hash, prov);
}

std::vector<AdapterSnapshot> LoRAOrchestrator::listAdapterSnapshots(
    const std::string& adapter_id) const {
    return impl_->provenance_mgr.listSnapshots(adapter_id);
}

/**
 * @brief Record Inference Audit.
 * @param[in] adapter_id Identifier of the adapter.
 * @param[in] entry Input parameter.
 * @return Return value.
 * @details Calls: appendAuditEntry(), std::move().
 */
InferenceAuditEntry LoRAOrchestrator::recordInferenceAudit(
    const std::string& adapter_id,
    InferenceAuditEntry entry) {
    return impl_->provenance_mgr.appendAuditEntry(adapter_id, std::move(entry));
}

std::vector<InferenceAuditEntry> LoRAOrchestrator::getInferenceAuditLog(
    const std::string& adapter_id) const {
    return impl_->provenance_mgr.getAuditLog(adapter_id);
}

bool LoRAOrchestrator::verifyAuditChain(const std::string& adapter_id) const {
    return impl_->provenance_mgr.verifyAuditChain(adapter_id);
}

/**
 * @brief Set Decision Record Processor.
 * @param[in] processor Input parameter.
 */
void LoRAOrchestrator::setDecisionRecordProcessor(
    std::shared_ptr<themis::llm::DecisionRecordYamlProcessor> processor)
{
    std::unique_lock<std::shared_mutex> lock(impl_->state_mutex);
    impl_->dr_processor = std::move(processor);
}

} // namespace lora
} // namespace llm
} // namespace themis
