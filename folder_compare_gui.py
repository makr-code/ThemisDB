#!/usr/bin/env python3
"""
Folder Compare Tool (Windows GUI)

Features:
- Compare two directories recursively
- Show left-only, right-only, and different files
- Synchronize directories
- Copy only differences to a target folder
- Compress differences into a zip archive

Design goals:
- OOP separation of concerns
- SOLID-friendly structure
- Responsive GUI via threading + queue
"""

from __future__ import annotations

import argparse
import concurrent.futures
import datetime
import filecmp
import importlib.util
import json
import os
import queue
import re
import shutil
import socket
import sqlite3
import subprocess
import sys
import threading
import time
import uuid
import zipfile
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from dataclasses import dataclass
from enum import Enum
from pathlib import Path
from typing import Dict, List, Optional, Set, Tuple
from urllib import error as urlerror
from urllib import parse as urlparse
from urllib import request as urlrequest

import tkinter as tk
from tkinter import filedialog, messagebox, simpledialog, ttk


# -----------------------------------------------------------------------------
# Architecture scaffold (one-file policy)
# -----------------------------------------------------------------------------
# Layer order:
# 1. Core models and events
# 2. Portable compare/sync services
# 3. OS-aware watcher adapters
# 4. Storage and settings
# 5. UI controllers and tab/table rendering
# 6. Scheduler and live refresh coordination
# 7. CLI and bootstrap


class DiffType(str, Enum):
    LEFT_ONLY = "LEFT_ONLY"
    RIGHT_ONLY = "RIGHT_ONLY"
    DIFFERENT = "DIFFERENT"


@dataclass(frozen=True)
class DiffEntry:
    relative_path: Path
    diff_type: DiffType


@dataclass(init=False)
class CompareResult:
    left_root: Path
    right_root: Path
    entries: List[DiffEntry]

    def __init__(
        self,
        left_root: Path,
        right_root: Path,
        entries: Optional[List[DiffEntry]] = None,
        diffs: Optional[List[DiffEntry]] = None,
    ) -> None:
        payload = entries if entries is not None else diffs
        if payload is None:
            payload = []
        self.left_root = left_root
        self.right_root = right_root
        self.entries = list(payload)

    @property
    def diffs(self) -> List[DiffEntry]:
        return self.entries

    @diffs.setter
    def diffs(self, value: List[DiffEntry]) -> None:
        self.entries = list(value)

    def by_type(self, diff_type: DiffType) -> List[DiffEntry]:
        return [e for e in self.entries if e.diff_type == diff_type]

    def summary(self) -> Dict[str, int]:
        return {
            DiffType.LEFT_ONLY.value: len(self.by_type(DiffType.LEFT_ONLY)),
            DiffType.RIGHT_ONLY.value: len(self.by_type(DiffType.RIGHT_ONLY)),
            DiffType.DIFFERENT.value: len(self.by_type(DiffType.DIFFERENT)),
            "TOTAL": len(self.entries),
        }


class MessageType(str, Enum):
    LOG = "LOG"
    RESULT = "RESULT"
    SNAPSHOT = "SNAPSHOT"
    PROGRESS = "PROGRESS"
    BATCH = "BATCH"
    TASK_STATE = "TASK_STATE"
    DONE = "DONE"
    ERROR = "ERROR"


@dataclass
class WorkerMessage:
    message_type: MessageType
    payload: Optional[object] = None


@dataclass
class SnapshotEvent:
    pair_id: str
    result: CompareResult


@dataclass
class ProgressEvent:
    phase: str
    current: int
    total: int
    eta_seconds: Optional[float]
    current_folder: str

    def percent(self) -> float:
        if self.total <= 0:
            return 0.0
        return max(0.0, min(100.0, (self.current / self.total) * 100.0))


@dataclass
class TaskStateEvent:
    pair_id: str
    state: str


@dataclass
class SyncPair:
    pair_id: str
    name: str
    left: str
    right: str
    state: str = "idle"


@dataclass
class SyncJobState:
    """In-memory state for one sync tab/job."""

    pair_id: str
    search_text: str = ""
    filter_value: str = "all"
    selection_count: int = 0
    last_refresh_ns: int = 0


@dataclass
class WorkflowStep:
    """Single orchestration step inside a sync tab workflow."""

    step_id: str
    action: str
    title: str
    enabled: bool = True
    config: Dict[str, str] = None  # type: ignore[assignment]

    def __post_init__(self) -> None:
        if self.config is None:
            self.config = {}

    def to_payload(self) -> Dict[str, object]:
        return {
            "step_id": self.step_id,
            "action": self.action,
            "title": self.title,
            "enabled": self.enabled,
            "config": dict(self.config),
        }

    @staticmethod
    def from_payload(payload: Dict[str, object]) -> "WorkflowStep":
        config = payload.get("config", {})
        return WorkflowStep(
            step_id=str(payload.get("step_id", str(uuid.uuid4()))),
            action=str(payload.get("action", "compare")),
            title=str(payload.get("title", "Schritt")),
            enabled=bool(payload.get("enabled", True)),
            config=config if isinstance(config, dict) else {},
        )


@dataclass
class WatcherEvent:
    """Normalized file-change event emitted by a watcher backend."""

    pair_id: str
    path: str
    event_type: str
    timestamp_ns: int


@dataclass
class WatchScope:
    """Registered observation scope for one sync tab."""

    pair_id: str
    left_path: str
    right_path: str
    enabled: bool = True


@dataclass
class RuntimeDependencies:
    """Container for the single-file application's pluggable backends."""

    storage: "StorageBackend"
    watcher: "WatcherBackend"


class NullWatcherBackend:
    """Portable fallback watcher that does no native observation yet."""

    def __init__(self) -> None:
        self._scopes: Dict[str, WatchScope] = {}

    def start(self) -> None:
        return

    def stop(self) -> None:
        self._scopes.clear()
        return

    def poll(self):
        return []

    def register_scope(self, scope: WatchScope) -> None:
        self._scopes[scope.pair_id] = scope

    def unregister_scope(self, pair_id: str) -> None:
        self._scopes.pop(pair_id, None)


class WindowsWatcherBackend(NullWatcherBackend):
    """Windows-specific watcher placeholder for ReadDirectoryChangesW / USN Journal."""


class LinuxWatcherBackend(NullWatcherBackend):
    """Linux-specific watcher placeholder for inotify / statx based refreshes."""


class MacOSWatcherBackend(NullWatcherBackend):
    """macOS-specific watcher placeholder for FSEvents based refreshes."""


class WatcherFactory:
    """Creates the best available watcher backend for the current platform."""

    @staticmethod
    def create() -> WatcherBackend:
        if sys.platform.startswith("win"):
            return WindowsWatcherBackend()
        if sys.platform.startswith("linux"):
            return LinuxWatcherBackend()
        if sys.platform == "darwin":
            return MacOSWatcherBackend()
        return NullWatcherBackend()


class WatcherBackend:
    """Base interface for platform-specific file change detection."""

    def start(self) -> None:
        raise NotImplementedError

    def stop(self) -> None:
        raise NotImplementedError

    def poll(self) -> List[WatcherEvent]:
        raise NotImplementedError

    def register_scope(self, scope: WatchScope) -> None:
        raise NotImplementedError

    def unregister_scope(self, pair_id: str) -> None:
        raise NotImplementedError


class StorageBackend:
    """Base interface for persisted application state."""

    def load(self) -> Dict[str, str]:
        raise NotImplementedError

    def save(self, settings: Dict[str, str]) -> None:
        raise NotImplementedError

    def reset(self) -> None:
        raise NotImplementedError


class SyncTabControllerBase:
    """Base interface for a single sync tab with table, filter and selection."""

    def refresh(self) -> None:
        raise NotImplementedError

    def apply_search(self, text: str) -> None:
        raise NotImplementedError

    def apply_filter(self, value: str) -> None:
        raise NotImplementedError

    def get_selection(self):
        raise NotImplementedError


class SyncTabController(SyncTabControllerBase):
    """Concrete one-file controller for a single sync tab."""

    def __init__(self, app: "FolderCompareApp", pair_id: str) -> None:
        self.app = app
        self.pair_id = pair_id

    def refresh(self) -> None:
        self.app._refresh_visible_rows(self.pair_id)

    def apply_search(self, text: str) -> None:
        state = self.app._job_state_for_pair(self.pair_id)
        state.search_text = text.strip()
        widget = self.app.sync_pair_widgets.get(self.pair_id)
        if widget is not None:
            search_var = widget.get("search_var")
            if isinstance(search_var, tk.StringVar):
                search_var.set(state.search_text)
        self.app._update_pair_job_state(self.pair_id)
        self.app.after_idle(lambda: self.refresh())

    def apply_filter(self, value: str) -> None:
        state = self.app._job_state_for_pair(self.pair_id)
        state.filter_value = value.strip().lower() or "all"
        widget = self.app.sync_pair_widgets.get(self.pair_id)
        if widget is not None:
            filter_var = widget.get("filter_var")
            if isinstance(filter_var, tk.StringVar):
                filter_var.set(state.filter_value)
        self.app._update_pair_job_state(self.pair_id)
        self.app.after_idle(lambda: self.refresh())

    def get_selection(self):
        widget = self.app.sync_pair_widgets.get(self.pair_id)
        if widget is None:
            return []
        table = widget.get("sync_tree")
        if not isinstance(table, ttk.Treeview):
            return []
        return list(table.selection())


class SchedulerBackend:
    """Base interface for debouncing, live refresh and queue coordination."""

    def start(self) -> None:
        raise NotImplementedError

    def stop(self) -> None:
        raise NotImplementedError

    def request_refresh(self, pair_id: str) -> None:
        raise NotImplementedError


class SchedulerController(SchedulerBackend):
    """Concrete one-file scheduler for refresh, debounce and queue coordination."""

    SNAPSHOT_INTERVAL_SECONDS = 0.75
    SNAPSHOT_MIN_SECONDS = 0.35
    SNAPSHOT_MAX_SECONDS = 1.40
    OPERATION_START_GRACE_SECONDS = 2.0

    def __init__(self, app: "FolderCompareApp") -> None:
        self.app = app
        self._snapshot_refresh_stop = threading.Event()
        self._snapshot_refresh_thread: Optional[threading.Thread] = None

    def start(self) -> None:
        return

    def stop(self) -> None:
        self._stop_live_snapshot_refresh()

    def request_refresh(self, pair_id: str) -> None:
        self.app._refresh_visible_rows(pair_id)

    def start_live_snapshot_refresh(self, pairs: List[SyncPair]) -> None:
        self._stop_live_snapshot_refresh()

        active_pairs = [pair for pair in pairs if pair.left and pair.right]
        if not active_pairs:
            return

        stop_event = threading.Event()
        self._snapshot_refresh_stop = stop_event

        def _adaptive_interval(max_entries: int, pair_count: int) -> float:
            if max_entries <= 800:
                base = 0.40
            elif max_entries <= 3000:
                base = 0.65
            elif max_entries <= 10000:
                base = 0.95
            else:
                base = 1.20

            pair_penalty = max(0, pair_count - 1) * 0.10
            return max(self.SNAPSHOT_MIN_SECONDS, min(self.SNAPSHOT_MAX_SECONDS, base + pair_penalty))

        def refresh_once() -> float:
            max_entries = 0
            for pair in active_pairs:
                if stop_event.is_set() or not self.app.controller.is_operation_running():
                    break
                try:
                    result = self.app.controller.compare_snapshot(pair.left, pair.right)
                except Exception as exc:  # noqa: BLE001
                    self.app.controller.worker_queue.put(
                        WorkerMessage(MessageType.LOG, f"Live-Refresh '{pair.name}' fehlgeschlagen: {exc}")
                    )
                    continue
                if not isinstance(result, CompareResult):
                    self.app.controller.worker_queue.put(
                        WorkerMessage(MessageType.LOG, f"Live-Refresh '{pair.name}' lieferte kein gueltiges CompareResult.")
                    )
                    continue
                max_entries = max(max_entries, len(result.diffs))
                self.app.controller._push_snapshot(SnapshotEvent(pair_id=pair.pair_id, result=result))
            return _adaptive_interval(max_entries=max_entries, pair_count=len(active_pairs))

        def worker() -> None:
            # Wait briefly for operation state to flip to running (avoids startup race).
            start_deadline = time.monotonic() + self.OPERATION_START_GRACE_SECONDS
            while not stop_event.is_set() and not self.app.controller.is_operation_running() and time.monotonic() < start_deadline:
                stop_event.wait(0.05)

            # Emit one snapshot immediately once operation is running.
            interval = refresh_once()
            while not stop_event.wait(interval):
                if not self.app.controller.is_operation_running():
                    break
                interval = refresh_once()

        thread = threading.Thread(target=worker, daemon=True)
        self._snapshot_refresh_thread = thread
        thread.start()

    def _stop_live_snapshot_refresh(self) -> None:
        self._snapshot_refresh_stop.set()
        self._snapshot_refresh_thread = None


class UICompositionBase:
    """Base interface for the main UI composition."""

    def build(self) -> None:
        raise NotImplementedError

    def show(self) -> None:
        raise NotImplementedError


class HoverTooltip:
    """Simple hover tooltip for Tk widgets."""

    def __init__(self, widget, text: str) -> None:
        self.widget = widget
        self.text = text
        self.tip_window = None
        self._after_id = None

        widget.bind("<Enter>", self._on_enter, add="+")
        widget.bind("<Leave>", self._on_leave, add="+")
        widget.bind("<ButtonPress>", self._on_leave, add="+")

    def _on_enter(self, _event=None) -> None:
        self._schedule_show()

    def _on_leave(self, _event=None) -> None:
        self._cancel_show()
        self._hide()

    def _schedule_show(self) -> None:
        self._cancel_show()
        self._after_id = self.widget.after(500, self._show)

    def _cancel_show(self) -> None:
        if self._after_id is not None:
            try:
                self.widget.after_cancel(self._after_id)
            except Exception:
                pass
            self._after_id = None

    def _show(self) -> None:
        if self.tip_window is not None:
            return
        try:
            x = self.widget.winfo_rootx() + 14
            y = self.widget.winfo_rooty() + self.widget.winfo_height() + 8
        except Exception:
            return

        self.tip_window = tw = tk.Toplevel(self.widget)
        tw.wm_overrideredirect(True)
        tw.wm_geometry(f"+{x}+{y}")
        label = tk.Label(
            tw,
            text=self.text,
            justify=tk.LEFT,
            background="#111827",
            foreground="#F9FAFB",
            relief=tk.SOLID,
            borderwidth=1,
            padx=8,
            pady=5,
            font=("Segoe UI", 9),
            wraplength=360,
        )
        label.pack()

    def _hide(self) -> None:
        if self.tip_window is not None:
            self.tip_window.destroy()
            self.tip_window = None


class OperationCancelledError(Exception):
    pass


class OperationControl:
    """Cooperative execution control for pause/resume/cancel."""

    def __init__(self) -> None:
        self._pause_event = threading.Event()
        self._pause_event.set()
        self._cancel_event = threading.Event()

    def pause(self) -> None:
        self._pause_event.clear()

    def resume(self) -> None:
        self._pause_event.set()

    def cancel(self) -> None:
        self._cancel_event.set()
        self._pause_event.set()

    def checkpoint(self) -> None:
        if self._cancel_event.is_set():
            raise OperationCancelledError("Operation wurde abgebrochen.")
        while not self._pause_event.wait(timeout=0.1):
            if self._cancel_event.is_set():
                raise OperationCancelledError("Operation wurde abgebrochen.")

    def is_cancelled(self) -> bool:
        return self._cancel_event.is_set()


@dataclass
class LocationSpec:
    is_remote: bool
    display: str
    endpoint: str = ""
    folder_path: str = ""
    local_path: Optional[Path] = None


class NetworkLocationParser:
    """Parses local folder paths and remote net/http folder targets."""

    @staticmethod
    def parse(value: str) -> LocationSpec:
        raw = value.strip()
        parsed = urlparse.urlparse(raw)
        if parsed.scheme not in {"net", "http", "https"}:
            local = Path(raw)
            return LocationSpec(is_remote=False, display=str(local), local_path=local)

        if not parsed.netloc:
            raise ValueError("Netzwerkadresse ist ungueltig (Host fehlt).")

        endpoint = f"http://{parsed.netloc}" if parsed.scheme == "net" else f"{parsed.scheme}://{parsed.netloc}"
        query = urlparse.parse_qs(parsed.query)
        folder = ""
        if "path" in query and query["path"]:
            folder = query["path"][0]
        else:
            folder = urlparse.unquote(parsed.path.lstrip("/"))

        folder = folder.strip()
        if not folder:
            raise ValueError(
                "Netzwerkpfad muss Ordner enthalten, z.B. net://host:8765/C:/Daten oder http://host:8765?path=C:/Daten"
            )

        return LocationSpec(
            is_remote=True,
            display=f"{endpoint}::{folder}",
            endpoint=endpoint,
            folder_path=folder,
        )


class ManifestService:
    """Builds and compares file manifests for local and remote sync negotiation."""

    @staticmethod
    def build_local_manifest(root: Path, control: Optional[OperationControl] = None) -> Dict[str, Dict[str, int]]:
        if not root.exists() or not root.is_dir():
            raise ValueError(f"Ungueltiger lokaler Ordner: {root}")

        manifest: Dict[str, Dict[str, int]] = {}
        for dirpath, _, filenames in os.walk(root):
            if control is not None:
                control.checkpoint()
            base = Path(dirpath)
            for filename in filenames:
                if control is not None:
                    control.checkpoint()
                full = base / filename
                try:
                    st = full.stat()
                except OSError:
                    continue
                rel = full.relative_to(root).as_posix()
                manifest[rel] = {
                    "size": int(st.st_size),
                    "mtime_ns": int(getattr(st, "st_mtime_ns", int(st.st_mtime * 1_000_000_000))),
                }
        return manifest

    @staticmethod
    def manifest_to_diff_entries(left: Dict[str, Dict[str, int]], right: Dict[str, Dict[str, int]]) -> List[DiffEntry]:
        entries: List[DiffEntry] = []
        left_keys = set(left.keys())
        right_keys = set(right.keys())

        for key in sorted(left_keys - right_keys):
            entries.append(DiffEntry(relative_path=Path(key), diff_type=DiffType.LEFT_ONLY))
        for key in sorted(right_keys - left_keys):
            entries.append(DiffEntry(relative_path=Path(key), diff_type=DiffType.RIGHT_ONLY))
        for key in sorted(left_keys & right_keys):
            l = left[key]
            r = right[key]
            if l.get("size") != r.get("size") or l.get("mtime_ns") != r.get("mtime_ns"):
                entries.append(DiffEntry(relative_path=Path(key), diff_type=DiffType.DIFFERENT))

        return entries


class SyncAgentClient:
    """HTTP client for peer detection and manifest/sync request exchange."""

    def ping(self, endpoint: str, timeout_s: float = 2.0) -> Dict[str, object]:
        return self._request_json("GET", f"{endpoint}/api/ping", None, timeout_s)

    def get_manifest(self, endpoint: str, folder_path: str, timeout_s: float = 15.0) -> Dict[str, Dict[str, int]]:
        payload = {"path": folder_path}
        data = self._request_json("POST", f"{endpoint}/api/manifest", payload, timeout_s)
        manifest = data.get("manifest", {})
        if not isinstance(manifest, dict):
            raise RuntimeError("Ungueltige Manifest-Antwort von Gegenstelle.")
        # Runtime guard: enforce dict[str, dict]
        normalized: Dict[str, Dict[str, int]] = {}
        for key, value in manifest.items():
            if isinstance(key, str) and isinstance(value, dict):
                normalized[key] = {
                    "size": int(value.get("size", 0)),
                    "mtime_ns": int(value.get("mtime_ns", 0)),
                }
        return normalized

    def trigger_sync(self, endpoint: str, request_payload: Dict[str, object], timeout_s: float = 5.0) -> Dict[str, object]:
        return self._request_json("POST", f"{endpoint}/api/sync-request", request_payload, timeout_s)

    def _request_json(self, method: str, url: str, payload: Optional[Dict[str, object]], timeout_s: float) -> Dict[str, object]:
        body = None
        headers = {"Content-Type": "application/json"}
        if payload is not None:
            body = json.dumps(payload, ensure_ascii=True).encode("utf-8")

        req = urlrequest.Request(url=url, data=body, method=method, headers=headers)
        try:
            with urlrequest.urlopen(req, timeout=timeout_s) as resp:
                raw = resp.read().decode("utf-8")
                parsed = json.loads(raw)
                if not isinstance(parsed, dict):
                    raise RuntimeError("Antwort ist kein JSON-Objekt.")
                return parsed
        except urlerror.URLError as exc:
            raise RuntimeError(f"Netzwerkzugriff fehlgeschlagen: {exc}") from exc


class SyncAgentServer:
    """Embedded HTTP server for peer ping, manifest exchange and sync requests."""

    def __init__(self, host: str = "0.0.0.0", port: int = 8765) -> None:
        self.host = host
        self.port = port
        self._httpd: Optional[ThreadingHTTPServer] = None
        self._thread: Optional[threading.Thread] = None
        self._manifest_service = ManifestService()
        self._lock = threading.Lock()

    def start(self) -> None:
        handler = self._build_handler()
        self._httpd = ThreadingHTTPServer((self.host, self.port), handler)
        self._thread = threading.Thread(target=self._httpd.serve_forever, daemon=True)
        self._thread.start()

    def stop(self) -> None:
        if self._httpd is None:
            return
        self._httpd.shutdown()
        self._httpd.server_close()
        self._httpd = None
        self._thread = None

    def _build_handler(self):
        server_ref = self

        class Handler(BaseHTTPRequestHandler):
            def _send_json(self, status_code: int, payload: Dict[str, object]) -> None:
                out = json.dumps(payload, ensure_ascii=True).encode("utf-8")
                self.send_response(status_code)
                self.send_header("Content-Type", "application/json")
                self.send_header("Content-Length", str(len(out)))
                self.end_headers()
                self.wfile.write(out)

            def _read_json(self) -> Dict[str, object]:
                length = int(self.headers.get("Content-Length", "0"))
                raw = self.rfile.read(length) if length > 0 else b"{}"
                data = json.loads(raw.decode("utf-8"))
                if not isinstance(data, dict):
                    raise ValueError("JSON body must be object")
                return data

            def do_GET(self):  # noqa: N802
                if self.path == "/api/ping":
                    self._send_json(200, {"ok": True, "agent": "folder_compare_gui", "host": socket.gethostname()})
                    return
                self._send_json(404, {"ok": False, "error": "not_found"})

            def do_POST(self):  # noqa: N802
                if self.path == "/api/manifest":
                    try:
                        payload = self._read_json()
                        folder = str(payload.get("path", "")).strip()
                        if not folder:
                            self._send_json(400, {"ok": False, "error": "missing_path"})
                            return
                        manifest = server_ref._manifest_service.build_local_manifest(Path(folder))
                        self._send_json(200, {"ok": True, "manifest": manifest})
                        return
                    except Exception as exc:  # noqa: BLE001
                        self._send_json(500, {"ok": False, "error": str(exc)})
                        return

                if self.path == "/api/sync-request":
                    try:
                        payload = self._read_json()
                        with server_ref._lock:
                            server_ref._last_sync_request = payload
                        self._send_json(202, {"ok": True, "accepted": True, "mode": "peer_notify"})
                        return
                    except Exception as exc:  # noqa: BLE001
                        self._send_json(500, {"ok": False, "error": str(exc)})
                        return

                self._send_json(404, {"ok": False, "error": "not_found"})

            def log_message(self, format: str, *args) -> None:  # noqa: A003
                return

        return Handler


class BootstrapSetup:
    """Installs required third-party packages once before the first app start."""

    REQUIRED_PACKAGES: Dict[str, str] = {
        "send2trash": "send2trash>=1.8.3",
    }

    def __init__(self) -> None:
        self.state_file = self._resolve_state_file()

    def run(self) -> None:
        if self._is_setup_done():
            return

        self._print("[setup] Starte Initial-Setup...")
        missing = [name for name in self.REQUIRED_PACKAGES if not self._is_module_available(name)]

        if missing:
            self._print(f"[setup] Fehlende Pakete: {', '.join(missing)}")
            self._install_packages(missing)

        still_missing = [name for name in self.REQUIRED_PACKAGES if not self._is_module_available(name)]
        if still_missing:
            raise RuntimeError(
                "Initial-Setup fehlgeschlagen. Fehlende Pakete: " + ", ".join(still_missing)
            )

        self._write_state()
        self._print("[setup] Initial-Setup abgeschlossen.")

    def _is_setup_done(self) -> bool:
        try:
            if not self.state_file.exists():
                return False
            raw = self.state_file.read_text(encoding="utf-8")
            data = json.loads(raw)
            required = {
                "python": sys.version_info[:3],
                "requirements": self.REQUIRED_PACKAGES,
            }
            return (
                tuple(data.get("python", [])) == required["python"]
                and data.get("requirements", {}) == required["requirements"]
            )
        except Exception:
            return False

    def _install_packages(self, packages: List[str]) -> None:
        specs = [self.REQUIRED_PACKAGES[name] for name in packages]
        cmd = [sys.executable, "-m", "pip", "install", *specs]
        self._print("[setup] Installiere Pakete via pip...")
        subprocess.check_call(cmd)

    def _write_state(self) -> None:
        payload = {
            "python": list(sys.version_info[:3]),
            "requirements": self.REQUIRED_PACKAGES,
        }
        self.state_file.parent.mkdir(parents=True, exist_ok=True)
        self.state_file.write_text(json.dumps(payload, indent=2), encoding="utf-8")

    @staticmethod
    def _is_module_available(module_name: str) -> bool:
        return importlib.util.find_spec(module_name) is not None

    @staticmethod
    def _resolve_state_file() -> Path:
        local_app_data = os.environ.get("LOCALAPPDATA")
        if local_app_data:
            return Path(local_app_data) / "FolderCompareTool" / "setup_state.json"
        return Path.home() / ".folder_compare_tool" / "setup_state.json"

    @staticmethod
    def _print(message: str) -> None:
        print(message)


class AppSettingsStore(StorageBackend):
    """Persists and restores user settings in a JSON file."""

    CURRENT_SCHEMA_VERSION = 1

    def __init__(self) -> None:
        self.settings_file = self._resolve_settings_file()

    def load(self) -> Dict[str, str]:
        if not self.settings_file.exists():
            return {}

        try:
            raw = self.settings_file.read_text(encoding="utf-8")
            data = json.loads(raw)
            if isinstance(data, dict):
                migrated = self._migrate_if_needed(data)
                settings = migrated.get("settings", {})
                if isinstance(settings, dict):
                    return {str(k): str(v) for k, v in settings.items()}

                # Backward compatibility for legacy plain settings dict.
                return {str(k): str(v) for k, v in migrated.items() if k != "schema_version"}
        except Exception:
            return {}
        return {}

    def save(self, settings: Dict[str, str]) -> None:
        self.settings_file.parent.mkdir(parents=True, exist_ok=True)
        payload = {
            "schema_version": self.CURRENT_SCHEMA_VERSION,
            "settings": settings,
        }
        self.settings_file.write_text(
            json.dumps(payload, indent=2, ensure_ascii=True),
            encoding="utf-8",
        )

    def reset(self) -> None:
        if self.settings_file.exists():
            self.settings_file.unlink()

    def _migrate_if_needed(self, data: Dict[str, object]) -> Dict[str, object]:
        schema = data.get("schema_version")
        if isinstance(schema, int) and schema == self.CURRENT_SCHEMA_VERSION:
            return data

        # Legacy format migration: plain key/value object -> versioned structure.
        legacy_settings = {}
        for key, value in data.items():
            if key == "schema_version":
                continue
            legacy_settings[str(key)] = str(value)

        migrated = {
            "schema_version": self.CURRENT_SCHEMA_VERSION,
            "settings": legacy_settings,
        }

        try:
            self.save(legacy_settings)
        except Exception:
            # Migration write failure should not break app startup.
            pass

        return migrated

    @staticmethod
    def _resolve_settings_file() -> Path:
        local_app_data = os.environ.get("LOCALAPPDATA")
        if local_app_data:
            return Path(local_app_data) / "FolderCompareTool" / "app_settings.json"
        return Path.home() / ".folder_compare_tool" / "app_settings.json"


