#include <gtest/gtest.h>
#include "api/themisdb_grpc_service.h"
#include "api/themisdb_grpc_service_factory.h"
#include "index/graph_index.h"
#include "index/secondary_index.h"
#include "index/vector_index.h"
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"
#include "transaction/transaction_manager.h"
#include "themis/base/interfaces/query_interface.h"
#include "themis/base/interfaces/index_interface.h"
#include <filesystem>

using namespace themis::api;

namespace {
constexpr bool kHasThemisdbGrpcProtoHeader =
#if __has_include("themisdb.grpc.pb.h")
    true;
#else
    false;
#endif
} // namespace

#if __has_include("themisdb.grpc.pb.h")
#include <grpcpp/grpcpp.h>
#include "themisdb.grpc.pb.h"
#endif

// ============================================================================
// Minimal mock implementations for injection testing
// (used without proto stubs; test only the C++ wiring layer)
// ============================================================================

namespace {

// A minimal IQueryEngine that returns a configurable JSON string.
class StubQueryEngine : public themis::IQueryEngine {
public:
    std::string response_json = R"([{"_key":"doc1"},{"_key":"doc2"}])";
    std::string last_query;

    themis::Result<std::string> execute(const std::string& query) override {
        last_query = query;
        return response_json;
    }
    themis::Result<void> validate(const std::string& /*query*/) const override {
        return {};
    }
    themis::Result<std::unique_ptr<themis::IExpressionEvaluator>>
    createExpressionEvaluator() const override {
        return themis::Err<std::unique_ptr<themis::IExpressionEvaluator>>(
            themis::errors::ErrorCode::ERR_UNKNOWN, "not implemented in stub");
    }
    themis::Result<std::string> explainQuery(const std::string& /*query*/) const override {
        return std::string("stub-plan");
    }
};

// A minimal IVectorIndex that echoes back key names.
class StubVectorIndex : public themis::IVectorIndex {
public:
    bool insert(std::string_view /*pk*/, const std::vector<float>& /*vec*/) override {
        return true;
    }
    bool remove(std::string_view /*pk*/) override { return true; }

    std::vector<themis::VectorSearchResult> search(
        const std::vector<float>& /*qv*/, uint32_t k,
        const themis::IExpressionEvaluator* /*filter*/ = nullptr) const override {
        std::vector<themis::VectorSearchResult> hits;
        for (uint32_t i = 0; i < k && i < 3; ++i) {
            hits.emplace_back("key_" + std::to_string(i), static_cast<float>(i) * 0.1f);
        }
        return hits;
    }
    std::vector<themis::VectorSearchResult> rangeSearch(
        const std::vector<float>& /*qv*/, float /*max_dist*/,
        const themis::IExpressionEvaluator* /*filter*/ = nullptr) const override {
        return {};
    }
    std::string getName() const override { return "stub"; }
    uint32_t getDimension() const override { return 4; }
    std::string getStatistics() const override { return "{}"; }
};

} // namespace

// ============================================================================
// ThemisDBGrpcService – construction and service() accessor tests
// ============================================================================

TEST(ThemisDBGrpcServiceTest, ConstructWithNullComponents) {
    // Both components are optional (may be null when stubs are absent).
    ASSERT_NO_THROW({
        ThemisDBGrpcService svc(nullptr, nullptr);
    });
}

TEST(ThemisDBGrpcServiceTest, ServiceReturnsNullOrValidPointer) {
    ThemisDBGrpcService svc(nullptr, nullptr);

    // When the generated proto headers are present service() returns a non-null
    // grpc::Service*; when they are absent it returns nullptr.  Either value is
    // acceptable – the caller must check before registering.
    void* ptr = svc.service();
    (void)ptr;  // accepted: nullptr or a valid pointer both compile cleanly
    SUCCEED();
}

TEST(ThemisDBGrpcServiceTest, ServiceCallIsIdempotent) {
    ThemisDBGrpcService svc(nullptr, nullptr);

    void* first  = svc.service();
    void* second = svc.service();
    EXPECT_EQ(first, second);
}

// ============================================================================
// Extended constructor – wired components
// ============================================================================

TEST(ThemisDBGrpcServiceTest, ConstructWithWiredComponents) {
    auto engine = std::make_shared<StubQueryEngine>();
    auto index  = std::make_shared<StubVectorIndex>();

    ASSERT_NO_THROW({
        ThemisDBGrpcService svc(nullptr, nullptr, engine, index);
        (void)svc.service();
    });
}

TEST(ThemisDBGrpcServiceTest, ServicePointerStableAfterWiring) {
    auto engine = std::make_shared<StubQueryEngine>();
    ThemisDBGrpcService svc(nullptr, nullptr, engine, nullptr);
    void* p1 = svc.service();
    void* p2 = svc.service();
    EXPECT_EQ(p1, p2);
}

// ============================================================================
// ThemisDBGrpcServiceFactory – fluent builder tests
// ============================================================================

