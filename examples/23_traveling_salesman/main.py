"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            main.py                                            ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:05:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     774                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
ThemisDB TSP Example - Tkinter GUI Application
Demonstriert die Lösung des Traveling Salesman Problems mit ThemisDB
"""

import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext, filedialog
import uuid
import json
from typing import List, Optional

try:
    import matplotlib
    matplotlib.use('TkAgg')
    from matplotlib.figure import Figure
    from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
    MATPLOTLIB_AVAILABLE = True
except ImportError:
    MATPLOTLIB_AVAILABLE = False

from models import City, Route
from tsp_algorithms import TSPSolver
from themis_client import ThemisDBClient


# Konfiguration
THEMIS_HOST = "localhost"
THEMIS_PORT = 8080
WINDOW_WIDTH = 1400
WINDOW_HEIGHT = 900


class TSPApp:
    """Hauptanwendung für TSP-Beispiel."""
    
    def __init__(self, root: tk.Tk):
        """Initialisiert die Anwendung."""
        self.root = root
        self.root.title("ThemisDB - Traveling Salesman Problem")
        self.root.geometry(f"{WINDOW_WIDTH}x{WINDOW_HEIGHT}")
        
        # ThemisDB Client
        self.client = ThemisDBClient(host=THEMIS_HOST, port=THEMIS_PORT)
        
        # Daten
        self.cities: List[City] = []
        self.current_route: Optional[Route] = None
        self.routes_history: List[Route] = []
        
        # Check Matplotlib
        if not MATPLOTLIB_AVAILABLE:
            messagebox.showwarning(
                "Warnung",
                "Matplotlib nicht gefunden!\n\n"
                "Visualisierung wird nicht verfügbar sein.\n\n"
                "Installieren mit: pip install matplotlib"
            )
        
        # UI erstellen
        self._create_ui()
        
        # Verbindung prüfen
        self._check_connection()
    
    def _create_ui(self):
        """Erstellt die Benutzeroberfläche."""
        # Header
        self._create_header()
        
        # Main Content
        paned = tk.PanedWindow(self.root, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Linke Seite: Städte und Steuerung
        self._create_control_panel(paned)
        
        # Rechte Seite: Visualisierung
        self._create_visualization_panel(paned)
        
        # Status Bar
        self._create_status_bar()
    
    def _create_header(self):
        """Erstellt Header."""
        header_frame = tk.Frame(self.root, bg="#2c3e50", height=70)
        header_frame.pack(fill=tk.X)
        header_frame.pack_propagate(False)
        
        # Title
        title_label = tk.Label(
            header_frame,
            text="🗺️ Traveling Salesman Problem",
            bg="#2c3e50",
            fg="white",
            font=("Arial", 18, "bold")
        )
        title_label.pack(side=tk.LEFT, padx=20, pady=15)
        
        # Connection Status
        self.connection_status = tk.Label(
            header_frame,
            text="● Connecting...",
            bg="#2c3e50",
            fg="#f39c12",
            font=("Arial", 10)
        )
        self.connection_status.pack(side=tk.RIGHT, padx=20)
        
        # Subtitle
        subtitle = tk.Label(
            header_frame,
            text="Routenoptimierung mit ThemisDB Graph-Features",
            bg="#2c3e50",
            fg="#ecf0f1",
            font=("Arial", 10)
        )
        subtitle.pack(side=tk.LEFT, padx=(0, 20))
    
    def _create_control_panel(self, parent):
        """Erstellt Kontroll-Panel."""
        control_frame = tk.Frame(parent, width=450)
        parent.add(control_frame)
        
        # Städte-Verwaltung
        cities_frame = tk.LabelFrame(
            control_frame,
            text="📍 Städte",
            font=("Arial", 12, "bold"),
            padx=10,
            pady=10
        )
        cities_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Buttons
        btn_frame = tk.Frame(cities_frame)
        btn_frame.pack(fill=tk.X, pady=(0, 5))
        
        tk.Button(
            btn_frame,
            text="➕ Hinzufügen",
            command=self._add_city,
            bg="#27ae60",
            fg="white",
            font=("Arial", 9)
        ).pack(side=tk.LEFT, padx=2)
        
        tk.Button(
            btn_frame,
            text="✏️ Bearbeiten",
            command=self._edit_city,
            bg="#3498db",
            fg="white",
            font=("Arial", 9)
        ).pack(side=tk.LEFT, padx=2)
        
        tk.Button(
            btn_frame,
            text="🗑️ Löschen",
            command=self._delete_city,
            bg="#e74c3c",
            fg="white",
            font=("Arial", 9)
        ).pack(side=tk.LEFT, padx=2)
        
        tk.Button(
            btn_frame,
            text="📦 Demo laden",
            command=self._load_demo_data,
            bg="#9b59b6",
            fg="white",
            font=("Arial", 9)
        ).pack(side=tk.LEFT, padx=2)
        
        # Listbox
        list_frame = tk.Frame(cities_frame)
        list_frame.pack(fill=tk.BOTH, expand=True)
        
        scrollbar = tk.Scrollbar(list_frame)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        self.cities_listbox = tk.Listbox(
            list_frame,
            yscrollcommand=scrollbar.set,
            font=("Arial", 10),
            selectmode=tk.SINGLE
        )
        self.cities_listbox.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.config(command=self.cities_listbox.yview)
        
        # Algorithmen
        algo_frame = tk.LabelFrame(
            control_frame,
            text="🔍 Algorithmen",
            font=("Arial", 12, "bold"),
            padx=10,
            pady=10
        )
        algo_frame.pack(fill=tk.X, padx=5, pady=5)
        
        tk.Label(algo_frame, text="Algorithmus wählen:", font=("Arial", 10)).pack(anchor="w")
        
        self.algorithm_var = tk.StringVar(value="2-Opt")
        algos = ["Brute Force", "Nearest Neighbor", "2-Opt", "Multi-Start NN"]
        
        for algo in algos:
            tk.Radiobutton(
                algo_frame,
                text=algo,
                variable=self.algorithm_var,
                value=algo,
                font=("Arial", 9)
            ).pack(anchor="w", padx=10)
        
        tk.Button(
            algo_frame,
            text="🔍 Route berechnen",
            command=self._calculate_route,
            bg="#e67e22",
            fg="white",
            font=("Arial", 11, "bold"),
            height=2
        ).pack(fill=tk.X, pady=10)
        
        # Ergebnisse
        results_frame = tk.LabelFrame(
            control_frame,
            text="📊 Ergebnisse",
            font=("Arial", 12, "bold"),
            padx=10,
            pady=10
        )
        results_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        self.results_text = scrolledtext.ScrolledText(
            results_frame,
            height=12,
            font=("Courier", 9),
            bg="#f9f9f9",
            wrap=tk.WORD
        )
        self.results_text.pack(fill=tk.BOTH, expand=True)
        
        # Export
        export_frame = tk.Frame(results_frame)
        export_frame.pack(fill=tk.X, pady=(5, 0))
        
        tk.Button(
            export_frame,
            text="💾 Route exportieren",
            command=self._export_route,
            bg="#16a085",
            fg="white",
            font=("Arial", 9)
        ).pack(side=tk.LEFT, padx=2)
        
        tk.Button(
            export_frame,
            text="📊 Vergleichen",
            command=self._show_comparison,
            bg="#8e44ad",
            fg="white",
            font=("Arial", 9)
        ).pack(side=tk.LEFT, padx=2)
    
    def _create_visualization_panel(self, parent):
        """Erstellt Visualisierungs-Panel."""
        viz_frame = tk.Frame(parent)
        parent.add(viz_frame)
        
        if not MATPLOTLIB_AVAILABLE:
            # Fallback
            tk.Label(
                viz_frame,
                text="🗺️\n\nVisualisierung nicht verfügbar\n\n"
                     "Installieren Sie matplotlib:\n"
                     "pip install matplotlib",
                font=("Arial", 14),
                fg="#7f8c8d"
            ).pack(expand=True)
            return
        
        # Matplotlib Figure
        self.figure = Figure(figsize=(10, 8), dpi=100)
        self.figure.patch.set_facecolor('#ecf0f1')
        self.ax = self.figure.add_subplot(111)
        self.ax.set_facecolor('#ffffff')
        self.ax.set_title('TSP Visualisierung', fontsize=14, fontweight='bold')
        self.ax.set_xlabel('X-Koordinate', fontsize=10)
        self.ax.set_ylabel('Y-Koordinate', fontsize=10)
        self.ax.grid(True, alpha=0.3)
        
        # Canvas
        self.canvas = FigureCanvasTkAgg(self.figure, master=viz_frame)
        self.canvas.draw()
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Info
        info_frame = tk.Frame(viz_frame, bg="#ecf0f1", height=40)
        info_frame.pack(fill=tk.X, side=tk.BOTTOM)
        info_frame.pack_propagate(False)
        
        self.viz_info = tk.Label(
            info_frame,
            text="Bereit für Visualisierung",
            bg="#ecf0f1",
            font=("Arial", 10)
        )
        self.viz_info.pack(side=tk.LEFT, padx=10, pady=10)
    
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
    
    def _add_city(self):
        """Fügt neue Stadt hinzu."""
        dialog = tk.Toplevel(self.root)
        dialog.title("Stadt hinzufügen")
        dialog.geometry("400x300")
        dialog.transient(self.root)
        dialog.grab_set()
        
        # Form
        form = tk.Frame(dialog, padx=20, pady=20)
        form.pack(fill=tk.BOTH, expand=True)
        
        tk.Label(form, text="Name:*", font=("Arial", 10)).grid(row=0, column=0, sticky="w", pady=5)
        name_entry = tk.Entry(form, width=30, font=("Arial", 10))
        name_entry.grid(row=0, column=1, pady=5)
        
        tk.Label(form, text="X-Koordinate:*", font=("Arial", 10)).grid(row=1, column=0, sticky="w", pady=5)
        x_entry = tk.Entry(form, width=30, font=("Arial", 10))
        x_entry.grid(row=1, column=1, pady=5)
        
        tk.Label(form, text="Y-Koordinate:*", font=("Arial", 10)).grid(row=2, column=0, sticky="w", pady=5)
        y_entry = tk.Entry(form, width=30, font=("Arial", 10))
        y_entry.grid(row=2, column=1, pady=5)
        
        tk.Label(form, text="Land:", font=("Arial", 10)).grid(row=3, column=0, sticky="w", pady=5)
        country_entry = tk.Entry(form, width=30, font=("Arial", 10))
        country_entry.grid(row=3, column=1, pady=5)
        
        def save():
            name = name_entry.get().strip()
            x_str = x_entry.get().strip()
            y_str = y_entry.get().strip()
            
            if not name or not x_str or not y_str:
                messagebox.showerror("Fehler", "Name und Koordinaten sind erforderlich", parent=dialog)
                return
            
            try:
                x = float(x_str)
                y = float(y_str)
            except ValueError:
                messagebox.showerror("Fehler", "Koordinaten müssen Zahlen sein", parent=dialog)
                return
            
            city = City(
                id=str(uuid.uuid4()),
                name=name,
                x=x,
                y=y,
                country=country_entry.get().strip()
            )
            
            self.cities.append(city)
            self.client.create_city(city)
            self._refresh_cities()
            self._redraw_map()
            self._set_status(f"Stadt '{name}' hinzugefügt", "success")
            dialog.destroy()
        
        # Buttons
        btn_frame = tk.Frame(dialog)
        btn_frame.pack(pady=10)
        
        tk.Button(btn_frame, text="💾 Speichern", command=save, bg="#27ae60", fg="white", padx=20).pack(side=tk.LEFT, padx=5)
        tk.Button(btn_frame, text="✖️ Abbrechen", command=dialog.destroy, bg="#95a5a6", fg="white", padx=20).pack(side=tk.LEFT, padx=5)
    
    def _edit_city(self):
        """Bearbeitet ausgewählte Stadt."""
        selection = self.cities_listbox.curselection()
        if not selection:
            messagebox.showwarning("Warnung", "Bitte wählen Sie eine Stadt aus")
            return
        
        index = selection[0]
        city = self.cities[index]
        
        # Dialog zum Bearbeiten
        dialog = tk.Toplevel(self.root)
        dialog.title("Stadt bearbeiten")
        dialog.geometry("400x300")
        dialog.transient(self.root)
        dialog.grab_set()
        
        # Form
        form = tk.Frame(dialog, padx=20, pady=20)
        form.pack(fill=tk.BOTH, expand=True)
        
        tk.Label(form, text="Name:*", font=("Arial", 10)).grid(row=0, column=0, sticky="w", pady=5)
        name_entry = tk.Entry(form, width=30, font=("Arial", 10))
        name_entry.insert(0, city.name)
        name_entry.grid(row=0, column=1, pady=5)
        
        tk.Label(form, text="X-Koordinate:*", font=("Arial", 10)).grid(row=1, column=0, sticky="w", pady=5)
        x_entry = tk.Entry(form, width=30, font=("Arial", 10))
        x_entry.insert(0, str(city.x))
        x_entry.grid(row=1, column=1, pady=5)
        
        tk.Label(form, text="Y-Koordinate:*", font=("Arial", 10)).grid(row=2, column=0, sticky="w", pady=5)
        y_entry = tk.Entry(form, width=30, font=("Arial", 10))
        y_entry.insert(0, str(city.y))
        y_entry.grid(row=2, column=1, pady=5)
        
        tk.Label(form, text="Land:", font=("Arial", 10)).grid(row=3, column=0, sticky="w", pady=5)
        country_entry = tk.Entry(form, width=30, font=("Arial", 10))
        country_entry.insert(0, city.country)
        country_entry.grid(row=3, column=1, pady=5)
        
        def save():
            name = name_entry.get().strip()
            x_str = x_entry.get().strip()
            y_str = y_entry.get().strip()
            
            if not name or not x_str or not y_str:
                messagebox.showerror("Fehler", "Name und Koordinaten sind erforderlich", parent=dialog)
                return
            
            try:
                x = float(x_str)
                y = float(y_str)
            except ValueError:
                messagebox.showerror("Fehler", "Koordinaten müssen Zahlen sein", parent=dialog)
                return
            
            # Update city
            city.name = name
            city.x = x
            city.y = y
            city.country = country_entry.get().strip()
            
            # Update in ThemisDB
            self.client.create_city(city)
            
            self._refresh_cities()
            self._redraw_map()
            self._set_status(f"Stadt '{name}' aktualisiert", "success")
            dialog.destroy()
        
        # Buttons
        btn_frame = tk.Frame(dialog)
        btn_frame.pack(pady=10)
        
        tk.Button(btn_frame, text="💾 Speichern", command=save, bg="#27ae60", fg="white", padx=20).pack(side=tk.LEFT, padx=5)
        tk.Button(btn_frame, text="✖️ Abbrechen", command=dialog.destroy, bg="#95a5a6", fg="white", padx=20).pack(side=tk.LEFT, padx=5)
    
    def _delete_city(self):
        """Löscht ausgewählte Stadt."""
        selection = self.cities_listbox.curselection()
        if not selection:
            messagebox.showwarning("Warnung", "Bitte wählen Sie eine Stadt aus")
            return
        
        index = selection[0]
        city = self.cities[index]
        
        if not messagebox.askyesno("Bestätigen", f"Stadt '{city.name}' löschen?"):
            return
        
        self.client.delete_city(city.id)
        self.cities.pop(index)
        self._refresh_cities()
        self._redraw_map()
        self._set_status(f"Stadt '{city.name}' gelöscht", "success")
    
    def _load_demo_data(self):
        """Lädt Demo-Daten."""
        demo_cities = [
            City("c1", "Berlin", 52.52, 13.405, "Deutschland"),
            City("c2", "Hamburg", 53.55, 9.993, "Deutschland"),
            City("c3", "München", 48.137, 11.576, "Deutschland"),
            City("c4", "Köln", 50.937, 6.96, "Deutschland"),
            City("c5", "Frankfurt", 50.11, 8.682, "Deutschland"),
            City("c6", "Stuttgart", 48.78, 9.18, "Deutschland"),
            City("c7", "Düsseldorf", 51.227, 6.773, "Deutschland"),
            City("c8", "Dortmund", 51.514, 7.468, "Deutschland"),
        ]
        
        self.cities = demo_cities
        for city in self.cities:
            self.client.create_city(city)
        
        self._refresh_cities()
        self._redraw_map()
        self._set_status("Demo-Daten geladen (8 deutsche Städte)", "success")
    
    def _refresh_cities(self):
        """Aktualisiert Städte-Liste."""
        self.cities_listbox.delete(0, tk.END)
        for city in self.cities:
            self.cities_listbox.insert(tk.END, f"{city.name} ({city.x:.2f}, {city.y:.2f})")
    
    def _calculate_route(self):
        """Berechnet TSP-Route."""
        if len(self.cities) < 3:
            messagebox.showwarning("Warnung", "Mindestens 3 Städte erforderlich")
            return
        
        algorithm = self.algorithm_var.get()
        
        # Warnung bei Brute Force mit vielen Städten
        if algorithm == "Brute Force" and len(self.cities) > 10:
            if not messagebox.askyesno(
                "Warnung",
                f"Brute Force mit {len(self.cities)} Städten kann sehr lange dauern!\n\n"
                "Möchten Sie fortfahren?"
            ):
                return
        
        self._set_status(f"Berechne Route mit {algorithm}...", "info")
        self.root.update()
        
        try:
            if algorithm == "Brute Force":
                route = TSPSolver.brute_force(self.cities)
            elif algorithm == "Nearest Neighbor":
                route = TSPSolver.nearest_neighbor(self.cities)
            elif algorithm == "2-Opt":
                route = TSPSolver.two_opt(self.cities)
            elif algorithm == "Multi-Start NN":
                route = TSPSolver.multi_start_nearest_neighbor(self.cities)
            else:
                messagebox.showerror("Fehler", f"Unbekannter Algorithmus: {algorithm}")
                return
            
            self.current_route = route
            self.routes_history.append(route)
            
            # Speichere in ThemisDB
            self.client.save_route(route)
            
            # Zeige Ergebnisse
            self._show_results(route)
            self._redraw_map()
            
            self._set_status(f"Route berechnet: {route.total_distance:.2f} km", "success")
            
        except Exception as e:
            messagebox.showerror("Fehler", f"Fehler bei Berechnung:\n{str(e)}")
            self._set_status("Berechnung fehlgeschlagen", "error")
    
    def _show_results(self, route: Route):
        """Zeigt Ergebnis-Details."""
        self.results_text.delete(1.0, tk.END)
        
        self.results_text.insert(tk.END, f"{'='*50}\n", "header")
        self.results_text.insert(tk.END, f"ERGEBNIS: {route.algorithm}\n", "header")
        self.results_text.insert(tk.END, f"{'='*50}\n\n", "header")
        
        self.results_text.insert(tk.END, f"📏 Gesamtdistanz: {route.total_distance:.2f} km\n")
        self.results_text.insert(tk.END, f"⏱️  Berechnungszeit: {route.computation_time*1000:.2f} ms\n")
        self.results_text.insert(tk.END, f"🏙️  Anzahl Städte: {len(route.cities)-1}\n")
        
        if route.iterations > 0:
            self.results_text.insert(tk.END, f"🔄 Iterationen: {route.iterations}\n")
        
        self.results_text.insert(tk.END, f"\n📍 Route:\n")
        for i, city in enumerate(route.cities[:-1]):  # Ohne letzte (= erste)
            self.results_text.insert(tk.END, f"  {i+1}. {city.name}\n")
        self.results_text.insert(tk.END, f"\n↩️  Zurück nach {route.cities[0].name}\n")
    
    def _redraw_map(self):
        """Zeichnet Karte neu."""
        if not MATPLOTLIB_AVAILABLE:
            return
        
        self.ax.clear()
        self.ax.set_title('TSP Visualisierung', fontsize=14, fontweight='bold')
        self.ax.set_xlabel('X-Koordinate (Breitengrad)', fontsize=10)
        self.ax.set_ylabel('Y-Koordinate (Längengrad)', fontsize=10)
        self.ax.grid(True, alpha=0.3)
        
        if not self.cities:
            self.ax.text(0.5, 0.5, 'Keine Städte vorhanden\nKlicken Sie "Demo laden"',
                        ha='center', va='center', transform=self.ax.transAxes,
                        fontsize=12, color='gray')
            self.canvas.draw()
            return
        
        # Städte als Punkte
        x_coords = [city.x for city in self.cities]
        y_coords = [city.y for city in self.cities]
        
        self.ax.scatter(x_coords, y_coords, c='blue', s=100, alpha=0.6, zorder=3)
        
        # Stadt-Namen
        for city in self.cities:
            self.ax.annotate(
                city.name,
                (city.x, city.y),
                xytext=(5, 5),
                textcoords='offset points',
                fontsize=8,
                fontweight='bold'
            )
        
        # Route zeichnen
        if self.current_route:
            route_x = [city.x for city in self.current_route.cities]
            route_y = [city.y for city in self.current_route.cities]
            
            self.ax.plot(route_x, route_y, 'r-', linewidth=2, alpha=0.7, zorder=2)
            
            # Startstadt hervorheben
            self.ax.scatter(
                [route_x[0]], [route_y[0]],
                c='red', s=200, marker='*',
                zorder=4, label='Start/Ziel'
            )
            
            self.ax.legend()
        
        self.ax.set_aspect('equal', adjustable='datalim')
        self.canvas.draw()
        
        if hasattr(self, 'viz_info'):
            if self.current_route:
                self.viz_info.config(
                    text=f"Route: {self.current_route.algorithm} | "
                         f"Distanz: {self.current_route.total_distance:.2f} km"
                )
            else:
                self.viz_info.config(text=f"Städte: {len(self.cities)} | Keine Route berechnet")
    
    def _export_route(self):
        """Exportiert aktuelle Route."""
        if not self.current_route:
            messagebox.showwarning("Warnung", "Keine Route zum Exportieren vorhanden")
            return
        
        filename = filedialog.asksaveasfilename(
            defaultextension=".json",
            filetypes=[("JSON files", "*.json"), ("All files", "*.*")]
        )
        
        if filename:
            try:
                data = {
                    "algorithm": self.current_route.algorithm,
                    "total_distance": self.current_route.total_distance,
                    "computation_time": self.current_route.computation_time,
                    "cities": [
                        {
                            "name": city.name,
                            "x": city.x,
                            "y": city.y,
                            "country": city.country
                        }
                        for city in self.current_route.cities
                    ]
                }
                
                with open(filename, 'w', encoding='utf-8') as f:
                    json.dump(data, f, indent=2, ensure_ascii=False)
                
                messagebox.showinfo("Erfolg", f"Route exportiert nach:\n{filename}")
            except Exception as e:
                messagebox.showerror("Fehler", f"Export fehlgeschlagen:\n{str(e)}")
    
    def _show_comparison(self):
        """Zeigt Vergleich aller berechneten Routen."""
        if not self.routes_history:
            messagebox.showinfo("Info", "Noch keine Routen berechnet")
            return
        
        # Erstelle Vergleichs-Fenster
        comp_window = tk.Toplevel(self.root)
        comp_window.title("Routen-Vergleich")
        comp_window.geometry("700x500")
        
        # Tabelle
        tree = ttk.Treeview(
            comp_window,
            columns=("Algorithm", "Distance", "Time", "Quality"),
            show="headings"
        )
        
        tree.heading("Algorithm", text="Algorithmus")
        tree.heading("Distance", text="Distanz (km)")
        tree.heading("Time", text="Zeit (ms)")
        tree.heading("Quality", text="Qualität (%)")
        
        # Beste Distanz finden
        best_distance = min(r.total_distance for r in self.routes_history)
        
        for route in self.routes_history:
            quality = (best_distance / route.total_distance) * 100
            tree.insert("", tk.END, values=(
                route.algorithm,
                f"{route.total_distance:.2f}",
                f"{route.computation_time*1000:.2f}",
                f"{quality:.1f}%"
            ))
        
        tree.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        tk.Button(
            comp_window,
            text="Schließen",
            command=comp_window.destroy,
            bg="#95a5a6",
            fg="white"
        ).pack(pady=10)


def main():
    """Hauptfunktion."""
    root = tk.Tk()
    app = TSPApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
