# TODO: Automatische Dokumentations-DB-Generierung beim Build

## Übersicht

Diese Datei beschreibt die notwendigen Schritte, um die automatische Generierung der Dokumentations-Datenbank beim Compilieren zu implementieren.

## Status: 🟡 In Bearbeitung

### ✅ Bereits implementiert

- [x] Python-Skript zur JSON-Dokumenten-Extraktion (`scripts/generate_docs_database.py`)
- [x] Python-Skript zur RocksDB-Generierung (`scripts/generate_docs_rocksdb.py`)
- [x] C++ DocsAssistant Klasse (`include/llm/docs_assistant.h`, `src/llm/docs_assistant.cpp`)
- [x] REST API Endpunkte (`/api/v1/llm/docs/{query,config,troubleshoot}`)
- [x] Dokumentation (EN/DE)
- [x] RocksDB Column Families: relational, graph_nodes, graph_edges, vector, metadata

### 🔴 Noch zu implementieren

## 1. Dokumente in `:document` Collection schreiben

**Priorität: HOCH**  
**Geschätzter Aufwand: 2-3 Stunden**

### Problem
Aktuell werden Dokumente in separate Collections geschrieben (`docs_relational`, `docs_graph_nodes`, etc.), aber nicht in die native ThemisDB `:document` Collection.

### Lösung
In `scripts/generate_docs_rocksdb.py` ergänzen:

```python
# Nach Zeile 72 einfügen:
aql_commands.append("""
-- Native :document Collection für volle ThemisDB-Integration
CREATE COLLECTION IF NOT EXISTS :document;
""")

# In der Dokument-Schleife (nach Zeile 101):
for doc in docs_data.get('documents', []):
    file_hash = doc.get('file_hash', '')
    file_name = doc.get('metadata', {}).get('file_name', 'unknown')
    text_content = doc.get('themis_metadata', {}).get('vector', {}).get('text_content', '')
    
    aql_commands.append(f"""
INSERT INTO :document (
    _key, 
    _id,
    type,
    title,
    content,
    source,
    metadata,
    created_at
) VALUES (
    '{file_hash}',
    ':document/{file_hash}',
    'documentation',
    '{file_name}',
    '{text_content.replace("'", "''")}',
    '{doc.get('file_path', '').replace("'", "''")}',
    '{json.dumps(doc.get('metadata', {})).replace("'", "''")}',
    '{doc.get('ingestion_time', '')}'
);
""")
```

### Dateien zu ändern
- `scripts/generate_docs_rocksdb.py` (Zeilen 70-160)

### Test
```bash
python3 scripts/generate_docs_rocksdb.py --method cli
# Prüfen ob :document Collection erstellt wird
```

---

## 2. AQL API Endpunkt als Einstiegspunkt

**Priorität: HOCH**  
**Geschätzter Aufwand: 3-4 Stunden**

### Warum sinnvoll?
✅ **JA, ein AQL API Call macht definitiv Sinn als Einstiegspunkt!**

Vorteile:
1. **Native Integration**: Nutzer können direkt AQL verwenden, ohne REST API
2. **Flexibilität**: Komplexe Abfragen möglich (JOINs, Aggregationen)
3. **Konsistenz**: Passt zum ThemisDB-Ökosystem
4. **Performance**: Direkter Zugriff auf RocksDB ohne HTTP-Overhead
5. **Batch-Operationen**: Mehrere Dokumente auf einmal abfragen

### Implementierung

#### 2.1 AQL-Funktionen registrieren

**Datei**: `src/aql/llm_aql_handler.cpp` (erweitern oder neu erstellen)

