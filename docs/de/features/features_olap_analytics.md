---
category: "📊 Data Features"
version: "v1.3.0"
status: "✅"
date: "22.12.2025"
---

# 📊 OLAP Analytics

Online Analytical Processing für Business Intelligence und Datenanalyse mit Aggregationen und Grouping Operators.

## 📋 Inhaltsverzeichnis

- [📋 Übersicht](#-übersicht)
- [✨ Features](#-features)
- [🚀 Schnellstart](#-schnellstart)
- [📖 Detaillierte Dokumentation](#-detaillierte-dokumentation)
- [💡 Best Practices](#-best-practices)
- [🔧 Troubleshooting](#-troubleshooting)
- [📚 Siehe auch](#-siehe-auch)
- [📝 Changelog](#-changelog)

## 📋 Übersicht

ThemisDB unterstützt fortgeschrittene OLAP (Online Analytical Processing) Features für Business Intelligence und Datenanalyse.

## Features

### ✅ Implementiert

- **Aggregations-Funktionen**
  - COUNT, SUM, AVG, MIN, MAX
  - STDDEV, VARIANCE
  - MEDIAN, PERCENTILE
  - COUNT_DISTINCT
  - FIRST, LAST

- **Grouping Operators**
  - Simple GROUP BY
  - CUBE (alle Kombinationen)
  - ROLLUP (hierarchisch)
  - GROUPING SETS (benutzerdefiniert)

- **Window Functions**
  - PARTITION BY
  - ORDER BY
  - ROWS PRECEDING/FOLLOWING

- **Columnar Store**
  - Spaltenorientierte Speicherung
  - Vektorisierte Aggregationen
  - Column Statistics

- **Materialized Views**
  - Pre-computed Aggregations
  - Manual/Periodic Refresh
  - Incremental Updates (geplant)

## Verwendung

### OLAPQuery erstellen

```cpp
#include "analytics/olap.h"

using namespace themis::analytics;

OLAPQuery query;
query.collection = "sales";

// Dimensionen
query.dimensions.push_back({"region", "", true});
query.dimensions.push_back({"product", "", true});

// Measures
query.measures.push_back({"total_sales", "amount", Measure::Function::Sum});
query.measures.push_back({"avg_sales", "amount", Measure::Function::Avg});
query.measures.push_back({"order_count", "id", Measure::Function::Count});

// Filter
Filter filter;
filter.field = "year";
filter.op = Filter::Operator::Eq;
filter.value = int64_t(2024);
query.filters.push_back(filter);

// Sortierung
query.sorts.push_back({"total_sales", false, false});  // DESC

// Pagination
query.limit = 100;
query.offset = 0;
```

### Query ausführen

```cpp
OLAPEngine engine;
auto result = engine.execute(query);

std::cout << "Rows: " << result.total_rows << std::endl;
std::cout << "Execution time: " << result.execution_time_ms << " ms" << std::endl;

for (const auto& row : result.rows) {
    auto region = std::get<std::string>(row.values.at("region"));
    auto total = std::get<double>(row.values.at("total_sales"));
    std::cout << region << ": " << total << std::endl;
}
```

### CUBE Query

CUBE generiert alle möglichen Gruppierungskombinationen:

```cpp
OLAPQuery query;
query.collection = "sales";
query.grouping_mode = OLAPQuery::GroupingMode::Cube;

query.dimensions.push_back({"region", "", true});
query.dimensions.push_back({"product", "", true});
query.dimensions.push_back({"year", "", true});

query.measures.push_back({"total", "amount", Measure::Function::Sum});

// Generiert:
// - (region, product, year) - Detail
// - (region, product)       - year aggregiert
// - (region, year)          - product aggregiert
// - (product, year)         - region aggregiert
// - (region)                - product, year aggregiert
// - (product)               - region, year aggregiert
// - (year)                  - region, product aggregiert
// - ()                      - Grand Total

auto cells = engine.executeCube("sales", query.dimensions, query.measures);

for (const auto& cell : cells) {
    std::cout << "Grouping ID: " << cell.grouping_id << std::endl;
    for (const auto& [dim, value] : cell.dimensions) {
        if (value) {
            std::cout << "  " << dim << ": " << *value << std::endl;
        } else {
            std::cout << "  " << dim << ": (ALL)" << std::endl;
        }
    }
    std::cout << "  Total: " << cell.measures.at("total") << std::endl;
}
```

### ROLLUP Query

ROLLUP generiert hierarchische Aggregationen:

```cpp
OLAPQuery query;
query.collection = "sales";
query.grouping_mode = OLAPQuery::GroupingMode::Rollup;

// Hierarchie: Jahr > Quartal > Monat
query.dimensions.push_back({"year", "", true});
query.dimensions.push_back({"quarter", "", true});
query.dimensions.push_back({"month", "", true});

query.measures.push_back({"total", "amount", Measure::Function::Sum});

// Generiert:
// - (year, quarter, month) - Detail
// - (year, quarter)        - Monatssummen
// - (year)                 - Quartalssummen
// - ()                     - Grand Total

auto rows = engine.executeRollup("sales", query.dimensions, query.measures);

for (const auto& row : rows) {
    std::cout << "Level: " << row.level << std::endl;
    // Level 0 = Detail, höhere Level = Subtotals
}
```

### GROUPING SETS

Benutzerdefinierte Gruppierungssätze:

```cpp
OLAPQuery query;
query.collection = "sales";
query.grouping_mode = OLAPQuery::GroupingMode::GroupingSets;

query.dimensions.push_back({"region", "", true});
query.dimensions.push_back({"product", "", true});
query.dimensions.push_back({"year", "", true});

// Spezifische Kombinationen
query.grouping_sets.push_back({{"region", "product"}});
query.grouping_sets.push_back({{"region", "year"}});
query.grouping_sets.push_back({{"product"}});

auto result = engine.execute(query);
```

### Window Functions

```cpp
OLAPQuery::WindowSpec window;
window.name = "rolling_avg";
window.partition_by = {"region"};
window.order_by.push_back({"date", true, false});  // ASC
window.rows_preceding = 2;  // 3-Tage gleitender Durchschnitt
window.rows_following = 0;

query.windows.push_back(window);

std::vector<Measure> measures;
measures.push_back({"avg_sales", "amount", Measure::Function::Avg});

std::vector<std::unordered_map<std::string, double>> data = {
    {{"region", 1.0}, {"date", 1.0}, {"amount", 100.0}},
    {{"region", 1.0}, {"date", 2.0}, {"amount", 150.0}},
    {{"region", 1.0}, {"date", 3.0}, {"amount", 200.0}},
    // ...
};

auto windowResults = engine.evaluateWindowFunctions(data, measures, window);
```

### Query Plan

```cpp
auto plan = engine.explain(query);

std::cout << "Estimated rows: " << plan.estimated_rows << std::endl;
std::cout << "Estimated cost: " << plan.estimated_cost << std::endl;

for (const auto& note : plan.optimization_notes) {
    std::cout << "- " << note << std::endl;
}
```

## Columnar Store

### Grundlagen

```cpp
ColumnarStore store;

// Spalten erstellen
store.createColumn("id", "string");
store.createColumn("region", "string");
store.createColumn("amount", "double");

// Daten einfügen
using Value = std::variant<std::nullptr_t, bool, int64_t, double, std::string>;

std::vector<std::unordered_map<std::string, Value>> rows = {
    {{"id", std::string("1")}, {"region", std::string("North")}, {"amount", 100.0}},
    {{"id", std::string("2")}, {"region", std::string("South")}, {"amount", 200.0}},
    {{"id", std::string("3")}, {"region", std::string("North")}, {"amount", 150.0}}
};

store.appendRows(rows);

// Aggregationen
std::cout << "Sum: " << store.sum("amount") << std::endl;
std::cout << "Avg: " << store.avg("amount") << std::endl;
std::cout << "Min: " << store.min("amount") << std::endl;
std::cout << "Max: " << store.max("amount") << std::endl;
std::cout << "Count: " << store.count("amount") << std::endl;
std::cout << "Distinct: " << store.countDistinct("region") << std::endl;
```

### Gefilterte Aggregation

```cpp
std::vector<bool> mask = {true, false, true};  // Include rows 0 and 2
double sum = store.sumWhere("amount", mask);   // 100 + 150 = 250
```

### Column Statistics

```cpp
auto stats = store.getColumnStats("amount");

std::cout << "Rows: " << stats.row_count << std::endl;
std::cout << "Nulls: " << stats.null_count << std::endl;
std::cout << "Distinct: " << stats.distinct_count << std::endl;
std::cout << "Min: " << *stats.min_value << std::endl;
std::cout << "Max: " << *stats.max_value << std::endl;
std::cout << "Avg: " << stats.avg_value << std::endl;
```

## Materialized Views

### View erstellen

```cpp
MaterializedView::Definition def;
def.name = "sales_by_region";
def.source_collection = "sales";
def.dimensions.push_back({"region", "", true});
def.measures.push_back({"total_sales", "amount", Measure::Function::Sum});
def.measures.push_back({"order_count", "id", Measure::Function::Count});
def.refresh_mode = MaterializedView::Definition::RefreshMode::Periodic;
def.refresh_interval_seconds = 3600;  // Stündlich

MaterializedView view(def);
view.refresh();  // Erste Aktualisierung
```

### View abfragen

```cpp
std::vector<Filter> filters;
Filter f;
f.field = "region";
f.op = Filter::Operator::Eq;
f.value = std::string("North");
filters.push_back(f);

auto result = view.query(filters, {}, 10);
```

### Status prüfen

```cpp
if (view.isStale()) {
    view.refresh();
}

std::cout << "Last refresh: " 
          << std::chrono::system_clock::to_time_t(view.lastRefreshTime())
          << std::endl;
std::cout << "Row count: " << view.rowCount() << std::endl;
```

## Aggregations-Funktionen

| Funktion | Beschreibung |
|----------|--------------|
| COUNT | Anzahl der Werte |
| SUM | Summe aller Werte |
| AVG | Durchschnitt |
| MIN | Minimum |
| MAX | Maximum |
| STDDEV | Standardabweichung |
| VARIANCE | Varianz |
| MEDIAN | Median (50. Perzentil) |
| PERCENTILE | Beliebiges Perzentil |
| COUNT_DISTINCT | Anzahl eindeutiger Werte |
| FIRST | Erster Wert |
| LAST | Letzter Wert |

### Percentile Beispiel

```cpp
Measure m;
m.name = "p95_latency";
m.field = "latency";
m.function = Measure::Function::Percentile;
m.percentile_value = 95.0;  // 95. Perzentil
```

## Filter-Operatoren

| Operator | Beschreibung |
|----------|--------------|
| Eq | Gleichheit (=) |
| Ne | Ungleichheit (!=) |
| Lt | Kleiner als (<) |
| Le | Kleiner gleich (<=) |
| Gt | Größer als (>) |
| Ge | Größer gleich (>=) |
| In | Enthält in Liste |
| NotIn | Nicht in Liste |
| Contains | String enthält |
| StartsWith | String beginnt mit |
| EndsWith | String endet mit |
| IsNull | Ist NULL |
| IsNotNull | Ist nicht NULL |
| Between | Zwischen zwei Werten |

## Performance-Tipps

### 1. Index-Nutzung

- Filter auf indizierte Spalten verwenden
- Sortierung nach indizierten Spalten

### 2. Materialized Views

- Häufig verwendete Aggregationen vorberechnen
- Refresh-Intervall an Aktualitätsanforderungen anpassen

### 3. Columnar Store

- Für große Datasets verwenden
- Vektorisierte Operationen nutzen

### 4. Query-Optimierung

- EXPLAIN zur Plananalyse
- Limit für Ergebnismengen
- Selektive Filter verwenden

## Limitationen

- Keine echte Columnar-Persistenz (in-memory)
- Keine automatische View-Auswahl
- Keine parallele Aggregation (single-threaded)

## Data Export

### Arrow Export (GAP-003) - 🚧 Phase 1 Complete

OLAP-Ergebnisse können in verschiedene Formate exportiert werden:

```cpp
#include "analytics/arrow_export.h"
#include "analytics/analytics_export.h"

// OLAP Query ausführen
auto result = engine.execute(query);

// Zu RecordBatch konvertieren
ArrowRecordBatch batch;
batch.addColumn({"dimension", ArrowRecordBatch::DataType::STRING, false});
batch.addColumn({"measure", ArrowRecordBatch::DataType::DOUBLE, false});

for (const auto& row : result.rows) {
    // Konvertiere OLAP Result zu RecordBatch
    // ... (Implementierung folgt in Phase 2)
}

// Export zu JSON oder CSV
auto exporter = ExporterFactory::createDefaultExporter();
ExportOptions options;
options.format = ExportFormat::JSON;

auto export_result = exporter->exportToFile(batch, "analytics.json", options);
```

**Unterstützte Formate:**
- ✅ JSON (vollständig implementiert)
- ✅ CSV (vollständig implementiert)
- ⚠️ Arrow IPC (Placeholder für Phase 2)
- ⚠️ Parquet (geplant für Phase 2)

Siehe [GAP-003 Dokumentation](../analytics/GAP_003_ARROW_ANALYTICS.md) für Details zur Arrow-Integration.

## Roadmap

- [ ] Persistente Columnar Storage
- [ ] Parallel Aggregation
- [ ] Automatic View Selection
- [ ] Incremental View Refresh
- [x] Analytics Export Interface (GAP-003 Phase 1)
- [ ] Apache Arrow C++ Integration (GAP-003 Phase 2)
- [ ] Parquet Format Support
- [ ] GPU-beschleunigte Aggregation

## Siehe auch

- [GAP-003: Arrow Analytics](../analytics/GAP_003_ARROW_ANALYTICS.md)
- [Analytics Module README](../../../src/analytics/README.md)
- [OLAP Guide](../analytics/olap_guide.md)

---

**Letzte Aktualisierung:** 04. Februar 2026  
**Maintainer:** ThemisDB Team
