#include "../test_data_generator.h"
#include "../test_fixture.h"

#include <algorithm>
#include <nlohmann/json.hpp>

namespace themis { namespace test { 

namespace {

struct IngestionOutcome {
    bool ok{false};
    std::string error;
};

class IngestionPipeline {
public:
    IngestionPipeline(std::shared_ptr<InMemoryPipelineStorage> storage,
                      std::shared_ptr<MockPipelineIndex> index,
                      std::shared_ptr<PipelineAuditLog> audit,
                      size_t rate_limit_per_batch)
        : storage_(std::move(storage)), index_(std::move(index)), audit_(std::move(audit)), rate_limit_per_batch_(rate_limit_per_batch) {}

    [[nodiscard]] IngestionOutcome IngestDocument(const nlohmann::json& doc, bool content_error = false) {
        const auto id = doc.value("id", "");
        if (id.empty()) {
            return {false, "missing_id"};
        }

        if (content_error) {
            audit_->Record({"content", "parse_error", id});
            return {false, "content_parse_error"};
        }

        if (!doc.contains("title")) {
            audit_->Record({"metadata", "schema_error", id});
            return {false, "schema_validation_error"};
        }

        storage_->Write(id, doc.dump());
        index_->IndexDocument(id, doc.value("terms", std::vector<std::string>{id}));

        cdc_events_.push_back("cdc:" + id);
        audit_->Record({"cdc", "emit", id});
        return {true, ""};
    }

    [[nodiscard]] size_t BatchIngest(const std::vector<nlohmann::json>& docs, size_t start_offset = 0) {
        const size_t end = std::min(docs.size(), start_offset + rate_limit_per_batch_);
        for (size_t i = start_offset; i < end; ++i) {
            const auto outcome = IngestDocument(docs[i]);
            if (!outcome.ok) {
                break;
            }
            checkpoint_ = i + 1;
        }
        return checkpoint_;
    }

    [[nodiscard]] size_t ResumeFromCheckpoint(const std::vector<nlohmann::json>& docs) {
        while (checkpoint_ < docs.size()) {
            const auto next = BatchIngest(docs, checkpoint_);
            if (next == checkpoint_) {
                break;
            }
        }
        return checkpoint_;
    }

    [[nodiscard]] const std::vector<std::string>& CdcEvents() const {
        return cdc_events_;
    }

private:
    std::shared_ptr<InMemoryPipelineStorage> storage_;
    std::shared_ptr<MockPipelineIndex> index_;
    std::shared_ptr<PipelineAuditLog> audit_;
    size_t rate_limit_per_batch_{1};
    size_t checkpoint_{0};
    std::vector<std::string> cdc_events_;
};

} // namespace

class IngestionPipelineTest : public IntegrationTestFixture {
protected:
    void SetUp() override {
        IntegrationTestFixture::SetUp();
        data_gen_ = std::make_unique<TestDataGenerator>();
        storage_ = CreateInMemoryStorage();
        index_ = CreateMockIndex();
        audit_ = CreateAuditLog();
    }

