# Prompt für Google Gemini

Analysiere ThemisDB detailliert und leite daraus die nächsten Entwicklungsschritte ab. Nutze die verlinkten Quellen als Primärbasis, gleiche Architektur, Roadmap, aktuelle Entwicklungsprioritäten und reale Risiken gegeneinander ab und vermeide unbelegte Annahmen. Wenn etwas nicht sauber aus den Quellen ableitbar ist, markiere es ausdrücklich als Lücke oder offene Frage.

## Quellen

### Root-Dokumente
- [ARCHITECTURE.md](https://github.com/makr-code/ThemisDB/blob/develop/ARCHITECTURE.md)
- [ROADMAP.md](https://github.com/makr-code/ThemisDB/blob/develop/ROADMAP.md)
- [FUTURE_ENHANCEMENTS.md](https://github.com/makr-code/ThemisDB/blob/develop/FUTURE_ENHANCEMENTS.md)
- [AI_WIKI_INTEGRATION_PLAYBOOK.md](https://github.com/makr-code/ThemisDB/blob/develop/AI_WIKI_INTEGRATION_PLAYBOOK.md)
- [MODULE_INDEX.md](https://github.com/makr-code/ThemisDB/blob/develop/MODULE_INDEX.md)
- [src/README.md](https://github.com/makr-code/ThemisDB/blob/develop/src/README.md)

### Architektur- und Governance-Quellen
- [docs/architecture/MODULE_ARCHITECTURE.md](https://github.com/makr-code/ThemisDB/blob/develop/docs/architecture/MODULE_ARCHITECTURE.md)
- [docs/architecture/RELEASE_ARCHITECTURE_STATUS.md](https://github.com/makr-code/ThemisDB/blob/develop/docs/architecture/RELEASE_ARCHITECTURE_STATUS.md)
- [src/CROSS_MODULE_INTEGRATION.md](https://github.com/makr-code/ThemisDB/blob/develop/src/CROSS_MODULE_INTEGRATION.md)

### Wichtige Modul-Roadmaps
- [src/query/ROADMAP.md](https://github.com/makr-code/ThemisDB/blob/develop/src/query/ROADMAP.md)
- [src/transaction/ROADMAP.md](https://github.com/makr-code/ThemisDB/blob/develop/src/transaction/ROADMAP.md)
- [src/gpu/ROADMAP.md](https://github.com/makr-code/ThemisDB/blob/develop/src/gpu/ROADMAP.md)
- [src/llm/ROADMAP.md](https://github.com/makr-code/ThemisDB/blob/develop/src/llm/ROADMAP.md)
- [src/llm_wiki/ROADMAP.md](https://github.com/makr-code/ThemisDB/blob/develop/src/llm_wiki/ROADMAP.md)
- [src/server/ROADMAP.md](https://github.com/makr-code/ThemisDB/blob/develop/src/server/ROADMAP.md)
- [src/storage/ROADMAP.md](https://github.com/makr-code/ThemisDB/blob/develop/src/storage/ROADMAP.md)
- [src/access_model/ROADMAP.md](https://github.com/makr-code/ThemisDB/blob/develop/src/access_model/ROADMAP.md)

## Analyseauftrag

1. Gib eine knappe, aber fundierte Gesamtbewertung von ThemisDB:
   - Reifegrad
   - Architekturprinzipien
   - Hauptstärken
   - wichtigste offene Baustellen

2. Leite daraus die nächsten 5 bis 10 sinnvollsten Entwicklungsschritte ab, priorisiert nach:
   - Wirkung
   - Risiko
   - Abhängigkeiten
   - Release-Relevanz

3. Für jeden Schritt nenne:
   - Ziel
   - Begründung
   - betroffene Module und Dateien
   - Abhängigkeiten
   - Risiken und mögliche Nebenwirkungen
   - empfohlene Tests und Validierung
   - messbare Erfolgskriterien

4. Trenne die Empfehlungen in:
   - kurzfristig: 1 bis 2 Wochen
   - mittelfristig: 1 bis 2 Monate
   - strategisch: 1 Quartal und darüber hinaus

5. Prüfe auf Widersprüche zwischen:
   - Architektur
   - Roadmap
   - Future Enhancements

   Liste diese explizit auf und sage, welche Quelle aus deiner Sicht Vorrang hat.

6. Trenne Release- und Governance-Themen klar von reinen Feature-Themen.

7. Wenn nach dem Lesen noch Unsicherheiten bestehen, nenne konkrete Rückfragen oder Evidenzlücken statt zu raten.

## Ausgabeformat

- Zuerst eine Executive Summary in 5 bis 8 Sätzen
- Danach eine priorisierte Liste der nächsten Schritte
- Danach eine kurze Tabelle mit:
  - Schritt
  - Priorität
  - Aufwand
  - Risiko
  - Erwarteter Nutzen
- Zum Schluss ein Abschnitt mit dem Titel Offene Fragen und Evidenzlücken

Wenn du möchtest, kann ich dir daraus noch eine zweite Version machen, die stärker auf Architektur, Produkt-Roadmap oder Release- und Governance-Entscheidungen optimiert ist.