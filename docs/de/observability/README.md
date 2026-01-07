# Analytics Module

**Stand:** 22. Dezember 2025  
**Version:** v1.3.0  
**Kategorie:** 🔍 Observability

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [Source-Code Referenz](#source-code-referenz)
- [Implementierte Klassen](#implementierte-klassen)

## Übersicht

Das Analytics-Modul bietet erweiterte OLAP-Funktionen (Online Analytical Processing) und Complex Event Processing (CEP) für ThemisDB.

## Source-Code Referenz

| Komponente | Header | Source | LOC |
|------------|--------|--------|-----|
| OLAP Engine | `include/analytics/olap.h` | `src/analytics/olap.cpp` | ~2,000 |
| CEP Engine | `include/analytics/cep.h` | `src/analytics/cep.cpp` | ~1,700 |

## Implementierte Klassen

### OLAPEngine

Die OLAP Engine unterstützt:

**Aggregationsfunktionen:**
- `COUNT`, `SUM`, `AVG`, `MIN`, `MAX`
- `STDDEV`, `VARIANCE`, `MEDIAN`
- `PERCENTILE`, `COUNT_DISTINCT`
- `FIRST`, `LAST`

**Grouping Operators:**
- `CUBE` - Alle Kombinationen von Dimensionen
- `ROLLUP` - Hierarchische Aggregation
- `GROUPING SETS` - Benutzerdefinierte Gruppierungen

**Filter Operators:**
- `Eq`, `Ne`, `Lt`, `Le`, `Gt`, `Ge`, `In`, `Between`, `Like`, `IsNull`

### ColumnarStore

Vektorisierte spaltenbasierte Speicherung für analytische Workloads:
- Apache Arrow Integration
- SIMD-optimierte Scans
- Batch-Verarbeitung

### CEPEngine (Complex Event Processing)

Echtzeit-Streaming-Analytics mit:
- Event Pattern Language (EPL)
- Pattern Matching (Sequence, Any, All)
- Window Operations (Tumbling, Sliding, Session)
- Event Correlation

## API Beispiele

### OLAP Query
```cpp
OLAPQuery query;
query.dimensions = {{"region", ""}, {"product", ""}};
query.measures = {{"revenue", "amount", Measure::Function::Sum}};
query.grouping_mode = GroupingMode::Cube;

auto results = olap_engine->execute(query);
```

### CEP Pattern
```cpp
cep_engine->registerPattern("high_value_sequence", R"(
    SELECT * FROM orders
    MATCH_RECOGNIZE (
        PARTITION BY customer_id
        MEASURES A.amount as first_amount, B.amount as second_amount
        PATTERN (A B)
        DEFINE A AS amount > 1000, B AS amount > 1000
    )
)");
```

## Verwandte Dokumentation

- [CEP Streaming Analytics](CEP_STREAMING_ANALYTICS.md) - Detaillierte CEP-Dokumentation
- [Features OLAP Analytics](../features/features_olap_analytics.md) - Feature-Übersicht
