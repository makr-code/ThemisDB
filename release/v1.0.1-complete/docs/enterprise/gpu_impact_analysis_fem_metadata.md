# FEM-Inspired Impact Analysis - Metadata Factors & Ingestion

**Version:** 1.0.0  
**Date:** December 7, 2025  
**Status:** Design Specification

---

## 1. Überblick

Dieses Dokument beschreibt, welche Faktoren (Kennwerte) von Dokumenten-Verbindungen die Dämpfungs- und Materialkonstanten aus der Finite Element Methode (FEM) widerspiegeln sollen, und wie diese Metadaten bereits bei der Ingestion erzeugt werden können.

---

## 2. FEM-Analogie zu Graph-Verbindungen

### 2.1 FEM-Konzepte → Graph-Mapping

| FEM-Konzept | Graph-Äquivalent | Dokumenten-Faktor |
|-------------|------------------|-------------------|
| **Steifigkeit (Stiffness)** | Kantengewicht (Edge Weight) | Stärke der Abhängigkeit |
| **Dämpfung (Damping)** | Propagations-Koeffizient | Verlustrate bei Weitergabe |
| **Masse (Mass)** | Knotenträgheit | Widerstand gegen Änderung |
| **Elastizitätsmodul (E)** | Material-Konstante | Typ-spezifische Steifigkeit |
| **Querkontraktionszahl (ν)** | Lateral-Koeffizient | Seiteneffekte |
| **Externe Kraft** | Änderungsmagnitude | Stärke der Dokumentänderung |

### 2.2 Edge Weight als primäre Steifigkeitskonstante

Die **Kantengewichtung** (`_weight`) repräsentiert die Steifigkeit der Verbindung in FEM-Terminologie:

```
Hohe Steifigkeit (weight → 1.0):
- Starke Kopplung
- Änderungen propagieren fast vollständig
- Beispiel: Parent-Child, kritische Abhängigkeiten

Niedrige Steifigkeit (weight → 0.0):
- Schwache Kopplung  
- Änderungen werden stark gedämpft
- Beispiel: Ähnlichkeits-Links, optionale Referenzen
```

---

## 3. Metadaten-Faktoren für Impact-Analyse

### 3.1 Edge-Level Metadata (Kanten-Metadaten)

Jede Kante sollte folgende Metadaten bei der Ingestion erhalten:

```json
{
  "edge_id": "e_123",
  "_from": "doc_A",
  "_to": "doc_B",
  "_type": "DEPENDS_ON",
  
  // FEM-inspirierte Faktoren
  "fem_metadata": {
    // 1. Steifigkeit / Gewicht (0.0 - 1.0)
    "_weight": 0.8,
    
    // 2. Dämpfungsfaktor (0.0 - 1.0)
    // Wie viel Impact wird bei Propagierung verloren?
    "damping_coefficient": 0.15,
    
    // 3. Materialkonstante (edge type spezifisch)
    // Beeinflusst wie sich Impact ausbreitet
    "material_stiffness": 0.9,
    
    // 4. Bidirektionalität
    // Propagiert Impact auch rückwärts?
    "bidirectional_factor": 0.3,
    
    // 5. Temporal Decay Rate
    // Wie schnell nimmt der Impact über Zeit ab?
    "temporal_decay_rate": 0.05,
    
    // 6. Kritikalität
    // Kritische Verbindungen haben höhere Propagierung
    "criticality": "high",  // low, medium, high, critical
    
    // 7. Änderungs-Sensitivität
    // Wie sensitiv ist die Ziel-Node auf Änderungen?
    "change_sensitivity": 0.7,
    
    // 8. Propagierungs-Verzögerung (in Stunden)
    // Zeitverzögerung bis Impact sichtbar wird
    "propagation_delay_hours": 0.0
  },
  
  // Zusätzliche Kontext-Informationen
  "context": {
    "dependency_type": "functional",  // functional, data, structural, reference
    "update_frequency": "high",       // low, medium, high
    "business_impact": "critical"     // low, medium, high, critical
  }
}
```

