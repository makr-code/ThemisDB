/**
 * @file workflow_engine.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=17, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "ingestion/workflow_engine.h"
#include <stdexcept>
#include "utils/error_registry.h"

#include <nlohmann/json.hpp>
#ifdef HAVE_YAML_CPP
#include <yaml-cpp/yaml.h>
#endif
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>
#include <shared_mutex>

#if defined(_WIN32)
#  include <windows.h>
#else
#  include <fnmatch.h>
#  include <dlfcn.h>
#endif

using json = nlohmann::json;

namespace themis {
namespace ingestion {

using namespace themis::errors;

// ─────────────────────────────────────────────────────────────────────────────
// FilePattern::matches
// ─────────────────────────────────────────────────────────────────────────────

namespace {

#if defined(_WIN32)
using DynamicLibHandle = HMODULE;

DynamicLibHandle openDynamicLibrary(const std::string& path) {
    return ::LoadLibraryA(path.c_str());
}

void* resolveDynamicSymbol(DynamicLibHandle handle, const char* symbol) {
    if (!handle) {
        return nullptr;
    }
    return reinterpret_cast<void*>(::GetProcAddress(handle, symbol));
}

void closeDynamicLibrary(DynamicLibHandle handle) {
    if (handle) {
        ::FreeLibrary(handle);
    }
}

std::string getDynamicLibraryError() {
    const DWORD code = ::GetLastError();
    return "LoadLibrary/GetProcAddress failed with Win32 error " + std::to_string(code);
}
#else
using DynamicLibHandle = void*;

DynamicLibHandle openDynamicLibrary(const std::string& path) {
    return ::dlopen(path.c_str(), RTLD_LAZY | RTLD_LOCAL);
}

void* resolveDynamicSymbol(DynamicLibHandle handle, const char* symbol) {
    return ::dlsym(handle, symbol);
}

void closeDynamicLibrary(DynamicLibHandle handle) {
    if (handle) {
        ::dlclose(handle);
    }
}

std::string getDynamicLibraryError() {
    const char* err = ::dlerror();
    return err ? std::string(err) : std::string("unknown dlerror");
}
#endif

bool mimeMatches(const std::string& pattern, const std::string& mime) {
    if (pattern.empty()) {
      return true;
    }
    if (pattern == mime) {
      return true;
    }
    // Simple prefix wildcard: "application/vnd.openxmlformats*"
    if (!pattern.empty() && pattern.back() == '*') {
        const auto prefix = pattern.substr(0, static_cast<int>(pattern.size()) - 1);
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
    if (!p.empty() && p.front() == '*') {
      p = p.substr(1);
    }
    if (!p.empty() && p.back() == '*') {
      p = p.substr(0, static_cast<int>(p.size()) - 1);
    }
    return n.find(p) != std::string::npos;
#else
    return ::fnmatch(pattern.c_str(), name.c_str(), FNM_CASEFOLD) == 0;
#endif
}
} // anonymous namespace

bool FilePattern::matches(const std::string& mime,
                           const std::string& filename) const {
    if (mime_types.empty() && filename_patterns.empty()) {
      return true;
    }

    bool mime_ok = mime_types.empty();
    for (const auto& mp : mime_types) {
        if (mimeMatches(mp, mime)) { mime_ok = true; break; }
    }
    if (!mime_ok) {
      return false;
    }

    bool name_ok = filename_patterns.empty();
    for (const auto& fp : filename_patterns) {
        if (filenameMatchesGlob(fp, filename)) { name_ok = true; break; }
    }
    return name_ok;
}

// ─────────────────────────────────────────────────────────────────────────────
// StepRegistry::Impl
// ─────────────────────────────────────────────────────────────────────────────

/** @brief StepRegistry::Impl. */
class StepRegistry::Impl {
public:
    struct Entry {
        std::shared_ptr<IIngestionStep> step;
        DynamicLibHandle dl_handle{nullptr}; ///< Non-null for dynamically loaded plugins
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
                resolveDynamicSymbol(entry.dl_handle, "themis_destroy_step"));
            if (destroy && entry.step) {
              destroy(entry.step.get());
            }
            closeDynamicLibrary(entry.dl_handle);
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
        const std::string& library_path,
        const StepPluginManifest& manifest) {
    // §Phase 3 DLL sandbox — validate path and MIME constraints
    if (!manifest.allowed_paths.empty()) {
        namespace fs = std::filesystem;
        std::error_code ec = {};
        const auto canonical = fs::weakly_canonical(library_path, ec);
        const std::string canon_str = ec ? library_path : canonical.string();
        bool path_allowed = false;
        for (const auto& prefix : manifest.allowed_paths) {
            const auto canon_prefix = fs::weakly_canonical(prefix, ec);
            const std::string prefix_str = ec ? prefix : canon_prefix.string();
            if (canon_str.rfind(prefix_str, 0) == 0) {
                path_allowed = true;
                break;
            }
        }
        if (!path_allowed) {
            return tl::make_unexpected(
                Error{ErrorCode::ERR_WORKFLOW_PLUGIN_LOAD_FAILED,
                      "Plugin path '" + library_path
                      + "' is not in the allowedPaths sandbox manifest"});
        }
    }

    if (!manifest.allowed_mime_types.empty()) {
        // Read sidecar manifest: <library_path>.manifest.json with {"mime_type":"..."}
        const std::string sidecar_path = library_path + ".manifest.json";
        std::string plugin_mime = {};
        {
            std::ifstream sf(sidecar_path);
            if (sf.is_open()) {
                try {
                    const json sidecar = json::parse(sf, nullptr, true, true);
                    if (sidecar.contains("mime_type") && sidecar["mime_type"].is_string())
                        plugin_mime = sidecar["mime_type"].get<std::string>();
                } catch (...) {}
            }
        }
        if (plugin_mime.empty()) {
            return tl::make_unexpected(
                Error{ErrorCode::ERR_WORKFLOW_PLUGIN_LOAD_FAILED,
                      "Plugin '" + library_path
                      + "' has no sidecar .manifest.json with 'mime_type' field "
                        "(required by allowedMime sandbox)"});
        }
        const bool mime_allowed = std::any_of(
            manifest.allowed_mime_types.begin(), manifest.allowed_mime_types.end(),
            [&]([[maybe_unused]] const std::string& allowed) { return allowed == plugin_mime; });
        if (!mime_allowed) {
            return tl::make_unexpected(
                Error{ErrorCode::ERR_WORKFLOW_PLUGIN_LOAD_FAILED,
                      "Plugin MIME type '" + plugin_mime
                      + "' is not in the allowedMime sandbox manifest"});
        }
    }

    DynamicLibHandle handle = openDynamicLibrary(library_path);
    if (!handle) {
        return tl::make_unexpected(
            Error{ErrorCode::ERR_WORKFLOW_PLUGIN_LOAD_FAILED,
                  std::string("Library load failed for '") + library_path
                  + "': " + getDynamicLibraryError()});
    }
    using CreateFn = IIngestionStep*(*)();
    auto* create = reinterpret_cast<CreateFn>(
        resolveDynamicSymbol(handle, "themis_create_step"));
    if (!create) {
        closeDynamicLibrary(handle);
        return tl::make_unexpected(
            Error{ErrorCode::ERR_WORKFLOW_STEP_NOT_A_STEP,
                  "Library '" + library_path
                  + "' does not export themis_create_step"});
    }
    IIngestionStep* raw = create();
    if (!raw) {
        closeDynamicLibrary(handle);
        return tl::make_unexpected(
            Error{ErrorCode::ERR_WORKFLOW_STEP_NOT_A_STEP,
                  "themis_create_step() returned nullptr in '" + library_path + "'"});
    }
    // Wrap in shared_ptr with custom deleter that calls themis_destroy_step
    using DestroyFn = void(*)(IIngestionStep*);
    auto* destroy = reinterpret_cast<DestroyFn>(
        resolveDynamicSymbol(handle, "themis_destroy_step"));
    std::shared_ptr<IIngestionStep> step(
        raw,
        [destroy](IIngestionStep* p) { if (destroy) destroy(p); });

    std::unique_lock<std::shared_mutex> lock(impl_->mutex_);
    if (impl_->steps_.count(plugin_name)) {
        closeDynamicLibrary(handle);
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
    std::vector<std::string> names = {};

    names.reserve(impl_->steps_.size());
    for (const auto& [name, _] : impl_->steps_) {
      names.push_back(name);
    }
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
        closeDynamicLibrary(it->second.dl_handle);
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
    if (obj.contains(key) && obj[key].is_string()) {
      return obj[key];
    }
    return def;
}

bool safeBool(const json& obj, const std::string& key, bool def = false) {
    if (obj.contains(key) && obj[key].is_boolean()) {
      return obj[key];
    }
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
                if (m.is_string()) {
                  p.file_patterns.mime_types.push_back(m);
                }
            }
        }
        if (fp.contains("filename_patterns") && fp["filename_patterns"].is_array()) {
            for (const auto& fn : fp["filename_patterns"]) {
                if (fn.is_string()) {
                  p.file_patterns.filename_patterns.push_back(fn);
                }
            }
        }
    }

    // steps
    if (doc.contains("steps") && doc["steps"].is_array()) {
        for (const auto& s : doc["steps"]) {
            if (!s.is_object()) {
              continue;
            }
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

#ifdef HAVE_YAML_CPP
/**
 * @brief Convert a YAML node to a flat nlohmann::json object (one level deep).
 *
 * Scalar values are converted to their native JSON types (bool, int64, double,
 * or string).  Sequences become JSON arrays of strings.  Nested maps and other
 * complex nodes are serialized as their YAML string representation.
 */
static nlohmann::json yamlNodeToJson(const YAML::Node& node) {
    nlohmann::json obj = nlohmann::json::object();
    if (!node || !node.IsMap()) {
      return obj;
    }
    for (const auto& kv : node) {
        const std::string key = kv.first.as<std::string>("");
        if (key.empty()) {
          continue;
        }
        const YAML::Node& val = kv.second;
        if (val.IsScalar()) {
            try { obj[key] = val.as<bool>(); continue; } catch (...) {}
            try { obj[key] = val.as<int64_t>(); continue; } catch (...) {}
            try { obj[key] = val.as<double>(); continue; } catch (...) {}
            obj[key] = val.as<std::string>("");
        } else if (val.IsSequence()) {
            auto arr = nlohmann::json::array();
            for (const auto& item : val) {
                if (item.IsScalar()) {
                  arr.push_back(item.as<std::string>(""));
                }
            }
            obj[key] = arr;
        } else {
            obj[key] = val.as<std::string>("");
        }
    }
    return obj;
}

WorkflowProfile parseProfileFromYaml(const YAML::Node& root,
                                      const std::string& source_path) {
    WorkflowProfile p;
    p.source_path = source_path;
    auto asStr = [](const YAML::Node& n, const std::string& def = "") -> std::string {
        return (n && n.IsScalar()) ? n.as<std::string>(def) : def;
    };
    auto asBool = [](const YAML::Node& n, bool def = false) -> bool {
        return (n && n.IsScalar()) ? n.as<bool>(def) : def;
    };

    p.api_version = asStr(root["apiVersion"]);
    p.kind        = asStr(root["kind"]);
    p.name        = asStr(root["name"]);
    p.description = asStr(root["description"]);

    // file_patterns
    const auto& fp = root["file_patterns"];
    if (fp && fp.IsMap()) {
        const auto& mt = fp["mime_types"];
        if (mt && mt.IsSequence()) {
            for (const auto& m : mt)
                if (m.IsScalar()) {
                  p.file_patterns.mime_types.push_back(m.as<std::string>(""));
                }
        }
        const auto& fn = fp["filename_patterns"];
        if (fn && fn.IsSequence()) {
            for (const auto& f : fn)
                if (f.IsScalar()) {
                  p.file_patterns.filename_patterns.push_back(f.as<std::string>(""));
                }
        }
    }

    // steps
    const auto& steps = root["steps"];
    if (steps && steps.IsSequence()) {
        for (const auto& s : steps) {
            if (!s.IsMap()) {
              continue;
            }
            StepConfig sc;
            sc.name       = asStr(s["name"]);
            sc.plugin     = asStr(s["plugin"]);
            sc.condition  = asStr(s["condition"]);
            sc.on_failure = asStr(s["on_failure"], "abort");
            sc.parallel   = asBool(s["parallel"], false);
            sc.config     = yamlNodeToJson(s["config"]);
            p.steps.push_back(std::move(sc));
        }
    }

    // output
    const auto& out = root["output"];
    if (out && out.IsMap()) {
        p.output_graph           = asBool(out["graph"], true);
        p.output_vector          = asBool(out["vector"], true);
        p.output_document_store  = asBool(out["document_store"], true);
        const auto& qg = out["quality_gate"];
        if (qg && qg.IsMap()) {
            if (qg["min_entities"] && qg["min_entities"].IsScalar())
                p.quality_gate_min_entities = qg["min_entities"].as<std::size_t>(0);
            if (qg["min_quality_score"] && qg["min_quality_score"].IsScalar())
                p.quality_gate_min_quality_score = qg["min_quality_score"].as<double>(0.0);
        }
    }
    return p;
}
#endif // HAVE_YAML_CPP

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// WorkflowEngine::Impl
// ─────────────────────────────────────────────────────────────────────────────

/** @brief WorkflowEngine::Impl. */
class WorkflowEngine::Impl {
public:
    mutable std::shared_mutex profiles_mutex_;
    std::vector<WorkflowProfile> profiles_;   ///< In registration order
    StepRegistry step_registry_;

    const WorkflowProfile* findProfileByName(const std::string& name) const {
        for (const auto& p : profiles_)
            if (p.name == name) {
              return &p;
            }
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
                                           + result.error().message());
                    continue;
                }
                if (step_cfg.quarantineOnFailure()) {
                    return tl::make_unexpected(
                        Error{ErrorCode::ERR_WORKFLOW_QUARANTINED,
                              "File quarantined after step '" + step_cfg.name
                              + "': " + result.error().message()});
                }
                return tl::make_unexpected(
                    Error{ErrorCode::ERR_WORKFLOW_STEP_EXECUTION_FAILED,
                          "Step '" + step_cfg.name + "' failed: "
                          + result.error().message()});
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
                if (!v.embedding.empty()) {
                  ++with_embed;
                }
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
    std::ostringstream ss = {};
    ss << file.rdbuf();
    const std::string content = ss.str();

    // Parse the profile: prefer native yaml-cpp when available, fall back to
    // nlohmann/json (which handles JSON and simple YAML subsets).
    WorkflowProfile profile;
#ifdef HAVE_YAML_CPP
    // Primary path: native yaml-cpp parser (supports full YAML syntax)
    try {
        YAML::Node root = YAML::Load(content);
        if (!root["name"] || !root["name"].IsScalar() || root["name"].as<std::string>("").empty()) {
            return tl::make_unexpected(
                Error{ErrorCode::ERR_WORKFLOW_PROFILE_INVALID,
                      "Profile '" + yaml_path + "' is missing 'name' field"});
        }
        if (!root["steps"] || !root["steps"].IsSequence()) {
            return tl::make_unexpected(
                Error{ErrorCode::ERR_WORKFLOW_PROFILE_INVALID,
                      "Profile '" + yaml_path + "' is missing 'steps' array"});
        }
        profile = parseProfileFromYaml(root, yaml_path);
    } catch (const YAML::Exception& e) {
        return tl::make_unexpected(
            Error{ErrorCode::ERR_WORKFLOW_PROFILE_INVALID,
                  std::string("YAML parse error in '") + yaml_path + "': " + e.what()});
    }
#else
    // Fallback path: nlohmann/json (handles JSON and simple YAML subsets)
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

    profile = parseProfile(doc, yaml_path);
#endif // HAVE_YAML_CPP

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
    std::error_code ec = {};
    for (const auto& entry : fs::directory_iterator(directory_path, ec)) {
        if (ec) {
          break;
        }
        if (!entry.is_regular_file()) {
          continue;
        }
        const auto ext = entry.path().extension().string();
        if (ext != ".yaml" && ext != ".yml" && ext != ".json") {
          continue;
        }
        auto result = loadProfile(entry.path().string());
        if (result) {
          ++count;
        }
    }
    return count;
}

const WorkflowProfile* WorkflowEngine::selectProfile(
        const std::string& mime,
        const std::string& filename) const {
    std::shared_lock<std::shared_mutex> lock(impl_->profiles_mutex_);
    for (const auto& p : impl_->profiles_) {
        if (p.name == "default") continue;  // evaluated last
        if (p.file_patterns.matches(mime, filename)) {
          return &p;
        }
    }
    // Fall back to "default"
    return impl_->findProfileByName("default");
}

std::vector<std::string> WorkflowEngine::listProfiles() const {
    std::shared_lock<std::shared_mutex> lock(impl_->profiles_mutex_);
    std::vector<std::string> names = {};

    names.reserve(impl_->profiles_.size());
    for (const auto& p : impl_->profiles_) {
      names.push_back(p.name);
    }
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

