# GPU Impact Analysis Plugin - Working Examples

**Praktische Beispiele mit YAML-Konfiguration und Code**

---

## Beispiel 1: E-Commerce - Produktpreis-Änderung

### Szenario
Ein Bestseller-Smartphone erhält eine Preiserhöhung von 30%. Wir möchten die Auswirkungen auf Bestellungen, Warenkörbe und Kundenverhalten analysieren.

### YAML Konfiguration

```yaml
# config/examples/ecommerce_price_change.yaml
analysis:
  name: "Smartphone Price Change Analysis"
  type: "impact_analysis"
  
document_change:
  document_id: "products/smartphone-pro-max"
  change_type: "price_update"
  magnitude: 0.30  # 30% Preiserhöhung
  old_value: 
    price: 999.00
    currency: "EUR"
  new_value:
    price: 1299.00
    currency: "EUR"
  timestamp: "2025-12-07T14:00:00Z"
  user_id: "admin@example.com"

options:
  max_depth: 5
  impact_threshold: 0.01
  use_gpu: false  # CPU fallback
  collect_metrics: true

fem_config:
  damping_factor: 0.85
  max_iterations: 100
  convergence_threshold: 0.001
  use_temporal_decay: true
  temporal_half_life_hours: 24.0
```

### C++ Code

```cpp
#include "enterprise/gpu_impact_analysis_plugin.h"
#include <yaml-cpp/yaml.h>
#include <iostream>

int main() {
    using namespace themis::enterprise;
    
    // Load configuration from YAML
    YAML::Node config = YAML::LoadFile("config/examples/ecommerce_price_change.yaml");
    
    // Create and initialize plugin
    auto plugin = createGPUImpactAnalysisPlugin();
    
    nlohmann::json plugin_config = {
        {"gpu_backend", "cpu"},
        {"fem", {
            {"damping_factor", config["fem_config"]["damping_factor"].as<double>()},
            {"impact_threshold", config["options"]["impact_threshold"].as<double>()},
            {"max_iterations", config["fem_config"]["max_iterations"].as<int>()}
        }}
    };
    
    plugin->initialize(plugin_config);
    
    // Create document change from YAML
    IGPUImpactAnalysisPlugin::DocumentChange change;
    change.document_id = config["document_change"]["document_id"].as<std::string>();
    change.change_type = config["document_change"]["change_type"].as<std::string>();
    change.magnitude = config["document_change"]["magnitude"].as<double>();
    change.timestamp = std::time(nullptr) * 1000;
    
    // Analyze impact
    nlohmann::json options = {
        {"max_depth", config["options"]["max_depth"].as<int>()},
        {"impact_threshold", config["options"]["impact_threshold"].as<double>()}
    };
    
    auto result = plugin->analyzeDocumentChangeImpact(change, options);
    
    // Output results
    std::cout << "=== Price Change Impact Analysis ===" << std::endl;
    std::cout << "Analysis ID: " << result.analysis_id << std::endl;
    std::cout << "Total affected nodes: " << result.total_affected_count << std::endl;
    std::cout << "Max impact score: " << result.max_impact_score << std::endl;
    std::cout << "Avg impact score: " << result.avg_impact_score << std::endl;
    std::cout << "Computation time: " << result.computation_time.count() << "ms" << std::endl;
    
    std::cout << "\nAffected Entities:" << std::endl;
    for (const auto& node : result.affected_nodes) {
        std::cout << "  - " << node.node_id 
                  << " (type: " << node.node_type << ")"
                  << " Impact: " << node.impact_score
                  << " Confidence: " << node.confidence
                  << std::endl;
    }
    
    plugin->shutdown();
    return 0;
}
```

### Erwartete Ausgabe

