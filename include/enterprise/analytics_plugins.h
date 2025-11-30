#pragma once

#include <string>
#include <memory>
#include <vector>
#include <functional>
#include <unordered_map>
#include <optional>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis {
namespace enterprise {

/**
 * @brief Enterprise Analytics Plugin System für ThemisDB
 * 
 * Dieses Modul definiert das Plugin-Interface für Enterprise Analytics,
 * die als separate DLLs/Shared Libraries bereitgestellt werden können.
 * 
 * ## Plugin-Kategorien
 * 
 * ### 1. Machine Learning & AI
 * - Predictive Analytics (Vorhersagemodelle)
 * - Anomaly Detection (Anomalieerkennung)
 * - Classification & Clustering
 * - Natural Language Processing
 * - Time Series Forecasting
 * 
 * ### 2. Advanced Analytics
 * - What-If Analysis (Szenarioanalyse)
 * - Root Cause Analysis
 * - Impact Analysis
 * - Correlation Discovery
 * - Trend Detection
 * 
 * ### 3. Business Intelligence
 * - KPI Computation
 * - Dashboard Metrics
 * - Report Generation
 * - Data Quality Scoring
 * - Benchmark Comparison
 * 
 * ### 4. Graph Analytics (Advanced)
 * - Graph Neural Networks
 * - Knowledge Graph Reasoning
 * - Link Prediction
 * - Node Classification
 * - Graph Embeddings
 * 
 * ### 5. Streaming Analytics
 * - Real-time Aggregation
 * - Complex Event Processing
 * - Pattern Matching
 * - Sliding Window Analytics
 * 
 * ### 6. Data Science
 * - Feature Engineering
 * - Data Profiling
 * - Statistical Testing
 * - Sampling & Simulation
 * 
 * ## Plugin-Lizenzierung
 * 
 * Enterprise Plugins können verschiedene Lizenzmodelle haben:
 * - Core (kostenlos, in ThemisDB enthalten)
 * - Professional (kostenpflichtig, per Node)
 * - Enterprise (kostenpflichtig, unbegrenzt)
 * - Custom (kundenspezifisch)
 */

// ============================================================================
// Plugin Interface Definitions
// ============================================================================

/**
 * @brief Plugin-Lizenztyp
 */
enum class PluginLicense {
    CORE,           ///< Kostenlos, in ThemisDB enthalten
    PROFESSIONAL,   ///< Kostenpflichtig, per Node
    ENTERPRISE,     ///< Kostenpflichtig, unbegrenzt
    TRIAL,          ///< Testversion, zeitlich begrenzt
    CUSTOM          ///< Kundenspezifisch
};

/**
 * @brief Plugin-Kategorie
 */
enum class PluginCategory {
    ML_AI,              ///< Machine Learning & AI
    ADVANCED_ANALYTICS, ///< Advanced Analytics
    BUSINESS_INTEL,     ///< Business Intelligence
    GRAPH_ANALYTICS,    ///< Graph Analytics
    STREAMING,          ///< Streaming Analytics
    DATA_SCIENCE,       ///< Data Science
    PROCESS_MINING,     ///< Process Mining
    SECURITY,           ///< Security Analytics
    COMPLIANCE,         ///< Compliance & Audit
    INTEGRATION         ///< External Integrations
};

/**
 * @brief Plugin-Metadaten
 */
struct PluginMetadata {
    std::string id;                     ///< Unique plugin ID
    std::string name;                   ///< Display name
    std::string version;                ///< Semantic version
    std::string vendor;                 ///< Vendor/Author
    std::string description;            ///< Description
    PluginCategory category;
    PluginLicense license;
    
    // Dependencies
    std::vector<std::string> required_plugins;
    std::string min_themis_version;
    
    // Capabilities
    std::vector<std::string> provided_functions;    ///< AQL functions
    std::vector<std::string> provided_aggregations; ///< OLAP aggregations
    bool supports_gpu = false;
    bool supports_distributed = false;
};

/**
 * @brief Basis-Interface für alle Analytics-Plugins
 */
class IAnalyticsPlugin {
public:
    virtual ~IAnalyticsPlugin() = default;
    
