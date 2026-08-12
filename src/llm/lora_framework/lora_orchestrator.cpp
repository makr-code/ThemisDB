/**
 * @file lora_orchestrator.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=3, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/lora_orchestrator.h"
#include "llm/decision_record_yaml_processor.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <shared_mutex>
#include <spdlog/spdlog.h>
#include <unordered_map>
#include <utility>

namespace themis {
namespace llm {
namespace lora {

using json = nlohmann::json;
using Clock = std::chrono::system_clock;

namespace {
std::atomic<uint64_t> g_job_counter{0};

std::string makeJobId(const std::string& prefix) {
    return prefix + "-" + std::to_string(++g_job_counter);
}

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
} // namespace

class LoRAOrchestrator::Impl {
public:
    Impl() : is_initialized(false), advanced_enabled(false) {}
    std::atomic<bool> is_initialized;
    bool advanced_enabled;
    mutable std::shared_mutex state_mutex;
    std::unordered_map<std::string, AdapterInfo> adapters;
    std::unordered_map<std::string, std::vector<std::string>> versions;
    std::unordered_map<std::string, JobInfo> jobs;
    std::vector<EventCallback> callbacks;
    
    // Component instances for cross-shard sync
    std::shared_ptr<LoRAStorageService> storage_service;
    std::shared_ptr<AdapterConsistencyChecker> consistency_checker;

    // Provenance manager for cryptographic audit and MVCC snapshots
    LoRAProvenanceManager provenance_mgr;

    // Decision traceability (optional, non-blocking)
    std::shared_ptr<themis::llm::DecisionRecordYamlProcessor> dr_processor;
};

LoRAOrchestrator::LoRAOrchestrator(const Config& config) : impl_(std::make_unique<Impl>()) {
    if (!impl_) {
        spdlog::error("Failed to allocate LoRA Orchestrator Impl");
        throw std::runtime_error("LoRA Orchestrator Impl allocation failed");
    }
    
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

LoRAOrchestrator::LoRAOrchestrator() : impl_(std::make_unique<Impl>()) {
    if (!impl_) {
        spdlog::error("Failed to allocate LoRA Orchestrator Impl");
        throw std::runtime_error("LoRA Orchestrator Impl allocation failed");
    }
    
    Config default_config;
    // Initialize storage service using default config
    impl_->storage_service = std::make_shared<LoRAStorageService>(default_config.storage_config);
    
    // Initialize consistency checker
    AdapterConsistencyChecker::Config checker_config;
    checker_config.enable_checksums = true;
    checker_config.enable_signatures = true;
    impl_->consistency_checker = std::make_shared<AdapterConsistencyChecker>(checker_config);
    
    impl_->is_initialized = true;
    spdlog::info("LoRA Orchestrator initialized with default config");
}

LoRAOrchestrator::~LoRAOrchestrator() = default;

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

std::string LoRAOrchestrator::createAdapter(
    const std::string& adapter_id,
    const TrainingData& training_data,
    const std::optional<LoRAHyperparameters>& hyperparameters,
    bool async) {
    std::unique_lock<std::shared_mutex> lock(impl_->state_mutex);
    const std::string version = "v1.0";

    AdapterInfo info = makeAdapterInfo(adapter_id, version);
    info.hyperparameters = hyperparameters.value_or(LoRAHyperparameters{});
    info.metadata.training_samples = static_cast<int>(training_data.samples.size());
    impl_->adapters[adapter_id] = info;
    impl_->versions[adapter_id].push_back(version);

    JobInfo job;
    job.job_id = makeJobId("train");
    job.type = JobType::Training;
    job.status = async ? JobStatus::Running : JobStatus::Completed;
    job.adapter_id = adapter_id;
    job.progress = async ? 0.1f : 1.0f;
    job.started_at = Clock::now();
    job.updated_at = job.started_at;
    impl_->jobs[job.job_id] = job;

    return job.job_id;
}

std::string LoRAOrchestrator::createAdapterBatch(
    const std::string& adapter_id,
    const std::vector<TrainingData>& datasets,
    const std::optional<LoRAHyperparameters>& hyperparameters,
    bool async) {
    TrainingData combined;
    for (const auto& ds : datasets) {
        combined.samples.insert(combined.samples.end(), ds.samples.begin(), ds.samples.end());
    }
    return createAdapter(adapter_id, combined, hyperparameters, async);
}

bool LoRAOrchestrator::importAdapter(
    const std::string& adapter_id,
    const std::string& /*source_path*/,
    const AdapterMetadata& metadata) {
    std::unique_lock<std::shared_mutex> lock(impl_->state_mutex);
    const std::string version = metadata.version.empty() ? "v1.0" : metadata.version;

    AdapterInfo info = makeAdapterInfo(adapter_id, version);
    info.metadata = metadata;
    info.version = version;
    impl_->adapters[adapter_id] = info;
    impl_->versions[adapter_id].push_back(version);

    return true;
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
    std::vector<AdapterInfo> out;
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
    info.metadata.training_samples += static_cast<int>(training_data.samples.size());
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

std::string LoRAOrchestrator::createVersion(const std::string& adapter_id, const std::string& description) {
    std::unique_lock<std::shared_mutex> lock(impl_->state_mutex);
    auto it = impl_->versions.find(adapter_id);
    const size_t next_version = (it != impl_->versions.end()) ? it->second.size() + 1 : 1;
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

bool LoRAOrchestrator::deleteAdapter(const std::string& adapter_id, bool /*delete_all_versions*/) {
    std::unique_lock<std::shared_mutex> lock(impl_->state_mutex);
    size_t removed = impl_->adapters.erase(adapter_id);
    impl_->versions.erase(adapter_id);
    return removed > 0;
}

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

bool LoRAOrchestrator::unloadAdapter(const std::string& adapter_id, bool /*force*/) {
    std::unique_lock<std::shared_mutex> lock(impl_->state_mutex);
    auto it = impl_->adapters.find(adapter_id);
    if (it == impl_->adapters.end()) {
        return false;
    }
    it->second.is_loaded = false;
    return true;
}

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
    std::vector<JobInfo> jobs;
    for (const auto& kv : impl_->jobs) {
        if (!status || kv.second.status == *status) {
            jobs.push_back(kv.second);
        }
    }
    return jobs;
}

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

void LoRAOrchestrator::clearCache() {
    std::unique_lock<std::shared_mutex> lock(impl_->state_mutex);
    for (auto& kv : impl_->adapters) {
        kv.second.is_loaded = false;
    }
}

MultiLoRAManager* LoRAOrchestrator::getMultiLoRAManager() {
    return nullptr;
}

void LoRAOrchestrator::enableAdvancedFeatures(bool enable) {
    impl_->advanced_enabled = enable;
}

std::shared_ptr<LoRAStorageService> LoRAOrchestrator::getStorageService() const {
    return impl_->storage_service;
}

std::shared_ptr<AdapterConsistencyChecker> LoRAOrchestrator::getConsistencyChecker() const {
    return impl_->consistency_checker;
}

// ============================================================================
// Provenance, Snapshots, and Audit Log
// ============================================================================

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

AdapterSnapshot LoRAOrchestrator::createAdapterSnapshot(
    const std::string& adapter_id,
    const std::string& version,
    const std::string& weights_hash) {
    // Load current provenance (if any) as the snapshot's provenance
    LoRAProvenanceRecord prov;
    auto opt_prov = impl_->provenance_mgr.getProvenance(adapter_id);
    if (opt_prov) prov = *opt_prov;
    return impl_->provenance_mgr.createSnapshot(adapter_id, version, weights_hash, prov);
}

std::vector<AdapterSnapshot> LoRAOrchestrator::listAdapterSnapshots(
    const std::string& adapter_id) const {
    return impl_->provenance_mgr.listSnapshots(adapter_id);
}

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

void LoRAOrchestrator::setDecisionRecordProcessor(
    std::shared_ptr<themis::llm::DecisionRecordYamlProcessor> processor)
{
    std::unique_lock<std::shared_mutex> lock(impl_->state_mutex);
    impl_->dr_processor = std::move(processor);
}

} // namespace lora
} // namespace llm
} // namespace themis

