/**
 * @file wal_grpc_service.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @author makr-code
 * @version 0.0.47
 * @date 2026-06-02 20:56:29
 * @note Maturity: 🟡 RELEASE-CANDIDATE
 * @note Score: 78/100
 * @note Lines: 283
 * @note Gap Summary: total=5; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=1, Debt=0, C=0, H=3, M=1, L=0
 * @note PR History (last 5): none
 * @note Status: Release Candidate
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/wal_grpc_service.h"

#include "sharding/wal_applier.h"
#include "sharding/wal_manager.h"
#include "utils/zstd_codec.h"
#include "utils/logger.h"
#include <nlohmann/json.hpp>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#if __has_include("sharding/shard_rpc.grpc.pb.h")
#include <grpcpp/grpcpp.h>
#include "sharding/shard_rpc.grpc.pb.h"
#include "shard_rpc.pb.h"
#define THEMIS_HAS_SHARD_GRPC 1
#else
#define THEMIS_HAS_SHARD_GRPC 0
#endif

namespace themis {
namespace server {

namespace {
std::mutex g_wal_grpc_service_mutex;
themis::server::WalGrpcService::ServiceFn g_wal_grpc_service_fn;
} // namespace

/** @brief Implementation detail. */
class WalGrpcService::Impl {
public:
#if THEMIS_HAS_SHARD_GRPC
    Impl(std::shared_ptr<sharding::WALApplier> wal_applier)
        : wal_applier_(std::move(wal_applier))
        , service_(wal_applier_) {}

    themis::sharding::proto::ShardService::Service* get() { return &service_; }

private:
    class ServiceImpl final : public themis::sharding::proto::ShardService::Service {
    public:
        explicit ServiceImpl(std::shared_ptr<sharding::WALApplier> wal_applier)
            : wal_applier_(std::move(wal_applier)) {}

        grpc::Status ApplyWalBatch(
            grpc::ServerContext* /*context*/,
            const themis::sharding::proto::ApplyWalBatchRequest* request,
            themis::sharding::proto::ApplyWalBatchResponse* response
        ) override {
            if (!wal_applier_) {
                return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "WALApplier not configured");
            }
            auto& wal_applier = *wal_applier_;

            std::vector<sharding::WALEntry> entries;
            auto status = hydrateEntries(*request, entries);
            if (!status.ok()) {
                return status;
            }

            auto result = wal_applier.applyBatch(entries);
            response->set_success(result.success);
            response->set_entries_applied(static_cast<uint32_t>(result.entries_applied));
            response->set_last_applied_lsn(result.last_applied_lsn.toString());
            for (const auto& err : result.errors) {
              response->add_errors(err);
            }

            if (!result.success) {
                return grpc::Status(grpc::StatusCode::INTERNAL, "Apply failed");
            }
            return grpc::Status::OK;
        }

    private:
        static sharding::WALEntryType toInternalType(themis::sharding::proto::WalEntryType t) {
            using W = themis::sharding::proto::WalEntryType;
            switch (t) {
                case W::WAL_INSERT: return sharding::WALEntryType::INSERT;
                case W::WAL_UPDATE: return sharding::WALEntryType::UPDATE;
                case W::WAL_DELETE: return sharding::WALEntryType::DELETE;
                case W::WAL_BEGIN_TX: return sharding::WALEntryType::BEGIN_TX;
                case W::WAL_COMMIT_TX: return sharding::WALEntryType::COMMIT_TX;
                case W::WAL_ABORT_TX: return sharding::WALEntryType::ABORT_TX;
                case W::WAL_CHECKPOINT: return sharding::WALEntryType::CHECKPOINT;
                default: return sharding::WALEntryType::INSERT;
            }
        }

