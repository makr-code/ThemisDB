"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tabs/raw_editor_tab.py                             ║
  Module:          tools/themis_config_editor                         ║
  Description:     Raw YAML/JSON editor tab with validation.          ║
                   Syncs bidirectionally with the shared config dict.  ║
╚═════════════════════════════════════════════════════════════════════╝
"""

from __future__ import annotations

import json
import tkinter as tk
from tkinter import messagebox, ttk
from typing import Any, Callable, Dict, Optional

import yaml

from ..config_io import ConfigIO

# Exceptions produced by the two parsers we support
_PARSE_ERRORS = (yaml.YAMLError, json.JSONDecodeError, ValueError)


class RawEditorTab(ttk.Frame):
    """Full-text YAML/JSON editor for the ThemisDB configuration.

    Exposes the same ``load_from_config`` / ``save_to_config`` interface
    as :class:`~tabs.base_tab.BaseTab` so the application can treat all
    tabs uniformly.

    The editor also provides inline validation and a format-toggle button
    (YAML ↔ JSON).
    """

    def __init__(
        self,
        parent: tk.Widget,
        on_change: Optional[Callable[[], None]] = None,
    ) -> None:
        super().__init__(parent)
        self._on_change = on_change
        self._fmt = "yaml"          # current serialization format
        self._suppress_trace = False

        self._build_ui()

    # ------------------------------------------------------------------
    # UI construction
    # ------------------------------------------------------------------

    def _build_ui(self) -> None:
        # --- toolbar ---
        toolbar = ttk.Frame(self)
        toolbar.pack(fill=tk.X, padx=6, pady=(6, 0))

        self._fmt_var = tk.StringVar(value="YAML")
        ttk.Label(toolbar, text="Format:").pack(side=tk.LEFT, padx=(0, 4))
        self._yaml_btn = ttk.Radiobutton(
            toolbar, text="YAML", variable=self._fmt_var, value="YAML",
            command=self._on_format_change
        )
        self._yaml_btn.pack(side=tk.LEFT)
        self._json_btn = ttk.Radiobutton(
            toolbar, text="JSON", variable=self._fmt_var, value="JSON",
            command=self._on_format_change
        )
        self._json_btn.pack(side=tk.LEFT, padx=(4, 12))

        ttk.Button(toolbar, text="Validieren", command=self._validate).pack(
            side=tk.LEFT
        )
        ttk.Button(toolbar, text="Formatieren", command=self._reformat).pack(
            side=tk.LEFT, padx=(4, 0)
        )

        self._status_var = tk.StringVar(value="")
        self._status_lbl = ttk.Label(
            toolbar, textvariable=self._status_var, foreground="#005500"
        )
        self._status_lbl.pack(side=tk.LEFT, padx=(12, 0))

        # --- text editor with line numbers ---
        editor_frame = ttk.Frame(self)
        editor_frame.pack(fill=tk.BOTH, expand=True, padx=6, pady=6)

        # Line-number canvas
        self._linenos = tk.Canvas(
            editor_frame, width=40, background="#f0f0f0",
            highlightthickness=0
        )
        self._linenos.pack(side=tk.LEFT, fill=tk.Y)

        # Scrollbars (store vbar as instance attr for yscrollcommand handler)
        self._vbar = ttk.Scrollbar(editor_frame, orient=tk.VERTICAL)
        hbar = ttk.Scrollbar(editor_frame, orient=tk.HORIZONTAL)
        self._vbar.pack(side=tk.RIGHT, fill=tk.Y)
        hbar.pack(side=tk.BOTTOM, fill=tk.X)

        # Main text widget
        self._text = tk.Text(
            editor_frame,
            wrap=tk.NONE,
            undo=True,
            font=("Courier New", 10),
            yscrollcommand=self._on_yscroll,
            xscrollcommand=hbar.set,
            background="#1e1e1e",
            foreground="#d4d4d4",
            insertbackground="#ffffff",
            selectbackground="#264f78",
        )
        self._text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        self._vbar.configure(command=self._text.yview)
        hbar.configure(command=self._text.xview)

        self._text.bind("<<Modified>>", self._on_text_modified)
        self._text.bind("<KeyRelease>", self._update_linenos)
        self._text.bind("<ButtonRelease>", self._update_linenos)

        self._update_linenos()

    # ------------------------------------------------------------------
    # Line-number rendering
    # ------------------------------------------------------------------

    def _on_yscroll(self, first: Any, last: Any) -> None:
        """Combined yscrollcommand handler: update scrollbar + line numbers."""
        self._vbar.set(first, last)
        self._update_linenos()

    def _update_linenos(self, _event: Any = None) -> None:
        """Redraw the line-number canvas to match the visible text region."""
        self._linenos.delete("all")
        pos = self._text.index("@0,0")
        last_y = -1
        while True:
            dline = self._text.dlineinfo(pos)
            if dline is None:
                break
            _, y, _, height, _ = dline
            if y == last_y:
                break  # no forward progress
            last_y = y
            line_num = int(pos.split(".")[0])
            self._linenos.create_text(
                36, y + height // 2,
                anchor="e",
                text=str(line_num),
                fill="#999999",
                font=("Courier New", 10),
            )
            next_pos = self._text.index(f"{pos}+1line")
            if next_pos == pos:
                break
            pos = next_pos

    # ------------------------------------------------------------------
    # Event handlers
    # ------------------------------------------------------------------

    def _on_text_modified(self, _event: Any = None) -> None:
        if self._suppress_trace:
            return
        self._text.edit_modified(False)
        self._set_status("")
        if self._on_change:
            self._on_change()

    def _on_format_change(self) -> None:
        """Convert the editor content between YAML and JSON."""
        new_fmt = self._fmt_var.get().lower()
        if new_fmt == self._fmt:
            return
        try:
            current_text = self._text.get("1.0", tk.END)
            data = ConfigIO.deserialize(current_text, self._fmt)
            self._fmt = new_fmt
            self._load_text(ConfigIO.serialize(data, self._fmt))
            self._set_status(f"Format geändert → {new_fmt.upper()}")
        except _PARSE_ERRORS as exc:
            messagebox.showerror(
                "Format-Fehler",
                f"Kann nicht nach {new_fmt.upper()} konvertieren:\n{exc}",
            )
            # Revert radio button
            self._fmt_var.set(self._fmt.upper())

    # ------------------------------------------------------------------
    # Validation / Reformatting
    # ------------------------------------------------------------------

    def _validate(self) -> None:
        text = self._text.get("1.0", tk.END)
        try:
            ConfigIO.deserialize(text, self._fmt)
            self._set_status("✔ Syntax korrekt", color="#005500")
        except _PARSE_ERRORS as exc:
            self._set_status(f"✘ Fehler: {exc}", color="#aa0000")

    def _reformat(self) -> None:
        text = self._text.get("1.0", tk.END)
        try:
            data = ConfigIO.deserialize(text, self._fmt)
            self._load_text(ConfigIO.serialize(data, self._fmt))
            self._set_status("Formatiert.", color="#005500")
        except _PARSE_ERRORS as exc:
            self._set_status(f"✘ Formatierung fehlgeschlagen: {exc}", color="#aa0000")

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _set_status(self, msg: str, color: str = "#005500") -> None:
        self._status_var.set(msg)
        self._status_lbl.configure(foreground=color)

    def _load_text(self, content: str) -> None:
        self._suppress_trace = True
        try:
            self._text.delete("1.0", tk.END)
            self._text.insert("1.0", content)
            self._text.edit_modified(False)
        finally:
            self._suppress_trace = False
        self._update_linenos()

    # ------------------------------------------------------------------
    # Data synchronisation (same interface as BaseTab)
    # ------------------------------------------------------------------

    def load_from_config(self, data: Dict[str, Any]) -> None:
        """Serialize *data* into the text widget."""
        content = ConfigIO.serialize(data, self._fmt)
        self._load_text(content)

    def save_to_config(self, data: Dict[str, Any]) -> None:
        """Parse the text content and merge it into *data* in-place."""
        text = self._text.get("1.0", tk.END)
        try:
            parsed = ConfigIO.deserialize(text, self._fmt)
        except _PARSE_ERRORS as exc:
            messagebox.showerror(
                "Parse-Fehler",
                f"Konfiguration konnte nicht geparst werden:\n{exc}\n\n"
                "Bitte Fehler korrigieren, bevor die Datei gespeichert wird.",
            )
            return
        data.clear()
        data.update(parsed)

    @property
    def current_format(self) -> str:
        """Return the current serialization format ('yaml' or 'json')."""
        return self._fmt
