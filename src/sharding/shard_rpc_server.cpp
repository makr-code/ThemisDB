/**
 * @file shard_rpc_server.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/shard_rpc_server.h"
#include "sharding/raft_consensus.h"
#include "sharding/shard_transfer_store.h"
#include "core/production_mode.h"
#include "shard_rpc_tls_validation.h"
#include "utils/logger.h"
#include "utils/file_utils.h"
#include <algorithm>
#include <optional>
#include <chrono>
#include <set>
#include <stdexcept>

#ifdef THEMIS_ENABLE_GRPC
#if __has_include("shard_rpc.grpc.pb.h")
#include <grpcpp/grpcpp.h>
#include "shard_rpc.grpc.pb.h"
#include "shard_rpc.pb.h"
#define THEMIS_HAS_SHARD_GRPC 1
#else
#define THEMIS_HAS_SHARD_GRPC 0
#endif
#else
#define THEMIS_HAS_SHARD_GRPC 0
#endif

namespace themis::sharding {


#if THEMIS_HAS_SHARD_GRPC

namespace {

struct AuthenticatedPeerIdentity {
    std::string property_name;
    std::vector<std::string> values;
};

std::optional<AuthenticatedPeerIdentity> getAuthenticatedPeerIdentity(
    grpc::ServerContext* context) {
    if (!context) {
        return std::nullopt;
    }

    const auto auth_context = context->auth_context();
    if (!auth_context || !auth_context->IsPeerAuthenticated()) {
        return std::nullopt;
    }

    const auto identity_property = auth_context->GetPeerIdentityPropertyName();
    if (identity_property.rfind("x509_", 0) != 0) {
        return std::nullopt;
    }

    const auto peer_identities = auth_context->GetPeerIdentity();
    if (peer_identities.empty()) {
        return std::nullopt;
    }

    AuthenticatedPeerIdentity identity;
    identity.property_name = identity_property;
    identity.values.reserve(peer_identities.size());
    for (const auto& peer_identity : peer_identities) {
        if (peer_identity.empty()) {
            return std::nullopt;
        }
        identity.values.emplace_back(peer_identity.data(), peer_identity.size());
    }
    return identity;
}

std::optional<std::string> resolveCoordinatorShardId(
    const AuthenticatedPeerIdentity& peer_identity,
    const std::vector<ShardRPCServer::CoordinatorIdentityBinding>& bindings) {
    std::optional<std::string> resolved_shard_id;
    for (const auto& binding : bindings) {
        if (binding.property_name != peer_identity.property_name) {
            continue;
        }
        for (const auto& value : peer_identity.values) {
            if (binding.property_value != value) {
                continue;
            }
            if (resolved_shard_id &&
                *resolved_shard_id != binding.coordinator_shard_id) {
                return std::nullopt;
            }
            resolved_shard_id = binding.coordinator_shard_id;
        }
    }
    return resolved_shard_id;
}

std::optional<std::string> resolveRaftNodeId(
    const AuthenticatedPeerIdentity& peer_identity,
    const std::vector<ShardRPCServer::RaftIdentityBinding>& bindings) {
    std::optional<std::string> resolved_node_id;
    for (const auto& binding : bindings) {
        if (binding.property_name != peer_identity.property_name) {
            continue;
        }
        for (const auto& value : peer_identity.values) {
            if (binding.property_value != value) {
                continue;
            }
            if (resolved_node_id && *resolved_node_id != binding.node_id) {
                return std::nullopt;
            }
            resolved_node_id = binding.node_id;
        }
    }
    return resolved_node_id;
}

} // namespace

class ShardServiceImpl final : public themis::sharding::proto::ShardService::Service {
public:
    /**
     * @brief Shard Service Impl.
     * @param[in,out] handler Input/output parameter.
     * @return Return value.
     */
    ShardServiceImpl(
        ShardRPCServer::RequestHandler* handler,
        std::vector<ShardRPCServer::CoordinatorIdentityBinding> identity_bindings,
        std::shared_ptr<ShardTransferStore> transfer_store,
        std::vector<ShardRPCServer::CoordinatorIdentityBinding> transfer_bindings,
        std::function<std::shared_ptr<themisdb::sharding::RaftConsensus>(
            const std::string&)> raft_group_resolver,
        std::vector<ShardRPCServer::RaftIdentityBinding> raft_identity_bindings)
        : handler_(handler),
          identity_bindings_(std::move(identity_bindings)),
          transfer_store_(std::move(transfer_store)),
          transfer_bindings_(std::move(transfer_bindings)),
          raft_group_resolver_(std::move(raft_group_resolver)),
          raft_identity_bindings_(std::move(raft_identity_bindings)) {}

    grpc::Status PrepareShardTransfer(
        grpc::ServerContext* context, const proto::ShardTransferManifest* request,
        proto::ShardTransferCheckpoint* response) override {
        return withTransferPrincipal(context, [&](const std::string& principal) {
            ShardTransferManifest manifest;
            manifest.id = request->migration_id();
            manifest.source = request->source_shard();
            manifest.target = request->target_shard();
            manifest.range_start = request->range_start();
            manifest.range_end = request->range_end();
            manifest.epoch = request->topology_epoch();
            manifest.watermark = request->snapshot_watermark();
            manifest.chunks = request->chunks();
            manifest.bytes = request->bytes();
            manifest.sha256 = request->sha256();
            response->set_next_chunk(transfer_store_->prepare(manifest, principal));
            return grpc::Status::OK;
        });
    }

    grpc::Status StreamShardTransfer(
        grpc::ServerContext* context,
        grpc::ServerReaderWriter<proto::ShardTransferCheckpoint, proto::ShardTransferChunk>* stream) override {
        return withTransferPrincipal(context, [&](const std::string& principal) {
            proto::ShardTransferChunk chunk;
            std::optional<std::string> id;
            // Bound replay traffic independently of staged bytes and manifest size.
            constexpr unsigned max_frames = 256;
            for (unsigned frames = 0; stream->Read(&chunk); ++frames) {
                if (context->IsCancelled()) {
                    return grpc::Status(grpc::StatusCode::CANCELLED, "transfer interrupted");
                }
                if (frames == max_frames || chunk.payload().size() > ShardTransferStore::kMaxChunkBytes ||
                    (id && *id != chunk.migration_id())) {
                    return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "invalid transfer stream frame");
                }
                if (!id) {
                    id = chunk.migration_id();
                }
                proto::ShardTransferCheckpoint ack;
                ack.set_next_chunk(transfer_store_->append(
                    *id, principal, chunk.index(), chunk.payload(), chunk.sha256()));
                if (!stream->Write(ack)) {
                    return grpc::Status(grpc::StatusCode::CANCELLED, "checkpoint delivery interrupted");
                }
            }
            return context->IsCancelled()
                ? grpc::Status(grpc::StatusCode::CANCELLED, "transfer interrupted")
                : grpc::Status::OK;
        });
    }

    grpc::Status VerifyShardTransfer(
        grpc::ServerContext* context, const proto::ShardTransferId* request,
        proto::ShardTransferVerified* response) override {
        return withTransferPrincipal(context, [&](const std::string& principal) {
            transfer_store_->verify(request->migration_id(), principal);
            response->set_verified(true);
            return grpc::Status::OK;
        });
    }

    grpc::Status AbortShardTransfer(
        grpc::ServerContext* context, const proto::ShardTransferId* request,
        proto::ShardTransferAborted* response) override {
        return withTransferPrincipal(context, [&](const std::string& principal) {
            transfer_store_->abort(request->migration_id(), principal);
            response->set_aborted(true);
            return grpc::Status::OK;
        });
    }

    /**
     * @brief Called once Impl is constructed so we can serve shard identity.
     * @param[in] address Input parameter.
     * @details Implements setImplRef without additional internal calls.
     */
    void setImplRef(const std::string& address) {
        listen_address_ = address;
    }
    
    grpc::Status PrepareTransaction(
        grpc::ServerContext* context,
        const themis::sharding::proto::PrepareRequest* request,
        themis::sharding::proto::PrepareResponse* response
    ) override {
        if (!handler_) {
            return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "No request handler configured");
        }

        const auto peer_identity = getAuthenticatedPeerIdentity(context);
        if (!peer_identity) {
            return grpc::Status(
                grpc::StatusCode::UNAUTHENTICATED,
                "PREPARE requires an authenticated X.509 shard identity");
        }
        const auto coordinator_shard_id =
            resolveCoordinatorShardId(*peer_identity, identity_bindings_);
        if (!coordinator_shard_id) {
            return grpc::Status(
                grpc::StatusCode::PERMISSION_DENIED,
                "authenticated X.509 identity is not configured for shard coordination");
        }
        if (*coordinator_shard_id != request->coordinator_shard_id()) {
            THEMIS_WARN(
                "gRPC PrepareTransaction rejected coordinator identity mismatch: "
                "authenticated={}, claimed={}",
                *coordinator_shard_id,
                request->coordinator_shard_id());
            return grpc::Status(
                grpc::StatusCode::PERMISSION_DENIED,
                "coordinator identity does not match authenticated peer");
        }
        
        THEMIS_DEBUG("gRPC PrepareTransaction: txn_id={}", request->transaction_id());
        
        try {
            bool vote_commit = handler_->onPrepare(
                request->transaction_id(),
                request->coordinator_shard_id(),
                request->transaction_data()
            );
            
            response->set_vote_commit(vote_commit);
            
            return grpc::Status::OK;
            
        } catch (const std::exception& e) {
            THEMIS_ERROR("PrepareTransaction failed: {}", e.what());
            response->set_vote_commit(false);
            response->set_error(e.what());
            return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
        }
    }
    
    grpc::Status CommitTransaction(
        grpc::ServerContext* context,
        const themis::sharding::proto::CommitRequest* request,
        themis::sharding::proto::CommitResponse* response
    ) override {
        if (!handler_) {
            return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "No request handler configured");
        }
        
        THEMIS_DEBUG("gRPC CommitTransaction: txn_id={}", request->transaction_id());
        
        try {
            bool success = handler_->onCommit(request->transaction_id());
            response->set_success(success);
            
            return grpc::Status::OK;
            
        } catch (const std::exception& e) {
            THEMIS_ERROR("CommitTransaction failed: {}", e.what());
            response->set_success(false);
            response->set_error(e.what());
            return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
        }
    }
    
    grpc::Status AbortTransaction(
        grpc::ServerContext* context,
        const themis::sharding::proto::AbortRequest* request,
        themis::sharding::proto::AbortResponse* response
    ) override {
        if (!handler_) {
            return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION, "No request handler configured");
        }
        
        THEMIS_DEBUG("gRPC AbortTransaction: txn_id={}", request->transaction_id());
        
        try {
            bool success = handler_->onAbort(request->transaction_id());
            response->set_success(success);
            
            return grpc::Status::OK;
            
        } catch (const std::exception& e) {
            THEMIS_ERROR("AbortTransaction failed: {}", e.what());
            response->set_success(false);
            return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
        }
    }
    
    grpc::Status HealthCheck(
        grpc::ServerContext* context,
        const themis::sharding::proto::HealthRequest* request,
        themis::sharding::proto::HealthResponse* response
    ) override {
        THEMIS_DEBUG("gRPC HealthCheck");
        
        if (handler_) {
            auto health_info = handler_->onHealthCheck();
            response->set_status(health_info.is_healthy ? "healthy" : "unhealthy");
            response->set_version(health_info.version);
            response->set_uptime_seconds(health_info.uptime_seconds);
        } else {
            response->set_status("healthy");
            // Use compile-time version string
#ifdef THEMIS_VERSION_STRING
            response->set_version(THEMIS_VERSION_STRING);
#else
            response->set_version("unknown");
#endif
            response->set_uptime_seconds(0);
        }
        
        return grpc::Status::OK;
    }

    grpc::Status RequestVote(
        grpc::ServerContext* context,
        const proto::RaftVoteRequestProto* request,
        proto::RaftVoteResponseProto* response) override {
        if (!hasBoundedRaftDeadline(context)) {
            return {grpc::StatusCode::INVALID_ARGUMENT, "Raft RPC requires a deadline of at most five seconds"};
        }
        std::shared_ptr<themisdb::sharding::RaftConsensus> raft;
        const auto status = validateRaftPeer(context, request->raft_group_id(),
                                             request->candidate_id(), raft);
        if (!status.ok()) {
            return status;
        }
        if (request->raft_group_id().empty() || request->candidate_id().empty()) {
            return {grpc::StatusCode::INVALID_ARGUMENT, "Raft group and candidate IDs are required"};
        }
        try {
            const auto result = raft->getRaftState().handleVoteRequest(
                {request->candidate_id(), request->term(),
                 request->last_log_index(), request->last_log_term()});
            response->set_voter_id(result.voter_id);
            response->set_term(result.term);
            response->set_vote_granted(result.vote_granted);
            response->set_reason(result.reason);
            return grpc::Status::OK;
        } catch (const std::exception& error) {
            THEMIS_ERROR("Raft vote handler failed: {}", error.what());
            return {grpc::StatusCode::INTERNAL, "Raft vote handler failed"};
        }
    }

    grpc::Status AppendEntries(
        grpc::ServerContext* context,
        const proto::RaftAppendEntriesRequestProto* request,
        proto::RaftAppendEntriesResponseProto* response) override {
        if (!hasBoundedRaftDeadline(context)) {
            return {grpc::StatusCode::INVALID_ARGUMENT, "Raft RPC requires a deadline of at most five seconds"};
        }
        std::shared_ptr<themisdb::sharding::RaftConsensus> raft;
        const auto status = validateRaftPeer(context, request->raft_group_id(),
                                             request->leader_id(), raft);
        if (!status.ok()) {
            return status;
        }
        if (request->raft_group_id().empty() || request->leader_id().empty()) {
            return {grpc::StatusCode::INVALID_ARGUMENT, "Raft group and leader IDs are required"};
        }
        constexpr int kMaxRaftEntriesPerRpc = 256;
        constexpr std::size_t kMaxRaftCommandBytes = 1024 * 1024;
        constexpr std::size_t kMaxRaftPayloadBytes = 3 * 1024 * 1024;
        if (request->entries_size() > kMaxRaftEntriesPerRpc) {
            return {grpc::StatusCode::RESOURCE_EXHAUSTED, "too many Raft log entries"};
        }

        try {
            themisdb::sharding::AppendEntriesRequest append;
            append.term = request->term();
            append.leader_id = request->leader_id();
            append.prev_log_index = request->prev_log_index();
            append.prev_log_term = request->prev_log_term();
            append.leader_commit = request->leader_commit();
            append.entries.reserve(static_cast<std::size_t>(request->entries_size()));
            std::size_t payload_bytes = 0;
            for (const auto& entry : request->entries()) {
                if (entry.command().size() > kMaxRaftCommandBytes ||
                    payload_bytes > kMaxRaftPayloadBytes - entry.command().size()) {
                    return {grpc::StatusCode::RESOURCE_EXHAUSTED,
                            "Raft AppendEntries exceeds the payload size limit"};
                }
                payload_bytes += entry.command().size();
                append.entries.emplace_back(entry.term(), entry.index(), entry.command(),
                                            entry.timestamp_ns());
            }
            const auto result = raft->getRaftState().handleAppendEntries(append);
            response->set_follower_id(raft->getRaftState().getNodeId());
            response->set_term(result.term);
            response->set_success(result.success);
            response->set_match_index(result.match_index);
            return grpc::Status::OK;
        } catch (const std::exception& error) {
            THEMIS_ERROR("Raft AppendEntries handler failed: {}", error.what());
            return {grpc::StatusCode::INTERNAL, "Raft AppendEntries handler failed"};
        }
    }

    grpc::Status CollectWaitForEdges(
        grpc::ServerContext* context,
        const themis::sharding::proto::CollectWaitForEdgesRequest* request,
        themis::sharding::proto::CollectWaitForEdgesResponse* response
    ) override {
        THEMIS_DEBUG("gRPC CollectWaitForEdges");

        response->set_shard_id(listen_address_);

        if (handler_) {
            try {
                const auto edges = handler_->onCollectWaitForEdges();
                for (const auto& edge : edges) {
                    auto* proto_edge = response->add_edges();
                    proto_edge->set_waiting_transaction_id(edge.waiting_transaction_id);
                    proto_edge->set_blocking_transaction_id(edge.blocking_transaction_id);
                }
            } catch (const std::exception& e) {
                THEMIS_ERROR("CollectWaitForEdges: onCollectWaitForEdges threw: {}", e.what());
                return grpc::Status(grpc::StatusCode::INTERNAL, e.what());
            }
        }
        // No handler: return an empty edge list (fail-open — no deadlock assumed).

        return grpc::Status::OK;
    }
    
    grpc::Status GetShardStatus(
        grpc::ServerContext* context,
        const themis::sharding::proto::StatusRequest* request,
        themis::sharding::proto::StatusResponse* response
    ) override {
        THEMIS_DEBUG("gRPC GetShardStatus (include_metrics={})", request->include_metrics());

        // shard_id: use the server's listen address as the stable shard identity
        if (listen_address_.empty()) {
            return grpc::Status(grpc::StatusCode::FAILED_PRECONDITION,
                                "Shard identity not yet initialised (setImplRef not called)");
        }
        response->set_shard_id(listen_address_);

        // state: derived from health check when handler is available
        if (handler_) {
            try {
                auto health = handler_->onHealthCheck();
                response->set_state(health.is_healthy ? "healthy" : "unhealthy");
            } catch (const std::exception& e) {
                THEMIS_ERROR("GetShardStatus: health check threw: {}", e.what());
                response->set_state("unknown");
            }
        } else {
            response->set_state("healthy");
        }

        // Token range: advertise the full uint64 range as a sentinel.
        // A full ring-aware implementation would query the ConsistentHashRing here.
        response->set_token_range_start(0);
        response->set_token_range_end(UINT64_MAX);

        return grpc::Status::OK;
    }
    
