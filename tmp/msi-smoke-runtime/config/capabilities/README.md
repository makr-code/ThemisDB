# Shard Capability Configuration Files

Dieses Verzeichnis enthält die Capability-Konfigurationen für alle ThemisDB-Shards im YAML-Format. Diese Dateien werden kontinuierlich aktualisiert und definieren die Spezialisierung jedes Shards für das adaptive Capability-Based Routing.

## Struktur

Jeder Shard hat eine eigene YAML-Konfigurationsdatei mit dem Schema:
```
shard_<standort>_<fachgebiet>_<nummer>.yaml
```

### Beispiele:
- `shard_hamburg_bauamt_001.yaml` - Hamburg Bauamt (Hauptshard)
- `shard_berlin_gesundheit_001.yaml` - Berlin Gesundheitsamt
- `shard_de_law_federal_001.yaml` - Bundesweite Rechtsdatenbank

## YAML-Schema

```yaml
# Grundlegende Shard-Identifikation
shard_id: string                    # Eindeutige Shard-ID
shard_name: string                  # Beschreibender Name
datacenter: string                  # Datacenter-Lokation
region: string                      # Geografische Region

# Versionierung und Aktualisierung
last_updated: timestamp             # Letzte Aktualisierung (ISO 8601)
version: semver                     # Semantic versioning (MAJOR.MINOR.PATCH)

# Capability-Definition
capabilities:
  domains: [string]                 # Fachgebiete (z.B. construction, law, health)
  organizations: [string]           # Organisationen (z.B. hamburg_bauamt)
  regions: [string]                 # Geografische Regionen (z.B. hamburg, germany)
  data_types: [string]              # Datentypen (z.B. building_permits, legal_documents)
  keywords: [string]                # Schlüsselwörter für Text-Matching
  
  # Optionale Unter-Capabilities
  sub_capabilities:
    <category>: [string]            # Kategorisierte Sub-Capabilities

# Semantic Embeddings (optional, aber empfohlen)
embeddings:
  model: string                     # Verwendetes Embedding-Modell
  dimension: integer                # Vektor-Dimension
  last_generated: timestamp         # Letzte Generierung
  embedding_file: path              # Pfad zur Embedding-Datei

# Metadata für Optimierung
metadata:
  document_count: integer           # Anzahl Dokumente im Shard
  total_size_gb: float              # Gesamtgröße in GB
  common_query_patterns: [string]   # Häufige Query-Muster
  avg_query_time_ms: integer        # Durchschnittliche Query-Zeit
  p95_query_time_ms: integer        # 95. Perzentil Query-Zeit
  cache_hit_ratio: float            # Cache-Trefferquote
  update_frequency: string          # Aktualisierungsfrequenz
  last_major_update: timestamp      # Letzte größere Aktualisierung
  owner_team: string                # Verantwortliches Team
  contact: email                    # Kontakt-Email

# Qualitätsindikatoren für Matching
quality_indicators:
  completeness_score: float         # 0.0-1.0: Vollständigkeit
  recency_score: float              # 0.0-1.0: Aktualität
  availability_score: float         # 0.0-1.0: Verfügbarkeit
  specialization_score: float       # 0.0-1.0: Spezialisierungsgrad
```

## Verwendung

### 1. Capability-Datei erstellen

Für einen neuen Shard:

```bash
cp shard_template.yaml config/capabilities/shard_<name>.yaml
# Datei bearbeiten und anpassen
```

### 2. Capabilities in ThemisDB laden

**Via REST API:**
```bash
# Einzelner Shard
curl -X PUT http://localhost:8080/api/v1/admin/shard/shard_hamburg_bauamt_001/capabilities \
  -H "Content-Type: application/yaml" \
  --data-binary @config/capabilities/shard_hamburg_bauamt_001.yaml

# Bulk-Import aller Shards
curl -X POST http://localhost:8080/api/v1/admin/capabilities/bulk/import \
  -H "Content-Type: application/json" \
  -d '{
    "directory": "/etc/themis/capabilities",
    "format": "yaml"
  }'
```

**Via CLI:**
```bash
themis-admin capabilities import \
  --file config/capabilities/shard_hamburg_bauamt_001.yaml

# Oder alle Dateien in einem Verzeichnis
themis-admin capabilities import \
  --directory config/capabilities/ \
  --format yaml
```

### 3. Capabilities aktualisieren

Capabilities sollten kontinuierlich aktualisiert werden, wenn:
- Neue Datentypen zum Shard hinzugefügt werden
- Neue Keywords identifiziert werden (aus Query-Logs)
- Organisationsstrukturen sich ändern
- Performance-Charakteristiken sich ändern

**Git-basierter Workflow (empfohlen):**

```bash
# 1. Capability-Datei bearbeiten
vim config/capabilities/shard_hamburg_bauamt_001.yaml

# 2. Version erhöhen (PATCH für kleine Änderungen)
# version: "1.4.2" → "1.4.3"

# 3. Timestamp aktualisieren
# last_updated: "2026-02-10T15:30:00Z"

# 4. Committen
git add config/capabilities/shard_hamburg_bauamt_001.yaml
git commit -m "Update Hamburg Bauamt capabilities: add renovation permits"

# 5. Push triggert automatische Synchronisation
git push origin main
```

### 4. Embeddings generieren

