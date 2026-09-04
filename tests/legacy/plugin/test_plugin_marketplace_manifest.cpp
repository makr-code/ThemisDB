// Tests for plugin marketplace manifest format (JSON schema v2).
//
// Covers:
//   - ManifestSchemaValidator::validate()     – required/optional field rules
//   - ManifestSchemaValidator::parseMarketplaceManifest() – struct population
//   - MarketplaceManifest / PluginSignatureInfo data structures

#include <gtest/gtest.h>
#include "plugins/plugin_interface.h"
#include <nlohmann/json.hpp>
#include <filesystem>
#include <fstream>

// Project root used by integration tests that load plugin.json files from disk.
#ifndef THEMIS_PROJECT_ROOT
#define THEMIS_PROJECT_ROOT "/home/runner/work/ThemisDB/ThemisDB"
#endif

using namespace themis::plugins;
using json = nlohmann::json;
namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static json minimalValidManifest() {
    return json{
        {"name",        "test_plugin"},
        {"version",     "1.0.0"},
        {"type",        "custom"},
        {"description", "A test plugin"},
        {"binary",      json{{"linux", "test_plugin.so"}}}
    };
}

// ---------------------------------------------------------------------------
// ManifestSchemaValidator – required field checks
// ---------------------------------------------------------------------------

TEST(ManifestSchemaValidator, AcceptsMinimalValidManifest) {
    auto result = ManifestSchemaValidator::validate(minimalValidManifest());
    EXPECT_TRUE(result.valid);
    EXPECT_TRUE(result.errors.empty());
}

TEST(ManifestSchemaValidator, RejectsNonObject) {
    auto result = ManifestSchemaValidator::validate(json::array());
    EXPECT_FALSE(result.valid);
    EXPECT_FALSE(result.errors.empty());
}

TEST(ManifestSchemaValidator, RejectsMissingName) {
    auto j = minimalValidManifest();
    j.erase("name");
    auto result = ManifestSchemaValidator::validate(j);
    EXPECT_FALSE(result.valid);
    ASSERT_FALSE(result.errors.empty());
    EXPECT_TRUE(result.errors[0].find("name") != std::string::npos);
}

TEST(ManifestSchemaValidator, RejectsMissingVersion) {
    auto j = minimalValidManifest();
    j.erase("version");
    auto result = ManifestSchemaValidator::validate(j);
    EXPECT_FALSE(result.valid);
}

TEST(ManifestSchemaValidator, RejectsMissingDescription) {
    auto j = minimalValidManifest();
    j.erase("description");
    auto result = ManifestSchemaValidator::validate(j);
    EXPECT_FALSE(result.valid);
}

TEST(ManifestSchemaValidator, RejectsMissingType) {
    auto j = minimalValidManifest();
    j.erase("type");
    auto result = ManifestSchemaValidator::validate(j);
    EXPECT_FALSE(result.valid);
}

TEST(ManifestSchemaValidator, RejectsMissingBinary) {
    auto j = minimalValidManifest();
    j.erase("binary");
    auto result = ManifestSchemaValidator::validate(j);
    EXPECT_FALSE(result.valid);
}

TEST(ManifestSchemaValidator, RejectsEmptyBinaryObject) {
    auto j = minimalValidManifest();
    j["binary"] = json::object();
    auto result = ManifestSchemaValidator::validate(j);
    EXPECT_FALSE(result.valid);
}

// ---------------------------------------------------------------------------
// ManifestSchemaValidator – type enum validation
// ---------------------------------------------------------------------------

TEST(ManifestSchemaValidator, AcceptsAllValidTypes) {
    static const std::vector<std::string> types = {
        "compute_backend", "blob_storage", "importer", "exporter",
        "hsm_provider", "embedding", "llm_backend", "custom"
    };
    for (const auto& t : types) {
        auto j = minimalValidManifest();
        j["type"] = t;
        auto result = ManifestSchemaValidator::validate(j);
        EXPECT_TRUE(result.valid) << "type='" << t << "' should be valid";
    }
}

