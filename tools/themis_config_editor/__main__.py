"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            __main__.py                                        ║
  Module:          tools/themis_config_editor                         ║
  Description:     Entry point — run with:                            ║
                     python -m tools.themis_config_editor             ║
                   or (from tools/):                                  ║
                     python -m themis_config_editor                   ║
╚═════════════════════════════════════════════════════════════════════╝
"""

from __future__ import annotations

import sys


def main() -> None:
    try:
        import tkinter  # noqa: F401
    except ImportError:
        print(
            "Fehler: tkinter ist nicht verfügbar.\n"
            "Bitte python3-tk installieren (z.B. 'apt install python3-tk').",
            file=sys.stderr,
        )
        sys.exit(1)

    try:
        import yaml  # noqa: F401
    except ImportError:
        print(
            "Fehler: PyYAML ist nicht installiert.\n"
            "Bitte 'pip install pyyaml' ausführen.",
            file=sys.stderr,
        )
        sys.exit(1)

    from .app import ThemisConfigApp

    app = ThemisConfigApp()
    app.mainloop()


if __name__ == "__main__":
    main()