TEST(ThemisDBGrpcServiceFactoryTest, BuildWithNoComponents) {
    // Factory with no components should build cleanly and produce a valid svc.
    auto svc = ThemisDBGrpcServiceFactory{}.build();
    ASSERT_NE(svc, nullptr);
    // service() returns nullptr without stubs; either value is acceptable.
    (void)svc->service();
    SUCCEED();
}

TEST(ThemisDBGrpcServiceFactoryTest, BuildWithQueryEngine) {
    auto engine = std::make_shared<StubQueryEngine>();
    auto svc = ThemisDBGrpcServiceFactory{}
                   .withQueryEngine(engine)
                   .build();
    ASSERT_NE(svc, nullptr);
    SUCCEED();
}

TEST(ThemisDBGrpcServiceFactoryTest, BuildWithVectorIndex) {
    auto index = std::make_shared<StubVectorIndex>();
    auto svc = ThemisDBGrpcServiceFactory{}
                   .withVectorIndex(index)
                   .build();
    ASSERT_NE(svc, nullptr);
    SUCCEED();
}

TEST(ThemisDBGrpcServiceFactoryTest, BuildFullyWired) {
    auto engine = std::make_shared<StubQueryEngine>();
    auto index  = std::make_shared<StubVectorIndex>();
    auto svc = ThemisDBGrpcServiceFactory{}
                   .withQueryEngine(engine)
                   .withVectorIndex(index)
                   .build();
    ASSERT_NE(svc, nullptr);
    // service() is non-null when proto stubs are compiled in.
    (void)svc->service();
    SUCCEED();
}

TEST(ThemisDBGrpcServiceFactoryTest, FactoryIsReusable) {
    // A factory instance can build multiple independent service objects.
    auto engine = std::make_shared<StubQueryEngine>();
    ThemisDBGrpcServiceFactory factory;
    factory.withQueryEngine(engine);

    auto svc1 = factory.build();
    auto svc2 = factory.build();
    ASSERT_NE(svc1, nullptr);
    ASSERT_NE(svc2, nullptr);
    // They must be distinct objects.
    EXPECT_NE(svc1.get(), svc2.get());
}

// ============================================================================
// Bug-fix regression tests (audit findings 2026-04-07)
// ============================================================================

// ── Fix 1: AQL identifier validation ────────────────────────────────────────
#include "api/aql_utils.h"

TEST(AqlUtilsTest, ValidIdentifiers) {
    EXPECT_TRUE(themis::api::isValidAqlIdentifier("documents"));
    EXPECT_TRUE(themis::api::isValidAqlIdentifier("_system"));
    EXPECT_TRUE(themis::api::isValidAqlIdentifier("MyCollection123"));
    EXPECT_TRUE(themis::api::isValidAqlIdentifier("a"));
    EXPECT_TRUE(themis::api::isValidAqlIdentifier("_"));
}

TEST(AqlUtilsTest, InvalidIdentifiers) {
    EXPECT_FALSE(themis::api::isValidAqlIdentifier(""));
    EXPECT_FALSE(themis::api::isValidAqlIdentifier("123abc"));       // starts with digit
    EXPECT_FALSE(themis::api::isValidAqlIdentifier("col RETURN 1")); // space + injection
    EXPECT_FALSE(themis::api::isValidAqlIdentifier("col/sub"));      // slash
    EXPECT_FALSE(themis::api::isValidAqlIdentifier("col; DROP"));    // semicolon
    EXPECT_FALSE(themis::api::isValidAqlIdentifier("col\nFILTER"));  // newline
    EXPECT_FALSE(themis::api::isValidAqlIdentifier("col`OTHER"));    // backtick
}

TEST(AqlUtilsTest, EscapeLiteralHandlesSpecialChars) {
    EXPECT_EQ(themis::api::aqlEscapeLiteral("hello"),         "hello");
    EXPECT_EQ(themis::api::aqlEscapeLiteral("it's"),          "it\\'s");
    EXPECT_EQ(themis::api::aqlEscapeLiteral("path\\to\\end"), "path\\\\to\\\\end");
    EXPECT_EQ(themis::api::aqlEscapeLiteral("a'b\\c"),        "a\\'b\\\\c");
    EXPECT_EQ(themis::api::aqlEscapeLiteral(""),              "");
}

TEST(AqlUtilsTest, EscapeLiteralDoesNotEscapeIdentifierChars) {
    // Normal alphanumeric query strings must pass through unchanged.
    const std::string q = "Bundesministerium fuer Wirtschaft";
    EXPECT_EQ(themis::api::aqlEscapeLiteral(q), q);
}

TEST(ThemisDBGrpcServiceBuildTest, ProtoHeaderVisibility) {
    EXPECT_TRUE(kHasThemisdbGrpcProtoHeader)
        << "Expected themisdb.grpc.pb.h to be visible in this test target include path";
}

#if __has_include("themisdb.grpc.pb.h")

