/**
 * @file ethics_ai_plugin.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "ethics_ai/ethics_ai_plugin_interface.h"
#include "philosophy_loader.h"
#include "argument_store.h"
#include "rag_context_engine.h"
#include "discourse_engine.h"
#include "ethics_evaluator.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>

using json = nlohmann::json;

namespace themis {
namespace plugins {
namespace ethics {

/**
 * @brief Ethics AI Plugin Implementation
 * 
 * Native C++ implementation of the Ethical AI Framework.
 */
class EthicsAIPlugin : public IEthicsAIPlugin {
private:
    // Core components
    std::shared_ptr<PhilosophyLoader> philosophy_loader_;
    std::shared_ptr<ArgumentStore> argument_store_;
    std::shared_ptr<RAGContextEngine> rag_engine_;
    std::shared_ptr<EthicalDiscourseEngine> discourse_engine_;
    std::shared_ptr<EthicsEvaluator> evaluator_;
    
    // Configuration
    std::map<std::string, std::string> config_;
    bool initialized_;
    
    // Integration with core system
    void* ethical_guidelines_manager_ = nullptr;  // EthicalGuidelinesManager* (forward declared)
    
    // Metrics
    struct Metrics {
        size_t total_debates = 0;
        size_t total_decisions = 0;
        size_t total_arguments = 0;
        size_t total_evaluations = 0;
        double avg_decision_quality = 0.0;
    } metrics_;
    
    mutable std::mutex metrics_mutex_;
    
public:
    EthicsAIPlugin() : initialized_(false) {}
    
    ~EthicsAIPlugin() override {
        if (initialized_) {
            shutdown();
        }
    }
    
    // ========== IThemisPlugin Interface ==========
    
    const char* getName() const override {
        return "EthicsAI";
    }
    
    const char* getVersion() const override {
        return "1.0.0";
    }
    
    PluginType getType() const override {
        return PluginType::CUSTOM;
    }
    
    PluginCapabilities getCapabilities() const override {
        PluginCapabilities caps;
        caps.supports_streaming = false;
        caps.supports_batching = true;
        caps.supports_transactions = true;
        caps.thread_safe = true;
        caps.gpu_accelerated = false;
        return caps;
    }
    
    bool initialize(const char* config_json) override {
        if (initialized_) {
            return false;
        }
        
        try {
            // Parse configuration
            if (config_json && std::string(config_json) != "{}") {
                auto config = json::parse(config_json);
                for (auto& [key, value] : config.items()) {
                    if (value.is_string()) {
                        config_[key] = value.get<std::string>();
                    }
                }
            }
            
            // Create components
            philosophy_loader_ = std::make_shared<PhilosophyLoader>();
            argument_store_ = std::make_shared<ArgumentStore>();
            rag_engine_ = std::make_shared<RAGContextEngine>(argument_store_);
            discourse_engine_ = std::make_shared<EthicalDiscourseEngine>(
                philosophy_loader_, argument_store_, rag_engine_
            );
            evaluator_ = std::make_shared<EthicsEvaluator>();

            auto chain_out_it = config_.find("chain_visualizer_output_path");
            if (chain_out_it != config_.end() && !chain_out_it->second.empty()) {
                discourse_engine_->setChainVisualizerOutputPath(chain_out_it->second);
            }
            
            // Initialize argument store
                auto status = argument_store_->initialize(nullptr);
                if (!status.isOK()) {
                return false;
            }
            
            // Load philosophy profiles if directory specified
            auto phil_dir_it = config_.find("philosophy_dir");
            if (phil_dir_it != config_.end()) {
                loadPhilosophyProfiles(phil_dir_it->second);
            }
            
            // Register philosophies with EthicalGuidelinesManager if available
            // (ethical_guidelines_manager_ is a void* forward-declared pointer;
            //  actual registration requires the LLM subsystem headers which are
            //  not available in standalone plugin builds.  Skip if not set.)
            if (ethical_guidelines_manager_) {
                // Reserved: integration with themis::llm::EthicalGuidelinesManager
                // requires full LLM subsystem linkage (not available in standalone mode).
            }
            
            initialized_ = true;
            return true;
            
        } catch ([[maybe_unused]] const std::exception& e) {
            return false;
        }
    }
    
