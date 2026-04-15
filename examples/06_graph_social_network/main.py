"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            main.py                                            ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:01:24                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     641                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
Soziales Netzwerk - Graph-Visualisierung
Tkinter GUI mit NetworkX für soziale Netzwerke
"""

import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext
import uuid
from typing import List, Dict, Optional

# NetworkX imports
try:
    import networkx as nx
    NETWORKX_AVAILABLE = True
except ImportError:
    NETWORKX_AVAILABLE = False

# Matplotlib imports
try:
    import matplotlib
    matplotlib.use('TkAgg')
    from matplotlib.figure import Figure
    from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
    MATPLOTLIB_AVAILABLE = True
except ImportError:
    MATPLOTLIB_AVAILABLE = False

from themis_client import SocialNetworkClient
from models import User, Friendship, GraphAlgorithm


# Konfiguration
THEMIS_HOST = "localhost"
THEMIS_PORT = 8080
WINDOW_WIDTH = 1200
WINDOW_HEIGHT = 750


class SocialNetworkApp:
    """
    Hauptanwendung für Soziales Netzwerk.
    """
    
    def __init__(self, root: tk.Tk):
        """Initialisiert die Anwendung."""
        self.root = root
        self.root.title("ThemisDB - Soziales Netzwerk")
        self.root.geometry(f"{WINDOW_WIDTH}x{WINDOW_HEIGHT}")
        
        # Client
        self.client = SocialNetworkClient(host=THEMIS_HOST, port=THEMIS_PORT)
        
        # Daten
        self.users: List[User] = []
        self.friendships: List[Friendship] = []
        self.selected_user_id: Optional[str] = None
        
        # Check dependencies
        if not NETWORKX_AVAILABLE or not MATPLOTLIB_AVAILABLE:
            missing = []
            if not NETWORKX_AVAILABLE:
                missing.append("networkx")
            if not MATPLOTLIB_AVAILABLE:
                missing.append("matplotlib")
            
            messagebox.showwarning(
                "Warnung",
                f"Fehlende Bibliotheken: {', '.join(missing)}\n\n"
                f"Graph-Visualisierung wird nicht verfügbar sein.\n\n"
                f"Installieren mit: pip install {' '.join(missing)}"
            )
        
        # UI erstellen
        self._create_ui()
        
        # Verbindung prüfen
        self._check_connection()
        
        # Demo-Daten laden
        self._load_demo_data()
        
        # Tastenkombinationen
        self._setup_keybindings()
    
    def _create_ui(self):
        """Erstellt die Benutzeroberfläche."""
        # Header
        self._create_header()
        
        # Main Content
        paned = tk.PanedWindow(self.root, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Linke Seite: Benutzer-Liste und Info
        self._create_user_panel(paned)
        
        # Rechte Seite: Graph-Visualisierung
        self._create_graph_panel(paned)
        
        # Status Bar
        self._create_status_bar()
    
    def _create_header(self):
        """Erstellt Header."""
        header_frame = tk.Frame(self.root, bg="#2c3e50", height=60)
        header_frame.pack(fill=tk.X)
        header_frame.pack_propagate(False)
        
        # Title
        title_label = tk.Label(
            header_frame,
            text="🌐 Soziales Netzwerk",
            bg="#2c3e50",
            fg="white",
            font=("Arial", 16, "bold")
        )
        title_label.pack(side=tk.LEFT, padx=20, pady=10)
        
        # Connection Status
        self.connection_status = tk.Label(
            header_frame,
            text="● Connecting...",
            bg="#2c3e50",
            fg="#f39c12",
            font=("Arial", 10)
        )
        self.connection_status.pack(side=tk.RIGHT, padx=20)
        
        # Toolbar
        toolbar = tk.Frame(header_frame, bg="#2c3e50")
        toolbar.pack(side=tk.LEFT, padx=10)
        
        tk.Button(
            toolbar,
            text="➕ Benutzer",
            command=self._add_user,
            bg="#27ae60",
            fg="white",
            padx=10
        ).pack(side=tk.LEFT, padx=2)
        
        tk.Button(
            toolbar,
            text="🤝 Freundschaft",
            command=self._add_friendship,
            bg="#3498db",
            fg="white",
            padx=10
        ).pack(side=tk.LEFT, padx=2)
        
        tk.Button(
            toolbar,
            text="🔄 Graph neu zeichnen",
            command=self._redraw_graph,
            bg="#9b59b6",
            fg="white",
            padx=10
        ).pack(side=tk.LEFT, padx=2)
    
    def _create_user_panel(self, parent):
        """Erstellt Benutzer-Panel."""
        user_frame = tk.Frame(parent, width=400)
        parent.add(user_frame)
        
        # Benutzer-Liste
        users_label = tk.LabelFrame(
            user_frame,
            text="Benutzer",
            font=("Arial", 12, "bold")
        )
        users_label.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Listbox
        list_frame = tk.Frame(users_label)
        list_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        scrollbar = tk.Scrollbar(list_frame)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        self.user_listbox = tk.Listbox(
            list_frame,
            yscrollcommand=scrollbar.set,
            font=("Arial", 11),
            selectmode=tk.SINGLE
        )
        self.user_listbox.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.config(command=self.user_listbox.yview)
        
        self.user_listbox.bind("<<ListboxSelect>>", self._on_user_select)
        
        # Benutzer-Details
        details_label = tk.LabelFrame(
            user_frame,
            text="Details",
            font=("Arial", 12, "bold")
        )
        details_label.pack(fill=tk.BOTH, padx=5, pady=5)
        
        self.details_text = scrolledtext.ScrolledText(
            details_label,
            height=8,
            font=("Arial", 10),
            wrap=tk.WORD
        )
        self.details_text.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Analyse-Bereich
        analysis_label = tk.LabelFrame(
            user_frame,
            text="Graph-Analyse",
            font=("Arial", 12, "bold")
        )
        analysis_label.pack(fill=tk.X, padx=5, pady=5)
        
        btn_frame = tk.Frame(analysis_label)
        btn_frame.pack(fill=tk.X, padx=5, pady=5)
        
        tk.Button(
            btn_frame,
            text="👥 Freunde von Freunden",
            command=self._find_fof,
            bg="#3498db",
            fg="white"
        ).pack(fill=tk.X, pady=2)
        
        tk.Button(
            btn_frame,
            text="💡 Freunde empfehlen",
            command=self._recommend_friends,
            bg="#9b59b6",
            fg="white"
        ).pack(fill=tk.X, pady=2)
        
        self.analysis_text = tk.Text(
            analysis_label,
            height=6,
            font=("Arial", 9),
            wrap=tk.WORD,
            bg="#ecf0f1"
        )
        self.analysis_text.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
    
    def _create_graph_panel(self, parent):
        """Erstellt Graph-Panel."""
        graph_frame = tk.Frame(parent)
        parent.add(graph_frame)
        
        if not NETWORKX_AVAILABLE or not MATPLOTLIB_AVAILABLE:
            # Fallback
            tk.Label(
                graph_frame,
                text="🌐\n\nGraph-Visualisierung nicht verfügbar\n\n"
                     "Installieren Sie networkx und matplotlib:\n"
                     "pip install networkx matplotlib",
                font=("Arial", 14),
                fg="#7f8c8d"
            ).pack(expand=True)
            return
        
        # Graph mit matplotlib
        self.figure = Figure(figsize=(8, 6), dpi=100)
        self.figure.patch.set_facecolor('#ecf0f1')
        self.ax = self.figure.add_subplot(111)
        self.ax.set_facecolor('#ffffff')
        
        # Canvas
        self.canvas = FigureCanvasTkAgg(self.figure, master=graph_frame)
        self.canvas.draw()
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Info
        info_frame = tk.Frame(graph_frame, bg="#ecf0f1", height=40)
        info_frame.pack(fill=tk.X, side=tk.BOTTOM)
        info_frame.pack_propagate(False)
        
        self.graph_info = tk.Label(
            info_frame,
            text="Bereit",
            bg="#ecf0f1",
            font=("Arial", 10)
        )
        self.graph_info.pack(side=tk.LEFT, padx=10)
    
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
        self.root.bind("<Control-n>", lambda e: self._add_user())
        self.root.bind("<Control-f>", lambda e: self._add_friendship())
        self.root.bind("<F5>", lambda e: self._redraw_graph())
    
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
    
    def _load_demo_data(self):
        """Lädt Demo-Daten."""
        # Demo-Benutzer
        demo_users = [
            User("u1", "Alice", "Software Engineer", ["Python", "AI"], "Berlin"),
            User("u2", "Bob", "Data Scientist", ["ML", "Statistics"], "München"),
            User("u3", "Charlie", "DevOps Engineer", ["Docker", "K8s"], "Hamburg"),
            User("u4", "Diana", "Frontend Dev", ["React", "CSS"], "Berlin"),
            User("u5", "Eve", "Backend Dev", ["Java", "Spring"], "Frankfurt"),
        ]
        
        for user in demo_users:
            self.users.append(user)
        
        # Demo-Freundschaften
        demo_friendships = [
            Friendship("u1", "u2"),
            Friendship("u1", "u3"),
            Friendship("u2", "u4"),
            Friendship("u3", "u4"),
            Friendship("u3", "u5"),
        ]
        
        for friendship in demo_friendships:
            self.friendships.append(friendship)
        
        self._refresh_users()
        self._redraw_graph()
    
    def _add_user(self):
        """Fügt neuen Benutzer hinzu."""
        dialog = tk.Toplevel(self.root)
        dialog.title("Neuer Benutzer")
        dialog.geometry("400x350")
        dialog.transient(self.root)
        dialog.grab_set()
        
        # Form
        form_frame = tk.Frame(dialog, padx=20, pady=20)
        form_frame.pack(fill=tk.BOTH, expand=True)
        
        tk.Label(form_frame, text="Name:*").grid(row=0, column=0, sticky="w", pady=5)
        name_entry = tk.Entry(form_frame, width=30)
        name_entry.grid(row=0, column=1, pady=5)
        
        tk.Label(form_frame, text="Bio:").grid(row=1, column=0, sticky="w", pady=5)
        bio_text = tk.Text(form_frame, width=30, height=3)
        bio_text.grid(row=1, column=1, pady=5)
        
        tk.Label(form_frame, text="Standort:").grid(row=2, column=0, sticky="w", pady=5)
        location_entry = tk.Entry(form_frame, width=30)
        location_entry.grid(row=2, column=1, pady=5)
        
        def save():
            name = name_entry.get().strip()
            if not name:
                messagebox.showerror("Fehler", "Name ist erforderlich", parent=dialog)
                return
            
            user = User(
                id=str(uuid.uuid4()),
                name=name,
                bio=bio_text.get(1.0, tk.END).strip(),
                location=location_entry.get().strip()
            )
            
            try:
                self.client.create_user(user)
                self.users.append(user)
                self._refresh_users()
                self._redraw_graph()
                self._set_status(f"Benutzer '{name}' hinzugefügt", "success")
                dialog.destroy()
            except Exception as e:
                messagebox.showerror("Fehler", str(e), parent=dialog)
        
        btn_frame = tk.Frame(dialog)
        btn_frame.pack(pady=10)
        
        tk.Button(btn_frame, text="💾 Speichern", command=save, bg="#27ae60", fg="white", padx=20).pack(side=tk.LEFT, padx=5)
        tk.Button(btn_frame, text="✖️ Abbrechen", command=dialog.destroy, bg="#95a5a6", fg="white", padx=20).pack(side=tk.LEFT, padx=5)
    
    def _add_friendship(self):
        """Fügt Freundschaft hinzu."""
        if len(self.users) < 2:
            messagebox.showwarning("Warnung", "Mindestens 2 Benutzer erforderlich")
            return
        
        dialog = tk.Toplevel(self.root)
        dialog.title("Freundschaft hinzufügen")
        dialog.geometry("350x200")
        dialog.transient(self.root)
        dialog.grab_set()
        
        form_frame = tk.Frame(dialog, padx=20, pady=20)
        form_frame.pack(fill=tk.BOTH, expand=True)
        
        user_names = [f"{u.name} ({u.id[:8]})" for u in self.users]
        
        tk.Label(form_frame, text="Benutzer 1:").grid(row=0, column=0, sticky="w", pady=5)
        user1_combo = ttk.Combobox(form_frame, values=user_names, state="readonly", width=25)
        user1_combo.grid(row=0, column=1, pady=5)
        
        tk.Label(form_frame, text="Benutzer 2:").grid(row=1, column=0, sticky="w", pady=5)
        user2_combo = ttk.Combobox(form_frame, values=user_names, state="readonly", width=25)
        user2_combo.grid(row=1, column=1, pady=5)
        
        def save():
            if not user1_combo.get() or not user2_combo.get():
                messagebox.showerror("Fehler", "Beide Benutzer auswählen", parent=dialog)
                return
            
            user1_idx = user1_combo.current()
            user2_idx = user2_combo.current()
            
            if user1_idx == user2_idx:
                messagebox.showerror("Fehler", "Verschiedene Benutzer auswählen", parent=dialog)
                return
            
            user1 = self.users[user1_idx]
            user2 = self.users[user2_idx]
            
            # Check if friendship already exists
            exists = any(
                (f.from_user == user1.id and f.to_user == user2.id) or
                (f.from_user == user2.id and f.to_user == user1.id)
                for f in self.friendships
            )
            
            if exists:
                messagebox.showerror("Fehler", "Freundschaft existiert bereits", parent=dialog)
                return
            
            friendship = Friendship(user1.id, user2.id)
            
            try:
                self.client.create_friendship(friendship)
                self.friendships.append(friendship)
                self._redraw_graph()
                self._set_status(f"Freundschaft zwischen {user1.name} und {user2.name} erstellt", "success")
                dialog.destroy()
            except Exception as e:
                messagebox.showerror("Fehler", str(e), parent=dialog)
        
        btn_frame = tk.Frame(dialog)
        btn_frame.pack(pady=10)
        
        tk.Button(btn_frame, text="💾 Speichern", command=save, bg="#27ae60", fg="white", padx=20).pack(side=tk.LEFT, padx=5)
        tk.Button(btn_frame, text="✖️ Abbrechen", command=dialog.destroy, bg="#95a5a6", fg="white", padx=20).pack(side=tk.LEFT, padx=5)
    
    def _refresh_users(self):
        """Aktualisiert Benutzer-Liste."""
        self.user_listbox.delete(0, tk.END)
        for user in self.users:
            self.user_listbox.insert(tk.END, f"{user.name} ({user.location})")
    
    def _on_user_select(self, event):
        """Handler für Benutzer-Auswahl."""
        selection = self.user_listbox.curselection()
        if not selection:
            return
        
        index = selection[0]
        user = self.users[index]
        self.selected_user_id = user.id
        
        # Zeige Details
        self.details_text.delete(1.0, tk.END)
        self.details_text.insert(tk.END, f"Name: {user.name}\n")
        self.details_text.insert(tk.END, f"ID: {user.id}\n")
        self.details_text.insert(tk.END, f"Bio: {user.bio}\n")
        self.details_text.insert(tk.END, f"Standort: {user.location}\n")
        self.details_text.insert(tk.END, f"Interessen: {', '.join(user.interests)}\n")
        
        # Zähle Freunde
        friend_count = sum(
            1 for f in self.friendships
            if f.from_user == user.id or f.to_user == user.id
        )
        self.details_text.insert(tk.END, f"\nFreunde: {friend_count}")
    
    def _find_fof(self):
        """Findet Freunde von Freunden."""
        if not self.selected_user_id:
            messagebox.showwarning("Warnung", "Bitte Benutzer auswählen")
            return
        
        fof_ids = GraphAlgorithm.friends_of_friends(
            self.selected_user_id,
            self.users,
            self.friendships
        )
        
        self.analysis_text.delete(1.0, tk.END)
        self.analysis_text.insert(tk.END, "Freunde von Freunden:\n\n")
        
        if fof_ids:
            for fof_id in fof_ids:
                user = next((u for u in self.users if u.id == fof_id), None)
                if user:
                    self.analysis_text.insert(tk.END, f"• {user.name}\n")
        else:
            self.analysis_text.insert(tk.END, "Keine gefunden")
    
    def _recommend_friends(self):
        """Empfiehlt Freunde."""
        if not self.selected_user_id:
            messagebox.showwarning("Warnung", "Bitte Benutzer auswählen")
            return
        
        rec_ids = GraphAlgorithm.friend_recommendations(
            self.selected_user_id,
            self.users,
            self.friendships
        )
        
        self.analysis_text.delete(1.0, tk.END)
        self.analysis_text.insert(tk.END, "Empfohlene Freunde:\n\n")
        
        if rec_ids:
            for rec_id in rec_ids:
                user = next((u for u in self.users if u.id == rec_id), None)
                if user:
                    self.analysis_text.insert(tk.END, f"• {user.name}\n")
        else:
            self.analysis_text.insert(tk.END, "Keine Empfehlungen")
    
    def _redraw_graph(self):
        """Zeichnet Graph neu."""
        if not NETWORKX_AVAILABLE or not MATPLOTLIB_AVAILABLE:
            return
        
        # Erstelle NetworkX Graph
        G = nx.Graph()
        
        # Knoten hinzufügen
        for user in self.users:
            G.add_node(user.id, name=user.name)
        
        # Kanten hinzufügen
        for friendship in self.friendships:
            G.add_edge(friendship.from_user, friendship.to_user)
        
        # Zeichne
        self.ax.clear()
        
        if len(G.nodes()) == 0:
            self.ax.text(0.5, 0.5, 'Keine Benutzer', ha='center', va='center')
            self.canvas.draw()
            return
        
        # Layout
        pos = nx.spring_layout(G, k=1, iterations=50)
        
        # Zeichne Knoten
        nx.draw_networkx_nodes(
            G, pos, ax=self.ax,
            node_color='#3498db',
            node_size=700,
            alpha=0.9
        )
        
        # Zeichne Kanten
        nx.draw_networkx_edges(
            G, pos, ax=self.ax,
            edge_color='#95a5a6',
            width=2,
            alpha=0.6
        )
        
        # Labels
        labels = {node: G.nodes[node]['name'] for node in G.nodes()}
        nx.draw_networkx_labels(
            G, pos, labels, ax=self.ax,
            font_size=9,
            font_weight='bold'
        )
        
        self.ax.set_title('Soziales Netzwerk Graph', fontsize=14, fontweight='bold')
        self.ax.axis('off')
        
        self.canvas.draw()
        
        # Update Info
        if hasattr(self, 'graph_info'):
            self.graph_info.config(
                text=f"Knoten: {len(G.nodes())} | Kanten: {len(G.edges())}"
            )


def main():
    """Hauptfunktion."""
    root = tk.Tk()
    app = SocialNetworkApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