private:
    static bool hasBoundedRaftDeadline(grpc::ServerContext* context) {
        const auto now = std::chrono::system_clock::now();
        return context && context->deadline() > now &&
               context->deadline() <= now + std::chrono::seconds(5);
    }

    grpc::Status validateRaftPeer(grpc::ServerContext* context,
                                  const std::string& group_id,
                                  const std::string& claimed_node_id,
                                  std::shared_ptr<themisdb::sharding::RaftConsensus>& raft) const {
        if (!raft_group_resolver_ || raft_identity_bindings_.empty()) {
            return {grpc::StatusCode::FAILED_PRECONDITION,
                    "authenticated Raft transport is not configured"};
        }
        if (group_id.empty() || claimed_node_id.empty()) {
            return {grpc::StatusCode::INVALID_ARGUMENT,
                    "Raft group and member IDs are required"};
        }
        const auto peer = getAuthenticatedPeerIdentity(context);
        if (!peer) {
            return {grpc::StatusCode::UNAUTHENTICATED, "verified X.509 peer required"};
        }
        const auto authenticated_node_id = resolveRaftNodeId(*peer, raft_identity_bindings_);
        if (!authenticated_node_id || *authenticated_node_id != claimed_node_id) {
            return {grpc::StatusCode::PERMISSION_DENIED,
                    "Raft member ID does not match the authenticated X.509 identity"};
        }
        try {
            raft = raft_group_resolver_(group_id);
        } catch (const std::exception& error) {
            THEMIS_ERROR("Raft group resolver failed: {}", error.what());
            return {grpc::StatusCode::INTERNAL, "Raft group resolver failed"};
        }
        if (!raft) {
            return {grpc::StatusCode::NOT_FOUND, "Raft group is not configured"};
        }
        const auto members = raft->getRaftState().getClusterMembers();
        if (std::find(members.begin(), members.end(), *authenticated_node_id) == members.end()) {
            return {grpc::StatusCode::PERMISSION_DENIED,
                    "authenticated node is not a member of the requested Raft group"};
        }
        return grpc::Status::OK;
    }

    template <typename Operation>
    grpc::Status withTransferPrincipal(grpc::ServerContext* context, Operation&& operation) {
        if (!transfer_store_) {
            return {grpc::StatusCode::FAILED_PRECONDITION, "no shard transfer store configured"};
        }
        if (context->deadline() > std::chrono::system_clock::now() + std::chrono::minutes(5)) {
            return {grpc::StatusCode::INVALID_ARGUMENT, "transfer deadline exceeds five minutes"};
        }
        try {
            const auto peer = getAuthenticatedPeerIdentity(context);
            if (!peer) {
                return {grpc::StatusCode::UNAUTHENTICATED, "verified X.509 peer required"};
            }
            const auto principal = resolveCoordinatorShardId(*peer, transfer_bindings_);
            if (!principal) {
                return {grpc::StatusCode::PERMISSION_DENIED, "no matching transfer principal"};
            }
            return operation(*principal);
        } catch (const std::invalid_argument&) {
            return {grpc::StatusCode::INVALID_ARGUMENT, "invalid transfer metadata or payload"};
        } catch (const std::logic_error&) {
            return {grpc::StatusCode::FAILED_PRECONDITION, "transfer assignment or state rejected"};
        } catch (const std::exception& e) {
            THEMIS_ERROR("Shard transfer storage operation failed: {}", e.what());
            return {grpc::StatusCode::INTERNAL, "transfer storage operation failed"};
        } catch (...) {
            return {grpc::StatusCode::INTERNAL, "transfer operation failed"};
        }
    }

    ShardRPCServer::RequestHandler* handler_;
    std::vector<ShardRPCServer::CoordinatorIdentityBinding> identity_bindings_;
    std::shared_ptr<ShardTransferStore> transfer_store_;
    std::vector<ShardRPCServer::CoordinatorIdentityBinding> transfer_bindings_;
    std::function<std::shared_ptr<themisdb::sharding::RaftConsensus>(
        const std::string&)> raft_group_resolver_;
    std::vector<ShardRPCServer::RaftIdentityBinding> raft_identity_bindings_;
    std::string listen_address_;
};