class SQLiteAppStore(StorageBackend):
    """Persists app settings and sync logs in a local SQLite database."""

    def __init__(self) -> None:
        self.db_path = self._resolve_db_file()
        self.db_path.parent.mkdir(parents=True, exist_ok=True)
        self._pair_db_root = self.db_path.parent / "pairs"
        self._pair_db_root.mkdir(parents=True, exist_ok=True)
        self._lock = threading.Lock()
        self._pair_connections: Dict[str, sqlite3.Connection] = {}
        self._fallback_mode = False
        self._conn = self._open_connection(self.db_path)
        try:
            self._init_schema()
            self._migrate_legacy_json_if_needed()
        except Exception:
            self._fallback_mode = True
            try:
                self._conn.close()
            except Exception:
                pass
            self.db_path = Path(":memory:")
            self._conn = sqlite3.connect(":memory:", check_same_thread=False)
            self._configure_connection(self._conn)
            self._init_schema()

    def _init_schema(self) -> None:
        with self._conn:
            self._conn.execute(
                """
                CREATE TABLE IF NOT EXISTS settings (
                    key TEXT PRIMARY KEY,
                    value TEXT NOT NULL
                )
                """
            )
            self._conn.execute(
                """
                CREATE TABLE IF NOT EXISTS app_event_log (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    ts REAL NOT NULL,
                    level TEXT NOT NULL,
                    pair_id TEXT,
                    message TEXT NOT NULL
                )
                """
            )

    def _init_pair_schema(self, conn: sqlite3.Connection) -> None:
        self._configure_connection(conn)
        with conn:
            conn.execute(
                """
                CREATE TABLE IF NOT EXISTS pair_event_log (
                    id INTEGER PRIMARY KEY AUTOINCREMENT,
                    ts REAL NOT NULL,
                    level TEXT NOT NULL,
                    pair_id TEXT,
                    message TEXT NOT NULL
                )
                """
            )

    def _open_connection(self, db_path: Path) -> sqlite3.Connection:
        try:
            conn = sqlite3.connect(str(db_path), check_same_thread=False)
            self._configure_connection(conn)
            return conn
        except sqlite3.OperationalError:
            conn = sqlite3.connect(":memory:", check_same_thread=False)
            self._configure_connection(conn)
            self._fallback_mode = True
            self.db_path = Path(":memory:")
            return conn

    def _migrate_legacy_json_if_needed(self) -> None:
        with self._lock:
            cur = self._conn.execute("SELECT COUNT(*) FROM settings")
            row = cur.fetchone()
            if row and int(row[0]) > 0:
                return

        legacy_file = AppSettingsStore._resolve_settings_file()
        if not legacy_file.exists():
            return

        try:
            raw = legacy_file.read_text(encoding="utf-8")
            parsed = json.loads(raw)
        except Exception:
            return

        migrated: Dict[str, str] = {}
        if isinstance(parsed, dict):
            settings_obj = parsed.get("settings")
            if isinstance(settings_obj, dict):
                migrated = {str(k): str(v) for k, v in settings_obj.items()}
            else:
                for key, value in parsed.items():
                    if key == "schema_version":
                        continue
                    migrated[str(key)] = str(value)

        if migrated:
            self.save(migrated)

    def load(self) -> Dict[str, str]:
        with self._lock:
            cur = self._conn.execute("SELECT key, value FROM settings")
            rows = cur.fetchall()
        return {str(k): str(v) for k, v in rows}

    def save(self, settings: Dict[str, str]) -> None:
        normalized = {str(k): str(v) for k, v in settings.items()}
        with self._lock:
            with self._conn:
                if normalized:
                    keys = list(normalized.keys())
                    placeholders = ",".join(["?"] * len(keys))
                    self._conn.execute(f"DELETE FROM settings WHERE key NOT IN ({placeholders})", keys)
                    self._conn.executemany(
                        "INSERT INTO settings(key, value) VALUES(?, ?) ON CONFLICT(key) DO UPDATE SET value=excluded.value",
                        list(normalized.items()),
                    )
                else:
                    self._conn.execute("DELETE FROM settings")

    def append_log(self, message: str, pair_id: str = "", level: str = "INFO") -> None:
        text = str(message)
        if not text:
            return
        with self._lock:
            with self._conn:
                self._conn.execute(
                    "INSERT INTO app_event_log(ts, level, pair_id, message) VALUES(?, ?, ?, ?)",
                    (time.time(), str(level), str(pair_id or ""), text),
                )

    def load_recent_log_messages(self, limit: int = 1500) -> List[str]:
        safe_limit = max(1, min(int(limit), 10000))
        with self._lock:
            cur = self._conn.execute(
                "SELECT message FROM app_event_log ORDER BY id DESC LIMIT ?",
                (safe_limit,),
            )
            rows = cur.fetchall()
        return [str(row[0]) for row in reversed(rows)]

    def append_pair_log(self, pair: SyncPair, message: str, level: str = "INFO") -> None:
        text = str(message)
        if not text:
            return

        conn = self._pair_connection(pair)
        with self._lock:
            with conn:
                conn.execute(
                    "INSERT INTO pair_event_log(ts, level, pair_id, message) VALUES(?, ?, ?, ?)",
                    (time.time(), str(level), str(pair.pair_id), text),
                )

    def load_recent_pair_log_messages(self, pair: SyncPair, limit: int = 1500) -> List[str]:
        safe_limit = max(1, min(int(limit), 10000))
        conn = self._pair_connection(pair)
        with self._lock:
            cur = conn.execute(
                "SELECT message FROM pair_event_log ORDER BY id DESC LIMIT ?",
                (safe_limit,),
            )
            rows = cur.fetchall()
        return [str(row[0]) for row in reversed(rows)]

    def app_db_exists(self) -> bool:
        return not self._fallback_mode and self.db_path.exists()

    def app_db_health(self) -> str:
        if self._fallback_mode:
            return "healthy (memory fallback)"
        return self._health_check(self._conn)

    def pair_db_exists(self, pair: SyncPair) -> bool:
        if self._fallback_mode:
            return False
        return self._pair_db_path(pair).exists()

    def pair_db_health(self, pair: SyncPair) -> str:
        conn = self._pair_connection(pair)
        if self._fallback_mode:
            return "healthy (memory fallback)"
        return self._health_check(conn)

    def pair_db_path(self, pair: SyncPair) -> Path:
        return self._pair_db_path(pair)

    @staticmethod
    def _health_check(conn: sqlite3.Connection) -> str:
        try:
            with conn:
                cur = conn.execute("PRAGMA quick_check")
                row = cur.fetchone()
        except Exception as exc:
            return f"unhealthy: {exc}"

        if not row:
            return "unhealthy: no response"

        result = str(row[0]).strip().lower()
        if result == "ok":
            return "healthy"
        return f"unhealthy: {row[0]}"

    @staticmethod
    def _configure_connection(conn: sqlite3.Connection) -> None:
        try:
            conn.execute("PRAGMA journal_mode=DELETE")
        except sqlite3.OperationalError:
            pass
        try:
            conn.execute("PRAGMA synchronous=NORMAL")
        except sqlite3.OperationalError:
            pass

    def _pair_connection(self, pair: SyncPair) -> sqlite3.Connection:
        key = str(pair.pair_id)
        with self._lock:
            conn = self._pair_connections.get(key)
            if conn is None:
                if self._fallback_mode:
                    conn = sqlite3.connect(":memory:", check_same_thread=False)
                else:
                    db_path = self._pair_db_path(pair)
                    db_path.parent.mkdir(parents=True, exist_ok=True)
                    conn = sqlite3.connect(str(db_path), check_same_thread=False)
                self._configure_connection(conn)
                self._init_pair_schema(conn)
                self._pair_connections[key] = conn
            return conn

    def _pair_db_path(self, pair: SyncPair) -> Path:
        left_name = self._safe_path_component(Path(pair.left).name or "left")
        right_name = self._safe_path_component(Path(pair.right).name or "right")
        pair_name = self._safe_path_component(pair.name or "pair")
        pair_suffix = self._safe_path_component(pair.pair_id[-8:])
        file_name = f"{left_name}__{right_name}__{pair_name}__{pair_suffix}.db"
        return self._pair_db_root / file_name

    @staticmethod
    def _safe_path_component(value: str) -> str:
        cleaned = [character if character.isalnum() or character in ("-", "_") else "_" for character in str(value).strip()]
        text = "".join(cleaned).strip("._")
        return text[:64] or "pair"

    def reset(self) -> None:
        with self._lock:
            with self._conn:
                self._conn.execute("DELETE FROM settings")
                self._conn.execute("DELETE FROM app_event_log")
            for conn in self._pair_connections.values():
                with conn:
                    conn.execute("DELETE FROM pair_event_log")

    def close(self) -> None:
        with self._lock:
            try:
                for conn in self._pair_connections.values():
                    try:
                        conn.close()
                    except Exception:
                        pass
                self._conn.close()
            except Exception:
                pass

    @staticmethod
    def _resolve_db_file() -> Path:
        local_app_data = os.environ.get("LOCALAPPDATA")
        if local_app_data:
            return Path(local_app_data) / "FolderCompareTool" / "app_state.db"
        return Path.home() / ".folder_compare_tool" / "app_state.db"


class AppStateRepository:
    """Single-file repository for app settings, sync pairs and tab states."""

    def __init__(self, storage: StorageBackend) -> None:
        self.storage = storage

    def load_settings(self) -> Dict[str, str]:
        return self.storage.load()

    def save_settings(self, settings: Dict[str, str]) -> None:
        self.storage.save(settings)

    def reset(self) -> None:
        self.storage.reset()

    def load_job_states(self, settings: Dict[str, str]) -> Dict[str, SyncJobState]:
        raw = settings.get("tab_states", "{}")
        try:
            data = json.loads(raw)
        except Exception:
            return {}

        if not isinstance(data, dict):
            return {}

        states: Dict[str, SyncJobState] = {}
        for pair_id, payload in data.items():
            if not isinstance(pair_id, str) or not isinstance(payload, dict):
                continue
            states[pair_id] = SyncJobState(
                pair_id=pair_id,
                search_text=str(payload.get("search_text", "")),
                filter_value=str(payload.get("filter_value", "all")),
                selection_count=int(payload.get("selection_count", 0) or 0),
                last_refresh_ns=int(payload.get("last_refresh_ns", 0) or 0),
            )
        return states

    def save_job_states(self, settings: Dict[str, str], job_states: Dict[str, SyncJobState]) -> None:
        data = {
            pair_id: {
                "search_text": state.search_text,
                "filter_value": state.filter_value,
                "selection_count": state.selection_count,
                "last_refresh_ns": state.last_refresh_ns,
            }
            for pair_id, state in job_states.items()
        }
        settings["tab_states"] = json.dumps(data, ensure_ascii=True)

    def load_workflow_steps(self, settings: Dict[str, str]) -> Dict[str, List[WorkflowStep]]:
        raw = settings.get("workflow_steps", "{}")
        try:
            data = json.loads(raw)
        except Exception:
            return {}

        if not isinstance(data, dict):
            return {}

        workflows: Dict[str, List[WorkflowStep]] = {}
        for pair_id, payload in data.items():
            if not isinstance(pair_id, str) or not isinstance(payload, list):
                continue
            steps: List[WorkflowStep] = []
            for item in payload:
                if isinstance(item, dict):
                    steps.append(WorkflowStep.from_payload(item))
            if steps:
                workflows[pair_id] = steps
        return workflows

    def save_workflow_steps(self, settings: Dict[str, str], workflow_steps: Dict[str, List[WorkflowStep]]) -> None:
        data = {
            pair_id: [step.to_payload() for step in steps]
            for pair_id, steps in workflow_steps.items()
        }
        settings["workflow_steps"] = json.dumps(data, ensure_ascii=True)

    def load_sync_pairs(self, settings: Dict[str, str]) -> List[SyncPair]:
        raw = settings.get("sync_pairs", "[]")
        try:
            data = json.loads(raw)
        except Exception:
            return []

        pairs: List[SyncPair] = []
        if not isinstance(data, list):
            return pairs

        for item in data:
            if not isinstance(item, dict):
                continue
            pair_id = str(item.get("pair_id", "")).strip() or str(uuid.uuid4())
            name = str(item.get("name", "")).strip()
            left = str(item.get("left", "")).strip()
            right = str(item.get("right", "")).strip()
            state = str(item.get("state", "idle")).strip().lower() or "idle"
            if not name or not left or not right:
                continue
            pairs.append(SyncPair(pair_id=pair_id, name=name, left=left, right=right, state=state))
        return pairs

    def save_sync_pairs(self, settings: Dict[str, str], sync_pairs: List[SyncPair]) -> None:
        data = [{"pair_id": p.pair_id, "name": p.name, "left": p.left, "right": p.right, "state": p.state} for p in sync_pairs]
        settings["sync_pairs"] = json.dumps(data, ensure_ascii=True)


def resolve_trash_delete_handler():
    """Resolve recycle-bin delete implementation, with safe fallback."""
    if importlib.util.find_spec("send2trash") is None:
        return None
    from send2trash import send2trash

    return send2trash


class DirectoryComparator:
    """Compares two directories recursively and returns typed differences."""

    def compare(self, left_root: Path, right_root: Path, progress_callback=None, control: Optional[OperationControl] = None) -> CompareResult:
        entries: List[DiffEntry] = []
        total = max(1, self._count_fs_items(left_root) + self._count_fs_items(right_root))
        state = {
            "processed": 0,
            "last_emit": 0.0,
            "started": time.monotonic(),
        }

        def emit(folder: Path, step: int = 0, force: bool = False) -> None:
            if control is not None:
                control.checkpoint()
            if progress_callback is None:
                return

            state["processed"] = min(total, state["processed"] + max(0, step))
            now = time.monotonic()
            if not force and state["processed"] < total and now - state["last_emit"] < 0.05:
                return

            elapsed = now - state["started"]
            processed = max(0, int(state["processed"]))
            eta = None
            if processed > 0:
                remaining = max(0, total - processed)
                eta = (elapsed / processed) * remaining

            folder_text = str(folder) if str(folder) else "."
            progress_callback(
                ProgressEvent(
                    phase="compare",
                    current=processed,
                    total=total,
                    eta_seconds=eta,
                    current_folder=folder_text,
                )
            )
            state["last_emit"] = now

        self._compare_dirs(left_root, right_root, Path(""), entries, emit, control)
        emit(Path("."), force=True)
        return CompareResult(left_root=left_root, right_root=right_root, diffs=entries)

    @staticmethod
    def _configure_connection(conn: sqlite3.Connection) -> None:
        try:
            conn.execute("PRAGMA journal_mode=DELETE")
        except sqlite3.OperationalError:
            pass
        try:
            conn.execute("PRAGMA synchronous=NORMAL")
        except sqlite3.OperationalError:
            pass

    def _compare_dirs(
        self,
        left_root: Path,
        right_root: Path,
        relative: Path,
        entries: List[DiffEntry],
        emit_progress,
        control: Optional[OperationControl],
    ) -> None:
        emit_progress(relative, force=True)
        if control is not None:
            control.checkpoint()
        left_dir = left_root / relative
        right_dir = right_root / relative

        left_names = set()
        right_names = set()

        if left_dir.exists() and left_dir.is_dir():
            left_names = {p.name for p in left_dir.iterdir()}
        if right_dir.exists() and right_dir.is_dir():
            right_names = {p.name for p in right_dir.iterdir()}

        only_left = sorted(left_names - right_names)
        only_right = sorted(right_names - left_names)
        common = sorted(left_names & right_names)

        for name in only_left:
            rel = relative / name
            left_path = left_root / rel
            if left_path.is_dir():
                self._add_tree(rel, DiffType.LEFT_ONLY, left_root, entries, emit_progress, control)
            else:
                entries.append(DiffEntry(rel, DiffType.LEFT_ONLY))
                emit_progress(relative, step=1)

        for name in only_right:
            rel = relative / name
            right_path = right_root / rel
            if right_path.is_dir():
                self._add_tree(rel, DiffType.RIGHT_ONLY, right_root, entries, emit_progress, control)
            else:
                entries.append(DiffEntry(rel, DiffType.RIGHT_ONLY))
                emit_progress(relative, step=1)

        for name in common:
            rel = relative / name
            left_path = left_root / rel
            right_path = right_root / rel

            if left_path.is_dir() and right_path.is_dir():
                self._compare_dirs(left_root, right_root, rel, entries, emit_progress, control)
                continue

            if left_path.is_file() and right_path.is_file():
                # shallow=False ensures content comparison, not just metadata.
                if not filecmp.cmp(left_path, right_path, shallow=False):
                    entries.append(DiffEntry(rel, DiffType.DIFFERENT))
                emit_progress(relative, step=2)
                continue

            # Type mismatch (file vs dir) is treated as a difference.
            entries.append(DiffEntry(rel, DiffType.DIFFERENT))
            emit_progress(relative, step=2)

    def _add_tree(
        self,
        relative: Path,
        diff_type: DiffType,
        root: Path,
        entries: List[DiffEntry],
        emit_progress,
        control: Optional[OperationControl],
    ) -> None:
        path = root / relative
        if path.is_file():
            entries.append(DiffEntry(relative, diff_type))
            emit_progress(relative.parent, step=1)
            return

        for dirpath, _, filenames in os.walk(path):
            if control is not None:
                control.checkpoint()
            dirpath_obj = Path(dirpath)
            for filename in filenames:
                full_path = dirpath_obj / filename
                entries.append(DiffEntry(full_path.relative_to(root), diff_type))
                emit_progress(full_path.parent.relative_to(root), step=1)

    @staticmethod
    def _count_fs_items(root: Path) -> int:
        if not root.exists() or not root.is_dir():
            return 0
        total = 0
        for _, dirs, files in os.walk(root):
            total += len(dirs) + len(files)
        return total


class FileOperationService:
    """Executes file operations based on compare results."""

    def __init__(self, trash_delete_handler=None) -> None:
        self._trash_delete_handler = trash_delete_handler

    @staticmethod
    def synchronize_plan(result: CompareResult) -> Dict[str, int]:
        return {
            "copied": len(result.by_type(DiffType.LEFT_ONLY)),
            "deleted": len(result.by_type(DiffType.RIGHT_ONLY)),
            "overwritten": len(result.by_type(DiffType.DIFFERENT)),
        }

    def synchronize(self, result: CompareResult, progress_callback=None, control: Optional[OperationControl] = None) -> Dict[str, int]:
        """
        Mirror left -> right for all detected differences.
        - LEFT_ONLY: copy to right
        - RIGHT_ONLY: delete from right
        - DIFFERENT: overwrite right with left
        """
        copied = 0
        deleted = 0
        overwritten = 0
        dirs_created = 0
        dirs_removed = 0

        left_only = {e.relative_path for e in result.by_type(DiffType.LEFT_ONLY)}
        right_only = {e.relative_path for e in result.by_type(DiffType.RIGHT_ONLY)}
        different = {e.relative_path for e in result.by_type(DiffType.DIFFERENT)}

        left_dirs = self._collect_relative_dirs(result.left_root)
        right_dirs = self._collect_relative_dirs(result.right_root)
        dirs_to_create = sorted(left_dirs - right_dirs, key=lambda p: (len(p.parts), str(p)))
        dirs_to_remove = sorted(right_dirs - left_dirs, key=lambda p: (-len(p.parts), str(p)))

        total_ops = max(1, len(dirs_to_create) + len(left_only) + len(right_only) + len(different) + len(dirs_to_remove))
        prog = self._build_progress(progress_callback, phase="sync", total=total_ops)

        for rel_dir in dirs_to_create:
            if control is not None:
                control.checkpoint()
            target_dir = result.right_root / rel_dir
            target_dir.mkdir(parents=True, exist_ok=True)
            dirs_created += 1
            prog(rel_dir)

        for rel in sorted(left_only):
            if control is not None:
                control.checkpoint()
            src = result.left_root / rel
            dst = result.right_root / rel
            self._copy_file(src, dst)
            copied += 1
            prog(rel.parent)

        for rel in sorted(right_only):
            if control is not None:
                control.checkpoint()
            target = result.right_root / rel
            if target.exists() and target.is_file():
                if self._trash_delete_handler is not None:
                    self._trash_delete_handler(str(target))
                else:
                    target.unlink()
                deleted += 1
            prog(rel.parent)

        for rel in sorted(different):
            if control is not None:
                control.checkpoint()
            src = result.left_root / rel
            dst = result.right_root / rel
            if src.exists() and src.is_file():
                self._copy_file(src, dst)
                overwritten += 1
            prog(rel.parent)

        for rel_dir in dirs_to_remove:
            if control is not None:
                control.checkpoint()
            target_dir = result.right_root / rel_dir
            if target_dir.exists() and target_dir.is_dir():
                shutil.rmtree(target_dir, ignore_errors=True)
                dirs_removed += 1
            prog(rel_dir)

        prog(Path("."), force=True)

        return {
            "copied": copied,
            "deleted": deleted,
            "overwritten": overwritten,
            "dirs_created": dirs_created,
            "dirs_removed": dirs_removed,
        }

    def copy_differences(
        self,
        result: CompareResult,
        target_root: Path,
        progress_callback=None,
        control: Optional[OperationControl] = None,
    ) -> Dict[str, int]:
        """
        Copy all differences into target folder grouped by side.
        Structure:
          target/left_only/...
          target/right_only/...
          target/different/left/...
          target/different/right/...
        """
        count = 0
        total_ops = max(1, len(result.diffs) * 2)
        prog = self._build_progress(progress_callback, phase="copy", total=total_ops)

        for entry in result.by_type(DiffType.LEFT_ONLY):
            if control is not None:
                control.checkpoint()
            src = result.left_root / entry.relative_path
            dst = target_root / "left_only" / entry.relative_path
            if src.exists() and src.is_file():
                self._copy_file(src, dst)
                count += 1
            prog(entry.relative_path.parent)

        for entry in result.by_type(DiffType.RIGHT_ONLY):
            if control is not None:
                control.checkpoint()
            src = result.right_root / entry.relative_path
            dst = target_root / "right_only" / entry.relative_path
            if src.exists() and src.is_file():
                self._copy_file(src, dst)
                count += 1
            prog(entry.relative_path.parent)

        for entry in result.by_type(DiffType.DIFFERENT):
            if control is not None:
                control.checkpoint()
            left_src = result.left_root / entry.relative_path
            right_src = result.right_root / entry.relative_path

            if left_src.exists() and left_src.is_file():
                self._copy_file(left_src, target_root / "different" / "left" / entry.relative_path)
                count += 1
            prog(entry.relative_path.parent)

            if right_src.exists() and right_src.is_file():
                self._copy_file(right_src, target_root / "different" / "right" / entry.relative_path)
                count += 1
            prog(entry.relative_path.parent)

        prog(Path("."), force=True)

        return {"copied": count}

    def compress_differences(
        self,
        result: CompareResult,
        zip_path: Path,
        progress_callback=None,
        control: Optional[OperationControl] = None,
    ) -> Dict[str, int]:
        """Write all differing files to a zip file with side prefixes."""
        zipped = 0
        zip_path.parent.mkdir(parents=True, exist_ok=True)
        total_ops = max(1, len(result.diffs) * 2)
        prog = self._build_progress(progress_callback, phase="zip", total=total_ops)

        with zipfile.ZipFile(zip_path, mode="w", compression=zipfile.ZIP_DEFLATED) as zf:
            for entry in result.by_type(DiffType.LEFT_ONLY):
                if control is not None:
                    control.checkpoint()
                src = result.left_root / entry.relative_path
                if src.exists() and src.is_file():
                    zf.write(src, arcname=str(Path("left_only") / entry.relative_path))
                    zipped += 1
                prog(entry.relative_path.parent)

            for entry in result.by_type(DiffType.RIGHT_ONLY):
                if control is not None:
                    control.checkpoint()
                src = result.right_root / entry.relative_path
                if src.exists() and src.is_file():
                    zf.write(src, arcname=str(Path("right_only") / entry.relative_path))
                    zipped += 1
                prog(entry.relative_path.parent)

            for entry in result.by_type(DiffType.DIFFERENT):
                if control is not None:
                    control.checkpoint()
                left_src = result.left_root / entry.relative_path
                right_src = result.right_root / entry.relative_path

                if left_src.exists() and left_src.is_file():
                    zf.write(left_src, arcname=str(Path("different/left") / entry.relative_path))
                    zipped += 1
                prog(entry.relative_path.parent)

                if right_src.exists() and right_src.is_file():
                    zf.write(right_src, arcname=str(Path("different/right") / entry.relative_path))
                    zipped += 1
                prog(entry.relative_path.parent)

            prog(Path("."), force=True)

        return {"zipped": zipped}

    @staticmethod
    def _copy_file(src: Path, dst: Path) -> None:
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dst)

    @staticmethod
    def _collect_relative_dirs(root: Path) -> set[Path]:
        collected: set[Path] = {Path("")}
        for dirpath, _, _ in os.walk(root):
            rel = Path(dirpath).relative_to(root)
            collected.add(rel)
        return collected

    @staticmethod
    def _build_progress(progress_callback, phase: str, total: int):
        state = {
            "current": 0,
            "last_emit": 0.0,
            "started": time.monotonic(),
        }

        def emit(folder: Path, force: bool = False) -> None:
            if progress_callback is None:
                return

            state["current"] = min(total, state["current"] + 1)
            now = time.monotonic()
            if not force and state["current"] < total and now - state["last_emit"] < 0.05:
                return

            elapsed = now - state["started"]
            current = max(0, int(state["current"]))
            eta = None
            if current > 0:
                eta = (elapsed / current) * max(0, total - current)

            folder_text = str(folder) if str(folder) else "."
            progress_callback(
                ProgressEvent(
                    phase=phase,
                    current=current,
                    total=total,
                    eta_seconds=eta,
                    current_folder=folder_text,
                )
            )
            state["last_emit"] = now

        return emit


