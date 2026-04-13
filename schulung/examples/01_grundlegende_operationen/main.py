"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            main.py                                            ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-04-13 04:22:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     279                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 99e8682b66  2026-03-24  Add complete schulung/ training materials folder ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
ThemisDB Schulungsbeispiel 1: Grundlegende Operationen (CRUD)
============================================================
Demonstriert:
  - Verbindung herstellen
  - Collections erstellen
  - INSERT, UPDATE, REPLACE, REMOVE, UPSERT
  - Einfache Abfragen
  - Transaktionen

Voraussetzungen:
  pip install themis-client
  docker run -d -p 8080:8080 themisdb/themisdb:latest
"""

import sys
from themis_client import ThemisClient, ThemisQueryError


def setup(client: ThemisClient) -> None:
    """Collections erstellen und Testdaten einfügen."""
    print("\n=== Setup ===")

    # Collection erstellen (idempotent)
    client.query("CREATE COLLECTION IF NOT EXISTS schulung_users")
    client.query("TRUNCATE COLLECTION schulung_users")
    print("Collection 'schulung_users' erstellt und geleert.")

    # Testdaten einfügen (Batch)
    result = client.query("""
        FOR person IN [
          { name: "Anna",    age: 30, city: "Berlin",  email: "anna@example.com",  active: true  },
          { name: "Bob",     age: 25, city: "Hamburg", email: "bob@example.com",   active: true  },
          { name: "Clara",   age: 35, city: "München", email: "clara@example.com", active: false },
          { name: "David",   age: 28, city: "Berlin",  email: "david@example.com", active: true  },
          { name: "Eva",     age: 42, city: "Köln",    email: "eva@example.com",   active: true  }
        ]
          INSERT person INTO schulung_users
          RETURN NEW._key
    """)
    print(f"Eingefügte _keys: {result}")


def demo_read(client: ThemisClient) -> None:
    """Verschiedene Lesezugriffe demonstrieren."""
    print("\n=== Lesen ===")

    # Alle Benutzer
    all_users = client.query("FOR u IN schulung_users SORT u.name RETURN u")
    print(f"Alle Benutzer ({len(all_users)}):")
    for u in all_users:
        status = "✅" if u["active"] else "❌"
        print(f"  {status} {u['name']:8} | {u['city']:10} | {u['age']} Jahre")

    # Gefiltert: nur aktive Berliner
    berliner = client.query("""
        FOR u IN schulung_users
          FILTER u.active == true AND u.city == "Berlin"
          SORT u.name
          RETURN { name: u.name, email: u.email }
    """)
    print(f"\nAktive Berliner: {[u['name'] for u in berliner]}")

    # Mit Bind-Variablen (empfohlen!)
    young = client.query(
        "FOR u IN schulung_users FILTER u.age < @max_age SORT u.age RETURN u.name",
        bind_vars={"max_age": 30},
    )
    print(f"Jünger als 30: {young}")


def demo_update(client: ThemisClient) -> None:
    """UPDATE und REPLACE demonstrieren."""
    print("\n=== Aktualisieren ===")

    # UPDATE: Einzelne Felder ändern
    result = client.query("""
        FOR u IN schulung_users
          FILTER u.name == "Anna"
          UPDATE u WITH { age: 31, last_login: DATE_NOW() }
          IN schulung_users
          RETURN { old_age: OLD.age, new_age: NEW.age }
    """)
    print(f"Anna's Alter: {result[0]['old_age']} → {result[0]['new_age']}")

    # Alle inaktiven Benutzer reaktivieren
    count = client.query("""
        LET updated = (
          FOR u IN schulung_users
            FILTER u.active == false
            UPDATE u WITH { active: true, reactivated: true }
            IN schulung_users
            RETURN 1
        )
        RETURN LENGTH(updated)
    """)
    print(f"Reaktivierte Benutzer: {count[0]}")


def demo_upsert(client: ThemisClient) -> None:
    """UPSERT demonstrieren: Insert oder Update."""
    print("\n=== Upsert ===")

    for i in range(3):
        result = client.query("""
            UPSERT { email: "frank@example.com" }
              INSERT {
                email:       "frank@example.com",
                name:        "Frank",
                age:         29,
                city:        "Stuttgart",
                active:      true,
                login_count: 1
              }
              UPDATE {
                login_count: OLD.login_count + 1,
                last_login:  DATE_NOW()
              }
            IN schulung_users
            RETURN { action: OLD == null ? "inserted" : "updated", logins: NEW.login_count }
        """)
        print(f"  Iteration {i+1}: {result[0]}")


def demo_remove(client: ThemisClient) -> None:
    """REMOVE demonstrieren."""
    print("\n=== Löschen ===")

    before = client.query("RETURN LENGTH(schulung_users)")[0]

    # Inaktive oder reaktivierte Benutzer löschen
    removed = client.query("""
        FOR u IN schulung_users
          FILTER u.reactivated == true
          REMOVE u IN schulung_users
          RETURN OLD.name
    """)
    print(f"Gelöschte Benutzer: {removed}")

    after = client.query("RETURN LENGTH(schulung_users)")[0]
    print(f"Collection: {before} → {after} Dokumente")


def demo_transaction(client: ThemisClient) -> None:
    """Explizite Transaktion demonstrieren."""
    print("\n=== Transaktion (Kontotransfer) ===")

    # Konten einrichten
    client.query("CREATE COLLECTION IF NOT EXISTS schulung_accounts")
    client.query("TRUNCATE COLLECTION schulung_accounts")
    client.query("""
        FOR acc IN [
          { _key: "alice_account", owner: "Alice", balance: 1000.00 },
          { _key: "bob_account",   owner: "Bob",   balance:  500.00 }
        ]
          INSERT acc INTO schulung_accounts
    """)

    print("Vor der Überweisung:")
    accounts = client.query("FOR a IN schulung_accounts RETURN { owner: a.owner, balance: a.balance }")
    for a in accounts:
        print(f"  {a['owner']}: {a['balance']:.2f} EUR")

    # Transaktion: 200 EUR von Alice zu Bob
    try:
        client.query("""
            BEGIN TRANSACTION

              LET from_balance = DOCUMENT("schulung_accounts", "alice_account").balance
              LET amount = 200.0

              FILTER from_balance >= amount   // Prüfung auf ausreichend Guthaben

              UPDATE "alice_account" WITH { balance: from_balance - amount } IN schulung_accounts
              UPDATE "bob_account"   WITH {
                balance: DOCUMENT("schulung_accounts", "bob_account").balance + amount
              } IN schulung_accounts

            COMMIT
        """)
        print("\nNach der Überweisung (200 EUR Alice → Bob):")
        accounts = client.query(
            "FOR a IN schulung_accounts RETURN { owner: a.owner, balance: a.balance }"
        )
        for a in accounts:
            print(f"  {a['owner']}: {a['balance']:.2f} EUR")
    except ThemisQueryError as e:
        print(f"Transaktion fehlgeschlagen: {e}")


def demo_aggregation(client: ThemisClient) -> None:
    """Aggregationen demonstrieren."""
    print("\n=== Aggregationen ===")

    # Benutzer pro Stadt
    by_city = client.query("""
        FOR u IN schulung_users
          COLLECT city = u.city WITH COUNT INTO count
          SORT count DESC
          RETURN { city, count }
    """)
    print("Benutzer pro Stadt:")
    for row in by_city:
        print(f"  {row['city']:12}: {row['count']}")

    # Statistiken
    stats = client.query("""
        FOR u IN schulung_users
          COLLECT AGGREGATE
            avg_age = AVG(u.age),
            min_age = MIN(u.age),
            max_age = MAX(u.age),
            total   = COUNT(1)
          RETURN { avg_age: ROUND(avg_age, 1), min_age, max_age, total }
    """)
    print(f"\nAlters-Statistik: {stats[0]}")


def cleanup(client: ThemisClient) -> None:
    """Testdaten aufräumen."""
    print("\n=== Cleanup ===")
    client.query("DROP COLLECTION schulung_users")
    client.query("DROP COLLECTION schulung_accounts")
    print("Test-Collections gelöscht.")


def main() -> int:
    print("ThemisDB Schulungsbeispiel 1: Grundlegende Operationen")
    print("=" * 55)

    client = ThemisClient("http://localhost:8080")

    try:
        health = client.health()
        print(f"Verbunden mit ThemisDB {health.get('version', 'unbekannt')}")
    except Exception as e:
        print(f"FEHLER: Kann nicht zu ThemisDB verbinden: {e}")
        print("Starten Sie ThemisDB: docker run -d -p 8080:8080 themisdb/themisdb:latest")
        return 1

    try:
        setup(client)
        demo_read(client)
        demo_update(client)
        demo_upsert(client)
        demo_remove(client)
        demo_transaction(client)
        demo_aggregation(client)
    finally:
        cleanup(client)

    print("\n✅ Beispiel 1 erfolgreich abgeschlossen!")
    return 0


if __name__ == "__main__":
    sys.exit(main())
