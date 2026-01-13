# GitHub Issues Templates für ThemisDB Kompendium

Dieses Verzeichnis enthält Issue-Templates für standardisierte Aufgaben im Kompendium-Projekt.

## Verfügbare Templates

### 📖 kapitel-verbesserung.md
**Für:** Verbesserung bestehender Kapitel  
**Verwendung:** Neuen Issue erstellen → "Kapitel-Generierung / Kapitel-Verbesserung" wählen

**Enthält:**
- ⚠️ Warnung: Kapitel existieren bereits!
- 📋 Kapitel-Details & Ziele
- 🔍 Recherche-Material
- 🎯 Arbeitsschritte (4 Phasen)
- 📝 LLM-Prompt-Vorlage
- 🔗 Links zu allen Richtlinien
- ✅ Akzeptanz-Kriterien
- 📊 Checklisten

## Verwendungsbeispiele

### Beispiel 1: Kapitel 6 verbessern
```
Title: "Kapitel-Verbesserung: chapter_06_graph.md - Graph-Datenmodell"
Template: kapitel-verbesserung.md

Ziele:
- Wissenschaftlichere Sprache
- RocksDB Speicher-Details integrieren
- Code-Beispiele: 2 → 5
- Performance-Benchmarks vs. Neo4j
```

### Beispiel 2: Kapitel 15 mit neuen Quellen
```
Title: "Kapitel-Verbesserung: chapter_15_analytics.md - Analytics"
Template: kapitel-verbesserung.md

Ziele:
- Boost C++ Analytics-Bibliotheken integrieren
- Benchmark-Daten hinzufügen
- Design-Standards beachten
```

## Wichtige Richtlinien

Vor der Verwendung dieser Templates **UNBEDINGT** lesen:

1. **[KAPITEL_MINDSET.md](../KAPITEL_MINDSET.md)** ⭐
   - Mentalität: Kapitel verbessern, nicht neu schreiben
   - 47 Kapitel existieren bereits
   - <1% neue Kapitel brauchen

2. **[CHAPTER_GENERATION_GUIDE.md](../CHAPTER_GENERATION_GUIDE.md)**
   - Vollständiger Guide mit Prompt-Template
   - Best Practices & Qualitätskriterien

3. **[SOURCES_INVENTORY.md](../SOURCES_INVENTORY.md)**
   - Alle 92 verfügbaren Quellen
   - Externe Libraries, Richtlinien, Doku

## Workflow

```
1. Issue erstellen
   → "Kapitel-Verbesserung / Kapitel-Generierung" Template
   
2. Details ausfüllen
   → Kapitel-Nummer, Ziele, Recherche
   
3. LLM-Prompt erstellen
   → Bestehendes Kapitel + Anforderungen
   
4. Kapitel verbessern
   → Quellen integrieren, Code-Beispiele, Performance
   
5. Validierung
   → Code testen, Links überprüfen, Design beachten
   
6. Pull Request
   → chapter_XX.md aktualisieren (nicht erstellen)
```

## Design- & Richtlinien-Dokumente

Diese sind in den Issue-Templates referenziert:

- **IMPLEMENTATION_COMPLETE.md** - Layout-Standards
- **THEMISDB_CUSTOM_THEME.md** - Design-Richtlinien
- **STRATEGY_WITH_EXAMPLES.md** - Struktur-Vorbilder
- **styles_modern_book.scss** - CSS-Design-Philosophie

## Häufige Fragen

**Q: Darf ich ein neues Kapitel erstellen?**  
A: Nur wenn das Thema in KEINEM der 47 bestehenden Kapitel behandelt wird. Mit Team abstimmen!

**Q: Soll ich chapter_06_graph_v2.md erstellen?**  
A: NEIN! Immer chapter_06_graph.md ÜBERSCHREIBEN, nicht duplizieren.

**Q: Welche LLM verwenden?**  
A: Claude 3.5 Sonnet (empfohlen) oder GPT-4o - Siehe CHAPTER_GENERATION_GUIDE.md

**Q: Wie lange dauert ein Kapitel?**  
A: ~5-8 Stunden (Recherche, Prompt, Generierung, Validierung, Testing)

---

**Erstellt:** 13. Januar 2026  
**Status:** Aktiv & verfügbar  
**Letzte Aktualisierung:** 13. Januar 2026
