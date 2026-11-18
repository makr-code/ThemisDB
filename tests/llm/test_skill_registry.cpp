#include "llm/ai_orchestrator.h"
#include "llm/skill_registry.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

namespace fs = std::filesystem;
using themis::errors::ErrorCode;
using themis::llm::ModeSpec;
using themis::llm::SkillRegistry;
using themis::llm::SkillRegistryOptions;
using themis::llm::ToolRegistry;
using themis::llm::registerSkillTools;

class TemporaryDirectory {
public:
    TemporaryDirectory() {
        static std::atomic_uint64_t sequence{0};
        const auto timestamp = std::chrono::steady_clock::now()
                                   .time_since_epoch()
                                   .count();
        path_ = fs::temp_directory_path() /
                ("themis_skill_registry_" + std::to_string(timestamp) + "_" +
                 std::to_string(sequence.fetch_add(1)));
        fs::create_directories(path_);
    }

    ~TemporaryDirectory() {
        std::error_code error;
        fs::remove_all(path_, error);
    }

    [[nodiscard]] const fs::path& path() const { return path_; }

private:
    fs::path path_;
};

void writeFile(const fs::path& path, const std::string& contents) {
    fs::create_directories(path.parent_path());
    std::ofstream output(path, std::ios::binary);
    if (!output.is_open()) {
        throw std::runtime_error("test fixture could not be written");
    }
    output.write(contents.data(), static_cast<std::streamsize>(contents.size()));
}

fs::path writeSkill(const fs::path& root,
                    const std::string& name,
                    const std::string& instructions = "Use the documented process.\n") {
    const auto package = root / name;
    writeFile(package / "SKILL.md",
              "---\nname: " + name + "\ndescription: Guide for " + name +
                  ".\n---\n" + instructions);
    return package;
}

SkillRegistryOptions optionsFor(const fs::path& root) {
    SkillRegistryOptions options;
    options.roots = {root};
    return options;
}

TEST(SkillRegistryTest, DiscoversAndLoadsShippedSkillsAndReference) {
    const auto repo_root =
        fs::path(__FILE__).parent_path().parent_path().parent_path();
    auto skills = std::make_shared<SkillRegistry>(
        optionsFor(repo_root / "config" / "agent_skills"));
    const auto refreshed = skills->refresh();
    ASSERT_TRUE(refreshed);
    EXPECT_EQ(*refreshed, 3u);

    ToolRegistry tools;
    registerSkillTools(tools, skills);
    ModeSpec mode;
    mode.tools_allowed = {"list_skills", "load_skill", "read_skill_resource"};
    const auto catalog = tools.invokeTool("list_skills", {}, mode);
    ASSERT_TRUE(catalog.contains("skills"));
    ASSERT_EQ(catalog["skills"].size(), 3u);
    EXPECT_EQ(catalog["skills"][0]["name"], "aql-read-query");
    EXPECT_EQ(catalog["skills"][1]["name"], "evidence-answer");
    EXPECT_EQ(catalog["skills"][2]["name"], "graph-exploration");

    for (const auto& entry : catalog["skills"]) {
        ASSERT_TRUE(entry.contains("name"));
        const auto loaded = tools.invokeTool(
            "load_skill", {{"name", entry["name"]}}, mode);
        ASSERT_TRUE(loaded.contains("instructions"));
        EXPECT_FALSE(loaded["instructions"].get<std::string>().empty());
    }

    const auto reference = tools.invokeTool(
        "read_skill_resource",
        {{"name", "evidence-answer"}, {"path", "references/source-checklist.md"}},
        mode);
    ASSERT_TRUE(reference.contains("content"));
    EXPECT_NE(reference["content"].get<std::string>().find("authorized scope"),
              std::string::npos);

    mode.tools_denied = {"load_skill"};
    const auto denied =
        tools.invokeTool("load_skill", {{"name", "aql-read-query"}}, mode);
    EXPECT_TRUE(denied.contains("error"));
}

