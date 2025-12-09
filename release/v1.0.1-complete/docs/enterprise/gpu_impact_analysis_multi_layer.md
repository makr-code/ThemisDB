# Multi-Layer Impact Analysis

**Version:** 1.0.0  
**Date:** 2025-12-07  
**Feature:** Universal Multi-Layer FEM Analysis

---

## Overview

The GPU Impact Analysis Plugin supports **universal multi-layer analysis**, enabling impact tracking across different architectural layers without requiring core database modifications.

## Supported Layers

### 1. Document Layer
- **Type:** `document`
- **Purpose:** Document/data storage layer
- **Examples:** Markdown files, configuration files, data documents
- **Metadata:** `_layer: "document"`

### 2. Process Layer (BPMN)
- **Type:** `process`
- **Purpose:** Business process/workflow layer
- **Examples:** BPMN workflows, EPK processes, state machines
- **Metadata:** `_layer: "process"`, `task_type`, `process_id`

### 3. API Layer
- **Type:** `api`
- **Purpose:** Service/API layer
- **Examples:** REST endpoints, GraphQL APIs, gRPC services
- **Metadata:** `_layer: "api"`, `api_version`, `endpoint_type`

### 4. Database Layer
- **Type:** `database`
- **Purpose:** Database schema layer
- **Examples:** Tables, schemas, stored procedures
- **Metadata:** `_layer: "database"`, `schema_name`, `table_type`

### 5. UI Layer
- **Type:** `ui`
- **Purpose:** User interface layer
- **Examples:** React components, Vue pages, Angular modules
- **Metadata:** `_layer: "ui"`, `component_type`, `framework`

### 6. Infrastructure Layer
- **Type:** `infrastructure`
- **Purpose:** Deployment/infrastructure layer
- **Examples:** Docker containers, Kubernetes configs, CI/CD pipelines
- **Metadata:** `_layer: "infrastructure"`

### 7. Custom Layers
- **Type:** `custom`
- **Purpose:** User-defined layers
- **Examples:** Any domain-specific layer
- **Metadata:** `_layer: "custom"`, user-defined properties

---

## Layer Metadata Schema

### Document Structure

```json
{
  "_id": "api/v2/payment/process",
  "_layer": "api",
  "_layer_metadata": {
    "api_version": "v2",
    "endpoint_type": "REST",
    "public_facing": true,
    "criticality": 0.95
  },
  "content": "..."
}
```

### Process Node Example

```json
{
  "_id": "order_workflow/validate_payment",
  "_layer": "process",
  "_layer_metadata": {
    "process_id": "order_workflow",
    "task_type": "SERVICE_TASK",
    "criticality": 0.90
  }
}
```

---

## Cross-Layer Edge Types

### Edge Type Definitions

| Edge Type | From Layer | To Layer | Description |
|-----------|------------|----------|-------------|
| `DOCUMENT_USES_API` | document | api | Document references API |
| `PROCESS_CALLS_API` | process | api | Process task calls API |
| `API_QUERIES_DATABASE` | api | database | API queries database |
| `UI_CALLS_API` | ui | api | UI component calls API |
| `UI_TRIGGERS_PROCESS` | ui | process | UI triggers workflow |
| `PROCESS_UPDATES_DOCUMENT` | process | document | Process updates document |
| `DATABASE_SCHEMA_FOR_API` | database | api | Database schema for API |
| `API_RETURNS_TO_UI` | api | ui | API returns data to UI |

### Edge Example

```json
{
  "_from": "ui/checkout/payment-form",
  "_to": "api/v2/payment/process",
  "_type": "UI_CALLS_API",
  "weight": 0.95,
  "_edge_metadata": {
    "criticality": 0.95,
    "frequency": "high"
  }
}
```

---

## Layer-Specific Damping Factors

### Concept

Different layers have different impact propagation characteristics:
- **High damping (e.g., 0.90)**: Impact propagates well within the layer
- **Low damping (e.g., 0.70)**: Impact is more contained/localized

### Default Configuration

```yaml
layer_damping_factors:
  document: 0.90      # Documents propagate well
  process: 0.85       # Process workflows propagate well
  api: 0.95           # API changes have high propagation
  database: 0.80      # Database changes are more contained
  ui: 0.75            # UI changes are more localized
  infrastructure: 0.70 # Infrastructure changes are contained
```

### Cross-Layer Damping

```yaml
cross_layer_damping:
  "api->process": 0.90      # API → Process (high impact)
  "api->database": 0.85     # API → Database
  "api->ui": 0.80           # API → UI
  "process->api": 0.75      # Process → API (moderate)
  "database->api": 0.85     # Database → API
  "ui->api": 0.60           # UI → API (lower impact)
```

---

## Usage Examples

### Example 1: API Breaking Change Impact