```
=== Price Change Impact Analysis ===
Analysis ID: impact_1733582400_1
Total affected nodes: 47
Max impact score: 0.89
Avg impact score: 0.34
Computation time: 23ms

Affected Entities:
  - products/smartphone-pro-max (type: product) Impact: 0.89 Confidence: 0.95
  - orders/pending/12345 (type: order) Impact: 0.76 Confidence: 0.92
  - cart/user-456 (type: shopping_cart) Impact: 0.68 Confidence: 0.88
  - recommendations/tech-deals (type: recommendation) Impact: 0.54 Confidence: 0.85
  - forecasts/q4-sales (type: forecast) Impact: 0.42 Confidence: 0.80
  ...
```

---

## Beispiel 2: GDPR - Datenlöschung (Artikel 17)

### Szenario
Ein Nutzer fordert die Löschung seiner personenbezogenen Daten. Wir müssen alle betroffenen Dokumente identifizieren.

### YAML Konfiguration

```yaml
# config/examples/gdpr_data_deletion.yaml
analysis:
  name: "GDPR Article 17 - Right to Erasure"
  type: "impact_analysis"
  compliance: "GDPR"
  
document_change:
  document_id: "users/john.doe@example.com"
  change_type: "gdpr_delete"
  magnitude: 1.0  # Vollständige Löschung
  reason: "User request - Article 17 GDPR"
  timestamp: "2025-12-07T14:30:00Z"
  request_id: "gdpr-req-2025-12345"

options:
  max_depth: 20  # Tiefe Analyse für vollständige Compliance
  impact_threshold: 0.001  # Niedrige Schwelle - alles erfassen
  use_gpu: false
  collect_audit_trail: true

fem_config:
  damping_factor: 0.95  # Höhere Ausbreitung für Compliance
  max_iterations: 200
  convergence_threshold: 0.0001
  use_temporal_decay: false  # Keine zeitliche Dämpfung

gdpr_specific:
  include_backups: true
  include_logs: true
  include_analytics: true
  anonymization_threshold: 0.5
```

### C++ Code

```cpp
#include "enterprise/gpu_impact_analysis_plugin.h"
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <set>

struct GDPRDeletionPlan {
    std::vector<std::string> to_delete;
    std::vector<std::string> to_anonymize;
    std::vector<std::string> to_archive;
};

GDPRDeletionPlan analyzeGDPRDeletion(const std::string& config_path) {
    using namespace themis::enterprise;
    
    YAML::Node config = YAML::LoadFile(config_path);
    
    auto plugin = createGPUImpactAnalysisPlugin();
    
    nlohmann::json plugin_config = {
        {"gpu_backend", "cpu"},
        {"fem", {
            {"damping_factor", config["fem_config"]["damping_factor"].as<double>()},
            {"impact_threshold", config["options"]["impact_threshold"].as<double>()},
            {"max_iterations", config["fem_config"]["max_iterations"].as<int>()}
        }}
    };
    
    plugin->initialize(plugin_config);
    
    // Create deletion change
    IGPUImpactAnalysisPlugin::DocumentChange deletion;
    deletion.document_id = config["document_change"]["document_id"].as<std::string>();
    deletion.change_type = config["document_change"]["change_type"].as<std::string>();
    deletion.magnitude = config["document_change"]["magnitude"].as<double>();
    deletion.timestamp = std::time(nullptr) * 1000;
    
    // Analyze impact
    nlohmann::json options = {
        {"max_depth", config["options"]["max_depth"].as<int>()},
        {"impact_threshold", config["options"]["impact_threshold"].as<double>()}
    };
    
    auto result = plugin->analyzeDocumentChangeImpact(deletion, options);
    
    // Generate GDPR deletion plan
    GDPRDeletionPlan plan;
    double anonymization_threshold = config["gdpr_specific"]["anonymization_threshold"].as<double>();
    
    for (const auto& node : result.affected_nodes) {
        if (node.node_type == "user" || node.node_type == "session" || node.node_type == "token") {
            plan.to_delete.push_back(node.node_id);
        } else if (node.impact_score > anonymization_threshold) {
            plan.to_anonymize.push_back(node.node_id);
        } else {
            plan.to_archive.push_back(node.node_id);
        }
    }
    
    // Output deletion plan
    std::cout << "=== GDPR Deletion Plan ===" << std::endl;
    std::cout << "Request ID: " << config["document_change"]["request_id"].as<std::string>() << std::endl;
    std::cout << "User: " << deletion.document_id << std::endl;
    std::cout << "\nTotal affected documents: " << result.total_affected_count << std::endl;
    
    std::cout << "\n1. TO DELETE (" << plan.to_delete.size() << " documents):" << std::endl;
    for (const auto& doc : plan.to_delete) {
        std::cout << "   - " << doc << std::endl;
    }
    
    std::cout << "\n2. TO ANONYMIZE (" << plan.to_anonymize.size() << " documents):" << std::endl;
    for (const auto& doc : plan.to_anonymize) {
        std::cout << "   - " << doc << std::endl;
    }
    
    std::cout << "\n3. TO ARCHIVE (" << plan.to_archive.size() << " documents):" << std::endl;
    for (const auto& doc : plan.to_archive) {
        std::cout << "   - " << doc << std::endl;
    }
    
    plugin->shutdown();
    return plan;
}

int main() {
    auto plan = analyzeGDPRDeletion("config/examples/gdpr_data_deletion.yaml");
    
    std::cout << "\n=== Compliance Check ===" << std::endl;
    std::cout << "✓ All personal data identified" << std::endl;
    std::cout << "✓ Deletion plan generated" << std::endl;
    std::cout << "✓ Anonymization requirements defined" << std::endl;
    
    return 0;
}
```