namespace {

class GrpcTransactionPathTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "./data/test_themisdb_grpc_service_rpc";
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }

        themis::RocksDBWrapper::Config cfg;
        cfg.db_path = test_db_path_;
        db_ = std::make_shared<themis::RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());

        sec_idx_ = std::make_unique<themis::SecondaryIndexManager>(*db_);
        graph_idx_ = std::make_unique<themis::GraphIndexManager>(*db_);
        vec_idx_ = std::make_unique<themis::VectorIndexManager>(*db_);

        txn_mgr_ = std::make_shared<themis::TransactionManager>(
            *db_, *sec_idx_, *graph_idx_, *vec_idx_);

        service_ = std::make_unique<themis::api::ThemisDBGrpcService>(db_, txn_mgr_);
        rpc_ = static_cast<themis::api::ThemisDBService::Service*>(service_->service());
        ASSERT_NE(rpc_, nullptr);
    }

    void TearDown() override {
        service_.reset();
        txn_mgr_.reset();
        vec_idx_.reset();
        graph_idx_.reset();
        sec_idx_.reset();
        if (db_) {
            db_->close();
            db_.reset();
        }
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
    }

    std::string test_db_path_;
    std::shared_ptr<themis::RocksDBWrapper> db_;
    std::unique_ptr<themis::SecondaryIndexManager> sec_idx_;
    std::unique_ptr<themis::GraphIndexManager> graph_idx_;
    std::unique_ptr<themis::VectorIndexManager> vec_idx_;
    std::shared_ptr<themis::TransactionManager> txn_mgr_;
    std::unique_ptr<themis::api::ThemisDBGrpcService> service_;
    themis::api::ThemisDBService::Service* rpc_{nullptr};
};

TEST_F(GrpcTransactionPathTest, CreateDocumentTransactionDefersWriteUntilCommit) {
    const auto tx_id = txn_mgr_->beginTransaction();

    themis::api::CreateDocumentRequest req;
    req.set_transaction_id(std::to_string(tx_id));
    auto* doc = req.mutable_document();
    doc->set_collection("users");
    doc->set_key("alice");
    const std::string payload = R"({"name":"Alice"})";
    doc->set_body(payload.data(), payload.size());

    themis::api::CreateDocumentResponse resp;
    grpc::ServerContext ctx;
    const auto status = rpc_->CreateDocument(&ctx, &req, &resp);
    ASSERT_TRUE(status.ok());
    ASSERT_TRUE(resp.success());

    std::string body;
    EXPECT_FALSE(db_->get("entity:users:alice", body));

    const auto commit_status = txn_mgr_->commitTransaction(tx_id);
    ASSERT_TRUE(commit_status.ok) << commit_status.message;

    ASSERT_TRUE(db_->get("entity:users:alice", body));
    EXPECT_FALSE(body.empty());
}

TEST_F(GrpcTransactionPathTest, DeleteDocumentTransactionDefersDeleteUntilCommit) {
    const auto tx_create = txn_mgr_->beginTransaction();
    themis::api::CreateDocumentRequest create_req;
    create_req.set_transaction_id(std::to_string(tx_create));
    auto* cdoc = create_req.mutable_document();
    cdoc->set_collection("users");
    cdoc->set_key("bob");
    const std::string payload = R"({"name":"Bob"})";
    cdoc->set_body(payload.data(), payload.size());

    themis::api::CreateDocumentResponse create_resp;
    grpc::ServerContext create_ctx;
    ASSERT_TRUE(rpc_->CreateDocument(&create_ctx, &create_req, &create_resp).ok());
    ASSERT_TRUE(create_resp.success());
    ASSERT_TRUE(txn_mgr_->commitTransaction(tx_create).ok);

    std::string body;
    ASSERT_TRUE(db_->get("entity:users:bob", body));

    const auto tx_delete = txn_mgr_->beginTransaction();
    themis::api::DeleteDocumentRequest delete_req;
    delete_req.set_collection("users");
    delete_req.set_key("bob");
    delete_req.set_transaction_id(std::to_string(tx_delete));

    themis::api::DeleteDocumentResponse delete_resp;
    grpc::ServerContext delete_ctx;
    const auto delete_status = rpc_->DeleteDocument(&delete_ctx, &delete_req, &delete_resp);
    ASSERT_TRUE(delete_status.ok());
    ASSERT_TRUE(delete_resp.success());

    EXPECT_TRUE(db_->get("entity:users:bob", body));

    const auto commit_status = txn_mgr_->commitTransaction(tx_delete);
    ASSERT_TRUE(commit_status.ok) << commit_status.message;
    EXPECT_FALSE(db_->get("entity:users:bob", body));
}

