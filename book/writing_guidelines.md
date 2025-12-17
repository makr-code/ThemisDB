# Schreibrichtlinien für das ThemisDB-Buch

**Version:** 1.0.0  
**Stand:** Dezember 2025

---

## Zweck dieses Dokuments

Dieses Dokument definiert Stil, Format und Qualitätsstandards für alle Buchkapitel, um Konsistenz und hohe Qualität sicherzustellen.

---

## 1. Allgemeine Prinzipien

### 1.1 Zielgruppe

Das Buch richtet sich an:
- **Primär**: Erfahrene Softwareentwickler mit C++-Kenntnissen
- **Sekundär**: Datenbankarchitekten und Studierende (fortgeschritten)
- **Tertiär**: DevOps-Engineers und technische Entscheider

**Ton:**
- Professionell und präzise
- Technisch fundiert, aber verständlich
- Praxisorientiert mit nachvollziehbaren Beispielen

---

### 1.2 Schreibstil

**✅ Empfohlen:**
- Aktive Formulierungen: "Wir implementieren..." statt "Es wird implementiert..."
- Konkrete Beispiele: "Die memtable hat 256 MB" statt "Die memtable ist groß"
- Technisch präzise Begriffe: "LSM-Tree" statt "spezielle Datenstruktur"
- Code-Beispiele aus dem echten Projekt

**❌ Vermeiden:**
- Umgangssprachliche Formulierungen
- Vage Aussagen ohne Zahlen
- Marketing-Sprache ("revolutionär", "einzigartig")
- Theoretische Beispiele ohne Bezug zum Code

---

### 1.3 Sprache

**Primärsprache:** Deutsch

**Fachbegriffe:**
- Englische Fachbegriffe NICHT übersetzen (z.B. "LSM-Tree", nicht "Log-Strukturierter Merge-Baum")
- Bei erster Verwendung: Englischer Begriff + deutsche Erklärung
- Konsistente Verwendung im gesamten Buch

**Beispiel:**
> Der **LSM-Tree (Log-Structured Merge Tree)** ist eine Datenstruktur, die Schreiboperationen optimiert, indem sie zunächst in einer memtable im Speicher gesammelt werden.

---

## 2. Struktur und Formatierung

### 2.1 Kapitelstruktur

Jedes Kapitel folgt dieser Struktur:

1. **Metadaten**: Autor, Version, Status
2. **Lernziele**: Klar definierte Ziele
3. **Voraussetzungen**: Benötigte Vorkenntnisse
4. **Überblick**: 2-3 Absätze Einführung
5. **Hauptinhalt**: 3-5 Hauptabschnitte
6. **Praktisches Beispiel**: End-to-End Code
7. **Häufige Probleme**: Troubleshooting
8. **Zusammenfassung**: Key Takeaways
9. **Übungsaufgaben**: Mindestens 3 Aufgaben
10. **Weiterführende Ressourcen**: Links und Referenzen

---

### 2.2 Überschriften

```markdown
# Kapiteltitel (nur einmal pro Datei)

## Hauptabschnitt (Level 2)

### Unterabschnitt (Level 3)

#### Detail-Abschnitt (Level 4, sparsam verwenden)
```

**Regeln:**
- Maximal 4 Hierarchieebenen
- Überschriften beginnen mit Großbuchstaben
- Keine Satzzeichen am Ende
- Nummerierung optional (konsistent im gesamten Kapitel)

---

### 2.3 Listen

**Ungeordnete Listen:**
```markdown
- Punkt 1
- Punkt 2
  - Unterpunkt 2.1
  - Unterpunkt 2.2
- Punkt 3
```

**Geordnete Listen:**
```markdown
1. Schritt 1
2. Schritt 2
3. Schritt 3
```

**Checklisten:**
```markdown
- [ ] Noch zu erledigen
- [x] Bereits erledigt
```

---

### 2.4 Hervorhebungen

**Fett:**
```markdown
**Wichtiger Begriff** oder **Betonung**
```

**Kursiv:**
```markdown
*Leichte Betonung* oder *Variablennamen*
```

**Code inline:**
```markdown
Die Funktion `create_database()` initialisiert die Datenbank.
```

**Kombiniert:**
```markdown
**Wichtig:** Die Variable `max_buffer_size` sollte nicht über `1GB` liegen.
```

---

### 2.5 Blockzitate

