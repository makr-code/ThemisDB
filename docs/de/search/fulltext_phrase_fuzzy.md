# Enhanced Fulltext Search: Phrase and Fuzzy Matching

**Stand:** 22. April 2026  
**Version:** 1.4.1  
**Kategorie:** Search

---

**Status:** ✅ Implementiert – Phrase Matching und Fuzzy Search

## Übersicht

ThemisDB bietet drei Arten der Volltextsuche:

1. **FULLTEXT()** - BM25-basierte Relevanz-Suche (Standard)
2. **PHRASE()** - Exakte Phrasen-Suche mit Positionsprüfung
3. **FUZZY()** - Ungefähre Suche mit Levenshtein-Distanz

## 1. FULLTEXT - BM25 Relevanz-Suche

### AQL Syntax
```aql
FOR doc IN articles
  FILTER FULLTEXT(doc.content, "machine learning", 100)
  RETURN doc
```

### Parameter
- `column` - Spalte für die Suche
- `query` - Suchbegriff(e) (mehrere Wörter mit AND-Logik)
- `limit` (optional) - Max. Anzahl Ergebnisse (default: 1000)

### Features
- **BM25 Scoring**: Relevanz-basiertes Ranking
- **AND-Logik**: Alle Tokens müssen vorkommen
- **Stemming**: Optional (EN/DE)
- **Stopwords**: Optional filtern
- **Quoted Phrases**: `"deep learning"` für exakte Phrasen

### Beispiel
```aql
FOR doc IN articles
  FILTER FULLTEXT(doc.content, "neural network optimization")
  RETURN {
    id: doc._key,
    title: doc.title,
    content: doc.content
  }
```

---

## 2. PHRASE - Exakte Phrasen-Suche

### AQL Syntax
```aql
FOR doc IN articles
  FILTER PHRASE(doc.content, "machine learning algorithm")
  RETURN doc
```

### Parameter
- `column` - Spalte für die Suche
- `phrase` - Exakte Phrase als String
- `limit` (optional) - Max. Anzahl Ergebnisse (default: 1000)

### Features
- **Exakte Übereinstimmung**: Phrase muss genau in dieser Reihenfolge vorkommen
- **Case-Insensitive**: Groß-/Kleinschreibung wird ignoriert
- **Position-Aware**: Prüft tatsächliche Wortposition im Dokument
- **Tokenisierung**: Nutzt dieselbe Config wie der Index (Stemming, Stopwords)

### Beispiel
```aql
// Findet nur Dokumente mit exakter Phrase
FOR doc IN articles
  FILTER PHRASE(doc.content, "deep learning for computer vision")
  RETURN {
    id: doc._key,
    title: doc.title,
    excerpt: SUBSTRING(doc.content, 0, 200)
  }
```

### Vergleich FULLTEXT vs. PHRASE

| Query | FULLTEXT | PHRASE |
|-------|----------|--------|
| "machine learning" | ✅ Findet "machine" AND "learning" überall | ✅ Findet nur "machine learning" als Phrase |
| "deep learning neural" | ✅ Findet alle 3 Wörter irgendwo | ❌ Muss exakte Reihenfolge haben |
| Scoring | BM25 Relevanz-Score | Einfacher Score (1.0) |

---

## 3. FUZZY - Ungefähre Suche

### AQL Syntax
```aql
FOR doc IN articles
  FILTER FUZZY(doc.content, "machene", 2, 50)
  RETURN doc
```

### Parameter
- `column` - Spalte für die Suche
- `query` - Suchbegriff (kann Tippfehler enthalten)
- `maxDistance` (optional) - Max. Levenshtein-Distanz (default: 2)
- `limit` (optional) - Max. Anzahl Ergebnisse (default: 1000)

### Features
- **Levenshtein-Distanz**: Misst Ähnlichkeit zwischen Strings
- **Fehlertoleranz**: Findet Wörter trotz Rechtschreibfehlern
- **Similarity Scoring**: Höhere Scores für nähere Übereinstimmungen
- **Edit Operations**: Ersetzung, Einfügung, Löschung

### Levenshtein-Distanz Beispiele

| Query | Target | Distance | Matched? (maxDist=2) |
|-------|--------|----------|----------------------|
| "learning" | "learning" | 0 | ✅ Exakt |
| "lerning" | "learning" | 1 | ✅ 1 fehlendes Zeichen |
| "lernig" | "learning" | 1 | ✅ 1 falsches Zeichen |
| "machene" | "machine" | 2 | ✅ 2 Editier-Operationen |
| "algoritm" | "algorithm" | 1 | ✅ 1 fehlendes Zeichen |
| "netwrk" | "network" | 1 | ✅ 1 fehlendes Zeichen |
| "qwerty" | "learning" | 8 | ❌ Zu große Distanz |

### Scoring
- Score = `1.0 / (1 + distance)`
- Exakte Übereinstimmung: Score = 1.0
- Distance 1: Score = 0.5
- Distance 2: Score = 0.33

### Beispiele