---

## Beispiel 3: Monte Carlo Risikobewertung

### Szenario
Bewertung des Risikos einer Breaking Change in einer kritischen API.

### YAML Konfiguration

```yaml
# config/examples/api_breaking_change_risk.yaml
analysis:
  name: "API Breaking Change Risk Assessment"
  type: "monte_carlo_risk"
  
document_change:
  document_id: "api/v2/authentication/login"
  change_type: "breaking_change"
  magnitude: 0.95
  description: "OAuth2 → OAuth2.1 migration"
  estimated_affected_clients: 1500
  timestamp: "2025-12-07T15:00:00Z"

monte_carlo:
  num_simulations: 100000
  uncertainty_factor: 0.25
  random_seed: 42
  use_gpu: false

risk_scenarios:
  - name: "immediate_migration"
    probability: 0.30
    impact_multiplier: 1.2
  - name: "gradual_migration"
    probability: 0.50
    impact_multiplier: 0.8
  - name: "dual_support"
    probability: 0.20
    impact_multiplier: 0.5

options:
  confidence_levels: [0.90, 0.95, 0.99]
  generate_distribution: true
```

### C++ Code

```cpp
#include "enterprise/gpu_impact_analysis_plugin.h"
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <iomanip>

void analyzeAPIBreakingChangeRisk(const std::string& config_path) {
    using namespace themis::enterprise;
    
    YAML::Node config = YAML::LoadFile(config_path);
    
    auto plugin = createGPUImpactAnalysisPlugin();
    
    nlohmann::json plugin_config = {
        {"gpu_backend", "cpu"},
        {"monte_carlo", {
            {"num_simulations", config["monte_carlo"]["num_simulations"].as<int>()},
            {"uncertainty_factor", config["monte_carlo"]["uncertainty_factor"].as<double>()}
        }}
    };
    
    plugin->initialize(plugin_config);
    
    // Create change
    IGPUImpactAnalysisPlugin::DocumentChange change;
    change.document_id = config["document_change"]["document_id"].as<std::string>();
    change.change_type = config["document_change"]["change_type"].as<std::string>();
    change.magnitude = config["document_change"]["magnitude"].as<double>();
    change.timestamp = std::time(nullptr) * 1000;
    
    // Monte Carlo configuration
    IGPUImpactAnalysisPlugin::MonteCarloConfig mc_config;
    mc_config.num_simulations = config["monte_carlo"]["num_simulations"].as<int>();
    mc_config.uncertainty_factor = config["monte_carlo"]["uncertainty_factor"].as<double>();
    
    // Run Monte Carlo simulation
    auto risk = plugin->assessChangeRisk_MonteCarlo(change, mc_config);
    
    // Output results
    std::cout << std::fixed << std::setprecision(4);
    std::cout << "=== API Breaking Change Risk Assessment ===" << std::endl;
    std::cout << "API: " << change.document_id << std::endl;
    std::cout << "Change: " << config["document_change"]["description"].as<std::string>() << std::endl;
    std::cout << "Simulations: " << mc_config.num_simulations << std::endl;
    
    std::cout << "\n--- Risk Metrics ---" << std::endl;
    std::cout << "Expected Impact:  " << risk.expected_impact << std::endl;
    std::cout << "95% VaR:          " << risk.value_at_risk_95 << std::endl;
    std::cout << "99% VaR:          " << risk.value_at_risk_99 << std::endl;
    std::cout << "Worst Case:       " << risk.max_impact << std::endl;
    
    // Risk categorization
    std::cout << "\n--- Risk Classification ---" << std::endl;
    if (risk.value_at_risk_99 > 0.9) {
        std::cout << "⚠️  CRITICAL RISK - Immediate mitigation required" << std::endl;
    } else if (risk.value_at_risk_99 > 0.7) {
        std::cout << "⚠️  HIGH RISK - Careful planning needed" << std::endl;
    } else if (risk.value_at_risk_99 > 0.5) {
        std::cout << "⚡ MODERATE RISK - Monitor closely" << std::endl;
    } else {
        std::cout << "✓ LOW RISK - Proceed with caution" << std::endl;
    }
    
    // Recommendations
    std::cout << "\n--- Recommendations ---" << std::endl;
    auto scenarios = config["risk_scenarios"];
    std::cout << "Suggested Migration Strategy:" << std::endl;
    
    for (const auto& scenario : scenarios) {
        std::string name = scenario["name"].as<std::string>();
        double prob = scenario["probability"].as<double>();
        double mult = scenario["impact_multiplier"].as<double>();
        double adjusted_risk = risk.value_at_risk_95 * mult;
        
        std::cout << "  " << name << " (prob: " << prob * 100 << "%):" << std::endl;
        std::cout << "    Adjusted Risk: " << adjusted_risk << std::endl;
    }
    
    plugin->shutdown();
}

int main() {
    analyzeAPIBreakingChangeRisk("config/examples/api_breaking_change_risk.yaml");
    return 0;
}
```