    // Metadata
    virtual PluginMetadata getMetadata() const = 0;
    
    // Lifecycle
    virtual bool initialize(const nlohmann::json& config) = 0;
    virtual void shutdown() = 0;
    virtual bool isReady() const = 0;
    
    // License verification
    virtual bool verifyLicense(const std::string& license_key) = 0;
    virtual std::string getLicenseInfo() const = 0;
    
    // Health check
    virtual nlohmann::json healthCheck() const = 0;
};

// ============================================================================
// ML & AI Plugin Interface
// ============================================================================

/**
 * @brief Machine Learning Plugin Interface
 */
class IMLPlugin : public IAnalyticsPlugin {
public:
    // Model management
    virtual bool loadModel(const std::string& model_path) = 0;
    virtual bool saveModel(const std::string& model_path) = 0;
    virtual std::vector<std::string> listModels() const = 0;
    
    // Training
    struct TrainingConfig {
        std::string model_type;
        nlohmann::json hyperparameters;
        double validation_split = 0.2;
        int max_epochs = 100;
        double early_stopping_patience = 10;
    };
    
    struct TrainingResult {
        bool success;
        std::string model_id;
        double training_loss;
        double validation_loss;
        nlohmann::json metrics;
        int epochs_completed;
    };
    
    virtual TrainingResult train(
        const std::vector<nlohmann::json>& training_data,
        const TrainingConfig& config
    ) = 0;
    
    // Prediction
    virtual nlohmann::json predict(
        const std::string& model_id,
        const nlohmann::json& input
    ) = 0;
    
    virtual std::vector<nlohmann::json> predictBatch(
        const std::string& model_id,
        const std::vector<nlohmann::json>& inputs
    ) = 0;
    
    // Feature importance
    virtual nlohmann::json getFeatureImportance(const std::string& model_id) = 0;
};

/**
 * @brief Anomaly Detection Plugin Interface
 */
class IAnomalyDetectionPlugin : public IAnalyticsPlugin {
public:
    struct AnomalyConfig {
        std::string algorithm;  // isolation_forest, autoencoder, lstm, statistical
        double contamination = 0.01;    // Expected anomaly ratio
        int window_size = 100;          // For time series
        double threshold = 3.0;         // Sigma threshold
    };
    
    struct Anomaly {
        std::string record_id;
        double anomaly_score;
        std::string anomaly_type;
        std::vector<std::string> contributing_features;
        nlohmann::json explanation;
    };
    
    // Detection
    virtual std::vector<Anomaly> detect(
        const std::vector<nlohmann::json>& data,
        const AnomalyConfig& config
    ) = 0;
    
    // Streaming detection
    virtual std::optional<Anomaly> detectSingle(
        const nlohmann::json& record,
        const AnomalyConfig& config
    ) = 0;
    
    // Model training for custom detectors
    virtual bool trainDetector(
        const std::vector<nlohmann::json>& normal_data,
        const AnomalyConfig& config
    ) = 0;
};

/**
 * @brief Natural Language Processing Plugin Interface
 */
class INLPPlugin : public IAnalyticsPlugin {
public:
    // Text embedding
    virtual std::vector<float> embed(const std::string& text) = 0;
    virtual std::vector<std::vector<float>> embedBatch(const std::vector<std::string>& texts) = 0;
    
    // Named Entity Recognition
    struct Entity {
        std::string text;
        std::string type;   // PERSON, ORG, LOCATION, DATE, etc.
        int start_pos;
        int end_pos;
        double confidence;
    };
    virtual std::vector<Entity> extractEntities(const std::string& text) = 0;
    
    // Sentiment Analysis
    struct Sentiment {
        double positive;
        double negative;
        double neutral;
        std::string overall;    // positive, negative, neutral
    };
    virtual Sentiment analyzeSentiment(const std::string& text) = 0;
    