TEST_F(GrpcTransactionPathTest, GetDocumentTransactionReadsUncommittedWrite) {
    const auto tx_id = txn_mgr_->beginTransaction();

    themis::api::CreateDocumentRequest create_req;
    create_req.set_transaction_id(std::to_string(tx_id));
    auto* doc = create_req.mutable_document();
    doc->set_collection("users");
    doc->set_key("carol");
    const std::string payload = R"({"name":"Carol","role":"admin"})";
    doc->set_body(payload.data(), payload.size());

    themis::api::CreateDocumentResponse create_resp;
    grpc::ServerContext create_ctx;
    ASSERT_TRUE(rpc_->CreateDocument(&create_ctx, &create_req, &create_resp).ok());
    ASSERT_TRUE(create_resp.success());

    themis::api::GetDocumentRequest get_req;
    get_req.set_collection("users");
    get_req.set_key("carol");
    get_req.set_transaction_id(std::to_string(tx_id));

    themis::api::GetDocumentResponse get_resp;
    grpc::ServerContext get_ctx;
    const auto get_status = rpc_->GetDocument(&get_ctx, &get_req, &get_resp);
    ASSERT_TRUE(get_status.ok());
    ASSERT_TRUE(get_resp.success());
    ASSERT_TRUE(get_resp.has_document());

    const std::string body(get_resp.document().body().begin(), get_resp.document().body().end());
    EXPECT_NE(body.find("Carol"), std::string::npos);
    EXPECT_NE(body.find("admin"), std::string::npos);
}

TEST_F(GrpcTransactionPathTest, BatchReadTransactionSeesOwnWrites) {
    const auto tx_id = txn_mgr_->beginTransaction();

    themis::api::CreateDocumentRequest create_req;
    create_req.set_transaction_id(std::to_string(tx_id));
    auto* doc = create_req.mutable_document();
    doc->set_collection("users");
    doc->set_key("dave");
    const std::string payload = R"({"name":"Dave"})";
    doc->set_body(payload.data(), payload.size());

    themis::api::CreateDocumentResponse create_resp;
    grpc::ServerContext create_ctx;
    ASSERT_TRUE(rpc_->CreateDocument(&create_ctx, &create_req, &create_resp).ok());
    ASSERT_TRUE(create_resp.success());

    themis::api::BatchReadRequest batch_req;
    batch_req.set_collection("users");
    batch_req.set_transaction_id(std::to_string(tx_id));
    batch_req.add_keys("dave");
    batch_req.add_keys("missing");

    themis::api::BatchReadResponse batch_resp;
    grpc::ServerContext batch_ctx;
    const auto batch_status = rpc_->BatchRead(&batch_ctx, &batch_req, &batch_resp);
    ASSERT_TRUE(batch_status.ok());
    ASSERT_TRUE(batch_resp.success());
    ASSERT_EQ(batch_resp.documents_size(), 1);
    EXPECT_EQ(batch_resp.documents(0).key(), "dave");
}

TEST_F(GrpcTransactionPathTest, GetDocumentWithoutTransactionUsesEntityFallback) {
    const auto tx_id = txn_mgr_->beginTransaction();

    themis::api::CreateDocumentRequest create_req;
    create_req.set_transaction_id(std::to_string(tx_id));
    auto* doc = create_req.mutable_document();
    doc->set_collection("users");
    doc->set_key("erin");
    const std::string payload = R"({"name":"Erin"})";
    doc->set_body(payload.data(), payload.size());

    themis::api::CreateDocumentResponse create_resp;
    grpc::ServerContext create_ctx;
    ASSERT_TRUE(rpc_->CreateDocument(&create_ctx, &create_req, &create_resp).ok());
    ASSERT_TRUE(create_resp.success());
    ASSERT_TRUE(txn_mgr_->commitTransaction(tx_id).ok);

    themis::api::GetDocumentRequest get_req;
    get_req.set_collection("users");
    get_req.set_key("erin");

    themis::api::GetDocumentResponse get_resp;
    grpc::ServerContext get_ctx;
    const auto get_status = rpc_->GetDocument(&get_ctx, &get_req, &get_resp);
    ASSERT_TRUE(get_status.ok());
    ASSERT_TRUE(get_resp.success());
    ASSERT_TRUE(get_resp.has_document());

    const std::string body(get_resp.document().body().begin(), get_resp.document().body().end());
    EXPECT_NE(body.find("Erin"), std::string::npos);
}

class GrpcVectorFetchDocsFallbackTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path_ = "./data/test_themisdb_grpc_service_vector_fallback";
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }

        themis::RocksDBWrapper::Config cfg;
        cfg.db_path = test_db_path_;
        db_ = std::make_shared<themis::RocksDBWrapper>(cfg);
        ASSERT_TRUE(db_->open());

        sec_idx_ = std::make_unique<themis::SecondaryIndexManager>(*db_);
        graph_idx_ = std::make_unique<themis::GraphIndexManager>(*db_);
        vec_idx_ = std::make_unique<themis::VectorIndexManager>(*db_);

        txn_mgr_ = std::make_shared<themis::TransactionManager>(
            *db_, *sec_idx_, *graph_idx_, *vec_idx_);

        vector_stub_ = std::make_shared<StubVectorIndex>();
        service_ = std::make_unique<themis::api::ThemisDBGrpcService>(
            db_, txn_mgr_, nullptr, vector_stub_);
        rpc_ = static_cast<themis::api::ThemisDBService::Service*>(service_->service());
        ASSERT_NE(rpc_, nullptr);
    }

    void seedUserEntity(const std::string& key,
                        const std::string& name,
                        const std::string& tier = "",
                        int64_t age = -1,
                        bool has_score = false,
                        double score = 0.0) {
        const auto tx_id = txn_mgr_->beginTransaction();
        auto tx = txn_mgr_->getTransaction(tx_id);
        ASSERT_NE(tx, nullptr);

        themis::BaseEntity entity(key);
        entity.setField("name", name);
        if (!tier.empty()) {
            entity.setField("tier", tier);
        }
        if (age >= 0) {
            entity.setField("age", age);
        }
        if (has_score) {
            entity.setField("score", score);
        }
        ASSERT_TRUE(tx->putEntity("users", entity).ok);
        ASSERT_TRUE(txn_mgr_->commitTransaction(tx_id).ok);
    }

    void TearDown() override {
        service_.reset();
        vector_stub_.reset();
        txn_mgr_.reset();
        vec_idx_.reset();
        graph_idx_.reset();
        sec_idx_.reset();
        if (db_) {
            db_->close();
            db_.reset();
        }
        if (std::filesystem::exists(test_db_path_)) {
            std::filesystem::remove_all(test_db_path_);
        }
    }

    std::string test_db_path_;
    std::shared_ptr<themis::RocksDBWrapper> db_;
    std::unique_ptr<themis::SecondaryIndexManager> sec_idx_;
    std::unique_ptr<themis::GraphIndexManager> graph_idx_;
    std::unique_ptr<themis::VectorIndexManager> vec_idx_;
    std::shared_ptr<themis::TransactionManager> txn_mgr_;
    std::shared_ptr<StubVectorIndex> vector_stub_;
    std::unique_ptr<themis::api::ThemisDBGrpcService> service_;
    themis::api::ThemisDBService::Service* rpc_{nullptr};
};

TEST_F(GrpcVectorFetchDocsFallbackTest, VectorSearchFetchDocsUsesEntityKeyFallback) {
    const auto tx_id = txn_mgr_->beginTransaction();
    auto tx = txn_mgr_->getTransaction(tx_id);
    ASSERT_NE(tx, nullptr);

    themis::BaseEntity entity("key_0");
    entity.setField("name", std::string("VectorKey0"));
    ASSERT_TRUE(tx->putEntity("users", entity).ok);
    ASSERT_TRUE(txn_mgr_->commitTransaction(tx_id).ok);

    themis::api::VectorSearchRequest req;
    req.set_collection("users");
    req.set_k(1);
    req.set_fetch_docs(true);
    req.mutable_query_vector()->add_values(0.12f);
    req.mutable_query_vector()->add_values(0.34f);

    themis::api::VectorSearchResponse resp;
    grpc::ServerContext ctx;
    const auto status = rpc_->VectorSearch(&ctx, &req, &resp);
    ASSERT_TRUE(status.ok());
    ASSERT_TRUE(resp.success());
    ASSERT_EQ(resp.hits_size(), 1);

    const auto& hit = resp.hits(0);
    ASSERT_EQ(hit.key(), "key_0");
    const std::string body(hit.document().begin(), hit.document().end());
    EXPECT_NE(body.find("VectorKey0"), std::string::npos);
}

TEST_F(GrpcVectorFetchDocsFallbackTest, FilteredVectorSearchFetchDocsUsesEntityKeyFallback) {
    const auto tx_id = txn_mgr_->beginTransaction();
    auto tx = txn_mgr_->getTransaction(tx_id);
    ASSERT_NE(tx, nullptr);

    themis::BaseEntity entity("key_0");
    entity.setField("name", std::string("FilteredVectorKey0"));
    ASSERT_TRUE(tx->putEntity("users", entity).ok);
    ASSERT_TRUE(txn_mgr_->commitTransaction(tx_id).ok);

    themis::api::FilteredVectorSearchRequest req;
    req.set_collection("users");
    req.set_k(1);
    req.set_fetch_docs(true);
    req.mutable_query_vector()->add_values(0.99f);

    themis::api::VectorSearchResponse resp;
    grpc::ServerContext ctx;
    const auto status = rpc_->FilteredVectorSearch(&ctx, &req, &resp);
    ASSERT_TRUE(status.ok());
    ASSERT_TRUE(resp.success());
    ASSERT_EQ(resp.hits_size(), 1);

    const auto& hit = resp.hits(0);
    ASSERT_EQ(hit.key(), "key_0");
    const std::string body(hit.document().begin(), hit.document().end());
    EXPECT_NE(body.find("FilteredVectorKey0"), std::string::npos);
}

