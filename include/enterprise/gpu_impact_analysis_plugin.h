#pragma once

#include "analytics_plugins.h"
#include <nlohmann/json.hpp>
#include <vector>
#include <string>
#include <unordered_map>
#include <chrono>
#include <optional>

namespace themis {
namespace enterprise {

/**
 * @brief GPU-Accelerated Impact Analysis Plugin Interface
 * 
 * FEM (Finite Element Method) inspired cause-effect analysis
 * for document changes and their impact propagation through the graph.
 * 
 * ## Use Cases
 * - Document change analysis: How does a change affect dependent documents?
 * - Graph propagation: Which nodes are affected by a change?
 * - Risk assessment: What are the potential impacts of a change?
 * - Temporal analysis: How does impact evolve over time?
 * 
 * ## GPU Acceleration (Target Performance)
 * - Graph traversal: 10-50x speedup over CPU
 * - Impact simulation: 100-1000x speedup (Monte Carlo)
 * - Pattern matching: 100-500x speedup (FFT)
 * 
 * ## FEM Inspiration
 * The FEM model is adapted for graph analysis:
 * - Nodes = finite elements
 * - Edges = connections with weights (dependency strength)
 * - Change = external force/load
 * - Impact = deformation/stress in the system
 * 
 * @note This is an ENTERPRISE PLUGIN INTERFACE demonstrating advanced analytics
 * capabilities. Reference implementation provides CPU fallback; GPU acceleration
 * requires backend integration (CUDA, Vulkan, HIP, etc.).
 */
class IGPUImpactAnalysisPlugin : public IAnalyticsPlugin {
public:
    virtual ~IGPUImpactAnalysisPlugin() = default;
    
    // ========================================================================
    // Core Impact Analysis
    // ========================================================================
    
    /**
     * @brief Dokumentänderung (Input für Analyse)
     */
    struct DocumentChange {
        std::string document_id;
        std::string change_type;        // insert, update, delete, schema_change
        nlohmann::json old_value;
        nlohmann::json new_value;
        int64_t timestamp;
        std::string user_id;
        std::vector<std::string> affected_fields;
        double magnitude;               // Stärke der Änderung (0.0-1.0)
    };
    
    /**
     * @brief Impact auf einen Knoten
     */
    struct NodeImpact {
        std::string node_id;
        std::string node_type;
        double impact_score;            // 0.0-1.0
        std::vector<std::string> propagation_path;
        int distance_from_source;
        double confidence;
        nlohmann::json impact_details;
    };
    
    /**
     * @brief Gesamtergebnis der Impact-Analyse
     */
    struct ImpactAnalysisResult {
        std::string analysis_id;
        DocumentChange source_change;
        std::vector<NodeImpact> affected_nodes;
        int total_affected_count;
        double max_impact_score;
        double avg_impact_score;
        int max_propagation_depth;
        std::chrono::milliseconds computation_time;
        nlohmann::json metadata;
    };
    
    /**
     * @brief Analysiere Impact einer Dokumentänderung
     * 
     * @param change Die Änderung
     * @param config Konfiguration (max_depth, threshold, etc.)
     * @return Impact auf alle betroffenen Knoten
     */
    virtual ImpactAnalysisResult analyzeDocumentChangeImpact(
        const DocumentChange& change,
        const nlohmann::json& config
    ) = 0;
    
    /**
     * @brief Batch-Analyse mehrerer Änderungen
     * 
     * GPU-beschleunigt für parallele Verarbeitung
     */
    virtual std::vector<ImpactAnalysisResult> analyzeBatchChanges(
        const std::vector<DocumentChange>& changes,
        const nlohmann::json& config
    ) = 0;
    
    // ========================================================================
    // FEM-Inspired Graph Propagation
    // ========================================================================
    
    /**
     * @brief FEM-Konfiguration für Graph-Propagierung
     */
    struct FEMPropagationConfig {
        double damping_factor = 0.85;       // Dämpfung bei jedem Hop (ähnlich PageRank)
        double impact_threshold = 0.01;     // Minimaler Impact für Propagierung
        int max_iterations = 100;           // Max Iterationen für Konvergenz
        double convergence_threshold = 0.001;
        bool use_temporal_decay = true;     // Zeitabhängige Dämpfung
        double temporal_half_life_hours = 24.0;
    };
    
    /**
     * @brief FEM-basierte Impact-Propagierung
     * 
     * Simuliert Ausbreitung einer "Kraft" (Änderung) durch den Graphen,
     * analog zur Spannungsausbreitung in FEM.
     * 
     * @param source_nodes Start-Knoten
     * @param initial_impacts Initiale Impact-Werte
     * @param graph_structure Graph-Topologie
     * @param config FEM-Parameter
     * @return Finale Impact-Verteilung
     */
    virtual std::unordered_map<std::string, double> propagateImpactFEM(
        const std::vector<std::string>& source_nodes,
        const std::vector<double>& initial_impacts,
        const nlohmann::json& graph_structure,
        const FEMPropagationConfig& config
    ) = 0;
    