```cpp
#include "aql/llm_aql_handler.h"
#include "llm/docs_assistant.h"

namespace themis::aql {

// Registriere AQL-Funktionen für Dokumentations-Queries
void registerDocsAssistantFunctions(AQLContext& ctx) {
    
    // 1. DOCS_QUERY(query_string) -> string
    ctx.registerFunction("DOCS_QUERY", [](const std::vector<AQLValue>& args) -> AQLValue {
        if (args.empty() || !args[0].isString()) {
            throw AQLError("DOCS_QUERY requires a string argument");
        }
        
        static themis::llm::DocsAssistant assistant;
        static bool initialized = false;
        
        if (!initialized) {
            themis::llm::DocsAssistantConfig config;
            config.docs_database_path = "data/docs.db";
            config.database_type = "rocksdb";
            assistant = themis::llm::DocsAssistant(config);
            assistant.loadDatabase();
            initialized = true;
        }
        
        auto result = assistant.query(args[0].getString());
        return AQLValue(result.generated_answer);
    });
    
    // 2. DOCS_SEARCH(query_string, limit) -> array<object>
    ctx.registerFunction("DOCS_SEARCH", [](const std::vector<AQLValue>& args) -> AQLValue {
        if (args.empty() || !args[0].isString()) {
            throw AQLError("DOCS_SEARCH requires a string argument");
        }
        
        int limit = args.size() > 1 ? args[1].getInt() : 5;
        
        static themis::llm::DocsAssistant assistant;
        // ... (wie oben)
        
        auto docs = assistant.searchDocs(args[0].getString(), limit);
        
        // Konvertiere zu AQL Array
        std::vector<AQLValue> results;
        for (const auto& doc : docs) {
            AQLObject obj;
            obj["file_name"] = doc.file_name;
            obj["relevance_score"] = doc.relevance_score;
            obj["content_preview"] = doc.text_content.substr(0, 200);
            results.push_back(AQLValue(obj));
        }
        
        return AQLValue(results);
    });
    
    // 3. DOCS_CONFIG_HELP(topic) -> string
    ctx.registerFunction("DOCS_CONFIG_HELP", [](const std::vector<AQLValue>& args) -> AQLValue {
        if (args.empty() || !args[0].isString()) {
            throw AQLError("DOCS_CONFIG_HELP requires a string argument");
        }
        
        static themis::llm::DocsAssistant assistant;
        // ... (initialisieren wie oben)
        
        auto result = assistant.getConfigHelp(args[0].getString());
        return AQLValue(result.generated_answer);
    });
    
    // 4. DOCS_TROUBLESHOOT(error) -> string
    ctx.registerFunction("DOCS_TROUBLESHOOT", [](const std::vector<AQLValue>& args) -> AQLValue {
        if (args.empty() || !args[0].isString()) {
            throw AQLError("DOCS_TROUBLESHOOT requires a string argument");
        }
        
        static themis::llm::DocsAssistant assistant;
        // ... (initialisieren wie oben)
        
        auto result = assistant.getTroubleshootingHelp(args[0].getString());
        return AQLValue(result.generated_answer);
    });
}

} // namespace themis::aql
```

#### 2.2 Neue Datei für AQL Handler Header

**Datei**: `include/aql/docs_assistant_functions.h` (neu erstellen)

```cpp
#pragma once

#include "aql/aql_context.h"

namespace themis::aql {

/**
 * @brief Register documentation assistant functions in AQL
 * 
 * Available functions:
 * - DOCS_QUERY(query: string) -> string
 * - DOCS_SEARCH(query: string, limit: int = 5) -> array<object>
 * - DOCS_CONFIG_HELP(topic: string) -> string
 * - DOCS_TROUBLESHOOT(error: string) -> string
 */
void registerDocsAssistantFunctions(AQLContext& ctx);

} // namespace themis::aql
```

#### 2.3 AQL Verwendungsbeispiele

**Datei**: `docs/en/features/DOCS_AQL_API.md` (neu erstellen)

```sql
-- Beispiel 1: Einfache Dokumentationsabfrage
SELECT DOCS_QUERY('How do I enable sharding?') AS answer;

-- Beispiel 2: Dokumentensuche mit Limit
SELECT DOCS_SEARCH('RAID configuration', 10) AS relevant_docs;

-- Beispiel 3: Konfigurationshilfe
SELECT DOCS_CONFIG_HELP('security') AS config_guide;

-- Beispiel 4: Fehlerbehebung
SELECT DOCS_TROUBLESHOOT('Server hangs at startup') AS solution;

-- Beispiel 5: Kombinierte Abfrage (Suche + LLM-Antwort)
LET docs = DOCS_SEARCH('vector embeddings', 5)
LET answer = DOCS_QUERY('How to use vector embeddings?')
RETURN {
    answer: answer,
    sources: docs
};

-- Beispiel 6: Batch-Abfragen
FOR topic IN ['sharding', 'replication', 'security']
    RETURN {
        topic: topic,
        help: DOCS_CONFIG_HELP(topic)
    };

-- Beispiel 7: Integration mit bestehenden Daten
FOR doc IN :document
    FILTER doc.type == 'documentation'
    LET summary = DOCS_QUERY(CONCAT('Summarize: ', doc.title))
    RETURN {
        title: doc.title,
        summary: summary
    };
```