#endif // THEMIS_HAS_SHARD_GRPC

struct ShardRPCServer::Impl {
    std::string listen_address = {};
    RequestHandler* handler = nullptr;
    ShardRPCServer::Config config;  // Store full configuration
    
#if THEMIS_HAS_SHARD_GRPC
    std::unique_ptr<grpc::Server> server;
    std::unique_ptr<ShardServiceImpl> service;
#endif
    
    explicit Impl(const std::string& addr) : listen_address(addr) {
        config.listen_address = addr;
    }
    
    explicit Impl(const ShardRPCServer::Config& cfg) : listen_address(cfg.listen_address), config(cfg) {}
};

ShardRPCServer::ShardRPCServer(const std::string& listen_address)
    : impl_(std::make_unique<Impl>(listen_address))
{
    THEMIS_INFO("ShardRPCServer created on: {}", listen_address);
}

ShardRPCServer::ShardRPCServer(const Config& config)
    : impl_(std::make_unique<Impl>(config))
{
    THEMIS_INFO("ShardRPCServer created on: {} (mTLS: {})", config.listen_address, config.enable_mtls);
}

ShardRPCServer::~ShardRPCServer() {
    stop();
}

/**
 * @brief Set Request Handler.
 * @param[in,out] handler Input/output parameter.
 * @details Implements setRequestHandler without additional internal calls.
 */
