# Quick Start Guide für Buchautoren

**Version:** 1.0.0  
**Stand:** Dezember 2025

---

## Schnelleinstieg in 5 Minuten

### Schritt 1: Kapitel auswählen

Prüfen Sie in `book/README.md`, welches Kapitel Sie schreiben möchten.

---

### Schritt 2: Template kopieren

```bash
cd /path/to/ThemisDB/book
cp chapter_template.md kapitel_XX_titel.md
```

Ersetzen Sie `XX` mit der Kapitelnummer und `titel` mit einem kurzen, sprechenden Namen.

---

### Schritt 3: Metadaten ausfüllen

```markdown
## [XX]: [Voller Titel]

**Autor:** Ihr Name
**Reviewer:** TBD
**Status:** Draft
**Letzte Aktualisierung:** 2025-12-08
**Version:** 0.1.0
```

---

### Schritt 4: Referenzdokumente finden

Schauen Sie in `book/chapter_mapping.md` nach den relevanten Dokumenten für Ihr Kapitel.

**Beispiel für Kapitel 4:**
```markdown
**Primäre Quellen:**
- `/docs/architecture/architecture_overview.md`
- `/docs/architecture/architecture_strategic.md`
```

---

### Schritt 5: Schreiben starten!

Folgen Sie dem Template und den [Schreibrichtlinien](writing_guidelines.md).

---

## Checkliste vor dem Einreichen

- [ ] **Alle Platzhalter ersetzt**: Keine `[...]` mehr im Text
- [ ] **Code kompiliert**: Alle Beispiele getestet
- [ ] **Links validiert**: Alle Referenzen funktionieren
- [ ] **Rechtschreibung**: Durchgesehen
- [ ] **Metadaten**: Vollständig ausgefüllt

---

## Häufige Anfängerfehler

### ❌ Fehler 1: Nicht-kompilierende Code-Beispiele

**Problem:**
```cpp
auto db = ...;  // Unvollständig!
```

**Lösung:**
```cpp
#include <themis/database.hpp>
auto db = Database::create("/data");
```

---

### ❌ Fehler 2: Vage Performance-Aussagen

**Problem:**
> "ThemisDB ist sehr schnell."

**Lösung:**
> "ThemisDB erreicht 50,000 Schreiboperationen/Sekunde (TPC-C Benchmark, Intel Xeon E5-2680)."

---

### ❌ Fehler 3: Fehlende Kontext für Code

**Problem:**
Nur ein Code-Snippet ohne Erklärung.

**Lösung:**
Code + Erklärung + Erwartete Ausgabe

---

## Wo finde ich Hilfe?

### Dokumentation

1. **Struktur**: `book/README.md`
2. **Mapping**: `book/chapter_mapping.md`
3. **Template**: `book/chapter_template.md`
4. **Style Guide**: `book/writing_guidelines.md`

---

### Code-Beispiele

**Quellen für Code:**
1. Bestehendes Repository: `src/`, `include/`
2. Test-Dateien: `tests/test_*.cpp`
3. Beispiele: `examples/`

**Code extrahieren:**
```bash
# Funktion aus Quellcode finden
grep -r "create_database" src/

# In Tests nachsehen
grep -r "TEST(" tests/ | grep database
```

---

### Referenz-Dokumentation

**Im Repository:**
- Architecture: `docs/architecture/`
- Features: `docs/features/`
- APIs: `docs/api/`
- Guides: `docs/guides/`

**Navigation:**
```bash
# Alle Dokumente zu einem Thema finden
find docs/ -name "*vector*"

# Dokumenten-Index anzeigen
cat docs/DOCUMENTATION_INDEX.md
```

---

## Nützliche Kommandos

### Code-Formatierung

```bash
# C++ Code formatieren
clang-format -i your_example.cpp

# Alle Beispiele formatieren
find book/examples -name "*.cpp" -exec clang-format -i {} \;
```

---

### Links validieren

```bash
# Markdown Links prüfen (benötigt npm)
npm install -g markdown-link-check
markdown-link-check kapitel_04_systemarchitektur.md
```

---

### Rechtschreibung

```bash
# Mit aspell (Linux)
aspell check -l de_DE kapitel_04.md

# In VS Code: German Language Pack installieren
```

---

### Preview

```bash
# Markdown Preview in VS Code
# Strg+Shift+V (Windows/Linux)
# Cmd+Shift+V (Mac)

# Oder mit Pandoc zu HTML konvertieren
pandoc kapitel_04.md -o preview.html
```

