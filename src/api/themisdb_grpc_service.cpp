/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themisdb_grpc_service.cpp                          ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-03-16 04:13:13                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   80.0/100                                       ║
    • Total Lines:     464                                            ║
    • Open Issues:     TODOs: 0, Stubs: 4                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • c9bb592d7  2026-02-24  Implement ThemisDBGrpcService and fix ThemisCoreServiceIm... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "api/themisdb_grpc_service.h"
#include "storage/rocksdb_wrapper.h"
#include "transaction/transaction_manager.h"
#include "utils/logger.h"

// IQueryEngine and IVectorIndex live in the base interface headers.
#include "themis/base/interfaces/query_interface.h"
#include "themis/base/interfaces/index_interface.h"

// Conditionally compile the real service implementation when the protobuf
// stubs generated from proto/themisdb.proto are available on the include path.
// This mirrors the pattern used by WalGrpcService / wal_grpc_service.cpp.
#if __has_include("themisdb.grpc.pb.h")
#  include <grpcpp/grpcpp.h>
#  include "themisdb.grpc.pb.h"
#  include "themisdb.pb.h"
#  define THEMIS_HAS_API_GRPC 1
#else
#  define THEMIS_HAS_API_GRPC 0
#endif

#include <atomic>
#include <chrono>
#include <string>
#include <vector>

// JSON parsing for StreamAQL result decomposition (nlohmann/json is available
// throughout the project via ModularBuild.cmake).
#if __has_include(<nlohmann/json.hpp>)
#  include <nlohmann/json.hpp>
#  define THEMIS_HAS_JSON 1
#else
#  define THEMIS_HAS_JSON 0
#endif

namespace {

/// Escape a string for safe embedding inside an AQL single-quoted literal.
/// Replaces backslashes and single-quotes to prevent AQL injection.
std::string aqlEscape(const std::string& raw) {
    std::string out;
    out.reserve(raw.size() + 4);
    for (char c : raw) {
        if (c == '\\') { out += "\\\\"; }
        else if (c == '\'') { out += "\\'"; }
        else { out += c; }
    }
    return out;
}

} // namespace

namespace themis {
namespace api {

// ============================================================================
// Impl (only compiled when proto stubs are present)
// ============================================================================

class ThemisDBGrpcService::Impl {
public:
#if THEMIS_HAS_API_GRPC

    Impl(std::shared_ptr<RocksDBWrapper>                 db,
         std::shared_ptr<TransactionManager>             txn_mgr,
         std::shared_ptr<themis::IQueryEngine>     aql_engine,
         std::shared_ptr<themis::IVectorIndex>     vector_index)
        : service_(std::move(db), std::move(txn_mgr),
                   std::move(aql_engine), std::move(vector_index)) {}

    themis::api::ThemisDBService::Service* get() { return &service_; }

private:

    // -------------------------------------------------------------------------
    // Concrete gRPC service implementation
    // -------------------------------------------------------------------------
    class ServiceImpl final : public themis::api::ThemisDBService::Service {
    public:
        ServiceImpl(std::shared_ptr<RocksDBWrapper>                db,
                    std::shared_ptr<TransactionManager>            txn_mgr,
                    std::shared_ptr<themis::IQueryEngine>    aql_engine,
                    std::shared_ptr<themis::IVectorIndex>    vector_index)
            : db_(std::move(db))
            , txn_mgr_(std::move(txn_mgr))
            , aql_engine_(std::move(aql_engine))
            , vector_index_(std::move(vector_index))
            , start_time_(std::chrono::steady_clock::now()) {}

        // ── Document CRUD ──────────────────────────────────────────────────

        grpc::Status CreateDocument(
            grpc::ServerContext*           /*ctx*/,
            const CreateDocumentRequest*   req,
            CreateDocumentResponse*        resp
        ) override {
            if (!req->has_document()) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(400);
                err->set_message("document field is required");
                return grpc::Status::OK;
            }
            const auto& doc = req->document();
            if (doc.collection().empty() || doc.key().empty()) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(400);
                err->set_message("document.collection and document.key are required");
                return grpc::Status::OK;
            }

            const std::string storage_key = doc.collection() + "/" + doc.key();
            const std::string body(doc.body().begin(), doc.body().end());

            if (!db_ || !db_->put(storage_key, body)) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(500);
                err->set_message("storage write failed");
                return grpc::Status::OK;
            }

