#include <gtest/gtest.h>
#include "content/content_manager.h"
#include "rag/rag_provider.h"
#include "storage/base_entity.h"
#include "storage/key_schema.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include "ingestion/ingestion_sinks.h"

#include <filesystem>
#include <limits>
#include <nlohmann/json.hpp>
#include <thread>

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
        db_ = std::make_shared<RocksDBWrapper>(config);
        ASSERT_TRUE(db_->open());
        secondary_ = std::make_shared<SecondaryIndexManager>(*db_);
        graph_ = std::make_shared<GraphIndexManager>(*db_);
        vector_ = std::make_shared<VectorIndexManager>(*db_);
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
    std::shared_ptr<RocksDBWrapper> db_;
    std::shared_ptr<SecondaryIndexManager> secondary_;
    std::shared_ptr<GraphIndexManager> graph_;
    std::shared_ptr<VectorIndexManager> vector_;
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

TEST_F(RAGProviderTest, VectorIngestionSinkMaterializesPersistedSourceText) {
    ingestion::VectorIndexSinkAdapter sink(
        vector_,
        "docs", 2);
    ingestion::VectorRecord record;
    record.chunk_id = "chunk:source-1:0";
    record.source_file_id = "source-1";
    record.text_snippet = "authoritative vector source text";
    record.embedding = {1.0f, 0.0f};
    ASSERT_TRUE(sink.writeVectors({record}));

    query::VectorGeoQuery query;
    query.table = "docs";
    query.query_vector = {1.0f, 0.0f};
    rag::RetrievalRequest request;
    request.query = query;
    const auto result = provider_->retrieve(request);

    ASSERT_TRUE(result) << result.error().context();
    ASSERT_EQ(result->candidates.size(), 1);
    EXPECT_EQ(result->candidates.front().content, "authoritative vector source text");
    ASSERT_TRUE(result->candidates.front().source_reference.has_value());
    EXPECT_EQ(result->candidates.front().source_reference->type, "ingestion_vector");
    EXPECT_EQ(result->candidates.front().source_reference->collection, "docs");
    EXPECT_EQ(result->candidates.front().source_reference->key, record.chunk_id);
    EXPECT_EQ(result->candidates.front().source_reference->provenance, record.source_file_id);
}

TEST_F(RAGProviderTest, ContentManagerChunkVectorPreservesTextAndSourceIdentity) {
    auto content_vector = std::make_shared<VectorIndexManager>(*db_);
    ASSERT_TRUE(content_vector->init("chunks", 2, VectorIndexManager::Metric::COSINE).ok);
    content::ContentManager content_manager(db_, content_vector, graph_, secondary_);
    const nlohmann::json spec = {
        {"content", {
            {"id", "content-1"},
            {"mime_type", "text/plain"},
            {"category", "TEXT"},
            {"original_filename", "source.txt"},
            {"size_bytes", 28},
            {"created_at", 42},
            {"text_extracted", true},
            {"chunked", true},
            {"indexed", false},
            {"chunk_count", 1}
        }},
        {"chunks", {{
            {"id", "content-1-chunk-0"},
            {"content_id", "content-1"},
            {"seq_num", 0},
            {"chunk_type", "text"},
            {"text", "content-manager evidence"},
            {"created_at", 123},
            {"embedding", {1.0f, 0.0f}}
        }}}
    };
    ASSERT_TRUE(content_manager.importContent(spec).ok);

    query::QueryEngine content_engine(*db_, *secondary_, *graph_, content_vector.get(), nullptr);
    rag::RAGProvider content_provider(content_engine);
    query::VectorGeoQuery query;
    query.table = "chunks";
    query.query_vector = {1.0f, 0.0f};
    rag::RetrievalRequest request;
    request.query = query;
    const auto result = content_provider.retrieve(request);

    ASSERT_TRUE(result) << result.error().context();
    ASSERT_EQ(result->candidates.size(), 1);
    const auto& candidate = result->candidates.front();
    EXPECT_EQ(candidate.content, "content-manager evidence");
    ASSERT_TRUE(candidate.source_reference.has_value());
    EXPECT_EQ(candidate.source_reference->type, "content_chunk");
    EXPECT_EQ(candidate.source_reference->collection, "content-1");
    EXPECT_EQ(candidate.source_reference->key, "chunk:content-1-chunk-0");
    EXPECT_EQ(candidate.source_reference->provenance, "content_manager");
    ASSERT_TRUE(candidate.source_reference->version.has_value());
    EXPECT_EQ(*candidate.source_reference->version, "123");
}

TEST_F(RAGProviderTest, VectorOnlyMetadataIsNotMaterializedAsEvidence) {
    const auto vector_entity = BaseEntity::fromFields("metadata-only", {
        {"embedding", std::vector<float>{1.0f, 0.0f}},
        {"source_file_id", std::string{"source-2"}}
    });
    ASSERT_TRUE(vector_->addEntity(vector_entity).ok);

    query::VectorGeoQuery query;
    query.table = "docs";
    query.query_vector = {1.0f, 0.0f};
    rag::RetrievalRequest request;
    request.query = query;
    const auto result = provider_->retrieve(request);

    ASSERT_TRUE(result) << result.error().context();
    EXPECT_TRUE(result->candidates.empty());
    EXPECT_EQ(result->rejected_documents, 1);
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

TEST_F(RAGProviderTest, RequestScopedCollectionCheckerAllowsOnlyAuthorizedCollection) {
    storeDocument("d1", "authorized evidence");
    engine_->setCollectionAccessChecker(
        [](const std::string& collection, const std::string&) {
            return collection == "docs";
        });

    query::ConjunctiveQuery authorized_query;
    authorized_query.table = "docs";
    authorized_query.pk_eq = "d1";
    rag::RetrievalRequest authorized_request;
    authorized_request.query = authorized_query;
    const auto authorized_result = provider_->retrieve(authorized_request);
    ASSERT_TRUE(authorized_result) << authorized_result.error().context();
    ASSERT_EQ(authorized_result->candidates.size(), 1);
    EXPECT_EQ(authorized_result->candidates.front().content, "authorized evidence");

    query::ConjunctiveQuery other_collection_query;
    other_collection_query.table = "other";
    other_collection_query.pk_eq = "d1";
    rag::RetrievalRequest denied_request;
    denied_request.query = other_collection_query;
    const auto denied_result = provider_->retrieve(denied_request);
    ASSERT_FALSE(denied_result);
    EXPECT_EQ(denied_result.error().code(), errors::ErrorCode::ERR_QUERY_ACCESS_DENIED);
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

TEST_F(RAGProviderTest, StopsAfterBlockingQueryEngineCallExceedsDeadline) {
    storeDocument("d1", "deadline-bound evidence");
    bool access_check_called = false;
    engine_->setCollectionAccessChecker(
        [&access_check_called](const std::string&, const std::string&) {
            access_check_called = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            return true;
        });

    query::ConjunctiveQuery query;
    query.table = "docs";
    query.pk_eq = "d1";
    rag::RetrievalRequest request;
    request.query = query;

    rag::RetrievalExecutionContext context;
    context.deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(5);
    const auto result = provider_->retrieve(request, context);

    ASSERT_FALSE(result);
    EXPECT_EQ(result.error().code(), errors::ErrorCode::ERR_QUERY_TIMEOUT);
    EXPECT_TRUE(access_check_called);
}
