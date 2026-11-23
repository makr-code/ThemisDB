#include <gtest/gtest.h>

#include "storage/tensor_network_storage_engine.h"
#include "tensor/dream_candidate_store.h"

#include <stdexcept>
#include <vector>

using themis::tensor::DreamCandidateProvenance;
using themis::tensor::DreamCandidateStore;

namespace {

DreamCandidateProvenance validProvenance() {
    return {true, true, "dream-run-17"};
}

std::string storageKey(const std::string& candidate_id) {
    constexpr char kHexDigits[] = "0123456789abcdef";
    std::string key = "__dream_candidate__:v1:";
    for (const auto ch : candidate_id) {
        const auto byte = static_cast<unsigned char>(ch);
        key.push_back(kHexDigits[byte >> 4]);
        key.push_back(kHexDigits[byte & 0x0f]);
    }
    return key;
}

std::vector<uint8_t> asBytes(const std::string& value) {
    std::vector<uint8_t> bytes;
    bytes.reserve(value.size());
    for (const auto ch : value) {
        bytes.push_back(static_cast<uint8_t>(static_cast<unsigned char>(ch)));
    }
    return bytes;
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

TEST(DreamCandidateStore, PersistsProvenanceAcrossStoreInstancesInIsolatedNamespace) {
    auto backend = std::make_shared<themis::storage::InMemoryTensorBackend>();
    ASSERT_TRUE(backend->put("__production_truth__:untouched", {1, 2, 3}));

    {
        DreamCandidateStore writer(backend);
        ASSERT_TRUE(writer.store("candidate/persisted", {{"value", 42}}, validProvenance()));
    }

    DreamCandidateStore reopened(backend);
    const auto loaded = reopened.get("candidate/persisted");
    ASSERT_TRUE(loaded.has_value());
    EXPECT_TRUE(loaded->provenance.synthetic);
    EXPECT_TRUE(loaded->provenance.dream_mode);
    EXPECT_EQ(loaded->provenance.run_id, "dream-run-17");
    EXPECT_TRUE(DreamCandidateStore::verifyArtifact(*loaded));

    const auto keys = backend->listKeys("__dream_candidate__:");
    ASSERT_EQ(keys.size(), 1U);
    EXPECT_EQ(keys.front(), storageKey("candidate/persisted"));
    EXPECT_EQ(backend->get("__production_truth__:untouched"),
              std::optional<std::vector<uint8_t>>(std::vector<uint8_t>{1, 2, 3}));
}

TEST(DreamCandidateStore, RejectsCorruptedPersistedEnvelopeOnRead) {
    auto backend = std::make_shared<themis::storage::InMemoryTensorBackend>();
    DreamCandidateStore writer(backend);
    ASSERT_TRUE(writer.store("candidate-corrupt", {{"value", 1}}, validProvenance()));
    const auto exported = writer.exportArtifact("candidate-corrupt");
    ASSERT_TRUE(exported.has_value());

    auto corrupted = nlohmann::json::parse(*exported);
    corrupted["payload"]["value"] = 2;
    ASSERT_TRUE(backend->put(storageKey("candidate-corrupt"), asBytes(corrupted.dump())));

    DreamCandidateStore reopened(backend);
    EXPECT_FALSE(reopened.get("candidate-corrupt").has_value());
    EXPECT_FALSE(reopened.exportArtifact("candidate-corrupt").has_value());
}

TEST(DreamCandidateStore, RejectsNullInjectedBackend) {
    EXPECT_THROW(DreamCandidateStore(nullptr), std::invalid_argument);
}