    void shutdown() override {
        if (!initialized_) {
            return;
        }
        
        if (argument_store_) {
            argument_store_->shutdown();
        }
        
        philosophy_loader_.reset();
        argument_store_.reset();
        rag_engine_.reset();
        discourse_engine_.reset();
        evaluator_.reset();
        
        initialized_ = false;
    }
    
    void* getInstance() override {
        return static_cast<void*>(this);
    }
    
    // ========== IEthicsAIPlugin Interface ==========
    
    std::variant<DebateInitialization, Status> initializeDebate(
        const std::string& dilemma_description,
        const std::vector<std::string>& philosophy_schools,
        const std::string& category) override {
        
        if (!initialized_) {
            return Status::Error("Plugin not initialized");
        }
        
        auto result = discourse_engine_->initializeDebate(
            dilemma_description, philosophy_schools, category
        );
        
        if (std::holds_alternative<DebateInitialization>(result)) {
            std::lock_guard<std::mutex> lock(metrics_mutex_);
            metrics_.total_debates++;
        }
        
        return result;
    }
    
    Status storeArgument(
        const EthicalArgument& argument,
        bool store_vector) override {
        
        if (!initialized_) {
            return Status::Error("Plugin not initialized");
        }
        
        auto status = argument_store_->storeArgument(argument, store_vector);
        
        if (status.isOK()) {
            std::lock_guard<std::mutex> lock(metrics_mutex_);
            metrics_.total_arguments++;
        }
        
        return status;
    }
    
    std::variant<std::vector<EthicalArgument>, Status> getArgumentsByPhilosophy(
        const std::string& philosophy_school,
        const std::vector<ArgumentType>& argument_types,
        size_t limit) override {
        
        if (!initialized_) {
            return Status::Error("Plugin not initialized");
        }
        
        return argument_store_->getArgumentsByPhilosophy(
            philosophy_school, argument_types, limit
        );
    }
    
    std::variant<EthicalArgument, Status> getArgumentById(
        const std::string& argument_id) override {
        
        if (!initialized_) {
            return Status::Error("Plugin not initialized");
        }
        
        return argument_store_->getArgument(argument_id);
    }
    
    Status storeArgumentChain(const ArgumentChain& chain) override {
        if (!initialized_) {
            return Status::Error("Plugin not initialized");
        }
        
        return argument_store_->storeChain(chain);
    }
    
    std::variant<ArgumentChain, Status> getArgumentChain(
        const std::string& chain_id) override {
        
        if (!initialized_) {
            return Status::Error("Plugin not initialized");
        }
        
        return argument_store_->getChain(chain_id);
    }
    
    std::variant<RAGContext, Status> buildRAGContext(
        const std::string& dilemma_description,
        const std::vector<std::string>& philosophy_schools,
        const std::string& category) override {
        
        if (!initialized_) {
            return Status::Error("Plugin not initialized");
        }
        
        return rag_engine_->buildContext(
            dilemma_description, philosophy_schools, category
        );
    }
    
    std::variant<std::vector<std::string>, Status> findSimilarDilemmas(
        const std::string& query_text,
        double threshold,
        size_t limit) override {
        
        if (!initialized_) {
            return Status::Error("Plugin not initialized");
        }
        
        return rag_engine_->findSimilarDilemmas(query_text, threshold, limit);
    }
    
    std::variant<std::vector<std::string>, Status> getBestPractices(
        const std::string& category,
        double min_satisfaction,
        size_t limit) override {
        
        if (!initialized_) {
            return Status::Error("Plugin not initialized");
        }
        
        return rag_engine_->getBestPractices(category, min_satisfaction, limit);
    }
    