---

## Workflow

### 1. Draft schreiben

```bash
# Neues Kapitel aus Template
cp chapter_template.md kapitel_04_systemarchitektur.md

# Bearbeiten
code kapitel_04_systemarchitektur.md
```

Status: `Draft`

---

### 2. Self-Review

- Checkliste durchgehen
- Code testen
- Links prüfen
- Rechtschreibung

Status: `Draft` → `Review`

---

### 3. Technical Review

- Code von Experte prüfen lassen
- Performance-Zahlen validieren
- Technische Korrektheit

Status: `Review` → `Final`

---

### 4. Editorial Review

- Stil und Lesbarkeit
- Grammatik
- Formatierung

Status: `Final` (fertig!)

---

## Code-Beispiel Template

Verwenden Sie dieses Pattern für alle Code-Beispiele:

```markdown
### Beispiel: [Aussagekräftiger Titel]

**Ziel:** [Was wird demonstriert?]

**Code:**
\`\`\`cpp
// Vollständiger, kompilierender Code
#include <themis/module.hpp>

int main() {
    // Setup
    auto component = create_component();
    
    // Hauptoperation
    auto result = component->process();
    
    // Verifizierung
    assert(result.is_valid());
    
    return 0;
}
\`\`\`

**Erklärung:**
1. **Zeile 5-6:** Setup des Components
2. **Zeile 9:** Verarbeitung
3. **Zeile 12:** Validierung des Ergebnisses

**Erwartete Ausgabe:**
\`\`\`
Processing completed successfully
Result: 42
\`\`\`

**Hinweis:**
> 💡 Verwenden Sie `create_component()` statt manueller Initialisierung.
```

---

## Diagramm-Beispiel Template

```markdown
### Architektur-Diagramm

\`\`\`
┌─────────────────────────────────┐
│      Presentation Layer         │
│         (HTTP API)               │
└────────────┬────────────────────┘
             │
             ▼
┌─────────────────────────────────┐
│      Business Logic Layer       │
│      (Query Engine)              │
└────────────┬────────────────────┘
             │
             ▼
┌─────────────────────────────────┐
│      Data Access Layer          │
│         (Storage)                │
└─────────────────────────────────┘
\`\`\`

*Abbildung X.Y: Drei-Schichten-Architektur*
```

---

## Tabellen-Template

### Vergleichstabelle

```markdown
| Feature | Option A | Option B | Empfehlung |
|---------|----------|----------|------------|
| Performance | ⚡ Schnell | 🐌 Langsam | Option A |
| Komplexität | 🔴 Hoch | 🟢 Niedrig | Option B |
| Skalierbarkeit | ✅ Ja | ❌ Nein | Option A |
```

---

### Konfigurationstabelle

```markdown
| Parameter | Typ | Default | Range | Beschreibung |
|-----------|-----|---------|-------|--------------|
| `max_size` | int | 1024 | 1-10000 | Maximale Größe in MB |
| `enabled` | bool | true | - | Feature aktivieren |
| `timeout` | duration | 30s | 1s-5m | Timeout-Dauer |
```

---

## Ressourcen

### Dokumentation

- **Buch-Struktur**: `book/README.md`
- **Kapitel-Mapping**: `book/chapter_mapping.md`
- **Schreibrichtlinien**: `book/writing_guidelines.md`
- **Template**: `book/chapter_template.md`

---

### Code-Referenz

- **Header-Dateien**: `include/`
- **Implementierung**: `src/`
- **Tests**: `tests/`
- **Beispiele**: `examples/`

---

### Projekt-Dokumentation

- **Gesamt-README**: `README.md`
- **Dokumentations-Index**: `docs/DOCUMENTATION_INDEX.md`
- **Architektur**: `docs/architecture/`
- **Features**: `docs/features/`

---

## Kontakt

**Fragen?**
- Technical Review: Siehe `CONTRIBUTING.md`
- Style Fragen: Siehe `book/writing_guidelines.md`
- Allgemeine Fragen: GitHub Issues

---

## Beispiel-Workflow

Sehen wir uns einen typischen Workflow an:

### Tag 1: Setup und Recherche

```bash
# Template kopieren
cp chapter_template.md kapitel_08_storage.md

# Referenzdokumente finden
cat chapter_mapping.md | grep "Kapitel 8"

# Dokumente lesen
code docs/storage/storage_rocksdb.md
```

