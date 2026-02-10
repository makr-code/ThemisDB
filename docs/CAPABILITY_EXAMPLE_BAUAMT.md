# Beispiel: Bauamt-Shard Capability-Mitteilung

## Übersicht

Dieses Dokument zeigt, wie ein Bauamt-Shard seine Capabilities an das ThemisDB-Routing-System mitteilt. Die Mitteilung erfolgt als **fortschreibendes YAML-Dokument**, das kontinuierlich aktualisiert wird.

## Struktur der Shard-Mitteilung

```
┌─────────────────────────────────────────────────────────────┐
│  Shard-Capability-Mitteilung (YAML)                         │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│  📋 Identifikation                                           │
│     - Shard-ID: shard_hamburg_bauamt_001                    │
│     - Name: Hamburg Bauamt - Hauptshard                     │
│     - Standort: Hamburg, Datacenter de_north_1              │
│     - Version: 1.4.2 (Semantic Versioning)                  │
│                                                              │
│  🎯 Fachgebiete (Domains)                                    │
│     - construction (Bauwesen)                               │
│     - law (Rechtswesen)                                     │
│     - urban_planning (Stadtplanung)                         │
│     - building_permits (Baugenehmigungen)                   │
│                                                              │
│  🏢 Organisationen                                            │
│     - hamburg_bauamt                                        │
│     - hamburg_stadtplanung                                  │
│     - hamburg_baubehoerde                                   │
│     - bezirksamt_nord / mitte / wandsbek                   │
│                                                              │
│  📍 Geografische Regionen                                     │
│     - hamburg (primär)                                      │
│     - hamburg_nord / mitte / wandsbek                      │
│     - germany (sekundär)                                    │
│     - schleswig_holstein (grenzüberschreitend)             │
│                                                              │
│  📄 Datentypen                                                │
│     - building_permits (Baugenehmigungen)                  │
│     - construction_plans (Baupläne)                        │
│     - legal_documents (Rechtsdokumente)                    │
│     - zoning_documents (Bebauungspläne)                    │
│     - inspection_reports (Prüfberichte)                    │
│     ... 5 weitere Typen                                    │
│                                                              │
│  🔑 Schlüsselwörter (60+ Keywords)                           │
│     Deutsch:                                                │
│     - baurecht, baugenehmigung, bauantrag                  │
│     - bebauungsplan, flächennutzungsplan                   │
│     - bauvoranfrage, nutzungsänderung                      │
│     - abstandsflächen, brandschutz, schallschutz          │
│     - wohnfläche, geschossfläche, firsthöhe               │
│     English:                                                │
│     - building, permit, construction, zoning               │
│     - planning, application, approval                      │
│                                                              │
│  📊 Performance-Metriken                                      │
│     - 1.247.893 Dokumente, 342.5 GB                        │
│     - Ø Query-Zeit: 45ms, P95: 120ms                       │
│     - Cache-Trefferquote: 73%                              │
│     - Update-Frequenz: Stündlich                           │
│                                                              │
│  ⭐ Qualitätsindikatoren                                      │
│     - Vollständigkeit: 95% (Hamburg Baugenehmigungen)      │
│     - Aktualität: 92% (Updates < 24h)                      │
│     - Verfügbarkeit: 99.8% (Uptime)                        │
│     - Spezialisierung: 98% (Hochspezialisiert)             │
│                                                              │
│  🔗 Semantic Embeddings                                       │
│     - Modell: paraphrase-multilingual-mpnet-base-v2        │
│     - Dimension: 384                                        │
│     - Vektor-Datei: embeddings/shard_hamburg_bauamt.bin    │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

## Beispiel: Hamburg Bauamt (Ausschnitt)

```yaml
# Shard Capability Configuration - Hamburg Bauamt
shard_id: shard_hamburg_bauamt_001
shard_name: "Hamburg Bauamt - Hauptshard"
datacenter: de_north_1
region: hamburg
last_updated: "2026-02-10T10:15:30Z"
version: "1.4.2"

capabilities:
  domains:
    - construction      # Bauwesen
    - law              # Rechtswesen
    - urban_planning   # Stadtplanung
    - building_permits # Baugenehmigungen
  
  organizations:
    - hamburg_bauamt
    - hamburg_stadtplanung
    - hamburg_baubehoerde
    - bezirksamt_nord
    - bezirksamt_mitte
    - bezirksamt_wandsbek
  
  regions:
    - hamburg
    - hamburg_nord
    - hamburg_mitte
    - hamburg_wandsbek
    - germany
  
  data_types:
    - building_permits           # Baugenehmigungen
    - construction_plans         # Baupläne
    - legal_documents           # Rechtsdokumente
    - planning_applications     # Planungsanträge
    - building_regulations      # Bauvorschriften
    - zoning_documents          # Bebauungspläne
    - inspection_reports        # Prüfberichte
  
  keywords:
    # Fachbegriffe Deutsch
    - baurecht
    - baugenehmigung
    - bauantrag
    - bebauungsplan
    - flächennutzungsplan
    - bauvoranfrage
    - nutzungsänderung
    - abstandsflächen
    - brandschutz
    - schallschutz
    - hamburg
    - bauamt
    
    # English terms
    - building
    - permit
    - construction
    - zoning
    - planning
    
    # Technische Begriffe
    - wohnfläche
    - geschossfläche
    - grundflächenzahl
    - firsthöhe