### 3.2 Node-Level Metadata (Knoten-Metadaten)

Dokumente sollten folgende Metadaten haben:

```json
{
  "_id": "doc_A",
  "_key": "doc_A",
  
  // FEM-inspirierte Node-Faktoren
  "fem_metadata": {
    // 1. Masse / Trägheit
    // Wie resistent ist das Dokument gegen Änderungen?
    "inertia": 0.5,
    
    // 2. Elastizität
    // Wie schnell "erholt" sich das Dokument?
    "elasticity": 0.8,
    
    // 3. Stabilität
    // Wie stabil ist das Dokument (wenig Änderungen)?
    "stability": 0.9,
    
    // 4. Change Amplification
    // Verstärkt oder dämpft das Dokument Änderungen?
    "change_amplification": 1.0,
    
    // 5. Impact Radius
    // Wie weit propagiert Impact von diesem Dokument?
    "impact_radius": 3,  // Max Hops
    
    // 6. Kritikalität
    "criticality": "high"
  },
  
  "context": {
    "document_type": "specification",
    "update_frequency": "weekly",
    "owner": "team_architecture",
    "business_value": "high"
  }
}
```

---

## 4. Berechnung der Faktoren bei Ingestion

### 4.1 Automatische Faktoren-Berechnung

**EdgeWeightCalculator** - Berechnet Kantengewicht basierend auf:

```cpp
class FEMMetadataGenerator {
public:
    struct EdgeMetadata {
        double weight;
        double damping_coefficient;
        double material_stiffness;
        double bidirectional_factor;
        double temporal_decay_rate;
        std::string criticality;
        double change_sensitivity;
        double propagation_delay_hours;
    };
    
    /**
     * @brief Berechne Edge-Metadaten basierend auf Typ und Kontext
     */
    EdgeMetadata calculateEdgeMetadata(
        const std::string& edge_type,
        const nlohmann::json& from_node,
        const nlohmann::json& to_node,
        const nlohmann::json& context = {}
    );
    
    /**
     * @brief Berechne Node-Metadaten basierend auf Historie
     */
    nlohmann::json calculateNodeMetadata(
        const nlohmann::json& node,
        const std::vector<nlohmann::json>& historical_changes,
        const nlohmann::json& context = {}
    );
};
```

### 4.2 Edge Type → Default Factors Mapping