**Wichtige Hinweise:**
```markdown
> **Wichtig:** Diese Operation ist nicht thread-safe.
```

**Warnungen:**
```markdown
> ⚠️ **Warnung:** Diese Konfiguration kann zu Datenverlust führen.
```

**Tips:**
```markdown
> 💡 **Tipp:** Verwenden Sie Bloom-Filter für schnellere Lookups.
```

---

## 3. Code-Beispiele

### 3.1 Code-Blöcke

**C++ Code:**
```cpp
// Header-Kommentar erklärt den Zweck
auto database = Database::create({
    .data_dir = "/var/lib/themisdb",
    .cache_size = 1024 * 1024 * 1024  // 1 GB
});

// Kommentare erklären nicht-offensichtliche Teile
database->configure_compaction({
    .strategy = CompactionStrategy::LEVELED,
    .max_levels = 7
});
```

**AQL Queries:**
```aql
FOR doc IN documents
  FILTER doc.status == "published"
  SORT doc.created_at DESC
  LIMIT 10
  RETURN {
    id: doc._key,
    title: doc.title,
    author: doc.author
  }
```

**JSON:**
```json
{
  "table": "users",
  "primary_key": "123",
  "data": {
    "name": "Alice",
    "age": 30,
    "email": "alice@example.com"
  }
}
```

**Shell/Bash:**
```bash
# Server starten
./themis_server --config /etc/themisdb/config.json

# Health-Check
curl http://localhost:8765/health
```

---

### 3.2 Code-Qualität

**✅ Gute Code-Beispiele:**
- Kompilieren ohne Fehler
- Folgen dem Projekt-Coding-Style (`.clang-format`)
- Haben aussagekräftige Variablennamen
- Enthalten Kommentare für komplexe Logik
- Zeigen Best Practices
- Sind vollständig (keine `...` Auslassungen)

**❌ Schlechte Code-Beispiele:**
- Pseudo-Code statt echtem Code
- Verkürzte/unvollständige Beispiele
- Ohne Kontext (fehlende Includes, etc.)
- Veraltete APIs
- Schlechter Stil

---

### 3.3 Code-Kommentare

**Im Code:**
```cpp
// Kurze Erklärung ÜBER der Zeile
auto index = create_index(name, type);
```

**Nach dem Code-Block:**
```markdown
**Erklärung:** Die Funktion `create_index()` erstellt einen neuen Index...
```

**Für komplexe Beispiele:**
```markdown
**Schritt-für-Schritt:**
1. Zeile 3-5: Initialisierung der Datenbank
2. Zeile 7-9: Konfiguration des Indexes
3. Zeile 11: Ausführung der Query
```

---

## 4. Diagramme und Visualisierungen

### 4.1 ASCII-Diagramme

**Für einfache Architekturen:**
```
┌─────────────────────────────────┐
│      HTTP Server Layer          │
│         (Port 8765)              │
└────────────┬────────────────────┘
             │
             ▼
┌─────────────────────────────────┐
│      Query Engine Layer         │
└────────────┬────────────────────┘
             │
             ▼
┌─────────────────────────────────┐
│       Storage Layer             │
│         (RocksDB)                │
└─────────────────────────────────┘
```

**Unicode-Zeichen:**
- `┌ ┐ └ ┘` - Ecken
- `─ │` - Linien
- `├ ┤ ┬ ┴ ┼` - Verbindungen
- `▼ ▲ ► ◄` - Pfeile

---

### 4.2 Sequenzdiagramme

```
Client          Server          Database
  │               │                │
  │─── POST ────→ │                │
  │               │                │
  │               │─── Write ────→ │
  │               │                │
  │               │←─── OK ────── │
  │               │                │
  │←─── 200 ───── │                │
  │               │                │
```

---

### 4.3 Flussdiagramme

```
Start
  │
  ▼
┌──────────────┐
│ Parse Query  │
└──────┬───────┘
       │
       ▼
┌──────────────┐    No    ┌──────────────┐
│ Valid Syntax?│─────────→│ Return Error │
└──────┬───────┘          └──────────────┘
       │ Yes
       ▼
┌──────────────┐
│ Execute Plan │
└──────┬───────┘
       │
       ▼
     End
```

---

### 4.4 Externe Diagramme

Für komplexe Diagramme:
```markdown
![Architektur-Diagramm](diagrams/architecture.png)

*Abbildung 4.1: ThemisDB Gesamtarchitektur*
```