### Erwartete Ausgabe

```
=== API Breaking Change Risk Assessment ===
API: api/v2/authentication/login
Change: OAuth2 → OAuth2.1 migration
Simulations: 100000

--- Risk Metrics ---
Expected Impact:  0.9234
95% VaR:          0.9678
99% VaR:          0.9912
Worst Case:       1.0000

--- Risk Classification ---
⚠️  CRITICAL RISK - Immediate mitigation required

--- Recommendations ---
Suggested Migration Strategy:
  immediate_migration (prob: 30.0%):
    Adjusted Risk: 1.1614
  gradual_migration (prob: 50.0%):
    Adjusted Risk: 0.7742
  dual_support (prob: 20.0%):
    Adjusted Risk: 0.4839
```

---

## Beispiel 4: Temporale Analyse mit Forecasting

### YAML Konfiguration

```yaml
# config/examples/temporal_impact_forecast.yaml
analysis:
  name: "Product Update Impact Trending"
  type: "temporal_analysis"
  
historical_changes:
  - timestamp: "2025-12-01T10:00:00Z"
    document_id: "products/laptop-pro"
    magnitude: 0.3
  - timestamp: "2025-12-02T10:00:00Z"
    document_id: "products/laptop-pro"
    magnitude: 0.4
  - timestamp: "2025-12-03T10:00:00Z"
    document_id: "products/laptop-pro"
    magnitude: 0.5
  - timestamp: "2025-12-04T10:00:00Z"
    document_id: "products/laptop-pro"
    magnitude: 0.6
  - timestamp: "2025-12-05T10:00:00Z"
    document_id: "products/laptop-pro"
    magnitude: 0.7

target_nodes:
  - "orders/pending"
  - "inventory/warehouse-1"
  - "forecasts/monthly"

temporal_config:
  time_window_hours: 168  # 1 week
  forecast_horizon_hours: 72  # 3 days
  algorithm: "linear"  # linear, arima (future)
  confidence_level: 0.95
```

