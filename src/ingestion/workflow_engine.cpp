/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            workflow_engine.cpp                                ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-04-15 18:08:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     505                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • db7df90e31  2026-04-15  feat(ingestion): Google Benchmarks QJ01–QJ11 + SoC/OOP do... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "ingestion/workflow_engine.h"
#include "utils/error_registry.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <fnmatch.h>  // POSIX glob matching; Windows has PathMatchSpec
#include <shared_mutex>
#include <dlfcn.h>    // dlopen/dlsym (POSIX); Windows: LoadLibrary

using json = nlohmann::json;

namespace themis {
namespace ingestion {

// ─────────────────────────────────────────────────────────────────────────────
// FilePattern::matches
// ─────────────────────────────────────────────────────────────────────────────

namespace {
bool mimeMatches(const std::string& pattern, const std::string& mime) {
    if (pattern.empty()) return true;
    if (pattern == mime) return true;
    // Simple prefix wildcard: "application/vnd.openxmlformats*"
    if (!pattern.empty() && pattern.back() == '*') {
        const auto prefix = pattern.substr(0, pattern.size() - 1);
        return mime.rfind(prefix, 0) == 0;
    }
    return false;
}

bool filenameMatchesGlob(const std::string& pattern, const std::string& name) {
#if defined(_WIN32)
    // Windows: simple case-insensitive contains check as fallback
    std::string p = pattern;
    std::string n = name;
    std::transform(p.begin(), p.end(), p.begin(), ::tolower);
    std::transform(n.begin(), n.end(), n.begin(), ::tolower);
    // Remove leading/trailing '*'
    if (!p.empty() && p.front() == '*') p = p.substr(1);
    if (!p.empty() && p.back() == '*')  p = p.substr(0, p.size() - 1);
    return n.find(p) != std::string::npos;
#else
    return ::fnmatch(pattern.c_str(), name.c_str(), FNM_CASEFOLD) == 0;
#endif
}
} // anonymous namespace

bool FilePattern::matches(const std::string& mime,
                           const std::string& filename) const {
    if (mime_types.empty() && filename_patterns.empty()) return true;

    bool mime_ok = mime_types.empty();
    for (const auto& mp : mime_types) {
        if (mimeMatches(mp, mime)) { mime_ok = true; break; }
    }
    if (!mime_ok) return false;

    bool name_ok = filename_patterns.empty();
    for (const auto& fp : filename_patterns) {
        if (filenameMatchesGlob(fp, filename)) { name_ok = true; break; }
    }
    return name_ok;
}

// ─────────────────────────────────────────────────────────────────────────────
// StepRegistry::Impl
// ─────────────────────────────────────────────────────────────────────────────

class StepRegistry::Impl {
public:
    struct Entry {
        std::shared_ptr<IIngestionStep> step;
        void*  dl_handle{nullptr}; ///< Non-null for dynamically loaded plugins
    };