TEST(SkillRegistryTest, DiscoversMetadataAndLoadsInstructionsOnRequest) {
    TemporaryDirectory temporary;
    const auto root = temporary.path() / "skills";
    const auto package = writeSkill(root, "weather");
    writeSkill(root, "alpha", "Alpha instructions.\n");
    writeFile(package / "references" / "format.md", "Use Celsius.\n");

    SkillRegistry registry(optionsFor(root));
    const auto refresh_result = registry.refresh();
    ASSERT_TRUE(refresh_result);
    EXPECT_EQ(*refresh_result, 2u);

    const auto listed = registry.list();
    ASSERT_TRUE(listed);
    ASSERT_EQ(listed->size(), 2u);
    EXPECT_EQ((*listed)[0].name, "alpha");
    EXPECT_EQ((*listed)[0].description, "Guide for alpha.");
    EXPECT_EQ((*listed)[1].name, "weather");
    EXPECT_EQ((*listed)[1].description, "Guide for weather.");

    const auto document = registry.load("weather");
    ASSERT_TRUE(document);
    EXPECT_EQ(document->instructions, "Use the documented process.\n");

    const auto resource = registry.readResource("weather", "references/format.md");
    ASSERT_TRUE(resource);
    EXPECT_EQ(*resource, "Use Celsius.\n");
}

TEST(SkillRegistryTest, DiscoveryDoesNotApplyInstructionBodyLimit) {
    TemporaryDirectory temporary;
    writeSkill(temporary.path() / "skills", "large-body",
               std::string(64 * 1024, 'x'));

    auto options = optionsFor(temporary.path() / "skills");
    options.max_instruction_bytes = 8;
    SkillRegistry registry(std::move(options));

    const auto refreshed = registry.refresh();
    ASSERT_TRUE(refreshed);
    EXPECT_EQ(*refreshed, 1u);
    ASSERT_TRUE(registry.list());

    const auto document = registry.load("large-body");
    ASSERT_FALSE(document);
    EXPECT_EQ(document.error().code(), ErrorCode::ERR_LLM_SKILL_SIZE_LIMIT);
}

TEST(SkillRegistryTest, RejectsOversizedFrontmatterDuringDiscovery) {
    TemporaryDirectory temporary;
    writeSkill(temporary.path() / "skills", "large-metadata",
               "Instructions.\n");

    auto options = optionsFor(temporary.path() / "skills");
    options.max_frontmatter_bytes = 16;
    SkillRegistry registry(std::move(options));
    const auto refreshed = registry.refresh();

    ASSERT_FALSE(refreshed);
    EXPECT_EQ(refreshed.error().code(), ErrorCode::ERR_LLM_SKILL_SIZE_LIMIT);
}

TEST(SkillRegistryTest, ListRequiresSuccessfulRefresh) {
    TemporaryDirectory temporary;
    SkillRegistry registry(optionsFor(temporary.path()));

    const auto listed = registry.list();
    ASSERT_FALSE(listed);
    EXPECT_EQ(listed.error().code(),
              ErrorCode::ERR_LLM_SKILL_REGISTRY_NOT_INITIALIZED);
}

TEST(SkillRegistryTest, InvalidManifestDoesNotReplacePreviousSnapshot) {
    TemporaryDirectory temporary;
    const auto root = temporary.path() / "skills";
    writeSkill(root, "valid");

    SkillRegistry registry(optionsFor(root));
    ASSERT_TRUE(registry.refresh());

    writeFile(root / "broken" / "SKILL.md", "name: broken\ndescription: missing fences\n");
    const auto refresh_result = registry.refresh();
    ASSERT_FALSE(refresh_result);
    EXPECT_EQ(refresh_result.error().code(),
              ErrorCode::ERR_LLM_SKILL_INVALID_METADATA);

    const auto listed = registry.list();
    ASSERT_TRUE(listed);
    ASSERT_EQ(listed->size(), 1u);
    EXPECT_EQ(listed->front().name, "valid");
}

TEST(SkillRegistryTest, ConcurrentRefreshAndReadersObserveCompleteSnapshots) {
    TemporaryDirectory temporary;
    const auto root = temporary.path() / "skills";
    writeSkill(root, "alpha", "Stable instructions.\n");

    SkillRegistry registry(optionsFor(root));
    ASSERT_TRUE(registry.refresh());
    writeSkill(root, "beta", "New instructions.\n");

    std::atomic<bool> start{false};
    std::atomic<bool> finished{false};
    std::atomic<bool> failed{false};
    std::thread refresher([&] {
        while (!start.load()) {
            std::this_thread::yield();
        }
        for (int attempt = 0; attempt < 32; ++attempt) {
            const auto refreshed = registry.refresh();
            if (!refreshed || *refreshed != 2u) {
                failed.store(true);
                break;
            }
        }
        finished.store(true);
    });

    start.store(true);
    while (!finished.load()) {
        const auto listed = registry.list();
        if (!listed || (listed->size() != 1u && listed->size() != 2u)) {
            failed.store(true);
            break;
        }
        const auto document = registry.load("alpha");
        if (!document || document->instructions != "Stable instructions.\n") {
            failed.store(true);
            break;
        }
        std::this_thread::yield();
    }
    refresher.join();

    EXPECT_FALSE(failed.load());
    const auto listed = registry.list();
    ASSERT_TRUE(listed);
    EXPECT_EQ(listed->size(), 2u);
}

