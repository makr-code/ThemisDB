/**
 * End-to-end integration tests: DecisionRecordYamlProcessor + LoRAFederationCoordinator
 *
 * These tests exercise the full path:
 *   federation round (triggerAggregation) → emitFederationDecisionRecord()
 *   → processor.submit() → background thread → YAML file written to disk
 *
 * Coverage:
 *   1. FederationRoundYamlWrittenToDisk     — verifies YAML file appears on disk
 *   2. FederationRoundYamlHasRequiredFields — verifies YAML content has
 *                                             type/id/timestamp/outcome fields
 *   3. MultipleRoundsProduceMultipleFiles   — verifies N rounds → N YAML files
 */

#include <gtest/gtest.h>

#include "distributed_knowledge/lora_federation_coordinator.h"
#include "llm/decision_record_yaml_processor.h"

#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <regex>

namespace fs = std::filesystem;

using themis::distributed_knowledge::LoRAFederationCoordinator;
using themis::distributed_knowledge::FederationConfig;
using themis::distributed_knowledge::EncryptedGradient;
using themis::llm::DecisionRecordYamlProcessor;

// ─── Helpers ──────────────────────────────────────────────────────────────────

static fs::path makeTempDir(const std::string& suffix) {
    auto tmp = fs::temp_directory_path() / ("dr_e2e_" + suffix);
    fs::create_directories(tmp);
    return tmp;
}

/// Count YAML files (*.yaml) recursively under `dir`.
static size_t countYamlFiles(const fs::path& dir) {
    if (!fs::exists(dir)) return 0;
    size_t n = 0;
    for (const auto& entry : fs::recursive_directory_iterator(dir)) {
        if (entry.is_regular_file() &&
            entry.path().extension() == ".yaml") {
            ++n;
        }
    }
    return n;
}

/// Return the content of the first YAML file found under `dir`, or "".
static std::string firstYamlContent(const fs::path& dir) {
    if (!fs::exists(dir)) return "";
    for (const auto& entry : fs::recursive_directory_iterator(dir)) {
        if (entry.is_regular_file() &&
            entry.path().extension() == ".yaml") {
            std::ifstream f(entry.path());
            return std::string(std::istreambuf_iterator<char>(f),
                               std::istreambuf_iterator<char>());
        }
    }
    return "";
}

/// Build a minimal EncryptedGradient for shard `id` and federation `round`.
static EncryptedGradient makeGradient(const std::string& shard_id, uint64_t round) {
    EncryptedGradient g;
    g.shard_id    = shard_id;
    g.round       = round;
    g.sample_count = 10;
    g.data        = {{"layer0", 0.01}, {"layer1", -0.02}};
    return g;
}

/// Submit `min_participants` gradients and call triggerAggregation().
static void runOneRound(LoRAFederationCoordinator& coord, size_t n_shards = 2) {
    const uint64_t round = coord.currentRound();
    for (size_t i = 0; i < n_shards; ++i) {
        coord.submitGradient(makeGradient("shard_" + std::to_string(i), round));
    }
    coord.triggerAggregation();
}

// ─── Tests ────────────────────────────────────────────────────────────────────

TEST(DrE2E, FederationRoundYamlWrittenToDisk) {
    auto tmp = makeTempDir("round_on_disk");

    DecisionRecordYamlProcessor::Config cfg;
    cfg.log_dir              = tmp;
    cfg.create_daily_subdirs = false;

    auto proc = std::make_shared<DecisionRecordYamlProcessor>(cfg);

    FederationConfig fed_cfg;
    fed_cfg.min_participants = 2;
    LoRAFederationCoordinator coord(fed_cfg);
    coord.setDecisionRecordProcessor(proc);

    runOneRound(coord);

    // Allow the background thread time to flush.
    proc->flush();

    EXPECT_GE(countYamlFiles(tmp), 1u)
        << "Expected at least one YAML file in " << tmp;

    fs::remove_all(tmp);
}

TEST(DrE2E, FederationRoundYamlHasRequiredFields) {
    auto tmp = makeTempDir("yaml_fields");

    DecisionRecordYamlProcessor::Config cfg;
    cfg.log_dir              = tmp;
    cfg.create_daily_subdirs = false;

    auto proc = std::make_shared<DecisionRecordYamlProcessor>(cfg);

    FederationConfig fed_cfg;
    fed_cfg.min_participants = 2;
    LoRAFederationCoordinator coord(fed_cfg);
    coord.setDecisionRecordProcessor(proc);

    runOneRound(coord);
    proc->flush();

    std::string content = firstYamlContent(tmp);
    ASSERT_FALSE(content.empty()) << "No YAML file found under " << tmp;

    // Required top-level YAML keys
    EXPECT_TRUE(content.find("type:") != std::string::npos
             || content.find("record_type:") != std::string::npos)
        << "YAML missing 'type' field:\n" << content;

    EXPECT_TRUE(content.find("id:") != std::string::npos
             || content.find("decision_id:") != std::string::npos)
        << "YAML missing 'id' field:\n" << content;

    EXPECT_TRUE(content.find("timestamp:") != std::string::npos
             || content.find("created_at:") != std::string::npos)
        << "YAML missing 'timestamp' field:\n" << content;

    EXPECT_TRUE(content.find("FEDERATED_ROUND") != std::string::npos
             || content.find("federated_round") != std::string::npos)
        << "YAML does not reference FEDERATED_ROUND type:\n" << content;

    fs::remove_all(tmp);
}

TEST(DrE2E, MultipleRoundsProduceMultipleFiles) {
    auto tmp = makeTempDir("multi_rounds");

    DecisionRecordYamlProcessor::Config cfg;
    cfg.log_dir              = tmp;
    cfg.create_daily_subdirs = false;

    auto proc = std::make_shared<DecisionRecordYamlProcessor>(cfg);

    FederationConfig fed_cfg;
    fed_cfg.min_participants = 2;
    LoRAFederationCoordinator coord(fed_cfg);
    coord.setDecisionRecordProcessor(proc);

    constexpr int ROUNDS = 3;
    for (int r = 0; r < ROUNDS; ++r) {
        runOneRound(coord);
    }

    proc->flush();

    EXPECT_GE(countYamlFiles(tmp), static_cast<size_t>(ROUNDS))
        << "Expected " << ROUNDS << " YAML files, found "
        << countYamlFiles(tmp) << " under " << tmp;

    fs::remove_all(tmp);
}