### Dateien zu erstellen/ändern
- `include/aql/docs_assistant_functions.h` (neu)
- `src/aql/docs_assistant_functions.cpp` (neu)
- `src/aql/aql_context.cpp` (erweitern - registerDocsAssistantFunctions() aufrufen)
- `docs/en/features/DOCS_AQL_API.md` (neu)

### Test
```bash
# Über CLI
echo "SELECT DOCS_QUERY('How to configure RAID?') AS answer;" | ./build/themis_cli --database data/docs.db

# Erwartete Ausgabe:
# {
#   "answer": "To configure RAID in ThemisDB, set THEMIS_ENABLE_SHARDING=true..."
# }
```

---

## 3. CMake Build-Integration

**Priorität: MITTEL-HOCH**  
**Geschätzter Aufwand: 2-3 Stunden**

### Implementierung

**Datei**: `CMakeLists.txt` (root)

```cmake
# Nach Zeile 98 (add_subdirectory(cmake)) einfügen:

# ============================================================================
# DOCUMENTATION DATABASE GENERATION
# ============================================================================

if(THEMIS_ENABLE_LLM)
    # Find Python3 for documentation database generation
    find_package(Python3 COMPONENTS Interpreter REQUIRED)
    
    # Documentation database generation target
    set(DOCS_DB_OUTPUT "${CMAKE_BINARY_DIR}/data/docs.db")
    set(DOCS_JSON_OUTPUT "${CMAKE_BINARY_DIR}/data/docs_database.json")
    
    # Generate JSON documentation database
    add_custom_command(
        OUTPUT ${DOCS_JSON_OUTPUT}
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_SOURCE_DIR}/scripts/generate_docs_database.py
                --output ${DOCS_JSON_OUTPUT}
        DEPENDS
            ${CMAKE_SOURCE_DIR}/docs
            ${CMAKE_SOURCE_DIR}/compendium
            ${CMAKE_SOURCE_DIR}/scripts/generate_docs_database.py
            ${CMAKE_SOURCE_DIR}/tools/ingest.py
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating documentation JSON database (1151 documents)..."
        VERBATIM
    )
    
    # Generate C++ RocksDB importer
    set(DOCS_IMPORTER_CPP "${CMAKE_BINARY_DIR}/data/import_docs_rocksdb.cpp")
    add_custom_command(
        OUTPUT ${DOCS_IMPORTER_CPP}
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_SOURCE_DIR}/scripts/generate_docs_rocksdb.py
                --method cpp
                --output ${DOCS_DB_OUTPUT}
        DEPENDS
            ${DOCS_JSON_OUTPUT}
            ${CMAKE_SOURCE_DIR}/scripts/generate_docs_rocksdb.py
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating RocksDB importer C++ code..."
        VERBATIM
    )
    
    # Build RocksDB importer executable
    add_executable(import_docs_rocksdb ${DOCS_IMPORTER_CPP})
    target_link_libraries(import_docs_rocksdb
        PRIVATE
            RocksDB::rocksdb
            nlohmann_json::nlohmann_json
    )
    
    # Import documentation into RocksDB
    add_custom_command(
        OUTPUT ${DOCS_DB_OUTPUT}
        COMMAND import_docs_rocksdb ${DOCS_JSON_OUTPUT} ${DOCS_DB_OUTPUT}
        DEPENDS
            import_docs_rocksdb
            ${DOCS_JSON_OUTPUT}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMENT "Importing documentation into RocksDB (relational, graph, vector, metadata)..."
        VERBATIM
    )
    
    # Create top-level target
    add_custom_target(docs_database
        DEPENDS ${DOCS_DB_OUTPUT}
    )
    
    # Make themis_server depend on docs_database
    add_dependencies(themis_server docs_database)
    
    # Install documentation database
    install(
        DIRECTORY ${DOCS_DB_OUTPUT}
        DESTINATION ${CMAKE_INSTALL_PREFIX}/data
        COMPONENT documentation
    )
    
    message(STATUS "Documentation database generation enabled")
    message(STATUS "  Output: ${DOCS_DB_OUTPUT}")
    message(STATUS "  Size: ~2-3 MB (1151 documents)")
endif()
```

### Dateien zu ändern
- `CMakeLists.txt` (root, Zeile ~98)

### Test
```bash
# Clean build
rm -rf build
cmake -B build -DTHEMIS_ENABLE_LLM=ON
cmake --build build --target docs_database

# Sollte ausgeben:
# -- Generating documentation JSON database (1151 documents)...
# -- Generating RocksDB importer C++ code...
# -- Building import_docs_rocksdb executable...
# -- Importing documentation into RocksDB...
# -- Documentation database created: build/data/docs.db

# Prüfen
ls -lh build/data/docs.db
# Erwartete Größe: 2-3 MB
```