TEST(ManifestSchemaValidator, RejectsUnknownType) {
    auto j = minimalValidManifest();
    j["type"] = "unknown_type";
    auto result = ManifestSchemaValidator::validate(j);
    EXPECT_FALSE(result.valid);
}

// ---------------------------------------------------------------------------
// ManifestSchemaValidator – binary platform validation
// ---------------------------------------------------------------------------

TEST(ManifestSchemaValidator, AcceptsAllPlatformCombinations) {
    // windows-only
    auto j = minimalValidManifest();
    j["binary"] = json{{"windows", "plugin.dll"}};
    EXPECT_TRUE(ManifestSchemaValidator::validate(j).valid);

    // all three platforms
    j["binary"] = json{
        {"windows", "plugin.dll"},
        {"linux",   "plugin.so"},
        {"macos",   "plugin.dylib"}
    };
    EXPECT_TRUE(ManifestSchemaValidator::validate(j).valid);
}

TEST(ManifestSchemaValidator, RejectsUnknownPlatform) {
    auto j = minimalValidManifest();
    j["binary"] = json{{"amiga", "plugin.lha"}};
    auto result = ManifestSchemaValidator::validate(j);
    EXPECT_FALSE(result.valid);
}

// ---------------------------------------------------------------------------
// ManifestSchemaValidator – optional field validation
// ---------------------------------------------------------------------------

TEST(ManifestSchemaValidator, AcceptsFullMarketplaceFields) {
    auto j = minimalValidManifest();
    j["author"]             = "ThemisDB Team";
    j["license"]            = "MIT";
    j["homepage"]           = "https://example.com";
    j["repository"]         = "https://github.com/example/plugin";
    j["documentation"]      = "docs/README.md";
    j["tags"]               = json::array({"storage", "aws"});
    j["category"]           = "storage";
    j["marketplace_id"]     = "550e8400-e29b-41d4-a716-446655440000";
    j["min_themis_version"] = "1.0.0";
    j["max_themis_version"] = "2.0.0";
    j["verified_publisher"] = true;
    j["auto_load"]          = false;
    j["load_priority"]      = 50;
    j["expected_hash"]      = std::string(64, 'a');
    j["signature"] = json{
        {"fingerprint", "deadbeef00112233"},
        {"algorithm",   "ed25519"},
        {"signed_at",   "2026-01-01T00:00:00Z"}
    };

    auto result = ManifestSchemaValidator::validate(j);
    EXPECT_TRUE(result.valid) << [&]() {
        std::string msg;
        for (const auto& e : result.errors) {
          msg += e + "\n";
        }
        return msg;
    }();
}

TEST(ManifestSchemaValidator, RejectsInvalidCategory) {
    auto j = minimalValidManifest();
    j["category"] = "invalid-cat";
    auto result = ManifestSchemaValidator::validate(j);
    EXPECT_FALSE(result.valid);
}

TEST(ManifestSchemaValidator, AcceptsAllValidCategories) {
    static const std::vector<std::string> cats = {
        "storage", "compute", "security", "data-import", "data-export",
        "machine-learning", "observability", "replication", "custom"
    };
    for (const auto& c : cats) {
        auto j = minimalValidManifest();
        j["category"] = c;
        EXPECT_TRUE(ManifestSchemaValidator::validate(j).valid)
            << "category='" << c << "' should be valid";
    }
}

TEST(ManifestSchemaValidator, RejectsNonBoolAutoLoad) {
    auto j = minimalValidManifest();
    j["auto_load"] = "yes";
    auto result = ManifestSchemaValidator::validate(j);
    EXPECT_FALSE(result.valid);
}

TEST(ManifestSchemaValidator, RejectsLoadPriorityOutOfRange) {
    auto j = minimalValidManifest();
    j["load_priority"] = 9999;
    auto result = ManifestSchemaValidator::validate(j);
    EXPECT_FALSE(result.valid);
}

