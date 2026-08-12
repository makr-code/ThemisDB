/**
 * @file themisdb_grpc_service.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=13; TODO=1, Stub=3, Unimpl=7, Mock=1, Sim=1, Debt=0, C=3, H=13, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "api/themisdb_grpc_service.h"
#include <stdexcept>
#include "api/aql_utils.h"
#include "storage/rocksdb_wrapper.h"
#include "storage/base_entity.h"
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
#  ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable : 4267)
#  endif
#  include "themisdb.grpc.pb.h"
#  include "themisdb.pb.h"
#  ifdef _MSC_VER
#    pragma warning(pop)
#  endif
#  define THEMIS_HAS_API_GRPC 1
#else
#  define THEMIS_HAS_API_GRPC 0
#endif

#ifdef THEMIS_HAS_PROMETHEUS
#include <prometheus/counter.h>
#include <prometheus/family.h>
#include <prometheus/registry.h>
#endif

#include <atomic>
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
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

#if THEMIS_HAS_API_GRPC
/// Map canonical Themis error codes to transport-level gRPC status codes.
/// This keeps RPC error semantics stable across API handlers.
grpc::StatusCode mapThemisErrorCodeToGrpcStatusCode(themis::errors::ErrorCode code) {
    using themis::errors::ErrorCode;
    switch (code) {
        case ErrorCode::ERR_API_UNAUTHORIZED:
            return grpc::StatusCode::UNAUTHENTICATED;

        case ErrorCode::ERR_QUERY_ACCESS_DENIED:
        case ErrorCode::ERR_UTIL_PERMISSION_DENIED:
        case ErrorCode::ERR_STORAGE_PERMISSION_DENIED:
        case ErrorCode::ERR_EXPORT_TENANT_UNAUTHORIZED:
        case ErrorCode::ERR_EXPORT_POLICY_DENIED:
            return grpc::StatusCode::PERMISSION_DENIED;

        case ErrorCode::ERR_DOC_NOT_FOUND:
        case ErrorCode::ERR_INDEX_NOT_FOUND:
        case ErrorCode::ERR_SCHEMA_TABLE_NOT_FOUND:
        case ErrorCode::ERR_BACKUP_NOT_FOUND:
        case ErrorCode::ERR_PLUGIN_NOT_FOUND:
        case ErrorCode::ERR_STORAGE_FILE_NOT_FOUND:
            return grpc::StatusCode::NOT_FOUND;

        case ErrorCode::ERR_QUERY_INVALID_SYNTAX:
        case ErrorCode::ERR_QUERY_INVALID:
        case ErrorCode::ERR_QUERY_INVALID_INPUT:
        case ErrorCode::ERR_UTIL_INVALID_ARGUMENT:
        case ErrorCode::ERR_API_INVALID_REQUEST:
            return grpc::StatusCode::INVALID_ARGUMENT;

        case ErrorCode::ERR_QUERY_TIMEOUT:
        case ErrorCode::ERR_NET_TIMEOUT:
        case ErrorCode::ERR_LLM_INFERENCE_TIMEOUT:
            return grpc::StatusCode::DEADLINE_EXCEEDED;

        case ErrorCode::ERR_QUERY_CANCELLED:
            return grpc::StatusCode::CANCELLED;

        case ErrorCode::ERR_QUERY_RESOURCE_EXHAUSTED:
        case ErrorCode::ERR_API_RATE_LIMIT:
        case ErrorCode::ERR_API_RESOURCE_EXHAUSTED:
        case ErrorCode::ERR_MEMORY_POOL_EXHAUSTED:
        case ErrorCode::ERR_MEMORY_ALLOCATION_FAILED:
        case ErrorCode::ERR_LLM_GPU_OOM:
        case ErrorCode::ERR_LLM_RAM_OOM:
        case ErrorCode::ERR_CACHE_ENTRY_TOO_LARGE:
        case ErrorCode::ERR_CACHE_FULL:
        case ErrorCode::ERR_EXPORT_SIZE_LIMIT_EXCEEDED:
            return grpc::StatusCode::RESOURCE_EXHAUSTED;

        case ErrorCode::ERR_STORAGE_TRANSACTION_FAILED:
            return grpc::StatusCode::ABORTED;

        case ErrorCode::ERR_NET_CONNECTION_REFUSED:
        case ErrorCode::ERR_NET_DNS_FAILURE:
            return grpc::StatusCode::UNAVAILABLE;

        case ErrorCode::ERR_PLUGIN_INCOMPATIBLE:
        case ErrorCode::ERR_QUERY_TYPE_MISMATCH:
        case ErrorCode::ERR_SCHEMA_INVALID_TYPE:
            return grpc::StatusCode::FAILED_PRECONDITION;

        default:
            return grpc::StatusCode::INTERNAL;
    }
}

grpc::Status mapThemisErrorToGrpcStatus(const themis::Error& error) {
    return grpc::Status(mapThemisErrorCodeToGrpcStatusCode(error.code()),
                        error.message());
}

const char* grpcStatusCodeLabel(grpc::StatusCode code) {
    switch (code) {
        case grpc::StatusCode::OK: return "ok";
        case grpc::StatusCode::CANCELLED: return "cancelled";
        case grpc::StatusCode::UNKNOWN: return "unknown";
        case grpc::StatusCode::INVALID_ARGUMENT: return "invalid_argument";
        case grpc::StatusCode::DEADLINE_EXCEEDED: return "deadline_exceeded";
        case grpc::StatusCode::NOT_FOUND: return "not_found";
        case grpc::StatusCode::ALREADY_EXISTS: return "already_exists";
        case grpc::StatusCode::PERMISSION_DENIED: return "permission_denied";
        case grpc::StatusCode::UNAUTHENTICATED: return "unauthenticated";
        case grpc::StatusCode::RESOURCE_EXHAUSTED: return "resource_exhausted";
        case grpc::StatusCode::FAILED_PRECONDITION: return "failed_precondition";
        case grpc::StatusCode::ABORTED: return "aborted";
        case grpc::StatusCode::OUT_OF_RANGE: return "out_of_range";
        case grpc::StatusCode::UNIMPLEMENTED: return "unimplemented";
        case grpc::StatusCode::INTERNAL: return "internal";
        case grpc::StatusCode::UNAVAILABLE: return "unavailable";
        case grpc::StatusCode::DATA_LOSS: return "data_loss";
        default: return "other";
    }
}
#endif

} // namespace

namespace themis {
namespace api {

// ============================================================================
// Impl (only compiled when proto stubs are present)
// ============================================================================

/** @brief Impl (only compiled when proto stubs are present). */
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

