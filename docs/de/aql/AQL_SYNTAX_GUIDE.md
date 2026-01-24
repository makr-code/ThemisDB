# AQL Syntax Guide - ThemisDB v1.4

**Version:** 1.4.0  
**Status:** ✅ Produktionsreif  
**Aktualisiert:** Januar 2026

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [Grundlegende Syntax](#grundlegende-syntax)
- [Datentypen](#datentypen)
- [Operatoren](#operatoren)
- [Query-Struktur](#query-struktur)
- [Funktionen](#funktionen)
- [Graph-Operationen](#graph-operationen)
- [Vector-Operationen](#vector-operationen)
- [Subqueries](#subqueries)
- [Window Functions](#window-functions)
- [Vollständige Beispiele](#vollständige-beispiele)

---

## Übersicht

**AQL (Advanced Query Language)** ist die native Query-Sprache von ThemisDB. Sie kombiniert SQL-ähnliche Syntax mit erweiterten Features für Multi-Model-Datenbanken (Relational, Document, Graph, Vector).

### AQL Features

- ✅ **Deklarative Syntax**: SQL-ähnlich und lesbar
- ✅ **Multi-Model**: Eine Sprache für alle Datenmodelle
- ✅ **Type-Safe**: Starke Typisierung mit Type Inference
- ✅ **Performance**: Query Optimizer mit Cost-based Execution
- ✅ **Composable**: Subqueries und CTEs (WITH-Klausel)

---

## Grundlegende Syntax

### Einfachste Query

```aql
FOR doc IN users
  RETURN doc
```

**Bedeutung:** Alle Dokumente aus der Kollektion `users` zurückgeben.

### Query mit Filter

```aql
FOR doc IN users
  FILTER doc.age > 25
  RETURN doc
```

**Bedeutung:** Nur Dokumente mit `age > 25` zurückgeben.

### Query mit Sortierung und Limit

```aql
FOR doc IN users
  FILTER doc.age > 25
  SORT doc.age DESC
  LIMIT 10
  RETURN doc
```

**Bedeutung:** Die 10 ältesten Benutzer über 25 Jahre zurückgeben.

### Projektion

```aql
FOR doc IN users
  RETURN {
    name: doc.name,
    email: doc.email,
    age: doc.age
  }
```

**Bedeutung:** Nur spezifische Felder zurückgeben (keine kompletten Dokumente).

---

## Datentypen

### Primitive Typen

```aql
// Numbers
LET integer = 42
LET float = 3.14159
LET scientific = 1.23e-10

// Strings
LET str = "Hello World"
LET escaped = "Line 1\nLine 2"

// Booleans
LET isActive = true
LET isDeleted = false

// Null
LET empty = null
```

### Collection Types

```aql
// Arrays
LET numbers = [1, 2, 3, 4, 5]
LET mixed = [1, "two", true, null]

// Objects
LET person = {
  name: "John Doe",
  age: 30,
  email: "john@example.com"
}

// Nested
LET complex = {
  user: {
    name: "Alice",
    tags: ["admin", "developer"]
  }
}
```

### Date/Time

```aql
// Current timestamp
LET now = DATE_NOW()

// ISO 8601 string
LET date = DATE_ISO8601("2026-01-24T14:00:00Z")

// Unix timestamp
LET timestamp = DATE_TIMESTAMP(1706104800)
```

---

## Operatoren

### Vergleichsoperatoren

```aql
doc.age == 30        // Gleich
doc.age != 30        // Ungleich
doc.age > 30         // Größer
doc.age >= 30        // Größer oder gleich
doc.age < 30         // Kleiner
doc.age <= 30        // Kleiner oder gleich
doc.age IN [25, 30]  // In Liste
doc.age NOT IN [25]  // Nicht in Liste
```

### Logische Operatoren

```aql
doc.age > 25 AND doc.status == "active"
doc.role == "admin" OR doc.role == "moderator"
NOT doc.isDeleted
```

### String-Operatoren

```aql
doc.name LIKE "John%"          // Beginnt mit "John"
doc.email LIKE "%@example.com" // Endet mit "@example.com"
doc.name =~ "^[A-Z]"           // Regex Match
```

### Arithmetische Operatoren

```aql
LET sum = 10 + 5        // Addition
LET diff = 10 - 5       // Subtraktion
LET product = 10 * 5    // Multiplikation
LET quotient = 10 / 5   // Division
LET remainder = 10 % 3  // Modulo
```

### Array-Operatoren

```aql
"admin" IN doc.roles                    // Element in Array
"admin" NOT IN doc.roles                // Element nicht in Array
doc.tags ALL == ["admin", "developer"]  // Alle Elemente vorhanden
doc.tags ANY == ["admin"]               // Mindestens ein Element
doc.tags NONE == ["banned"]             // Kein Element vorhanden
```

---

## Query-Struktur

### FOR - Iteration

```aql
FOR doc IN collection
  RETURN doc
```

**Nested FOR:**
```aql
FOR user IN users
  FOR project IN projects
    FILTER project.assignee == user._id
    RETURN {user, project}
```

### FILTER - Bedingungen

```aql
FOR doc IN users
  FILTER doc.age > 25 AND doc.status == "active"
  RETURN doc
```

**Multiple FILTER:**
```aql
FOR doc IN users
  FILTER doc.age > 25
  FILTER doc.status == "active"
  FILTER doc.email LIKE "%@company.com"
  RETURN doc
```

### LET - Variablen

```aql
FOR doc IN users
  LET fullName = CONCAT(doc.firstName, " ", doc.lastName)
  LET isAdult = doc.age >= 18
  RETURN {fullName, isAdult, age: doc.age}
```

### SORT - Sortierung

```aql
// Einzelnes Feld
FOR doc IN users
  SORT doc.age DESC
  RETURN doc

// Multiple Felder
FOR doc IN users
  SORT doc.age DESC, doc.name ASC
  RETURN doc
```

### LIMIT - Begrenzung

```aql
// Limit nur
FOR doc IN users
  LIMIT 10
  RETURN doc

// Offset und Limit (Pagination)
FOR doc IN users
  LIMIT 20, 10  // Skip 20, return 10
  RETURN doc
```

### COLLECT - Gruppierung

```aql
// Gruppierung mit Zählung
FOR doc IN users
  COLLECT age = doc.age WITH COUNT INTO count
  RETURN {age, count}

// Gruppierung mit Aggregation
FOR doc IN sales
  COLLECT product = doc.product
  AGGREGATE total = SUM(doc.amount)
  RETURN {product, total}
```

### RETURN - Rückgabe

```aql
// Komplette Dokumente
FOR doc IN users
  RETURN doc

// Projektion
FOR doc IN users
  RETURN {
    name: doc.name,
    email: doc.email
  }

// Expression
FOR doc IN users
  RETURN CONCAT(doc.firstName, " ", doc.lastName)
```

---

## Funktionen

### String-Funktionen

```aql
CONCAT("Hello", " ", "World")           // "Hello World"
CONCAT_SEPARATOR(", ", "A", "B", "C")  // "A, B, C"
LOWER("HELLO")                          // "hello"
UPPER("hello")                          // "HELLO"
LENGTH("hello")                         // 5
SUBSTRING("hello", 1, 3)                // "ell"
TRIM("  hello  ")                       // "hello"
SPLIT("a,b,c", ",")                     // ["a", "b", "c"]
REPLACE("hello world", "world", "AQL")  // "hello AQL"
```

### Numeric-Funktionen

```aql
ABS(-5)                  // 5
CEIL(3.2)                // 4
FLOOR(3.8)               // 3
ROUND(3.14159, 2)        // 3.14
SQRT(16)                 // 4
POW(2, 3)                // 8
MIN(1, 2, 3)             // 1
MAX(1, 2, 3)             // 3
SUM([1, 2, 3])           // 6
AVG([1, 2, 3])           // 2
```

### Array-Funktionen

```aql
LENGTH([1, 2, 3])                          // 3
PUSH([1, 2], 3)                            // [1, 2, 3]
POP([1, 2, 3])                             // [1, 2]
APPEND([1, 2], [3, 4])                     // [1, 2, 3, 4]
FIRST([1, 2, 3])                           // 1
LAST([1, 2, 3])                            // 3
NTH([1, 2, 3], 1)                          // 2
POSITION([1, 2, 3], 2)                     // 1
REVERSE([1, 2, 3])                         // [3, 2, 1]
UNIQUE([1, 2, 2, 3, 3])                    // [1, 2, 3]
UNION([1, 2], [2, 3])                      // [1, 2, 3]
INTERSECTION([1, 2, 3], [2, 3, 4])         // [2, 3]
MINUS([1, 2, 3], [2])                      // [1, 3]
FLATTEN([[1, 2], [3, 4]])                  // [1, 2, 3, 4]
```

### Date-Funktionen

```aql
DATE_NOW()                                 // Aktueller Timestamp
DATE_ISO8601("2026-01-24T14:00:00Z")       // Parse ISO 8601
DATE_TIMESTAMP(1706104800)                 // Unix Timestamp zu Date
DATE_YEAR(date)                            // Jahr
DATE_MONTH(date)                           // Monat
DATE_DAY(date)                             // Tag
DATE_HOUR(date)                            // Stunde
DATE_MINUTE(date)                          // Minute
DATE_SECOND(date)                          // Sekunde
DATE_DIFF(date1, date2, "days")            // Differenz in Tagen
DATE_ADD(date, 7, "days")                  // 7 Tage addieren
DATE_FORMAT(date, "%Y-%m-%d %H:%M:%S")     // Formatierung
```

### Aggregation-Funktionen

```aql
COUNT(expression)          // Anzahl
SUM(expression)           // Summe
AVG(expression)           // Durchschnitt
MIN(expression)           // Minimum
MAX(expression)           // Maximum
VARIANCE(expression)      // Varianz
STDDEV(expression)        // Standardabweichung
MEDIAN(expression)        // Median
PERCENTILE(expr, 95)      // 95. Perzentil
```

### Type-Check-Funktionen

```aql
IS_NULL(value)            // Ist null?
IS_BOOL(value)            // Ist boolean?
IS_NUMBER(value)          // Ist number?
IS_STRING(value)          // Ist string?
IS_ARRAY(value)           // Ist array?
IS_OBJECT(value)          // Ist object?
TO_NUMBER(value)          // Zu Number konvertieren
TO_STRING(value)          // Zu String konvertieren
TO_BOOL(value)            // Zu Boolean konvertieren
TO_ARRAY(value)           // Zu Array konvertieren
```

---

## Graph-Operationen

### Einfacher Traversal

```aql
FOR vertex IN 1..3 OUTBOUND "users/john" edges
  RETURN vertex
```

**Bedeutung:** Traversiere 1-3 Hops ausgehend von "users/john" über "edges".

### Kürzester Pfad

```aql
FOR vertex, edge, path IN OUTBOUND SHORTEST_PATH
  "users/john" TO "users/alice"
  edges
  RETURN path
```

### All Paths

```aql
FOR path IN 1..5 OUTBOUND "users/john" edges
  OPTIONS {uniqueVertices: 'path'}
  RETURN path
```

### Named Graph

```aql
FOR vertex, edge IN 1..3 OUTBOUND "users/john"
  GRAPH "social_network"
  RETURN {vertex, edge}
```

### Filter während Traversal

```aql
FOR vertex, edge, path IN 1..3 OUTBOUND "users/john" edges
  PRUNE vertex.blocked == true
  FILTER vertex.age > 25
  RETURN vertex
```

---

## Vector-Operationen

### Vector Similarity Search

```aql
FOR doc IN documents
  LET similarity = COSINE_SIMILARITY(doc.embedding, @queryVector)
  FILTER similarity > 0.8
  SORT similarity DESC
  LIMIT 10
  RETURN {doc, similarity}
```

### Vector Index Usage

```aql
FOR doc IN documents
  OPTIONS {indexHint: "vector_idx"}
  FILTER VECTOR_DISTANCE(doc.embedding, @queryVector, "cosine") < 0.2
  SORT VECTOR_DISTANCE(doc.embedding, @queryVector, "cosine") ASC
  LIMIT 5
  RETURN doc
```

### Hybrid Search (Vector + Fulltext)

```aql
LET textResults = (
  FOR doc IN documents
    SEARCH ANALYZER(doc.content IN TOKENS(@query, "text_en"), "text_en")
    RETURN doc
)

LET vectorResults = (
  FOR doc IN documents
    FILTER VECTOR_DISTANCE(doc.embedding, @queryVector, "cosine") < 0.3
    RETURN doc
)

FOR doc IN UNION(textResults, vectorResults)
  RETURN DISTINCT doc
```

---

## Subqueries

### LET mit Subquery

```aql
FOR user IN users
  LET projects = (
    FOR project IN projects
      FILTER project.owner == user._id
      RETURN project
  )
  RETURN {
    user: user.name,
    projectCount: LENGTH(projects),
    projects: projects
  }
```

### Filter mit Subquery

```aql
FOR user IN users
  FILTER (
    FOR project IN projects
      FILTER project.owner == user._id AND project.status == "active"
      LIMIT 1
      RETURN 1
  ) != []
  RETURN user
```

### EXISTS-Pattern

```aql
FOR user IN users
  FILTER LENGTH(
    FOR project IN projects
      FILTER project.owner == user._id
      LIMIT 1
      RETURN 1
  ) > 0
  RETURN user
```

---

## Window Functions

### ROW_NUMBER

```aql
FOR doc IN users
  WINDOW w AS {
    PARTITION BY doc.department
    ORDER BY doc.salary DESC
  }
  RETURN {
    name: doc.name,
    department: doc.department,
    salary: doc.salary,
    rank: ROW_NUMBER() OVER w
  }
```

### RANK und DENSE_RANK

```aql
FOR doc IN sales
  WINDOW w AS {
    PARTITION BY doc.region
    ORDER BY doc.revenue DESC
  }
  RETURN {
    salesperson: doc.name,
    region: doc.region,
    revenue: doc.revenue,
    rank: RANK() OVER w,
    dense_rank: DENSE_RANK() OVER w
  }
```

### LAG und LEAD

```aql
FOR doc IN stock_prices
  SORT doc.date ASC
  WINDOW w AS {ORDER BY doc.date ASC}
  RETURN {
    date: doc.date,
    price: doc.price,
    previous: LAG(doc.price, 1) OVER w,
    next: LEAD(doc.price, 1) OVER w,
    change: doc.price - LAG(doc.price, 1) OVER w
  }
```

### SUM und AVG (Window)

```aql
FOR doc IN sales
  WINDOW w AS {
    PARTITION BY doc.product
    ORDER BY doc.date ASC
    ROWS BETWEEN 2 PRECEDING AND CURRENT ROW
  }
  RETURN {
    date: doc.date,
    product: doc.product,
    sales: doc.amount,
    movingAvg: AVG(doc.amount) OVER w,
    runningTotal: SUM(doc.amount) OVER w
  }
```

---

## Vollständige Beispiele

### Beispiel 1: E-Commerce Analytics

```aql
// Top 10 Produkte mit Umsatz und Bewertungen
FOR order IN orders
  COLLECT product = order.product_id
  AGGREGATE
    totalSales = SUM(order.amount),
    orderCount = COUNT(1)
  
  LET productInfo = DOCUMENT("products", product)
  LET avgRating = (
    FOR review IN reviews
      FILTER review.product_id == product
      RETURN review.rating
  )
  
  SORT totalSales DESC
  LIMIT 10
  
  RETURN {
    product: productInfo.name,
    totalSales: totalSales,
    orderCount: orderCount,
    avgRating: AVG(avgRating),
    reviewCount: LENGTH(avgRating)
  }
```

### Beispiel 2: Social Network Analysis

```aql
// Finde Influencer (Benutzer mit vielen Followern)
FOR user IN users
  LET followers = (
    FOR vertex IN 1..1 INBOUND user follows
      RETURN vertex
  )
  
  LET following = (
    FOR vertex IN 1..1 OUTBOUND user follows
      RETURN vertex
  )
  
  LET posts = (
    FOR post IN posts
      FILTER post.author == user._id
      RETURN post
  )
  
  LET engagement = (
    FOR post IN posts
      RETURN SUM([
        LENGTH(post.likes || []),
        LENGTH(post.comments || []),
        LENGTH(post.shares || [])
      ])
  )
  
  FILTER LENGTH(followers) > 1000
  
  SORT LENGTH(followers) DESC
  LIMIT 20
  
  RETURN {
    user: user.name,
    followerCount: LENGTH(followers),
    followingCount: LENGTH(following),
    postCount: LENGTH(posts),
    totalEngagement: SUM(engagement),
    engagementRate: SUM(engagement) / LENGTH(posts) / LENGTH(followers) * 100
  }
```

### Beispiel 3: Time-Series Analysis

```aql
// CPU-Auslastung mit gleitendem Durchschnitt
FOR metric IN metrics
  FILTER metric.type == "cpu_usage"
  SORT metric.timestamp ASC
  
  WINDOW w AS {
    ORDER BY metric.timestamp ASC
    ROWS BETWEEN 4 PRECEDING AND CURRENT ROW
  }
  
  LET movingAvg = AVG(metric.value) OVER w
  LET threshold = 80
  
  RETURN {
    timestamp: metric.timestamp,
    value: metric.value,
    movingAvg: movingAvg,
    alert: movingAvg > threshold ? "HIGH" : "NORMAL"
  }
```

### Beispiel 4: Recommendation Engine

```aql
// Produkt-Empfehlungen basierend auf ähnlichen Benutzern
LET targetUser = DOCUMENT("users/user_123")

// Finde ähnliche Benutzer
LET similarUsers = (
  FOR user IN users
    FILTER user._id != targetUser._id
    LET similarity = COSINE_SIMILARITY(
      targetUser.preferences_vector,
      user.preferences_vector
    )
    FILTER similarity > 0.7
    SORT similarity DESC
    LIMIT 10
    RETURN {user, similarity}
)

// Sammle Produkte, die ähnliche Benutzer mögen
LET recommendedProducts = (
  FOR su IN similarUsers
    FOR purchase IN purchases
      FILTER purchase.user_id == su.user._id
      // Nicht Produkte, die Zielbenutzer bereits hat
      FILTER !(purchase.product_id IN targetUser.purchased_products)
      RETURN {
        product_id: purchase.product_id,
        weight: su.similarity
      }
)

// Aggregiere und ranke Produkte
FOR rec IN recommendedProducts
  COLLECT productId = rec.product_id
  AGGREGATE score = SUM(rec.weight), count = COUNT(1)
  
  LET product = DOCUMENT("products", productId)
  
  SORT score DESC
  LIMIT 5
  
  RETURN {
    product: product.name,
    score: score,
    recommendedBy: count,
    category: product.category
  }
```

---

## WITH (CTEs - Common Table Expressions)

### Einfaches WITH

```aql
WITH users_active = (
  FOR user IN users
    FILTER user.status == "active"
    RETURN user
)

FOR user IN users_active
  FILTER user.age > 25
  RETURN user
```

### Multiple CTEs

```aql
WITH
  active_users = (
    FOR user IN users
      FILTER user.status == "active"
      RETURN user
  ),
  premium_products = (
    FOR product IN products
      FILTER product.tier == "premium"
      RETURN product
  )

FOR user IN active_users
  FOR product IN premium_products
    FILTER product.category IN user.interests
    RETURN {user, product}
```

### Recursive CTE (Hierarchien)

```aql
WITH RECURSIVE hierarchy AS (
  // Basis: Root-Elemente
  FOR doc IN categories
    FILTER doc.parent_id == null
    RETURN doc
  
  UNION
  
  // Rekursion: Kinder
  FOR doc IN categories
    FOR parent IN hierarchy
      FILTER doc.parent_id == parent._id
      RETURN doc
)

FOR cat IN hierarchy
  RETURN cat
```

---

## Siehe auch

- [AQL Best Practices](AQL_BEST_PRACTICES.md)
- [AQL Performance Guide](AQL_PERFORMANCE_GUIDE.md)
- [AQL Function Reference](aql_functions_reference.md)
- [Query Optimizer Details](QUERY_OPTIMIZER.md)
- [REST API Specification](../apis/REST_API_SPECIFICATION.md)
