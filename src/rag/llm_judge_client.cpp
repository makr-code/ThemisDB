/**
 * @file llm_judge_client.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=19, M=4, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/llm_judge_client.h"
#include <stdexcept>
#include "llm/inference_engine_enhanced.h"
#include "llm/llama_wrapper.h"
#include "utils/logger.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <filesystem>
#include <cstdlib>
#include <array>
#include <optional>
#include <sstream>
#include <iomanip>
#include <atomic>
#include <unordered_set>
#include <mutex>
#include <cctype>

using json = nlohmann::json;

namespace themis::rag::judge {

namespace {
namespace fs = std::filesystem;

bool isModelFile(const fs::path& path) {
    if (!fs::exists(path) || !fs::is_regular_file(path)) {
        return false;
    }

    const auto extension = path.extension().string();
    return extension == ".gguf" || extension == ".bin";
}

std::vector<fs::path> candidateModelDirs() {
    std::vector<fs::path> dirs;

    if (const char* env_dir = std::getenv("THEMIS_LLM_MODELS_PATH");
        env_dir != nullptr && *env_dir != '\0') {
        dirs.emplace_back(env_dir);
    }

    const auto cwd = fs::current_path();
    constexpr std::array<std::string_view, 5> relative_dirs = {
        "models",
        "../models",
        "../../models",
        "../../../models",
        "../../../../models",
    };

    for (const auto relative_dir : relative_dirs) {
        dirs.emplace_back(cwd / relative_dir);
    }

    return dirs;
}

std::vector<std::string> candidateModelNames(const std::string& model_name) {
    if (model_name.empty()) {
        return {};
    }

    std::vector<std::string> names;
    names.push_back(model_name);

    const fs::path as_path(model_name);
    if (as_path.extension().empty()) {
        names.push_back(model_name + ".gguf");
        names.push_back(model_name + ".bin");
    }

    return names;
}

std::vector<fs::path> resolveLocalModelPaths(const std::string& model_name) {
    std::vector<fs::path> candidates;
    std::unordered_set<std::string> seen;

    const auto push_unique_if_model = [&](const fs::path& path) {
        if (!isModelFile(path)) {
            return;
        }

        const auto normalized = path.lexically_normal().string();
        if (seen.insert(normalized).second) {
            candidates.push_back(path);
        }
    };

    if (const char* explicit_path = std::getenv("THEMIS_LLM_MODEL_PATH");
        explicit_path != nullptr && *explicit_path != '\0') {
        push_unique_if_model(fs::path(explicit_path));
    }

    if (!model_name.empty()) {
        push_unique_if_model(fs::path(model_name));
    }

    const auto model_dirs = candidateModelDirs();
    const auto model_names = candidateModelNames(model_name);

    for (const auto& dir : model_dirs) {
        if (!fs::exists(dir) || !fs::is_directory(dir)) {
            continue;
        }

        for (const auto& name : model_names) {
            push_unique_if_model(dir / name);
        }
    }

    for (const auto& dir : model_dirs) {
        if (!fs::exists(dir) || !fs::is_directory(dir)) {
            continue;
        }

        try {
            // Store iterator to avoid potential temporary issues
            fs::directory_iterator dir_iter(dir);
            for (const auto& entry : dir_iter) {
                if (isModelFile(entry.path())) {
                    push_unique_if_model(entry.path());
                }
            }
        } catch (const std::exception& e) {
            THEMIS_WARN("LLMJudgeClient: Error iterating directory {}: {}", dir.string(), e.what());
            continue;
        }
    }

    return candidates;
}

std::string normalizeExpectedSha256(std::string sidecar_line) {
    sidecar_line.erase(
        std::remove_if(sidecar_line.begin(), sidecar_line.end(), [](unsigned char ch) {
            return std::isspace(ch);
        }),
        sidecar_line.end()
    );

    if (sidecar_line.size() >= 64) {
        sidecar_line = sidecar_line.substr(0, 64);
    }

    std::transform(sidecar_line.begin(), sidecar_line.end(), sidecar_line.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return sidecar_line;
}
} // namespace

// ═══════════════════════════════════════════════════════════
// LLMJudgeClient Implementation
// ═══════════════════════════════════════════════════════════

struct LLMJudgeClient::Impl {
    Config config;
    std::shared_ptr<llm::InferenceEngineEnhanced> inference_engine;
    std::string model_id;
    mutable std::mutex state_mutex;  // Protect shared state access

    void tryAutoRegisterLocalModel() {
        if (const char* disable_auto_register = std::getenv("THEMIS_DISABLE_LLM_AUTO_REGISTER");
            disable_auto_register != nullptr &&
            (std::string_view(disable_auto_register) == "1" ||
             std::string_view(disable_auto_register) == "true" ||
             std::string_view(disable_auto_register) == "TRUE")) {
            THEMIS_INFO("LLMJudgeClient: local model auto-registration disabled by THEMIS_DISABLE_LLM_AUTO_REGISTER");
            return;
        }

        const auto model_paths = resolveLocalModelPaths(config.model_name);
        if (model_paths.empty()) {
            THEMIS_DEBUG("LLMJudgeClient: no local model candidate found for '{}'", config.model_name);
            return;
        }

        json plugin_config;
        plugin_config["context_length"] = 4096;

        for (const auto& model_path : model_paths) {
            llm::LlamaWrapper::Config wrapper_config;
            wrapper_config.enable_response_cache = false;
            wrapper_config.use_continuous_batching = false;
            auto plugin = std::make_shared<llm::LlamaWrapper>(wrapper_config);

            try {
                // Verify model integrity via SHA-256 sidecar if available
                std::string sha_path = model_path.string() + ".sha256";
                if (std::filesystem::exists(sha_path)) {
                    std::ifstream sidecar(sha_path);
                    if (sidecar.is_open()) {
                        std::string expected_hash;
                        std::getline(sidecar, expected_hash);
                        sidecar.close();

                        expected_hash = normalizeExpectedSha256(expected_hash);
                        std::string actual_hash = themis::utils::calculateSHA256(model_path.string());
                        if (actual_hash.empty() || expected_hash.empty()) {
                            THEMIS_WARN("LLMJudgeClient: model integrity check unavailable for {} (empty hash)", model_path.string());
                            continue;
                        }
                        if (actual_hash != expected_hash) {
                            THEMIS_WARN("LLMJudgeClient: model integrity check failed for {}", model_path.string());
                            continue;
                        }
                    }
                }
                
                if (!plugin->loadModel(model_path.string(), plugin_config)) {
                    THEMIS_WARN("LLMJudgeClient: failed to load local judge model candidate {}", model_path.string());
                    continue;
                }

                inference_engine->registerModel(config.model_name, plugin);
                model_id = config.model_name;
                THEMIS_INFO("LLMJudgeClient auto-registered local model '{}' from {}",
                            config.model_name, model_path.string());
                return;
            } catch (const std::exception& ex) {
                THEMIS_WARN("LLMJudgeClient: local model auto-registration failed for {}: {}",
                            model_path.string(), ex.what());
            }
        }

        THEMIS_WARN("LLMJudgeClient: found {} local model candidate(s), but none could be loaded for '{}'",
                    model_paths.size(), config.model_name);
    }
    
    Impl(const Config& cfg) : config(cfg) {
        // Initialize inference engine with appropriate config
        llm::InferenceEngineEnhanced::Config engine_config;
        engine_config.enable_context_caching = config.enable_caching;
        engine_config.enable_batch_processing = config.enable_batching;
        engine_config.max_batch_size = config.batch_size;
        engine_config.batch_timeout_ms = config.batch_timeout_ms;
        
        inference_engine = std::make_shared<llm::InferenceEngineEnhanced>(engine_config);
        inference_engine->start();
        tryAutoRegisterLocalModel();
        
        THEMIS_INFO("LLMJudgeClient initialized with model: {}", config.model_name);
    }
    
    ~Impl() {
        if (inference_engine) {
            inference_engine->shutdown();
        }
    }
};

LLMJudgeClient::LLMJudgeClient()
    : LLMJudgeClient(Config{}) {
}

LLMJudgeClient::LLMJudgeClient(const Config& config)
    : impl_(std::make_unique<Impl>(config)) {
}

LLMJudgeClient::~LLMJudgeClient() = default;

std::string LLMJudgeClient::evaluate(const std::string& prompt) {
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        // Create inference request
        llm::InferenceEngineEnhanced::EnhancedInferenceRequest request;
        request.base_request.prompt = prompt;
        request.base_request.max_tokens = impl_->config.max_tokens;
        request.base_request.temperature = static_cast<float>(impl_->config.temperature);
        request.base_request.top_p = 0.95f;
        request.base_request.stop_sequences = impl_->config.stop_sequences;
        
        request.priority = impl_->config.priority;
        request.timeout = std::chrono::milliseconds(impl_->config.timeout_ms);
        request.allow_caching = impl_->config.enable_caching;
        request.preferred_model_id = impl_->config.model_name;
        request.request_id = generateRequestId();
        request.submitted_at = std::chrono::steady_clock::now();
        
        // Submit request and wait for response
        llm::InferenceHandle handle;
        {
            std::lock_guard<std::mutex> lock(impl_->state_mutex);
            handle = impl_->inference_engine->submit(request);
        }
        auto response = handle.get();
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);
        
        THEMIS_DEBUG("LLM evaluation completed in {}ms", duration.count());
        
        return response.text;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("LLM evaluation failed: {}", e.what());
        throw;
    }
}

std::vector<std::string> LLMJudgeClient::evaluateBatch(
    const std::vector<std::string>& prompts
) {
    std::vector<std::string> results;
    results.reserve(prompts.size());
    
    if (!impl_->config.enable_batching || prompts.size() == 1) {
        // Sequential processing
        for (const auto& prompt : prompts) {
            results.push_back(evaluate(prompt));
        }
        return results;
    }
    
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        // Submit all requests
        std::vector<llm::InferenceHandle> handles;
        handles.reserve(prompts.size());
        
        for (const auto& prompt : prompts) {
            llm::InferenceEngineEnhanced::EnhancedInferenceRequest request;
            request.base_request.prompt = prompt;
            request.base_request.max_tokens = impl_->config.max_tokens;
            request.base_request.temperature = static_cast<float>(impl_->config.temperature);
            request.base_request.top_p = 0.95f;
            request.base_request.stop_sequences = impl_->config.stop_sequences;
            
            request.priority = impl_->config.priority;
            request.timeout = std::chrono::milliseconds(impl_->config.timeout_ms);
            request.allow_caching = impl_->config.enable_caching;
            request.preferred_model_id = impl_->config.model_name;
            request.request_id = generateRequestId();
            request.submitted_at = std::chrono::steady_clock::now();
            
            {
                std::lock_guard<std::mutex> lock(impl_->state_mutex);
                handles.push_back(impl_->inference_engine->submit(request));
            }
        }
        
        // Wait for all responses
        for (auto& handle : handles) {
            auto response = handle.get();
            results.push_back(response.text);
        }
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);
        
        THEMIS_INFO("Batch evaluation of {} prompts completed in {}ms",
                   prompts.size(), duration.count());
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Batch evaluation failed: {}", e.what());
        throw;
    }
    
    return results;
}

EvaluationResponse LLMJudgeClient::evaluateDimension(
    const std::string& query,
    const std::string& answer,
    const std::vector<std::pair<std::string, std::string>>& documents,
    const std::string& dimension
) {
    // Build evaluation prompt
    std::ostringstream prompt;
    
    prompt << "You are evaluating a generated answer for a RAG system.\n\n";
    prompt << "Query: " << query << "\n\n";
    prompt << "Generated Answer: " << answer << "\n\n";
    
    if (!documents.empty()) {
        prompt << "Retrieved Documents:\n";
        size_t doc_count = std::min(documents.size(), size_t(5));
        for (size_t i = 0; i < doc_count; i++) {
            prompt << "Document " << (i+1) << " (ID: " << documents[i].first << "):\n";
            prompt << documents[i].second.substr(0, 500);
            if (documents[i].second.size() > 500) {
                prompt << "...";
            }
            prompt << "\n\n";
        }
    }
    
    prompt << "Evaluate the answer for: " << dimension << "\n\n";
    prompt << "Provide your evaluation in the following JSON format:\n";
    prompt << "{\n";
    prompt << "  \"score\": <float 0.0-1.0>,\n";
    prompt << "  \"reasoning\": \"<explanation>\",\n";
    prompt << "  \"confidence\": <float 0.0-1.0>\n";
    prompt << "}\n";
    
    auto start_time = std::chrono::steady_clock::now();
    
    try {
        std::string llm_response = evaluate(prompt.str());
        
        auto end_time = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            end_time - start_time);
        
        // Parse JSON response
        EvaluationResponse response;
        response.raw_response = llm_response;
        response.evaluation_time = duration;
        
        // Simple JSON parsing (in production, use a proper JSON library)
        parseEvaluationResponse(llm_response, response);
        
        return response;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("Dimension evaluation failed: {}", e.what());
        
        EvaluationResponse error_response;
        error_response.score = 0.0;
        error_response.reasoning = std::string("Evaluation failed: ") + e.what();
        error_response.confidence = 0.0;
        return error_response;
    }
}

void LLMJudgeClient::setInferenceEngine(
    std::shared_ptr<llm::InferenceEngineEnhanced> engine
) {
    impl_->inference_engine = engine;
    THEMIS_INFO("Custom inference engine set");
}

void LLMJudgeClient::registerModel(
    const std::string& model_id,
    std::shared_ptr<llm::ILLMPlugin> plugin
) {
    if (impl_->inference_engine) {
        impl_->inference_engine->registerModel(model_id, plugin);
        impl_->model_id = model_id;
        THEMIS_INFO("Model registered: {}", model_id);
    }
}

LLMJudgeClient::Config LLMJudgeClient::getConfig() const {
    return impl_->config;
}

void LLMJudgeClient::setConfig(const Config& config) {
    impl_->config = config;
}

std::string LLMJudgeClient::generateRequestId() {
    static std::atomic<uint64_t> counter{0};
    auto count = counter.fetch_add(1);
    
    std::ostringstream oss;
    oss << "llm_judge_" << std::setfill('0') << std::setw(10) << count;
    return oss.str();
}

void LLMJudgeClient::parseEvaluationResponse(
    const std::string& response,
    EvaluationResponse& parsed
) {
    // Use nlohmann/json for proper JSON parsing
    try {
        json j = json::parse(response);
        
        if (j.contains("score")) {
            parsed.score = j["score"].get<double>();
        } else {
            parsed.score = 0.5;  // Default
        }
        
        if (j.contains("reasoning")) {
            parsed.reasoning = j["reasoning"].get<std::string>();
        }
        
        if (j.contains("confidence")) {
            parsed.confidence = j["confidence"].get<double>();
        } else {
            parsed.confidence = 0.5;  // Default
        }
        
    } catch (const json::exception&) {
        // Fallback to simple parsing for non-JSON responses
        // Look for score
    size_t score_pos = response.find("\"score\"");
    if (score_pos != std::string::npos) {
        size_t colon_pos = response.find(":", score_pos);
        if (colon_pos != std::string::npos) {
            size_t comma_pos = response.find(",", colon_pos);
            if (comma_pos == std::string::npos) {
                comma_pos = response.find("}", colon_pos);
            }
            if (comma_pos != std::string::npos) {
                std::string score_str = response.substr(colon_pos + 1, 
                                                       comma_pos - colon_pos - 1);
                // Trim whitespace
                score_str.erase(0, score_str.find_first_not_of(" \t\n\r"));
                score_str.erase(score_str.find_last_not_of(" \t\n\r,") + 1);
                
                try {
                    parsed.score = std::stod(score_str);
                } catch (const std::exception& e) {
                    THEMIS_WARN("Failed to parse score '{}': {}", score_str, e.what());
                    parsed.score = 0.5; // Default
                }
            }
        }
    }
    
    // Look for reasoning
    size_t reasoning_pos = response.find("\"reasoning\"");
    if (reasoning_pos != std::string::npos) {
        size_t colon_pos = response.find(":", reasoning_pos);
        if (colon_pos != std::string::npos) {
            size_t quote1 = response.find("\"", colon_pos);
            if (quote1 != std::string::npos) {
                size_t quote2 = response.find("\"", quote1 + 1);
                if (quote2 != std::string::npos) {
                    parsed.reasoning = response.substr(quote1 + 1, 
                                                      quote2 - quote1 - 1);
                }
            }
        }
    }
}

} // namespace themis::rag::judge
