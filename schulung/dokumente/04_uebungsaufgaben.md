# Übungsaufgaben ThemisDB

> Praktische Übungen zur Vertiefung der ThemisDB-Kenntnisse. Alle Aufgaben bauen auf den vorherigen Modulen auf.

**Voraussetzungen**: ThemisDB läuft lokal (`docker run -d -p 8080:8080 themisdb/themisdb:latest`)

---

## Aufgabenblock 1: Grundlegende Operationen ⭐

### Aufgabe 1.1 — Collection und CRUD

**Ziel**: Erstellen Sie eine Bibliotheksverwaltung.

**Schritt 1**: Erstellen Sie eine Collection `books` mit mindestens 5 Büchern:

```
Pflichtfelder:
- title   (String)
- author  (String)
- year    (Integer)
- genre   (String)
- pages   (Integer)
- available (Boolean, Standard: true)
```

**Schritt 2**: Abfrage — Alle Bücher nach Jahr sortiert ausgeben.

**Schritt 3**: Update — Markieren Sie alle Bücher vor 1990 als `classic: true`.

**Schritt 4**: Löschen — Entfernen Sie alle Bücher mit mehr als 1000 Seiten.

**Erwartetes Ergebnis**: Mindestens 3 Bücher in der Collection, mind. ein `classic: true`.

<details>
<summary>💡 Musterlösung</summary>

```aql
// Schritt 1
CREATE COLLECTION IF NOT EXISTS books

FOR book IN [
  { title: "Der Herr der Ringe", author: "J.R.R. Tolkien",    year: 1954, genre: "Fantasy",   pages: 1200, available: true },
  { title: "1984",               author: "George Orwell",     year: 1949, genre: "Dystopie",  pages: 328,  available: true },
  { title: "Dune",               author: "Frank Herbert",     year: 1965, genre: "SciFi",     pages: 688,  available: true },
  { title: "Der Alchemist",      author: "Paulo Coelho",      year: 1988, genre: "Roman",     pages: 208,  available: true },
  { title: "Neuromancer",        author: "William Gibson",    year: 1984, genre: "Cyberpunk", pages: 271,  available: false }
]
  INSERT book INTO books

// Schritt 2
FOR b IN books SORT b.year ASC RETURN b

// Schritt 3
FOR b IN books
  FILTER b.year < 1990
  UPDATE b WITH { classic: true } IN books

// Schritt 4
FOR b IN books
  FILTER b.pages > 1000
  REMOVE b IN books
```

</details>

---

### Aufgabe 1.2 — Aggregation

**Ziel**: Analysieren Sie die Bücher-Collection.

Schreiben Sie AQL-Abfragen für:

1. Anzahl der Bücher pro Genre
2. Durchschnittliche Seitenzahl aller Bücher
3. Das neueste und älteste Buch (Titel + Jahr)
4. Alle Genres, in denen mehr als 1 Buch vorhanden ist

<details>
<summary>💡 Musterlösung</summary>

```aql
// 1. Bücher pro Genre
FOR b IN books
  COLLECT genre = b.genre WITH COUNT INTO count
  RETURN { genre, count }

// 2. Durchschnittliche Seitenzahl
FOR b IN books
  COLLECT AGGREGATE avg_pages = AVG(b.pages)
  RETURN { avg_pages: ROUND(avg_pages, 1) }

// 3. Neuestes und ältestes Buch
FOR b IN books
  SORT b.year DESC
  LIMIT 1
  RETURN { newest: b.title, year: b.year }

FOR b IN books
  SORT b.year ASC
  LIMIT 1
  RETURN { oldest: b.title, year: b.year }

// 4. Genres mit mehr als 1 Buch
FOR b IN books
  COLLECT genre = b.genre WITH COUNT INTO count
  FILTER count > 1
  RETURN { genre, count }
```

</details>

---

## Aufgabenblock 2: Graph-Operationen ⭐⭐

### Aufgabe 2.1 — Freundschaftsnetzwerk

**Ziel**: Modellieren Sie ein Freundschaftsnetzwerk.