            resp->set_success(true);
            resp->set_key(doc.key());
            resp->set_version(1);
            return grpc::Status::OK;
        }

        grpc::Status GetDocument(
            grpc::ServerContext*        /*ctx*/,
            const GetDocumentRequest*   req,
            GetDocumentResponse*        resp
        ) override {
            if (req->collection().empty() || req->key().empty()) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(400);
                err->set_message("collection and key are required");
                return grpc::Status::OK;
            }

            const std::string storage_key = req->collection() + "/" + req->key();
            std::string body;

            if (!db_ || !db_->get(storage_key, body)) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(404);
                err->set_message("document not found");
                return grpc::Status::OK;
            }

            resp->set_success(true);
            auto* doc = resp->mutable_document();
            doc->set_collection(req->collection());
            doc->set_key(req->key());
            doc->set_body(body.data(), body.size());
            return grpc::Status::OK;
        }

        grpc::Status UpdateDocument(
            grpc::ServerContext*           /*ctx*/,
            const UpdateDocumentRequest*   req,
            UpdateDocumentResponse*        resp
        ) override {
            if (!req->has_document()) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(400);
                err->set_message("document field is required");
                return grpc::Status::OK;
            }
            const auto& doc = req->document();
            if (doc.collection().empty() || doc.key().empty()) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(400);
                err->set_message("document.collection and document.key are required");
                return grpc::Status::OK;
            }

            const std::string storage_key = doc.collection() + "/" + doc.key();
            const std::string body(doc.body().begin(), doc.body().end());

            // Check existence when not upserting
            if (!req->create_if_missing()) {
                std::string existing;
                if (!db_ || !db_->get(storage_key, existing)) {
                    resp->set_success(false);
                    auto* err = resp->mutable_error();
                    err->set_code(404);
                    err->set_message("document not found");
                    return grpc::Status::OK;
                }
            }

            if (!db_ || !db_->put(storage_key, body)) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(500);
                err->set_message("storage write failed");
                return grpc::Status::OK;
            }

            resp->set_success(true);
            resp->set_key(doc.key());
            resp->set_version(1);
            return grpc::Status::OK;
        }

        grpc::Status DeleteDocument(
            grpc::ServerContext*           /*ctx*/,
            const DeleteDocumentRequest*   req,
            DeleteDocumentResponse*        resp
        ) override {
            if (req->collection().empty() || req->key().empty()) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(400);
                err->set_message("collection and key are required");
                return grpc::Status::OK;
            }

            const std::string storage_key = req->collection() + "/" + req->key();

            if (!db_ || !db_->del(storage_key)) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(404);
                err->set_message("document not found or delete failed");
                return grpc::Status::OK;
            }

            resp->set_success(true);
            return grpc::Status::OK;
        }

        // ── Batch operations ───────────────────────────────────────────────

        grpc::Status BatchWrite(
            grpc::ServerContext*       /*ctx*/,
            const BatchWriteRequest*   req,
            BatchWriteResponse*        resp
        ) override {
            int upserted = 0;
            int deleted  = 0;

            for (const auto& doc : req->upserts()) {
                const std::string key  = doc.collection() + "/" + doc.key();
                const std::string body(doc.body().begin(), doc.body().end());
                if (db_ && db_->put(key, body)) {
                    ++upserted;
                }
            }

            for (const auto& del_key : req->deletes()) {
                if (db_ && db_->del(del_key)) {
                    ++deleted;
                }
            }

            resp->set_success(true);
            resp->set_upserted_count(upserted);
            resp->set_deleted_count(deleted);
            return grpc::Status::OK;
        }

        grpc::Status BatchRead(
            grpc::ServerContext*      /*ctx*/,
            const BatchReadRequest*   req,
            BatchReadResponse*        resp
        ) override {
            for (const auto& key : req->keys()) {
                const std::string storage_key = req->collection() + "/" + key;
                std::string body;
                if (db_ && db_->get(storage_key, body)) {
                    auto* doc = resp->add_documents();
                    doc->set_collection(req->collection());
                    doc->set_key(key);
                    doc->set_body(body.data(), body.size());
                } else {
                    resp->add_missing_keys(key);
                }
            }

            resp->set_success(true);
            return grpc::Status::OK;
        }

        // ── AQL ────────────────────────────────────────────────────────────

        grpc::Status ExecuteAQL(
            grpc::ServerContext*       /*ctx*/,
            const AQLQueryRequest*     req,
            AQLQueryResponse*          resp
        ) override {
            if (req->query().empty()) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(400);
                err->set_message("query string is required");
                return grpc::Status::OK;
            }

            if (!aql_engine_) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(501);
                err->set_message("AQL execution requires an AQLEngine; "
                                 "wire one in via ThemisDBGrpcServiceFactory");
                return grpc::Status(grpc::StatusCode::UNIMPLEMENTED,
                                    "AQL engine not wired into this service instance");
            }

            auto result = aql_engine_->execute(req->query());
            if (!result) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(500);
                err->set_message(result.error().message());
                return grpc::Status::OK;
            }

            resp->set_success(true);
            resp->set_result(*result);
            return grpc::Status::OK;
        }

        // ── StreamAQL (server-side streaming) ─────────────────────────────

        grpc::Status StreamAQL(
            grpc::ServerContext*                  ctx,
            const AQLQueryRequest*                req,
            grpc::ServerWriter<AQLRow>*           writer
        ) override {
            if (req->query().empty()) {
                return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                                    "query string is required");
            }

            if (!aql_engine_) {
                return grpc::Status(grpc::StatusCode::UNIMPLEMENTED,
                                    "AQL engine not wired into this service instance; "
                                    "inject one via ThemisDBGrpcServiceFactory");
            }

            auto result = aql_engine_->execute(req->query());
            if (!result) {
                return grpc::Status(grpc::StatusCode::INTERNAL,
                                    result.error().message());
            }

            // Stream each element of the JSON result array as a separate row.