```yaml
# fem_edge_type_defaults.yaml

edge_types:
  # STRUCTURAL - Starke Kopplung
  PARENT_OF:
    weight: 0.95
    damping_coefficient: 0.05
    material_stiffness: 0.95
    bidirectional_factor: 0.8  # Kinder beeinflussen Eltern auch
    criticality: "critical"
    
  CHILD_OF:
    weight: 0.90
    damping_coefficient: 0.10
    material_stiffness: 0.90
    bidirectional_factor: 0.3
    criticality: "high"
    
  CONTAINS:
    weight: 0.85
    damping_coefficient: 0.15
    material_stiffness: 0.85
    bidirectional_factor: 0.6
    criticality: "high"
  
  # REFERENCE - Mittlere Kopplung
  REFERENCES:
    weight: 0.60
    damping_coefficient: 0.30
    material_stiffness: 0.60
    bidirectional_factor: 0.1
    criticality: "medium"
    
  LINKS_TO:
    weight: 0.40
    damping_coefficient: 0.50
    material_stiffness: 0.40
    bidirectional_factor: 0.05
    criticality: "low"
    
  CITES:
    weight: 0.50
    damping_coefficient: 0.40
    material_stiffness: 0.50
    bidirectional_factor: 0.2
    criticality: "medium"
  
  # WORKFLOW - Prozess-abhängig
  DEPENDS_ON:
    weight: 0.90
    damping_coefficient: 0.10
    material_stiffness: 0.90
    bidirectional_factor: 0.0  # Nur forward
    criticality: "critical"
    
  TRIGGERS:
    weight: 0.80
    damping_coefficient: 0.20
    material_stiffness: 0.80
    bidirectional_factor: 0.0
    criticality: "high"
    
  FOLLOWS:
    weight: 0.70
    damping_coefficient: 0.25
    material_stiffness: 0.70
    bidirectional_factor: 0.1
    criticality: "medium"
  
  # SEMANTIC - Schwache Kopplung
  SIMILAR_TO:
    weight: 0.30
    damping_coefficient: 0.60
    material_stiffness: 0.30
    bidirectional_factor: 0.5  # Symmetrisch
    criticality: "low"
    
  RELATED_TO:
    weight: 0.40
    damping_coefficient: 0.50
    material_stiffness: 0.40
    bidirectional_factor: 0.4
    criticality: "low"
  
  # TEMPORAL - Zeit-abhängig
  SUCCEEDED_BY:
    weight: 0.75
    damping_coefficient: 0.20
    material_stiffness: 0.75
    bidirectional_factor: 0.0
    temporal_decay_rate: 0.1  # Nimmt über Zeit ab
    criticality: "medium"
    
  VALID_DURING:
    weight: 0.85
    damping_coefficient: 0.15
    material_stiffness: 0.85
    temporal_decay_rate: 0.05
    criticality: "high"

# Context-basierte Anpassungen
context_modifiers:
  # Business Impact erhöht Gewicht
  business_impact:
    critical: 1.2
    high: 1.1
    medium: 1.0
    low: 0.8
  
  # Update Frequency beeinflusst Dämpfung
  update_frequency:
    high: 0.8    # Weniger Dämpfung bei häufigen Updates
    medium: 1.0
    low: 1.2     # Mehr Dämpfung bei seltenen Updates
  
  # Dependency Type
  dependency_type:
    functional: 1.2   # Funktionale Abhängigkeiten wichtiger
    data: 1.1
    structural: 1.0
    reference: 0.8
```

### 4.3 Berechnung basierend auf historischen Daten

```cpp
EdgeMetadata FEMMetadataGenerator::calculateEdgeMetadata(
    const std::string& edge_type,
    const nlohmann::json& from_node,
    const nlohmann::json& to_node,
    const nlohmann::json& context
) {
    EdgeMetadata metadata;
    
    // 1. Basis-Werte aus Edge Type
    auto defaults = getEdgeTypeDefaults(edge_type);
    metadata.weight = defaults.weight;
    metadata.damping_coefficient = defaults.damping_coefficient;
    metadata.material_stiffness = defaults.material_stiffness;
    
    // 2. Context-basierte Anpassungen
    if (context.contains("business_impact")) {
        auto modifier = getBusinessImpactModifier(context["business_impact"]);
        metadata.weight *= modifier;
    }
    
    if (context.contains("update_frequency")) {
        auto modifier = getUpdateFrequencyModifier(context["update_frequency"]);
        metadata.damping_coefficient *= modifier;
    }
    
    // 3. Historische Analyse
    // Wenn Knoten häufig zusammen geändert werden → höheres Gewicht
    auto co_change_rate = analyzeCoChangeHistory(from_node["_id"], to_node["_id"]);
    if (co_change_rate > 0.5) {
        metadata.weight = std::min(1.0, metadata.weight * 1.2);
    }
    
    // 4. Strukturelle Position
    // Zentrale Knoten haben höheren Impact
    auto from_centrality = calculateNodeCentrality(from_node["_id"]);
    auto to_centrality = calculateNodeCentrality(to_node["_id"]);
    
    if (from_centrality > 0.8 || to_centrality > 0.8) {
        metadata.criticality = "critical";
        metadata.weight *= 1.1;
    }
    
    // 5. Sensitivität aus Ziel-Node
    if (to_node.contains("fem_metadata") && 
        to_node["fem_metadata"].contains("change_sensitivity")) {
        metadata.change_sensitivity = to_node["fem_metadata"]["change_sensitivity"];
    } else {
        // Default basierend auf Node-Typ
        metadata.change_sensitivity = estimateChangeSensitivity(to_node);
    }
    
    return metadata;
}
```

