# AQL Zeit/Datums-Funktionen - Vollständige Übersicht

**Datum:** 22. Dezember 2025  
**Kontext:** Zeit-Helper und Interval-Funktionen in AQL  
**Status:** ✅ Vollständig in v1.3.0 implementiert

---

## Zusammenfassung

Die Zeit- und Datums-Funktionen (inkl. INTERVAL, DAYS, WORKDAYS etc.) sind **bereits vollständig in AQL v1.3.0 implementiert** mit ~45 Funktionen für relationale, graph, vector, LLM, vision, timeline, geo-spatial und process-Anwendungen.

### Schnellantwort

| Aspekt | Status | Anzahl |
|--------|--------|--------|
| **Date/Time Funktionen** | ✅ v1.3.0 | ~45 Funktionen |
| **Interval Helper** | ✅ v1.3.0 | DAYS, WEEKS, MONTHS, YEARS, etc. |
| **Arbeitstage** | ✅ v1.3.0 | WORKDAYS, WORKDAYS_ADD, IS_WORKDAY |
| **Feiertags-Kalender** | ✅ v1.3.0 | HOLIDAYS("DE_2024"), 6+ Länder |
| **PostgreSQL-kompatibel** | ✅ v1.3.0 | INTERVAL, NOW(), DATE_TRUNC |
| **Excel-kompatibel** | ✅ v1.3.0 | TODAY(), DAYS(), WORKDAYS() |
| **Reservierte Wörter** | ✅ v1.3.0 | In 72 Keywords enthalten |

---

## Vollständige Funktionsliste (45 Funktionen)

### 1. Aktuelle Zeit/Datum (11 Funktionen)

```aql
DATE_NOW()              -- Unix Timestamp in ms
NOW()                   -- SQL-Standard Alias
CURRENT_TIMESTAMP()     -- SQL-Standard
CURRENT_DATE()          -- Nur Datum (00:00:00 UTC)
CURRENT_TIME()          -- Zeit seit Mitternacht (ms)
TODAY()                 -- Start von heute (00:00:00)
YESTERDAY()             -- Start von gestern
TOMORROW()              -- Start von morgen
GETDATE()               -- SQL Server kompatibel
SYSDATE()               -- Oracle kompatibel
UNIX_TIMESTAMP()        -- MySQL kompatibel (Sekunden)
```

**Beispiel:**
```aql
-- Ereignisse der letzten 7 Tage
FOR event IN events
  FILTER event.date >= NOW() - DAYS(7)
  RETURN event
```

### 2. Interval-Funktionen (8 Funktionen) ⭐

**PostgreSQL & Excel kompatibel**

```aql
YEARS(n)                -- n Jahre in ms
MONTHS(n)               -- n Monate in ms
WEEKS(n)                -- n Wochen in ms
DAYS(n)                 -- n Tage in ms
HOURS(n)                -- n Stunden in ms
MINUTES(n)              -- n Minuten in ms
SECONDS(n)              -- n Sekunden in ms
INTERVAL(n, unit)       -- Flexibel: INTERVAL(2.5, "weeks")
```

**Beispiel:**
```aql
-- Elegante relative Zeitangaben
LET lastWeek = NOW() - WEEKS(1)
LET nextMonth = TODAY() + MONTHS(1)
LET deadline = orderDate + DAYS(30)

-- PostgreSQL-Stil
FOR order IN orders
  FILTER order.created >= NOW() - INTERVAL(3, "months")
  RETURN order
```

### 3. Datum-Komponenten (11 Funktionen)

