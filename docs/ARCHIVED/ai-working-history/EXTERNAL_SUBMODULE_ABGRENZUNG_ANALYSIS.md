# External Submodule Abgrenzung — L0-L3 Analyse

**Datum**: 2026-06-25 | **Status**: ✅ VERIFIED

---

## 📊 Findings: Abgrenzung zu externen GitHub-Submodulen

### ✅ Ja, die Abgrenzung wurde berücksichtigt — aber IMPLIZIT, nicht EXPLIZIT

---

## 1️⃣ Externe Submodule im Repository

Die ThemisDB hat folgende externe GitHub-Submodule:

| Submodul | Typ | Status | Scan-Behandlung |
|----------|-----|--------|-----------------|
| `llama.cpp` | Directory | Vorhanden | [INCLUDED in L0 raw] |
| `whisper.cpp` | Symlink | Vorhanden | [INCLUDED in L0 raw] |
| `vcpkg` | Symlink | Vorhanden | [INCLUDED in L0 raw] |
| `vcpkg_installed` | Directory | Vorhanden | [INCLUDED in L0 raw] |
| `onnx-clip` | (Referenced) | Vorhanden | [INCLUDED in L0 raw] |

**Scanner Status**: Der `gs3_orchestrator.py` mit Phase 1-11 Scanner scannt diese Module **vollständig**.

---

## 2️⃣ Scope-Klassifizierung (Implicit Segregation)

Die L0-Ergebnisse kategorisieren Findings in 4 Scopes:

```python
def _classify_scope(path: str) -> str:
    if path.startswith('tests/'):
        return 'themis_tests'
    elif path.startswith('benchmarks/'):
        return 'themis_benchmarks'
    elif path.startswith(('src/', 'include/', 'tools/', 'scripts/', 'cmake/', 'docs/', 'examples/')):
        return 'themis_core'
    else:
        return 'third_party'  # ← Externe Submodule landen hier
```

**Klassifizierung der externen Submodule**:

```
llama.cpp/src/ggml.cpp              → third_party (nicht in themis_core-Präfix)
whisper.cpp/whisper.cpp             → third_party
vcpkg/ports/grpc/portfile.cmake     → third_party
onnx-clip/models/...                → third_party
```

---

## 3️⃣ L0-L3 Propagation: Filterung

### ✅ L0 (Raw Scan)
- **Enthält**: Alle Findings (themis_core + themis_tests + themis_benchmarks + **third_party**)
- **Metadata**: `scope_breakdown: { themis_core: ..., third_party: ... }`
- **Beispiel Output**: 1821 raw findings für src/graph

### ✅ L0.5 (Gap Verification)
- **Filterung**: KEINE explizite Filterung — AI-Verifikation läuft auf **ALLEN** Scopes
- **Status**: 22,160 verifizierte Gaps (wahrscheinlich mit third_party-Anteil)

### ✅ L1 (Module Documentation)
- **Filter Applied**: ✅ **Nur themis_core Module**
- **Beweis**: 66 MODULE_GAPS.md Dateien in `src/*/`
- **Keine Einträge für**: llama.cpp, whisper.cpp, vcpkg, onnx-clip
- **Verifikation**: `grep -r 'llama.cpp\|whisper\|vcpkg' src/*/MODULE_GAPS.md` → 0 Treffer

### ✅ L2 (Aggregates)
- **Filter Applied**: ✅ **Nur themis_core Module**
- **Quelle**: Aggregiert aus L1 (daher nur themis_core)
- **Datei**: `ai_working/MODULE_SNAPSHOT_AGGREGATE_L2.md`

### ✅ L3 (Root Documentation)
- **Filter Applied**: ✅ **Nur themis_core Module**
- **Updated**: CHANGELOG.md (nur themis_core Findings)
- **Updated**: README.md, ARCHITECTURE.md (nur themis_core Status)
- **Keine Erwähnung**: llama.cpp, whisper.cpp, vcpkg

---

## 🤔 Problem: Implizite vs. Explizite Filterung

### Current State (IMPLIZIT)

**Wie Filterung derzeit funktioniert**:

1. ✅ L0: Scanner scannt ALLES (externe + themis_core)
2. ✅ L0.5: Verifikation läuft auf ALLEN Findings
3. ✅ L1-L3: doc-orchestrator wählt automatisch nur Module in `src/*/` aus
   - **Grund**: Schema erwartet `src/<MODULE>/README.md` + `src/<MODULE>/ROADMAP.md`
   - **Resultat**: Externe Submodule (llama.cpp/, vcpkg/, etc.) sind nicht in `src/*/` → werden ignoriert

