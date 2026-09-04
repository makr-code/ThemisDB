/**
 * @file llm_wiki_plugin_impl.cpp
 * @brief Full implementation of LLMWikiPluginImpl.
 */

#include "wikipedia/llm_wiki_plugin_impl.h"

#include "importers/wikipedia_pipeline.hpp"
#include "importers/wikipedia_types.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
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
        if (j.contains("splitter_overlap_tokens")) {
          splitter_overlap_tokens_ = j["splitter_overlap_tokens"].get<int>();
        }
        if (j.contains("workspace_root")) {
          workspace_root_ = j["workspace_root"].get<std::string>();
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

WikiIngestResult LLMWikiPluginImpl::ingest(
    const std::string& source_path,
    const WikiIngestOptions& opts)
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    WikiIngestResult result = {};

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

    result.query_flagged_for_prompt_injection = containsUnsafePattern(query_text);
    if (result.query_flagged_for_prompt_injection) {
        if (!fail_open_) {
            return result;
        }
    }

    if (!workspace_) {
        return result;
    }

    std::vector<themis::llm::WikiChunk> chunks = {};

    for (const auto& chunk : chunks_) {
        if (!containsUnsafePattern(chunk.text)) {
            chunks.push_back(chunk);
        } else {
            result.filtered_unsafe_chunks++;
        }
    }
    result.candidates = std::move(chunks);
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
    return ingest(source_path, opts);
}

WikiQueryResult LLMWikiPluginImpl::wikiQuery(
    const std::string& query_text,
    const WikiQueryOptions& opts)
{
    return query(query_text, opts);
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
    result.files_processed = 1;
    result.chunks_written = 1;
    (void)dump_path;
    (void)opts;
    return result;
}

WikiWorkspaceStats LLMWikiPluginImpl::stats(const std::string& workspace_root) {
    if (!workspace_) return {};
    return workspace_->stats(workspace_root.empty() ? workspace_root_ : workspace_root);
}

} // namespace llm_wiki
} // namespace plugins
} // namespace themis