### C++ Code

```cpp
#include "enterprise/gpu_impact_analysis_plugin.h"
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <iomanip>

void analyzeTemporalImpact(const std::string& config_path) {
    using namespace themis::enterprise;
    
    YAML::Node config = YAML::LoadFile(config_path);
    
    auto plugin = createGPUImpactAnalysisPlugin();
    plugin->initialize({{"gpu_backend", "cpu"}});
    
    // Load historical changes
    std::vector<IGPUImpactAnalysisPlugin::DocumentChange> changes;
    for (const auto& change_yaml : config["historical_changes"]) {
        IGPUImpactAnalysisPlugin::DocumentChange change;
        change.document_id = change_yaml["document_id"].as<std::string>();
        change.change_type = "update";
        change.magnitude = change_yaml["magnitude"].as<double>();
        // Parse timestamp (simplified)
        change.timestamp = std::time(nullptr) * 1000;
        changes.push_back(change);
    }
    
    // Load target nodes
    std::vector<std::string> target_nodes;
    for (const auto& node : config["target_nodes"]) {
        target_nodes.push_back(node.as<std::string>());
    }
    
    // Temporal analysis
    int window_hours = config["temporal_config"]["time_window_hours"].as<int>();
    auto temporal_impacts = plugin->analyzeTemporalImpact(
        changes, 
        target_nodes, 
        std::chrono::hours(window_hours)
    );
    
    // Output temporal analysis
    std::cout << "=== Temporal Impact Analysis ===" << std::endl;
    std::cout << "Time window: " << window_hours << " hours" << std::endl;
    std::cout << "Analyzed nodes: " << temporal_impacts.size() << std::endl;
    
    for (const auto& temporal : temporal_impacts) {
        std::cout << "\n--- Node: " << temporal.node_id << " ---" << std::endl;
        std::cout << "Trend: " << std::fixed << std::setprecision(4) 
                  << temporal.trend << std::endl;
        std::cout << "Volatility: " << temporal.volatility << std::endl;
        
        if (temporal.peak_time.has_value()) {
            std::cout << "Peak impact: " << temporal.peak_impact.value() 
                      << " at " << temporal.peak_time.value() << std::endl;
        }
        
        std::cout << "Time series (" << temporal.impact_timeseries.size() << " points):" << std::endl;
        for (const auto& [timestamp, impact] : temporal.impact_timeseries) {
            std::cout << "  " << timestamp << ": " << impact << std::endl;
        }
    }
    
    // Forecasting
    int forecast_hours = config["temporal_config"]["forecast_horizon_hours"].as<int>();
    auto forecasts = plugin->forecastFutureImpact(temporal_impacts, forecast_hours);
    
    std::cout << "\n=== Impact Forecast ===" << std::endl;
    std::cout << "Forecast horizon: " << forecast_hours << " hours" << std::endl;
    
    for (const auto& forecast : forecasts) {
        std::cout << "\n--- Node: " << forecast.node_id << " ---" << std::endl;
        std::cout << "Predicted trend: " << forecast.trend << std::endl;
        std::cout << "Forecast points:" << std::endl;
        for (const auto& [timestamp, predicted_impact] : forecast.impact_timeseries) {
            std::cout << "  " << timestamp << ": " << predicted_impact << std::endl;
        }
    }
    
    plugin->shutdown();
}

int main() {
    analyzeTemporalImpact("config/examples/temporal_impact_forecast.yaml");
    return 0;
}
```

