# Kontinuierlicher Lern-Orchestrator

## Überblick

Der **Kontinuierliche Lern-Orchestrator** ist ein zentrales System, das automatisch alle RAG-Komponenten (Retrieval-Augmented Generation) durch kontinuierliche Überwachung, A/B-Tests und statistische Validierung optimiert. Es ermöglicht ThemisDB, sich selbst verbessernde KI-Systeme aufzubauen, die aus Produktionsdaten ohne manuellen Eingriff lernen.

## Hauptfunktionen

### 🔄 Automatisches LoRA-Retraining
- **Feedback-basierte Trigger**: Retraining wenn genug Benutzerfeedback gesammelt wurde
- **Leistungsbasierte Trigger**: Retraining bei Genauigkeitsabfall unter Schwellenwert
- **Zeitbasierte Trigger**: Regelmäßiges Retraining nach Zeitplan (z.B. täglich)
- **Nahtlose Integration**: Funktioniert mit vorhandenen ThemisHelpLoRA-Adaptern

### 📝 Prompt-Optimierung
- **Leistungsanalyse**: Identifiziert automatisch leistungsschwache Prompts
- **LLM-generierte Variationen**: Erstellt verbesserte Prompt-Variationen
- **Historisches Testen**: Validiert Verbesserungen an vergangenen fehlgeschlagenen Abfragen
- **Schrittweise Einführung**: Führt Verbesserungen durch A/B-Tests ein

### 🎯 Automatische Retrieval-Parameter-Optimierung
- **Bayesianische Optimierung**: Effiziente Erkundung des Parameterraums
- **Multi-Ziel**: Optimiert top_k, similarity_threshold, coverage_threshold
- **Datengesteuert**: Verwendet historische Abfrageleistung zur Validierung
- **Sichere Bereitstellung**: A/B-Tests von Parameteränderungen vor vollständigem Rollout

### 🧪 A/B-Testing-Framework
- **Traffic-Splitting**: Leitet Prozentsatz des Traffics zu neuen Modellen
- **Statistische Validierung**: Zweistichproben-t-Tests für Signifikanz
- **Automatische Entscheidungen**: Befördert oder rollt zurück basierend auf Ergebnissen
- **Sicherheitsgarantien**: Erfordert minimale Verbesserungsschwelle

### 📊 Metrik-Persistenz
- **Langzeittracking**: Speichert alle Interaktionen und Metriken
- **Leistungshistorie**: Zeitreihendaten für Trendanalyse
- **Modell-Registry**: Bewahrt Checkpoints erfolgreicher Modelle
- **Verbesserungsereignisse**: Protokolliert alle automatischen Optimierungen

## Architektur

```
┌─────────────────────────────────────────────────────────────┐
│         ContinuousLearningOrchestrator                      │
│  ┌────────────┐  ┌────────────┐  ┌────────────┐           │
│  │ LoRA       │  │ Prompt     │  │ Retrieval  │           │
│  │ Training   │  │ Optimizer  │  │ Tuner      │           │
│  └────────────┘  └────────────┘  └────────────┘           │
│         ↓               ↓               ↓                   │
│  ┌──────────────────────────────────────────────┐          │
│  │         A/B Testing Framework                │          │
│  └──────────────────────────────────────────────┘          │
│         ↓                                                   │
│  ┌──────────────────────────────────────────────┐          │
│  │       Metrics Store (RocksDB/SQLite)         │          │
│  └──────────────────────────────────────────────┘          │
└─────────────────────────────────────────────────────────────┘
```

## Schnellstart

### 1. Basiskonfiguration

```cpp
#include "rag/continuous_learning_orchestrator.h"

using namespace themis::rag::learning;

// Orchestrator konfigurieren
ContinuousLearningConfig config;
config.min_feedback_samples = 100;        // Retraining nach 100 Feedback-Elementen
config.min_accuracy_drop = 0.05;          // 5% Abfall löst Retraining aus
config.retraining_interval = std::chrono::hours(24);  // Tägliches Retraining
config.enable_ab_testing = true;
config.ab_test_traffic_split = 0.1;       // 10% Traffic für neue Modelle
config.min_improvement_threshold = 0.02;  // 2% minimale Verbesserung

auto orchestrator = std::make_unique<ContinuousLearningOrchestrator>(config);
```

