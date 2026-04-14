"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            main.py                                            ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:49:16                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     522                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
DMS/ERP-System - Beispiel 08
Demonstriert komplexe Enterprise-Anwendung mit ThemisDB

Features:
- Dokumenten-Management-System (DMS)
- Enterprise Resource Planning (ERP)
- Multi-Model-Architektur
- Audit-Logging für Compliance
- Umfassendes Dashboard
"""

import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext
from typing import List, Optional
from datetime import datetime
import uuid

from models import (
    Document, Customer, Order, OrderItem, AuditLog,
    DocumentType, DocumentStatus, Priority, OrderStatus,
    DMSStats
)
from themis_client import DMSERPClient


class DMSERPApp:
    """Haupt-Anwendung für DMS/ERP-System"""
    
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("ThemisDB DMS/ERP-System - Beispiel 08")
        self.root.geometry("1400x900")
        
        # Client initialisieren
        self.client = DMSERPClient()
        
        # Caches
        self.documents: List[Document] = []
        self.customers: List[Customer] = []
        self.orders: List[Order] = []
        
        # UI aufbauen
        self.setup_ui()
        
        # Demo-Daten laden
        self.load_demo_data()
        
        # Initial laden
        self.refresh_all()
    
    def setup_ui(self):
        """Erstellt die Benutzeroberfläche"""
        # Menüleiste
        self.create_menu()
        
        # Haupt-Notebook mit Tabs
        self.notebook = ttk.Notebook(self.root)
        self.notebook.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Tabs
        self.create_dashboard_tab()
        self.create_documents_tab()
        self.create_customers_tab()
        self.create_orders_tab()
        self.create_audit_tab()
        
        # Statusleiste
        self.status_bar = ttk.Label(self.root, text="Bereit", relief=tk.SUNKEN)
        self.status_bar.pack(side=tk.BOTTOM, fill=tk.X)
    
    def create_menu(self):
        """Erstellt Menüleiste"""
        menubar = tk.Menu(self.root)
        self.root.config(menu=menubar)
        
        # Datei-Menü
        file_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="Datei", menu=file_menu)
        file_menu.add_command(label="Demo-Daten laden", command=self.load_demo_data)
        file_menu.add_command(label="Aktualisieren (F5)", command=self.refresh_all)
        file_menu.add_separator()
        file_menu.add_command(label="Beenden", command=self.root.quit)
        
        # Hilfe-Menü
        help_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="Hilfe", menu=help_menu)
        help_menu.add_command(label="Über", command=self.show_about)
        
        # Tastenkombinationen
        self.root.bind('<F5>', lambda e: self.refresh_all())
    
    def create_dashboard_tab(self):
        """Erstellt Dashboard-Tab"""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="📊 Dashboard")
        
        # Titel
        title = ttk.Label(frame, text="DMS/ERP Dashboard", font=('Arial', 16, 'bold'))
        title.pack(pady=10)
        
        # Statistik-Container
        stats_frame = ttk.Frame(frame)
        stats_frame.pack(fill=tk.BOTH, expand=True, padx=20, pady=10)
        
        # DMS-Statistiken
        dms_frame = ttk.LabelFrame(stats_frame, text="📄 Dokumenten-Management", padding=10)
        dms_frame.grid(row=0, column=0, padx=10, pady=10, sticky='nsew')
        
        self.dms_stats_text = scrolledtext.ScrolledText(dms_frame, height=10, width=40)
        self.dms_stats_text.pack(fill=tk.BOTH, expand=True)
        
        # ERP-Statistiken (Kunden)
        customer_frame = ttk.LabelFrame(stats_frame, text="👥 Kunden", padding=10)
        customer_frame.grid(row=0, column=1, padx=10, pady=10, sticky='nsew')
        
        self.customer_stats_text = scrolledtext.ScrolledText(customer_frame, height=10, width=40)
        self.customer_stats_text.pack(fill=tk.BOTH, expand=True)
        
        # ERP-Statistiken (Bestellungen)
        order_frame = ttk.LabelFrame(stats_frame, text="📦 Bestellungen", padding=10)
        order_frame.grid(row=1, column=0, padx=10, pady=10, sticky='nsew')
        
        self.order_stats_text = scrolledtext.ScrolledText(order_frame, height=10, width=40)
        self.order_stats_text.pack(fill=tk.BOTH, expand=True)
        
        # System-Info
        system_frame = ttk.LabelFrame(stats_frame, text="⚙️ System", padding=10)
        system_frame.grid(row=1, column=1, padx=10, pady=10, sticky='nsew')
        
        self.system_stats_text = scrolledtext.ScrolledText(system_frame, height=10, width=40)
        self.system_stats_text.pack(fill=tk.BOTH, expand=True)
        
        # Grid-Konfiguration
        stats_frame.columnconfigure(0, weight=1)
        stats_frame.columnconfigure(1, weight=1)
        stats_frame.rowconfigure(0, weight=1)
        stats_frame.rowconfigure(1, weight=1)
        
        # Aktualisieren-Button
        ttk.Button(frame, text="🔄 Dashboard aktualisieren", 
                   command=self.update_dashboard).pack(pady=10)
    
    def create_documents_tab(self):
        """Erstellt Dokumente-Tab"""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="📄 Dokumente")
        
        # Toolbar
        toolbar = ttk.Frame(frame)
        toolbar.pack(fill=tk.X, padx=5, pady=5)
        
        ttk.Button(toolbar, text="➕ Neu", command=self.create_document).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="✏️ Bearbeiten", command=self.edit_document).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="🗑️ Löschen", command=self.delete_document).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="🔄 Aktualisieren", command=self.refresh_documents).pack(side=tk.LEFT, padx=2)
        
        # Suchleiste
        search_frame = ttk.Frame(toolbar)
        search_frame.pack(side=tk.RIGHT, padx=5)
        
        ttk.Label(search_frame, text="🔍 Suche:").pack(side=tk.LEFT, padx=2)
        self.doc_search_var = tk.StringVar()
        self.doc_search_var.trace('w', lambda *args: self.filter_documents())
        ttk.Entry(search_frame, textvariable=self.doc_search_var, width=30).pack(side=tk.LEFT)
        
        # Dokumente-Liste
        list_frame = ttk.Frame(frame)
        list_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        columns = ('ID', 'Titel', 'Typ', 'Status', 'Priorität', 'Erstellt')
        self.doc_tree = ttk.Treeview(list_frame, columns=columns, show='headings', height=20)
        
        for col in columns:
            self.doc_tree.heading(col, text=col)
            self.doc_tree.column(col, width=150)
        
        scrollbar = ttk.Scrollbar(list_frame, orient=tk.VERTICAL, command=self.doc_tree.yview)
        self.doc_tree.configure(yscrollcommand=scrollbar.set)
        
        self.doc_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
    
    def create_customers_tab(self):
        """Erstellt Kunden-Tab"""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="👥 Kunden")
        
        # Toolbar
        toolbar = ttk.Frame(frame)
        toolbar.pack(fill=tk.X, padx=5, pady=5)
        
        ttk.Button(toolbar, text="➕ Neu", command=self.create_customer).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="✏️ Bearbeiten", command=self.edit_customer).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="🔄 Aktualisieren", command=self.refresh_customers).pack(side=tk.LEFT, padx=2)
        
        # Kunden-Liste
        list_frame = ttk.Frame(frame)
        list_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        columns = ('Kundennr.', 'Name', 'Firma', 'Email', 'Telefon', 'Stadt', 'Saldo')
        self.customer_tree = ttk.Treeview(list_frame, columns=columns, show='headings', height=20)
        
        for col in columns:
            self.customer_tree.heading(col, text=col)
            self.customer_tree.column(col, width=150)
        
        scrollbar = ttk.Scrollbar(list_frame, orient=tk.VERTICAL, command=self.customer_tree.yview)
        self.customer_tree.configure(yscrollcommand=scrollbar.set)
        
        self.customer_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
    
    def create_orders_tab(self):
        """Erstellt Bestellungen-Tab"""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="📦 Bestellungen")
        
        # Toolbar
        toolbar = ttk.Frame(frame)
        toolbar.pack(fill=tk.X, padx=5, pady=5)
        
        ttk.Button(toolbar, text="➕ Neu", command=self.create_order).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="👁️ Anzeigen", command=self.view_order).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="🔄 Aktualisieren", command=self.refresh_orders).pack(side=tk.LEFT, padx=2)
        
        # Bestellungen-Liste
        list_frame = ttk.Frame(frame)
        list_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        columns = ('Bestellnr.', 'Kunde', 'Status', 'Summe', 'Steuer', 'Gesamt', 'Datum')
        self.order_tree = ttk.Treeview(list_frame, columns=columns, show='headings', height=20)
        
        for col in columns:
            self.order_tree.heading(col, text=col)
            self.order_tree.column(col, width=150)
        
        scrollbar = ttk.Scrollbar(list_frame, orient=tk.VERTICAL, command=self.order_tree.yview)
        self.order_tree.configure(yscrollcommand=scrollbar.set)
        
        self.order_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
    
    def create_audit_tab(self):
        """Erstellt Audit-Log-Tab"""
        frame = ttk.Frame(self.notebook)
        self.notebook.add(frame, text="📋 Audit-Log")
        
        # Toolbar
        toolbar = ttk.Frame(frame)
        toolbar.pack(fill=tk.X, padx=5, pady=5)
        
        ttk.Button(toolbar, text="🔄 Aktualisieren", command=self.refresh_audit_logs).pack(side=tk.LEFT, padx=2)
        
        # Audit-Log-Liste
        list_frame = ttk.Frame(frame)
        list_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        columns = ('Zeitstempel', 'Benutzer', 'Entität', 'Aktion', 'Änderungen')
        self.audit_tree = ttk.Treeview(list_frame, columns=columns, show='headings', height=20)
        
        for col in columns:
            self.audit_tree.heading(col, text=col)
            self.audit_tree.column(col, width=200)
        
        scrollbar = ttk.Scrollbar(list_frame, orient=tk.VERTICAL, command=self.audit_tree.yview)
        self.audit_tree.configure(yscrollcommand=scrollbar.set)
        
        self.audit_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
    
    # ==================== Daten-Operationen ====================
    
    def load_demo_data(self):
        """Lädt Demo-Daten"""
        # Demo-Dokumente
        demo_docs = [
            Document(
                id="doc1", title="Rechnung 2024-001", content="Rechnung für Projekt Alpha",
                doc_type=DocumentType.INVOICE.value, status=DocumentStatus.APPROVED.value,
                priority=Priority.NORMAL.value, tags=["rechnung", "2024"],
                created_at=datetime.now().isoformat(), updated_at=datetime.now().isoformat(),
                created_by="admin", file_size=15000
            ),
            Document(
                id="doc2", title="Vertrag ABC-Corp", content="Liefervertrag mit ABC Corp",
                doc_type=DocumentType.CONTRACT.value, status=DocumentStatus.PENDING.value,
                priority=Priority.HIGH.value, tags=["vertrag", "wichtig"],
                created_at=datetime.now().isoformat(), updated_at=datetime.now().isoformat(),
                created_by="admin", file_size=25000
            ),
        ]
        
        for doc in demo_docs:
            self.client.create_document(doc)
        
        # Demo-Kunden
        demo_customers = [
            Customer(
                id="cust1", name="Max Mustermann", company="ABC Corp",
                email="max@abc-corp.de", phone="+49 123 456789",
                address="Musterstraße 1", city="München", postal_code="80331",
                country="Deutschland", customer_number="K-001",
                credit_limit=50000.0, balance=12500.0,
                created_at=datetime.now().isoformat(), notes="Stammkunde seit 2020"
            ),
            Customer(
                id="cust2", name="Anna Schmidt", company="XYZ GmbH",
                email="anna@xyz.de", phone="+49 987 654321",
                address="Beispielweg 10", city="Berlin", postal_code="10115",
                country="Deutschland", customer_number="K-002",
                credit_limit=30000.0, balance=5000.0,
                created_at=datetime.now().isoformat(), notes="Neukunde 2024"
            ),
        ]
        
        for customer in demo_customers:
            self.client.create_customer(customer)
        
        self.set_status("Demo-Daten geladen")
        self.refresh_all()
    
    def refresh_all(self):
        """Aktualisiert alle Daten"""
        self.refresh_documents()
        self.refresh_customers()
        self.refresh_orders()
        self.refresh_audit_logs()
        self.update_dashboard()
        self.set_status("Alle Daten aktualisiert")
    
    def refresh_documents(self):
        """Aktualisiert Dokumente"""
        self.documents = self.client.list_documents()
        self.update_document_list()
    
    def refresh_customers(self):
        """Aktualisiert Kunden"""
        self.customers = self.client.list_customers()
        self.update_customer_list()
    
    def refresh_orders(self):
        """Aktualisiert Bestellungen"""
        self.orders = self.client.list_orders()
        self.update_order_list()
    
    def refresh_audit_logs(self):
        """Aktualisiert Audit-Logs"""
        logs = self.client.get_audit_logs()
        self.update_audit_list(logs)
    
    # ==================== UI-Updates ====================
    
    def update_dashboard(self):
        """Aktualisiert Dashboard"""
        # DMS-Statistiken
        doc_stats = DMSStats.calculate_document_stats(self.documents)
        self.dms_stats_text.delete('1.0', tk.END)
        self.dms_stats_text.insert('1.0', f"Gesamt Dokumente: {doc_stats['total']}\n\n")
        self.dms_stats_text.insert(tk.END, "Nach Typ:\n")
        for doc_type, count in doc_stats['by_type'].items():
            self.dms_stats_text.insert(tk.END, f"  {doc_type}: {count}\n")
        self.dms_stats_text.insert(tk.END, f"\nGesamtgröße: {doc_stats['total_size'] / 1024:.2f} KB")
        
        # Kunden-Statistiken
        customer_stats = DMSStats.calculate_customer_stats(self.customers)
        self.customer_stats_text.delete('1.0', tk.END)
        self.customer_stats_text.insert('1.0', f"Gesamt Kunden: {customer_stats['total']}\n\n")
        self.customer_stats_text.insert(tk.END, f"Kreditlimit: {customer_stats['total_credit_limit']:.2f} €\n")
        self.customer_stats_text.insert(tk.END, f"Gesamt Saldo: {customer_stats['total_balance']:.2f} €\n")
        self.customer_stats_text.insert(tk.END, f"Ø Saldo: {customer_stats['avg_balance']:.2f} €")
        
        # Bestellungs-Statistiken
        order_stats = DMSStats.calculate_order_stats(self.orders)
        self.order_stats_text.delete('1.0', tk.END)
        self.order_stats_text.insert('1.0', f"Gesamt Bestellungen: {order_stats['total']}\n\n")
        self.order_stats_text.insert(tk.END, "Nach Status:\n")
        for status, count in order_stats['by_status'].items():
            self.order_stats_text.insert(tk.END, f"  {status}: {count}\n")
        self.order_stats_text.insert(tk.END, f"\nGesamtumsatz: {order_stats['total_revenue']:.2f} €\n")
        self.order_stats_text.insert(tk.END, f"Ø Bestellwert: {order_stats['avg_order_value']:.2f} €")
        
        # System-Info
        self.system_stats_text.delete('1.0', tk.END)
        connected = self.client.check_connection()
        status = "✅ Verbunden" if connected else "❌ Nicht verbunden"
        self.system_stats_text.insert('1.0', f"ThemisDB: {status}\n\n")
        self.system_stats_text.insert(tk.END, f"Dokumente im Cache: {len(self.documents)}\n")
        self.system_stats_text.insert(tk.END, f"Kunden im Cache: {len(self.customers)}\n")
        self.system_stats_text.insert(tk.END, f"Bestellungen im Cache: {len(self.orders)}\n")
    
    def update_document_list(self):
        """Aktualisiert Dokumente-Liste"""
        self.doc_tree.delete(*self.doc_tree.get_children())
        for doc in self.documents:
            self.doc_tree.insert('', tk.END, values=(
                doc.id[:8], doc.title, doc.doc_type, doc.status,
                doc.priority, doc.created_at[:10]
            ))
    
    def update_customer_list(self):
        """Aktualisiert Kunden-Liste"""
        self.customer_tree.delete(*self.customer_tree.get_children())
        for customer in self.customers:
            self.customer_tree.insert('', tk.END, values=(
                customer.customer_number, customer.name, customer.company,
                customer.email, customer.phone, customer.city,
                f"{customer.balance:.2f} €"
            ))
    
    def update_order_list(self):
        """Aktualisiert Bestellungen-Liste"""
        self.order_tree.delete(*self.order_tree.get_children())
        for order in self.orders:
            self.order_tree.insert('', tk.END, values=(
                order.order_number, order.customer_name, order.status,
                f"{order.subtotal:.2f} €", f"{order.tax:.2f} €",
                f"{order.total:.2f} €", order.created_at[:10]
            ))
    
    def update_audit_list(self, logs: List[AuditLog]):
        """Aktualisiert Audit-Log-Liste"""
        self.audit_tree.delete(*self.audit_tree.get_children())
        for log in logs:
            changes_str = str(log.changes) if log.changes else "-"
            self.audit_tree.insert('', tk.END, values=(
                log.timestamp[:19], log.user, f"{log.entity_type}/{log.entity_id[:8]}",
                log.action, changes_str[:50]
            ))
    
    def filter_documents(self):
        """Filtert Dokumente nach Suchtext"""
        query = self.doc_search_var.get().lower()
        if not query:
            self.update_document_list()
            return
        
        filtered = [d for d in self.documents if query in d.title.lower() or query in d.content.lower()]
        self.doc_tree.delete(*self.doc_tree.get_children())
        for doc in filtered:
            self.doc_tree.insert('', tk.END, values=(
                doc.id[:8], doc.title, doc.doc_type, doc.status,
                doc.priority, doc.created_at[:10]
            ))
    
    # ==================== CRUD-Operationen (Stubs) ====================
    
    def create_document(self):
        messagebox.showinfo("Info", "Dokument erstellen - Feature kommt bald")
    
    def edit_document(self):
        messagebox.showinfo("Info", "Dokument bearbeiten - Feature kommt bald")
    
    def delete_document(self):
        messagebox.showinfo("Info", "Dokument löschen - Feature kommt bald")
    
    def create_customer(self):
        messagebox.showinfo("Info", "Kunde erstellen - Feature kommt bald")
    
    def edit_customer(self):
        messagebox.showinfo("Info", "Kunde bearbeiten - Feature kommt bald")
    
    def create_order(self):
        messagebox.showinfo("Info", "Bestellung erstellen - Feature kommt bald")
    
    def view_order(self):
        messagebox.showinfo("Info", "Bestellung anzeigen - Feature kommt bald")
    
    # ==================== Hilfsfunktionen ====================
    
    def set_status(self, message: str):
        """Setzt Statusleisten-Text"""
        self.status_bar.config(text=message)
        self.root.update_idletasks()
    
    def show_about(self):
        """Zeigt About-Dialog"""
        messagebox.showinfo(
            "Über DMS/ERP-System",
            "ThemisDB DMS/ERP-System - Beispiel 08\n\n"
            "Demonstriert komplexe Enterprise-Anwendung mit:\n"
            "- Dokumenten-Management (DMS)\n"
            "- Enterprise Resource Planning (ERP)\n"
            "- Multi-Model-Architektur\n"
            "- Audit-Logging\n"
            "- Umfassendes Dashboard\n\n"
            "Powered by ThemisDB"
        )


def main():
    """Hauptfunktion"""
    root = tk.Tk()
    app = DMSERPApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
