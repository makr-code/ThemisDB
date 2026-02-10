# Prompt-Vorlagen Übersicht

Diese Übersicht beschreibt alle verfügbaren Prompt-Vorlagen in deutscher Sprache.

## 📁 Verzeichnisstruktur

```
config/prompts/
├── README.md                       - Englische Dokumentation
├── UEBERSICHT.md                  - Diese Datei (Deutsche Übersicht)
├── standard_prompts.yaml           - Standard-Anfragen
├── scientific_prompts.yaml         - Wissenschaftliche Methoden
├── legal_prompts.yaml              - Rechtliche Anfragen
├── administrative_law_prompts.yaml - Verwaltungsrecht (NEU!)
├── technical_prompts.yaml          - Technische Anfragen
├── economic_prompts.yaml           - Wirtschaftliche Anfragen
├── mathematical_prompts.yaml       - Mathematische Anfragen
└── geographic_prompts.yaml         - Geografische Anfragen
```

## 🎯 Domänen-Übersicht

### 1. Standard-Anfragen (standard_prompts.yaml)

**Zweck**: Allgemeine Datenbankabfragen ohne spezialisiertes Fachwissen

**6 Prompts enthalten:**
- `standard_query` - Allgemeine Anfragen
- `information_retrieval` - Informationsabruf
- `data_summary` - Datenzusammenfassungen
- `comparison_query` - Vergleiche
- `aggregation_query` - Aggregationen
- `search_query` - Suchoperationen

**Anwendungsfälle:**
- Einfache Datenbankabfragen
- Allgemeine Informationsanfragen
- Datenexploration

---

### 2. Wissenschaftliche Anfragen (scientific_prompts.yaml)

**Zweck**: Wissenschaftliche Forschungsmethoden und akademische Arbeit

**7 Prompts enthalten:**
- `scientific_genesis` - Genese (Ursprungsanalyse)
- `scientific_synthesis` - Synthese (Quellenintegration)
- `scientific_hypothesis` - Hypothesentests
- `scientific_analysis` - Wissenschaftliche Datenanalyse
- `literature_review` - Literaturrecherche
- `experimental_design` - Versuchsplanung
- `peer_review_analysis` - Peer-Review-Analyse

**Anwendungsfälle:**
- Forschungsarbeiten
- Hypothesentests
- Experimentelles Design
- Peer-Review

**Methoden abgedeckt:**
- ✅ Genesis (Ursprung und Entwicklung)
- ✅ Synthese (Mehrere Quellen kombinieren)
- ✅ Analyse (Datenanalyse)
- ✅ Experimentaldesign
- ✅ Literaturrecherche

---

### 3. Rechtliche Anfragen (legal_prompts.yaml)

**Zweck**: Rechtliche Analysen, Vertragsüberprüfung, Compliance

**7 Prompts enthalten:**
- `legal_analysis` - Rechtsanalyse
- `contract_review` - Vertragsüberprüfung
- `compliance_check` - Compliance-Prüfung
- `legal_research` - Rechtliche Recherche
- `risk_assessment` - Risikoanalyse
- `due_diligence` - Due Diligence
- `policy_interpretation` - Richtlinieninterpretation

**Anwendungsfälle:**
- Vertragsüberprüfung
- Compliance-Prüfungen
- Rechtliche Recherche
- Risikobewertung

**⚠️ Wichtiger Hinweis:**
Diese Vorlagen dienen nur zur Information, nicht als Rechtsberatung.
Konsultieren Sie immer einen qualifizierten Rechtsanwalt.

---

### 3b. Verwaltungsrecht (administrative_law_prompts.yaml) ⭐ NEU!

**Zweck**: Deutsches Verwaltungsrecht, Verwaltungsverfahren, Immissionsschutzrecht

**8 Prompts enthalten:**
- `verwaltungsrecht_analysis` - Verwaltungsrechtliche Analyse
- `verwaltungsverfahren` - Verwaltungsverfahren nach VwVfG
- `bimschg_immissionsschutz` - Bundes-Immissionsschutzgesetz (BImSchG)
- `ta_laerm_noise_analysis` - TA Lärm - Lärmschutzanalyse
- `genehmigungsverfahren` - Genehmigungsverfahren
- `umweltrecht_compliance` - Umweltrechtliche Compliance
- `verwaltungsakt_pruefung` - Verwaltungsaktprüfung
- `baurecht_genehmigung` - Baurecht und Baugenehmigungen

**Anwendungsfälle:**
- Verwaltungsakte prüfen
- Genehmigungsverfahren analysieren (BImSchG, Baurecht)
- Immissionsschutzrechtliche Bewertung
- Lärmschutz nach TA Lärm
- Umweltrechtliche Compliance-Prüfung
- Baurechtsanalyse und Baugenehmigungen

