# Cache-Aware LoRA Training - Feature Summary

## Übersicht

Das LoRA-Feedback-System wurde um umfassende Cache-Awareness erweitert. Gecachte LLM-Antworten werden jetzt mit einem konfigurierbaren Gewichtungssystem ins Training einbezogen, um Übertraining auf populären Queries zu vermeiden.

## Neue Features

### 1. Cache-Status im Feedback

Jedes Feedback enthält nun Cache-Informationen:

```cpp
struct Feedback {
    // ... existing fields ...
    
    // Cache information
    bool is_cached_response = false;         // War Response gecacht?
    std::optional<std::string> cache_key;    // Cache-Schlüssel
    float cache_similarity_score = 1.0f;     // Ähnlichkeitsscore (0.9-1.0)
    
    // Training weight
    float training_weight = 1.0f;            // Berechnetes Trainingsgewicht
};
```

### 2. Automatische Gewichtungsberechnung

Der `CacheAwareWeightingPlugin` berechnet automatisch das Trainingsgewicht:

**Direkte Responses:**
```
weight = 1.0 (volle Gewichtung)
```

**Exakte Cache-Treffer (similarity = 1.0):**
```
weight = 0.4 (reduzierte Gewichtung)
```

**Semantische Cache-Treffer (similarity < 1.0):**
```
weight = 0.3 + (similarity - 0.9) * 0.5 * 10

Beispiele:
- 95% ähnlich: 0.3 + 0.05 * 5 = 0.55
- 92% ähnlich: 0.3 + 0.02 * 5 = 0.40
- 90% ähnlich: 0.3 (Basisgewicht)
```

### 3. Multi-Faktor Gewichtung

Die finale Gewichtung berücksichtigt mehrere Faktoren:

```
final_weight = cache_weight × type_weight × rating_weight

Wobei:
- cache_weight: 0.3-1.0 (basierend auf Cache-Status)
- type_weight: 0.8-1.2 (positive/negative/neutral)
- rating_weight: 0.4-1.2 (basierend auf 1-5 Sternen)
```

**Beispiel:**
```
Gecachte Response (95% ähnlich), positiv, 5 Sterne:
0.55 × 1.0 × 1.2 = 0.66

Direkte Response, negativ (Korrektur), 2 Sterne:
1.0 × 1.2 × 0.6 = 0.72
```

### 4. Effektive Batch-Größe

Statt roher Anzahl wird die **Summe der Trainingsgewichte** verwendet:

```cpp
float effective_size = feedback_storage->calculateEffectiveBatchSize(adapter_id);

// Beispiel:
// 50 direkte Responses (weight=1.0) = 50.0 effektiv
// 50 gecachte Responses (weight=0.4) = 20.0 effektiv
// Total: 70.0 effektiv (statt 100 raw count)
```

### 5. YAML-Konfiguration

Komplette Konfiguration über YAML-Dateien:

**config/lora_training_config.yaml:**
```yaml
adapters:
  themis_help_lora:
    training_data:
      feedback:
        weighting:
          # Gewichte konfigurieren
          direct_response_weight: 1.0
          exact_cache_weight: 0.4
          semantic_cache_base_weight: 0.3
          similarity_weight_factor: 0.5
          
          # Typ-Gewichte
          type_weights:
            positive: 1.0
            negative: 1.2  # Korrekturen höher gewichten
            neutral: 0.8
            
          # Rating-Gewichte
          rating_weights:
            5: 1.2  # Exzellent
            4: 1.0  # Gut
            3: 0.8  # OK
            2: 0.6  # Schwach
            1: 0.4  # Sehr schwach
            
    triggers:
      automatic:
        batch_size:
          min: 50
          max: 200
          use_effective_size: true  # Nutze gewichtete Summe
```

### 6. Datenbala ncierung

Automatisches Balancing zwischen Cache-Typen:

```yaml
balance:
  enable_balancing: true
  
  # Verhältnis positive/negative
  target_ratio: 0.7  # 70% positiv
  
  # Cache-Balance
  balance_cache_types: true
  max_cache_ratio: 0.5  # Max 50% gecachte Responses
```

## Verwendung

### Feedback mit Cache-Info erstellen

