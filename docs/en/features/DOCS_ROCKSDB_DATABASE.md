# ThemisDB Documentation RocksDB Database

## Überblick

Diese vorkompilierte RocksDB-Datenbank enthält die gesamte ThemisDB-Dokumentation aus `./docs` und `./compendium` und ist über alle ThemisDB-Datenmodelle zugänglich:

- **Relational**: Dokumententabellen für SQL-ähnliche Abfragen
- **Graph**: Dokumenten-Knoten und Beziehungen für Graph-Traversierung
- **Vector**: Dokumenten-Embeddings für semantische Suche
- **Metadata**: Datenbank-Metadaten und Statistiken

## Struktur

### Column Families

Die RocksDB-Datenbank verwendet folgende Column Families:

1. **`relational`**: Relationaler Dokumenten-Store
   - Key: `doc:<file_hash>`
   - Value: JSON mit `{id, file_name, file_path, content_type, text_content, created_at}`

2. **`graph_nodes`**: Graph-Dokumenten-Knoten
   - Key: `node:<file_hash>`
   - Value: JSON mit `{_id, _key, type, name, hash}`

3. **`graph_edges`**: Graph-Beziehungen zwischen Dokumenten
   - Key: `edge:<from_hash>:<to_hash>`
   - Value: JSON mit `{_from, _to, type, weight}`

4. **`vector`**: Vektor-Embeddings für semantische Suche
   - Key: `vec:<file_hash>`
   - Value: JSON mit `{id, document_hash, text_chunk, embedding, embedding_pending}`

5. **`metadata`**: Datenbank-Metadaten
   - Key: `<metadata_key>` (z.B. `version`, `generation_time`, `total_documents`)
   - Value: String

## Generierung

### Voraussetzungen

```bash
# Python-Abhängigkeiten
pip install pyyaml tqdm

# C++ Build-Abhängigkeiten
sudo apt-get install librocksdb-dev nlohmann-json3-dev
```

### Methode 1: C++ Direct Writer (Empfohlen)

Der C++ Direct Writer schreibt direkt in RocksDB ohne CLI:

```bash
# 1. Generiere C++ Importer
python3 scripts/generate_docs_rocksdb.py --method cpp --output data/docs.db

# 2. Kompiliere den Importer
g++ -std=c++17 data/import_docs_rocksdb.cpp -o import_docs_rocksdb \
    -lrocksdb -lpthread -I/path/to/json/include

# 3. Führe den Import aus
./import_docs_rocksdb /tmp/docs_database_temp.json data/docs.db

# Ausgabe:
# ======================================================
# ThemisDB Documentation RocksDB Import
# ======================================================
# JSON file: /tmp/docs_database_temp.json
# Database path: data/docs.db
# 
# Importing relational data...
#   ✓ Imported 1151 relational records
# Importing graph nodes...
#   ✓ Imported 1151 graph nodes
# Importing graph edges...
#   ✓ Graph edges (placeholder)
# Importing vector data...
#   ✓ Imported 1151 vector entries
# Importing metadata...
#   ✓ Imported metadata
# ✓ Documentation database created successfully!
# Location: data/docs.db
```

### Methode 2: ThemisDB CLI (Alternative)

Verwende die ThemisDB CLI für den Import:

```bash
# 1. Generiere Import-Skript
python3 scripts/generate_docs_rocksdb.py --method cli --output data/docs.db

# 2. Führe das Skript aus
data/import_docs_to_rocksdb.sh data/docs.db

# Oder mit benutzerdefiniertem CLI-Pfad:
THEMIS_CLI=./build/themis_cli data/import_docs_to_rocksdb.sh data/docs.db
```

## Verwendung

### Abfragen via ThemisDB API

#### Relational (SQL-ähnlich)

```cpp
#include "storage/rocksdb_wrapper.h"

// Öffne die Dokumentationsdatenbank
RocksDBWrapper docs_db("data/docs.db");

// Lese ein Dokument
std::string doc_json;
auto status = docs_db.get("relational", "doc:<file_hash>", &doc_json);

if (status.ok()) {
    auto doc = json::parse(doc_json);
    std::cout << "File: " << doc["file_name"] << "\n";
    std::cout << "Content: " << doc["text_content"] << "\n";
}
```

#### Graph Traversierung

```cpp
// Lese einen Graph-Knoten
std::string node_json;
docs_db.get("graph_nodes", "node:<file_hash>", &node_json);

auto node = json::parse(node_json);
std::cout << "Document: " << node["name"] << "\n";
std::cout << "Type: " << node["type"] << "\n";
```

#### Vektor-Suche

```cpp
// Lese Vektor-Eintrag
std::string vec_json;
docs_db.get("vector", "vec:<file_hash>", &vec_json);

auto vec = json::parse(vec_json);
if (vec["embedding_pending"]) {
    // Embedding muss noch generiert werden
    std::string text = vec["text_chunk"];
    auto embedding = generate_embedding(text);
    vec["embedding"] = embedding;
    vec["embedding_pending"] = false;
    docs_db.put("vector", "vec:<file_hash>", vec.dump());
}
```

### Integration mit DocsAssistant

Der `DocsAssistant` kann direkt die RocksDB-Datenbank verwenden:

```cpp
#include "llm/docs_assistant.h"

// Konfiguration für RocksDB-Backend
themis::llm::DocsAssistantConfig config;
config.docs_database_path = "data/docs.db";
config.database_type = "rocksdb";  // Anstatt "json"

themis::llm::DocsAssistant assistant(config);

if (!assistant.loadDatabase()) {
    std::cerr << "Fehler beim Laden der RocksDB-Datenbank\n";
    return;
}

// Abfrage
auto result = assistant.query("Wie konfiguriere ich RAID Sharding?");
std::cout << result.generated_answer << "\n";
```

## Größe und Performance

### Vergleich: JSON vs. RocksDB

| Metrik | JSON | RocksDB | Vorteil |
|--------|------|---------|---------|
| **Größe (unkomprimiert)** | 4.2 MB | ~2-3 MB | RocksDB: -30-40% |
| **Größe (komprimiert)** | 1.2 MB (gzip) | ~800 KB | RocksDB: -33% |
| **Ladezeit** | 10-50 ms | 5-10 ms | RocksDB: 2-5x schneller |
| **Abfragezeit** | O(n) linear | O(log n) | RocksDB: logarithmisch |
| **Speicher-Footprint** | 4.2 MB (vollständig im RAM) | ~500 KB (cache) | RocksDB: -88% |
| **Multi-Modell Support** | Nein | Ja | RocksDB unterstützt alle Modelle |

### Benchmarks

```bash
# JSON-basierte Suche
Zeit: ~15-30 ms (linear search durch 1151 Dokumente)

# RocksDB-basierte Suche
Zeit: ~2-5 ms (logarithmischer Index-Lookup)

# Geschwindigkeitsvorteil: 3-15x schneller
```

## Release-Integration

### Automatische Generierung beim Build

Die Dokumentationsdatenbank wird automatisch beim Build generiert:

```cmake
# In CMakeLists.txt
add_custom_target(docs_database
    COMMAND ${PYTHON_EXECUTABLE} ${CMAKE_SOURCE_DIR}/scripts/generate_docs_rocksdb.py
            --output ${CMAKE_BINARY_DIR}/data/docs.db
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Generating documentation RocksDB database..."
)

# Abhängigkeit für themis_server
add_dependencies(themis_server docs_database)
```

### Packaging

Die RocksDB-Datenbank wird in Release-Pakete aufgenommen:

```bash
# Linux .tar.gz
themisdb-v1.4.0-linux-x64/
├── bin/
│   └── themis_server
├── data/
│   └── docs.db/          # RocksDB-Datenbank (2-3 MB)
├── config/
└── docs/

# Windows .zip
themisdb-v1.4.0-windows-x64/
├── bin/
│   └── themis_server.exe
├── data/
│   └── docs.db/          # RocksDB-Datenbank
└── ...

# Docker Image
FROM ubuntu:24.04
...
COPY data/docs.db /app/data/docs.db
```

## Wartung

### Aktualisierung der Datenbank

Nach Änderungen an der Dokumentation:

```bash
# Regeneriere die Datenbank
python3 scripts/generate_docs_rocksdb.py --output data/docs.db

# Oder nur JSON-Intermediate
python3 scripts/generate_docs_database.py --output data/docs_database.json
```

### Backup

```bash
# Backup der RocksDB-Datenbank
tar -czf docs_db_backup_$(date +%Y%m%d).tar.gz data/docs.db/

# Restore
tar -xzf docs_db_backup_20260111.tar.gz -C /
```

### Komprimierung für Release

```bash
# Komprimiere für Release-Paket
tar -czf docs.db.tar.gz data/docs.db/

# Größe: ~800 KB (komprimiert) vs. 2-3 MB (unkomprimiert)
```

## Fehlerbehebung

### Problem: RocksDB-Datenbank nicht gefunden

```bash
# Lösung: Generiere die Datenbank neu
python3 scripts/generate_docs_rocksdb.py
```

### Problem: Korrupte Datenbank

```bash
# Lösung: Lösche und regeneriere
rm -rf data/docs.db
python3 scripts/generate_docs_rocksdb.py
```

### Problem: Veraltete Dokumentation

```bash
# Lösung: Aktualisiere docs/ und compendium/, dann regeneriere
git pull origin main
python3 scripts/generate_docs_rocksdb.py
```

## Entwicklung

### Erweiterte Column Families

Um zusätzliche Column Families hinzuzufügen:

1. Bearbeite `scripts/generate_docs_rocksdb.py`
2. Füge neue CF in `openDatabase()` hinzu
3. Implementiere `importCustomData()` Methode
4. Regeneriere die Datenbank

### Custom Indexierung

```cpp
// Beispiel: Volltextindex für schnelle Suche
void importFullTextIndex(const json& docs_data) {
    auto* cf = cf_handles_[6];  // fulltext CF
    
    for (const auto& doc : docs_data["documents"]) {
        std::string text = doc["themis_metadata"]["vector"]["text_content"];
        
        // Tokenize und indexiere
        auto tokens = tokenize(text);
        for (const auto& token : tokens) {
            std::string key = "token:" + token + ":" + doc["file_hash"].get<std::string>();
            db_->Put(rocksdb::WriteOptions(), cf, key, "1");
        }
    }
}
```

## Lizenz

Gleiche Lizenz wie ThemisDB - MIT

## Support

- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Dokumentation: `docs/en/features/DOCS_ASSISTANT.md`
- Release Notes: `CHANGELOG.md`
