# Data Directory

Dieses Verzeichnis enthält Daten für das Railway Monitoring System.

## Struktur

```
data/
├── cache/          # Gecachte API-Daten (Overpass, Elevation, etc.)
├── config/         # Konfigurationsdateien
├── samples/        # Beispieldaten für Tests
└── temp/           # Temporäre Dateien (wird ignoriert)
```

## Cache-Daten

- **Overpass API**: OSM-Daten für Kreuzungen, Siedlungen, Bahnlinien
- **Elevation Data**: Höhendaten für Geländeanalyse
- **Land Prices**: BORIS Bodenrichtwerte

Cache-Einträge haben konfigurierbare TTL (Time To Live):
- Overpass: 7 Tage
- Elevation: 365 Tage
- Land Prices: 30 Tage

## Konfiguration

Beispiel-Konfigurationsdateien für:
- ThemisDB Connection
- Ollama LLM Settings
- Map Rendering Settings

## Hinweise

- `cache/` und `temp/` sind in `.gitignore` und werden nicht versioniert
- Sensible Daten niemals committen (API Keys, etc.)
