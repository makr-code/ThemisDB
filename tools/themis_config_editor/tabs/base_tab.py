"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tabs/base_tab.py                                   ║
  Module:          tools/themis_config_editor                         ║
  Description:     Schema-driven scrollable form tab.                 ║
                   Renders a list of FieldDef entries as a grid form  ║
                   inside a Canvas-based scrollable frame.            ║
╚═════════════════════════════════════════════════════════════════════╝
"""

from __future__ import annotations

import tkinter as tk
from tkinter import ttk
from typing import Any, Callable, Dict, List, Optional

from ..config_io import ConfigIO
from ..models import FieldDef, FieldType
from ..widgets.form_widgets import FormRow, ScrollableForm


class BaseTab(ttk.Frame):
    """A scrollable, schema-driven configuration form tab.

    Each instance receives a list of :class:`~models.FieldDef` objects
    that describe which config keys to display and how.  Sections are
    rendered as visual separators; all other field types become labeled
    input rows.

    Parameters
    ----------
    parent:
        The ``ttk.Notebook`` (or any tk parent) that owns this tab.
    fields:
        Ordered list of ``FieldDef`` objects for this tab.
    on_change:
        Optional callback invoked whenever any field value changes.
        Signature: ``on_change() -> None``.
    """

    def __init__(
        self,
        parent: tk.Widget,
        fields: List[FieldDef],
        on_change: Optional[Callable[[], None]] = None,
    ) -> None:
        super().__init__(parent)
        self._fields = fields
        self._on_change = on_change
        self._rows: List[FormRow] = []

        self._build_ui()

    # ------------------------------------------------------------------
    # UI construction
    # ------------------------------------------------------------------

    def _build_ui(self) -> None:
        self._form = ScrollableForm(self)
        self._form.pack(fill=tk.BOTH, expand=True)

        grid = self._form.inner
        grid.columnconfigure(1, weight=1)

        row_idx = 0
        for fd in self._fields:
            form_row = FormRow(
                grid, fd, row_idx, on_change=self._on_change
            )
            self._rows.append(form_row)
            # SECTION uses 2 real grid rows (separator + label)
            row_idx += 2 if fd.field_type is FieldType.SECTION else 1

        # Bottom padding
        ttk.Label(grid, text="").grid(row=row_idx, column=0, pady=10)

    # ------------------------------------------------------------------
    # Data synchronisation
    # ------------------------------------------------------------------

    def load_from_config(self, data: Dict[str, Any]) -> None:
        """Populate all form fields from the config *data* dict."""
        for row in self._rows:
            if row.is_section:
                continue
            fd = row.field_def
            value = ConfigIO.get(data, fd.key, default=fd.default)
            row.set(value)

    def save_to_config(self, data: Dict[str, Any]) -> None:
        """Write all form field values into the config *data* dict."""
        for row in self._rows:
            if row.is_section:
                continue
            fd = row.field_def
            value = row.get()
            # Only write non-None / non-empty values to keep the config lean
            if value is not None and value != "":
                ConfigIO.set(data, fd.key, value)
            # If the field was cleared (empty string / None) and the key
            # already exists in data, remove it to avoid stale entries.
            elif ConfigIO.get(data, fd.key) is not None:
                ConfigIO.delete(data, fd.key)