**Format:** PNG oder SVG  
**Verzeichnis:** `book/diagrams/`  
**Naming:** `kapitel_XX_diagramm_name.png`

---

## 5. Tabellen

### 5.1 Vergleichstabellen

```markdown
| Feature | ThemisDB | Alternative A | Alternative B |
|---------|----------|---------------|---------------|
| MVCC | ✅ Vollständig | ⚠️ Partiell | ❌ Nein |
| Sharding | ✅ Automatisch | 🔧 Manuell | ❌ Nein |
| GPU | ✅ 10 Backends | ❌ Nein | ❌ Nein |
```

**Symbole:**
- ✅ Vollständig unterstützt
- ⚠️ Teilweise unterstützt
- 🔧 Erfordert Konfiguration
- 📋 Geplant
- ❌ Nicht unterstützt

---

### 5.2 Datentabellen

```markdown
| Metrik | Wert | Einheit | Benchmark |
|--------|------|---------|-----------|
| Write Throughput | 50,000 | ops/sec | TPC-C |
| Read Latency | 0.5 | ms | p99 |
| Storage Overhead | 20 | % | Compression |
```

---

### 5.3 Referenztabellen

```markdown
| Parameter | Typ | Default | Beschreibung |
|-----------|-----|---------|--------------|
| `cache_size` | size_t | 1GB | Block Cache Größe |
| `max_levels` | int | 7 | LSM Level Count |
| `compression` | enum | LZ4 | Compression Type |
```

---

## 6. Referenzen und Links

### 6.1 Interne Links (zu anderen Kapiteln)

```markdown
Siehe [Kapitel 4: Systemarchitektur](kapitel_04_systemarchitektur.md) für Details.

Wie in Abschnitt [5.2: Base Entity](#52-base-entity) beschrieben...
```

---

### 6.2 Dokumentations-Links

```markdown
Weitere Details finden Sie in der [Storage-Dokumentation](/docs/storage/storage_rocksdb.md).
```

**Format:**
- Absolute Pfade vom Repository-Root: `/docs/...`
- Relative Pfade nur innerhalb von `book/`

---

### 6.3 Externe Links

```markdown
Mehr über LSM-Trees: [The Log-Structured Merge-Tree (LSM-Tree)](https://www.cs.umb.edu/~poneil/lsmtree.pdf)
```

**Regeln:**
- Vollständige URLs
- Beschreibender Link-Text (nicht "hier klicken")
- Akademische Papers mit vollständiger Zitation

---

### 6.4 Code-Referenzen

```markdown
Die Implementierung finden Sie in:
- Header: [`include/storage/base_entity.hpp`](/include/storage/base_entity.hpp)
- Source: [`src/storage/base_entity.cpp`](/src/storage/base_entity.cpp)
- Tests: [`tests/test_base_entity.cpp`](/tests/test_base_entity.cpp)
```

---

### 6.5 Akademische Zitationen

**PFLICHT:** Alle Konzepte, Algorithmen, Design-Patterns und Benchmarks müssen zitiert werden.

#### Zitationsstil: IEEE/ACM Format

**Journal Paper:**
```
[1] Nachname, V. (Jahr). "Titel des Papers". Journal Name, Band(Ausgabe), Seiten.

Beispiel:
[1] O'Neil, P., Cheng, E., Gawlick, D., O'Neil, E. (1996). "The Log-Structured Merge-Tree (LSM-Tree)". Acta Informatica, 33(4), 351-385.
```

**Conference Paper:**
```
[2] Nachname, V., Nachname, N. (Jahr). "Titel". Conference Acronym, Seiten.

Beispiel:
[2] Berenson, H., Bernstein, P., Gray, J., et al. (1995). "A Critique of ANSI SQL Isolation Levels". ACM SIGMOD, 24(2), 1-10.
```

**Buch:**
```
[3] Nachname, V. (Jahr). "Buchtitel". Verlag.

Beispiel:
[3] Stroustrup, B. (2013). "The C++ Programming Language, 4th Edition". Addison-Wesley Professional.
```

**Technical Report / Whitepaper:**
```
[4] Organization (Jahr). "Titel". Technical Report Nummer.

Beispiel:
[4] Facebook Engineering (2021). "RocksDB: Evolution of Development Priorities in a Key-Value Store". Facebook Tech Report.
```