### ⚠️ Risiko: Unbeabsichtigte Einführung

Wenn in Zukunft jemand externe Submodule in `src/` verschieben würde (z.B. `src/llama_cpp/`), würde die L1-L3 Propagation **automatisch** externe Submodule dokumentieren — **ohne explizite Abgrenzung**.

---

## 🛠️ Empfehlungen

### Option 1: Status Quo (IMPLIZIT) ✅ **AKTUELL**
- ✅ Funktioniert gut für aktuelle Struktur
- ⚠️ Abhängig von Directory-Struktur (`src/` vs `llama.cpp/`, etc.)
- ⚠️ Nicht robust gegen Refactoring

### Option 2: Explizite Filterung (EMPFOHLEN) 🎯

**Implementierung**:

```python
# In tools/gs3_orchestrator.py oder gap-verifier agent

EXTERNAL_SUBMODULES = {
    'llama.cpp',
    'whisper.cpp', 
    'vcpkg',
    'vcpkg_installed',
    'onnx-clip',
}

def is_external_submodule(file_path: str) -> bool:
    """Check if a finding belongs to external submodule"""
    normalized = file_path.replace('\\', '/').lower()
    return any(sub in normalized for sub in EXTERNAL_SUBMODULES)

def filter_external_submodules(gaps: list[Gap]) -> list[Gap]:
    """Remove findings from external submodules"""
    return [g for g in gaps if not is_external_submodule(g.file)]
```

**Wo anwenden**:
- In L0.5 gap-verifier: Explizit filtern vor export
- In L1-L3 orchestrator: Explizit filtern vor MODULE_GAPS.md creation
- In CHANGELOG export: Nur themis_core findings

**Vorteile**:
- ✅ Explizit dokumentiert (nicht implizit von Directory-Struktur abhängig)
- ✅ Robust gegen Refactoring
- ✅ Keine Überraschungen, wenn Struktur sich ändert

---

## 📋 Validation Checklist

| Kriterium | Status | Beweis |
|-----------|--------|--------|
| Externe Submodule im L0-Scanner? | ✅ Ja (gescannt) | Phase 1-11 läuft auf llama.cpp/, whisper.cpp/, vcpkg/ |
| Klassifizierung als third_party? | ✅ Ja | _classify_scope() kategorisiert sie richtig |
| Filterung in L0.5? | ⚠️ Teilweise | Verifikation läuft auf ALLEN, aber nicht gewünscht |
| Filterung in L1? | ✅ Ja | MODULE_GAPS.md nur in src/<MODULE>/ |
| Filterung in L2? | ✅ Ja | Aggregiert nur aus themis_core L1s |
| Filterung in L3? | ✅ Ja | CHANGELOG/README/ARCHITECTURE nur themis_core |
| Explizite Filterung dokumentiert? | ❌ Nein | Nur implizit via Directory-Struktur |

---

## 🎓 Conclusion

**Ist die Abgrenzung zu externen GitHub-Submodulen berücksichtigt?**

### Antwort: **JA, aber IMPLIZIT** ✅

**Was wurde korrekt gemacht**:
1. ✅ Scanner kennt externe Submodule und kategorisiert sie als `third_party`
2. ✅ L1-L3 Propagation enthält KEINE external submodule Dokumentation
3. ✅ MODULE_GAPS.md, CHANGELOG.md, README.md sind sauber (nur themis_core)

**Was könnte verbessert werden**:
1. ⚠️ Filterung ist implizit (Directory-Struktur abhängig), nicht explizit dokumentiert
2. ⚠️ L0.5 gap-verifier wird auf 100% der Findings (incl. external) angewendet
3. ⚠️ Keine `--exclude-external-submodules` CLI-Option für Scanner

**Empfehlung**:
- **Kurz-Term**: Status Quo ist akzeptabel, funktioniert gut
- **Mittel-Term**: Explizite Filterung in L0.5 (gap-verifier) einführen
- **Lang-Term**: `--exclude-external-submodules` Flag im Scanner hinzufügen + dokumentieren

---

**Nächste Schritte**:
1. Bestätigen mit Produktteam: Ist implizite Filterung ausreichend?
2. Falls nein: Phase 5 Implementation für explizite externe-Submodul-Filterung planen
3. Falls ja: Diese Analyse als Governance-Dokument für zukünftige Änderungen speichern