    // Text Classification
    virtual std::vector<std::pair<std::string, double>> classify(
        const std::string& text,
        const std::vector<std::string>& categories
    ) = 0;
    
    // Summarization
    virtual std::string summarize(const std::string& text, int max_length = 100) = 0;
    
    // Question Answering
    virtual std::string answerQuestion(const std::string& context, const std::string& question) = 0;
};

// ============================================================================
// Advanced Analytics Plugin Interface
// ============================================================================

/**
 * @brief What-If Analysis Plugin Interface
 */
class IWhatIfPlugin : public IAnalyticsPlugin {
public:
    struct Scenario {
        std::string name;
        nlohmann::json parameter_changes;
        std::optional<std::string> base_scenario;
    };
    
    struct ScenarioResult {
        std::string scenario_name;
        nlohmann::json outcomes;
        nlohmann::json impact_analysis;
        double confidence;
    };
    
    // Scenario modeling
    virtual ScenarioResult evaluateScenario(
        const Scenario& scenario,
        const nlohmann::json& current_state
    ) = 0;
    
    virtual std::vector<ScenarioResult> compareScenarios(
        const std::vector<Scenario>& scenarios,
        const nlohmann::json& current_state
    ) = 0;
    
    // Sensitivity analysis
    virtual nlohmann::json sensitivityAnalysis(
        const std::string& target_metric,
        const std::vector<std::string>& input_parameters,
        const nlohmann::json& current_state
    ) = 0;
    
    // Monte Carlo simulation
    virtual nlohmann::json monteCarloSimulation(
        const Scenario& scenario,
        int num_simulations = 10000
    ) = 0;
};

/**
 * @brief Root Cause Analysis Plugin Interface
 */
class IRootCausePlugin : public IAnalyticsPlugin {
public:
    struct Symptom {
        std::string metric;
        std::string condition;  // e.g., "value > threshold"
        double severity;
    };
    
    struct RootCause {
        std::string cause;
        double probability;
        std::vector<std::string> evidence;
        std::vector<std::string> recommended_actions;
    };
    
    // Analysis
    virtual std::vector<RootCause> analyze(
        const std::vector<Symptom>& symptoms,
        const nlohmann::json& context
    ) = 0;
    
    // Causal graph
    virtual nlohmann::json buildCausalGraph(
        const std::vector<std::string>& metrics,
        const std::vector<nlohmann::json>& historical_data
    ) = 0;
};

/**
 * @brief Time Series Forecasting Plugin Interface
 */
class IForecastingPlugin : public IAnalyticsPlugin {
public:
    struct ForecastConfig {
        std::string algorithm;  // arima, prophet, lstm, xgboost
        int horizon;            // Forecast steps
        int seasonality = 0;    // Seasonal period (0 = auto-detect)
        double confidence_level = 0.95;
    };
    
    struct Forecast {
        std::vector<double> predictions;
        std::vector<double> lower_bound;
        std::vector<double> upper_bound;
        std::vector<int64_t> timestamps;
        nlohmann::json model_info;
    };
    
    // Forecasting
    virtual Forecast forecast(
        const std::vector<std::pair<int64_t, double>>& timeseries,
        const ForecastConfig& config
    ) = 0;
    
    // Seasonality detection
    virtual nlohmann::json detectSeasonality(
        const std::vector<std::pair<int64_t, double>>& timeseries
    ) = 0;
    
    // Trend analysis
    virtual nlohmann::json analyzeTrend(
        const std::vector<std::pair<int64_t, double>>& timeseries
    ) = 0;
};

// ============================================================================
// Graph Analytics Plugin Interface
// ============================================================================

/**
 * @brief Advanced Graph Analytics Plugin Interface
 */
class IGraphMLPlugin : public IAnalyticsPlugin {
public:
    // Graph Neural Networks
    struct GNNConfig {
        std::string architecture;   // gcn, gat, graphsage, gin
        int num_layers = 2;
        int hidden_dim = 64;
        double dropout = 0.5;
    };
    