**Online-Ressource:**
```
[5] Autor/Organization (Jahr). "Titel". URL (Zugegriffen: YYYY-MM-DD)

Beispiel:
[5] TechEmpower (2023). "Web Framework Benchmarks - Round 22". https://www.techempower.com/benchmarks/ (Zugegriffen: 2025-12-08)
```

#### Im Text zitieren

**Inline-Zitation:**
```markdown
LSM-Trees wurden erstmals von O'Neil et al. [1] beschrieben und optimieren Schreiboperationen durch...
```

**Mehrfach-Zitation:**
```markdown
Verschiedene MVCC-Implementierungen wurden untersucht [2, 3, 5], wobei PostgreSQL [2] den Vacuum-basierten Ansatz nutzt.
```

**Indirekte Zitation:**
```markdown
Wie in der Literatur beschrieben [1-4], bieten LSM-Trees...
```

#### Wann zitieren?

**IMMER zitieren bei:**
- ✅ Algorithmen (z.B. "HNSW-Algorithmus [7]")
- ✅ Theoretischen Konzepten (z.B. "CAP-Theorem [12]")
- ✅ Performance-Behauptungen (z.B. "RocksDB erreicht 500K ops/sec [4]")
- ✅ Design-Patterns (z.B. "MVCC nach Gray & Reuter [3]")
- ✅ Vergleichen (z.B. "Im Vergleich zu PostgreSQL [2]...")
- ✅ Historical Context (z.B. "Seit Brewer's PODC Keynote [12]...")

**NICHT zitieren:**
- ❌ Allgemein bekannte Fakten ("Datenbanken speichern Daten")
- ❌ Eigene Implementierungsdetails (Code im Repository)
- ❌ Triviale Definitionen

#### Bibliographie am Kapitelende

**Format:**
```markdown
## Vollständige Bibliographie (Kapitel X)

**Foundational Papers:**
[1] O'Neil, P., et al. (1996). "The Log-Structured Merge-Tree (LSM-Tree)". Acta Informatica, 33(4), 351-385.

**Books:**
[2] Gray, J., Reuter, A. (1992). "Transaction Processing: Concepts and Techniques". Morgan Kaufmann.

**Comparative Studies:**
[3] Abadi, D. (2012). "Consistency Tradeoffs in Modern Distributed Database System Design". IEEE Computer, 45(2), 37-42.

**Implementation Reports:**
[4] Dong, S., et al. (2021). "RocksDB: Evolution of Development Priorities". ACM TOCS, 39(4).

**Online Resources:**
[5] TechEmpower (2023). "Web Framework Benchmarks". https://... (Zugegriffen: 2025-12-08)
```

#### Plagiarism vermeiden

**❌ Plagiat:**
```markdown
LSM-Trees optimize write operations by buffering writes in memory and then flushing them to disk in sorted batches.
```

**✅ Korrekt (Paraphrase + Zitation):**
```markdown
LSM-Trees optimieren Schreiboperationen, indem Writes zunächst im Speicher gepuffert und dann in sortierten Batches auf Disk geschrieben werden [1].
```

**✅ Korrekt (Direktes Zitat):**
```markdown
Wie O'Neil et al. [1] beschreiben: "The LSM-tree uses an algorithm that defers and batches index changes, cascading the changes from a memory-based component through one or more disk components."
```

#### Zitations-Management

**Tools:**
- BibTeX für LaTeX (falls PDF-Generation)
- Zotero für Referenz-Management
- Google Scholar für Zitation-Suche
- Semantic Scholar für Paper-Discovery

**Datei-Organisation:**
```
book/
├── references/
│   ├── bibliography.bib        # Alle Referenzen
│   ├── chapter_01.bib         # Kapitel-spezifisch
│   └── chapter_02.bib
```

---

## 7. Technische Präzision

### 7.1 Zahlen und Einheiten

**✅ Korrekt:**
- `256 MB` (Leerzeichen zwischen Zahl und Einheit)
- `1.5 GB`
- `10,000 requests/sec` (Tausendertrennzeichen)
- `0.5 ms` (Dezimalpunkt)

**❌ Falsch:**
- `256MB` (kein Leerzeichen)
- `1,5 GB` (Komma statt Punkt)
- `10000 requests/sec` (keine Trenner)

---

### 7.2 Performance-Metriken

**Immer angeben:**
- Absolute Werte
- Einheiten
- Kontext (Hardware, Workload)
- Benchmark-Methode