TEST(SkillRegistryTest, DuplicateFrontmatterKeysAreRejected) {
    TemporaryDirectory temporary;
    writeFile(temporary.path() / "skills" / "docs" / "SKILL.md",
              "---\nname: docs\nname: alternate\ndescription: Guide.\n---\n");

    SkillRegistry registry(optionsFor(temporary.path() / "skills"));
    const auto refreshed = registry.refresh();

    ASSERT_FALSE(refreshed);
    EXPECT_EQ(refreshed.error().code(),
              ErrorCode::ERR_LLM_SKILL_INVALID_METADATA);
}

TEST(SkillRegistryTest, InvalidUtf8InIgnoredFrontmatterTextIsRejected) {
    TemporaryDirectory temporary;
    std::string manifest =
        "---\nname: docs\ndescription: Guide.\n# ignored comment ";
    manifest.append("\xc3", 1);
    manifest.append("\n---\nInstructions are loaded later.\n");
    writeFile(temporary.path() / "skills" / "docs" / "SKILL.md", manifest);

    SkillRegistry registry(optionsFor(temporary.path() / "skills"));
    const auto refreshed = registry.refresh();

    ASSERT_FALSE(refreshed);
    EXPECT_EQ(refreshed.error().code(), ErrorCode::ERR_LLM_SKILL_INVALID_CONTENT);
}

TEST(SkillRegistryTest, DuplicateNamesAcrossRootsAreRejected) {
    TemporaryDirectory temporary;
    const auto first_root = temporary.path() / "first";
    const auto second_root = temporary.path() / "second";
    writeSkill(first_root, "shared");
    writeSkill(second_root, "shared");

    auto options = optionsFor(first_root);
    options.roots.push_back(second_root);
    SkillRegistry registry(std::move(options));
    const auto refreshed = registry.refresh();

    ASSERT_FALSE(refreshed);
    EXPECT_EQ(refreshed.error().code(),
              ErrorCode::ERR_LLM_SKILL_DUPLICATE_NAME);
    EXPECT_FALSE(registry.list());
}

TEST(SkillRegistryTest, RejectsTraversalAndOversizedResources) {
    TemporaryDirectory temporary;
    const auto root = temporary.path() / "skills";
    const auto package = writeSkill(root, "docs");
    writeFile(package / "large.md", "123456789");

    auto options = optionsFor(root);
    options.max_resource_bytes = 8;
    SkillRegistry registry(std::move(options));
    ASSERT_TRUE(registry.refresh());

    const auto traversal = registry.readResource("docs", "../outside.md");
    ASSERT_FALSE(traversal);
    EXPECT_EQ(traversal.error().code(), ErrorCode::ERR_LLM_SKILL_PATH_ESCAPE);

    const auto absolute = registry.readResource(
        "docs", (temporary.path() / "outside.md").string());
    ASSERT_FALSE(absolute);
    EXPECT_EQ(absolute.error().code(), ErrorCode::ERR_LLM_SKILL_PATH_ESCAPE);

    const auto oversized = registry.readResource("docs", "large.md");
    ASSERT_FALSE(oversized);
    EXPECT_EQ(oversized.error().code(), ErrorCode::ERR_LLM_SKILL_SIZE_LIMIT);
}

TEST(SkillRegistryTest, MissingResourceReturnsNotFound) {
    TemporaryDirectory temporary;
    const auto root = temporary.path() / "skills";
    writeSkill(root, "docs");

    SkillRegistry registry(optionsFor(root));
    ASSERT_TRUE(registry.refresh());

    const auto resource = registry.readResource("docs", "references/missing.md");
    ASSERT_FALSE(resource);
    EXPECT_EQ(resource.error().code(), ErrorCode::ERR_LLM_SKILL_NOT_FOUND);
}