---

## 5. Integration in Ingestion Pipeline

### 5.1 Content Ingestion Hook

```cpp
// In ContentManager::ingestContent()

IngestionResult ContentManager::ingestContent(
    const std::string& content_id,
    const std::vector<uint8_t>& data,
    const ContentType& type,
    const nlohmann::json& metadata
) {
    // ... existing ingestion ...
    
    // 1. Generate FEM metadata for document node
    auto fem_metadata_gen = FEMMetadataGenerator::instance();
    auto node_metadata = fem_metadata_gen.calculateNodeMetadata(
        content_entity,
        getHistoricalChanges(content_id),
        metadata
    );
    
    // Add to content entity
    content_entity["fem_metadata"] = node_metadata;
    
    // 2. Store node with FEM metadata
    storage_->put(content_key, content_entity.dump());
    
    // 3. Extract and create edges with FEM metadata
    auto edges = extractDocumentRelationships(content_entity, data, type);
    
    for (auto& edge : edges) {
        // Calculate FEM metadata for this edge
        auto edge_metadata = fem_metadata_gen.calculateEdgeMetadata(
            edge["_type"],
            content_entity,
            getNode(edge["_to"]),
            edge.value("context", nlohmann::json{})
        );
        
        // Add FEM metadata to edge
        edge["fem_metadata"] = {
            {"_weight", edge_metadata.weight},
            {"damping_coefficient", edge_metadata.damping_coefficient},
            {"material_stiffness", edge_metadata.material_stiffness},
            {"bidirectional_factor", edge_metadata.bidirectional_factor},
            {"temporal_decay_rate", edge_metadata.temporal_decay_rate},
            {"criticality", edge_metadata.criticality},
            {"change_sensitivity", edge_metadata.change_sensitivity},
            {"propagation_delay_hours", edge_metadata.propagation_delay_hours}
        };
        
        // Store edge
        std::string edge_key = "edge:" + edge["_id"].get<std::string>();
        storage_->put(edge_key, edge.dump());
        
        // Update graph indexes
        graph_index_->addEdge(edge);
    }
    
    return IngestionResult::success(content_id);
}
```

### 5.2 Edge Creation API Extension

```cpp
// HTTP API: POST /graph/edge/create
{
  "from": "doc_A",
  "to": "doc_B",
  "type": "DEPENDS_ON",
  
  // Optional: Provide context for FEM calculation
  "context": {
    "business_impact": "critical",
    "update_frequency": "high",
    "dependency_type": "functional"
  },
  
  // Optional: Override auto-calculated values
  "fem_metadata": {
    "_weight": 0.95  // Override if you have specific knowledge
  }
}

// Response includes calculated FEM metadata
{
  "edge_id": "e_456",
  "from": "doc_A",
  "to": "doc_B",
  "type": "DEPENDS_ON",
  "fem_metadata": {
    "_weight": 0.95,
    "damping_coefficient": 0.08,
    "material_stiffness": 0.92,
    "criticality": "critical",
    // ... all FEM factors
  }
}
```

---

## 6. Verwendung in Impact-Analyse

### 6.1 FEM Propagierung mit Metadaten

