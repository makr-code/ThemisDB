/**
 * @file wiki_workspace_orchestrator.cpp
 * @brief Implementation of WikiWorkspaceOrchestrator.
 */

#include "wikipedia/wiki_workspace_orchestrator.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <unordered_set>

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace themis {
namespace plugins {
namespace llm_wiki {

namespace {

static constexpr std::array<std::string_view, 5> CONTRADICTION_CUES = {
    "however",
    "but",
    "contradict",
    "in contrast",
    "on the other hand",
};

static constexpr std::string_view SCHEMA_MD = R"( # Wiki Schema

This wiki workspace is maintained by ThemisDB LLM Wiki Plugin.

## Directory layout

| Path | Purpose |
|------|---------|
| `raw_sources/` | Immutable copies of every ingested source file |
| `wiki/pages/`  | LLM-maintained pages (source summaries + query results) |
| `wiki/index.md` | Human-readable content catalogue |
| `wiki/log.md`   | Append-only ingest / query / lint timeline |
| `wiki/state.json` | Structured state: pages, links, assertions, tasks |

## Maintenance rules

1. Never hand-edit `state.json`; let the plugin maintain it.
2. `log.md` is append-only; do not truncate it.
3. Pages in `wiki/pages/` may be freely edited; they are not regenerated
   unless explicitly re-ingested.
4. A `contradiction_review` task in `state.json` flags potential conflicting
   claims; resolve them before closing the task.
)";

static constexpr std::string_view INDEX_HEADER_MD = R"(# Wiki Index

| Page | Source | Updated |
|------|--------|---------|
)";

static constexpr std::string_view LOG_HEADER_MD = R"(# Wiki Log

)";

static constexpr std::string_view STATE_INITIAL_JSON = R"({
  "version": "wiki-cpp-1",
  "pages": {},
  "links": [],
  "assertions": [],
  "tasks": []
})";

void writeIfAbsent(const std::filesystem::path& path, std::string_view content) {
    if (std::filesystem::exists(path)) {
      return;
    }
    std::ofstream ofs(path);
    if (ofs.is_open()) {
        ofs << content;
    }
}

void mkdirP(const std::filesystem::path& p) {
    std::error_code ec;
    std::filesystem::create_directories(p, ec);
}

} // anonymous namespace

std::string WikiWorkspaceOrchestrator::slugify(const std::string& text) {
    std::string result;
    result.reserve(text.size());
    for (unsigned char c : text) {
        if (std::isalnum(c)) {
            result += static_cast<char>(std::tolower(c));
        } else if (c == ' ' || c == '_' || c == '-' || c == '.' || c == '/') {
            if (!result.empty() && result.back() != '-') {
                result += '-';
            }
        }
    }
    while (!result.empty() && result.back() == '-') {
      result.pop_back();
    }
    return result.empty() ? "page" : result;
}

std::string WikiWorkspaceOrchestrator::isoTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
#ifdef _WIN32
    gmtime_s(&tm_buf, &t);
#else
    gmtime_r(&t, &tm_buf);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &tm_buf);
    return buf;
}