TEST(SkillRegistryTest, RejectsInvalidUtf8AndNulInResourcePaths) {
    TemporaryDirectory temporary;
    const auto root = temporary.path() / "skills";
    writeSkill(root, "docs");

    SkillRegistry registry(optionsFor(root));
    ASSERT_TRUE(registry.refresh());

    const std::string invalid_utf8_path("\xc3\x28", 2);
    const auto invalid_utf8 = registry.readResource("docs", invalid_utf8_path);
    ASSERT_FALSE(invalid_utf8);
    EXPECT_EQ(invalid_utf8.error().code(), ErrorCode::ERR_LLM_SKILL_PATH_ESCAPE);

    const std::string nul_path("missing\0file.md", 15);
    const auto nul = registry.readResource("docs", nul_path);
    ASSERT_FALSE(nul);
    EXPECT_EQ(nul.error().code(), ErrorCode::ERR_LLM_SKILL_PATH_ESCAPE);
}

TEST(SkillRegistryTest, RejectsSymlinkedResources) {
    TemporaryDirectory temporary;
    const auto root = temporary.path() / "skills";
    const auto package = writeSkill(root, "docs");
    const auto outside = temporary.path() / "outside.md";
    writeFile(outside, "outside package\n");

    std::error_code error;
    fs::create_symlink(outside, package / "linked.md", error);
    if (error) {
        GTEST_SKIP() << "Symlink creation is not permitted in this environment";
    }

    SkillRegistry registry(optionsFor(root));
    ASSERT_TRUE(registry.refresh());
    const auto resource = registry.readResource("docs", "linked.md");
    ASSERT_FALSE(resource);
    EXPECT_EQ(resource.error().code(), ErrorCode::ERR_LLM_SKILL_PATH_ESCAPE);
}

TEST(SkillRegistryTest, RejectsSymlinkedPackageDirectoryDuringDiscovery) {
    TemporaryDirectory temporary;
    const auto root = temporary.path() / "skills";
    const auto target = writeSkill(temporary.path() / "external", "docs");
    fs::create_directories(root);

    std::error_code error;
    fs::create_directory_symlink(target, root / "docs", error);
    if (error) {
        GTEST_SKIP() << "Symlink creation is not permitted in this environment";
    }

    SkillRegistry registry(optionsFor(root));
    const auto refreshed = registry.refresh();

    ASSERT_FALSE(refreshed);
    EXPECT_EQ(refreshed.error().code(), ErrorCode::ERR_LLM_SKILL_PATH_ESCAPE);
}

TEST(SkillRegistryTest, RejectsSymlinkedManifestDuringDiscovery) {
    TemporaryDirectory temporary;
    const auto root = temporary.path() / "skills";
    const auto package = root / "docs";
    fs::create_directories(package);
    const auto outside = temporary.path() / "SKILL.md";
    writeFile(outside,
              "---\nname: docs\ndescription: External manifest.\n---\n");

    std::error_code error;
    fs::create_symlink(outside, package / "SKILL.md", error);
    if (error) {
        GTEST_SKIP() << "Symlink creation is not permitted in this environment";
    }

    SkillRegistry registry(optionsFor(root));
    const auto refreshed = registry.refresh();

    ASSERT_FALSE(refreshed);
    EXPECT_EQ(refreshed.error().code(), ErrorCode::ERR_LLM_SKILL_PATH_ESCAPE);
}

TEST(SkillRegistryTest, RejectsManifestReplacedBySymlinkAfterRefresh) {
    TemporaryDirectory temporary;
    const auto root = temporary.path() / "skills";
    const auto package = writeSkill(root, "docs");
    const auto outside = temporary.path() / "outside.md";
    writeFile(outside, "---\nname: docs\ndescription: outside\n---\nsecret\n");

    SkillRegistry registry(optionsFor(root));
    ASSERT_TRUE(registry.refresh());

    std::error_code error;
    fs::remove(package / "SKILL.md", error);
    ASSERT_FALSE(error);
    fs::create_symlink(outside, package / "SKILL.md", error);
    if (error) {
        GTEST_SKIP() << "Symlink creation is not permitted in this environment";
    }

    const auto document = registry.load("docs");
    ASSERT_FALSE(document);
    EXPECT_EQ(document.error().code(), ErrorCode::ERR_LLM_SKILL_PATH_ESCAPE);
}