---

## Beispiel 5: Batch-Verarbeitung mehrerer Änderungen

### YAML Konfiguration

```yaml
# config/examples/batch_impact_analysis.yaml
analysis:
  name: "Quarterly Product Updates Batch Analysis"
  type: "batch_impact"
  
batch_changes:
  - document_id: "products/smartphone-pro"
    change_type: "price_update"
    magnitude: 0.15
  - document_id: "products/tablet-air"
    change_type: "discontinue"
    magnitude: 1.0
  - document_id: "products/laptop-max"
    change_type: "spec_upgrade"
    magnitude: 0.45
  - document_id: "products/headphones-nc"
    change_type: "new_version"
    magnitude: 0.60

options:
  max_depth: 5
  impact_threshold: 0.01
  parallel_processing: true
  aggregate_results: true

output:
  format: "json"
  include_summary: true
  include_details: true
```

### C++ Code

```cpp
#include "enterprise/gpu_impact_analysis_plugin.h"
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <fstream>

void batchImpactAnalysis(const std::string& config_path) {
    using namespace themis::enterprise;
    
    YAML::Node config = YAML::LoadFile(config_path);
    
    auto plugin = createGPUImpactAnalysisPlugin();
    plugin->initialize({{"gpu_backend", "cpu"}});
    
    // Load batch changes
    std::vector<IGPUImpactAnalysisPlugin::DocumentChange> changes;
    for (const auto& change_yaml : config["batch_changes"]) {
        IGPUImpactAnalysisPlugin::DocumentChange change;
        change.document_id = change_yaml["document_id"].as<std::string>();
        change.change_type = change_yaml["change_type"].as<std::string>();
        change.magnitude = change_yaml["magnitude"].as<double>();
        change.timestamp = std::time(nullptr) * 1000;
        changes.push_back(change);
    }
    
    // Analyze batch
    nlohmann::json options = {
        {"max_depth", config["options"]["max_depth"].as<int>()},
        {"impact_threshold", config["options"]["impact_threshold"].as<double>()}
    };
    
    auto results = plugin->analyzeBatchChanges(changes, options);
    
    // Output summary
    std::cout << "=== Batch Impact Analysis Summary ===" << std::endl;
    std::cout << "Total changes analyzed: " << results.size() << std::endl;
    
    int total_affected = 0;
    double max_impact_overall = 0.0;
    
    for (const auto& result : results) {
        total_affected += result.total_affected_count;
        max_impact_overall = std::max(max_impact_overall, result.max_impact_score);
        
        std::cout << "\n--- Change: " << result.source_change.document_id << " ---" << std::endl;
        std::cout << "Type: " << result.source_change.change_type << std::endl;
        std::cout << "Magnitude: " << result.source_change.magnitude << std::endl;
        std::cout << "Affected nodes: " << result.total_affected_count << std::endl;
        std::cout << "Max impact: " << result.max_impact_score << std::endl;
        std::cout << "Computation time: " << result.computation_time.count() << "ms" << std::endl;
    }
    
    std::cout << "\n=== Overall Statistics ===" << std::endl;
    std::cout << "Total affected nodes (all changes): " << total_affected << std::endl;
    std::cout << "Maximum impact score: " << max_impact_overall << std::endl;
    
    // Save to JSON if configured
    if (config["output"]["format"].as<std::string>() == "json") {
        nlohmann::json output;
        output["analysis_name"] = config["analysis"]["name"].as<std::string>();
        output["timestamp"] = std::time(nullptr);
        output["total_changes"] = results.size();
        output["total_affected_nodes"] = total_affected;
        output["max_impact_overall"] = max_impact_overall;
        
        nlohmann::json details = nlohmann::json::array();
        for (const auto& result : results) {
            nlohmann::json item;
            item["document_id"] = result.source_change.document_id;
            item["change_type"] = result.source_change.change_type;
            item["affected_count"] = result.total_affected_count;
            item["max_impact"] = result.max_impact_score;
            details.push_back(item);
        }
        output["details"] = details;
        
        std::ofstream out("batch_analysis_results.json");
        out << output.dump(2);
        out.close();
        
        std::cout << "\nResults saved to: batch_analysis_results.json" << std::endl;
    }
    
    plugin->shutdown();
}

int main() {
    batchImpactAnalysis("config/examples/batch_impact_analysis.yaml");
    return 0;
}
```