TEST(ManifestSchemaValidator, AcceptsTagsArray) {
    auto j = minimalValidManifest();
    j["tags"] = json::array({"storage", "cloud"});
    EXPECT_TRUE(ManifestSchemaValidator::validate(j).valid);
}

TEST(ManifestSchemaValidator, RejectsTooManyTags) {
    auto j = minimalValidManifest();
    json tags = json::array();
    for (int i = 0; i < 17; ++i) {
        tags.push_back("tag" + std::to_string(i));
    }
    j["tags"] = tags;
    auto result = ManifestSchemaValidator::validate(j);
    EXPECT_FALSE(result.valid);
}

TEST(ManifestSchemaValidator, RejectsTagsNonArray) {
    auto j = minimalValidManifest();
    j["tags"] = "not-an-array";
    EXPECT_FALSE(ManifestSchemaValidator::validate(j).valid);
}

// ---------------------------------------------------------------------------
// ManifestSchemaValidator – signature sub-object
// ---------------------------------------------------------------------------

TEST(ManifestSchemaValidator, RejectsSignatureMissingFingerprint) {
    auto j = minimalValidManifest();
    j["signature"] = json{{"algorithm", "ed25519"}, {"signed_at", "2026-01-01T00:00:00Z"}};
    EXPECT_FALSE(ManifestSchemaValidator::validate(j).valid);
}

TEST(ManifestSchemaValidator, RejectsSignatureFingerprintTooShort) {
    auto j = minimalValidManifest();
    j["signature"] = json{
        {"fingerprint", "deadbeef"},   // only 8 chars, schema requires >= 16
        {"algorithm",   "ed25519"},
        {"signed_at",   "2026-01-01T00:00:00Z"}
    };
    EXPECT_FALSE(ManifestSchemaValidator::validate(j).valid);
}

TEST(ManifestSchemaValidator, RejectsSignatureInvalidAlgorithm) {
    auto j = minimalValidManifest();
    j["signature"] = json{
        {"fingerprint", "deadbeef00112233"},
        {"algorithm",   "md5"},
        {"signed_at",   "2026-01-01T00:00:00Z"}
    };
    EXPECT_FALSE(ManifestSchemaValidator::validate(j).valid);
}

TEST(ManifestSchemaValidator, AcceptsAllValidSignatureAlgorithms) {
    static const std::vector<std::string> algos = {
        "ed25519", "ecdsa-p256", "rsa-pss-sha256"
    };
    for (const auto& algo : algos) {
        auto j = minimalValidManifest();
        j["signature"] = json{
            {"fingerprint", "deadbeef00112233"},
            {"algorithm",   algo},
            {"signed_at",   "2026-01-01T00:00:00Z"}
        };
        EXPECT_TRUE(ManifestSchemaValidator::validate(j).valid)
            << "algorithm='" << algo << "' should be valid";
    }
}

// ---------------------------------------------------------------------------
// ManifestSchemaValidator – dependencies formats
// ---------------------------------------------------------------------------

TEST(ManifestSchemaValidator, AcceptsDependenciesAsArray) {
    auto j = minimalValidManifest();
    j["dependencies"] = json::array({"nlohmann/json", "openssl"});
    EXPECT_TRUE(ManifestSchemaValidator::validate(j).valid);
}

TEST(ManifestSchemaValidator, AcceptsDependenciesAsObject) {
    auto j = minimalValidManifest();
    j["dependencies"] = json{{"nlohmann/json", ">=3.11.0"}, {"openssl", ">=1.1.1"}};
    EXPECT_TRUE(ManifestSchemaValidator::validate(j).valid);
}

TEST(ManifestSchemaValidator, RejectsDependenciesAsString) {
    auto j = minimalValidManifest();
    j["dependencies"] = "nlohmann/json";
    EXPECT_FALSE(ManifestSchemaValidator::validate(j).valid);
}