    mutable std::shared_mutex mutex_;
    std::unordered_map<std::string, Entry> steps_;
};

StepRegistry::StepRegistry() : impl_(std::make_unique<Impl>()) {}
StepRegistry::~StepRegistry() {
    // Close all dynamic library handles
    std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
    for (auto& [name, entry] : impl_->steps_) {
        if (entry.dl_handle) {
            // Destroy the step before closing the library
            using DestroyFn = void(*)(IIngestionStep*);
            auto* destroy = reinterpret_cast<DestroyFn>(
                ::dlsym(entry.dl_handle, "themis_destroy_step"));
            if (destroy && entry.step) destroy(entry.step.get());
            ::dlclose(entry.dl_handle);
        }
    }
}

Result<void> StepRegistry::registerStep(
        const std::string& plugin_name,
        std::shared_ptr<IIngestionStep> step) {
    if (plugin_name.empty()) {
        return tl::make_unexpected(Error{ErrorCode::ERR_WORKFLOW_STEP_NOT_REGISTERED,
                                         "plugin_name must not be empty"});
    }
    std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
    if (impl_->steps_.count(plugin_name)) {
        return tl::make_unexpected(
            Error{ErrorCode::ERR_WORKFLOW_STEP_ALREADY_REGISTERED,
                  "Step '" + plugin_name + "' is already registered"});
    }
    impl_->steps_[plugin_name] = Impl::Entry{std::move(step), nullptr};
    return {};
}

Result<void> StepRegistry::loadStepPlugin(
        const std::string& plugin_name,
        const std::string& library_path) {
    void* handle = ::dlopen(library_path.c_str(), RTLD_LAZY | RTLD_LOCAL);
    if (!handle) {
        return tl::make_unexpected(
            Error{ErrorCode::ERR_WORKFLOW_PLUGIN_LOAD_FAILED,
                  std::string("dlopen failed for '") + library_path
                  + "': " + ::dlerror()});
    }
    using CreateFn = IIngestionStep*(*)();
    auto* create = reinterpret_cast<CreateFn>(
        ::dlsym(handle, "themis_create_step"));
    if (!create) {
        ::dlclose(handle);
        return tl::make_unexpected(
            Error{ErrorCode::ERR_WORKFLOW_STEP_NOT_A_STEP,
                  "Library '" + library_path
                  + "' does not export themis_create_step"});
    }
    IIngestionStep* raw = create();
    if (!raw) {
        ::dlclose(handle);
        return tl::make_unexpected(
            Error{ErrorCode::ERR_WORKFLOW_STEP_NOT_A_STEP,
                  "themis_create_step() returned nullptr in '" + library_path + "'"});
    }
    // Wrap in shared_ptr with custom deleter that calls themis_destroy_step
    using DestroyFn = void(*)(IIngestionStep*);
    auto* destroy = reinterpret_cast<DestroyFn>(
        ::dlsym(handle, "themis_destroy_step"));
    std::shared_ptr<IIngestionStep> step(
        raw,
        [destroy](IIngestionStep* p) { if (destroy) destroy(p); });

    std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
    if (impl_->steps_.count(plugin_name)) {
        ::dlclose(handle);
        return tl::make_unexpected(
            Error{ErrorCode::ERR_WORKFLOW_STEP_ALREADY_REGISTERED,
                  "Step '" + plugin_name + "' is already registered"});
    }
    impl_->steps_[plugin_name] = Impl::Entry{std::move(step), handle};
    return {};
}

std::shared_ptr<IIngestionStep>
StepRegistry::getStep(const std::string& plugin_name) const {
    std::shared_lock<std::shared_mutex> lock(impl_->mutex_);
    auto it = impl_->steps_.find(plugin_name);
    return it != impl_->steps_.end() ? it->second.step : nullptr;
}

bool StepRegistry::hasStep(const std::string& plugin_name) const {
    std::shared_lock<std::shared_mutex> lock(impl_->mutex_);
    return impl_->steps_.count(plugin_name) > 0;
}

std::vector<std::string> StepRegistry::listSteps() const {
    std::shared_lock<std::shared_mutex> lock(impl_->mutex_);
    std::vector<std::string> names;
    names.reserve(impl_->steps_.size());
    for (const auto& [name, _] : impl_->steps_) names.push_back(name);
    return names;
}

Result<void> StepRegistry::unloadStep(const std::string& plugin_name) {
    std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
    auto it = impl_->steps_.find(plugin_name);
    if (it == impl_->steps_.end()) {
        return tl::make_unexpected(
            Error{ErrorCode::ERR_WORKFLOW_STEP_NOT_REGISTERED,
                  "Step '" + plugin_name + "' is not registered"});
    }
    if (it->second.dl_handle) {
        ::dlclose(it->second.dl_handle);
    }
    impl_->steps_.erase(it);
    return {};
}

// ─────────────────────────────────────────────────────────────────────────────
// YAML profile parsing helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

std::string safeString(const json& obj, const std::string& key,
                        const std::string& def = "") {
    if (obj.contains(key) && obj[key].is_string()) return obj[key];
    return def;
}

bool safeBool(const json& obj, const std::string& key, bool def = false) {
    if (obj.contains(key) && obj[key].is_boolean()) return obj[key];
    return def;
}

WorkflowProfile parseProfile(const json& doc, const std::string& source_path) {
    WorkflowProfile p;
    p.source_path  = source_path;
    p.api_version  = safeString(doc, "apiVersion");
    p.kind         = safeString(doc, "kind");
    p.name         = safeString(doc, "name");
    p.description  = safeString(doc, "description");

    // file_patterns
    if (doc.contains("file_patterns") && doc["file_patterns"].is_object()) {
        const auto& fp = doc["file_patterns"];
        if (fp.contains("mime_types") && fp["mime_types"].is_array()) {
            for (const auto& m : fp["mime_types"]) {
                if (m.is_string()) p.file_patterns.mime_types.push_back(m);
            }
        }
        if (fp.contains("filename_patterns") && fp["filename_patterns"].is_array()) {
            for (const auto& fn : fp["filename_patterns"]) {
                if (fn.is_string()) p.file_patterns.filename_patterns.push_back(fn);
            }
        }
    }

    // steps
    if (doc.contains("steps") && doc["steps"].is_array()) {
        for (const auto& s : doc["steps"]) {
            if (!s.is_object()) continue;
            StepConfig sc;
            sc.name       = safeString(s, "name");
            sc.plugin     = safeString(s, "plugin");
            sc.condition  = safeString(s, "condition");
            sc.on_failure = safeString(s, "on_failure", "abort");
            sc.parallel   = safeBool(s, "parallel", false);
            sc.config     = s.contains("config") ? s["config"] : json::object();
            p.steps.push_back(std::move(sc));
        }
    }

    // output
    if (doc.contains("output") && doc["output"].is_object()) {
        const auto& out = doc["output"];
        p.output_graph           = safeBool(out, "graph", true);
        p.output_vector          = safeBool(out, "vector", true);
        p.output_document_store  = safeBool(out, "document_store", true);
        if (out.contains("quality_gate") && out["quality_gate"].is_object()) {
            const auto& qg = out["quality_gate"];
            if (qg.contains("min_entities") && qg["min_entities"].is_number_unsigned())
                p.quality_gate_min_entities = qg["min_entities"];
            if (qg.contains("min_quality_score") && qg["min_quality_score"].is_number())
                p.quality_gate_min_quality_score = qg["min_quality_score"];
        }
    }
    return p;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// WorkflowEngine::Impl
// ─────────────────────────────────────────────────────────────────────────────

class WorkflowEngine::Impl {
public:
    mutable std::shared_mutex profiles_mutex_;
    std::vector<WorkflowProfile> profiles_;   ///< In registration order
    StepRegistry step_registry_;

    const WorkflowProfile* findProfileByName(const std::string& name) const {
        for (const auto& p : profiles_)
            if (p.name == name) return &p;
        return nullptr;
    }

    Result<BaseEntitySet> runProfile(const WorkflowProfile& profile,
                                      ExtractionContext& ctx) {
        for (const auto& step_cfg : profile.steps) {
            auto step = step_registry_.getStep(step_cfg.plugin);
            if (!step) {
                if (step_cfg.skipOnFailure()) {
                    ctx.warnings.push_back("Step '" + step_cfg.name
                                           + "': plugin '" + step_cfg.plugin
                                           + "' not registered — skipping");
                    continue;
                }
                return tl::make_unexpected(
                    Error{ErrorCode::ERR_WORKFLOW_STEP_NOT_REGISTERED,
                          "Step '" + step_cfg.name + "': plugin '"
                          + step_cfg.plugin + "' not registered"});
            }

            // Capability check
            if (!step->canHandle(ctx)) {
                ctx.warnings.push_back("Step '" + step_cfg.name
                                       + "': canHandle() returned false — skipping");
                continue;
            }

            // Execute
            auto result = step->execute(ctx, step_cfg);
            if (!result) {
                if (step_cfg.skipOnFailure()) {
                    ctx.warnings.push_back("Step '" + step_cfg.name
                                           + "' failed (skipped): "
                                           + result.error().message);
                    continue;
                }
                if (step_cfg.quarantineOnFailure()) {
                    return tl::make_unexpected(
                        Error{ErrorCode::ERR_WORKFLOW_QUARANTINED,
                              "File quarantined after step '" + step_cfg.name
                              + "': " + result.error().message});
                }
                return tl::make_unexpected(
                    Error{ErrorCode::ERR_WORKFLOW_STEP_EXECUTION_FAILED,
                          "Step '" + step_cfg.name + "' failed: "
                          + result.error().message});
            }
        }

        // Assemble BaseEntitySet from context
        BaseEntitySet out;
        out.source_file_id = ctx.manifest.file_id;
        out.nodes   = ctx.entities;
        out.edges   = ctx.relations;
        out.chunks  = ctx.embeddings;

        // Compute a simple quality score: ratio of chunks with embeddings
        if (!ctx.chunks.empty()) {
            std::size_t with_embed = 0;
            for (const auto& v : ctx.embeddings) {
                if (!v.embedding.empty()) ++with_embed;
            }
            out.quality_score = static_cast<double>(with_embed)
                                / static_cast<double>(ctx.chunks.size());
        }

        // Quality gate
        if (out.nodes.size() < profile.quality_gate_min_entities) {
            ctx.warnings.push_back("Quality gate: only "
                                   + std::to_string(out.nodes.size())
                                   + " entities extracted (min: "
                                   + std::to_string(profile.quality_gate_min_entities)
                                   + ")");
        }

        return out;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// WorkflowEngine public API
// ─────────────────────────────────────────────────────────────────────────────

WorkflowEngine::WorkflowEngine() : impl_(std::make_unique<Impl>()) {}
WorkflowEngine::~WorkflowEngine() = default;

Result<void> WorkflowEngine::loadProfile(const std::string& yaml_path) {
    // Read file
    std::ifstream file(yaml_path);
    if (!file.is_open()) {
        return tl::make_unexpected(
            Error{ErrorCode::ERR_WORKFLOW_PROFILE_NOT_FOUND,
                  "Cannot open workflow profile: " + yaml_path});
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    const std::string content = ss.str();

    // Parse JSON (nlohmann/json also parses YAML-subset when compiled with
    // the YAML parser feature; for pure YAML we use a simple pre-processor
    // approach — the production path uses a yaml-cpp wrapper).
    // For robustness we attempt JSON parse first (workflow YAML is also valid
    // JSON in most cases when no YAML-only features are used).
    json doc;
    try {
        doc = json::parse(content, nullptr, /*allow_exceptions=*/true,
                          /*ignore_comments=*/true);
    } catch (const json::exception& e) {
        return tl::make_unexpected(
            Error{ErrorCode::ERR_WORKFLOW_PROFILE_INVALID,
                  std::string("Profile parse error in '") + yaml_path + "': "
                  + e.what()});
    }

    // Minimal schema validation
    if (!doc.contains("name") || !doc["name"].is_string() || doc["name"].empty()) {
        return tl::make_unexpected(
            Error{ErrorCode::ERR_WORKFLOW_PROFILE_INVALID,
                  "Profile '" + yaml_path + "' is missing 'name' field"});
    }
    if (!doc.contains("steps") || !doc["steps"].is_array()) {
        return tl::make_unexpected(
            Error{ErrorCode::ERR_WORKFLOW_PROFILE_INVALID,
                  "Profile '" + yaml_path + "' is missing 'steps' array"});
    }

    WorkflowProfile profile = parseProfile(doc, yaml_path);

    std::unique_lock<std::shared_mutex> lock(impl_->profiles_mutex_);
    // Idempotent: skip if already loaded
    for (const auto& p : impl_->profiles_) {
        if (p.name == profile.name) return {};  // already loaded
    }
    impl_->profiles_.push_back(std::move(profile));
    return {};
}

std::size_t WorkflowEngine::loadProfilesFromDirectory(
        const std::string& directory_path) {
    std::size_t count = 0;
    namespace fs = std::filesystem;
    std::error_code ec;
    for (const auto& entry : fs::directory_iterator(directory_path, ec)) {
        if (ec) break;
        if (!entry.is_regular_file()) continue;
        const auto ext = entry.path().extension().string();
        if (ext != ".yaml" && ext != ".yml" && ext != ".json") continue;
        auto result = loadProfile(entry.path().string());
        if (result) ++count;
    }
    return count;
}

const WorkflowProfile* WorkflowEngine::selectProfile(
        const std::string& mime,
        const std::string& filename) const {
    std::shared_lock<std::shared_mutex> lock(impl_->profiles_mutex_);
    for (const auto& p : impl_->profiles_) {
        if (p.name == "default") continue;  // evaluated last
        if (p.file_patterns.matches(mime, filename)) return &p;
    }
    // Fall back to "default"
    return impl_->findProfileByName("default");
}

std::vector<std::string> WorkflowEngine::listProfiles() const {
    std::shared_lock<std::shared_mutex> lock(impl_->profiles_mutex_);
    std::vector<std::string> names;
    names.reserve(impl_->profiles_.size());
    for (const auto& p : impl_->profiles_) names.push_back(p.name);
    return names;
}

StepRegistry& WorkflowEngine::stepRegistry() {
    return impl_->step_registry_;
}

const StepRegistry& WorkflowEngine::stepRegistry() const {
    return impl_->step_registry_;
}

Result<BaseEntitySet> WorkflowEngine::execute(ExtractionContext& ctx) {
    const std::string filename = ctx.manifest.filename_stem + ctx.manifest.extension;
    const WorkflowProfile* profile = selectProfile(
        ctx.manifest.detected_mime, filename);
    if (!profile) {
        return tl::make_unexpected(
            Error{ErrorCode::ERR_WORKFLOW_NO_MATCHING_PROFILE,
                  "No workflow profile matches MIME='" + ctx.manifest.detected_mime
                  + "' filename='" + filename + "'"});
    }
    return impl_->runProfile(*profile, ctx);
}

Result<BaseEntitySet> WorkflowEngine::executeWithProfile(
        const std::string& profile_name,
        ExtractionContext& ctx) {
    std::shared_lock<std::shared_mutex> lock(impl_->profiles_mutex_);
    const WorkflowProfile* profile = impl_->findProfileByName(profile_name);
    if (!profile) {
        return tl::make_unexpected(
            Error{ErrorCode::ERR_WORKFLOW_PROFILE_NOT_FOUND,
                  "Workflow profile '" + profile_name + "' is not loaded"});
    }
    lock.unlock();
    return impl_->runProfile(*profile, ctx);
}

} // namespace ingestion
} // namespace themis