#ifdef THEMIS_HAS_PROMETHEUS
    void setPrometheusRegistry(std::shared_ptr<prometheus::Registry> registry) {
        service_.setPrometheusRegistry(std::move(registry));
    }
#endif

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

        grpc::Status resolveActiveTransaction(
            const std::string& transaction_id,
            std::shared_ptr<TransactionManager::Transaction>* out_txn
        ) {
            if (out_txn) {
                *out_txn = nullptr;
            }
            if (transaction_id.empty()) {
                return grpc::Status::OK;
            }
            if (!txn_mgr_) {
                return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION,
                                    "transaction_id provided but TransactionManager is not wired");
            }

            uint64_t parsed_id = 0;
            try {
                parsed_id = std::stoull(transaction_id);
            } catch (const std::exception&) {
                return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                                    "transaction_id must be an unsigned integer string");
            }

            auto txn = txn_mgr_->getTransaction(parsed_id);
            if (!txn || txn->isFinished()) {
                return grpc::Status(grpc::StatusCode::NOT_FOUND,
                                    "transaction_id does not reference an active transaction");
            }

            if (out_txn) {
                *out_txn = std::move(txn);
            }
            return grpc::Status::OK;
        }

        template<typename Fn>
        grpc::Status withRpcMetrics(const char* method, Fn&& fn) {
            const grpc::Status status = fn();
            recordRpcStatus(method, status.error_code());
            return status;
        }

#ifdef THEMIS_HAS_PROMETHEUS
        void setPrometheusRegistry(std::shared_ptr<prometheus::Registry> registry) {
            std::lock_guard<std::mutex> lock(grpc_metrics_mutex_);
            prom_registry_ = std::move(registry);
            grpc_requests_family_ = nullptr;
            grpc_request_counters_.clear();

            if (!prom_registry_) {
                return;
            }

            try {
                auto& family = prometheus::BuildCounter()
                                   .Name("grpc_requests_total")
                                   .Help("Total number of gRPC requests by method and transport status.")
                                   .Register(*prom_registry_);
                grpc_requests_family_ = &family;
            } catch (const std::exception& e) {
                THEMIS_WARN("ThemisDBGrpcService: failed to register grpc_requests_total: {}",
                            e.what());
                prom_registry_.reset();
                grpc_requests_family_ = nullptr;
                grpc_request_counters_.clear();
            } catch (...) {
                THEMIS_WARN("ThemisDBGrpcService: failed to register grpc_requests_total");
                prom_registry_.reset();
                grpc_requests_family_ = nullptr;
                grpc_request_counters_.clear();
            }
        }