```aql
DATE_YEAR(ts)           -- Jahr extrahieren
DATE_MONTH(ts)          -- Monat (1-12)
DATE_DAY(ts)            -- Tag (1-31)
DATE_HOUR(ts)           -- Stunde (0-23)
DATE_MINUTE(ts)         -- Minute (0-59)
DATE_SECOND(ts)         -- Sekunde (0-59)
DATE_MILLISECOND(ts)    -- Millisekunde
DATE_DAYOFWEEK(ts)      -- Wochentag (0=Sonntag)
DATE_DAYOFYEAR(ts)      -- Tag im Jahr (1-366)
DATE_QUARTER(ts)        -- Quartal (1-4)
DATE_WEEK(ts)           -- ISO-Kalenderwoche
```

**Beispiel:**
```aql
-- Gruppierung nach Monat
FOR sale IN sales
  COLLECT 
    year = DATE_YEAR(sale.date),
    month = DATE_MONTH(sale.date)
  AGGREGATE total = SUM(sale.amount)
  RETURN { year, month, total }
```

### 4. Arbeitstage-Funktionen (6 Funktionen) ⭐

**Excel WORKDAYS() kompatibel mit erweiterten Features**

```aql
WORKDAYS(start, end, holidays)          -- Arbeitstage zählen (Mo-Fr ohne Feiertage)
WORKDAYS_ADD(date, days, holidays)      -- Arbeitstage addieren
IS_WEEKEND(date)                         -- Prüft Samstag/Sonntag
IS_WORKDAY(date, holidays)               -- Prüft Arbeitstag (nicht Wochenende/Feiertag)
HOLIDAYS(calendar)                       -- Feiertags-Kalender laden
HOLIDAYS_BETWEEN(calendar, start, end)   -- Feiertage in Zeitraum
LIST_CALENDARS()                         -- Verfügbare Kalender auflisten
```

**Beispiel:**
```aql
-- Lieferzeit-Berechnung mit Feiertagen
LET holidays = HOLIDAYS("DE_2024")
LET orderDate = NOW()
LET deliveryDate = WORKDAYS_ADD(orderDate, 10, holidays)

-- Arbeitstage bis Deadline
LET daysRemaining = WORKDAYS(NOW(), project.deadline, holidays)
```

**Verfügbare Feiertags-Kalender:**
```aql
HOLIDAYS("DE_2024")          -- Deutschland 2024
HOLIDAYS("AT_2024")          -- Österreich 2024
HOLIDAYS("CH_2024")          -- Schweiz 2024
HOLIDAYS("US_FEDERAL_2024")  -- USA Federal 2024
HOLIDAYS("UK_2024")          -- Großbritannien 2024
HOLIDAYS("FR_2024")          -- Frankreich 2024

-- Mehrere Kalender kombinieren
LET companyHolidays = HOLIDAYS("DE_2024", "AT_2024")

-- Custom Feiertage
LET customHolidays = HOLIDAYS("2024-12-23", "2024-12-27", "2024-12-30")
```

### 5. Datum-Arithmetik (5 Funktionen)

```aql
DATE_ADD(date, amount, unit)       -- Datum addieren
DATE_SUBTRACT(date, amount, unit)  -- Datum subtrahieren
DATE_DIFF(start, end, unit)        -- Differenz berechnen
AGE(birthdate)                     -- Alter in Jahren
DATE_COMPARE(date1, date2)         -- Vergleich (-1, 0, 1)
```

**Beispiel:**
```aql
-- Alter berechnen
FOR person IN persons
  LET age = AGE(person.birthdate)
  FILTER age >= 18
  RETURN { name: person.name, age }

-- Differenz in verschiedenen Einheiten
LET start = DATE_TIMESTAMP("2024-01-01")
LET end = DATE_TIMESTAMP("2024-12-31")
RETURN {
  days: DATE_DIFF(start, end, "days"),      -- 365
  months: DATE_DIFF(start, end, "months"),  -- 12
  weeks: DATE_DIFF(start, end, "weeks")     -- 52
}
```

### 6. Datum-Formatierung (4 Funktionen)

```aql
DATE_FORMAT(date, format)     -- Custom Formatierung
DATE_ISO8601(date)            -- ISO 8601 String
DATE_TRUNC(date, unit)        -- Auf Einheit runden
DATE_PARSE(string, format)    -- String zu Datum parsen
```

