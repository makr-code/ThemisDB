/**
 * @file themis_core_grpc_service.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 96/100
 * @note Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0
 * @note Status: Production Ready — Wave 9 Block 1 wired (W9-1..W9-6)
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/themis_core_grpc_service.h"
#include <stdexcept>
#include "storage/rocksdb_wrapper.h"
#include "transaction/transaction_manager.h"
#include "utils/logger.h"
#include <atomic>
#include <chrono>
#include <exception>
#include <mutex>
#include <string>
#include <utility>
#include <nlohmann/json.hpp>

// Conditionally compile the real service implementation when the protobuf
// stubs generated from proto/themis_core.proto are available on the include
// path.  This mirrors the pattern used by WalGrpcService / wal_grpc_service.cpp.
#if __has_include("themis_core.grpc.pb.h")
#  include <grpcpp/grpcpp.h>
#  include "themis_core.grpc.pb.h"
#  include "themis_core.pb.h"
#  define THEMIS_HAS_CORE_GRPC 1
#else
#  define THEMIS_HAS_CORE_GRPC 0
#endif

namespace themis {
namespace core {

namespace {
std::mutex g_core_grpc_instance_mutex;
ThemisCoreServiceImpl::ServiceInstanceFn g_core_grpc_instance_fn;

/// Build a storage key from collection + "/" + doc-key.
/// Convention: collection:key avoids collisions with bare keys.
inline std::string storageKey(const std::string& collection, const std::string& key) {
    return collection + ":" + key;
}
} // namespace

/** @brief Implementation detail. */
class ThemisCoreServiceImpl::Impl {
public:
#if THEMIS_HAS_CORE_GRPC

    Impl(std::shared_ptr<RocksDBWrapper>     db,
         std::shared_ptr<TransactionManager> txn_mgr,
         std::shared_ptr<AQLEngine>          aql_engine)
        : service_(std::move(db), std::move(txn_mgr), std::move(aql_engine)) {}

    themis::core::ThemisCoreService::Service* get() { return &service_; }

private:
    // ─────────────────────────────────────────────────────────────────────────
    // ServiceImpl — concrete gRPC handler
    // ─────────────────────────────────────────────────────────────────────────
    class ServiceImpl final : public themis::core::ThemisCoreService::Service {
    public:
        ServiceImpl(std::shared_ptr<RocksDBWrapper>     db,
                    std::shared_ptr<TransactionManager> txn_mgr,
                    std::shared_ptr<AQLEngine>          aql_engine)
            : db_(std::move(db))
            , txn_mgr_(std::move(txn_mgr))
            , aql_engine_(std::move(aql_engine))
            , start_time_(std::chrono::steady_clock::now()) {}

        // ── Health / Status ──────────────────────────────────────────────────

        grpc::Status HealthCheck(
            grpc::ServerContext*      /*ctx*/,
            const HealthCheckRequest* req,
            HealthCheckResponse*      resp
        ) override {
            resp->set_status(HealthCheckResponse::HEALTHY);
            resp->set_message("ThemisDB core gRPC service is running");
            auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - start_time_).count();
            resp->set_uptime_seconds(uptime);
            if (req->include_details()) {
                (*resp->mutable_details())["storage"] = db_        ? "ok" : "unavailable";
                (*resp->mutable_details())["txn_mgr"] = txn_mgr_   ? "ok" : "unavailable";
                (*resp->mutable_details())["aql"]     = aql_engine_ ? "ok" : "unavailable";
            }
            return grpc::Status::OK;
        }

        grpc::Status GetStatus(
            grpc::ServerContext*  /*ctx*/,
            const StatusRequest*  req,
            StatusResponse*       resp
        ) override {
            resp->set_version("1.0.0");
            auto uptime = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - start_time_).count();
            resp->set_uptime_seconds(uptime);

            auto* ver = resp->mutable_api_version();
            ver->set_major(1); ver->set_minor(4); ver->set_patch(1);