// ---------------------------------------------------------------------------
// parseMarketplaceManifest – struct population
// ---------------------------------------------------------------------------

TEST(ParseMarketplaceManifest, ReturnsNulloptForInvalidManifest) {
    auto result = ManifestSchemaValidator::parseMarketplaceManifest(json::object());
    EXPECT_FALSE(result.has_value());
}

TEST(ParseMarketplaceManifest, PopulatesBaseFields) {
    auto j = minimalValidManifest();
    j["binary"] = json{{"linux", "test.so"}, {"windows", "test.dll"}};

    auto m = ManifestSchemaValidator::parseMarketplaceManifest(j);
    ASSERT_TRUE(m.has_value());
    EXPECT_EQ(m->name, "test_plugin");
    EXPECT_EQ(m->version, "1.0.0");
    EXPECT_EQ(m->description, "A test plugin");
    EXPECT_EQ(m->type, PluginType::CUSTOM);
    EXPECT_EQ(m->binary_linux, "test.so");
    EXPECT_EQ(m->binary_windows, "test.dll");
    EXPECT_TRUE(m->binary_macos.empty());
}

TEST(ParseMarketplaceManifest, PopulatesMarketplaceFields) {
    auto j = minimalValidManifest();
    j["author"]             = "Alice";
    j["license"]            = "Apache-2.0";
    j["homepage"]           = "https://example.com";
    j["repository"]         = "https://github.com/example/plugin";
    j["tags"]               = json::array({"storage", "cloud"});
    j["category"]           = "storage";
    j["marketplace_id"]     = "550e8400-e29b-41d4-a716-446655440000";
    j["min_themis_version"] = "1.2.0";
    j["max_themis_version"] = "2.0.0";
    j["verified_publisher"] = true;

    auto m = ManifestSchemaValidator::parseMarketplaceManifest(j);
    ASSERT_TRUE(m.has_value());
    EXPECT_EQ(m->author, "Alice");
    EXPECT_EQ(m->license, "Apache-2.0");
    EXPECT_EQ(m->homepage, "https://example.com");
    EXPECT_EQ(m->repository, "https://github.com/example/plugin");
    EXPECT_EQ(m->category, "storage");
    EXPECT_EQ(m->marketplace_id, "550e8400-e29b-41d4-a716-446655440000");
    EXPECT_EQ(m->min_themis_version, "1.2.0");
    EXPECT_EQ(m->max_themis_version, "2.0.0");
    EXPECT_TRUE(m->verified_publisher);
    ASSERT_EQ(m->tags.size(), 2u);
    EXPECT_EQ(m->tags[0], "storage");
    EXPECT_EQ(m->tags[1], "cloud");
}

TEST(ParseMarketplaceManifest, PopulatesCapabilities) {
    auto j = minimalValidManifest();
    j["capabilities"] = json{
        {"streaming",     true},
        {"batching",      true},
        {"transactions",  false},
        {"thread_safe",   true},
        {"gpu_accelerated", false}
    };

    auto m = ManifestSchemaValidator::parseMarketplaceManifest(j);
    ASSERT_TRUE(m.has_value());
    EXPECT_TRUE(m->capabilities.supports_streaming);
    EXPECT_TRUE(m->capabilities.supports_batching);
    EXPECT_FALSE(m->capabilities.supports_transactions);
    EXPECT_TRUE(m->capabilities.thread_safe);
    EXPECT_FALSE(m->capabilities.gpu_accelerated);
}

TEST(ParseMarketplaceManifest, PopulatesSignature) {
    auto j = minimalValidManifest();
    j["signature"] = json{
        {"fingerprint", "abc1230000000000"},   // >= 16 chars
        {"algorithm",   "ed25519"},
        {"signed_at",   "2026-02-01T12:00:00Z"}
    };

    auto m = ManifestSchemaValidator::parseMarketplaceManifest(j);
    ASSERT_TRUE(m.has_value());
    EXPECT_EQ(m->signature.fingerprint, "abc1230000000000");
    EXPECT_EQ(m->signature.algorithm, "ed25519");
    EXPECT_EQ(m->signature.signed_at, "2026-02-01T12:00:00Z");
}

