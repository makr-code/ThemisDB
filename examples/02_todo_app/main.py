"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            main.py                                            ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:22:13                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     582                                            ║
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
Todo-App - Aufgabenverwaltung mit ThemisDB
Tkinter GUI für Todo-Listen
"""

import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext
import uuid
from datetime import datetime
from typing import List, Optional
from themis_client import TodoClient
from models import Task, TaskStatus, TaskPriority


# Konfiguration
THEMIS_HOST = "localhost"
THEMIS_PORT = 8080
WINDOW_WIDTH = 800
WINDOW_HEIGHT = 600


class TodoApp:
    """
    Hauptanwendung für Todo-Verwaltung.
    """
    
    def __init__(self, root: tk.Tk):
        """
        Initialisiert die Anwendung.
        
        Args:
            root: Tkinter Root-Widget
        """
        self.root = root
        self.root.title("ThemisDB - Todo-App")
        self.root.geometry(f"{WINDOW_WIDTH}x{WINDOW_HEIGHT}")
        
        # Client
        self.client = TodoClient(host=THEMIS_HOST, port=THEMIS_PORT)
        
        # Task-Liste (in-memory cache für Demo)
        self.tasks: List[Task] = []
        
        # UI erstellen
        self._create_ui()
        
        # Verbindung prüfen
        self._check_connection()
        
        # Tastenkombinationen
        self._setup_keybindings()
    
    def _create_ui(self):
        """Erstellt die Benutzeroberfläche."""
        
        # Header
        self._create_header()
        
        # Main Content (PanedWindow für flexible Layout)
        self.paned = tk.PanedWindow(self.root, orient=tk.HORIZONTAL)
        self.paned.pack(fill=tk.BOTH, expand=True)
        
        # Linke Seite: Task-Liste
        self._create_task_list_panel()
        
        # Rechte Seite: Task-Details
        self._create_task_details_panel()
        
        # Status Bar
        self._create_status_bar()
    
    def _create_header(self):
        """Erstellt Header mit Toolbar."""
        header_frame = tk.Frame(self.root, bg="#2c3e50", height=60)
        header_frame.pack(fill=tk.X)
        header_frame.pack_propagate(False)
        
        # Title
        title_label = tk.Label(
            header_frame,
            text="📝 Todo-App",
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
            command=self._new_task,
            bg="#27ae60",
            fg="white",
            font=("Arial", 10),
            padx=10
        ).pack(side=tk.LEFT, padx=2)
        
        tk.Button(
            toolbar,
            text="🗑️ Löschen",
            command=self._delete_task,
            bg="#e74c3c",
            fg="white",
            font=("Arial", 10),
            padx=10
        ).pack(side=tk.LEFT, padx=2)
    
    def _create_task_list_panel(self):
        """Erstellt das Task-Listen-Panel."""
        list_frame = tk.Frame(self.root)
        self.paned.add(list_frame, width=400)
        
        # Filter-Leiste
        filter_frame = tk.Frame(list_frame, bg="#ecf0f1", height=40)
        filter_frame.pack(fill=tk.X)
        filter_frame.pack_propagate(False)
        
        tk.Label(filter_frame, text="Filter:", bg="#ecf0f1").pack(side=tk.LEFT, padx=5)
        
        # Status-Filter
        self.status_filter = ttk.Combobox(
            filter_frame,
            values=["Alle", "Offen", "In Arbeit", "Erledigt"],
            state="readonly",
            width=12
        )
        self.status_filter.set("Alle")
        self.status_filter.pack(side=tk.LEFT, padx=5)
        self.status_filter.bind("<<ComboboxSelected>>", lambda e: self._apply_filters())
        
        # Prioritäts-Filter
        self.priority_filter = ttk.Combobox(
            filter_frame,
            values=["Alle", "Niedrig", "Normal", "Hoch"],
            state="readonly",
            width=12
        )
        self.priority_filter.set("Alle")
        self.priority_filter.pack(side=tk.LEFT, padx=5)
        self.priority_filter.bind("<<ComboboxSelected>>", lambda e: self._apply_filters())
        
        # Suche
        search_frame = tk.Frame(list_frame, bg="#ecf0f1", height=35)
        search_frame.pack(fill=tk.X)
        search_frame.pack_propagate(False)
        
        tk.Label(search_frame, text="🔍", bg="#ecf0f1").pack(side=tk.LEFT, padx=5)
        self.search_entry = tk.Entry(search_frame)
        self.search_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=5)
        self.search_entry.bind("<KeyRelease>", lambda e: self._apply_filters())
        
        # Task-Liste
        list_container = tk.Frame(list_frame)
        list_container.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        scrollbar = tk.Scrollbar(list_container)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        self.task_listbox = tk.Listbox(
            list_container,
            yscrollcommand=scrollbar.set,
            font=("Arial", 10),
            selectmode=tk.SINGLE
        )
        self.task_listbox.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.config(command=self.task_listbox.yview)
        
        self.task_listbox.bind("<<ListboxSelect>>", self._on_task_select)
        self.task_listbox.bind("<Double-Button-1>", lambda e: self._edit_task())
    
    def _create_task_details_panel(self):
        """Erstellt das Task-Details-Panel."""
        details_frame = tk.Frame(self.root, bg="white")
        self.paned.add(details_frame, width=400)
        
        # Header
        header = tk.Label(
            details_frame,
            text="Task-Details",
            font=("Arial", 14, "bold"),
            bg="white",
            anchor="w"
        )
        header.pack(fill=tk.X, padx=10, pady=10)
        
        # Form
        form_frame = tk.Frame(details_frame, bg="white")
        form_frame.pack(fill=tk.BOTH, expand=True, padx=10)
        
        # Titel
        tk.Label(form_frame, text="Titel:", bg="white", anchor="w").pack(fill=tk.X, pady=(0, 2))
        self.title_entry = tk.Entry(form_frame, font=("Arial", 11))
        self.title_entry.pack(fill=tk.X, pady=(0, 10))
        
        # Beschreibung
        tk.Label(form_frame, text="Beschreibung:", bg="white", anchor="w").pack(fill=tk.X, pady=(0, 2))
        self.description_text = scrolledtext.ScrolledText(
            form_frame,
            height=6,
            font=("Arial", 10),
            wrap=tk.WORD
        )
        self.description_text.pack(fill=tk.BOTH, expand=True, pady=(0, 10))
        
        # Status und Priorität
        status_prio_frame = tk.Frame(form_frame, bg="white")
        status_prio_frame.pack(fill=tk.X, pady=(0, 10))
        
        # Status
        status_frame = tk.Frame(status_prio_frame, bg="white")
        status_frame.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 5))
        tk.Label(status_frame, text="Status:", bg="white", anchor="w").pack(fill=tk.X)
        self.status_combo = ttk.Combobox(
            status_frame,
            values=["Offen", "In Arbeit", "Erledigt"],
            state="readonly"
        )
        self.status_combo.set("Offen")
        self.status_combo.pack(fill=tk.X)
        
        # Priorität
        priority_frame = tk.Frame(status_prio_frame, bg="white")
        priority_frame.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(5, 0))
        tk.Label(priority_frame, text="Priorität:", bg="white", anchor="w").pack(fill=tk.X)
        self.priority_combo = ttk.Combobox(
            priority_frame,
            values=["Niedrig", "Normal", "Hoch"],
            state="readonly"
        )
        self.priority_combo.set("Normal")
        self.priority_combo.pack(fill=tk.X)
        
        # Buttons
        button_frame = tk.Frame(form_frame, bg="white")
        button_frame.pack(fill=tk.X, pady=(10, 0))
        
        self.save_button = tk.Button(
            button_frame,
            text="💾 Speichern",
            command=self._save_task,
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
        self.selected_task_id: Optional[str] = None
        self._set_form_enabled(False)
    
    def _create_status_bar(self):
        """Erstellt die Status-Leiste."""
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
        self.root.bind("<Control-n>", lambda e: self._new_task())
        self.root.bind("<Control-s>", lambda e: self._save_task())
        self.root.bind("<Control-d>", lambda e: self._delete_task())
        self.root.bind("<Control-f>", lambda e: self.search_entry.focus())
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
        """Aktiviert/Deaktiviert Form-Felder."""
        state = tk.NORMAL if enabled else tk.DISABLED
        self.title_entry.config(state=state)
        self.description_text.config(state=state)
        self.status_combo.config(state="readonly" if enabled else tk.DISABLED)
        self.priority_combo.config(state="readonly" if enabled else tk.DISABLED)
        self.save_button.config(state=state)
    
    def _clear_form(self):
        """Löscht Form-Felder."""
        self.title_entry.delete(0, tk.END)
        self.description_text.delete(1.0, tk.END)
        self.status_combo.set("Offen")
        self.priority_combo.set("Normal")
        self.selected_task_id = None
    
    def _new_task(self):
        """Erstellt neue Task."""
        self._clear_form()
        self._set_form_enabled(True)
        self.title_entry.focus()
        self._set_status("Neue Aufgabe erstellen", "info")
    
    def _save_task(self):
        """Speichert aktuelle Task."""
        title = self.title_entry.get().strip()
        
        if not title:
            messagebox.showerror("Fehler", "Titel ist erforderlich")
            return
        
        description = self.description_text.get(1.0, tk.END).strip()
        status_map = {
            "Offen": TaskStatus.OPEN,
            "In Arbeit": TaskStatus.IN_PROGRESS,
            "Erledigt": TaskStatus.DONE
        }
        priority_map = {
            "Niedrig": TaskPriority.LOW,
            "Normal": TaskPriority.NORMAL,
            "Hoch": TaskPriority.HIGH
        }
        
        status = status_map[self.status_combo.get()]
        priority = priority_map[self.priority_combo.get()]
        
        try:
            if self.selected_task_id:
                # Update existing
                task = next((t for t in self.tasks if t.id == self.selected_task_id), None)
                if task:
                    task.title = title
                    task.description = description
                    task.status = status
                    task.priority = priority
                    self.client.update_task(task)
                    self._set_status(f"Aufgabe '{title}' aktualisiert", "success")
            else:
                # Create new
                task = Task(
                    id=str(uuid.uuid4()),
                    title=title,
                    description=description,
                    status=status,
                    priority=priority
                )
                self.client.create_task(task)
                self.tasks.append(task)
                self._set_status(f"Aufgabe '{title}' erstellt", "success")
            
            self._refresh_list()
            self._clear_form()
            self._set_form_enabled(False)
        except Exception as e:
            self._set_status(f"Fehler: {str(e)}", "error")
            messagebox.showerror("Fehler", str(e))
    
    def _delete_task(self):
        """Löscht ausgewählte Task."""
        if not self.selected_task_id:
            messagebox.showwarning("Warnung", "Keine Aufgabe ausgewählt")
            return
        
        task = next((t for t in self.tasks if t.id == self.selected_task_id), None)
        if not task:
            return
        
        if not messagebox.askyesno("Bestätigung", f"Aufgabe '{task.title}' löschen?"):
            return
        
        try:
            self.client.delete_task(task.id)
            self.tasks.remove(task)
            self._refresh_list()
            self._clear_form()
            self._set_form_enabled(False)
            self._set_status(f"Aufgabe '{task.title}' gelöscht", "success")
        except Exception as e:
            self._set_status(f"Fehler: {str(e)}", "error")
            messagebox.showerror("Fehler", str(e))
    
    def _edit_task(self):
        """Bearbeitet ausgewählte Task."""
        if not self.selected_task_id:
            return
        
        task = next((t for t in self.tasks if t.id == self.selected_task_id), None)
        if not task:
            return
        
        self._set_form_enabled(True)
        self.title_entry.delete(0, tk.END)
        self.title_entry.insert(0, task.title)
        
        self.description_text.delete(1.0, tk.END)
        self.description_text.insert(1.0, task.description)
        
        status_map_inv = {
            TaskStatus.OPEN: "Offen",
            TaskStatus.IN_PROGRESS: "In Arbeit",
            TaskStatus.DONE: "Erledigt"
        }
        priority_map_inv = {
            TaskPriority.LOW: "Niedrig",
            TaskPriority.NORMAL: "Normal",
            TaskPriority.HIGH: "Hoch"
        }
        
        self.status_combo.set(status_map_inv[task.status])
        self.priority_combo.set(priority_map_inv[task.priority])
        
        self._set_status(f"Aufgabe '{task.title}' bearbeiten", "info")
    
    def _cancel_edit(self):
        """Bricht Bearbeitung ab."""
        self._clear_form()
        self._set_form_enabled(False)
        self._set_status("Bearbeitung abgebrochen", "info")
    
    def _on_task_select(self, event):
        """Handler für Task-Auswahl."""
        selection = self.task_listbox.curselection()
        if not selection:
            return
        
        index = selection[0]
        filtered_tasks = self._get_filtered_tasks()
        if index < len(filtered_tasks):
            task = filtered_tasks[index]
            self.selected_task_id = task.id
            self._display_task(task)
    
    def _display_task(self, task: Task):
        """Zeigt Task-Details an."""
        self._set_form_enabled(False)
        
        self.title_entry.config(state=tk.NORMAL)
        self.title_entry.delete(0, tk.END)
        self.title_entry.insert(0, task.title)
        self.title_entry.config(state=tk.DISABLED)
        
        self.description_text.config(state=tk.NORMAL)
        self.description_text.delete(1.0, tk.END)
        self.description_text.insert(1.0, task.description)
        self.description_text.config(state=tk.DISABLED)
        
        status_map_inv = {
            TaskStatus.OPEN: "Offen",
            TaskStatus.IN_PROGRESS: "In Arbeit",
            TaskStatus.DONE: "Erledigt"
        }
        priority_map_inv = {
            TaskPriority.LOW: "Niedrig",
            TaskPriority.NORMAL: "Normal",
            TaskPriority.HIGH: "Hoch"
        }
        
        self.status_combo.set(status_map_inv[task.status])
        self.priority_combo.set(priority_map_inv[task.priority])
    
    def _get_filtered_tasks(self) -> List[Task]:
        """Gibt gefilterte Task-Liste zurück."""
        filtered = list(self.tasks)
        
        # Status-Filter
        status_filter = self.status_filter.get()
        if status_filter != "Alle":
            status_map = {
                "Offen": TaskStatus.OPEN,
                "In Arbeit": TaskStatus.IN_PROGRESS,
                "Erledigt": TaskStatus.DONE
            }
            filtered = [t for t in filtered if t.status == status_map[status_filter]]
        
        # Prioritäts-Filter
        priority_filter = self.priority_filter.get()
        if priority_filter != "Alle":
            priority_map = {
                "Niedrig": TaskPriority.LOW,
                "Normal": TaskPriority.NORMAL,
                "Hoch": TaskPriority.HIGH
            }
            filtered = [t for t in filtered if t.priority == priority_map[priority_filter]]
        
        # Such-Filter
        search_text = self.search_entry.get().lower()
        if search_text:
            filtered = [
                t for t in filtered
                if search_text in t.title.lower() or search_text in t.description.lower()
            ]
        
        return filtered
    
    def _apply_filters(self):
        """Wendet Filter an und aktualisiert Liste."""
        self._refresh_list()
    
    def _refresh_list(self):
        """Aktualisiert Task-Liste."""
        self.task_listbox.delete(0, tk.END)
        
        filtered_tasks = self._get_filtered_tasks()
        for task in filtered_tasks:
            self.task_listbox.insert(tk.END, str(task))
        
        # Statistik in Status
        total = len(self.tasks)
        filtered = len(filtered_tasks)
        if filtered < total:
            self._set_status(f"{filtered} von {total} Aufgaben angezeigt", "info")
        else:
            self._set_status(f"{total} Aufgaben", "info")


def main():
    """Hauptfunktion."""
    root = tk.Tk()
    app = TodoApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
