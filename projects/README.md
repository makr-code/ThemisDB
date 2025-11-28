# ThemisDB Projects

Dieses Verzeichnis enthält **eigenständige Projekte**, die unabhängig von ThemisDB entwickelt werden können.

## Verfügbare Projekte

### RESPO - RAG LLM Programmierhilfe
**Pfad:** `respo/`  
**Typ:** Eigenständiges Python-Projekt  
**Status:** Development

On-premise RAG-basierter LLM Coding Assistant:
- **Unabhängig** - Keine feste Abhängigkeit von ThemisDB
- **Pluggable Vector Stores** - ChromaDB, Qdrant, Weaviate, ThemisDB
- **vLLM** - Hochperformante LLM-Inferenz mit LoRA Support
- **Ohne Vendor-Login** - Vollständig lokale Ausführung
- **Air-Gapped Deployment** - Läuft komplett offline

**Features:**
- GitHub Scraper für Trainingsdaten-Sammlung
- LoRA Fine-Tuning für domänenspezifische Anpassung
- RAG Pipeline mit Hybrid Search
- CLI und REST API

**Nutzung:**
```bash
cd respo
pip install -e .
respo server --port 8080
```

**Dokumentation:** Siehe [respo/README.md](respo/README.md)

---

## Projekte vs. Adapters

| Aspekt | Projekte (`projects/`) | Adapters (`adapters/`) |
|--------|------------------------|------------------------|
| **Abhängigkeit** | Unabhängig | Benötigt ThemisDB |
| **Deployment** | Eigenständig | Teil von ThemisDB |
| **Repository** | Kann ausgelagert werden | Teil des Haupt-Repos |
| **Zielgruppe** | Endnutzer | ThemisDB-Integration |
