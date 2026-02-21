/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            llm_grpc_service.h                                 ║
  Version:         0.0.10                                             ║
  Last Modified:   2026-02-21 13:56:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     156                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "llm/llm_plugin_manager.h"
#include "proto/llm_service.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <memory>
#include <string>

namespace themis::server {

// gRPC service implementation for LLM operations
// Provides high-performance binary protocol access to all LLM capabilities
class LLMGrpcService final : public llm::LLMService::Service {
public:
    explicit LLMGrpcService(std::shared_ptr<llm::LLMPluginManager> plugin_manager);
    ~LLMGrpcService() override = default;

    // Inference operations
    grpc::Status Inference(
        grpc::ServerContext* context,
        const llm::InferenceRequest* request,
        llm::InferenceResponse* response) override;

    grpc::Status RAGInference(
        grpc::ServerContext* context,
        const llm::RAGRequest* request,
        llm::InferenceResponse* response) override;

    grpc::Status GenerateEmbedding(
        grpc::ServerContext* context,
        const llm::EmbeddingRequest* request,
        llm::EmbeddingResponse* response) override;

    grpc::Status StreamInference(
        grpc::ServerContext* context,
        const llm::InferenceRequest* request,
        grpc::ServerWriter<llm::TokenResponse>* writer) override;

    // Model management
    grpc::Status ListModels(
        grpc::ServerContext* context,
        const llm::ListModelsRequest* request,
        llm::ModelListResponse* response) override;

    grpc::Status LoadModel(
        grpc::ServerContext* context,
        const llm::ModelLoadRequest* request,
        llm::ModelOperationResponse* response) override;

    grpc::Status UnloadModel(
        grpc::ServerContext* context,
        const llm::ModelUnloadRequest* request,
        llm::ModelOperationResponse* response) override;

    grpc::Status GetModelInfo(
        grpc::ServerContext* context,
        const llm::ModelInfoRequest* request,
        llm::ModelInfo* response) override;

    grpc::Status IngestModel(
        grpc::ServerContext* context,
        grpc::ServerReader<llm::ModelChunk>* reader,
        llm::ModelOperationResponse* response) override;

    // LoRA adapter management
    grpc::Status ListLoRAs(
        grpc::ServerContext* context,
        const llm::ListLoRAsRequest* request,
        llm::LoRAListResponse* response) override;

    grpc::Status LoadLoRA(
        grpc::ServerContext* context,
        const llm::LoRALoadRequest* request,
        llm::LoRAOperationResponse* response) override;

    grpc::Status UnloadLoRA(
        grpc::ServerContext* context,
        const llm::LoRAUnloadRequest* request,
        llm::LoRAOperationResponse* response) override;

    // Statistics and health
    grpc::Status GetStatistics(
        grpc::ServerContext* context,
        const llm::StatisticsRequest* request,
        llm::StatisticsResponse* response) override;

    grpc::Status GetCacheStatistics(
        grpc::ServerContext* context,
        const llm::CacheStatisticsRequest* request,
        llm::CacheStatisticsResponse* response) override;

    grpc::Status ClearCache(
        grpc::ServerContext* context,
        const llm::ClearCacheRequest* request,
        llm::ClearCacheResponse* response) override;

    grpc::Status HealthCheck(
        grpc::ServerContext* context,
        const llm::HealthCheckRequest* request,
        llm::HealthCheckResponse* response) override;

    // Distributed operations
    grpc::Status ExportLoRA(
        grpc::ServerContext* context,
        const llm::LoRAExportRequest* request,
        grpc::ServerWriter<llm::LoRAChunk>* writer) override;

    grpc::Status ImportLoRA(
        grpc::ServerContext* context,
        grpc::ServerReader<llm::LoRAChunk>* reader,
        llm::LoRAOperationResponse* response) override;

private:
    std::shared_ptr<llm::LLMPluginManager> plugin_manager_;

    // Helper methods
    bool validateBearerToken(grpc::ServerContext* context);
    std::string extractBearerToken(grpc::ServerContext* context);
    
    // Conversion helpers between protobuf and internal types
    void convertToInternalRequest(
        const llm::InferenceRequest& pb_req,
        ::themis::llm::InferenceRequest& internal_req);
    
    void convertToProtoResponse(
        const ::themis::llm::InferenceResponse& internal_resp,
        llm::InferenceResponse& pb_resp);
};

} // namespace themis::server
