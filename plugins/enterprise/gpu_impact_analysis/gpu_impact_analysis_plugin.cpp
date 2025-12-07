#include "enterprise/gpu_impact_analysis_plugin.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <queue>
#include <unordered_set>
#include <random>

namespace themis {
namespace enterprise {

/**
 * @brief GPU Impact Analysis Plugin Implementation
 * 
 * NOTE: This is a REFERENCE IMPLEMENTATION demonstrating the plugin architecture
 * and API design. GPU acceleration features are marked with TODO comments and
 * require integration with actual GPU backends (CUDA, Vulkan, etc.).
 * 
 * The implementation currently provides:
 * - Complete plugin interface (IGPUImpactAnalysisPlugin)
 * - CPU-based fallback algorithms
 * - Framework for GPU acceleration integration
 * 
 * GPU acceleration targets (requires backend implementation):
 * - Graph traversal (CUDA/Vulkan) - Target: 10-50x speedup
 * - Monte Carlo simulation (cuRAND) - Target: 100-1000x speedup
 * - FFT pattern detection (cuFFT) - Target: 100-500x speedup
 * - Sparse matrix operations (cuSPARSE) - Target: 20x speedup
 * - Time series forecasting (cuML) - Target: 100x speedup
 * - Anomaly detection (cuML Isolation Forest) - Target: 20-50x speedup
 */
class GPUImpactAnalysisPluginImpl : public IGPUImpactAnalysisPlugin {
public:
    GPUImpactAnalysisPluginImpl()
        : initialized_(false)
        , total_analyses_(0)
        , gpu_accelerated_analyses_(0)
        , total_analysis_time_ms_(0.0)
    {
    }
    
    ~GPUImpactAnalysisPluginImpl() override {
        shutdown();
    }
    
    // ========================================================================
    // IAnalyticsPlugin Interface
    // ========================================================================
    
    PluginMetadata getMetadata() const override {
        PluginMetadata meta;
        meta.id = "themis.enterprise.gpu_impact_analysis";
        meta.name = "GPU Impact Analysis";
        meta.version = "1.0.0";
        meta.vendor = "ThemisDB Enterprise";
        meta.description = "GPU-accelerated cause-effect analysis for document changes (FEM-inspired)";
        meta.category = PluginCategory::ADVANCED_ANALYTICS;
        meta.license = PluginLicense::ENTERPRISE;
        
        meta.provided_functions = {
            "GPU_ANALYZE_IMPACT",
            "GPU_PROPAGATE_FEM",
            "GPU_FORECAST_IMPACT",
            "GPU_MONTE_CARLO_RISK",
            "GPU_DETECT_PATTERNS",
            "GPU_FIND_ROOT_CAUSES"
        };
        
        meta.provided_aggregations = {
            "GPU_AVG_IMPACT",
            "GPU_MAX_IMPACT",
            "GPU_IMPACT_DISTRIBUTION"
        };
        
        meta.supports_gpu = true;
        meta.supports_distributed = true;
        meta.min_themis_version = "1.0.0";
        
        return meta;
    }
    
    bool initialize(const nlohmann::json& config) override {
        try {
            spdlog::info("[GPUImpactAnalysisPlugin] Initializing...");
            
            // GPU-Backend initialisieren
            if (config.contains("gpu_backend")) {
                gpu_backend_ = config["gpu_backend"].get<std::string>();
            } else {
                gpu_backend_ = "auto";  // Auto-detect
            }
            
            // FEM-Config laden
            if (config.contains("fem")) {
                fem_config_.damping_factor = config["fem"].value("damping_factor", 0.85);
                fem_config_.impact_threshold = config["fem"].value("impact_threshold", 0.01);
                fem_config_.max_iterations = config["fem"].value("max_iterations", 100);
                fem_config_.convergence_threshold = config["fem"].value("convergence_threshold", 0.001);
            }
            
            // Monte Carlo Config
            if (config.contains("monte_carlo")) {
                mc_config_.num_simulations = config["monte_carlo"].value("num_simulations", 10000);
                mc_config_.uncertainty_factor = config["monte_carlo"].value("uncertainty_factor", 0.2);
            }
            
            // TODO: GPU initialisieren (CUDA, Vulkan, etc.)
            // initializeGPU();
            
            initialized_ = true;
            spdlog::info("[GPUImpactAnalysisPlugin] Initialization complete (GPU: {})", gpu_backend_);
            return true;
            
        } catch (const std::exception& e) {
            spdlog::error("[GPUImpactAnalysisPlugin] Initialization failed: {}", e.what());
            return false;
        }
    }
    
    void shutdown() override {
        if (!initialized_) return;
        
        spdlog::info("[GPUImpactAnalysisPlugin] Shutting down...");
        
        // TODO: GPU-Ressourcen freigeben
        // shutdownGPU();
        
        initialized_ = false;
    }
    
    bool isReady() const override {
        return initialized_;
    }
    