**Beispiel:**
```aql
LET ts = DATE_NOW()
RETURN {
  german: DATE_FORMAT(ts, "%d.%m.%Y %H:%M"),  -- "22.12.2025 14:30"
  iso: DATE_ISO8601(ts),                      -- "2025-12-22T14:30:00Z"
  weekStart: DATE_TRUNC(ts, "week")           -- Wochenanfang
}
```

---

## Integration mit Multi-Model Features

### 1. Timeline-Queries (Zeitreihen)

```aql
-- Aktivität über Zeit mit Interval-Funktionen
FOR event IN timeline_events
  FILTER event.timestamp >= NOW() - MONTHS(6)
  COLLECT week = DATE_TRUNC(event.timestamp, "week")
  AGGREGATE 
    events = COUNT(1),
    avgDuration = AVG(event.duration)
  SORT week ASC
  RETURN {
    week: DATE_FORMAT(week, "%Y-W%V"),
    events,
    avgDuration
  }
```

### 2. Process Mining mit Arbeitstagen

```aql
-- Process-Durchlaufzeit ohne Wochenenden
LET holidays = HOLIDAYS("DE_2024")

FOR process IN bpmn_instances
  FILTER process.status == "completed"
  LET duration = WORKDAYS(
    process.start_time,
    process.end_time,
    holidays
  )
  COLLECT 
    processType = process.type
  AGGREGATE
    avgDuration = AVG(duration),
    p95Duration = PERCENTILE(duration, 95)
  RETURN {
    processType,
    avgWorkdays: avgDuration,
    p95Workdays: p95Duration
  }
```

### 3. Geo-Spatial mit Zeitfilter

```aql
-- Geo-Events der letzten Woche
FOR location IN geo_events
  FILTER location.timestamp >= NOW() - DAYS(7)
  FILTER GEO_DISTANCE(
    location.coords,
    GEO_POINT(13.405, 52.520)  -- Berlin
  ) < 10000  -- 10 km
  RETURN {
    location: location.name,
    coords: location.coords,
    age: DATE_DIFF(location.timestamp, NOW(), "hours") + " hours ago"
  }
```

### 4. LLM/Vision mit Zeitkontext

```aql
-- Vision-Analyse von Bildern der letzten 30 Tage
FOR image IN drone_images
  FILTER image.captured_at >= TODAY() - DAYS(30)
  LET analysis = LLM VISION ANALYZE image.path
    USING MODEL 'llava-7b'
    DETECT [objects, text]
  RETURN {
    image_id: image._id,
    captured: DATE_FORMAT(image.captured_at, "%Y-%m-%d"),
    daysAgo: DATE_DIFF(image.captured_at, NOW(), "days"),
    analysis: analysis
  }
```

### 5. Graph-Traversal mit Zeitfilter

```aql
-- Freundes-Netzwerk mit Aktivität
FOR friend IN 1..3 OUTBOUND @userId friendship
  -- Nur aktive Freunde (letzte 90 Tage)
  FILTER friend.last_active >= NOW() - DAYS(90)
  LET daysSinceActive = DATE_DIFF(friend.last_active, NOW(), "days")
  SORT daysSinceActive ASC
  RETURN {
    name: friend.name,
    lastActive: DATE_FORMAT(friend.last_active, "%d.%m.%Y"),
    daysAgo: daysSinceActive
  }
```

### 6. Vector-Search mit Recency Scoring

```aql
-- Ähnlichkeitssuche mit zeitlicher Gewichtung
LET query_embedding = LLM EMBED @search_query

FOR doc IN documents
  LET similarity = COSINE_SIMILARITY(doc.embedding, query_embedding)
  FILTER similarity > 0.7
  
  -- Zeitliche Gewichtung: neuere Dokumente bevorzugen
  LET recency_score = 1 / (1 + DATE_DIFF(doc.published, NOW(), "days") / 30)
  LET final_score = similarity * 0.7 + recency_score * 0.3
  
  SORT final_score DESC
  LIMIT 10
  RETURN {
    doc: doc,
    similarity: similarity,
    daysOld: DATE_DIFF(doc.published, NOW(), "days"),
    finalScore: final_score
  }
```