Für semantic matching sollten Embeddings generiert werden:

```bash
themis-admin capabilities generate-embeddings \
  --shard shard_hamburg_bauamt_001 \
  --model sentence-transformers/paraphrase-multilingual-mpnet-base-v2 \
  --output embeddings/shard_hamburg_bauamt_001.bin
```

Oder automatisch für alle Shards:

```bash
themis-admin capabilities generate-embeddings \
  --all \
  --model sentence-transformers/paraphrase-multilingual-mpnet-base-v2
```

## Best Practices

### Keywords

**Gute Keywords:**
- ✅ Spezifisch und relevant: `baugenehmigung`, `bebauungsplan`
- ✅ Fachterminologie: `geschossflaechenzahl`, `abstandsflächen`
- ✅ Organisationsnamen: `hamburg_bauamt`, `bezirksamt_nord`
- ✅ Häufige Schreibweisen: `baurecht`, `bau-recht`

**Schlechte Keywords:**
- ❌ Zu generisch: `dokument`, `datei`, `information`
- ❌ Stopwords: `der`, `die`, `das`, `und`, `oder`
- ❌ Redundant: Wenn `hamburg` schon in `regions`, nicht nochmal in keywords

### Domains

Verwenden Sie konsistente Domain-Namen über alle Shards:
- `construction` statt `building`, `bau`, `bauwesen`
- `law` statt `legal`, `recht`, `rechtswesen`
- `health` statt `medical`, `gesundheit`, `medizin`

### Versioning

Semantic Versioning:
- **MAJOR** (1.0.0 → 2.0.0): Breaking changes (z.B. Struktur-Änderung)
- **MINOR** (1.4.0 → 1.5.0): Neue Capabilities hinzugefügt
- **PATCH** (1.4.2 → 1.4.3): Keywords/Metadata aktualisiert

### Update-Frequenz

Empfohlene Update-Frequenzen je nach Shard-Typ:

| Shard-Typ | Frequenz | Beispiel |
|-----------|----------|----------|
| Real-time Daten | Kontinuierlich | Gesundheitsdaten, Börsendaten |
| Hochfrequent | Stündlich | Bauanträge, Verwaltungsvorgänge |
| Normal | Täglich | Rechtsdatenbank, Archive |
| Statisch | Wöchentlich/Monatlich | Historische Daten |

## Monitoring

### Capability-Qualität überwachen

```bash
# Capability-Statistiken abrufen
curl http://localhost:8080/api/v1/admin/capabilities/stats

# Für einzelnen Shard
curl http://localhost:8080/api/v1/admin/shard/shard_hamburg_bauamt_001/capabilities/stats
```

### Query-Matching überwachen

```bash
# Matching-Performance
curl http://localhost:8080/api/v1/admin/stats/adaptive_routing

# Queries ohne gute Matches (Candidates für neue Keywords)
curl http://localhost:8080/api/v1/admin/stats/low_match_queries?threshold=0.5
```

## Automatisierung

### CI/CD Integration

```yaml
# .github/workflows/capabilities-sync.yml
name: Sync Shard Capabilities

on:
  push:
    paths:
      - 'config/capabilities/**/*.yaml'

jobs:
  sync:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      
      - name: Sync to ThemisDB
        run: |
          for file in config/capabilities/*.yaml; do
            shard_id=$(yq eval '.shard_id' $file)
            curl -X PUT \
              http://themis-admin:8080/api/v1/admin/shard/$shard_id/capabilities \
              -H "Content-Type: application/yaml" \
              --data-binary @$file
          done
```

### Automatische Keyword-Extraktion aus Query-Logs

```bash
# Query-Logs analysieren und neue Keywords vorschlagen
themis-admin capabilities suggest-keywords \
  --shard shard_hamburg_bauamt_001 \
  --query-log /var/log/themis/queries.log \
  --min-frequency 100 \
  --output keyword-suggestions.yaml
```

## Troubleshooting

### Problem: Shard wird nicht für relevante Queries ausgewählt

**Lösung:**
1. Query-Logs prüfen: Welche Keywords werden verwendet?
2. Keywords zur Capability-Datei hinzufügen
3. Embeddings neu generieren
4. Version erhöhen und committen

### Problem: Zu viele irrelevante Shards werden ausgewählt

**Lösung:**
1. Keywords zu generisch → spezifischere Keywords verwenden
2. Thresholds in adaptive_routing config erhöhen
3. Domain-Zuordnung präzisieren

### Problem: Embeddings veraltet

**Lösung:**
```bash
# Embeddings für alle Shards neu generieren
themis-admin capabilities regenerate-embeddings --all

# Oder nur für geänderte Capabilities
themis-admin capabilities regenerate-embeddings --stale
```

## Weitere Ressourcen

- **Dokumentation**: `docs/ADAPTIVE_SHARD_ROUTING.md`
- **API Referenz**: `docs/api/ADMIN_API.md`
- **Konfiguration**: `config/adaptive_routing.example.json`
- **Schema-Vorlage**: `config/capabilities/shard_template.yaml`

## Kontakt

Bei Fragen oder Problemen:
- **Issues**: https://github.com/makr-code/ThemisDB/issues
- **Dokumentation**: https://makr-code.github.io/ThemisDB/
- **Team**: capabilities-team@themisdb.org
