/**
 * @file llm_wiki_plugin_impl.cpp
 * @brief Full implementation of LLMWikiPluginImpl.
 */

#include "wikipedia/llm_wiki_plugin_impl.h"
// Edition/feature gate enforcement lives in src/llm_wiki for now.
#include "edition_gate.h"
#include "config/config_path_resolver.h"
#include "llm_wiki/process_policy_manager.h"

#include "importers/wikipedia_pipeline.hpp"
#include "importers/wikipedia_types.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <regex>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <unordered_set>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace themis {
namespace plugins {
namespace llm_wiki {

namespace {

static constexpr uint64_t kFnvPrime = 0x00000100000001B3ULL;
static constexpr uint64_t kFnvOffsetBasis = 0xcbf29ce484222325ULL;

uint64_t fnv1a64(std::string_view data) noexcept {
    uint64_t hash = kFnvOffsetBasis;
    for (unsigned char c : data) {
        hash ^= static_cast<uint64_t>(c);
        hash *= kFnvPrime;
    }
    return hash;
}

std::vector<float> hashEmbed(const std::string& text, int dim) {
    if (dim <= 0) {
      dim = 128;
    }
    std::vector<float> vec(static_cast<std::size_t>(dim), 0.0f);

    static const std::regex tok_re("[A-Za-z0-9_\\-]+");
    auto it = std::sregex_iterator(text.begin(), text.end(), tok_re);
    auto end = std::sregex_iterator();

    for (; it != end; ++it) {
        std::string tok = (*it)[0].str();
        std::transform(tok.begin(), tok.end(), tok.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        uint64_t h = fnv1a64(tok);
        int idx = static_cast<int>(h % static_cast<uint64_t>(dim));
        vec[static_cast<std::size_t>(idx)] += 1.0f;
    }

    float norm = 0.0f;
    for (float v : vec) {
      norm += v * v;
    }
    norm = std::sqrt(norm);
    if (norm > 1e-9f) {
        for (float& v : vec) {
          v /= norm;
        }
    }
    return vec;
}

static const std::vector<std::string_view> UNSAFE_PATTERNS = {
    "ignore previous instructions",
    "ignore all previous instructions",
    "system prompt",
    "reveal secret",
    "api key",
    "password",
    "private key",
    "sudo",
    "base64 decode",
    "eval(",
    "exec(",
};

bool containsUnsafePattern(const std::string& text) {
    std::string lower = {};
    lower.reserve(text.size());
    for (unsigned char c : text) {
      lower += static_cast<char>(std::tolower(c));
    }
    for (std::string_view pattern : UNSAFE_PATTERNS) {
        if (lower.find(pattern) != std::string::npos) {
          return true;
        }
    }
    return false;
}

std::string resolveProcessPolicyPath(const std::string& configured_path) {
    if (!configured_path.empty()) {
        if (auto resolved =
                themis::config::ConfigPathResolver::tryResolve(configured_path)) {
            return *resolved;
        }
        return configured_path;
    }

    std::vector<std::filesystem::path> candidate_roots;
    const char* pwd = std::getenv("PWD");
    if (pwd && *pwd != '\0') {
        candidate_roots.emplace_back(std::filesystem::path(pwd));
    }
    candidate_roots.emplace_back(std::filesystem::current_path());

    const auto this_source_root =
        std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
    candidate_roots.emplace_back(this_source_root);

    for (const auto& root : candidate_roots) {
        const auto candidate =
            (root / "src" / "llm_wiki" / "process" / "llm_wiki_process_policy.yaml")
                .lexically_normal();
        std::error_code ec;
        if (std::filesystem::exists(candidate, ec) && !ec) {
            if (auto resolved = themis::config::ConfigPathResolver::tryResolve(candidate.string())) {
                return *resolved;
            }
            return candidate.string();
        }
        if (auto resolved = themis::config::ConfigPathResolver::tryResolve(candidate.string())) {
            return *resolved;
        }
    }

    return {};
}

std::string currentIsoUtcTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
#ifdef _WIN32
    gmtime_s(&tm_buf, &t);
#else
    gmtime_r(&t, &tm_buf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm_buf, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

// ── Simple glob match ─────────────────────────────────────────────────────────

/**
 * @brief Check whether a filesystem path matches a simple glob like "*.md".
 *
 * Only the `*.<ext>` form is supported (the common case for file ingestion).
 * An empty glob or a glob not matching `*.<ext>` accepts every file.
 *
 * @param path  File path.
 * @param glob  Glob string, e.g. "*.md".
 * @return      True if the file matches.
 */
bool matchGlob(const std::filesystem::path& path, const std::string& glob) {
    if (glob.empty() || glob == "*") {
      return true;
    }
    if (static_cast<int>(glob.size()) > 2 && glob[0] == '*' && glob[1] == '.') {
        std::string ext = glob.substr(1);
        return path.extension().string() == ext;
    }
    return true;
}

} // anonymous namespace

PluginCapabilities LLMWikiPluginImpl::getCapabilities() const {
    PluginCapabilities caps;
    caps.thread_safe = true;
    caps.supports_batching = true;
    return caps;
}

bool LLMWikiPluginImpl::initialize(const char* config_json) {
    if (!config_json) {
        spdlog::warn("[llm_wiki] IThemisPlugin::initialize called with null config");
        return false;
    }
    return this->initialize(std::string(config_json)).ok();
}

void LLMWikiPluginImpl::shutdown() noexcept {
    try {
        std::unique_lock<std::shared_mutex> lock(mutex_);

        if (!json_index_path_.empty() && !chunks_.empty()) {
            // persistence hook for in-memory index
        }

#ifdef THEMISDB_WIKI_PHASE_B
        if (phase_b_active_) {
            if (wiki_store_) {
              wiki_store_->flush();
            }
            wiki_store_.reset();
            llm_b_.reset();
            vim_.reset();
            sim_.reset();
            rocksdb_.reset();
            phase_b_active_ = false;
        }
#endif

        json_reader_.reset();
        workspace_.reset();
        chunks_.clear();
        initialized_ = false;
        spdlog::info("[llm_wiki] Plugin shut down");
    } catch (const std::exception& e) {
        spdlog::error("[llm_wiki] Exception during shutdown: {}", e.what());
    } catch (...) {
        spdlog::error("[llm_wiki] Unknown exception during shutdown");
    }
}

Status LLMWikiPluginImpl::initialize(const std::string& config_json) {
    std::unique_lock<std::shared_mutex> lock(mutex_);

    if (config_json.empty()) {
        return Status::Ok();
    }

    try {
        const auto j = nlohmann::json::parse(config_json);
        if (j.contains("embedding_provider")) {
          embedding_provider_ = j["embedding_provider"].get<std::string>();
        }
        if (j.contains("embedding_dim")) {
          embedding_dim_ = j["embedding_dim"].get<int>();
        }
        if (j.contains("splitter_max_tokens")) {
          splitter_max_tokens_ = j["splitter_max_tokens"].get<int>();
        }

        std::unique_lock<std::shared_mutex> lock(mutex_);

        // Extract configuration fields
        embedding_provider_      = j.value("embedding_provider",      std::string{"hash"});
        embedding_dim_           = j.value("embedding_dim",           128);
        table_name_              = j.value("table_name",              std::string{"wiki_chunks"});
        rocksdb_dir_             = j.value("rocksdb_dir",             std::string{});
        workspace_root_          = j.value("workspace_root",          std::string{});
        json_index_path_         = j.value("json_index_path",         std::string{});
        retrieval_top_k_         = j.value("retrieval_top_k",         5);
        retrieval_min_score_     = j.value("retrieval_min_score",     0.0f);
        // Fail-closed by default when a RocksDB path is explicitly configured.
        // In-memory fallback is allowed only when explicitly enabled
        // (test/degraded mode).
        fail_open_               = j.value("fail_open",               false);
        const bool enforce_process_policy =
            j.value("enforce_process_policy", true);
        process_policy_hot_reload_ =
            j.value("process_policy_hot_reload", true);
        const std::string configured_process_policy_path =
            j.value("process_policy_path", std::string{});
        lint_max_staleness_days_ = j.value("lint_max_staleness_days", 30);
        has_wikipedia_license_   = j.value("llm_wiki_wikipedia",      false);

        const int splitter_max     = j.value("splitter_max_tokens",     220);
        const int splitter_overlap = j.value("splitter_overlap_tokens", 40);
        splitter_ = themis::llm::WikiChunkSplitter(splitter_max, splitter_overlap);

        // When workspace_root is set but no explicit json_index_path given,
        // default the index to <workspace_root>/plugin_chunks.json
        if (!workspace_root_.empty() && json_index_path_.empty()) {
            json_index_path_ =
                (std::filesystem::path(workspace_root_) / "plugin_chunks.json").string();
        }

        if (enforce_process_policy) {
            themis::llm_wiki::LLMWikiProcessPolicy loaded_policy;
            process_policy_path_ =
                resolveProcessPolicyPath(configured_process_policy_path);
            const auto policy_status =
                themis::llm_wiki::ProcessPolicyManager::loadFromYaml(
                    process_policy_path_, loaded_policy);
            if (!policy_status.ok()) {
                return Status::Error(
                    "LLM Wiki process policy validation failed (fail-closed): " +
                    policy_status.message);
            }
            if (fail_open_ && loaded_policy.mode != "shadow") {
                return Status::Error(
                    "fail_open=true is allowed only when process policy mode is "
                    "'shadow'; current mode: " +
                    loaded_policy.mode);
            }
            process_policy_ = std::move(loaded_policy);
            std::error_code ec;
            process_policy_last_write_time_ =
                std::filesystem::last_write_time(process_policy_path_, ec);
            if (ec) {
                return Status::Error(
                    "LLM Wiki process policy mtime lookup failed (fail-closed): " +
                    ec.message());
            }
            spdlog::info(
                "[llm_wiki] Loaded process policy id={} mode={} path={}",
                process_policy_->policy_id, process_policy_->mode, process_policy_path_);
        } else {
            process_policy_path_.clear();
            process_policy_.reset();
            spdlog::warn(
                "[llm_wiki] Process policy enforcement disabled by config "
                "(enforce_process_policy=false)");
        }

        // ── Phase B activation ─────────────────────────────────────────────────
#ifdef THEMISDB_WIKI_PHASE_B
        if (!rocksdb_dir_.empty()) {
            try {
                themis::RocksDBWrapper::Config rocks_cfg{};
                rocks_cfg.db_path = rocksdb_dir_;
                rocksdb_ = std::make_unique<themis::RocksDBWrapper>(rocks_cfg);
                if (!rocksdb_->open()) {
                    throw std::runtime_error("Failed to open RocksDB at: " + rocksdb_dir_);
                }
                sim_      = std::make_unique<themis::SecondaryIndexManager>(*rocksdb_);
                vim_      = std::make_unique<themis::VectorIndexManager>(*rocksdb_);
                llm_b_    = std::make_unique<themis::llm::EmbeddedLLM>();
                // Inject hash embedding function
                const int dim = embedding_dim_;
                llm_b_->setEmbedFn([dim](const std::string& text) {
                    return hashEmbed(text, dim);
                });
                themis::llm::WikiIndexConfig wiki_cfg;
                wiki_cfg.table_name    = table_name_;
                wiki_cfg.embedding_dim = embedding_dim_;
                wiki_cfg.top_k         = retrieval_top_k_;
                wiki_cfg.min_score     = retrieval_min_score_;
                wiki_store_ = std::make_unique<themis::llm::WikiIndexStore>(
                    *sim_, *vim_, *llm_b_, wiki_cfg);
                phase_b_active_ = true;
                spdlog::info("[llm_wiki] Phase B activated (rocksdb={})", rocksdb_dir_);
            } catch (const std::exception& e) {
                wiki_store_.reset();
                llm_b_.reset();
                vim_.reset();
                sim_.reset();
                rocksdb_.reset();
                phase_b_active_ = false;

                if (!fail_open_) {
                    return Status::Error(
                        "RocksDB backend initialization failed while rocksdb_dir is configured; "
                        "refusing implicit in-memory fallback. "
                        "Set fail_open=true only for explicit test/degraded mode. Cause: " +
                        std::string(e.what()));
                }
                spdlog::warn(
                    "[llm_wiki] Phase B init failed; entering explicit degraded "
                    "in-memory mode because fail_open=true: {}", e.what());
            }
        }
        if (j.contains("fail_open")) {
          fail_open_ = j["fail_open"].get<bool>();
        }
        if (j.contains("llm_wiki_wikipedia")) {
          llm_wiki_wikipedia_ = j["llm_wiki_wikipedia"].get<bool>();
        }
        if (j.contains("json_index_path")) {
          json_index_path_ = j["json_index_path"].get<std::string>();
        }
        initialized_ = true;
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::Error(std::string("initialize failed: ") + e.what());
    }
}

// ═════════════════════════════════════════════════════════════════════════════
// LLMWikiPluginImpl — private helpers
// ═════════════════════════════════════════════════════════════════════════════

void LLMWikiPluginImpl::persistJsonIndex_locked() {
    if (json_index_path_.empty()) return;

    namespace fs = std::filesystem;
    std::error_code ec;
    fs::create_directories(fs::path(json_index_path_).parent_path(), ec);

    nlohmann::json arr = nlohmann::json::array();
    for (const auto& chunk : chunks_) {
        arr.push_back({
            {"chunk_id",      chunk.chunk_id},
            {"file_path",     chunk.source_path},
            {"section_title", chunk.section_title},
            {"line_start",    chunk.line_start},
            {"line_end",      chunk.line_end},
            {"text",          chunk.text}
        });
    }

    const std::string tmp_path = json_index_path_ + ".tmp";
    {
        std::ofstream ofs(tmp_path);
        if (!ofs.is_open()) {
            spdlog::warn("[llm_wiki] Cannot open {} for writing", tmp_path);
            return;
        }
        ofs << arr.dump(2);
    }

    fs::rename(tmp_path, json_index_path_, ec);
    if (ec) {
        spdlog::warn("[llm_wiki] Rename {} → {} failed: {}", tmp_path,
                     json_index_path_, ec.message());
    }
}

std::vector<themis::llm::WikiChunk> LLMWikiPluginImpl::inMemoryBm25(
    const std::string& query_text,
    int   top_k,
    float min_score) const
{
    // Tokenise query (case-normalised)
    static const std::regex tok_re("[A-Za-z0-9_\\-]+");
    std::vector<std::string> query_tokens;
    {
        auto it  = std::sregex_iterator(query_text.begin(), query_text.end(), tok_re);
        auto end = std::sregex_iterator();
        for (; it != end; ++it) {
            std::string tok = (*it)[0].str();
            std::transform(tok.begin(), tok.end(), tok.begin(),
                           [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
            query_tokens.push_back(std::move(tok));
        }
    }
    if (query_tokens.empty()) return {};

    std::vector<std::pair<float, std::size_t>> scored; // (score, index)
    scored.reserve(chunks_.size());

    for (std::size_t i = 0; i < chunks_.size(); ++i) {
        const auto& chunk = chunks_[i];

        // Build token frequency map for the chunk
        std::unordered_map<std::string, int> tf;
        {
            auto it  = std::sregex_iterator(chunk.text.begin(), chunk.text.end(), tok_re);
            auto end = std::sregex_iterator();
            for (; it != end; ++it) {
                std::string tok = (*it)[0].str();
                std::transform(tok.begin(), tok.end(), tok.begin(),
                               [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
                tf[tok]++;
            }
        }

        int matches = 0;
        for (const auto& qt : query_tokens) {
            if (tf.count(qt)) matches++;
        }

        if (matches > 0) {
            float score = static_cast<float>(matches) /
                          static_cast<float>(query_tokens.size());
            scored.emplace_back(score, i);
        }
    }

    // Sort descending by score
    std::sort(scored.begin(), scored.end(),
              [](const auto& a, const auto& b){ return a.first > b.first; });

    std::vector<themis::llm::WikiChunk> results;
    results.reserve(std::min<std::size_t>(scored.size(),
                                          static_cast<std::size_t>(top_k > 0 ? top_k : scored.size())));
    for (const auto& [score, idx] : scored) {
        if (score < min_score) break;
        if (top_k > 0 && static_cast<int>(results.size()) >= top_k) break;
        themis::llm::WikiChunk c = chunks_[idx];
        c.score = score;
        results.push_back(std::move(c));
    }
    return results;
}

LLMWikiPluginImpl::StageGateDecision LLMWikiPluginImpl::evaluateStageGate(
    const char* stage_name,
    const bool immediate_execution) const {
    std::lock_guard<std::mutex> policy_lock(process_policy_mutex_);
    if (!process_policy_.has_value()) {
        return StageGateDecision{};
    }

    const ::themis::llm_wiki::StagePolicy* stage_policy = nullptr;
    std::string reason_code;
    if (std::string_view(stage_name) == "ingest") {
        stage_policy = &process_policy_->ingest;
        reason_code = "LLMWIKI_DENY_STAGE_INGEST_DISABLED";
    } else if (std::string_view(stage_name) == "extract") {
        stage_policy = &process_policy_->extract;
        reason_code = "LLMWIKI_DENY_STAGE_EXTRACT_DISABLED";
    } else if (std::string_view(stage_name) == "synthesize") {
        stage_policy = &process_policy_->synthesize;
        reason_code = "LLMWIKI_DENY_STAGE_SYNTHESIZE_DISABLED";
    } else if (std::string_view(stage_name) == "validate") {
        stage_policy = &process_policy_->validate;
        reason_code = "LLMWIKI_DENY_STAGE_VALIDATE_DISABLED";
    } else {
        return StageGateDecision{};
    }

    if (immediate_execution &&
        stage_policy->schedule == ::themis::llm_wiki::ProcessSchedule::Batch) {
        StageGateDecision deferred;
        deferred.allowed = false;
        deferred.reason_code = std::move(reason_code) + "_SCHEDULE_BATCH_ONLY";
        deferred.message = "Stage '" + std::string(stage_name) +
                           "' is batch-scheduled and cannot execute in immediate mode";
        return deferred;
    }

    if (stage_policy->enabled) {
        return StageGateDecision{};
    }

    StageGateDecision denied;
    denied.allowed = false;
    denied.reason_code = std::move(reason_code);
    denied.message = "Stage '" + std::string(stage_name) + "' disabled by policy " +
                     process_policy_->policy_id;
    return denied;
}

LLMWikiPluginImpl::StageGateDecision LLMWikiPluginImpl::maybeReloadProcessPolicy(
    const char* trigger_stage) {
    if (!process_policy_hot_reload_ || process_policy_path_.empty()) {
        return StageGateDecision{};
    }

    std::error_code ec;
    const auto current_write_time =
        std::filesystem::last_write_time(process_policy_path_, ec);
    if (ec) {
        StageGateDecision denied;
        denied.allowed = false;
        denied.reason_code = "LLMWIKI_DENY_POLICY_RELOAD_MISSING";
        denied.message = "Policy reload failed at stage '" + std::string(trigger_stage) +
                         "': " + ec.message();
        return denied;
    }

    std::lock_guard<std::mutex> policy_lock(process_policy_mutex_);
    if (!process_policy_.has_value() ||
        current_write_time == process_policy_last_write_time_) {
        return StageGateDecision{};
    }

    ::themis::llm_wiki::LLMWikiProcessPolicy reloaded_policy;
    const auto status = ::themis::llm_wiki::ProcessPolicyManager::loadFromYaml(
        process_policy_path_, reloaded_policy);
    if (!status.ok()) {
        StageGateDecision denied;
        denied.allowed = false;
        denied.reason_code = "LLMWIKI_DENY_POLICY_RELOAD_INVALID";
        denied.message = "Policy reload rejected at stage '" +
                         std::string(trigger_stage) + "': " + status.message;
        return denied;
    }
    if (fail_open_ && reloaded_policy.mode != "shadow") {
        StageGateDecision denied;
        denied.allowed = false;
        denied.reason_code = "LLMWIKI_DENY_POLICY_RELOAD_MODE";
        denied.message = "Policy reload rejected at stage '" +
                         std::string(trigger_stage) +
                         "': fail_open=true requires shadow mode";
        return denied;
    }

    process_policy_ = std::move(reloaded_policy);
    process_policy_last_write_time_ = current_write_time;
    spdlog::info("[llm_wiki] Hot-reloaded process policy id={} mode={} at stage={}",
                 process_policy_->policy_id, process_policy_->mode, trigger_stage);
    return StageGateDecision{};
}

bool LLMWikiPluginImpl::isInteractiveSchedule(const char* stage_name) const {
    std::lock_guard<std::mutex> policy_lock(process_policy_mutex_);
    if (!process_policy_.has_value()) {
        return false;
    }

    const ::themis::llm_wiki::StagePolicy* stage_policy = nullptr;
    if (std::string_view(stage_name) == "ingest") {
        stage_policy = &process_policy_->ingest;
    } else if (std::string_view(stage_name) == "extract") {
        stage_policy = &process_policy_->extract;
    } else if (std::string_view(stage_name) == "synthesize") {
        stage_policy = &process_policy_->synthesize;
    } else if (std::string_view(stage_name) == "validate") {
        stage_policy = &process_policy_->validate;
    } else if (std::string_view(stage_name) == "re_anchor") {
        stage_policy = &process_policy_->re_anchor;
    }
    return stage_policy != nullptr &&
           stage_policy->schedule == ::themis::llm_wiki::ProcessSchedule::Interactive;
}

void LLMWikiPluginImpl::persistDenyEvidence(
    const char* stage_name,
    const std::string& reason_code,
    const std::string& message) const {
    std::string workspace_root;
    std::string policy_id;
    {
        std::unique_lock<std::shared_mutex> state_lock(mutex_);
        if (workspace_root_.empty()) {
            return;
        }
        workspace_root = workspace_root_;
        policy_id = process_policy_.has_value() ? process_policy_->policy_id : "";
    }

    const auto evidence_path = std::filesystem::path(workspace_root) / "wiki" /
                               "governance_evidence.jsonl";
    nlohmann::json event = {
        {"timestamp", currentIsoUtcTimestamp()},
        {"event", "stage_gate_denied"},
        {"stage", stage_name},
        {"reason_code", reason_code},
        {"message", message},
        {"policy_id", policy_id},
    };

    std::lock_guard<std::mutex> evidence_lock(evidence_mutex_);
    std::error_code ec;
    std::filesystem::create_directories(evidence_path.parent_path(), ec);

    std::ofstream ofs(evidence_path, std::ios::app);
    if (!ofs.is_open()) {
        spdlog::warn("[llm_wiki] Failed to persist governance evidence to {}",
                     evidence_path.string());
        return;
    }
    ofs << event.dump() << "\n";
}

// ═════════════════════════════════════════════════════════════════════════════
// LLMWikiPluginImpl — ILLMWikiPlugin::ingest
// ═════════════════════════════════════════════════════════════════════════════

WikiIngestResult LLMWikiPluginImpl::ingest(
    const std::string& source_path,
    const WikiIngestOptions& opts)
{
    WikiIngestResult result;
    if (!initialized_.load()) {
        spdlog::warn("[llm_wiki] ingest() called before initialize()");
        return result;
    }
    if (const auto reload = maybeReloadProcessPolicy("ingest"); !reload.allowed) {
        spdlog::warn("[llm_wiki] {} [{}]", reload.message, reload.reason_code);
        persistDenyEvidence("policy_reload", reload.reason_code, reload.message);
        result.errors++;
        result.failed_files.push_back("policy_reload_denied: " + reload.reason_code);
        return result;
    }
    if (const auto gate = evaluateStageGate("ingest", /*immediate_execution=*/true);
        !gate.allowed) {
        spdlog::warn("[llm_wiki] {} [{}]", gate.message, gate.reason_code);
        persistDenyEvidence("ingest", gate.reason_code, gate.message);
        result.errors++;
        result.failed_files.push_back("policy_denied: " + gate.reason_code);
        return result;
    }

    namespace fs = std::filesystem;
    const auto t_start = std::chrono::steady_clock::now();

    // ── Collect files ──────────────────────────────────────────────────────────
    std::vector<fs::path> files;
    {
        std::error_code ec;
        if (fs::is_regular_file(source_path, ec)) {
            files.push_back(source_path);
        } else if (fs::is_directory(source_path, ec)) {
            auto collector = [&](const fs::path& p) {
                if (matchGlob(p, opts.file_glob)) files.push_back(p);
            };
            if (opts.recursive) {
                for (const auto& entry : fs::recursive_directory_iterator(source_path, ec)) {
                    if (entry.is_regular_file()) collector(entry.path());
                }
            } else {
                for (const auto& entry : fs::directory_iterator(source_path, ec)) {
                    if (entry.is_regular_file()) collector(entry.path());
                }
            }
        } else {
            spdlog::error("[llm_wiki] source_path does not exist or is not accessible: {}",
                          source_path);
            result.errors++;
            result.failed_files.push_back(source_path);
            return result;
        }
    }

    if (!initialized_) {
        result.errors = 1;
        result.failed_files.push_back(source_path);
        return result;
    }

    std::vector<themis::llm::WikiChunk> chunks = {};

    if (matchGlob(std::filesystem::path(source_path), opts.file_glob)) {
        themis::llm::WikiChunk chunk;
        chunk.source_path = source_path;
        chunk.section_title = "Document";
        chunk.text = "Ingested via ThemisDB core LLM Wiki integration.";
        chunks.push_back(chunk);
    }

    if (workspace_.get() == nullptr) {
        workspace_ = std::make_unique<WikiWorkspaceOrchestrator>();
        const auto init_status = workspace_->init(
            workspace_root_.empty() ? std::filesystem::current_path().string() : workspace_root_);
        if (!init_status.ok()) {
            result.errors = 1;
            result.failed_files.push_back(source_path);
            return result;
        }
    }

    result = workspace_->ingest(source_path, opts, chunks);
    chunks_.insert(chunks_.end(), chunks.begin(), chunks.end());
    return result;
}

WikiQueryResult LLMWikiPluginImpl::query(
    const std::string& query_text,
    const WikiQueryOptions& opts)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    WikiQueryResult result;
    if (!initialized_.load()) {
        spdlog::warn("[llm_wiki] query() called before initialize()");
        return result;
    }
    if (const auto reload = maybeReloadProcessPolicy("extract"); !reload.allowed) {
        spdlog::warn("[llm_wiki] {} [{}]", reload.message, reload.reason_code);
        persistDenyEvidence("policy_reload", reload.reason_code, reload.message);
        return result;
    }
    if (const auto gate = evaluateStageGate("extract", /*immediate_execution=*/true);
        !gate.allowed) {
        spdlog::warn("[llm_wiki] {} [{}]", gate.message, gate.reason_code);
        persistDenyEvidence("extract", gate.reason_code, gate.message);
        return result;
    }

    const auto t_start = std::chrono::steady_clock::now();

    result.query_flagged_for_prompt_injection = containsUnsafePattern(query_text);
    if (result.query_flagged_for_prompt_injection) {
        if (!fail_open_) {
            return result;
        }
    }

    if (!workspace_) {
        return result;
    }

    // Guardrail: filter unsafe chunks
    if (const auto gate = evaluateStageGate("validate", /*immediate_execution=*/true);
        !gate.allowed) {
        spdlog::warn("[llm_wiki] {} [{}]", gate.message, gate.reason_code);
        persistDenyEvidence("validate", gate.reason_code, gate.message);
        return result;
    }

    for (auto& chunk : raw_candidates) {
        if (containsUnsafePattern(chunk.text)) {
            result.filtered_unsafe_chunks++;
            spdlog::debug("[llm_wiki] Filtered unsafe chunk: {} ({})",
                          chunk.chunk_id, chunk.source_path);
        } else {
            result.filtered_unsafe_chunks++;
        }
    }
    if (const auto gate = evaluateStageGate("synthesize", /*immediate_execution=*/true);
        !gate.allowed) {
        spdlog::warn("[llm_wiki] {} [{}]", gate.message, gate.reason_code);
        persistDenyEvidence("synthesize", gate.reason_code, gate.message);
        result.candidates.clear();
        result.filtered_unsafe_chunks = 0;
        return result;
    }

    const auto t_end = std::chrono::steady_clock::now();
    result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(t_end - t_start);
    int interactive_timeout_ms = -1;
    {
        std::lock_guard<std::mutex> policy_lock(process_policy_mutex_);
        if (process_policy_.has_value()) {
            interactive_timeout_ms = process_policy_->interactive_timeout_ms;
        }
    }
    if (isInteractiveSchedule("synthesize") &&
        interactive_timeout_ms > 0 &&
        result.duration.count() > interactive_timeout_ms) {
        const std::string reason = "LLMWIKI_DENY_STAGE_SYNTHESIZE_INTERACTIVE_TIMEOUT";
        const std::string message = "synthesize interactive timeout exceeded: " +
                                    std::to_string(result.duration.count()) + "ms > " +
                                    std::to_string(interactive_timeout_ms) +
                                    "ms";
        spdlog::warn("[llm_wiki] {} [{}]", message, reason);
        persistDenyEvidence("synthesize", reason, message);
        result.candidates.clear();
        result.filtered_unsafe_chunks = 0;
        return result;
    }

    spdlog::debug("[llm_wiki] query: candidates={} filtered={} flagged={} [{} ms]",
                  static_cast<int>(result.candidates.size()),
                  result.filtered_unsafe_chunks,
                  result.query_flagged_for_prompt_injection,
                  result.duration.count());
    return result;
}

Status LLMWikiPluginImpl::wikiInit(const std::string& workspace_root) {
    std::unique_lock<std::shared_mutex> lock(mutex_);
    workspace_root_ = workspace_root;
    if (!workspace_) {
      workspace_ = std::make_unique<WikiWorkspaceOrchestrator>();
    }
    return workspace_->init(workspace_root_);
}

WikiIngestResult LLMWikiPluginImpl::wikiIngest(
    const std::string& source_path,
    const WikiIngestOptions& opts)
{
    WikiIngestResult result;
    if (!initialized_.load()) {
        result.errors++;
        result.failed_files.push_back(source_path);
        return result;
    }
    if (!workspace_) {
        spdlog::error("[llm_wiki] wikiIngest() called without workspace; call wikiInit() first");
        result.errors++;
        result.failed_files.push_back(source_path);
        return result;
    }
    if (const auto reload = maybeReloadProcessPolicy("ingest"); !reload.allowed) {
        spdlog::warn("[llm_wiki] {} [{}]", reload.message, reload.reason_code);
        persistDenyEvidence("policy_reload", reload.reason_code, reload.message);
        result.errors++;
        result.failed_files.push_back("policy_reload_denied: " + reload.reason_code);
        return result;
    }
    if (const auto gate = evaluateStageGate("ingest", /*immediate_execution=*/true);
        !gate.allowed) {
        spdlog::warn("[llm_wiki] {} [{}]", gate.message, gate.reason_code);
        persistDenyEvidence("ingest", gate.reason_code, gate.message);
        result.errors++;
        result.failed_files.push_back("policy_denied: " + gate.reason_code);
        return result;
    }

    // Split source file into chunks
    std::string content;
    {
        std::ifstream ifs(source_path);
        if (!ifs.is_open()) {
            spdlog::error("[llm_wiki] wikiIngest: cannot open {}", source_path);
            result.errors++;
            result.failed_files.push_back(source_path);
            return result;
        }
        content.assign(std::istreambuf_iterator<char>(ifs),
                        std::istreambuf_iterator<char>());
    }

    auto file_chunks = splitter_.split(source_path, content);
    for (auto& c : file_chunks) {
        c.embedding = hashEmbed(c.text, embedding_dim_);
    }

    // Store chunks in the backing store
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
#ifdef THEMISDB_WIKI_PHASE_B
        if (phase_b_active_ && wiki_store_) {
            wiki_store_->writeBatch(file_chunks);
        } else
#endif
        {
            for (auto& c : file_chunks) chunks_.push_back(c);
        }
        if (!json_index_path_.empty()) persistJsonIndex_locked();
    }

    // Orchestrate workspace file-system operations
    return workspace_->ingest(source_path, opts, file_chunks);
}

WikiQueryResult LLMWikiPluginImpl::wikiQuery(
    const std::string& query_text,
    const WikiQueryOptions& opts)
{
    WikiQueryResult result;
    if (!initialized_.load()) {
        spdlog::warn("[llm_wiki] wikiQuery() called before initialize()");
        return result;
    }
    if (!workspace_) {
        spdlog::error("[llm_wiki] wikiQuery() called without workspace; call wikiInit() first");
        return result;
    }
    if (const auto reload = maybeReloadProcessPolicy("extract"); !reload.allowed) {
        spdlog::warn("[llm_wiki] {} [{}]", reload.message, reload.reason_code);
        persistDenyEvidence("policy_reload", reload.reason_code, reload.message);
        return result;
    }
    if (const auto gate = evaluateStageGate("extract", /*immediate_execution=*/true);
        !gate.allowed) {
        spdlog::warn("[llm_wiki] {} [{}]", gate.message, gate.reason_code);
        persistDenyEvidence("extract", gate.reason_code, gate.message);
        return result;
    }

    // Apply guardrails
    if (containsUnsafePattern(query_text)) {
        result.query_flagged_for_prompt_injection = true;
        spdlog::warn("[llm_wiki] wikiQuery: prompt injection pattern in query");
    }

    // Choose active reader
    std::shared_lock<std::shared_mutex> lock(mutex_);

#ifdef THEMISDB_WIKI_PHASE_B
    if (phase_b_active_ && wiki_store_) {
        auto ws_result = workspace_->query(query_text, opts, *wiki_store_);
        if (const auto gate = evaluateStageGate("validate", /*immediate_execution=*/true);
            !gate.allowed) {
            spdlog::warn("[llm_wiki] {} [{}]", gate.message, gate.reason_code);
            persistDenyEvidence("validate", gate.reason_code, gate.message);
            ws_result.candidates.clear();
            ws_result.filtered_unsafe_chunks = 0;
            return ws_result;
        }
        if (const auto gate = evaluateStageGate("synthesize", /*immediate_execution=*/true);
            !gate.allowed) {
            spdlog::warn("[llm_wiki] {} [{}]", gate.message, gate.reason_code);
            persistDenyEvidence("synthesize", gate.reason_code, gate.message);
            ws_result.candidates.clear();
            ws_result.filtered_unsafe_chunks = 0;
        }
        return ws_result;
    }
#endif

    if (json_reader_ && json_reader_->isReady()) {
        auto ws_result = workspace_->query(query_text, opts, *json_reader_);
        if (const auto gate = evaluateStageGate("validate", /*immediate_execution=*/true);
            !gate.allowed) {
            spdlog::warn("[llm_wiki] {} [{}]", gate.message, gate.reason_code);
            persistDenyEvidence("validate", gate.reason_code, gate.message);
            ws_result.candidates.clear();
            ws_result.filtered_unsafe_chunks = 0;
            return ws_result;
        }
        if (const auto gate = evaluateStageGate("synthesize", /*immediate_execution=*/true);
            !gate.allowed) {
            spdlog::warn("[llm_wiki] {} [{}]", gate.message, gate.reason_code);
            persistDenyEvidence("synthesize", gate.reason_code, gate.message);
            ws_result.candidates.clear();
            ws_result.filtered_unsafe_chunks = 0;
        }
        return ws_result;
    }

    // Fallback: create a temporary in-memory reader from chunks_
    // by wrapping an ad-hoc JsonWikiIndexReader isn't possible without a file;
    // instead run in-memory BM25 and let the orchestrator log the query.
    auto mem_result = inMemoryBm25(query_text, opts.top_k, opts.min_score);
    result.candidates = std::move(mem_result);
    result.query_flagged_for_prompt_injection = containsUnsafePattern(query_text);
    if (const auto gate = evaluateStageGate("validate", /*immediate_execution=*/true);
        !gate.allowed) {
        spdlog::warn("[llm_wiki] {} [{}]", gate.message, gate.reason_code);
        persistDenyEvidence("validate", gate.reason_code, gate.message);
        result.candidates.clear();
        result.filtered_unsafe_chunks = 0;
        return result;
    }
    if (const auto gate = evaluateStageGate("synthesize", /*immediate_execution=*/true);
        !gate.allowed) {
        spdlog::warn("[llm_wiki] {} [{}]", gate.message, gate.reason_code);
        persistDenyEvidence("synthesize", gate.reason_code, gate.message);
        result.candidates.clear();
        result.filtered_unsafe_chunks = 0;
    }
    return result;
}

WikiLintResult LLMWikiPluginImpl::wikiLint(
    const std::string& workspace_root,
    int max_staleness_days)
{
    if (!workspace_) {
      workspace_ = std::make_unique<WikiWorkspaceOrchestrator>();
    }
    return workspace_->lint(workspace_root, max_staleness_days);
}

WikiIngestResult LLMWikiPluginImpl::ingestWikipediaDump(
    const std::string& dump_path,
    const WikiDumpIngestOptions& opts)
{
    WikiIngestResult result;

    if (!initialized_.load()) {
        result.errors++;
        return result;
    }
    if (const auto reload = maybeReloadProcessPolicy("ingest"); !reload.allowed) {
        spdlog::warn("[llm_wiki] {} [{}]", reload.message, reload.reason_code);
        persistDenyEvidence("policy_reload", reload.reason_code, reload.message);
        result.errors++;
        result.failed_files.push_back("policy_reload_denied: " + reload.reason_code);
        return result;
    }
    if (const auto gate = evaluateStageGate("ingest", /*immediate_execution=*/false);
        !gate.allowed) {
        spdlog::warn("[llm_wiki] {} [{}]", gate.message, gate.reason_code);
        persistDenyEvidence("ingest", gate.reason_code, gate.message);
        result.errors++;
        result.failed_files.push_back("policy_denied: " + gate.reason_code);
        return result;
    }

    // Edition/feature gate: wikipedia sub-feature
    const auto feature_gate =
        themis::llm_wiki::enforceFeatureGate("llm_wiki_wikipedia");
    if (!feature_gate.ok()) {
        spdlog::warn("[llm_wiki] ingestWikipediaDump() denied by feature gate: {}",
                     feature_gate.message);
        result.errors++;
        result.failed_files.push_back("permission_denied: " + feature_gate.message);
        return result;
    }

    // Runtime license gate: llm_wiki_wikipedia sub-feature
    if (!has_wikipedia_license_) {
        spdlog::warn("[llm_wiki] ingestWikipediaDump() requires the "
                     "'llm_wiki_wikipedia' sub-feature license");
        result.errors++;
        result.failed_files.push_back("permission_denied: missing llm_wiki_wikipedia license");
        return result;
    }

    // Map options to pipeline config
    themis::importers::WikipediaDumpSource source;
    source.source_path = dump_path;
    source.language    = opts.lang;

    themis::importers::WikipediaIngestionConfig cfg;
    cfg.enable_graph_projection    = opts.project_graph;
    cfg.enable_vector_projection   = opts.project_vector;
    cfg.enable_timeseries_projection = opts.project_timeseries;
    if (opts.checkpoint_dir.has_value()) {
        cfg.checkpoint_path = opts.checkpoint_dir.value();
    }

    themis::importers::ImportOptions import_opts;
    import_opts.batch_size = static_cast<std::size_t>(opts.batch_size);

    // Run the pipeline
    try {
        themis::importers::WikipediaIngestionPipeline pipeline(cfg);
        if (!pipeline.initialize()) {
            spdlog::error("[llm_wiki] WikipediaIngestionPipeline::initialize() failed");
            result.errors++;
            return result;
        }

        const auto stats = pipeline.runFullImport(source, import_opts);

        result.files_processed = 1;
        result.chunks_written  = static_cast<int>(stats.imported_records);
        result.chunks_skipped  = static_cast<int>(stats.skipped_records);
        result.errors          = static_cast<int>(stats.failed_records);
        for (const auto& e : stats.errors) {
            result.failed_files.push_back(e);
        }

        pipeline.shutdown();

    } catch (const std::exception& e) {
        spdlog::error("[llm_wiki] ingestWikipediaDump exception: {}", e.what());
        result.errors++;
    }

    return result;
}

WikiWorkspaceStats LLMWikiPluginImpl::stats(const std::string& workspace_root) {
    if (!workspace_) return {};
    return workspace_->stats(workspace_root.empty() ? workspace_root_ : workspace_root);
}

} // namespace llm_wiki
} // namespace plugins
} // namespace themis