class AppController:
    """Coordinates UI actions and background work."""

    UI_BATCH_FLUSH_SECONDS = 0.12

    def __init__(self, ui: "FolderCompareApp") -> None:
        self.ui = ui
        self.comparator = DirectoryComparator()
        self.operations = FileOperationService(trash_delete_handler=resolve_trash_delete_handler())
        self.location_parser = NetworkLocationParser()
        self.manifest_service = ManifestService()
        self.agent_client = SyncAgentClient()
        self.worker_queue: "queue.Queue[WorkerMessage]" = queue.Queue()
        self.current_result: Optional[CompareResult] = None
        self.current_left_location: Optional[LocationSpec] = None
        self.current_right_location: Optional[LocationSpec] = None
        self._worker_lock = threading.Lock()
        self._worker_running = False
        self._active_control: Optional[OperationControl] = None
        self._ui_batch_lock = threading.Lock()
        self._ui_batch_progress: Optional[ProgressEvent] = None
        self._ui_batch_snapshots: Dict[str, SnapshotEvent] = {}
        self._ui_batch_last_flush = 0.0

    def compare(self, left: str, right: str) -> None:
        left_loc = self.location_parser.parse(left)
        right_loc = self.location_parser.parse(right)
        self.current_left_location = left_loc
        self.current_right_location = right_loc

        def job() -> None:
            self._push_log("Vergleich gestartet...")
            control = self._require_control()
            result = self._compare_locations(left_loc, right_loc, control)
            if not isinstance(result, CompareResult):
                raise RuntimeError("Vergleich lieferte kein gueltiges Ergebnisobjekt.")
            self.worker_queue.put(WorkerMessage(MessageType.RESULT, result))
            self._push_log(f"Vergleich abgeschlossen. Eintraege: {len(result.diffs)}")

        self._run_in_background(job)

    def compare_snapshot(self, left: str, right: str) -> CompareResult:
        left_loc = self.location_parser.parse(left)
        right_loc = self.location_parser.parse(right)
        result = self._compare_locations(left_loc, right_loc, OperationControl())
        if not isinstance(result, CompareResult):
            raise RuntimeError("Snapshot-Vergleich lieferte kein gueltiges Ergebnisobjekt.")
        return result

    def synchronize(self) -> None:
        result = self._require_result()

        def job() -> None:
            control = self._require_control()
            self._push_log("Synchronisierung gestartet (left -> right)...")

            left_loc = self.current_left_location
            right_loc = self.current_right_location
            if left_loc is None or right_loc is None:
                left_loc = self.location_parser.parse(str(result.left_root))
                right_loc = self.location_parser.parse(str(result.right_root))
            if left_loc.is_remote or right_loc.is_remote:
                self._sync_network_request(left_loc, right_loc, result)
                return

            stats = self.operations.synchronize(
                result,
                progress_callback=self._push_progress,
                control=control,
            )
            self._push_log(
                f"Synchronisierung fertig. Kopiert: {stats['copied']}, "
                f"Geloescht: {stats['deleted']}, Ueberschrieben: {stats['overwritten']}, "
                f"Dirs erstellt: {stats['dirs_created']}, Dirs entfernt: {stats['dirs_removed']}"
            )

        self._run_in_background(job)

    def _compare_locations(self, left_loc: LocationSpec, right_loc: LocationSpec, control: OperationControl) -> CompareResult:
        if not left_loc.is_remote and not right_loc.is_remote:
            left_path = left_loc.local_path
            right_path = right_loc.local_path
            if left_path is None or right_path is None:
                raise ValueError("Lokale Pfade konnten nicht aufgeloest werden.")
            self._validate_dirs(left_path, right_path)
            return self.comparator.compare(
                left_path,
                right_path,
                progress_callback=self._push_progress,
                control=control,
            )

        self._push_log("Netzwerkpfad erkannt: pruefe Gegenstelle...")
        left_manifest = self._fetch_manifest_for_location(left_loc, control)
        right_manifest = self._fetch_manifest_for_location(right_loc, control)

        entries = self.manifest_service.manifest_to_diff_entries(left_manifest, right_manifest)
        left_root = Path(left_loc.display)
        right_root = Path(right_loc.display)
        return CompareResult(left_root=left_root, right_root=right_root, diffs=entries)

    def _fetch_manifest_for_location(self, loc: LocationSpec, control: OperationControl) -> Dict[str, Dict[str, int]]:
        control.checkpoint()
        if not loc.is_remote:
            if loc.local_path is None:
                raise ValueError("Lokaler Pfad fehlt.")
            self._push_log(f"Lokales Manifest: {loc.local_path}")
            return self.manifest_service.build_local_manifest(loc.local_path, control=control)

        self._push_log(f"Peer-Check: {loc.endpoint}")
        ping = self.agent_client.ping(loc.endpoint)
        if not bool(ping.get("ok")):
            raise RuntimeError(f"Gegenstelle antwortet nicht korrekt: {loc.endpoint}")
        self._push_log(f"Remote Manifest: {loc.endpoint} -> {loc.folder_path}")
        return self.agent_client.get_manifest(loc.endpoint, loc.folder_path)

    def _sync_network_request(self, left_loc: LocationSpec, right_loc: LocationSpec, result: CompareResult) -> None:
        if right_loc.is_remote and not left_loc.is_remote:
            payload = {
                "action": "sync_request",
                "source": str(left_loc.local_path) if left_loc.local_path else left_loc.display,
                "target": right_loc.folder_path,
                "summary": result.summary(),
            }
            resp = self.agent_client.trigger_sync(right_loc.endpoint, payload)
            self._push_log(f"Sync-Request an {right_loc.endpoint} gesendet: accepted={resp.get('accepted')}")
            return

        if left_loc.is_remote and not right_loc.is_remote:
            payload = {
                "action": "sync_request",
                "source": left_loc.folder_path,
                "target": str(right_loc.local_path) if right_loc.local_path else right_loc.display,
                "summary": result.summary(),
            }
            resp = self.agent_client.trigger_sync(left_loc.endpoint, payload)
            self._push_log(f"Sync-Request an {left_loc.endpoint} gesendet: accepted={resp.get('accepted')}")
            return

        if left_loc.is_remote and right_loc.is_remote:
            self._push_log("Remote-zu-Remote: nur Change-Exchange verifiziert, kein direkter Datentransfer lokal ausgefuehrt.")
            return

    def copy_differences(self, target: str) -> None:
        result = self._require_result()
        target_path = Path(target)

        def job() -> None:
            self._push_log(f"Kopiere Differenzen nach: {target_path}")
            stats = self.operations.copy_differences(
                result,
                target_path,
                progress_callback=self._push_progress,
                control=self._require_control(),
            )
            self._push_log(f"Kopieren abgeschlossen. Dateien: {stats['copied']}")

        self._run_in_background(job)

    def compress_differences(self, zip_file: str) -> None:
        result = self._require_result()
        zip_path = Path(zip_file)

        def job() -> None:
            self._push_log(f"Komprimiere Differenzen nach: {zip_path}")
            stats = self.operations.compress_differences(
                result,
                zip_path,
                progress_callback=self._push_progress,
                control=self._require_control(),
            )
            self._push_log(f"Komprimierung abgeschlossen. Dateien: {stats['zipped']}")

        self._run_in_background(job)

    def synchronize_pairs(self, pairs: List[SyncPair], mode: str, max_parallel: int) -> None:
        if not pairs:
            raise ValueError("Keine Sync-Paare vorhanden.")
        clean_mode = mode.lower().strip()
        if clean_mode not in {"sequential", "parallel"}:
            raise ValueError("Ungueltiger Modus. Erlaubt: sequential|parallel")
        if max_parallel < 1:
            raise ValueError("max_parallel muss >= 1 sein.")

        normalized = [self._validate_sync_pair(pair) for pair in pairs]

        def job() -> None:
            control = self._require_control()
            self._push_log(f"Starte Multi-Sync ({clean_mode}) mit {len(normalized)} Paaren")
            if clean_mode == "sequential":
                for idx, pair in enumerate(normalized, start=1):
                    control.checkpoint()
                    self._run_pair_sync(pair, idx, len(normalized), control)
                self._push_log("Multi-Sync (sequential) abgeschlossen.")
                return

            max_workers = min(max_parallel, len(normalized))
            with concurrent.futures.ThreadPoolExecutor(max_workers=max_workers) as executor:
                futures = [
                    executor.submit(self._run_pair_sync, pair, idx, len(normalized), control)
                    for idx, pair in enumerate(normalized, start=1)
                ]
                for future in concurrent.futures.as_completed(futures):
                    control.checkpoint()
                    future.result()
            self._push_log("Multi-Sync (parallel) abgeschlossen.")

        self._run_in_background(job)

    @staticmethod
    def _parse_bool_flag(value: str) -> bool:
        return str(value).strip().lower() in {"1", "true", "yes", "ja", "on"}

    @staticmethod
    def _parse_csv_tokens(value: str) -> List[str]:
        return [item.strip().lower() for item in str(value).split(",") if item.strip()]

    @staticmethod
    def _parse_iso_date(value: str) -> Optional[datetime.datetime]:
        clean = str(value).strip()
        if not clean:
            return None
        try:
            return datetime.datetime.fromisoformat(clean)
        except ValueError as exc:
            raise ValueError(f"Ungueltiges Datumsformat '{value}'. Erwartet: YYYY-MM-DD oder YYYY-MM-DDTHH:MM:SS") from exc

    @staticmethod
    def _entry_source_path(result: CompareResult, entry: DiffEntry) -> Optional[Path]:
        rel = entry.relative_path
        if entry.diff_type == DiffType.LEFT_ONLY:
            p = result.left_root / rel
            return p if p.exists() else None
        if entry.diff_type == DiffType.RIGHT_ONLY:
            p = result.right_root / rel
            return p if p.exists() else None
        left_candidate = result.left_root / rel
        if left_candidate.exists():
            return left_candidate
        right_candidate = result.right_root / rel
        return right_candidate if right_candidate.exists() else None

    def _apply_filter_workflow_step(self, result: CompareResult, step: WorkflowStep, dry_run: bool = False) -> CompareResult:
        cfg = step.config or {}

        min_size = str(cfg.get("min_size_bytes", "")).strip()
        max_size = str(cfg.get("max_size_bytes", "")).strip()
        date_from = self._parse_iso_date(str(cfg.get("date_from", "")).strip())
        date_to = self._parse_iso_date(str(cfg.get("date_to", "")).strip())
        unzip_enabled = self._parse_bool_flag(str(cfg.get("unzip_zips", "false")))
        index_enabled = self._parse_bool_flag(str(cfg.get("index_entries", "false")))
        include_extensions = self._parse_csv_tokens(str(cfg.get("include_extensions", "")))
        exclude_extensions = self._parse_csv_tokens(str(cfg.get("exclude_extensions", "")))
        exclude_folders = self._parse_csv_tokens(str(cfg.get("exclude_folders", "")))
        include_regex_text = str(cfg.get("include_regex", "")).strip()
        exclude_regex_text = str(cfg.get("exclude_regex", "")).strip()
        include_regex_icase = self._parse_bool_flag(str(cfg.get("include_regex_icase", "false")))
        exclude_regex_icase = self._parse_bool_flag(str(cfg.get("exclude_regex_icase", "false")))

        include_regex = None
        exclude_regex = None
        if include_regex_text:
            try:
                include_regex = re.compile(include_regex_text, re.IGNORECASE if include_regex_icase else 0)
            except re.error as exc:
                raise ValueError(f"Ungueltiger Include-Regex: {exc}") from exc
        if exclude_regex_text:
            try:
                exclude_regex = re.compile(exclude_regex_text, re.IGNORECASE if exclude_regex_icase else 0)
            except re.error as exc:
                raise ValueError(f"Ungueltiger Exclude-Regex: {exc}") from exc

        include_extensions = [f".{ext[1:] if ext.startswith('.') else ext}" for ext in include_extensions]
        exclude_extensions = [f".{ext[1:] if ext.startswith('.') else ext}" for ext in exclude_extensions]

        min_size_value = int(min_size) if min_size else None
        max_size_value = int(max_size) if max_size else None

        if min_size_value is not None and min_size_value < 0:
            raise ValueError("min_size_bytes muss >= 0 sein.")
        if max_size_value is not None and max_size_value < 0:
            raise ValueError("max_size_bytes muss >= 0 sein.")
        if min_size_value is not None and max_size_value is not None and min_size_value > max_size_value:
            raise ValueError("min_size_bytes darf nicht groesser als max_size_bytes sein.")

        kept_entries: List[DiffEntry] = []
        extracted_count = 0
        index_payload: List[Dict[str, object]] = []

        unzip_target_text = str(cfg.get("unzip_target", "")).strip()
        unzip_target = Path(unzip_target_text) if unzip_target_text else None

        for entry in result.diffs:
            src = self._entry_source_path(result, entry)
            if src is None:
                continue
            if not src.exists() or not src.is_file():
                continue

            rel_folder_parts = [part.lower() for part in entry.relative_path.parts[:-1]]
            if exclude_folders and any(folder in rel_folder_parts for folder in exclude_folders):
                continue

            suffix = src.suffix.lower()
            if include_extensions and suffix not in include_extensions:
                continue
            if exclude_extensions and suffix in exclude_extensions:
                continue

            rel_text = entry.relative_path.as_posix()
            if include_regex is not None and include_regex.search(rel_text) is None:
                continue
            if exclude_regex is not None and exclude_regex.search(rel_text) is not None:
                continue

            stat = src.stat()
            size_ok = True
            if min_size_value is not None and stat.st_size < min_size_value:
                size_ok = False
            if max_size_value is not None and stat.st_size > max_size_value:
                size_ok = False
            if not size_ok:
                continue

            modified_at = datetime.datetime.fromtimestamp(stat.st_mtime)
            if date_from is not None and modified_at < date_from:
                continue
            if date_to is not None and modified_at > date_to:
                continue

            kept_entries.append(entry)

            if unzip_enabled and src.suffix.lower() == ".zip" and unzip_target is not None:
                if dry_run:
                    extracted_count += 1
                else:
                    unzip_target.mkdir(parents=True, exist_ok=True)
                    extract_dir = unzip_target / src.stem
                    extract_dir.mkdir(parents=True, exist_ok=True)
                    try:
                        with zipfile.ZipFile(src, "r") as archive:
                            archive.extractall(extract_dir)
                        extracted_count += 1
                    except Exception as exc:
                        self._push_log(f"ZIP-Entpacken uebersprungen ({src.name}): {exc}")

            if index_enabled:
                index_payload.append(
                    {
                        "relative_path": entry.relative_path.as_posix(),
                        "diff_type": entry.diff_type.value,
                        "source": str(src),
                        "size_bytes": stat.st_size,
                        "modified": modified_at.isoformat(timespec="seconds"),
                    }
                )

        if index_enabled:
            index_file_text = str(cfg.get("index_file", "")).strip()
            if index_file_text and not dry_run:
                index_file = Path(index_file_text)
                index_file.parent.mkdir(parents=True, exist_ok=True)
                index_file.write_text(json.dumps(index_payload, ensure_ascii=False, indent=2), encoding="utf-8")
                self._push_log(f"Filter-Index geschrieben: {index_file}")
            elif index_file_text and dry_run:
                self._push_log(f"Dry-run: Filter-Index wuerde geschrieben: {index_file_text}")
            else:
                self._push_log("Filter-Index aktiv, aber keine index_file gesetzt. Index wurde nicht gespeichert.")

        filtered_result = CompareResult(
            left_root=result.left_root,
            right_root=result.right_root,
            diffs=kept_entries,
        )
        self._push_log(
            f"{'Dry-run: ' if dry_run else ''}Filter-Card angewendet: {len(result.diffs)} -> {len(filtered_result.diffs)} Eintraege"
            + (f", ZIP entpackt: {extracted_count}" if unzip_enabled else "")
            + (f", Ext+={','.join(include_extensions)}" if include_extensions else "")
            + (f", Ext-={','.join(exclude_extensions)}" if exclude_extensions else "")
            + (f", Ordner-={','.join(exclude_folders)}" if exclude_folders else "")
            + (f", Re+={include_regex_text}" if include_regex_text else "")
            + (f", Re-={exclude_regex_text}" if exclude_regex_text else "")
            + (", Re+(i)" if include_regex_text and include_regex_icase else "")
            + (", Re-(i)" if exclude_regex_text and exclude_regex_icase else "")
        )
        return filtered_result

    def run_workflow(self, pair: SyncPair, steps: List[WorkflowStep]) -> None:
        if not steps:
            raise ValueError("Workflow hat keine Schritte.")

        def job() -> None:
            control = self._require_control()
            left_path: Optional[Path] = Path(pair.left) if str(pair.left).strip() else None
            right_path: Optional[Path] = Path(pair.right) if str(pair.right).strip() else None
            self.current_left_location = self.location_parser.parse(str(left_path)) if left_path is not None else None
            self.current_right_location = self.location_parser.parse(str(right_path)) if right_path is not None else None
            self._push_task_state(pair.pair_id, "running")
            self._push_log(f"Workflow fuer '{pair.name}' gestartet ({len(steps)} Schritte)")

            current_result: Optional[CompareResult] = None
            try:
                for index, step in enumerate(steps, start=1):
                    control.checkpoint()
                    if not step.enabled:
                        self._push_log(f"[{index}/{len(steps)}] {step.title} uebersprungen (deaktiviert)")
                        continue

                    action = str(step.action).strip().lower()
                    dry_run_step = self._parse_bool_flag(str(step.config.get("dry_run", "false")))
                    self._push_log(f"[{index}/{len(steps)}] {step.title} -> {action}")

                    if action == "start_meta":
                        run_label = str(step.config.get("run_label", "")).strip()
                        run_note = str(step.config.get("run_note", "")).strip()
                        if run_label:
                            self._push_log(f"Start-Meta: Label='{run_label}'")
                        if run_note:
                            self._push_log(f"Start-Meta: Notiz='{run_note}'")
                        continue

                    if action == "select_paths":
                        folder_a = str(step.config.get("folder_a", "")).strip()
                        folder_b = str(step.config.get("folder_b", "")).strip()
                        if not folder_a or not folder_b:
                            raise ValueError("Step 'select_paths' braucht Ordner A und B.")
                        left_path = Path(folder_a)
                        right_path = Path(folder_b)
                        if not left_path.exists() or not left_path.is_dir():
                            raise ValueError(f"Ordner A nicht gefunden: {left_path}")
                        if not right_path.exists() or not right_path.is_dir():
                            raise ValueError(f"Ordner B nicht gefunden: {right_path}")

                        self.current_left_location = self.location_parser.parse(str(left_path))
                        self.current_right_location = self.location_parser.parse(str(right_path))
                        self._push_log(f"Ordner gesetzt: A={left_path} | B={right_path}")
                        continue

                    if action == "compare":
                        if left_path is None or right_path is None:
                            raise ValueError("Step 'compare' braucht gueltige Ordner A/B. Bitte zuerst 'Ordner A/B' konfigurieren.")
                        current_result = self.comparator.compare(
                            left_path,
                            right_path,
                            progress_callback=self._push_progress,
                            control=control,
                        )
                        if not isinstance(current_result, CompareResult):
                            raise RuntimeError("Workflow-Check lieferte kein gueltiges Vergleichsergebnis.")
                        self.current_result = current_result
                        self.worker_queue.put(WorkerMessage(MessageType.RESULT, current_result))
                        self._push_log(f"Vergleich fertig: {len(current_result.diffs)} Unterschiede")
                        continue

                    if action == "sync_left":
                        if left_path is None or right_path is None:
                            raise ValueError("Step 'sync_left' braucht gueltige Ordner A/B. Bitte zuerst 'Ordner A/B' konfigurieren.")
                        if current_result is None:
                            current_result = self.comparator.compare(
                                left_path,
                                right_path,
                                progress_callback=self._push_progress,
                                control=control,
                            )
                            self.current_result = current_result
                            self.worker_queue.put(WorkerMessage(MessageType.RESULT, current_result))
                        if dry_run_step:
                            stats = self.operations.synchronize_plan(current_result)
                            self._push_log(
                                f"Dry-run Sync: Kopiert={stats['copied']}, Geloescht={stats['deleted']}, Ueberschrieben={stats['overwritten']}"
                            )
                            continue
                        stats = self.operations.synchronize(
                            current_result,
                            progress_callback=self._push_progress,
                            control=control,
                        )
                        self._push_log(
                            f"Sync fertig: Kopiert={stats['copied']}, Geloescht={stats['deleted']}, Ueberschrieben={stats['overwritten']}"
                        )
                        continue

                    if action == "copy_diff":
                        if left_path is None or right_path is None:
                            raise ValueError("Step 'copy_diff' braucht gueltige Ordner A/B. Bitte zuerst 'Ordner A/B' konfigurieren.")
                        if current_result is None:
                            current_result = self.comparator.compare(
                                left_path,
                                right_path,
                                progress_callback=self._push_progress,
                                control=control,
                            )
                            self.current_result = current_result
                            self.worker_queue.put(WorkerMessage(MessageType.RESULT, current_result))
                        target_text = str(step.config.get("target", "")).strip()
                        if not target_text:
                            raise ValueError("Step 'copy_diff' braucht ein Zielverzeichnis.")
                        if dry_run_step:
                            self._push_log(f"Dry-run Copy: {len(current_result.diffs)} Eintraege wuerden nach '{target_text}' kopiert")
                            continue
                        stats = self.operations.copy_differences(
                            current_result,
                            Path(target_text),
                            progress_callback=self._push_progress,
                            control=control,
                        )
                        self._push_log(f"Differenzen kopiert: {stats['copied']}")
                        continue

                    if action == "zip_diff":
                        if left_path is None or right_path is None:
                            raise ValueError("Step 'zip_diff' braucht gueltige Ordner A/B. Bitte zuerst 'Ordner A/B' konfigurieren.")
                        if current_result is None:
                            current_result = self.comparator.compare(
                                left_path,
                                right_path,
                                progress_callback=self._push_progress,
                                control=control,
                            )
                            self.current_result = current_result
                            self.worker_queue.put(WorkerMessage(MessageType.RESULT, current_result))
                        zip_text = str(step.config.get("zip_file", "")).strip()
                        if not zip_text:
                            raise ValueError("Step 'zip_diff' braucht eine Zip-Datei.")
                        if dry_run_step:
                            self._push_log(f"Dry-run ZIP: {len(current_result.diffs)} Eintraege wuerden nach '{zip_text}' gepackt")
                            continue
                        stats = self.operations.compress_differences(
                            current_result,
                            Path(zip_text),
                            progress_callback=self._push_progress,
                            control=control,
                        )
                        self._push_log(f"Differenzen gepackt: {stats['zipped']}")
                        continue

                    if action == "filter_ops":
                        if left_path is None or right_path is None:
                            raise ValueError("Step 'filter_ops' braucht gueltige Ordner A/B. Bitte zuerst 'Ordner A/B' konfigurieren.")
                        if current_result is None:
                            current_result = self.comparator.compare(
                                left_path,
                                right_path,
                                progress_callback=self._push_progress,
                                control=control,
                            )
                            self.current_result = current_result
                            self.worker_queue.put(WorkerMessage(MessageType.RESULT, current_result))
                        current_result = self._apply_filter_workflow_step(current_result, step, dry_run=dry_run_step)
                        self.current_result = current_result
                        self.worker_queue.put(WorkerMessage(MessageType.RESULT, current_result))
                        continue

                    if action == "finish":
                        self._push_log("Workflow-Schritt 'Finish' erreicht.")
                        break

                    raise ValueError(f"Unbekannte Workflow-Aktion: {step.action}")

                self._push_log(f"Workflow fuer '{pair.name}' abgeschlossen.")
                self._push_task_state(pair.pair_id, "done")
            except Exception:
                self._push_task_state(pair.pair_id, "error")
                raise

        self._run_in_background(job)

    def pause_active_operation(self) -> None:
        control = self._require_control()
        control.pause()
        self._push_log("Operation unterbrochen.")

    def resume_active_operation(self) -> None:
        control = self._require_control()
        control.resume()
        self._push_log("Operation fortgesetzt.")

    def cancel_active_operation(self) -> None:
        control = self._require_control()
        control.cancel()
        self._push_log("Abbruch angefordert...")

    def is_operation_running(self) -> bool:
        with self._worker_lock:
            return self._worker_running

    def _require_result(self) -> CompareResult:
        if self.current_result is None:
            raise ValueError("Bitte zuerst einen Vergleich durchfuehren.")
        return self.current_result

    def _validate_dirs(self, left: Path, right: Path) -> None:
        if not left.exists() or not left.is_dir():
            raise ValueError(f"Ungueltiger linker Ordner: {left}")
        if not right.exists() or not right.is_dir():
            raise ValueError(f"Ungueltiger rechter Ordner: {right}")

    def _run_in_background(self, func) -> None:
        with self._worker_lock:
            if self._worker_running:
                raise RuntimeError("Es laeuft bereits eine Operation.")
            self._worker_running = True
            self._active_control = OperationControl()

        def wrapped() -> None:
            try:
                func()
                self._flush_ui_batch(force=True)
                self.worker_queue.put(WorkerMessage(MessageType.DONE))
            except OperationCancelledError as exc:
                self.worker_queue.put(WorkerMessage(MessageType.LOG, str(exc)))
                self._flush_ui_batch(force=True)
                self.worker_queue.put(WorkerMessage(MessageType.DONE))
            except Exception as exc:  # noqa: BLE001 - GUI should display any runtime error.
                self._flush_ui_batch(force=True)
                self.worker_queue.put(WorkerMessage(MessageType.ERROR, str(exc)))
            finally:
                with self._worker_lock:
                    self._worker_running = False
                    self._active_control = None

        thread = threading.Thread(target=wrapped, daemon=True)
        thread.start()

    def _require_control(self) -> OperationControl:
        control = self._active_control
        if control is None:
            raise RuntimeError("Keine aktive Operation.")
        return control

    @staticmethod
    def _validate_sync_pair(pair: SyncPair) -> SyncPair:
        left = Path(pair.left)
        right = Path(pair.right)
        if not left.exists() or not left.is_dir():
            raise ValueError(f"Ungueltiger linker Ordner im Paar '{pair.name}': {left}")
        if not right.exists() or not right.is_dir():
            raise ValueError(f"Ungueltiger rechter Ordner im Paar '{pair.name}': {right}")
        return SyncPair(pair_id=pair.pair_id, name=pair.name, left=str(left), right=str(right))

    def _run_pair_sync(self, pair: SyncPair, idx: int, total: int, control: OperationControl) -> None:
        control.checkpoint()
        left_path = Path(pair.left)
        right_path = Path(pair.right)
        self._push_task_state(pair.pair_id, "running")
        self._push_log(f"[{idx}/{total}] Sync-Paar '{pair.name}' gestartet")

        def prefixed_progress(event: ProgressEvent) -> None:
            prefixed = ProgressEvent(
                phase=event.phase,
                current=event.current,
                total=event.total,
                eta_seconds=event.eta_seconds,
                current_folder=f"{pair.name}: {event.current_folder}",
            )
            self._push_progress(prefixed)

        result = self.comparator.compare(
            left_path,
            right_path,
            progress_callback=prefixed_progress,
            control=control,
        )
        stats = self.operations.synchronize(
            result,
            progress_callback=prefixed_progress,
            control=control,
        )
        self._push_log(
            f"[{idx}/{total}] Sync-Paar '{pair.name}' fertig: "
            f"Kopiert={stats['copied']}, Geloescht={stats['deleted']}, "
            f"Ueberschrieben={stats['overwritten']}, Dirs+={stats['dirs_created']}, Dirs-={stats['dirs_removed']}"
        )
        self._push_task_state(pair.pair_id, "done")

    def _push_log(self, text: str) -> None:
        self.worker_queue.put(WorkerMessage(MessageType.LOG, text))

    def _push_progress(self, progress: ProgressEvent) -> None:
        self._enqueue_ui_batch(progress=progress)

    def _push_snapshot(self, snapshot: SnapshotEvent) -> None:
        self._enqueue_ui_batch(snapshot=snapshot)

    def _enqueue_ui_batch(self, progress: Optional[ProgressEvent] = None, snapshot: Optional[SnapshotEvent] = None) -> None:
        now = time.monotonic()
        with self._ui_batch_lock:
            if progress is not None:
                self._ui_batch_progress = progress
            if snapshot is not None:
                self._ui_batch_snapshots[snapshot.pair_id] = snapshot

            should_flush = (now - self._ui_batch_last_flush) >= self.UI_BATCH_FLUSH_SECONDS
        if should_flush:
            self._flush_ui_batch(force=False)

    def _flush_ui_batch(self, force: bool = False) -> None:
        payload = None
        with self._ui_batch_lock:
            if not self._ui_batch_snapshots and self._ui_batch_progress is None:
                return

            now = time.monotonic()
            if not force and (now - self._ui_batch_last_flush) < self.UI_BATCH_FLUSH_SECONDS:
                return

            payload = {
                "snapshots": list(self._ui_batch_snapshots.values()),
                "progress": self._ui_batch_progress,
            }
            self._ui_batch_snapshots = {}
            self._ui_batch_progress = None
            self._ui_batch_last_flush = now

        self.worker_queue.put(WorkerMessage(MessageType.BATCH, payload))

    def _push_task_state(self, pair_id: str, state: str) -> None:
        self.worker_queue.put(WorkerMessage(MessageType.TASK_STATE, TaskStateEvent(pair_id=pair_id, state=state)))


class CommandLineRunner:
    """Provides command-line access to all main application functions."""

    def __init__(self) -> None:
        self.comparator = DirectoryComparator()
        self.operations = FileOperationService(trash_delete_handler=resolve_trash_delete_handler())

    def compare(self, left: str, right: str, show_files: bool) -> Dict[str, object]:
        left_path, right_path = self._validate_dirs(left, right)
        result = self.comparator.compare(left_path, right_path)
        payload: Dict[str, object] = {
            "operation": "compare",
            "left": str(left_path),
            "right": str(right_path),
            "summary": result.summary(),
        }
        if show_files:
            payload["diffs"] = [
                {"type": e.diff_type.value, "path": str(e.relative_path)} for e in result.diffs
            ]
        return payload

    def synchronize(self, left: str, right: str, dry_run: bool) -> Dict[str, object]:
        left_path, right_path = self._validate_dirs(left, right)
        result = self.comparator.compare(left_path, right_path)
        if dry_run:
            stats = self.operations.synchronize_plan(result)
            return {
                "operation": "sync",
                "mode": "dry-run",
                "left": str(left_path),
                "right": str(right_path),
                "summary": result.summary(),
                "stats": stats,
            }

        stats = self.operations.synchronize(result)
        return {
            "operation": "sync",
            "mode": "apply",
            "left": str(left_path),
            "right": str(right_path),
            "summary": result.summary(),
            "stats": stats,
        }

    def copy_differences(self, left: str, right: str, target: str) -> Dict[str, object]:
        left_path, right_path = self._validate_dirs(left, right)
        target_path = Path(target).expanduser().resolve()
        result = self.comparator.compare(left_path, right_path)
        stats = self.operations.copy_differences(result, target_path)
        return {
            "operation": "copy-diff",
            "left": str(left_path),
            "right": str(right_path),
            "target": str(target_path),
            "summary": result.summary(),
            "stats": stats,
        }

    def compress_differences(self, left: str, right: str, zip_file: str) -> Dict[str, object]:
        left_path, right_path = self._validate_dirs(left, right)
        zip_path = Path(zip_file).expanduser().resolve()
        result = self.comparator.compare(left_path, right_path)
        stats = self.operations.compress_differences(result, zip_path)
        return {
            "operation": "zip-diff",
            "left": str(left_path),
            "right": str(right_path),
            "zip_file": str(zip_path),
            "summary": result.summary(),
            "stats": stats,
        }

    def automate(
        self,
        left: str,
        right: str,
        do_sync: bool,
        dry_run: bool,
        copy_target: Optional[str],
        zip_file: Optional[str],
    ) -> Dict[str, object]:
        left_path, right_path = self._validate_dirs(left, right)
        result = self.comparator.compare(left_path, right_path)

        actions: List[Dict[str, object]] = []
        actions.append({
            "step": "compare",
            "summary": result.summary(),
        })

        if do_sync:
            if dry_run:
                actions.append({
                    "step": "sync",
                    "mode": "dry-run",
                    "stats": self.operations.synchronize_plan(result),
                })
            else:
                actions.append({
                    "step": "sync",
                    "mode": "apply",
                    "stats": self.operations.synchronize(result),
                })

        if copy_target:
            target_path = Path(copy_target).expanduser().resolve()
            actions.append({
                "step": "copy-diff",
                "target": str(target_path),
                "stats": self.operations.copy_differences(result, target_path),
            })

        if zip_file:
            zip_path = Path(zip_file).expanduser().resolve()
            actions.append({
                "step": "zip-diff",
                "zip_file": str(zip_path),
                "stats": self.operations.compress_differences(result, zip_path),
            })

        return {
            "operation": "automate",
            "left": str(left_path),
            "right": str(right_path),
            "actions": actions,
        }

    @staticmethod
    def _validate_dirs(left: str, right: str) -> tuple[Path, Path]:
        left_path = Path(left).expanduser().resolve()
        right_path = Path(right).expanduser().resolve()

        if not left_path.exists() or not left_path.is_dir():
            raise ValueError(f"Ungueltiger linker Ordner: {left_path}")
        if not right_path.exists() or not right_path.is_dir():
            raise ValueError(f"Ungueltiger rechter Ordner: {right_path}")

        return left_path, right_path


def build_cli_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Folder Compare Tool - GUI und CLI",
    )
    parser.add_argument("--json-out", help="Optionaler Pfad fuer JSON-Report der Operation.")

    def add_json_out_arg(subparser: argparse.ArgumentParser) -> None:
        subparser.add_argument(
            "--json-out",
            dest="json_out",
            help="Optionaler Pfad fuer JSON-Report der Operation.",
        )

    subparsers = parser.add_subparsers(dest="command")

    compare_p = subparsers.add_parser("compare", help="Vergleich zweier Ordner")
    compare_p.add_argument("--left", required=True, help="Linker Ordner")
    compare_p.add_argument("--right", required=True, help="Rechter Ordner")
    compare_p.add_argument(
        "--show-files",
        action="store_true",
        help="Listet alle Differenzdateien im JSON-Output.",
    )
    add_json_out_arg(compare_p)

    sync_p = subparsers.add_parser("sync", help="Synchronisiert links -> rechts")
    sync_p.add_argument("--left", required=True, help="Linker Ordner")
    sync_p.add_argument("--right", required=True, help="Rechter Ordner")
    sync_p.add_argument(
        "--dry-run",
        action="store_true",
        help="Nur geplante Aenderungen berechnen, nichts schreiben.",
    )
    add_json_out_arg(sync_p)

    copy_p = subparsers.add_parser("copy-diff", help="Kopiert Differenzen in Zielordner")
    copy_p.add_argument("--left", required=True, help="Linker Ordner")
    copy_p.add_argument("--right", required=True, help="Rechter Ordner")
    copy_p.add_argument("--target", required=True, help="Zielordner")
    add_json_out_arg(copy_p)

    zip_p = subparsers.add_parser("zip-diff", help="Komprimiert Differenzen als ZIP")
    zip_p.add_argument("--left", required=True, help="Linker Ordner")
    zip_p.add_argument("--right", required=True, help="Rechter Ordner")
    zip_p.add_argument("--zip-file", required=True, help="Ziel-ZIP-Datei")
    add_json_out_arg(zip_p)

    auto_p = subparsers.add_parser("automate", help="Automatisierungs-Pipeline")
    auto_p.add_argument("--left", required=True, help="Linker Ordner")
    auto_p.add_argument("--right", required=True, help="Rechter Ordner")
    auto_p.add_argument("--sync", action="store_true", help="Synchronisierung ausfuehren")
    auto_p.add_argument(
        "--dry-run",
        action="store_true",
        help="Nur bei --sync relevant: geplante Sync-Aenderungen zeigen.",
    )
    auto_p.add_argument("--copy-target", help="Optional: Differenzen in Ordner kopieren")
    auto_p.add_argument("--zip-file", help="Optional: Differenzen als ZIP speichern")
    add_json_out_arg(auto_p)

    return parser


def write_json_report(payload: Dict[str, object], json_out: Optional[str]) -> None:
    json_text = json.dumps(payload, indent=2, ensure_ascii=True)
    print(json_text)
    if not json_out:
        return

    out_path = Path(json_out).expanduser().resolve()
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json_text, encoding="utf-8")


def run_cli(args: argparse.Namespace) -> int:
    runner = CommandLineRunner()

    if args.command == "compare":
        payload = runner.compare(args.left, args.right, args.show_files)
        write_json_report(payload, args.json_out)
        return 0

    if args.command == "sync":
        payload = runner.synchronize(args.left, args.right, args.dry_run)
        write_json_report(payload, args.json_out)
        return 0

    if args.command == "copy-diff":
        payload = runner.copy_differences(args.left, args.right, args.target)
        write_json_report(payload, args.json_out)
        return 0

    if args.command == "zip-diff":
        payload = runner.compress_differences(args.left, args.right, args.zip_file)
        write_json_report(payload, args.json_out)
        return 0

    if args.command == "automate":
        payload = runner.automate(
            left=args.left,
            right=args.right,
            do_sync=args.sync,
            dry_run=args.dry_run,
            copy_target=args.copy_target,
            zip_file=args.zip_file,
        )
        write_json_report(payload, args.json_out)
        return 0

    return 1