**Rechtsgebiete abgedeckt:**
- ✅ Allgemeines Verwaltungsrecht
- ✅ Verwaltungsverfahrensgesetz (VwVfG)
- ✅ Bundes-Immissionsschutzgesetz (BImSchG)
- ✅ TA Lärm (Technische Anleitung zum Schutz gegen Lärm)
- ✅ Umweltrecht (WHG, KrWG, BBodSchG, BNatSchG)
- ✅ Baurecht (BauGB, BauNVO, Landesbauordnung)
- ✅ Genehmigungsverfahren

**Prüfungsschemata:**
- Formelle und materielle Rechtmäßigkeit
- Ermessensprüfung
- Verhältnismäßigkeitsprüfung
- Grenzwertprüfung (TA Lärm Immissionsrichtwerte)
- Bauplanungsrecht und Bauordnungsrecht
- Umweltrechtliche Compliance-Checklisten

**⚠️ Wichtiger Hinweis:**
Diese Vorlagen dienen nur zur Information, nicht als Rechtsberatung.
Konsultieren Sie immer einen qualifizierten Rechtsanwalt für konkrete Rechtsfragen.

---

### 4. Technische Anfragen (technical_prompts.yaml)

**Zweck**: Technische Analysen, Systemarchitektur, Engineering

**8 Prompts enthalten:**
- `technical_architecture` - Architekturanalyse
- `debugging_analysis` - Fehleranalyse
- `performance_optimization` - Performance-Optimierung
- `code_review` - Code-Review
- `system_design` - Systemdesign
- `technical_documentation` - Technische Dokumentation
- `security_analysis` - Sicherheitsanalyse
- `api_design` - API-Design

**Anwendungsfälle:**
- Systemdesign
- Debugging
- Performance-Tuning
- Code-Review
- Dokumentation

---

### 5. Wirtschaftliche Anfragen (economic_prompts.yaml)

**Zweck**: Geschäftsanalyse, Finanzanalyse, Marktforschung

**8 Prompts enthalten:**
- `business_analysis` - Geschäftsanalyse
- `financial_analysis` - Finanzanalyse
- `market_research` - Marktforschung
- `economic_forecast` - Wirtschaftsprognosen
- `cost_benefit_analysis` - Kosten-Nutzen-Analyse
- `pricing_strategy` - Preisstrategie
- `roi_analysis` - ROI-Analyse
- `competitive_analysis` - Wettbewerbsanalyse

**Anwendungsfälle:**
- Geschäftsplanung
- Finanzanalyse
- Marktforschung
- Investitionsentscheidungen

---

### 6. Mathematische Anfragen (mathematical_prompts.yaml)

**Zweck**: Mathematische Probleme, Statistik, numerische Methoden

**8 Prompts enthalten:**
- `mathematical_analysis` - Mathematische Analyse
- `statistical_analysis` - Statistische Analyse
- `optimization_problem` - Optimierungsprobleme
- `probability_calculation` - Wahrscheinlichkeitsrechnung
- `numerical_methods` - Numerische Methoden
- `linear_algebra` - Lineare Algebra
- `calculus_problem` - Analysis/Calculus
- `combinatorics` - Kombinatorik

**Anwendungsfälle:**
- Statistische Analyse
- Mathematische Modellierung
- Optimierung
- Numerische Berechnungen

---

### 7. Geografische Anfragen (geographic_prompts.yaml)

**Zweck**: Geografische Abfragen, Raumanalyse, GIS

**8 Prompts enthalten:**
- `location_query` - Standortabfragen
- `spatial_analysis` - Raumanalyse
- `distance_calculation` - Entfernungsberechnungen
- `region_analysis` - Regionalanalyse
- `map_generation` - Kartenbeschreibungen
- `route_planning` - Routenplanung
- `geospatial_statistics` - Geospatialstatistik
- `geocoding` - Geocodierung

**Anwendungsfälle:**
- GIS-Operationen
- Raumanalyse
- Routenplanung
- Standortdienste

---

## 📊 Gesamtstatistik

- **Domänen**: 7
- **Prompts gesamt**: 53+
- **Zeilen Code**: 2.071+
- **Gesamtgröße**: 62KB
- **Sprache**: Englisch (Prompts), Deutsch (Dokumentation)

## 💡 Verwendung

### 1. Prompts laden

```cpp
#include "prompt_engineering/prompt_manager.h"

auto manager = std::make_shared<PromptManager>();

// Alle Domänen laden
manager->loadFromYAML("config/prompts/standard_prompts.yaml");
manager->loadFromYAML("config/prompts/scientific_prompts.yaml");
manager->loadFromYAML("config/prompts/legal_prompts.yaml");
manager->loadFromYAML("config/prompts/administrative_law_prompts.yaml");  // NEU!
manager->loadFromYAML("config/prompts/technical_prompts.yaml");
manager->loadFromYAML("config/prompts/economic_prompts.yaml");
manager->loadFromYAML("config/prompts/mathematical_prompts.yaml");
manager->loadFromYAML("config/prompts/geographic_prompts.yaml");
```

### 2. Prompt mit Kontext verwenden

