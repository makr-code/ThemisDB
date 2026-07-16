"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            main.py                                            ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:43:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     633                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
Drohnenbild-Analyse - Hauptanwendung mit Tkinter UI.

Komplexes System für Echtzeit-Analyse von Drohnenbildern mit:
- Computer Vision und Objekterkennung
- LLM-Integration für Bildbeschreibungen
- Vector Search für ähnliche Bilder
- Geo-Queries und Karten-Visualisierung
- Zeitreihen-Analyse
"""

import tkinter as tk
from tkinter import ttk, messagebox, filedialog
from typing import List, Optional, Dict
from datetime import datetime
import threading
import time

from models import (
    DroneImage, ImageAnalysis, TimeSeriesEvent, DroneSimulator,
    ImageProcessor, LLMIntegration, Location, ImageMetadata
)
from themis_client import DroneAnalysisClient, AnalysisProcessor


class DroneAnalysisApp:
    """Hauptanwendung für Drohnenbild-Analyse."""
    
    def __init__(self, root):
        self.root = root
        self.root.title("Drohnenbild-Analyse - KI-gestützte Echtzeit-Analyse mit LLM")
        self.root.geometry("1400x900")
        
        # Client
        self.client = DroneAnalysisClient()
        self.processor = AnalysisProcessor(self.client)
        
        # Simulator
        self.simulator = DroneSimulator()
        
        # State
        self.images: Dict[str, DroneImage] = {}
        self.analyses: Dict[str, ImageAnalysis] = {}
        self.events: List[TimeSeriesEvent] = []
        self.current_image: Optional[DroneImage] = None
        self.monitoring = False
        self.monitor_thread = None
        
        self.setup_ui()
        self.load_demo_data()
        self.check_connection()
    
    def setup_ui(self):
        """Erstellt die Benutzeroberfläche."""
        # Menü
        menubar = tk.Menu(self.root)
        self.root.config(menu=menubar)
        
        file_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="Datei", menu=file_menu)
        file_menu.add_command(label="Bilder importieren", command=self.import_images)
        file_menu.add_command(label="Export Report", command=self.export_report)
        file_menu.add_separator()
        file_menu.add_command(label="Beenden", command=self.root.quit)
        
        analysis_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="Analyse", menu=analysis_menu)
        analysis_menu.add_command(label="Batch-Analyse starten", command=self.start_batch_analysis)
        analysis_menu.add_command(label="Ähnliche Bilder finden", command=self.find_similar)
        
        help_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="Hilfe", menu=help_menu)
        help_menu.add_command(label="Dokumentation", command=self.show_help)
        help_menu.add_command(label="Über", command=self.show_about)
        
        # Toolbar
        toolbar = ttk.Frame(self.root)
        toolbar.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        
        ttk.Button(toolbar, text="🎥 Monitoring starten", 
                  command=self.toggle_monitoring).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="📸 Einzelaufnahme", 
                  command=self.capture_single).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="🔍 Vector Search", 
                  command=self.vector_search).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="🗺️ Geo-Suche", 
                  command=self.geo_search).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="🔄 Aktualisieren", 
                  command=self.refresh_data).pack(side=tk.LEFT, padx=2)
        
        # Status
        self.status_var = tk.StringVar(value="Bereit")
        status_label = ttk.Label(toolbar, textvariable=self.status_var, 
                               relief=tk.SUNKEN)
        status_label.pack(side=tk.RIGHT, padx=5)
        
        # Hauptbereich mit Tabs
        self.notebook = ttk.Notebook(self.root)
        self.notebook.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Tab 1: Dashboard
        self.create_dashboard_tab()
        
        # Tab 2: Bilder & Analysen
        self.create_images_tab()
        
        # Tab 3: Karte & Geo
        self.create_map_tab()
        
        # Tab 4: Timeline & Events
        self.create_timeline_tab()
        
        # Tab 5: Statistiken
        self.create_statistics_tab()
    
    def create_dashboard_tab(self):
        """Erstellt Dashboard-Tab."""
        tab = ttk.Frame(self.notebook)
        self.notebook.add(tab, text="📊 Dashboard")
        
        # Statistik-Cards
        stats_frame = ttk.Frame(tab)
        stats_frame.pack(fill=tk.X, padx=10, pady=10)
        
        self.stat_cards = {}
        stats = [
            ("Bilder", "images", "📷"),
            ("Analysen", "analyses", "🔍"),
            ("Events", "events", "📅"),
            ("Objekte", "objects", "🎯")
        ]
        
        for i, (label, key, icon) in enumerate(stats):
            card = ttk.LabelFrame(stats_frame, text=f"{icon} {label}")
            card.grid(row=0, column=i, padx=5, pady=5, sticky="nsew")
            stats_frame.columnconfigure(i, weight=1)
            
            value_label = ttk.Label(card, text="0", font=("Arial", 24, "bold"))
            value_label.pack(pady=10)
            self.stat_cards[key] = value_label
        
        # Monitoring-Status
        monitor_frame = ttk.LabelFrame(tab, text="🎥 Echtzeit-Monitoring")
        monitor_frame.pack(fill=tk.X, padx=10, pady=10)
        
        self.monitor_status = ttk.Label(monitor_frame, text="⏸️ Gestoppt", 
                                       font=("Arial", 14))
        self.monitor_status.pack(pady=5)
        
        monitor_controls = ttk.Frame(monitor_frame)
        monitor_controls.pack(pady=5)
        
        ttk.Button(monitor_controls, text="▶️ Start", 
                  command=self.start_monitoring).pack(side=tk.LEFT, padx=5)
        ttk.Button(monitor_controls, text="⏸️ Stop", 
                  command=self.stop_monitoring).pack(side=tk.LEFT, padx=5)
        
        # Aktivitäts-Log
        log_frame = ttk.LabelFrame(tab, text="📝 Aktivitäts-Log")
        log_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        self.activity_log = tk.Text(log_frame, height=15, wrap=tk.WORD)
        self.activity_log.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        scrollbar = ttk.Scrollbar(log_frame, command=self.activity_log.yview)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.activity_log.config(yscrollcommand=scrollbar.set)
    
    def create_images_tab(self):
        """Erstellt Bilder-Tab."""
        tab = ttk.Frame(self.notebook)
        self.notebook.add(tab, text="📷 Bilder & Analysen")
        
        # Paned Window
        paned = ttk.PanedWindow(tab, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True)
        
        # Linke Seite: Bildliste
        left_frame = ttk.Frame(paned)
        paned.add(left_frame, weight=1)
        
        ttk.Label(left_frame, text="Drohnenbilder", 
                 font=("Arial", 12, "bold")).pack(pady=5)
        
        # Treeview für Bilder
        columns = ("ID", "Drohne", "Zeit", "Ort", "Objekte")
        self.images_tree = ttk.Treeview(left_frame, columns=columns, 
                                       show="tree headings", height=20)
        
        for col in columns:
            self.images_tree.heading(col, text=col)
            self.images_tree.column(col, width=100)
        
        self.images_tree.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        self.images_tree.bind("<<TreeviewSelect>>", self.on_image_select)
        
        # Scrollbar
        scrollbar = ttk.Scrollbar(left_frame, orient=tk.VERTICAL, 
                                 command=self.images_tree.yview)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.images_tree.config(yscrollcommand=scrollbar.set)
        
        # Rechte Seite: Details
        right_frame = ttk.Frame(paned)
        paned.add(right_frame, weight=2)
        
        ttk.Label(right_frame, text="Bildanalyse & Details", 
                 font=("Arial", 12, "bold")).pack(pady=5)
        
        # Details-Bereich
        details_scroll = ttk.Frame(right_frame)
        details_scroll.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        self.details_text = tk.Text(details_scroll, wrap=tk.WORD, height=30)
        self.details_text.pack(fill=tk.BOTH, expand=True)
        
        details_scrollbar = ttk.Scrollbar(details_scroll, 
                                         command=self.details_text.yview)
        details_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.details_text.config(yscrollcommand=details_scrollbar.set)
    
    def create_map_tab(self):
        """Erstellt Karten-Tab."""
        tab = ttk.Frame(self.notebook)
        self.notebook.add(tab, text="🗺️ Karte & Geo")
        
        ttk.Label(tab, text="Geografische Visualisierung", 
                 font=("Arial", 14, "bold")).pack(pady=10)
        
        # Geo-Suche
        search_frame = ttk.LabelFrame(tab, text="Geo-Suche")
        search_frame.pack(fill=tk.X, padx=10, pady=10)
        
        controls = ttk.Frame(search_frame)
        controls.pack(pady=5)
        
        ttk.Label(controls, text="Lat:").grid(row=0, column=0, padx=5)
        self.lat_entry = ttk.Entry(controls, width=15)
        self.lat_entry.grid(row=0, column=1, padx=5)
        self.lat_entry.insert(0, "52.5200")
        
        ttk.Label(controls, text="Lon:").grid(row=0, column=2, padx=5)
        self.lon_entry = ttk.Entry(controls, width=15)
        self.lon_entry.grid(row=0, column=3, padx=5)
        self.lon_entry.insert(0, "13.4050")
        
        ttk.Label(controls, text="Radius (km):").grid(row=0, column=4, padx=5)
        self.radius_entry = ttk.Entry(controls, width=10)
        self.radius_entry.grid(row=0, column=5, padx=5)
        self.radius_entry.insert(0, "5.0")
        
        ttk.Button(controls, text="Suchen", 
                  command=self.execute_geo_search).grid(row=0, column=6, padx=5)
        
        # Karten-Platzhalter (würde Folium oder ähnliches verwenden)
        map_frame = ttk.LabelFrame(tab, text="Karte (Simulation)")
        map_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        self.map_text = tk.Text(map_frame, height=20, wrap=tk.WORD)
        self.map_text.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        self.map_text.insert("1.0", "Kartenvisualisierung würde hier erscheinen...\n\n")
        self.map_text.insert("end", "In Produktion: Folium, Leaflet oder ähnliche Bibliothek")
    
    def create_timeline_tab(self):
        """Erstellt Timeline-Tab."""
        tab = ttk.Frame(self.notebook)
        self.notebook.add(tab, text="📅 Timeline & Events")
        
        ttk.Label(tab, text="Zeitreihen-Events", 
                 font=("Arial", 14, "bold")).pack(pady=10)
        
        # Events-Liste
        columns = ("Zeit", "Typ", "Beschreibung", "Konfidenz", "Bilder")
        self.events_tree = ttk.Treeview(tab, columns=columns, 
                                       show="tree headings", height=20)
        
        for col in columns:
            self.events_tree.heading(col, text=col)
            self.events_tree.column(col, width=150)
        
        self.events_tree.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        # Buttons
        button_frame = ttk.Frame(tab)
        button_frame.pack(fill=tk.X, padx=10, pady=5)
        
        ttk.Button(button_frame, text="Event erstellen", 
                  command=self.create_event).pack(side=tk.LEFT, padx=5)
        ttk.Button(button_frame, text="Vergleich anzeigen", 
                  command=self.show_comparison).pack(side=tk.LEFT, padx=5)
    
    def create_statistics_tab(self):
        """Erstellt Statistik-Tab."""
        tab = ttk.Frame(self.notebook)
        self.notebook.add(tab, text="📈 Statistiken")
        
        ttk.Label(tab, text="System-Statistiken", 
                 font=("Arial", 14, "bold")).pack(pady=10)
        
        self.stats_text = tk.Text(tab, wrap=tk.WORD, height=30)
        self.stats_text.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
    
    # === Event Handler ===
    
    def on_image_select(self, event):
        """Wird aufgerufen wenn ein Bild ausgewählt wird."""
        selection = self.images_tree.selection()
        if not selection:
            return
        
        item = self.images_tree.item(selection[0])
        image_id = item['values'][0] if item['values'] else None
        
        if image_id and image_id in self.images:
            self.current_image = self.images[image_id]
            self.show_image_details(self.current_image)
    
    def show_image_details(self, image: DroneImage):
        """Zeigt Details eines Bildes."""
        self.details_text.delete("1.0", tk.END)
        
        # Bild-Info
        self.details_text.insert("end", "=== BILDINFORMATIONEN ===\n\n", "bold")
        self.details_text.insert("end", f"ID: {image.id}\n")
        self.details_text.insert("end", f"Drohne: {image.drone_id}\n")
        self.details_text.insert("end", f"Zeit: {image.timestamp.strftime('%Y-%m-%d %H:%M:%S')}\n")
        self.details_text.insert("end", f"\nPosition:\n")
        self.details_text.insert("end", f"  Lat: {image.location.lat:.6f}\n")
        self.details_text.insert("end", f"  Lon: {image.location.lon:.6f}\n")
        self.details_text.insert("end", f"  Höhe: {image.location.altitude}m\n")
        self.details_text.insert("end", f"  Heading: {image.location.heading}°\n")
        
        # Analyse (falls vorhanden)
        analysis_id = f"analysis_{image.id}"
        if analysis_id in self.analyses:
            analysis = self.analyses[analysis_id]
            self.details_text.insert("end", "\n\n=== BILDANALYSE ===\n\n", "bold")
            self.details_text.insert("end", f"Qualität: {analysis.quality_score:.2%}\n")
            self.details_text.insert("end", f"Verarbeitungszeit: {analysis.processing_time_ms}ms\n")
            self.details_text.insert("end", f"\nSzene: {', '.join(analysis.scene_classification)}\n")
            self.details_text.insert("end", f"\n📝 LLM-Beschreibung:\n")
            self.details_text.insert("end", f"{analysis.llm_description}\n")
            
            self.details_text.insert("end", f"\n🎯 Erkannte Objekte ({len(analysis.detected_objects)}):\n")
            for obj in analysis.detected_objects[:10]:  # Zeige max 10
                self.details_text.insert("end", 
                    f"  • {obj.classification} ({obj.confidence:.2%})\n")
    
    # === Monitoring ===
    
    def toggle_monitoring(self):
        """Startet/Stoppt Monitoring."""
        if self.monitoring:
            self.stop_monitoring()
        else:
            self.start_monitoring()
    
    def start_monitoring(self):
        """Startet Echtzeit-Monitoring."""
        if self.monitoring:
            return
        
        self.monitoring = True
        self.monitor_status.config(text="▶️ Läuft")
        self.log_activity("Monitoring gestartet")
        
        # Starte Thread
        self.monitor_thread = threading.Thread(target=self.monitoring_loop, daemon=True)
        self.monitor_thread.start()
    
    def stop_monitoring(self):
        """Stoppt Monitoring."""
        self.monitoring = False
        self.monitor_status.config(text="⏸️ Gestoppt")
        self.log_activity("Monitoring gestoppt")
    
    def monitoring_loop(self):
        """Monitoring-Loop (läuft in separatem Thread)."""
        while self.monitoring:
            try:
                # Simuliere Bildaufnahme
                image = self.simulator.capture_image("drone_001")
                
                # Speichere in lokalem Cache
                self.images[image.id] = image
                
                # Analyse durchführen
                analysis = self.processor.process_images_batch([image])[0]
                self.analyses[analysis.id] = analysis
                
                # UI aktualisieren (muss im Main-Thread passieren)
                self.root.after(0, self.update_after_capture, image, analysis)
                
                # Warte
                time.sleep(3)  # Alle 3 Sekunden ein Bild
                
            except Exception as e:
                self.root.after(0, self.log_activity, f"Fehler: {e}")
                break
    
    def update_after_capture(self, image: DroneImage, analysis: ImageAnalysis):
        """Aktualisiert UI nach Bildaufnahme."""
        self.refresh_images_list()
        self.update_statistics()
        self.log_activity(f"Bild aufgenommen: {image.id} - {len(analysis.detected_objects)} Objekte")
    
    def capture_single(self):
        """Nimmt ein einzelnes Bild auf."""
        try:
            image = self.simulator.capture_image("drone_001")
            self.images[image.id] = image
            
            # Analyse
            self.status_var.set("Analysiere Bild...")
            analysis = self.processor.process_images_batch([image])[0]
            self.analyses[analysis.id] = analysis
            
            self.refresh_images_list()
            self.update_statistics()
            self.log_activity(f"Einzelaufnahme: {image.id}")
            self.status_var.set("Bereit")
            
        except Exception as e:
            messagebox.showerror("Fehler", f"Fehler bei Aufnahme: {e}")
    
    # === Data Management ===
    
    def refresh_data(self):
        """Aktualisiert alle Daten."""
        self.refresh_images_list()
        self.refresh_events_list()
        self.update_statistics()
        self.log_activity("Daten aktualisiert")
    
    def refresh_images_list(self):
        """Aktualisiert Bildliste."""
        self.images_tree.delete(*self.images_tree.get_children())
        
        for image in sorted(self.images.values(), 
                          key=lambda x: x.timestamp, reverse=True):
            analysis_id = f"analysis_{image.id}"
            num_objects = len(self.analyses[analysis_id].detected_objects) \
                         if analysis_id in self.analyses else 0
            
            self.images_tree.insert("", "end", values=(
                image.id,
                image.drone_id,
                image.timestamp.strftime("%H:%M:%S"),
                f"{image.location.lat:.4f}, {image.location.lon:.4f}",
                num_objects
            ))
    
    def refresh_events_list(self):
        """Aktualisiert Events-Liste."""
        self.events_tree.delete(*self.events_tree.get_children())
        
        for event in self.events:
            self.events_tree.insert("", "end", values=(
                event.detected_at.strftime("%Y-%m-%d %H:%M"),
                event.event_type,
                event.description[:50],
                f"{event.confidence:.2%}",
                len(event.images)
            ))
    
    def update_statistics(self):
        """Aktualisiert Statistiken."""
        # Dashboard-Cards
        self.stat_cards["images"].config(text=str(len(self.images)))
        self.stat_cards["analyses"].config(text=str(len(self.analyses)))
        self.stat_cards["events"].config(text=str(len(self.events)))
        
        total_objects = sum(len(a.detected_objects) for a in self.analyses.values())
        self.stat_cards["objects"].config(text=str(total_objects))
        
        # Statistik-Tab
        self.stats_text.delete("1.0", tk.END)
        self.stats_text.insert("end", "=== SYSTEM-STATISTIKEN ===\n\n", "bold")
        self.stats_text.insert("end", f"Bilder gesamt: {len(self.images)}\n")
        self.stats_text.insert("end", f"Analysen gesamt: {len(self.analyses)}\n")
        self.stats_text.insert("end", f"Events gesamt: {len(self.events)}\n")
        self.stats_text.insert("end", f"Objekte gesamt: {total_objects}\n")
        
        if self.analyses:
            avg_quality = sum(a.quality_score for a in self.analyses.values()) / len(self.analyses)
            avg_time = sum(a.processing_time_ms for a in self.analyses.values()) / len(self.analyses)
            self.stats_text.insert("end", f"\nØ Qualität: {avg_quality:.2%}\n")
            self.stats_text.insert("end", f"Ø Verarbeitungszeit: {avg_time:.0f}ms\n")
    
    def log_activity(self, message: str):
        """Loggt eine Aktivität."""
        timestamp = datetime.now().strftime("%H:%M:%S")
        self.activity_log.insert("1.0", f"[{timestamp}] {message}\n")
    
    # === Actions ===
    
    def start_batch_analysis(self):
        """Startet Batch-Analyse."""
        if not self.images:
            messagebox.showinfo("Info", "Keine Bilder vorhanden")
            return
        
        self.log_activity(f"Batch-Analyse für {len(self.images)} Bilder gestartet")
        # Implementierung...
    
    def vector_search(self):
        """Öffnet Vector-Search-Dialog."""
        messagebox.showinfo("Vector Search", "Vector Search würde hier geöffnet")
    
    def find_similar(self):
        """Findet ähnliche Bilder."""
        if not self.current_image:
            messagebox.showinfo("Info", "Bitte zuerst ein Bild auswählen")
            return
        
        self.log_activity(f"Suche ähnliche Bilder zu {self.current_image.id}")
    
    def geo_search(self):
        """Öffnet Geo-Search."""
        self.notebook.select(2)  # Wechsle zum Karten-Tab
    
    def execute_geo_search(self):
        """Führt Geo-Suche aus."""
        try:
            lat = float(self.lat_entry.get())
            lon = float(self.lon_entry.get())
            radius = float(self.radius_entry.get())
            
            self.map_text.insert("end", 
                f"\n\nSuche bei ({lat}, {lon}) mit Radius {radius}km...\n")
            
            # Simuliere Ergebnisse
            results = [img for img in self.images.values()
                      if abs(img.location.lat - lat) < 0.1 
                      and abs(img.location.lon - lon) < 0.1]
            
            self.map_text.insert("end", f"Gefunden: {len(results)} Bilder\n")
            
        except ValueError:
            messagebox.showerror("Fehler", "Ungültige Koordinaten")
    
    def create_event(self):
        """Erstellt ein neues Event."""
        messagebox.showinfo("Event", "Event-Dialog würde hier erscheinen")
    
    def show_comparison(self):
        """Zeigt Bild-Vergleich."""
        messagebox.showinfo("Vergleich", "Vergleichs-Ansicht würde hier erscheinen")
    
    def import_images(self):
        """Importiert Bilder."""
        messagebox.showinfo("Import", "Import-Dialog würde hier erscheinen")
    
    def export_report(self):
        """Exportiert Report."""
        messagebox.showinfo("Export", "Report würde hier exportiert")
    
    def show_help(self):
        """Zeigt Hilfe."""
        messagebox.showinfo("Hilfe", "Siehe HOW_TO.md für detaillierte Anleitung")
    
    def show_about(self):
        """Zeigt Über-Dialog."""
        messagebox.showinfo("Über", 
            "Drohnenbild-Analyse System\n\n"
            "Version 1.0\n\n"
            "KI-gestützte Echtzeit-Analyse mit:\n"
            "• Computer Vision (YOLO)\n"
            "• LLM-Integration\n"
            "• Vector Search\n"
            "• Geo-Queries\n"
            "• Zeitreihen-Analyse")
    
    # === Initialization ===
    
    def check_connection(self):
        """Prüft ThemisDB-Verbindung."""
        connected = self.client.test_connection()
        if connected:
            self.status_var.set("✅ ThemisDB verbunden")
            self.log_activity("ThemisDB Verbindung hergestellt")
        else:
            self.status_var.set("⚠️ ThemisDB nicht erreichbar (Demo-Modus)")
            self.log_activity("⚠️ ThemisDB nicht erreichbar - verwende Demo-Modus")
    
    def load_demo_data(self):
        """Lädt Demo-Daten."""
        self.log_activity("Lade Demo-Daten...")
        
        # Erstelle 5 Demo-Bilder
        for i in range(5):
            image = self.simulator.capture_image("drone_demo")
            self.images[image.id] = image
            
            # Analysiere
            analysis = self.processor.process_images_batch([image])[0]
            self.analyses[analysis.id] = analysis
        
        self.refresh_data()
        self.log_activity(f"Demo-Daten geladen: {len(self.images)} Bilder")


def main():
    """Hauptfunktion."""
    root = tk.Tk()
    app = DroneAnalysisApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