def create_runtime_dependencies() -> RuntimeDependencies:
    """Build the default portable runtime backends for the one-file app."""
    storage = SQLiteAppStore()
    watcher = WatcherFactory.create()
    return RuntimeDependencies(storage=storage, watcher=watcher)


class FolderCompareApp(tk.Tk):
    """Tkinter-based Windows desktop app for directory comparison."""

    def __init__(self) -> None:
        super().__init__()
        self.title("Folder Compare Tool")
        self.default_geometry = "920x580"
        self.geometry(self.default_geometry)
        self.minsize(760, 460)
        self.resizable(True, True)

        self.style = ttk.Style(self)

        self.runtime_deps = create_runtime_dependencies()
        self.runtime_deps.watcher.start()
        self.controller = AppController(self)
        self.scheduler = SchedulerController(self)
        self.settings_store = self.runtime_deps.storage
        self.state_repository = AppStateRepository(self.settings_store)
        self.settings: Dict[str, str] = self.state_repository.load_settings()
        self.job_states: Dict[str, SyncJobState] = self.state_repository.load_job_states(self.settings)
        self.tab_controllers: Dict[str, SyncTabController] = {}
        self.current_theme = self.settings.get("ui_theme", "light").strip().lower() or "light"
        self._apply_theming(self.current_theme)

        self.left_var = tk.StringVar(value=self.settings.get("left_path", ""))
        self.right_var = tk.StringVar(value=self.settings.get("right_path", ""))
        self.status_var = tk.StringVar(value="Bereit")
        self.progress_var = tk.DoubleVar(value=0.0)
        self.progress_text_var = tk.StringVar(value="0%")
        self.current_folder_var = tk.StringVar(value="Ordner: -")
        self.eta_var = tk.StringVar(value="ETA: -")
        self.sync_mode_var = tk.StringVar(value=self.settings.get("sync_pairs_mode", "sequential"))
        self.max_parallel_var = tk.IntVar(value=max(1, self._parse_int_setting("sync_pairs_max_parallel", 2)))
        self._is_paused = False
        self.sync_pairs: List[SyncPair] = self._load_sync_pairs_from_settings()
        self.workflow_steps_by_pair: Dict[str, List[WorkflowStep]] = self._load_workflow_steps_from_settings()
        self.sync_pair_widgets: Dict[str, Dict[str, object]] = {}
        self._overall_batch_pairs: Set[str] = set()
        self.current_running_pair_id: Optional[str] = None
        self.last_active_pair_id: str = self.settings.get("last_active_pair_id", "")
        self.current_operation_pair_id: Optional[str] = None
        self._snapshot_refresh_stop = threading.Event()
        self._snapshot_refresh_thread: Optional[threading.Thread] = None

        self.dashboard_total_pairs_var = tk.StringVar(value=str(len(self.sync_pairs)))
        self.dashboard_mode_var = tk.StringVar(value=self.sync_mode_var.get())
        self.dashboard_workers_var = tk.StringVar(value=str(self.max_parallel_var.get()))
        self.dashboard_running_var = tk.StringVar(value="-")
        self.sqlite_app_var = tk.StringVar(value="SQLite-App: -")
        self.sqlite_pair_var = tk.StringVar(value="SQLite-Paar: -")
        self.dashboard_log_text: Optional[tk.Text] = None
        self.global_log_lines = self._load_persistent_logs()
        self.agent_port_var = tk.StringVar(value=self.settings.get("agent_port", "8765"))
        self.agent_state_var = tk.StringVar(value="Agent: nicht gestartet")
        self.agent_server: Optional[SyncAgentServer] = None
        self._tooltips: List[HoverTooltip] = []

        self._apply_window_settings()
        self.bind("<Configure>", self._on_window_configure, add="+")
        self._build_ui()
        self._build_main_menu()
        self._install_button_tooltips()
        self._refresh_sqlite_status()
        self._start_sync_agent_server()
        self.protocol("WM_DELETE_WINDOW", self._on_close)
        self._set_busy(False)
        self._poll_worker_queue()

    def _theme_parent_name(self) -> str:
        names = set(self.style.theme_names())
        if sys.platform.startswith("win"):
            for candidate in ("vista", "xpnative", "winnative", "clam"):
                if candidate in names:
                    return candidate
        if sys.platform == "darwin" and "aqua" in names:
            return "aqua"
        return "clam" if "clam" in names else next(iter(names), "default")

    def _theme_palette(self, variant: str) -> Dict[str, str]:
        if variant == "dark":
            return {
                "bg": "#111827",
                "panel": "#1F2937",
                "surface": "#0F172A",
                "text": "#E5E7EB",
                "muted": "#9CA3AF",
                "accent": "#2563EB",
                "accent_soft": "#1E3A8A",
                "border": "#334155",
                "tree_alt": "#1F2937",
            }
        return {
            "bg": "#F3F4F6",
            "panel": "#E6F0FA",
            "surface": "#FFFFFF",
            "text": "#0F172A",
            "muted": "#334155",
            "accent": "#1D4ED8",
            "accent_soft": "#DCEBFF",
            "border": "#CBD5E1",
            "tree_alt": "#F8FAFC",
        }

    def _apply_theming(self, variant: str) -> None:
        variant = "dark" if variant == "dark" else "light"
        palette = self._theme_palette(variant)
        parent = self._theme_parent_name()
        theme_name = f"themis_modern_{variant}"

        if theme_name not in self.style.theme_names():
            self.style.theme_create(
                theme_name,
                parent=parent,
                settings={
                    ".": {
                        "configure": {
                            "background": palette["bg"],
                            "foreground": palette["text"],
                            "font": ("Segoe UI", 9),
                        }
                    },
                    "TFrame": {"configure": {"background": palette["bg"]}},
                    "TLabelframe": {
                        "configure": {
                            "background": palette["surface"],
                            "foreground": palette["text"],
                            "bordercolor": palette["border"],
                            "relief": "groove",
                        }
                    },
                    "TLabelframe.Label": {"configure": {"background": palette["surface"], "foreground": palette["text"]}},
                    "TLabel": {"configure": {"background": palette["bg"], "foreground": palette["text"]}},
                    "TButton": {
                        "configure": {
                            "padding": (8, 5),
                            "background": palette["surface"],
                            "foreground": palette["text"],
                            "bordercolor": palette["border"],
                        },
                        "map": {
                            "background": [("active", palette["accent_soft"]), ("pressed", palette["accent"])],
                            "foreground": [("pressed", "#FFFFFF")],
                        },
                    },
                    "TEntry": {"configure": {"fieldbackground": palette["surface"], "foreground": palette["text"]}},
                    "TCombobox": {
                        "configure": {
                            "padding": (5, 3),
                            "fieldbackground": palette["surface"],
                            "foreground": palette["text"],
                        }
                    },
                    "TNotebook": {"configure": {"background": palette["bg"], "bordercolor": palette["border"]}},
                    "TNotebook.Tab": {
                        "configure": {"padding": (10, 6), "font": ("Segoe UI", 9, "bold")},
                        "map": {
                            "background": [("selected", palette["surface"]), ("active", palette["accent_soft"])],
                            "foreground": [("selected", palette["text"])],
                        },
                    },
                    "Treeview": {
                        "configure": {
                            "background": palette["surface"],
                            "foreground": palette["text"],
                            "fieldbackground": palette["surface"],
                            "rowheight": 22,
                        },
                        "map": {
                            "background": [("selected", palette["accent"])],
                            "foreground": [("selected", "#FFFFFF")],
                        },
                    },
                    "Treeview.Heading": {
                        "configure": {
                            "font": ("Segoe UI", 9, "bold"),
                            "background": palette["tree_alt"],
                            "foreground": palette["text"],
                        }
                    },
                    "TProgressbar": {
                        "configure": {"background": palette["accent"], "troughcolor": palette["tree_alt"]}
                    },
                },
            )

        self.style.theme_use(theme_name)
        self.style.configure("Compact.TButton", padding=(8, 5), font=("Segoe UI", 9))
        self.style.configure("Compact.TLabel", font=("Segoe UI", 9), background=palette["bg"], foreground=palette["text"])
        self.style.configure("Compact.TCombobox", padding=(5, 3), font=("Segoe UI", 9))
        self.style.configure("Header.TFrame", background=palette["panel"])
        self.style.configure("HeaderTitle.TLabel", background=palette["panel"], font=("Segoe UI", 12, "bold"), foreground=palette["text"])
        self.style.configure("HeaderMeta.TLabel", background=palette["panel"], font=("Segoe UI", 9), foreground=palette["muted"])
        self.style.configure("HeaderStatus.TLabel", background=palette["accent_soft"], font=("Segoe UI", 9, "bold"), foreground=palette["accent"], padding=(10, 4))
        self.style.configure("Section.TFrame", background=palette["tree_alt"])
        self.style.configure("Toolbar.TFrame", background=palette["tree_alt"])
        self.style.configure(
            "WorkflowCard.TFrame",
            background=palette["panel"],
            borderwidth=2,
            relief="raised",
        )
        self.style.configure(
            "WorkflowCardSelected.TFrame",
            background=palette["accent_soft"],
            borderwidth=2,
            relief="raised",
        )
        self.style.configure("WorkflowCardRow.TFrame", background=palette["panel"])
        self.style.configure("WorkflowCardRowSelected.TFrame", background=palette["accent_soft"])
        self.style.configure("WorkflowCardLabel.TLabel", background=palette["panel"], foreground=palette["text"])
        self.style.configure("WorkflowCardLabelSelected.TLabel", background=palette["accent_soft"], foreground=palette["text"])
        self.style.configure("WorkflowCardIndex.TLabel", background=palette["panel"], foreground=palette["accent"], font=("Segoe UI", 11, "bold"))
        self.style.configure("WorkflowCardIndexSelected.TLabel", background=palette["accent_soft"], foreground=palette["accent"], font=("Segoe UI", 11, "bold"))
        self.style.configure(
            "WorkflowInsert.TFrame",
            background=palette["tree_alt"],
            borderwidth=1,
            relief="solid",
        )
        self.configure(bg=palette["bg"])
        self.option_add("*TCombobox*Listbox.font", "Segoe UI 9")

        dashboard_log = getattr(self, "dashboard_log_text", None)
        if isinstance(dashboard_log, tk.Text):
            dashboard_log.configure(
                background=palette["surface"],
                foreground=palette["text"],
                insertbackground=palette["text"],
            )

        self.current_theme = variant
        self.settings["ui_theme"] = variant

    def _start_sync_agent_server(self) -> None:
        try:
            port = int(self.agent_port_var.get())
        except Exception:
            port = 8765
            self.agent_port_var.set("8765")

        try:
            self.agent_server = SyncAgentServer(host="0.0.0.0", port=port)
            self.agent_server.start()
            self.agent_state_var.set(f"Agent: aktiv auf Port {port}")
            self._append_log(f"Sync-Agent gestartet auf Port {port}")
        except Exception as exc:  # noqa: BLE001
            self.agent_server = None
            self.agent_state_var.set("Agent: Fehler beim Start")
            self._append_log(f"Sync-Agent Startfehler: {exc}")

    def _stop_sync_agent_server(self) -> None:
        if self.agent_server is None:
            return
        try:
            self.agent_server.stop()
            self.agent_state_var.set("Agent: gestoppt")
        except Exception:
            pass
        finally:
            self.agent_server = None

    def _parse_int_setting(self, key: str, default_value: int) -> int:
        raw = self.settings.get(key, str(default_value))
        try:
            return int(raw)
        except Exception:
            return default_value

    def _apply_window_settings(self) -> None:
        geometry = self.settings.get("window_geometry", "").strip()
        if geometry:
            try:
                self.geometry(geometry)
            except tk.TclError:
                pass

    def _on_window_configure(self, event: Optional[tk.Event] = None) -> None:
        if event is not None and event.widget is not self:
            return
        try:
            current_state = str(self.state())
        except tk.TclError:
            current_state = "normal"
        if current_state in {"zoomed", "fullscreen"}:
            return
        try:
            self.settings["window_geometry"] = self.geometry()
        except tk.TclError:
            pass

    def _minimize_window(self) -> None:
        try:
            self.iconify()
        except tk.TclError:
            pass

    def _toggle_maximize_window(self) -> None:
        try:
            current_state = str(self.state())
        except tk.TclError:
            current_state = "normal"
        if current_state == "zoomed":
            self.state("normal")
        else:
            self.state("zoomed")

    def _restore_window(self) -> None:
        try:
            self.state("normal")
        except tk.TclError:
            pass

    def _build_ui(self) -> None:
        container = ttk.Frame(self, padding=4)
        container.pack(fill=tk.BOTH, expand=True)

        self._build_toolbar(container)

        self.sync_notebook = ttk.Notebook(container)
        self.sync_notebook.pack(fill=tk.BOTH, expand=True)
        self.sync_notebook.bind("<<NotebookTabChanged>>", self._on_notebook_tab_changed)
        self.sync_notebook.bind("<Double-Button-1>", self._on_notebook_tab_double_click)
        self._refresh_sync_tabs()

        self._build_statusbar(container)

    def _install_button_tooltips(self) -> None:
        for widget in self.winfo_children():
            self._install_button_tooltips_recursive(widget)

    def _install_button_tooltips_recursive(self, widget) -> None:
        if isinstance(widget, (ttk.Button, tk.Button)):
            if not bool(widget.tk.call("info", "exists", str(widget))):
                return
            if str(widget.cget("state")) == "disabled":
                pass
            tooltip_text = self._tooltip_for_button(widget)
            if tooltip_text:
                try:
                    if not getattr(widget, "_has_tooltip", False):
                        self._tooltips.append(HoverTooltip(widget, tooltip_text))
                        setattr(widget, "_has_tooltip", True)
                except Exception:
                    pass

        for child in widget.winfo_children():
            self._install_button_tooltips_recursive(child)

    @staticmethod
    def _tooltip_for_button(widget) -> str:
        raw = str(widget.cget("text")).strip().lower()
        cleaned = (
            raw.replace("➕", "").replace("🗑", "").replace("▶", "").replace("⏵", "")
            .replace("🗕", "").replace("🗖", "").replace("🗗", "").replace("📂", "")
            .replace("🔍", "").replace("🔄", "").replace("⏸", "").replace("⛔", "")
            .replace("📋", "").replace("🗜", "").replace("📁", "").replace("🧹", "")
            .replace("🔎", "").replace("☐", "").replace("☑", "")
            .replace("⚙", "").replace("📊", "").replace("✖", "")
            .strip()
        )
        tips = {
            "neu": "Erstellt ein neues Sync-Tab mit eigenem Ordnerpaar.",
            "tab löschen": "Entfernt das aktuell aktive Sync-Tab.",
            "aktiven starten": "Startet die Synchronisierung nur fuer das aktive Tab.",
            "alle starten": "Startet die Synchronisierung fuer alle angelegten Tabs.",
            "workflow": "Startet den gespeicherten Workflow fuer das aktive Tab.",
            "start": "Startet den Workflow fuer dieses Sync-Tab.",
            "plan starten": "Fuehrt die Schrittfolge des aktuellen Sync-Tabs aus.",
            "ordner": "Blendet die Ordnerauswahl als Bedarfsbereich ein oder aus.",
            "ordner a": "Waehlt den Quellordner A fuer die Ordner-A/B-Card.",
            "ordner b": "Waehlt den Zielordner B fuer die Ordner-A/B-Card.",
            "pfad a": "Waehlt den Quellpfad A fuer die Ordner-A/B-Card.",
            "pfad b": "Waehlt den Zielpfad B fuer die Ordner-A/B-Card.",
            "filter": "Blendet die Filteraktionen als Bedarfsbereich ein oder aus.",
            "aktionen": "Blendet Check/Sync/Kopier/ZIP-Aktionen als Bedarfsbereich ein oder aus.",
            "ergebnis": "Blendet die Ergebnisansicht als Bedarfsbereich ein oder aus.",
            "schliessen": "Schliesst das aktuell eingeblendete Detail-Overlay.",
            "links": "Oeffnet die Ordnerauswahl fuer die linke Quelle.",
            "rechts": "Oeffnet die Ordnerauswahl fuer die rechte Quelle.",
            "check": "Vergleicht linke und rechte Seite und aktualisiert die Ergebnisliste.",
            "sync": "Fuehrt die Synchronisierung gemaess aktueller Regeln aus.",
            "pause": "Pausiert oder setzt die laufende Operation fort.",
            "abbruch": "Bricht die laufende Operation ab.",
            "kopieren": "Kopiert nur erkannte Differenzen in einen Zielordner.",
            "zip": "Packt die Differenzen in ein ZIP-Archiv.",
            "+": "Fuegt zwischen zwei Karten einen neuen Workflow-Schritt ein.",
            "schritt": "Fuegt einen weiteren Workflow-Schritt hinzu.",
            "vorlagen": "Laedt eine fertige Workflow-Vorlage.",
            "standard": "Setzt die Workflow-Schrittfolge auf die Standardvorlage zurueck.",
            "ordner wählen": "Waehlt einen Ordner fuer diese Seite aus.",
            "filter leeren": "Setzt Such- und Filterwerte in der Tabelle zurueck.",
            "neu filtern": "Wendet die aktuellen Filter sofort erneut an.",
            "auswahl löschen": "Entfernt alle gesetzten Checkbox-Auswahlen.",
            "alle sichtbaren": "Markiert alle aktuell sichtbaren Tabellenzeilen.",
        }
        return tips.get(cleaned, "Aktion ausfuehren")

    def _build_main_menu(self) -> None:
        menu_bar = tk.Menu(self)

        file_menu = tk.Menu(menu_bar, tearoff=0)
        file_menu.add_command(label="Linken Ordner auswaehlen", command=self._pick_left)
        file_menu.add_command(label="Rechten Ordner auswaehlen", command=self._pick_right)
        file_menu.add_separator()
        file_menu.add_command(label="Einstellungen zuruecksetzen", command=self._reset_settings)
        file_menu.add_separator()
        file_menu.add_command(label="Beenden", command=self._on_close)
        menu_bar.add_cascade(label="Datei", menu=file_menu)

        action_menu = tk.Menu(menu_bar, tearoff=0)
        action_menu.add_command(label="Vergleichen", command=self._on_compare)
        action_menu.add_command(label="Synchronisieren", command=self._on_sync)
        action_menu.add_command(label="Unterbrechen/Fortsetzen", command=self._on_pause_resume)
        action_menu.add_command(label="Abbrechen", command=self._on_cancel)
        action_menu.add_separator()
        action_menu.add_command(label="Aktiven Sync-Tab synchronisieren", command=self._on_sync_active_pair)
        action_menu.add_command(label="Alle Sync-Tabs synchronisieren", command=self._on_sync_pairs)
        action_menu.add_command(label="Differenz kopieren", command=self._on_copy_diff)
        action_menu.add_command(label="Differenz komprimieren", command=self._on_zip_diff)
        menu_bar.add_cascade(label="Aktionen", menu=action_menu)

        view_menu = tk.Menu(menu_bar, tearoff=0)
        view_menu.add_command(label="Theme: Hell", command=lambda: self._set_theme("light"))
        view_menu.add_command(label="Theme: Dunkel", command=lambda: self._set_theme("dark"))
        menu_bar.add_cascade(label="Ansicht", menu=view_menu)

        help_menu = tk.Menu(menu_bar, tearoff=0)
        help_menu.add_command(label="Ueber", command=self._show_about)
        menu_bar.add_cascade(label="Hilfe", menu=help_menu)

        self.config(menu=menu_bar)

    def _set_theme(self, variant: str) -> None:
        self._apply_theming(variant)
        self._refresh_theme_sensitive_widgets()
        self._append_log(f"Theme gewechselt: {self.current_theme}")

    def _refresh_theme_sensitive_widgets(self) -> None:
        palette = self._theme_palette(self.current_theme)
        for pair_id, widget in self.sync_pair_widgets.items():
            workflow_canvas = widget.get("workflow_canvas")
            if isinstance(workflow_canvas, tk.Canvas):
                workflow_canvas.configure(background=palette["surface"])

            optional_overlay = widget.get("optional_overlay")
            if isinstance(optional_overlay, tk.Frame):
                optional_overlay.configure(bg=palette["panel"], highlightbackground=palette["border"])

            result_overlay = widget.get("result_overlay")
            if isinstance(result_overlay, tk.Frame):
                result_overlay.configure(bg=palette["surface"])

            color_bar = widget.get("color_bar")
            if isinstance(color_bar, tk.Frame):
                # Keep state-colored badges/bars but harmonize idle tone with active theme.
                try:
                    if str(color_bar.cget("bg")).lower() in {"#6b7280", "grey", "gray"}:
                        color_bar.configure(bg=palette["muted"])
                except Exception:
                    pass

            self._render_workflow_steps(pair_id)

    def _build_toolbar(self, parent: ttk.Frame) -> None:
        toolbar = ttk.Frame(parent, style="Toolbar.TFrame", padding=(6, 4))
        toolbar.pack(fill=tk.X, pady=(0, 6))

        task_group = ttk.Frame(toolbar, style="Toolbar.TFrame")
        task_group.pack(side=tk.LEFT)
        ttk.Label(task_group, text="Tabs", style="Compact.TLabel").pack(side=tk.LEFT, padx=(0, 6))
        self.add_pair_btn = ttk.Button(task_group, text="➕ Neu", command=self._on_add_pair, style="Compact.TButton")
        self.add_pair_btn.pack(side=tk.LEFT, padx=(0, 5))
        self.remove_pair_btn = ttk.Button(task_group, text="🗑 Tab löschen", command=self._on_remove_pair, style="Compact.TButton")
        self.remove_pair_btn.pack(side=tk.LEFT, padx=(0, 8))

        ttk.Separator(toolbar, orient=tk.VERTICAL).pack(side=tk.LEFT, fill=tk.Y, padx=(0, 8), pady=2)

        run_group = ttk.Frame(toolbar, style="Toolbar.TFrame")
        run_group.pack(side=tk.LEFT)
        ttk.Label(run_group, text="Ausführung", style="Compact.TLabel").pack(side=tk.LEFT, padx=(0, 6))
        self.sync_tab_btn = ttk.Button(run_group, text="▶ Aktiven starten", command=self._on_sync_active_pair, style="Compact.TButton")
        self.sync_tab_btn.pack(side=tk.LEFT, padx=(0, 5))
        self.sync_all_pairs_btn = ttk.Button(run_group, text="⏵ Alle starten", command=self._on_sync_pairs, style="Compact.TButton")
        self.sync_all_pairs_btn.pack(side=tk.LEFT, padx=(0, 8))
        self.workflow_run_btn = ttk.Button(run_group, text="🧩 Workflow", command=self._on_run_workflow_active_pair, style="Compact.TButton")
        self.workflow_run_btn.pack(side=tk.LEFT, padx=(0, 8))

        overall_group = ttk.Frame(toolbar, style="Toolbar.TFrame")
        overall_group.pack(side=tk.RIGHT, padx=(8, 0))
        ttk.Label(overall_group, text="Gesamt", style="Compact.TLabel").pack(side=tk.LEFT, padx=(0, 6))
        self.overall_progress_bar = ttk.Progressbar(
            overall_group,
            orient=tk.HORIZONTAL,
            mode="determinate",
            variable=self.progress_var,
            maximum=100.0,
            length=130,
        )
        self.overall_progress_bar.pack(side=tk.LEFT)
        self.overall_progress_label = ttk.Label(overall_group, textvariable=self.progress_text_var, style="Compact.TLabel")
        self.overall_progress_label.pack(side=tk.LEFT, padx=(6, 0))

        settings_group = ttk.Frame(toolbar, style="Toolbar.TFrame")
        settings_group.pack(side=tk.RIGHT)
        ttk.Label(settings_group, text="Modus:", style="Compact.TLabel").pack(side=tk.LEFT, padx=(0, 4))
        self.sync_mode_combo = ttk.Combobox(
            settings_group,
            textvariable=self.sync_mode_var,
            values=("sequential", "parallel"),
            state="readonly",
            width=12,
            style="Compact.TCombobox",
        )
        self.sync_mode_combo.pack(side=tk.LEFT)
        ttk.Label(settings_group, text="Worker:", style="Compact.TLabel").pack(side=tk.LEFT, padx=(8, 4))
        self.max_parallel_spin = ttk.Spinbox(
            settings_group,
            from_=1,
            to=32,
            textvariable=self.max_parallel_var,
            width=5,
            style="Compact.TCombobox",
        )
        self.max_parallel_spin.pack(side=tk.LEFT)

    def _build_statusbar(self, parent: ttk.Frame) -> None:
        bar = ttk.Frame(parent)
        bar.pack(fill=tk.X, pady=(6, 0))

        ttk.Separator(bar, orient=tk.HORIZONTAL).pack(fill=tk.X, pady=(0, 3))
        row1 = ttk.Frame(bar)
        row1.pack(fill=tk.X)
        ttk.Label(row1, textvariable=self.status_var, anchor="w", style="Compact.TLabel").pack(side=tk.LEFT)
        ttk.Label(row1, textvariable=self.current_folder_var, anchor="w", style="Compact.TLabel").pack(side=tk.LEFT, padx=(12, 0))
        ttk.Label(row1, textvariable=self.eta_var, anchor="w", style="Compact.TLabel").pack(side=tk.RIGHT)

    def _poll_worker_queue(self) -> None:
        latest_progress: Optional[ProgressEvent] = None
        latest_snapshot_by_pair: Dict[str, SnapshotEvent] = {}
        try:
            processed = 0
            max_per_tick = 400
            while True:
                message = self.controller.worker_queue.get_nowait()
                processed += 1

                if message.message_type == MessageType.PROGRESS and isinstance(message.payload, ProgressEvent):
                    latest_progress = message.payload
                elif message.message_type == MessageType.SNAPSHOT and isinstance(message.payload, SnapshotEvent):
                    latest_snapshot_by_pair[message.payload.pair_id] = message.payload
                else:
                    self._handle_worker_message(message)

                if processed >= max_per_tick:
                    break
        except queue.Empty:
            pass

        for snapshot in latest_snapshot_by_pair.values():
            self._render_snapshot(snapshot)

        if latest_progress is not None:
            self._apply_progress(latest_progress)

        self.after(60, self._poll_worker_queue)

    def _handle_worker_message(self, message: WorkerMessage) -> None:
        if message.message_type == MessageType.BATCH:
            payload = message.payload
            if isinstance(payload, dict):
                snapshots = payload.get("snapshots", [])
                if isinstance(snapshots, list):
                    for snapshot in snapshots:
                        if isinstance(snapshot, SnapshotEvent):
                            self._render_snapshot(snapshot)
                progress = payload.get("progress")
                if isinstance(progress, ProgressEvent):
                    self._apply_progress(progress)
            return

        if message.message_type == MessageType.LOG:
            self._append_log(str(message.payload))
            return

        if message.message_type == MessageType.RESULT:
            result = message.payload
            if isinstance(result, CompareResult):
                self.controller.current_result = result
                self._render_result(result)
            return

        if message.message_type == MessageType.SNAPSHOT:
            snapshot = message.payload
            if isinstance(snapshot, SnapshotEvent):
                self._render_snapshot(snapshot)
            return

        if message.message_type == MessageType.PROGRESS:
            progress = message.payload
            if isinstance(progress, ProgressEvent):
                self._apply_progress(progress)
            return

        if message.message_type == MessageType.TASK_STATE:
            event = message.payload
            if isinstance(event, TaskStateEvent):
                self._set_pair_state(event.pair_id, event.state)
            return

        if message.message_type == MessageType.ERROR:
            self._stop_live_snapshot_refresh()
            pair_id = self.current_operation_pair_id or self.current_running_pair_id
            if pair_id is not None:
                self._set_pair_state(pair_id, "error")
                self._update_workflow_result_card(
                    pair_id,
                    title="Workflow-Fehler",
                    line=str(message.payload),
                )
            self._append_log(f"FEHLER: {message.payload}")
            self._set_status("Fehler")
            messagebox.showerror("Fehler", str(message.payload))
            self._set_busy(False)
            return

        if message.message_type == MessageType.DONE:
            self._stop_live_snapshot_refresh()
            pair_id = self.current_running_pair_id or self.current_operation_pair_id
            if pair_id is not None:
                pair = next((item for item in self.sync_pairs if item.pair_id == pair_id), None)
                if pair is not None and pair.state not in {"error", "cancelled"}:
                    self._set_pair_state(pair_id, "done")
                    self._update_workflow_result_card(pair_id, title="Workflow abgeschlossen")
                self.current_running_pair_id = None
                self.dashboard_running_var.set("-")
            self.current_operation_pair_id = None
            self._set_status("Bereit")
            self._set_busy(False)

    def _render_result(self, result: CompareResult) -> None:
        widget = self._current_pair_widgets()
        if widget is None:
            return
        widget["current_result"] = result
        summary_var = widget.get("result_summary_var")
        if isinstance(summary_var, tk.StringVar):
            summary_var.set(f"Ergebnis: {len(result.diffs)} Treffer")
        table = widget.get("sync_tree")
        if not isinstance(table, ttk.Treeview):
            return

        table.delete(*table.get_children())
        widget["all_rows"] = []
        row_state_by_key = widget.setdefault("row_state_by_key", {})
        row_state_by_item = {}

        search_var = widget.get("search_var")
        filter_var = widget.get("filter_var")
        search_text = search_var.get().strip().lower() if isinstance(search_var, tk.StringVar) else ""
        filter_value = filter_var.get().strip().lower() if isinstance(filter_var, tk.StringVar) else "all"
        column_filters = widget.get("column_filters", {})

        for entry in sorted(result.diffs, key=lambda e: (e.diff_type.value, str(e.relative_path))):
            rel_text = str(entry.relative_path)
            entry_type = entry.diff_type.value.lower()
            side_text = "links" if entry.diff_type == DiffType.LEFT_ONLY else "rechts" if entry.diff_type == DiffType.RIGHT_ONLY else "beide"
            if filter_value not in {"all", entry_type}:
                continue
            if search_text and search_text not in rel_text.lower() and search_text not in entry_type and search_text not in side_text.lower():
                continue

            matches_column_filters = True
            for key, value_var in column_filters.items():
                if not isinstance(value_var, tk.StringVar):
                    continue
                needle = value_var.get().strip().lower()
                if not needle:
                    continue
                column_value = ""
                if key == "type":
                    column_value = entry.diff_type.value.lower()
                elif key == "side":
                    column_value = side_text.lower()
                elif key == "path":
                    column_value = rel_text.lower()
                if needle not in column_value:
                    matches_column_filters = False
                    break
            if not matches_column_filters:
                continue

            entry_key = self._entry_state_key(entry)
            previous_state = row_state_by_key.get(entry_key, {})
            checked = bool(previous_state.get("checked", False))
            operation = str(previous_state.get("operation", self._default_row_operation(entry)))

            values = (
                "☑" if checked else "☐",
                self._operation_display_label(operation),
                self._diff_type_display_label(entry.diff_type),
                side_text,
                rel_text,
            )
            item_id = table.insert("", tk.END, values=values, tags=(entry_type,))
            widget.setdefault("all_rows", []).append(item_id)
            row_state_by_item[item_id] = {
                "key": entry_key,
                "entry": entry,
                "checked": checked,
                "operation": operation,
            }
            row_state_by_key[entry_key] = {
                "entry": entry,
                "checked": checked,
                "operation": operation,
            }

        widget["row_state_by_item"] = row_state_by_item
        pair_id = self.current_operation_pair_id or self._selected_pair_id_if_visible()
        self._update_pair_selection_label(pair_id)

        left_only = len(result.by_type(DiffType.LEFT_ONLY))
        right_only = len(result.by_type(DiffType.RIGHT_ONLY))
        different = len(result.by_type(DiffType.DIFFERENT))
        if self.current_operation_pair_id:
            self._update_workflow_result_card(
                self.current_operation_pair_id,
                title=f"Ergebnis: {len(result.diffs)} Treffer",
                line=f"LEFT={left_only} | RIGHT={right_only} | DIFF={different}",
            )
        self._update_action_button_states()
        self._append_log(
            f"Ergebnis - LEFT_ONLY: {left_only}, RIGHT_ONLY: {right_only}, DIFFERENT: {different}"
        )

    def _update_workflow_result_card(
        self,
        pair_id: str,
        title: Optional[str] = None,
        line: Optional[str] = None,
        *,
        reset: bool = False,
    ) -> None:
        widget = self.sync_pair_widgets.get(pair_id)
        if widget is None:
            return
        title_var = widget.get("workflow_result_title_var")
        body_var = widget.get("workflow_result_body_var")

        if isinstance(title_var, tk.StringVar) and title:
            title_var.set(title)

        if isinstance(body_var, tk.StringVar):
            if reset:
                body_var.set(str(line) if line else "Warte auf Workflow-Start...")
            elif line is not None:
                body_var.set(str(line))

    def _show_result_list_overlay(self, pair_id: str) -> None:
        widget = self.sync_pair_widgets.get(pair_id)
        if widget is None:
            return

        result = widget.get("current_result")
        if not isinstance(result, CompareResult):
            messagebox.showinfo("Ergebnis", "Für diese Aufgabe liegt noch keine Ergebnisliste vor.")
            return

        top = tk.Toplevel(self.root)
        top.title(f"Ergebnisliste - {widget.get('name', pair_id)}")
        top.geometry("980x620")
        top.minsize(760, 420)
        top.transient(self.root)
        top.grab_set()

        search_var = tk.StringVar(value="")
        type_var = tk.StringVar(value="all")
        side_var = tk.StringVar(value="all")

        toolbar = ttk.Frame(top, padding=(10, 8, 10, 4))
        toolbar.pack(fill=tk.X)
        ttk.Label(toolbar, text="Suche:").pack(side=tk.LEFT, padx=(0, 6))
        ttk.Entry(toolbar, textvariable=search_var).pack(side=tk.LEFT, fill=tk.X, expand=True)
        ttk.Combobox(toolbar, textvariable=type_var, values=("all", "left_only", "right_only", "different"), state="readonly", width=14).pack(side=tk.LEFT, padx=(8, 0))
        ttk.Combobox(toolbar, textvariable=side_var, values=("all", "links", "rechts", "beide"), state="readonly", width=12).pack(side=tk.LEFT, padx=(8, 0))

        info_var = tk.StringVar(value=f"{len(result.diffs)} Einträge")
        ttk.Label(toolbar, textvariable=info_var, justify=tk.RIGHT).pack(side=tk.RIGHT, padx=(8, 0))

        tree = ttk.Treeview(
            top,
            columns=("type", "side", "path"),
            show="headings",
            selectmode="browse",
            height=20,
        )
        tree.heading("type", text="Diff-Typ")
        tree.heading("side", text="Seite")
        tree.heading("path", text="Relativer Pfad")
        tree.column("type", width=130, anchor="center")
        tree.column("side", width=100, anchor="center")
        tree.column("path", width=640, anchor="w")
        tree.tag_configure("left_only", background="#EFF6FF")
        tree.tag_configure("right_only", background="#FFF7ED")
        tree.tag_configure("different", background="#FEF3C7")
        tree.pack(fill=tk.BOTH, expand=True, padx=10, pady=(0, 10))

        def render_rows() -> None:
            tree.delete(*tree.get_children())
            needle = search_var.get().strip().lower()
            type_filter = type_var.get().strip().lower()
            side_filter = side_var.get().strip().lower()

            for entry in sorted(result.diffs, key=lambda e: (e.diff_type.value, str(e.relative_path))):
                side_text = "links" if entry.diff_type == DiffType.LEFT_ONLY else "rechts" if entry.diff_type == DiffType.RIGHT_ONLY else "beide"
                rel_text = str(entry.relative_path)
                typ_text = entry.diff_type.value.lower()

                if type_filter != "all" and typ_text != type_filter:
                    continue
                if side_filter != "all" and side_text != side_filter:
                    continue
                if needle and needle not in rel_text.lower() and needle not in typ_text and needle not in side_text:
                    continue

                tree.insert(
                    "",
                    tk.END,
                    values=(typ_text, side_text, rel_text),
                    tags=(typ_text,),
                )

            info_var.set(f"{len(tree.get_children())} Einträge sichtbar / {len(result.diffs)} gesamt")

        search_var.trace_add("write", lambda *_: render_rows())
        type_var.trace_add("write", lambda *_: render_rows())
        side_var.trace_add("write", lambda *_: render_rows())
        render_rows()

        def on_close() -> None:
            top.grab_release()
            top.destroy()

        top.protocol("WM_DELETE_WINDOW", on_close)

    def _toggle_result_overlay(self, pair_id: str) -> None:
        widget = self.sync_pair_widgets.get(pair_id)
        if widget is None:
            return
        overlay = widget.get("result_overlay")
        visible_var = widget.get("result_overlay_visible")
        button = widget.get("result_overlay_btn")
        if not isinstance(overlay, tk.Frame) or not isinstance(visible_var, tk.BooleanVar):
            return

        visible = not bool(visible_var.get())
        visible_var.set(visible)
        if visible:
            overlay.pack(fill=tk.BOTH, expand=True, padx=8, pady=(0, 8))
        else:
            overlay.pack_forget()

        if isinstance(button, ttk.Button):
            button.configure(text="Tabelle verbergen" if visible else "Tabelle anzeigen")

    def _set_optional_panel_visible(self, pair_id: str, panel: str, visible: bool) -> None:
        widget = self.sync_pair_widgets.get(pair_id)
        if widget is None:
            return

        visibility = widget.get("optional_visibility")
        if not isinstance(visibility, dict):
            visibility = {}
            widget["optional_visibility"] = visibility

        panel_widget = None
        if panel == "paths":
            panel_widget = widget.get("path_frame")
        elif panel == "filters":
            panel_widget = widget.get("control_row")
        elif panel == "actions":
            panel_widget = widget.get("action_row")
        elif panel == "result":
            panel_widget = widget.get("result_frame")

        overlay = widget.get("optional_overlay")
        overlay_body = widget.get("optional_overlay_body")
        overlay_title_var = widget.get("optional_overlay_title_var")
        frame = widget.get("frame")
        if panel_widget is None or not isinstance(overlay, tk.Frame) or not isinstance(overlay_body, ttk.Frame):
            return

        panel_titles = {
            "paths": "Ordnerauswahl",
            "filters": "Filter",
            "actions": "Aktionen",
            "result": "Sync-Ergebnis",
        }

        # Clear existing content from overlay body before showing a new optional panel.
        for p in ("paths", "filters", "actions", "result"):
            w = widget.get("path_frame") if p == "paths" else widget.get("control_row") if p == "filters" else widget.get("action_row") if p == "actions" else widget.get("result_frame")
            if w is not None:
                try:
                    w.pack_forget()
                except Exception:
                    pass

        if visible:
            if panel == "paths":
                panel_widget.pack(in_=overlay_body, fill=tk.X, pady=(0, 6))
            elif panel == "filters":
                panel_widget.pack(in_=overlay_body, fill=tk.X, pady=(0, 6))
            elif panel == "actions":
                panel_widget.pack(in_=overlay_body, fill=tk.X, pady=(0, 6))
            elif panel == "result":
                panel_widget.pack(in_=overlay_body, fill=tk.BOTH, expand=True, pady=(0, 6))

            if isinstance(overlay_title_var, tk.StringVar):
                overlay_title_var.set(panel_titles.get(panel, "Details"))
            if isinstance(frame, ttk.Frame):
                overlay.place(in_=frame, relx=0.02, rely=0.14, relwidth=0.96, relheight=0.82)
                overlay.lift()
            widget["active_optional_panel"] = panel
        else:
            overlay.place_forget()
            widget["active_optional_panel"] = ""
            if panel == "result":
                result_table_overlay = widget.get("result_overlay")
                overlay_flag = widget.get("result_overlay_visible")
                overlay_btn = widget.get("result_overlay_btn")
                if isinstance(result_table_overlay, tk.Frame):
                    result_table_overlay.pack_forget()
                if isinstance(overlay_flag, tk.BooleanVar):
                    overlay_flag.set(False)
                if isinstance(overlay_btn, ttk.Button):
                    overlay_btn.configure(text="Tabelle anzeigen")

        visibility[panel] = visible

        panel_btn_map = {
            "paths": "folder_panel_btn",
            "filters": "filter_panel_btn",
            "actions": "actions_panel_btn",
            "result": "result_panel_btn",
        }
        panel_titles = {
            "paths": "Ordner",
            "filters": "Filter",
            "actions": "Aktionen",
            "result": "Ergebnis",
        }
        panel_icons = {
            "paths": "📁",
            "filters": "🔎",
            "actions": "⚙",
            "result": "📊",
        }
        btn = widget.get(panel_btn_map.get(panel, ""))
        if isinstance(btn, ttk.Button):
            title = panel_titles.get(panel, panel)
            icon = panel_icons.get(panel, "")
            btn.configure(text=(f"✖ {title}" if visible else f"{icon} {title}".strip()))

    def _toggle_optional_panel(self, pair_id: str, panel: str) -> None:
        widget = self.sync_pair_widgets.get(pair_id)
        if widget is None:
            return
        if panel == "close":
            current_panel = str(widget.get("active_optional_panel") or "")
            if current_panel:
                self._set_optional_panel_visible(pair_id, current_panel, False)
            return
        visibility = widget.get("optional_visibility")
        if not isinstance(visibility, dict):
            visibility = {}
            widget["optional_visibility"] = visibility
        current_panel = str(widget.get("active_optional_panel") or "")
        if current_panel and current_panel != panel:
            self._set_optional_panel_visible(pair_id, current_panel, False)
        target_visible = not bool(visibility.get(panel, False)) if current_panel == panel else True
        self._set_optional_panel_visible(pair_id, panel, target_visible)

    def _render_snapshot(self, snapshot: SnapshotEvent) -> None:
        if snapshot.pair_id not in self.sync_pair_widgets:
            return

        self.controller.current_result = snapshot.result
        self.current_operation_pair_id = snapshot.pair_id
        self._render_result(snapshot.result)

    def _set_busy(self, busy: bool) -> None:
        if not busy:
            self._is_paused = False
        if busy:
            self._set_status("Operation laeuft...")
        self._update_action_button_states()

    def _update_action_button_states(self) -> None:
        is_running = self.controller.is_operation_running()

        for key in ("add_pair_btn", "remove_pair_btn", "sync_tab_btn", "sync_all_pairs_btn", "workflow_run_btn"):
            btn = getattr(self, key, None)
            if isinstance(btn, ttk.Button):
                btn.configure(state=tk.DISABLED if is_running else tk.NORMAL)

        if hasattr(self, "sync_mode_combo"):
            self.sync_mode_combo.configure(state="disabled" if is_running else "readonly")
        if hasattr(self, "max_parallel_spin"):
            self.max_parallel_spin.configure(state=tk.DISABLED if is_running else tk.NORMAL)

        active_pair_id = self.current_running_pair_id or self.current_operation_pair_id
        selected_pair = self._selected_pair()
        selected_workflow_ok = False
        if selected_pair is not None:
            selected_workflow_ok, _selected_states, _selected_error = self._validate_workflow_for_pair(selected_pair.pair_id)

        workflow_btn = getattr(self, "workflow_run_btn", None)
        if isinstance(workflow_btn, ttk.Button):
            workflow_btn.configure(state=tk.NORMAL if ((not is_running) and selected_pair is not None and selected_workflow_ok) else tk.DISABLED)

        for pair_id, widget in self.sync_pair_widgets.items():
            has_paths = bool(str(widget.get("left_var").get()).strip()) and bool(str(widget.get("right_var").get()).strip()) if isinstance(widget.get("left_var"), tk.StringVar) and isinstance(widget.get("right_var"), tk.StringVar) else False
            has_result = isinstance(widget.get("current_result"), CompareResult)
            is_active_pair = bool(active_pair_id and pair_id == active_pair_id)
            workflow_ok, _step_states, _workflow_error = self._validate_workflow_for_pair(pair_id)

            self._set_widget_button_state(widget.get("workflow_run_pair_btn"), (not is_running) and workflow_ok)
            self._set_widget_button_state(widget.get("pick_left_btn"), not is_running)
            self._set_widget_button_state(widget.get("pick_right_btn"), not is_running)
            self._set_widget_button_state(widget.get("compare_btn"), (not is_running) and has_paths)
            self._set_widget_button_state(widget.get("sync_btn"), (not is_running) and has_paths)
            self._set_widget_button_state(widget.get("copy_btn"), (not is_running) and has_result)
            self._set_widget_button_state(widget.get("zip_btn"), (not is_running) and has_result)

            pause_btn = widget.get("pause_btn")
            cancel_btn = widget.get("cancel_btn")
            toolbar_pause_btn = widget.get("workflow_pause_pair_btn")
            toolbar_cancel_btn = widget.get("workflow_cancel_pair_btn")

            self._set_widget_button_state(pause_btn, is_running and is_active_pair)
            self._set_widget_button_state(cancel_btn, is_running and is_active_pair)
            self._set_widget_button_state(toolbar_pause_btn, is_running and is_active_pair)
            self._set_widget_button_state(toolbar_cancel_btn, is_running and is_active_pair)

            pause_text = "⏵ Fortsetzen" if self._is_paused and is_active_pair else "⏸ Pause"
            if isinstance(pause_btn, ttk.Button):
                pause_btn.configure(text=pause_text)
            if isinstance(toolbar_pause_btn, ttk.Button):
                toolbar_pause_btn.configure(text=pause_text)

            # Table utilities are safe during runtime and can stay enabled.
            self._set_widget_button_state(widget.get("clear_filter_btn"), True)
            self._set_widget_button_state(widget.get("apply_filter_btn"), True)
            self._set_widget_button_state(widget.get("clear_selection_btn"), True)
            self._set_widget_button_state(widget.get("select_visible_btn"), True)

    @staticmethod
    def _set_widget_button_state(widget, enabled: bool) -> None:
        if isinstance(widget, ttk.Button):
            widget.configure(state=tk.NORMAL if enabled else tk.DISABLED)

    def _pair_by_id(self, pair_id: str) -> Optional[SyncPair]:
        for pair in self.sync_pairs:
            if pair.pair_id == pair_id:
                return pair
        return None

    def _append_log(self, text: str, pair_id: Optional[str] = None) -> None:
        line = str(text)
        self.global_log_lines.append(line)
        if len(self.global_log_lines) > 5000:
            self.global_log_lines = self.global_log_lines[-5000:]

        storage = self.settings_store
        if isinstance(storage, SQLiteAppStore):
            target_pair_id = pair_id or self.current_running_pair_id or self.current_operation_pair_id
            if target_pair_id:
                pair = self._pair_by_id(target_pair_id)
                if pair is not None:
                    try:
                        storage.append_pair_log(pair, line)
                    except Exception:
                        pass
                    else:
                        log_text = self.dashboard_log_text
                        if isinstance(log_text, tk.Text):
                            log_text.configure(state="normal")
                            log_text.insert(tk.END, line + "\n")
                            log_text.see(tk.END)
                            log_text.configure(state="disabled")
                        return
            try:
                storage.append_log(line)
            except Exception:
                pass

        log_text = self.dashboard_log_text
        if not isinstance(log_text, tk.Text):
            return
        log_text.configure(state="normal")
        log_text.insert(tk.END, line + "\n")
        log_text.see(tk.END)
        log_text.configure(state="disabled")

    def _set_status(self, text: str) -> None:
        self.status_var.set(text)

    def _recompute_overall_progress(self) -> None:
        batch_pair_ids = [pair_id for pair_id in self._overall_batch_pairs if pair_id in self.sync_pair_widgets]
        if not batch_pair_ids:
            self.progress_var.set(0.0)
            self.progress_text_var.set("0%")
            return

        total = 0.0
        for pair_id in batch_pair_ids:
            pair = self._pair_by_id(pair_id)
            state = (pair.state if pair is not None else "idle").strip().lower()
            widget = self.sync_pair_widgets.get(pair_id)
            progress_var = widget.get("progress_var") if isinstance(widget, dict) else None
            pct = 0.0
            if state == "done":
                pct = 100.0
            elif isinstance(progress_var, tk.DoubleVar):
                try:
                    pct = float(progress_var.get())
                except Exception:
                    pct = 0.0
            total += max(0.0, min(100.0, pct))

        overall = total / max(1, len(batch_pair_ids))
        self.progress_var.set(overall)
        self.progress_text_var.set(f"{overall:.0f}%")

    def _apply_progress(self, progress: ProgressEvent) -> None:
        pair_id = self._pair_id_from_progress(progress.current_folder)
        if pair_id is None:
            # Single-tab operations often emit plain relative folders without pair prefix.
            pair_id = self.current_running_pair_id or self.current_operation_pair_id
        if pair_id and pair_id != self.current_running_pair_id:
            if self.current_running_pair_id is not None:
                self._set_pair_state(self.current_running_pair_id, "done")
            self.current_running_pair_id = pair_id
            self._set_pair_state(pair_id, "running")

        if progress.current_folder:
            self.current_folder_var.set(f"Ordner: {progress.current_folder}")

        if pair_id:
            self._apply_pair_progress(pair_id, progress)
            widget = self.sync_pair_widgets.get(pair_id)
            if widget is not None:
                bucket = int(progress.percent() // 10)
                last_bucket = widget.get("workflow_result_last_progress_bucket", -1)
                if bucket != last_bucket:
                    widget["workflow_result_last_progress_bucket"] = bucket
                    self._update_workflow_result_card(
                        pair_id,
                        title=f"{progress.phase}: {int(progress.percent())}%",
                        line=f"{progress.phase} {int(progress.percent())}% - {progress.current_folder}",
                    )

        if progress.eta_seconds is None:
            self.eta_var.set("ETA: -")
        else:
            eta = max(0, int(progress.eta_seconds))
            if eta < 60:
                self.eta_var.set(f"ETA: {eta}s")
            else:
                self.eta_var.set(f"ETA: {eta // 60}m {eta % 60}s")
        self._recompute_overall_progress()

    def _apply_pair_progress(self, pair_id: str, progress: ProgressEvent) -> None:
        widget_info = self.sync_pair_widgets.get(pair_id)
        if widget_info is None:
            return

        pct = progress.percent()
        progress_var = widget_info.get("progress_var")
        progress_text_var = widget_info.get("progress_text_var")
        folder_var = widget_info.get("folder_var")
        eta_var = widget_info.get("eta_var")
        phase_var = widget_info.get("workflow_phase_var")

        if isinstance(progress_var, tk.DoubleVar):
            progress_var.set(pct)
        if isinstance(progress_text_var, tk.StringVar):
            progress_text_var.set(f"{pct:.0f}%")
        if isinstance(phase_var, tk.StringVar):
            phase = (progress.phase or "Workflow").strip()
            phase_var.set(f"{phase}")
        if isinstance(folder_var, tk.StringVar):
            folder_var.set(f"Ordner: {progress.current_folder}")
        if isinstance(eta_var, tk.StringVar):
            if progress.eta_seconds is None:
                eta_var.set("ETA: -")
            else:
                eta = max(0, int(progress.eta_seconds))
                if eta < 60:
                    eta_var.set(f"ETA: {eta}s")
                else:
                    eta_var.set(f"ETA: {eta // 60}m {eta % 60}s")

    def _on_table_column_filter_changed(self, pair_id: str, key: str) -> None:
        widget = self.sync_pair_widgets.get(pair_id)
        if widget is None:
            return
        column_search_vars = widget.get("column_search_vars") or {}
        var = column_search_vars.get(key)
        if not isinstance(var, tk.StringVar):
            return
        self._refresh_visible_rows(pair_id)

    def _pick_left(self) -> None:
        pair = self._selected_pair()
        if pair is None:
            messagebox.showerror("Fehler", "Bitte zuerst ein Sync-Tab waehlen.")
            return
        self._pick_left_for_pair(pair)

    def _pick_left_for_pair(self, pair: SyncPair) -> None:
        widget = self.sync_pair_widgets.get(pair.pair_id)
        if widget is None:
            return
        left_var = widget.get("left_var")
        if not isinstance(left_var, tk.StringVar):
            return

        initial = left_var.get().strip() or self.settings.get("left_path", "")
        selected = filedialog.askdirectory(title="Linken Ordner waehlen", initialdir=initial or None)
        if selected:
            left_var.set(selected)
            pair.left = selected
            self.settings["left_path"] = selected
            self._set_status(f"Linker Ordner gesetzt: {selected}")
            self._update_action_button_states()

    def _pick_right(self) -> None:
        pair = self._selected_pair()
        if pair is None:
            messagebox.showerror("Fehler", "Bitte zuerst ein Sync-Tab waehlen.")
            return
        self._pick_right_for_pair(pair)

    def _pick_right_for_pair(self, pair: SyncPair) -> None:
        widget = self.sync_pair_widgets.get(pair.pair_id)
        if widget is None:
            return
        right_var = widget.get("right_var")
        if not isinstance(right_var, tk.StringVar):
            return

        initial = right_var.get().strip() or self.settings.get("right_path", "")
        selected = filedialog.askdirectory(title="Rechten Ordner waehlen", initialdir=initial or None)
        if selected:
            right_var.set(selected)
            pair.right = selected
            self.settings["right_path"] = selected
            self._set_status(f"Rechter Ordner gesetzt: {selected}")
            self._update_action_button_states()

    def _on_compare(self) -> None:
        pair = self._selected_pair()
        if pair is None:
            messagebox.showerror("Fehler", "Bitte zuerst ein Sync-Tab waehlen.")
            return
        self._on_compare_for_pair(pair)

    def _on_compare_for_pair(self, pair: SyncPair) -> None:
        self._sync_pair_model_from_widgets(pair)
        self._set_status("Vergleiche Ordner...")
        self.current_operation_pair_id = pair.pair_id
        self._run_ui_action(lambda: self.controller.compare(pair.left, pair.right))

    def _on_sync(self) -> None:
        pair = self._selected_pair()
        if pair is None:
            messagebox.showerror("Fehler", "Bitte zuerst ein Sync-Tab waehlen.")
            return
        self._on_sync_for_pair(pair)

    def _on_sync_for_pair(self, pair: SyncPair) -> None:
        self._sync_pair_model_from_widgets(pair)
        if not self._confirm(
            "Synchronisieren bestaetigen",
            "Rechter Ordner wird an linken Ordner angeglichen. Fortfahren?",
        ):
            return
        self._set_status("Synchronisiere...")
        self.current_operation_pair_id = pair.pair_id
        self.scheduler.start_live_snapshot_refresh([pair])
        self._run_ui_action(self.controller.synchronize)

    def _on_pause_resume(self) -> None:
        if not self.controller.is_operation_running():
            return

        try:
            if self._is_paused:
                self.controller.resume_active_operation()
                self._is_paused = False
                self._set_status("Operation fortgesetzt")
            else:
                self.controller.pause_active_operation()
                self._is_paused = True
                self._set_status("Operation unterbrochen")
            self._update_action_button_states()
        except Exception as exc:  # noqa: BLE001
            messagebox.showerror("Fehler", str(exc))

    def _on_pause_resume_for_pair(self, pair: SyncPair) -> None:
        self.current_operation_pair_id = pair.pair_id
        self._on_pause_resume()

    def _on_cancel(self) -> None:
        if not self.controller.is_operation_running():
            return
        if not self._confirm("Abbruch bestaetigen", "Aktive Operation wirklich abbrechen?"):
            return
        try:
            pair_id = self.current_running_pair_id or self.current_operation_pair_id
            if pair_id is not None:
                self._set_pair_state(pair_id, "cancelled")
            self.controller.cancel_active_operation()
            self._set_status("Abbruch angefordert...")
        except Exception as exc:  # noqa: BLE001
            messagebox.showerror("Fehler", str(exc))

    def _on_cancel_for_pair(self, pair: SyncPair) -> None:
        self.current_operation_pair_id = pair.pair_id
        self._on_cancel()

    def _on_copy_diff(self) -> None:
        if self._selected_pair() is None:
            messagebox.showerror("Fehler", "Bitte zuerst ein Sync-Tab waehlen.")
            return
        self._on_copy_diff_for_pair(self._selected_pair())

    def _on_copy_diff_for_pair(self, pair: SyncPair) -> None:
        self.current_operation_pair_id = pair.pair_id
        initial = self.settings.get("last_copy_target", "")
        target = filedialog.askdirectory(
            title="Zielordner fuer Differenzkopie",
            initialdir=initial or None,
        )
        if not target:
            return
        self.settings["last_copy_target"] = target
        self._set_status("Kopiere Differenzen...")
        self._run_ui_action(lambda: self.controller.copy_differences(target))

    def _on_zip_diff(self) -> None:
        if self._selected_pair() is None:
            messagebox.showerror("Fehler", "Bitte zuerst ein Sync-Tab waehlen.")
            return
        self._on_zip_diff_for_pair(self._selected_pair())

    def _on_zip_diff_for_pair(self, pair: SyncPair) -> None:
        self.current_operation_pair_id = pair.pair_id
        initial_dir = self.settings.get("last_zip_dir", "")
        initial_name = self.settings.get("last_zip_name", "differences.zip")
        zip_file = filedialog.asksaveasfilename(
            title="Zip-Datei speichern",
            defaultextension=".zip",
            filetypes=[("Zip Archive", "*.zip")],
            initialdir=initial_dir or None,
            initialfile=initial_name,
        )
        if not zip_file:
            return
        zip_path = Path(zip_file)
        self.settings["last_zip_dir"] = str(zip_path.parent)
        self.settings["last_zip_name"] = zip_path.name
        self._set_status("Komprimiere Differenzen...")
        self._run_ui_action(lambda: self.controller.compress_differences(zip_file))

    def _show_about(self) -> None:
        messagebox.showinfo(
            "Ueber Folder Compare Tool",
            "Folder Compare Tool\n\n"
            "- Ordnervergleich\n"
            "- Synchronisieren\n"
            "- Differenzen kopieren oder ZIP erstellen\n"
            "- Threading + Queue fuer responsive GUI\n"
            "- Multi-Ordner-Sync mit Pause/Fortsetzen/Abbrechen",
        )

    def _load_sync_pairs_from_settings(self) -> List[SyncPair]:
        return self.state_repository.load_sync_pairs(self.settings)

    def _load_workflow_steps_from_settings(self) -> Dict[str, List[WorkflowStep]]:
        workflows = self.state_repository.load_workflow_steps(self.settings)
        for pair in self.sync_pairs:
            workflows.setdefault(pair.pair_id, self._default_workflow_steps(pair))
        return workflows

    def _default_workflow_steps(self, pair: SyncPair) -> List[WorkflowStep]:
        return [
            WorkflowStep(step_id=str(uuid.uuid4()), action="start_meta", title="Start"),
            WorkflowStep(
                step_id=str(uuid.uuid4()),
                action="select_paths",
                title="Ordner A/B",
                config={"folder_a": pair.left, "folder_b": pair.right},
            ),
            WorkflowStep(step_id=str(uuid.uuid4()), action="compare", title="Check"),
            WorkflowStep(step_id=str(uuid.uuid4()), action="sync_left", title="Sync"),
            WorkflowStep(step_id=str(uuid.uuid4()), action="finish", title="Finish"),
        ]

    def _load_persistent_logs(self) -> List[str]:
        logs: List[str] = []
        storage = self.settings_store
        if isinstance(storage, SQLiteAppStore):
            try:
                logs.extend(storage.load_recent_log_messages())
            except Exception:
                pass
            for pair in self.sync_pairs:
                try:
                    pair_logs = storage.load_recent_pair_log_messages(pair)
                except Exception:
                    continue
                if pair_logs:
                    logs.append(f"[{pair.name}] Historie geladen")
                    logs.extend(pair_logs)
        return logs

    def _refresh_sqlite_status(self) -> None:
        storage = self.settings_store
        if isinstance(storage, SQLiteAppStore):
            try:
                app_exists = storage.app_db_exists()
                app_health = storage.app_db_health()
                self.sqlite_app_var.set(
                    f"SQLite-App: {'vorhanden' if app_exists else 'fehlt'} | {app_health} | {storage.db_path.name}"
                )
            except Exception as exc:
                self.sqlite_app_var.set(f"SQLite-App: error | {exc}")

            pair = self._selected_pair() or (self.sync_pairs[0] if self.sync_pairs else None)
            if pair is not None:
                try:
                    pair_exists = storage.pair_db_exists(pair)
                    pair_health = storage.pair_db_health(pair) if pair_exists else "nicht angelegt"
                    pair_path = storage.pair_db_path(pair)
                    pair_text = f"SQLite-Paar: {pair.name} | {'vorhanden' if pair_exists else 'fehlt'} | {pair_health} | {pair_path.name}"
                    self.sqlite_pair_var.set(pair_text)
                    widget = self.sync_pair_widgets.get(pair.pair_id)
                    if widget is not None:
                        sqlite_var = widget.get("sqlite_status_var")
                        if isinstance(sqlite_var, tk.StringVar):
                            sqlite_var.set(pair_text)
                except Exception as exc:
                    self.sqlite_pair_var.set(f"SQLite-Paar: error | {exc}")
            else:
                self.sqlite_pair_var.set("SQLite-Paar: -")
        else:
            self.sqlite_app_var.set("SQLite-App: nicht verfügbar")
            self.sqlite_pair_var.set("SQLite-Paar: nicht verfügbar")

    def _save_sync_pairs_to_settings(self) -> None:
        self.state_repository.save_sync_pairs(self.settings, self.sync_pairs)
        self.settings["sync_pairs_mode"] = self.sync_mode_var.get()
        self.settings["sync_pairs_max_parallel"] = str(self.max_parallel_var.get())
        self.state_repository.save_job_states(self.settings, self.job_states)
        self.state_repository.save_workflow_steps(self.settings, self.workflow_steps_by_pair)
        self.dashboard_mode_var.set(self.sync_mode_var.get())
        self.dashboard_workers_var.set(str(self.max_parallel_var.get()))

    def _load_job_states_from_settings(self) -> None:
        self.job_states = self.state_repository.load_job_states(self.settings)

    def _save_job_states_to_settings(self) -> None:
        self.state_repository.save_job_states(self.settings, self.job_states)

    def _workflow_steps_for_pair(self, pair_id: str) -> List[WorkflowStep]:
        steps = self.workflow_steps_by_pair.get(pair_id)
        if steps:
            self._normalize_workflow_steps(pair_id)
            return self.workflow_steps_by_pair[pair_id]

        pair = self._pair_by_id(pair_id)
        if pair is None:
            steps = [WorkflowStep(step_id=str(uuid.uuid4()), action="compare", title="Check")]
        else:
            steps = self._default_workflow_steps(pair)
        self.workflow_steps_by_pair[pair_id] = steps
        self._normalize_workflow_steps(pair_id)
        return self.workflow_steps_by_pair[pair_id]

    @staticmethod
    def _is_start_step(step: WorkflowStep) -> bool:
        return str(step.action).strip().lower() == "start_meta"

    @staticmethod
    def _is_finish_step(step: WorkflowStep) -> bool:
        return str(step.action).strip().lower() == "finish"

    def _is_fixed_workflow_step(self, step: WorkflowStep) -> bool:
        return self._is_start_step(step) or self._is_finish_step(step)

    def _normalize_workflow_steps(self, pair_id: str) -> None:
        steps = self.workflow_steps_by_pair.get(pair_id) or []

        start_steps = [step for step in steps if self._is_start_step(step)]
        finish_steps = [step for step in steps if self._is_finish_step(step)]
        middle_steps = [step for step in steps if not self._is_start_step(step) and not self._is_finish_step(step)]

        if start_steps:
            start = start_steps[0]
            start.action = "start_meta"
            start.title = self._workflow_step_title("start_meta")
            start.enabled = True
        else:
            start = WorkflowStep(
                step_id=str(uuid.uuid4()),
                action="start_meta",
                title=self._workflow_step_title("start_meta"),
                enabled=True,
            )

        if finish_steps:
            finish = finish_steps[-1]
            finish.action = "finish"
            finish.title = self._workflow_step_title("finish")
            finish.enabled = True
            finish.config.clear()
        else:
            finish = WorkflowStep(
                step_id=str(uuid.uuid4()),
                action="finish",
                title=self._workflow_step_title("finish"),
                enabled=True,
            )

        select_step = None
        compare_step = None
        sync_step = None

        for step in middle_steps:
            action = str(step.action).strip().lower()
            if action == "select_paths":
                step.action = "select_paths"
                step.title = self._workflow_step_title("select_paths")
                select_step = step
            elif action == "compare":
                step.action = "compare"
                step.title = self._workflow_step_title("compare")
                compare_step = step
            elif action == "sync_left":
                step.action = "sync_left"
                step.title = self._workflow_step_title("sync_left")
                sync_step = step

        ordered_middle: List[WorkflowStep] = []
        if select_step is not None:
            ordered_middle.append(select_step)
        elif not ordered_middle:
            ordered_middle.append(WorkflowStep(step_id=str(uuid.uuid4()), action="select_paths", title=self._workflow_step_title("select_paths"), config={}))

        if compare_step is not None:
            ordered_middle.append(compare_step)
        else:
            ordered_middle.append(WorkflowStep(step_id=str(uuid.uuid4()), action="compare", title=self._workflow_step_title("compare")))

        if sync_step is not None:
            ordered_middle.append(sync_step)
        else:
            ordered_middle.append(WorkflowStep(step_id=str(uuid.uuid4()), action="sync_left", title=self._workflow_step_title("sync_left")))

        self.workflow_steps_by_pair[pair_id] = [start, *ordered_middle, finish]

    def _workflow_step_title(self, action: str) -> str:
        titles = {
            "start_meta": "Start",
            "select_paths": "Ordner A/B",
            "compare": "Check",
            "sync_left": "Sync left",
            "copy_diff": "Zip Diff",
            "zip_diff": "Zip Export",
            "filter_ops": "Filter-Card",
            "finish": "Finish",
        }
        return titles.get(action.strip().lower(), action.strip().title() or "Schritt")

    def _workflow_step_summary(self, step: WorkflowStep) -> str:
        action = step.action.strip().lower()
        dry_run = str(step.config.get("dry_run", "")).strip().lower() in {"1", "true", "yes", "ja", "on"}
        dry_prefix = "Dry-run | " if dry_run else ""
        if action == "start_meta":
            run_label = str(step.config.get("run_label", "")).strip()
            run_note = str(step.config.get("run_note", "")).strip()
            parts: List[str] = []
            if run_label:
                parts.append(f"Label: {run_label}")
            if run_note:
                parts.append(f"Notiz: {run_note}")
            return " | ".join(parts) if parts else "Metainformationen"
        if action == "select_paths":
            folder_a = str(step.config.get("folder_a", "")).strip()
            folder_b = str(step.config.get("folder_b", "")).strip()
            if folder_a or folder_b:
                return f"{dry_prefix}A: {folder_a or '-'} | B: {folder_b or '-'}"
            return f"{dry_prefix}Ordner A/B nicht gesetzt"
        if action == "copy_diff":
            return f"{dry_prefix}Ziel: {step.config.get('target', '-')}"
        if action == "zip_diff":
            return f"{dry_prefix}Zip: {step.config.get('zip_file', '-')}"
        if action == "sync_left":
            return f"{dry_prefix}Mirror left -> right"
        if action == "compare":
            return f"{dry_prefix}Nur prüfen"
        if action == "filter_ops":
            parts: List[str] = []
            min_size = str(step.config.get("min_size_bytes", "")).strip()
            max_size = str(step.config.get("max_size_bytes", "")).strip()
            date_from = str(step.config.get("date_from", "")).strip()
            date_to = str(step.config.get("date_to", "")).strip()
            include_ext = str(step.config.get("include_extensions", "")).strip()
            exclude_ext = str(step.config.get("exclude_extensions", "")).strip()
            exclude_folders = str(step.config.get("exclude_folders", "")).strip()
            include_regex = str(step.config.get("include_regex", "")).strip()
            exclude_regex = str(step.config.get("exclude_regex", "")).strip()
            include_regex_icase = str(step.config.get("include_regex_icase", "")).strip().lower() in {"1", "true", "yes", "ja", "on"}
            exclude_regex_icase = str(step.config.get("exclude_regex_icase", "")).strip().lower() in {"1", "true", "yes", "ja", "on"}
            if min_size or max_size:
                parts.append(f"Size {min_size or '-'}..{max_size or '-'}")
            if date_from or date_to:
                parts.append(f"Datum {date_from or '-'}..{date_to or '-'}")
            if include_ext:
                parts.append(f"Ext+ {include_ext}")
            if exclude_ext:
                parts.append(f"Ext- {exclude_ext}")
            if exclude_folders:
                parts.append(f"Ordner- {exclude_folders}")
            if include_regex:
                parts.append(f"Re+ {include_regex}{' (i)' if include_regex_icase else ''}")
            if exclude_regex:
                parts.append(f"Re- {exclude_regex}{' (i)' if exclude_regex_icase else ''}")
            if str(step.config.get("unzip_zips", "")).strip().lower() in {"1", "true", "yes", "ja", "on"}:
                parts.append("ZIP entpacken")
            if str(step.config.get("index_entries", "")).strip().lower() in {"1", "true", "yes", "ja", "on"}:
                parts.append("Indizieren")
            return f"{dry_prefix}{' | '.join(parts) if parts else 'Filter ohne Regeln'}"
        if action == "finish":
            return f"{dry_prefix}Plan beenden"
        return ""

    @staticmethod
    def _parse_int_optional(value: str) -> Optional[int]:
        text = (value or "").strip()
        if not text:
            return None
        return int(text)

    @staticmethod
    def _parse_iso_datetime_optional(value: str) -> Optional[datetime.datetime]:
        text = (value or "").strip()
        if not text:
            return None
        return datetime.datetime.fromisoformat(text)

    def _validate_workflow_for_pair(self, pair_id: str) -> Tuple[bool, Dict[str, Tuple[bool, str]], str]:
        pair = self._pair_by_id(pair_id)
        steps = self._workflow_steps_for_pair(pair_id)

        active_left = (pair.left if pair is not None else "").strip()
        active_right = (pair.right if pair is not None else "").strip()
        step_states: Dict[str, Tuple[bool, str]] = {}
        errors: List[str] = []

        def _paths_valid(left_text: str, right_text: str) -> Tuple[bool, str]:
            if not left_text or not right_text:
                return False, "Ordner A/B fehlen"
            left_path = Path(left_text)
            right_path = Path(right_text)
            if not left_path.exists() or not left_path.is_dir():
                return False, f"Ordner A ungueltig: {left_path}"
            if not right_path.exists() or not right_path.is_dir():
                return False, f"Ordner B ungueltig: {right_path}"
            return True, ""

        for index, step in enumerate(steps, start=1):
            action = str(step.action).strip().lower()

            if not step.enabled and not self._is_fixed_workflow_step(step):
                step_states[step.step_id] = (True, "deaktiviert")
                continue

            valid = True
            reason = "OK"

            try:
                if action == "start_meta":
                    valid, reason = True, "OK"
                elif action == "select_paths":
                    folder_a = str(step.config.get("folder_a", "")).strip()
                    folder_b = str(step.config.get("folder_b", "")).strip()
                    valid, reason = _paths_valid(folder_a, folder_b)
                    if valid:
                        active_left = folder_a
                        active_right = folder_b
                elif action in {"compare", "sync_left", "filter_ops", "copy_diff", "zip_diff"}:
                    valid, reason = _paths_valid(active_left, active_right)
                    if valid and action == "copy_diff":
                        target = str(step.config.get("target", "")).strip()
                        if not target:
                            valid, reason = False, "Copy-Ziel fehlt"
                    if valid and action == "zip_diff":
                        zip_file = str(step.config.get("zip_file", "")).strip()
                        if not zip_file:
                            valid, reason = False, "ZIP-Datei fehlt"
                    if valid and action == "filter_ops":
                        min_size_text = str(step.config.get("min_size_bytes", ""))
                        max_size_text = str(step.config.get("max_size_bytes", ""))
                        include_regex = str(step.config.get("include_regex", "")).strip()
                        exclude_regex = str(step.config.get("exclude_regex", "")).strip()
                        date_from_text = str(step.config.get("date_from", ""))
                        date_to_text = str(step.config.get("date_to", ""))

                        min_size = self._parse_int_optional(min_size_text)
                        max_size = self._parse_int_optional(max_size_text)
                        if min_size is not None and min_size < 0:
                            valid, reason = False, "min_size_bytes < 0"
                        if valid and max_size is not None and max_size < 0:
                            valid, reason = False, "max_size_bytes < 0"
                        if valid and min_size is not None and max_size is not None and min_size > max_size:
                            valid, reason = False, "min_size_bytes > max_size_bytes"
                        if valid and include_regex:
                            re.compile(include_regex)
                        if valid and exclude_regex:
                            re.compile(exclude_regex)
                        if valid:
                            date_from = self._parse_iso_datetime_optional(date_from_text)
                            date_to = self._parse_iso_datetime_optional(date_to_text)
                            if date_from is not None and date_to is not None and date_from > date_to:
                                valid, reason = False, "date_from > date_to"
                elif action == "finish":
                    valid, reason = True, "OK"
                else:
                    valid, reason = False, f"Unbekannte Aktion: {step.action}"
            except Exception as exc:
                valid = False
                reason = str(exc) or "ungueltige Konfiguration"

            step_states[step.step_id] = (valid, reason)
            if not valid:
                errors.append(f"[{index}] {step.title}: {reason}")

        overall_ok = len(errors) == 0
        return overall_ok, step_states, " | ".join(errors)

    def _save_workflow_steps_to_settings(self) -> None:
        self.state_repository.save_workflow_steps(self.settings, self.workflow_steps_by_pair)

    def _add_workflow_step(self, pair_id: str, action: str = "compare", insert_index: Optional[int] = None) -> None:
        action = str(action).strip().lower()
        if action in {"start_meta", "finish"}:
            self._set_status("Start/Finish sind fixe System-Cards und werden automatisch verwaltet.")
            return
        steps = self._workflow_steps_for_pair(pair_id)
        step = WorkflowStep(
            step_id=str(uuid.uuid4()),
            action=action,
            title=self._workflow_step_title(action),
        )
        start_index = next((idx for idx, item in enumerate(steps) if self._is_start_step(item)), -1)
        finish_index = next((idx for idx, item in enumerate(steps) if self._is_finish_step(item)), len(steps))
        min_insert = start_index + 1
        if insert_index is None:
            steps.insert(finish_index, step)
        else:
            safe_index = max(min_insert, min(int(insert_index), finish_index))
            steps.insert(safe_index, step)
        self._normalize_workflow_steps(pair_id)
        self._save_workflow_steps_to_settings()
        self._render_workflow_steps(pair_id)

    def _remove_workflow_step(self, pair_id: str, step_id: str) -> None:
        steps = self._workflow_steps_for_pair(pair_id)
        target = next((step for step in steps if step.step_id == step_id), None)
        if target is not None and self._is_fixed_workflow_step(target):
            self._set_status("Start/Finish sind fixe System-Cards und koennen nicht geloescht werden.")
            return
        remaining = [step for step in steps if step.step_id != step_id]
        if not remaining:
            remaining = self._default_workflow_steps(self._pair_by_id(pair_id) or SyncPair(pair_id=pair_id, name="Sync", left="", right=""))
        self.workflow_steps_by_pair[pair_id] = remaining
        self._normalize_workflow_steps(pair_id)
        self._save_workflow_steps_to_settings()
        self._render_workflow_steps(pair_id)

    def _duplicate_workflow_step(self, pair_id: str, step_id: str) -> None:
        steps = self._workflow_steps_for_pair(pair_id)
        for index, step in enumerate(steps):
            if step.step_id == step_id:
                if self._is_fixed_workflow_step(step):
                    self._set_status("Start/Finish sind fixe System-Cards und koennen nicht dupliziert werden.")
                    return
                duplicate = WorkflowStep(
                    step_id=str(uuid.uuid4()),
                    action=step.action,
                    title=f"{step.title} Kopie",
                    enabled=step.enabled,
                    config=dict(step.config),
                )
                steps.insert(index + 1, duplicate)
                self._normalize_workflow_steps(pair_id)
                self._save_workflow_steps_to_settings()
                self._render_workflow_steps(pair_id)
                return

    def _set_workflow_step_action(self, pair_id: str, step_id: str, action: str) -> None:
        steps = self._workflow_steps_for_pair(pair_id)
        for step in steps:
            if step.step_id == step_id:
                if self._is_fixed_workflow_step(step):
                    self._set_status("Start/Finish bleiben fixe System-Cards.")
                    return
                step.action = action
                step.title = self._workflow_step_title(action)
                if action not in {"copy_diff", "zip_diff", "filter_ops", "select_paths", "start_meta"}:
                    step.config.clear()
                break
        self._normalize_workflow_steps(pair_id)
        self._save_workflow_steps_to_settings()
        self._render_workflow_steps(pair_id)

    def _configure_workflow_step(self, pair_id: str, step_id: str) -> None:
        steps = self._workflow_steps_for_pair(pair_id)
        pair = self._pair_by_id(pair_id)
        step = next((item for item in steps if item.step_id == step_id), None)
        if step is None:
            return

        action = step.action.strip().lower()
        if action == "start_meta":
            run_label = simpledialog.askstring(
                "Start-Card konfigurieren",
                "Label (optional):",
                initialvalue=str(step.config.get("run_label", "")).strip(),
            )
            if run_label is None:
                return
            run_note = simpledialog.askstring(
                "Start-Card konfigurieren",
                "Notiz (optional):",
                initialvalue=str(step.config.get("run_note", "")).strip(),
            )
            if run_note is None:
                return
            step.config["run_label"] = run_label.strip()
            step.config["run_note"] = run_note.strip()
            self._save_workflow_steps_to_settings()
            self._render_workflow_steps(pair_id)
            return

        if action == "select_paths":
            folder_a = filedialog.askdirectory(
                title="Ordner A fuer Workflow-Schritt",
                initialdir=step.config.get("folder_a", pair.left if pair is not None else "") or None,
            )
            if not folder_a:
                return
            folder_b = filedialog.askdirectory(
                title="Ordner B fuer Workflow-Schritt",
                initialdir=step.config.get("folder_b", pair.right if pair is not None else "") or None,
            )
            if not folder_b:
                return
            step.config["folder_a"] = folder_a
            step.config["folder_b"] = folder_b
            self.settings["left_path"] = folder_a
            self.settings["right_path"] = folder_b
        elif action == "copy_diff":
            initial = step.config.get("target", self.settings.get("last_copy_target", ""))
            target = filedialog.askdirectory(title="Zielordner fuer Workflow-Schritt", initialdir=initial or None)
            if not target:
                return
            step.config["target"] = target
            self.settings["last_copy_target"] = target
        elif action == "zip_diff":
            initial_dir = step.config.get("zip_file", self.settings.get("last_zip_dir", ""))
            initial_name = Path(step.config.get("zip_file", self.settings.get("last_zip_name", "differences.zip"))).name
            zip_file = filedialog.asksaveasfilename(
                title="Zip-Datei fuer Workflow-Schritt",
                defaultextension=".zip",
                filetypes=[("Zip Archive", "*.zip")],
                initialdir=str(Path(initial_dir).parent) if initial_dir else None,
                initialfile=initial_name,
            )
            if not zip_file:
                return
            step.config["zip_file"] = zip_file
            zip_path = Path(zip_file)
            self.settings["last_zip_dir"] = str(zip_path.parent)
            self.settings["last_zip_name"] = zip_path.name
        elif action == "filter_ops":
            min_size = simpledialog.askstring(
                "Filter-Card konfigurieren",
                "Min. Dateigroesse in Bytes (leer = kein Limit):",
                initialvalue=step.config.get("min_size_bytes", ""),
            )
            if min_size is None:
                return
            max_size = simpledialog.askstring(
                "Filter-Card konfigurieren",
                "Max. Dateigroesse in Bytes (leer = kein Limit):",
                initialvalue=step.config.get("max_size_bytes", ""),
            )
            if max_size is None:
                return
            date_from = simpledialog.askstring(
                "Filter-Card konfigurieren",
                "Datum von (YYYY-MM-DD, leer = offen):",
                initialvalue=step.config.get("date_from", ""),
            )
            if date_from is None:
                return
            date_to = simpledialog.askstring(
                "Filter-Card konfigurieren",
                "Datum bis (YYYY-MM-DD, leer = offen):",
                initialvalue=step.config.get("date_to", ""),
            )
            if date_to is None:
                return
            include_ext = simpledialog.askstring(
                "Filter-Card konfigurieren",
                "Nur Dateiendungen (CSV, z.B. .jpg,.png oder leer):",
                initialvalue=step.config.get("include_extensions", ""),
            )
            if include_ext is None:
                return
            exclude_ext = simpledialog.askstring(
                "Filter-Card konfigurieren",
                "Dateiendungen ausschliessen (CSV, z.B. .tmp,.bak oder leer):",
                initialvalue=step.config.get("exclude_extensions", ""),
            )
            if exclude_ext is None:
                return
            exclude_folders = simpledialog.askstring(
                "Filter-Card konfigurieren",
                "Ordner ausschliessen (CSV nach Ordnername, z.B. node_modules,.git):",
                initialvalue=step.config.get("exclude_folders", ""),
            )
            if exclude_folders is None:
                return
            include_regex = simpledialog.askstring(
                "Filter-Card konfigurieren",
                "Include Regex (relativer Pfad, leer = aus):",
                initialvalue=step.config.get("include_regex", ""),
            )
            if include_regex is None:
                return
            exclude_regex = simpledialog.askstring(
                "Filter-Card konfigurieren",
                "Exclude Regex (relativer Pfad, leer = aus):",
                initialvalue=step.config.get("exclude_regex", ""),
            )
            if exclude_regex is None:
                return
            include_regex_icase = messagebox.askyesno(
                "Filter-Card konfigurieren",
                "Include Regex mit Ignore-Case anwenden?",
                parent=self,
            )
            exclude_regex_icase = messagebox.askyesno(
                "Filter-Card konfigurieren",
                "Exclude Regex mit Ignore-Case anwenden?",
                parent=self,
            )
            unzip_flag = messagebox.askyesno(
                "Filter-Card konfigurieren",
                "ZIP-Dateien aus dem gefilterten Ergebnis entpacken?",
                parent=self,
            )
            unzip_target = ""
            if unzip_flag:
                unzip_target = filedialog.askdirectory(
                    title="Zielordner fuer ZIP-Entpacken",
                    initialdir=step.config.get("unzip_target", "") or None,
                )
                if not unzip_target:
                    return

            index_flag = messagebox.askyesno(
                "Filter-Card konfigurieren",
                "Gefilterte Treffer indizieren (JSON-Datei schreiben)?",
                parent=self,
            )
            index_file = ""
            if index_flag:
                index_file = filedialog.asksaveasfilename(
                    title="Index-Datei speichern",
                    defaultextension=".json",
                    filetypes=[("JSON", "*.json")],
                    initialfile=Path(step.config.get("index_file", "filter_index.json")).name,
                )
                if not index_file:
                    return

            step.config["min_size_bytes"] = (min_size or "").strip()
            step.config["max_size_bytes"] = (max_size or "").strip()
            step.config["date_from"] = (date_from or "").strip()
            step.config["date_to"] = (date_to or "").strip()
            step.config["include_extensions"] = (include_ext or "").strip()
            step.config["exclude_extensions"] = (exclude_ext or "").strip()
            step.config["exclude_folders"] = (exclude_folders or "").strip()
            step.config["include_regex"] = (include_regex or "").strip()
            step.config["exclude_regex"] = (exclude_regex or "").strip()
            step.config["include_regex_icase"] = "true" if include_regex_icase else "false"
            step.config["exclude_regex_icase"] = "true" if exclude_regex_icase else "false"
            step.config["unzip_zips"] = "true" if unzip_flag else "false"
            step.config["unzip_target"] = unzip_target
            step.config["index_entries"] = "true" if index_flag else "false"
            step.config["index_file"] = index_file
        else:
            title = simpledialog.askstring(
                "Workflow-Schritt konfigurieren",
                "Titel fuer diesen Schritt:",
                initialvalue=step.title,
            )
            if title is not None:
                step.title = title.strip() or step.title

        self._save_workflow_steps_to_settings()
        self._render_workflow_steps(pair_id)

    def _set_workflow_step_path(self, pair_id: str, step_id: str, path_key: str) -> None:
        if path_key not in {"folder_a", "folder_b"}:
            return
        steps = self._workflow_steps_for_pair(pair_id)
        pair = self._pair_by_id(pair_id)
        step = next((item for item in steps if item.step_id == step_id), None)
        if step is None:
            return

        current = str(step.config.get(path_key, "")).strip()
        fallback = ""
        if pair is not None:
            fallback = pair.left if path_key == "folder_a" else pair.right
        selected = filedialog.askdirectory(
            title="Pfad A waehlen" if path_key == "folder_a" else "Pfad B waehlen",
            initialdir=current or fallback or None,
        )
        if not selected:
            return

        step.config[path_key] = selected
        if path_key == "folder_a":
            self.settings["left_path"] = selected
        else:
            self.settings["right_path"] = selected
        self._save_workflow_steps_to_settings()
        self._render_workflow_steps(pair_id)

    def _move_workflow_step(self, pair_id: str, step_id: str, target_step_id: str) -> None:
        steps = self._workflow_steps_for_pair(pair_id)
        from_idx = next((idx for idx, step in enumerate(steps) if step.step_id == step_id), None)
        to_idx = next((idx for idx, step in enumerate(steps) if step.step_id == target_step_id), None)
        if from_idx is None or to_idx is None or from_idx == to_idx:
            return

        if self._is_fixed_workflow_step(steps[from_idx]) or self._is_fixed_workflow_step(steps[to_idx]):
            self._set_status("Start/Finish sind fixe System-Cards und nicht verschiebbar.")
            return

        step = steps.pop(from_idx)
        if from_idx < to_idx:
            to_idx -= 1
        steps.insert(to_idx, step)
        self._normalize_workflow_steps(pair_id)
        self._save_workflow_steps_to_settings()
        self._render_workflow_steps(pair_id)

    def _workflow_card_from_widget(self, widget) -> Optional[tk.Widget]:
        current = widget
        while current is not None:
            if hasattr(current, "_workflow_step_id"):
                return current
            current = getattr(current, "master", None)
        return None

    def _workflow_step_id_from_widget(self, widget) -> str:
        card = self._workflow_card_from_widget(widget)
        if card is None:
            return ""
        return str(getattr(card, "_workflow_step_id", ""))

    def _workflow_select_step(self, pair_id: str, step_id: str) -> None:
        widget = self.sync_pair_widgets.get(pair_id)
        if widget is None:
            return
        widget["workflow_selected_step_id"] = step_id
        self._render_workflow_steps(pair_id)

    def _workflow_drag_begin(self, pair_id: str, step_id: str, event) -> None:
        widget = self.sync_pair_widgets.get(pair_id)
        if widget is None:
            return
        steps = self._workflow_steps_for_pair(pair_id)
        step = next((item for item in steps if item.step_id == step_id), None)
        if step is not None and self._is_fixed_workflow_step(step):
            return
        widget["workflow_drag_step_id"] = step_id
        widget["workflow_drag_source_x"] = event.x_root
        widget["workflow_drag_source_y"] = event.y_root

    def _workflow_drag_end(self, pair_id: str, event) -> None:
        widget = self.sync_pair_widgets.get(pair_id)
        if widget is None:
            return
        drag_step_id = str(widget.get("workflow_drag_step_id", ""))
        if not drag_step_id:
            return
        widget["workflow_drag_step_id"] = ""
        card = self._workflow_card_from_widget(event.widget)
        if card is None:
            return
        target_step_id = str(getattr(card, "_workflow_step_id", ""))
        if not target_step_id or target_step_id == drag_step_id:
            return
        self._move_workflow_step(pair_id, drag_step_id, target_step_id)

    def _render_workflow_steps(self, pair_id: str) -> None:
        widget = self.sync_pair_widgets.get(pair_id)
        if widget is None:
            return

        cards_frame = widget.get("workflow_cards_frame")
        if cards_frame is None or not hasattr(cards_frame, "winfo_children"):
            return

        for child in cards_frame.winfo_children():
            child.destroy()

        steps = self._workflow_steps_for_pair(pair_id)
        _workflow_ok, step_states, _workflow_error = self._validate_workflow_for_pair(pair_id)
        selected_id = str(widget.get("workflow_selected_step_id", ""))

        for index, step in enumerate(steps, start=1):
            is_start = self._is_start_step(step)
            is_finish = self._is_finish_step(step)
            is_fixed = is_start or is_finish
            is_valid, validate_reason = step_states.get(step.step_id, (True, "OK"))
            is_selected = step.step_id == selected_id
            card_style = "WorkflowCardSelected.TFrame" if is_selected else "WorkflowCard.TFrame"
            row_style = "WorkflowCardRowSelected.TFrame" if is_selected else "WorkflowCardRow.TFrame"
            label_style = "WorkflowCardLabelSelected.TLabel" if is_selected else "WorkflowCardLabel.TLabel"
            index_style = "WorkflowCardIndexSelected.TLabel" if is_selected else "WorkflowCardIndex.TLabel"

            card = ttk.Frame(
                cards_frame,
                style=card_style,
                width=236,
            )
            setattr(card, "_workflow_step_id", step.step_id)
            card.pack(side=tk.LEFT, fill=tk.Y, padx=(0, 8), pady=4)
            card.bind("<Button-1>", lambda _event, p=pair_id, s=step.step_id: self._workflow_select_step(p, s))
            if not is_fixed:
                card.bind("<ButtonPress-1>", lambda event, p=pair_id, s=step.step_id: self._workflow_drag_begin(p, s, event))
                card.bind("<ButtonRelease-1>", lambda event, p=pair_id: self._workflow_drag_end(p, event))

            action_menu = tk.Menu(card, tearoff=0)
            for action_name in ("select_paths", "compare", "sync_left"):
                action_menu.add_command(
                    label=self._workflow_step_title(action_name),
                    command=lambda current_pair_id=pair_id, current_step_id=step.step_id, selected_action=action_name: self._set_workflow_step_action(current_pair_id, current_step_id, selected_action),
                )

            top = ttk.Frame(card, style=row_style)
            top.pack(fill=tk.X, padx=10, pady=(10, 6))
            ttk.Label(top, text=f"{index}", style=index_style).pack(side=tk.LEFT)

            title_btn = ttk.Button(
                top,
                text=("Start (Meta)" if is_start else ("Finish (Result)" if is_finish else step.title)),
                style="Compact.TButton",
                command=lambda current_pair_id=pair_id, current_step_id=step.step_id: self._configure_workflow_step(current_pair_id, current_step_id),
            )
            title_btn.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(8, 0))
            if is_finish:
                title_btn.configure(state=tk.DISABLED)

            action_split_btn = ttk.Menubutton(top, text="▾", width=3, style="Compact.TButton", direction="below")
            action_split_btn.configure(menu=action_menu)
            action_split_btn.pack(side=tk.LEFT, padx=(6, 0))
            if is_fixed:
                action_split_btn.configure(state=tk.DISABLED)

            enabled_var = tk.BooleanVar(value=step.enabled)
            dry_run_var = tk.BooleanVar(value=str(step.config.get("dry_run", "")).strip().lower() in {"1", "true", "yes", "ja", "on"})

            def _toggle_enabled(var=enabled_var, current_step=step, current_pair_id=pair_id) -> None:
                current_step.enabled = bool(var.get())
                self._save_workflow_steps_to_settings()
                self._render_workflow_steps(current_pair_id)

            def _toggle_dry_run(var=dry_run_var, current_step=step, current_pair_id=pair_id) -> None:
                current_step.config["dry_run"] = "true" if bool(var.get()) else "false"
                self._save_workflow_steps_to_settings()
                self._render_workflow_steps(current_pair_id)

            state_row = ttk.Frame(card, style=row_style)
            state_row.pack(fill=tk.X, padx=8, pady=(0, 2))

            enabled_check = ttk.Checkbutton(state_row, text="Aktiv", variable=enabled_var, command=_toggle_enabled)
            enabled_check.pack(side=tk.LEFT)
            if is_fixed:
                enabled_var.set(True)
                enabled_check.configure(state=tk.DISABLED)
            else:
                dry_run_check = ttk.Checkbutton(state_row, text="Dry-run", variable=dry_run_var, command=_toggle_dry_run)
                dry_run_check.pack(side=tk.LEFT, padx=(10, 0))

            status_row = ttk.Frame(card, style=row_style)
            status_row.pack(fill=tk.X, padx=8, pady=(0, 4))
            ttk.Label(
                status_row,
                text=("[OK]" if is_valid else f"[ERR] {validate_reason}"),
                style=label_style,
            ).pack(side=tk.LEFT)

            if step.action.strip().lower() == "select_paths":
                path_row = ttk.Frame(card, style=row_style)
                path_row.pack(fill=tk.X, padx=8, pady=(0, 6))
                ttk.Button(
                    path_row,
                    text="📁 Ordner A",
                    command=lambda current_pair_id=pair_id, current_step_id=step.step_id: self._set_workflow_step_path(current_pair_id, current_step_id, "folder_a"),
                    style="Compact.TButton",
                ).pack(side=tk.LEFT, fill=tk.X, expand=True)
                ttk.Button(
                    path_row,
                    text="📁 Ordner B",
                    command=lambda current_pair_id=pair_id, current_step_id=step.step_id: self._set_workflow_step_path(current_pair_id, current_step_id, "folder_b"),
                    style="Compact.TButton",
                ).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(6, 0))

                folder_a_text = str(step.config.get("folder_a", "")).strip()
                folder_b_text = str(step.config.get("folder_b", "")).strip()

                def _compact_path(value: str, keep: int = 40) -> str:
                    clean = value.strip()
                    if not clean:
                        return "-"
                    if len(clean) <= keep:
                        return clean
                    head = max(12, keep // 2 - 2)
                    tail = max(12, keep - head - 3)
                    return f"{clean[:head]}...{clean[-tail:]}"

                ttk.Label(
                    card,
                    text=f"A: {_compact_path(folder_a_text)}",
                    style=label_style,
                    wraplength=190,
                ).pack(fill=tk.X, padx=8, pady=(0, 2))
                ttk.Label(
                    card,
                    text=f"B: {_compact_path(folder_b_text)}",
                    style=label_style,
                    wraplength=190,
                ).pack(fill=tk.X, padx=8, pady=(0, 6))
            summary = self._workflow_step_summary(step)
            ttk.Label(card, text=summary or "Keine Konfiguration", style=label_style, wraplength=170).pack(fill=tk.X, padx=8, pady=(0, 6))

            if is_finish:
                finish_result_btn = ttk.Button(
                    card,
                    text="Ergebnis",
                    style="Compact.TButton",
                    command=lambda p=pair_id: self._show_result_list_overlay(p),
                )
                finish_result_btn.pack(fill=tk.X, padx=8, pady=(0, 6))
                setattr(card, "_finish_result_btn", finish_result_btn)

            footer = ttk.Frame(card, style=row_style)
            footer.pack(fill=tk.X, padx=8, pady=(0, 8))
            if is_fixed:
                ttk.Label(footer, text=("Fixer Start" if is_start else "Fixer Abschluss"), style=label_style).pack(side=tk.LEFT)
            else:
                ttk.Button(footer, text="⧉", width=3, command=lambda current_pair_id=pair_id, current_step_id=step.step_id: self._duplicate_workflow_step(current_pair_id, current_step_id), style="Compact.TButton").pack(side=tk.LEFT)
                ttk.Button(footer, text="🗑", width=3, command=lambda current_pair_id=pair_id, current_step_id=step.step_id: self._remove_workflow_step(current_pair_id, current_step_id), style="Compact.TButton").pack(side=tk.LEFT, padx=(6, 0))

            for child in card.winfo_children():
                child.bind("<Button-1>", lambda event, p=pair_id, s=step.step_id: self._workflow_select_step(p, s))
                if not is_fixed:
                    child.bind("<ButtonPress-1>", lambda event, p=pair_id, s=step.step_id: self._workflow_drag_begin(p, s, event))
                    child.bind("<ButtonRelease-1>", lambda event, p=pair_id: self._workflow_drag_end(p, event))

            # The fixed Finish card itself acts as the result view, so no extra trailing
            # result card is needed after the terminal card.
            if index < len(steps):
                insert_card = ttk.Frame(
                    cards_frame,
                    style="WorkflowInsert.TFrame",
                    width=34,
                )
                insert_card.pack(side=tk.LEFT, fill=tk.Y, padx=(0, 8), pady=4)
                insert_card.pack_propagate(False)
                ttk.Button(
                    insert_card,
                    text="+",
                    width=2,
                    style="Compact.TButton",
                    command=lambda p=pair_id, insert_at=index: self._add_workflow_step(p, "compare", insert_at),
                ).pack(expand=True, padx=4, pady=6)

        widget["workflow_selected_step_id"] = selected_id if selected_id in {step.step_id for step in steps} else (steps[0].step_id if steps else "")

    def _show_popup_menu(self, menu: tk.Menu, button) -> None:
        try:
            x = button.winfo_rootx()
            y = button.winfo_rooty() + button.winfo_height()
            menu.tk_popup(x, y)
        finally:
            menu.grab_release()

    def _job_state_for_pair(self, pair_id: str) -> SyncJobState:
        state = self.job_states.get(pair_id)
        if state is None:
            state = SyncJobState(pair_id=pair_id)
            self.job_states[pair_id] = state
        return state

    def _refresh_sync_tabs(self) -> None:
        for tab_id in self.sync_notebook.tabs():
            self.sync_notebook.forget(tab_id)

        self.sync_pair_widgets.clear()
        self.tab_controllers.clear()
        if hasattr(self.runtime_deps.watcher, "_scopes"):
            self.runtime_deps.watcher._scopes.clear()
        self._add_dashboard_tab()

        for pair in self.sync_pairs:
            self._add_sync_task_tab(pair)

        self.dashboard_total_pairs_var.set(str(len(self.sync_pairs)))
        self._restore_active_sync_tab()
        self._recompute_overall_progress()
        self._update_action_button_states()
        self._install_button_tooltips()
        self._refresh_sqlite_status()

    def _add_sync_task_tab(self, pair: SyncPair) -> None:
        frame = ttk.Frame(self.sync_notebook, padding=6)
        state_key = pair.state.lower().strip() or "idle"
        compact_name = self._compact_tab_name(pair.name)
        self.sync_notebook.add(frame, text=f"{compact_name} [{state_key.upper()}]")
        palette = self._theme_palette(self.current_theme)

        job_state = self._job_state_for_pair(pair.pair_id)

        card_stack = ttk.Frame(frame)

        color_bar = tk.Frame(frame, height=6, bg="#6B7280")
        color_bar.pack(fill=tk.X, pady=(0, 8))

        name_row = ttk.Frame(frame)
        name_row.pack(fill=tk.X)
        task_name_var = tk.StringVar(value=f"Aufgabe: {pair.name}")
        ttk.Label(name_row, textvariable=task_name_var).pack(side=tk.LEFT)
        badge = tk.Label(name_row, text="IDLE", bg="#6B7280", fg="white", padx=8, pady=3)
        badge.pack(side=tk.RIGHT)

        left_var = tk.StringVar(value=pair.left)
        right_var = tk.StringVar(value=pair.right)
        pair_eta_var = tk.StringVar(value="ETA: -")
        pair_folder_var = tk.StringVar(value="Ordner: -")
        pair_progress_text_var = tk.StringVar(value="0%")
        pair_progress_var = tk.DoubleVar(value=0.0)
        pair_phase_var = tk.StringVar(value="Bereit")
        sqlite_status_var = tk.StringVar(value="SQLite: -")
        selection_var = tk.StringVar(value="Auswahl: 0")

        pair_toolbar = ttk.Frame(frame, style="Section.TFrame")
        pair_toolbar.pack(fill=tk.X, pady=(8, 0))
        ttk.Label(pair_toolbar, text="Workflow", style="Compact.TLabel").pack(side=tk.LEFT, padx=(8, 6))
        workflow_add_btn = ttk.Button(pair_toolbar, text="➕ Schritt", command=lambda p=pair.pair_id: self._add_workflow_step(p), style="Compact.TButton")
        workflow_add_btn.pack(side=tk.LEFT, padx=(0, 6))
        workflow_template_btn = ttk.Button(pair_toolbar, text="Vorlagen", command=lambda p=pair.pair_id: self._show_workflow_template_menu(p), style="Compact.TButton")
        workflow_template_btn.pack(side=tk.LEFT, padx=(0, 6))
        workflow_reset_btn = ttk.Button(pair_toolbar, text="↺ Standard", command=lambda p=pair.pair_id: self._reset_workflow_to_default(p), style="Compact.TButton")
        workflow_reset_btn.pack(side=tk.LEFT, padx=(0, 6))
        workflow_run_pair_btn = ttk.Button(pair_toolbar, text="▶ Start", command=lambda p=pair: self._on_run_workflow_for_pair(p), style="Compact.TButton")
        workflow_run_pair_btn.pack(side=tk.LEFT, padx=(0, 6))

        ttk.Separator(pair_toolbar, orient=tk.VERTICAL).pack(side=tk.LEFT, fill=tk.Y, padx=(0, 8))
        ttk.Label(pair_toolbar, text="Ansicht", style="Compact.TLabel").pack(side=tk.LEFT, padx=(0, 6))
        folder_panel_btn = ttk.Button(pair_toolbar, text="📁 Ordner", command=lambda p=pair.pair_id: self._toggle_optional_panel(p, "paths"), style="Compact.TButton")
        folder_panel_btn.pack(side=tk.LEFT, padx=(0, 6))
        filter_panel_btn = ttk.Button(pair_toolbar, text="🔎 Filter", command=lambda p=pair.pair_id: self._toggle_optional_panel(p, "filters"), style="Compact.TButton")
        filter_panel_btn.pack(side=tk.LEFT, padx=(0, 6))

        toolbar_progress = ttk.Frame(pair_toolbar, style="Section.TFrame")
        toolbar_progress.pack(side=tk.RIGHT, fill=tk.X, expand=True, padx=(10, 8))
        ttk.Label(toolbar_progress, textvariable=pair_phase_var, style="Compact.TLabel").pack(side=tk.LEFT, padx=(0, 8))
        ttk.Progressbar(
            toolbar_progress,
            orient=tk.HORIZONTAL,
            mode="determinate",
            variable=pair_progress_var,
            maximum=100.0,
            length=180,
        ).pack(side=tk.LEFT, fill=tk.X, expand=True)
        ttk.Label(toolbar_progress, textvariable=pair_progress_text_var, style="Compact.TLabel").pack(side=tk.LEFT, padx=(8, 6))
        ttk.Label(toolbar_progress, textvariable=pair_eta_var, style="Compact.TLabel").pack(side=tk.LEFT)

        card_stack.pack(fill=tk.BOTH, expand=True)

        path_frame = ttk.LabelFrame(card_stack, text="1. Ordnerauswahl")
        path_frame.pack(fill=tk.X, pady=(8, 10))
        path_frame.pack_forget()

        folder_row = ttk.Frame(path_frame)
        folder_row.pack(fill=tk.X, padx=8, pady=(8, 10))

        left_wrapper = ttk.Frame(folder_row)
        left_wrapper.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 6))
        ttk.Label(left_wrapper, text="Links", style="Compact.TLabel").pack(anchor="w", pady=(0, 4))
        left_inner = ttk.Frame(left_wrapper)
        left_inner.pack(fill=tk.X)
        left_entry = ttk.Entry(left_inner, textvariable=left_var)
        left_entry.pack(side=tk.LEFT, fill=tk.X, expand=True)
        pick_left_btn = ttk.Button(left_inner, text="📁 Ordner wählen", command=lambda p=pair: self._pick_left_for_pair(p), style="Compact.TButton")
        pick_left_btn.pack(side=tk.LEFT, padx=(8, 0))

        right_wrapper = ttk.Frame(folder_row)
        right_wrapper.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(6, 0))
        ttk.Label(right_wrapper, text="Rechts", style="Compact.TLabel").pack(anchor="w", pady=(0, 4))
        right_inner = ttk.Frame(right_wrapper)
        right_inner.pack(fill=tk.X)
        right_entry = ttk.Entry(right_inner, textvariable=right_var)
        right_entry.pack(side=tk.LEFT, fill=tk.X, expand=True)
        pick_right_btn = ttk.Button(right_inner, text="📁 Ordner wählen", command=lambda p=pair: self._pick_right_for_pair(p), style="Compact.TButton")
        pick_right_btn.pack(side=tk.LEFT, padx=(8, 0))

        status_frame = ttk.LabelFrame(card_stack, text="2. Status")
        status_frame.pack(fill=tk.X, pady=(0, 8))

        control_row = ttk.Frame(status_frame, style="Section.TFrame")
        control_row.pack(fill=tk.X, padx=8, pady=(8, 8))
        control_row.pack_forget()

        search_var = tk.StringVar(value=job_state.search_text)
        filter_var = tk.StringVar(value=job_state.filter_value or "all")
        search_var.trace_add("write", lambda *_args, p=pair.pair_id: self._on_tab_search_changed(p))
        filter_var.trace_add("write", lambda *_args, p=pair.pair_id: self._on_tab_filter_changed(p))

        clear_filter_btn = ttk.Button(control_row, text="🧹 Filter leeren", command=lambda p=pair.pair_id: self._clear_pair_filters(p), style="Compact.TButton")
        clear_filter_btn.pack(side=tk.LEFT, padx=(8, 6))
        apply_filter_btn = ttk.Button(control_row, text="🔎 Neu filtern", command=lambda p=pair.pair_id: self._refresh_visible_rows(p), style="Compact.TButton")
        apply_filter_btn.pack(side=tk.LEFT, padx=(0, 6))
        clear_selection_btn = ttk.Button(control_row, text="☐ Auswahl löschen", command=lambda p=pair.pair_id: self._clear_pair_selection(p), style="Compact.TButton")
        clear_selection_btn.pack(side=tk.LEFT, padx=(0, 6))
        select_visible_btn = ttk.Button(control_row, text="☑ Alle sichtbaren", command=lambda p=pair.pair_id: self._select_visible_rows(p), style="Compact.TButton")
        select_visible_btn.pack(side=tk.LEFT)

        action_row = ttk.Frame(status_frame, style="Section.TFrame")
        action_row.pack(fill=tk.X, padx=8, pady=(0, 8))
        action_row.pack_forget()
        ttk.Label(action_row, text="Aktionen", style="Compact.TLabel").pack(side=tk.LEFT, padx=(8, 6))
        compare_btn = ttk.Button(action_row, text="🔍 Check", command=lambda p=pair: self._on_compare_for_pair(p), style="Compact.TButton")
        compare_btn.pack(side=tk.LEFT, padx=(0, 6))
        sync_btn = ttk.Button(action_row, text="🔄 Sync", command=lambda p=pair: self._on_sync_for_pair(p), style="Compact.TButton")
        sync_btn.pack(side=tk.LEFT, padx=(0, 6))
        pause_btn = ttk.Button(action_row, text="⏸ Pause", command=lambda p=pair: self._on_pause_resume_for_pair(p), style="Compact.TButton")
        pause_btn.pack(side=tk.LEFT, padx=(0, 6))
        cancel_btn = ttk.Button(action_row, text="⛔ Abbruch", command=lambda p=pair: self._on_cancel_for_pair(p), style="Compact.TButton")
        cancel_btn.pack(side=tk.LEFT, padx=(0, 6))
        copy_btn = ttk.Button(action_row, text="📋 Kopieren", command=lambda p=pair: self._on_copy_diff_for_pair(p), style="Compact.TButton")
        copy_btn.pack(side=tk.LEFT, padx=(0, 6))
        zip_btn = ttk.Button(action_row, text="🗜 ZIP", command=lambda p=pair: self._on_zip_diff_for_pair(p), style="Compact.TButton")
        zip_btn.pack(side=tk.LEFT)

        workflow_frame = ttk.LabelFrame(card_stack, text="3. Workflow-Karten")
        workflow_frame.pack(fill=tk.X, pady=(0, 8))

        workflow_canvas = tk.Canvas(workflow_frame, height=1, background=palette["surface"], highlightthickness=0)
        workflow_scroll = ttk.Scrollbar(workflow_frame, orient=tk.HORIZONTAL, command=workflow_canvas.xview)
        workflow_canvas.configure(xscrollcommand=workflow_scroll.set)
        workflow_cards_frame = ttk.Frame(workflow_canvas)
        workflow_canvas_window = workflow_canvas.create_window((0, 0), window=workflow_cards_frame, anchor="nw")
        workflow_canvas.pack(fill=tk.X, padx=8, pady=(0, 4))

        scroll_state = {"visible": False}

        def _update_workflow_scrollregion(_event=None, canvas=workflow_canvas, inner=workflow_cards_frame) -> None:
            try:
                canvas.update_idletasks()
                inner_width = max(1, int(inner.winfo_reqwidth()))
                inner_height = max(1, int(inner.winfo_reqheight()))
                view_width = max(1, int(canvas.winfo_width()))

                # Skip visibility decisions until geometry is initialized.
                if view_width <= 32:
                    if scroll_state["visible"]:
                        workflow_scroll.pack_forget()
                        scroll_state["visible"] = False
                    return

                canvas.configure(scrollregion=canvas.bbox("all"))
                canvas.itemconfigure(workflow_canvas_window, width=inner_width)

                target_height = max(96, inner_height + 10)
                canvas.configure(height=target_height)

                need_scroll = inner_width > (view_width + 4)
                if need_scroll and not scroll_state["visible"]:
                    workflow_scroll.pack(fill=tk.X, padx=8, pady=(0, 8))
                    scroll_state["visible"] = True
                elif (not need_scroll) and scroll_state["visible"]:
                    workflow_scroll.pack_forget()
                    scroll_state["visible"] = False
            except Exception:
                pass

        workflow_cards_frame.bind("<Configure>", _update_workflow_scrollregion)
        workflow_canvas.bind("<Configure>", _update_workflow_scrollregion)

        workflow_result_title_var = tk.StringVar(value="ResultCard")
        workflow_result_body_var = tk.StringVar(value="Warte auf Workflow-Start...")

        optional_overlay = tk.Frame(frame, bg=palette["panel"], highlightbackground=palette["border"], highlightthickness=1)
        optional_overlay_header = ttk.Frame(optional_overlay, style="Section.TFrame")
        optional_overlay_header.pack(fill=tk.X, padx=8, pady=(8, 4))
        optional_overlay_title_var = tk.StringVar(value="Details")
        ttk.Label(optional_overlay_header, textvariable=optional_overlay_title_var, style="Compact.TLabel").pack(side=tk.LEFT)
        optional_overlay_close_btn = ttk.Button(optional_overlay_header, text="✖ Schliessen", command=lambda p=pair.pair_id: self._toggle_optional_panel(p, "close"), style="Compact.TButton")
        optional_overlay_close_btn.pack(side=tk.RIGHT)
        optional_overlay_body = ttk.Frame(optional_overlay)
        optional_overlay_body.pack(fill=tk.BOTH, expand=True, padx=8, pady=(0, 8))
        optional_overlay.place_forget()

        result_frame = ttk.LabelFrame(card_stack, text="4. Sync-Ergebnis")
        result_frame.pack(fill=tk.BOTH, expand=True, pady=(0, 8))
        result_frame.pack_forget()
        result_header = ttk.Frame(result_frame, style="Section.TFrame")
        result_header.pack(fill=tk.X, padx=8, pady=(8, 4))
        result_summary_var = tk.StringVar(value="Ergebnis: -")
        result_overlay_visible = tk.BooleanVar(value=False)
        result_overlay_btn = ttk.Button(result_header, text="Tabelle anzeigen", style="Compact.TButton")
        result_overlay_btn.configure(command=lambda p=pair.pair_id: self._toggle_result_overlay(p))
        result_overlay_btn.pack(side=tk.RIGHT)
        ttk.Label(result_header, textvariable=result_summary_var, style="Compact.TLabel").pack(side=tk.LEFT, padx=(8, 0))

        result_overlay = tk.Frame(result_frame, bg=palette["surface"])
        result_overlay.pack(fill=tk.BOTH, expand=True, padx=8, pady=(0, 8))
        result_overlay.pack_forget()
        table_container = ttk.Frame(result_overlay)
        table_container.pack(fill=tk.BOTH, expand=True)

        column_filter_frame = ttk.LabelFrame(table_container, text="Tabellenkopf-Filter")
        column_filter_frame.pack(fill=tk.X, pady=(0, 4))
        column_filter_vars: Dict[str, tk.StringVar] = {}

        # Align filters with the table columns so search/filter feels part of the header.
        column_filter_frame.grid_columnconfigure(0, minsize=78, weight=0)
        column_filter_frame.grid_columnconfigure(1, minsize=180, weight=1)
        column_filter_frame.grid_columnconfigure(2, minsize=120, weight=0)
        column_filter_frame.grid_columnconfigure(3, minsize=120, weight=0)
        column_filter_frame.grid_columnconfigure(4, minsize=540, weight=3)

        ttk.Label(column_filter_frame, text="Filter", style="Compact.TLabel").grid(row=0, column=0, sticky="w", padx=(8, 6), pady=(4, 6))

        search_entry = ttk.Entry(column_filter_frame, textvariable=search_var)
        search_entry.grid(row=0, column=1, sticky="ew", padx=(0, 6), pady=(4, 6))

        type_combo = ttk.Combobox(
            column_filter_frame,
            textvariable=filter_var,
            values=("all", "left_only", "right_only", "different"),
            state="readonly",
            width=12,
            style="Compact.TCombobox",
        )
        type_combo.grid(row=0, column=2, sticky="ew", padx=(0, 6), pady=(4, 6))

        side_var = tk.StringVar(value="")
        column_filter_vars["side"] = side_var
        ttk.Entry(column_filter_frame, textvariable=side_var).grid(row=0, column=3, sticky="ew", padx=(0, 6), pady=(4, 6))
        side_var.trace_add("write", lambda *_args, pair_id=pair.pair_id: self._on_column_filter_changed(pair_id, "side"))

        path_var = tk.StringVar(value="")
        column_filter_vars["path"] = path_var
        ttk.Entry(column_filter_frame, textvariable=path_var).grid(row=0, column=4, sticky="ew", padx=(0, 8), pady=(4, 6))
        path_var.trace_add("write", lambda *_args, pair_id=pair.pair_id: self._on_column_filter_changed(pair_id, "path"))

        sync_tree = ttk.Treeview(
            table_container,
            columns=("check", "operation", "type", "side", "path"),
            show="headings",
            height=8,
            selectmode="extended",
        )
        sync_tree.heading("check", text="Auswahl")
        sync_tree.heading("operation", text="Operation")
        sync_tree.heading("type", text="Diff-Typ")
        sync_tree.heading("side", text="Seite")
        sync_tree.heading("path", text="Relativer Pfad")
        sync_tree.column("check", width=78, anchor="center", stretch=False)
        sync_tree.column("operation", width=160, anchor="w", stretch=False)
        sync_tree.column("type", width=120, anchor="center")
        sync_tree.column("side", width=100, anchor="center")
        sync_tree.column("path", width=420, anchor="w")
        sync_tree.pack(fill=tk.BOTH, expand=True)

        sync_tree.tag_configure("left_only", background="#EFF6FF")
        sync_tree.tag_configure("right_only", background="#FFF7ED")
        sync_tree.tag_configure("different", background="#FEF3C7")
        sync_tree.tag_configure("selected", foreground="#111827")

        sync_tree.bind("<Button-1>", lambda event, p=pair.pair_id: self._on_sync_tree_click(event, p))
        sync_tree.bind("<ButtonRelease-1>", lambda _event, p=pair.pair_id: self._update_pair_selection_label(p))
        sync_tree.bind("<<TreeviewSelect>>", lambda _event, p=pair.pair_id: self._update_pair_selection_label(p))

        status_row = ttk.Frame(status_frame)
        status_row.pack(fill=tk.X, padx=8, pady=(8, 8))
        ttk.Label(status_row, textvariable=selection_var).pack(side=tk.LEFT)
        ttk.Label(status_row, textvariable=sqlite_status_var).pack(side=tk.LEFT, padx=(10, 0))

        self.sync_pair_widgets[pair.pair_id] = {
            "frame": frame,
            "badge": badge,
            "name": pair.name,
            "task_name_var": task_name_var,
            "color_bar": color_bar,
            "eta_var": pair_eta_var,
            "folder_var": pair_folder_var,
            "progress_text_var": pair_progress_text_var,
            "progress_var": pair_progress_var,
            "sqlite_status_var": sqlite_status_var,
            "search_var": search_var,
            "filter_var": filter_var,
            "selection_var": selection_var,
            "left_var": left_var,
            "right_var": right_var,
            "status_frame": status_frame,
            "status_row": status_row,
            "pick_left_btn": pick_left_btn,
            "pick_right_btn": pick_right_btn,
            "compare_btn": compare_btn,
            "sync_btn": sync_btn,
            "pause_btn": pause_btn,
            "cancel_btn": cancel_btn,
            "copy_btn": copy_btn,
            "zip_btn": zip_btn,
            "clear_filter_btn": clear_filter_btn,
            "apply_filter_btn": apply_filter_btn,
            "clear_selection_btn": clear_selection_btn,
            "select_visible_btn": select_visible_btn,
            "workflow_frame": workflow_frame,
            "workflow_add_btn": workflow_add_btn,
            "workflow_template_btn": workflow_template_btn,
            "workflow_reset_btn": workflow_reset_btn,
            "workflow_run_pair_btn": workflow_run_pair_btn,
            "workflow_phase_var": pair_phase_var,
            "folder_panel_btn": folder_panel_btn,
            "filter_panel_btn": filter_panel_btn,
            "path_frame": path_frame,
            "control_row": control_row,
            "action_row": action_row,
            "workflow_canvas": workflow_canvas,
            "workflow_scroll": workflow_scroll,
            "workflow_cards_frame": workflow_cards_frame,
            "workflow_canvas_window": workflow_canvas_window,
            "workflow_selected_step_id": "",
            "workflow_drag_step_id": "",
            "workflow_result_title_var": workflow_result_title_var,
            "workflow_result_body_var": workflow_result_body_var,
            "workflow_result_last_progress_bucket": -1,
            "result_summary_var": result_summary_var,
            "result_overlay": result_overlay,
            "result_overlay_visible": result_overlay_visible,
            "result_overlay_btn": result_overlay_btn,
            "optional_overlay": optional_overlay,
            "optional_overlay_body": optional_overlay_body,
            "optional_overlay_title_var": optional_overlay_title_var,
            "optional_overlay_close_btn": optional_overlay_close_btn,
            "active_optional_panel": "",
            "sync_tree": sync_tree,
            "column_filters": column_filter_vars,
            "row_state_by_key": {},
            "row_state_by_item": {},
            "result_frame": result_frame,
            "optional_visibility": {
                "paths": False,
                "filters": False,
                "actions": False,
                "result": False,
            },
        }

        self.tab_controllers[pair.pair_id] = SyncTabController(self, pair.pair_id)
        self.runtime_deps.watcher.register_scope(
            WatchScope(pair_id=pair.pair_id, left_path=pair.left, right_path=pair.right, enabled=True)
        )
        self._sync_job_state_to_widgets(pair.pair_id)
        self._render_workflow_steps(pair.pair_id)
        self._update_action_button_states()
        self._install_button_tooltips()

    def _restore_active_sync_tab(self) -> None:
        if not self.last_active_pair_id:
            self.sync_notebook.select(0)
            return

        for idx, pair in enumerate(self.sync_pairs, start=1):
            if pair.pair_id == self.last_active_pair_id:
                self.sync_notebook.select(idx)
                return

        self.sync_notebook.select(0)

    def _add_dashboard_tab(self) -> None:
        frame = ttk.Frame(self.sync_notebook, padding=10)
        self.sync_notebook.add(frame, text="Dashboard")

        stats = ttk.LabelFrame(frame, text="Dashboard")
        stats.pack(fill=tk.X, pady=(0, 10))
        ttk.Label(stats, text="Sync-Tabs:").grid(row=0, column=0, padx=8, pady=6, sticky="w")
        ttk.Label(stats, textvariable=self.dashboard_total_pairs_var).grid(row=0, column=1, padx=8, pady=6, sticky="w")
        ttk.Label(stats, text="Modus:").grid(row=0, column=2, padx=8, pady=6, sticky="w")
        ttk.Label(stats, textvariable=self.dashboard_mode_var).grid(row=0, column=3, padx=8, pady=6, sticky="w")
        ttk.Label(stats, text="Worker:").grid(row=0, column=4, padx=8, pady=6, sticky="w")
        ttk.Label(stats, textvariable=self.dashboard_workers_var).grid(row=0, column=5, padx=8, pady=6, sticky="w")
        ttk.Label(stats, text="Aktiver Sync:").grid(row=0, column=6, padx=8, pady=6, sticky="w")
        ttk.Label(stats, textvariable=self.dashboard_running_var).grid(row=0, column=7, padx=8, pady=6, sticky="w")
        ttk.Label(stats, textvariable=self.sqlite_app_var).grid(row=1, column=0, columnspan=4, padx=8, pady=6, sticky="w")
        ttk.Label(stats, textvariable=self.sqlite_pair_var).grid(row=1, column=4, columnspan=4, padx=8, pady=6, sticky="w")
        ttk.Label(stats, text="Agent:").grid(row=2, column=0, padx=8, pady=6, sticky="w")
        ttk.Label(stats, textvariable=self.agent_state_var).grid(row=2, column=1, columnspan=3, padx=8, pady=6, sticky="w")
        ttk.Label(stats, text="Agent Port:").grid(row=2, column=4, padx=8, pady=6, sticky="w")
        ttk.Entry(stats, textvariable=self.agent_port_var, width=8).grid(row=2, column=5, padx=8, pady=6, sticky="w")

        hint = ttk.Label(
            frame,
            text="Dashboard zeigt Uebersicht und globales Log fuer alle Operationen.",
            anchor="w",
        )
        hint.pack(fill=tk.X, pady=(8, 0))

        log_frame = ttk.LabelFrame(frame, text="Globales Log")
        log_frame.pack(fill=tk.BOTH, expand=True, pady=(8, 0))
        log_text = tk.Text(log_frame, height=12, wrap="word", state="disabled")
        log_text.pack(fill=tk.BOTH, expand=True)
        self.dashboard_log_text = log_text
        palette = self._theme_palette(self.current_theme)
        log_text.configure(
            background=palette["surface"],
            foreground=palette["text"],
            insertbackground=palette["text"],
        )

        if self.global_log_lines:
            log_text.configure(state="normal")
            log_text.insert(tk.END, "\n".join(self.global_log_lines) + "\n")
            log_text.see(tk.END)
            log_text.configure(state="disabled")

    def _on_notebook_tab_changed(self, _event=None) -> None:
        idx = self._selected_pair_index()
        if idx is None:
            self.last_active_pair_id = ""
            self.settings["last_active_pair_id"] = ""
            self._refresh_sqlite_status()
            return

        pair = self.sync_pairs[idx]
        self.last_active_pair_id = pair.pair_id
        self.settings["last_active_pair_id"] = pair.pair_id
        self._sync_job_state_to_widgets(pair.pair_id)
        self._refresh_sqlite_status()
        self._update_action_button_states()

    def _on_notebook_tab_double_click(self, event) -> None:
        try:
            clicked_index = self.sync_notebook.index(f"@{event.x},{event.y}")
        except Exception:
            return

        if clicked_index <= 0:
            return

        pair_idx = clicked_index - 1
        if pair_idx < 0 or pair_idx >= len(self.sync_pairs):
            return

        pair = self.sync_pairs[pair_idx]
        new_name = simpledialog.askstring(
            "Sync-Tab umbenennen",
            "Neuer Name fuer die Sync-Aufgabe:",
            initialvalue=pair.name,
        )
        if new_name is None:
            return

        new_name = self._unique_pair_name(new_name.strip() or pair.name)
        if new_name == pair.name:
            return

        pair.name = new_name
        widget = self.sync_pair_widgets.get(pair.pair_id)
        if widget is not None:
            widget["name"] = new_name
            task_name_var = widget.get("task_name_var")
            if isinstance(task_name_var, tk.StringVar):
                task_name_var.set(f"Aufgabe: {new_name}")
            frame = widget.get("frame")
            if isinstance(frame, ttk.Frame):
                current_text = self.sync_notebook.tab(frame, "text")
                state_suffix = ""
                if " [" in current_text and current_text.endswith("]"):
                    state_suffix = current_text[current_text.rfind(" ["):]
                self.sync_notebook.tab(frame, text=f"{new_name}{state_suffix}")

        self._save_sync_pairs_to_settings()
        self._append_log(f"Sync-Tab umbenannt: {new_name}", pair.pair_id)

    def _selected_pair_index(self) -> Optional[int]:
        try:
            tab_idx = self.sync_notebook.index("current")
        except Exception:
            return None
        if tab_idx <= 0:
            return None
        pair_idx = tab_idx - 1
        if pair_idx < 0 or pair_idx >= len(self.sync_pairs):
            return None
        return pair_idx

    def _on_add_pair(self) -> None:
        left = ""
        right = ""
        default_name = f"Sync-Aufgabe {len(self.sync_pairs) + 1}"
        name = simpledialog.askstring("Sync-Paar", "Name fuer das Sync-Paar:", initialvalue=default_name)
        if name is None:
            return
        name = self._unique_pair_name(name.strip() or default_name)

        pair = SyncPair(pair_id=str(uuid.uuid4()), name=name, left=left, right=right)
        self.sync_pairs.append(pair)
        self.workflow_steps_by_pair[pair.pair_id] = self._default_workflow_steps(pair)
        self._refresh_sync_tabs()
        self._save_sync_pairs_to_settings()
        self._append_log(f"Sync-Paar hinzugefuegt: {name}", self.sync_pairs[-1].pair_id)

    def _on_remove_pair(self) -> None:
        idx = self._selected_pair_index()
        if idx is None:
            return
        pair = self.sync_pairs[idx]
        del self.sync_pairs[idx]
        if self.last_active_pair_id == pair.pair_id:
            self.last_active_pair_id = ""
        self.runtime_deps.watcher.unregister_scope(pair.pair_id)
        self.tab_controllers.pop(pair.pair_id, None)
        self.workflow_steps_by_pair.pop(pair.pair_id, None)
        self._refresh_sync_tabs()
        self._save_sync_pairs_to_settings()
        self._append_log(f"Sync-Paar entfernt: {pair.name}", pair.pair_id)

    def _on_use_pair(self) -> None:
        idx = self._selected_pair_index()
        if idx is None:
            return
        pair = self.sync_pairs[idx]
        self._sync_pair_model_from_widgets(pair)
        self.settings["left_path"] = pair.left
        self.settings["right_path"] = pair.right
        self._set_status(f"Paar geladen: {pair.name}")

    def _workflow_template(self, pair: SyncPair, name: str) -> List[WorkflowStep]:
        template = name.strip().lower()
        if template == "check_sync_finish":
            return [
                WorkflowStep(step_id=str(uuid.uuid4()), action="select_paths", title="Ordner A/B", config={"folder_a": pair.left, "folder_b": pair.right}),
                WorkflowStep(step_id=str(uuid.uuid4()), action="compare", title="Check"),
                WorkflowStep(step_id=str(uuid.uuid4()), action="sync_left", title="Sync"),
                WorkflowStep(step_id=str(uuid.uuid4()), action="finish", title="Finish"),
            ]
        if template == "check_zip_finish":
            return [
                WorkflowStep(step_id=str(uuid.uuid4()), action="select_paths", title="Ordner A/B", config={"folder_a": pair.left, "folder_b": pair.right}),
                WorkflowStep(step_id=str(uuid.uuid4()), action="compare", title="Check"),
                WorkflowStep(step_id=str(uuid.uuid4()), action="sync_left", title="Sync"),
                WorkflowStep(step_id=str(uuid.uuid4()), action="finish", title="Finish"),
            ]
        if template == "check_copy_finish":
            return [
                WorkflowStep(step_id=str(uuid.uuid4()), action="select_paths", title="Ordner A/B", config={"folder_a": pair.left, "folder_b": pair.right}),
                WorkflowStep(step_id=str(uuid.uuid4()), action="compare", title="Check"),
                WorkflowStep(step_id=str(uuid.uuid4()), action="sync_left", title="Sync"),
                WorkflowStep(step_id=str(uuid.uuid4()), action="finish", title="Finish"),
            ]
        return self._default_workflow_steps(pair)

    def _reset_workflow_to_default(self, pair_id: str) -> None:
        pair = self._pair_by_id(pair_id)
        if pair is None:
            return
        self.workflow_steps_by_pair[pair_id] = self._default_workflow_steps(pair)
        self._save_workflow_steps_to_settings()
        self._render_workflow_steps(pair_id)

    def _show_workflow_template_menu(self, pair_id: str) -> None:
        widget = self.sync_pair_widgets.get(pair_id)
        if widget is None:
            return
        button = widget.get("workflow_template_btn")
        if not isinstance(button, ttk.Button):
            return

        menu = tk.Menu(self, tearoff=0)
        menu.add_command(label="Check -> Sync -> Finish", command=lambda p=pair_id: self._apply_workflow_template(p, "check_sync_finish"))
        self._show_popup_menu(menu, button)

    def _apply_workflow_template(self, pair_id: str, template_name: str) -> None:
        pair = self._pair_by_id(pair_id)
        if pair is None:
            return
        self.workflow_steps_by_pair[pair_id] = self._workflow_template(pair, template_name)
        self._save_workflow_steps_to_settings()
        self._render_workflow_steps(pair_id)

    def _on_run_workflow_for_pair(self, pair: SyncPair) -> None:
        self._sync_pair_model_from_widgets(pair)
        steps = list(self._workflow_steps_for_pair(pair.pair_id))
        if not steps:
            messagebox.showerror("Fehler", "Workflow ist leer.")
            return
        workflow_ok, _step_states, workflow_error = self._validate_workflow_for_pair(pair.pair_id)
        if not workflow_ok:
            detail = workflow_error or "Workflow ist nicht valide."
            self._set_status("Workflow-Validierung fehlgeschlagen")
            messagebox.showerror("Workflow ungueltig", f"Workflow kann nicht gestartet werden:\n\n{detail}")
            return
        if not self._confirm("Workflow starten", f"Workflow fuer '{pair.name}' jetzt starten?"):
            return
        self._overall_batch_pairs = {pair.pair_id}
        self._set_pair_state(pair.pair_id, "running")
        self.current_operation_pair_id = pair.pair_id
        self.dashboard_running_var.set(pair.name)
        self._set_status(f"Workflow gestartet: {pair.name}")
        active_steps = sum(1 for step in steps if step.enabled)
        self._update_workflow_result_card(
            pair.pair_id,
            title="Workflow gestartet",
            line=f"Aktiver Schritt: Vorbereitung ({active_steps} Steps)",
            reset=True,
        )
        widget = self.sync_pair_widgets.get(pair.pair_id)
        if widget is not None:
            widget["workflow_result_last_progress_bucket"] = -1
            progress_var = widget.get("progress_var")
            progress_text_var = widget.get("progress_text_var")
            eta_var = widget.get("eta_var")
            folder_var = widget.get("folder_var")
            phase_var = widget.get("workflow_phase_var")
            if isinstance(progress_var, tk.DoubleVar):
                progress_var.set(0.0)
            if isinstance(progress_text_var, tk.StringVar):
                progress_text_var.set("0%")
            if isinstance(eta_var, tk.StringVar):
                eta_var.set("ETA: -")
            if isinstance(folder_var, tk.StringVar):
                folder_var.set("Ordner: -")
            if isinstance(phase_var, tk.StringVar):
                phase_var.set("Startet...")
        self.scheduler.start_live_snapshot_refresh([pair])
        self._save_sync_pairs_to_settings()
        self._run_ui_action(lambda: self.controller.run_workflow(pair, steps))

    def _on_sync_active_pair(self) -> None:
        idx = self._selected_pair_index()
        if idx is None:
            messagebox.showerror("Fehler", "Kein Sync-Tab aktiv.")
            return
        pair = self.sync_pairs[idx]
        self._sync_pair_model_from_widgets(pair)
        if not self._confirm("Sync-Tab bestaetigen", f"Sync-Tab '{pair.name}' jetzt verarbeiten?"):
            return
        self._overall_batch_pairs = {pair.pair_id}
        self._set_pair_state(pair.pair_id, "running")
        self.dashboard_running_var.set(pair.name)
        self.current_operation_pair_id = pair.pair_id
        self.scheduler.start_live_snapshot_refresh([pair])
        self._save_sync_pairs_to_settings()
        self._set_status(f"Sync-Tab gestartet: {pair.name}")
        self._run_ui_action(lambda: self.controller.synchronize_pairs([pair], "sequential", 1))

    def _on_run_workflow_active_pair(self) -> None:
        pair = self._selected_pair()
        if pair is None:
            messagebox.showerror("Fehler", "Kein Sync-Tab aktiv.")
            return
        self._on_run_workflow_for_pair(pair)

    def _on_sync_pairs(self) -> None:
        if not self.sync_pairs:
            messagebox.showerror("Fehler", "Keine Sync-Paare vorhanden.")
            return
        if not self._confirm(
            "Multi-Sync bestaetigen",
            "Alle Sync-Paare jetzt gemaess Einstellung verarbeiten?",
        ):
            return

        mode = self.sync_mode_var.get().strip().lower() or "sequential"
        workers = max(1, int(self.max_parallel_var.get()))
        for pair in self.sync_pairs:
            self._sync_pair_model_from_widgets(pair)
        self._overall_batch_pairs = {pair.pair_id for pair in self.sync_pairs}
        for pair in self.sync_pairs:
            self._set_pair_state(pair.pair_id, "idle")
        self.dashboard_running_var.set("-")
        self._save_sync_pairs_to_settings()
        self._set_status(f"Multi-Sync gestartet ({mode})...")
        self.scheduler.start_live_snapshot_refresh(self.sync_pairs)
        self._run_ui_action(lambda: self.controller.synchronize_pairs(self.sync_pairs, mode, workers))

    def _selected_pair_id_if_visible(self) -> str:
        pair = self._selected_pair()
        return pair.pair_id if pair is not None else ""

    def _selected_pair(self) -> Optional[SyncPair]:
        idx = self._selected_pair_index()
        if idx is None:
            return None
        return self.sync_pairs[idx]

    def _current_pair_widgets(self) -> Optional[Dict[str, object]]:
        pair_id = self.current_operation_pair_id
        if pair_id and pair_id in self.sync_pair_widgets:
            return self.sync_pair_widgets.get(pair_id)
        pair = self._selected_pair()
        if pair is None:
            return None
        return self.sync_pair_widgets.get(pair.pair_id)

    def _sync_pair_model_from_widgets(self, pair: SyncPair) -> None:
        widget = self.sync_pair_widgets.get(pair.pair_id)
        if widget is None:
            return
        left_var = widget.get("left_var")
        right_var = widget.get("right_var")
        if isinstance(left_var, tk.StringVar):
            pair.left = left_var.get().strip()
        if isinstance(right_var, tk.StringVar):
            pair.right = right_var.get().strip()

    def _on_tab_search_changed(self, pair_id: str) -> None:
        controller = self.tab_controllers.get(pair_id)
        widget = self.sync_pair_widgets.get(pair_id)
        if controller is None or widget is None:
            return
        search_var = widget.get("search_var")
        if isinstance(search_var, tk.StringVar):
            controller.apply_search(search_var.get())

    def _on_tab_filter_changed(self, pair_id: str) -> None:
        controller = self.tab_controllers.get(pair_id)
        widget = self.sync_pair_widgets.get(pair_id)
        if controller is None or widget is None:
            return
        filter_var = widget.get("filter_var")
        if isinstance(filter_var, tk.StringVar):
            controller.apply_filter(filter_var.get())

    def _sync_job_state_to_widgets(self, pair_id: str) -> None:
        widget = self.sync_pair_widgets.get(pair_id)
        state = self.job_states.get(pair_id)
        if widget is None or state is None:
            return

        search_var = widget.get("search_var")
        filter_var = widget.get("filter_var")
        if isinstance(search_var, tk.StringVar) and search_var.get() != state.search_text:
            search_var.set(state.search_text)
        if isinstance(filter_var, tk.StringVar) and filter_var.get() != state.filter_value:
            filter_var.set(state.filter_value or "all")

    def _update_pair_job_state(self, pair_id: str) -> None:
        widget = self.sync_pair_widgets.get(pair_id)
        state = self.job_states.get(pair_id)
        if widget is None or state is None:
            return

        search_var = widget.get("search_var")
        filter_var = widget.get("filter_var")
        selection_var = widget.get("selection_var")
        table = widget.get("sync_tree")

        if isinstance(search_var, tk.StringVar):
            state.search_text = search_var.get().strip()
        if isinstance(filter_var, tk.StringVar):
            state.filter_value = filter_var.get().strip().lower() or "all"
        if isinstance(table, ttk.Treeview):
            state.selection_count = self._checked_count(pair_id)
        if isinstance(selection_var, tk.StringVar):
            selection_var.set(f"Auswahl: {state.selection_count}")

        state.last_refresh_ns = time.time_ns()
        self._save_job_states_to_settings()

    def _toggle_pair_section(self, pair_id: str, section_name: str) -> None:
        widget = self.sync_pair_widgets.get(pair_id)
        if widget is None:
            return
        current = bool(widget.get(f"{section_name}_expanded", False))
        self._set_pair_section_state(pair_id, section_name, expanded=not current)

    def _set_pair_section_state(self, pair_id: str, section_name: str, expanded: bool) -> None:
        widget = self.sync_pair_widgets.get(pair_id)
        if widget is None:
            return

        section_key = f"{section_name}_frame"
        expanded_key = f"{section_name}_expanded"
        button_key = f"toggle_{section_name}_btn"
        frame = widget.get(section_key)
        if not isinstance(frame, (ttk.Frame, tk.Frame)):
            return

        if expanded:
            frame.pack(fill=tk.BOTH, expand=True)
            if section_name == "result":
                frame.pack_configure(pady=(0, 8))
        else:
            frame.pack_forget()

        widget[expanded_key] = expanded
        button = widget.get(button_key)
        if isinstance(button, ttk.Button):
            if section_name == "result":
                button.configure(text="Ergebnisse ausblenden" if expanded else "Ergebnisse anzeigen")
            else:
                button.configure(text="Log ausblenden" if expanded else "Log anzeigen")

    def _on_column_filter_changed(self, pair_id: str, key: str) -> None:
        self._refresh_visible_rows(pair_id)

    def _refresh_visible_rows(self, pair_id: str) -> None:
        widget = self.sync_pair_widgets.get(pair_id)
        if widget is None:
            return
        result = widget.get("current_result")
        if isinstance(result, CompareResult):
            self.current_operation_pair_id = pair_id
            self._render_result(result)

    def _clear_pair_filters(self, pair_id: str) -> None:
        widget = self.sync_pair_widgets.get(pair_id)
        if widget is None:
            return

        search_var = widget.get("search_var")
        filter_var = widget.get("filter_var")
        column_filters = widget.get("column_filters")

        if isinstance(search_var, tk.StringVar):
            search_var.set("")
        if isinstance(filter_var, tk.StringVar):
            filter_var.set("all")
        if isinstance(column_filters, dict):
            for value in column_filters.values():
                if isinstance(value, tk.StringVar):
                    value.set("")

        self._refresh_visible_rows(pair_id)

    def _clear_pair_selection(self, pair_id: str) -> None:
        widget = self.sync_pair_widgets.get(pair_id)
        if widget is None:
            return
        table = widget.get("sync_tree")
        row_state_by_item = widget.get("row_state_by_item")
        if isinstance(table, ttk.Treeview) and isinstance(row_state_by_item, dict):
            for item_id in list(row_state_by_item.keys()):
                self._set_item_checked_state(pair_id, item_id, False)
            table.selection_remove(table.selection())
        self._update_pair_selection_label(pair_id)

    def _select_visible_rows(self, pair_id: str) -> None:
        widget = self.sync_pair_widgets.get(pair_id)
        if widget is None:
            return
        table = widget.get("sync_tree")
        if not isinstance(table, ttk.Treeview):
            return
        visible = widget.get("all_rows")
        if isinstance(visible, list):
            for item_id in visible:
                self._set_item_checked_state(pair_id, item_id, True)
            table.selection_set(visible)
            self._update_pair_selection_label(pair_id)

    def _update_pair_selection_label(self, pair_id: str) -> None:
        widget = self.sync_pair_widgets.get(pair_id)
        if widget is None:
            return
        selection_var = widget.get("selection_var")
        if not isinstance(selection_var, tk.StringVar):
            return
        selection_var.set(f"Auswahl: {self._checked_count(pair_id)}")

    @staticmethod
    def _entry_state_key(entry: DiffEntry) -> str:
        return f"{entry.diff_type.value}:{entry.relative_path.as_posix()}"

    @staticmethod
    def _default_row_operation(entry: DiffEntry) -> str:
        if entry.diff_type == DiffType.LEFT_ONLY:
            return "L -> R kopieren"
        if entry.diff_type == DiffType.RIGHT_ONLY:
            return "R -> L kopieren"
        return "L -> R aktualisieren"

    @staticmethod
    def _row_operation_options() -> List[str]:
        return [
            "L -> R kopieren",
            "R -> L kopieren",
            "L -> R aktualisieren",
            "Nur links loeschen",
            "Nur rechts loeschen",
            "Ueberspringen",
        ]

    @staticmethod
    def _diff_type_display_label(diff_type: DiffType) -> str:
        if diff_type == DiffType.LEFT_ONLY:
            return "←"
        if diff_type == DiffType.RIGHT_ONLY:
            return "→"
        return "↔"

    @staticmethod
    def _operation_display_label(operation: str) -> str:
        return f"{operation}  ▾"

    def _checked_count(self, pair_id: str) -> int:
        widget = self.sync_pair_widgets.get(pair_id)
        if widget is None:
            return 0
        row_state_by_item = widget.get("row_state_by_item")
        if not isinstance(row_state_by_item, dict):
            return 0
        return sum(1 for state in row_state_by_item.values() if isinstance(state, dict) and bool(state.get("checked", False)))

    def _set_item_checked_state(self, pair_id: str, item_id: str, checked: bool) -> None:
        widget = self.sync_pair_widgets.get(pair_id)
        if widget is None:
            return
        table = widget.get("sync_tree")
        row_state_by_item = widget.get("row_state_by_item")
        row_state_by_key = widget.get("row_state_by_key")
        if not isinstance(table, ttk.Treeview) or not isinstance(row_state_by_item, dict) or not isinstance(row_state_by_key, dict):
            return

        row_state = row_state_by_item.get(item_id)
        if not isinstance(row_state, dict):
            return

        row_state["checked"] = checked
        key = row_state.get("key")
        if isinstance(key, str) and key in row_state_by_key and isinstance(row_state_by_key[key], dict):
            row_state_by_key[key]["checked"] = checked

        values = list(table.item(item_id, "values"))
        if not values:
            return
        values[0] = "☑" if checked else "☐"
        table.item(item_id, values=tuple(values))

    def _set_item_operation(self, pair_id: str, item_id: str, operation: str) -> None:
        widget = self.sync_pair_widgets.get(pair_id)
        if widget is None:
            return
        table = widget.get("sync_tree")
        row_state_by_item = widget.get("row_state_by_item")
        row_state_by_key = widget.get("row_state_by_key")
        if not isinstance(table, ttk.Treeview) or not isinstance(row_state_by_item, dict) or not isinstance(row_state_by_key, dict):
            return

        row_state = row_state_by_item.get(item_id)
        if not isinstance(row_state, dict):
            return

        row_state["operation"] = operation
        key = row_state.get("key")
        if isinstance(key, str) and key in row_state_by_key and isinstance(row_state_by_key[key], dict):
            row_state_by_key[key]["operation"] = operation

        values = list(table.item(item_id, "values"))
        if len(values) < 2:
            return
        values[1] = self._operation_display_label(operation)
        table.item(item_id, values=tuple(values))

    def _advance_item_operation(self, pair_id: str, item_id: str) -> None:
        widget = self.sync_pair_widgets.get(pair_id)
        if widget is None:
            return
        row_state_by_item = widget.get("row_state_by_item")
        if not isinstance(row_state_by_item, dict):
            return
        row_state = row_state_by_item.get(item_id)
        if not isinstance(row_state, dict):
            return

        options = self._row_operation_options()
        if not options:
            return
        current = str(row_state.get("operation", ""))
        try:
            idx = options.index(current)
        except ValueError:
            idx = -1
        next_operation = options[(idx + 1) % len(options)]
        self._set_item_operation(pair_id, item_id, next_operation)

    def _on_sync_tree_click(self, event, pair_id: str):
        widget = self.sync_pair_widgets.get(pair_id)
        if widget is None:
            return None
        table = widget.get("sync_tree")
        if not isinstance(table, ttk.Treeview):
            return None

        row_id = table.identify_row(event.y)
        column_id = table.identify_column(event.x)
        if not row_id:
            return None

        if column_id == "#1":
            row_state_by_item = widget.get("row_state_by_item")
            if isinstance(row_state_by_item, dict):
                row_state = row_state_by_item.get(row_id)
                checked = bool(row_state.get("checked", False)) if isinstance(row_state, dict) else False
                self._set_item_checked_state(pair_id, row_id, not checked)
                self._update_pair_selection_label(pair_id)
                self._update_pair_job_state(pair_id)
            return "break"

        if column_id == "#2":
            bbox = table.bbox(row_id, column_id)
            if bbox:
                col_x, _col_y, col_w, _col_h = bbox
                arrow_zone_start = col_x + max(0, col_w - 22)
                if event.x >= arrow_zone_start:
                    self._show_row_operation_menu(pair_id, row_id, event.x_root, event.y_root)
                else:
                    self._advance_item_operation(pair_id, row_id)
            else:
                self._show_row_operation_menu(pair_id, row_id, event.x_root, event.y_root)
            return "break"

        return None

    def _show_row_operation_menu(self, pair_id: str, item_id: str, x_root: int, y_root: int) -> None:
        widget = self.sync_pair_widgets.get(pair_id)
        if widget is None:
            return
        row_state_by_item = widget.get("row_state_by_item")
        if not isinstance(row_state_by_item, dict):
            return
        row_state = row_state_by_item.get(item_id)
        if not isinstance(row_state, dict):
            return

        current = str(row_state.get("operation", ""))
        menu = tk.Menu(self, tearoff=0)
        selected_var = tk.StringVar(value=current)
        for option in self._row_operation_options():
            menu.add_radiobutton(
                label=option,
                value=option,
                variable=selected_var,
                command=lambda op=option: self._set_item_operation(pair_id, item_id, op),
            )
        try:
            menu.tk_popup(x_root, y_root)
        finally:
            menu.grab_release()

    def _stop_live_snapshot_refresh(self) -> None:
        self.scheduler.stop()

    def _set_pair_state(self, pair_id: str, state: str) -> None:
        widget_info = self.sync_pair_widgets.get(pair_id)
        if widget_info is None:
            return

        colors = {
            "idle": "#6B7280",
            "running": "#1D4ED8",
            "done": "#15803D",
            "error": "#B91C1C",
            "cancelled": "#92400E",
        }
        state_key = state.lower().strip()
        for pair in self.sync_pairs:
            if pair.pair_id == pair_id:
                pair.state = state_key
                break
        badge = widget_info.get("badge")
        if isinstance(badge, tk.Label):
            badge.configure(text=state_key.upper(), bg=colors.get(state_key, "#6B7280"))

        color_bar = widget_info.get("color_bar")
        if isinstance(color_bar, tk.Frame):
            color_bar.configure(bg=colors.get(state_key, "#6B7280"))

        phase_var = widget_info.get("workflow_phase_var")
        if isinstance(phase_var, tk.StringVar):
            phase_map = {
                "idle": "Bereit",
                "running": "Laeuft",
                "done": "Abgeschlossen",
                "error": "Fehler",
                "cancelled": "Abgebrochen",
            }
            phase_var.set(phase_map.get(state_key, state_key.title()))

        frame = widget_info.get("frame")
        name = str(widget_info.get("name", "Task"))
        if isinstance(frame, ttk.Frame):
            compact_name = self._compact_tab_name(name)
            self.sync_notebook.tab(frame, text=f"{compact_name} [{state_key.upper()}]")
            if state_key == "running":
                self.sync_notebook.select(frame)

        self._recompute_overall_progress()
        self._save_sync_pairs_to_settings()

    def _pair_id_from_progress(self, folder_text: str) -> Optional[str]:
        if ":" not in folder_text:
            return None
        prefix = folder_text.split(":", 1)[0].strip()
        for pair in self.sync_pairs:
            if pair.name == prefix:
                return pair.pair_id
        return None

    def _unique_pair_name(self, base_name: str) -> str:
        existing = {pair.name for pair in self.sync_pairs}
        if base_name not in existing:
            return base_name
        counter = 2
        while True:
            candidate = f"{base_name} ({counter})"
            if candidate not in existing:
                return candidate
            counter += 1

    @staticmethod
    def _compact_tab_name(name: str, max_len: int = 18) -> str:
        text = (name or "Sync").strip()
        if len(text) <= max_len:
            return text
        return text[: max_len - 1].rstrip() + "…"

    def _run_ui_action(self, action) -> None:
        try:
            self._set_busy(True)
            action()
        except Exception as exc:  # noqa: BLE001 - display validation errors in GUI.
            self._stop_live_snapshot_refresh()
            self._set_busy(False)
            messagebox.showerror("Fehler", str(exc))

    def _reset_settings(self) -> None:
        if not self._confirm(
            "Einstellungen zuruecksetzen",
            "Alle gespeicherten App-Einstellungen werden geloescht. Fortfahren?",
        ):
            return

        try:
            self.settings_store.reset()
        except Exception as exc:  # noqa: BLE001
            messagebox.showerror("Fehler", f"Reset fehlgeschlagen: {exc}")
            return

        self.settings.clear()
        self._stop_live_snapshot_refresh()
        self.left_var.set("")
        self.right_var.set("")
        self.geometry(self.default_geometry)
        self._apply_theming("light")
        self._set_status("Einstellungen zurueckgesetzt")
        self._append_log("Einstellungen wurden zurueckgesetzt.")

    def _on_close(self) -> None:
        self._stop_live_snapshot_refresh()
        self.runtime_deps.watcher.stop()
        self._stop_sync_agent_server()
        for pair in self.sync_pairs:
            self._sync_pair_model_from_widgets(pair)
        if self.sync_pairs:
            self.settings["left_path"] = self.sync_pairs[0].left
            self.settings["right_path"] = self.sync_pairs[0].right
        self.settings["window_geometry"] = self.geometry()
        self.settings["last_active_pair_id"] = self.last_active_pair_id
        self.settings["agent_port"] = self.agent_port_var.get().strip() or "8765"
        self._save_sync_pairs_to_settings()

        try:
            self.settings_store.save(self.settings)
        except Exception:
            # Do not block app shutdown on a settings write failure.
            pass

        close_method = getattr(self.settings_store, "close", None)
        if callable(close_method):
            try:
                close_method()
            except Exception:
                pass

        self.destroy()

    @staticmethod
    def _confirm(title: str, text: str) -> bool:
        return bool(messagebox.askyesno(title, text))


def main() -> None:
    BootstrapSetup().run()
    parser = build_cli_parser()
    args = parser.parse_args()

    if args.command:
        try:
            exit_code = run_cli(args)
            raise SystemExit(exit_code)
        except Exception as exc:  # noqa: BLE001 - CLI must report all runtime errors.
            print(json.dumps({"error": str(exc)}, ensure_ascii=True))
            raise SystemExit(1)

    app = FolderCompareApp()
    app.mainloop()


if __name__ == "__main__":
    main()