---

## PostgreSQL-Kompatibilität

### Syntax-Vergleich

| PostgreSQL | AQL | Beschreibung |
|------------|-----|--------------|
| `NOW()` | `NOW()` | ✅ Identisch |
| `CURRENT_DATE` | `CURRENT_DATE()` | ✅ Kompatibel |
| `INTERVAL '7 days'` | `DAYS(7)` oder `INTERVAL(7, "days")` | ✅ Äquivalent |
| `date + INTERVAL '1 month'` | `date + MONTHS(1)` | ✅ Äquivalent |
| `date_trunc('week', timestamp)` | `DATE_TRUNC(timestamp, "week")` | ✅ Kompatibel |
| `extract(year from date)` | `DATE_YEAR(date)` | ✅ Äquivalent |
| `age(birthdate)` | `AGE(birthdate)` | ✅ Identisch |

### Migration von PostgreSQL

**PostgreSQL:**
```sql
SELECT 
  date_trunc('month', order_date) as month,
  COUNT(*) as orders
FROM orders
WHERE order_date >= NOW() - INTERVAL '6 months'
GROUP BY month
ORDER BY month;
```

**AQL:**
```aql
FOR order IN orders
  FILTER order.order_date >= NOW() - MONTHS(6)
  COLLECT month = DATE_TRUNC(order.order_date, "month")
  AGGREGATE orders = COUNT(1)
  SORT month ASC
  RETURN { month, orders }
```

---

## Excel-Kompatibilität

### Funktions-Mapping

| Excel | AQL | Beschreibung |
|-------|-----|--------------|
| `TODAY()` | `TODAY()` | ✅ Identisch |
| `NOW()` | `NOW()` | ✅ Identisch |
| `DAYS(start, end)` | `DATE_DIFF(start, end, "days")` | ✅ Äquivalent |
| `WORKDAY(start, days)` | `WORKDAYS_ADD(start, days, holidays)` | ✅ Erweitert |
| `NETWORKDAYS(start, end)` | `WORKDAYS(start, end, holidays)` | ✅ Erweitert |
| `YEAR(date)` | `DATE_YEAR(date)` | ✅ Äquivalent |
| `MONTH(date)` | `DATE_MONTH(date)` | ✅ Äquivalent |
| `DAY(date)` | `DATE_DAY(date)` | ✅ Äquivalent |

### Migration von Excel

**Excel:**
```excel
=WORKDAY(A2, 10)  // Lieferdatum nach 10 Arbeitstagen
=NETWORKDAYS(A2, B2)  // Arbeitstage zwischen Daten
```

**AQL:**
```aql
-- Lieferdatum mit Feiertagen
LET holidays = HOLIDAYS("DE_2024")
LET deliveryDate = WORKDAYS_ADD(order.date, 10, holidays)

-- Arbeitstage zwischen Daten
LET workdays = WORKDAYS(start_date, end_date, holidays)
```

---

## Performance & Best Practices

### 1. Interval-Funktionen bevorzugen

**Schlecht (langsam):**
```aql
FILTER event.date >= DATE_SUBTRACT(NOW(), 7, "days")
```

**Gut (schnell):**
```aql
FILTER event.date >= NOW() - DAYS(7)
```

### 2. Indizes auf Zeitfeldern

```aql
-- Index für effiziente Zeitbereich-Queries
CREATE INDEX events_timestamp ON events(timestamp)

-- Nutzt Index effizient
FOR event IN events
  FILTER event.timestamp >= NOW() - MONTHS(1)
  RETURN event
```