TEST(SkillRegistryTest, RejectsPackageRootReplacedBySymlinkAfterRefresh) {
    TemporaryDirectory temporary;
    const auto root = temporary.path() / "skills";
    writeSkill(root, "docs");

    SkillRegistry registry(optionsFor(root));
    ASSERT_TRUE(registry.refresh());

    const auto external_root = temporary.path() / "external_skills";
    const auto external_package = writeSkill(external_root, "docs");
    writeFile(external_package / "private.md", "external content\n");

    std::error_code error;
    const auto moved_root = temporary.path() / "original_skills";
    fs::rename(root, moved_root, error);
    if (error) {
        GTEST_SKIP() << "Root replacement is not permitted in this environment";
    }
    fs::create_directory_symlink(external_root, root, error);
    if (error) {
        GTEST_SKIP() << "Directory symlink creation is not permitted in this environment";
    }

    const auto document = registry.load("docs");
    ASSERT_FALSE(document);
    EXPECT_EQ(document.error().code(), ErrorCode::ERR_LLM_SKILL_PATH_ESCAPE);

    const auto resource = registry.readResource("docs", "private.md");
    ASSERT_FALSE(resource);
    EXPECT_EQ(resource.error().code(), ErrorCode::ERR_LLM_SKILL_PATH_ESCAPE);
}

TEST(SkillRegistryTest, RejectsInvalidUtf8Resources) {
    TemporaryDirectory temporary;
    const auto root = temporary.path() / "skills";
    const auto package = writeSkill(root, "docs");
    writeFile(package / "invalid.txt", std::string("\xc3\x28", 2));

    SkillRegistry registry(optionsFor(root));
    ASSERT_TRUE(registry.refresh());
    const auto resource = registry.readResource("docs", "invalid.txt");
    ASSERT_FALSE(resource);
    EXPECT_EQ(resource.error().code(), ErrorCode::ERR_LLM_SKILL_INVALID_CONTENT);
}

TEST(SkillRegistryTest, OptionalToolsRemainGatedByTheActiveMode) {
    TemporaryDirectory temporary;
    const auto package =
        writeSkill(temporary.path() / "skills", "howto", "Do the task.\n");
    writeFile(package / "references" / "guide.md", "Read the guide.\n");
    auto registry = std::make_shared<SkillRegistry>(
        optionsFor(temporary.path() / "skills"));
    ASSERT_TRUE(registry->refresh());

    ToolRegistry tools;
    registerSkillTools(tools, registry);

    ModeSpec mode;
    mode.id = "agent";
    auto result = tools.invokeTool("list_skills", {}, mode);
    EXPECT_TRUE(result.contains("error"));
    result = tools.invokeTool(
        "read_skill_resource",
        {{"name", "howto"}, {"path", "references/guide.md"}}, mode);
    EXPECT_TRUE(result.contains("error"));

    mode.tools_allowed = {"list_skills"};
    result = tools.invokeTool("list_skills", {}, mode);
    ASSERT_TRUE(result.contains("skills"));
    ASSERT_EQ(result["skills"].size(), 1u);
    EXPECT_EQ(result["skills"][0]["name"], "howto");

    result = tools.invokeTool("load_skill", {{"name", "howto"}}, mode);
    EXPECT_TRUE(result.contains("error"));
    result = tools.invokeTool(
        "read_skill_resource",
        {{"name", "howto"}, {"path", "references/guide.md"}}, mode);
    EXPECT_TRUE(result.contains("error"));

    mode.tools_allowed.push_back("load_skill");
    result = tools.invokeTool("load_skill", {{"name", "howto"}}, mode);
    ASSERT_TRUE(result.contains("instructions"));
    EXPECT_EQ(result["instructions"], "Do the task.\n");

    mode.tools_allowed.push_back("read_skill_resource");
    mode.tools_denied = {"read_skill_resource"};
    result = tools.invokeTool(
        "read_skill_resource",
        {{"name", "howto"}, {"path", "references/guide.md"}}, mode);
    EXPECT_TRUE(result.contains("error"));

    mode.tools_denied.clear();
    result = tools.invokeTool(
        "read_skill_resource",
        {{"name", "howto"}, {"path", "references/guide.md"}}, mode);
    ASSERT_TRUE(result.contains("content"));
    EXPECT_EQ(result["content"], "Read the guide.\n");
}

TEST(SkillRegistryTest, ToolNameConflictDoesNotRegisterPartialSkillTools) {
    ToolRegistry tools;
    themis::llm::ToolSpec conflicting;
    conflicting.name = "load_skill";
    tools.registerTool(conflicting, [](const themis::llm::json&,
                                       const ModeSpec&) {
        return themis::llm::json::object();
    });

    auto registry = std::make_shared<SkillRegistry>(SkillRegistryOptions{});
    EXPECT_THROW(registerSkillTools(tools, std::move(registry)),
                 std::invalid_argument);

    const auto names = tools.listTools();
    ASSERT_EQ(names.size(), 1u);
    EXPECT_EQ(names.front(), "load_skill");
}

} // namespace
