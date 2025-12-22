# 📚 Subquery & CTE Quick Reference

**Category:** 📚 References  
**Version:** v1.3.0  
**Status:** ✅ Complete  
**Datum:** 22. Dezember 2025

---

## 📑 Inhaltsverzeichnis

- [📋 Übersicht](#-übersicht)
- [✨ Features & Highlights](#-features--highlights)
- [🚀 Schnellstart](#-schnellstart)
- [📖 Detaillierte Dokumentation](#-detaillierte-dokumentation)
- [💡 Best Practices](#-best-practices)
- [🔧 Troubleshooting](#-troubleshooting)
- [📚 Siehe auch](#-siehe-auch)
- [📝 Changelog](#-changelog)

---

## 📋 Übersicht

Diese Seite bietet eine Schnellreferenz für Subquery & CTE Syntax in ThemisDB AQL.

---

## 🚀 Schnellstart - Syntax Examples

### Basic CTE
```aql
WITH expensive AS (
    FOR h IN hotels 
    FILTER h.price > 200 
    RETURN h
)
FOR doc IN expensive
RETURN doc
```

### Multiple CTEs
```aql
WITH 
  expensive AS (FOR h IN hotels FILTER h.price > 200 RETURN h),
  berlin AS (FOR e IN expensive FILTER e.city == "Berlin" RETURN e)
FOR doc IN berlin RETURN doc
```

### Scalar Subquery
```aql
FOR user IN users
LET avgAge = (FOR u IN users RETURN AVG(u.age))
RETURN {user: user.name, avgAge: avgAge[0]}
```

### Correlated Subquery
```aql
FOR user IN users
LET userOrders = (
    FOR o IN orders 
    FILTER o.userId == user._key 
    RETURN o
)
RETURN {user: user.name, orders: userOrders}
```

### ANY Quantifier
```aql
FOR doc IN users
FILTER ANY tag IN doc.tags SATISFIES tag == "premium"
RETURN doc
```

### ALL Quantifier
```aql
FOR order IN orders
FILTER ALL item IN order.items SATISFIES item.price < 100
RETURN order
```

## Implementation Files

### Core Implementation
- `include/query/aql_ast.h` - AST nodes (WithNode, SubqueryExpr, etc.)
- `src/query/aql_parser.cpp` - Parsing logic
- `include/query/aql_translator.h` - CTE execution metadata
- `src/query/aql_translator.cpp` - CTE collection and optimization
- `include/query/query_engine.h` - Execution interface
- `src/query/query_engine.cpp` - Execution logic

### Memory Management
- `include/query/cte_cache.h` - CTECache interface
- `src/query/cte_cache.cpp` - Spill-to-disk implementation

### Tests
- `tests/test_aql_subqueries.cpp` - Parser & execution tests (21 tests)
- `tests/test_cte_cache.cpp` - Memory management tests (15 tests)

## Key Features

✅ **WITH Clause CTEs** - Common Table Expressions with multiple references  
✅ **Scalar Subqueries** - In LET and RETURN expressions  
✅ **Correlated Subqueries** - Access to outer variables  
✅ **ANY/ALL Quantifiers** - With full subquery support  
✅ **Memory Management** - Automatic spill-to-disk for large CTEs (100MB default)  
✅ **Optimization** - Materialization heuristics based on reference count  

## Memory Configuration

Default: 100MB in-memory cache, automatic spill to `./themis_cte_spill/`

Future: Configurable via QueryEngine constructor or config file

---

## 📚 Siehe auch

- [Subquery Implementation](aql_subquery_implementation.md) - Implementierungsdetails
- [AQL Syntax](aql_syntax.md) - WITH-Klausel Syntax
- [Query Engine](aql_query_engine.md) - CTE Execution
- [Best Practices](aql_subquery_reference.md#performance-tips) - Wann CTEs nutzen

---

## 📝 Changelog

### v1.3.0 - 22. Dezember 2025
- ✅ **Template-Update:** Standardisierung auf v1.3.0 Dokumentationsformat
- ✅ **Struktur:** 8-Abschnitte-Format mit Emojis und TOC

### v1.0 - 17. November 2025
- Initial Release mit Subquery & CTE Quick Reference
- Syntax-Beispiele für alle Subquery-Typen
- Multiple references to same subquery
- Complex filtering that should be materialized
- Improving query readability

**Avoid CTEs when:**
- Single-use subqueries (inlining faster)
- Very large result sets (consider streaming)
- Simple filters (better to inline)

## Status

**Phase 3:** ✅ Parsing & AST (14h)  
**Phase 4:** ✅ Execution & Memory Management (14h)  
**Total:** 28 hours implementation time  

**Compilation:** ⚠️ Pending (OpenSSL dependency issue)  
**Tests:** ✅ 36 tests implemented (21 execution + 15 cache)  
**Documentation:** ✅ Complete  

## Next Steps

1. Fix OpenSSL build dependency
2. Run full test suite
3. Performance benchmarks
4. Consider Phase 5: Window Functions or Advanced JOINs

See `docs/SUBQUERY_IMPLEMENTATION_SUMMARY.md` for complete details.