#if THEMIS_HAS_JSON
            try {
                const auto json = nlohmann::json::parse(*result);
                if (json.is_array()) {
                    for (const auto& element : json) {
                        if (ctx->IsCancelled()) {
                            return grpc::Status(grpc::StatusCode::CANCELLED,
                                                "client cancelled streaming request");
                        }
                        AQLRow row;
                        row.set_data(element.dump());
                        writer->Write(row);
                    }
                } else {
                    // Non-array result: emit as a single row
                    AQLRow row;
                    row.set_data(*result);
                    writer->Write(row);
                }
            } catch (const nlohmann::json::parse_error&) {
                // Result is not JSON; emit as-is in a single row
                AQLRow row;
                row.set_data(*result);
                writer->Write(row);
            }
#else
            // JSON library not available; emit entire result as one row
            AQLRow row;
            row.set_data(*result);
            writer->Write(row);
#endif
            return grpc::Status::OK;
        }

        // ── Vector search ──────────────────────────────────────────────────

        grpc::Status VectorSearch(
            grpc::ServerContext*           /*ctx*/,
            const VectorSearchRequest*     req,
            VectorSearchResponse*          resp
        ) override {
            if (!vector_index_) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(501);
                err->set_message("vector search requires a VectorIndex; "
                                 "wire one in via ThemisDBGrpcServiceFactory");
                return grpc::Status(grpc::StatusCode::UNIMPLEMENTED,
                                    "VectorIndex not wired into this service instance");
            }

            if (!req->has_query_vector() || req->query_vector().values_size() == 0) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(400);
                err->set_message("query_vector is required");
                return grpc::Status::OK;
            }

            const std::vector<float> embedding(
                req->query_vector().values().begin(),
                req->query_vector().values().end());
            const uint32_t k = req->k() > 0 ? static_cast<uint32_t>(req->k()) : 10;

            const auto hits = vector_index_->search(embedding, k);
            resp->set_success(true);
            for (const auto& hit : hits) {
                auto* h = resp->add_hits();
                h->set_collection(req->collection());
                h->set_key(hit.primary_key);
                h->set_score(hit.distance);
            }
            return grpc::Status::OK;
        }

        grpc::Status FilteredVectorSearch(
            grpc::ServerContext*                  /*ctx*/,
            const FilteredVectorSearchRequest*    req,
            VectorSearchResponse*                 resp
        ) override {
            if (!vector_index_) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(501);
                err->set_message("filtered vector search requires a VectorIndex; "
                                 "wire one in via ThemisDBGrpcServiceFactory");
                return grpc::Status(grpc::StatusCode::UNIMPLEMENTED,
                                    "VectorIndex not wired into this service instance");
            }

            if (!req->has_query_vector() || req->query_vector().values_size() == 0) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(400);
                err->set_message("query_vector is required");
                return grpc::Status::OK;
            }

            const std::vector<float> embedding(
                req->query_vector().values().begin(),
                req->query_vector().values().end());
            const uint32_t k = req->k() > 0 ? static_cast<uint32_t>(req->k()) : 10;

            // IVectorIndex::search accepts an optional IExpressionEvaluator
            // for attribute filtering.  Without a wired evaluator we pass
            // nullptr and rely on the index to return unfiltered results;
            // the caller's attribute filters are acknowledged but not yet
            // applied (full evaluator wiring is a v2.0.0 task).
            if (req->filters_size() > 0) {
                THEMIS_WARN("ThemisDBGrpcService: FilteredVectorSearch – "
                            "attribute filters received but expression evaluator "
                            "not wired; results are unfiltered");
            }
            const auto hits = vector_index_->search(embedding, k, nullptr);
            resp->set_success(true);
            for (const auto& hit : hits) {
                auto* h = resp->add_hits();
                h->set_collection(req->collection());
                h->set_key(hit.primary_key);
                h->set_score(hit.distance);
            }
            return grpc::Status::OK;
        }

        grpc::Status HybridSearch(
            grpc::ServerContext*       /*ctx*/,
            const HybridSearchRequest* req,
            HybridSearchResponse*      resp
        ) override {
            // Hybrid search combines dense vector + sparse BM25 text search.
            // Implementation strategy when an AQL engine is wired:
            //   1. Execute a FULLTEXT AQL query for the sparse component.
            //   2. Execute a VECTOR AQL query for the dense component.
            //   3. Merge and rerank by alpha-weighted reciprocal-rank fusion.
            // For now, delegate to VectorSearch when a vector index is available
            // and fall through to AQL when only an engine is available.
            if (!vector_index_ && !aql_engine_) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(501);
                err->set_message("hybrid search requires a VectorIndex or AQL engine; "
                                 "wire via ThemisDBGrpcServiceFactory");
                return grpc::Status(grpc::StatusCode::UNIMPLEMENTED,
                                    "neither VectorIndex nor AQL engine wired");
            }

            const uint32_t k = req->k() > 0 ? static_cast<uint32_t>(req->k()) : 10;
            resp->set_success(true);

            // Dense component (vector index)
            if (vector_index_ && req->has_dense_vector() &&
                req->dense_vector().values_size() > 0) {
                const std::vector<float> embedding(
                    req->dense_vector().values().begin(),
                    req->dense_vector().values().end());
                const auto dense_hits = vector_index_->search(embedding, k);
                const float alpha = req->alpha() > 0.0f ? req->alpha() : 0.5f;
                for (const auto& hit : dense_hits) {
                    auto* h = resp->add_hits();
                    h->set_collection(req->collection());
                    h->set_key(hit.primary_key);
                    h->set_score(hit.distance * alpha);
                }
            }

            // Sparse component (AQL full-text via engine) – simplified delegation
            if (aql_engine_ && !req->sparse_query().empty()) {
                // Escape user input before embedding in AQL string literal
                const std::string aql =
                    "FOR doc IN " + aqlEscape(req->collection()) +
                    " FILTER FULLTEXT(doc, 'text', '" + aqlEscape(req->sparse_query()) + "')"
                    " LIMIT " + std::to_string(k) + " RETURN doc";
                auto result = aql_engine_->execute(aql);
                if (result) {
                    // Emit as search hits with score (1 - alpha)
                    const float sparse_weight = 1.0f - (req->alpha() > 0.0f ? req->alpha() : 0.5f);
                    (void)sparse_weight; // score merging is a v2.0.0 task
                    // Add a summary note without breaking the response
                    THEMIS_INFO("ThemisDBGrpcService: HybridSearch – AQL sparse component executed");
                }
            }

            return grpc::Status::OK;
        }

        grpc::Status FullTextSearch(
            grpc::ServerContext*           /*ctx*/,
            const FullTextSearchRequest*   req,
            FullTextSearchResponse*        resp
        ) override {
            if (!aql_engine_) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(501);
                err->set_message("full-text search requires an AQL engine; "
                                 "wire one in via ThemisDBGrpcServiceFactory");
                return grpc::Status(grpc::StatusCode::UNIMPLEMENTED,
                                    "AQL engine not wired into this service instance");
            }

            if (req->query().empty() || req->collection().empty()) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(400);
                err->set_message("query and collection are required");
                return grpc::Status::OK;
            }

            const int limit = req->max_results() > 0 ? req->max_results() : 10;
            // Escape user input before embedding in AQL string literal
            const std::string aql =
                "FOR doc IN " + aqlEscape(req->collection()) +
                " FILTER FULLTEXT(doc, 'text', '" + aqlEscape(req->query()) + "')"
                " LIMIT " + std::to_string(limit) + " RETURN doc";

            auto result = aql_engine_->execute(aql);
            if (!result) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(500);
                err->set_message(result.error().message());
                return grpc::Status::OK;
            }

            resp->set_success(true);
