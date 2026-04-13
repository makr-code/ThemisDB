"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            main.py                                            ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:19:24                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     488                                            ║
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
IoT Sensor Network - ThemisDB Beispiel #09

Echtzeit IoT-Datenerfassung mit Complex Event Processing (CEP) und Anomalie-Erkennung.

Features:
- IoT-Geräte Verwaltung
- Echtzeit Sensor-Datenerfassung
- CEP-Engine mit konfigurierbaren Regeln
- Statistische Anomalie-Erkennung
- Alert-Management
"""

import tkinter as tk
from tkinter import ttk, messagebox, simpledialog
from datetime import datetime
import uuid
import threading
import time
from typing import Dict, List, Optional

from models import (
    Device, SensorReading, CEPRule, Alert, Anomaly,
    DeviceType, DeviceStatus, AlertSeverity, RuleOperator,
    SensorSimulator, AnomalyDetector, CEPEngine
)
from themis_client import IoTClient


class IoTSensorNetworkApp:
    """Hauptanwendung für IoT Sensor Network."""
    
    def __init__(self, root):
        self.root = root
        self.root.title("ThemisDB - IoT Sensor Network mit CEP & Anomalie-Erkennung")
        self.root.geometry("1200x800")
        
        # Client und Engines
        self.client = IoTClient()
        self.cep_engine = CEPEngine()
        self.anomaly_detector = AnomalyDetector(threshold=3.0)
        
        # Cache
        self.devices: Dict[str, Device] = {}
        self.simulators: Dict[str, SensorSimulator] = {}
        self.alerts: List[Alert] = []
        
        # Monitoring Thread
        self.monitoring_active = False
        self.monitoring_thread = None
        self.time_step = 0
        
        self.setup_ui()
        self.load_demo_data()
    
    def setup_ui(self):
        """Erstellt die Benutzeroberfläche."""
        # Notebook für Tabs
        self.notebook = ttk.Notebook(self.root)
        self.notebook.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Tabs
        self.setup_dashboard_tab()
        self.setup_devices_tab()
        self.setup_rules_tab()
        self.setup_alerts_tab()
    
    def setup_dashboard_tab(self):
        """Dashboard Tab."""
        tab = ttk.Frame(self.notebook)
        self.notebook.add(tab, text="📊 Dashboard")
        
        # Control Panel
        control_frame = ttk.LabelFrame(tab, text="Monitoring Control", padding=10)
        control_frame.pack(fill=tk.X, padx=10, pady=5)
        
        self.start_button = ttk.Button(control_frame, text="▶️ Start Monitoring",
                                       command=self.start_monitoring)
        self.start_button.pack(side=tk.LEFT, padx=5)
        
        self.stop_button = ttk.Button(control_frame, text="⏸️ Stop Monitoring",
                                      command=self.stop_monitoring, state=tk.DISABLED)
        self.stop_button.pack(side=tk.LEFT, padx=5)
        
        self.status_label = ttk.Label(control_frame, text="Status: Stopped")
        self.status_label.pack(side=tk.LEFT, padx=20)
        
        # Statistics
        stats_frame = ttk.LabelFrame(tab, text="Statistiken", padding=10)
        stats_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)
        
        # 4 Stat Cards
        cards_frame = ttk.Frame(stats_frame)
        cards_frame.pack(fill=tk.BOTH, expand=True)
        
        # Card 1: Devices
        card1 = ttk.LabelFrame(cards_frame, text="IoT-Geräte", padding=10)
        card1.grid(row=0, column=0, padx=5, pady=5, sticky="nsew")
        self.devices_label = ttk.Label(card1, text="0 Geräte\n0 Aktiv", font=("Arial", 16, "bold"))
        self.devices_label.pack()
        
        # Card 2: Readings
        card2 = ttk.LabelFrame(cards_frame, text="Messwerte", padding=10)
        card2.grid(row=0, column=1, padx=5, pady=5, sticky="nsew")
        self.readings_label = ttk.Label(card2, text="0 Readings", font=("Arial", 16, "bold"))
        self.readings_label.pack()
        
        # Card 3: Rules
        card3 = ttk.LabelFrame(cards_frame, text="CEP-Regeln", padding=10)
        card3.grid(row=1, column=0, padx=5, pady=5, sticky="nsew")
        self.rules_label = ttk.Label(card3, text="0 Regeln\n0 Aktiv", font=("Arial", 16, "bold"))
        self.rules_label.pack()
        
        # Card 4: Alerts
        card4 = ttk.LabelFrame(cards_frame, text="Alerts", padding=10)
        card4.grid(row=1, column=1, padx=5, pady=5, sticky="nsew")
        self.alerts_label = ttk.Label(card4, text="0 Alerts\n0 Unbestätigt", font=("Arial", 16, "bold"))
        self.alerts_label.pack()
        
        for i in range(2):
            cards_frame.grid_rowconfigure(i, weight=1)
            cards_frame.grid_columnconfigure(i, weight=1)
    
    def setup_devices_tab(self):
        """Devices Tab."""
        tab = ttk.Frame(self.notebook)
        self.notebook.add(tab, text="🔌 Geräte")
        
        # Toolbar
        toolbar = ttk.Frame(tab)
        toolbar.pack(fill=tk.X, padx=5, pady=5)
        
        ttk.Button(toolbar, text="➕ Gerät hinzufügen",
                  command=self.add_device).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="🔄 Aktualisieren",
                  command=self.refresh_devices).pack(side=tk.LEFT, padx=2)
        
        # Devices Tree
        tree_frame = ttk.Frame(tab)
        tree_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        columns = ("name", "type", "location", "status", "battery", "last_seen")
        self.devices_tree = ttk.Treeview(tree_frame, columns=columns, show="tree headings")
        
        self.devices_tree.heading("#0", text="ID")
        self.devices_tree.heading("name", text="Name")
        self.devices_tree.heading("type", text="Typ")
        self.devices_tree.heading("location", text="Standort")
        self.devices_tree.heading("status", text="Status")
        self.devices_tree.heading("battery", text="Batterie")
        self.devices_tree.heading("last_seen", text="Zuletzt gesehen")
        
        self.devices_tree.column("#0", width=100)
        self.devices_tree.column("name", width=150)
        self.devices_tree.column("type", width=100)
        self.devices_tree.column("location", width=150)
        self.devices_tree.column("status", width=80)
        self.devices_tree.column("battery", width=80)
        self.devices_tree.column("last_seen", width=150)
        
        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.devices_tree.yview)
        self.devices_tree.configure(yscrollcommand=scrollbar.set)
        
        self.devices_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
    
    def setup_rules_tab(self):
        """CEP Rules Tab."""
        tab = ttk.Frame(self.notebook)
        self.notebook.add(tab, text="⚙️ CEP-Regeln")
        
        # Toolbar
        toolbar = ttk.Frame(tab)
        toolbar.pack(fill=tk.X, padx=5, pady=5)
        
        ttk.Button(toolbar, text="➕ Regel hinzufügen",
                  command=self.add_rule).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="🔄 Aktualisieren",
                  command=self.refresh_rules).pack(side=tk.LEFT, padx=2)
        
        # Rules Tree
        tree_frame = ttk.Frame(tab)
        tree_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        columns = ("name", "device", "operator", "threshold", "severity", "enabled")
        self.rules_tree = ttk.Treeview(tree_frame, columns=columns, show="tree headings")
        
        self.rules_tree.heading("#0", text="ID")
        self.rules_tree.heading("name", text="Name")
        self.rules_tree.heading("device", text="Gerät")
        self.rules_tree.heading("operator", text="Operator")
        self.rules_tree.heading("threshold", text="Schwellwert")
        self.rules_tree.heading("severity", text="Schweregrad")
        self.rules_tree.heading("enabled", text="Aktiv")
        
        self.rules_tree.column("#0", width=100)
        self.rules_tree.column("name", width=200)
        self.rules_tree.column("device", width=150)
        self.rules_tree.column("operator", width=80)
        self.rules_tree.column("threshold", width=100)
        self.rules_tree.column("severity", width=100)
        self.rules_tree.column("enabled", width=80)
        
        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.rules_tree.yview)
        self.rules_tree.configure(yscrollcommand=scrollbar.set)
        
        self.rules_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
    
    def setup_alerts_tab(self):
        """Alerts Tab."""
        tab = ttk.Frame(self.notebook)
        self.notebook.add(tab, text="🚨 Alerts")
        
        # Toolbar
        toolbar = ttk.Frame(tab)
        toolbar.pack(fill=tk.X, padx=5, pady=5)
        
        ttk.Button(toolbar, text="✅ Bestätigen",
                  command=self.acknowledge_selected_alert).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="🔄 Aktualisieren",
                  command=self.refresh_alerts).pack(side=tk.LEFT, padx=2)
        
        # Filter
        ttk.Label(toolbar, text="Filter:").pack(side=tk.LEFT, padx=10)
        self.alert_filter_var = tk.StringVar(value="all")
        ttk.Radiobutton(toolbar, text="Alle", variable=self.alert_filter_var,
                       value="all", command=self.refresh_alerts).pack(side=tk.LEFT)
        ttk.Radiobutton(toolbar, text="Unbestätigt", variable=self.alert_filter_var,
                       value="unacknowledged", command=self.refresh_alerts).pack(side=tk.LEFT)
        
        # Alerts Tree
        tree_frame = ttk.Frame(tab)
        tree_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        columns = ("timestamp", "severity", "device", "message", "value", "acknowledged")
        self.alerts_tree = ttk.Treeview(tree_frame, columns=columns, show="tree headings")
        
        self.alerts_tree.heading("#0", text="ID")
        self.alerts_tree.heading("timestamp", text="Zeit")
        self.alerts_tree.heading("severity", text="Schwere")
        self.alerts_tree.heading("device", text="Gerät")
        self.alerts_tree.heading("message", text="Nachricht")
        self.alerts_tree.heading("value", text="Wert")
        self.alerts_tree.heading("acknowledged", text="Bestätigt")
        
        self.alerts_tree.column("#0", width=80)
        self.alerts_tree.column("timestamp", width=150)
        self.alerts_tree.column("severity", width=80)
        self.alerts_tree.column("device", width=100)
        self.alerts_tree.column("message", width=350)
        self.alerts_tree.column("value", width=80)
        self.alerts_tree.column("acknowledged", width=80)
        
        scrollbar = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.alerts_tree.yview)
        self.alerts_tree.configure(yscrollcommand=scrollbar.set)
        
        self.alerts_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
    
    def load_demo_data(self):
        """Lädt Demo-Daten."""
        # Demo Devices
        devices = [
            Device("temp-01", "Temperatur Sensor Raum 1", DeviceType.TEMPERATURE,
                   "Büro A", DeviceStatus.ACTIVE, "1.2.0", 95.0),
            Device("hum-01", "Luftfeuchtigkeit Sensor Raum 1", DeviceType.HUMIDITY,
                   "Büro A", DeviceStatus.ACTIVE, "1.1.0", 88.0),
            Device("press-01", "Druck Sensor Außen", DeviceType.PRESSURE,
                   "Dach", DeviceStatus.ACTIVE, "1.0.5", 72.0),
        ]
        
        for device in devices:
            self.devices[device.device_id] = device
            self.simulators[device.device_id] = SensorSimulator(device.device_type)
            self.client.create_device(device)
        
        # Demo CEP Rules
        rules = [
            CEPRule("rule-01", "Temperatur zu hoch", "temp-01",
                   RuleOperator.GREATER_THAN, 28.0, AlertSeverity.WARNING,
                   description="Warnung wenn Temperatur > 28°C"),
            CEPRule("rule-02", "Luftfeuchtigkeit niedrig", "hum-01",
                   RuleOperator.LESS_THAN, 40.0, AlertSeverity.WARNING,
                   description="Warnung wenn Luftfeuchtigkeit < 40%"),
        ]
        
        for rule in rules:
            self.cep_engine.add_rule(rule)
            self.client.create_rule(rule)
        
        self.refresh_all()
    
    def start_monitoring(self):
        """Startet das Monitoring."""
        if not self.monitoring_active:
            self.monitoring_active = True
            self.start_button.config(state=tk.DISABLED)
            self.stop_button.config(state=tk.NORMAL)
            self.status_label.config(text="Status: Running ▶️")
            
            self.monitoring_thread = threading.Thread(target=self.monitoring_loop, daemon=True)
            self.monitoring_thread.start()
    
    def stop_monitoring(self):
        """Stoppt das Monitoring."""
        self.monitoring_active = False
        self.start_button.config(state=tk.NORMAL)
        self.stop_button.config(state=tk.DISABLED)
        self.status_label.config(text="Status: Stopped ⏸️")
    
    def monitoring_loop(self):
        """Monitoring Thread Loop."""
        while self.monitoring_active:
            for device_id, device in self.devices.items():
                if device.status != DeviceStatus.ACTIVE:
                    continue
                
                # Simuliere Sensor-Wert
                simulator = self.simulators.get(device_id)
                if simulator:
                    value = simulator.get_value(self.time_step)
                    unit = simulator.get_unit()
                    
                    # Erstelle Reading
                    reading = SensorReading(
                        reading_id=str(uuid.uuid4()),
                        device_id=device_id,
                        timestamp=datetime.now(),
                        value=value,
                        unit=unit
                    )
                    self.client.create_reading(reading)
                    
                    # CEP Processing
                    alerts = self.cep_engine.process_reading(reading)
                    for alert in alerts:
                        self.alerts.append(alert)
                        self.client.create_alert(alert)
                    
                    # Anomaly Detection
                    self.anomaly_detector.add_reading(device_id, value)
                    anomaly = self.anomaly_detector.detect_anomaly(device_id, value)
                    if anomaly:
                        self.client.create_anomaly(anomaly)
                    
                    # Update Device
                    device.last_seen = datetime.now()
                    device.battery_level = max(0, device.battery_level - 0.01)
                    self.client.update_device(device)
            
            self.time_step += 1
            
            # UI Update
            self.root.after(0, self.update_dashboard_stats)
            
            time.sleep(1)
    
    def add_device(self):
        """Fügt ein neues Gerät hinzu."""
        messagebox.showinfo("Demo", "In der Demo vorausgefüllt mit 3 Sensoren")
    
    def add_rule(self):
        """Fügt eine neue Regel hinzu."""
        messagebox.showinfo("Demo", "In der Demo vorausgefüllt mit 2 Regeln")
    
    def acknowledge_selected_alert(self):
        """Bestätigt den ausgewählten Alert."""
        selection = self.alerts_tree.selection()
        if not selection:
            messagebox.showwarning("Warnung", "Bitte wählen Sie einen Alert aus")
            return
        
        alert_id = self.alerts_tree.item(selection[0])["text"]
        if self.client.acknowledge_alert(alert_id):
            messagebox.showinfo("Erfolg", "Alert bestätigt")
            self.refresh_alerts()
        else:
            messagebox.showerror("Fehler", "Alert konnte nicht bestätigt werden")
    
    def refresh_devices(self):
        """Aktualisiert die Geräteliste."""
        self.devices_tree.delete(*self.devices_tree.get_children())
        
        for device in self.devices.values():
            status_icon = "🟢" if device.status == DeviceStatus.ACTIVE else "🔴"
            last_seen = device.last_seen.strftime("%H:%M:%S") if device.last_seen else "Nie"
            
            self.devices_tree.insert("", tk.END, text=device.device_id,
                                    values=(device.name, device.device_type.value,
                                           device.location, f"{status_icon} {device.status.value}",
                                           f"{device.battery_level:.0f}%", last_seen))
    
    def refresh_rules(self):
        """Aktualisiert die Regelliste."""
        self.rules_tree.delete(*self.rules_tree.get_children())
        
        for device_id in self.cep_engine.rules:
            for rule in self.cep_engine.rules[device_id]:
                device_name = self.devices.get(rule.device_id, Device("", "", DeviceType.CUSTOM, "")).name
                enabled_icon = "✅" if rule.enabled else "❌"
                
                self.rules_tree.insert("", tk.END, text=rule.rule_id,
                                      values=(rule.name, device_name, rule.operator.value,
                                             rule.threshold, rule.severity.value,
                                             enabled_icon))
    
    def refresh_alerts(self):
        """Aktualisiert die Alert-Liste."""
        self.alerts_tree.delete(*self.alerts_tree.get_children())
        
        filter_val = self.alert_filter_var.get()
        
        for alert in reversed(self.alerts[-100:]):  # Letzte 100 Alerts
            if filter_val == "unacknowledged" and alert.acknowledged:
                continue
            
            severity_icon = {"info": "🟢", "warning": "🟡", "critical": "🔴"}.get(alert.severity.value, "⚪")
            ack_icon = "✅" if alert.acknowledged else "❌"
            
            self.alerts_tree.insert("", tk.END, text=alert.alert_id[:8],
                                   values=(alert.timestamp.strftime("%H:%M:%S"),
                                          f"{severity_icon} {alert.severity.value}",
                                          alert.device_id, alert.message,
                                          f"{alert.value:.2f}", ack_icon))
    
    def update_dashboard_stats(self):
        """Aktualisiert Dashboard-Statistiken."""
        # Devices
        total_devices = len(self.devices)
        active_devices = sum(1 for d in self.devices.values() if d.status == DeviceStatus.ACTIVE)
        self.devices_label.config(text=f"{total_devices} Geräte\n{active_devices} Aktiv")
        
        # Readings
        total_readings = len(self.client.list_readings())
        self.readings_label.config(text=f"{total_readings} Readings")
        
        # Rules
        total_rules = sum(len(rules) for rules in self.cep_engine.rules.values())
        active_rules = sum(1 for rules in self.cep_engine.rules.values()
                          for rule in rules if rule.enabled)
        self.rules_label.config(text=f"{total_rules} Regeln\n{active_rules} Aktiv")
        
        # Alerts
        total_alerts = len(self.alerts)
        unack_alerts = sum(1 for a in self.alerts if not a.acknowledged)
        self.alerts_label.config(text=f"{total_alerts} Alerts\n{unack_alerts} Unbestätigt")
    
    def refresh_all(self):
        """Aktualisiert alle Ansichten."""
        self.refresh_devices()
        self.refresh_rules()
        self.refresh_alerts()
        self.update_dashboard_stats()


def main():
    """Hauptfunktion."""
    root = tk.Tk()
    app = IoTSensorNetworkApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
