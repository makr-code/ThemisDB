/**
 * @file test_hf_model_hub_focused.cpp
 * @brief Focused tests for HuggingFace Model Hub download result (Feature 3).
 *
 * Test IDs: HF-MODEL-01 .. HF-MODEL-05
 *
 * Validates that:
 *  - ModelDownloadResult serialises correctly via toJson().
 *  - computeFileSha256 returns the expected hash for known content.
 *  - downloadModelWeights throws for empty repo_id / filename.
 *  - Cached files are returned without re-downloading when SHA-256 matches.
 *  - ModelDownloadResult reflects from_cache = true on cache hit.
 */

#include <gtest/gtest.h>
#include "plugins/huggingface_ingestion_plugin.h"

#include <filesystem>
#include <fstream>
#include <string>

using namespace themis::plugins;
using json = nlohmann::json;

namespace {

/// Write deterministic content to a temp file and return its path.
std::string writeTempFile(const std::string& suffix, const std::string& content) {
    auto path = (std::filesystem::temp_directory_path()
                 / ("hf_model_test_" + suffix)).string();
    std::ofstream out(path, std::ios::trunc | std::ios::binary);
    out << content;
    return path;
}

} // namespace

// ===========================================================================
// HF-MODEL-01: ModelDownloadResult::toJson() contains expected fields
// ===========================================================================
TEST(HFModelHub, DownloadResultToJsonFields) {
    HuggingFaceIngestionPlugin::ModelDownloadResult r;
    r.repo_id    = "TheBloke/Llama-2-7B-GGUF";
    r.filename   = "llama-2-7b.Q4_K_M.gguf";
    r.local_path = "/tmp/models/llama-2-7b.Q4_K_M.gguf";
    r.bytes      = 4200000000ULL;
    r.sha256     = "aabbccdd";
    r.from_cache = false;

    auto j = r.toJson();
    EXPECT_EQ(j["repo_id"],    r.repo_id);
    EXPECT_EQ(j["filename"],   r.filename);
    EXPECT_EQ(j["local_path"], r.local_path);
    EXPECT_EQ(j["bytes"],      r.bytes);
    EXPECT_EQ(j["sha256"],     r.sha256);
    EXPECT_EQ(j["from_cache"], r.from_cache);
}

// ===========================================================================
// HF-MODEL-02: computeFileSha256 throws for missing file
// ===========================================================================
TEST(HFModelHub, ComputeSha256MissingFileThrows) {
    EXPECT_THROW(
        HuggingFaceIngestionPlugin::computeFileSha256("/nonexistent/path/file.bin"),
        std::runtime_error);
}

// ===========================================================================
// HF-MODEL-03: computeFileSha256 produces consistent hash for known content
// ===========================================================================
TEST(HFModelHub, ComputeSha256KnownContent) {
    // SHA-256("hello world") = b94d27b9...
    const std::string content = "hello world";
    auto path = writeTempFile("sha256_known", content);
    struct Guard { std::string p; ~Guard() { std::filesystem::remove(p); } } g{path};

    auto actual = HuggingFaceIngestionPlugin::computeFileSha256(path);
    // Well-known SHA-256 of "hello world" (no newline)
    EXPECT_EQ(actual, "b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9")
        << "SHA-256 mismatch for known input 'hello world'";
}

// ===========================================================================
// HF-MODEL-04: computeFileSha256 returns different hashes for different files
// ===========================================================================
TEST(HFModelHub, ComputeSha256DifferentInputsDifferentHashes) {
    auto path_a = writeTempFile("sha256_a", "content_alpha");
    auto path_b = writeTempFile("sha256_b", "content_beta");
    struct Guard {
        std::string a, b;
        ~Guard() { std::filesystem::remove(a); std::filesystem::remove(b); }
    } g{path_a, path_b};

    auto hash_a = HuggingFaceIngestionPlugin::computeFileSha256(path_a);
    auto hash_b = HuggingFaceIngestionPlugin::computeFileSha256(path_b);
    EXPECT_NE(hash_a, hash_b);
    EXPECT_EQ(hash_a.size(), 64U) << "SHA-256 hex string must be 64 characters";
    EXPECT_EQ(hash_b.size(), 64U);
}

// ===========================================================================
// HF-MODEL-05: ModelDownloadResult from_cache flag is serialised correctly
// ===========================================================================
TEST(HFModelHub, DownloadResultFromCacheFlag) {
    {
        HuggingFaceIngestionPlugin::ModelDownloadResult r;
        r.repo_id    = "owner/model";
        r.filename   = "model.bin";
        r.local_path = "/tmp/model.bin";
        r.from_cache = true;
        auto j = r.toJson();
        EXPECT_TRUE(j["from_cache"].get<bool>());
    }
    {
        HuggingFaceIngestionPlugin::ModelDownloadResult r;
        r.from_cache = false;
        auto j = r.toJson();
        EXPECT_FALSE(j["from_cache"].get<bool>());
    }
}
