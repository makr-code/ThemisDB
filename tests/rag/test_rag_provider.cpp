#include <gtest/gtest.h>
#include "rag/rag_provider.h"
#include "storage/base_entity.h"
#include "storage/key_schema.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"

#include <filesystem>
#include <limits>

using namespace themis;

class RAGProviderTest : public ::testing::Test {
protected:
    void SetUp() override {
        path_ = std::filesystem::temp_directory_path() /
            ("themis_rag_provider_" + std::to_string(
                std::chrono::steady_clock::now().time_since_epoch().count()));
        RocksDBWrapper::Config config;
        config.db_path = path_.string();
        config.enable_blobdb = false;
        db_ = std::make_unique<RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());
        secondary_ = std::make_unique<SecondaryIndexManager>(*db_);
        graph_ = std::make_unique<GraphIndexManager>(*db_);
        vector_ = std::make_unique<VectorIndexManager>(*db_);
        ASSERT_TRUE(vector_->init("docs", 2, VectorIndexManager::Metric::COSINE).ok);
        engine_ = std::make_unique<query::QueryEngine>(
            *db_, *secondary_, *graph_, vector_.get(), nullptr);
        provider_ = std::make_unique<rag::RAGProvider>(*engine_);
    }

    void TearDown() override {
        provider_.reset();
        engine_.reset();
        vector_.reset();
        graph_.reset();
        secondary_.reset();
        db_.reset();
        std::filesystem::remove_all(path_);
    }

    void storeDocument(const std::string& id, const std::string& text) {
        const auto entity = BaseEntity::fromFields(id, {
            {"content", text}, {"category", std::string{"public"}},
            {"embedding", std::vector<float>{1.0f, 0.0f}}
        });
        ASSERT_TRUE(secondary_->put("docs", entity).ok);
        ASSERT_TRUE(vector_->addEntity(entity).ok);
    }

    std::filesystem::path path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::unique_ptr<SecondaryIndexManager> secondary_;
    std::unique_ptr<GraphIndexManager> graph_;
    std::unique_ptr<VectorIndexManager> vector_;
    std::unique_ptr<query::QueryEngine> engine_;
    std::unique_ptr<rag::RAGProvider> provider_;
};

TEST_F(RAGProviderTest, RetrievesBinaryEntitiesThroughTheRelationalContract) {
    storeDocument("d1", "stored evidence");
    query::ConjunctiveQuery query;
    query.table = "docs";
    query.pk_eq = "d1";
    rag::RetrievalRequest request;
    request.query = query;
    const auto result = provider_->retrieve(request);
    ASSERT_TRUE(result) << result.error().context();
    ASSERT_EQ(result->candidates.size(), 1);
    EXPECT_EQ(result->candidates.front().content, "stored evidence");
    EXPECT_EQ(result->candidates.front().source_namespace, "docs");
}

TEST_F(RAGProviderTest, VectorIndexMaterializesCanonicalBinaryStorage) {
    storeDocument("d1", "indexed evidence");
    query::VectorGeoQuery query;
    query.table = "docs";
    query.query_vector = {1.0f, 0.0f};
    rag::RetrievalRequest request;
    request.query = query;
    const auto result = provider_->retrieve(request);
    ASSERT_TRUE(result) << result.error().context();
    ASSERT_EQ(result->candidates.size(), 1);
    EXPECT_EQ(result->candidates.front().doc_id, "d1");
    EXPECT_EQ(result->candidates.front().content, "indexed evidence");
}

TEST_F(RAGProviderTest, GraphMaterializesCanonicalNodeStorage) {
    const auto entity = BaseEntity::fromFields(
        "docs/g1", {{"content", std::string{"graph evidence"}}});
    ASSERT_TRUE(db_->put(KeySchema::makeGraphNodeKey("docs/g1"), entity.serialize()));
    rag::GraphRetrievalQuery query;
    query.start_vertex = "docs/g1";
    query.max_depth = 0;
    rag::RetrievalRequest request;
    request.query = query;
    const auto result = provider_->retrieve(request);
    ASSERT_TRUE(result) << result.error().context();
    ASSERT_EQ(result->candidates.size(), 1);
    EXPECT_EQ(result->candidates.front().content, "graph evidence");
    EXPECT_EQ(result->candidates.front().source_namespace, "docs");
}

