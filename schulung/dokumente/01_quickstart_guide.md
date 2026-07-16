# ThemisDB Quick Start Guide

> **Ziel**: In 15 Minuten mit ThemisDB arbeiten — von der Installation bis zur ersten Multi-Model-Abfrage.

---

## Schritt 1: ThemisDB starten

```bash
docker run -d \
  --name themisdb \
  -p 8080:8080 \
  -p 18765:18765 \
  -v themisdb-data:/var/lib/themisdb \
  themisdb/themisdb:latest

# Warten bis der Server bereit ist
curl http://localhost:8080/health
# Erwartet: {"status":"ok","version":"1.8.0"}
```

---

## Schritt 2: Python-Client installieren

```bash
pip install themis-client
```

```python
from themis_client import ThemisClient

client = ThemisClient("http://localhost:8080")
print(client.health())
```

---

## Schritt 3: Daten einfügen

```python
# Collection erstellen
client.query("CREATE COLLECTION IF NOT EXISTS users")

# Dokumente einfügen
client.query("""
  FOR person IN [
    { name: "Anna",  age: 30, city: "Berlin" },
    { name: "Bob",   age: 25, city: "Hamburg" },
    { name: "Clara", age: 35, city: "München" }
  ]
    INSERT person INTO users
""")
```

---

## Schritt 4: Daten abfragen

```python
# Alle Benutzer
result = client.query("FOR u IN users RETURN u")
for doc in result:
    print(doc['name'], '—', doc['city'])

# Gefilterte Abfrage
result = client.query("""
  FOR u IN users
    FILTER u.age > @min_age
    SORT u.name ASC
    RETURN u
""", bind_vars={"min_age": 28})
```

---

## Schritt 5: Daten aktualisieren

```python
# Einzelnes Dokument aktualisieren
client.query("""
  FOR u IN users
    FILTER u.name == "Anna"
    UPDATE u WITH { age: 31, last_login: DATE_NOW() }
    IN users
""")

# Dokument löschen
client.query("""
  FOR u IN users
    FILTER u.name == "Bob"
    REMOVE u IN users
""")
```

---

## Schritt 6: Graph — Verbindungen erstellen

```python
# Vertex-Collections
client.query("CREATE COLLECTION IF NOT EXISTS people TYPE VERTEX")
client.query("CREATE COLLECTION IF NOT EXISTS knows TYPE EDGE FROM people TO people")

# Graph definieren
client.query("""
  CREATE GRAPH IF NOT EXISTS network
    EDGE DEFINITION knows FROM people TO people
""")

# Knoten einfügen
client.query("""
  FOR person IN [
    { _key: "alice", name: "Alice" },
    { _key: "bob",   name: "Bob"   },
    { _key: "charlie", name: "Charlie" }
  ]
    INSERT person INTO people
""")

# Kanten einfügen
client.query("""
  FOR edge IN [
    { _from: "people/alice",   _to: "people/bob",     since: "2024-01-01" },
    { _from: "people/bob",     _to: "people/charlie", since: "2024-06-01" }
  ]
    INSERT edge INTO knows
""")
```

---

## Schritt 7: Graph traversieren

```python
# Direkte Verbindungen von Alice
result = client.query("""
  FOR v IN 1..1 OUTBOUND "people/alice"
    GRAPH "network"
    RETURN v.name
""")
print("Alice kennt:", result)  # ['Bob']

# Transitive Verbindungen (Freunde von Freunden)
result = client.query("""
  FOR v IN 1..2 OUTBOUND "people/alice"
    GRAPH "network"
    RETURN DISTINCT v.name
""")
print("Alice kennt indirekt:", result)  # ['Bob', 'Charlie']
```

---

## Schritt 8: Aggregation

```python
result = client.query("""
  FOR u IN users
    COLLECT city = u.city
      WITH COUNT INTO count
    SORT count DESC
    RETURN { city, count }
""")

for row in result:
    print(f"{row['city']}: {row['count']} Benutzer")
```

---

## Nächste Schritte

| Thema | Ressource |
|---|---|
| AQL Referenz | `02_aql_referenz_kurzuebersicht.md` |
| Datenmodellierung | `03_datenmodellierung_guide.md` |
| Übungsaufgaben | `04_uebungsaufgaben.md` |
| Best Practices | `05_best_practices_guide.md` |
| Code-Beispiele | `../examples/` |

## 🔗 Hilfreiche Links

- [Vollständige Dokumentation](../../docs/)
- [AQL Grammatik](../../aql/README.md)
- [Alle Beispiele](../../examples/)
- [GitHub Issues](https://github.com/makr-code/ThemisDB/issues)