### 3. Arbeitstage-Cache

```aql
-- Holidays einmal laden und wiederverwenden
LET holidays = HOLIDAYS("DE_2024")

FOR order IN orders
  LET deliveryDate = WORKDAYS_ADD(order.date, 10, holidays)
  -- holidays wird wiederverwendet
  RETURN { order, deliveryDate }
```

### 4. DATE_TRUNC für Gruppierung

```aql
-- Effiziente Gruppierung nach Zeiteinheit
FOR event IN events
  COLLECT month = DATE_TRUNC(event.timestamp, "month")
  AGGREGATE count = COUNT(1)
  RETURN { month, count }
```

---

## Reservierte Wörter & Sprachumfang

### Aktuelle Situation (v1.3.0)

Die Zeit-Funktionen sind **Teil der 72 reservierten Keywords** in v1.3.0:
- `NOW` - Keyword für aktuelle Zeit
- Alle anderen Funktionen wie `DAYS`, `WORKDAYS`, `INTERVAL` sind **normale Funktionen** (nicht Keywords)

### Integration mit v1.3.1 Proposal

Die Zeit-Funktionen profitieren von den vorgeschlagenen OOP-Features:

#### 1. Mit User-Defined Functions
```aql
NAMESPACE company.calendar;

FUNCTION business_days_between(
  start: Date,
  end: Date,
  country: String = "DE"
) -> Int {
  LET holidays = HOLIDAYS(country + "_2024")
  RETURN WORKDAYS(start, end, holidays)
}

-- Verwendung
LET workdays = company.calendar::business_days_between(
  order.start,
  order.end,
  "DE"
)
```

#### 2. Mit User-Defined Types
```aql
TYPE DeliveryEstimate {
  order_date: Date,
  delivery_date: Date,
  workdays: Int,
  is_express: Bool
}

FUNCTION calculate_delivery(
  order_date: Date,
  is_express: Bool
) -> DeliveryEstimate {
  LET holidays = HOLIDAYS("DE_2024")
  LET days = is_express ? 3 : 10
  LET delivery = WORKDAYS_ADD(order_date, days, holidays)
  
  RETURN DeliveryEstimate {
    order_date: order_date,
    delivery_date: delivery,
    workdays: days,
    is_express: is_express
  }
}
```

#### 3. Mit Pipeline Operator
```aql
-- Elegante Zeitbereichs-Queries
LET last_month_data = events
  |> FILTER(_, e -> e.timestamp >= NOW() - MONTHS(1))
  |> MAP(_, e -> {
       event: e,
       daysAgo: DATE_DIFF(e.timestamp, NOW(), "days")
     })
  |> SORT(_, e -> e.daysAgo ASC);
```

---

## Sprachumfang-Optimierung

### Ziel: Geringer Sprachumfang für Multi-Model

ThemisDB erreicht **minimalen Sprachumfang** durch:

1. **Wiederverwendbare Funktionen** statt spezielle Syntax
   - Eine `DATE_DIFF` Funktion statt separater `DATEDIFF`, `TIMEDIFF`, etc.
   - Eine `INTERVAL` Funktion für alle Zeiteinheiten

2. **Funktionen statt Keywords**
   - `DAYS(7)` ist Funktion, nicht Keyword
   - `WORKDAYS()` ist Funktion, nicht Keyword
   - Nur `NOW` ist Keyword (SQL-kompatibel)

3. **Intelligente Defaults**
   - `DATE_DIFF` erkennt automatisch beste Einheit
   - `WORKDAYS` funktioniert ohne Feiertags-Kalender (Mo-Fr)

### Vergleich mit anderen Datenbanken

| Feature | PostgreSQL | MySQL | ThemisDB | Vorteil |
|---------|-----------|-------|----------|---------|
| Zeiteinheiten | 10+ Keywords | 8+ Keywords | 8 Funktionen | Weniger Keywords |
| Arbeitstage | Keine | Keine | Native | ✅ Eingebaut |
| Feiertage | Extension | Keine | Native | ✅ Built-in |
| Timeline | Zeitreihen-Extensions | Keine | Native | ✅ Integriert |

