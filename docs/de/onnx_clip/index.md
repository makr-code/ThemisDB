[docs](../../README.md) > [de](../INDEX.md) > [onnx_clip](./index.md)
**Datum:** 2026-05-13
**Status:** review
**Primary (Quelle der Wahrheit):**
- `include/onnx_clip/README.md`
- `src/onnx_clip/README.md`
- `src/onnx_clip/ARCHITECTURE.md`
- `src/onnx_clip/ROADMAP.md`
- `src/onnx_clip/FUTURE_ENHANCEMENTS.md`

**Bezug / Reference:**
- Issue: `[Docs][Module] onnx_clip`
- Kontext: Modulweise Doku-Aktualisierung für öffentliche Oberfläche, Laufzeitverhalten und Querverweise.

---

# onnx_clip — Modulübersicht

Diese Sekundärdoku verankert die aktualisierte Modulbeschreibung für `onnx_clip`
gegen den aktuellen Quellcode-Stand.

## Dokumente

- [README](./README.md)
- [PRIMARY_SOURCES](./PRIMARY_SOURCES.md)

## Kernaussagen

- Öffentliche Nutzung ist dokumentiert, obwohl der Header aktuell source-lokal in `src/onnx_clip/onnx_clip_plugin.h` lebt.
- Die API umfasst Bild-Embedding, Text-Embedding, Batch-Verarbeitung, Statistik, Health-Check und optionalen Modell-Hash-Check.
- `BackendType::AUTO` wählt im aktuellen generischen Build deterministisch `CPU`.
- Offene Folgearbeiten bleiben in [`../../../src/onnx_clip/ROADMAP.md`](../../../src/onnx_clip/ROADMAP.md) und [`../../../src/onnx_clip/FUTURE_ENHANCEMENTS.md`](../../../src/onnx_clip/FUTURE_ENHANCEMENTS.md) dokumentiert.
