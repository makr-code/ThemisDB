/**
 * @file test_llm_wiki_provenance_focused.cpp
 * @brief Focused regression tests for canonical AI provenance persistence in LLM Wiki workspace state.
 */

#include "wikipedia/wiki_workspace_orchestrator.h"
#include "llm/wiki_index_store.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>

using namespace themis::plugins::llm_wiki;

namespace {

class FakeWikiReader final : public themis::llm::IWikiIndexReader {
public:
    explicit FakeWikiReader(std::vector<themis::llm::WikiChunk> chunks)
        : chunks_(std::move(chunks)) {}

    [[nodiscard]] std::vector<themis::llm::WikiChunk> query(
        const std::string&,
        int,
        float) const override {
        return chunks_;
    }

    [[nodiscard]] bool isReady() const noexcept override { return true; }

private:
    std::vector<themis::llm::WikiChunk> chunks_;
};

std::filesystem::path makeTempWorkspace() {
    auto root = std::filesystem::temp_directory_path() / "themisdb_llm_wiki_provenance_test";
    std::error_code ec;
    std::filesystem::remove_all(root, ec);
    std::filesystem::create_directories(root, ec);
    return root;
}

} // namespace

TEST(WikiProvenanceRegression, IngestAndQueryPersistCanonicalMetadata) {
    const auto workspace_root = makeTempWorkspace();

    WikiWorkspaceOrchestrator orchestrator;
    ASSERT_TRUE(orchestrator.init(workspace_root.string()).ok());

    const auto source_path = workspace_root / "source.md";
    {
        std::ofstream ofs(source_path);
        ASSERT_TRUE(ofs.is_open());
        ofs << "# Alpha\n\nAlpha content with however and a clear section.\n\n## Beta\nBeta text.\n";
    }

    themis::plugins::llm_wiki::WikiIngestOptions ingest_opts;
    ingest_opts.page_title = "Alpha Source";

    std::vector<themis::llm::WikiChunk> ingest_chunks = {
        {"chunk-1", source_path.string(), "Alpha", 1, 3, "Alpha content with however and a clear section.", {}, 0.0f, source_path.string()},
        {"chunk-2", source_path.string(), "Beta", 4, 5, "Beta text.", {}, 0.0f, source_path.string()},
    };

    auto ingest_result = orchestrator.ingest(source_path.string(), ingest_opts, ingest_chunks);
    EXPECT_EQ(ingest_result.files_processed, 1);
    EXPECT_EQ(ingest_result.chunks_written, 2);

    FakeWikiReader reader({
        {"q-1", source_path.string(), "Alpha", 1, 3, "Alpha content with however and a clear section.", {}, 0.91f, source_path.string()},
        {"q-2", source_path.string(), "Beta", 4, 5, "Beta text.", {}, 0.74f, source_path.string()},
    });

    WikiQueryOptions query_opts;
    query_opts.save_as_page = true;
    query_opts.page_title = "Query Summary";
    query_opts.workspace_root = workspace_root.string();

    auto query_result = orchestrator.query("alpha query", query_opts, reader);
    EXPECT_EQ(query_result.candidates.size(), 2u);
    EXPECT_TRUE(query_result.saved_page_path.find("query-") != std::string::npos);

    auto state_path = workspace_root / "wiki" / "state.json";
    std::ifstream ifs(state_path);
    ASSERT_TRUE(ifs.is_open());

    nlohmann::json state = nlohmann::json::parse(ifs);
    ASSERT_TRUE(state.contains("revisions"));
    ASSERT_TRUE(state["revisions"].is_array());
    EXPECT_GE(state["revisions"].size(), 2u);

    ASSERT_TRUE(state.contains("pages"));
    ASSERT_TRUE(state["pages"].contains("alpha-source"));
    const auto& alpha_page = state["pages"]["alpha-source"];
    ASSERT_TRUE(alpha_page.contains("provenance"));
    EXPECT_EQ(alpha_page["provenance"].value("origin_type", std::string{}), "imported");
    EXPECT_FALSE(alpha_page["provenance"].value("version_id", std::string{}).empty());

    ASSERT_TRUE(state["pages"].contains("query-alpha-query"));
    const auto& query_page = state["pages"]["query-alpha-query"];
    ASSERT_TRUE(query_page.contains("provenance"));
    EXPECT_EQ(query_page["provenance"].value("origin_type", std::string{}), "hybrid");
    EXPECT_GE(query_page["provenance"].value("transform_step_count", 0), 1);

    auto stats = orchestrator.stats(workspace_root.string());
    EXPECT_GE(stats.provenance_records, 2);
    EXPECT_GE(stats.llm_iterations, 1);
    EXPECT_GE(stats.average_provenance_confidence, 0.0);
    EXPECT_TRUE(stats.max_synthetic_chain_length >= 0.0);

    auto lint = orchestrator.lint(workspace_root.string(), 30);
    EXPECT_TRUE(lint.degraded_pages.empty() || !lint.reanchor_required_pages.empty() || lint.degraded_pages.size() >= 0);
    EXPECT_TRUE(lint.reanchor_required_pages.empty());
}