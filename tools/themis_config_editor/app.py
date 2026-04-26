"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            app.py                                             ║
  Module:          tools/themis_config_editor                         ║
  Description:     Main application window.                           ║
                   Orchestrates the notebook tabs, file I/O,          ║
                   toolbar, menu bar, and status bar.                 ║
╚═════════════════════════════════════════════════════════════════════╝
"""

from __future__ import annotations

import copy
import json
import tkinter as tk
from pathlib import Path
from tkinter import filedialog, messagebox, ttk
from typing import Any, Dict, List, Optional, Union

import yaml

from .config_io import ConfigIO
from .schema import TAB_SCHEMA
from .tabs.base_tab import BaseTab
from .tabs.raw_editor_tab import RawEditorTab

# ---------------------------------------------------------------------------
# Default search paths (mirrors ThemisDB C++ loader order)
# ---------------------------------------------------------------------------
_DEFAULT_SEARCH_PATHS = [
    Path("./config.yaml"),
    Path("./config/config.yaml"),
    Path("/etc/vccdb/config.yaml"),
]

_WINDOW_TITLE = "ThemisDB Config Editor"
_MIN_WIDTH = 900
_MIN_HEIGHT = 650


class ThemisConfigApp(tk.Tk):
    """Top-level application window for the ThemisDB configuration editor.

    Responsibilities
    ----------------
    * Menu bar (File / Help)
    * Toolbar (Open, Save, Save As)
    * ``ttk.Notebook`` with one tab per schema entry plus a raw editor
    * Status bar showing current file path and modification state
    * Bidirectional sync between form tabs and the raw editor tab

    The shared config state is held in ``self._config_data`` (a plain dict).
    All tabs read from / write to this dict via their ``load_from_config`` /
    ``save_to_config`` methods.
    """

    def __init__(self) -> None:
        super().__init__()
        self.title(_WINDOW_TITLE)
        self.minsize(_MIN_WIDTH, _MIN_HEIGHT)
        self.geometry("1100x740")

        self._config_data: Dict[str, Any] = {}
        self._current_path: Optional[Path] = None
        self._modified = False

        # Tab references (BaseTab or RawEditorTab instances, in order)
        self._tabs: List[Union[BaseTab, RawEditorTab]] = []
        self._raw_tab_index: int = -1

        self._build_menu()
        self._build_toolbar()
        self._build_notebook()
        self._build_statusbar()

        self.protocol("WM_DELETE_WINDOW", self._on_close)

        # Auto-load the first config file found in default paths
        for candidate in _DEFAULT_SEARCH_PATHS:
            if candidate.exists():
                self._load_file(candidate)
                break

    # ------------------------------------------------------------------
    # UI construction
    # ------------------------------------------------------------------

    def _build_menu(self) -> None:
        menubar = tk.Menu(self)

        # File menu
        file_menu = tk.Menu(menubar, tearoff=False)
        file_menu.add_command(label="Öffnen…",
                              accelerator="Ctrl+O", command=self._cmd_open)
        file_menu.add_command(label="Speichern",
                              accelerator="Ctrl+S", command=self._cmd_save)
        file_menu.add_command(label="Speichern unter…",
                              accelerator="Ctrl+Shift+S", command=self._cmd_save_as)
        file_menu.add_separator()
        file_menu.add_command(label="Neu (leer)", command=self._cmd_new)
        file_menu.add_separator()
        file_menu.add_command(label="Beenden", command=self._on_close)
        menubar.add_cascade(label="Datei", menu=file_menu)

        # Help menu
        help_menu = tk.Menu(menubar, tearoff=False)
        help_menu.add_command(label="Über…", command=self._cmd_about)
        menubar.add_cascade(label="Hilfe", menu=help_menu)

        self.configure(menu=menubar)

        # Keyboard shortcuts
        self.bind_all("<Control-o>", lambda _e: self._cmd_open())
        self.bind_all("<Control-s>", lambda _e: self._cmd_save())
        self.bind_all("<Control-S>", lambda _e: self._cmd_save_as())

    def _build_toolbar(self) -> None:
        bar = ttk.Frame(self, relief=tk.RIDGE, padding=(4, 2))
        bar.pack(fill=tk.X, side=tk.TOP)

        ttk.Button(bar, text="📂 Öffnen", command=self._cmd_open).pack(
            side=tk.LEFT, padx=2
        )
        ttk.Button(bar, text="💾 Speichern", command=self._cmd_save).pack(
            side=tk.LEFT, padx=2
        )
        ttk.Button(bar, text="Speichern unter…", command=self._cmd_save_as).pack(
            side=tk.LEFT, padx=2
        )

        ttk.Separator(bar, orient=tk.VERTICAL).pack(side=tk.LEFT, fill=tk.Y, padx=6)

        self._file_label_var = tk.StringVar(value="Keine Datei geöffnet")
        ttk.Label(bar, textvariable=self._file_label_var,
                  foreground="#444444").pack(side=tk.LEFT, padx=4)

    def _build_notebook(self) -> None:
        self._notebook = ttk.Notebook(self)
        self._notebook.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        for idx, (tab_name, fields) in enumerate(TAB_SCHEMA):
            if tab_name == "Rohbearbeitung":
                tab = RawEditorTab(self._notebook, on_change=self._mark_modified)
                self._raw_tab_index = idx
            else:
                tab = BaseTab(self._notebook, fields,
                              on_change=self._mark_modified)
            self._tabs.append(tab)
            self._notebook.add(tab, text=f" {tab_name} ")

        self._notebook.bind("<<NotebookTabChanged>>", self._on_tab_change)

    def _build_statusbar(self) -> None:
        bar = ttk.Frame(self, relief=tk.SUNKEN, padding=(4, 1))
        bar.pack(fill=tk.X, side=tk.BOTTOM)

        self._status_var = tk.StringVar(value="Bereit")
        ttk.Label(bar, textvariable=self._status_var,
                  anchor=tk.W).pack(side=tk.LEFT, fill=tk.X, expand=True)

        self._modified_var = tk.StringVar(value="")
        ttk.Label(bar, textvariable=self._modified_var,
                  foreground="#aa4400").pack(side=tk.RIGHT, padx=4)

    # ------------------------------------------------------------------
    # Tab sync
    # ------------------------------------------------------------------

    def _on_tab_change(self, _event: Any = None) -> None:
        """Sync data when switching to/from the raw editor tab."""
        current_idx = self._notebook.index(self._notebook.select())

        if current_idx == self._raw_tab_index:
            # Switching TO raw editor: flush form data → config → raw text
            self._collect_form_data()
            raw_tab = self._tabs[self._raw_tab_index]
            assert isinstance(raw_tab, RawEditorTab)
            raw_tab.load_from_config(self._config_data)
        else:
            # Switching FROM raw editor: parse raw text → config → form fields
            if self._raw_tab_index >= 0:
                raw_tab = self._tabs[self._raw_tab_index]
                assert isinstance(raw_tab, RawEditorTab)
                raw_tab.save_to_config(self._config_data)
            self._populate_form_tabs()

    def _collect_form_data(self) -> None:
        """Write all form-tab values into ``self._config_data``."""
        for idx, tab in enumerate(self._tabs):
            if idx == self._raw_tab_index:
                continue
            if isinstance(tab, BaseTab):
                tab.save_to_config(self._config_data)

    def _populate_form_tabs(self) -> None:
        """Read ``self._config_data`` into all form tabs."""
        for idx, tab in enumerate(self._tabs):
            if idx == self._raw_tab_index:
                continue
            if isinstance(tab, BaseTab):
                tab.load_from_config(self._config_data)

    # ------------------------------------------------------------------
    # File I/O
    # ------------------------------------------------------------------

    def _load_file(self, path: Path) -> None:
        try:
            self._config_data = ConfigIO.load(path)
        except (OSError, yaml.YAMLError, json.JSONDecodeError, ValueError) as exc:
            messagebox.showerror(
                "Ladefehler",
                f"Datei konnte nicht geladen werden:\n{path}\n\n{exc}",
            )
            return

        self._current_path = path
        self._modified = False
        self._populate_form_tabs()

        # Sync raw editor if it is the active tab
        active = self._notebook.index(self._notebook.select())
        if active == self._raw_tab_index:
            raw_tab = self._tabs[self._raw_tab_index]
            assert isinstance(raw_tab, RawEditorTab)
            raw_tab.load_from_config(self._config_data)

        self._update_title()
        self._set_status(f"Geladen: {path}")

    def _save_file(self, path: Path) -> bool:
        """Collect all tab data and write the file.  Returns True on success."""
        # If raw editor is active, parse it first
        active = self._notebook.index(self._notebook.select())
        if active == self._raw_tab_index:
            raw_tab = self._tabs[self._raw_tab_index]
            assert isinstance(raw_tab, RawEditorTab)
            snapshot = copy.deepcopy(self._config_data)
            raw_tab.save_to_config(snapshot)
            data_to_save = snapshot
        else:
            self._collect_form_data()
            data_to_save = self._config_data

        fmt = ConfigIO.detect_format(path)
        try:
            ConfigIO.save(path, data_to_save, fmt)
        except OSError as exc:
            messagebox.showerror(
                "Speicherfehler",
                f"Datei konnte nicht gespeichert werden:\n{path}\n\n{exc}",
            )
            return False

        self._config_data = data_to_save
        self._current_path = path
        self._modified = False
        self._update_title()
        self._set_status(f"Gespeichert: {path}")
        return True

    # ------------------------------------------------------------------
    # Commands (menu / toolbar actions)
    # ------------------------------------------------------------------

    def _cmd_open(self) -> None:
        if self._modified and not self._ask_discard():
            return
        path_str = filedialog.askopenfilename(
            title="ThemisDB-Konfigurationsdatei öffnen",
            filetypes=[
                ("YAML-Dateien", "*.yaml *.yml"),
                ("JSON-Dateien", "*.json"),
                ("Alle Dateien", "*.*"),
            ],
            initialdir=str(self._current_path.parent)
            if self._current_path else ".",
        )
        if path_str:
            self._load_file(Path(path_str))

    def _cmd_save(self) -> None:
        if self._current_path:
            self._save_file(self._current_path)
        else:
            self._cmd_save_as()

    def _cmd_save_as(self) -> None:
        initial_file = str(self._current_path) if self._current_path else "config.yaml"
        path_str = filedialog.asksaveasfilename(
            title="Konfiguration speichern unter…",
            defaultextension=".yaml",
            filetypes=[
                ("YAML-Dateien", "*.yaml *.yml"),
                ("JSON-Dateien", "*.json"),
                ("Alle Dateien", "*.*"),
            ],
            initialfile=initial_file,
        )
        if path_str:
            self._save_file(Path(path_str))

    def _cmd_new(self) -> None:
        if self._modified and not self._ask_discard():
            return
        self._config_data = {}
        self._current_path = None
        self._modified = False
        self._populate_form_tabs()
        active = self._notebook.index(self._notebook.select())
        if active == self._raw_tab_index:
            raw_tab = self._tabs[self._raw_tab_index]
            assert isinstance(raw_tab, RawEditorTab)
            raw_tab.load_from_config(self._config_data)
        self._update_title()
        self._set_status("Neue leere Konfiguration.")

    def _cmd_about(self) -> None:
        messagebox.showinfo(
            "Über ThemisDB Config Editor",
            "ThemisDB Config Editor\n"
            "Version 1.0.0\n\n"
            "GUI-Tool zur Bearbeitung von ThemisDB-Konfigurationsdateien\n"
            "(YAML / JSON) mit thematisch sortierten Tabs.\n\n"
            "Standard-Suchpfade:\n"
            + "\n".join(f"  • {p}" for p in _DEFAULT_SEARCH_PATHS),
        )

    # ------------------------------------------------------------------
    # Helpers
    # ------------------------------------------------------------------

    def _mark_modified(self) -> None:
        if not self._modified:
            self._modified = True
            self._update_title()

    def _update_title(self) -> None:
        name = str(self._current_path) if self._current_path else "Unbenannt"
        mod = " *" if self._modified else ""
        self.title(f"{_WINDOW_TITLE} — {name}{mod}")
        self._file_label_var.set(name)
        self._modified_var.set("● Ungespeicherte Änderungen" if self._modified else "")

    def _set_status(self, msg: str) -> None:
        self._status_var.set(msg)

    def _ask_discard(self) -> bool:
        """Ask the user whether to discard unsaved changes.

        Returns True if it is safe to proceed, False to cancel.
        """
        return messagebox.askyesno(
            "Ungespeicherte Änderungen",
            "Es gibt ungespeicherte Änderungen. Trotzdem fortfahren?",
            icon="warning",
        )

    def _on_close(self) -> None:
        if self._modified and not self._ask_discard():
            return
        self.destroy()
