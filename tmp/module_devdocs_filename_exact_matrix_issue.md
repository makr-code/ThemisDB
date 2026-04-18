## Ziel
Entwickler-Dokumentation in den Bereichen `src/`, `include/`, `examples/`, `tools/`, `benchmarks/`, `tests/` strikt gegen den aktuellen Sourcecode validieren, korrigieren und vervollstaendigen. Danach Root-Dokumentation zu diesen Bereichen synchronisieren.

## Scope (verifiziert)
- src: 422 MD-Dateien
- include: 392 MD-Dateien
- xamples: 320 MD-Dateien
- 	ools: 171 MD-Dateien
- enchmarks: 165 MD-Dateien
- 	ests: 145 MD-Dateien
- Gesamt: 1615 MD-Dateien

## Verbindliche Arbeitsregeln
- Keine halluzinierten APIs/Kommandos/Pfade.
- Primär Dokumentation ändern, nicht Produktivcode.
- Jede geänderte Aussage muss einen belegbaren Codebezug haben (Datei/Symbol/Command).
- Bei Unsicherheit explizit als `TBD` markieren statt raten.

## Exakte Dateinamen-Matrix (Inhalt + Arbeitsaufgabe pro Datei)
Jede Datei mit einem der folgenden **exakten Dateinamen** ist gemäß zugehörigem Inhalt/Task zu bearbeiten:

| Dateiname | Erwarteter Inhalt | Arbeitsaufgabe |
|---|---|---|
| $safe | Schritt-fuer-Schritt-Anleitung mit verifizierten Kommandos und erwarteter Ausgabe/Artefakten. | Alle Schritte einmal dry-run validieren; nicht funktionierende Kommandos korrigieren. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Schritt-fuer-Schritt-Anleitung mit verifizierten Kommandos und erwarteter Ausgabe/Artefakten. | Alle Schritte einmal dry-run validieren; nicht funktionierende Kommandos korrigieren. |
| $safe | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test-Kommandos, relevante Quellpfade. | README gegen aktuellen Modulcode synchronisieren; Einstieg, Commands und Pfade pruefen. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | API-Endpunkte/Signaturen/Parameter, Fehlerfaelle, Beispielaufrufe gegen realen Codeabgleich. | Signaturen/Parameter/Fehlercodes gegen Implementierungstypen und Endpunkte pruefen. |
| $safe | API-Endpunkte/Signaturen/Parameter, Fehlerfaelle, Beispielaufrufe gegen realen Codeabgleich. | Signaturen/Parameter/Fehlercodes gegen Implementierungstypen und Endpunkte pruefen. |
| $safe | API-Endpunkte/Signaturen/Parameter, Fehlerfaelle, Beispielaufrufe gegen realen Codeabgleich. | Signaturen/Parameter/Fehlercodes gegen Implementierungstypen und Endpunkte pruefen. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Komponenten, Datenfluss, zentrale Typen/Funktionen, Grenzen/Interfaces, Diagramm-Text + Codebezug. | Komponentendiagramm textuell an reale Klassen/Namespaces/Funktionen anpassen. |
| $safe | Komponenten, Datenfluss, zentrale Typen/Funktionen, Grenzen/Interfaces, Diagramm-Text + Codebezug. | Komponentendiagramm textuell an reale Klassen/Namespaces/Funktionen anpassen. |
| $safe | Soll/Ist-Abgleich gegen Code mit Evidenzpfaden, offene Risiken, falsche Aussagen markieren. | Code-Evidenzen pro Befund ergaenzen; falsche Befunde entfernen/korrekt stellen. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test-Kommandos, relevante Quellpfade. | README gegen aktuellen Modulcode synchronisieren; Einstieg, Commands und Pfade pruefen. |
| $safe | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test-Kommandos, relevante Quellpfade. | README gegen aktuellen Modulcode synchronisieren; Einstieg, Commands und Pfade pruefen. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Soll/Ist-Abgleich gegen Code mit Evidenzpfaden, offene Risiken, falsche Aussagen markieren. | Code-Evidenzen pro Befund ergaenzen; falsche Befunde entfernen/korrekt stellen. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Schritt-fuer-Schritt-Anleitung mit verifizierten Kommandos und erwarteter Ausgabe/Artefakten. | Alle Schritte einmal dry-run validieren; nicht funktionierende Kommandos korrigieren. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test-Kommandos, relevante Quellpfade. | README gegen aktuellen Modulcode synchronisieren; Einstieg, Commands und Pfade pruefen. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Versionierte Aenderungen mit Datum, Wirkung, Migration, Referenz auf reale Commits/PRs. | Nur reale, nachvollziehbare Aenderungen behalten; Dubletten und Spekulationen entfernen. |
| $safe | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test-Kommandos, relevante Quellpfade. | README gegen aktuellen Modulcode synchronisieren; Einstieg, Commands und Pfade pruefen. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Schritt-fuer-Schritt-Anleitung mit verifizierten Kommandos und erwarteter Ausgabe/Artefakten. | Alle Schritte einmal dry-run validieren; nicht funktionierende Kommandos korrigieren. |
| $safe | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test-Kommandos, relevante Quellpfade. | README gegen aktuellen Modulcode synchronisieren; Einstieg, Commands und Pfade pruefen. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Schritt-fuer-Schritt-Anleitung mit verifizierten Kommandos und erwarteter Ausgabe/Artefakten. | Alle Schritte einmal dry-run validieren; nicht funktionierende Kommandos korrigieren. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Datenmodell inkl. Felder, Constraints, Migrationen, Indizes, Query-Beispiele. | Felder/Typen/Constraints/Indexe gegen aktuelle Strukturen und Migrationen validieren. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test-Kommandos, relevante Quellpfade. | README gegen aktuellen Modulcode synchronisieren; Einstieg, Commands und Pfade pruefen. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Deployment-Pfade, Voraussetzungen, Konfigvariablen, Rollback, Betriebschecks. | Deploy-/Rollback-Anleitung mit aktuellen Skripten/Targets synchronisieren. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test-Kommandos, relevante Quellpfade. | README gegen aktuellen Modulcode synchronisieren; Einstieg, Commands und Pfade pruefen. |
| $safe | Schritt-fuer-Schritt-Anleitung mit verifizierten Kommandos und erwarteter Ausgabe/Artefakten. | Alle Schritte einmal dry-run validieren; nicht funktionierende Kommandos korrigieren. |
| $safe | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test-Kommandos, relevante Quellpfade. | README gegen aktuellen Modulcode synchronisieren; Einstieg, Commands und Pfade pruefen. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test-Kommandos, relevante Quellpfade. | README gegen aktuellen Modulcode synchronisieren; Einstieg, Commands und Pfade pruefen. |
| $safe | Schritt-fuer-Schritt-Anleitung mit verifizierten Kommandos und erwarteter Ausgabe/Artefakten. | Alle Schritte einmal dry-run validieren; nicht funktionierende Kommandos korrigieren. |
| $safe | Schritt-fuer-Schritt-Anleitung mit verifizierten Kommandos und erwarteter Ausgabe/Artefakten. | Alle Schritte einmal dry-run validieren; nicht funktionierende Kommandos korrigieren. |
| $safe | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test-Kommandos, relevante Quellpfade. | README gegen aktuellen Modulcode synchronisieren; Einstieg, Commands und Pfade pruefen. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Soll/Ist-Abgleich gegen Code mit Evidenzpfaden, offene Risiken, falsche Aussagen markieren. | Code-Evidenzen pro Befund ergaenzen; falsche Befunde entfernen/korrekt stellen. |
| $safe | Konkrete umsetzbare Tasks mit Checkboxen, Phasenmodell, Akzeptanzkriterien, betroffene Dateien. | Roadmap auf produktionsreife, implementierbare Tasks mit Phasen 1-6 umstellen. |
| $safe | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test-Kommandos, relevante Quellpfade. | README gegen aktuellen Modulcode synchronisieren; Einstieg, Commands und Pfade pruefen. |
| $safe | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test-Kommandos, relevante Quellpfade. | README gegen aktuellen Modulcode synchronisieren; Einstieg, Commands und Pfade pruefen. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Messbare Erweiterungen: Scope, Constraints, Interfaces, Tests, Performanceziele, Security/Recovery. | Vage Punkte durch messbare Enhancements mit Interface-/Test-/Perf-Zielen ersetzen. |
| $safe | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test-Kommandos, relevante Quellpfade. | README gegen aktuellen Modulcode synchronisieren; Einstieg, Commands und Pfade pruefen. |
| $safe | Teststrategie, Ausfuehrungskommandos, Fixtures, bekannte Flakes, Coverage-/Qualitaetskriterien. | Testkommandos, Filter und Fixture-Pfade verifizieren; Flaky-Hinweise aktualisieren. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Integrationspunkte, Schnittstellenvertrag, Kompatibilitaet, End-to-End-Validierung. | Integrationsvoraussetzungen und End-to-End-Pfade gegen echte Module pruefen. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Schritt-fuer-Schritt-Anleitung mit verifizierten Kommandos und erwarteter Ausgabe/Artefakten. | Alle Schritte einmal dry-run validieren; nicht funktionierende Kommandos korrigieren. |
| $safe | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test-Kommandos, relevante Quellpfade. | README gegen aktuellen Modulcode synchronisieren; Einstieg, Commands und Pfade pruefen. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Schritt-fuer-Schritt-Anleitung mit verifizierten Kommandos und erwarteter Ausgabe/Artefakten. | Alle Schritte einmal dry-run validieren; nicht funktionierende Kommandos korrigieren. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Soll/Ist-Abgleich gegen Code mit Evidenzpfaden, offene Risiken, falsche Aussagen markieren. | Code-Evidenzen pro Befund ergaenzen; falsche Befunde entfernen/korrekt stellen. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Schritt-fuer-Schritt-Anleitung mit verifizierten Kommandos und erwarteter Ausgabe/Artefakten. | Alle Schritte einmal dry-run validieren; nicht funktionierende Kommandos korrigieren. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Teststrategie, Ausfuehrungskommandos, Fixtures, bekannte Flakes, Coverage-/Qualitaetskriterien. | Testkommandos, Filter und Fixture-Pfade verifizieren; Flaky-Hinweise aktualisieren. |
| $safe | Schritt-fuer-Schritt-Anleitung mit verifizierten Kommandos und erwarteter Ausgabe/Artefakten. | Alle Schritte einmal dry-run validieren; nicht funktionierende Kommandos korrigieren. |
| $safe | Schritt-fuer-Schritt-Anleitung mit verifizierten Kommandos und erwarteter Ausgabe/Artefakten. | Alle Schritte einmal dry-run validieren; nicht funktionierende Kommandos korrigieren. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Teststrategie, Ausfuehrungskommandos, Fixtures, bekannte Flakes, Coverage-/Qualitaetskriterien. | Testkommandos, Filter und Fixture-Pfade verifizieren; Flaky-Hinweise aktualisieren. |
| $safe | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test-Kommandos, relevante Quellpfade. | README gegen aktuellen Modulcode synchronisieren; Einstieg, Commands und Pfade pruefen. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Integrationspunkte, Schnittstellenvertrag, Kompatibilitaet, End-to-End-Validierung. | Integrationsvoraussetzungen und End-to-End-Pfade gegen echte Module pruefen. |
| $safe | Integrationspunkte, Schnittstellenvertrag, Kompatibilitaet, End-to-End-Validierung. | Integrationsvoraussetzungen und End-to-End-Pfade gegen echte Module pruefen. |
| $safe | Schritt-fuer-Schritt-Anleitung mit verifizierten Kommandos und erwarteter Ausgabe/Artefakten. | Alle Schritte einmal dry-run validieren; nicht funktionierende Kommandos korrigieren. |
| $safe | Teststrategie, Ausfuehrungskommandos, Fixtures, bekannte Flakes, Coverage-/Qualitaetskriterien. | Testkommandos, Filter und Fixture-Pfade verifizieren; Flaky-Hinweise aktualisieren. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Schritt-fuer-Schritt-Anleitung mit verifizierten Kommandos und erwarteter Ausgabe/Artefakten. | Alle Schritte einmal dry-run validieren; nicht funktionierende Kommandos korrigieren. |
| $safe | Teststrategie, Ausfuehrungskommandos, Fixtures, bekannte Flakes, Coverage-/Qualitaetskriterien. | Testkommandos, Filter und Fixture-Pfade verifizieren; Flaky-Hinweise aktualisieren. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Teststrategie, Ausfuehrungskommandos, Fixtures, bekannte Flakes, Coverage-/Qualitaetskriterien. | Testkommandos, Filter und Fixture-Pfade verifizieren; Flaky-Hinweise aktualisieren. |
| $safe | Teststrategie, Ausfuehrungskommandos, Fixtures, bekannte Flakes, Coverage-/Qualitaetskriterien. | Testkommandos, Filter und Fixture-Pfade verifizieren; Flaky-Hinweise aktualisieren. |
| $safe | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test-Kommandos, relevante Quellpfade. | README gegen aktuellen Modulcode synchronisieren; Einstieg, Commands und Pfade pruefen. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Schritt-fuer-Schritt-Anleitung mit verifizierten Kommandos und erwarteter Ausgabe/Artefakten. | Alle Schritte einmal dry-run validieren; nicht funktionierende Kommandos korrigieren. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Schritt-fuer-Schritt-Anleitung mit verifizierten Kommandos und erwarteter Ausgabe/Artefakten. | Alle Schritte einmal dry-run validieren; nicht funktionierende Kommandos korrigieren. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test-Kommandos, relevante Quellpfade. | README gegen aktuellen Modulcode synchronisieren; Einstieg, Commands und Pfade pruefen. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Konkrete umsetzbare Tasks mit Checkboxen, Phasenmodell, Akzeptanzkriterien, betroffene Dateien. | Roadmap auf produktionsreife, implementierbare Tasks mit Phasen 1-6 umstellen. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Schritt-fuer-Schritt-Anleitung mit verifizierten Kommandos und erwarteter Ausgabe/Artefakten. | Alle Schritte einmal dry-run validieren; nicht funktionierende Kommandos korrigieren. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Konkrete umsetzbare Tasks mit Checkboxen, Phasenmodell, Akzeptanzkriterien, betroffene Dateien. | Roadmap auf produktionsreife, implementierbare Tasks mit Phasen 1-6 umstellen. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Schritt-fuer-Schritt-Anleitung mit verifizierten Kommandos und erwarteter Ausgabe/Artefakten. | Alle Schritte einmal dry-run validieren; nicht funktionierende Kommandos korrigieren. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Schritt-fuer-Schritt-Anleitung mit verifizierten Kommandos und erwarteter Ausgabe/Artefakten. | Alle Schritte einmal dry-run validieren; nicht funktionierende Kommandos korrigieren. |
| $safe | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test-Kommandos, relevante Quellpfade. | README gegen aktuellen Modulcode synchronisieren; Einstieg, Commands und Pfade pruefen. |
| $safe | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test-Kommandos, relevante Quellpfade. | README gegen aktuellen Modulcode synchronisieren; Einstieg, Commands und Pfade pruefen. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Schritt-fuer-Schritt-Anleitung mit verifizierten Kommandos und erwarteter Ausgabe/Artefakten. | Alle Schritte einmal dry-run validieren; nicht funktionierende Kommandos korrigieren. |
| $safe | Schritt-fuer-Schritt-Anleitung mit verifizierten Kommandos und erwarteter Ausgabe/Artefakten. | Alle Schritte einmal dry-run validieren; nicht funktionierende Kommandos korrigieren. |
| $safe | Schritt-fuer-Schritt-Anleitung mit verifizierten Kommandos und erwarteter Ausgabe/Artefakten. | Alle Schritte einmal dry-run validieren; nicht funktionierende Kommandos korrigieren. |
| $safe | Teststrategie, Ausfuehrungskommandos, Fixtures, bekannte Flakes, Coverage-/Qualitaetskriterien. | Testkommandos, Filter und Fixture-Pfade verifizieren; Flaky-Hinweise aktualisieren. |
| $safe | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test-Kommandos, relevante Quellpfade. | README gegen aktuellen Modulcode synchronisieren; Einstieg, Commands und Pfade pruefen. |
| $safe | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test-Kommandos, relevante Quellpfade. | README gegen aktuellen Modulcode synchronisieren; Einstieg, Commands und Pfade pruefen. |
| $safe | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test-Kommandos, relevante Quellpfade. | README gegen aktuellen Modulcode synchronisieren; Einstieg, Commands und Pfade pruefen. |
| $safe | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test-Kommandos, relevante Quellpfade. | README gegen aktuellen Modulcode synchronisieren; Einstieg, Commands und Pfade pruefen. |
| $safe | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test-Kommandos, relevante Quellpfade. | README gegen aktuellen Modulcode synchronisieren; Einstieg, Commands und Pfade pruefen. |
| $safe | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test-Kommandos, relevante Quellpfade. | README gegen aktuellen Modulcode synchronisieren; Einstieg, Commands und Pfade pruefen. |
| $safe | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test-Kommandos, relevante Quellpfade. | README gegen aktuellen Modulcode synchronisieren; Einstieg, Commands und Pfade pruefen. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Schritt-fuer-Schritt-Anleitung mit verifizierten Kommandos und erwarteter Ausgabe/Artefakten. | Alle Schritte einmal dry-run validieren; nicht funktionierende Kommandos korrigieren. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Konkrete umsetzbare Tasks mit Checkboxen, Phasenmodell, Akzeptanzkriterien, betroffene Dateien. | Roadmap auf produktionsreife, implementierbare Tasks mit Phasen 1-6 umstellen. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Soll/Ist-Abgleich gegen Code mit Evidenzpfaden, offene Risiken, falsche Aussagen markieren. | Code-Evidenzen pro Befund ergaenzen; falsche Befunde entfernen/korrekt stellen. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Schritt-fuer-Schritt-Anleitung mit verifizierten Kommandos und erwarteter Ausgabe/Artefakten. | Alle Schritte einmal dry-run validieren; nicht funktionierende Kommandos korrigieren. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test-Kommandos, relevante Quellpfade. | README gegen aktuellen Modulcode synchronisieren; Einstieg, Commands und Pfade pruefen. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Threat-Model, Angriffsvektoren, Sicherheitsflags, harte Defaults, Test- und Incident-Hinweise. | Sicherheitsabschnitte mit realen Codepfaden/Config-Flags und Testfaellen abgleichen. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Schritt-fuer-Schritt-Anleitung mit verifizierten Kommandos und erwarteter Ausgabe/Artefakten. | Alle Schritte einmal dry-run validieren; nicht funktionierende Kommandos korrigieren. |
| $safe | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test-Kommandos, relevante Quellpfade. | README gegen aktuellen Modulcode synchronisieren; Einstieg, Commands und Pfade pruefen. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |
| $safe | Reproduzierbare Benchmark-Methodik, Hardware, Parameter, Metriken, Skriptpfade, Ergebnisgrenzen. | Messmethodik reproduzierbar machen; obsolete Zahlen als historisch kennzeichnen. |
| $safe | Schritt-fuer-Schritt-Anleitung mit verifizierten Kommandos und erwarteter Ausgabe/Artefakten. | Alle Schritte einmal dry-run validieren; nicht funktionierende Kommandos korrigieren. |
| $safe | Teststrategie, Ausfuehrungskommandos, Fixtures, bekannte Flakes, Coverage-/Qualitaetskriterien. | Testkommandos, Filter und Fixture-Pfade verifizieren; Flaky-Hinweise aktualisieren. |
| $safe | Teststrategie, Ausfuehrungskommandos, Fixtures, bekannte Flakes, Coverage-/Qualitaetskriterien. | Testkommandos, Filter und Fixture-Pfade verifizieren; Flaky-Hinweise aktualisieren. |
| $safe | Teststrategie, Ausfuehrungskommandos, Fixtures, bekannte Flakes, Coverage-/Qualitaetskriterien. | Testkommandos, Filter und Fixture-Pfade verifizieren; Flaky-Hinweise aktualisieren. |
| $safe | Teststrategie, Ausfuehrungskommandos, Fixtures, bekannte Flakes, Coverage-/Qualitaetskriterien. | Testkommandos, Filter und Fixture-Pfade verifizieren; Flaky-Hinweise aktualisieren. |
| $safe | Teststrategie, Ausfuehrungskommandos, Fixtures, bekannte Flakes, Coverage-/Qualitaetskriterien. | Testkommandos, Filter und Fixture-Pfade verifizieren; Flaky-Hinweise aktualisieren. |
| $safe | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test-Kommandos, relevante Quellpfade. | README gegen aktuellen Modulcode synchronisieren; Einstieg, Commands und Pfade pruefen. |
| $safe | Integrationspunkte, Schnittstellenvertrag, Kompatibilitaet, End-to-End-Validierung. | Integrationsvoraussetzungen und End-to-End-Pfade gegen echte Module pruefen. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test-Kommandos, relevante Quellpfade. | README gegen aktuellen Modulcode synchronisieren; Einstieg, Commands und Pfade pruefen. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Schritt-fuer-Schritt-Anleitung mit verifizierten Kommandos und erwarteter Ausgabe/Artefakten. | Alle Schritte einmal dry-run validieren; nicht funktionierende Kommandos korrigieren. |
| $safe | Schritt-fuer-Schritt-Anleitung mit verifizierten Kommandos und erwarteter Ausgabe/Artefakten. | Alle Schritte einmal dry-run validieren; nicht funktionierende Kommandos korrigieren. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test-Kommandos, relevante Quellpfade. | README gegen aktuellen Modulcode synchronisieren; Einstieg, Commands und Pfade pruefen. |
| $safe | Zweck, Modulgrenzen, Abhaengigkeiten, Build/Run/Test-Kommandos, relevante Quellpfade. | README gegen aktuellen Modulcode synchronisieren; Einstieg, Commands und Pfade pruefen. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Integrationspunkte, Schnittstellenvertrag, Kompatibilitaet, End-to-End-Validierung. | Integrationsvoraussetzungen und End-to-End-Pfade gegen echte Module pruefen. |
| $safe | API-Endpunkte/Signaturen/Parameter, Fehlerfaelle, Beispielaufrufe gegen realen Codeabgleich. | Signaturen/Parameter/Fehlercodes gegen Implementierungstypen und Endpunkte pruefen. |
| $safe | Teststrategie, Ausfuehrungskommandos, Fixtures, bekannte Flakes, Coverage-/Qualitaetskriterien. | Testkommandos, Filter und Fixture-Pfade verifizieren; Flaky-Hinweise aktualisieren. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Dateizweck klar definieren, alle Aussagen gegen Code validieren, veraltete Teile entfernen oder markieren. | Inhalt Satz-fuer-Satz gegen Sourcecode validieren; inkonsistente Stellen korrigieren. |
| $safe | Status-/Ergebnisdokument mit verifizierbarem Ist-Zustand, Abweichungen, ToDos, Datum/Quelle. | Statusdokument auf aktuellen Stand bringen oder als Historie markieren/verschieben. |

