# ThemisDB: Das vollständige Handbuch

**Version 1.3.4** | **Dezember 2025**

---

## Willkommen

Dies ist das offizielle Handbuch für ThemisDB - eine umfassende, narrative Dokumentation, die Sie von den Grundlagen bis zur Mastery führt.

### Was ist dieses Handbuch?

Im Gegensatz zur Referenzdokumentation (700+ Einzeldokumente) bietet dieses Handbuch:

- **📖 Narrative Struktur:** Ausformulierte Texte statt Stichpunkte
- **🎯 Didaktischer Aufbau:** Von einfach zu komplex
- **💻 Vollständige Examples:** Alle 21 Praxisbeispiele integriert
- **🔗 Zusammenhänge:** Wie alles zusammenspielt
- **📊 Vergleiche:** ThemisDB vs. Alternativen

### Für wen ist dieses Buch?

- **Einsteiger:** Die noch nie mit ThemisDB gearbeitet haben
- **Entwickler:** Die produktionsreife Anwendungen bauen wollen
- **Architekten:** Die Systemdesign-Entscheidungen treffen müssen
- **Admins:** Die ThemisDB betreiben und skalieren

### Wie dieses Buch zu lesen ist

**Sequential (empfohlen):**
Lesen Sie die Kapitel in Reihenfolge - jedes baut auf dem vorherigen auf.

**Topic-based:**
Springen Sie direkt zu Themen, die Sie interessieren:
- **Multi-Model verstehen?** → Teil II (Kapitel 5-8)
- **Production-ready machen?** → Teil V (Kapitel 17-20)
- **Sicherheit härten?** → Teil VI (Kapitel 21-24)

**Example-driven:**
Arbeiten Sie die Examples durch und lesen Sie die zugehörigen Kapitel.

---

## Struktur

### Teil I: Grundlagen (4 Kapitel)
Die Fundamente von ThemisDB. Architektur, Konzepte, Installation.

### Teil II: Datenmodelle (4 Kapitel)
Relational, Graph, Dokument, Vektor - tiefgehend erklärt.

### Teil III: Spezialanwendungen (4 Kapitel)
IoT, Enterprise, Realtime, Computer Vision.

### Teil IV: Erweiterte Features (4 Kapitel)
AQL Mastery, Events, Storage Internals, Transaktionen.

### Teil V: Skalierung (4 Kapitel)
Horizontal Scaling, HA, Monitoring, Performance Tuning.

### Teil VI: Sicherheit (4 Kapitel)
Auth, Verschlüsselung, Audit, PKI.

### Teil VII: Entwicklung (4 Kapitel)
SDKs, APIs, DevOps, Testing.

### Teil VIII: Best Practices (2 Kapitel)
Migration, Patterns, Anti-Patterns.

### Anhänge
Referenzen, Glossar, alle Examples im Detail.

---

## Downloads

- **[PDF Version](../ThemisDB-Kompendium-v1.3.4.pdf)** - Vollständiges Handbuch (ca. 880 Seiten)
- **[Examples Repository](https://github.com/makr-code/ThemisDB/tree/main/examples)** - Alle Code-Beispiele
- **[Referenzdokumentation](../de/README.md)** - Einzeldokumente zum Nachschlagen

---

## Online-Ressourcen

- **GitHub Repository:** [makr-code/ThemisDB](https://github.com/makr-code/ThemisDB)
- **Discussions:** [Community Forum](https://github.com/makr-code/ThemisDB/discussions)
- **Issues:** [Bug Reports & Feature Requests](https://github.com/makr-code/ThemisDB/issues)
- **Examples Playground:** [Interactive Examples](https://playground.themisdb.dev) (coming soon)

---

## Konventionen

### Code-Beispiele

```python
# Python Code wird so dargestellt
client = ThemisClient("localhost", 8765)
```

```sql
-- AQL Queries so
FOR user IN users
    RETURN user
```

### Hinweise

> **💡 Tipp:** Nützliche Hinweise und Best Practices

> **⚠️ Achtung:** Wichtige Warnungen und häufige Fehler

> **🔬 Hintergrund:** Tiefere technische Details

> **📖 Beispiel:** Verweis auf vollständige Examples

### Hervorhebungen

- **Fett:** Wichtige Begriffe beim ersten Auftreten
- `Code`: Inline Code, Commands, Dateinamen
- *Kursiv:* Betonung

---

## Feedback

Haben Sie Feedback zu diesem Handbuch?

- **Fehler gefunden?** → [Issue erstellen](https://github.com/makr-code/ThemisDB/issues/new)
- **Verbesserungsvorschlag?** → [Discussion starten](https://github.com/makr-code/ThemisDB/discussions/new)
- **Beispiel fehlt?** → [Feature Request](https://github.com/makr-code/ThemisDB/issues/new?labels=documentation)

---

## Lizenz

Dieses Handbuch steht unter der [Creative Commons CC-BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/) Lizenz.

ThemisDB selbst ist Open Source unter der [MIT Lizenz](https://github.com/makr-code/ThemisDB/blob/main/LICENSE).

---

## Los geht's!

Bereit, ThemisDB zu meistern?

**[→ Zum Vorwort](preface.md)**

**[→ Direkt zu Kapitel 1: Einführung](chapter_01_introduction.md)**