WikiState WikiWorkspaceOrchestrator::loadState(const std::string& root) {
    WikiState state;
    auto state_path = std::filesystem::path(root) / "wiki" / "state.json";
    if (!std::filesystem::exists(state_path)) {
      return state;
    }

    std::ifstream ifs(state_path);
    if (!ifs.is_open()) {
      return state;
    }

    try {
        auto j = nlohmann::json::parse(ifs, nullptr, false);
        if (!j.is_object()) {
          return state;
        }

        state.version = j.value("version", std::string{"wiki-cpp-1"});

        if (j.contains("pages") && j["pages"].is_object()) {
            for (auto& [slug, pj] : j["pages"].items()) {
                WikiPageMeta meta;
                meta.title = pj.value("title", slug);
                meta.source = pj.value("source", std::string{});
                meta.created = pj.value("created", std::string{});
                meta.updated = pj.value("updated", std::string{});
                state.pages[slug] = std::move(meta);
            }
        }
        if (j.contains("links") && j["links"].is_array()) {
            for (const auto& lj : j["links"]) {
                WikiLink lnk;
                lnk.from = lj.value("from", std::string{});
                lnk.to = lj.value("to", std::string{});
                lnk.type = lj.value("type", std::string{"concept"});
                if (!lnk.from.empty() && !lnk.to.empty()) {
                    state.links.push_back(std::move(lnk));
                }
            }
        }
        if (j.contains("assertions") && j["assertions"].is_array()) {
            for (const auto& aj : j["assertions"]) {
                WikiAssertion a;
                a.id = aj.value("id", std::string{});
                a.text = aj.value("text", std::string{});
                a.source = aj.value("source", std::string{});
                state.assertions.push_back(std::move(a));
            }
        }
        if (j.contains("tasks") && j["tasks"].is_array()) {
            for (const auto& tj : j["tasks"]) {
                WikiTask task;
                task.id = tj.value("id", std::string{});
                task.type = tj.value("type", std::string{});
                task.status = tj.value("status", std::string{"open"});
                if (tj.contains("refs") && tj["refs"].is_array()) {
                    task.refs = tj["refs"].get<std::vector<std::string>>();
                }
                state.tasks.push_back(std::move(task));
            }
        }
    } catch (const std::exception& e) {
        spdlog::warn("[llm_wiki/orch] Failed to parse state.json: {}", e.what());
    }
    return state;
}

void WikiWorkspaceOrchestrator::saveState(const std::string& root, const WikiState& state) {
    auto wiki_dir = std::filesystem::path(root) / "wiki";
    mkdirP(wiki_dir);

    auto state_path = wiki_dir / "state.json";
    std::string tmp_path = state_path.string() + ".tmp";

    nlohmann::json pages_j = nlohmann::json::object();
    for (const auto& [slug, meta] : state.pages) {
        pages_j[slug] = {
            {"title", meta.title},
            {"source", meta.source},
            {"created", meta.created},
            {"updated", meta.updated},
        };
    }

    nlohmann::json links_j = nlohmann::json::array();
    for (const auto& lnk : state.links) {
        links_j.push_back({{"from", lnk.from}, {"to", lnk.to}, {"type", lnk.type}});
    }

    nlohmann::json assertions_j = nlohmann::json::array();
    for (const auto& a : state.assertions) {
        assertions_j.push_back({{"id", a.id}, {"text", a.text}, {"source", a.source}});
    }

    nlohmann::json tasks_j = nlohmann::json::array();
    for (const auto& t : state.tasks) {
        tasks_j.push_back({
            {"id", t.id},
            {"type", t.type},
            {"status", t.status},
            {"refs", t.refs},
        });
    }

    nlohmann::json j = {
        {"version", state.version},
        {"pages", pages_j},
        {"links", links_j},
        {"assertions", assertions_j},
        {"tasks", tasks_j},
    };

    {
        std::ofstream ofs(tmp_path);
        if (!ofs.is_open()) {
            spdlog::warn("[llm_wiki/orch] Cannot write to {}", tmp_path);
            return;
        }
        ofs << j.dump(2);
    }

    std::error_code ec;
    std::filesystem::rename(tmp_path, state_path, ec);
    if (ec) {
        spdlog::warn("[llm_wiki/orch] Rename {} -> {} failed: {}", tmp_path, state_path.string(), ec.message());
    }
}

void WikiWorkspaceOrchestrator::appendLog(const std::string& root, const std::string& entry) {
    auto log_path = std::filesystem::path(root) / "wiki" / "log.md";
    std::ofstream ofs(log_path, std::ios::app);
    if (ofs.is_open()) {
        ofs << "- " << entry << "\n";
    }
}

