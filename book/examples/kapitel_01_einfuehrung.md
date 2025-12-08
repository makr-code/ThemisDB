# Kapitel 1: Einführung in ThemisDB

**Autor:** ThemisDB Development Team  
**Reviewer:** TBD  
**Status:** Example Chapter  
**Letzte Aktualisierung:** 2025-12-08  
**Version:** 1.0.0

---

## Lernziele

Nach dem Durcharbeiten dieses Kapitels sollten Sie:

- [x] Die Motivation und Vision hinter ThemisDB verstehen
- [x] Die wichtigsten Features und Fähigkeiten kennen
- [x] Die technologischen Grundentscheidungen nachvollziehen können
- [x] Einen Überblick über die Multi-Model-Architektur haben
- [x] Das erste praktische Beispiel ausführen können

---

## Voraussetzungen

Dieses Kapitel setzt folgende Kenntnisse voraus:

- **Grundlagen Datenbanken**: Relationale und NoSQL-Konzepte
- **Programmierung**: Grundkenntnisse in C++ oder ähnlichen Sprachen
- **Systemarchitektur**: Verständnis von Client-Server-Architekturen

---

## Überblick

ThemisDB ist eine moderne Multi-Model-Datenbank, die auf einem Log-Structured Merge Tree (LSM-Tree) basiert und verschiedene Datenmodelle in einer einzigen Plattform vereint. Die Entwicklung begann mit der Vision, die fragmentierte Datenbanklandschaft zu vereinfachen, in der Unternehmen oft mehrere spezialisierte Datenbanken parallel betreiben müssen.

Statt separate Systeme für Dokumente, Graphen, Zeitreihen und Vektorsuche zu verwalten, bietet ThemisDB all diese Fähigkeiten in einer kohärenten Architektur. Dies reduziert nicht nur die Betriebskomplexität, sondern ermöglicht auch innovative Anwendungsfälle durch die Kombination verschiedener Datenmodelle in einer einzigen Query.

**In diesem Kapitel behandeln wir:**
1. Die Problemstellung und Motivation für ThemisDB
2. Kernfeatures und technische Highlights
3. Die Multi-Model-Architektur im Überblick
4. Technologie-Stack und Design-Entscheidungen
5. Ein erstes praktisches "Hello World"-Beispiel

---

## 1. Die Vision: Eine vereinheitlichte Multi-Model-Datenbank

### 1.1 Das Problem: Fragmentierte Datenbanklandschaft

Moderne Anwendungen benötigen oft verschiedene Arten von Datenbankfunktionalität:

- **Dokumenten-Datenbank**: Für flexible, schema-lose Datenstrukturen
- **Graph-Datenbank**: Für Beziehungen und Netzwerkanalysen
- **Vektor-Datenbank**: Für Similarity-Search und ML-Embeddings
- **Zeitreihen-Datenbank**: Für Sensor-Daten und Metriken
- **Spatial-Datenbank**: Für geografische Abfragen

Die traditionelle Lösung ist der Betrieb mehrerer spezialisierter Datenbanken. ThemisDB bietet eine vereinheitlichte Lösung.

---

### 1.2 Die Lösung: ThemisDB Multi-Model-Ansatz

ThemisDB vereint alle diese Fähigkeiten in einer einzigen Datenbank mit:

1. ✅ **Einheitliche Plattform**: Ein System für alle Datenmodelle
2. ✅ **Konsistente Transaktionen**: ACID über alle Datentypen
3. ✅ **Vereinfachte Operations**: Eine Installation, ein Monitoring
4. ✅ **Einheitliche Query-Sprache**: AQL für alle Modelle
5. ✅ **Reduzierte Kosten**: Weniger Infrastruktur, weniger Lizenzen

---

## 2. Kernfeatures

ThemisDB v1.0.0 bietet:

| Feature | Status | Beschreibung |
|---------|--------|--------------|
| **MVCC Transactions** | ✅ Production | ACID-Transaktionen mit Snapshot Isolation |
| **Document Store** | ✅ Production | Schema-lose JSON-Dokumente |
| **Graph Database** | ✅ Production | Kanten-basiertes Graphmodell |
| **Vector Search** | ✅ Production | HNSW-Index, Similarity Search |
| **Time Series** | ✅ Production | Gorilla-Kompression |
| **Geospatial** | ✅ Production | R*-Tree, GeoJSON |
| **Sharding** | ✅ Production | Horizontale Skalierung |
| **Replication** | ✅ Production | Leader-Follower, Multi-Master |
| **GPU Acceleration** | ✅ Production | 10 GPU-Backends |

---

## 3. Erstes Beispiel

### Server starten:

```bash
./themis_server --config config/default.json
```

### Entity einfügen:

```bash
curl -X POST http://localhost:8765/entities/users:alice \
  -H "Content-Type: application/json" \
  -d '{"name": "Alice", "age": 30}'
```

### Query mit AQL:

```bash
curl -X POST http://localhost:8765/query \
  -H "Content-Type: application/json" \
  -d '{"query": "FOR u IN users FILTER u.age > 25 RETURN u"}'
```

---

## Zusammenfassung

**Wichtigste Erkenntnisse:**

1. 🎯 ThemisDB vereint verschiedene Datenmodelle in einer Plattform
2. 🎯 LSM-basierter Storage mit RocksDB als Basis
3. 🎯 AQL als einheitliche Query-Sprache
4. 🎯 Production-ready mit Enterprise-Features

---

## Weiterführende Ressourcen

- **README**: `/README.md`
- **Architektur**: `/docs/architecture/architecture_overview.md`
- **Features**: `/docs/features/features_overview.md`

---

**Metadaten:**

**Schwierigkeitsgrad:** Einsteiger  
**Geschätzte Lesezeit:** 30 Minuten  
**Tags:** introduction, overview, getting-started
