// ThemisDB gRPC Service Implementation
// Version: 1.0.0
// Date: December 25, 2024

#include "server/grpc_service.h"
#include "logger.h"
#include <chrono>

namespace themis {
namespace grpc_service {

// Helper function to generate unique session ID
static std::string generateSessionId() {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    return "session_" + std::to_string(millis) + "_" + std::to_string(rand());
}

// Helper function to generate unique transaction ID
static std::string generateTransactionId() {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    return "tx_" + std::to_string(millis) + "_" + std::to_string(rand());
}

ThemisGRPCServiceImpl::ThemisGRPCServiceImpl(
    std::shared_ptr<RocksDBWrapper> db,
    std::shared_ptr<MultiAgentOrchestrator> orchestrator)
    : db_(db), orchestrator_(orchestrator) {
    THEMIS_INFO("ThemisGRPCServiceImpl initialized");
}

::grpc::Status ThemisGRPCServiceImpl::Hello(
    ::grpc::ServerContext* context,
    const themis::wire::v1::HelloRequest* request,
    themis::wire::v1::HelloAck* response) {
    
    response->set_protocol_version(1);
#ifdef THEMIS_VERSION_STRING
    response->set_server_version("ThemisDB/" THEMIS_VERSION_STRING);
#else
    response->set_server_version("ThemisDB/1.4.0");
#endif
    response->add_capabilities("compression");
    response->add_capabilities("tls");
    response->add_capabilities("streaming");
    response->add_capabilities("multi_agent");
    response->set_auth_required(false);  // TODO: Implement auth
    response->set_session_id(generateSessionId());
    
    THEMIS_DEBUG("gRPC Hello: client={}, version={}", 
                 request->client_name(), request->client_version());
    
    return ::grpc::Status::OK;
}

::grpc::Status ThemisGRPCServiceImpl::Auth(
    ::grpc::ServerContext* context,
    const themis::wire::v1::AuthResponse* request,
    themis::wire::v1::AuthSuccess* response) {
    
    // TODO: Implement actual authentication
    response->set_user_id(request->username());
    response->add_roles("user");
    (*response->mutable_session_params())["namespace"] = request->namespace_();
    
    return ::grpc::Status::OK;
}

::grpc::Status ThemisGRPCServiceImpl::Get(
    ::grpc::ServerContext* context,
    const themis::wire::v1::GetRequest* request,
    themis::wire::v1::GetResponse* response) {
    
    std::string key = buildKey(request->model(), request->collection(), request->uuid());
    std::string value;
    
    rocksdb::Status status = db_->get(key, value);
    
    if (status.ok()) {
        response->set_found(true);
        response->set_entity(value);
        response->set_version(1);  // TODO: Implement MVCC version tracking
        response->set_timestamp_ns(getCurrentTimestampNs());
    } else if (status.IsNotFound()) {
        response->set_found(false);
    } else {
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, status.ToString());
    }
    
    return ::grpc::Status::OK;
}

::grpc::Status ThemisGRPCServiceImpl::Put(
    ::grpc::ServerContext* context,
    const themis::wire::v1::PutRequest* request,
    themis::wire::v1::PutResponse* response) {
    
    std::string key = buildKey(request->model(), request->collection(), request->uuid());
    
    rocksdb::Status status = db_->put(key, request->entity());
    
    if (status.ok()) {
        response->set_success(true);
        response->set_version(1);  // TODO: Implement MVCC version
        response->set_timestamp_ns(getCurrentTimestampNs());
    } else {
        response->set_success(false);
        response->set_error(status.ToString());
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, status.ToString());
    }
    
    return ::grpc::Status::OK;
}

::grpc::Status ThemisGRPCServiceImpl::Delete(
    ::grpc::ServerContext* context,
    const themis::wire::v1::DeleteRequest* request,
    themis::wire::v1::DeleteResponse* response) {
    
    std::string key = buildKey(request->model(), request->collection(), request->uuid());
    
    rocksdb::Status status = db_->delete_key(key);
    
    if (status.ok()) {
        response->set_success(true);
        response->set_deleted_version(1);  // TODO: Implement MVCC version
    } else {
        response->set_success(false);
        response->set_error(status.ToString());
        return ::grpc::Status(::grpc::StatusCode::INTERNAL, status.ToString());
    }
    
    return ::grpc::Status::OK;
}

::grpc::Status ThemisGRPCServiceImpl::BatchGet(
    ::grpc::ServerContext* context,
    const themis::wire::v1::BatchGetRequest* request,
    themis::wire::v1::BatchGetResponse* response) {
    
    uint32_t found_count = 0;
    uint32_t not_found_count = 0;
    
    for (const auto& uuid : request->uuids()) {
        std::string key = buildKey(request->model(), request->collection(), uuid);
        std::string value;
        
        auto* result = response->add_results();
        rocksdb::Status status = db_->get(key, value);
        
        if (status.ok()) {
            result->set_found(true);
            result->set_entity(value);
            result->set_version(1);
            result->set_timestamp_ns(getCurrentTimestampNs());
            found_count++;
        } else {
            result->set_found(false);
            not_found_count++;
        }
    }
    
    response->set_found_count(found_count);
    response->set_not_found_count(not_found_count);
    
    return ::grpc::Status::OK;
}