    // Node embedding
    virtual std::vector<std::vector<float>> computeNodeEmbeddings(
        const nlohmann::json& graph,
        const GNNConfig& config
    ) = 0;
    
    // Link prediction
    struct LinkPrediction {
        std::string source;
        std::string target;
        double probability;
        std::string relationship_type;
    };
    virtual std::vector<LinkPrediction> predictLinks(
        const nlohmann::json& graph,
        int top_k = 100
    ) = 0;
    
    // Node classification
    virtual std::map<std::string, std::string> classifyNodes(
        const nlohmann::json& graph,
        const std::map<std::string, std::string>& labeled_nodes
    ) = 0;
    
    // Knowledge graph reasoning
    struct KGQuery {
        std::string head;
        std::string relation;
        std::string tail;   // "?" for query
    };
    virtual std::vector<std::pair<std::string, double>> queryKnowledgeGraph(
        const KGQuery& query
    ) = 0;
};

// ============================================================================
// Streaming Analytics Plugin Interface
// ============================================================================

/**
 * @brief Complex Event Processing Plugin Interface
 */
class ICEPPlugin : public IAnalyticsPlugin {
public:
    // Pattern definition
    struct EventPattern {
        std::string name;
        std::string pattern_expression;     // CEP pattern language
        std::chrono::milliseconds window;
        std::string action;                 // Callback or notification
    };
    
    // Pattern registration
    virtual bool registerPattern(const EventPattern& pattern) = 0;
    virtual bool unregisterPattern(const std::string& pattern_name) = 0;
    virtual std::vector<std::string> listPatterns() const = 0;
    
    // Event processing
    struct Event {
        std::string type;
        int64_t timestamp;
        nlohmann::json payload;
    };
    
    struct PatternMatch {
        std::string pattern_name;
        std::vector<Event> matching_events;
        int64_t match_timestamp;
    };
    
    virtual std::vector<PatternMatch> processEvent(const Event& event) = 0;
    virtual std::vector<PatternMatch> processBatch(const std::vector<Event>& events) = 0;
    
    // Window aggregations
    virtual nlohmann::json getWindowAggregates(
        const std::string& event_type,
        const std::vector<std::string>& aggregations
    ) = 0;
};

/**
 * @brief Real-time Aggregation Plugin Interface
 */
class IStreamAggPlugin : public IAnalyticsPlugin {
public:
    // Aggregation definition
    struct StreamAggregation {
        std::string name;
        std::string source;                 // Event source
        std::string group_by;               // Grouping field
        std::vector<std::pair<std::string, std::string>> aggregations; // field -> function
        std::chrono::milliseconds window;
        std::chrono::milliseconds slide;    // For sliding windows
    };
    
    virtual bool createAggregation(const StreamAggregation& agg) = 0;
    virtual bool dropAggregation(const std::string& name) = 0;
    
    // Real-time results
    virtual nlohmann::json getAggregationResults(const std::string& name) = 0;
    virtual nlohmann::json getAggregationHistory(
        const std::string& name,
        int64_t start_time,
        int64_t end_time
    ) = 0;
};

// ============================================================================
// Data Science Plugin Interface
// ============================================================================

/**
 * @brief Data Profiling Plugin Interface
 */
class IDataProfilingPlugin : public IAnalyticsPlugin {
public:
    struct ColumnProfile {
        std::string name;
        std::string inferred_type;
        int64_t count;
        int64_t null_count;
        int64_t distinct_count;
        double null_percentage;
        
        // Numeric stats
        std::optional<double> min;
        std::optional<double> max;
        std::optional<double> mean;
        std::optional<double> median;
        std::optional<double> std_dev;
        std::vector<double> percentiles;    // 25, 50, 75, 90, 95, 99
        
        // String stats
        std::optional<int> min_length;
        std::optional<int> max_length;
        std::optional<double> avg_length;
        
        // Pattern detection
        std::vector<std::pair<std::string, double>> detected_patterns;
        
        // Top values
        std::vector<std::pair<std::string, int64_t>> top_values;
    };
    
