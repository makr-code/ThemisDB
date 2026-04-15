"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            main.py                                            ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:05:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     793                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
Kontaktmanager - Adressbuch mit ThemisDB
Tkinter GUI für Kontaktverwaltung
"""

import tkinter as tk
from tkinter import ttk, messagebox, filedialog, scrolledtext
import uuid
from typing import List, Optional
from themis_client import ContactClient, ExportHandler
from models import Contact, ContactCategory, Address


# Konfiguration
THEMIS_HOST = "localhost"
THEMIS_PORT = 8080
WINDOW_WIDTH = 900
WINDOW_HEIGHT = 650


class ContactManagerApp:
    """
    Hauptanwendung für Kontaktverwaltung.
    """
    
    def __init__(self, root: tk.Tk):
        """Initialisiert die Anwendung."""
        self.root = root
        self.root.title("ThemisDB - Kontaktmanager")
        self.root.geometry(f"{WINDOW_WIDTH}x{WINDOW_HEIGHT}")
        
        # Client
        self.client = ContactClient(host=THEMIS_HOST, port=THEMIS_PORT)
        self.export_handler = ExportHandler()
        
        # Kontakt-Liste (in-memory cache)
        self.contacts: List[Contact] = []
        self.selected_contact_id: Optional[str] = None
        
        # UI erstellen
        self._create_menu()
        self._create_ui()
        
        # Verbindung prüfen
        self._check_connection()
        
        # Tastenkombinationen
        self._setup_keybindings()
    
    def _create_menu(self):
        """Erstellt Menüleiste."""
        menubar = tk.Menu(self.root)
        self.root.config(menu=menubar)
        
        # Datei-Menü
        file_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="Datei", menu=file_menu)
        file_menu.add_command(label="Neuer Kontakt", command=self._new_contact, accelerator="Ctrl+N")
        file_menu.add_separator()
        file_menu.add_command(label="Importieren (JSON)...", command=lambda: self._import_contacts("json"), accelerator="Ctrl+I")
        file_menu.add_command(label="Importieren (CSV)...", command=lambda: self._import_contacts("csv"))
        file_menu.add_command(label="Exportieren (JSON)...", command=lambda: self._export_contacts("json"), accelerator="Ctrl+Shift+E")
        file_menu.add_command(label="Exportieren (CSV)...", command=lambda: self._export_contacts("csv"))
        file_menu.add_separator()
        file_menu.add_command(label="Beenden", command=self.root.quit, accelerator="Ctrl+Q")
        
        # Bearbeiten-Menü
        edit_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="Bearbeiten", menu=edit_menu)
        edit_menu.add_command(label="Bearbeiten", command=self._edit_contact, accelerator="Ctrl+E")
        edit_menu.add_command(label="Löschen", command=self._delete_contact, accelerator="Ctrl+D")
        edit_menu.add_separator()
        edit_menu.add_command(label="Als Favorit markieren", command=self._toggle_favorite)
    
    def _create_ui(self):
        """Erstellt die Benutzeroberfläche."""
        # Header
        self._create_header()
        
        # Main Content
        self.paned = tk.PanedWindow(self.root, orient=tk.HORIZONTAL, sashrelief=tk.RAISED)
        self.paned.pack(fill=tk.BOTH, expand=True)
        
        # Linke Seite: Kontakt-Liste
        self._create_contact_list_panel()
        
        # Rechte Seite: Kontakt-Details
        self._create_contact_details_panel()
        
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
            text="📇 Kontaktmanager",
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
            text="➕ Neu",
            command=self._new_contact,
            bg="#27ae60",
            fg="white",
            font=("Arial", 10),
            padx=10
        ).pack(side=tk.LEFT, padx=2)
        
        tk.Button(
            toolbar,
            text="✏️ Bearbeiten",
            command=self._edit_contact,
            bg="#3498db",
            fg="white",
            font=("Arial", 10),
            padx=10
        ).pack(side=tk.LEFT, padx=2)
        
        tk.Button(
            toolbar,
            text="🗑️ Löschen",
            command=self._delete_contact,
            bg="#e74c3c",
            fg="white",
            font=("Arial", 10),
            padx=10
        ).pack(side=tk.LEFT, padx=2)
    
    def _create_contact_list_panel(self):
        """Erstellt Kontakt-Listen-Panel."""
        list_frame = tk.Frame(self.root)
        self.paned.add(list_frame, width=350)
        
        # Filter-Leiste
        filter_frame = tk.Frame(list_frame, bg="#ecf0f1", height=40)
        filter_frame.pack(fill=tk.X)
        filter_frame.pack_propagate(False)
        
        tk.Label(filter_frame, text="Kategorie:", bg="#ecf0f1").pack(side=tk.LEFT, padx=5)
        
        self.category_filter = ttk.Combobox(
            filter_frame,
            values=["Alle", "Freunde", "Familie", "Arbeit", "Sonstiges"],
            state="readonly",
            width=12
        )
        self.category_filter.set("Alle")
        self.category_filter.pack(side=tk.LEFT, padx=5)
        self.category_filter.bind("<<ComboboxSelected>>", lambda e: self._apply_filters())
        
        # Favoriten-Checkbox
        self.favorites_only = tk.BooleanVar()
        tk.Checkbutton(
            filter_frame,
            text="⭐ Nur Favoriten",
            variable=self.favorites_only,
            bg="#ecf0f1",
            command=self._apply_filters
        ).pack(side=tk.LEFT, padx=5)
        
        # Suche
        search_frame = tk.Frame(list_frame, bg="#ecf0f1", height=35)
        search_frame.pack(fill=tk.X)
        search_frame.pack_propagate(False)
        
        tk.Label(search_frame, text="🔍", bg="#ecf0f1").pack(side=tk.LEFT, padx=5)
        self.search_entry = tk.Entry(search_frame)
        self.search_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=5)
        self.search_entry.bind("<KeyRelease>", lambda e: self._apply_filters())
        
        # Kontakt-Liste
        list_container = tk.Frame(list_frame)
        list_container.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        scrollbar = tk.Scrollbar(list_container)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        self.contact_listbox = tk.Listbox(
            list_container,
            yscrollcommand=scrollbar.set,
            font=("Arial", 11),
            selectmode=tk.SINGLE
        )
        self.contact_listbox.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.config(command=self.contact_listbox.yview)
        
        self.contact_listbox.bind("<<ListboxSelect>>", self._on_contact_select)
        self.contact_listbox.bind("<Double-Button-1>", lambda e: self._edit_contact())
    
    def _create_contact_details_panel(self):
        """Erstellt Kontakt-Details-Panel."""
        details_frame = tk.Frame(self.root, bg="white")
        self.paned.add(details_frame, width=550)
        
        # Header
        header = tk.Label(
            details_frame,
            text="Kontakt-Details",
            font=("Arial", 14, "bold"),
            bg="white",
            anchor="w"
        )
        header.pack(fill=tk.X, padx=10, pady=10)
        
        # Scrollable form
        canvas = tk.Canvas(details_frame, bg="white", highlightthickness=0)
        scrollbar = tk.Scrollbar(details_frame, orient="vertical", command=canvas.yview)
        scrollable_frame = tk.Frame(canvas, bg="white")
        
        scrollable_frame.bind(
            "<Configure>",
            lambda e: canvas.configure(scrollregion=canvas.bbox("all"))
        )
        
        canvas.create_window((0, 0), window=scrollable_frame, anchor="nw")
        canvas.configure(yscrollcommand=scrollbar.set)
        
        canvas.pack(side="left", fill="both", expand=True)
        scrollbar.pack(side="right", fill="y")
        
        form_frame = tk.Frame(scrollable_frame, bg="white")
        form_frame.pack(fill=tk.BOTH, expand=True, padx=10)
        
        # Name
        name_frame = tk.Frame(form_frame, bg="white")
        name_frame.pack(fill=tk.X, pady=(0, 10))
        
        first_frame = tk.Frame(name_frame, bg="white")
        first_frame.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 5))
        tk.Label(first_frame, text="Vorname:*", bg="white", anchor="w").pack(fill=tk.X)
        self.first_name_entry = tk.Entry(first_frame, font=("Arial", 11))
        self.first_name_entry.pack(fill=tk.X)
        
        last_frame = tk.Frame(name_frame, bg="white")
        last_frame.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(5, 0))
        tk.Label(last_frame, text="Nachname:*", bg="white", anchor="w").pack(fill=tk.X)
        self.last_name_entry = tk.Entry(last_frame, font=("Arial", 11))
        self.last_name_entry.pack(fill=tk.X)
        
        # Email und Telefon
        contact_frame = tk.Frame(form_frame, bg="white")
        contact_frame.pack(fill=tk.X, pady=(0, 10))
        
        email_frame = tk.Frame(contact_frame, bg="white")
        email_frame.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 5))
        tk.Label(email_frame, text="Email:", bg="white", anchor="w").pack(fill=tk.X)
        self.email_entry = tk.Entry(email_frame, font=("Arial", 11))
        self.email_entry.pack(fill=tk.X)
        
        phone_frame = tk.Frame(contact_frame, bg="white")
        phone_frame.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(5, 0))
        tk.Label(phone_frame, text="Telefon:", bg="white", anchor="w").pack(fill=tk.X)
        self.phone_entry = tk.Entry(phone_frame, font=("Arial", 11))
        self.phone_entry.pack(fill=tk.X)
        
        # Adresse
        tk.Label(form_frame, text="Adresse:", bg="white", anchor="w", font=("Arial", 10, "bold")).pack(fill=tk.X, pady=(5, 2))
        
        tk.Label(form_frame, text="Straße:", bg="white", anchor="w").pack(fill=tk.X)
        self.street_entry = tk.Entry(form_frame, font=("Arial", 11))
        self.street_entry.pack(fill=tk.X, pady=(0, 5))
        
        addr_frame = tk.Frame(form_frame, bg="white")
        addr_frame.pack(fill=tk.X, pady=(0, 10))
        
        plz_frame = tk.Frame(addr_frame, bg="white")
        plz_frame.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 5))
        tk.Label(plz_frame, text="PLZ:", bg="white", anchor="w").pack(fill=tk.X)
        self.postal_code_entry = tk.Entry(plz_frame, font=("Arial", 11))
        self.postal_code_entry.pack(fill=tk.X)
        
        city_frame = tk.Frame(addr_frame, bg="white")
        city_frame.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(5, 0))
        tk.Label(city_frame, text="Stadt:", bg="white", anchor="w").pack(fill=tk.X)
        self.city_entry = tk.Entry(city_frame, font=("Arial", 11))
        self.city_entry.pack(fill=tk.X)
        
        tk.Label(form_frame, text="Land:", bg="white", anchor="w").pack(fill=tk.X)
        self.country_entry = tk.Entry(form_frame, font=("Arial", 11))
        self.country_entry.pack(fill=tk.X, pady=(0, 10))
        
        # Kategorie und Favorit
        cat_fav_frame = tk.Frame(form_frame, bg="white")
        cat_fav_frame.pack(fill=tk.X, pady=(0, 10))
        
        cat_frame = tk.Frame(cat_fav_frame, bg="white")
        cat_frame.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 5))
        tk.Label(cat_frame, text="Kategorie:", bg="white", anchor="w").pack(fill=tk.X)
        self.category_combo = ttk.Combobox(
            cat_frame,
            values=["Freunde", "Familie", "Arbeit", "Sonstiges"],
            state="readonly"
        )
        self.category_combo.set("Sonstiges")
        self.category_combo.pack(fill=tk.X)
        
        fav_frame = tk.Frame(cat_fav_frame, bg="white")
        fav_frame.pack(side=tk.LEFT, padx=(5, 0))
        tk.Label(fav_frame, text=" ", bg="white").pack()  # Spacer
        self.is_favorite_var = tk.BooleanVar()
        tk.Checkbutton(
            fav_frame,
            text="⭐ Favorit",
            variable=self.is_favorite_var,
            bg="white",
            font=("Arial", 10)
        ).pack()
        
        # Notizen
        tk.Label(form_frame, text="Notizen:", bg="white", anchor="w").pack(fill=tk.X, pady=(0, 2))
        self.notes_text = scrolledtext.ScrolledText(
            form_frame,
            height=4,
            font=("Arial", 10),
            wrap=tk.WORD
        )
        self.notes_text.pack(fill=tk.BOTH, expand=True, pady=(0, 10))
        
        # Buttons
        button_frame = tk.Frame(form_frame, bg="white")
        button_frame.pack(fill=tk.X, pady=(10, 0))
        
        self.save_button = tk.Button(
            button_frame,
            text="💾 Speichern",
            command=self._save_contact,
            bg="#27ae60",
            fg="white",
            font=("Arial", 10),
            width=15
        )
        self.save_button.pack(side=tk.LEFT, padx=(0, 5))
        
        tk.Button(
            button_frame,
            text="✖️ Abbrechen",
            command=self._cancel_edit,
            bg="#95a5a6",
            fg="white",
            font=("Arial", 10),
            width=15
        ).pack(side=tk.LEFT)
        
        # Initially disable form
        self._set_form_enabled(False)
    
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
        self.root.bind("<Control-n>", lambda e: self._new_contact())
        self.root.bind("<Control-e>", lambda e: self._edit_contact())
        self.root.bind("<Control-s>", lambda e: self._save_contact())
        self.root.bind("<Control-d>", lambda e: self._delete_contact())
        self.root.bind("<Control-f>", lambda e: self.search_entry.focus())
        self.root.bind("<Control-i>", lambda e: self._import_contacts("json"))
        self.root.bind("<Control-Shift-E>", lambda e: self._export_contacts("json"))
        self.root.bind("<Control-q>", lambda e: self.root.quit())
        self.root.bind("<F5>", lambda e: self._refresh_list())
    
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
    
    def _set_form_enabled(self, enabled: bool):
        """Aktiviert/Deaktiviert Form."""
        state = tk.NORMAL if enabled else tk.DISABLED
        for entry in [self.first_name_entry, self.last_name_entry, self.email_entry,
                     self.phone_entry, self.street_entry, self.city_entry,
                     self.postal_code_entry, self.country_entry]:
            entry.config(state=state)
        self.category_combo.config(state="readonly" if enabled else tk.DISABLED)
        self.notes_text.config(state=state)
        self.save_button.config(state=state)
    
    def _clear_form(self):
        """Löscht Form."""
        self.first_name_entry.delete(0, tk.END)
        self.last_name_entry.delete(0, tk.END)
        self.email_entry.delete(0, tk.END)
        self.phone_entry.delete(0, tk.END)
        self.street_entry.delete(0, tk.END)
        self.city_entry.delete(0, tk.END)
        self.postal_code_entry.delete(0, tk.END)
        self.country_entry.delete(0, tk.END)
        self.category_combo.set("Sonstiges")
        self.is_favorite_var.set(False)
        self.notes_text.delete(1.0, tk.END)
        self.selected_contact_id = None
    
    def _new_contact(self):
        """Erstellt neuen Kontakt."""
        self._clear_form()
        self._set_form_enabled(True)
        self.first_name_entry.focus()
        self._set_status("Neuen Kontakt erstellen", "info")
    
    def _save_contact(self):
        """Speichert Kontakt."""
        first_name = self.first_name_entry.get().strip()
        last_name = self.last_name_entry.get().strip()
        
        if not first_name or not last_name:
            messagebox.showerror("Fehler", "Vor- und Nachname sind erforderlich")
            return
        
        category_map = {
            "Freunde": ContactCategory.FRIENDS,
            "Familie": ContactCategory.FAMILY,
            "Arbeit": ContactCategory.WORK,
            "Sonstiges": ContactCategory.OTHER
        }
        
        address = Address(
            street=self.street_entry.get().strip(),
            city=self.city_entry.get().strip(),
            postal_code=self.postal_code_entry.get().strip(),
            country=self.country_entry.get().strip()
        )
        
        try:
            if self.selected_contact_id:
                # Update
                contact = next((c for c in self.contacts if c.id == self.selected_contact_id), None)
                if contact:
                    contact.first_name = first_name
                    contact.last_name = last_name
                    contact.email = self.email_entry.get().strip()
                    contact.phone = self.phone_entry.get().strip()
                    contact.address = address
                    contact.category = category_map[self.category_combo.get()]
                    contact.is_favorite = self.is_favorite_var.get()
                    contact.notes = self.notes_text.get(1.0, tk.END).strip()
                    self.client.update_contact(contact)
                    self._set_status(f"Kontakt '{contact.full_name}' aktualisiert", "success")
            else:
                # Create
                contact = Contact(
                    id=str(uuid.uuid4()),
                    first_name=first_name,
                    last_name=last_name,
                    email=self.email_entry.get().strip(),
                    phone=self.phone_entry.get().strip(),
                    address=address,
                    category=category_map[self.category_combo.get()],
                    is_favorite=self.is_favorite_var.get(),
                    notes=self.notes_text.get(1.0, tk.END).strip()
                )
                self.client.create_contact(contact)
                self.contacts.append(contact)
                self._set_status(f"Kontakt '{contact.full_name}' erstellt", "success")
            
            self._refresh_list()
            self._clear_form()
            self._set_form_enabled(False)
        except Exception as e:
            self._set_status(f"Fehler: {str(e)}", "error")
            messagebox.showerror("Fehler", str(e))
    
    def _delete_contact(self):
        """Löscht Kontakt."""
        if not self.selected_contact_id:
            messagebox.showwarning("Warnung", "Kein Kontakt ausgewählt")
            return
        
        contact = next((c for c in self.contacts if c.id == self.selected_contact_id), None)
        if not contact:
            return
        
        if not messagebox.askyesno("Bestätigung", f"Kontakt '{contact.full_name}' löschen?"):
            return
        
        try:
            self.client.delete_contact(contact.id)
            self.contacts.remove(contact)
            self._refresh_list()
            self._clear_form()
            self._set_form_enabled(False)
            self._set_status(f"Kontakt '{contact.full_name}' gelöscht", "success")
        except Exception as e:
            self._set_status(f"Fehler: {str(e)}", "error")
            messagebox.showerror("Fehler", str(e))
    
    def _edit_contact(self):
        """Bearbeitet Kontakt."""
        if not self.selected_contact_id:
            return
        
        contact = next((c for c in self.contacts if c.id == self.selected_contact_id), None)
        if not contact:
            return
        
        self._set_form_enabled(True)
        
        self.first_name_entry.delete(0, tk.END)
        self.first_name_entry.insert(0, contact.first_name)
        
        self.last_name_entry.delete(0, tk.END)
        self.last_name_entry.insert(0, contact.last_name)
        
        self.email_entry.delete(0, tk.END)
        self.email_entry.insert(0, contact.email)
        
        self.phone_entry.delete(0, tk.END)
        self.phone_entry.insert(0, contact.phone)
        
        self.street_entry.delete(0, tk.END)
        self.street_entry.insert(0, contact.address.street)
        
        self.city_entry.delete(0, tk.END)
        self.city_entry.insert(0, contact.address.city)
        
        self.postal_code_entry.delete(0, tk.END)
        self.postal_code_entry.insert(0, contact.address.postal_code)
        
        self.country_entry.delete(0, tk.END)
        self.country_entry.insert(0, contact.address.country)
        
        category_map_inv = {
            ContactCategory.FRIENDS: "Freunde",
            ContactCategory.FAMILY: "Familie",
            ContactCategory.WORK: "Arbeit",
            ContactCategory.OTHER: "Sonstiges"
        }
        self.category_combo.set(category_map_inv[contact.category])
        
        self.is_favorite_var.set(contact.is_favorite)
        
        self.notes_text.delete(1.0, tk.END)
        self.notes_text.insert(1.0, contact.notes)
        
        self._set_status(f"Kontakt '{contact.full_name}' bearbeiten", "info")
    
    def _cancel_edit(self):
        """Bricht Bearbeitung ab."""
        self._clear_form()
        self._set_form_enabled(False)
        self._set_status("Bearbeitung abgebrochen", "info")
    
    def _toggle_favorite(self):
        """Schaltet Favorit-Status um."""
        if not self.selected_contact_id:
            return
        
        contact = next((c for c in self.contacts if c.id == self.selected_contact_id), None)
        if not contact:
            return
        
        contact.is_favorite = not contact.is_favorite
        try:
            self.client.update_contact(contact)
            self._refresh_list()
            status = "Favorit" if contact.is_favorite else "kein Favorit mehr"
            self._set_status(f"'{contact.full_name}' ist jetzt {status}", "success")
        except Exception as e:
            self._set_status(f"Fehler: {str(e)}", "error")
    
    def _on_contact_select(self, event):
        """Handler für Kontakt-Auswahl."""
        selection = self.contact_listbox.curselection()
        if not selection:
            return
        
        index = selection[0]
        filtered_contacts = self._get_filtered_contacts()
        if index < len(filtered_contacts):
            contact = filtered_contacts[index]
            self.selected_contact_id = contact.id
            self._display_contact(contact)
    
    def _display_contact(self, contact: Contact):
        """Zeigt Kontakt-Details an."""
        self._set_form_enabled(False)
        
        # Enable to populate, then disable
        for entry, value in [
            (self.first_name_entry, contact.first_name),
            (self.last_name_entry, contact.last_name),
            (self.email_entry, contact.email),
            (self.phone_entry, contact.phone),
            (self.street_entry, contact.address.street),
            (self.city_entry, contact.address.city),
            (self.postal_code_entry, contact.address.postal_code),
            (self.country_entry, contact.address.country)
        ]:
            entry.config(state=tk.NORMAL)
            entry.delete(0, tk.END)
            entry.insert(0, value)
            entry.config(state=tk.DISABLED)
        
        category_map_inv = {
            ContactCategory.FRIENDS: "Freunde",
            ContactCategory.FAMILY: "Familie",
            ContactCategory.WORK: "Arbeit",
            ContactCategory.OTHER: "Sonstiges"
        }
        self.category_combo.set(category_map_inv[contact.category])
        
        self.is_favorite_var.set(contact.is_favorite)
        
        self.notes_text.config(state=tk.NORMAL)
        self.notes_text.delete(1.0, tk.END)
        self.notes_text.insert(1.0, contact.notes)
        self.notes_text.config(state=tk.DISABLED)
    
    def _get_filtered_contacts(self) -> List[Contact]:
        """Gibt gefilterte Kontakt-Liste zurück."""
        filtered = list(self.contacts)
        
        # Kategorie-Filter
        category_filter = self.category_filter.get()
        if category_filter != "Alle":
            category_map = {
                "Freunde": ContactCategory.FRIENDS,
                "Familie": ContactCategory.FAMILY,
                "Arbeit": ContactCategory.WORK,
                "Sonstiges": ContactCategory.OTHER
            }
            filtered = [c for c in filtered if c.category == category_map[category_filter]]
        
        # Favoriten-Filter
        if self.favorites_only.get():
            filtered = [c for c in filtered if c.is_favorite]
        
        # Such-Filter
        search_text = self.search_entry.get()
        if search_text:
            filtered = [c for c in filtered if c.matches_search(search_text)]
        
        return sorted(filtered, key=lambda c: c.full_name)
    
    def _apply_filters(self):
        """Wendet Filter an."""
        self._refresh_list()
    
    def _refresh_list(self):
        """Aktualisiert Kontakt-Liste."""
        self.contact_listbox.delete(0, tk.END)
        
        filtered_contacts = self._get_filtered_contacts()
        for contact in filtered_contacts:
            self.contact_listbox.insert(tk.END, contact.display_name)
        
        # Statistik
        total = len(self.contacts)
        filtered = len(filtered_contacts)
        if filtered < total:
            self._set_status(f"{filtered} von {total} Kontakten angezeigt", "info")
        else:
            self._set_status(f"{total} Kontakte", "info")
    
    def _import_contacts(self, format: str):
        """Importiert Kontakte."""
        filetypes = [
            ("JSON-Dateien", "*.json") if format == "json" else ("CSV-Dateien", "*.csv")
        ]
        filepath = filedialog.askopenfilename(
            title="Kontakte importieren",
            filetypes=filetypes
        )
        
        if not filepath:
            return
        
        try:
            if format == "json":
                imported = self.export_handler.import_json(filepath)
            else:
                imported = self.export_handler.import_csv(filepath)
            
            # Add to contacts and save to ThemisDB
            for contact in imported:
                self.client.create_contact(contact)
                self.contacts.append(contact)
            
            self._refresh_list()
            self._set_status(f"{len(imported)} Kontakte importiert", "success")
            messagebox.showinfo("Erfolg", f"{len(imported)} Kontakte erfolgreich importiert")
        except Exception as e:
            self._set_status(f"Import-Fehler: {str(e)}", "error")
            messagebox.showerror("Fehler", f"Import fehlgeschlagen:\n{str(e)}")
    
    def _export_contacts(self, format: str):
        """Exportiert Kontakte."""
        if not self.contacts:
            messagebox.showwarning("Warnung", "Keine Kontakte zum Exportieren")
            return
        
        filetypes = [
            ("JSON-Dateien", "*.json") if format == "json" else ("CSV-Dateien", "*.csv")
        ]
        filepath = filedialog.asksaveasfilename(
            title="Kontakte exportieren",
            defaultextension=f".{format}",
            filetypes=filetypes
        )
        
        if not filepath:
            return
        
        try:
            if format == "json":
                self.export_handler.export_json(self.contacts, filepath)
            else:
                self.export_handler.export_csv(self.contacts, filepath)
            
            self._set_status(f"{len(self.contacts)} Kontakte exportiert", "success")
            messagebox.showinfo("Erfolg", f"{len(self.contacts)} Kontakte erfolgreich exportiert")
        except Exception as e:
            self._set_status(f"Export-Fehler: {str(e)}", "error")
            messagebox.showerror("Fehler", f"Export fehlgeschlagen:\n{str(e)}")


def main():
    """Hauptfunktion."""
    root = tk.Tk()
    app = ContactManagerApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