void WikiWorkspaceOrchestrator::rebuildIndex(const std::string& root, const WikiState& state) {
    auto index_path = std::filesystem::path(root) / "wiki" / "index.md";
    std::ofstream ofs(index_path);
    if (!ofs.is_open()) {
      return;
    }

    ofs << "# Wiki Index\n\n";
    ofs << "| Page | Source | Updated |\n";
    ofs << "|------|--------|----------|\n";
    for (const auto& [slug, meta] : state.pages) {
        ofs << "| [" << meta.title << "](" << "pages/" << slug << ".md) | "
            << meta.source << " | " << meta.updated << " |\n";
    }
    ofs << "\n_Generated by ThemisDB LLM Wiki Plugin_\n";
}

bool WikiWorkspaceOrchestrator::hasContradictionCue(const std::string& text) {
    std::string lower;
    lower.reserve(text.size());
    for (unsigned char c : text) {
      lower += static_cast<char>(std::tolower(c));
    }

    for (std::string_view cue : CONTRADICTION_CUES) {
        if (lower.find(cue) != std::string::npos) {
          return true;
        }
    }
    return false;
}

Status WikiWorkspaceOrchestrator::init(const std::string& workspace_root) {
    std::lock_guard<std::mutex> lock(mutex_);

    try {
        mkdirP(std::filesystem::path(workspace_root) / "raw_sources");
        mkdirP(std::filesystem::path(workspace_root) / "wiki" / "pages");

        writeIfAbsent(std::filesystem::path(workspace_root) / "wiki" / "schema.md", SCHEMA_MD);
        writeIfAbsent(std::filesystem::path(workspace_root) / "wiki" / "index.md", INDEX_HEADER_MD);
        writeIfAbsent(std::filesystem::path(workspace_root) / "wiki" / "log.md", LOG_HEADER_MD);
        writeIfAbsent(std::filesystem::path(workspace_root) / "wiki" / "state.json", STATE_INITIAL_JSON);

        workspace_root_ = workspace_root;
        spdlog::info("[llm_wiki/orch] Workspace initialised at {}", workspace_root);
        return Status::Ok();
    } catch (const std::exception& e) {
        return Status::Error(std::string("workspace init failed: ") + e.what());
    }
}