**Beispiel:**
```markdown
**Performance (Intel Xeon E5-2680, 64 GB RAM, SSD):**
- Write Throughput: 50,000 ops/sec (TPC-C Workload)
- Read Latency: 0.5 ms p99 (Random Access)
- Storage Overhead: 20% (LZ4 Compression)
```

---

### 7.3 Versionsnummern

**Format:** Semantic Versioning (SemVer)

```markdown
ThemisDB v1.0.0
C++20 Standard
RocksDB v8.6.7
```

**Bei API-Änderungen:**
```markdown
> **Version Info:** Verfügbar ab v1.2.0
```

---

## 8. Beispiele und Übungen

### 8.1 Praktische Beispiele

**Struktur:**
1. **Ziel:** Was wird demonstriert?
2. **Code:** Vollständiges, lauffähiges Beispiel
3. **Erklärung:** Schritt-für-Schritt
4. **Ausgabe:** Erwartetes Ergebnis

**Beispiel:**
```markdown
### Beispiel: Entity einfügen

**Ziel:** Eine Entity in die Datenbank einfügen und wieder abrufen.

**Code:**
[vollständiger Code]

**Erklärung:**
[Schritt-für-Schritt]

**Erwartete Ausgabe:**
[Ausgabe]
```

---

### 8.2 Übungsaufgaben

**Schwierigkeitsgrade:**
- **Einfach**: Anwendung des gelernten Materials
- **Mittel**: Kombination mehrerer Konzepte
- **Fortgeschritten**: Erweiterung oder Optimierung

**Struktur:**
```markdown
### Aufgabe 1: [Titel] (Einfach)

**Ziel:** [Klares Lernziel]

**Aufgabenstellung:**
1. [Schritt 1]
2. [Schritt 2]
3. [Schritt 3]

**Erwartetes Ergebnis:** [Beschreibung]

**Hinweise:**
- [Hilfreicher Hinweis]
- [Verweis auf relevante Abschnitte]
```

---

### 8.3 Lösungen

**Platzierung:**
- Am Ende des Kapitels
- In separatem Abschnitt "Lösungen"

**Format:**
```markdown
### Lösung Aufgabe 1

[Vollständige Lösung mit Code]

**Erklärung:**
[Warum diese Lösung funktioniert]

**Alternativen:**
[Andere mögliche Ansätze]
```

---

## 9. Qualitätssicherung

### 9.1 Pre-Submission Checklist

- [ ] Alle Platzhalter `[...]` ersetzt
- [ ] Code-Beispiele kompilieren
- [ ] Code-Beispiele getestet
- [ ] **ALLE Quellen korrekt zitiert (IEEE/ACM Style)**
- [ ] **Akademische Referenzen am Kapitelende aufgelistet**
- [ ] **Vergleichende Analysen durchgeführt**
- [ ] **Design-Entscheidungen begründet**
- [ ] Rechtschreibprüfung durchgeführt
- [ ] Links validiert (intern und extern)
- [ ] Diagramme erstellt/eingefügt
- [ ] Glossar-Begriffe definiert
- [ ] Metadaten vollständig
- [ ] Self-Review durchgeführt

---

### 9.2 Code-Validierung

**Alle Code-Beispiele müssen:**
1. Kompilieren ohne Warnings
2. Mit `.clang-format` formatiert sein
3. Keine `TODO` oder `FIXME` enthalten
4. Memory-leak-frei sein (Valgrind)
5. Thread-safe sein (wo relevant)

**Test-Prozess:**
```bash
# Code aus Beispiel extrahieren
# In test-Datei speichern
# Kompilieren
cmake --build build --target test_chapter_example

# Ausführen
./build/test_chapter_example

# Valgrind-Check
valgrind --leak-check=full ./build/test_chapter_example
```

---

### 9.3 Technical Review

**Reviewer prüft:**
- ✅ Technische Korrektheit
- ✅ Code-Qualität
- ✅ Performance-Aussagen
- ✅ Best Practices
- ✅ Vollständigkeit

**Review-Kommentare:**
```markdown
**Reviewer:** Max Mustermann
**Datum:** 2025-12-08

**Kommentare:**
- [ ] Zeile 42: Performance-Claim benötigt Benchmark-Referenz
- [x] Code-Beispiel auf Zeile 89 kompiliert nicht (behoben)
- [ ] Abschnitt 3.2 könnte mehr Details zu X enthalten
```

---

### 9.4 Editorial Review