TEST_F(GrpcVectorFetchDocsFallbackTest, FilteredVectorSearchAppliesKeyEqFilter) {
    themis::api::FilteredVectorSearchRequest req;
    req.set_collection("users");
    req.set_k(3);
    req.mutable_query_vector()->add_values(0.1f);
    auto* f = req.add_filters();
    f->set_field("_key");
    f->set_operator_("eq");
    f->set_value("key_1");

    themis::api::VectorSearchResponse resp;
    grpc::ServerContext ctx;
    const auto status = rpc_->FilteredVectorSearch(&ctx, &req, &resp);
    ASSERT_TRUE(status.ok());
    ASSERT_TRUE(resp.success());
    ASSERT_EQ(resp.hits_size(), 1);
    EXPECT_EQ(resp.hits(0).key(), "key_1");
}

TEST_F(GrpcVectorFetchDocsFallbackTest, FilteredVectorSearchAppliesKeyInFilter) {
    themis::api::FilteredVectorSearchRequest req;
    req.set_collection("users");
    req.set_k(3);
    req.mutable_query_vector()->add_values(0.2f);
    auto* f = req.add_filters();
    f->set_field("_key");
    f->set_operator_("in");
    f->set_value(R"(["key_0","key_2"])");

    themis::api::VectorSearchResponse resp;
    grpc::ServerContext ctx;
    const auto status = rpc_->FilteredVectorSearch(&ctx, &req, &resp);
    ASSERT_TRUE(status.ok());
    ASSERT_TRUE(resp.success());
    ASSERT_EQ(resp.hits_size(), 2);
    EXPECT_EQ(resp.hits(0).key(), "key_0");
    EXPECT_EQ(resp.hits(1).key(), "key_2");
}

TEST_F(GrpcVectorFetchDocsFallbackTest, FilteredVectorSearchAppliesIdEqFilter) {
    themis::api::FilteredVectorSearchRequest req;
    req.set_collection("users");
    req.set_k(3);
    req.mutable_query_vector()->add_values(0.3f);
    auto* f = req.add_filters();
    f->set_field("_id");
    f->set_operator_("eq");
    f->set_value("users/key_2");

    themis::api::VectorSearchResponse resp;
    grpc::ServerContext ctx;
    const auto status = rpc_->FilteredVectorSearch(&ctx, &req, &resp);
    ASSERT_TRUE(status.ok());
    ASSERT_TRUE(resp.success());
    ASSERT_EQ(resp.hits_size(), 1);
    EXPECT_EQ(resp.hits(0).key(), "key_2");
}

TEST_F(GrpcVectorFetchDocsFallbackTest, FilteredVectorSearchAppliesKeyNeFilter) {
    themis::api::FilteredVectorSearchRequest req;
    req.set_collection("users");
    req.set_k(3);
    req.mutable_query_vector()->add_values(0.4f);
    auto* f = req.add_filters();
    f->set_field("_key");
    f->set_operator_("ne");
    f->set_value("key_1");

    themis::api::VectorSearchResponse resp;
    grpc::ServerContext ctx;
    const auto status = rpc_->FilteredVectorSearch(&ctx, &req, &resp);
    ASSERT_TRUE(status.ok());
    ASSERT_TRUE(resp.success());
    ASSERT_EQ(resp.hits_size(), 2);
    EXPECT_EQ(resp.hits(0).key(), "key_0");
    EXPECT_EQ(resp.hits(1).key(), "key_2");
}

TEST_F(GrpcVectorFetchDocsFallbackTest, FilteredVectorSearchAppliesTopLevelAttributeEqFilter) {
    seedUserEntity("key_0", "Alice", "gold");
    seedUserEntity("key_1", "Bob", "silver");
    seedUserEntity("key_2", "Bob", "silver");

    themis::api::FilteredVectorSearchRequest req;
    req.set_collection("users");
    req.set_k(3);
    req.mutable_query_vector()->add_values(0.5f);
    auto* f = req.add_filters();
    f->set_field("name");
    f->set_operator_("eq");
    f->set_value("Bob");

    themis::api::VectorSearchResponse resp;
    grpc::ServerContext ctx;
    const auto status = rpc_->FilteredVectorSearch(&ctx, &req, &resp);
    ASSERT_TRUE(status.ok());
    ASSERT_TRUE(resp.success());
    ASSERT_EQ(resp.hits_size(), 2);
    EXPECT_EQ(resp.hits(0).key(), "key_1");
    EXPECT_EQ(resp.hits(1).key(), "key_2");
}

