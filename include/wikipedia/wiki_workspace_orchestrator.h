/**
 * @file wiki_workspace_orchestrator.h
 * @brief C++ port of the Python MVP workspace orchestrator for the LLM Wiki plugin.
 *
 * This file was integrated into the core monorepo and retains the canonical
 * workspace-scaffolding logic used by the LLM Wiki implementation.
 */

#pragma once

#include "llm_wiki/llm_wiki_plugin_interface.h"

#include <map>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace plugins {
namespace llm_wiki {

struct WikiPageMeta {
    std::string title;
    std::string source;
    std::string created;
    std::string updated;
};

struct WikiLink {
    std::string from;
    std::string to;
    std::string type = "concept";
};

struct WikiAssertion {
    std::string id;
    std::string text;
    std::string source;
};

struct WikiTask {
    std::string id;
    std::string type;
    std::string status = "open";
    std::vector<std::string> refs;
};

struct WikiState {
    std::string version = "wiki-cpp-1";
    std::map<std::string, WikiPageMeta> pages;
    std::vector<WikiLink> links;
    std::vector<WikiAssertion> assertions;
    std::vector<WikiTask> tasks;
};

class WikiWorkspaceOrchestrator {
public:
    WikiWorkspaceOrchestrator() = default;
    ~WikiWorkspaceOrchestrator() = default;

    WikiWorkspaceOrchestrator(const WikiWorkspaceOrchestrator&) = delete;
    WikiWorkspaceOrchestrator& operator=(const WikiWorkspaceOrchestrator&) = delete;
    WikiWorkspaceOrchestrator(WikiWorkspaceOrchestrator&&) = default;
    WikiWorkspaceOrchestrator& operator=(WikiWorkspaceOrchestrator&&) = default;

    [[nodiscard]] Status init(const std::string& workspace_root);

    [[nodiscard]] WikiIngestResult ingest(
        const std::string& source_path,
        const WikiIngestOptions& opts,
        const std::vector<themis::llm::WikiChunk>& chunks);

    [[nodiscard]] WikiQueryResult query(
        const std::string& query_text,
        const WikiQueryOptions& opts,
        themis::llm::IWikiIndexReader& reader);

    [[nodiscard]] WikiLintResult lint(
        const std::string& workspace_root,
        int max_staleness_days = 30);

    [[nodiscard]] WikiWorkspaceStats stats(const std::string& workspace_root);

private:
    [[nodiscard]] static std::string slugify(const std::string& text);
    [[nodiscard]] static std::string isoTimestamp();
    [[nodiscard]] static WikiState loadState(const std::string& root);
    static void saveState(const std::string& root, const WikiState& state);
    static void appendLog(const std::string& root, const std::string& entry);
    static void rebuildIndex(const std::string& root, const WikiState& state);
    static bool hasContradictionCue(const std::string& text);

    std::mutex mutex_;
    std::string workspace_root_;
};

} // namespace llm_wiki
} // namespace plugins
} // namespace themis