    struct DataProfile {
        std::string collection;
        int64_t row_count;
        std::vector<ColumnProfile> columns;
        
        // Data quality
        double completeness_score;
        double consistency_score;
        double uniqueness_score;
        double validity_score;
        double overall_quality_score;
        
        // Correlations
        std::vector<std::tuple<std::string, std::string, double>> correlations;
        
        // Anomalies
        std::vector<std::string> data_quality_issues;
    };
    
    virtual DataProfile profileCollection(
        const std::string& collection,
        int sample_size = 0  // 0 = full scan
    ) = 0;
    
    virtual DataProfile profileQuery(
        const std::string& aql_query,
        int sample_size = 0
    ) = 0;
    
    // Schema inference
    virtual nlohmann::json inferSchema(const std::string& collection) = 0;
};

/**
 * @brief Statistical Testing Plugin Interface
 */
class IStatisticsPlugin : public IAnalyticsPlugin {
public:
    struct TestResult {
        std::string test_name;
        double statistic;
        double p_value;
        bool reject_null;
        double confidence_level;
        std::string interpretation;
    };
    
    // Hypothesis testing
    virtual TestResult tTest(
        const std::vector<double>& sample1,
        const std::vector<double>& sample2,
        double alpha = 0.05
    ) = 0;
    
    virtual TestResult chiSquareTest(
        const std::vector<std::vector<int>>& contingency_table,
        double alpha = 0.05
    ) = 0;
    
    virtual TestResult anovaTest(
        const std::vector<std::vector<double>>& groups,
        double alpha = 0.05
    ) = 0;
    
    virtual TestResult correlationTest(
        const std::vector<double>& x,
        const std::vector<double>& y,
        double alpha = 0.05
    ) = 0;
    
    // Distribution fitting
    virtual nlohmann::json fitDistribution(
        const std::vector<double>& data
    ) = 0;
    
    // A/B Testing
    struct ABTestResult {
        std::string winner;
        double lift;
        double confidence;
        int sample_size_a;
        int sample_size_b;
        nlohmann::json detailed_stats;
    };
    
    virtual ABTestResult abTest(
        const std::vector<double>& control,
        const std::vector<double>& treatment,
        double min_detectable_effect = 0.05
    ) = 0;
};

// ============================================================================
// Plugin Loader & Registry
// ============================================================================

/**
 * @brief Enterprise Plugin Loader
 */
class EnterprisePluginLoader {
public:
    static EnterprisePluginLoader& instance();
    
    // Loading
    bool loadPlugin(const std::string& library_path);
    size_t loadPluginsFromDirectory(const std::string& directory);
    void unloadPlugin(const std::string& plugin_id);
    void unloadAllPlugins();
    
    // Discovery
    std::vector<PluginMetadata> discoverAvailablePlugins(const std::string& directory);
    
    // Access
    template<typename T>
    T* getPlugin(const std::string& plugin_id) const;
    
    std::vector<PluginMetadata> getLoadedPlugins() const;
    std::vector<PluginMetadata> getPluginsByCategory(PluginCategory category) const;
    
    // License management
    bool activateLicense(const std::string& plugin_id, const std::string& license_key);
    nlohmann::json getLicenseStatus() const;
    
private:
    EnterprisePluginLoader() = default;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// Plugin Export Macros
// ============================================================================

#ifdef _WIN32
    #define THEMIS_ENTERPRISE_EXPORT __declspec(dllexport)
#else
    #define THEMIS_ENTERPRISE_EXPORT __attribute__((visibility("default")))
#endif

#define THEMIS_DEFINE_ANALYTICS_PLUGIN(PluginClass, PluginInterface) \
    extern "C" THEMIS_ENTERPRISE_EXPORT themis::enterprise::IAnalyticsPlugin* CreateAnalyticsPlugin() { \
        return new PluginClass(); \
    } \
    extern "C" THEMIS_ENTERPRISE_EXPORT const char* GetPluginType() { \
        return #PluginInterface; \
    }

} // namespace enterprise
} // namespace themis