TEST_F(GrpcVectorFetchDocsFallbackTest, FilteredVectorSearchAppliesTopLevelAttributeNeFilter) {
    seedUserEntity("key_0", "Alice", "gold");
    seedUserEntity("key_1", "Bob", "silver");
    seedUserEntity("key_2", "Charlie", "silver");

    themis::api::FilteredVectorSearchRequest req;
    req.set_collection("users");
    req.set_k(3);
    req.mutable_query_vector()->add_values(0.6f);
    auto* f = req.add_filters();
    f->set_field("tier");
    f->set_operator_("ne");
    f->set_value("gold");

    themis::api::VectorSearchResponse resp;
    grpc::ServerContext ctx;
    const auto status = rpc_->FilteredVectorSearch(&ctx, &req, &resp);
    ASSERT_TRUE(status.ok());
    ASSERT_TRUE(resp.success());
    ASSERT_EQ(resp.hits_size(), 2);
    EXPECT_EQ(resp.hits(0).key(), "key_1");
    EXPECT_EQ(resp.hits(1).key(), "key_2");
}

TEST_F(GrpcVectorFetchDocsFallbackTest, FilteredVectorSearchAppliesTopLevelNumericGtFilter) {
    seedUserEntity("key_0", "Alice", "gold", 21);
    seedUserEntity("key_1", "Bob", "silver", 35);
    seedUserEntity("key_2", "Charlie", "silver", 44);

    themis::api::FilteredVectorSearchRequest req;
    req.set_collection("users");
    req.set_k(3);
    req.mutable_query_vector()->add_values(0.7f);
    auto* f = req.add_filters();
    f->set_field("age");
    f->set_operator_("gt");
    f->set_value("30");

    themis::api::VectorSearchResponse resp;
    grpc::ServerContext ctx;
    const auto status = rpc_->FilteredVectorSearch(&ctx, &req, &resp);
    ASSERT_TRUE(status.ok());
    ASSERT_TRUE(resp.success());
    ASSERT_EQ(resp.hits_size(), 2);
    EXPECT_EQ(resp.hits(0).key(), "key_1");
    EXPECT_EQ(resp.hits(1).key(), "key_2");
}

TEST_F(GrpcVectorFetchDocsFallbackTest, FilteredVectorSearchAppliesTopLevelNumericLteFilter) {
    seedUserEntity("key_0", "Alice", "gold", 21);
    seedUserEntity("key_1", "Bob", "silver", 35);
    seedUserEntity("key_2", "Charlie", "silver", 44);

    themis::api::FilteredVectorSearchRequest req;
    req.set_collection("users");
    req.set_k(3);
    req.mutable_query_vector()->add_values(0.8f);
    auto* f = req.add_filters();
    f->set_field("age");
    f->set_operator_("lte");
    f->set_value("35");

    themis::api::VectorSearchResponse resp;
    grpc::ServerContext ctx;
    const auto status = rpc_->FilteredVectorSearch(&ctx, &req, &resp);
    ASSERT_TRUE(status.ok());
    ASSERT_TRUE(resp.success());
    ASSERT_EQ(resp.hits_size(), 2);
    EXPECT_EQ(resp.hits(0).key(), "key_0");
    EXPECT_EQ(resp.hits(1).key(), "key_1");
}

TEST_F(GrpcVectorFetchDocsFallbackTest, FilteredVectorSearchAppliesTopLevelNumericInFilter) {
    seedUserEntity("key_0", "Alice", "gold", 21);
    seedUserEntity("key_1", "Bob", "silver", 35);
    seedUserEntity("key_2", "Charlie", "silver", 44);

    themis::api::FilteredVectorSearchRequest req;
    req.set_collection("users");
    req.set_k(3);
    req.mutable_query_vector()->add_values(0.9f);
    auto* f = req.add_filters();
    f->set_field("age");
    f->set_operator_("in");
    f->set_value(R"([21,44])");

    themis::api::VectorSearchResponse resp;
    grpc::ServerContext ctx;
    const auto status = rpc_->FilteredVectorSearch(&ctx, &req, &resp);
    ASSERT_TRUE(status.ok());
    ASSERT_TRUE(resp.success());
    ASSERT_EQ(resp.hits_size(), 2);
    EXPECT_EQ(resp.hits(0).key(), "key_0");
    EXPECT_EQ(resp.hits(1).key(), "key_2");
}

TEST_F(GrpcVectorFetchDocsFallbackTest, FilteredVectorSearchIgnoresMixedTypeNumericInFilter) {
    seedUserEntity("key_0", "Alice", "gold", 21);
    seedUserEntity("key_1", "Bob", "silver", 35);
    seedUserEntity("key_2", "Charlie", "silver", 44);

    themis::api::FilteredVectorSearchRequest req;
    req.set_collection("users");
    req.set_k(3);
    req.mutable_query_vector()->add_values(1.0f);
    auto* f = req.add_filters();
    f->set_field("age");
    f->set_operator_("in");
    f->set_value(R"([21,"44"])" );

    themis::api::VectorSearchResponse resp;
    grpc::ServerContext ctx;
    const auto status = rpc_->FilteredVectorSearch(&ctx, &req, &resp);
    ASSERT_TRUE(status.ok());
    ASSERT_TRUE(resp.success());
    // Mixed numeric/string array is treated as unsupported filter and ignored.
    ASSERT_EQ(resp.hits_size(), 3);
    EXPECT_EQ(resp.hits(0).key(), "key_0");
    EXPECT_EQ(resp.hits(1).key(), "key_1");
    EXPECT_EQ(resp.hits(2).key(), "key_2");
}

