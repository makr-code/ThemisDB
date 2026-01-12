# LoRA Training YAML Configuration Guide

## Übersicht

Das LoRA-Training-System in ThemisDB wird vollständig über YAML-Konfigurationsdateien gesteuert. Dies ermöglicht eine flexible und wartbare Konfiguration des Trainings, insbesondere für die Gewichtung von gecachten vs. direkten Responses.

## Hauptkonfigurationsdatei

**Speicherort:** `config/lora_training_config.yaml`

## Struktur

### 1. Globale LoRA-Einstellungen

```yaml
lora:
  enabled: true
  storage:
    backend: themisdb
    collection_name: lora_adapters
    enable_versioning: true
  security:
    enable_encryption: true
```

### 2. Adapter-Konfiguration

Jeder LoRA-Adapter hat seine eigene Konfiguration:

```yaml
adapters:
  themis_help_lora:
    enabled: true
    base_model:
      name: llama-2-7b
      path: models/llama-2-7b-chat.gguf
    hyperparameters:
      rank: 8
      alpha: 16.0
      learning_rate: 0.0003
```

### 3. Cache-Gewichtungssystem ⭐

Dies ist der zentrale Teil für die Bewertung von gecachten Responses:

```yaml
training_data:
  feedback:
    enabled: true
    weighting:
      # Direkte (nicht-gecachte) LLM-Antworten
      direct_response_weight: 1.0
      
      # Exakte Cache-Treffer (Similarity = 1.0)
      exact_cache_weight: 0.4
      
      # Semantische Cache-Treffer (Similarity < 1.0)
      semantic_cache_base_weight: 0.3
      similarity_weight_factor: 0.5
      
      # Cache-Training komplett deaktivieren?
      disable_cache_training: false
```

#### Gewichtungslogik

**Formel für semantische Cache-Treffer:**
```
weight = semantic_cache_base_weight + 
         (similarity - 0.9) * similarity_weight_factor * 10
```

**Beispiele:**
- Similarity 1.00 (exakt): `weight = 0.4` (exact_cache_weight)
- Similarity 0.95: `weight = 0.3 + (0.95-0.9)*0.5*10 = 0.55`
- Similarity 0.92: `weight = 0.3 + (0.92-0.9)*0.5*10 = 0.40`
- Similarity 0.90: `weight = 0.3` (base_weight)

#### Zusätzliche Gewichtungsfaktoren

**Typ-basiert:**
```yaml
type_weights:
  positive: 1.0    # Gut bewertete Antworten
  negative: 1.2    # Korrekturen (höher gewichtet!)
  neutral: 0.8     # Mittelmäßige Antworten
```

**Rating-basiert:**
```yaml
rating_weights:
  5: 1.2  # Exzellente Antworten
  4: 1.0  # Gute Antworten
  3: 0.8  # Akzeptable Antworten
  2: 0.6  # Schwache Antworten
  1: 0.4  # Sehr schwache Antworten
```

**Finale Gewichtung:**
```
final_weight = cache_weight * type_weight * rating_weight
```

### 4. Training Triggers

Konfiguration, wann Training ausgelöst wird:

```yaml
triggers:
  automatic:
    enabled: true
    batch_size:
      min: 50
      max: 200
      use_effective_size: true  # Verwendet Summe der Gewichte
    time:
      max_wait_hours: 24
      cron_schedule: "0 2 * * *"  # Täglich um 2 Uhr
    quality:
      min_avg_rating: 3.5
      min_positive_ratio: 0.6
```

#### Effektive Batch-Größe

Wenn `use_effective_size: true`:
```
effective_size = Σ(training_weight für alle Feedback-Einträge)
```

**Beispiel:**
- 50 direkte Responses (weight=1.0) = 50 effektiv
- 50 gecachte Responses (weight=0.4) = 20 effektiv
- **Total: 70 effektiv** statt 100 raw count

### 5. Datenbalancing