#### Tippfehler korrigieren
```aql
// Findet "machine" trotz Tippfehler
FOR doc IN articles
  FILTER FUZZY(doc.content, "machene", 2)
  RETURN {
    id: doc._key,
    title: doc.title
  }
```

#### Rechtschreibvarianten
```aql
// Findet "optimization" und "optimisation"
FOR doc IN articles
  FILTER FUZZY(doc.content, "optimisation", 1)
  RETURN doc
```

#### Kombiniert mit anderen Filtern
```aql
// Fuzzy Search + strukturierte Filter
FOR doc IN articles
  FILTER FUZZY(doc.content, "neural netwrk", 2)
  FILTER doc.published == true
  FILTER doc.year >= 2020
  RETURN doc
```

---

## HTTP API Endpoints

### POST /search/phrase
```bash
curl -X POST http://localhost:8080/search/phrase \
  -H "Content-Type: application/json" \
  -d '{
    "table": "articles",
    "column": "content",
    "phrase": "machine learning algorithm",
    "limit": 50
  }'
```

**Response:**
```json
{
  "count": 12,
  "table": "articles",
  "column": "content",
  "phrase": "machine learning algorithm",
  "results": [
    {"pk": "art_123", "score": 1.0},
    {"pk": "art_456", "score": 1.0}
  ]
}
```

### POST /search/fuzzy
```bash
curl -X POST http://localhost:8080/search/fuzzy \
  -H "Content-Type: application/json" \
  -d '{
    "table": "articles",
    "column": "content",
    "query": "machene lerning",
    "maxDistance": 2,
    "limit": 50
  }'
```

**Response:**
```json
{
  "count": 28,
  "table": "articles",
  "column": "content",
  "query": "machene lerning",
  "maxDistance": 2,
  "results": [
    {"pk": "art_123", "score": 0.5},
    {"pk": "art_789", "score": 0.33}
  ]
}
```

---

## Performance-Überlegungen

### FULLTEXT
- **Schnellste Option** für Standardsuchen
- O(n log n) Sortierung nach Score
- Inverted Index-Lookup ist sehr effizient

### PHRASE
- **Moderate Performance**
- Kandidatenbildung via Token-Index (schnell)
- Post-Filter prüft exakte Position (langsamer)
- Empfehlung: Verwenden für spezifische Phrasen-Queries

### FUZZY
- **Langsamer** als FULLTEXT/PHRASE
- Muss alle Tokens im Index scannen
- Levenshtein-Berechnung für jeden Token
- **Empfehlung**: 
  - Verwenden mit `maxDistance <= 2`
  - Limit niedrig halten (< 100)
  - Nur für Fehlertoleranz, nicht für Standard-Suche

---

## Best Practices

### Wann welche Suche verwenden?

| Use Case | Empfohlene Methode |
|----------|-------------------|
| Allgemeine Suche | FULLTEXT |
| Exakte Zitate finden | PHRASE |
| Tippfehler tolerieren | FUZZY |
| Rechtschreibvarianten | FUZZY |
| Mehrere Keywords AND | FULLTEXT |
| Spezifische Fachbegriffe | PHRASE |

### Index-Konfiguration

```bash
POST /index/create
{
  "table": "articles",
  "column": "content",
  "type": "fulltext",
  "config": {
    "stemming_enabled": true,
    "language": "en",
    "stopwords_enabled": true,
    "normalize_umlauts": false
  }
}
```

**Tipp:** Verwende dieselbe Index-Config für alle drei Suchtechniken.

### Kombinierte Queries

```aql
// Multi-Stage Search: FULLTEXT + PHRASE Filter
FOR doc IN articles
  FILTER FULLTEXT(doc.content, "machine learning")
  LET hasPhrase = PHRASE(doc.content, "deep learning") != null
  FILTER hasPhrase
  RETURN doc

// FUZZY mit Strukturfiltern
FOR doc IN articles
  FILTER FUZZY(doc.content, "algoritm", 2)
  FILTER doc.category == "computer-science"
  SORT doc.publish_date DESC
  LIMIT 20
  RETURN doc
```

---

## Implementierungsdetails

### Phrase Search Algorithmus
1. Tokenisiere Phrase mit Index-Config
2. Hole Kandidaten-Dokumente (alle Tokens müssen vorkommen)
3. Lade Originaldokument für jeden Kandidaten
4. Normalisiere Dokumenttext (lowercase, umlauts)
5. Prüfe ob normalisierte Phrase als Substring vorkommt
6. Returniere nur Dokumente mit exakter Phrase

### Fuzzy Search Algorithmus
1. Tokenisiere Query mit Index-Config
2. Scanne alle Tokens im Index
3. Berechne Levenshtein-Distanz für jeden Token
4. Sammle Dokumente mit Tokens innerhalb `maxDistance`
5. Score basierend auf minimaler Distanz
6. Sortiere nach Score absteigend

---

## Siehe auch

- [Fulltext API](fulltext_api.md) - Standard BM25 Suche
- [Hybrid Search](hybrid_search_design.md) - Kombination mit Vektorsuche
- [AQL Fulltext Release](../aql/aql_fulltext_release.md) - AQL Syntax Details