TEST(ParseMarketplaceManifest, PopulatesAllPluginTypes) {
    struct TypeMapping { const char* str; PluginType expected; };
    static const TypeMapping mappings[] = {
        {"compute_backend", PluginType::COMPUTE_BACKEND},
        {"blob_storage",    PluginType::BLOB_STORAGE},
        {"importer",        PluginType::IMPORTER},
        {"exporter",        PluginType::EXPORTER},
        {"hsm_provider",    PluginType::HSM_PROVIDER},
        {"embedding",       PluginType::EMBEDDING},
        {"llm_backend",     PluginType::LLM_BACKEND},
        {"custom",          PluginType::CUSTOM},
    };
    for (const auto& [str, expected] : mappings) {
        auto j = minimalValidManifest();
        j["type"] = str;
        auto m = ManifestSchemaValidator::parseMarketplaceManifest(j);
        ASSERT_TRUE(m.has_value()) << "type='" << str << "' failed to parse";
        EXPECT_EQ(m->type, expected) << "type='" << str << "'";
    }
}

TEST(ParseMarketplaceManifest, PopulatesArrayDependencies) {
    auto j = minimalValidManifest();
    j["dependencies"] = json::array({"nlohmann/json", "curl"});

    auto m = ManifestSchemaValidator::parseMarketplaceManifest(j);
    ASSERT_TRUE(m.has_value());
    ASSERT_EQ(m->dependencies.size(), 2u);
    EXPECT_EQ(m->dependencies[0], "nlohmann/json");
    EXPECT_EQ(m->dependencies[1], "curl");
}

TEST(ParseMarketplaceManifest, DefaultsForOptionalScalars) {
    auto m = ManifestSchemaValidator::parseMarketplaceManifest(minimalValidManifest());
    ASSERT_TRUE(m.has_value());
    EXPECT_FALSE(m->auto_load);
    EXPECT_EQ(m->load_priority, 100);
    EXPECT_TRUE(m->expected_hash.empty());
    EXPECT_FALSE(m->verified_publisher);
    EXPECT_TRUE(m->author.empty());
    EXPECT_TRUE(m->tags.empty());
    EXPECT_TRUE(m->signature.fingerprint.empty());
}

// ---------------------------------------------------------------------------
// PluginSignatureInfo – struct basics
// ---------------------------------------------------------------------------

TEST(PluginSignatureInfo, DefaultConstruction) {
    PluginSignatureInfo sig;
    EXPECT_TRUE(sig.fingerprint.empty());
    EXPECT_TRUE(sig.algorithm.empty());
    EXPECT_TRUE(sig.signed_at.empty());
}

// ---------------------------------------------------------------------------
// MarketplaceManifest – inherits PluginManifest
// ---------------------------------------------------------------------------

TEST(MarketplaceManifest, InheritsPluginManifestFields) {
    MarketplaceManifest m;
    m.name    = "my_plugin";
    m.version = "2.0.0";
    m.type    = PluginType::BLOB_STORAGE;

    // Can be referenced as PluginManifest
    const PluginManifest& base = m;
    EXPECT_EQ(base.name, "my_plugin");
    EXPECT_EQ(base.version, "2.0.0");
    EXPECT_EQ(base.type, PluginType::BLOB_STORAGE);
}

// ---------------------------------------------------------------------------
// ManifestSchemaValidator – expected_hash length validation
// ---------------------------------------------------------------------------

TEST(ManifestSchemaValidator, RejectsExpectedHashWrongLength) {
    auto j = minimalValidManifest();
    j["expected_hash"] = "abc123";  // too short (not 64 chars)
    EXPECT_FALSE(ManifestSchemaValidator::validate(j).valid);
}