::grpc::Status ThemisGRPCServiceImpl::BatchPut(
    ::grpc::ServerContext* context,
    const themis::wire::v1::BatchPutRequest* request,
    themis::wire::v1::BatchPutResponse* response) {
    
    // Use WriteBatch for atomic commit
    auto batch = db_->createWriteBatch();
    
    uint32_t success_count = 0;
    uint32_t failure_count = 0;
    
    // Add all operations to batch
    for (const auto& item : request->items()) {
        std::string key = buildKey(request->model(), request->collection(), item.uuid());
        batch->put(key, item.entity());
    }
    
    // Atomic commit
    rocksdb::Status status = batch->commit();
    
    if (status.ok()) {
        success_count = request->items_size();
        
        // Add individual success responses
        for (int i = 0; i < request->items_size(); ++i) {
            auto* result = response->add_results();
            result->set_success(true);
            result->set_version(1);
            result->set_timestamp_ns(getCurrentTimestampNs());
        }
    } else {
        failure_count = request->items_size();
        
        // All failed due to atomic commit failure
        for (int i = 0; i < request->items_size(); ++i) {
            auto* result = response->add_results();
            result->set_success(false);
            result->set_error(status.ToString());
        }
    }
    
    response->set_success_count(success_count);
    response->set_failure_count(failure_count);
    
    return ::grpc::Status::OK;
}

::grpc::Status ThemisGRPCServiceImpl::Query(
    ::grpc::ServerContext* context,
    const themis::wire::v1::QueryRequest* request,
    themis::wire::v1::QueryResult* response) {
    
    // TODO: Implement AQL query execution
    response->set_has_more(false);
    response->set_total_count(0);
    response->set_query_time_us(0);
    
    THEMIS_WARN("gRPC Query not yet implemented");
    return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "Query not yet implemented");
}

::grpc::Status ThemisGRPCServiceImpl::VectorSearch(
    ::grpc::ServerContext* context,
    const themis::wire::v1::VectorSearchRequest* request,
    themis::wire::v1::VectorSearchResponse* response) {
    
    // TODO: Implement vector search with HNSW
    response->set_search_time_us(0);
    
    THEMIS_WARN("gRPC VectorSearch not yet implemented");
    return ::grpc::Status(::grpc::StatusCode::UNIMPLEMENTED, "VectorSearch not yet implemented");
}

::grpc::Status ThemisGRPCServiceImpl::TransactionBegin(
    ::grpc::ServerContext* context,
    const themis::wire::v1::TransactionBeginRequest* request,
    themis::wire::v1::TransactionBeginResponse* response) {
    
    // TODO: Implement MVCC transactions
    response->set_transaction_id(generateTransactionId());
    response->set_timestamp_ns(getCurrentTimestampNs());
    
    THEMIS_WARN("gRPC Transactions not yet fully implemented");
    return ::grpc::Status::OK;
}

::grpc::Status ThemisGRPCServiceImpl::TransactionCommit(
    ::grpc::ServerContext* context,
    const themis::wire::v1::TransactionCommitRequest* request,
    themis::wire::v1::TransactionCommitResponse* response) {
    
    // TODO: Implement transaction commit
    response->set_success(true);
    response->set_commit_timestamp_ns(getCurrentTimestampNs());
    
    return ::grpc::Status::OK;
}

::grpc::Status ThemisGRPCServiceImpl::TransactionAbort(
    ::grpc::ServerContext* context,
    const themis::wire::v1::TransactionAbortRequest* request,
    themis::wire::v1::TransactionAbortResponse* response) {
    
    // TODO: Implement transaction abort
    response->set_success(true);
    
    return ::grpc::Status::OK;
}

::grpc::Status ThemisGRPCServiceImpl::Ping(
    ::grpc::ServerContext* context,
    const themis::wire::v1::PingRequest* request,
    themis::wire::v1::PongResponse* response) {
    
    response->set_timestamp_ns(request->timestamp_ns());
    response->set_server_timestamp_ns(getCurrentTimestampNs());
    
    return ::grpc::Status::OK;
}

// Helper methods
std::string ThemisGRPCServiceImpl::buildKey(
    const std::string& model,
    const std::string& collection,
    const std::string& uuid) {
    return model + ":" + collection + ":" + uuid;
}

bool ThemisGRPCServiceImpl::validateAuth(::grpc::ServerContext* context) {
    // TODO: Implement authentication validation
    return true;
}

uint64_t ThemisGRPCServiceImpl::getCurrentTimestampNs() {
    using namespace std::chrono;
    return duration_cast<nanoseconds>(
        system_clock::now().time_since_epoch()
    ).count();
}

// GRPCServerManager implementation
GRPCServerManager::GRPCServerManager(
    const std::string& server_address,
    std::shared_ptr<RocksDBWrapper> db,
    std::shared_ptr<MultiAgentOrchestrator> orchestrator)
    : server_address_(server_address),
      db_(db),
      orchestrator_(orchestrator) {
}

GRPCServerManager::~GRPCServerManager() {
    stop();
}

void GRPCServerManager::start() {
    if (running_) {
        THEMIS_WARN("gRPC server already running");
        return;
    }
    
    service_ = std::make_unique<ThemisGRPCServiceImpl>(db_, orchestrator_);
    
    ::grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address_, ::grpc::InsecureServerCredentials());
    builder.RegisterService(service_.get());
    
    // Performance optimizations
    builder.SetMaxReceiveMessageSize(100 * 1024 * 1024);  // 100MB
    builder.SetMaxSendMessageSize(100 * 1024 * 1024);     // 100MB
    
    server_ = builder.BuildAndStart();
    running_ = true;
    
    THEMIS_INFO("gRPC server listening on {}", server_address_);
}

void GRPCServerManager::stop() {
    if (server_ && running_) {
        THEMIS_INFO("Stopping gRPC server");
        server_->Shutdown();
        running_ = false;
    }
}

void GRPCServerManager::waitForShutdown() {
    if (server_) {
        server_->Wait();
    }
}

bool GRPCServerManager::isRunning() const {
    return running_;
}

} // namespace grpc_service
} // namespace themis