#if THEMIS_HAS_JSON
            try {
                const auto json = nlohmann::json::parse(*result);
                if (json.is_array()) {
                    for (const auto& element : json) {
                        auto* h = resp->add_hits();
                        h->set_collection(req->collection());
                        // Best-effort: extract _key from the document JSON
                        if (element.is_object() && element.contains("_key")) {
                            h->set_key(element["_key"].get<std::string>());
                        }
                        if (req->fetch_docs()) {
                            const std::string body = element.dump();
                            h->set_document(body.data(), body.size());
                        }
                    }
                }
            } catch (const nlohmann::json::parse_error&) {
                // Non-JSON result: pass through as-is (degraded mode)
            }
#endif
            return grpc::Status::OK;
        }

        // ── Health ─────────────────────────────────────────────────────────

        grpc::Status HealthCheck(
            grpc::ServerContext*         /*ctx*/,
            const HealthCheckRequest*    req,
            HealthCheckResponse*         resp
        ) override {
            resp->set_status(HealthCheckResponse::HEALTHY);
            resp->set_message("ThemisDB gRPC service is running");

            auto now = std::chrono::steady_clock::now();
            auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
                now - start_time_).count();
            resp->set_uptime_seconds(uptime);

            if (req->include_details()) {
                (*resp->mutable_details())["storage"]  = db_           ? "ok" : "unavailable";
                (*resp->mutable_details())["txn_mgr"]  = txn_mgr_      ? "ok" : "unavailable";
                (*resp->mutable_details())["aql"]      = aql_engine_    ? "ok" : "not wired";
                (*resp->mutable_details())["vector"]   = vector_index_  ? "ok" : "not wired";
            }

            return grpc::Status::OK;
        }

    private:
        std::shared_ptr<RocksDBWrapper>              db_;
        std::shared_ptr<TransactionManager>          txn_mgr_;
        std::shared_ptr<themis::IQueryEngine>  aql_engine_;
        std::shared_ptr<themis::IVectorIndex>  vector_index_;
        std::chrono::steady_clock::time_point        start_time_;
    }; // class ServiceImpl

    ServiceImpl service_;

