# GPU Impact Analysis Plugin - Beispiele

Praktische Code-Beispiele zur Verwendung des GPU Impact Analysis Plugin.

## Übersicht

Dieses Verzeichnis enthält vollständige Arbeitsbeispiele für verschiedene Anwendungsfälle:

1. **E-Commerce**: Preisänderungs-Impact-Analyse
2. **GDPR**: Datenlöschungs-Compliance (Artikel 17)
3. **API**: Breaking Change Risikobewertung (Monte Carlo)
4. **Batch**: Mehrere Änderungen gleichzeitig analysieren

## Verfügbare Beispiele

### 1. E-Commerce Price Change
**Datei**: `ecommerce_price_change.yaml`

Analysiert die Auswirkungen einer Preisänderung auf:
- Kundenbestellungen
- Warenkörbe
- Produktempfehlungen
- Verkaufsprognosen

### 2. GDPR Data Deletion
**Datei**: `gdpr_data_deletion.yaml`

DSGVO-konforme Analyse einer Datenlöschungsanfrage:
- Identifikation aller betroffenen Dokumente
- Unterscheidung: Löschen vs. Anonymisieren
- Compliance-Prüfung

### 3. API Breaking Change Risk
**Datei**: `api_breaking_change_risk.yaml`

Monte Carlo Risikobewertung mit 100.000 Simulationen:
- Value at Risk (VaR) Berechnung
- Mehrere Migrationsszenarien
- Risiko-Klassifizierung

### 4. Batch Analysis
**Datei**: `batch_impact_analysis.yaml`

Gleichzeitige Analyse mehrerer Produktänderungen:
- Parallele Verarbeitung
- Aggregierte Statistiken
- JSON-Export

## Verwendung

### Voraussetzung

Plugin muss gebaut und verfügbar sein:

```bash
cd /home/runner/work/ThemisDB/ThemisDB
cmake -B build -DTHEMIS_BUILD_ENTERPRISE_PLUGINS=ON
cmake --build build
```

### Beispiel ausführen

Die vollständigen C++ Beispiel-Implementierungen finden Sie in:
- **Dokumentation**: `../../docs/enterprise/gpu_impact_analysis_working_examples.md`

Dort sind für jedes YAML-Beispiel vollständige C++ Programme mit:
- YAML-Laden
- Plugin-Initialisierung
- Impact-Analyse
- Ergebnis-Ausgabe

## Quick Start

### Minimales Beispiel

```cpp
#include "enterprise/gpu_impact_analysis_plugin.h"

int main() {
    using namespace themis::enterprise;
    
    // Plugin erstellen
    auto plugin = createGPUImpactAnalysisPlugin();
    
    // Initialisieren
    nlohmann::json config = {{"gpu_backend", "cpu"}};
    plugin->initialize(config);
    
    // Änderung definieren
    IGPUImpactAnalysisPlugin::DocumentChange change;
    change.document_id = "products/example";
    change.change_type = "update";
    change.magnitude = 0.5;
    change.timestamp = std::time(nullptr) * 1000;
    
    // Analysieren
    auto result = plugin->analyzeDocumentChangeImpact(change, {});
    
    // Ausgabe
    std::cout << "Affected nodes: " << result.total_affected_count << std::endl;
    
    plugin->shutdown();
    return 0;
}
```

## YAML Struktur

Alle Beispiele folgen dieser Struktur:

```yaml
analysis:
  name: "Beispielname"
  type: "impact_analysis"  # oder monte_carlo_risk, batch_impact, etc.

document_change:
  document_id: "eindeutige-id"
  change_type: "änderungstyp"
  magnitude: 0.0-1.0
  timestamp: "ISO-8601"

options:
  max_depth: 5
  impact_threshold: 0.01
  use_gpu: false

fem_config:
  damping_factor: 0.85
  max_iterations: 100
  convergence_threshold: 0.001
```

## Anpassung

### Eigene Beispiele erstellen

1. Kopieren Sie ein bestehendes YAML
2. Passen Sie die Parameter an
3. Implementieren Sie den C++ Code (siehe Dokumentation)
4. Kompilieren und ausführen

### Parameter-Tuning

**FEM Damping Factor** (0.0-1.0):
- `0.7` = Niedrige Ausbreitung (lokalisiert)
- `0.85` = Standard (ausgewogen)
- `0.95` = Hohe Ausbreitung (weitreichend)

**Impact Threshold** (0.0-1.0):
- `0.001` = Sehr sensitiv (alles erfassen)
- `0.01` = Standard
- `0.1` = Nur signifikante Impacts

**Monte Carlo Simulations**:
- `1,000` = Schnell, ungefähre Ergebnisse
- `10,000` = Standard
- `100,000` = Präzise, langsamer

## Weitere Ressourcen

- **Vollständige Code-Beispiele**: [../../docs/enterprise/gpu_impact_analysis_working_examples.md](../../docs/enterprise/gpu_impact_analysis_working_examples.md)
- **Plugin-Dokumentation**: [../../docs/enterprise/gpu_impact_analysis_plugin.md](../../docs/enterprise/gpu_impact_analysis_plugin.md)
- **API-Referenz**: [../../plugins/enterprise/gpu_impact_analysis/README.md](../../plugins/enterprise/gpu_impact_analysis/README.md)

## Support

- **GitHub Issues**: https://github.com/makr-code/ThemisDB/issues
- **Enterprise Support**: enterprise-support@themisdb.com

---

**Version:** 1.0.0  
**Last Updated:** 2025-12-07
