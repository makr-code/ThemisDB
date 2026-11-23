#include <gtest/gtest.h>

#include "tensor/dream_candidate_store.h"

using themis::tensor::DreamCandidateProvenance;
using themis::tensor::DreamCandidateStore;

namespace {

DreamCandidateProvenance validProvenance() {
    return {true, true, "dream-run-17"};
}

} // namespace

TEST(DreamCandidateStore, StoresReadsAndRoundTripsProvenance) {
    DreamCandidateStore store;
    const nlohmann::json payload = {{"hypothesis", "candidate"}, {"score", 0.75}};

    ASSERT_TRUE(store.store("candidate-1", payload, validProvenance()));
    const auto stored = store.get("candidate-1");
    ASSERT_TRUE(stored.has_value());
    EXPECT_EQ(stored->payload, payload);
    EXPECT_TRUE(stored->provenance.synthetic);
    EXPECT_TRUE(stored->provenance.dream_mode);
    EXPECT_EQ(stored->provenance.run_id, "dream-run-17");
    EXPECT_TRUE(DreamCandidateStore::verifyArtifact(*stored));

    const auto exported = store.exportArtifact("candidate-1");
    ASSERT_TRUE(exported.has_value());
    DreamCandidateStore imported;
    ASSERT_TRUE(imported.importArtifact(*exported));
    const auto round_tripped = imported.get("candidate-1");
    ASSERT_TRUE(round_tripped.has_value());
    EXPECT_EQ(round_tripped->toJson(), stored->toJson());
}

TEST(DreamCandidateStore, RejectsMissingOrMalformedProvenance) {
    DreamCandidateStore store;
    std::string error;

    auto provenance = validProvenance();
    provenance.synthetic = false;
    EXPECT_FALSE(store.store("missing-synthetic", {{"value", 1}}, provenance, &error));
    EXPECT_FALSE(error.empty());

    provenance = validProvenance();
    provenance.dream_mode = false;
    EXPECT_FALSE(store.store("missing-dream-mode", {{"value", 1}}, provenance, &error));

    provenance = validProvenance();
    provenance.run_id = "  \t";
    EXPECT_FALSE(store.store("missing-run-id", {{"value", 1}}, provenance, &error));
    EXPECT_FALSE(store.store("null-payload", nullptr, validProvenance(), &error));
    EXPECT_FALSE(store.get("missing-synthetic").has_value());
}

TEST(DreamCandidateStore, ImportRejectsMissingTagsMalformedEnvelopeAndChecksumMismatch) {
    DreamCandidateStore source;
    ASSERT_TRUE(source.store("candidate-2", {{"value", 2}}, validProvenance()));
    const auto exported = source.exportArtifact("candidate-2");
    ASSERT_TRUE(exported.has_value());

    auto envelope = nlohmann::json::parse(*exported);
    envelope["provenance"].erase("synthetic");
    DreamCandidateStore missing_tag;
    EXPECT_FALSE(missing_tag.importArtifact(envelope.dump()));

    envelope = nlohmann::json::parse(*exported);
    envelope["provenance"]["dream_mode"] = false;
    DreamCandidateStore malformed_tag;
    EXPECT_FALSE(malformed_tag.importArtifact(envelope.dump()));

    envelope = nlohmann::json::parse(*exported);
    envelope["payload"]["value"] = 99;
    DreamCandidateStore corrupted;
    EXPECT_FALSE(corrupted.importArtifact(envelope.dump()));
}

TEST(DreamCandidateStore, InstancesRemainIsolatedAndDuplicateIdsAreRejected) {
    DreamCandidateStore first;
    DreamCandidateStore second;
    ASSERT_TRUE(first.store("candidate-3", {{"value", 3}}, validProvenance()));

    EXPECT_FALSE(second.get("candidate-3").has_value());
    std::string error;
    EXPECT_FALSE(first.store("candidate-3", {{"value", 4}}, validProvenance(), &error));
    EXPECT_NE(error.find("already exists"), std::string::npos);
}