---

## 4. Release Packaging Integration

**Priorität: MITTEL**  
**Geschätzter Aufwand: 1-2 Stunden**

### Implementierung

**Datei**: `.github/workflows/04-release_build-binary-linux.yml`

```bash
# Nach dem Build, vor dem Packaging einfügen:

echo "Building documentation database..."
cmake --build build --target docs_database

# Prüfe ob Datenbank existiert
if [ ! -d "build/data/docs.db" ]; then
    echo "ERROR: Documentation database not found!"
    exit 1
fi

echo "Documentation database size:"
du -h build/data/docs.db

# Beim Packaging in Release-Archiv aufnehmen
mkdir -p themisdb-${VERSION}-${PLATFORM}/data
cp -r build/data/docs.db themisdb-${VERSION}-${PLATFORM}/data/

echo "✓ Documentation database included in release package"
```

**Datei**: `Dockerfile.themis-server`

```dockerfile
# Nach dem Build-Stage
FROM builder AS docs-builder

# Generiere Dokumentations-Datenbank
RUN python3 scripts/generate_docs_database.py --output /build/data/docs_database.json
RUN python3 scripts/generate_docs_rocksdb.py --method cpp --output /build/data/docs.db
RUN g++ -std=c++17 /build/data/import_docs_rocksdb.cpp \
    -o /build/import_docs_rocksdb \
    -lrocksdb -lpthread
RUN /build/import_docs_rocksdb /build/data/docs_database.json /build/data/docs.db

# Final stage
FROM ubuntu:24.04

# Kopiere Dokumentations-Datenbank
COPY --from=docs-builder /build/data/docs.db /app/data/docs.db

# Umgebungsvariablen
ENV THEMIS_DOCS_DATABASE_PATH=/app/data/docs.db
ENV THEMIS_DOCS_DATABASE_TYPE=rocksdb
ENV THEMIS_ENABLE_DOCS_ASSISTANT=true
```

### Dateien zu ändern
- `.github/workflows/04-release_build-binary-linux.yml`
- `Dockerfile.themis-server`
- `docker/compose/docker-compose.yml` (Volume mapping für docs.db)

---

## 5. Unit Tests

**Priorität: MITTEL**  
**Geschätzter Aufwand: 4-5 Stunden**

### Tests zu erstellen

#### 5.1 RocksDB Import Test

**Datei**: `tests/test_docs_rocksdb_import.cpp`

```cpp
#include <gtest/gtest.h>
#include "llm/docs_assistant.h"
#include <filesystem>

class DocsRocksDBTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_db_path = "/tmp/test_docs.db";
        // Generiere Test-Datenbank
        std::system("python3 scripts/generate_docs_rocksdb.py --output /tmp/test_docs.db");
    }
    
    void TearDown() override {
        std::filesystem::remove_all(test_db_path);
    }
    
    std::string test_db_path;
};

TEST_F(DocsRocksDBTest, DatabaseExists) {
    EXPECT_TRUE(std::filesystem::exists(test_db_path));
}

TEST_F(DocsRocksDBTest, LoadDatabase) {
    themis::llm::DocsAssistantConfig config;
    config.docs_database_path = test_db_path;
    config.database_type = "rocksdb";
    
    themis::llm::DocsAssistant assistant(config);
    EXPECT_TRUE(assistant.loadDatabase());
    EXPECT_TRUE(assistant.isReady());
}

TEST_F(DocsRocksDBTest, QueryDocumentation) {
    themis::llm::DocsAssistantConfig config;
    config.docs_database_path = test_db_path;
    config.database_type = "rocksdb";
    
    themis::llm::DocsAssistant assistant(config);
    assistant.loadDatabase();
    
    auto result = assistant.query("How to configure sharding?");
    
    EXPECT_FALSE(result.generated_answer.empty());
    EXPECT_GT(result.confidence_score, 0.0f);
    EXPECT_GT(result.docs_included_in_context, 0);
}
```

#### 5.2 AQL Functions Test

**Datei**: `tests/test_docs_aql_functions.cpp`

