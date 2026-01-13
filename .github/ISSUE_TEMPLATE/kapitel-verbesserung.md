---
name: Kapitel-Generierung / Kapitel-Verbesserung
about: Neuen Inhalt für ThemisDB Kompendium generieren oder bestehendes Kapitel verbessern
title: "Kapitel-Verbesserung: [Kapitel-Nummer/Titel]"
labels: ["documentation", "content-generation", "llm-assisted"]
assignees: []
---

## 📖 Aufgabenstellung: Kapitel für ThemisDB Kompendium

### ⚠️ WICHTIG: Vor dem Start lesen!
**Dieses Issue ist für Kapitel-VERBESSERUNG, nicht für neue Kapitel!**

- ✅ 47 Kapitel existieren bereits (chapter_00.md bis chapter_46.md)
- ✅ Lese zuerst: [KAPITEL_MINDSET.md](../../KAPITEL_MINDSET.md)
- ✅ Kapitel verbessern statt neu schreiben!

Siehe auch: [Vollständiger Guide](../../CHAPTER_GENERATION_GUIDE.md)

---

## 📋 Kapitel-Details

### Welches Kapitel?
- [ ] **Kapitel-Nummer:** chapter_XX.md
- [ ] **Kapitel-Titel:** [z.B. "Graph-Datenmodell"]
- [ ] **Bestehendes Kapitel:** [Link zu chapter_XX.md]

### Ziele dieser Verbesserung

**Warum wird das Kapitel verbessert?** (Mehrere möglich)
- [ ] Wissenschaftlichere Sprache
- [ ] Bessere Quellen-Integration (Code, Benchmarks, Papers)
- [ ] Mehr Code-Beispiele
- [ ] Performance-Daten hinzufügen
- [ ] Modernere Best Practices
- [ ] Layout-Standards beachten
- [ ] Fehler korrigieren
- [ ] Weitere: _________________

### Qualitäts-Anforderungen

Das verbesserte Kapitel soll erfüllen:
- [ ] **Wissenschaftlicher Stil:** Formal, präzise, objektiv
- [ ] **Gute Quellen:** RocksDB, Boost, MVCC, akademische Papers
- [ ] **Code-Beispiele:** Syntaktisch korrekt, lauffähig, getestet
- [ ] **Performance-Daten:** Mit Methodologie und Benchmarks
- [ ] **Design-Standards:** IMPLEMENTATION_COMPLETE.md beachten
- [ ] **Layout-Standards:** Widow/Orphan, Marker-System
- [ ] **Querverweise:** Zu anderen Kapiteln
- [ ] **Glossar-Einträge:** Für Fachbegriffe

---

## 🔍 Recherche-Material (optional vorbereiten)

**Relevante externe Ressourcen:**
- [ ] RocksDB Wiki: https://github.com/facebook/rocksdb/wiki
- [ ] Boost C++ Docs: https://www.boost.org/doc/libs/
- [ ] Akademische Papers: (Liste)
- [ ] Performance Benchmarks: (Links)
- [ ] Community Resources: (URLs)

**Lokale Richtlinien beachten:**
- [ ] CHAPTER_GENERATION_GUIDE.md (Template & Best Practices)
- [ ] SOURCES_INVENTORY.md (Quellen-Übersicht)
- [ ] IMPLEMENTATION_COMPLETE.md (Layout-Standards)
- [ ] THEMISDB_CUSTOM_THEME.md (Design-Richtlinien)
- [ ] STRATEGY_WITH_EXAMPLES.md (Struktur-Vorbilder)
- [ ] KAPITEL_MINDSET.md (Mentale Modelle)

---

## 🎯 Arbeitsschritte

### Phase 1: Vorbereitung
- [ ] Bestehendes Kapitel gelesen und analysiert
- [ ] Relevante Quellen gesammelt
- [ ] Design-Richtlinien reviewt
- [ ] Mit Team abgestimmt (falls Fragen)

### Phase 2: LLM-Generierung
- [ ] Prompt erstellt (Bestehendes Kapitel + Anforderungen)
- [ ] LLM (Claude 3.5 Sonnet) aufgerufen
- [ ] Output erhalten

### Phase 3: Validierung
- [ ] Technische Korrektheit (Code, Aussagen)
- [ ] Quellen-Links überprüft
- [ ] Code-Beispiele getestet
- [ ] Layout-Standards beachtet
- [ ] Design-Richtlinien befolgt

### Phase 4: Integration
- [ ] Verbessertes Kapitel in chapter_XX.md eingefügt
- [ ] Datei-Name unverändert
- [ ] mkdocs-nav.yml unverändert
- [ ] Pull Request erstellt/Code Review

---

## 📝 LLM-Prompt-Vorlage