```yaml
balance:
  enable_balancing: true
  target_ratio: 0.7  # 70% positiv, 30% negativ/neutral
  balance_cache_types: true
  max_cache_ratio: 0.5  # Max 50% gecachte Responses
```

### 6. Quality Assurance

```yaml
quality:
  ab_testing:
    enabled: true
    traffic_split: 0.1  # 10% Traffic auf neue Version
    duration_hours: 24
    min_improvement: 0.05
    
  auto_rollback:
    enabled: true
    triggers:
      - accuracy_drop > 0.1
      - avg_rating < 3.0
    cooldown_hours: 6
```

## Verwendung im Code

### Konfiguration laden

```cpp
#include "llm/lora_framework/lora_training_config.h"

// Laden der Konfiguration
auto config = LoRATrainingConfig::loadFromFile(
    "config/lora_training_config.yaml"
);

// Adapter-Konfiguration abrufen
auto adapter_config = config.getAdapterConfig("themis_help_lora");
if (adapter_config) {
    std::cout << "Base model: " << adapter_config->base_model_name << std::endl;
    std::cout << "Rank: " << adapter_config->hyperparameters.rank << std::endl;
}
```

### Plugins aus Konfiguration erstellen

```cpp
// Cache-Weighting Plugin erstellen
auto cache_plugin = config.createCacheWeightingPlugin("themis_help_lora");
feedback_storage->registerPlugin(cache_plugin);

// Training Trigger Plugin erstellen
auto trigger_plugin = config.createTrainingTriggerPlugin("themis_help_lora");
feedback_storage->registerPlugin(trigger_plugin);
```

### Feedback mit Cache-Info speichern

```cpp
Feedback feedback;
feedback.adapter_id = "themis_help_lora";
feedback.user_id = "user123";
feedback.rating = 5;
feedback.prompt = "What is ThemisDB?";
feedback.response = "ThemisDB is a multi-model database...";

// Cache-Information
feedback.is_cached_response = true;
feedback.cache_key = "prompt_hash_12345";
feedback.cache_similarity_score = 0.95f;  // 95% Ähnlichkeit

// Training-Weight wird automatisch durch Plugin berechnet
auto stored = feedback_storage->createFeedback(feedback);

// Weight wurde auf ~0.55 gesetzt (semantic cache mit 95% similarity)
std::cout << "Training weight: " << stored->training_weight << std::endl;
```

### Training mit effektiver Batch-Größe

```cpp
// Effektive Batch-Größe berechnen
float effective_size = feedback_storage->calculateEffectiveBatchSize(
    "themis_help_lora"
);

std::cout << "Effective batch size: " << effective_size << std::endl;

if (effective_size >= 50.0f) {
    // Training auslösen
    lora->trainFromFeedback();
}
```

## Best Practices

### 1. Cache-Gewichtung anpassen

**Hohe Cache-Nutzung (70%+ gecacht):**
```yaml
exact_cache_weight: 0.3      # Niedriger
semantic_cache_base_weight: 0.2
disable_cache_training: false
```

**Niedrige Cache-Nutzung (30%- gecacht):**
```yaml
exact_cache_weight: 0.5      # Höher
semantic_cache_base_weight: 0.4
```

### 2. Verschiedene Adapter-Profile

**Dokumentations-Assistent:**
- Cache-Treffer sind wertvoll (etablierte Antworten)
- `exact_cache_weight: 0.4`

**Code-Assistent:**
- Cache-Treffer weniger wertvoll (Code ist kontextabhängig)
- `exact_cache_weight: 0.2`

**Chat-Bot:**
- Ausgeglichene Gewichtung
- `exact_cache_weight: 0.35`

### 3. Monitoring anpassen

```yaml
monitoring:
  metrics:
    - effective_batch_size
    - cache_weight_distribution
    - training_samples_by_cache_type
    
  alerts:
    max_cache_percentage: 80  # Warnung bei zu viel Cache
```

### 4. Schrittweise Anpassung