WikiIngestResult WikiWorkspaceOrchestrator::ingest(
    const std::string& source_path,
    const WikiIngestOptions& opts,
    const std::vector<themis::llm::WikiChunk>& chunks)
{
    std::lock_guard<std::mutex> lock(mutex_);
    WikiIngestResult result;

    if (workspace_root_.empty()) {
        result.errors++;
        result.failed_files.push_back(source_path);
        spdlog::error("[llm_wiki/orch] ingest() called before init()");
        return result;
    }

    namespace fs = std::filesystem;
    const std::string ts = isoTimestamp();
    const std::string root = workspace_root_;

    fs::path src(source_path);
    const std::string title = opts.page_title.value_or(src.stem().string());
    const std::string slug = slugify(title);

    auto raw_dest = fs::path(root) / "raw_sources" / src.filename();
    if (!fs::exists(raw_dest)) {
        std::error_code ec;
        fs::copy_file(source_path, raw_dest, fs::copy_options::skip_existing, ec);
        if (ec) {
            spdlog::warn("[llm_wiki/orch] copy_file failed ({} → {}): {}", source_path, raw_dest.string(), ec.message());
        }
    }

    mkdirP(fs::path(root) / "wiki" / "pages");
    auto page_path = fs::path(root) / "wiki" / "pages" / (slug + ".md");
    {
        std::ofstream ofs(page_path);
        if (ofs.is_open()) {
            ofs << "# " << title << "\n\n";
            ofs << "**Source**: " << source_path << "\n";
            ofs << "**Ingested**: " << ts << "\n\n";
            ofs << "## Chunks\n\n";
            for (const auto& chunk : chunks) {
                ofs << "### " << (chunk.section_title.empty() ? "Preamble" : chunk.section_title) << "\n\n";
                const auto& txt = chunk.text;
                ofs << "> " << txt.substr(0, std::min<std::size_t>(300, txt.size()));
                if (txt.size() > 300) {
                  ofs << "…";
                }
                ofs << "\n\n";
            }
        }
    }

    WikiState state = loadState(root);
    {
        WikiPageMeta meta;
        meta.title = title;
        meta.source = source_path;
        meta.created = state.pages.count(slug) ? state.pages[slug].created : ts;
        meta.updated = ts;
        state.pages[slug] = meta;
    }

    {
        std::vector<std::string> seen_sections;
        for (const auto& chunk : chunks) {
            if (!chunk.section_title.empty()) {
                std::string ss = slugify(chunk.section_title);
                if (ss != slug && std::find(seen_sections.begin(), seen_sections.end(), ss) == seen_sections.end()) {
                    seen_sections.push_back(ss);
                }
            }
        }
        for (std::size_t i = 0; i < seen_sections.size(); ++i) {
            WikiLink lnk;
            lnk.from = slug;
            lnk.to = seen_sections[i];
            lnk.type = "concept";
            state.links.push_back(lnk);
            if (i + 1 < seen_sections.size()) {
                WikiLink adj;
                adj.from = seen_sections[i];
                adj.to = seen_sections[i + 1];
                adj.type = "concept";
                state.links.push_back(adj);
            }
        }
    }

    {
        bool cue_found = false;
        for (const auto& chunk : chunks) {
            if (hasContradictionCue(chunk.text)) { cue_found = true; break; }
        }
        if (cue_found) {
            WikiTask task;
            task.id = "task-" + slug + "-" + ts.substr(0, 10);
            task.type = "contradiction_review";
            task.status = "open";
            task.refs = {slug};
            bool already_exists = false;
            for (const auto& t : state.tasks) {
                if (t.id == task.id) { already_exists = true; break; }
            }
            if (!already_exists) {
                state.tasks.push_back(std::move(task));
                spdlog::info("[llm_wiki/orch] Contradiction cue detected in '{}'; task created", slug);
            }
        }
    }

    saveState(root, state);
    rebuildIndex(root, state);
    appendLog(root, ts + " INGEST source=" + source_path + " chunks=" + std::to_string(static_cast<int>(chunks.size())));

    result.files_processed = 1;
    result.chunks_written = static_cast<int>(chunks.size());
    return result;
}

WikiQueryResult WikiWorkspaceOrchestrator::query(
    const std::string& query_text,
    const WikiQueryOptions& opts,
    themis::llm::IWikiIndexReader& reader)
{
    std::lock_guard<std::mutex> lock(mutex_);
    WikiQueryResult result;

    const std::string root = opts.workspace_root.value_or(workspace_root_);
    if (root.empty()) {
        spdlog::warn("[llm_wiki/orch] query() called without workspace root");
        return result;
    }

    const std::string ts = isoTimestamp();
    auto raw = reader.query(query_text, opts.top_k, opts.min_score);
    result.candidates = std::move(raw);

    if (opts.save_as_page) {
        namespace fs = std::filesystem;

        const std::string title = opts.page_title.value_or("Query: " + query_text);
        const std::string slug = "query-" + slugify(query_text).substr(0, std::min<std::size_t>(40, slugify(query_text).size()));

        mkdirP(fs::path(root) / "wiki" / "pages");
        auto page_path = fs::path(root) / "wiki" / "pages" / (slug + ".md");

        {
            std::ofstream ofs(page_path);
            if (ofs.is_open()) {
                ofs << "# " << title << "\n\n";
                ofs << "**Query**: " << query_text << "\n";
                ofs << "**Date**: " << ts << "\n\n";
                ofs << "## Results\n\n";
                for (const auto& chunk : result.candidates) {
                    ofs << "### " << (chunk.section_title.empty() ? "Preamble" : chunk.section_title)
                        << " (" << chunk.source_path << ")\n\n";
                    const auto& txt = chunk.text;
                    ofs << "> " << txt.substr(0, std::min<std::size_t>(300, txt.size()));
                    if (txt.size() > 300) {
                      ofs << "…";
                    }
                    ofs << "\n\n---\n\n";
                }
            }
        }

        result.saved_page_path = page_path.string();

        WikiState state = loadState(root);
        {
            WikiPageMeta meta;
            meta.title = title;
            meta.source = "query:" + query_text.substr(0, 60);
            meta.created = ts;
            meta.updated = ts;
            state.pages[slug] = std::move(meta);
        }
        saveState(root, state);
        rebuildIndex(root, state);
    }

    appendLog(root, ts + " QUERY text=" + query_text.substr(0, 60) + " results=" + std::to_string(static_cast<int>(result.candidates.size())));
    return result;
}

