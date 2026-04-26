# ThemisDB – Projektindex (rekursiv kondensiert)

> Stand: 2026-04-19
> Quelle: tatsächliche Verzeichnisstruktur im Repository

Dieser Index ist auf die Entwickler-Primärquellen fokussiert und ersetzt veraltete Pfad-/Statusaussagen.

## Primäre Entwicklerbereiche

| Bereich | Markdown-Dateien | Einstieg |
|---|---:|---|
| `src/` | 424 | [`src/README.md`](src/README.md) |
| `include/` | 64 | [`include/README.md`](include/README.md) |
| `examples/` | 320 | [`examples/README.md`](examples/README.md) |
| `tools/` | 170 | [`tools/README.md`](tools/README.md) |
| `benchmarks/` | 165 | [`benchmarks/README.md`](benchmarks/README.md), [`benchmarks/INDEX.md`](benchmarks/INDEX.md) |
| `tests/` | 145 | [`tests/README.md`](tests/README.md) |

## Root-Einstiege

- [`README.md`](README.md) – Produkt- und Build-Überblick
- [`roadmap.md`](roadmap.md) – Modulübergreifender Umsetzungsstand
- [`CHANGELOG.md`](CHANGELOG.md) – verifizierte Änderungen
- [`CMakePresets.json`](CMakePresets.json) – Build-/Test-Quelle der Wahrheit

## Verifizierbare Build/Test-Kommandos

```bash
cmake --list-presets
cmake --preset linux-ninja-release
cmake --build --preset linux-ninja-release
ctest --preset linux-ninja-release
```

## Hinweis zu Alt-Dokumenten

Historische Detailreports in Unterordnern (insbesondere Benchmarks/Tools) bleiben erhalten, sind aber nicht mehr Root-Navigationsquelle. Root-Links zeigen nur auf aktuell verifizierte Einstiege.