### 2. Komponenten registrieren

```cpp
// LoRA-Adapter registrieren
orchestrator->registerLoRAAdapter("themis_help_lora", "Dokumentations-Q&A");

// Retrieval-System registrieren
orchestrator->registerRetrievalSystem("vector_index_main");

// Prompt-Bibliothek registrieren
orchestrator->registerPromptSystem("prompt_library");

// Knowledge Gap Detector registrieren
orchestrator->registerKnowledgeGapDetector("gap_detector");
```

### 3. Lernschleife starten

```cpp
// Hintergrund-Thread für kontinuierliches Lernen starten
orchestrator->startLearningLoop();
```

### 4. Interaktionen protokollieren

```cpp
// Während Produktion jede RAG-Interaktion protokollieren
Interaction interaction;
interaction.interaction_id = "unique_id";
interaction.timestamp = std::chrono::system_clock::now();
interaction.query = user_query;
interaction.generated_answer = rag_response;
interaction.confidence_score = confidence;
interaction.user_feedback = FeedbackType::POSITIVE;  // Falls verfügbar

orchestrator->logInteraction(interaction);
```

### 5. Fortschritt überwachen

```cpp
// Lernstatistiken prüfen
auto stats = orchestrator->getStats();
std::cout << "Genauigkeit: " << stats.current_accuracy * 100 << "%" << std::endl;
std::cout << "Trend: " << (stats.accuracy_trend > 0 ? "↑" : "↓") << std::endl;
std::cout << "Retraining-Anzahl: " << stats.lora_retraining_count << std::endl;

// Prüfen ob System sich verbessert
bool improving = orchestrator->isSystemImproving();
```

## Konfigurationsparameter

| Parameter | Standard | Beschreibung |
|-----------|---------|-------------|
| `min_feedback_samples` | 100 | Minimale Feedback-Elemente vor Retraining |
| `min_accuracy_drop` | 0.05 | Genauigkeitsabfall (5%) der Retraining auslöst |
| `retraining_interval` | 24h | Maximale Zeit zwischen Retraining-Zyklen |
| `ab_test_traffic_split` | 0.1 | Prozentsatz des Traffics für neue Modelle (10%) |
| `min_ab_samples` | 1000 | Minimale Stichproben für statistische Signifikanz |
| `min_improvement_threshold` | 0.02 | Minimale Verbesserung zur Beförderung (2%) |
| `enable_auto_rollback` | true | Automatischer Rollback bei negativen Ergebnissen |
| `learning_loop_interval` | 3600s | Intervall zwischen Lernprüfungen |

## Lern-Trigger

### LoRA Retraining-Trigger

1. **Feedback-Akkumulation**
   - Löst aus wenn `feedback_count >= min_feedback_samples`
   - Stellt genug Trainingsdaten sicher

2. **Genauigkeitsabfall**
   - Löst aus wenn `current_accuracy < historical_avg - min_accuracy_drop`
   - Reagiert auf Leistungsverschlechterung

3. **Geplantes Intervall**
   - Löst aus wenn `time_since_last_training > retraining_interval`
   - Stellt regelmäßige Updates sicher

## Datenstrukturen

### Interaction

Vollständiger Datensatz einer RAG-Interaktion:

```cpp
struct Interaction {
    std::string interaction_id;
    std::chrono::system_clock::time_point timestamp;
    
    // Eingabe
    std::string query;
    std::vector<RetrievedDocument> retrieved_docs;
    std::string prompt_template_used;
    
    // Ausgabe
    std::string generated_answer;
    std::vector<double> token_probabilities;
    
    // Metriken
    knowledge_gap::DetectionResult gap_detection_result;
    double perplexity;
    double confidence_score;
    
    // Feedback
    std::optional<FeedbackType> user_feedback;
    std::optional<std::string> user_correction;
    
    // Metadaten
    std::string model_version;
    std::string retrieval_config_version;
    std::string prompt_version;
    bool is_ab_test_traffic;
};
```