## Verbindliche Ablaufreihenfolge
1. Vollinventar aller betroffenen Dateien je Bereich erstellen (`path`, `basename`, letzter Git-Änderungsstand).
2. Dateiweise Validierung gegen Code (Symbole, Befehle, Pfade, Flags).
3. Dateiweise Aktualisierung gemäß Matrix.
4. Bereichsweise Link- und Konsistenzprüfung.
5. Danach Root-Doku für `src/include/examples/tools/benchmarks/tests` synchronisieren.

## Root-Doku (nach Modul-Update verpflichtend)
- Alle Root-Einstiege/Indexe/Hubs auf korrekte Pfade und aktuelle Bereichsstände bringen.
- Veraltete Root-Verweise auf geänderte Moduldateien entfernen.
- Überblickstabellen (`Was ist wo dokumentiert?`) aktualisieren.

## Abnahmekriterien
- Keine Broken Links in geänderten MD-Dateien.
- Stichproben pro Bereich: mind. 5 Symbol-/Kommando-Validierungen dokumentiert.
- Root-Doku auf neue/aktualisierte Modul-Doku konsistent verlinkt.
- PR enthält Dateiliste mit `vorher/nachher`-Zusammenfassung pro bearbeiteter Datei.

## PR-Reporting Pflicht
1. Vollständige Liste aller geänderten MD-Dateien (mit Bereich).
2. Für jede Datei: `alte Abweichung -> Korrektur` in 1–3 Sätzen.
3. Liste korrigierter Links und validierter Kommandos/Symbole.
4. Offene Punkte mit Begründung.