```yaml
# Woche 1: Konservativ starten
exact_cache_weight: 0.5

# Woche 2: Reduzieren
exact_cache_weight: 0.4

# Woche 3: Weiter optimieren
exact_cache_weight: 0.3
```

## Validierung

### Konfiguration validieren

```cpp
auto config = LoRATrainingConfig::loadFromFile("config.yaml");

if (!config.validate()) {
    auto errors = config.getValidationErrors();
    for (const auto& error : errors) {
        std::cerr << "Error: " << error << std::endl;
    }
}
```

### Häufige Validierungsfehler

- ❌ `direct_response_weight` außerhalb [0.0, 1.0]
- ❌ `exact_cache_weight` > `direct_response_weight`
- ❌ Fehlende `base_model_path`
- ❌ Negativer `rank` oder `learning_rate`

## Beispiel-Szenarien

### Szenario 1: Hohe Cache-Nutzung

**Situation:** 80% der Antworten kommen aus dem Cache

**Lösung:**
```yaml
cache_weighting:
  exact_cache_weight: 0.3
  disable_cache_training: false
balance:
  max_cache_ratio: 0.6  # Erlaube mehr Cache im Training
```

### Szenario 2: Schlechte gecachte Antworten

**Situation:** Gecachte Antworten haben niedrige Ratings

**Lösung:**
```yaml
cache_weighting:
  exact_cache_weight: 0.2  # Sehr niedrig
  # Oder komplett deaktivieren:
  disable_cache_training: true
```

### Szenario 3: Unterschiedliche Qualität

**Situation:** Manche gecachte Antworten gut, andere schlecht

**Lösung:**
```yaml
cache_weighting:
  # Nutze Rating-Gewichte zur Differenzierung
rating_weights:
  5: 1.5  # Cache + hoher Rating = gutes Signal
  4: 1.0
  3: 0.5  # Cache + mittlerer Rating = weniger Gewicht
  2: 0.2
  1: 0.0  # Cache + niedriger Rating = ignorieren
```

## Migration

### Von Code-Konfiguration zu YAML

**Vorher (Code):**
```cpp
TrainingTriggerPlugin::Config config;
config.min_batch_size = 50;
config.max_batch_size = 200;
auto plugin = std::make_shared<TrainingTriggerPlugin>(config);
```

**Nachher (YAML):**
```yaml
# config/lora_training_config.yaml
triggers:
  automatic:
    batch_size:
      min: 50
      max: 200
```

```cpp
// Code
auto config = LoRATrainingConfig::loadFromFile("config.yaml");
auto plugin = config.createTrainingTriggerPlugin("themis_help_lora");
```

## Troubleshooting

### Problem: Training wird nicht ausgelöst

**Prüfen:**
```yaml
triggers:
  automatic:
    enabled: true  # ✓
    batch_size:
      use_effective_size: true  # Effective size berechnen
```

**Debug:**
```cpp
float effective = feedback_storage->calculateEffectiveBatchSize("adapter_id");
std::cout << "Effective: " << effective << std::endl;
// Wenn zu niedrig: Cache-Weights erhöhen oder mehr Feedback sammeln
```

### Problem: Training überwiegend auf gecachten Daten

**Lösung:**
```yaml
balance:
  balance_cache_types: true
  max_cache_ratio: 0.4  # Max 40% Cache
```

### Problem: Konfiguration wird nicht geladen

**Prüfen:**
```bash
# YAML-Syntax validieren
yamllint config/lora_training_config.yaml

# Pfad prüfen
ls -la config/lora_training_config.yaml
```

## Referenzen

- [LoRA Framework Documentation](./LORA_FRAMEWORK_GUIDE.md)
- [Feedback API Documentation](./LORA_FEEDBACK_API.md)
- [Plugin Developer Guide](./PLUGIN_DEVELOPER_GUIDE.md)
- [ThemisHelp Integration](./THEMIS_HELP_LORA_INTEGRATION.md)