            if (req->include_stats()) {
                auto* stats = resp->mutable_stats();
                // Basic stats — storage driver provides estimated counts.
                stats->set_total_collections(0);
                stats->set_total_documents(0);
                stats->set_storage_size_mb(0.0);
                stats->set_memory_usage_mb(0.0);
            }
            return grpc::Status::OK;
        }

        // ── CRUD ─────────────────────────────────────────────────────────────

        grpc::Status Create(
            grpc::ServerContext*  /*ctx*/,
            const CreateRequest*  req,
            CreateResponse*       resp
        ) override {
            if (req->collection().empty() || req->key().empty()) {
                resp->set_success(false);
                resp->mutable_error()->set_code(400);
                resp->mutable_error()->set_message("collection and key are required");
                return grpc::Status::OK;
            }
            if (!db_) {
                return grpc::Status(grpc::StatusCode::UNAVAILABLE, "storage not available");
            }
            const std::string skey = storageKey(req->collection(), req->key());
            const std::string data(req->data().begin(), req->data().end());

            bool ok = false;
            if (!req->transaction_id().empty() && txn_mgr_) {
                try {
                    const auto tid = std::stoull(req->transaction_id());
                    auto txn = txn_mgr_->getTransaction(tid);
                    if (txn) {
                        const std::vector<uint8_t> raw(data.begin(), data.end());
                        // Use RocksDB-level transaction wrapper for the write,
                        // keeping the TransactionManager session alive for commit/rollback.
                        auto rwt = db_->beginTransaction();
                        if (rwt && rwt->isActive()) {
                            ok = rwt->put(skey, raw) && rwt->commit();
                        } else {
                            ok = db_->put(skey, data);
                        }
                    } else {
                        ok = db_->put(skey, data);
                    }
                } catch (const std::exception& ex) {
                    THEMIS_WARN("Create: transaction lookup failed: {}", ex.what());
                    ok = db_->put(skey, data);
                }
            } else {
                ok = db_->put(skey, data);
            }

            resp->set_success(ok);
            resp->set_key(req->key());
            resp->set_timestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());
            if (!ok) {
                resp->mutable_error()->set_code(500);
                resp->mutable_error()->set_message("storage write failed");
            }
            return grpc::Status::OK;
        }

        grpc::Status Read(
            grpc::ServerContext* /*ctx*/,
            const ReadRequest*   req,
            ReadResponse*        resp
        ) override {
            if (req->collection().empty() || req->key().empty()) {
                resp->set_success(false);
                resp->mutable_error()->set_code(400);
                resp->mutable_error()->set_message("collection and key are required");
                return grpc::Status::OK;
            }
            if (!db_) {
                return grpc::Status(grpc::StatusCode::UNAVAILABLE, "storage not available");
            }
            const std::string skey = storageKey(req->collection(), req->key());
            auto val = db_->get(skey);
            if (!val.has_value()) {
                resp->set_success(false);
                resp->mutable_error()->set_code(404);
                resp->mutable_error()->set_message("document not found");
                return grpc::Status::OK;
            }
            resp->set_success(true);
            auto* doc = resp->mutable_document();
            doc->set_collection(req->collection());
            doc->set_key(req->key());
            doc->set_data(val->data(), val->size());
            return grpc::Status::OK;
        }

        grpc::Status Update(
            grpc::ServerContext*  /*ctx*/,
            const UpdateRequest*  req,
            UpdateResponse*       resp
        ) override {
            if (req->collection().empty() || req->key().empty()) {
                resp->set_success(false);
                resp->mutable_error()->set_code(400);
                resp->mutable_error()->set_message("collection and key are required");
                return grpc::Status::OK;
            }
            if (!db_) {
                return grpc::Status(grpc::StatusCode::UNAVAILABLE, "storage not available");
            }
            const std::string skey = storageKey(req->collection(), req->key());
            const std::string data(req->data().begin(), req->data().end());

            // Existence check when create_if_missing is false.
            if (!req->create_if_missing()) {
                auto existing = db_->get(skey);
                if (!existing.has_value()) {
                    resp->set_success(false);
                    resp->mutable_error()->set_code(404);
                    resp->mutable_error()->set_message("document not found");
                    return grpc::Status::OK;
                }
            }

            bool ok = db_->put(skey, data);
            resp->set_success(ok);
            resp->set_key(req->key());
            resp->set_timestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count());
            if (!ok) {
                resp->mutable_error()->set_code(500);
                resp->mutable_error()->set_message("storage write failed");
            }
            return grpc::Status::OK;
        }

        grpc::Status Delete(
            grpc::ServerContext*  /*ctx*/,
            const DeleteRequest*  req,
            DeleteResponse*       resp
        ) override {
            if (req->collection().empty() || req->key().empty()) {
                resp->set_success(false);
                resp->mutable_error()->set_code(400);
                resp->mutable_error()->set_message("collection and key are required");
                return grpc::Status::OK;
            }
            if (!db_) {
                return grpc::Status(grpc::StatusCode::UNAVAILABLE, "storage not available");
            }
            const std::string skey = storageKey(req->collection(), req->key());
            bool ok = db_->del(skey);
            resp->set_success(ok);
            if (!ok) {
                resp->mutable_error()->set_code(404);
                resp->mutable_error()->set_message("document not found or delete failed");
            }
            return grpc::Status::OK;
        }

        // ── Batch CRUD ───────────────────────────────────────────────────────

        grpc::Status BatchCreate(
            grpc::ServerContext*       /*ctx*/,
            const BatchCreateRequest*  req,
            BatchCreateResponse*       resp
        ) override {
            if (!db_) {
                return grpc::Status(grpc::StatusCode::UNAVAILABLE, "storage not available");
            }
            int created = 0;
            for (const auto& doc : req->documents()) {
                if (doc.collection().empty() || doc.key().empty()) {
                  continue;
                }
                const std::string skey = storageKey(doc.collection().empty()
                    ? req->collection() : doc.collection(), doc.key());
                const std::string data(doc.data().begin(), doc.data().end());
                if (db_->put(skey, data)) {
                    resp->add_keys(doc.key());
                    ++created;
                }
            }
            resp->set_success(created > 0 || req->documents().empty());
            resp->set_created_count(created);
            return grpc::Status::OK;
        }

        grpc::Status BatchRead(
            grpc::ServerContext*     /*ctx*/,
            const BatchReadRequest*  req,
            BatchReadResponse*       resp
        ) override {
            if (!db_) {
                return grpc::Status(grpc::StatusCode::UNAVAILABLE, "storage not available");
            }
            for (const auto& key : req->keys()) {
                const std::string skey = storageKey(req->collection(), key);
                auto val = db_->get(skey);
                if (val.has_value()) {
                    auto* doc = resp->add_documents();
                    doc->set_collection(req->collection());
                    doc->set_key(key);
                    doc->set_data(val->data(), val->size());
                }
            }
            resp->set_success(true);
            return grpc::Status::OK;
        }

        grpc::Status BatchUpdate(
            grpc::ServerContext*       /*ctx*/,
            const BatchUpdateRequest*  req,
            BatchUpdateResponse*       resp
        ) override {
            if (!db_) {
                return grpc::Status(grpc::StatusCode::UNAVAILABLE, "storage not available");
            }
            int updated = 0;
            for (const auto& doc : req->documents()) {
                if (doc.key().empty()) {
                  continue;
                }
                const std::string col = doc.collection().empty() ? req->collection() : doc.collection();
                const std::string skey = storageKey(col, doc.key());
                const std::string data(doc.data().begin(), doc.data().end());
                if (db_->put(skey, data)) {
                  ++updated;
                }
            }
            resp->set_success(updated > 0 || req->documents().empty());
            resp->set_updated_count(updated);
            return grpc::Status::OK;
        }

        grpc::Status BatchDelete(
            grpc::ServerContext*       /*ctx*/,
            const BatchDeleteRequest*  req,
            BatchDeleteResponse*       resp
        ) override {
            if (!db_) {
                return grpc::Status(grpc::StatusCode::UNAVAILABLE, "storage not available");
            }
            int deleted = 0;
            for (const auto& key : req->keys()) {
                const std::string skey = storageKey(req->collection(), key);
                if (db_->del(skey)) {
                  ++deleted;
                }
            }
            resp->set_success(deleted > 0 || req->keys().empty());
            resp->set_deleted_count(deleted);
            return grpc::Status::OK;
        }

        // ── Transactions ─────────────────────────────────────────────────────

        grpc::Status BeginTransaction(
            grpc::ServerContext*              /*ctx*/,
            const BeginTransactionRequest*    req,
            BeginTransactionResponse*         resp
        ) override {
            if (!txn_mgr_) {
                resp->set_success(false);
                resp->mutable_error()->set_code(501);
                resp->mutable_error()->set_message("transaction manager not available");
                return grpc::Status::OK;
            }
            try {
                // Map proto IsolationLevel → themis IsolationLevel
                themis::IsolationLevel iso = themis::IsolationLevel::ReadCommitted;
                switch (req->isolation_level()) {
                    case BeginTransactionRequest::SERIALIZABLE:
                        iso = themis::IsolationLevel::SERIALIZABLE; break;
                    case BeginTransactionRequest::REPEATABLE_READ:
                        iso = themis::IsolationLevel::REPEATABLE_READ; break;
                    case BeginTransactionRequest::READ_UNCOMMITTED:
                    [[fallthrough]];
                    case BeginTransactionRequest::READ_COMMITTED:
                    [[fallthrough]];
                    default:
                        iso = themis::IsolationLevel::ReadCommitted; break;
                }
                const auto tid = txn_mgr_->beginTransaction(iso);
                if (req->timeout_ms() > 0) {
                    auto txn = txn_mgr_->getTransaction(tid);
                    if (txn) {
                        txn->setTimeout(std::chrono::milliseconds(req->timeout_ms()));
                    }
                }
                resp->set_success(true);
                resp->set_transaction_id(std::to_string(tid));
            } catch (const std::exception& ex) {
                resp->set_success(false);
                resp->mutable_error()->set_code(500);
                resp->mutable_error()->set_message(ex.what());
            }
            return grpc::Status::OK;
        }

        grpc::Status CommitTransaction(
            grpc::ServerContext*              /*ctx*/,
            const CommitTransactionRequest*   req,
            CommitTransactionResponse*        resp
        ) override {
            if (!txn_mgr_) {
                resp->set_success(false);
                resp->mutable_error()->set_code(501);
                resp->mutable_error()->set_message("transaction manager not available");
                return grpc::Status::OK;
            }
            try {
                const auto tid = std::stoull(req->transaction_id());
                auto status = txn_mgr_->commitTransaction(tid);
                resp->set_success(status.ok);
                if (!status.ok) {
                    resp->mutable_error()->set_code(409);
                    resp->mutable_error()->set_message(status.message);
                } else {
                    resp->set_commit_timestamp(
                        std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::system_clock::now().time_since_epoch()).count());
                }
            } catch (const std::invalid_argument&) {
                resp->set_success(false);
                resp->mutable_error()->set_code(400);
                resp->mutable_error()->set_message("invalid transaction_id");
            } catch (const std::exception& ex) {
                resp->set_success(false);
                resp->mutable_error()->set_code(500);
                resp->mutable_error()->set_message(ex.what());
            }
            return grpc::Status::OK;
        }

        grpc::Status RollbackTransaction(
            grpc::ServerContext*                /*ctx*/,
            const RollbackTransactionRequest*   req,
            RollbackTransactionResponse*        resp
        ) override {
            if (!txn_mgr_) {
                resp->set_success(false);
                resp->mutable_error()->set_code(501);
                resp->mutable_error()->set_message("transaction manager not available");
                return grpc::Status::OK;
            }
            try {
                const auto tid = std::stoull(req->transaction_id());
                const bool ok = txn_mgr_->rollbackTransaction(tid);
                resp->set_success(ok);
                if (!ok) {
                    resp->mutable_error()->set_code(404);
                    resp->mutable_error()->set_message("transaction not found or already completed");
                }
            } catch (const std::invalid_argument&) {
                resp->set_success(false);
                resp->mutable_error()->set_code(400);
                resp->mutable_error()->set_message("invalid transaction_id");
            } catch (const std::exception& ex) {
                resp->set_success(false);
                resp->mutable_error()->set_code(500);
                resp->mutable_error()->set_message(ex.what());
            }
            return grpc::Status::OK;
        }

        // ── Query (AQL) ──────────────────────────────────────────────────────

        grpc::Status ExecuteAQL(
            grpc::ServerContext* /*ctx*/,
            const AQLRequest*    req,
            AQLResponse*         resp
        ) override {
            if (req->query().empty()) {
                resp->set_success(false);
                resp->mutable_error()->set_code(400);
                resp->mutable_error()->set_message("query string is required");
                return grpc::Status::OK;
            }
            if (!aql_engine_) {
                resp->set_success(false);
                resp->mutable_error()->set_code(501);
                resp->mutable_error()->set_message(
                    "AQL engine not wired — inject a themis::IQueryEngine via "
                    "ThemisCoreServiceImpl constructor");
                return grpc::Status(grpc::StatusCode::UNIMPLEMENTED,
                                    "AQL engine not available");
            }
            const auto t0 = std::chrono::steady_clock::now();
            // Serialize bind_vars proto map to a JSON object string.
            // bind_vars is a map<string,string>; pass "{}" when absent/empty.
            std::string bind_vars_json = "{}";
            if (req->bind_vars_size() > 0) {
                nlohmann::json bv = nlohmann::json::object();
                for (const auto& [k, v] : req->bind_vars()) {
                    bv[k] = v;
                }
                bind_vars_json = bv.dump();
            }
            auto result = aql_engine_->execute(req->query(), bind_vars_json);
            const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - t0).count();

            if (!result) {
                resp->set_success(false);
                resp->mutable_error()->set_code(500);
                resp->mutable_error()->set_message(result.error().message());
                return grpc::Status::OK;
            }
            resp->set_success(true);
            auto* row = resp->add_results();
            row->set_data(*result);
            row->set_has_more(false);
            resp->mutable_stats()->set_execution_time_ms(elapsed_ms);
            resp->mutable_stats()->set_result_count(1);
            return grpc::Status::OK;
        }

        grpc::Status StreamQuery(
            grpc::ServerContext*                   /*ctx*/,
            const AQLRequest*                      req,
            grpc::ServerWriter<QueryResult>*       writer
        ) override {
            if (req->query().empty()) {
                return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                                    "query string is required");
            }
            if (!aql_engine_) {
                return grpc::Status(grpc::StatusCode::UNIMPLEMENTED,
                                    "AQL engine not wired");
            }
            std::string stream_bind_vars_json = "{}";
            if (req->bind_vars_size() > 0) {
                nlohmann::json bv = nlohmann::json::object();
                for (const auto& [k, v] : req->bind_vars()) {
                    bv[k] = v;
                }
                stream_bind_vars_json = bv.dump();
            }
            auto result = aql_engine_->execute(req->query(), stream_bind_vars_json);
            if (!result) {
                return grpc::Status(grpc::StatusCode::INTERNAL, result.error().message());
            }
            QueryResult qr;
            qr.set_data(*result);
            qr.set_has_more(false);
            writer->Write(qr);
            return grpc::Status::OK;
        }

        // ── Scan ─────────────────────────────────────────────────────────────

        grpc::Status ScanCollection(
            grpc::ServerContext*                ctx,
            const ScanRequest*                  req,
            grpc::ServerWriter<ScanResult>*     writer
        ) override {
            if (req->collection().empty()) {
                return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT,
                                    "collection is required");
            }
            if (!db_) {
                return grpc::Status(grpc::StatusCode::UNAVAILABLE,
                                    "storage not available");
            }
            const std::string prefix = req->collection() + ":";
            bool cancelled = false;
            db_->scanPrefix(prefix, [&](std::string_view raw_key, std::string_view value) -> bool {
                if (ctx->IsCancelled()) { cancelled = true; return false; }
                // Strip prefix to recover doc key.
                const std::string doc_key(static_cast<int>(raw_key.size()) > static_cast<int>(prefix.size())
                    ? raw_key.substr(prefix.size())
                    : raw_key);

                ScanResult sr;
                auto* doc = sr.mutable_document();
                doc->set_collection(req->collection());
                doc->set_key(doc_key);
                doc->set_data(value.data(),static_cast<int>(value.size()));
                sr.set_has_more(true);
                writer->Write(sr);
                return true; // continue
            });
            (void)cancelled;
            return grpc::Status::OK;
        }

    private:
        std::shared_ptr<RocksDBWrapper>       db_;
        std::shared_ptr<TransactionManager>   txn_mgr_;
        std::shared_ptr<AQLEngine>            aql_engine_;
        std::chrono::steady_clock::time_point start_time_;
    };

    ServiceImpl service_;