#endif

        void recordRpcStatus(const std::string& method, grpc::StatusCode code) {
#ifdef THEMIS_HAS_PROMETHEUS
            std::lock_guard<std::mutex> lock(grpc_metrics_mutex_);
            if (!grpc_requests_family_) {
                return;
            }

            const char* status_label = grpcStatusCodeLabel(code);
            const std::string key = method + "|" + status_label;

            auto it = grpc_request_counters_.find(key);
            if (it == grpc_request_counters_.end()) {
                auto& counter = grpc_requests_family_->Add(
                    {{"method", method}, {"status", status_label}});
                it = grpc_request_counters_.emplace(key, &counter).first;
            }
            it->second->Increment(1.0);
#else
            (void)method;
            (void)code;
#endif
        }

        bool tryResolveDocumentBody(std::string_view collection,
                                    std::string_view key,
                                    std::string* out_body) const {
            if (!out_body || !db_) {
                return false;
            }

            const std::string direct_key = std::string(collection) + "/" + std::string(key);
            if (db_->get(direct_key, *out_body)) {
                return true;
            }

            std::string entity_blob;
            const std::string entity_key = "entity:" + std::string(collection) + ":" + std::string(key);
            if (!db_->get(entity_key, entity_blob)) {
                return false;
            }

            const std::vector<uint8_t> blob(entity_blob.begin(), entity_blob.end());
            *out_body = BaseEntity::deserialize(key, blob).toJson();
            return true;
        }

        // ── Document CRUD ──────────────────────────────────────────────────

        grpc::Status CreateDocument(
            grpc::ServerContext*           /*ctx*/,
            const CreateDocumentRequest*   req,
            CreateDocumentResponse*        resp
        ) override {
            return withRpcMetrics("CreateDocument", [&]() -> grpc::Status {
            std::shared_ptr<TransactionManager::Transaction> active_txn;
            const auto txn_status = resolveActiveTransaction(req->transaction_id(), &active_txn);
            if (!txn_status.ok()) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(static_cast<int>(txn_status.error_code()));
                err->set_message(txn_status.error_message());
                return txn_status;
            }

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

            if (active_txn) {
                try {
                    auto entity = BaseEntity::fromJson(doc.key(), body);
                    const auto st = active_txn->putEntity(doc.collection(), entity);
                    if (!st.ok) {
                        resp->set_success(false);
                        auto* err = resp->mutable_error();
                        err->set_code(static_cast<int>(grpc::StatusCode::FAILED_PRECONDITION));
                        err->set_message(st.message);
                        return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, st.message);
                    }
                } catch (const std::exception& e) {
                    resp->set_success(false);
                    auto* err = resp->mutable_error();
                    err->set_code(static_cast<int>(grpc::StatusCode::INVALID_ARGUMENT));
                    err->set_message(e.what());
                    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, e.what());
                }

                resp->set_success(true);
                resp->set_key(doc.key());
                resp->set_version(0);
                return grpc::Status::OK;
            }

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
            });
        }

        grpc::Status GetDocument(
            grpc::ServerContext*        /*ctx*/,
            const GetDocumentRequest*   req,
            GetDocumentResponse*        resp
        ) override {
            return withRpcMetrics("GetDocument", [&]() -> grpc::Status {
            std::shared_ptr<TransactionManager::Transaction> active_txn;
            const auto txn_status = resolveActiveTransaction(req->transaction_id(), &active_txn);
            if (!txn_status.ok()) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(static_cast<int>(txn_status.error_code()));
                err->set_message(txn_status.error_message());
                return txn_status;
            }

            if (req->collection().empty() || req->key().empty()) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(400);
                err->set_message("collection and key are required");
                return grpc::Status::OK;
            }

            std::string body;

            if (active_txn) {
                auto tx_body = active_txn->readEntityJson(req->collection(), req->key());
                if (!tx_body) {
                    resp->set_success(false);
                    auto* err = resp->mutable_error();
                    err->set_code(404);
                    err->set_message("document not found");
                    return grpc::Status::OK;
                }
                body = std::move(*tx_body);
            } else {
                if (!tryResolveDocumentBody(req->collection(), req->key(), &body)) {
                    resp->set_success(false);
                    auto* err = resp->mutable_error();
                    err->set_code(404);
                    err->set_message("document not found");
                    return grpc::Status::OK;
                }
            }

            resp->set_success(true);
            auto* doc = resp->mutable_document();
            doc->set_collection(req->collection());
            doc->set_key(req->key());
            doc->set_body(body.data(), body.size());
            return grpc::Status::OK;
            });
        }

        grpc::Status UpdateDocument(
            grpc::ServerContext*           /*ctx*/,
            const UpdateDocumentRequest*   req,
            UpdateDocumentResponse*        resp
        ) override {
            return withRpcMetrics("UpdateDocument", [&]() -> grpc::Status {
            std::shared_ptr<TransactionManager::Transaction> active_txn;
            const auto txn_status = resolveActiveTransaction(req->transaction_id(), &active_txn);
            if (!txn_status.ok()) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(static_cast<int>(txn_status.error_code()));
                err->set_message(txn_status.error_message());
                return txn_status;
            }

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

            if (active_txn) {
                try {
                    auto entity = BaseEntity::fromJson(doc.key(), body);
                    const auto st = active_txn->putEntity(doc.collection(), entity);
                    if (!st.ok) {
                        resp->set_success(false);
                        auto* err = resp->mutable_error();
                        err->set_code(static_cast<int>(grpc::StatusCode::FAILED_PRECONDITION));
                        err->set_message(st.message);
                        return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, st.message);
                    }
                } catch (const std::exception& e) {
                    resp->set_success(false);
                    auto* err = resp->mutable_error();
                    err->set_code(static_cast<int>(grpc::StatusCode::INVALID_ARGUMENT));
                    err->set_message(e.what());
                    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, e.what());
                }

                resp->set_success(true);
                resp->set_key(doc.key());
                resp->set_version(0);
                return grpc::Status::OK;
            }

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
                    catch (const std::invalid_argument&) {
                        THEMIS_WARN("UpdateDocument: malformed version counter '{}' for key '{}'; "
                                    "resetting to 1", ver_str, storage_key);
                        new_version = 1;
                    }
                    catch (const std::out_of_range&) {
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
            });
        }

        grpc::Status DeleteDocument(
            grpc::ServerContext*           /*ctx*/,
            const DeleteDocumentRequest*   req,
            DeleteDocumentResponse*        resp
        ) override {
            return withRpcMetrics("DeleteDocument", [&]() -> grpc::Status {
            std::shared_ptr<TransactionManager::Transaction> active_txn;
            const auto txn_status = resolveActiveTransaction(req->transaction_id(), &active_txn);
            if (!txn_status.ok()) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(static_cast<int>(txn_status.error_code()));
                err->set_message(txn_status.error_message());
                return txn_status;
            }

            if (req->collection().empty() || req->key().empty()) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(400);
                err->set_message("collection and key are required");
                return grpc::Status::OK;
            }

            const std::string storage_key = req->collection() + "/" + req->key();

            if (active_txn) {
                const auto st = active_txn->eraseEntity(req->collection(), req->key());
                if (!st.ok) {
                    resp->set_success(false);
                    auto* err = resp->mutable_error();
                    err->set_code(static_cast<int>(grpc::StatusCode::FAILED_PRECONDITION));
                    err->set_message(st.message);
                    return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, st.message);
                }
                resp->set_success(true);
                return grpc::Status::OK;
            }

            if (!db_ || !db_->del(storage_key)) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(404);
                err->set_message("document not found or delete failed");
                return grpc::Status::OK;
            }

            resp->set_success(true);
            return grpc::Status::OK;
            });
        }

        // ── Batch operations ───────────────────────────────────────────────

        grpc::Status BatchWrite(
            grpc::ServerContext*       /*ctx*/,
            const BatchWriteRequest*   req,
            BatchWriteResponse*        resp
        ) override {
            return withRpcMetrics("BatchWrite", [&]() -> grpc::Status {
            std::shared_ptr<TransactionManager::Transaction> active_txn;
            const auto txn_status = resolveActiveTransaction(req->transaction_id(), &active_txn);
            if (!txn_status.ok()) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(static_cast<int>(txn_status.error_code()));
                err->set_message(txn_status.error_message());
                return txn_status;
            }

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
                if (active_txn) {
                    try {
                        auto entity = BaseEntity::fromJson(doc.key(), body);
                        auto st = active_txn->putEntity(doc.collection(), entity);
                        if (st.ok) {
                            ++upserted;
                        } else {
                            all_ok = false;
                        }
                    } catch (const std::exception&) {
                        all_ok = false;
                    }
                } else {
                    if (db_ && db_->put(key, body)) {
                        ++upserted;
                    } else {
                        all_ok = false;
                    }
                }
            }

            for (const auto& del_key : req->deletes()) {
                if (active_txn) {
                    // Batch delete keys use the canonical "collection/key" form.
                    const auto sep = del_key.find('/');
                    if (sep == std::string::npos || sep == 0 || sep + 1 >= del_key.size()) {
                        all_ok = false;
                        continue;
                    }
                    const std::string collection = del_key.substr(0, sep);
                    const std::string key = del_key.substr(sep + 1);
                    auto st = active_txn->eraseEntity(collection, key);
                    if (st.ok) {
                        ++deleted;
                    } else {
                        all_ok = false;
                    }
                } else {
                    if (db_ && db_->del(del_key)) {
                        ++deleted;
                    } else {
                        all_ok = false;
                    }
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
            });
        }

        grpc::Status BatchRead(
            grpc::ServerContext*      /*ctx*/,
            const BatchReadRequest*   req,
            BatchReadResponse*        resp
        ) override {
            return withRpcMetrics("BatchRead", [&]() -> grpc::Status {
            std::shared_ptr<TransactionManager::Transaction> active_txn;
            const auto txn_status = resolveActiveTransaction(req->transaction_id(), &active_txn);
            if (!txn_status.ok()) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(static_cast<int>(txn_status.error_code()));
                err->set_message(txn_status.error_message());
                return txn_status;
            }

            // Reject oversized key lists before allocating any per-item resources.
            if (req->keys_size() > kMaxBatchItems) {
                return grpc::Status(
                    grpc::StatusCode::RESOURCE_EXHAUSTED,
                    "batch exceeds maximum of " + std::to_string(kMaxBatchItems) + " keys; "
                    "split the request into smaller batches");
            }

            for (const auto& key : req->keys()) {
                std::string body;

                bool found = false;
                if (active_txn) {
                    auto tx_body = active_txn->readEntityJson(req->collection(), key);
                    if (tx_body) {
                        body = std::move(*tx_body);
                        found = true;
                    }
                } else if (tryResolveDocumentBody(req->collection(), key, &body)) {
                    found = true;
                }

                if (found) {
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
            });
        }

        // ── AQL ────────────────────────────────────────────────────────────

        grpc::Status ExecuteAQL(
            grpc::ServerContext*       /*ctx*/,
            const AQLQueryRequest*     req,
            AQLQueryResponse*          resp
        ) override {
            return withRpcMetrics("ExecuteAQL", [&]() -> grpc::Status {
            const auto txn_status = resolveActiveTransaction(req->transaction_id(), nullptr);
            if (!txn_status.ok()) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(static_cast<int>(txn_status.error_code()));
                err->set_message(txn_status.error_message());
                return txn_status;
            }

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
                const auto status = mapThemisErrorToGrpcStatus(result.error());
                err->set_code(static_cast<int>(status.error_code()));
                err->set_message(status.error_message());
                return status;
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
            } catch (const nlohmann::json::exception&) {
                // Fall back to raw payload when response is not valid JSON.
                auto* row = resp->add_rows();
                row->set_data(*result);
                row->set_has_more(false);
            } catch (const std::string&) {
                // Fall back to raw payload when response is not valid JSON.
                auto* row = resp->add_rows();
                row->set_data(*result);
                row->set_has_more(false);
            } catch (const char*) {
                // Fall back to raw payload when response is not valid JSON.
                auto* row = resp->add_rows();
                row->set_data(*result);
                row->set_has_more(false);
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
            });
        }

        // ── StreamAQL (server-side streaming) ─────────────────────────────

        grpc::Status StreamAQL(
            grpc::ServerContext*                  ctx,
            const AQLQueryRequest*                req,
            grpc::ServerWriter<AQLRow>*           writer
        ) override {
            return withRpcMetrics("StreamAQL", [&]() -> grpc::Status {
            const auto txn_status = resolveActiveTransaction(req->transaction_id(), nullptr);
            if (!txn_status.ok()) {
                return txn_status;
            }

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
                return mapThemisErrorToGrpcStatus(result.error());
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
            });
        }

        // ── Vector search ──────────────────────────────────────────────────

        grpc::Status VectorSearch(
            grpc::ServerContext*           /*ctx*/,
            const VectorSearchRequest*     req,
            VectorSearchResponse*          resp
        ) override {
            return withRpcMetrics("VectorSearch", [&]() -> grpc::Status {
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

            if (req->fetch_docs() && !db_) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(501);
                err->set_message("fetch_docs requires storage backend wiring");
                return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION,
                                    "storage backend not wired into this service instance");
            }

            const auto hits = vector_index_->search(embedding, k);
            resp->set_success(true);
            for (const auto& hit : hits) {
                auto* h = resp->add_hits();
                h->set_collection(req->collection());
                h->set_key(hit.primary_key);
                h->set_score(hit.distance);
                if (req->fetch_docs()) {
                    std::string body;
                    if (tryResolveDocumentBody(req->collection(), hit.primary_key, &body)) {
                        h->set_document(body.data(), body.size());
                    }
                }
            }
            return grpc::Status::OK;
            });
        }

        grpc::Status FilteredVectorSearch(
            grpc::ServerContext*                  /*ctx*/,
            const FilteredVectorSearchRequest*    req,
            VectorSearchResponse*                 resp
        ) override {
            return withRpcMetrics("FilteredVectorSearch", [&]() -> grpc::Status {
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

            if (req->fetch_docs() && !db_) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(501);
                err->set_message("fetch_docs requires storage backend wiring");
                return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION,
                                    "storage backend not wired into this service instance");
            }

            struct KeyFilterClause {
                enum class Type { Eq, Ne, In, Gt, Gte, Lt, Lte };
                enum class Field { Key, Id, Attribute };
                Field field{Field::Key};
                Type type;
                std::string attribute_name;
                std::string scalar;
                std::vector<std::string> set;
                bool has_numeric_scalar{false};
                double numeric_scalar{0.0};
                bool has_numeric_set{false};
                std::vector<double> numeric_set;
            };

            std::vector<KeyFilterClause> key_filters;
            bool has_unsupported_filters = false;

            const auto parseDoubleStrict = [](const std::string& raw, double* out) -> bool {
                if (!out) {
                    return false;
                }
                try {
                    size_t consumed = 0;
                    const double value = std::stod(raw, &consumed);
                    if (consumed != raw.size()) {
                        return false;
                    }
                    *out = value;
                    return true;
                } catch (const std::exception&) {
                    return false;
                }
            };

            const auto nearlyEqual = [](double lhs, double rhs) -> bool {
                constexpr double kRelEpsilon = 1e-9;
                const double diff = std::fabs(lhs - rhs);
                const double scale = std::max(1.0, std::max(std::fabs(lhs), std::fabs(rhs)));
                return diff <= (kRelEpsilon * scale);
            };

            for (const auto& filter : req->filters()) {
                KeyFilterClause::Field field = KeyFilterClause::Field::Key;
                if (filter.field() == "_key") {
                    field = KeyFilterClause::Field::Key;
                } else if (filter.field() == "_id") {
                    field = KeyFilterClause::Field::Id;
                } else {
                    field = KeyFilterClause::Field::Attribute;
                    if (!db_) {
                        has_unsupported_filters = true;
                        continue;
                    }
                }

                std::string op = filter.operator_();
                std::transform(op.begin(), op.end(), op.begin(),
                               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

                if (op == "eq" || op == "ne") {
                    std::string scalar = filter.value();
                    bool has_numeric_scalar = false;
                    double numeric_scalar = 0.0;
#if THEMIS_HAS_JSON
                    try {
                        const auto parsed = nlohmann::json::parse(filter.value());
                        if (parsed.is_string()) {
                            scalar = parsed.get<std::string>();
                            has_numeric_scalar = parseDoubleStrict(scalar, &numeric_scalar);
                        } else if (parsed.is_number()) {
                            has_numeric_scalar = true;
                            numeric_scalar = parsed.get<double>();
                        }
                    } catch (const nlohmann::json::exception&) {
                        // Keep raw scalar when filter value is not JSON-encoded.
                        has_numeric_scalar = parseDoubleStrict(scalar, &numeric_scalar);
                    }
#else
                    has_numeric_scalar = parseDoubleStrict(scalar, &numeric_scalar);
#endif
                    KeyFilterClause clause;
                    clause.field = field;
                    clause.type = op == "eq" ? KeyFilterClause::Type::Eq : KeyFilterClause::Type::Ne;
                    clause.attribute_name = field == KeyFilterClause::Field::Attribute ? filter.field() : "";
                    clause.scalar = std::move(scalar);
                    clause.has_numeric_scalar = has_numeric_scalar;
                    clause.numeric_scalar = numeric_scalar;
                    key_filters.push_back(std::move(clause));
                    continue;
                }

                if (op == "in") {
#if THEMIS_HAS_JSON
                    try {
                        const auto parsed = nlohmann::json::parse(filter.value());
                        if (!parsed.is_array()) {
                            has_unsupported_filters = true;
                            continue;
                        }

                        KeyFilterClause clause{field,
                                               KeyFilterClause::Type::In,
                                               field == KeyFilterClause::Field::Attribute ? filter.field() : "",
                                               {},
                                               {}};
                        bool saw_string = false;
                        bool saw_number = false;
                        bool saw_unsupported_item = false;
                        for (const auto& item : parsed) {
                            if (item.is_string()) {
                                saw_string = true;
                                clause.set.push_back(item.get<std::string>());
                            } else if (field == KeyFilterClause::Field::Attribute && item.is_number()) {
                                saw_number = true;
                                clause.has_numeric_set = true;
                                clause.numeric_set.push_back(item.get<double>());
                            } else {
                                saw_unsupported_item = true;
                            }
                        }

                        const bool mixed_scalar_types = saw_string && saw_number;
                        if (mixed_scalar_types || saw_unsupported_item ||
                            (clause.set.empty() && clause.numeric_set.empty())) {
                            has_unsupported_filters = true;
                            continue;
                        }

                        if (!clause.numeric_set.empty() && !clause.set.empty()) {
                            has_unsupported_filters = true;
                            continue;
                        }

                        key_filters.push_back(std::move(clause));
                    } catch (const nlohmann::json::exception&) {
                        has_unsupported_filters = true;
                    }
#else
                    has_unsupported_filters = true;
#endif
                    continue;
                }

                if (op == "gt" || op == "gte" || op == "lt" || op == "lte") {
                    if (field != KeyFilterClause::Field::Attribute) {
                        has_unsupported_filters = true;
                        continue;
                    }

                    double numeric_scalar = 0.0;
                    bool parsed_number = false;
#if THEMIS_HAS_JSON
                    try {
                        const auto parsed = nlohmann::json::parse(filter.value());
                        if (parsed.is_number()) {
                            numeric_scalar = parsed.get<double>();
                            parsed_number = true;
                        } else if (parsed.is_string()) {
                            parsed_number = parseDoubleStrict(parsed.get<std::string>(), &numeric_scalar);
                        }
                    } catch (const nlohmann::json::exception&) {
                        parsed_number = parseDoubleStrict(filter.value(), &numeric_scalar);
                    }
#else
                    parsed_number = parseDoubleStrict(filter.value(), &numeric_scalar);
#endif

                    if (!parsed_number) {
                        has_unsupported_filters = true;
                        continue;
                    }

                    KeyFilterClause clause;
                    clause.field = field;
                    clause.attribute_name = filter.field();
                    clause.has_numeric_scalar = true;
                    clause.numeric_scalar = numeric_scalar;
                    if (op == "gt") {
                        clause.type = KeyFilterClause::Type::Gt;
                    } else if (op == "gte") {
                        clause.type = KeyFilterClause::Type::Gte;
                    } else if (op == "lt") {
                        clause.type = KeyFilterClause::Type::Lt;
                    } else {
                        clause.type = KeyFilterClause::Type::Lte;
                    }
                    key_filters.push_back(std::move(clause));
                    continue;
                }

                has_unsupported_filters = true;
            }

            if (has_unsupported_filters) {
                THEMIS_WARN("ThemisDBGrpcService: FilteredVectorSearch – "
                            "only supported filters (_key/_id and top-level attributes with eq/ne/in/gt/gte/lt/lte) are applied; "
                            "other filters are ignored");
            }

#if THEMIS_HAS_JSON
            class JsonClauseFilterEvaluator final : public themis::IExpressionEvaluator {
            public:
                JsonClauseFilterEvaluator(std::vector<KeyFilterClause> clauses,
                                          std::string collection)
                    : clauses_(std::move(clauses)),
                      collection_(std::move(collection)) {}

                bool evaluate(const std::string& /*expression*/, const void* context) const override {
                    if (!context) {
                        return false;
                    }

                    const auto* doc = static_cast<const nlohmann::json*>(context);
                    if (!doc || !doc->is_object()) {
                        return false;
                    }

                    const std::string key = doc->value("_key", std::string{});
                    const std::string id = collection_ + "/" + key;

                    for (const auto& clause : clauses_) {
                        std::string candidate;
                        bool has_numeric_candidate = false;
                        double numeric_candidate = 0.0;

                        if (clause.field == KeyFilterClause::Field::Key) {
                            candidate = key;
                        } else if (clause.field == KeyFilterClause::Field::Id) {
                            candidate = id;
                        } else {
                            if (!doc->contains(clause.attribute_name)) {
                                return false;
                            }
                            const auto& field_value = (*doc)[clause.attribute_name];
                            if (field_value.is_string()) {
                                candidate = field_value.get<std::string>();
                                if (clause.type == KeyFilterClause::Type::Eq ||
                                    clause.type == KeyFilterClause::Type::Ne) {
                                    has_numeric_candidate =
                                        parseDoubleStrict(candidate, &numeric_candidate);
                                }
                            } else if (field_value.is_number()) {
                                has_numeric_candidate = true;
                                numeric_candidate = field_value.get<double>();
                            } else {
                                return false;
                            }
                        }

                        switch (clause.type) {
                            case KeyFilterClause::Type::Eq:
                                if (clause.has_numeric_scalar) {
                                    if (!has_numeric_candidate ||
                                        !nearlyEqual(numeric_candidate, clause.numeric_scalar)) {
                                        return false;
                                    }
                                } else if (candidate != clause.scalar) {
                                    return false;
                                }
                                break;
                            case KeyFilterClause::Type::Ne:
                                if (clause.has_numeric_scalar) {
                                    if (!has_numeric_candidate ||
                                        nearlyEqual(numeric_candidate, clause.numeric_scalar)) {
                                        return false;
                                    }
                                } else if (candidate == clause.scalar) {
                                    return false;
                                }
                                break;
                            case KeyFilterClause::Type::In:
                                if (clause.has_numeric_set) {
                                    const bool matches_numeric_set = has_numeric_candidate &&
                                        std::any_of(clause.numeric_set.begin(), clause.numeric_set.end(),
                                                    [&](double item) {
                                                        return nearlyEqual(numeric_candidate, item);
                                                    });
                                    if (!matches_numeric_set) {
                                        return false;
                                    }
                                } else if (std::find(clause.set.begin(), clause.set.end(), candidate) ==
                                           clause.set.end()) {
                                    return false;
                                }
                                break;
                            case KeyFilterClause::Type::Gt:
                                if (!has_numeric_candidate || !clause.has_numeric_scalar ||
                                    !(numeric_candidate > clause.numeric_scalar)) {
                                    return false;
                                }
                                break;
                            case KeyFilterClause::Type::Gte:
                                if (!has_numeric_candidate || !clause.has_numeric_scalar ||
                                    !(numeric_candidate >= clause.numeric_scalar)) {
                                    return false;
                                }
                                break;
                            case KeyFilterClause::Type::Lt:
                                if (!has_numeric_candidate || !clause.has_numeric_scalar ||
                                    !(numeric_candidate < clause.numeric_scalar)) {
                                    return false;
                                }
                                break;
                            case KeyFilterClause::Type::Lte:
                                if (!has_numeric_candidate || !clause.has_numeric_scalar ||
                                    !(numeric_candidate <= clause.numeric_scalar)) {
                                    return false;
                                }
                                break;
                        }
                    }

                    return true;
                }

                std::string get_expression_type() const override {
                    return "themis_json_context_v1";
                }

            private:
                static bool parseDoubleStrict(const std::string& raw, double* out) {
                    if (!out) {
                        return false;
                    }
                    try {
                        size_t consumed = 0;
                        const double value = std::stod(raw, &consumed);
                        if (consumed != raw.size()) {
                            return false;
                        }
                        *out = value;
                        return true;
                    } catch (const std::exception&) {
                        return false;
                    }
                }

                static bool nearlyEqual(double lhs, double rhs) {
                    constexpr double kRelEpsilon = 1e-9;
                    const double diff = std::fabs(lhs - rhs);
                    const double scale = std::max(1.0, std::max(std::fabs(lhs), std::fabs(rhs)));
                    return diff <= (kRelEpsilon * scale);
                }

                std::vector<KeyFilterClause> clauses_;
                std::string collection_;
            };

            std::unique_ptr<themis::IExpressionEvaluator> vector_filter_evaluator;
            if (!key_filters.empty()) {
                vector_filter_evaluator =
                    std::make_unique<JsonClauseFilterEvaluator>(key_filters, req->collection());
            }
            const auto hits = vector_index_->search(embedding, k, vector_filter_evaluator.get());
#else
            const auto hits = vector_index_->search(embedding, k, nullptr);
#endif
            resp->set_success(true);
            for (const auto& hit : hits) {
                bool include_hit = true;
                std::string doc_body;
                bool has_doc_body = false;
#if THEMIS_HAS_JSON
                nlohmann::json doc_json;
                bool has_doc_json = false;
#endif
                for (const auto& clause : key_filters) {
                    std::string candidate;
                    bool has_numeric_candidate = false;
                    double numeric_candidate = 0.0;
                    if (clause.field == KeyFilterClause::Field::Key) {
                        candidate = hit.primary_key;
                    } else if (clause.field == KeyFilterClause::Field::Id) {
                        candidate = req->collection() + "/" + hit.primary_key;
                    } else {
                        if (!has_doc_body) {
                            has_doc_body = tryResolveDocumentBody(req->collection(), hit.primary_key, &doc_body);
                        }
                        if (!has_doc_body) {
                            include_hit = false;
                            break;
                        }
#if THEMIS_HAS_JSON
                        if (!has_doc_json) {
                            try {
                                doc_json = nlohmann::json::parse(doc_body);
                                has_doc_json = true;
                            } catch (const nlohmann::json::exception&) {
                                include_hit = false;
                                break;
                            }
                        }

                        if (!doc_json.is_object() || !doc_json.contains(clause.attribute_name)) {
                            include_hit = false;
                            break;
                        }

                        const auto& field_value = doc_json[clause.attribute_name];
                        if (field_value.is_string()) {
                            candidate = field_value.get<std::string>();
                            if (clause.type == KeyFilterClause::Type::Eq || clause.type == KeyFilterClause::Type::Ne) {
                                has_numeric_candidate = parseDoubleStrict(candidate, &numeric_candidate);
                            }
                        } else if (field_value.is_number()) {
                            has_numeric_candidate = true;
                            numeric_candidate = field_value.get<double>();
                        } else {
                            include_hit = false;
                            break;
                        }
#else
                        include_hit = false;
                        break;
#endif
                    }

                    switch (clause.type) {
                        case KeyFilterClause::Type::Eq:
                            if (clause.has_numeric_scalar) {
                                if (!has_numeric_candidate ||
                                    !nearlyEqual(numeric_candidate, clause.numeric_scalar)) {
                                    include_hit = false;
                                }
                            } else if (candidate != clause.scalar) {
                                include_hit = false;
                            }
                            break;
                        case KeyFilterClause::Type::Ne:
                            if (clause.has_numeric_scalar) {
                                if (!has_numeric_candidate ||
                                    nearlyEqual(numeric_candidate, clause.numeric_scalar)) {
                                    include_hit = false;
                                }
                            } else if (candidate == clause.scalar) {
                                include_hit = false;
                            }
                            break;
                        case KeyFilterClause::Type::In:
                            if (clause.has_numeric_set) {
                                const bool matches_numeric_set = has_numeric_candidate &&
                                    std::any_of(clause.numeric_set.begin(), clause.numeric_set.end(),
                                                [&](double item) {
                                                    return nearlyEqual(numeric_candidate, item);
                                                });
                                if (!matches_numeric_set) {
                                    include_hit = false;
                                }
                            } else if (std::find(clause.set.begin(), clause.set.end(),
                                                 candidate) == clause.set.end()) {
                                include_hit = false;
                            }
                            break;
                        case KeyFilterClause::Type::Gt:
                            if (!has_numeric_candidate || !clause.has_numeric_scalar ||
                                !(numeric_candidate > clause.numeric_scalar)) {
                                include_hit = false;
                            }
                            break;
                        case KeyFilterClause::Type::Gte:
                            if (!has_numeric_candidate || !clause.has_numeric_scalar ||
                                !(numeric_candidate >= clause.numeric_scalar)) {
                                include_hit = false;
                            }
                            break;
                        case KeyFilterClause::Type::Lt:
                            if (!has_numeric_candidate || !clause.has_numeric_scalar ||
                                !(numeric_candidate < clause.numeric_scalar)) {
                                include_hit = false;
                            }
                            break;
                        case KeyFilterClause::Type::Lte:
                            if (!has_numeric_candidate || !clause.has_numeric_scalar ||
                                !(numeric_candidate <= clause.numeric_scalar)) {
                                include_hit = false;
                            }
                            break;
                    }
                    if (!include_hit) {
                        break;
                    }
                }

                if (!include_hit) {
                    continue;
                }

                auto* h = resp->add_hits();
                h->set_collection(req->collection());
                h->set_key(hit.primary_key);
                h->set_score(hit.distance);
                if (req->fetch_docs()) {
                    std::string body;
                    if (tryResolveDocumentBody(req->collection(), hit.primary_key, &body)) {
                        h->set_document(body.data(), body.size());
                    }
                }
            }
            return grpc::Status::OK;
            });
        }

        grpc::Status HybridSearch(
            grpc::ServerContext*       /*ctx*/,
            const HybridSearchRequest* req,
            HybridSearchResponse*      resp
        ) override {
            return withRpcMetrics("HybridSearch", [&]() -> grpc::Status {
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

            if (req->fetch_docs() && !db_) {
                resp->set_success(false);
                auto* err = resp->mutable_error();
                err->set_code(501);
                err->set_message("fetch_docs requires storage backend wiring");
                return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION,
                                    "storage backend not wired into this service instance");
            }

            auto maybeAttachDoc = [&](SearchHit* hit) {
                if (!req->fetch_docs() || !hit || !db_) {
                    return;
                }
                std::string body;
                if (tryResolveDocumentBody(req->collection(), hit->key(), &body)) {
                    hit->set_document(body.data(), body.size());
                }
            };

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
                    maybeAttachDoc(h);
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
                                maybeAttachDoc(h);
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
            });
        }

        grpc::Status FullTextSearch(
            grpc::ServerContext*           /*ctx*/,
            const FullTextSearchRequest*   req,
            FullTextSearchResponse*        resp
        ) override {
            return withRpcMetrics("FullTextSearch", [&]() -> grpc::Status {
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
                const auto status = mapThemisErrorToGrpcStatus(result.error());
                err->set_code(static_cast<int>(status.error_code()));
                err->set_message(status.error_message());
                return status;
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
            });
        }

        // ── Health ─────────────────────────────────────────────────────────

        grpc::Status HealthCheck(
            grpc::ServerContext*         /*ctx*/,
            const HealthCheckRequest*    req,
            HealthCheckResponse*         resp
        ) override {
            return withRpcMetrics("HealthCheck", [&]() -> grpc::Status {
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
            });
        }

    private:
        std::shared_ptr<RocksDBWrapper>              db_;
        std::shared_ptr<TransactionManager>          txn_mgr_;
        std::shared_ptr<themis::IQueryEngine>  aql_engine_;
        std::shared_ptr<themis::IVectorIndex>  vector_index_;
        std::chrono::steady_clock::time_point        start_time_;

#ifdef THEMIS_HAS_PROMETHEUS
        std::shared_ptr<prometheus::Registry>        prom_registry_;
        prometheus::Family<prometheus::Counter>*     grpc_requests_family_{nullptr};
        std::unordered_map<std::string, prometheus::Counter*> grpc_request_counters_;
        std::mutex                                   grpc_metrics_mutex_;
#endif
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
        } catch (const std::string&) {
            THEMIS_ERROR("ThemisDBGrpcService: service callback failed: unknown error");
            service_ptr_ = nullptr;
        } catch (const char*) {
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

#ifdef THEMIS_HAS_PROMETHEUS
void ThemisDBGrpcService::setPrometheusRegistry(std::shared_ptr<prometheus::Registry> registry) {
#if THEMIS_HAS_API_GRPC
    if (impl_) {
        impl_->setPrometheusRegistry(std::move(registry));
    }
#else
    (void)registry;
#endif
}
#endif

void* ThemisDBGrpcService::service() {
#if THEMIS_HAS_API_GRPC
    return impl_ ? static_cast<void*>(impl_->get()) : nullptr;
#else
    return service_ptr_;
#endif
}

} // namespace api
} // namespace themis