---

### Tag 2: Erste Draft

```markdown
## 8: Storage Layer

**Autor:** Max Mustermann
**Status:** Draft
...

[Schreiben Sie den Hauptinhalt]
```

---

### Tag 3: Code-Beispiele

```bash
# Code aus Tests extrahieren
grep -A 20 "TEST(Storage" tests/test_storage.cpp > /tmp/storage_examples.cpp

# Beispiel anpassen und testen
code /tmp/storage_examples.cpp

# In Kapitel einfügen
```

---

### Tag 4: Review und Finalisierung

```bash
# Links prüfen
markdown-link-check kapitel_08_storage.md

# Code formatieren
clang-format -i code_examples/*.cpp

# Rechtschreibung
aspell check -l de_DE kapitel_08_storage.md

# Status ändern: Draft → Review
```

---

### Tag 5: Einreichen

```bash
# Final review checklist
# [ ] Alle Punkte abgehakt

# Version bumpen: 0.1.0 → 1.0.0
# Status: Review → Final
```

---

## Tipps für Produktivität

### 1. Verwenden Sie Snippets

**VS Code Snippet:**
```json
{
  "ThemisDB Code Example": {
    "prefix": "tmdb-example",
    "body": [
      "### Beispiel: $1",
      "",
      "**Ziel:** $2",
      "",
      "**Code:**",
      "```cpp",
      "$3",
      "```",
      "",
      "**Erklärung:** $4"
    ]
  }
}
```

---

### 2. Verwenden Sie vorhandene Tests

Tests sind oft die besten Code-Beispiele:

```bash
# Test finden
grep -r "MVCC" tests/

# Test als Beispiel verwenden (angepasst)
```

---

### 3. Batch-Operationen

```bash
# Alle Code-Dateien formatieren
find book/examples -name "*.cpp" -exec clang-format -i {} \;

# Alle Markdown-Dateien prüfen
for f in kapitel_*.md; do
  markdown-link-check "$f"
done
```

---

### 4. Template-Variablen

Verwenden Sie ein Skript zum Ersetzen:

```bash
#!/bin/bash
# generate_chapter.sh

CHAPTER_NUM=$1
CHAPTER_TITLE=$2
AUTHOR=$3

cp chapter_template.md "kapitel_${CHAPTER_NUM}_${CHAPTER_TITLE}.md"

sed -i "s/\[Kapitelnummer\]/$CHAPTER_NUM/g" "kapitel_${CHAPTER_NUM}_${CHAPTER_TITLE}.md"
sed -i "s/\[Kapiteltitel\]/$CHAPTER_TITLE/g" "kapitel_${CHAPTER_NUM}_${CHAPTER_TITLE}.md"
sed -i "s/\[Name\]/$AUTHOR/g" "kapitel_${CHAPTER_NUM}_${CHAPTER_TITLE}.md"
```

**Verwendung:**
```bash
./generate_chapter.sh 08 "Storage Layer" "Max Mustermann"
```

---

## Abschließende Checkliste

Vor dem Einreichen Ihres Kapitels:

### Inhalt
- [ ] Alle Abschnitte aus Template ausgefüllt
- [ ] Lernziele klar definiert
- [ ] Mindestens 3 Übungsaufgaben mit Lösungen
- [ ] Praktisches End-to-End-Beispiel

### Code
- [ ] Alle Code-Beispiele kompilieren
- [ ] Code mit `.clang-format` formatiert
- [ ] Includes vollständig
- [ ] Keine Memory Leaks (Valgrind)

### Dokumentation
- [ ] Links zu Referenzdokumenten
- [ ] Cross-References zu anderen Kapiteln
- [ ] Glossar-Begriffe definiert
- [ ] Diagramme erstellt/eingefügt

### Qualität
- [ ] Rechtschreibprüfung durchgeführt
- [ ] Links validiert
- [ ] Self-Review durchgeführt
- [ ] Metadaten vollständig

### Formatierung
- [ ] Markdown-Syntax korrekt
- [ ] Überschriften-Hierarchie konsistent
- [ ] Code-Blöcke mit Syntax-Highlighting
- [ ] Tabellen korrekt formatiert

---

**Viel Erfolg beim Schreiben!** 📚✍️

---

**Version History:**
- 1.0.0 (Dezember 2025): Initiale Quick Start Guide
