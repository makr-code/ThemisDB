"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            main.py                                            ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-04-14 18:45:39                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     340                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 99e8682b66  2026-03-24  Add complete schulung/ training materials folder ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
ThemisDB Schulungsbeispiel 2: AQL Queries
=========================================
Demonstriert:
  - Komplexe FILTER-Bedingungen
  - Multi-Collection Joins
  - COLLECT / Aggregationen
  - Subqueries und LET
  - SORT, LIMIT, Pagination
  - Array-Funktionen
  - Datums-Funktionen
  - UPSERT

Voraussetzungen:
  pip install themis-client
  docker run -d -p 8080:8080 themisdb/themisdb:latest
"""

import sys
from datetime import datetime, timedelta
from themis_client import ThemisClient


def setup_data(client: ThemisClient) -> None:
    """Testdaten für alle Beispiele aufbauen."""
    print("\n=== Setup ===")

    for coll in ["sq_products", "sq_categories", "sq_orders", "sq_order_items"]:
        client.query(f"CREATE COLLECTION IF NOT EXISTS {coll}")
        client.query(f"TRUNCATE COLLECTION {coll}")

    # Kategorien
    client.query("""
        FOR c IN [
          { _key: "cat_db",     name: "Datenbanken",    parent: null      },
          { _key: "cat_ai",     name: "Künstliche Intelligenz", parent: null },
          { _key: "cat_aql",    name: "AQL",            parent: "cat_db"  },
          { _key: "cat_graph",  name: "Graph-DBs",      parent: "cat_db"  },
          { _key: "cat_ml",     name: "Machine Learning", parent: "cat_ai" }
        ]
          INSERT c INTO sq_categories
    """)

    # Produkte
    client.query("""
        FOR p IN [
          { _key: "p1", name: "ThemisDB Handbuch",  price: 49.99, category: "cat_db",  stock: 100, tags: ["database", "themisdb"] },
          { _key: "p2", name: "AQL Deep Dive",       price: 39.99, category: "cat_aql", stock:  50, tags: ["aql", "query"] },
          { _key: "p3", name: "Graph Algorithms",    price: 59.99, category: "cat_graph", stock: 30, tags: ["graph", "algorithms"] },
          { _key: "p4", name: "ML mit ThemisDB",     price: 44.99, category: "cat_ml",  stock:  75, tags: ["ml", "themisdb", "ai"] },
          { _key: "p5", name: "KI Grundlagen",       price: 34.99, category: "cat_ai",  stock:   0, tags: ["ai", "intro"] },
          { _key: "p6", name: "Vector Search Guide", price: 29.99, category: "cat_ai",  stock:  20, tags: ["vector", "search", "ai"] }
        ]
          INSERT p INTO sq_products
    """)

    # Bestellungen (letzte 30 Tage)
    base_date = (datetime.utcnow() - timedelta(days=30)).strftime("%Y-%m-%dT%H:%M:%SZ")
    mid_date  = (datetime.utcnow() - timedelta(days=15)).strftime("%Y-%m-%dT%H:%M:%SZ")
    now       = datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%SZ")

    client.query(f"""
        FOR o IN [
          {{ _key: "o1", user: "alice", status: "completed", created: "{base_date}", total: 89.98  }},
          {{ _key: "o2", user: "bob",   status: "completed", created: "{mid_date}",  total: 44.99  }},
          {{ _key: "o3", user: "alice", status: "pending",   created: "{now}",       total: 29.99  }},
          {{ _key: "o4", user: "clara", status: "completed", created: "{mid_date}",  total: 104.98 }},
          {{ _key: "o5", user: "bob",   status: "cancelled", created: "{base_date}", total: 59.99  }}
        ]
          INSERT o INTO sq_orders
    """)

    # Bestellpositionen
    client.query("""
        FOR i IN [
          { order_id: "o1", product_id: "p1", qty: 1, price: 49.99 },
          { order_id: "o1", product_id: "p2", qty: 1, price: 39.99 },
          { order_id: "o2", product_id: "p4", qty: 1, price: 44.99 },
          { order_id: "o3", product_id: "p6", qty: 1, price: 29.99 },
          { order_id: "o4", product_id: "p1", qty: 1, price: 49.99 },
          { order_id: "o4", product_id: "p3", qty: 1, price: 59.99 },
          { order_id: "o5", product_id: "p3", qty: 1, price: 59.99 }
        ]
          INSERT i INTO sq_order_items
    """)
    print("Testdaten eingefügt.")


def demo_complex_filter(client: ThemisClient) -> None:
    """Komplexe Filterbedingungen."""
    print("\n=== Komplexe Filter ===")

    # Verfügbare Produkte mit Preis-Range
    result = client.query("""
        FOR p IN sq_products
          FILTER p.stock > 0
          FILTER p.price >= @min_price AND p.price <= @max_price
          SORT p.price ASC
          RETURN { name: p.name, price: p.price, stock: p.stock }
    """, bind_vars={"min_price": 30, "max_price": 50})
    print("Produkte (30–50 EUR, verfügbar):")
    for p in result:
        print(f"  {p['name']:30} {p['price']:.2f} EUR (Lager: {p['stock']})")

    # Tag-Filter mit IN
    ai_products = client.query("""
        FOR p IN sq_products
          FILTER "ai" IN p.tags OR "ml" IN p.tags
          RETURN p.name
    """)
    print(f"\nProdukte mit Tag 'ai' oder 'ml': {ai_products}")


def demo_join(client: ThemisClient) -> None:
    """Joins zwischen Collections."""
    print("\n=== Joins ===")

    # Bestellungen mit Kundennamen und Produktnamen
    result = client.query("""
        FOR order IN sq_orders
          FILTER order.status == "completed"
          FOR item IN sq_order_items
            FILTER item.order_id == order._key
            FOR product IN sq_products
              FILTER product._key == item.product_id
              RETURN {
                user:    order.user,
                product: product.name,
                qty:     item.qty,
                price:   item.price
              }
    """)
    print("Abgeschlossene Bestellungen:")
    for r in result:
        print(f"  {r['user']:8} kaufte: {r['product']:30} ({r['price']:.2f} EUR)")

    # DOCUMENT()-Lookup (effizienter bei bekanntem _key)
    result2 = client.query("""
        FOR item IN sq_order_items
          LET product = DOCUMENT("sq_products", item.product_id)
          LET order   = DOCUMENT("sq_orders",   item.order_id)
          FILTER order.status == "completed"
          RETURN {
            order:   item.order_id,
            product: product.name,
            total:   item.qty * item.price
          }
    """)
    print(f"\nVia DOCUMENT(): {len(result2)} Positionen abgerufen.")


def demo_aggregation(client: ThemisClient) -> None:
    """Aggregationen und Grouping."""
    print("\n=== Aggregationen ===")

    # Umsatz pro Benutzer
    revenue_by_user = client.query("""
        FOR order IN sq_orders
          FILTER order.status == "completed"
          COLLECT user = order.user
            AGGREGATE
              total_revenue = SUM(order.total),
              order_count   = COUNT(1),
              avg_order     = AVG(order.total)
          SORT total_revenue DESC
          RETURN {
            user,
            total_revenue: ROUND(total_revenue, 2),
            order_count,
            avg_order: ROUND(avg_order, 2)
          }
    """)
    print("Umsatz pro Benutzer:")
    for r in revenue_by_user:
        print(f"  {r['user']:8}: {r['total_revenue']:.2f} EUR ({r['order_count']} Bestellungen, Ø {r['avg_order']:.2f} EUR)")

    # Produkte nach Kategorie
    by_category = client.query("""
        FOR p IN sq_products
          COLLECT category = p.category
            AGGREGATE count = COUNT(1), avg_price = AVG(p.price)
          LET cat = DOCUMENT("sq_categories", category)
          RETURN { category: cat.name, count, avg_price: ROUND(avg_price, 2) }
    """)
    print("\nProdukte nach Kategorie:")
    for r in by_category:
        print(f"  {r['category']:25}: {r['count']} Produkte, Ø {r['avg_price']:.2f} EUR")


def demo_subquery(client: ThemisClient) -> None:
    """Subqueries und verschachtelte Abfragen."""
    print("\n=== Subqueries ===")

    # Benutzer mit Bestellhistorie
    result = client.query("""
        FOR user IN UNIQUE(
          FOR o IN sq_orders RETURN o.user
        )
          LET orders = (
            FOR o IN sq_orders
              FILTER o.user == user AND o.status == "completed"
              LET items = (
                FOR i IN sq_order_items
                  FILTER i.order_id == o._key
                  LET product = DOCUMENT("sq_products", i.product_id)
                  RETURN product.name
              )
              RETURN { order_id: o._key, total: o.total, products: items }
          )
          FILTER LENGTH(orders) > 0
          RETURN { user, completed_orders: LENGTH(orders), history: orders }
    """)
    print("Bestellhistorie pro Benutzer:")
    for r in result:
        print(f"\n  {r['user']} ({r['completed_orders']} Bestellungen):")
        for o in r["history"]:
            print(f"    Bestellung {o['order_id']}: {o['total']:.2f} EUR — {', '.join(o['products'])}")


def demo_date_functions(client: ThemisClient) -> None:
    """Datums-Funktionen."""
    print("\n=== Datums-Funktionen ===")

    result = client.query("""
        LET now        = DATE_NOW()
        LET week_ago   = DATE_SUBTRACT(now, 7, "day")
        LET month_ago  = DATE_SUBTRACT(now, 30, "day")

        LET orders_7d = LENGTH(
          FOR o IN sq_orders
            FILTER o.created >= DATE_ISO8601(week_ago)
            RETURN 1
        )
        LET orders_30d = LENGTH(
          FOR o IN sq_orders
            FILTER o.created >= DATE_ISO8601(month_ago)
            RETURN 1
        )

        RETURN {
          now:       DATE_FORMAT(now, "%yyyy-%mm-%dd"),
          orders_7d,
          orders_30d
        }
    """)
    print(f"Datum heute: {result[0]['now']}")
    print(f"Bestellungen letzte 7 Tage:  {result[0]['orders_7d']}")
    print(f"Bestellungen letzte 30 Tage: {result[0]['orders_30d']}")


def demo_array_functions(client: ThemisClient) -> None:
    """Array-Funktionen."""
    print("\n=== Array-Funktionen ===")

    result = client.query("""
        LET all_tags = UNIQUE(FLATTEN(
          FOR p IN sq_products RETURN p.tags
        ))
        LET sorted_tags = SORTED(all_tags)
        RETURN {
          unique_tags: sorted_tags,
          count: LENGTH(sorted_tags)
        }
    """)
    print(f"Alle eindeutigen Tags ({result[0]['count']}): {result[0]['unique_tags']}")

    # Produkte, die mehrere gesuchte Tags haben
    result2 = client.query("""
        LET search_tags = ["ai", "database", "themisdb"]
        FOR p IN sq_products
          LET matching = INTERSECTION(p.tags, search_tags)
          FILTER LENGTH(matching) >= 2
          SORT LENGTH(matching) DESC
          RETURN { name: p.name, matching_tags: matching, score: LENGTH(matching) }
    """)
    print("\nProdukte mit mind. 2 gesuchten Tags:")
    for r in result2:
        print(f"  {r['name']:30} Tags: {r['matching_tags']}")


def cleanup(client: ThemisClient) -> None:
    """Aufräumen."""
    print("\n=== Cleanup ===")
    for coll in ["sq_products", "sq_categories", "sq_orders", "sq_order_items"]:
        client.query(f"DROP COLLECTION {coll}")
    print("Collections gelöscht.")


def main() -> int:
    print("ThemisDB Schulungsbeispiel 2: AQL Queries")
    print("=" * 45)

    client = ThemisClient("http://localhost:8080")

    try:
        health = client.health()
        print(f"Verbunden mit ThemisDB {health.get('version', 'unbekannt')}")
    except Exception as e:
        print(f"FEHLER: {e}")
        return 1

    try:
        setup_data(client)
        demo_complex_filter(client)
        demo_join(client)
        demo_aggregation(client)
        demo_subquery(client)
        demo_date_functions(client)
        demo_array_functions(client)
    finally:
        cleanup(client)

    print("\n✅ Beispiel 2 erfolgreich abgeschlossen!")
    return 0


if __name__ == "__main__":
    sys.exit(main())