    /**
     * @brief GPU-beschleunigte Sparse-Matrix-Multiplikation für Propagierung
     * 
     * Nutzt cuSPARSE für effiziente Graph-Traversierung
     */
    virtual std::vector<double> sparseMatrixVectorMultiply_GPU(
        const nlohmann::json& adjacency_matrix,
        const std::vector<double>& input_vector
    ) = 0;
    
    // ========================================================================
    // Temporal Impact Analysis
    // ========================================================================
    
    /**
     * @brief Zeitreihe von Impacts
     */
    struct TemporalImpact {
        std::string node_id;
        std::vector<std::pair<int64_t, double>> impact_timeseries;
        double trend;                   // Slope der Zeitreihe
        double volatility;              // Standardabweichung
        std::optional<int64_t> peak_time;
        std::optional<double> peak_impact;
    };
    
    /**
     * @brief Analysiere Impact-Entwicklung über Zeit
     * 
     * @param changes Historische Änderungen
     * @param target_nodes Zu beobachtende Knoten
     * @param time_window Zeitfenster
     * @return Zeitreihen für jeden Knoten
     */
    virtual std::vector<TemporalImpact> analyzeTemporalImpact(
        const std::vector<DocumentChange>& changes,
        const std::vector<std::string>& target_nodes,
        std::chrono::hours time_window
    ) = 0;
    
    /**
     * @brief Forecast zukünftiger Impact (ARIMA-basiert, GPU-beschleunigt)
     * 
     * @param historical_impacts Historische Impact-Zeitreihen
     * @param forecast_horizon Wie weit in die Zukunft (in Stunden)
     * @return Vorhergesagte Impacts
     */
    virtual std::vector<TemporalImpact> forecastFutureImpact(
        const std::vector<TemporalImpact>& historical_impacts,
        int forecast_horizon_hours
    ) = 0;
    
    // ========================================================================
    // Monte Carlo Risk Assessment
    // ========================================================================
    
    /**
     * @brief Monte Carlo Simulation für Risikobewertung
     */
    struct MonteCarloConfig {
        int num_simulations = 10000;
        double uncertainty_factor = 0.2;    // Unsicherheit der Impact-Schätzung
        std::vector<std::string> risk_scenarios;
        bool use_gpu = true;
    };
    
    struct RiskAssessment {
        double value_at_risk_95;        // 95% Quantil
        double value_at_risk_99;        // 99% Quantil
        double expected_impact;         // Erwartungswert
        double max_impact;              // Maximum über alle Simulationen
        std::vector<double> impact_distribution;
        nlohmann::json scenario_probabilities;
    };
    
    /**
     * @brief Monte Carlo Risikobewertung für Änderung
     * 
     * GPU-beschleunigt (100-1000x Speedup)
     * 
     * @param change Geplante Änderung
     * @param config Simulation-Parameter
     * @return Risikobewertung
     */
    virtual RiskAssessment assessChangeRisk_MonteCarlo(
        const DocumentChange& change,
        const MonteCarloConfig& config
    ) = 0;
    
    // ========================================================================
    // Pattern Detection
    // ========================================================================
    
    /**
     * @brief Impact-Muster
     */
    struct ImpactPattern {
        std::string pattern_id;
        std::string pattern_type;       // cascade, isolated, cyclic, explosive
        std::vector<std::string> typical_nodes;
        double frequency;               // Wie oft kommt dieses Muster vor?
        double severity;                // Durchschnittlicher Impact
        nlohmann::json signature;       // FFT-Spektrum oder ähnliches
    };
    
    /**
     * @brief Erkenne wiederkehrende Impact-Muster (FFT-basiert)
     * 
     * GPU-beschleunigt (100-500x Speedup)
     * 
     * @param historical_results Historische Impact-Analysen
     * @return Erkannte Muster
     */
    virtual std::vector<ImpactPattern> detectImpactPatterns_FFT(
        const std::vector<ImpactAnalysisResult>& historical_results
    ) = 0;
    
    /**
     * @brief Finde ähnliche Impact-Szenarien (DTW-basiert)
     * 
     * @param query_impact Gesuchtes Impact-Szenario
     * @param historical_database Historische Szenarien
     * @param top_k Anzahl ähnlichster Szenarien
     * @return Ähnlichste Szenarien
     */
    virtual std::vector<std::pair<std::string, double>> findSimilarImpactScenarios_DTW(
        const ImpactAnalysisResult& query_impact,
        const std::vector<ImpactAnalysisResult>& historical_database,
        int top_k = 10
    ) = 0;
    
    // ========================================================================
    // Anomaly Detection
    // ========================================================================
    