```markdown
# Aufgabe: Kapitel-Verbesserung

Ich möchte Kapitel [N] "[TITEL]" wissenschaftlich verbessern.

## BESTEHENDES KAPITEL
[Kompletter Text aus chapter_N.md]

## VERBESSERUNGSANFORDERUNGEN
1. Wissenschaftlichere Sprache
2. Bessere Quellen-Integration (RocksDB, Boost, Papers)
3. Mehr Code-Beispiele ([aktuelle: X] → [neue: Y])
4. Performance-Benchmarks hinzufügen
5. Struktur: [Kleppmann-Pattern / andere]

## RICHTLINIEN
- Layout: Beachte Widow/Orphan, Seitenmary, Marker-System
- Stil: Formal-wissenschaftlich, Wir-Form, Präsens
- Design: IMPLEMENTATION_COMPLETE.md, THEMISDB_CUSTOM_THEME.md

## WICHTIG
- Grundstruktur erhalten (Kapitel-Nummer gleich)
- Nur GLEICHE Datei aktualisieren, KEINE neue Datei
- Output: Verbesserte Version des GLEICHEN Kapitels

## QUELLEN-MATERIAL
[Optional: Zusätzliche Ressourcen/Links eingeben]
```

---

## 🔗 Relevante Dokumentation

### Pflichtlektüre
1. **[KAPITEL_MINDSET.md](../../KAPITEL_MINDSET.md)** ⭐
   - Mentale Modelle (Richtig vs. Falsch)
   - Typische Szenarien
   - Checkliste

2. **[CHAPTER_GENERATION_GUIDE.md](../../CHAPTER_GENERATION_GUIDE.md)**
   - Vollständiger Prompt-Template
   - Struktur-Anforderungen
   - Code-Beispiele

3. **[SOURCES_INVENTORY.md](../../SOURCES_INVENTORY.md)**
   - Alle 92 Quellen kategorisiert
   - Externe Libraries (RocksDB, Boost, etc.)
   - Akademische Ressourcen

### Design & Standards
- **[IMPLEMENTATION_COMPLETE.md](../../IMPLEMENTATION_COMPLETE.md)** - Layout-Standards
- **[THEMISDB_CUSTOM_THEME.md](../../THEMISDB_CUSTOM_THEME.md)** - Design-Richtlinien
- **[STRATEGY_WITH_EXAMPLES.md](../../STRATEGY_WITH_EXAMPLES.md)** - Struktur-Vorbilder
- **[styles_modern_book.scss](../../styles_modern_book.scss)** - CSS-Design

### Metadaten
- **[MASTER_IMPLEMENTATION_SUMMARY.md](../../MASTER_IMPLEMENTATION_SUMMARY.md)** - Gesamt-Übersicht
- **[STATUS_UPDATE.md](../../STATUS_UPDATE.md)** - Aktueller Stand
- **[BUILD_GAPS_ANALYSIS.md](../../BUILD_GAPS_ANALYSIS.md)** - Identified Gaps

---

## ✅ Akzeptanz-Kriterien

Das verbesserte Kapitel ist fertig, wenn:

- [ ] Sprache ist wissenschaftlicher & präziser
- [ ] Quellen sind besser integriert (Code, Benchmarks, Papers)
- [ ] Code-Beispiele sind syntaktisch korrekt & getestet
- [ ] Performance-Daten mit Methodologie
- [ ] Design-Standards beachtet (IMPLEMENTATION_COMPLETE.md)
- [ ] Layout-Standards beachtet (Widow/Orphan, Marker)
- [ ] Querverweise zu anderen Kapiteln
- [ ] Datei: chapter_XX.md (Name unverändert)
- [ ] mkdocs-nav.yml: Unverändert
- [ ] Keine neuen Kapitel-Dateien erstellt
- [ ] Pull Request / Code Review bestanden

---

## 📊 Checkliste: Nicht vergessen!

**Vor Start:**
- [ ] KAPITEL_MINDSET.md gelesen?
- [ ] Bestehendes Kapitel analysiert?
- [ ] Quellen gesammelt?
- [ ] Mit Team abgestimmt?

**Bei Generierung:**
- [ ] LLM-Prompt mit BESTEHENDEM Text?
- [ ] Anforderungen klar formuliert?
- [ ] Richtlinien referenziert?

**Nach Generierung:**
- [ ] Code-Beispiele getestet?
- [ ] Links überprüft?
- [ ] Design-Standards?
- [ ] Datei-Name gleich?
- [ ] mkdocs-nav.yml unverändert?

---

## 💬 Kommentare / Weitere Info

[Hier zusätzliche Informationen eintragen]

---

**Ressourcen:**
- 🎯 [KAPITEL_MINDSET.md](../../KAPITEL_MINDSET.md) - MUSS GELESEN WERDEN!
- 📖 [CHAPTER_GENERATION_GUIDE.md](../../CHAPTER_GENERATION_GUIDE.md)
- 📚 [SOURCES_INVENTORY.md](../../SOURCES_INVENTORY.md)
- 🎨 [IMPLEMENTATION_COMPLETE.md](../../IMPLEMENTATION_COMPLETE.md)
