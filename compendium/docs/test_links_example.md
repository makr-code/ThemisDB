# Test: Link-Beispiele im Fließtext

Dieses Dokument demonstriert verschiedene Link-Typen, die vom neuen Anchor-System unterstützt werden.

## 1. Links zu anderen Kapiteln

### Einfache Datei-Links
Siehe auch [Einführung](chapter_01_introduction.md) für grundlegende Konzepte.

Weitere Details finden Sie in [Architektur](chapter_02_architecture.md).

### Links mit relativen Pfaden
Gehen Sie zurück zur [Genesis](./chapter_00_genesis.md) um die Entstehungsgeschichte zu verstehen.

## 2. Links zu spezifischen Abschnitten

### Links mit Ankern in anderen Dateien
Für Details zur RocksDB-Integration siehe [Architektur: Storage Layer](chapter_02_architecture.md#storage-layer).

Die Multimodel-Fähigkeiten werden in [Multimodel-Datenbanken: Graph-Modell](chapter_03_multimodel.md#graph-modell) erklärt.

### Links zu Überschriften im selben Dokument
Siehe [Abschnitt über Links](#einfache-datei-links) oben.

## 3. Links zu Diagrammen

Siehe Diagramm der [Systemarchitektur](#diagram-1) für eine visuelle Darstellung.

## 4. Links zu Tabellen

Eine Übersicht finden Sie in [Tabelle 1](#table-chapter-02-1).

## 5. Externe Links (bleiben unverändert)

Weitere Informationen auf [ThemisDB.org](https://themisdb.org).

## 6. File-Protocol Links (werden konvertiert)

Legacy-Link: [Einführung](file:///chapter_01_introduction).

## Verwendung in der Praxis

Im normalen Fließtext können Sie einfach schreiben:

"Die **Installation** (siehe [Kapitel 4](chapter_04_installation.md)) ist einfach und schnell. 
Nach der Installation können Sie mit den [relationalen Datenmodellen](chapter_05_relational.md) 
beginnen oder direkt zu [Graph-Datenbanken](chapter_06_graph.md) übergehen."

"Für Performance-Optimierung konsultieren Sie [Performance-Tuning: Indizes](chapter_21_performance.md#indizes) 
und [Monitoring: Metriken](chapter_19_monitoring.md#metriken)."

## Ergebnis

Alle diese Links werden automatisch in interne Anchors konvertiert:
- `[Text](file.md)` → `<a href="#file">`
- `[Text](file.md#section)` → `<a href="#section">`
- `[Text](#local)` → `<a href="#local">` (validiert gegen Registry)
- `file://` URLs → `<a href="#file">`

Das System stellt sicher, dass alle Links zu registrierten Elementen zeigen:
- **273 registrierte Anchors** für Kapitel, Teile, Überschriften, Diagramme, Tabellen
- **Automatische Validierung** gegen Anchor-Registry
- **Slugified IDs** für konsistente Namensgebung
- **Cross-Reference-Unterstützung** zwischen allen Dokumenten