```cpp
#include "enterprise/gpu_impact_analysis_plugin.h"

auto plugin = createGPUImpactAnalysisPlugin();
plugin->initialize({{"gpu_backend", "cpu"}});

// API change with layer metadata
IGPUImpactAnalysisPlugin::DocumentChange change;
change.document_id = "api/v2/payment/process";
change.change_type = "breaking_change";
change.magnitude = 0.95;
change.source_layer = "api";

// Analyze impact across all layers
std::vector<std::string> target_layers = {"process", "ui", "database"};

nlohmann::json config = {
    {"fem", {
        {"layer_damping_factors", {
            {"api", 0.95},
            {"process", 0.85},
            {"ui", 0.75}
        }},
        {"cross_layer_damping", {
            {"api->process", 0.90},
            {"api->ui", 0.80}
        }}
    }}
};

auto result = plugin->analyzeMultiLayerImpact(change, target_layers, config);

// Results with cross-layer tracking
std::cout << "Total affected nodes: " << result.total_affected_count << std::endl;
std::cout << "Cross-layer transitions: " << result.cross_layer_transitions << std::endl;

for (const auto& [layer, count] : result.affected_nodes_per_layer) {
    std::cout << "Layer " << layer << ": " << count << " nodes affected" << std::endl;
}

for (const auto& node : result.affected_nodes) {
    if (node.is_cross_layer_impact) {
        std::cout << "Cross-layer impact: " << node.node_id 
                  << " (layers: ";
        for (const auto& l : node.crossed_layers) {
            std::cout << l << " ";
        }
        std::cout << ")" << std::endl;
    }
}
```

### Example 2: Process Change Impact

```cpp
IGPUImpactAnalysisPlugin::DocumentChange process_change;
process_change.document_id = "order_workflow/validate_payment";
process_change.change_type = "task_removed";
process_change.magnitude = 1.0;
process_change.source_layer = "process";

// Layer metadata
IGPUImpactAnalysisPlugin::LayerMetadata layer_meta;
layer_meta.layer_type = IGPUImpactAnalysisPlugin::LayerType::PROCESS;
layer_meta.layer_name = "order_workflow";
layer_meta.criticality = 0.90;
layer_meta.layer_properties = {
    {"task_type", "SERVICE_TASK"},
    {"depends_on_api", "api/v2/payment/validate"}
};
process_change.layer_metadata = layer_meta;

auto result = plugin->analyzeMultiLayerImpact(
    process_change, 
    {}, // Analyze all layers
    config
);
```

### Example 3: Database Schema Change

```cpp
IGPUImpactAnalysisPlugin::DocumentChange db_change;
db_change.document_id = "schema/customers/email_column";
db_change.change_type = "column_removed";
db_change.magnitude = 0.85;
db_change.source_layer = "database";

nlohmann::json db_config = {
    {"fem", {
        {"cross_layer_damping", {
            {"database->api", 0.85},
            {"database->process", 0.65}
        }}
    }}
};

auto result = plugin->analyzeMultiLayerImpact(db_change, {}, db_config);
```

---

## Analysis Results

### Multi-Layer Result Structure

```cpp
struct ImpactAnalysisResult {
    // Standard fields
    std::string analysis_id;
    DocumentChange source_change;
    std::vector<NodeImpact> affected_nodes;
    
    // Multi-layer statistics
    std::map<std::string, int> affected_nodes_per_layer;
    std::map<std::string, double> max_impact_per_layer;
    int cross_layer_transitions;
    std::vector<std::pair<std::string, std::string>> layer_transition_paths;
};
```

### Node Impact with Layer Info

```cpp
struct NodeImpact {
    std::string node_id;
    double impact_score;
    
    // Layer information
    std::string node_layer;
    std::vector<std::string> crossed_layers;
    bool is_cross_layer_impact;
};
```

---

## Benefits

### 1. No Core Modifications Required
- ✅ Uses existing graph structure
- ✅ Layer info stored as metadata (`_layer`, `_layer_metadata`)
- ✅ Works with current ThemisDB implementation

### 2. Universal Applicability
- ✅ Works for any layer type
- ✅ Extensible with custom layers
- ✅ Layer-agnostic FEM algorithm

### 3. Cross-Layer Impact Tracking
- ✅ Tracks impact propagation across layers
- ✅ Identifies cross-layer dependencies
- ✅ Highlights critical cross-layer transitions

### 4. Configurable Propagation
- ✅ Layer-specific damping factors
- ✅ Cross-layer damping configuration
- ✅ Per-analysis customization

---

## Integration with C# Visualization Tool

The C# multi-layer visualization tool will display:

1. **Layer Separation**: Different colors/heights for different layers
2. **Cross-Layer Edges**: Special rendering for cross-layer connections
3. **Layer Statistics**: Per-layer impact summaries
4. **Transition Paths**: Visual flow of impact across layers

See `docs/enterprise/gpu_impact_analysis_visualization.md` for details.

---

## Best Practices

### 1. Layer Assignment
- Assign `_layer` field to all documents
- Use consistent layer names across the system
- Set appropriate `criticality` values

### 2. Cross-Layer Edges
- Use descriptive edge types (e.g., `PROCESS_CALLS_API`)
- Set accurate edge weights based on dependency strength
- Document edge type semantics

### 3. Damping Configuration
- Start with default damping factors
- Tune based on observed impact patterns
- Use higher damping for well-connected layers
- Use lower damping for isolated layers

### 4. Performance
- Limit target layers for focused analysis
- Use appropriate impact thresholds
- Enable GPU acceleration for large graphs

---

## Future Enhancements

- **v1.1**: GPU-accelerated multi-layer analysis
- **v1.2**: Machine learning for auto-tuning damping factors
- **v2.0**: Dynamic layer detection and classification
- **v2.1**: Layer-specific impact prediction models

---

**Status:** ✅ Implemented  
**Last Updated:** 2025-12-07  
**Documentation:** Complete