### LearningStats

Aktuelle Lernstatistiken:

```cpp
struct LearningStats {
    size_t total_interactions_logged;
    size_t lora_retraining_count;
    size_t prompt_optimizations;
    size_t retrieval_optimizations;
    
    double current_accuracy;
    double accuracy_7d_avg;
    double accuracy_trend;  // Positiv = Verbesserung
    
    std::vector<ImprovementEvent> recent_improvements;
    std::vector<ABTestInfo> active_ab_tests;
};
```

## Leistungsüberlegungen

### CPU-Overhead
- **Lernschleife**: < 0,1% während Leerlaufphasen
- **Interaktionsprotokollierung**: < 10μs pro Interaktion
- **A/B-Test-Bewertung**: < 100ms

### Speichernutzung
- **Pro Interaktion**: ~500 Bytes
- **10k Interaktionen**: ~5 MB
- **Wachstumsrate**: ~50 MB pro 1M Interaktionen

## Best Practices

### 1. Konservativ starten
```cpp
config.min_feedback_samples = 200;      // Mehr Daten = besseres Training
config.ab_test_traffic_split = 0.05;    // Mit 5% starten
config.min_improvement_threshold = 0.05; // 5% Verbesserung verlangen
```

### 2. Aktiv überwachen
- Statistiken regelmäßig während Erstbereitstellung prüfen
- Auf unerwartete Retraining-Trigger achten
- A/B-Test-Entscheidungen validieren

### 3. Schrittweise Einführung
- Mit nicht-kritischen Komponenten beginnen
- Eine Optimierung nach der anderen aktivieren
- Traffic-Split schrittweise erhöhen

### 4. Qualitätsfeedback sammeln
- Benutzerfeedback zu Antworten fördern
- Explizite Korrekturen verfolgen
- Implizite Signale überwachen (Klickrate, Verweildauer)

## Problemlösung

### Problem: Kein Retraining findet statt

**Ursachen:**
- Nicht genug Feedback-Stichproben
- Zeitintervall nicht abgelaufen
- Kein Genauigkeitsabfall erkannt

**Lösung:**
```cpp
auto stats = orchestrator->getStats();
std::cout << "Interaktionen: " << stats.total_interactions_logged << std::endl;
std::cout << "Retraining-Anzahl: " << stats.lora_retraining_count << std::endl;

// Manuell auslösen
orchestrator->triggerLearningIteration();
```

## Integration mit bestehenden Komponenten

### KnowledgeGapDetector
```cpp
// Gap-Detection-Ergebnisse werden automatisch protokolliert
interaction.gap_detection_result = detector->detect(query, docs, answer);
orchestrator->logInteraction(interaction);
```

### ThemisHelpLoRA
```cpp
// Adapter für automatisches Retraining registrieren
orchestrator->registerLoRAAdapter("themis_help", adapter);

// LoRA wird basierend auf Triggern retrained
// Keine manuellen trainFromFeedback()-Aufrufe nötig
```

## Zukünftige Erweiterungen

- **Multi-Ziel-Optimierung**: Balance zwischen Genauigkeit, Latenz und Kosten
- **Föderiertes Lernen**: Verteiltes Training über Knoten
- **Erklärbare Verbesserungen**: Menschenlesbare Berichte
- **Kausale Inferenz**: Identifizierung welche Änderungen Verbesserungen antreiben
- **Verstärkendes Lernen**: Lernen aus Benutzerinteraktionen über Zeit

## API-Referenz

Siehe Inline-Dokumentation in Header-Dateien:
- `include/rag/continuous_learning_orchestrator.h`
- `include/rag/ab_testing_framework.h`
- `include/rag/bayesian_optimizer.h`
- `include/rag/learning_metrics.h`

## Beispiele

Vollständiges Beispiel: `examples/continuous_learning_example.cpp`

## Support

Für Probleme oder Fragen:
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Dokumentation: Siehe `docs/` Verzeichnis
- Community: ThemisDB Discord Server
