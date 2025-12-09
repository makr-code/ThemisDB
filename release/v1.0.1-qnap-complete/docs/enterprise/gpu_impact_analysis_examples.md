# GPU Impact Analysis Plugin - Example Configurations

## Example 1: E-Commerce Product Price Change

This configuration is optimized for analyzing the impact of price changes on orders, customers, and inventory.

```yaml
plugin:
  id: "themis.enterprise.gpu_impact_analysis"
  auto_load: true

gpu:
  backend: "cuda"
  device_id: 0

fem:
  damping_factor: 0.80  # Higher propagation for e-commerce
  impact_threshold: 0.005
  max_iterations: 100
  use_temporal_decay: true
  temporal_half_life_hours: 48.0

monte_carlo:
  num_simulations: 50000
  uncertainty_factor: 0.25

integration:
  themisdb:
    default_graph: "product_dependencies"
    use_cache: true
```

**Use Case:**
```sql
-- Analyze price change impact on order pipeline
LET price_increase = {
  document_id: 'products/laptop-pro-2025',
  change_type: 'price_update',
  old_value: {price: 999.99},
  new_value: {price: 1199.99},
  magnitude: 0.7,
  timestamp: DATE_NOW()
}

LET impact = GPU_ANALYZE_IMPACT(price_increase, {max_depth: 6})

RETURN {
  affected_pending_orders: LENGTH(
    FOR n IN impact.affected_nodes
      FILTER n.node_type == 'order' AND n.status == 'pending'
      RETURN n
  ),
  customer_churn_risk: SUM(
    FOR n IN impact.affected_nodes
      FILTER n.node_type == 'customer'
      RETURN n.impact_score > 0.5 ? 1 : 0
  )
}
```

---

## Example 2: Knowledge Base Article Update

Configuration for technical documentation where changes cascade through dependent articles.

```yaml
plugin:
  id: "themis.enterprise.gpu_impact_analysis"

fem:
  damping_factor: 0.90  # Strong propagation for docs
  impact_threshold: 0.01
  max_iterations: 150
  use_temporal_decay: false  # Docs don't decay over time

pattern_detection:
  fft:
    enabled: true
    min_pattern_frequency: 3
  
anomaly:
  enabled: true
  algorithm: "isolation_forest"
  contamination: 0.02

integration:
  themisdb:
    default_graph: "documentation_graph"
```

**Use Case:**
```sql
-- Find all articles that need review after API change
LET api_change = {
  document_id: 'docs/authentication-v2',
  change_type: 'breaking_change',
  affected_fields: ['authentication_method'],
  magnitude: 1.0
}

LET impact = GPU_ANALYZE_IMPACT(api_change, {
  max_depth: 20,
  impact_threshold: 0.05
})

FOR article IN impact.affected_nodes
  FILTER article.node_type == 'article'
  FILTER article.impact_score > 0.3
  SORT article.impact_score DESC
  RETURN {
    article_id: article.node_id,
    title: article.title,
    impact: article.impact_score,
    priority: article.impact_score > 0.7 ? 'CRITICAL' : 'REVIEW'
  }
```

---

## Example 3: GDPR Data Deletion Impact

Configuration for compliance scenarios where understanding deletion cascades is critical.

```yaml
plugin:
  id: "themis.enterprise.gpu_impact_analysis"

fem:
  damping_factor: 0.95  # Maximum propagation for compliance
  impact_threshold: 0.001  # Catch everything
  max_iterations: 200

logging:
  audit:
    enabled: true
    log_all_analyses: true

security:
  audit_all_operations: true
```

**Use Case:**
```sql
-- GDPR Article 17: Right to Erasure
LET user_deletion = {
  document_id: 'users/alice@example.com',
  change_type: 'gdpr_delete',
  magnitude: 1.0
}

LET impact = GPU_ANALYZE_IMPACT(user_deletion, {max_depth: 50})

RETURN {
  affected_collections: UNIQUE(
    FOR n IN impact.affected_nodes
      RETURN n.collection_name
  ),
  anonymization_plan: [
    FOR n IN impact.affected_nodes
      FILTER n.node_type IN ['order', 'review', 'comment']
      RETURN {
        id: n.node_id,
        action: 'ANONYMIZE',
        fields: ['user_email', 'user_name', 'user_address']
      }
  ],
  cascade_delete_plan: [
    FOR n IN impact.affected_nodes
      FILTER n.node_type IN ['session', 'token', 'preference']
      RETURN {
        id: n.node_id,
        action: 'DELETE'
      }
  ]
}
```

---

## Example 4: Supply Chain Disruption Analysis

Configuration for analyzing supply chain impacts with temporal forecasting.

```yaml
plugin:
  id: "themis.enterprise.gpu_impact_analysis"

fem:
  damping_factor: 0.75
  impact_threshold: 0.02
  use_temporal_decay: true
  temporal_half_life_hours: 72.0

forecasting:
  algorithm: "arima"
  confidence_level: 0.95
  use_gpu: true

monte_carlo:
  num_simulations: 100000
  uncertainty_factor: 0.4  # High uncertainty in supply chain
```

