/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themisdb_grpc_service.cpp                          ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 03:57:01                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   80.0/100                                       ║
    • Total Lines:     463                                            ║
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

namespace themis {
namespace api {

// ============================================================================
// Impl (only compiled when proto stubs are present)
// ============================================================================

class ThemisDBGrpcService::Impl {
public:
#if THEMIS_HAS_API_GRPC

    Impl(std::shared_ptr<RocksDBWrapper>     db,
         std::shared_ptr<TransactionManager> txn_mgr)
        : service_(std::move(db), std::move(txn_mgr)) {}

    themis::api::ThemisDBService::Service* get() { return &service_; }

private:

    // -------------------------------------------------------------------------
    // Concrete gRPC service implementation
    // -------------------------------------------------------------------------
    class ServiceImpl final : public themis::api::ThemisDBService::Service {
    public:
        ServiceImpl(std::shared_ptr<RocksDBWrapper>     db,
                    std::shared_ptr<TransactionManager> txn_mgr)
            : db_(std::move(db))
            , txn_mgr_(std::move(txn_mgr))
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

            // AQL execution is delegated to the query engine.
            // The ThemisDBService does not bundle a full AQL engine instance;
            // callers that need full AQL support should integrate an AQLEngine
            // and wire it in via a dedicated service factory rather than this
            // storage-layer adapter.  Return UNIMPLEMENTED to signal this.
            resp->set_success(false);
            auto* err = resp->mutable_error();
            err->set_code(501);
            err->set_message("AQL execution requires an AQLEngine; "
                             "wire one in via ThemisDBGrpcServiceFactory");
            return grpc::Status(grpc::StatusCode::UNIMPLEMENTED,
                                "AQL engine not wired into this service instance");
        }

        // ── StreamAQL (server-side streaming) ─────────────────────────────
        //
        // Streams AQL result rows one at a time to the client.  When an
        // AQLEngine is wired in this is the entry point for server-side
        // streaming of large result sets; see FUTURE_ENHANCEMENTS.md – gRPC
        // API Surface.  Until the engine is injected, returns UNIMPLEMENTED.

        grpc::Status StreamAQL(
            grpc::ServerContext*                  /*ctx*/,
            const AQLQueryRequest*                req,
            grpc::ServerWriter<AQLRow>*           writer
        ) override {
            if (req->query().empty()) {
                return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                                    "query string is required");
            }

            // When an AQL engine is available (injected via the extended
            // constructor), delegate to it and stream results row by row:
            //
            //   engine_->executeStreaming(
            //       req->query(), req->bind_vars(), req->options(),
            //       [&](const std::string& row_json, bool has_more) {
            //           AQLRow row;
            //           row.set_data(row_json);
            //           row.set_has_more(has_more);
            //           writer->Write(row);
            //       });
            //
            // For now, report that the engine has not been wired in.
            (void)writer;
            return grpc::Status(grpc::StatusCode::UNIMPLEMENTED,
                                "AQL engine not wired into this service instance; "
                                "inject one via ThemisDBGrpcServiceFactory");
        }

        // ── Vector search ──────────────────────────────────────────────────

        grpc::Status VectorSearch(
            grpc::ServerContext*           /*ctx*/,
            const VectorSearchRequest*     /*req*/,
            VectorSearchResponse*          resp
        ) override {
            resp->set_success(false);
            auto* err = resp->mutable_error();
            err->set_code(501);
            err->set_message("vector search requires a VectorIndex; "
                             "wire one in via ThemisDBGrpcServiceFactory");
            return grpc::Status(grpc::StatusCode::UNIMPLEMENTED,
                                "VectorIndex not wired into this service instance");
        }

        grpc::Status FilteredVectorSearch(
            grpc::ServerContext*                  /*ctx*/,
            const FilteredVectorSearchRequest*    /*req*/,
            VectorSearchResponse*                 resp
        ) override {
            resp->set_success(false);
            auto* err = resp->mutable_error();
            err->set_code(501);
            err->set_message("filtered vector search not yet wired");
            return grpc::Status(grpc::StatusCode::UNIMPLEMENTED,
                                "VectorIndex not wired into this service instance");
        }

        grpc::Status HybridSearch(
            grpc::ServerContext*       /*ctx*/,
            const HybridSearchRequest* /*req*/,
            HybridSearchResponse*      resp
        ) override {
            resp->set_success(false);
            auto* err = resp->mutable_error();
            err->set_code(501);
            err->set_message("hybrid search not yet wired");
            return grpc::Status(grpc::StatusCode::UNIMPLEMENTED,
                                "VectorIndex not wired into this service instance");
        }

        grpc::Status FullTextSearch(
            grpc::ServerContext*           /*ctx*/,
            const FullTextSearchRequest*   /*req*/,
            FullTextSearchResponse*        resp
        ) override {
            resp->set_success(false);
            auto* err = resp->mutable_error();
            err->set_code(501);
            err->set_message("full-text search not yet wired");
            return grpc::Status(grpc::StatusCode::UNIMPLEMENTED,
                                "FullTextIndex not wired into this service instance");
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
                (*resp->mutable_details())["storage"]  = db_       ? "ok" : "unavailable";
                (*resp->mutable_details())["txn_mgr"]  = txn_mgr_  ? "ok" : "unavailable";
                (*resp->mutable_details())["aql"]      = "not wired (see ThemisDBGrpcServiceFactory)";
                (*resp->mutable_details())["vector"]   = "not wired (see ThemisDBGrpcServiceFactory)";
            }

            return grpc::Status::OK;
        }

    private:
        std::shared_ptr<RocksDBWrapper>     db_;
        std::shared_ptr<TransactionManager> txn_mgr_;
        std::chrono::steady_clock::time_point start_time_;
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
#if THEMIS_HAS_API_GRPC
    impl_ = std::make_unique<Impl>(db_, txn_mgr_);
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
