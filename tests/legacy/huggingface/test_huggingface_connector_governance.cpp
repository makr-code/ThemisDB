/**
 * @file test_huggingface_connector_governance.cpp
 * @brief Unit tests for the data classification gate in HuggingFaceConnector (Gap 8).
 *
 * Tests
 * -----
 * HFC_GOV_01  No policy set → initialize() succeeds (degraded/backward-compat mode)
 * HFC_GOV_02  Policy permits → initialize() succeeds (gate PERMIT)
 * HFC_GOV_03  Policy denies (restricted collection) → initialize() returns false
 * HFC_GOV_04  Policy denies ("geheim" classification) → initialize() returns false
 * HFC_GOV_05  Policy denies only the first call; second call with different
 *             source_id is independent (policy decisions are per-request)
 *
 * Source: AI_ML_IMPACT_ASSESSMENT.md §7, Gap 8 (Severity: Medium/S1)
 * Tracked: src/ingestion/FUTURE_ENHANCEMENTS.md §"Data Classification Gate"
 *
 * Note: These tests use the mock HTTP GET hook so no live network is needed.
 */

#include <gtest/gtest.h>
#include "ingestion/huggingface_connector.h"
#include "governance/model_governance.h"
#include <memory>
#include <string>

using namespace themis::ingestion;
using namespace themis::governance;

namespace {

// Build a SourceConfig for a given dataset location.
SourceConfig makeConfig(const std::string& location = "owner/dataset",
                        const std::string& classification = "")
{
    SourceConfig cfg;
    cfg.source_id = "test-job-" + location;
    cfg.type      = SourceType::HUGGINGFACE;
    cfg.location  = location;
    cfg.options["split"] = "train";
    if (!classification.empty()) {
        cfg.options["classification"] = classification;
    }
    return cfg;
}

// Mock HTTP GET that simulates a successful metadata response so that
// Impl::initialize() does not fail on the network call.
ApiHttpGetFn okHttpGet() {
    return [](const std::string& /*url*/, const std::string& /*token*/)
               -> std::pair<int, std::string> {
        return {200, R"({"id":"owner/dataset","private":false,"downloads":100})"};
    };
}

} // namespace

// ---------------------------------------------------------------------------
// HFC_GOV_01 — no policy → backward-compatible success (WARN path)
// ---------------------------------------------------------------------------
TEST(HFC_GOV, HFC_GOV_01_NoPolicySucceeds) {
    HuggingFaceConnector connector;
    connector.setHttpGetForTesting(okHttpGet());
    // No setIngestionPolicy() call → degraded mode.
    EXPECT_TRUE(connector.initialize(makeConfig()))
        << "Without a policy, initialize() must succeed (backward-compat)";
}

// ---------------------------------------------------------------------------
// HFC_GOV_02 — policy permits → initialize() succeeds
// ---------------------------------------------------------------------------
TEST(HFC_GOV, HFC_GOV_02_PolicyPermitSucceeds) {
    HuggingFaceConnector connector;
    connector.setHttpGetForTesting(okHttpGet());

    auto policy = std::make_shared<ModelGovernancePolicy>();
    // No restricted collections, "offen" classification → always permitted.
    connector.setIngestionPolicy(policy);

    EXPECT_TRUE(connector.initialize(makeConfig("owner/public-dataset", "offen")))
        << "Policy with no restrictions must permit 'offen' datasets";
}

// ---------------------------------------------------------------------------
// HFC_GOV_03 — restricted collection → initialize() returns false
// ---------------------------------------------------------------------------
TEST(HFC_GOV, HFC_GOV_03_RestrictedCollectionDenies) {
    HuggingFaceConnector connector;
    connector.setHttpGetForTesting(okHttpGet());

    auto policy = std::make_shared<ModelGovernancePolicy>();
    policy->addRestrictedCollection("owner/restricted-data");
    connector.setIngestionPolicy(policy);

    EXPECT_FALSE(connector.initialize(makeConfig("owner/restricted-data")))
        << "Restricted collection must cause initialize() to return false";
}

// ---------------------------------------------------------------------------
// HFC_GOV_04 — "geheim" classification → initialize() returns false
// ---------------------------------------------------------------------------
TEST(HFC_GOV, HFC_GOV_04_GeheimClassificationDenies) {
    HuggingFaceConnector connector;
    connector.setHttpGetForTesting(okHttpGet());

    auto policy = std::make_shared<ModelGovernancePolicy>();
    connector.setIngestionPolicy(policy);

    EXPECT_FALSE(connector.initialize(makeConfig("owner/any-dataset", "geheim")))
        << "Classification 'geheim' must always cause initialize() to return false";
}

// ---------------------------------------------------------------------------
// HFC_GOV_05 — decisions are per-request (independent calls)
// ---------------------------------------------------------------------------
TEST(HFC_GOV, HFC_GOV_05_DecisionsArePerRequest) {
    HuggingFaceConnector connector;
    connector.setHttpGetForTesting(okHttpGet());

    auto policy = std::make_shared<ModelGovernancePolicy>();
    policy->addRestrictedCollection("owner/restricted");
    connector.setIngestionPolicy(policy);

    // First call: restricted → denied
    EXPECT_FALSE(connector.initialize(makeConfig("owner/restricted")))
        << "Restricted dataset must be denied";

    // Second call: different (open) dataset → permitted
    EXPECT_TRUE(connector.initialize(makeConfig("owner/open-data", "offen")))
        << "Open dataset must be permitted independently of previous denied call";
}