**Use Case:**
```sql
-- Analyze supplier disruption impact
LET supplier_disruption = {
  document_id: 'suppliers/chip-manufacturer-taiwan',
  change_type: 'supply_outage',
  magnitude: 0.95,
  timestamp: DATE_NOW()
}

LET impact = GPU_ANALYZE_IMPACT(supplier_disruption, {max_depth: 10})

-- Forecast impact over next 30 days
LET temporal = GPU_TEMPORAL_IMPACT(
  [supplier_disruption],
  (FOR n IN impact.affected_nodes RETURN n.node_id),
  P30D
)

LET forecast = GPU_FORECAST_IMPACT(temporal, 720)  // 30 days * 24 hours

-- Risk assessment
LET risk = GPU_MONTE_CARLO_RISK(supplier_disruption, {
  num_simulations: 100000,
  uncertainty_factor: 0.4
})

RETURN {
  immediate_impact: {
    affected_products: LENGTH(
      FOR n IN impact.affected_nodes
        FILTER n.node_type == 'product'
        RETURN n
    ),
    max_impact: impact.max_impact_score
  },
  forecasted_impact: forecast,
  risk_assessment: {
    expected_impact: risk.expected_impact,
    worst_case_95: risk.value_at_risk_95,
    worst_case_99: risk.value_at_risk_99
  }
}
```

---

## Example 5: Real-Time Anomaly Detection

Configuration for continuous monitoring of unusual impact patterns.

```yaml
plugin:
  id: "themis.enterprise.gpu_impact_analysis"

pattern_detection:
  anomaly:
    enabled: true
    algorithm: "isolation_forest"
    contamination: 0.005
    confidence_threshold: 0.99

performance:
  gpu_batch_size: 2000
  collect_metrics: true

integration:
  external:
    notifications:
      enabled: true
      webhook_url: "https://alerts.example.com/webhook"
      high_impact_threshold: 0.85
```

**Use Case:**
```sql
-- Real-time anomaly detection on impact stream
FOR event IN impact_analysis_stream
  WINDOW TUMBLING(PT1H)
  COLLECT batch = BATCH(event, 1000)
  
  LET anomalies = GPU_DETECT_ANOMALIES(batch, {
    algorithm: 'isolation_forest',
    contamination: 0.005
  })
  
  FOR anomaly IN anomalies
    FILTER anomaly.anomaly_score > 0.9
    
    // Trigger alert
    INSERT {
      alert_type: 'HIGH_IMPACT_ANOMALY',
      anomaly: anomaly,
      timestamp: DATE_NOW(),
      severity: 'CRITICAL'
    } INTO alerts
    
    RETURN anomaly
```

---

## Example 6: What-If Scenario Planning

Configuration optimized for strategic planning and scenario comparison.

```yaml
plugin:
  id: "themis.enterprise.gpu_impact_analysis"

fem:
  damping_factor: 0.85
  impact_threshold: 0.01

monte_carlo:
  num_simulations: 200000  # High accuracy for strategic decisions
  use_gpu: true

performance:
  num_threads: 16
  gpu_batch_size: 5000
```

**Use Case:**
```sql
-- Compare 3 business scenarios
LET scenarios = [
  {
    name: "aggressive_expansion",
    changes: [
      {document_id: 'market/us', change_type: 'expand', magnitude: 0.9},
      {document_id: 'market/eu', change_type: 'expand', magnitude: 0.8},
      {document_id: 'budget/marketing', change_type: 'increase', magnitude: 0.7}
    ]
  },
  {
    name: "conservative_growth",
    changes: [
      {document_id: 'market/us', change_type: 'expand', magnitude: 0.5},
      {document_id: 'budget/marketing', change_type: 'increase', magnitude: 0.3}
    ]
  },
  {
    name: "consolidation",
    changes: [
      {document_id: 'market/latam', change_type: 'reduce', magnitude: 0.6},
      {document_id: 'budget/marketing', change_type: 'decrease', magnitude: 0.4}
    ]
  }
]

LET comparison = GPU_COMPARE_SCENARIOS(scenarios)

// Sensitivity analysis for recommended scenario
LET recommended_scenario = (
  FOR s IN scenarios
    FILTER s.name == comparison.recommended_scenario
    RETURN s
)[0]

LET sensitivity = GPU_SENSITIVITY_ANALYSIS(
  recommended_scenario.changes[0],
  ['magnitude', 'damping_factor'],
  0.15
)

RETURN {
  comparison: comparison.comparison_matrix,
  recommended: comparison.recommended_scenario,
  reason: comparison.recommendation_reason,
  sensitivity: sensitivity
}
```

---

## Performance Presets

### Fast (Development/Testing)
```yaml
presets:
  fast:
    fem: {max_iterations: 50, convergence_threshold: 0.01}
    monte_carlo: {num_simulations: 1000}
    gpu: {cpu_fallback: false}
```

### Balanced (Production)
```yaml
presets:
  balanced:
    fem: {max_iterations: 100, convergence_threshold: 0.001}
    monte_carlo: {num_simulations: 10000}
    gpu: {cpu_fallback: true}
```

### Precise (Strategic Analysis)
```yaml
presets:
  precise:
    fem: {max_iterations: 200, convergence_threshold: 0.0001}
    monte_carlo: {num_simulations: 100000}
    forecasting: {confidence_level: 0.99}
```

---

## Tips

1. **Graph Selection:** Choose the right graph for your analysis
   - `product_dependencies` - E-commerce
   - `documentation_graph` - Knowledge base
   - `supply_chain_graph` - Logistics
   - `compliance_graph` - GDPR/regulatory

2. **Damping Factor Tuning:**
   - 0.7-0.8: Localized impact (e-commerce)
   - 0.85-0.90: Medium propagation (general use)
   - 0.90-0.95: Strong propagation (documentation, compliance)

3. **GPU Selection:**
   - CUDA: Best performance on NVIDIA GPUs
   - Vulkan: Cross-platform, good compatibility
   - CPU Fallback: Always enable for production

4. **Batch Size:**
   - Small graphs (<10K nodes): 500-1000
   - Medium graphs (10K-100K): 1000-2000
   - Large graphs (>100K): 2000-5000

---

**Last Updated:** December 7, 2025  
**Version:** 1.0.0
