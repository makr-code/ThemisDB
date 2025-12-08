# ThemisDB Projects

Dieses Verzeichnis enthält **eigenständige Projekte**, die unabhängig von ThemisDB entwickelt werden können.

## Verfügbare Projekte

### DocumentManager - Dokumentenverwaltung und RAG
**Pfad:** `include/projects/DocumentManager/`  
**Typ:** C++ Header-Only Bibliothek  
**Status:** Production

Dokumentenverwaltungssystem mit Unterstützung für:
- **Dokumenten-Upload** - Binäre und Textdokumente
- **Text-Extraktion** - Automatische Extraktion aus verschiedenen Formaten
- **Chunking** - Intelligentes Text-Chunking mit konfigurierbarem Overlap
- **Embeddings** - Integration mit Embedding-Generatoren
- **Graph-Konstruktion** - Automatische Verknüpfung von Dokumenten-Chunks
- **Vector Search** - Integration mit VectorIndexManager

**Nutzung:**
```cpp
#include "projects/DocumentManager/document_manager.h"

using namespace themis::projects;

// Assume storage, vector_index, and graph_index are already created
auto doc_manager = std::make_shared<DocumentManager>(
    storage, vector_index, graph_index
);

auto result = doc_manager->uploadDocument(
    blob, "text/plain", "example.txt"
);
```

**Dokumentation:** Siehe [include/projects/DocumentManager/README.md](../include/projects/DocumentManager/README.md)

---

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
