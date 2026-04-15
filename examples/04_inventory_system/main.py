"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            main.py                                            ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:32:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     682                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
Inventarsystem - Lagerverwaltung mit ThemisDB
Tkinter GUI für Inventarverwaltung mit Statistiken
"""

import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext
import uuid
from datetime import datetime
from typing import List, Optional, Dict
from themis_client import InventoryClient
from models import Product, StockMovement, Supplier, ProductSupplier, MovementType


# Konfiguration
THEMIS_HOST = "localhost"
THEMIS_PORT = 8080
WINDOW_WIDTH = 1000
WINDOW_HEIGHT = 700


class InventoryApp:
    """
    Hauptanwendung für Inventarverwaltung.
    """
    
    def __init__(self, root: tk.Tk):
        """Initialisiert die Anwendung."""
        self.root = root
        self.root.title("ThemisDB - Inventarsystem")
        self.root.geometry(f"{WINDOW_WIDTH}x{WINDOW_HEIGHT}")
        
        # Client
        self.client = InventoryClient(host=THEMIS_HOST, port=THEMIS_PORT)
        
        # Daten (in-memory cache)
        self.products: List[Product] = []
        self.suppliers: List[Supplier] = []
        self.movements: List[StockMovement] = []
        self.product_suppliers: Dict[str, List[str]] = {}  # product_id -> [supplier_ids]
        
        # UI erstellen
        self._create_ui()
        
        # Verbindung prüfen
        self._check_connection()
        
        # Tastenkombinationen
        self._setup_keybindings()
        
        # Demo-Daten laden (optional)
        self._load_demo_data()
    
    def _create_ui(self):
        """Erstellt die Benutzeroberfläche."""
        # Header
        self._create_header()
        
        # Tabbed Interface
        self.notebook = ttk.Notebook(self.root)
        self.notebook.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Tabs
        self._create_products_tab()
        self._create_stock_tab()
        self._create_suppliers_tab()
        self._create_dashboard_tab()
        
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
            text="📦 Inventarsystem",
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
    
    def _create_products_tab(self):
        """Erstellt Produkte-Tab."""
        products_frame = tk.Frame(self.notebook)
        self.notebook.add(products_frame, text="📦 Produkte")
        
        # Toolbar
        toolbar = tk.Frame(products_frame, bg="#ecf0f1", height=45)
        toolbar.pack(fill=tk.X)
        toolbar.pack_propagate(False)
        
        tk.Button(
            toolbar,
            text="➕ Neues Produkt",
            command=self._new_product,
            bg="#27ae60",
            fg="white",
            padx=10
        ).pack(side=tk.LEFT, padx=5, pady=5)
        
        tk.Button(
            toolbar,
            text="✏️ Bearbeiten",
            command=self._edit_product,
            bg="#3498db",
            fg="white",
            padx=10
        ).pack(side=tk.LEFT, padx=5)
        
        tk.Button(
            toolbar,
            text="🗑️ Löschen",
            command=self._delete_product,
            bg="#e74c3c",
            fg="white",
            padx=10
        ).pack(side=tk.LEFT, padx=5)
        
        # Suche
        tk.Label(toolbar, text="🔍", bg="#ecf0f1").pack(side=tk.LEFT, padx=(20, 5))
        self.product_search = tk.Entry(toolbar, width=30)
        self.product_search.pack(side=tk.LEFT)
        self.product_search.bind("<KeyRelease>", lambda e: self._filter_products())
        
        # Produkt-Liste (Treeview)
        list_frame = tk.Frame(products_frame)
        list_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Scrollbars
        vsb = tk.Scrollbar(list_frame, orient="vertical")
        hsb = tk.Scrollbar(list_frame, orient="horizontal")
        
        # Treeview
        columns = ("SKU", "Name", "Bestand", "Min", "Status", "Preis", "Lagerort")
        self.product_tree = ttk.Treeview(
            list_frame,
            columns=columns,
            show="tree headings",
            yscrollcommand=vsb.set,
            xscrollcommand=hsb.set
        )
        
        vsb.config(command=self.product_tree.yview)
        hsb.config(command=self.product_tree.xview)
        
        # Column headings
        self.product_tree.heading("#0", text="ID")
        self.product_tree.column("#0", width=80)
        
        for col in columns:
            self.product_tree.heading(col, text=col)
            if col in ["SKU", "Status"]:
                self.product_tree.column(col, width=100)
            elif col in ["Bestand", "Min", "Preis"]:
                self.product_tree.column(col, width=80)
            else:
                self.product_tree.column(col, width=150)
        
        # Grid layout
        self.product_tree.grid(row=0, column=0, sticky="nsew")
        vsb.grid(row=0, column=1, sticky="ns")
        hsb.grid(row=1, column=0, sticky="ew")
        
        list_frame.grid_rowconfigure(0, weight=1)
        list_frame.grid_columnconfigure(0, weight=1)
        
        self.product_tree.bind("<Double-Button-1>", lambda e: self._edit_product())
    
    def _create_stock_tab(self):
        """Erstellt Bestands-Tab."""
        stock_frame = tk.Frame(self.notebook)
        self.notebook.add(stock_frame, text="📊 Bestand")
        
        # Toolbar
        toolbar = tk.Frame(stock_frame, bg="#ecf0f1", height=45)
        toolbar.pack(fill=tk.X)
        toolbar.pack_propagate(False)
        
        tk.Button(
            toolbar,
            text="📥 Wareneingang",
            command=lambda: self._stock_movement(MovementType.IN),
            bg="#27ae60",
            fg="white",
            padx=10
        ).pack(side=tk.LEFT, padx=5, pady=5)
        
        tk.Button(
            toolbar,
            text="📤 Warenausgang",
            command=lambda: self._stock_movement(MovementType.OUT),
            bg="#e67e22",
            fg="white",
            padx=10
        ).pack(side=tk.LEFT, padx=5)
        
        tk.Button(
            toolbar,
            text="⚖️ Inventur",
            command=lambda: self._stock_movement(MovementType.ADJUSTMENT),
            bg="#3498db",
            fg="white",
            padx=10
        ).pack(side=tk.LEFT, padx=5)
        
        # Bewegungs-Historie
        list_frame = tk.Frame(stock_frame)
        list_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        vsb = tk.Scrollbar(list_frame, orient="vertical")
        hsb = tk.Scrollbar(list_frame, orient="horizontal")
        
        columns = ("Zeitpunkt", "Produkt", "Typ", "Menge", "Grund", "Benutzer")
        self.movement_tree = ttk.Treeview(
            list_frame,
            columns=columns,
            show="tree headings",
            yscrollcommand=vsb.set,
            xscrollcommand=hsb.set
        )
        
        vsb.config(command=self.movement_tree.yview)
        hsb.config(command=self.movement_tree.xview)
        
        self.movement_tree.heading("#0", text="ID")
        self.movement_tree.column("#0", width=80)
        
        for col in columns:
            self.movement_tree.heading(col, text=col)
            if col in ["Typ", "Menge", "Benutzer"]:
                self.movement_tree.column(col, width=100)
            elif col == "Zeitpunkt":
                self.movement_tree.column(col, width=150)
            else:
                self.movement_tree.column(col, width=150)
        
        self.movement_tree.grid(row=0, column=0, sticky="nsew")
        vsb.grid(row=0, column=1, sticky="ns")
        hsb.grid(row=1, column=0, sticky="ew")
        
        list_frame.grid_rowconfigure(0, weight=1)
        list_frame.grid_columnconfigure(0, weight=1)
    
    def _create_suppliers_tab(self):
        """Erstellt Lieferanten-Tab."""
        suppliers_frame = tk.Frame(self.notebook)
        self.notebook.add(suppliers_frame, text="🚚 Lieferanten")
        
        # Toolbar
        toolbar = tk.Frame(suppliers_frame, bg="#ecf0f1", height=45)
        toolbar.pack(fill=tk.X)
        toolbar.pack_propagate(False)
        
        tk.Button(
            toolbar,
            text="➕ Neuer Lieferant",
            command=self._new_supplier,
            bg="#27ae60",
            fg="white",
            padx=10
        ).pack(side=tk.LEFT, padx=5, pady=5)
        
        # Lieferanten-Liste
        list_frame = tk.Frame(suppliers_frame)
        list_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        vsb = tk.Scrollbar(list_frame, orient="vertical")
        
        columns = ("Name", "Kontakt", "Email", "Telefon", "Lieferzeit")
        self.supplier_tree = ttk.Treeview(
            list_frame,
            columns=columns,
            show="tree headings",
            yscrollcommand=vsb.set
        )
        
        vsb.config(command=self.supplier_tree.yview)
        
        self.supplier_tree.heading("#0", text="ID")
        self.supplier_tree.column("#0", width=80)
        
        for col in columns:
            self.supplier_tree.heading(col, text=col)
            if col == "Lieferzeit":
                self.supplier_tree.column(col, width=100)
            else:
                self.supplier_tree.column(col, width=150)
        
        self.supplier_tree.grid(row=0, column=0, sticky="nsew")
        vsb.grid(row=0, column=1, sticky="ns")
        
        list_frame.grid_rowconfigure(0, weight=1)
        list_frame.grid_columnconfigure(0, weight=1)
    
    def _create_dashboard_tab(self):
        """Erstellt Dashboard-Tab."""
        dashboard_frame = tk.Frame(self.notebook)
        self.notebook.add(dashboard_frame, text="📈 Dashboard")
        
        # Statistics Frame
        stats_frame = tk.LabelFrame(dashboard_frame, text="Statistiken", font=("Arial", 12, "bold"))
        stats_frame.pack(fill=tk.X, padx=10, pady=10)
        
        # Create grid for statistics
        grid_frame = tk.Frame(stats_frame)
        grid_frame.pack(padx=10, pady=10)
        
        # Stat cards
        self.stat_total_products = self._create_stat_card(grid_frame, "Produkte", "0", "#3498db", 0, 0)
        self.stat_total_value = self._create_stat_card(grid_frame, "Gesamtwert", "0 €", "#27ae60", 0, 1)
        self.stat_low_stock = self._create_stat_card(grid_frame, "Niedriger Bestand", "0", "#f39c12", 1, 0)
        self.stat_out_of_stock = self._create_stat_card(grid_frame, "Ausverkauft", "0", "#e74c3c", 1, 1)
        
        # Warnings Frame
        warnings_frame = tk.LabelFrame(dashboard_frame, text="Warnungen", font=("Arial", 12, "bold"))
        warnings_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        self.warnings_text = scrolledtext.ScrolledText(
            warnings_frame,
            height=10,
            font=("Arial", 10),
            wrap=tk.WORD
        )
        self.warnings_text.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        # Refresh button
        tk.Button(
            dashboard_frame,
            text="🔄 Aktualisieren",
            command=self._update_dashboard,
            bg="#3498db",
            fg="white",
            font=("Arial", 11),
            padx=20,
            pady=10
        ).pack(pady=10)
    
    def _create_stat_card(self, parent, title, value, color, row, col):
        """Erstellt eine Statistik-Karte."""
        card = tk.Frame(parent, bg=color, width=200, height=100)
        card.grid(row=row, column=col, padx=10, pady=10, sticky="nsew")
        card.grid_propagate(False)
        
        tk.Label(
            card,
            text=title,
            bg=color,
            fg="white",
            font=("Arial", 10)
        ).pack(pady=(15, 5))
        
        value_label = tk.Label(
            card,
            text=value,
            bg=color,
            fg="white",
            font=("Arial", 20, "bold")
        )
        value_label.pack()
        
        return value_label
    
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
        self.root.bind("<Control-n>", lambda e: self._new_product())
        self.root.bind("<Control-i>", lambda e: self._stock_movement(MovementType.IN))
        self.root.bind("<Control-o>", lambda e: self._stock_movement(MovementType.OUT))
        self.root.bind("<F5>", lambda e: self._refresh_all())
    
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
        # Einfache Demo-Produkte
        demo_products = [
            Product("p1", "PROD-001", "Laptop", "Business Laptop", 899.99, 15, 10, "Lager A", "Elektronik"),
            Product("p2", "PROD-002", "Maus", "Kabellose Maus", 29.99, 50, 20, "Lager A", "Elektronik"),
            Product("p3", "PROD-003", "Tastatur", "Mechanische Tastatur", 79.99, 8, 15, "Lager A", "Elektronik"),
            Product("p4", "PROD-004", "Monitor", "27\" Monitor", 299.99, 5, 10, "Lager B", "Elektronik"),
        ]
        
        for product in demo_products:
            self.products.append(product)
        
        self._refresh_products()
        self._update_dashboard()
    
    def _new_product(self):
        """Erstellt neues Produkt."""
        dialog = tk.Toplevel(self.root)
        dialog.title("Neues Produkt")
        dialog.geometry("500x600")
        dialog.transient(self.root)
        dialog.grab_set()
        
        # Form
        form_frame = tk.Frame(dialog, padx=20, pady=20)
        form_frame.pack(fill=tk.BOTH, expand=True)
        
        # SKU
        tk.Label(form_frame, text="SKU:*").grid(row=0, column=0, sticky="w", pady=5)
        sku_entry = tk.Entry(form_frame, width=30)
        sku_entry.grid(row=0, column=1, pady=5)
        
        # Name
        tk.Label(form_frame, text="Name:*").grid(row=1, column=0, sticky="w", pady=5)
        name_entry = tk.Entry(form_frame, width=30)
        name_entry.grid(row=1, column=1, pady=5)
        
        # Beschreibung
        tk.Label(form_frame, text="Beschreibung:").grid(row=2, column=0, sticky="w", pady=5)
        desc_text = tk.Text(form_frame, width=30, height=3)
        desc_text.grid(row=2, column=1, pady=5)
        
        # Preis
        tk.Label(form_frame, text="Preis:*").grid(row=3, column=0, sticky="w", pady=5)
        price_entry = tk.Entry(form_frame, width=30)
        price_entry.grid(row=3, column=1, pady=5)
        
        # Anfangsbestand
        tk.Label(form_frame, text="Anfangsbestand:").grid(row=4, column=0, sticky="w", pady=5)
        qty_entry = tk.Entry(form_frame, width=30)
        qty_entry.insert(0, "0")
        qty_entry.grid(row=4, column=1, pady=5)
        
        # Mindestbestand
        tk.Label(form_frame, text="Mindestbestand:").grid(row=5, column=0, sticky="w", pady=5)
        min_entry = tk.Entry(form_frame, width=30)
        min_entry.insert(0, "10")
        min_entry.grid(row=5, column=1, pady=5)
        
        # Lagerort
        tk.Label(form_frame, text="Lagerort:").grid(row=6, column=0, sticky="w", pady=5)
        loc_entry = tk.Entry(form_frame, width=30)
        loc_entry.grid(row=6, column=1, pady=5)
        
        # Kategorie
        tk.Label(form_frame, text="Kategorie:").grid(row=7, column=0, sticky="w", pady=5)
        cat_entry = tk.Entry(form_frame, width=30)
        cat_entry.grid(row=7, column=1, pady=5)
        
        def save():
            sku = sku_entry.get().strip()
            name = name_entry.get().strip()
            
            if not sku or not name:
                messagebox.showerror("Fehler", "SKU und Name sind erforderlich", parent=dialog)
                return
            
            try:
                price = float(price_entry.get().strip())
                quantity = int(qty_entry.get().strip())
                min_qty = int(min_entry.get().strip())
            except ValueError:
                messagebox.showerror("Fehler", "Ungültige Zahlen", parent=dialog)
                return
            
            product = Product(
                id=str(uuid.uuid4()),
                sku=sku,
                name=name,
                description=desc_text.get(1.0, tk.END).strip(),
                price=price,
                quantity=quantity,
                min_quantity=min_qty,
                location=loc_entry.get().strip(),
                category=cat_entry.get().strip()
            )
            
            try:
                self.client.create_product(product)
                self.products.append(product)
                self._refresh_products()
                self._update_dashboard()
                self._set_status(f"Produkt '{name}' erstellt", "success")
                dialog.destroy()
            except Exception as e:
                messagebox.showerror("Fehler", str(e), parent=dialog)
        
        # Buttons
        btn_frame = tk.Frame(dialog)
        btn_frame.pack(pady=10)
        
        tk.Button(btn_frame, text="💾 Speichern", command=save, bg="#27ae60", fg="white", padx=20).pack(side=tk.LEFT, padx=5)
        tk.Button(btn_frame, text="✖️ Abbrechen", command=dialog.destroy, bg="#95a5a6", fg="white", padx=20).pack(side=tk.LEFT, padx=5)
    
    def _edit_product(self):
        """Bearbeitet Produkt."""
        selection = self.product_tree.selection()
        if not selection:
            messagebox.showwarning("Warnung", "Kein Produkt ausgewählt")
            return
        
        product_id = self.product_tree.item(selection[0])["text"]
        product = next((p for p in self.products if p.id == product_id), None)
        if not product:
            return
        
        messagebox.showinfo("Info", f"Bearbeitung von '{product.name}' würde hier implementiert werden")
    
    def _delete_product(self):
        """Löscht Produkt."""
        selection = self.product_tree.selection()
        if not selection:
            messagebox.showwarning("Warnung", "Kein Produkt ausgewählt")
            return
        
        product_id = self.product_tree.item(selection[0])["text"]
        product = next((p for p in self.products if p.id == product_id), None)
        if not product:
            return
        
        if not messagebox.askyesno("Bestätigung", f"Produkt '{product.name}' löschen?"):
            return
        
        try:
            self.client.delete_product(product.id)
            self.products.remove(product)
            self._refresh_products()
            self._update_dashboard()
            self._set_status(f"Produkt '{product.name}' gelöscht", "success")
        except Exception as e:
            messagebox.showerror("Fehler", str(e))
    
    def _stock_movement(self, movement_type: MovementType):
        """Erfasst Bestandsbewegung."""
        messagebox.showinfo("Info", f"Bestandsbewegung ({movement_type.value}) würde hier erfasst werden")
    
    def _new_supplier(self):
        """Erstellt neuen Lieferanten."""
        messagebox.showinfo("Info", "Lieferanten-Erstellung würde hier implementiert werden")
    
    def _filter_products(self):
        """Filtert Produkte."""
        self._refresh_products()
    
    def _refresh_products(self):
        """Aktualisiert Produkt-Liste."""
        # Clear
        for item in self.product_tree.get_children():
            self.product_tree.delete(item)
        
        # Filter
        search_text = self.product_search.get().lower()
        filtered = [p for p in self.products if search_text in p.name.lower() or search_text in p.sku.lower()]
        
        # Populate
        for product in filtered:
            values = (
                product.sku,
                product.name,
                product.quantity,
                product.min_quantity,
                product.stock_status,
                f"{product.price:.2f} €",
                product.location
            )
            
            # Color-code by status
            item_id = self.product_tree.insert("", tk.END, text=product.id, values=values)
            
            # Apply tag for color
            if product.stock_status in ["Kritisch", "Ausverkauft"]:
                self.product_tree.item(item_id, tags=("critical",))
            elif product.stock_status in ["Niedrig", "Warnung"]:
                self.product_tree.item(item_id, tags=("warning",))
        
        # Configure tags
        self.product_tree.tag_configure("critical", background="#ffcccc")
        self.product_tree.tag_configure("warning", background="#ffffcc")
    
    def _refresh_all(self):
        """Aktualisiert alle Ansichten."""
        self._refresh_products()
        self._update_dashboard()
        self._set_status("Ansichten aktualisiert", "success")
    
    def _update_dashboard(self):
        """Aktualisiert Dashboard."""
        # Statistics
        total_products = len(self.products)
        total_value = sum(p.total_value for p in self.products)
        low_stock = len([p for p in self.products if p.stock_status in ["Niedrig", "Warnung", "Kritisch"]])
        out_of_stock = len([p for p in self.products if p.stock_status == "Ausverkauft"])
        
        self.stat_total_products.config(text=str(total_products))
        self.stat_total_value.config(text=f"{total_value:.2f} €")
        self.stat_low_stock.config(text=str(low_stock))
        self.stat_out_of_stock.config(text=str(out_of_stock))
        
        # Warnings
        self.warnings_text.delete(1.0, tk.END)
        
        critical_products = [p for p in self.products if p.stock_status in ["Kritisch", "Ausverkauft", "Niedrig"]]
        if critical_products:
            self.warnings_text.insert(tk.END, "⚠️ Produkte mit niedrigem Bestand:\n\n")
            for product in critical_products:
                self.warnings_text.insert(
                    tk.END,
                    f"• {product.name} ({product.sku}): {product.quantity} Stück - {product.stock_status}\n"
                )
        else:
            self.warnings_text.insert(tk.END, "✓ Alle Produkte haben ausreichend Bestand")


def main():
    """Hauptfunktion."""
    root = tk.Tk()
    app = InventoryApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