WikiLintResult WikiWorkspaceOrchestrator::lint(
    const std::string& workspace_root,
    int max_staleness_days)
{
    std::lock_guard<std::mutex> lock(mutex_);
    WikiLintResult result;

    if (workspace_root.empty()) {
      return result;
    }
    WikiState state = loadState(workspace_root);

    std::unordered_set<std::string> link_targets;
    for (const auto& lnk : state.links) {
        link_targets.insert(lnk.to);
    }

    std::unordered_set<std::string> page_slugs;
    for (const auto& [slug, meta] : state.pages) {
        page_slugs.insert(slug);
    }

    for (const auto& [slug, meta] : state.pages) {
        if (link_targets.find(slug) == link_targets.end()) {
            result.orphan_pages.push_back(slug);
        }
    }

    std::unordered_set<std::string> reported_missing;
    for (const auto& lnk : state.links) {
        if (page_slugs.find(lnk.to) == page_slugs.end() && reported_missing.find(lnk.to) == reported_missing.end()) {
            result.missing_backlinks.push_back(lnk.to);
            reported_missing.insert(lnk.to);
        }
    }

    const std::time_t now = std::time(nullptr);
    for (const auto& [slug, meta] : state.pages) {
        if (meta.updated.empty()) {
          continue;
        }
        std::tm tm_val{};
#ifdef _WIN32
        std::istringstream ss(meta.updated);
        ss >> std::get_time(&tm_val, "%Y-%m-%dT%H:%M:%SZ");
        std::time_t ts = _mkgmtime(&tm_val);
#else
        strptime(meta.updated.c_str(), "%Y-%m-%dT%H:%M:%SZ", &tm_val);
        std::time_t ts = timegm(&tm_val);
#endif
        if (ts > 0) {
            const int days = static_cast<int>((now - ts) / 86400);
            if (days > max_staleness_days) {
                result.stale_synthesis_pages.push_back(slug);
            }
        }
    }

    for (const auto& task : state.tasks) {
        if (task.status == "open") {
            result.unresolved_tasks.push_back(task.id);
        }
    }

    appendLog(workspace_root,
        isoTimestamp() + " LINT orphans=" + std::to_string(static_cast<int>(result.orphan_pages.size())) +
        " missing=" + std::to_string(static_cast<int>(result.missing_backlinks.size())) +
        " stale=" + std::to_string(static_cast<int>(result.stale_synthesis_pages.size())) +
        " tasks=" + std::to_string(static_cast<int>(result.unresolved_tasks.size())));

    return result;
}

WikiWorkspaceStats WikiWorkspaceOrchestrator::stats(const std::string& workspace_root) {
    std::lock_guard<std::mutex> lock(mutex_);
    WikiWorkspaceStats s;
    if (workspace_root.empty()) {
      return s;
    }

    WikiState state = loadState(workspace_root);
    s.wiki_pages = static_cast<int>(state.pages.size());

    for (const auto& task : state.tasks) {
        if (task.status == "open") {
          s.open_tasks++;
        }
    }

    std::unordered_set<std::string> link_targets;
    for (const auto& lnk : state.links) {
        link_targets.insert(lnk.to);
    }
    for (const auto& [slug, meta] : state.pages) {
        if (link_targets.find(slug) == link_targets.end()) {
            s.orphan_pages++;
        }
    }

    return s;
}

} // namespace llm_wiki
} // namespace plugins
} // namespace themis
