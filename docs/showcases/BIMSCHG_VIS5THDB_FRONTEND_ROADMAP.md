---
Author: "ThemisDB Team"
Created: "2026-11-19"
Last Updated: "2026-11-19"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
Source_Level: level3
SOT_Domain: e-government-showcase
Canonical_References:
  - DOCUMENTATION_GOVERNANCE.md
  - docs/showcases/BIMSCHG_VERWALTUNGSVERFAHREN.md
  - https://github.com/makr-code/VIS5ThDB
---

# VIS5ThDB-Frontend-Roadmap — kanonischer Ort

> Exact model backend not verifiable from final document text alone.

Die detaillierte Analyse und Umsetzungsroadmap für **behördliche Überwachung
und Aktenführung** wird eigenständig im Repository
[makr-code/VIS5ThDB](https://github.com/makr-code/VIS5ThDB) gepflegt:

**Datei im Zielrepository:** `docs\BIMSCHG_FRONTEND_ROADMAP.md`

**Aktueller lokaler Arbeitsstand:** `C:\Projects\VIS5ThDB\docs\BIMSCHG_FRONTEND_ROADMAP.md`

Dort liegen Ist-Analyse, Arbeitspakete BIM-VIS-00 bis BIM-VIS-08,
Abnahmekriterien, externe Server-Abhängigkeiten und der Eingangsvertrag zum
separaten Beteiligungs-/Beschwerde-Frontend. Der Einstieg ist auch in der
VIS5ThDB-README, dem Dokumentationsindex und den Copilot-Regeln verankert.
Die Übernahme vom 2026-11-19 ist eine lokale Dokumentationsänderung, kein
veröffentlichter Release und keine abgeschlossene Funktionsimplementierung.

ThemisDB bleibt fachliche Herkunft des
[BImSchG-Lebenszyklusmodells](BIMSCHG_VERWALTUNGSVERFAHREN.md) und
eigenständig bereitgestelltes Backend. Der
[Demo-Datensatz](bimschg-brandenburg-demo.json),
der [Verwaltungsakt-Prozessgraph](bimschg-verwaltungsakt-bpmn.json) und die
[optionalen Prüfprofile](../../config/agent_skills/process-bpmn-mining/references/verwaltungsakt-pruefung.yaml)
bleiben hier. Sie sind keine implizite Build-Abhängigkeit für VIS5ThDB.

Diese Seite ist ausschließlich ein Herkunfts- und Navigationsverweis.
Frontend-Paketstatus wird **nicht** parallel in ThemisDB gepflegt.
Server-Vertragslücken werden separat an ThemisDB übergeben; lokale
Frontend-Arbeit setzt weder einen parallelen Serverumbau noch die vollständige
gRPC-Roadmap voraus. Rechte, Integrität und Audit müssen für den Echtbetrieb
trotzdem am Server nachgewiesen sein.
