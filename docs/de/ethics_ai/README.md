# Ethics-AI-Modul

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: PRIMARY_SOURCES.md · ../../../src/ethics_ai/ -->

**Stand:** 6. April 2026  
**Version:** aktuell  
**Kategorie:** KI-Ethik / Governance  
**Status:** 🟡 Beta

---

## Übersicht

Das Ethics-AI-Modul implementiert ethische KI-Bewertung und Argumentation für ThemisDB-Operationen. Es stellt einen RAG-gestützten Ethik-Evaluator und einen philosophischen Argumentations-Engine bereit.

**Primäre Quelle:** [`src/ethics_ai/`](../../../src/ethics_ai/)

---

## Kernkomponenten

| Komponente | Source | Beschreibung |
|------------|--------|--------------|
| EthicsAIPlugin | `ethics_ai_plugin.cpp` | Plugin-Einstiegspunkt und Registrierung |
| EthicsEvaluator | `ethics_evaluator.cpp` | KI-Ethik-Bewertungsmotor |
| DiscourseEngine | `discourse_engine.cpp` | Philosophischer Argumentations-Engine |
| PhilosophyLoader | `philosophy_loader.cpp` | Philosophische Wissensladung (Rawls, Kant, Utilitarismus) |
| ArgumentStore | `argument_store.cpp` | Argumentations-Persistenz |
| RagContextEngine | `rag_context_engine.cpp` | RAG-Kontext-Engine für ethische Begründungen |
| EthicsAITypes | `ethics_ai_types.cpp` | Gemeinsame Datentypen |

---

## Primäre Dokumentation

| Dokument | Beschreibung |
|----------|--------------|
| [`src/ethics_ai/`](../../../src/ethics_ai/) | Implementierungen |