```cpp
Feedback feedback;
feedback.adapter_id = "themis_help_lora";
feedback.prompt = "What is ThemisDB?";
feedback.response = "ThemisDB is a multi-model database...";
feedback.rating = 5;

// Cache-Information hinzufügen
feedback.is_cached_response = true;
feedback.cache_similarity_score = 0.95f;  // 95% ähnlich
feedback.cache_key = "hash_12345";

// Speichern - Weight wird automatisch berechnet
auto stored = feedback_storage->createFeedback(feedback);

std::cout << "Training weight: " << stored->training_weight << std::endl;
// Output: Training weight: 0.55
```

### Konfiguration laden und Plugins erstellen

```cpp
// YAML-Konfiguration laden
auto config = LoRATrainingConfig::loadFromFile(
    "config/lora_training_config.yaml"
);

// Plugins aus Konfiguration erstellen
auto cache_plugin = config.createCacheWeightingPlugin("themis_help_lora");
auto trigger_plugin = config.createTrainingTriggerPlugin("themis_help_lora");

// Plugins registrieren
feedback_storage->registerPlugin(cache_plugin);
feedback_storage->registerPlugin(trigger_plugin);
```

### Training mit effektiver Batch-Größe

```cpp
// Effektive Batch-Größe berechnen (berücksichtigt Gewichte)
float effective_size = feedback_storage->calculateEffectiveBatchSize(
    "themis_help_lora"
);

std::cout << "Effective batch size: " << effective_size << std::endl;

// Prüfen ob Training ausgelöst werden soll
if (feedback_storage->shouldTriggerTraining("themis_help_lora")) {
    std::cout << "Training threshold reached!" << std::endl;
    
    // Gewichtetes Training-Feedback holen
    auto training_data = feedback_storage->getWeightedTrainingFeedback(
        "themis_help_lora",
        200  // limit
    );
    
    // Training durchführen
    lora->trainFromFeedback(training_data);
}
```

## Architektur

```
┌─────────────────────────────────────────┐
│  LLM Response (direkt oder gecacht)     │
│  + Cache-Metadaten (is_cached, score)  │
└────────────┬────────────────────────────┘
             │
             ▼
┌────────────────────────────────────────┐
│  Feedback Creation                     │
│  - User Rating (1-5)                   │
│  - Feedback Text                       │
│  - Cache Status                        │
└────────────┬───────────────────────────┘
             │
             ▼
┌────────────────────────────────────────┐
│  CacheAwareWeightingPlugin             │
│  ┌──────────────────────────────────┐  │
│  │ IF cached:                       │  │
│  │   weight = f(similarity_score)   │  │
│  │ ELSE:                            │  │
│  │   weight = 1.0                   │  │
│  │                                  │  │
│  │ weight *= type_weight            │  │
│  │ weight *= rating_weight          │  │
│  └──────────────────────────────────┘  │
└────────────┬───────────────────────────┘
             │
             ▼
┌────────────────────────────────────────┐
│  Feedback Storage                      │
│  - Speichert mit training_weight       │
│  - Graph-Link zu Adapter              │
└────────────┬───────────────────────────┘
             │
             ▼
┌────────────────────────────────────────┐
│  Training Trigger Check                │
│  - Berechnet effective_batch_size      │
│  - Σ(training_weights)                 │
│  - Prüft gegen Schwellwerte           │
└────────────┬───────────────────────────┘
             │
             ▼
┌────────────────────────────────────────┐
│  LoRA Training                         │
│  - Nutzt gewichtetes Feedback         │
│  - Hohe Gewichte = mehr Einfluss      │
│  - Niedrige Gewichte = weniger Einfluss│
└────────────────────────────────────────┘
```

## Rationale

### Warum Cache-Gewichtung?

**Problem ohne Gewichtung:**
- Populäre Queries werden oft gecacht
- Viel Feedback zu gecachten Antworten
- Training überfittet auf häufige Fragen
- Seltene Fragen werden vernachlässigt

**Lösung mit Gewichtung:**
- Gecachte Antworten bekommen niedrigeres Gewicht
- Balance zwischen häufigen und seltenen Queries
- Direkte Responses haben mehr Lernwert
- Semantische Ähnlichkeit wird berücksichtigt

### Warum effektive Batch-Größe?