---

## Zusammenfassung für Sprachumfang-Diskussion

### Was ist in v1.3.0 (72 Keywords)

**Zeit-Funktionen:**
- ✅ ~45 Date/Time Funktionen implementiert
- ✅ INTERVAL-Syntax (DAYS, WEEKS, MONTHS, etc.)
- ✅ WORKDAYS mit Feiertags-Kalendern (6+ Länder)
- ✅ PostgreSQL & Excel kompatibel
- ✅ Multi-Model Integration (Timeline, Process, Geo, Vector, Graph)

**Keywords:** Nur 1 Keyword für Zeit: `NOW`

**Funktionen (keine Keywords):**
- DAYS, WEEKS, MONTHS, YEARS, HOURS, MINUTES, SECONDS
- INTERVAL, WORKDAYS, WORKDAYS_ADD, IS_WORKDAY
- HOLIDAYS, DATE_FORMAT, DATE_DIFF, DATE_TRUNC
- Und 30+ weitere

### Integration in Gesamtarchitektur

```
AQL Sprachumfang v1.3.0 (72 Keywords):
├── Core (8): FOR, LET, FILTER, COLLECT, SORT, LIMIT, RETURN, IN
├── Logical (3): AND, OR, NOT
├── Graph (4): OUTBOUND, INBOUND, ANY, SHORTEST_PATH
├── DDL (5): CREATE, DROP, COLLECTION, INDEX, VIEW
├── DML (6): INSERT, UPDATE, REPLACE, REMOVE, UPSERT, INTO
├── LLM (13): LLM, INFER, RAG, EMBED, MODEL, LORA, STATS, ...
├── Zeit (1): NOW
├── Options (13): OPTIONS, DISTINCT, ASC, DESC, FROM, TO, TOP, ...
├── Index Types (7): HASH, SKIPLIST, FULLTEXT, GEO, VECTOR, ...
├── Literals (3): null, true, false
└── Reserved (9): WITH, USING, WHERE, WHEN, ...

Funktionen (keine Keywords):
├── Date/Time (45): DAYS, INTERVAL, WORKDAYS, DATE_FORMAT, ...
├── Math (30): ABS, CEIL, FLOOR, SQRT, ...
├── String (20): CONCAT, SUBSTRING, UPPER, LOWER, ...
├── Array (20): MAP, FILTER, REDUCE, FLATTEN, ...
├── Geo (25): ST_Point, ST_Distance, ST_Within, ...
├── Vector (20): COSINE_SIMILARITY, DOT_PRODUCT, ...
├── Excel (30): VLOOKUP, SUMPRODUCT, PMT, ...
└── Security (15): HASH_SHA256, ENCRYPT_AES, ...
```

### Ergebnis

**Minimaler Sprachumfang erreicht:**
- ✅ 72 reservierte Keywords (sehr gering)
- ✅ ~360 Funktionen (wiederverwendbar, kein Keyword-Overhead)
- ✅ Alle Multi-Model Use Cases abgedeckt
- ✅ PostgreSQL & Excel kompatibel
- ✅ Timeline, Process, Geo, Vector, Graph, LLM, Vision - alles integriert

---

## Kontakt & Feedback

- **Zeit-Funktionen Doku:** `/docs/de/aql/aql_functions_reference.md` (Zeile 1950-2180)
- **v1.3.1 OOP Proposal:** `/docs/de/aql/AQL_OOP_EXTENSION_PROPOSAL.md`
- **Reserved Words:** `/docs/de/aql/AQL_RESERVED_WORDS_ANALYSIS.md`
- **Excel Functions:** `/docs/de/aql/AQL_EXCEL_FUNCTIONS_STATUS.md`