    std::variant<std::vector<std::pair<std::string, double>>, Status> 
    vectorSemanticSearch(
        const std::vector<float>& query_embedding,
        const std::string& philosophy_school,
        size_t limit) override {
        
        if (!initialized_) {
            return Status::Error("Plugin not initialized");
        }
        
        return rag_engine_->vectorSemanticSearch(
            query_embedding, philosophy_school, limit
        );
    }
    
    std::variant<std::vector<std::string>, Status> traverseArgumentChain(
        const std::string& start_argument_id,
        size_t max_depth,
        const std::string& direction) override {
        
        if (!initialized_) {
            return Status::Error("Plugin not initialized");
        }
        
        return rag_engine_->traverseArgumentChain(
            start_argument_id, max_depth, direction
        );
    }
    
    std::variant<EthicalDecision, Status> makeDecision(
        const std::string& dilemma_description,
        const std::vector<std::string>& philosophy_schools,
        const std::string& category,
        bool use_rag) override {
        
        if (!initialized_) {
            return Status::Error("Plugin not initialized");
        }
        
        auto result = discourse_engine_->makeDecision(
            dilemma_description, philosophy_schools, category, use_rag
        );
        
        if (std::holds_alternative<EthicalDecision>(result)) {
            std::lock_guard<std::mutex> lock(metrics_mutex_);
            metrics_.total_decisions++;
        }
        
        return result;
    }
    
    Status storeDecision(const EthicalDecision& decision) override {
        if (!initialized_) {
            return Status::Error("Plugin not initialized");
        }
        
        return argument_store_->storeDecision(decision);
    }
    
    std::variant<EthicalDecision, Status> getDecision(
        const std::string& decision_id) override {
        
        if (!initialized_) {
            return Status::Error("Plugin not initialized");
        }
        
        return argument_store_->getDecision(decision_id);
    }
    
    std::variant<EthicsEvaluationResult, Status> evaluateDecision(
        const EthicalDecision& decision,
        const std::vector<EthicalArgument>& arguments) override {
        
        if (!initialized_) {
            return Status::Error("Plugin not initialized");
        }
        
        auto result = evaluator_->evaluateDecision(decision, arguments);
        
        if (auto* eval = std::get_if<EthicsEvaluationResult>(&result)) {
            std::lock_guard<std::mutex> lock(metrics_mutex_);
            metrics_.total_evaluations++;
            
            // Update running average
            double total = metrics_.avg_decision_quality * (metrics_.total_evaluations - 1);
            total += eval->overall_score;
            metrics_.avg_decision_quality = total / metrics_.total_evaluations;
        }
        
        return result;
    }
    
    std::variant<size_t, Status> loadPhilosophyProfiles(
        const std::string& philosophy_dir) override {
        
        if (!initialized_) {
            return Status::Error("Plugin not initialized");
        }
        
        return philosophy_loader_->loadFromDirectory(philosophy_dir);
    }
    
    std::variant<PhilosophyProfile, Status> getPhilosophyProfile(
        const std::string& school_id) override {
        
        if (!initialized_) {
            return Status::Error("Plugin not initialized");
        }
        
        return philosophy_loader_->getProfile(school_id);
    }
    
    std::vector<std::string> listPhilosophySchools() const override {
        if (!initialized_ || !philosophy_loader_) {
            return {};
        }
        
        return philosophy_loader_->getSchoolIds();
    }
    
    std::string getPrometheusMetrics() const override {
        std::stringstream ss;
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        
        ss << "# HELP ethics_ai_debates_total Total number of debates initialized\n";
        ss << "# TYPE ethics_ai_debates_total counter\n";
        ss << "ethics_ai_debates_total " << metrics_.total_debates << "\n\n";
        
        ss << "# HELP ethics_ai_decisions_total Total number of decisions made\n";
        ss << "# TYPE ethics_ai_decisions_total counter\n";
        ss << "ethics_ai_decisions_total " << metrics_.total_decisions << "\n\n";
        
        ss << "# HELP ethics_ai_arguments_total Total number of arguments stored\n";
        ss << "# TYPE ethics_ai_arguments_total counter\n";
        ss << "ethics_ai_arguments_total " << metrics_.total_arguments << "\n\n";
        
        ss << "# HELP ethics_ai_evaluations_total Total number of evaluations performed\n";
        ss << "# TYPE ethics_ai_evaluations_total counter\n";
        ss << "ethics_ai_evaluations_total " << metrics_.total_evaluations << "\n\n";
        
        ss << "# HELP ethics_ai_avg_decision_quality Average decision quality score\n";
        ss << "# TYPE ethics_ai_avg_decision_quality gauge\n";
        ss << "ethics_ai_avg_decision_quality " << std::fixed << std::setprecision(4) 
           << metrics_.avg_decision_quality << "\n";
        
        return ss.str();
    }
    