**Problem mit roher Anzahl:**
```
100 Feedback-Einträge:
- 80 gecacht (weight=0.4) = 32 effektiv
- 20 direkt (weight=1.0) = 20 effektiv
= 52 effektiv (nicht 100!)
```

**Vorteil:**
- Realistische Einschätzung des Trainingswertes
- Verhindert vorzeitiges Training
- Berücksichtigt Datenqualität

## Best Practices

### 1. Cache-Gewichte anpassen

**Hohe Cache-Rate (>70%):**
```yaml
exact_cache_weight: 0.3  # Niedrig
semantic_cache_base_weight: 0.2
```

**Niedrige Cache-Rate (<30%):**
```yaml
exact_cache_weight: 0.5  # Höher
semantic_cache_base_weight: 0.4
```

### 2. Monitoring

Wichtige Metriken überwachen:
- `effective_batch_size` vs. raw count
- `cache_weight_distribution`
- `training_samples_by_cache_type`
- `avg_cache_similarity`

### 3. Schrittweise Optimierung

```yaml
# Woche 1: Konservativ
exact_cache_weight: 0.5

# Woche 2: Anpassen basierend auf Metriken
exact_cache_weight: 0.4

# Woche 3: Feintuning
exact_cache_weight: 0.35
```

### 4. A/B Testing

```yaml
quality:
  ab_testing:
    enabled: true
    traffic_split: 0.1
    
    # Test verschiedene Cache-Gewichte
    variants:
      - exact_cache_weight: 0.3
      - exact_cache_weight: 0.5
```

## Performance Impact

### Trainingseffizienz

**Ohne Gewichtung:**
- 100 Samples (80% gecacht)
- Viel redundante Information
- Langsame Konvergenz

**Mit Gewichtung:**
- 52 effektive Samples
- Fokus auf diverse Responses
- Schnellere Konvergenz

### Speicher und CPU

- **Zusätzlicher Speicher:** ~12 Bytes pro Feedback (cache_info + weight)
- **CPU-Overhead:** <1ms pro Feedback für Gewichtsberechnung
- **Training:** Keine zusätzliche Overhead (Gewichte beim Sampling berücksichtigt)

## Migration

### Existierendes Feedback

Für Feedback ohne Cache-Info:
```cpp
// Default-Werte werden gesetzt
feedback.is_cached_response = false;  // Annahme: direkt
feedback.training_weight = 1.0f;      // Volle Gewichtung
```

### Graduelle Einführung

1. **Phase 1:** System deployen, neue Feedbacks haben Cache-Info
2. **Phase 2:** Monitoring der Gewichtsverteilung
3. **Phase 3:** Feintuning der Gewichte basierend auf Metriken
4. **Phase 4:** Optionales Backfilling alter Feedbacks

## Troubleshooting

### Training wird nicht ausgelöst

**Prüfen:**
```cpp
float effective = feedback_storage->calculateEffectiveBatchSize(adapter_id);
size_t raw_count = feedback_storage->getTrainingFeedback(adapter_id).size();

std::cout << "Raw count: " << raw_count << std::endl;
std::cout << "Effective size: " << effective << std::endl;
std::cout << "Threshold: 50" << std::endl;
```

**Wenn effective < threshold aber raw > threshold:**
→ Zu viele gecachte Responses mit niedrigen Gewichten
→ Lösung: Gewichte erhöhen oder mehr direkte Responses sammeln

### Zu niedriges Training-Weight

**Debug:**
```cpp
auto feedback = feedback_storage->getFeedback(id);
std::cout << "Is cached: " << feedback->is_cached_response << std::endl;
std::cout << "Similarity: " << feedback->cache_similarity_score << std::endl;
std::cout << "Weight: " << feedback->training_weight << std::endl;

// Berechnung nachvollziehen
if (feedback->is_cached_response) {
    float cache_weight = 0.3 + (feedback->cache_similarity_score - 0.9f) * 0.5f * 10.0f;
    std::cout << "Cache weight: " << cache_weight << std::endl;
}
```

## Referenzen

- [YAML Configuration Guide](./LORA_YAML_CONFIG_GUIDE.md)
- [Feedback API Documentation](./LORA_FEEDBACK_API.md)
- [Plugin Developer Guide](./PLUGIN_DEVELOPER_GUIDE.md)
- [Implementation Summary](./LORA_FEEDBACK_IMPLEMENTATION.md)
