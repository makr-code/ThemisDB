"""
conftest.py – Projekt-weite pytest-Konfiguration für CHIMERA.

Problem: reporter.py und statistics.py etc. liegen im root-Verzeichnis und
verwenden relative Imports (from .statistics import ...). Das funktioniert nur,
wenn das Verzeichnis als Package (chimera) importiert wird.

Lösung: Das Parent-Verzeichnis wird zu sys.path hinzugefügt, damit das
root-Verzeichnis als "chimera"-Package importierbar ist. Zusätzlich werden
flache Aliase angelegt (reporter → chimera.reporter), damit run_endurance.py
weiterhin flat importieren kann.
"""

import importlib
import importlib.util
import pathlib
import sys


_ROOT = pathlib.Path(__file__).parent
_PARENT = _ROOT.parent

# Parent-Verzeichnis hinzufügen, damit "import chimera" als Package funktioniert
if str(_PARENT) not in sys.path:
    sys.path.insert(0, str(_PARENT))

# chimera als Package laden (löst relative Imports in reporter.py, statistics.py etc.)
if "chimera" not in sys.modules:
    try:
        import chimera  # noqa: F401
    except Exception:
        pass

# Flache Aliase: "reporter" → chimera.reporter etc.
# run_endurance.py importiert flat (from reporter import ChimeraReporter)
_FLAT_ALIASES = [
    "statistics",
    "reporter",
    "colors",
    "citations",
    "scoring",
    "benchmark_harness",
    "benchmark_standards",
    "exporter",
]
for _name in _FLAT_ALIASES:
    _pkg_key = f"chimera.{_name}"
    if _name not in sys.modules and _pkg_key in sys.modules:
        sys.modules[_name] = sys.modules[_pkg_key]