        static grpc::Status parseJsonArray(const std::string& payload, std::vector<sharding::WALEntry>& out) {
            try {
                auto json_entries = nlohmann::json::parse(payload);
                if (!json_entries.is_array()) {
                    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "entries_compressed is not an array");
                }
                out.reserve(json_entries.size());
                for (const auto& item : json_entries) {
                    sharding::WALEntry e;
                    if (item.contains("lsn")) {
                        e.lsn = sharding::LSN::fromString(item.value("lsn", std::string("0/0")));
                    }
                    e.type = static_cast<sharding::WALEntryType>(item.value("type", 0));
                    e.timestamp = item.value("timestamp", uint64_t(0));
                    e.transaction_id = item.value("transaction_id", std::string());
                    if (item.contains("data")) {
                        e.data = item["data"];
                    }
                    out.push_back(std::move(e));
                }
                return grpc::Status::OK;
            } catch (const std::exception& e) {
                return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, std::string("Failed to parse entries_compressed: ") + e.what());
            }
        }

        static grpc::Status hydrateEntries(const themis::sharding::proto::ApplyWalBatchRequest& request,
                                           std::vector<sharding::WALEntry>& out) {
            if (request.entries_size() > 0) {
                out.reserve(request.entries_size());
                for (const auto& item : request.entries()) {
                    sharding::WALEntry e;
                    e.lsn = sharding::LSN::fromString(item.lsn());
                    e.type = toInternalType(item.type());
                    e.timestamp = item.timestamp();
                    e.transaction_id = item.transaction_id();
                    if (!item.data_json().empty()) {
                        try {
                            e.data = nlohmann::json::parse(item.data_json().begin(), item.data_json().end());
                        } catch (const std::exception& ex) {
                            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, std::string("Invalid data_json: ") + ex.what());
                        }
                    }
                    out.push_back(std::move(e));
                }
                return grpc::Status::OK;
            }

            if (!request.entries_compressed().empty()) {
                std::vector<uint8_t> compressed(request.entries_compressed().begin(), request.entries_compressed().end());
                auto decompressed = themis::utils::zstd_decompress(compressed);
                if (decompressed.empty()) {
                    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Failed to decompress entries_compressed");
                }
                std::string payload(reinterpret_cast<const char*>(decompressed.data()),static_cast<int>(decompressed.size()));
                return parseJsonArray(payload, out);
            }

            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "entries or entries_compressed must be provided");
        }

        std::shared_ptr<sharding::WALApplier> wal_applier_;
    };

    std::shared_ptr<sharding::WALApplier> wal_applier_;
    ServiceImpl service_;
#endif // THEMIS_HAS_SHARD_GRPC
};

WalGrpcService::WalGrpcService(std::shared_ptr<sharding::WALApplier> wal_applier)
    : wal_applier_(std::move(wal_applier)) {
#if THEMIS_HAS_SHARD_GRPC
    impl_ = std::make_unique<Impl>(wal_applier_);
#else
    (void)wal_applier_;
    ServiceFn fn;
    {
        std::lock_guard<std::mutex> lock(g_wal_grpc_service_mutex);
        fn = g_wal_grpc_service_fn;
    }
    if (!fn) {
        THEMIS_WARN(
            "Shard gRPC stubs not found and no injected ServiceFn; "
            "WalGrpcService endpoint remains disabled in this build"
        );
    } else {
        try {
            service_ptr_ = fn();
        } catch (const std::exception& e) {
            THEMIS_ERROR("WalGrpcService: service callback failed: {}", e.what());
            throw std::runtime_error([[maybe_unused]] "WalGrpcService service callback threw an exception");
        } catch (...) {
            THEMIS_ERROR([[maybe_unused]] "WalGrpcService: service callback failed: unknown error");
            throw std::runtime_error([[maybe_unused]] "WalGrpcService service callback threw an unknown exception");
        }
        if (!service_ptr_) {
            const std::string error =
                "WalGrpcService ServiceFn returned nullptr in non-proto build";
            THEMIS_CRITICAL("{}", error);
            throw std::runtime_error(error);
        }
    }
#endif
}

WalGrpcService::~WalGrpcService() = default;

void WalGrpcService::setServiceFn(ServiceFn fn) {
    std::lock_guard<std::mutex> lock(g_wal_grpc_service_mutex);
    g_wal_grpc_service_fn = std::move(fn);
}

void* WalGrpcService::service() {
#if THEMIS_HAS_SHARD_GRPC
    return impl_ ? static_cast<void*>(impl_->get()) : nullptr;
#else
    return service_ptr_;
#endif
}

} // namespace server
} // namespace themis