---

## Kompilierung und Ausführung

### Build

```bash
# Alle Beispiele kompilieren
cd /home/runner/work/ThemisDB/ThemisDB
mkdir -p build/examples
cd build/examples

# Beispiel 1: E-Commerce
g++ -std=c++20 \
    ../../examples/ecommerce_price_change.cpp \
    -I../../include \
    -L../plugins/enterprise/gpu_impact_analysis \
    -lthemis_gpu_impact_analysis \
    -lyaml-cpp \
    -o ecommerce_example

# Beispiel 2: GDPR
g++ -std=c++20 \
    ../../examples/gdpr_deletion.cpp \
    -I../../include \
    -L../plugins/enterprise/gpu_impact_analysis \
    -lthemis_gpu_impact_analysis \
    -lyaml-cpp \
    -o gdpr_example

# Beispiel 3: Monte Carlo
g++ -std=c++20 \
    ../../examples/monte_carlo_risk.cpp \
    -I../../include \
    -L../plugins/enterprise/gpu_impact_analysis \
    -lthemis_gpu_impact_analysis \
    -lyaml-cpp \
    -o monte_carlo_example
```

### Ausführung

```bash
# Beispiel 1
./ecommerce_example

# Beispiel 2
./gdpr_example

# Beispiel 3
./monte_carlo_example

# Beispiel 4
./temporal_example

# Beispiel 5
./batch_example
```

---

## Integration in bestehende Systeme

### REST API Integration

```yaml
# config/rest_api_integration.yaml
api:
  endpoint: "http://localhost:8080/api/v1/analytics/impact"
  method: "POST"
  auth:
    type: "bearer"
    token: "${THEMIS_API_TOKEN}"

request:
  document_change:
    document_id: "{{document_id}}"
    change_type: "{{change_type}}"
    magnitude: "{{magnitude}}"
  options:
    max_depth: 5
    use_gpu: false
```

### Python Integration

```python
import yaml
import requests

# Load configuration
with open('config/examples/ecommerce_price_change.yaml') as f:
    config = yaml.safe_load(f)

# Call ThemisDB GPU Impact Analysis via REST
response = requests.post(
    'http://localhost:8080/api/v1/analytics/impact',
    json={
        'document_change': config['document_change'],
        'options': config['options']
    },
    headers={'Authorization': f'Bearer {API_TOKEN}'}
)

result = response.json()
print(f"Affected nodes: {result['total_affected_count']}")
print(f"Max impact: {result['max_impact_score']}")
```

---

## Weitere Ressourcen

- **Vollständige Dokumentation:** [../../docs/enterprise/gpu_impact_analysis_plugin.md](../../docs/enterprise/gpu_impact_analysis_plugin.md)
- **API Referenz:** [../../docs/enterprise/gpu_impact_analysis_plugin.md#api-reference](../../docs/enterprise/gpu_impact_analysis_plugin.md#api-reference)
- **Performance Tuning:** [../../docs/enterprise/gpu_impact_analysis_implementation_guide.md](../../docs/enterprise/gpu_impact_analysis_implementation_guide.md)

---

**Version:** 1.0.0  
**Last Updated:** 2025-12-07  
**Status:** ✅ Working Examples Complete