```cpp
std::unordered_map<std::string, double> GPUImpactAnalysisPlugin::propagateImpactFEM(
    const std::vector<std::string>& source_nodes,
    const std::vector<double>& initial_impacts,
    const nlohmann::json& graph_structure,
    const FEMPropagationConfig& config
) {
    std::unordered_map<std::string, double> impact_map;
    
    // Initialize
    for (size_t i = 0; i < source_nodes.size(); ++i) {
        impact_map[source_nodes[i]] = initial_impacts[i];
    }
    
    // Iterative propagation with FEM metadata
    for (int iter = 0; iter < config.max_iterations; ++iter) {
        std::unordered_map<std::string, double> new_impact_map;
        
        for (const auto& [node_id, current_impact] : impact_map) {
            double incoming_impact = 0.0;
            
            // Get incoming edges with FEM metadata
            auto in_edges = getIncomingEdgesWithMetadata(node_id, graph_structure);
            
            for (const auto& edge : in_edges) {
                std::string source = edge["from"];
                
                if (impact_map.count(source)) {
                    // Use FEM metadata from edge
                    double weight = edge["fem_metadata"].value("_weight", 1.0);
                    double damping = edge["fem_metadata"].value("damping_coefficient", 0.15);
                    double stiffness = edge["fem_metadata"].value("material_stiffness", 0.85);
                    
                    // Effective damping = global_damping * edge_damping
                    double effective_damping = config.damping_factor * (1.0 - damping);
                    
                    // Impact propagates with weight and stiffness
                    double propagated = impact_map[source] * weight * stiffness * effective_damping;
                    
                    incoming_impact += propagated;
                }
            }
            
            // Get node metadata for inertia/resistance
            auto node_metadata = getNodeMetadata(node_id);
            double inertia = node_metadata.value("inertia", 0.5);
            double amplification = node_metadata.value("change_amplification", 1.0);
            
            // New impact considers node inertia and amplification
            double new_impact = current_impact * (1.0 - inertia) + 
                               incoming_impact * amplification;
            
            new_impact_map[node_id] = new_impact;
        }
        
        impact_map = std::move(new_impact_map);
        
        // Check convergence...
    }
    
    return impact_map;
}
```

### 6.2 AQL Queries mit FEM Metadata

```sql
-- Finde kritische Pfade (hohe Steifigkeit)
FOR edge IN edges
  FILTER edge.fem_metadata._weight > 0.8
  FILTER edge.fem_metadata.criticality IN ['critical', 'high']
  RETURN {
    from: edge._from,
    to: edge._to,
    weight: edge.fem_metadata._weight,
    criticality: edge.fem_metadata.criticality
  }

-- Analysiere Impact mit FEM-Faktoren
LET change = {
  document_id: 'docs/api-spec',
  magnitude: 0.9
}

LET impact = GPU_ANALYZE_IMPACT(change, {
  use_fem_metadata: true,  // Nutze Edge/Node FEM Metadata
  max_depth: 10
})

FOR node IN impact.affected_nodes
  SORT node.impact_score DESC
  LIMIT 20
  RETURN {
    node: node.node_id,
    impact: node.impact_score,
    confidence: node.confidence,
    propagation_path: node.propagation_path
  }
```

---

## 7. Machine Learning für Faktoren-Optimierung

### 7.1 Lern-basierte Gewichtsanpassung

```cpp
class FEMMetadataLearner {
public:
    /**
     * @brief Lerne optimale FEM-Faktoren aus historischen Impact-Daten
     * 
     * Nutzt beobachtete Impact-Propagierungen um Edge-Gewichte zu optimieren
     */
    void trainFromHistory(
        const std::vector<ImpactAnalysisResult>& historical_results
    );
    
    /**
     * @brief Aktualisiere Edge-Gewichte basierend auf Feedback
     */
    void updateEdgeWeights(
        const std::string& edge_id,
        double observed_propagation,
        double expected_propagation
    );
};
```

### 7.2 Feedback Loop

```
1. Impact-Analyse durchführen
   ↓
2. Beobachte tatsächliche Änderungen
   ↓
3. Vergleiche vorhergesagte vs. tatsächliche Impacts
   ↓
4. Passe FEM-Faktoren an (Learning)
   ↓
5. Update Edge Metadata in Datenbank
```

---

## 8. Konfiguration

### 8.1 FEM Metadata Generation Config

