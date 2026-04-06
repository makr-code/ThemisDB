# PostgreSQL Importer Plugin

**Stand:** 6. April 2026  
**Version:** 1.0.0  
**Kategorie:** Importers

---


**Version:** 1.0.0  
**Type:** Data Importer  
**Format:** pg_dump SQL

---

## 🎯 Übersicht

ThemisDB PostgreSQL Importer Plugin zum Importieren von PostgreSQL-Datenbanken via `pg_dump` SQL-Dumps.

### Features

- ✅ **pg_dump SQL-Format** - Direkter Import von PostgreSQL-Dumps
- ✅ **Schema-Parsing** - CREATE TABLE, CREATE SCHEMA
- ✅ **Daten-Import** - INSERT und COPY statements
- ✅ **Typ-Mapping** - Automatische Konvertierung PostgreSQL → ThemisDB
- ✅ **Batch-Processing** - Konfigurierbare Batch-Größe
- ✅ **Fortschritt-Tracking** - Progress-Callbacks
- ✅ **Dry-Run-Modus** - Validierung ohne Import
- ✅ **Filter** - Include/Exclude Tabellen

---

## 📦 Installation

### Als Plugin

```bash
# Plugin-Struktur
plugins/importers/postgres/
├── plugin.json                    # Manifest
├── plugin.json.sig                # Signatur
├── themis_import_postgres.dll     # Windows
├── themis_import_postgres.so      # Linux
└── themis_import_postgres.dylib   # macOS
```

### Plugin laden

```cpp
auto& pm = PluginManager::instance();
pm.scanPluginDirectory("./plugins");
auto* plugin = pm.loadPlugin("postgres_importer");
auto* importer = static_cast<IImporter*>(plugin->getInstance());
```

---

## 🚀 Verwendung

### Basis-Import

```cpp
#include "importers/importer_interface.h"

// Create importer
auto importer = /* get from plugin manager */;

// Import options
ImportOptions options;
options.batch_size = 1000;
options.default_namespace = "imported";
options.dry_run = false;

// Import data
auto stats = importer->importData("dump.sql", options);

// Check results
std::cout << "Imported: " << stats.imported_records << " records" << std::endl;
std::cout << "Failed: " << stats.failed_records << " records" << std::endl;
```

### Mit Progress-Callback

```cpp
ImportOptions options;

auto progress = [](const std::string& stage, size_t current, size_t total) {
    std::cout << stage << ": " << current << "/" << total << std::endl;
};

auto stats = importer->importData("dump.sql", options, progress);
```

---

## 📊 Typ-Mapping

| PostgreSQL | ThemisDB |
|------------|----------|
| INTEGER, INT, SERIAL | integer |
| BIGINT, BIGSERIAL | long |
| SMALLINT | integer |
| REAL, FLOAT | double |
| DOUBLE PRECISION | double |
| NUMERIC, DECIMAL | double |
| BOOLEAN, BOOL | boolean |
| CHAR, VARCHAR, TEXT | string |
| TIMESTAMP | datetime |
| DATE | date |
| TIME | time |
| JSON, JSONB | json |
| UUID | string |
| BYTEA | binary |

---

## 📁 pg_dump Format

### Unterstützte Statements

**DDL:**
```sql
CREATE SCHEMA app;
CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    username VARCHAR(50) NOT NULL,
    email VARCHAR(100),
    created_at TIMESTAMP DEFAULT NOW()
);
```

**DML:**
```sql
-- INSERT
INSERT INTO users (id, username, email) VALUES 
    (1, 'alice', 'alice@example.com'),
    (2, 'bob', 'bob@example.com');

-- COPY
COPY users (id, username, email) FROM stdin;
1	alice	alice@example.com
2	bob	bob@example.com
\.
```

### pg_dump erstellen

```bash
# SQL-Format (empfohlen)
pg_dump -d mydb -f dump.sql --inserts

# Mit COPY (schneller)
pg_dump -d mydb -f dump.sql

# Nur Schema
pg_dump -d mydb -f schema.sql --schema-only

# Nur Daten
pg_dump -d mydb -f data.sql --data-only
```

---

## 🔧 Konfiguration

### Import-Options

```cpp
struct ImportOptions {
    // General
    bool dry_run = false;
    bool continue_on_error = true;
    size_t batch_size = 1000;
    
    // Schema
    bool auto_create_schema = true;
    std::string default_namespace = "imported";
    
    // Data
    bool preserve_ids = false;
    bool update_existing = false;
    bool skip_duplicates = true;
    
    // Filtering
    std::vector<std::string> include_tables;
    std::vector<std::string> exclude_tables;
    std::vector<std::string> include_schemas;
    
    // Transformations
    std::map<std::string, std::string> column_mappings;
    std::map<std::string, std::string> table_mappings;
};
```

---

## 🚨 Einschränkungen

### Aktuell nicht unterstützt

- ❌ **Live-Connection** - Nur pg_dump Files
- ❌ **Komplexe SQL** - Nur Basic DDL/DML
- ❌ **Foreign Keys** - Werden ignoriert
- ❌ **Triggers** - Werden ignoriert
- ❌ **Functions** - Werden ignoriert
- ❌ **Custom Types** - Nur Standard-Typen

### Geplante Features (v2.0)

- [ ] Live PostgreSQL Connection (via libpq)
- [ ] Incremental Import (delta updates)
- [ ] Foreign Key Preservation
- [ ] Custom Type Mapping
- [ ] Parallel Processing
- [ ] Resume after failure

---

**Status:** ✅ Production-Ready  
**Getestet mit:** PostgreSQL 12, 13, 14, 15, 16  
**Dokumentation:** `docs/importers/POSTGRES_IMPORTER.md`
