/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            themisdb_grpc_service.cpp                          ║
  Version:         0.0.15                                             ║
  Last Modified:   2026-04-15 18:48:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   92.0/100                                       ║
    • Total Lines:     849                                            ║
    • Open Issues:     TODOs: 0, Stubs: 4                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7c2cc11ffb  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • ad6e8f172c  2026-04-14  refactor: replace (void)var; suppressions with C++17 [[ma... ║
    • ed2d46e8d1  2026-04-13  fix(api): complete gRPC stub wiring — bounds checks, stop... ║
    • 11ddb98b9f  2026-04-09  Add comprehensive documentation and security measures for... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "api/themisdb_grpc_service.h"
#include "api/aql_utils.h"
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
#include <exception>
#include <mutex>
#include <string>
#include <utility>
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

using themis::api::aqlEscapeLiteral;
using themis::api::isValidAqlIdentifier;

std::mutex g_api_grpc_service_mutex;
themis::api::ThemisDBGrpcService::ServiceFn g_api_grpc_service_fn;

/// Escape a string for safe embedding inside an AQL single-quoted literal.
/// Replaces backslashes and single-quotes to prevent AQL injection.
/// Kept as a local alias for backward compatibility with call sites below.
inline std::string aqlEscape(const std::string& raw) {
    return aqlEscapeLiteral(raw);
}

/// Validate a collection name used as an AQL identifier (FOR doc IN <name>).
/// Delegates to the shared header implementation.
inline bool isValidCollectionName(const std::string& name) {
    return isValidAqlIdentifier(name);
}

/// Version-counter key for a stored document.
/// Stored as a plain decimal string so that no additional serialisation is
/// needed and the counter survives a process restart.
inline std::string versionKey(const std::string& storage_key) {
    return "__ver/" + storage_key;
}

/// Hard upper bound on the number of items in a single BatchWrite or BatchRead
/// request (upserts + deletes for BatchWrite; keys for BatchRead).  Requests
/// that exceed this limit are rejected with RESOURCE_EXHAUSTED to prevent
/// unbounded memory allocation on the server.
static constexpr int kMaxBatchItems = 10'000;

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

            // Persist version counter (always 1 for a fresh create).
            db_->put(versionKey(storage_key), "1");

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

            // Read-increment-store version counter.
            uint64_t new_version = 1;
            if (db_) {
                std::string ver_str;
                if (db_->get(versionKey(storage_key), ver_str)) {
                    try { new_version = std::stoull(ver_str) + 1; }
                    catch (...) {
                        THEMIS_WARN("UpdateDocument: malformed version counter '{}' for key '{}'; "
                                    "resetting to 1", ver_str, storage_key);
                        new_version = 1;
                    }
                }
                db_->put(versionKey(storage_key), std::to_string(new_version));
            }

            resp->set_success(true);
            resp->set_key(doc.key());
            resp->set_version(new_version);
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
            // Reject oversized batches before allocating any per-item resources.
            if (req->upserts_size() + req->deletes_size() > kMaxBatchItems) {
                return grpc::Status(
                    grpc::StatusCode::RESOURCE_EXHAUSTED,
                    "batch exceeds maximum of " + std::to_string(kMaxBatchItems) + " items; "
                    "split the request into smaller batches");
            }

            int upserted = 0;
            int deleted  = 0;
            bool all_ok  = true;

            for (const auto& doc : req->upserts()) {
                const std::string key  = doc.collection() + "/" + doc.key();
                const std::string body(doc.body().begin(), doc.body().end());
                if (db_ && db_->put(key, body)) {
                    ++upserted;
                } else {
                    all_ok = false;
                }
            }

            for (const auto& del_key : req->deletes()) {
                if (db_ && db_->del(del_key)) {
                    ++deleted;
                } else {
                    all_ok = false;
                }
            }

            resp->set_success(all_ok);
            resp->set_upserted_count(upserted);
            resp->set_deleted_count(deleted);
            if (!all_ok) {
                const int expected = req->upserts_size() + req->deletes_size();
                const int done     = upserted + deleted;
                auto* err = resp->mutable_error();
                err->set_code(207);
                err->set_message("partial failure: " + std::to_string(done) +
                                 "/" + std::to_string(expected) +
                                 " operations succeeded");
            }
            return grpc::Status::OK;
        }

        grpc::Status BatchRead(
            grpc::ServerContext*      /*ctx*/,
            const BatchReadRequest*   req,
            BatchReadResponse*        resp
        ) override {
            // Reject oversized key lists before allocating any per-item resources.
            if (req->keys_size() > kMaxBatchItems) {
                return grpc::Status(
                    grpc::StatusCode::RESOURCE_EXHAUSTED,
                    "batch exceeds maximum of " + std::to_string(kMaxBatchItems) + " keys; "
                    "split the request into smaller batches");
            }

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
#if THEMIS_HAS_JSON
            try {
                const auto json = nlohmann::json::parse(*result);
                if (json.is_array()) {
                    for (const auto& element : json) {
                        auto* row = resp->add_rows();
                        row->set_data(element.dump());
                        row->set_has_more(false);
                    }
                } else {
                    auto* row = resp->add_rows();
                    row->set_data(*result);
                    row->set_has_more(false);
                }
            } catch (...) {
                // Fall back to raw payload when response is not valid JSON.
                auto* row = resp->add_rows();
                row->set_data(*result);
                row->set_has_more(false);
            }
#else
            auto* row = resp->add_rows();
            row->set_data(*result);
            row->set_has_more(false);
#endif
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

            // Validate the collection name used as an AQL identifier.
            if (!req->collection().empty() && !isValidCollectionName(req->collection())) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(400);
                err->set_message("invalid collection name: must match [a-zA-Z_][a-zA-Z0-9_]*");
                return grpc::Status::OK;
            }

            const uint32_t k = req->k() > 0 ? static_cast<uint32_t>(req->k()) : 10;
            const float alpha = req->alpha() > 0.0f ? req->alpha() : 0.5f;
            resp->set_success(true);

            // Dense component (vector index)
            if (vector_index_ && req->has_dense_vector() &&
                req->dense_vector().values_size() > 0) {
                const std::vector<float> embedding(
                    req->dense_vector().values().begin(),
                    req->dense_vector().values().end());
                const auto dense_hits = vector_index_->search(embedding, k);
                for (const auto& hit : dense_hits) {
                    auto* h = resp->add_hits();
                    h->set_collection(req->collection());
                    h->set_key(hit.primary_key);
                    h->set_score(hit.distance * alpha);
                }
            }

            // Sparse component (AQL full-text via engine).
            // Collection name is an AQL identifier (validated above); the query
            // string is embedded inside a single-quoted literal (escaped below).
            if (aql_engine_ && !req->sparse_query().empty()) {
                const std::string aql =
                    "FOR doc IN " + req->collection() +
                    " FILTER FULLTEXT(doc, 'text', '" + aqlEscape(req->sparse_query()) + "')"
                    " LIMIT " + std::to_string(k) + " RETURN doc";
                auto result = aql_engine_->execute(aql);
                if (result) {
                    const float sparse_weight = 1.0f - alpha;
#if THEMIS_HAS_JSON
                    try {
                        const auto json = nlohmann::json::parse(*result);
                        if (json.is_array()) {
                            for (const auto& element : json) {
                                if (!element.is_object()) continue;
                                auto* h = resp->add_hits();
                                h->set_collection(req->collection());
                                if (element.contains("_key")) {
                                    h->set_key(element["_key"].get<std::string>());
                                }
                                h->set_score(sparse_weight);
                            }
                        }
                    } catch (const nlohmann::json::parse_error& e) {
                        // AQL engine returned non-JSON (or malformed JSON) – emit
                        // as single hit and log so that engine issues are diagnosable.
                        THEMIS_WARN("ThemisDBGrpcService: HybridSearch – AQL result is not "
                                    "valid JSON ({}); emitting as single hit", e.what());
                        auto* h = resp->add_hits();
                        h->set_collection(req->collection());
                        h->set_score(sparse_weight);
                    }
#else
#endif
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

            // Validate the collection name used as an AQL identifier.
            if (!isValidCollectionName(req->collection())) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(400);
                err->set_message("invalid collection name: must match [a-zA-Z_][a-zA-Z0-9_]*");
                return grpc::Status::OK;
            }

            const int limit = req->max_results() > 0 ? req->max_results() : 10;
            // Collection name is a validated AQL identifier; query is embedded
            // inside a single-quoted AQL literal and escaped accordingly.
            const std::string aql =
                "FOR doc IN " + req->collection() +
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
            } catch (const nlohmann::json::parse_error& e) {
                // AQL engine returned non-JSON (or malformed JSON) – pass through
                // in degraded mode and log so that engine issues are diagnosable.
                THEMIS_WARN("ThemisDBGrpcService: FullTextSearch – AQL result is not "
                            "valid JSON ({}); degraded mode, no hits emitted", e.what());
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
    // STUB/SIMULATION NOTE:
    // Purpose: Allow ThemisDBGrpcService to be constructed and linked without
    //   the generated protobuf/gRPC stub files for themisdb.proto.  The
    //   service instance is null; service() returns nullptr so the gRPC server
    //   omits this service from its handler list.
    // Activation: THEMIS_HAS_API_GRPC == 0 (default when protoc has not been
    //   run against proto/themisdb.proto, or when themisdb.grpc.pb.h is not
    //   on the include path).
    // Production Delta: ThemisDBService is completely absent from the gRPC
    //   server; gRPC clients receive UNIMPLEMENTED for every method.  Document
    //   reads/writes, transaction operations, and vector search are inaccessible
    //   via the primary gRPC API surface.
    // Removal Plan: Run `cmake -DTHEMIS_ENABLE_GRPC=ON` with protoc installed;
    //   themisdb.grpc.pb.{h,cc} will be generated and THEMIS_HAS_API_GRPC set
    //   to 1.  This #else block becomes dead code.
    // Roadmap ref: src/api/FUTURE_ENHANCEMENTS.md §"gRPC API Service Activation"
    THEMIS_WARN("ThemisDBGrpcService: themisdb.grpc.pb.h not found; "
                "service will be a no-op until protoc generates the stubs");

    // Try injected accessor (for non-proto builds wiring an external service).
    ServiceFn fn;
    {
        std::lock_guard<std::mutex> lock(g_api_grpc_service_mutex);
        fn = g_api_grpc_service_fn;
    }
    if (fn) {
        try {
            service_ptr_ = fn();
        } catch (const std::exception& e) {
            THEMIS_ERROR("ThemisDBGrpcService: service callback failed: {}", e.what());
            service_ptr_ = nullptr;
        } catch (...) {
            THEMIS_ERROR("ThemisDBGrpcService: service callback failed: unknown error");
            service_ptr_ = nullptr;
        }
    }
#endif
}

ThemisDBGrpcService::~ThemisDBGrpcService() = default;

void ThemisDBGrpcService::setServiceFn(ServiceFn fn) {
    std::lock_guard<std::mutex> lock(g_api_grpc_service_mutex);
    g_api_grpc_service_fn = std::move(fn);
}

void* ThemisDBGrpcService::service() {
#if THEMIS_HAS_API_GRPC
    return impl_ ? static_cast<void*>(impl_->get()) : nullptr;
#else
    return service_ptr_;
#endif
}

} // namespace api
} // namespace themis