**Schritt 1**: Erstellen Sie:
- Vertex-Collection `persons` mit 6 Personen (name, age, city)
- Edge-Collection `friends` (Type EDGE)
- Graph `friendship_graph`

**Schritt 2**: Fügen Sie folgende Freundschaften ein:
```
Alice  → Bob, Clara, David
Bob    → Eva
Clara  → Frank
David  → Eva
```

**Schritt 3**: Finden Sie alle Freunde von Alice (direkt, Tiefe 1).

**Schritt 4**: Finden Sie alle Personen, die Alice über einen Zwischenmann kennt (Tiefe 2, exklusive direkte Freunde).

**Schritt 5**: Finden Sie den kürzesten Pfad von Alice zu Frank.

<details>
<summary>💡 Musterlösung</summary>

```aql
// Schritt 1 & 2
CREATE COLLECTION IF NOT EXISTS persons TYPE VERTEX
CREATE COLLECTION IF NOT EXISTS friends TYPE EDGE FROM persons TO persons
CREATE GRAPH IF NOT EXISTS friendship_graph EDGE DEFINITION friends FROM persons TO persons

FOR p IN [
  { _key: "alice",  name: "Alice",  age: 30, city: "Berlin"  },
  { _key: "bob",    name: "Bob",    age: 25, city: "Hamburg" },
  { _key: "clara",  name: "Clara",  age: 28, city: "München" },
  { _key: "david",  name: "David",  age: 32, city: "Berlin"  },
  { _key: "eva",    name: "Eva",    age: 27, city: "Köln"    },
  { _key: "frank",  name: "Frank",  age: 35, city: "Hamburg" }
] INSERT p INTO persons

FOR edge IN [
  { _from: "persons/alice", _to: "persons/bob"   },
  { _from: "persons/alice", _to: "persons/clara" },
  { _from: "persons/alice", _to: "persons/david" },
  { _from: "persons/bob",   _to: "persons/eva"   },
  { _from: "persons/clara", _to: "persons/frank" },
  { _from: "persons/david", _to: "persons/eva"   }
] INSERT edge INTO friends

// Schritt 3: Direkte Freunde
FOR v IN 1..1 OUTBOUND "persons/alice" GRAPH "friendship_graph"
  RETURN v.name

// Schritt 4: Freunde von Freunden (nicht direkt)
LET direct = (
  FOR v IN 1..1 OUTBOUND "persons/alice" GRAPH "friendship_graph"
    RETURN v._key
)
FOR v IN 2..2 OUTBOUND "persons/alice" GRAPH "friendship_graph"
  FILTER v._key NOT IN direct
  FILTER v._key != "alice"
  RETURN DISTINCT v.name

// Schritt 5: Kürzester Pfad
FOR path IN OUTBOUND SHORTEST_PATH "persons/alice" TO "persons/frank"
  GRAPH "friendship_graph"
  RETURN path.vertices[*].name
```

</details>

---

### Aufgabe 2.2 — Organisations-Hierarchie

**Ziel**: Modellieren Sie eine Firmen-Organigramm.

Erstellen Sie eine Hierarchie mit mindestens 8 Mitarbeitern in 3 Ebenen (CEO → Manager → Mitarbeiter) und beantworten Sie:

1. Wer sind die direkten Untergebenen von Manager X?
2. Wie viele Ebenen ist Mitarbeiter Y von der CEO-Ebene entfernt?
3. Gibt es Mitarbeiter ohne Vorgesetzten (außer CEO)?

---

## Aufgabenblock 3: AQL-Fortgeschritten ⭐⭐⭐

### Aufgabe 3.1 — Komplexe Aggregationen

**Ziel**: Vertriebsanalyse

Gegeben sind Collections `customers`, `products`, `sales` (mit customer_id, product_id, amount, date, region).

Schreiben Sie Abfragen für:

1. Top 5 Kunden nach Gesamtumsatz im letzten Quartal
2. Umsatz pro Region und Monat (Pivot-ähnlich)
3. Durchschnittlicher Bestellwert pro Kunde (nur Kunden mit > 3 Bestellungen)
4. Produkte, die zusammen gekauft wurden (Market Basket Analysis)

<details>
<summary>💡 Musterlösung (Abfrage 1)</summary>

