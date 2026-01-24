# AQL Best Practices - ThemisDB v1.4

**Version:** 1.4.0  
**Status:** ✅ Produktionsreif  
**Aktualisiert:** Januar 2026

---

## 📑 Inhaltsverzeichnis

- [Übersicht](#übersicht)
- [Query-Struktur](#query-struktur)
- [Index-Nutzung](#index-nutzung)
- [Performance-Optimierung](#performance-optimierung)
- [Sicherheit](#sicherheit)
- [Edge Cases](#edge-cases)
- [Häufige Fehler](#häufige-fehler)
- [Testing und Debugging](#testing-und-debugging)

---

## Übersicht

Dieser Guide enthält Best Practices für das Schreiben von effizienten, sicheren und wartbaren AQL-Queries in ThemisDB.

---

## Query-Struktur

### ✅ DO: Früh filtern

```aql
// ✅ Gut: Filter VOR Sortierung
FOR doc IN users
  FILTER doc.age > 25
  SORT doc.name ASC
  LIMIT 10
  RETURN doc
```

```aql
// ❌ Schlecht: Filter NACH Sortierung
FOR doc IN users
  SORT doc.name ASC
  FILTER doc.age > 25
  LIMIT 10
  RETURN doc
```

**Warum:** Filtern reduziert die Datenmenge früh, was nachfolgende Operationen beschleunigt.

### ✅ DO: Nur benötigte Felder zurückgeben

```aql
// ✅ Gut: Projektion
FOR doc IN users
  RETURN {
    name: doc.name,
    email: doc.email
  }
```

```aql
// ❌ Schlecht: Ganze Dokumente
FOR doc IN users
  RETURN doc
```

**Warum:** Reduziert Netzwerk-Traffic und Serialisierung-Overhead.

### ✅ DO: LIMIT früh verwenden

```aql
// ✅ Gut: LIMIT direkt nach FILTER
FOR doc IN users
  FILTER doc.status == "active"
  LIMIT 100
  SORT doc.created_at DESC
  LIMIT 10
  RETURN doc
```

**Warum:** Begrenzt Daten früh, bevor teure Operationen (wie SORT) ausgeführt werden.

### ✅ DO: LET für komplexe Expressions verwenden

```aql
// ✅ Gut: Wiederverwendbare Expression
FOR doc IN users
  LET fullName = CONCAT(doc.firstName, " ", doc.lastName)
  LET isAdult = doc.age >= 18
  FILTER isAdult
  RETURN {
    fullName: fullName,
    email: doc.email
  }
```

```aql
// ❌ Schlecht: Redundante Berechnung
FOR doc IN users
  FILTER doc.age >= 18
  RETURN {
    fullName: CONCAT(doc.firstName, " ", doc.lastName),
    email: doc.email,
    greeting: CONCAT("Hello ", CONCAT(doc.firstName, " ", doc.lastName))
  }
```

### ❌ DON'T: Nested FOR ohne Filter

```aql
// ❌ Schlecht: Kartesisches Produkt
FOR user IN users
  FOR project IN projects
    RETURN {user, project}
```

```aql
// ✅ Gut: Mit Filter
FOR user IN users
  FOR project IN projects
    FILTER project.owner == user._id
    RETURN {user, project}
```

**Warum:** Ohne Filter entsteht ein kartesisches Produkt (users × projects), was zu Millionen Ergebnissen führen kann.

---

## Index-Nutzung

### ✅ DO: Indizes für häufige Queries erstellen

```aql
// Index erstellen
CREATE INDEX idx_user_email ON users(email) HASH UNIQUE

// Query nutzt Index automatisch
FOR doc IN users
  FILTER doc.email == "john@example.com"
  RETURN doc
```

### ✅ DO: Composite Indexes für Multi-Field Queries

```aql
// Composite Index
CREATE INDEX idx_user_status_age ON users(status, age) BTREE

// Query nutzt Index
FOR doc IN users
  FILTER doc.status == "active" AND doc.age > 25
  RETURN doc
```

### ✅ DO: Index Hints verwenden (bei Bedarf)

```aql
FOR doc IN users
  OPTIONS {indexHint: "idx_user_email"}
  FILTER doc.email == @email
  RETURN doc
```

**Wann:** Wenn der Query Optimizer den falschen Index wählt.

### ❌ DON'T: Indizes auf häufig geänderten Feldern

```aql
// ❌ Schlecht: Index auf Timestamp, das bei jedem Update ändert
CREATE INDEX idx_updated_at ON users(updated_at)
```

**Warum:** Index-Wartung wird teuer bei häufigen Updates.

### ✅ DO: Partial Indexes für Subset-Queries

```aql
// Partial Index nur für aktive Benutzer
CREATE INDEX idx_active_users ON users(email)
  WHERE status == "active"
  
// Query auf aktive Benutzer
FOR doc IN users
  FILTER doc.status == "active" AND doc.email LIKE "%@company.com"
  RETURN doc
```

---

## Performance-Optimierung

### ✅ DO: EXPLAIN verwenden für Query-Analyse

```aql
EXPLAIN
FOR doc IN users
  FILTER doc.age > 25
  SORT doc.name ASC
  LIMIT 10
  RETURN doc
```

**Output:**
```json
{
  "plan": {
    "nodes": [
      {"type": "IndexNode", "index": "idx_age"},
      {"type": "FilterNode"},
      {"type": "SortNode"},
      {"type": "LimitNode"}
    ],
    "estimatedCost": 150,
    "estimatedNrItems": 10
  }
}
```

### ✅ DO: PROFILE für Execution-Statistiken

```aql
PROFILE
FOR doc IN users
  FILTER doc.age > 25
  RETURN doc
```

**Output:**
```json
{
  "stats": {
    "executionTime": 45.2,
    "scanned": 1000,
    "filtered": 250,
    "httpRequests": 0
  }
}
```

### ✅ DO: Batch Operations für große Datenmengen

```aql
// ✅ Gut: Batch Update
FOR doc IN users
  FILTER doc.status == "pending"
  LIMIT 1000
  UPDATE doc WITH {status: "active"} IN users
  OPTIONS {waitForSync: false}
```

```aql
// ❌ Schlecht: Einzelne Updates
FOR doc IN users
  FILTER doc.status == "pending"
  UPDATE doc WITH {status: "active"} IN users
```

### ✅ DO: Subqueries minimieren

```aql
// ❌ Schlecht: Subquery in jedem Iteration
FOR user IN users
  LET projects = (
    FOR project IN projects
      FILTER project.owner == user._id
      RETURN project
  )
  RETURN {user, projects}
```

```aql
// ✅ Besser: JOIN mit COLLECT
FOR user IN users
  FOR project IN projects
    FILTER project.owner == user._id
    COLLECT u = user INTO projectList = project
    RETURN {user: u, projects: projectList}
```

### ✅ DO: Streaming für große Ergebnismengen

```aql
// Enable streaming
FOR doc IN large_collection
  OPTIONS {stream: true}
  RETURN doc
```

**Warum:** Reduziert Memory-Verbrauch, da Ergebnisse schrittweise zurückgegeben werden.

---

## Sicherheit

### ✅ DO: Bind Variables verwenden

```aql
// ✅ Gut: Bind Variables (AQL Injection sicher)
FOR doc IN users
  FILTER doc.email == @email
  RETURN doc
```

**Bind Variables:**
```json
{
  "email": "john@example.com"
}
```

```aql
// ❌ Schlecht: String Concatenation (AQL Injection!)
LET email = "john@example.com"
FOR doc IN users
  FILTER doc.email == email
  RETURN doc
```

### ✅ DO: Input Validation

```javascript
// Server-side validation
function validateEmail(email) {
  if (typeof email !== 'string') {
    throw new Error('Email must be a string');
  }
  if (!/^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(email)) {
    throw new Error('Invalid email format');
  }
  return email;
}

const query = `
  FOR doc IN users
    FILTER doc.email == @email
    RETURN doc
`;

const bindVars = {
  email: validateEmail(userInput)
};
```

### ✅ DO: Least Privilege Principle

```aql
// User hat nur READ-Rechte auf 'users' Kollektion
FOR doc IN users
  FILTER doc._id == @userId
  RETURN doc
```

**Warum:** Minimiert Schaden bei kompromittierten Credentials.

### ❌ DON'T: Sensible Daten in Queries loggen

```javascript
// ❌ Schlecht
console.log(`Query: FOR doc IN users FILTER doc.password == "${password}"`);

// ✅ Gut
console.log(`Query executed with bind variables`);
```

---

## Edge Cases

### Handle NULL Values

```aql
// ✅ Gut: Explizite NULL-Checks
FOR doc IN users
  FILTER doc.age != null AND doc.age > 25
  RETURN doc
```

```aql
// Alternative: COALESCE
FOR doc IN users
  LET age = COALESCE(doc.age, 0)
  FILTER age > 25
  RETURN doc
```

### Handle Empty Arrays

```aql
// ✅ Gut: Check Array Length
FOR doc IN users
  FILTER LENGTH(doc.tags || []) > 0
  RETURN doc
```

### Handle Missing Fields

```aql
// ✅ Gut: HAS() Function
FOR doc IN users
  FILTER HAS(doc, "email") AND doc.email != null
  RETURN doc
```

### Division by Zero

```aql
// ✅ Gut: Guard gegen Division durch Zero
FOR doc IN metrics
  LET avg = doc.total > 0 ? doc.sum / doc.total : 0
  RETURN {doc, avg}
```

### Case-Insensitive Search

```aql
// ✅ Gut: LOWER() für Case-Insensitive Match
FOR doc IN users
  FILTER LOWER(doc.email) == LOWER(@email)
  RETURN doc
```

---

## Häufige Fehler

### 1. COLLECT ohne INTO

```aql
// ❌ Fehler: Gruppierte Daten verloren
FOR doc IN sales
  COLLECT product = doc.product
  RETURN product
```

```aql
// ✅ Richtig: Mit INTO
FOR doc IN sales
  COLLECT product = doc.product INTO sales = doc
  RETURN {product, sales}
```

### 2. Falsche LIMIT Verwendung

```aql
// ❌ Fehler: LIMIT vor SORT
FOR doc IN users
  LIMIT 10
  SORT doc.age DESC
  RETURN doc
```

```aql
// ✅ Richtig: SORT vor finalem LIMIT
FOR doc IN users
  SORT doc.age DESC
  LIMIT 10
  RETURN doc
```

### 3. Unnötige DISTINCT

```aql
// ❌ Langsam: DISTINCT auf großer Datenmenge
FOR doc IN large_collection
  RETURN DISTINCT doc.category
```

```aql
// ✅ Schneller: COLLECT für Unique Values
FOR doc IN large_collection
  COLLECT category = doc.category
  RETURN category
```

### 4. Vergessene Indexes

```aql
// ❌ Langsam: Full Collection Scan
FOR doc IN users
  FILTER doc.email == @email
  RETURN doc
```

**Solution:** Index erstellen
```aql
CREATE INDEX idx_email ON users(email) HASH
```

### 5. Zu viele Subqueries

```aql
// ❌ Sehr langsam: N+1 Problem
FOR user IN users
  LET projects = (FOR p IN projects FILTER p.owner == user._id RETURN p)
  LET tasks = (FOR t IN tasks FILTER t.assignee == user._id RETURN t)
  LET comments = (FOR c IN comments FILTER c.author == user._id RETURN c)
  RETURN {user, projects, tasks, comments}
```

**Solution:** Denormalisieren oder separate Queries mit JOIN

---

## Testing und Debugging

### Unit Tests für Queries

```javascript
// Jest Test
describe('User Queries', () => {
  test('should find users by age', async () => {
    const query = `
      FOR doc IN users
        FILTER doc.age > @minAge
        SORT doc.age ASC
        RETURN doc
    `;
    
    const result = await db.query(query, {minAge: 25});
    
    expect(result.length).toBeGreaterThan(0);
    expect(result[0].age).toBeGreaterThan(25);
  });
});
```

### Query Debugging

```aql
// 1. Check intermediate results
FOR doc IN users
  LET step1 = doc.age > 25
  LET step2 = doc.status == "active"
  FILTER step1 AND step2
  RETURN {
    doc,
    debug: {step1, step2}
  }
```

```aql
// 2. Use WARN() for logging
FOR doc IN users
  LET _ = WARN("Processing user:", doc._id)
  FILTER doc.age > 25
  RETURN doc
```

### Performance Testing

```javascript
// Benchmark
const iterations = 1000;
const startTime = Date.now();

for (let i = 0; i < iterations; i++) {
  await db.query(query, bindVars);
}

const endTime = Date.now();
const avgTime = (endTime - startTime) / iterations;

console.log(`Average query time: ${avgTime}ms`);
```

---

## Performance Checklist

- [ ] Frühe FILTER Klauseln verwenden
- [ ] Nur benötigte Felder zurückgeben (Projektion)
- [ ] Indizes für Filter-Felder erstellen
- [ ] EXPLAIN/PROFILE für Query-Analyse nutzen
- [ ] Bind Variables für Security verwenden
- [ ] NULL und Edge Cases behandeln
- [ ] Subqueries minimieren
- [ ] Batch Operations für große Updates
- [ ] LIMIT sinnvoll einsetzen
- [ ] Streaming für große Ergebnisse aktivieren

---

## Query-Patterns

### Pattern: Pagination

```aql
// Cursor-based Pagination
FOR doc IN users
  FILTER doc._id > @cursor
  SORT doc._id ASC
  LIMIT @pageSize
  RETURN doc
```

### Pattern: Top-N per Group

```aql
// Top 3 Produkte pro Kategorie
FOR doc IN products
  COLLECT category = doc.category INTO products = doc
  LET topProducts = (
    FOR p IN products
      SORT p.sales DESC
      LIMIT 3
      RETURN p
  )
  RETURN {category, topProducts}
```

### Pattern: Conditional Aggregation

```aql
FOR doc IN orders
  COLLECT date = DATE_FORMAT(doc.created_at, "%Y-%m-%d")
  AGGREGATE
    total = SUM(doc.amount),
    highValue = SUM(doc.amount > 100 ? doc.amount : 0),
    lowValue = SUM(doc.amount <= 100 ? doc.amount : 0)
  RETURN {date, total, highValue, lowValue}
```

### Pattern: Recursive Hierarchies

```aql
WITH RECURSIVE tree AS (
  FOR doc IN categories
    FILTER doc.parent_id == null
    RETURN {
      id: doc._id,
      name: doc.name,
      level: 0,
      path: [doc._id]
    }
  
  UNION
  
  FOR doc IN categories
    FOR parent IN tree
      FILTER doc.parent_id == parent.id
      RETURN {
        id: doc._id,
        name: doc.name,
        level: parent.level + 1,
        path: APPEND(parent.path, doc._id)
      }
)

FOR node IN tree
  SORT LENGTH(node.path) ASC
  RETURN node
```

---

## Siehe auch

- [AQL Syntax Guide](AQL_SYNTAX_GUIDE.md)
- [AQL Performance Guide](AQL_PERFORMANCE_GUIDE.md)
- [Query Optimizer Documentation](QUERY_OPTIMIZER.md)
- [Index Management](../architecture/indexes.md)
- [Security Best Practices](../security/best_practices.md)