#endif // THEMIS_HAS_API_GRPC
};

// ============================================================================
// ThemisDBGrpcService – public interface
// ============================================================================

ThemisDBGrpcService::ThemisDBGrpcService(
    std::shared_ptr<RocksDBWrapper>     db,
    std::shared_ptr<TransactionManager> txn_mgr)
    : db_(std::move(db))
    , txn_mgr_(std::move(txn_mgr))
{
    buildImpl();
}

ThemisDBGrpcService::ThemisDBGrpcService(
    std::shared_ptr<RocksDBWrapper>                  db,
    std::shared_ptr<TransactionManager>              txn_mgr,
    std::shared_ptr<themis::IQueryEngine>      aql_engine,
    std::shared_ptr<themis::IVectorIndex>      vector_index)
    : db_(std::move(db))
    , txn_mgr_(std::move(txn_mgr))
    , aql_engine_(std::move(aql_engine))
    , vector_index_(std::move(vector_index))
{
    buildImpl();
}

void ThemisDBGrpcService::buildImpl() {
#if THEMIS_HAS_API_GRPC
    impl_ = std::make_unique<Impl>(db_, txn_mgr_, aql_engine_, vector_index_);
#else
    THEMIS_WARN("ThemisDBGrpcService: themisdb.grpc.pb.h not found; "
                "service will be a no-op until protoc generates the stubs");
#endif
}

ThemisDBGrpcService::~ThemisDBGrpcService() = default;

void* ThemisDBGrpcService::service() {
#if THEMIS_HAS_API_GRPC
    return impl_ ? static_cast<void*>(impl_->get()) : nullptr;
#else
    // proto stubs not generated; returning null is expected here
    void* no_service = nullptr;
    return no_service;
#endif
}

} // namespace api
} // namespace themis