**Editor prüft:**
- ✅ Rechtschreibung und Grammatik
- ✅ Konsistente Terminologie
- ✅ Lesbarkeit
- ✅ Struktur und Fluss
- ✅ Formatierung

---

## 10. Versionierung und Updates

### 10.1 Semantic Versioning

**Kapitel-Versionen:**
- **Major (X.0.0)**: Grundlegende Umstrukturierung
- **Minor (0.X.0)**: Neue Abschnitte oder signifikante Erweiterungen
- **Patch (0.0.X)**: Korrekturen, Klarstellungen, kleine Verbesserungen

---

### 10.2 Änderungshistorie

```markdown
## Änderungshistorie

| Version | Datum | Autor | Änderungen |
|---------|-------|-------|------------|
| 1.0.0 | 2025-12-01 | M. Mustermann | Initiale Version |
| 1.0.1 | 2025-12-05 | M. Mustermann | Typos behoben, Code-Beispiel verbessert |
| 1.1.0 | 2025-12-10 | J. Doe | Neuer Abschnitt zu Optimierungen |
```

---

### 10.3 Deprecated Content

**Markierung:**
```markdown
> ⚠️ **Deprecated:** Dieser Abschnitt beschreibt eine veraltete API (v0.9.x).  
> Siehe [Abschnitt 5.3](#53-neue-api) für die aktuelle Implementierung.
```

---

## 11. Style Guide Zusammenfassung

### 11.1 Do's

✅ **DO:**
- Technisch präzise formulieren
- Code aus echtem Projekt verwenden
- Performance-Zahlen mit Kontext angeben
- Diagramme für komplexe Konzepte
- Praxisnahe Beispiele
- Fehlerbehandlung zeigen
- Best Practices empfehlen
- Tests für Code-Beispiele schreiben

---

### 11.2 Don'ts

❌ **DON'T:**
- Umgangssprachlich schreiben
- Marketing-Sprache verwenden
- Vage Aussagen ohne Belege
- Pseudo-Code statt echtem Code
- Wichtige Details weglassen
- Veraltete Informationen
- Nicht-funktionierende Code-Beispiele
- Standards ignorieren

---

## 12. Häufige Fehler

### 12.1 Formatierung

**❌ Falsch:**
```markdown
### schlechte überschrift
```

**✅ Korrekt:**
```markdown
### Gute Überschrift
```

---

**❌ Falsch:**
```markdown
Die Funktion create_database() ist wichtig.
```

**✅ Korrekt:**
```markdown
Die Funktion `create_database()` ist wichtig.
```

---

### 12.2 Code-Beispiele

**❌ Falsch:**
```cpp
// Beispiel (funktioniert nicht)
auto db = ...;  // Setup
db->insert(...);  // Insert
```

**✅ Korrekt:**
```cpp
// Vollständiges, funktionierendes Beispiel
#include <themis/database.hpp>

auto db = Database::create("/data");
db->insert("users:123", R"({"name": "Alice"})");
```

---

### 12.3 Technische Details

**❌ Falsch:**
```markdown
Der Cache ist groß genug für die meisten Use Cases.
```

**✅ Korrekt:**
```markdown
Der Block Cache hat eine Standardgröße von 1 GB, was für 
Workloads bis 10,000 ops/sec ausreichend ist (TPC-C Benchmark).
```

---

## Anhang

### A. Nützliche Tools

**Markdown-Editoren:**
- VS Code mit Markdown-Extensions
- Typora
- Mark Text

**Code-Formatierung:**
```bash
clang-format -i example.cpp
```

**Spell-Check:**
- VS Code: German Language Pack
- Hunspell (CLI)

**Link-Validation:**
```bash
# Markdown Link Check
npm install -g markdown-link-check
markdown-link-check kapitel_01.md
```

---

### B. Referenzmaterialien

**Style Guides:**
- Google C++ Style Guide
- Microsoft Writing Style Guide
- Technical Writing Guidelines (Google)

**Markdown Reference:**
- CommonMark Spec
- GitHub Flavored Markdown

---

### C. Kontakt und Support

**Fragen zur Schreibrichtlinie:**
- Dokumentations-Team: ma.krueger@outlook.com
- Technical Reviewer: ma.krueger@outlook.com

**Issue Tracking:**
- GitHub Issues: Technical Inaccuracies
- Internal Wiki: Style Questions

---

**Version History:**
- 1.0.0 (Dezember 2025): Initiale Schreibrichtlinien