TEST(ManifestSchemaValidator, RejectsExpectedHashTooLong) {
    auto j = minimalValidManifest();
    j["expected_hash"] = std::string(65, 'a');  // one char too many
    EXPECT_FALSE(ManifestSchemaValidator::validate(j).valid);
}

TEST(ManifestSchemaValidator, AcceptsExpectedHashExactly64Chars) {
    auto j = minimalValidManifest();
    j["expected_hash"] = std::string(64, 'a');  // exactly 64 hex chars
    EXPECT_TRUE(ManifestSchemaValidator::validate(j).valid);
}

TEST(ManifestSchemaValidator, AcceptsAbsentExpectedHash) {
    // expected_hash is optional – absence must not cause validation failure
    EXPECT_TRUE(ManifestSchemaValidator::validate(minimalValidManifest()).valid);
}

// ---------------------------------------------------------------------------
// Integration – existing in-tree plugin.json files must validate
// ---------------------------------------------------------------------------

static json loadJsonFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) {
        ADD_FAILURE() << "Cannot open plugin.json file: " << path;
        return json{};
    }
    json j;
    f >> j;
    return j;
}

static std::string workspacePluginPath(const std::string& rel) {
    for (fs::path cur = fs::current_path(); !cur.empty(); cur = cur.parent_path()) {
        const fs::path candidate = cur / rel;
        if (fs::exists(candidate)) {
            return candidate.string();
        }
        if (cur == cur.root_path()) {
            break;
        }
    }

    const fs::path project_root_candidate = fs::path(THEMIS_PROJECT_ROOT) / rel;
    if (fs::exists(project_root_candidate)) {
        return project_root_candidate.string();
    }

    return project_root_candidate.string();
}

TEST(ManifestSchemaValidator, ExistingPluginS3ValidatesOk) {
    auto j = loadJsonFile(workspacePluginPath("plugins/blob_storage/s3/plugin.json"));
    auto r = ManifestSchemaValidator::validate(j);
    EXPECT_TRUE(r.valid) << [&]() {
        std::string m; for (const auto& e : r.errors) m += e + "\n"; return m; }();
}

TEST(ManifestSchemaValidator, ExistingPluginAzureValidatesOk) {
    auto j = loadJsonFile(workspacePluginPath("plugins/blob_storage/azure/plugin.json"));
    auto r = ManifestSchemaValidator::validate(j);
    EXPECT_TRUE(r.valid) << [&]() {
        std::string m; for (const auto& e : r.errors) m += e + "\n"; return m; }();
}

TEST(ManifestSchemaValidator, ExistingPluginHuggingfaceValidatesOk) {
    auto j = loadJsonFile(workspacePluginPath("plugins/huggingface/plugin.json"));
    auto r = ManifestSchemaValidator::validate(j);
    EXPECT_TRUE(r.valid) << [&]() {
        std::string m; for (const auto& e : r.errors) m += e + "\n"; return m; }();
}

TEST(ManifestSchemaValidator, ExistingPluginPostgresValidatesOk) {
    auto j = loadJsonFile(workspacePluginPath("plugins/importers/postgres/plugin.json"));
    auto r = ManifestSchemaValidator::validate(j);
    EXPECT_TRUE(r.valid) << [&]() {
        std::string m; for (const auto& e : r.errors) m += e + "\n"; return m; }();
}

TEST(ManifestSchemaValidator, ExistingPluginJsonlLlmExporterValidatesOk) {
    auto j = loadJsonFile(workspacePluginPath("plugins/exporters/jsonl_llm/plugin.json"));
    auto r = ManifestSchemaValidator::validate(j);
    EXPECT_TRUE(r.valid) << [&]() {
        std::string m; for (const auto& e : r.errors) m += e + "\n"; return m; }();
}