TEST_F(GrpcVectorFetchDocsFallbackTest, FilteredVectorSearchIgnoresInvalidNumericInPayload) {
    seedUserEntity("key_0", "Alice", "gold", 21);
    seedUserEntity("key_1", "Bob", "silver", 35);
    seedUserEntity("key_2", "Charlie", "silver", 44);

    themis::api::FilteredVectorSearchRequest req;
    req.set_collection("users");
    req.set_k(3);
    req.mutable_query_vector()->add_values(1.1f);
    auto* f = req.add_filters();
    f->set_field("age");
    f->set_operator_("in");
    f->set_value(R"([{"x":1}])");

    themis::api::VectorSearchResponse resp;
    grpc::ServerContext ctx;
    const auto status = rpc_->FilteredVectorSearch(&ctx, &req, &resp);
    ASSERT_TRUE(status.ok());
    ASSERT_TRUE(resp.success());
    // Non-scalar array items are unsupported and ignored.
    ASSERT_EQ(resp.hits_size(), 3);
    EXPECT_EQ(resp.hits(0).key(), "key_0");
    EXPECT_EQ(resp.hits(1).key(), "key_1");
    EXPECT_EQ(resp.hits(2).key(), "key_2");
}

TEST_F(GrpcVectorFetchDocsFallbackTest, FilteredVectorSearchAppliesTopLevelNumericEqWithTolerance) {
    seedUserEntity("key_0", "Alice", "gold", -1, true, 0.30000000000000004);
    seedUserEntity("key_1", "Bob", "silver", -1, true, 0.31);
    seedUserEntity("key_2", "Charlie", "silver", -1, true, 0.29);

    themis::api::FilteredVectorSearchRequest req;
    req.set_collection("users");
    req.set_k(3);
    req.mutable_query_vector()->add_values(1.2f);
    auto* f = req.add_filters();
    f->set_field("score");
    f->set_operator_("eq");
    f->set_value("0.3");

    themis::api::VectorSearchResponse resp;
    grpc::ServerContext ctx;
    const auto status = rpc_->FilteredVectorSearch(&ctx, &req, &resp);
    ASSERT_TRUE(status.ok());
    ASSERT_TRUE(resp.success());
    ASSERT_EQ(resp.hits_size(), 1);
    EXPECT_EQ(resp.hits(0).key(), "key_0");
}

TEST_F(GrpcVectorFetchDocsFallbackTest, FilteredVectorSearchAppliesTopLevelNumericInWithTolerance) {
    seedUserEntity("key_0", "Alice", "gold", -1, true, 0.30000000000000004);
    seedUserEntity("key_1", "Bob", "silver", -1, true, 0.31);
    seedUserEntity("key_2", "Charlie", "silver", -1, true, 0.29);

    themis::api::FilteredVectorSearchRequest req;
    req.set_collection("users");
    req.set_k(3);
    req.mutable_query_vector()->add_values(1.3f);
    auto* f = req.add_filters();
    f->set_field("score");
    f->set_operator_("in");
    f->set_value(R"([0.3])");

    themis::api::VectorSearchResponse resp;
    grpc::ServerContext ctx;
    const auto status = rpc_->FilteredVectorSearch(&ctx, &req, &resp);
    ASSERT_TRUE(status.ok());
    ASSERT_TRUE(resp.success());
    ASSERT_EQ(resp.hits_size(), 1);
    EXPECT_EQ(resp.hits(0).key(), "key_0");
}

TEST_F(GrpcVectorFetchDocsFallbackTest, FilteredVectorSearchAppliesTopLevelNumericNeWithTolerance) {
    seedUserEntity("key_0", "Alice", "gold", -1, true, 0.30000000000000004);
    seedUserEntity("key_1", "Bob", "silver", -1, true, 0.31);
    seedUserEntity("key_2", "Charlie", "silver", -1, true, 0.29);

    themis::api::FilteredVectorSearchRequest req;
    req.set_collection("users");
    req.set_k(3);
    req.mutable_query_vector()->add_values(1.4f);
    auto* f = req.add_filters();
    f->set_field("score");
    f->set_operator_("ne");
    f->set_value("0.3");

    themis::api::VectorSearchResponse resp;
    grpc::ServerContext ctx;
    const auto status = rpc_->FilteredVectorSearch(&ctx, &req, &resp);
    ASSERT_TRUE(status.ok());
    ASSERT_TRUE(resp.success());
    ASSERT_EQ(resp.hits_size(), 2);
    EXPECT_EQ(resp.hits(0).key(), "key_1");
    EXPECT_EQ(resp.hits(1).key(), "key_2");
}

} // namespace

#endif