    /**
     * @brief Impact-Anomalie
     */
    struct ImpactAnomaly {
        std::string anomaly_id;
        std::string anomaly_type;       // unexpected_high, unexpected_low, unusual_pattern
        double anomaly_score;
        std::vector<std::string> affected_nodes;
        std::string explanation;
        nlohmann::json details;
    };
    
    /**
     * @brief Erkenne anomale Impact-Muster (Isolation Forest)
     * 
     * GPU-beschleunigt (20-50x Speedup)
     * 
     * @param recent_impacts Kürzliche Impact-Analysen
     * @param config Anomaly-Detection-Parameter
     * @return Erkannte Anomalien
     */
    virtual std::vector<ImpactAnomaly> detectImpactAnomalies(
        const std::vector<ImpactAnalysisResult>& recent_impacts,
        const nlohmann::json& config
    ) = 0;
    
    // ========================================================================
    // What-If Analysis
    // ========================================================================
    
    /**
     * @brief What-If Szenario
     */
    struct WhatIfScenario {
        std::string scenario_name;
        std::vector<DocumentChange> hypothetical_changes;
        nlohmann::json context;         // Zusätzlicher Kontext
    };
    
    /**
     * @brief Simuliere Impact von hypothetischen Änderungen
     * 
     * @param scenarios Szenarien
     * @return Impact-Prognosen für jedes Szenario
     */
    virtual std::vector<ImpactAnalysisResult> simulateWhatIfScenarios(
        const std::vector<WhatIfScenario>& scenarios
    ) = 0;
    
    /**
     * @brief Vergleiche Szenarien (Side-by-Side)
     */
    struct ScenarioComparison {
        std::vector<std::string> scenario_names;
        nlohmann::json comparison_matrix;
        std::string recommended_scenario;
        std::string recommendation_reason;
    };
    
    virtual ScenarioComparison compareScenarios(
        const std::vector<WhatIfScenario>& scenarios
    ) = 0;
    
    // ========================================================================
    // Sensitivity Analysis
    // ========================================================================
    
    /**
     * @brief Sensitivitätsanalyse
     * 
     * Wie sensitiv ist der Impact auf Parameteränderungen?
     * 
     * @param base_change Basis-Änderung
     * @param parameters Parameter zum Variieren
     * @param variation_range Variationsbereich (z.B. ±20%)
     * @return Sensitivität für jeden Parameter
     */
    virtual nlohmann::json analyzeSensitivity(
        const DocumentChange& base_change,
        const std::vector<std::string>& parameters,
        double variation_range = 0.2
    ) = 0;
    
    // ========================================================================
    // Causal Graph Construction
    // ========================================================================
    
    /**
     * @brief Kausal-Graph
     */
    struct CausalGraph {
        std::vector<std::string> nodes;
        std::vector<std::tuple<std::string, std::string, double>> edges;  // from, to, strength
        nlohmann::json metadata;
    };
    
    /**
     * @brief Konstruiere Kausal-Graph aus historischen Daten
     * 
     * Nutzt Granger-Kausalität und statistische Tests
     * 
     * @param historical_changes Historische Änderungen
     * @param confidence_threshold Konfidenz-Schwelle für Kanten
     * @return Kausal-Graph
     */
    virtual CausalGraph buildCausalGraph(
        const std::vector<DocumentChange>& historical_changes,
        double confidence_threshold = 0.95
    ) = 0;
    
    /**
     * @brief Finde Root Causes für beobachteten Impact
     * 
     * @param observed_impact Beobachteter Impact
     * @param causal_graph Kausal-Graph
     * @param top_k Anzahl wahrscheinlichster Ursachen
     * @return Wahrscheinlichste Root Causes
     */
    virtual std::vector<std::pair<std::string, double>> findRootCauses(
        const ImpactAnalysisResult& observed_impact,
        const CausalGraph& causal_graph,
        int top_k = 5
    ) = 0;
    
    // ========================================================================
    // Performance Monitoring
    // ========================================================================
    
    /**
     * @brief Performance-Metriken
     */
    struct PerformanceMetrics {
        int64_t total_analyses;
        int64_t gpu_accelerated_analyses;
        double avg_analysis_time_ms;
        double avg_speedup;             // GPU vs CPU
        double gpu_utilization;         // 0.0-1.0
        int64_t total_nodes_analyzed;
        nlohmann::json detailed_stats;
    };
    
    virtual PerformanceMetrics getPerformanceMetrics() const = 0;
    virtual void resetPerformanceMetrics() = 0;
};

// ============================================================================
// Factory Function
// ============================================================================

/**
 * @brief Create GPU Impact Analysis Plugin instance
 * 
 * Factory function to create a new instance of the GPU Impact Analysis Plugin.
 * The plugin must be initialized with a configuration before use.
 * 
 * @return Pointer to new plugin instance (caller owns the memory)
 */
IGPUImpactAnalysisPlugin* createGPUImpactAnalysisPlugin();

} // namespace enterprise
} // namespace themis
