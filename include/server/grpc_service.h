// ThemisDB gRPC Service Implementation
// Version: 1.0.0
// Date: December 25, 2024
// Based on: themis_wire_v1.proto

#ifndef THEMIS_GRPC_SERVICE_H
#define THEMIS_GRPC_SERVICE_H

#include <grpcpp/grpcpp.h>
#include "themis_wire_v1.grpc.pb.h"
#include "rocksdb_wrapper.h"
#include "multi_agent_orchestrator.h"
#include <memory>
#include <string>

namespace themis {
namespace grpc_service {

class ThemisGRPCServiceImpl final : public themis::wire::v1::ThemisService::Service {
public:
    explicit ThemisGRPCServiceImpl(
        std::shared_ptr<RocksDBWrapper> db,
        std::shared_ptr<MultiAgentOrchestrator> orchestrator = nullptr
    );
    
    // Connection Lifecycle
    ::grpc::Status Hello(
        ::grpc::ServerContext* context,
        const themis::wire::v1::HelloRequest* request,
        themis::wire::v1::HelloAck* response) override;
    
    ::grpc::Status Auth(
        ::grpc::ServerContext* context,
        const themis::wire::v1::AuthResponse* request,
        themis::wire::v1::AuthSuccess* response) override;
    
    // CRUD Operations
    ::grpc::Status Get(
        ::grpc::ServerContext* context,
        const themis::wire::v1::GetRequest* request,
        themis::wire::v1::GetResponse* response) override;
    
    ::grpc::Status Put(
        ::grpc::ServerContext* context,
        const themis::wire::v1::PutRequest* request,
        themis::wire::v1::PutResponse* response) override;
    
    ::grpc::Status Delete(
        ::grpc::ServerContext* context,
        const themis::wire::v1::DeleteRequest* request,
        themis::wire::v1::DeleteResponse* response) override;
    
    // Batch Operations (WriteBatch integration)
    ::grpc::Status BatchGet(
        ::grpc::ServerContext* context,
        const themis::wire::v1::BatchGetRequest* request,
        themis::wire::v1::BatchGetResponse* response) override;
    
    ::grpc::Status BatchPut(
        ::grpc::ServerContext* context,
        const themis::wire::v1::BatchPutRequest* request,
        themis::wire::v1::BatchPutResponse* response) override;
    
    // Query Operations
    ::grpc::Status Query(
        ::grpc::ServerContext* context,
        const themis::wire::v1::QueryRequest* request,
        themis::wire::v1::QueryResult* response) override;
    
    // Vector Search
    ::grpc::Status VectorSearch(
        ::grpc::ServerContext* context,
        const themis::wire::v1::VectorSearchRequest* request,
        themis::wire::v1::VectorSearchResponse* response) override;
    
    // Transaction Support
    ::grpc::Status TransactionBegin(
        ::grpc::ServerContext* context,
        const themis::wire::v1::TransactionBeginRequest* request,
        themis::wire::v1::TransactionBeginResponse* response) override;
    
    ::grpc::Status TransactionCommit(
        ::grpc::ServerContext* context,
        const themis::wire::v1::TransactionCommitRequest* request,
        themis::wire::v1::TransactionCommitResponse* response) override;
    
    ::grpc::Status TransactionAbort(
        ::grpc::ServerContext* context,
        const themis::wire::v1::TransactionAbortRequest* request,
        themis::wire::v1::TransactionAbortResponse* response) override;
    
    // Utility
    ::grpc::Status Ping(
        ::grpc::ServerContext* context,
        const themis::wire::v1::PingRequest* request,
        themis::wire::v1::PongResponse* response) override;
    
private:
    std::shared_ptr<RocksDBWrapper> db_;
    std::shared_ptr<MultiAgentOrchestrator> orchestrator_;
    
    // Helper methods
    std::string buildKey(const std::string& model, const std::string& collection, const std::string& uuid);
    bool validateAuth(::grpc::ServerContext* context);
    uint64_t getCurrentTimestampNs();
};

// gRPC Server Manager
class GRPCServerManager {
public:
    GRPCServerManager(
        const std::string& server_address,
        std::shared_ptr<RocksDBWrapper> db,
        std::shared_ptr<MultiAgentOrchestrator> orchestrator = nullptr
    );
    
    ~GRPCServerManager();
    
    // Start gRPC server
    void start();
    
    // Stop gRPC server
    void stop();
    
    // Wait for server to terminate
    void waitForShutdown();
    
    // Check if server is running
    bool isRunning() const;
    
private:
    std::string server_address_;
    std::shared_ptr<RocksDBWrapper> db_;
    std::shared_ptr<MultiAgentOrchestrator> orchestrator_;
    std::unique_ptr<ThemisGRPCServiceImpl> service_;
    std::unique_ptr<::grpc::Server> server_;
    bool running_{false};
};

} // namespace grpc_service
} // namespace themis

#endif // THEMIS_GRPC_SERVICE_H
