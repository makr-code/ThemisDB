#!/usr/bin/env python3
"""Tkinter launcher for scripts/quality-gate.ps1."""

from __future__ import annotations

import queue
import subprocess
import threading
import tkinter as tk
from pathlib import Path
from tkinter import filedialog, messagebox, ttk


class QualityGateApp:
    def __init__(self, root: tk.Tk) -> None:
        self.root = root
        self.root.title("ThemisDB Local Quality Gate")
        self.root.geometry("980x680")

        self.repo_root = Path(__file__).resolve().parents[1]
        self.script_path = self.repo_root / "scripts" / "quality-gate.ps1"
        self.process: subprocess.Popen[str] | None = None
        self.log_queue: queue.Queue[str] = queue.Queue()

        self._build_ui()
        self._pump_log_queue()

    def _build_ui(self) -> None:
        frame = ttk.Frame(self.root, padding=12)
        frame.pack(fill=tk.BOTH, expand=True)

        presets = ttk.LabelFrame(frame, text="Presets", padding=10)
        presets.pack(fill=tk.X)

        self.configure_preset_var = tk.StringVar(value="windows-release")
        self.build_preset_var = tk.StringVar(value="windows-release")
        self.test_preset_var = tk.StringVar(value="windows-release")
        self.report_dir_var = tk.StringVar(value="reports/local-quality")

        ttk.Label(presets, text="Configure preset").grid(row=0, column=0, sticky="w", padx=(0, 8), pady=4)
        ttk.Entry(presets, textvariable=self.configure_preset_var, width=32).grid(row=0, column=1, sticky="w", pady=4)

        ttk.Label(presets, text="Build preset").grid(row=0, column=2, sticky="w", padx=(18, 8), pady=4)
        ttk.Entry(presets, textvariable=self.build_preset_var, width=32).grid(row=0, column=3, sticky="w", pady=4)

        ttk.Label(presets, text="Test preset").grid(row=1, column=0, sticky="w", padx=(0, 8), pady=4)
        ttk.Entry(presets, textvariable=self.test_preset_var, width=32).grid(row=1, column=1, sticky="w", pady=4)

        ttk.Label(presets, text="Reports directory").grid(row=1, column=2, sticky="w", padx=(18, 8), pady=4)
        ttk.Entry(presets, textvariable=self.report_dir_var, width=32).grid(row=1, column=3, sticky="w", pady=4)

        options = ttk.LabelFrame(frame, text="Steps", padding=10)
        options.pack(fill=tk.X, pady=(10, 0))

        self.skip_configure_var = tk.BooleanVar(value=False)
        self.skip_build_var = tk.BooleanVar(value=False)
        self.skip_tests_var = tk.BooleanVar(value=False)
        self.skip_clang_tidy_var = tk.BooleanVar(value=False)
        self.skip_cppcheck_var = tk.BooleanVar(value=False)
        self.skip_semgrep_var = tk.BooleanVar(value=False)
        self.skip_codeql_var = tk.BooleanVar(value=False)
        self.skip_doxygen_var = tk.BooleanVar(value=False)
        self.continue_on_error_var = tk.BooleanVar(value=False)

        checks = [
            ("Skip configure", self.skip_configure_var),
            ("Skip build", self.skip_build_var),
            ("Skip tests", self.skip_tests_var),
            ("Skip clang-tidy", self.skip_clang_tidy_var),
            ("Skip cppcheck", self.skip_cppcheck_var),
            ("Skip semgrep", self.skip_semgrep_var),
            ("Skip CodeQL", self.skip_codeql_var),
            ("Skip Doxygen", self.skip_doxygen_var),
            ("Continue on error", self.continue_on_error_var),
        ]

        for idx, (label, var) in enumerate(checks):
            ttk.Checkbutton(options, text=label, variable=var).grid(row=idx // 3, column=idx % 3, sticky="w", padx=(0, 14), pady=2)

        controls = ttk.Frame(frame)
        controls.pack(fill=tk.X, pady=(10, 0))

        self.run_button = ttk.Button(controls, text="Run quality gate", command=self.run_gate)
        self.run_button.pack(side=tk.LEFT)

        self.stop_button = ttk.Button(controls, text="Stop", command=self.stop_gate, state=tk.DISABLED)
        self.stop_button.pack(side=tk.LEFT, padx=(8, 0))

        ttk.Button(controls, text="Open reports folder", command=self.open_reports_folder).pack(side=tk.LEFT, padx=(8, 0))
        ttk.Button(controls, text="Save log as...", command=self.save_log).pack(side=tk.LEFT, padx=(8, 0))

        self.status_var = tk.StringVar(value="Ready")
        ttk.Label(controls, textvariable=self.status_var).pack(side=tk.RIGHT)

        log_frame = ttk.LabelFrame(frame, text="Output", padding=10)
        log_frame.pack(fill=tk.BOTH, expand=True, pady=(10, 0))

        self.output = tk.Text(log_frame, wrap="word", height=22)
        self.output.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        scrollbar = ttk.Scrollbar(log_frame, orient=tk.VERTICAL, command=self.output.yview)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.output.configure(yscrollcommand=scrollbar.set)

    def append_log(self, text: str) -> None:
        self.output.insert(tk.END, text)
        self.output.see(tk.END)

    def build_command(self) -> list[str]:
        command = [
            "powershell",
            "-NoProfile",
            "-ExecutionPolicy",
            "Bypass",
            "-File",
            str(self.script_path),
            "-ConfigurePreset",
            self.configure_preset_var.get().strip(),
            "-BuildPreset",
            self.build_preset_var.get().strip(),
            "-TestPreset",
            self.test_preset_var.get().strip(),
            "-ReportsDir",
            self.report_dir_var.get().strip(),
        ]

        switches = [
            (self.skip_configure_var.get(), "-SkipConfigure"),
            (self.skip_build_var.get(), "-SkipBuild"),
            (self.skip_tests_var.get(), "-SkipTests"),
            (self.skip_clang_tidy_var.get(), "-SkipClangTidy"),
            (self.skip_cppcheck_var.get(), "-SkipCppcheck"),
            (self.skip_semgrep_var.get(), "-SkipSemgrep"),
            (self.skip_codeql_var.get(), "-SkipCodeQL"),
            (self.skip_doxygen_var.get(), "-SkipDoxygen"),
            (self.continue_on_error_var.get(), "-ContinueOnError"),
        ]

        for enabled, flag in switches:
            if enabled:
                command.append(flag)

        return command

    def run_gate(self) -> None:
        if self.process is not None:
            messagebox.showinfo("Quality gate", "A run is already in progress.")
            return

        if not self.script_path.exists():
            messagebox.showerror("Missing script", f"Script not found:\n{self.script_path}")
            return

        command = self.build_command()
        self.output.delete("1.0", tk.END)
        self.append_log(f"Repository: {self.repo_root}\n")
        self.append_log("Command:\n  " + " ".join(command) + "\n\n")

        self.run_button.configure(state=tk.DISABLED)
        self.stop_button.configure(state=tk.NORMAL)
        self.status_var.set("Running")

        self.process = subprocess.Popen(
            command,
            cwd=str(self.repo_root),
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
        )

        threading.Thread(target=self._read_process_output, daemon=True).start()

    def _read_process_output(self) -> None:
        assert self.process is not None
        assert self.process.stdout is not None

        for line in self.process.stdout:
            self.log_queue.put(line)

        code = self.process.wait()
        self.log_queue.put(f"\nProcess finished with exit code {code}.\n")
        self.log_queue.put(f"__EXIT__:{code}")

    def _pump_log_queue(self) -> None:
        try:
            while True:
                msg = self.log_queue.get_nowait()
                if msg.startswith("__EXIT__:"):
                    code = int(msg.split(":", 1)[1])
                    self.process = None
                    self.run_button.configure(state=tk.NORMAL)
                    self.stop_button.configure(state=tk.DISABLED)
                    self.status_var.set("Success" if code == 0 else "Failed")
                else:
                    self.append_log(msg)
        except queue.Empty:
            pass

        self.root.after(100, self._pump_log_queue)

    def stop_gate(self) -> None:
        if self.process is None:
            return

        self.process.terminate()
        self.append_log("\nTermination requested by user.\n")

    def open_reports_folder(self) -> None:
        reports = self.repo_root / self.report_dir_var.get().strip()
        reports.mkdir(parents=True, exist_ok=True)

        try:
            subprocess.Popen(["explorer", str(reports)])
        except OSError as exc:
            messagebox.showerror("Open reports folder", str(exc))

    def save_log(self) -> None:
        target = filedialog.asksaveasfilename(
            title="Save output log",
            defaultextension=".log",
            filetypes=[("Log files", "*.log"), ("Text files", "*.txt"), ("All files", "*.*")],
        )
        if not target:
            return

        text = self.output.get("1.0", tk.END)
        Path(target).write_text(text, encoding="utf-8")


def main() -> None:
    root = tk.Tk()
    app = QualityGateApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
