"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            widgets/form_widgets.py                            ║
  Module:          tools/themis_config_editor                         ║
  Description:     Reusable tkinter form-field widgets.               ║
                   Each field type has a matching widget class that    ║
                   exposes get() / set() and packs into a parent grid. ║
╚═════════════════════════════════════════════════════════════════════╝
"""

from __future__ import annotations

import tkinter as tk
from pathlib import Path
from tkinter import filedialog, ttk
from typing import Any, Optional

from ..models import FieldDef, FieldType

# ---------------------------------------------------------------------------
# Tooltip helper
# ---------------------------------------------------------------------------

class _Tooltip:
    """Simple hover-tooltip for any tkinter widget."""

    def __init__(self, widget: tk.Widget, text: str) -> None:
        self._widget = widget
        self._text = text
        self._tip: Optional[tk.Toplevel] = None
        widget.bind("<Enter>", self._show)
        widget.bind("<Leave>", self._hide)

    def _show(self, _event: Any = None) -> None:
        if not self._text or self._tip:
            return
        x = self._widget.winfo_rootx() + 20
        y = self._widget.winfo_rooty() + self._widget.winfo_height() + 4
        self._tip = tk.Toplevel(self._widget)
        self._tip.wm_overrideredirect(True)
        self._tip.wm_geometry(f"+{x}+{y}")
        label = tk.Label(
            self._tip,
            text=self._text,
            justify=tk.LEFT,
            background="#ffffe0",
            relief=tk.SOLID,
            borderwidth=1,
            font=("TkDefaultFont", 9),
            wraplength=380,
        )
        label.pack(ipadx=4, ipady=2)

    def _hide(self, _event: Any = None) -> None:
        if self._tip:
            self._tip.destroy()
            self._tip = None


# ---------------------------------------------------------------------------
# ScrollableForm — canvas-based scrollable container
# ---------------------------------------------------------------------------

class ScrollableForm(ttk.Frame):
    """A vertically scrollable ttk.Frame.

    Place child widgets into ``self.inner`` using grid/pack.
    Bind <Configure> and <MouseWheel> automatically.
    """

    def __init__(self, parent: tk.Widget, **kwargs: Any) -> None:
        super().__init__(parent, **kwargs)

        self._canvas = tk.Canvas(self, borderwidth=0, highlightthickness=0)
        self._scrollbar = ttk.Scrollbar(self, orient="vertical",
                                        command=self._canvas.yview)
        self._canvas.configure(yscrollcommand=self._scrollbar.set)

        self._scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self._canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        self.inner = ttk.Frame(self._canvas)
        self._window_id = self._canvas.create_window(
            (0, 0), window=self.inner, anchor="nw"
        )

        self.inner.bind("<Configure>", self._on_inner_configure)
        self._canvas.bind("<Configure>", self._on_canvas_configure)
        # Bind scroll events only while the pointer is inside this canvas,
        # to avoid interfering with other ScrollableForm instances.
        self._canvas.bind("<Enter>", self._bind_mousewheel)
        self._canvas.bind("<Leave>", self._unbind_mousewheel)

    def _on_inner_configure(self, _event: Any) -> None:
        self._canvas.configure(scrollregion=self._canvas.bbox("all"))

    def _on_canvas_configure(self, event: Any) -> None:
        self._canvas.itemconfig(self._window_id, width=event.width)

    def _bind_mousewheel(self, _event: Any = None) -> None:
        """Activate scroll bindings when the pointer enters this canvas."""
        self._canvas.bind("<MouseWheel>", self._on_mousewheel)
        self._canvas.bind("<Button-4>", self._on_mousewheel)
        self._canvas.bind("<Button-5>", self._on_mousewheel)

    def _unbind_mousewheel(self, _event: Any = None) -> None:
        """Deactivate scroll bindings when the pointer leaves this canvas."""
        self._canvas.unbind("<MouseWheel>")
        self._canvas.unbind("<Button-4>")
        self._canvas.unbind("<Button-5>")

    def _on_mousewheel(self, event: Any) -> None:
        # Cross-platform scroll
        if event.num == 4:
            self._canvas.yview_scroll(-1, "units")
        elif event.num == 5:
            self._canvas.yview_scroll(1, "units")
        else:
            self._canvas.yview_scroll(int(-1 * (event.delta / 120)), "units")


# ---------------------------------------------------------------------------
# FormRow — renders one FieldDef as a label + widget row
# ---------------------------------------------------------------------------

class FormRow:
    """Renders a single config field into a parent grid.

    Parameters
    ----------
    parent:
        Grid-managed parent widget.
    field_def:
        The FieldDef describing this field.
    row:
        Grid row index.
    on_change:
        Optional callback invoked whenever the field value changes.
    """

    _LABEL_COL = 0
    _WIDGET_COL = 1
    _UNIT_COL = 2

    def __init__(
        self,
        parent: tk.Widget,
        field_def: FieldDef,
        row: int,
        on_change: Optional[Any] = None,
    ) -> None:
        self._field_def = field_def
        self._on_change = on_change
        self._var: Optional[tk.Variable] = None
        self._widget: Optional[tk.Widget] = None

        ft = field_def.field_type

        if ft is FieldType.SECTION:
            self._render_section(parent, row)
        else:
            self._render_field(parent, row)

    # ------------------------------------------------------------------
    # Rendering helpers
    # ------------------------------------------------------------------

    def _render_section(self, parent: tk.Widget, row: int) -> None:
        """Render a visual section separator + heading."""
        ttk.Separator(parent, orient="horizontal").grid(
            row=row, column=0, columnspan=3, sticky=tk.EW, pady=(14, 2)
        )
        label = ttk.Label(
            parent,
            text=self._field_def.label,
            font=("TkDefaultFont", 9, "bold"),
            foreground="#336699",
        )
        label.grid(row=row + 1, column=0, columnspan=3, sticky=tk.W, pady=(0, 6))

    def _render_field(self, parent: tk.Widget, row: int) -> None:
        """Render label + input widget + optional unit label."""
        fd = self._field_def
        ft = fd.field_type

        # --- label ---
        lbl = ttk.Label(parent, text=fd.label, anchor=tk.W)
        lbl.grid(row=row, column=self._LABEL_COL, sticky=tk.W,
                 padx=(4, 8), pady=3)
        if fd.tooltip:
            _Tooltip(lbl, fd.tooltip)

        # --- widget ---
        if ft is FieldType.BOOL:
            self._var = tk.BooleanVar(value=bool(fd.default))
            self._widget = ttk.Checkbutton(
                parent, variable=self._var, command=self._notify
            )
            self._widget.grid(row=row, column=self._WIDGET_COL, sticky=tk.W, pady=3)

        elif ft is FieldType.ENUM:
            self._var = tk.StringVar(value=str(fd.default) if fd.default is not None else "")
            self._widget = ttk.Combobox(
                parent,
                textvariable=self._var,
                values=list(fd.choices),
                state="readonly",
                width=30,
            )
            self._widget.grid(row=row, column=self._WIDGET_COL, sticky=tk.W, pady=3)
            self._var.trace_add("write", lambda *_: self._notify())

        elif ft is FieldType.PATH:
            self._var = tk.StringVar(value=str(fd.default) if fd.default is not None else "")
            frame = ttk.Frame(parent)
            frame.grid(row=row, column=self._WIDGET_COL, sticky=tk.EW, pady=3)
            self._widget = ttk.Entry(frame, textvariable=self._var, width=42)
            self._widget.pack(side=tk.LEFT, fill=tk.X, expand=True)
            btn = ttk.Button(
                frame, text="…", width=3,
                command=self._browse_path
            )
            btn.pack(side=tk.LEFT, padx=(4, 0))
            self._var.trace_add("write", lambda *_: self._notify())

        elif ft is FieldType.TEXT:
            self._var = None  # Text widget has its own .get()
            frame = ttk.Frame(parent)
            frame.grid(row=row, column=self._WIDGET_COL, sticky=tk.EW, pady=3)
            self._widget = tk.Text(frame, width=42, height=4, wrap=tk.NONE)
            sb = ttk.Scrollbar(frame, command=self._widget.yview)
            self._widget.configure(yscrollcommand=sb.set)
            self._widget.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
            sb.pack(side=tk.LEFT, fill=tk.Y)
            if fd.default is not None:
                self._widget.insert("1.0", str(fd.default))
            self._widget.bind("<<Modified>>", lambda _: self._notify())

        else:
            # STRING, INT, FLOAT
            self._var = tk.StringVar(
                value=str(fd.default) if fd.default is not None else ""
            )
            self._widget = ttk.Entry(parent, textvariable=self._var, width=34)
            self._widget.grid(row=row, column=self._WIDGET_COL, sticky=tk.EW, pady=3)
            self._var.trace_add("write", lambda *_: self._notify())

        # --- unit label ---
        if fd.unit:
            unit_lbl = ttk.Label(parent, text=fd.unit, foreground="#666666")
            unit_lbl.grid(row=row, column=self._UNIT_COL, sticky=tk.W,
                          padx=(4, 4), pady=3)

        # Tooltip on widget too
        if fd.tooltip and self._widget is not None:
            _Tooltip(self._widget, fd.tooltip)

    def _browse_path(self) -> None:
        assert isinstance(self._var, tk.StringVar)
        current = self._var.get() or "."
        if self._field_def.path_is_dir:
            result = filedialog.askdirectory(initialdir=current)
        else:
            result = filedialog.askopenfilename(initialdir=str(Path(current).parent))
        if result:
            self._var.set(result)

    def _notify(self) -> None:
        if self._on_change:
            self._on_change()

    # ------------------------------------------------------------------
    # Public API
    # ------------------------------------------------------------------

    def get(self) -> Any:
        """Return the current value with type coercion applied."""
        fd = self._field_def
        ft = fd.field_type

        if ft is FieldType.SECTION:
            return None
        if ft is FieldType.BOOL:
            assert isinstance(self._var, tk.BooleanVar)
            return self._var.get()
        if ft is FieldType.TEXT:
            assert isinstance(self._widget, tk.Text)
            return self._widget.get("1.0", tk.END).rstrip("\n")
        assert isinstance(self._var, tk.StringVar)
        raw = self._var.get()
        if ft is FieldType.INT:
            try:
                return int(raw)
            except ValueError:
                return fd.default
        if ft is FieldType.FLOAT:
            try:
                return float(raw)
            except ValueError:
                return fd.default
        return raw  # STRING, ENUM, PATH

    def set(self, value: Any) -> None:
        """Update the widget with *value*."""
        fd = self._field_def
        ft = fd.field_type

        if ft is FieldType.SECTION:
            return
        if ft is FieldType.BOOL:
            assert isinstance(self._var, tk.BooleanVar)
            self._var.set(bool(value))
            return
        if ft is FieldType.TEXT:
            assert isinstance(self._widget, tk.Text)
            self._widget.delete("1.0", tk.END)
            if value is not None:
                self._widget.insert("1.0", str(value))
            return
        assert isinstance(self._var, tk.StringVar)
        self._var.set("" if value is None else str(value))

    @property
    def field_def(self) -> FieldDef:
        return self._field_def

    @property
    def is_section(self) -> bool:
        return self._field_def.field_type is FieldType.SECTION