void ShardRPCServer::setRequestHandler(RequestHandler* handler) {
    impl_->handler = handler;
}

/**
 * @brief Start.
 * @return True when the operation succeeds.
 * @throws std::runtime_error if an error occurs.
 * @details Calls: setImplRef(), empty(), themis::utils::readFileContents(), THEMIS_INFO(), push_back(), grpc::SslServerCredentials(), THEMIS_ERROR(), what().
 */
bool ShardRPCServer::start() {
#if THEMIS_HAS_SHARD_GRPC
    try {
        const bool production_mode = themis::core::ProductionMode::isEnabled();
        if (production_mode &&
            (!impl_->config.enable_mtls || !impl_->config.tls_require_client_cert)) {
            throw std::runtime_error(
                "ShardRPCServer: production mode requires mTLS and client certificate verification");
        }

        if (impl_->config.transfer_store &&
            (!impl_->config.enable_mtls || !impl_->config.tls_require_client_cert ||
             impl_->config.transfer_identity_bindings.empty())) {
            throw std::runtime_error(
                "ShardRPCServer: transfer storage requires verified mTLS and transfer identities");
        }

        const auto validate_bindings = [](const auto& bindings) {
            std::set<std::pair<std::string, std::string>> configured_identities;
            for (const auto& binding : bindings) {
                if (binding.property_name.rfind("x509_", 0) != 0 ||
                    binding.property_value.empty() ||
                    binding.coordinator_shard_id.empty() ||
                    !configured_identities.emplace(binding.property_name,
                                                   binding.property_value).second) {
                    throw std::runtime_error(
                        "ShardRPCServer: identity bindings must be unique, "
                        "non-empty X.509 property/value mappings");
                }
            }
        };
        validate_bindings(impl_->config.coordinator_identity_bindings);
        validate_bindings(impl_->config.transfer_identity_bindings);

        if (static_cast<bool>(impl_->config.raft_group_resolver) !=
            !impl_->config.raft_identity_bindings.empty()) {
            throw std::runtime_error(
                "ShardRPCServer: Raft transport requires both a group resolver and identity bindings");
        }
        if (impl_->config.raft_group_resolver &&
            (!impl_->config.enable_mtls || !impl_->config.tls_require_client_cert)) {
            throw std::runtime_error(
                "ShardRPCServer: Raft transport requires verified mutual TLS");
        }
        std::set<std::pair<std::string, std::string>> raft_identities;
        for (const auto& binding : impl_->config.raft_identity_bindings) {
            if (binding.property_name.rfind("x509_", 0) != 0 ||
                binding.property_value.empty() || binding.node_id.empty() ||
                !raft_identities.emplace(binding.property_name,
                                         binding.property_value).second) {
                throw std::runtime_error(
                    "ShardRPCServer: Raft identities must be unique, non-empty X.509 mappings");
            }
        }

        impl_->service = std::make_unique<ShardServiceImpl>(
            impl_->handler, impl_->config.coordinator_identity_bindings,
            impl_->config.transfer_store, impl_->config.transfer_identity_bindings,
            impl_->config.raft_group_resolver, impl_->config.raft_identity_bindings);
        impl_->service->setImplRef(impl_->listen_address);
        
        grpc::ServerBuilder builder;
        
        // Configure server credentials
        std::shared_ptr<grpc::ServerCredentials> credentials;
        
        if (impl_->config.enable_mtls) {
            if (impl_->config.tls_cert_path.empty() || impl_->config.tls_key_path.empty()) {
                throw std::runtime_error(
                    "ShardRPCServer: mTLS requires both a server certificate and private key path");
            }

            grpc::SslServerCredentialsOptions ssl_opts;
            grpc::SslServerCredentialsOptions::PemKeyCertPair key_cert_pair;
            key_cert_pair.private_key =
                themis::utils::readFileContents(impl_->config.tls_key_path);
            key_cert_pair.cert_chain =
                themis::utils::readFileContents(impl_->config.tls_cert_path);
            if (key_cert_pair.private_key.empty() || key_cert_pair.cert_chain.empty()) {
                throw std::runtime_error(
                    "ShardRPCServer: mTLS certificate and private key files must not be empty");
            }
            if (!detail::containsMatchingPemKeyPair(
                    key_cert_pair.cert_chain, key_cert_pair.private_key)) {
                throw std::runtime_error(
                    "ShardRPCServer: mTLS certificate and private key are invalid or mismatched");
            }
            ssl_opts.pem_key_cert_pairs.push_back(key_cert_pair);
            THEMIS_INFO("Loaded server certificate from: {}", impl_->config.tls_cert_path);

            if (impl_->config.tls_require_client_cert) {
                if (impl_->config.tls_ca_cert_path.empty()) {
                    throw std::runtime_error(
                        "ShardRPCServer: client certificate verification requires a CA certificate path");
                }
                ssl_opts.pem_root_certs =
                    themis::utils::readFileContents(impl_->config.tls_ca_cert_path);
                if (ssl_opts.pem_root_certs.empty()) {
                    throw std::runtime_error(
                        "ShardRPCServer: CA certificate file must not be empty");
                }
                if (!detail::containsPemCertificate(ssl_opts.pem_root_certs)) {
                    throw std::runtime_error(
                        "ShardRPCServer: CA certificate data is invalid");
                }
                ssl_opts.client_certificate_request =
                    GRPC_SSL_REQUEST_AND_REQUIRE_CLIENT_CERTIFICATE_AND_VERIFY;
                THEMIS_INFO("Client certificate verification enabled (mutual TLS)");
            } else {
                ssl_opts.client_certificate_request = GRPC_SSL_DONT_REQUEST_CLIENT_CERTIFICATE;
            }

            credentials = grpc::SslServerCredentials(ssl_opts);
            if (!credentials) {
                throw std::runtime_error(
                    "ShardRPCServer: gRPC rejected the configured TLS credentials");
            }
            THEMIS_INFO("mTLS enabled for shard RPC server");
        } else {
            // Insecure credentials are permitted only outside canonical production mode.
            credentials = grpc::InsecureServerCredentials();
            THEMIS_WARN("mTLS is disabled for shard RPC server. This is insecure and should only be used in development.");
        }
        
        builder.AddListeningPort(impl_->listen_address, credentials);
        builder.RegisterService(impl_->service.get());
        
        // Keepalive settings (match client settings)
        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIME_MS, 30000);
        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, 10000);
        builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, 1);
        builder.AddChannelArgument(GRPC_ARG_HTTP2_MAX_PINGS_WITHOUT_DATA, 0);
        
        // Connection settings
        builder.AddChannelArgument(GRPC_ARG_MAX_CONNECTION_IDLE_MS, 300000);  // 5 minutes
        builder.AddChannelArgument(GRPC_ARG_MAX_CONNECTION_AGE_MS, 3600000);  // 1 hour
        
        impl_->server = builder.BuildAndStart();
        
        if (impl_->server) {
            THEMIS_INFO("gRPC ShardService listening on: {}", impl_->listen_address);
            return true;
        } else {
            THEMIS_ERROR("Failed to start gRPC ShardService");
            return false;
        }
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Failed to start gRPC server: {}", e.what());
        return false;
    }
#else
    THEMIS_WARN("gRPC support not available, ShardRPCServer not started");
    return false;
#endif
}

/**
 * @brief Stop.
 * @details Calls: THEMIS_INFO(), Shutdown(), reset().
 */
void ShardRPCServer::stop() {
#if THEMIS_HAS_SHARD_GRPC
    if (impl_->server) {
        THEMIS_INFO("Stopping gRPC ShardService");
        impl_->server->Shutdown();
        impl_->server.reset();
    }
    impl_->service.reset();
#endif
}

/**
 * @brief Wait.
 * @details Calls: Wait().
 */
void ShardRPCServer::wait() {
#if THEMIS_HAS_SHARD_GRPC
    if (impl_->server) {
        impl_->server->Wait();
    }
#endif
}

} // namespace themis::sharding