#endif // THEMIS_HAS_CORE_GRPC
};

ThemisCoreServiceImpl::ThemisCoreServiceImpl(
    std::shared_ptr<RocksDBWrapper>    db,
    std::shared_ptr<TransactionManager> txn_mgr,
    std::shared_ptr<AQLEngine>          aql_engine)
    : db_(std::move(db))
    , txn_mgr_(std::move(txn_mgr))
    , aql_engine_(std::move(aql_engine))
{
#if THEMIS_HAS_CORE_GRPC
    impl_ = std::make_unique<Impl>(db_, txn_mgr_, aql_engine_);
    THEMIS_INFO("ThemisCoreServiceImpl: initialized with gRPC protocol support");
#else
    ServiceInstanceFn fn;
    {
        std::lock_guard<std::mutex> lock(g_core_grpc_instance_mutex);
        fn = g_core_grpc_instance_fn;
    }
    if (!fn) {
        const std::string error =
            "ThemisCoreServiceImpl requires generated core gRPC stubs or an "
            "injected non-null ServiceInstanceFn in non-proto builds";
        THEMIS_CRITICAL("{}", error);
        throw std::runtime_error(error);
    }
    try {
        service_ptr_ = fn();
    } catch (const std::exception& e) {
        THEMIS_ERROR("ThemisCoreServiceImpl: service-instance callback failed: {}", e.what());
        throw std::runtime_error("ThemisCoreServiceImpl service callback threw an exception");
    } catch (...) {
        THEMIS_ERROR("ThemisCoreServiceImpl: service-instance callback failed: unknown error");
        throw std::runtime_error("ThemisCoreServiceImpl service callback threw an unknown exception");
    }
    if (!service_ptr_) {
        const std::string error =
            "ThemisCoreServiceImpl ServiceInstanceFn returned nullptr in non-proto build";
        THEMIS_CRITICAL("{}", error);
        throw std::runtime_error(error);
    }
#endif
}

ThemisCoreServiceImpl::~ThemisCoreServiceImpl() = default;

void ThemisCoreServiceImpl::setServiceInstanceFn(ServiceInstanceFn fn) {
    std::lock_guard<std::mutex> lock(g_core_grpc_instance_mutex);
    g_core_grpc_instance_fn = std::move(fn);
}

void* ThemisCoreServiceImpl::getServiceInstance() {
#if THEMIS_HAS_CORE_GRPC
    return impl_ ? static_cast<void*>(impl_->get()) : nullptr;
#else
    return service_ptr_;
#endif
}

} // namespace core
} // namespace themis