```cpp
// Kontext erstellen
std::unordered_map<std::string, std::string> context = {
    {"query", "Analysiere Kundendaten"},
    {"context", "Kundendaten..."}
};

// Prompt mit injiziertem Kontext abrufen
auto prompt = manager->getPromptWithContext("statistical_analysis", context);

// Mit LLM verwenden
auto response = llm->generate(prompt.value());
```

### 3. Verwaltungsrecht Beispiel (NEU!)

```cpp
// Verwaltungsrechtliche Analyse
std::unordered_map<std::string, std::string> context = {
    {"query", "Genehmigung nach BImSchG prüfen"},
    {"context", "Anlage: Produktionsstätte..."}
};

auto prompt = manager->getPromptWithContext("bimschg_immissionsschutz", context);
auto response = llm->generate(prompt.value());

// Lärmschutzprüfung nach TA Lärm
context = {
    {"query", "Lärmemissionen bewerten"},
    {"context", "Messwerte: ..."}
};

prompt = manager->getPromptWithContext("ta_laerm_noise_analysis", context);
response = llm->generate(prompt.value());
```

### 4. Beispiele

Siehe `examples/domain_prompts_usage_example.cpp` für vollständige Beispiele aller 8 Domänen.

## 🔧 Template-Variablen

Alle Prompts unterstützen dynamische Variablen:

- `{query}` - Benutzeranfrage
- `{context}` - Zusätzlicher Kontext
- `{version}` - Datenbankversion
- `{edition}` - Edition (Community, Enterprise)
- `{table_count}` - Anzahl Tabellen
- `{total_rows}` - Gesamtanzahl Zeilen
- `{tables}` - JSON-Liste der Tabellen
- `{schema}` - Vollständiges Schema als JSON
- `{capabilities}` - Aktivierte Funktionen

## 📊 Statistik

**Prompt-Dateien gesamt**: 8
**Prompts gesamt**: 61+
**Abgedeckte Domänen**:
- Standard (6 Prompts)
- Wissenschaftlich (7 Prompts)
- Rechtlich (7 Prompts)
- Verwaltungsrecht (8 Prompts) ⭐ NEU!
- Technisch (8 Prompts)
- Wirtschaftlich (8 Prompts)
- Mathematisch (8 Prompts)
- Geografisch (8 Prompts)

**Spezielle Abdeckung**:
- Deutsches Verwaltungsrecht (Verwaltungsrecht)
- Umweltrecht (BImSchG, TA Lärm, Umweltrecht)
- Baurecht (Baurecht, Bauordnungsrecht)
- Verwaltungsverfahren (VwVfG, Genehmigungsverfahren)

## 🎯 Empfehlungen

### Für Standard-Anfragen
👉 Verwenden Sie `standard_prompts.yaml`

### Für wissenschaftliche Arbeit
👉 Verwenden Sie `scientific_prompts.yaml`
- Genesis-Analysen
- Synthese mehrerer Quellen
- Hypothesentests

### Für rechtliche Fragen (Allgemein)
👉 Verwenden Sie `legal_prompts.yaml`
- ⚠️ Nur zur Information, keine Rechtsberatung

### Für Verwaltungsrecht (BImSchG, TA Lärm, etc.) ⭐ NEU!
👉 Verwenden Sie `administrative_law_prompts.yaml`
- Verwaltungsverfahren nach VwVfG
- Genehmigungen nach BImSchG
- Lärmschutz nach TA Lärm
- Umweltrechtliche Compliance
- Baurecht und Baugenehmigungen
- ⚠️ Nur zur Information, keine Rechtsberatung

### Für technische Probleme
👉 Verwenden Sie `technical_prompts.yaml`
- Debugging
- Performance-Optimierung
- Systemdesign

### Für Geschäftsanalysen
👉 Verwenden Sie `economic_prompts.yaml`
- ROI-Berechnungen
- Marktforschung
- Finanzanalyse

### Für mathematische Probleme
👉 Verwenden Sie `mathematical_prompts.yaml`
- Statistik
- Optimierung
- Numerische Methoden

### Für geografische Anfragen
👉 Verwenden Sie `geographic_prompts.yaml`
- Entfernungen
- Routenplanung
- Raumanalyse

## 🔗 Integration

Diese Prompts funktionieren mit:
- ✅ PromptManager (Laden/Verwalten)
- ✅ PromptOptimizer (Verbessern)
- ✅ PromptPerformanceTracker (Metriken)
- ✅ FeedbackCollector (Feedback)
- ✅ PromptVersionControl (Versionierung)
- ✅ SelfImprovementOrchestrator (Autonome Optimierung)
- ✅ PromptEngineeringIntegration (Nahtlose Integration)

## ✅ Status

**Implementierung**: ✅ Vollständig
**Dokumentation**: ✅ Vollständig
**Beispiele**: ✅ Vorhanden
**Produktionsbereit**: ✅ Ja

---

**Lizenz**: Apache-2.0  
**Copyright**: © 2026 ThemisDB Contributors