TEST_F(RAGProviderTest, PropagatesCollectionDenialForRelationalVectorAndGraph) {
    engine_->setCollectionAccessChecker(
        [](const std::string&, const std::string&) { return false; });
    query::ConjunctiveQuery relational;
    relational.table = "private";
    query::VectorGeoQuery vector;
    vector.table = "private";
    vector.query_vector = {1.0f, 0.0f};
    rag::GraphRetrievalQuery graph;
    graph.start_vertex = "private/v1";
    for (const rag::RetrievalQuery& query :
         {rag::RetrievalQuery{relational}, rag::RetrievalQuery{vector}, rag::RetrievalQuery{graph}}) {
        rag::RetrievalRequest request;
        request.query = query;
        const auto result = provider_->retrieve(request);
        ASSERT_FALSE(result);
        EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_QUERY_ACCESS_DENIED);
    }
}

TEST_F(RAGProviderTest, GraphReferencesCanMaterializeCanonicalRelationalDocuments) {
    storeDocument("g2", "relational graph evidence");
    rag::GraphRetrievalQuery query;
    query.start_vertex = "docs/g2";
    query.max_depth = 0;
    rag::RetrievalRequest request;
    request.query = query;
    const auto result = provider_->retrieve(request);
    ASSERT_TRUE(result) << result.error().context();
    ASSERT_EQ(result->candidates.size(), 1);
    EXPECT_EQ(result->candidates.front().content, "relational graph evidence");
}

TEST_F(RAGProviderTest, DoesNotTurnNoMatchesIntoFabricatedEvidence) {
    query::ConjunctiveQuery query;
    query.table = "docs";
    query.pk_eq = "absent";
    rag::RetrievalRequest request;
    request.query = query;
    const auto result = provider_->retrieve(request);
    ASSERT_TRUE(result);
    EXPECT_TRUE(result->candidates.empty());
}

TEST_F(RAGProviderTest, RejectsZeroAndNonFiniteEmbeddings) {
    for (const auto& embedding : {std::vector<float>{0, 0},
                                 std::vector<float>{1, std::numeric_limits<float>::infinity()}}) {
        query::VectorGeoQuery query;
        query.table = "docs";
        query.query_vector = embedding;
        rag::RetrievalRequest request;
        request.query = query;
        const auto result = provider_->retrieve(request);
        ASSERT_FALSE(result);
        EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_QUERY_INVALID_INPUT);
    }
}

TEST_F(RAGProviderTest, EnforcesContentBudgetWithoutPartialSuccess) {
    storeDocument("d1", "larger than the requested budget");
    query::ConjunctiveQuery query;
    query.table = "docs";
    query.pk_eq = "d1";
    rag::RetrievalRequest request;
    request.query = query;
    request.max_content_bytes = 4;
    const auto result = provider_->retrieve(request);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_QUERY_RESOURCE_EXHAUSTED);
}

TEST_F(RAGProviderTest, RejectsPrimaryKeyQueriesThatWouldIgnoreHardPredicates) {
    storeDocument("d1", "protected evidence");
    query::ConjunctiveQuery query;
    query.table = "docs";
    query.pk_eq = "d1";
    query.predicates.push_back({"category", "denied"});
    rag::RetrievalRequest request;
    request.query = query;
    const auto result = provider_->retrieve(request);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_QUERY_INVALID_INPUT);
}

TEST_F(RAGProviderTest, HonorsCancellationAndExpiredDeadlines) {
    rag::RetrievalExecutionContext context;
    context.cancelled = std::make_shared<std::atomic<bool>>(true);
    auto result = provider_->retrieve({}, context);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_QUERY_CANCELLED);
    context.cancelled.reset();
    context.deadline = std::chrono::steady_clock::now() - std::chrono::seconds(1);
    result = provider_->retrieve({}, context);
    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_QUERY_TIMEOUT);
}
