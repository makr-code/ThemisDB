"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            ingest_graph_phi3_gui.py                           ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-04-15 07:24:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     251                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3db23979ed  2026-04-06  feat: Enable HTTP server by default + LLM API routing + c... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

#!/usr/bin/env python3
"""Tkinter GUI for ingest_graph_phi3 library with live process feedback."""

from __future__ import annotations

import queue
import threading
import tkinter as tk
from pathlib import Path
from tkinter import filedialog, messagebox, ttk
from typing import Any, Dict

from ingest_graph_phi3_lib import IngestionConfig, IngestionStats, run_ingestion


class IngestApp(tk.Tk):
    def __init__(self) -> None:
        super().__init__()
        self.title("ThemisDB Phi3 Ingestion GUI")
        self.geometry("1020x700")

        self._event_queue: "queue.Queue[tuple[str, Dict[str, Any], IngestionStats | None]]" = queue.Queue()
        self._worker: threading.Thread | None = None
        self._running = False

        self._build_ui()
        self.after(150, self._poll_queue)

    def _build_ui(self) -> None:
        frm = ttk.Frame(self, padding=12)
        frm.pack(fill=tk.BOTH, expand=True)

        cfg = ttk.LabelFrame(frm, text="Konfiguration", padding=10)
        cfg.pack(fill=tk.X)

        self.source_var = tk.StringVar(value=r"Y:\data")
        self.url_var = tk.StringVar(value="http://127.0.0.1:8765")
        self.token_var = tk.StringVar(value="")
        self.model_var = tk.StringVar(value="phi3")
        self.max_files_var = tk.StringVar(value="0")
        self.dry_run_var = tk.BooleanVar(value=True)
        self.skip_load_var = tk.BooleanVar(value=True)

        ttk.Label(cfg, text="Quelle").grid(row=0, column=0, sticky=tk.W, pady=4)
        ttk.Entry(cfg, textvariable=self.source_var, width=72).grid(row=0, column=1, sticky=tk.EW, pady=4)
        ttk.Button(cfg, text="...", width=4, command=self._pick_folder).grid(row=0, column=2, padx=6)

        ttk.Label(cfg, text="Themis URL").grid(row=1, column=0, sticky=tk.W, pady=4)
        ttk.Entry(cfg, textvariable=self.url_var, width=72).grid(row=1, column=1, sticky=tk.EW, pady=4)

        ttk.Label(cfg, text="Bearer Token").grid(row=2, column=0, sticky=tk.W, pady=4)
        ttk.Entry(cfg, textvariable=self.token_var, width=72, show="*").grid(row=2, column=1, sticky=tk.EW, pady=4)

        ttk.Label(cfg, text="Model ID").grid(row=3, column=0, sticky=tk.W, pady=4)
        ttk.Entry(cfg, textvariable=self.model_var, width=24).grid(row=3, column=1, sticky=tk.W, pady=4)

        ttk.Label(cfg, text="Max Files (0=alle)").grid(row=4, column=0, sticky=tk.W, pady=4)
        ttk.Entry(cfg, textvariable=self.max_files_var, width=24).grid(row=4, column=1, sticky=tk.W, pady=4)

        ttk.Checkbutton(cfg, text="Dry Run", variable=self.dry_run_var).grid(row=5, column=0, sticky=tk.W, pady=4)
        ttk.Checkbutton(cfg, text="Model Load ueberspringen", variable=self.skip_load_var).grid(row=5, column=1, sticky=tk.W, pady=4)

        cfg.columnconfigure(1, weight=1)

        actions = ttk.Frame(frm)
        actions.pack(fill=tk.X, pady=(10, 8))
        self.start_btn = ttk.Button(actions, text="Start", command=self._start)
        self.start_btn.pack(side=tk.LEFT)
        self.stop_hint = ttk.Label(actions, text="Abbruch: Fenster schliessen oder Lauf beenden lassen")
        self.stop_hint.pack(side=tk.LEFT, padx=10)

        progress = ttk.LabelFrame(frm, text="Fortschritt", padding=10)
        progress.pack(fill=tk.X)

        self.progress_var = tk.DoubleVar(value=0.0)
        self.progress = ttk.Progressbar(progress, variable=self.progress_var, mode="determinate", maximum=100)
        self.progress.pack(fill=tk.X)

        self.status_var = tk.StringVar(value="Bereit")
        ttk.Label(progress, textvariable=self.status_var).pack(anchor=tk.W, pady=(8, 0))

        counters = ttk.Frame(progress)
        counters.pack(fill=tk.X, pady=(6, 0))

        self.scanned_var = tk.StringVar(value="0")
        self.processed_var = tk.StringVar(value="0")
        self.skipped_var = tk.StringVar(value="0")
        self.failed_var = tk.StringVar(value="0")
        self.nodes_var = tk.StringVar(value="0")
        self.edges_var = tk.StringVar(value="0")

        labels = [
            ("Scanned", self.scanned_var),
            ("Processed", self.processed_var),
            ("Skipped", self.skipped_var),
            ("Failed", self.failed_var),
            ("Nodes", self.nodes_var),
            ("Edges", self.edges_var),
        ]
        for idx, (title, var) in enumerate(labels):
            ttk.Label(counters, text=f"{title}:").grid(row=0, column=idx * 2, sticky=tk.W, padx=(0, 4))
            ttk.Label(counters, textvariable=var).grid(row=0, column=idx * 2 + 1, sticky=tk.W, padx=(0, 12))

        log_frame = ttk.LabelFrame(frm, text="Prozess-Feedback", padding=10)
        log_frame.pack(fill=tk.BOTH, expand=True, pady=(10, 0))

        self.log_widget = tk.Text(log_frame, height=18, wrap=tk.WORD)
        self.log_widget.pack(fill=tk.BOTH, expand=True)
        self.log_widget.configure(state=tk.DISABLED)

    def _pick_folder(self) -> None:
        selected = filedialog.askdirectory(initialdir=self.source_var.get() or ".")
        if selected:
            self.source_var.set(selected)

    def _append_log(self, line: str) -> None:
        self.log_widget.configure(state=tk.NORMAL)
        self.log_widget.insert(tk.END, line + "\n")
        self.log_widget.see(tk.END)
        self.log_widget.configure(state=tk.DISABLED)

    def _build_config(self) -> IngestionConfig:
        max_files = int(self.max_files_var.get().strip() or "0")
        return IngestionConfig(
            source=self.source_var.get().strip(),
            themis_url=self.url_var.get().strip(),
            bearer_token=self.token_var.get().strip(),
            model_id=self.model_var.get().strip() or "phi3",
            dry_run=self.dry_run_var.get(),
            skip_model_load=self.skip_load_var.get(),
            max_files=max_files,
        )

    def _start(self) -> None:
        if self._running:
            return

        src = Path(self.source_var.get().strip())
        if not src.exists() or not src.is_dir():
            messagebox.showerror("Ungueltiger Pfad", f"Quellordner existiert nicht: {src}")
            return

        try:
            config = self._build_config()
        except ValueError as ex:
            messagebox.showerror("Ungueltige Eingabe", str(ex))
            return

        self._running = True
        self.start_btn.configure(state=tk.DISABLED)
        self.progress_var.set(0)
        self.status_var.set("Lauf gestartet...")
        self._append_log("=== Ingestion gestartet ===")

        def progress_cb(event: str, data: Dict[str, Any], stats: IngestionStats) -> None:
            self._event_queue.put((event, data, stats))

        def log_cb(message: str) -> None:
            self._event_queue.put(("log", {"message": message}, None))

        def worker() -> None:
            try:
                stats = run_ingestion(config, progress_cb=progress_cb, log_cb=log_cb)
                self._event_queue.put(("finished", {"ok": stats.write_failed == 0}, stats))
            except Exception as ex:
                self._event_queue.put(("error", {"error": str(ex)}, None))

        self._worker = threading.Thread(target=worker, daemon=True)
        self._worker.start()

    def _apply_stats(self, stats: IngestionStats) -> None:
        self.scanned_var.set(str(stats.scanned))
        self.processed_var.set(str(stats.processed))
        self.skipped_var.set(str(stats.skipped))
        self.failed_var.set(str(stats.failed))
        self.nodes_var.set(str(stats.nodes))
        self.edges_var.set(str(stats.edges))

        total = stats.total_candidates
        if total > 0:
            progress = min(100.0, (stats.scanned / total) * 100.0)
            self.progress_var.set(progress)

    def _poll_queue(self) -> None:
        try:
            while True:
                event, data, stats = self._event_queue.get_nowait()
                if event == "log":
                    self._append_log(data.get("message", ""))
                    continue

                if stats is not None:
                    self._apply_stats(stats)

                if event == "scan_start":
                    self.status_var.set(f"Scan gestartet: {data.get('total_candidates', 0)} Kandidaten")
                elif event == "file_processed":
                    self.status_var.set(f"Verarbeitet: {data.get('file', '?')}")
                elif event == "file_failed":
                    self.status_var.set(f"Fehler bei: {data.get('file', '?')}")
                    self._append_log(f"ERROR {data.get('file', '?')}: {data.get('error', '')}")
                elif event == "done":
                    self.status_var.set("Verarbeitung abgeschlossen")
                elif event == "finished":
                    ok = bool(data.get("ok", False))
                    self._running = False
                    self.start_btn.configure(state=tk.NORMAL)
                    self.progress_var.set(100.0)
                    self.status_var.set("Fertig" if ok else "Fertig mit Schreibfehlern")
                    self._append_log("=== Ingestion beendet ===")
                elif event == "error":
                    self._running = False
                    self.start_btn.configure(state=tk.NORMAL)
                    self.status_var.set("Abbruch mit Fehler")
                    self._append_log(f"FATAL: {data.get('error', '')}")
                    messagebox.showerror("Ingestion Fehler", data.get("error", "Unbekannter Fehler"))
        except queue.Empty:
            pass
        finally:
            self.after(150, self._poll_queue)


def main() -> None:
    app = IngestApp()
    app.mainloop()


if __name__ == "__main__":
    main()
