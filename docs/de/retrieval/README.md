# Retrieval Module

<!-- Status: current | validated: 2026-10-29 -->
<!-- Links: ../../../src/retrieval/README.md · ../../../include/retrieval/README.md -->

## Overview

Das Modul `retrieval` enthaelt Artefakt-/Manifest- und Validierungsfunktionen.
Es ist nicht die generische Retrieval-Engine. Der gemeinsame Wissenszugriff liegt
bei `rag::RAGProvider`; siehe [RAG-Vertrag](../../rag/API.md).

## Primary sources

- `src/retrieval/` — Artefaktbibliothek und Validierungsfunktionen
- `include/retrieval/` — dazugehoerige Header, kein allgemeiner RAG-Vertrag
- `src/retrieval/ARCHITECTURE.md` — Modulgrenzen
- `src/retrieval/EMBEDDING.md` — embedding and candidate generation guidance

## Key responsibilities

- LoRA-Artefakt- und Manifestverarbeitung
- Validierungs- und Deployment-Gate-Funktionen
- Keine Orchestrierung allgemeiner Graph-/Geo-/Relational-/Vektorabfragen

## Related documentation

- [src/retrieval/README.md](../../../src/retrieval/README.md)
- [src/retrieval/ARCHITECTURE.md](../../../src/retrieval/ARCHITECTURE.md)
- [include/retrieval/README.md](../../../include/retrieval/README.md)

## Validation

Diese Abgrenzung beschreibt die tatsaechlichen Modulzustaendigkeiten; sie ist
kein Nachweis einer vollstaendig angebundenen RAG-Laufzeit.
