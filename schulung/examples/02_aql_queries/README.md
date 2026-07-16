# AQL Queries — Beispiele

![Schwierigkeit](https://img.shields.io/badge/schwierigkeit-fortgeschritten-orange)
![Dauer](https://img.shields.io/badge/dauer-30--45%20min-blue)

## Übersicht

Dieses Beispiel vertieft AQL-Kenntnisse mit praxisnahen Beispielen:

- Komplexe FILTER-Bedingungen mit AND/OR, `IN`, Regex
- Multi-Collection Joins (verschachteltes FOR + DOCUMENT())
- COLLECT mit mehreren Aggregationsfunktionen
- Verschachtelte Subqueries
- Datums- und Array-Funktionen

## Ausführen

```bash
cd schulung/examples/02_aql_queries
pip install themis-client
python main.py
```

## Themen

### Komplexe Filter
```aql
FOR p IN products
  FILTER p.stock > 0
  FILTER p.price >= @min AND p.price <= @max
  FILTER "ai" IN p.tags OR "ml" IN p.tags
  RETURN p
```

### Effiziente Joins mit DOCUMENT()
```aql
FOR item IN order_items
  LET product = DOCUMENT("products", item.product_id)
  LET order   = DOCUMENT("orders",   item.order_id)
  RETURN { product: product.name, order: order.status }
```

### Aggregation mit COLLECT
```aql
FOR order IN orders
  COLLECT user = order.user_id
    AGGREGATE
      revenue    = SUM(order.total),
      avg_order  = AVG(order.total),
      count      = COUNT(1)
  RETURN { user, revenue, avg_order, count }
```

## Weiterführend

- [Nächstes Beispiel: Graph-Daten](../03_graph_daten/)
- [AQL Referenz](../../dokumente/02_aql_referenz_kurzuebersicht.md)
