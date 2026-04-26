"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            config_io.py                                       ║
  Module:          tools/themis_config_editor                         ║
  Description:     Service layer: YAML/JSON load/save +               ║
                   nested-key access (dot-path notation)              ║
╚═════════════════════════════════════════════════════════════════════╝
"""

from __future__ import annotations

import json
from pathlib import Path
from typing import Any, Dict, Literal, Optional

import yaml


class ConfigIO:
    """Load, save, and navigate ThemisDB YAML/JSON configuration files.

    All public methods are static/class methods so the class acts as a
    stateless service namespace — no instantiation required.
    """

    # ------------------------------------------------------------------
    # Format detection
    # ------------------------------------------------------------------

    @staticmethod
    def detect_format(path: Path) -> Literal["yaml", "json"]:
        """Return 'yaml' or 'json' based on file extension (fallback: content)."""
        suffix = path.suffix.lower()
        if suffix in {".yaml", ".yml"}:
            return "yaml"
        if suffix == ".json":
            return "json"
        # Peek at first non-whitespace character as fallback
        try:
            with path.open("r", encoding="utf-8") as fh:
                for ch in fh.read(256):
                    if ch.strip():
                        return "json" if ch == "{" else "yaml"
        except OSError:
            pass
        return "yaml"

    # ------------------------------------------------------------------
    # Load / Save
    # ------------------------------------------------------------------

    @classmethod
    def load(cls, path: Path) -> Dict[str, Any]:
        """Load a YAML or JSON config file and return a nested dict."""
        fmt = cls.detect_format(path)
        with path.open("r", encoding="utf-8") as fh:
            raw = fh.read()
        return cls.deserialize(raw, fmt)

    @classmethod
    def save(
        cls,
        path: Path,
        data: Dict[str, Any],
        fmt: Optional[str] = None,
    ) -> None:
        """Serialize *data* and write it to *path*.

        *fmt* defaults to the format detected from the file extension.
        """
        if fmt is None:
            fmt = cls.detect_format(path)
        with path.open("w", encoding="utf-8") as fh:
            fh.write(cls.serialize(data, fmt))

    # ------------------------------------------------------------------
    # Serialize / Deserialize (text ↔ dict)
    # ------------------------------------------------------------------

    @staticmethod
    def serialize(data: Dict[str, Any], fmt: str = "yaml") -> str:
        """Convert *data* dict to a YAML or JSON string."""
        if fmt == "json":
            return json.dumps(data, indent=2, ensure_ascii=False) + "\n"
        return yaml.dump(
            data,
            allow_unicode=True,
            default_flow_style=False,
            sort_keys=False,
        )

    @staticmethod
    def deserialize(text: str, fmt: str = "yaml") -> Dict[str, Any]:
        """Parse a YAML or JSON string and return a nested dict."""
        if not text or not text.strip():
            return {}
        if fmt == "json":
            result = json.loads(text)
        else:
            result = yaml.safe_load(text)
        return result if isinstance(result, dict) else {}

    # ------------------------------------------------------------------
    # Nested key access (dot-path notation)
    # ------------------------------------------------------------------

    @staticmethod
    def get(data: Dict[str, Any], dotpath: str, default: Any = None) -> Any:
        """Return the value at *dotpath* (e.g. 'storage.rocksdb_path').

        Returns *default* when the key chain is absent or traverses a
        non-dict node.
        """
        current: Any = data
        for key in dotpath.split("."):
            if not isinstance(current, dict) or key not in current:
                return default
            current = current[key]
        return current

    @staticmethod
    def set(data: Dict[str, Any], dotpath: str, value: Any) -> None:
        """Write *value* at *dotpath*, creating intermediate dicts as needed."""
        keys = dotpath.split(".")
        current = data
        for key in keys[:-1]:
            if key not in current or not isinstance(current[key], dict):
                current[key] = {}
            current = current[key]
        current[keys[-1]] = value

    @staticmethod
    def delete(data: Dict[str, Any], dotpath: str) -> bool:
        """Remove the key at *dotpath*.  Returns True if the key existed."""
        keys = dotpath.split(".")
        current: Any = data
        for key in keys[:-1]:
            if not isinstance(current, dict) or key not in current:
                return False
            current = current[key]
        if isinstance(current, dict) and keys[-1] in current:
            del current[keys[-1]]
            return True
        return False