```aql
LET quarter_start = DATE_SUBTRACT(DATE_NOW(), 3, "month")

FOR sale IN sales
  FILTER sale.date >= quarter_start
  COLLECT customer_id = sale.customer_id
    AGGREGATE total_revenue = SUM(sale.amount)
  SORT total_revenue DESC
  LIMIT 5
  LET customer = DOCUMENT("customers", customer_id)
  RETURN {
    name:          customer.name,
    total_revenue: ROUND(total_revenue, 2)
  }
```

</details>

---

### Aufgabe 3.2 — Subqueries und DOCUMENT()

**Ziel**: Verknüpfte Daten aus mehreren Collections abrufen.

Erstellen Sie Collections `projects`, `tasks`, `team_members` und schreiben Sie:

1. Alle Projekte mit Anzahl offener Aufgaben
2. Alle Projekte, bei denen kein Team-Mitglied zugewiesen ist
3. Team-Mitglieder mit dem höchsten Stundeneinsatz (aus tasks.hours)

---

## Aufgabenblock 4: Multi-Model ⭐⭐⭐

### Aufgabe 4.1 — Empfehlungssystem

**Ziel**: Einfaches Buchempfehlungssystem

**Setup**:
- Nutzen Sie die `books`-Collection aus Aufgabe 1.1
- Erstellen Sie `readers` (Vertex) und `read_book` (Edge)
- Fügen Sie 4 Leser mit je 2–3 gelesenen Büchern ein

**Aufgaben**:
1. Empfehlen Sie Bücher, die ähnliche Leser gelesen haben (Collaborative Filtering)
2. Finden Sie das "soziale Zentrum" — welches Buch wurde von den meisten Lesern gelesen?
3. Welche Leser haben keins der Top-3-Bücher gelesen?

---

### Aufgabe 4.2 — Zeitreihenanalyse

**Ziel**: Sensor-Datenanalyse

```python
# Testdaten generieren
import random
from datetime import datetime, timedelta

data = []
base_time = datetime.utcnow() - timedelta(hours=24)
for i in range(1440):  # 1 Messung/Minute für 24h
    data.append({
        "ts": (base_time + timedelta(minutes=i)).isoformat() + "Z",
        "sensor_id": random.choice(["s1", "s2", "s3"]),
        "temperature": 20 + random.gauss(0, 2),
        "humidity":    50 + random.gauss(0, 5)
    })
```

Schreiben Sie Abfragen für:
1. Stündlicher Temperatur-Durchschnitt pro Sensor
2. Stunden mit anomal hoher Temperatur (> Mittelwert + 2 × Standardabweichung)
3. Zeitreihen-Korrelation zwischen Temperatur und Luftfeuchtigkeit

---

## Aufgabenblock 5: Bonus-Aufgaben ⭐⭐⭐⭐

### Aufgabe 5.1 — Query-Optimierung

Gegeben ist die folgende langsame Abfrage:

```aql
FOR order IN orders
  FOR item IN order_items
    FOR product IN products
      FOR customer IN customers
        FILTER item.order_id == order._key
        FILTER item.product_id == product._key
        FILTER order.customer_id == customer._key
        FILTER order.status == "completed"
        FILTER product.category == "electronics"
        SORT order.total DESC
        LIMIT 100
        RETURN {
          customer: customer.name,
          product:  product.name,
          total:    order.total
        }
```

**Aufgabe**: Identifizieren Sie die Performance-Probleme und optimieren Sie die Abfrage. Welche Indizes würden helfen?

---

### Aufgabe 5.2 — Transaktionen

Modellieren Sie eine Banküberweisung mit:
- ACID-Transaktion
- Konsistenzprüfung (ausreichend Guthaben?)
- Audit-Log-Eintrag
- Rollback bei Fehler

---

## Lösungshinweise

- Nutzen Sie `EXPLAIN query` um den Ausführungsplan zu sehen
- `db._query(query, bindVars, {profile: true})` zeigt Performance-Details
- Alle Lösungen finden Sie in `../examples/02_aql_queries/`
- Bei Fragen: [GitHub Discussions](https://github.com/makr-code/ThemisDB/discussions)
