/**
 * @file llm_wiki_plugin_impl.h
 * @brief Private implementation header for `LLMWikiPluginImpl`.
 *
 * This file was integrated into the core monorepo and is now part of the
 * canonical ThemisDB implementation.
 */

#pragma once

#include "llm_wiki/llm_wiki_plugin_interface.h"
#include "wikipedia/wiki_workspace_orchestrator.h"

#include "llm/wiki_chunk_splitter.h"
#include "llm/wiki_index_store.h"

#ifdef THEMISDB_WIKI_PHASE_B
#include "index/secondary_index.h"
#include "index/vector_index.h"
#include "llm/embedded_llm.h"
#include "storage/rocksdb_wrapper.h"
#endif

#include <atomic>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_set>
#include <vector>

namespace themis {
namespace plugins {
namespace llm_wiki {

class LLMWikiPluginImpl final : public ILLMWikiPlugin {
public:
    LLMWikiPluginImpl() = default;
    ~LLMWikiPluginImpl() override = default;

    LLMWikiPluginImpl(const LLMWikiPluginImpl&) = delete;
    LLMWikiPluginImpl& operator=(const LLMWikiPluginImpl&) = delete;
    LLMWikiPluginImpl(LLMWikiPluginImpl&&) = delete;
    LLMWikiPluginImpl& operator=(LLMWikiPluginImpl&&) = delete;

    [[nodiscard]] const char* getName() const override { return "LLM Wiki Plugin"; }
    [[nodiscard]] const char* getVersion() const override { return "0.1.0"; }
    [[nodiscard]] PluginType getType() const override { return PluginType::CUSTOM; }

    [[nodiscard]] PluginCapabilities getCapabilities() const override;
    [[nodiscard]] bool initialize(const char* config_json) override;
    void shutdown() noexcept override;
    [[nodiscard]] void* getInstance() override { return static_cast<void*>(this); }

    [[nodiscard]] const char* pluginId() const noexcept override { return "llm_wiki"; }
    [[nodiscard]] const char* pluginVersion() const noexcept override { return "0.1.0"; }

    [[nodiscard]] Status initialize(const std::string& config_json) override;
    [[nodiscard]] WikiIngestResult ingest(
        const std::string& source_path,
        const WikiIngestOptions& opts = {}) override;
    [[nodiscard]] WikiQueryResult query(
        const std::string& query_text,
        const WikiQueryOptions& opts = {}) override;
    [[nodiscard]] Status wikiInit(const std::string& workspace_root) override;
    [[nodiscard]] WikiIngestResult wikiIngest(
        const std::string& source_path,
        const WikiIngestOptions& opts) override;
    [[nodiscard]] WikiQueryResult wikiQuery(
        const std::string& query_text,
        const WikiQueryOptions& opts) override;
    [[nodiscard]] WikiLintResult wikiLint(
        const std::string& workspace_root,
        int max_staleness_days = 30) override;
    [[nodiscard]] WikiIngestResult ingestWikipediaDump(
        const std::string& dump_path,
        const WikiDumpIngestOptions& opts = {}) override;
    [[nodiscard]] WikiWorkspaceStats stats(
        const std::string& workspace_root = {}) override;

    [[nodiscard]] bool isInitialized() const noexcept { return initialized_.load(); }

private:
    void persistJsonIndex_locked();
    [[nodiscard]] std::vector<themis::llm::WikiChunk> inMemoryBm25(
        const std::string& query_text,
        int top_k,
        float min_score) const;

    [[nodiscard]] static std::vector<float> hashEmbed(const std::string& text, int dim);
    [[nodiscard]] static bool containsUnsafePattern(const std::string& text);

    std::shared_mutex mutex_;
    std::unique_ptr<WikiWorkspaceOrchestrator> workspace_;
    std::vector<themis::llm::WikiChunk> chunks_;
    std::filesystem::path json_index_path_;
    std::unique_ptr<themis::llm::IWikiIndexReader> json_reader_;
    std::atomic_bool initialized_{false};
    std::string workspace_root_;
    std::string embedding_provider_ = "hash";
    int embedding_dim_ = 128;
    int splitter_max_tokens_ = 220;
    int splitter_overlap_tokens_ = 40;
    bool fail_open_ = true;
    bool llm_wiki_wikipedia_ = false;
    bool phase_b_active_ = false;

#ifdef THEMISDB_WIKI_PHASE_B
    std::unique_ptr<themis::storage::RocksDBWrapper> rocksdb_;
    std::unique_ptr<themis::index::SecondaryIndexManager> sim_;
    std::unique_ptr<themis::index::VectorIndexManager> vim_;
    std::unique_ptr<themis::llm::EmbeddedLLM> llm_b_;
    std::unique_ptr<themis::llm::WikiIndexStore> wiki_store_;
#endif
};

} // namespace llm_wiki
} // namespace plugins
} // namespace themis