```yaml
# config/fem_metadata_generation.yaml

fem_metadata:
  # Enable automatic FEM metadata generation
  enabled: true
  
  # Source for defaults
  edge_type_defaults_file: "config/fem_edge_type_defaults.yaml"
  
  # Historical analysis
  historical_analysis:
    enabled: true
    lookback_days: 90
    min_co_changes: 3
  
  # Machine learning
  learning:
    enabled: false  # Experimental
    update_frequency: "weekly"
    min_samples: 100
  
  # Override policy
  allow_manual_override: true
  preserve_existing: true  # Don't overwrite existing metadata

# Integration points
ingestion:
  generate_node_metadata: true
  generate_edge_metadata: true
  
graph_operations:
  validate_fem_metadata: true
  require_weight: false  # Don't require _weight on all edges
```

---

## 9. Best Practices

### 9.1 Wann welche Faktoren setzen?

**Bei Ingestion setzen:**
- `_weight` - Aus Edge Type + Context
- `damping_coefficient` - Aus Edge Type
- `material_stiffness` - Aus Edge Type
- `criticality` - Aus Business Context

**Bei Graph-Analyse aktualisieren:**
- `change_sensitivity` - Aus beobachteten Reaktionen
- `temporal_decay_rate` - Aus zeitlichen Mustern
- `inertia` - Aus Änderungshäufigkeit

**Durch ML optimieren:**
- `_weight` - Feintuning basierend auf Feedback
- `bidirectional_factor` - Aus beobachteten Rückwirkungen

### 9.2 Validierung

```cpp
bool validateFEMMetadata(const nlohmann::json& edge) {
    if (!edge.contains("fem_metadata")) {
        return false;
    }
    
    auto fem = edge["fem_metadata"];
    
    // Check ranges
    if (fem.contains("_weight")) {
        double w = fem["_weight"];
        if (w < 0.0 || w > 1.0) return false;
    }
    
    if (fem.contains("damping_coefficient")) {
        double d = fem["damping_coefficient"];
        if (d < 0.0 || d > 1.0) return false;
    }
    
    // Consistency checks
    // High weight should have low damping
    if (fem.contains("_weight") && fem.contains("damping_coefficient")) {
        double w = fem["_weight"];
        double d = fem["damping_coefficient"];
        // Warning if inconsistent
        if (w > 0.8 && d > 0.5) {
            spdlog::warn("Inconsistent FEM metadata: high weight with high damping");
        }
    }
    
    return true;
}
```

---

## 10. Zusammenfassung

### 10.1 Kernfaktoren

**Edge-Level (Steifigkeit):**
1. `_weight` - Primäre Steifigkeitskonstante (0.0-1.0)
2. `damping_coefficient` - Energieverlust bei Propagierung (0.0-1.0)
3. `material_stiffness` - Typ-spezifische Konstante (0.0-1.0)
4. `criticality` - Kategorische Wichtigkeit

**Node-Level (Masse/Trägheit):**
1. `inertia` - Widerstand gegen Änderungen (0.0-1.0)
2. `change_amplification` - Verstärkung/Dämpfung (>0.0)
3. `stability` - Historische Stabilität (0.0-1.0)

### 10.2 Ingestion-Strategie

1. **Edge Type Defaults** - Basis-Werte pro Edge-Typ
2. **Context Modifiers** - Business/Technical Context
3. **Historical Analysis** - Co-Change Patterns
4. **Structural Analysis** - Graph-Position (Centrality)
5. **Manual Override** - Experten-Wissen

### 10.3 Nächste Schritte

- [ ] Implementiere `FEMMetadataGenerator` Klasse
- [ ] Erstelle `fem_edge_type_defaults.yaml`
- [ ] Integriere in Content Ingestion Pipeline
- [ ] Erweitere Graph API für FEM Metadata
- [ ] Validierung & Testing
- [ ] Dokumentation für Endnutzer
- [ ] ML-basierte Optimierung (Phase 2)

---

**Erstellt:** 7. Dezember 2025  
**Version:** 1.0.0  
**Autor:** ThemisDB Team