```cpp
#include <gtest/gtest.h>
#include "aql/docs_assistant_functions.h"
#include "aql/aql_context.h"

TEST(DocsAQLTest, DocsQueryFunction) {
    themis::aql::AQLContext ctx;
    themis::aql::registerDocsAssistantFunctions(ctx);
    
    auto result = ctx.executeQuery("SELECT DOCS_QUERY('test') AS answer");
    EXPECT_FALSE(result.empty());
}

TEST(DocsAQLTest, DocsSearchFunction) {
    themis::aql::AQLContext ctx;
    themis::aql::registerDocsAssistantFunctions(ctx);
    
    auto result = ctx.executeQuery("SELECT DOCS_SEARCH('sharding', 5) AS docs");
    EXPECT_FALSE(result.empty());
}
```

### Dateien zu erstellen
- `tests/test_docs_rocksdb_import.cpp`
- `tests/test_docs_aql_functions.cpp`
- `tests/CMakeLists.txt` (erweitern mit neuen Tests)

---

## 6. Performance-Benchmarks

**Priorität: NIEDRIG**  
**Geschätzter Aufwand: 2-3 Stunden**

### Benchmark zu erstellen

**Datei**: `benchmarks/bench_docs_assistant.cpp`

```cpp
#include <benchmark/benchmark.h>
#include "llm/docs_assistant.h"

static void BM_DocsAssistant_LoadDatabase(benchmark::State& state) {
    for (auto _ : state) {
        themis::llm::DocsAssistantConfig config;
        config.docs_database_path = "data/docs.db";
        config.database_type = "rocksdb";
        
        themis::llm::DocsAssistant assistant(config);
        assistant.loadDatabase();
    }
}
BENCHMARK(BM_DocsAssistant_LoadDatabase);

static void BM_DocsAssistant_SearchDocs(benchmark::State& state) {
    themis::llm::DocsAssistant assistant;
    // ... setup
    
    for (auto _ : state) {
        auto docs = assistant.searchDocs("sharding", 5);
    }
}
BENCHMARK(BM_DocsAssistant_SearchDocs);

BENCHMARK_MAIN();
```

---

## Zusammenfassung der Arbeitsschritte

### Phase 1: Kritische Funktionalität (Woche 1)
1. ✅ **:document Collection Integration** (2-3h) - **COMPLETED**
2. ✅ **AQL API Endpunkte** (3-4h) - **COMPLETED**
3. ✅ **CMake Build-Integration** (2-3h) - **COMPLETED**

**Geschätzter Gesamtaufwand Phase 1: 7-10 Stunden** - ✅ **ABGESCHLOSSEN**

### Phase 2: Release-Integration (Woche 2)
4. 🟡 **Release Packaging** (1-2h) - **IN PROGRESS**
5. ✅ **Unit Tests** (4-5h) - **COMPLETED**

**Geschätzter Gesamtaufwand Phase 2: 5-7 Stunden** - 🟡 **FAST ABGESCHLOSSEN**

### Phase 3: Optimierung (Optional)
6. 🟡 **Performance-Benchmarks** (2-3h) - **OPTIONAL**
7. 🟡 **Dokumentation vervollständigen** (1-2h) - **OPTIONAL**

**Gesamtaufwand: 15-22 Stunden** - ✅ **90% ABGESCHLOSSEN**

---

## Prioritätenliste

1. ✅ **KRITISCH**: :document Collection Integration - **COMPLETED 2026-01-11**
2. ✅ **KRITISCH**: AQL API als Einstiegspunkt - **COMPLETED 2026-01-11**
3. ✅ **HOCH**: CMake Build-Integration - **COMPLETED 2026-01-11**
4. 🟡 **MITTEL**: Release Packaging - **IN PROGRESS**
5. ✅ **NIEDRIG**: Unit Tests - **COMPLETED 2026-01-11**
6. 🟢 **NIEDRIG**: Benchmarks - **OPTIONAL**

---

## Nächste Schritte

```bash
# 1. :document Collection implementieren
vim scripts/generate_docs_rocksdb.py

# 2. AQL API implementieren
vim include/aql/docs_assistant_functions.h
vim src/aql/docs_assistant_functions.cpp

# 3. CMake integrieren
vim CMakeLists.txt

# 4. Testen
cmake -B build -DTHEMIS_ENABLE_LLM=ON
cmake --build build --target docs_database
./build/themis_cli --execute "SELECT DOCS_QUERY('test') AS answer;"
```

---

## Kontakt

Bei Fragen oder Problemen:
- GitHub Issues: https://github.com/makr-code/ThemisDB/issues
- Pull Request: https://github.com/makr-code/ThemisDB/pull/XXX
