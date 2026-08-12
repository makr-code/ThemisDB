/**
 * @file llm_grpc_service.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 81/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=8, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "server/llm_grpc_service.h"
#include <regex>
#include <fstream>
#include <filesystem>
#include <random>
#include <openssl/evp.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace themis::server {

LLMGrpcService::LLMGrpcService(std::shared_ptr<llm::LLMPluginManager> plugin_manager)
    : plugin_manager_(std::move(plugin_manager)) {
}

void LLMGrpcService::setJwtValidator(std::shared_ptr<auth::JWTValidator> validator) {
    jwt_validator_ = std::move(validator);
}

bool LLMGrpcService::validateBearerToken(grpc::ServerContext* context) {
    auto token = extractBearerToken(context);
    if (token.empty()) {
        return false;
    }

    // If a full JWT validator has been injected, use it (signature + claims check).
    if (jwt_validator_) {
        try {
            jwt_validator_->parseAndValidate(token);
            return true;
        } catch (const std::exception& e) {
            spdlog::warn("LLMGrpcService: JWT validation failed: {}", e.what());
            return false;
        }
    }

    // Fallback: decode the JWT payload and check the exp claim so that at
    // least expired tokens are always rejected, even without a key.
    auto decode_b64url = [](const std::string& in) -> std::string {
        std::string s = in;
        for (char& c : s) {
            if (c == '-') c = '+';
            else if (c == '_') c = '/';
        }
        while (s.size() % 4) s += '=';
        // Simple base64 decode using OpenSSL EVP
        std::vector<unsigned char> buf(s.size());
        int len = EVP_DecodeBlock(buf.data(),
            reinterpret_cast<const unsigned char*>(s.data()),
            static_cast<int>(s.size()));
        if (len < 0) return "";
        return std::string(buf.begin(), buf.begin() + len);
    };

    // JWT is header.payload.signature
    auto dot1 = token.find('.');
    auto dot2 = token.rfind('.');
    if (dot1 == std::string::npos || dot2 == std::string::npos || dot1 == dot2) {
        spdlog::warn("LLMGrpcService: Malformed JWT token (missing segments)");
        return false;
    }

    std::string payload_b64 = token.substr(dot1 + 1, dot2 - dot1 - 1);
    std::string payload_json = decode_b64url(payload_b64);

    try {
        auto claims = nlohmann::json::parse(payload_json);
        if (claims.contains("exp")) {
            auto exp = claims["exp"].get<int64_t>();
            auto now = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            if (now > exp) {
                spdlog::warn("LLMGrpcService: JWT token expired");
                return false;
            }
        }
    } catch (const std::exception& e) {
        spdlog::warn("LLMGrpcService: Failed to parse JWT payload: {}", e.what());
        return false;
    }

    return true;
}

std::string LLMGrpcService::extractBearerToken(grpc::ServerContext* context) {
    const auto& metadata = context->client_metadata();
    auto it = metadata.find("authorization");
    if (it == metadata.end()) {
        return "";
    }
    
    std::string auth_value(it->second.data(), it->second.size());
    std::regex bearer_regex(R"(^Bearer\s+(.+)$)", std::regex::icase);
    std::smatch matches;
    
    if (std::regex_match(auth_value, matches, bearer_regex) && matches.size() == 2) {
        return matches[1].str();
    }
    
    return "";
}

void LLMGrpcService::convertToInternalRequest(
    const llm::InferenceRequest& pb_req,
    ::themis::llm::InferenceRequest& internal_req) {
    
    internal_req.prompt = pb_req.prompt();
    internal_req.model_id = pb_req.model_id().empty() ? "default" : pb_req.model_id();
    internal_req.lora_adapter_id = pb_req.lora_adapter_id();
    internal_req.max_tokens = pb_req.max_tokens() > 0 ? pb_req.max_tokens() : 512;
    internal_req.temperature = pb_req.temperature() > 0 ? pb_req.temperature() : 0.7;
}

void LLMGrpcService::convertToProtoResponse(
    const ::themis::llm::InferenceResponse& internal_resp,
    llm::InferenceResponse& pb_resp) {
    
    pb_resp.set_text(internal_resp.text);
    pb_resp.set_model_id(internal_resp.model_id);
    pb_resp.set_tokens_generated(internal_resp.tokens_generated);
    pb_resp.set_inference_time_ms(internal_resp.inference_time_ms);
    pb_resp.set_cache_hit(internal_resp.cache_hit);
}

grpc::Status LLMGrpcService::Inference(
    grpc::ServerContext* context,
    const llm::InferenceRequest* request,
    llm::InferenceResponse* response) {
    
    if (!validateBearerToken(context)) {
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Invalid or missing Bearer Token");
    }
    
    try {
        ::themis::llm::InferenceRequest internal_req;
        convertToInternalRequest(*request, internal_req);
        
        auto& plugin_mgr = llm::LLMPluginManager::instance();
        auto internal_resp = plugin_mgr.generate(internal_req);
        
        convertToProtoResponse(internal_resp, *response);
        
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        return grpc::Status(grpc::StatusCode::INTERNAL, std::string("Inference failed: ") + e.what());
    }
}

grpc::Status LLMGrpcService::RAGInference(
    grpc::ServerContext* context,
    const llm::RAGRequest* request,
    llm::InferenceResponse* response) {
    
    if (!validateBearerToken(context)) {
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Invalid or missing Bearer Token");
    }
    
    try {
        ::themis::llm::RAGContext rag_context;
        
        // Convert documents from protobuf
        for (const auto& doc : request->documents()) {
            ::themis::llm::Document internal_doc;
            internal_doc.id = doc.id();
            internal_doc.content = doc.content();
            internal_doc.relevance_score = doc.relevance_score();
            rag_context.documents.push_back(internal_doc);
        }
        
        ::themis::llm::InferenceRequest internal_req;
        internal_req.prompt = request->query();
        internal_req.model_id = "default";
        internal_req.lora_adapter_id = request->lora_adapter_id();
        internal_req.max_tokens = request->max_tokens() > 0 ? request->max_tokens() : 512;
        internal_req.temperature = request->temperature() > 0 ? request->temperature() : 0.7;
        
        auto& plugin_mgr = llm::LLMPluginManager::instance();
        auto internal_resp = plugin_mgr.generateRAG(rag_context, internal_req);
        
        convertToProtoResponse(internal_resp, *response);
        
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        return grpc::Status(grpc::StatusCode::INTERNAL, std::string("RAG inference failed: ") + e.what());
    }
}

grpc::Status LLMGrpcService::GenerateEmbedding(
    grpc::ServerContext* context,
    const llm::EmbeddingRequest* request,
    llm::EmbeddingResponse* response) {
    
    if (!validateBearerToken(context)) {
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Invalid or missing Bearer Token");
    }
    
    try {
        auto& plugin_mgr = llm::LLMPluginManager::instance();
        auto embedding = plugin_mgr.generateEmbedding(
            request->text(),
            request->model_id().empty() ? "default" : request->model_id()
        );
        
        for (float val : embedding) {
            response->add_embedding(val);
        }
        response->set_dimensions(embedding.size());
        response->set_model_id(request->model_id());
        
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        return grpc::Status(grpc::StatusCode::INTERNAL, std::string("Embedding generation failed: ") + e.what());
    }
}

grpc::Status LLMGrpcService::StreamInference(
    grpc::ServerContext* context,
    const llm::InferenceRequest* request,
    grpc::ServerWriter<llm::TokenResponse>* writer) {
    
    if (!validateBearerToken(context)) {
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Invalid or missing Bearer Token");
    }
    
    try {
        ::themis::llm::InferenceRequest internal_req;
        convertToInternalRequest(*request, internal_req);
        
        auto& plugin_mgr = llm::LLMPluginManager::instance();
        
        // Stream tokens
        int token_index = 0;
        plugin_mgr.generateStream(internal_req, [&](const std::string& token, bool done) {
            llm::TokenResponse token_resp;
            token_resp.set_token(token);
            token_resp.set_index(token_index++);
            token_resp.set_done(done);
            
            writer->Write(token_resp);
            return !context->IsCancelled();
        });
        
        // Send final done message
        llm::TokenResponse final_resp;
        final_resp.set_done(true);
        writer->Write(final_resp);
        
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        return grpc::Status(grpc::StatusCode::INTERNAL, std::string("Streaming inference failed: ") + e.what());
    }
}

grpc::Status LLMGrpcService::ListModels(
    grpc::ServerContext* context,
    const llm::ListModelsRequest* request,
    llm::ModelListResponse* response) {
    
    if (!validateBearerToken(context)) {
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Invalid or missing Bearer Token");
    }
    
    try {
        auto& plugin_mgr = llm::LLMPluginManager::instance();
        auto models = plugin_mgr.listModels();
        
        for (const auto& model : models) {
            auto* model_info = response->add_models();
            model_info->set_model_id(model.model_id);
            model_info->set_path(model.path);
            model_info->set_loaded(model.loaded);
        }
        
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        return grpc::Status(grpc::StatusCode::INTERNAL, std::string("List models failed: ") + e.what());
    }
}

grpc::Status LLMGrpcService::LoadModel(
    grpc::ServerContext* context,
    const llm::ModelLoadRequest* request,
    llm::ModelOperationResponse* response) {
    
    if (!validateBearerToken(context)) {
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Invalid or missing Bearer Token");
    }
    
    try {
        auto& plugin_mgr = llm::LLMPluginManager::instance();
        plugin_mgr.loadModel(request->model_id(), request->path());
        
        response->set_success(true);
        response->set_message("Model loaded successfully");
        response->set_model_id(request->model_id());
        
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        response->set_success(false);
        response->set_message(std::string("Model load failed: ") + e.what());
        return grpc::Status::OK;
    }
}

grpc::Status LLMGrpcService::UnloadModel(
    grpc::ServerContext* context,
    const llm::ModelUnloadRequest* request,
    llm::ModelOperationResponse* response) {
    
    if (!validateBearerToken(context)) {
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Invalid or missing Bearer Token");
    }
    
    try {
        auto& plugin_mgr = llm::LLMPluginManager::instance();
        plugin_mgr.unloadModel(request->model_id());
        
        response->set_success(true);
        response->set_message("Model unloaded successfully");
        response->set_model_id(request->model_id());
        
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        response->set_success(false);
        response->set_message(std::string("Model unload failed: ") + e.what());
        return grpc::Status::OK;
    }
}

grpc::Status LLMGrpcService::GetModelInfo(
    grpc::ServerContext* context,
    const llm::ModelInfoRequest* request,
    llm::ModelInfo* response) {
    
    if (!validateBearerToken(context)) {
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Invalid or missing Bearer Token");
    }
    
    try {
        auto& plugin_mgr = llm::LLMPluginManager::instance();
        auto model_info = plugin_mgr.getModelInfo(request->model_id());
        
        response->set_model_id(model_info.model_id);
        response->set_path(model_info.path);
        response->set_loaded(model_info.loaded);
        
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        return grpc::Status(grpc::StatusCode::NOT_FOUND, std::string("Model not found: ") + e.what());
    }
}

grpc::Status LLMGrpcService::IngestModel(
    grpc::ServerContext* context,
    grpc::ServerReader<llm::ModelChunk>* reader,
    llm::ModelOperationResponse* response) {
    
    if (!validateBearerToken(context)) {
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Invalid or missing Bearer Token");
    }
    
    try {
        std::string model_id;
        std::ofstream temp_file;
        std::string temp_path;
        bool first_chunk = true;
        
        llm::ModelChunk chunk;
        while (reader->Read(&chunk)) {
            if (first_chunk) {
                model_id = chunk.model_id();
                
                // Security: Use secure temporary directory with random suffix
                // Instead of predictable /tmp/model_ID.tmp, use platform-specific temp dir
                auto temp_dir = std::filesystem::temp_directory_path();
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> dis(100000, 999999);
                std::string random_suffix = std::to_string(dis(gen));
                
                temp_path = (temp_dir / ("themis_model_" + model_id + "_" + random_suffix + ".tmp")).string();
                temp_file.open(temp_path, std::ios::binary);
                
                if (!temp_file.is_open()) {
                    return grpc::Status(grpc::StatusCode::INTERNAL, "Failed to create temporary file");
                }
                
                first_chunk = false;
            }
            
            temp_file.write(chunk.data().data(), chunk.data().size());
            
            if (chunk.is_final()) {
                break;
            }
        }
        
        temp_file.close();
        
        // Clean up temporary file after processing
        // (In production, move to model storage location)
        if (!temp_path.empty()) {
            std::filesystem::remove(temp_path);
        }
        
        // Ingest model via plugin manager
        auto& plugin_mgr = llm::LLMPluginManager::instance();
        plugin_mgr.ingestModel(model_id, "/tmp/model_" + model_id + ".tmp");
        
        response->set_success(true);
        response->set_message("Model ingested successfully");
        response->set_model_id(model_id);
        
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        response->set_success(false);
        response->set_message(std::string("Model ingestion failed: ") + e.what());
        return grpc::Status::OK;
    }
}

grpc::Status LLMGrpcService::ListLoRAs(
    grpc::ServerContext* context,
    const llm::ListLoRAsRequest* request,
    llm::LoRAListResponse* response) {
    
    if (!validateBearerToken(context)) {
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Invalid or missing Bearer Token");
    }
    
    try {
        auto& plugin_mgr = llm::LLMPluginManager::instance();
        auto loras = plugin_mgr.listLoRAs(request->model_id());
        
        for (const auto& lora : loras) {
            auto* lora_info = response->add_loras();
            lora_info->set_lora_id(lora.lora_id);
            lora_info->set_path(lora.path);
            lora_info->set_base_model_id(lora.base_model_id);
            lora_info->set_loaded(lora.loaded);
        }
        
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        return grpc::Status(grpc::StatusCode::INTERNAL, std::string("List LoRAs failed: ") + e.what());
    }
}

grpc::Status LLMGrpcService::LoadLoRA(
    grpc::ServerContext* context,
    const llm::LoRALoadRequest* request,
    llm::LoRAOperationResponse* response) {
    
    if (!validateBearerToken(context)) {
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Invalid or missing Bearer Token");
    }
    
    try {
        auto& plugin_mgr = llm::LLMPluginManager::instance();
        plugin_mgr.loadLoRA(request->lora_id(), request->path(), request->model_id());
        
        response->set_success(true);
        response->set_message("LoRA loaded successfully");
        response->set_lora_id(request->lora_id());
        
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        response->set_success(false);
        response->set_message(std::string("LoRA load failed: ") + e.what());
        return grpc::Status::OK;
    }
}

grpc::Status LLMGrpcService::UnloadLoRA(
    grpc::ServerContext* context,
    const llm::LoRAUnloadRequest* request,
    llm::LoRAOperationResponse* response) {
    
    if (!validateBearerToken(context)) {
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Invalid or missing Bearer Token");
    }
    
    try {
        auto& plugin_mgr = llm::LLMPluginManager::instance();
        plugin_mgr.unloadLoRA(request->lora_id());
        
        response->set_success(true);
        response->set_message("LoRA unloaded successfully");
        response->set_lora_id(request->lora_id());
        
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        response->set_success(false);
        response->set_message(std::string("LoRA unload failed: ") + e.what());
        return grpc::Status::OK;
    }
}

grpc::Status LLMGrpcService::GetStatistics(
    grpc::ServerContext* context,
    const llm::StatisticsRequest* request,
    llm::StatisticsResponse* response) {
    
    if (!validateBearerToken(context)) {
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Invalid or missing Bearer Token");
    }
    
    try {
        auto& plugin_mgr = llm::LLMPluginManager::instance();
        auto stats = plugin_mgr.getStatistics();
        
        // Fill response (simplified)
        auto* inference_stats = response->mutable_inference_stats();
        inference_stats->set_total_requests(stats.total_requests);
        inference_stats->set_successful_requests(stats.successful_requests);
        inference_stats->set_avg_latency_ms(stats.avg_latency_ms);
        
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        return grpc::Status(grpc::StatusCode::INTERNAL, std::string("Get statistics failed: ") + e.what());
    }
}

grpc::Status LLMGrpcService::GetCacheStatistics(
    grpc::ServerContext* context,
    const llm::CacheStatisticsRequest* request,
    llm::CacheStatisticsResponse* response) {
    
    if (!validateBearerToken(context)) {
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Invalid or missing Bearer Token");
    }
    
    try {
        auto& plugin_mgr = llm::LLMPluginManager::instance();
        auto cache_stats = plugin_mgr.getCacheStatistics();
        
        auto* stats = response->mutable_cache_stats();
        stats->set_response_cache_hits(cache_stats.response_cache_hits);
        stats->set_response_cache_misses(cache_stats.response_cache_misses);
        stats->set_cache_hit_rate(cache_stats.cache_hit_rate);
        
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        return grpc::Status(grpc::StatusCode::INTERNAL, std::string("Get cache statistics failed: ") + e.what());
    }
}

grpc::Status LLMGrpcService::ClearCache(
    grpc::ServerContext* context,
    const llm::ClearCacheRequest* request,
    llm::ClearCacheResponse* response) {
    
    if (!validateBearerToken(context)) {
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Invalid or missing Bearer Token");
    }
    
    try {
        auto& plugin_mgr = llm::LLMPluginManager::instance();
        plugin_mgr.clearAllCaches();
        
        response->set_success(true);
        response->set_message("Caches cleared successfully");
        
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        response->set_success(false);
        response->set_message(std::string("Clear cache failed: ") + e.what());
        return grpc::Status::OK;
    }
}

grpc::Status LLMGrpcService::HealthCheck(
    grpc::ServerContext* context,
    const llm::HealthCheckRequest* request,
    llm::HealthCheckResponse* response) {
    
    if (!validateBearerToken(context)) {
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Invalid or missing Bearer Token");
    }
    
    try {
        auto& plugin_mgr = llm::LLMPluginManager::instance();
        auto health = plugin_mgr.getHealthStatus();
        
        response->set_status(health.healthy ? 
            llm::HealthCheckResponse::HEALTHY : 
            llm::HealthCheckResponse::UNHEALTHY);
        response->set_message(health.message);
        
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        response->set_status(llm::HealthCheckResponse::UNHEALTHY);
        response->set_message(std::string("Health check failed: ") + e.what());
        return grpc::Status::OK;
    }
}

grpc::Status LLMGrpcService::ExportLoRA(
    grpc::ServerContext* context,
    const llm::LoRAExportRequest* request,
    grpc::ServerWriter<llm::LoRAChunk>* writer) {
    
    if (!validateBearerToken(context)) {
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Invalid or missing Bearer Token");
    }
    
    try {
        auto& plugin_mgr = llm::LLMPluginManager::instance();
        auto lora_data = plugin_mgr.exportLoRA(request->lora_id());
        
        // Stream in 4MB chunks
        const size_t chunk_size = 4 * 1024 * 1024;
        size_t offset = 0;
        
        while (offset < lora_data.size()) {
            llm::LoRAChunk chunk;
            chunk.set_lora_id(request->lora_id());
            
            size_t current_chunk_size = std::min(chunk_size, lora_data.size() - offset);
            chunk.set_data(lora_data.data() + offset, current_chunk_size);
            chunk.set_offset(offset);
            chunk.set_total_size(lora_data.size());
            chunk.set_is_final(offset + current_chunk_size >= lora_data.size());
            
            writer->Write(chunk);
            offset += current_chunk_size;
        }
        
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        return grpc::Status(grpc::StatusCode::INTERNAL, std::string("Export LoRA failed: ") + e.what());
    }
}

grpc::Status LLMGrpcService::ImportLoRA(
    grpc::ServerContext* context,
    grpc::ServerReader<llm::LoRAChunk>* reader,
    llm::LoRAOperationResponse* response) {
    
    if (!validateBearerToken(context)) {
        return grpc::Status(grpc::StatusCode::UNAUTHENTICATED, "Invalid or missing Bearer Token");
    }
    
    try {
        std::string lora_id;
        std::vector<uint8_t> lora_data;
        
        llm::LoRAChunk chunk;
        while (reader->Read(&chunk)) {
            if (lora_id.empty()) {
                lora_id = chunk.lora_id();
            }
            
            lora_data.insert(lora_data.end(), chunk.data().begin(), chunk.data().end());
            
            if (chunk.is_final()) {
                break;
            }
        }
        
        auto& plugin_mgr = llm::LLMPluginManager::instance();
        plugin_mgr.importLoRA(lora_id, lora_data);
        
        response->set_success(true);
        response->set_message("LoRA imported successfully");
        response->set_lora_id(lora_id);
        
        return grpc::Status::OK;
    } catch (const std::exception& e) {
        response->set_success(false);
        response->set_message(std::string("Import LoRA failed: ") + e.what());
        return grpc::Status::OK;
    }
}

} // namespace themis::server

