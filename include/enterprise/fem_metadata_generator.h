#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>
#include <optional>

namespace themis {
namespace enterprise {

/**
 * @brief FEM Metadata Generator for Impact Analysis
 * 
 * Generates FEM (Finite Element Method) inspired metadata for edges and nodes
 * during document ingestion. These metadata factors are used by the GPU Impact
 * Analysis plugin to simulate impact propagation through the graph.
 * 
 * ## Key Concepts
 * 
 * ### Edge Metadata (Stiffness/Damping)
 * - **weight**: Primary stiffness constant (0.0-1.0)
 * - **damping_coefficient**: Energy loss during propagation (0.0-1.0)
 * - **material_stiffness**: Type-specific stiffness (0.0-1.0)
 * - **criticality**: Categorical importance (low, medium, high, critical)
 * 
 * ### Node Metadata (Mass/Inertia)
 * - **inertia**: Resistance to change (0.0-1.0)
 * - **change_amplification**: Amplification/dampening factor (>0.0)
 * - **stability**: Historical stability (0.0-1.0)
 * - **impact_radius**: Maximum propagation hops
 * 
 * ## Usage
 * 
 * ```cpp
 * auto gen = FEMMetadataGenerator::instance();
 * 
 * // During edge creation
 * auto edge_metadata = gen.calculateEdgeMetadata(
 *     "DEPENDS_ON",
 *     from_node,
 *     to_node,
 *     {{"business_impact", "critical"}}
 * );
 * 
 * edge["fem_metadata"] = edge_metadata.toJson();
 * ```
 */
class FEMMetadataGenerator {
public:
    /**
     * @brief Edge FEM metadata
     */
    struct EdgeMetadata {
        double weight = 0.5;
        double damping_coefficient = 0.3;
        double material_stiffness = 0.5;
        double bidirectional_factor = 0.0;
        double temporal_decay_rate = 0.0;
        std::string criticality = "medium";
        double change_sensitivity = 0.5;
        double propagation_delay_hours = 0.0;
        
        nlohmann::json toJson() const;
        static EdgeMetadata fromJson(const nlohmann::json& j);
    };
    
    /**
     * @brief Node FEM metadata
     */
    struct NodeMetadata {
        double inertia = 0.5;
        double elasticity = 0.5;
        double stability = 0.5;
        double change_amplification = 1.0;
        int impact_radius = 5;
        std::string criticality = "medium";
        
        nlohmann::json toJson() const;
        static NodeMetadata fromJson(const nlohmann::json& j);
    };
    
    /**
     * @brief Get singleton instance
     */
    static FEMMetadataGenerator& instance();
    
    /**
     * @brief Initialize with configuration
     */
    bool initialize(const nlohmann::json& config);
    
    /**
     * @brief Load edge type defaults from YAML file
     */
    bool loadEdgeTypeDefaults(const std::string& yaml_file);
    
    /**
     * @brief Calculate edge metadata based on type and context
     * 
     * @param edge_type Edge type (e.g., "DEPENDS_ON", "REFERENCES")
     * @param from_node Source node
     * @param to_node Target node
     * @param context Additional context for calculation
     * @return Calculated FEM metadata
     */
    EdgeMetadata calculateEdgeMetadata(
        const std::string& edge_type,
        const nlohmann::json& from_node,
        const nlohmann::json& to_node,
        const nlohmann::json& context = {}
    );
    
    /**
     * @brief Calculate node metadata based on history
     * 
     * @param node The node to calculate metadata for
     * @param historical_changes Historical change records
     * @param context Additional context
     * @return Calculated FEM metadata
     */
    NodeMetadata calculateNodeMetadata(
        const nlohmann::json& node,
        const std::vector<nlohmann::json>& historical_changes = {},
        const nlohmann::json& context = {}
    );
    
    /**
     * @brief Get default edge metadata for a type
     */
    std::optional<EdgeMetadata> getEdgeTypeDefaults(const std::string& edge_type) const;
    
    /**
     * @brief Register custom edge type defaults
     */
    void registerEdgeTypeDefaults(const std::string& edge_type, const EdgeMetadata& defaults);
    
    /**
     * @brief Validate FEM metadata
     */
    bool validateEdgeMetadata(const nlohmann::json& metadata) const;
    bool validateNodeMetadata(const nlohmann::json& metadata) const;
    
private:
    FEMMetadataGenerator() = default;
    ~FEMMetadataGenerator() = default;
    FEMMetadataGenerator(const FEMMetadataGenerator&) = delete;
    FEMMetadataGenerator& operator=(const FEMMetadataGenerator&) = delete;
    
    // Internal calculation methods
    double calculateCoChangeRate(const std::string& from_id, const std::string& to_id);
    double calculateNodeCentrality(const std::string& node_id);
    double estimateChangeSensitivity(const nlohmann::json& node);
    double getBusinessImpactModifier(const std::string& business_impact);
    double getUpdateFrequencyModifier(const std::string& update_frequency);
    double getDependencyTypeModifier(const std::string& dependency_type);
    
    // Calculate node properties from history
    double calculateInertia(const std::vector<nlohmann::json>& historical_changes);
    double calculateStability(const std::vector<nlohmann::json>& historical_changes);
    double calculateElasticity(const nlohmann::json& node);
    
    // Edge type defaults (loaded from YAML)
    std::unordered_map<std::string, EdgeMetadata> edge_type_defaults_;
    
    // Configuration
    nlohmann::json config_;
    bool initialized_ = false;
};

} // namespace enterprise
} // namespace themis
