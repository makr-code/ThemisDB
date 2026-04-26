"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            models.py                                          ║
  Module:          tools/themis_config_editor                         ║
  Description:     Domain models: FieldType, FieldDef                 ║
╚═════════════════════════════════════════════════════════════════════╝
"""

from __future__ import annotations

from dataclasses import dataclass, field
from enum import Enum, auto
from typing import Any, Sequence


class FieldType(Enum):
    """Widget type for a config field."""

    STRING = auto()   # single-line text entry
    INT = auto()      # integer entry (validated)
    FLOAT = auto()    # float entry (validated)
    BOOL = auto()     # checkbox
    ENUM = auto()     # combobox with fixed choices
    PATH = auto()     # text entry + browse button (file or directory)
    TEXT = auto()     # multi-line text area (e.g. PEM blocks)
    SECTION = auto()  # visual section separator / heading — not a real field


@dataclass
class FieldDef:
    """Descriptor for a single config form field.

    Attributes
    ----------
    key:
        Dot-path into the config dict, e.g. ``"storage.rocksdb_path"``.
        Ignored when ``field_type`` is ``SECTION``.
    label:
        Human-readable label shown next to the widget.
    field_type:
        Determines which widget class is used.
    default:
        Value used when the key is absent from the loaded config.
    choices:
        Allowed values for ``ENUM`` fields.
    tooltip:
        Short help text; shown in a tooltip or label under the widget.
    unit:
        Optional unit string appended after the widget (e.g. ``"MB"``).
    path_is_dir:
        For ``PATH`` fields — ``True`` opens a directory chooser,
        ``False`` opens a file chooser.
    """

    key: str
    label: str
    field_type: FieldType = FieldType.STRING
    default: Any = None
    choices: Sequence[str] = field(default_factory=list)
    tooltip: str = ""
    unit: str = ""
    path_is_dir: bool = False