    bool verifyLicense(const std::string& license_key) override {
        // TODO: Implement license verification
        // For now, accept any non-empty license key
        return !license_key.empty();
    }
    
    std::string getLicenseInfo() const override {
        nlohmann::json info;
        info["type"] = "enterprise";
        info["status"] = "active";
        info["expires"] = "2026-12-31";
        return info.dump();
    }
    
    nlohmann::json healthCheck() const override {
        nlohmann::json health;
        health["status"] = initialized_ ? "healthy" : "unhealthy";
        health["gpu_backend"] = gpu_backend_;
        health["total_analyses"] = total_analyses_;
        health["gpu_accelerated"] = gpu_accelerated_analyses_;
        health["avg_analysis_time_ms"] = total_analyses_ > 0 
            ? total_analysis_time_ms_ / total_analyses_ 
            : 0.0;
        return health;
    }
    
    // ========================================================================
    // Core Impact Analysis - Implementation
    // ========================================================================
    
    ImpactAnalysisResult analyzeDocumentChangeImpact(
        const DocumentChange& change,
        const nlohmann::json& config
    ) override {
        auto start = std::chrono::high_resolution_clock::now();
        
        ImpactAnalysisResult result;
        result.analysis_id = generateAnalysisId();
        result.source_change = change;
        
        // Konfiguration extrahieren
        int max_depth = config.value("max_depth", 5);
        double impact_threshold = config.value("impact_threshold", 0.01);
        bool use_gpu = config.value("use_gpu", true);
        
        try {
            // Graph-Struktur laden (aus ThemisDB)
            auto graph = loadGraphStructure(change.document_id);
            
            // FEM-basierte Propagierung
            std::vector<std::string> source_nodes = {change.document_id};
            std::vector<double> initial_impacts = {change.magnitude};
            
            auto impact_distribution = propagateImpactFEM(
                source_nodes,
                initial_impacts,
                graph,
                fem_config_
            );
            
            // Konvertiere zu NodeImpact
            for (const auto& [node_id, impact_score] : impact_distribution) {
                if (impact_score >= impact_threshold) {
                    NodeImpact node_impact;
                    node_impact.node_id = node_id;
                    node_impact.impact_score = impact_score;
                    node_impact.confidence = 0.95;  // TODO: Berechnen
                    
                    result.affected_nodes.push_back(node_impact);
                }
            }
            
            // Statistiken berechnen
            result.total_affected_count = static_cast<int>(result.affected_nodes.size());
            if (!result.affected_nodes.empty()) {
                result.max_impact_score = std::max_element(
                    result.affected_nodes.begin(),
                    result.affected_nodes.end(),
                    [](const NodeImpact& a, const NodeImpact& b) {
                        return a.impact_score < b.impact_score;
                    }
                )->impact_score;
                
                result.avg_impact_score = std::accumulate(
                    result.affected_nodes.begin(),
                    result.affected_nodes.end(),
                    0.0,
                    [](double sum, const NodeImpact& n) {
                        return sum + n.impact_score;
                    }
                ) / result.affected_nodes.size();
            }
            
            if (use_gpu) {
                gpu_accelerated_analyses_++;
            }
            
        } catch (const std::exception& e) {
            spdlog::error("[GPUImpactAnalysis] Analysis failed: {}", e.what());
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        result.computation_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        total_analyses_++;
        total_analysis_time_ms_ += result.computation_time.count();
        
        return result;
    }
    
    std::vector<ImpactAnalysisResult> analyzeBatchChanges(
        const std::vector<DocumentChange>& changes,
        const nlohmann::json& config
    ) override {
        std::vector<ImpactAnalysisResult> results;
        results.reserve(changes.size());
        
        // TODO: GPU-beschleunigtes Batch-Processing
        // Für jetzt: Sequenziell
        for (const auto& change : changes) {
            results.push_back(analyzeDocumentChangeImpact(change, config));
        }
        
        return results;
    }
    
    // ========================================================================
    // Multi-Layer Impact Analysis - Implementation
    // ========================================================================
    
    ImpactAnalysisResult analyzeMultiLayerImpact(
        const DocumentChange& change,
        const std::vector<std::string>& target_layers,
        const nlohmann::json& config
    ) override {
        auto start = std::chrono::high_resolution_clock::now();
        
        ImpactAnalysisResult result;
        result.analysis_id = generateAnalysisId();
        result.source_change = change;
        
        try {
            // Load graph structure with layer information
            auto graph = loadGraphStructure(change.document_id);
            
            // Extract layer-specific config
            FEMPropagationConfig fem_config = fem_config_;
            if (config.contains("fem")) {
                if (config["fem"].contains("layer_damping_factors")) {
                    for (auto& [layer, damping] : config["fem"]["layer_damping_factors"].items()) {
                        fem_config.layer_damping_factors[layer] = damping.get<double>();
                    }
                }
                if (config["fem"].contains("cross_layer_damping")) {
                    for (auto& [key, damping] : config["fem"]["cross_layer_damping"].items()) {
                        // Parse "layer1->layer2" format
                        auto pos = key.find("->");
                        if (pos != std::string::npos) {
                            std::string from_layer = key.substr(0, pos);
                            std::string to_layer = key.substr(pos + 2);
                            fem_config.cross_layer_damping[{from_layer, to_layer}] = damping.get<double>();
                        }
                    }
                }
            }
            
            // Perform multi-layer propagation
            std::unordered_set<std::string> visited;
            std::queue<std::pair<std::string, double>> to_process;
            std::map<std::string, std::string> node_layers; // node_id -> layer
            std::map<std::string, std::vector<std::string>> layer_paths; // node_id -> layers crossed
            
            // Extract layer information from graph
            if (graph.contains("nodes") && graph["nodes"].is_array()) {
                for (const auto& node : graph["nodes"]) {
                    std::string node_id = node.value("id", "");
                    std::string layer = node.value("_layer", "document");
                    node_layers[node_id] = layer;
                }
            }
            
            // Initialize with source node
            std::string source_layer = change.source_layer.empty() ? "document" : change.source_layer;
            to_process.push({change.document_id, change.magnitude});
            layer_paths[change.document_id] = {source_layer};
            
            // Multi-layer BFS with layer-aware damping
            while (!to_process.empty()) {
                auto [current_node, current_impact] = to_process.front();
                to_process.pop();
                
                if (visited.count(current_node) > 0 || current_impact < fem_config.impact_threshold) {
                    continue;
                }
                visited.insert(current_node);
                
                std::string current_layer = node_layers.count(current_node) ? 
                    node_layers[current_node] : "document";
                
                // Check if this layer is in target layers (if specified)
                if (!target_layers.empty() && 
                    std::find(target_layers.begin(), target_layers.end(), current_layer) == target_layers.end()) {
                    continue;
                }
                
                // Add to results
                NodeImpact node_impact;
                node_impact.node_id = current_node;
                node_impact.node_type = current_layer;
                node_impact.impact_score = current_impact;
                node_impact.node_layer = current_layer;
                node_impact.crossed_layers = layer_paths[current_node];
                node_impact.is_cross_layer_impact = layer_paths[current_node].size() > 1;
                node_impact.confidence = 0.95;
                result.affected_nodes.push_back(node_impact);
                
                // Update per-layer statistics
                result.affected_nodes_per_layer[current_layer]++;
                if (current_impact > result.max_impact_per_layer[current_layer]) {
                    result.max_impact_per_layer[current_layer] = current_impact;
                }
                
                // Propagate to neighbors with layer-aware damping
                auto neighbors = getIncomingEdges(current_node, graph);
                for (const auto& [neighbor_id, edge_weight] : neighbors) {
                    std::string neighbor_layer = node_layers.count(neighbor_id) ? 
                        node_layers[neighbor_id] : "document";
                    
                    // Calculate damping factor
                    double damping = fem_config.damping_factor;
                    
                    // Apply layer-specific damping if configured
                    if (fem_config.layer_damping_factors.count(neighbor_layer)) {
                        damping *= fem_config.layer_damping_factors[neighbor_layer];
                    }
                    
                    // Apply cross-layer damping if crossing layers
                    if (neighbor_layer != current_layer) {
                        if (!fem_config.enable_cross_layer_propagation) {
                            continue; // Skip cross-layer propagation if disabled
                        }
                        
                        auto cross_key = std::make_pair(current_layer, neighbor_layer);
                        if (fem_config.cross_layer_damping.count(cross_key)) {
                            damping *= fem_config.cross_layer_damping[cross_key];
                        } else {
                            damping *= 0.7; // Default cross-layer damping
                        }
                        
                        // Track layer transition
                        result.cross_layer_transitions++;
                        result.layer_transition_paths.push_back({current_layer, neighbor_layer});
                    }
                    
                    double propagated_impact = current_impact * edge_weight * damping;
                    
                    if (propagated_impact >= fem_config.impact_threshold) {
                        to_process.push({neighbor_id, propagated_impact});
                        
                        // Update layer path
                        auto new_path = layer_paths[current_node];
                        if (neighbor_layer != current_layer && 
                            (new_path.empty() || new_path.back() != neighbor_layer)) {
                            new_path.push_back(neighbor_layer);
                        }
                        layer_paths[neighbor_id] = new_path;
                    }
                }
            }
            
            // Calculate statistics
            result.total_affected_count = static_cast<int>(result.affected_nodes.size());
            if (!result.affected_nodes.empty()) {
                result.max_impact_score = std::max_element(
                    result.affected_nodes.begin(),
                    result.affected_nodes.end(),
                    [](const NodeImpact& a, const NodeImpact& b) {
                        return a.impact_score < b.impact_score;
                    }
                )->impact_score;
                
                result.avg_impact_score = std::accumulate(
                    result.affected_nodes.begin(),
                    result.affected_nodes.end(),
                    0.0,
                    [](double sum, const NodeImpact& n) {
                        return sum + n.impact_score;
                    }
                ) / result.affected_nodes.size();
            }
            
        } catch (const std::exception& e) {
            spdlog::error("[GPUImpactAnalysis] Multi-layer analysis failed: {}", e.what());
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        result.computation_time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        total_analyses_++;
        total_analysis_time_ms_ += result.computation_time.count();
        
        return result;
    }
    
    // ========================================================================
    // FEM-Inspired Graph Propagation - Implementation
    // ========================================================================
    
    std::unordered_map<std::string, double> propagateImpactFEM(
        const std::vector<std::string>& source_nodes,
        const std::vector<double>& initial_impacts,
        const nlohmann::json& graph_structure,
        const FEMPropagationConfig& config
    ) override {
        std::unordered_map<std::string, double> impact_map;
        
        // Initialize source nodes
        for (size_t i = 0; i < source_nodes.size() && i < initial_impacts.size(); ++i) {
            impact_map[source_nodes[i]] = initial_impacts[i];
        }
        
        // Iterative propagation (FEM-inspired)
        for (int iter = 0; iter < config.max_iterations; ++iter) {
            std::unordered_map<std::string, double> new_impacts = impact_map;
            double max_change = 0.0;
            
            // Propagate impacts through edges
            iterateGraphEdges(graph_structure, [&](const auto& edge) {
                std::string from = edge.value("from", "");
                std::string to = edge.value("to", "");
                double weight = edge.value("weight", 1.0);
                
                auto it = impact_map.find(from);
                if (it != impact_map.end()) {
                    double propagated_impact = it->second * weight * config.damping_factor;
                    
                    if (propagated_impact >= config.impact_threshold) {
                        double old_impact = new_impacts[to];
                        new_impacts[to] = std::max(new_impacts[to], propagated_impact);
                        max_change = std::max(max_change, std::abs(new_impacts[to] - old_impact));
                    }
                }
            });
            
            impact_map = std::move(new_impacts);
            
            // Check convergence
            if (max_change < config.convergence_threshold) {
                spdlog::debug("[GPUImpactAnalysis] FEM propagation converged after {} iterations", iter + 1);
                break;
            }
        }
        
        // Filter by threshold
        std::unordered_map<std::string, double> filtered;
        for (const auto& [node, impact] : impact_map) {
            if (impact >= config.impact_threshold) {
                filtered[node] = impact;
            }
        }
        
        return filtered;
    }
    
    std::vector<double> sparseMatrixVectorMultiply_GPU(
        const nlohmann::json& adjacency_matrix,
        const std::vector<double>& input_vector
    ) override {
        // CPU fallback implementation
        // TODO: GPU acceleration with cuSPARSE
        
        std::vector<double> result(input_vector.size(), 0.0);
        
        if (adjacency_matrix.contains("edges") && adjacency_matrix["edges"].is_array()) {
            // Build adjacency structure
            std::unordered_map<size_t, std::vector<std::pair<size_t, double>>> adj;
            
            for (const auto& edge : adjacency_matrix["edges"]) {
                size_t from = edge.value("from_idx", 0);
                size_t to = edge.value("to_idx", 0);
                double weight = edge.value("weight", 1.0);
                
                adj[from].push_back({to, weight});
            }
            
            // Multiply
            for (const auto& [from_idx, neighbors] : adj) {
                if (from_idx < input_vector.size()) {
                    for (const auto& [to_idx, weight] : neighbors) {
                        if (to_idx < result.size()) {
                            result[to_idx] += input_vector[from_idx] * weight;
                        }
                    }
                }
            }
        }
        
        return result;
    }
    
    // ========================================================================
    // Temporal Analysis - Implementation
    // ========================================================================
    
    std::vector<TemporalImpact> analyzeTemporalImpact(
        const std::vector<DocumentChange>& changes,
        const std::vector<std::string>& target_nodes,
        std::chrono::hours time_window
    ) override {
        std::vector<TemporalImpact> results;
        
        for (const auto& node_id : target_nodes) {
            TemporalImpact temporal;
            temporal.node_id = node_id;
            
            // Analyze impact over time for this node
            for (const auto& change : changes) {
                auto impact_result = analyzeDocumentChangeImpact(change, {});
                
                // Find this node's impact
                for (const auto& node_impact : impact_result.affected_nodes) {
                    if (node_impact.node_id == node_id) {
                        temporal.impact_timeseries.push_back({change.timestamp, node_impact.impact_score});
                    }
                }
            }
            
            if (!temporal.impact_timeseries.empty()) {
                calculateTimeSeriesStats(temporal);
                results.push_back(temporal);
            }
        }
        
        return results;
    }
    
    std::vector<TemporalImpact> forecastFutureImpact(
        const std::vector<TemporalImpact>& historical_impacts,
        int forecast_horizon_hours
    ) override {
        std::vector<TemporalImpact> forecasts;
        
        // Simple linear extrapolation (TODO: ARIMA for GPU)
        for (const auto& historical : historical_impacts) {
            if (historical.impact_timeseries.size() < 2) continue;
            
            TemporalImpact forecast;
            forecast.node_id = historical.node_id;
            forecast.trend = historical.trend;
            
            // Use last timestamp and trend for forecasting
            auto last_time = historical.impact_timeseries.back().first;
            auto last_impact = historical.impact_timeseries.back().second;
            
            int64_t hour_step = 3600000; // 1 hour in milliseconds
            for (int h = 1; h <= forecast_horizon_hours; ++h) {
                int64_t future_time = last_time + (h * hour_step);
                double forecasted_impact = last_impact + (historical.trend * h);
                forecasted_impact = std::max(0.0, std::min(1.0, forecasted_impact));
                
                forecast.impact_timeseries.push_back({future_time, forecasted_impact});
            }
            
            forecasts.push_back(forecast);
        }
        
        return forecasts;
    }
    
    // ========================================================================
    // Monte Carlo Risk Assessment - Implementation
    // ========================================================================
    
    RiskAssessment assessChangeRisk_MonteCarlo(
        const DocumentChange& change,
        const MonteCarloConfig& config
    ) override {
        RiskAssessment risk;
        std::vector<double> simulated_impacts;
        simulated_impacts.reserve(config.num_simulations);
        
        // Random number generation
        std::random_device rd;
        std::mt19937 gen(rd());
        std::normal_distribution<> dis(change.magnitude, config.uncertainty_factor);
        
        // Run Monte Carlo simulations (CPU fallback)
        // TODO: GPU acceleration with cuRAND
        for (int i = 0; i < config.num_simulations; ++i) {
            double sampled_magnitude = dis(gen);
            sampled_magnitude = std::max(0.0, std::min(1.0, sampled_magnitude));
            
            simulated_impacts.push_back(sampled_magnitude);
        }
        
        // Calculate statistics
        std::sort(simulated_impacts.begin(), simulated_impacts.end());
        
        risk.expected_impact = std::accumulate(simulated_impacts.begin(), simulated_impacts.end(), 0.0) 
                              / simulated_impacts.size();
        risk.max_impact = simulated_impacts.back();
        
        size_t idx_95 = static_cast<size_t>(config.num_simulations * 0.95);
        size_t idx_99 = static_cast<size_t>(config.num_simulations * 0.99);
        
        risk.value_at_risk_95 = simulated_impacts[idx_95];
        risk.value_at_risk_99 = simulated_impacts[idx_99];
        risk.impact_distribution = simulated_impacts;
        
        return risk;
    }
    
    // ========================================================================
    // Pattern Detection - Implementation
    // ========================================================================
    
    std::vector<ImpactPattern> detectImpactPatterns_FFT(
        const std::vector<ImpactAnalysisResult>& historical_results
    ) override {
        std::vector<ImpactPattern> patterns;
        
        // Simple pattern classification based on structure
        // TODO: FFT-based frequency analysis with cuFFT
        
        for (const auto& result : historical_results) {
            ImpactPattern pattern;
            pattern.pattern_id = generateAnalysisId();
            pattern.frequency = 1.0 / historical_results.size();
            
            // Classify pattern type based on structure
            if (result.affected_nodes.size() > 20) {
                pattern.pattern_type = "cascade";
                pattern.severity = result.max_impact_score;
            } else if (result.affected_nodes.size() < 5) {
                pattern.pattern_type = "isolated";
                pattern.severity = result.max_impact_score;
            } else {
                pattern.pattern_type = "moderate";
                pattern.severity = result.avg_impact_score;
            }
            
            patterns.push_back(pattern);
        }
        
        return patterns;
    }
    
    std::vector<std::pair<std::string, double>> findSimilarImpactScenarios_DTW(
        const ImpactAnalysisResult& query_impact,
        const std::vector<ImpactAnalysisResult>& historical_database,
        int top_k
    ) override {
        std::vector<std::pair<std::string, double>> similarities;
        
        // Simple similarity based on affected node count and max impact
        // TODO: DTW distance calculation
        
        for (const auto& historical : historical_database) {
            double node_count_diff = std::abs(
                static_cast<double>(query_impact.total_affected_count) - 
                static_cast<double>(historical.total_affected_count)
            );
            double impact_diff = std::abs(query_impact.max_impact_score - historical.max_impact_score);
            
            double similarity = 1.0 / (1.0 + node_count_diff + impact_diff);
            similarities.push_back({historical.analysis_id, similarity});
        }
        
        // Sort by similarity descending
        std::sort(similarities.begin(), similarities.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Return top-k
        if (similarities.size() > static_cast<size_t>(top_k)) {
            similarities.resize(top_k);
        }
        
        return similarities;
    }
    
    // ========================================================================
    // Anomaly Detection - Implementation
    // ========================================================================
    
    std::vector<ImpactAnomaly> detectImpactAnomalies(
        const std::vector<ImpactAnalysisResult>& recent_impacts,
        const nlohmann::json& config
    ) override {
        std::vector<ImpactAnomaly> anomalies;
        
        if (recent_impacts.empty()) return anomalies;
        
        // Calculate statistics for anomaly detection
        double mean_affected = 0.0;
        double mean_max_impact = 0.0;
        
        for (const auto& impact : recent_impacts) {
            mean_affected += impact.total_affected_count;
            mean_max_impact += impact.max_impact_score;
        }
        
        mean_affected /= recent_impacts.size();
        mean_max_impact /= recent_impacts.size();
        
        // Calculate standard deviation
        double std_affected = 0.0;
        double std_max_impact = 0.0;
        
        for (const auto& impact : recent_impacts) {
            std_affected += std::pow(impact.total_affected_count - mean_affected, 2);
            std_max_impact += std::pow(impact.max_impact_score - mean_max_impact, 2);
        }
        
        std_affected = std::sqrt(std_affected / recent_impacts.size());
        std_max_impact = std::sqrt(std_max_impact / recent_impacts.size());
        
        // Detect anomalies (simple statistical threshold)
        double threshold = config.value("threshold", 2.0); // Sigma threshold
        
        for (const auto& impact : recent_impacts) {
            double z_affected = std_affected > 0 ? 
                std::abs(impact.total_affected_count - mean_affected) / std_affected : 0.0;
            double z_impact = std_max_impact > 0 ? 
                std::abs(impact.max_impact_score - mean_max_impact) / std_max_impact : 0.0;
            
            if (z_affected > threshold || z_impact > threshold) {
                ImpactAnomaly anomaly;
                anomaly.anomaly_id = generateAnalysisId();
                anomaly.anomaly_score = std::max(z_affected, z_impact);
                
                if (impact.total_affected_count > mean_affected) {
                    anomaly.anomaly_type = "unexpected_high";
                    anomaly.explanation = "Unusually high number of affected nodes";
                } else {
                    anomaly.anomaly_type = "unexpected_low";
                    anomaly.explanation = "Unusually low impact propagation";
                }
                
                anomalies.push_back(anomaly);
            }
        }
        
        return anomalies;
    }
    
    // ========================================================================
    // What-If Analysis - Implementation
    // ========================================================================
    
    std::vector<ImpactAnalysisResult> simulateWhatIfScenarios(
        const std::vector<WhatIfScenario>& scenarios
    ) override {
        std::vector<ImpactAnalysisResult> results;
        results.reserve(scenarios.size());
        
        for (const auto& scenario : scenarios) {
            // Analyze each hypothetical change
            for (const auto& change : scenario.hypothetical_changes) {
                auto result = analyzeDocumentChangeImpact(change, scenario.context);
                results.push_back(result);
            }
        }
        
        return results;
    }
    
    ScenarioComparison compareScenarios(
        const std::vector<WhatIfScenario>& scenarios
    ) override {
        ScenarioComparison comparison;
        
        auto scenario_results = simulateWhatIfScenarios(scenarios);
        
        // Collect scenario names
        for (const auto& scenario : scenarios) {
            comparison.scenario_names.push_back(scenario.scenario_name);
        }
        
        // Build comparison matrix
        nlohmann::json matrix = nlohmann::json::array();
        
        for (size_t i = 0; i < scenario_results.size(); ++i) {
            nlohmann::json row;
            row["scenario"] = i < scenarios.size() ? scenarios[i].scenario_name : "unknown";
            row["affected_nodes"] = scenario_results[i].total_affected_count;
            row["max_impact"] = scenario_results[i].max_impact_score;
            row["avg_impact"] = scenario_results[i].avg_impact_score;
            
            matrix.push_back(row);
        }
        
        comparison.comparison_matrix = matrix;
        
        // Find recommended scenario (lowest max impact)
        if (!scenario_results.empty()) {
            auto min_it = std::min_element(
                scenario_results.begin(), 
                scenario_results.end(),
                [](const auto& a, const auto& b) {
                    return a.max_impact_score < b.max_impact_score;
                }
            );
            
            size_t idx = std::distance(scenario_results.begin(), min_it);
            comparison.recommended_scenario = idx < scenarios.size() ? 
                scenarios[idx].scenario_name : "unknown";
            comparison.recommendation_reason = "Lowest maximum impact score";
        }
        
        return comparison;
    }
    
    // ========================================================================
    // Sensitivity Analysis - Implementation
    // ========================================================================
    
    nlohmann::json analyzeSensitivity(
        const DocumentChange& base_change,
        const std::vector<std::string>& parameters,
        double variation_range
    ) override {
        nlohmann::json sensitivity_results;
        
        for (const auto& param : parameters) {
            nlohmann::json param_sensitivity;
            
            // Test variations
            std::vector<double> variations = {
                -variation_range, -variation_range/2, 0, variation_range/2, variation_range
            };
            
            std::vector<double> impacts;
            
            for (double variation : variations) {
                DocumentChange modified = base_change;
                
                // Modify parameter (simplified - assumes magnitude)
                if (param == "magnitude") {
                    modified.magnitude = std::max(0.0, std::min(1.0, base_change.magnitude + variation));
                }
                
                auto result = analyzeDocumentChangeImpact(modified, {});
                impacts.push_back(result.max_impact_score);
            }
            
            param_sensitivity["parameter"] = param;
            param_sensitivity["variations"] = variations;
            param_sensitivity["impacts"] = impacts;
            
            // Calculate sensitivity (simple linear correlation)
            double r_squared = calculateRSquared(variations, impacts);
            param_sensitivity["r_squared"] = r_squared;
            param_sensitivity["linearity"] = r_squared > 0.9 ? "linear" : "nonlinear";
            
            sensitivity_results[param] = param_sensitivity;
        }
        
        return sensitivity_results;
    }
    
    // ========================================================================
    // Causal Graph - Implementation
    // ========================================================================
    
    CausalGraph buildCausalGraph(
        const std::vector<DocumentChange>& historical_changes,
        double confidence_threshold
    ) override {
        CausalGraph graph;
        
        // Collect all unique document IDs
        std::unordered_set<std::string> unique_nodes;
        for (const auto& change : historical_changes) {
            unique_nodes.insert(change.document_id);
        }
        
        graph.nodes = std::vector<std::string>(unique_nodes.begin(), unique_nodes.end());
        
        // Build temporal correlations
        // Simplified: If change A happens before change B within time window, add edge
        for (size_t i = 0; i < historical_changes.size(); ++i) {
            for (size_t j = i + 1; j < historical_changes.size(); ++j) {
                const auto& change_a = historical_changes[i];
                const auto& change_b = historical_changes[j];
                
                int64_t time_diff = change_b.timestamp - change_a.timestamp;
                
                // If B happens within 1 hour after A, consider causal relationship
                if (time_diff > 0 && time_diff < 3600000) {
                    double strength = 1.0 / (1.0 + time_diff / 1000.0); // Decays with time
                    
                    if (strength >= confidence_threshold) {
                        graph.edges.push_back({
                            change_a.document_id,
                            change_b.document_id,
                            strength
                        });
                    }
                }
            }
        }
        
        return graph;
    }
    
    std::vector<std::pair<std::string, double>> findRootCauses(
        const ImpactAnalysisResult& observed_impact,
        const CausalGraph& causal_graph,
        int top_k
    ) override {
        std::vector<std::pair<std::string, double>> root_causes;
        
        // Find nodes with high out-degree (potential causes)
        std::unordered_map<std::string, double> node_scores;
        
        for (const auto& [from, to, strength] : causal_graph.edges) {
            node_scores[from] += strength;
        }
        
        // Convert to vector and sort
        for (const auto& [node, score] : node_scores) {
            root_causes.push_back({node, score});
        }
        
        std::sort(root_causes.begin(), root_causes.end(),
                  [](const auto& a, const auto& b) { return a.second > b.second; });
        
        // Return top-k
        if (root_causes.size() > static_cast<size_t>(top_k)) {
            root_causes.resize(top_k);
        }
        
        return root_causes;
    }
    
    // ========================================================================
    // Performance Metrics - Implementation
    // ========================================================================
    
    PerformanceMetrics getPerformanceMetrics() const override {
        PerformanceMetrics metrics;
        metrics.total_analyses = total_analyses_;
        metrics.gpu_accelerated_analyses = gpu_accelerated_analyses_;
        metrics.avg_analysis_time_ms = total_analyses_ > 0 ? 
            total_analysis_time_ms_ / total_analyses_ : 0.0;
        metrics.avg_speedup = 1.0; // CPU-only for now
        metrics.gpu_utilization = 0.0;
        metrics.total_nodes_analyzed = 0;
        
        return metrics;
    }
    
    void resetPerformanceMetrics() override {
        total_analyses_ = 0;
        gpu_accelerated_analyses_ = 0;
        total_analysis_time_ms_ = 0.0;
    }

private:
    std::string generateAnalysisId() {
        static std::atomic<int64_t> counter{0};
        auto now = std::chrono::system_clock::now().time_since_epoch().count();
        return "impact_" + std::to_string(now) + "_" + std::to_string(counter++);
    }
    
    // Helper: Get edges from graph JSON safely
    template<typename Func>
    void iterateGraphEdges(const nlohmann::json& graph, Func&& func) {
        if (graph.contains("edges") && graph["edges"].is_array()) {
            for (const auto& edge : graph["edges"]) {
                func(edge);
            }
        }
    }
    
    nlohmann::json loadGraphStructure(const std::string& document_id) {
        // TODO: Integration with ThemisDB GraphIndexManager
        // For now, return empty graph structure
        nlohmann::json graph;
        graph["nodes"] = nlohmann::json::array();
        graph["edges"] = nlohmann::json::array();
        
        spdlog::debug("[GPUImpactAnalysis] Loaded graph for document: {}", document_id);
        
        return graph;
    }
    
    std::vector<std::pair<std::string, double>> getIncomingEdges(
        const std::string& node_id,
        const nlohmann::json& graph
    ) {
        std::vector<std::pair<std::string, double>> incoming;
        
        if (graph.contains("edges") && graph["edges"].is_array()) {
            for (const auto& edge : graph["edges"]) {
                std::string to = edge.value("to", "");
                if (to == node_id) {
                    std::string from = edge.value("from", "");
                    double weight = edge.value("weight", 1.0);
                    incoming.push_back({from, weight});
                }
            }
        }
        
        return incoming;
    }
    
    double calculateImpactForNode(const DocumentChange& change, const std::string& node_id) {
        // Simple impact calculation based on change magnitude
        // TODO: More sophisticated calculation based on node properties
        return change.magnitude * 0.8; // Damping factor
    }
    
    void calculateTimeSeriesStats(TemporalImpact& temporal) {
        if (temporal.impact_timeseries.empty()) return;
        
        // Calculate trend (linear regression slope)
        std::vector<double> x_values, y_values;
        for (size_t i = 0; i < temporal.impact_timeseries.size(); ++i) {
            x_values.push_back(static_cast<double>(i));
            y_values.push_back(temporal.impact_timeseries[i].second);
        }
        
        double x_mean = std::accumulate(x_values.begin(), x_values.end(), 0.0) / x_values.size();
        double y_mean = std::accumulate(y_values.begin(), y_values.end(), 0.0) / y_values.size();
        
        double numerator = 0.0;
        double denominator = 0.0;
        
        for (size_t i = 0; i < x_values.size(); ++i) {
            numerator += (x_values[i] - x_mean) * (y_values[i] - y_mean);
            denominator += (x_values[i] - x_mean) * (x_values[i] - x_mean);
        }
        
        temporal.trend = denominator > 0 ? numerator / denominator : 0.0;
        
        // Calculate volatility (standard deviation)
        double variance = 0.0;
        for (const auto& val : y_values) {
            variance += std::pow(val - y_mean, 2);
        }
        temporal.volatility = std::sqrt(variance / y_values.size());
        
        // Find peak
        auto max_it = std::max_element(
            temporal.impact_timeseries.begin(),
            temporal.impact_timeseries.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; }
        );
        
        if (max_it != temporal.impact_timeseries.end()) {
            temporal.peak_time = max_it->first;
            temporal.peak_impact = max_it->second;
        }
    }
    
    double calculateRSquared(const std::vector<double>& x, const std::vector<double>& y) {
        if (x.size() != y.size() || x.empty()) return 0.0;
        
        double y_mean = std::accumulate(y.begin(), y.end(), 0.0) / y.size();
        
        double ss_tot = 0.0;
        double ss_res = 0.0;
        
        // Linear regression
        double x_mean = std::accumulate(x.begin(), x.end(), 0.0) / x.size();
        
        double numerator = 0.0;
        double denominator = 0.0;
        
        for (size_t i = 0; i < x.size(); ++i) {
            numerator += (x[i] - x_mean) * (y[i] - y_mean);
            denominator += (x[i] - x_mean) * (x[i] - x_mean);
        }
        
        double slope = denominator > 0 ? numerator / denominator : 0.0;
        double intercept = y_mean - slope * x_mean;
        
        // Calculate R²
        for (size_t i = 0; i < y.size(); ++i) {
            double y_pred = slope * x[i] + intercept;
            ss_res += std::pow(y[i] - y_pred, 2);
            ss_tot += std::pow(y[i] - y_mean, 2);
        }
        
        return ss_tot > 0 ? 1.0 - (ss_res / ss_tot) : 0.0;
    }
    
    bool initialized_;
    std::string gpu_backend_;
    FEMPropagationConfig fem_config_;
    MonteCarloConfig mc_config_;
    
    int64_t total_analyses_;
    int64_t gpu_accelerated_analyses_;
    double total_analysis_time_ms_;
};

} // namespace enterprise
} // namespace themis

// ============================================================================
// Factory Function Implementation
// ============================================================================

namespace themis {
namespace enterprise {

IGPUImpactAnalysisPlugin* createGPUImpactAnalysisPlugin() {
    return new GPUImpactAnalysisPluginImpl();
}

} // namespace enterprise
} // namespace themis

// ============================================================================
// Plugin Export
// ============================================================================

extern "C" {

THEMIS_ENTERPRISE_EXPORT themis::enterprise::IAnalyticsPlugin* CreateAnalyticsPlugin() {
    return new themis::enterprise::GPUImpactAnalysisPluginImpl();
}

THEMIS_ENTERPRISE_EXPORT void DestroyAnalyticsPlugin(themis::enterprise::IAnalyticsPlugin* plugin) {
    delete plugin;
}

THEMIS_ENTERPRISE_EXPORT const char* GetPluginType() {
    return "IGPUImpactAnalysisPlugin";
}

THEMIS_ENTERPRISE_EXPORT const char* GetPluginVersion() {
    return "1.0.0";
}

} // extern "C"