    std::unique_ptr<TestDataGenerator> data_gen_;
    std::shared_ptr<InMemoryPipelineStorage> storage_;
    std::shared_ptr<MockPipelineIndex> index_;
    std::shared_ptr<PipelineAuditLog> audit_;
};

TEST_F(IngestionPipelineTest, IP01_DocumentIngestWritesStorageIndexesAndEmitsCdc) {
    IngestionPipeline pipeline(storage_, index_, audit_, 10);

    auto doc = data_gen_->GenerateTestDocument("ingest");
    doc["terms"] = nlohmann::json::array({"pipeline", "doc"});

    const auto outcome = pipeline.IngestDocument(doc);

    ASSERT_TRUE(outcome.ok);
    EXPECT_TRUE(storage_->Contains(doc["id"].get<std::string>()));
    EXPECT_FALSE(index_->Search("pipeline").empty());
    ASSERT_EQ(pipeline.CdcEvents().size(), 1U);
    EXPECT_TRUE(audit_->Contains("cdc", "emit"));
}

TEST_F(IngestionPipelineTest, IP02_BatchRateLimitCheckpointAndResumeWork) {
    IngestionPipeline pipeline(storage_, index_, audit_, 3);

    auto docs = data_gen_->GenerateTestDocuments(7, "batch");
    for (auto& doc : docs) {
        doc["terms"] = nlohmann::json::array({"batch"});
    }

    const auto first_checkpoint = pipeline.BatchIngest(docs);
    EXPECT_EQ(first_checkpoint, 3U);

    const auto final_checkpoint = pipeline.ResumeFromCheckpoint(docs);
    EXPECT_EQ(final_checkpoint, docs.size());
    EXPECT_EQ(pipeline.CdcEvents().size(), docs.size());
}

TEST_F(IngestionPipelineTest, IP03_ContentErrorDoesNotCreatePartialArtifacts) {
    IngestionPipeline pipeline(storage_, index_, audit_, 10);

    auto doc = data_gen_->GenerateTestDocument("broken");
    const auto id = doc["id"].get<std::string>();

    const auto outcome = pipeline.IngestDocument(doc, true);

    EXPECT_FALSE(outcome.ok);
    EXPECT_FALSE(storage_->Contains(id));
    EXPECT_TRUE(index_->Search(id).empty());
    EXPECT_TRUE(pipeline.CdcEvents().empty());
}

TEST_F(IngestionPipelineTest, IP04_ContinuousIngestionFeedsSubscribersWithChanges) {
    IngestionPipeline pipeline(storage_, index_, audit_, 2);

    auto doc1 = data_gen_->GenerateTestDocument("stream");
    auto doc2 = data_gen_->GenerateTestDocument("stream");
    pipeline.IngestDocument(doc1);
    pipeline.IngestDocument(doc2);

    const auto& events = pipeline.CdcEvents();
    ASSERT_EQ(events.size(), 2U);
    EXPECT_NE(events[0].find("cdc:"), std::string::npos);
    EXPECT_NE(events[1].find("cdc:"), std::string::npos);
}

TEST_F(IngestionPipelineTest, IP05_SchemaValidationErrorIsAuditedAndStorageRemainsConsistent) {
    IngestionPipeline pipeline(storage_, index_, audit_, 10);

    auto doc = data_gen_->GenerateTestDocument("schema");
    const auto id = doc["id"].get<std::string>();
    doc.erase("title");

    const auto outcome = pipeline.IngestDocument(doc);

    EXPECT_FALSE(outcome.ok);
    EXPECT_EQ(outcome.error, "schema_validation_error");
    EXPECT_FALSE(storage_->Contains(id));
    EXPECT_TRUE(index_->Search(id).empty());
    EXPECT_TRUE(audit_->Contains("metadata", "schema_error"));
}

TEST_F(IngestionPipelineTest, IP06_ResumeStopsOnBrokenDocumentAndRetainsCheckpointForRecovery) {
    IngestionPipeline pipeline(storage_, index_, audit_, 5);

    auto docs = data_gen_->GenerateTestDocuments(3, "resume");
    for (auto& doc : docs) {
        doc["terms"] = nlohmann::json::array({"resume"});
    }
    docs[1].erase("title");

    const auto checkpoint_after_failure = pipeline.ResumeFromCheckpoint(docs);
    EXPECT_EQ(checkpoint_after_failure, 1U);
    EXPECT_EQ(pipeline.CdcEvents().size(), 1U);
    EXPECT_TRUE(audit_->Contains("metadata", "schema_error"));

    docs[1]["title"] = "repaired";
    const auto final_checkpoint = pipeline.ResumeFromCheckpoint(docs);
    EXPECT_EQ(final_checkpoint, docs.size());
    EXPECT_EQ(pipeline.CdcEvents().size(), docs.size());
}
} } // namespace themis::test
