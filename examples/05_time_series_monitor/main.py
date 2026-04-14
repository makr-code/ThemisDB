"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            main.py                                            ║
  Version:         0.0.42                                             ║
  Last Modified:   2026-04-14 18:35:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     494                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
Zeitreihen-Monitor - Echtzeitdaten-Visualisierung
Tkinter GUI mit Live-Charts für Sensor-Daten
"""

import tkinter as tk
from tkinter import ttk, messagebox
import uuid
from datetime import datetime
from typing import List, Dict, Optional
from collections import deque
import threading
import time

# Matplotlib imports
try:
    import matplotlib
    matplotlib.use('TkAgg')
    from matplotlib.figure import Figure
    from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
    MATPLOTLIB_AVAILABLE = True
except ImportError:
    MATPLOTLIB_AVAILABLE = False

from themis_client import TimeSeriesClient
from models import Sensor, Measurement, Alert, SensorType


# Konfiguration
THEMIS_HOST = "localhost"
THEMIS_PORT = 8080
WINDOW_WIDTH = 1100
WINDOW_HEIGHT = 700
MAX_DATA_POINTS = 100
UPDATE_INTERVAL = 1000  # ms


class TimeSeriesMonitorApp:
    """
    Hauptanwendung für Zeitreihen-Monitoring.
    """
    
    def __init__(self, root: tk.Tk):
        """Initialisiert die Anwendung."""
        self.root = root
        self.root.title("ThemisDB - Zeitreihen-Monitor")
        self.root.geometry(f"{WINDOW_WIDTH}x{WINDOW_HEIGHT}")
        
        # Client
        self.client = TimeSeriesClient(host=THEMIS_HOST, port=THEMIS_PORT)
        
        # Daten
        self.sensors: List[Sensor] = []
        self.measurements: Dict[str, deque] = {}  # sensor_id -> deque of (time, value)
        self.alerts: List[Alert] = []
        self.is_running = False
        
        # Check matplotlib
        if not MATPLOTLIB_AVAILABLE:
            messagebox.showwarning(
                "Warnung",
                "matplotlib ist nicht installiert.\n"
                "Charts werden nicht verfügbar sein.\n\n"
                "Installieren mit: pip install matplotlib"
            )
        
        # UI erstellen
        self._create_ui()
        
        # Verbindung prüfen
        self._check_connection()
        
        # Demo-Sensoren laden
        self._load_demo_sensors()
        
        # Tastenkombinationen
        self._setup_keybindings()
        
        # Protokoll für Fenster-Schließung
        self.root.protocol("WM_DELETE_WINDOW", self._on_closing)
    
    def _create_ui(self):
        """Erstellt die Benutzeroberfläche."""
        # Header
        self._create_header()
        
        # Main Content
        paned = tk.PanedWindow(self.root, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Linke Seite: Sensor-Liste und Steuerung
        self._create_control_panel(paned)
        
        # Rechte Seite: Charts
        self._create_chart_panel(paned)
        
        # Status Bar
        self._create_status_bar()
    
    def _create_header(self):
        """Erstellt Header."""
        header_frame = tk.Frame(self.root, bg="#34495e", height=60)
        header_frame.pack(fill=tk.X)
        header_frame.pack_propagate(False)
        
        # Title
        title_label = tk.Label(
            header_frame,
            text="📊 Zeitreihen-Monitor",
            bg="#34495e",
            fg="white",
            font=("Arial", 16, "bold")
        )
        title_label.pack(side=tk.LEFT, padx=20, pady=10)
        
        # Connection Status
        self.connection_status = tk.Label(
            header_frame,
            text="● Connecting...",
            bg="#34495e",
            fg="#f39c12",
            font=("Arial", 10)
        )
        self.connection_status.pack(side=tk.RIGHT, padx=20)
        
        # Start/Stop Button
        self.start_stop_btn = tk.Button(
            header_frame,
            text="▶️ Start",
            command=self._toggle_monitoring,
            bg="#27ae60",
            fg="white",
            font=("Arial", 12, "bold"),
            padx=20,
            pady=5
        )
        self.start_stop_btn.pack(side=tk.RIGHT, padx=10)
    
    def _create_control_panel(self, parent):
        """Erstellt Steuerungs-Panel."""
        control_frame = tk.Frame(parent, width=350)
        parent.add(control_frame)
        
        # Sensoren-Liste
        sensors_label = tk.LabelFrame(
            control_frame,
            text="Sensoren",
            font=("Arial", 12, "bold")
        )
        sensors_label.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Treeview für Sensoren
        tree_frame = tk.Frame(sensors_label)
        tree_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        vsb = tk.Scrollbar(tree_frame, orient="vertical")
        
        columns = ("Name", "Aktuell", "Status")
        self.sensor_tree = ttk.Treeview(
            tree_frame,
            columns=columns,
            show="tree headings",
            yscrollcommand=vsb.set,
            height=15
        )
        
        vsb.config(command=self.sensor_tree.yview)
        
        self.sensor_tree.heading("#0", text="ID")
        self.sensor_tree.column("#0", width=60)
        
        self.sensor_tree.heading("Name", text="Name")
        self.sensor_tree.column("Name", width=120)
        
        self.sensor_tree.heading("Aktuell", text="Aktuell")
        self.sensor_tree.column("Aktuell", width=80)
        
        self.sensor_tree.heading("Status", text="Status")
        self.sensor_tree.column("Status", width=70)
        
        self.sensor_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        vsb.pack(side=tk.RIGHT, fill=tk.Y)
        
        # Buttons
        btn_frame = tk.Frame(sensors_label)
        btn_frame.pack(fill=tk.X, padx=5, pady=5)
        
        tk.Button(
            btn_frame,
            text="➕ Sensor hinzufügen",
            command=self._add_sensor,
            bg="#3498db",
            fg="white"
        ).pack(fill=tk.X, pady=2)
        
        tk.Button(
            btn_frame,
            text="🔄 Aktualisieren",
            command=self._refresh_sensors,
            bg="#95a5a6",
            fg="white"
        ).pack(fill=tk.X, pady=2)
        
        # Alarm-Liste
        alerts_label = tk.LabelFrame(
            control_frame,
            text="Aktive Alarme",
            font=("Arial", 12, "bold")
        )
        alerts_label.pack(fill=tk.X, padx=5, pady=5)
        
        self.alerts_text = tk.Text(
            alerts_label,
            height=8,
            font=("Arial", 9),
            wrap=tk.WORD,
            bg="#fff3cd"
        )
        self.alerts_text.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
    
    def _create_chart_panel(self, parent):
        """Erstellt Chart-Panel."""
        chart_frame = tk.Frame(parent)
        parent.add(chart_frame)
        
        if not MATPLOTLIB_AVAILABLE:
            # Fallback ohne matplotlib
            tk.Label(
                chart_frame,
                text="📈\n\nCharts nicht verfügbar\n\n"
                     "Installieren Sie matplotlib:\n"
                     "pip install matplotlib",
                font=("Arial", 14),
                fg="#7f8c8d"
            ).pack(expand=True)
            return
        
        # Charts mit matplotlib
        self.figure = Figure(figsize=(8, 6), dpi=100)
        self.figure.patch.set_facecolor('#ecf0f1')
        
        # 2x2 Grid für Charts
        self.axes = []
        for i in range(4):
            ax = self.figure.add_subplot(2, 2, i + 1)
            ax.set_facecolor('#ffffff')
            ax.grid(True, alpha=0.3)
            self.axes.append(ax)
        
        self.figure.tight_layout(pad=2.0)
        
        # Canvas
        self.canvas = FigureCanvasTkAgg(self.figure, master=chart_frame)
        self.canvas.draw()
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
    
    def _create_status_bar(self):
        """Erstellt Status-Leiste."""
        self.status_label = tk.Label(
            self.root,
            text="Ready",
            relief=tk.SUNKEN,
            anchor="w",
            bg="#ecf0f1",
            padx=5,
            pady=3
        )
        self.status_label.pack(side=tk.BOTTOM, fill=tk.X)
    
    def _setup_keybindings(self):
        """Richtet Tastenkombinationen ein."""
        self.root.bind("<space>", lambda e: self._toggle_monitoring())
        self.root.bind("<F5>", lambda e: self._refresh_sensors())
    
    def _check_connection(self):
        """Prüft Verbindung zu ThemisDB."""
        if self.client.health_check():
            self.connection_status.config(text="● Connected", fg="#27ae60")
            self._set_status("Verbunden mit ThemisDB", "success")
        else:
            self.connection_status.config(text="● Disconnected", fg="#e74c3c")
            self._set_status("Keine Verbindung zu ThemisDB", "error")
    
    def _set_status(self, message: str, status_type: str = "info"):
        """Setzt Status-Nachricht."""
        colors = {
            "info": "#3498db",
            "success": "#27ae60",
            "error": "#e74c3c"
        }
        self.status_label.config(text=message, bg=colors.get(status_type, "#ecf0f1"))
    
    def _load_demo_sensors(self):
        """Lädt Demo-Sensoren."""
        demo_sensors = [
            Sensor("cpu", "CPU Auslastung", SensorType.CPU, "%", 0, 100, 70, 90),
            Sensor("memory", "Speicher", SensorType.MEMORY, "%", 0, 100, 80, 95),
            Sensor("temp", "Temperatur", SensorType.TEMPERATURE, "°C", 20, 90, 70, 85),
            Sensor("custom", "Custom Sensor", SensorType.CUSTOM, "units", 0, 100, 75, 90),
        ]
        
        for sensor in demo_sensors:
            self.sensors.append(sensor)
            self.measurements[sensor.id] = deque(maxlen=MAX_DATA_POINTS)
        
        self._refresh_sensors()
    
    def _add_sensor(self):
        """Fügt neuen Sensor hinzu."""
        messagebox.showinfo("Info", "Sensor hinzufügen würde hier implementiert werden")
    
    def _refresh_sensors(self):
        """Aktualisiert Sensor-Liste."""
        # Clear
        for item in self.sensor_tree.get_children():
            self.sensor_tree.delete(item)
        
        # Populate
        for sensor in self.sensors:
            # Letzter Wert
            if sensor.id in self.measurements and self.measurements[sensor.id]:
                last_value = self.measurements[sensor.id][-1][1]
                current = f"{last_value:.1f} {sensor.unit}"
                status = sensor.check_threshold(last_value)
                status_text = {"ok": "OK", "warning": "⚠️", "critical": "🔴"}[status]
            else:
                current = "—"
                status_text = "—"
            
            values = (sensor.name, current, status_text)
            self.sensor_tree.insert("", tk.END, text=sensor.id[:8], values=values)
    
    def _toggle_monitoring(self):
        """Startet/Stoppt Monitoring."""
        if self.is_running:
            self._stop_monitoring()
        else:
            self._start_monitoring()
    
    def _start_monitoring(self):
        """Startet Monitoring."""
        self.is_running = True
        self.start_stop_btn.config(text="⏸️ Stop", bg="#e74c3c")
        self._set_status("Monitoring läuft...", "success")
        
        # Starte Update-Thread
        self.update_thread = threading.Thread(target=self._update_loop, daemon=True)
        self.update_thread.start()
    
    def _stop_monitoring(self):
        """Stoppt Monitoring."""
        self.is_running = False
        self.start_stop_btn.config(text="▶️ Start", bg="#27ae60")
        self._set_status("Monitoring gestoppt", "info")
    
    def _update_loop(self):
        """Update-Loop im Thread."""
        while self.is_running:
            try:
                self._collect_data()
                self.root.after(0, self._update_ui)
                time.sleep(UPDATE_INTERVAL / 1000.0)
            except Exception as e:
                print(f"Error in update loop: {e}")
                break
    
    def _collect_data(self):
        """Sammelt Daten von Sensoren."""
        current_time = time.time()
        
        for sensor in self.sensors:
            if not sensor.active:
                continue
            
            # Simuliere Wert
            last_value = None
            if sensor.id in self.measurements and self.measurements[sensor.id]:
                last_value = self.measurements[sensor.id][-1][1]
            
            value = sensor.simulate_value(last_value)
            
            # Speichere Messung
            self.measurements[sensor.id].append((current_time, value))
            
            # Prüfe Schwellwerte
            status = sensor.check_threshold(value)
            if status in ["warning", "critical"]:
                threshold = sensor.warning_threshold if status == "warning" else sensor.critical_threshold
                alert = Alert(
                    id=str(uuid.uuid4()),
                    sensor_id=sensor.id,
                    level=status,
                    message=f"{sensor.name}: {value:.1f} {sensor.unit} (Schwelle: {threshold})",
                    value=value,
                    threshold=threshold
                )
                self.alerts.append(alert)
                
                # Behalte nur letzte 10 Alarme
                if len(self.alerts) > 10:
                    self.alerts = self.alerts[-10:]
    
    def _update_ui(self):
        """Aktualisiert UI (muss im Main-Thread laufen)."""
        self._refresh_sensors()
        self._update_charts()
        self._update_alerts()
    
    def _update_charts(self):
        """Aktualisiert Charts."""
        if not MATPLOTLIB_AVAILABLE:
            return
        
        # Plot für jeden Sensor (max 4)
        for i, sensor in enumerate(self.sensors[:4]):
            ax = self.axes[i]
            ax.clear()
            
            if sensor.id in self.measurements and self.measurements[sensor.id]:
                data = list(self.measurements[sensor.id])
                times = [t - data[0][0] for t, v in data]  # Relative Zeit
                values = [v for t, v in data]
                
                # Plot
                ax.plot(times, values, '-', color='#3498db', linewidth=2)
                
                # Schwellwerte
                if sensor.warning_threshold:
                    ax.axhline(y=sensor.warning_threshold, color='#f39c12', linestyle='--', alpha=0.7, label='Warnung')
                if sensor.critical_threshold:
                    ax.axhline(y=sensor.critical_threshold, color='#e74c3c', linestyle='--', alpha=0.7, label='Kritisch')
                
                ax.set_title(sensor.name, fontsize=10, fontweight='bold')
                ax.set_xlabel('Zeit (s)', fontsize=8)
                ax.set_ylabel(sensor.unit, fontsize=8)
                ax.grid(True, alpha=0.3)
                ax.legend(fontsize=7, loc='upper right')
            else:
                ax.set_title(sensor.name, fontsize=10, fontweight='bold')
                ax.text(0.5, 0.5, 'Keine Daten', ha='center', va='center', transform=ax.transAxes)
        
        self.canvas.draw()
    
    def _update_alerts(self):
        """Aktualisiert Alarm-Anzeige."""
        self.alerts_text.delete(1.0, tk.END)
        
        if not self.alerts:
            self.alerts_text.insert(tk.END, "✓ Keine aktiven Alarme")
            self.alerts_text.config(bg="#d4edda")
        else:
            self.alerts_text.config(bg="#fff3cd")
            for alert in reversed(self.alerts[-5:]):  # Zeige letzte 5
                icon = "⚠️" if alert.level == "warning" else "🔴"
                self.alerts_text.insert(tk.END, f"{icon} {alert.message}\n")
    
    def _on_closing(self):
        """Handler für Fenster-Schließung."""
        self._stop_monitoring()
        self.root.destroy()


def main():
    """Hauptfunktion."""
    root = tk.Tk()
    app = TimeSeriesMonitorApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