metadata:
  document_count: 1_247_893
  total_size_gb: 342.5
  avg_query_time_ms: 45
  update_frequency: "hourly"
  owner_team: "hamburg-it-team"

quality_indicators:
  completeness_score: 0.95
  recency_score: 0.92
  availability_score: 0.998
  specialization_score: 0.98
```

## Wie die Mitteilung verwendet wird

### 1. Query kommt rein
```
Query: "Baugenehmigung Hamburg Wohngebäude"
```

### 2. Capability Matching
```
ThemisDB Routing-System:
  ├─ Extrahiert Keywords: ["baugenehmigung", "hamburg", "wohngebäude"]
  ├─ Matched gegen alle Shard-Capabilities
  └─ Ranking:
      1. shard_hamburg_bauamt_001    → Score: 0.95 ⭐
      2. shard_bremen_bauamt_001     → Score: 0.78
      3. shard_de_law_federal_001    → Score: 0.65
      4. shard_berlin_bauamt_001     → Score: 0.52
```

### 3. Iterative Ausführung
```
Iteration 1 (Threshold > 0.8):
  → Query an: shard_hamburg_bauamt_001 (0.95)
  → Ergebnis: 247 Treffer ✓
  → STOP (Ziel erreicht: >= 100 Treffer)

Ergebnis: 1 Shard befragt statt 50 → 98% Traffic gespart
```

## Fortschreibung (Update-Workflow)

### Beispiel: Neue Keywords hinzufügen

**Ausgangslage (Version 1.4.2):**
```yaml
version: "1.4.2"
keywords:
  - baurecht
  - baugenehmigung
  - bauantrag
```

**Query-Log-Analyse zeigt:**
```
Häufige Suchen mit niedrigem Match-Score:
- "sanierungsgenehmigung" (127x, Score: 0.42)
- "denkmalschutz" (89x, Score: 0.38)
```

**Update durchführen:**
```bash
# 1. Datei bearbeiten
vim config/capabilities/shard_hamburg_bauamt_001.yaml

# 2. Keywords hinzufügen
keywords:
  - baurecht
  - baugenehmigung
  - bauantrag
  - sanierungsgenehmigung  # NEU
  - denkmalschutz          # NEU

# 3. Version erhöhen
version: "1.4.3"

# 4. Timestamp aktualisieren
last_updated: "2026-02-10T15:30:00Z"

# 5. Committen
git add config/capabilities/shard_hamburg_bauamt_001.yaml
git commit -m "feat: Add renovation and monument protection keywords"
git push

# 6. Automatische Synchronisation via CI/CD
# → ThemisDB erhält Update
# → Embeddings werden neu generiert
# → Routing verwendet neue Keywords
```

**Nach Update (Version 1.4.3):**
```yaml
version: "1.4.3"
keywords:
  - baurecht
  - baugenehmigung
  - bauantrag
  - sanierungsgenehmigung  # ← NEU
  - denkmalschutz          # ← NEU
```

**Ergebnis:**
```
Query: "Sanierungsgenehmigung Hamburg"
Vorher: Score 0.42 (niedrig)
Nachher: Score 0.89 (hoch) → Shard wird in Iteration 1 befragt ✓
```

## Git History (Beispiel)

```
commit a8800ca
feat: Add renovation and monument protection keywords
- Added: sanierungsgenehmigung, denkmalschutz
- Version: 1.4.2 → 1.4.3
- Reason: Query log analysis showed 127 searches

commit bf9077c
perf: Improve query performance metadata
- avg_query_time_ms: 50 → 45
- cache_hit_ratio: 0.68 → 0.73
- Version: 1.4.1 → 1.4.2

commit 6d591ae
feat: Add processing stages sub-capability
- Added: eingegangen, in_pruefung, genehmigt, abgelehnt
- Version: 1.4.0 → 1.4.1
```

## Zusammenfassung

Die Bauamt-Shard Capability-Mitteilung ist ein **fortschreibendes YAML-Dokument**, das:

✅ **Strukturiert**: Klare Hierarchie (Domains → Organizations → Regions → Data Types)  
✅ **Versioniert**: Semantic Versioning für jede Änderung  
✅ **Git-basiert**: Vollständige History aller Änderungen  
✅ **Lebendig**: Kontinuierliche Updates (automatisch + manuell)  
✅ **Messbar**: Performance-Metriken und Qualitätsindikatoren  
✅ **Semantic**: Embeddings für intelligentes Matching  

**Ziel**: Das Routing-System kann intelligent entscheiden, welche Shards für eine Query relevant sind, ohne alle Shards befragen zu müssen.

## Weitere Beispiele

Siehe auch:
- `shard_hamburg_bauamt_001.yaml` - Vollständiges Beispiel (150+ Zeilen)
- `shard_berlin_gesundheit_001.yaml` - Gesundheitsamt
- `shard_de_law_federal_001.yaml` - Bundesweite Rechtsdatenbank
- `shard_template.yaml` - Template für neue Shards
- `README.md` - Vollständige Dokumentation
- `UPDATE_WORKFLOW.md` - Workflow und Best Practices