    std::string getDashboardJSON() const override {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        
        json dashboard;
        dashboard["total_debates"] = metrics_.total_debates;
        dashboard["total_decisions"] = metrics_.total_decisions;
        dashboard["total_arguments"] = metrics_.total_arguments;
        dashboard["total_evaluations"] = metrics_.total_evaluations;
        dashboard["avg_decision_quality"] = metrics_.avg_decision_quality;
        dashboard["philosophy_count"] = philosophy_loader_ ? 
            philosophy_loader_->count() : 0;
        
        return dashboard.dump(2);
    }
    
    std::map<std::string, double> getStatistics() const override {
        std::lock_guard<std::mutex> lock(metrics_mutex_);
        
        std::map<std::string, double> stats;
        stats["total_debates"] = static_cast<double>(metrics_.total_debates);
        stats["total_decisions"] = static_cast<double>(metrics_.total_decisions);
        stats["total_arguments"] = static_cast<double>(metrics_.total_arguments);
        stats["total_evaluations"] = static_cast<double>(metrics_.total_evaluations);
        stats["avg_decision_quality"] = metrics_.avg_decision_quality;
        
        return stats;
    }
    
    Status setConfig(const std::string& key, const std::string& value) override {
        config_[key] = value;
        return Status::OK();
    }
    
    std::optional<std::string> getConfig(const std::string& key) const override {
        auto it = config_.find(key);
        if (it == config_.end()) {
            return std::nullopt;
        }
        return it->second;
    }
    
    void setEthicalGuidelinesManager(void* manager) override {
        ethical_guidelines_manager_ = manager;
    }
};

} // namespace ethics
} // namespace plugins
} // namespace themis

// ========== Plugin Entry Points ==========

extern "C" {

/**
 * @brief Create an EthicsAI plugin instance (C interface)
 * 
 * CRITICAL FIX: Return value MUST be immediately wrapped in a smart pointer
 * with destroyPlugin() as the custom deleter by the caller.
 * Recommended pattern:
 *   auto deleter = [](themis::plugins::IThemisPlugin* p) { destroyPlugin(p); };
 *   std::unique_ptr<themis::plugins::IThemisPlugin, decltype(deleter)> plugin(
 *       createPlugin(), deleter);
 * 
 * @return Raw pointer to EthicsAIPlugin; caller must manage lifetime
 */
THEMIS_PLUGIN_EXPORT themis::plugins::IThemisPlugin* createPlugin() {
    // CRITICAL FIX: Immediately wrap in smart pointer at call site
    // This is a C interface requirement; callers MUST use a custom deleter
    return new themis::plugins::ethics::EthicsAIPlugin();
}

/**
 * @brief Destroy an EthicsAI plugin instance (C interface)
 * 
 * CRITICAL FIX: Add null check and proper cleanup (delete_no_nullptr remediation)
 * 
 * @param plugin Pointer to plugin to destroy; may be nullptr (safe to call)
 */
THEMIS_PLUGIN_EXPORT void destroyPlugin(themis::plugins::IThemisPlugin* plugin) {
    if (plugin != nullptr) {
        delete plugin;
        // Note: Caller is responsible for setting their reference to nullptr
        // Use custom deleter to automate this:
        //   auto deleter = [](themis::plugins::IThemisPlugin* p) { destroyPlugin(p); };
        //   std::unique_ptr<themis::plugins::IThemisPlugin, decltype(deleter)> ...
    }
}

} // extern "C"
