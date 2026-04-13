"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            main.py                                            ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:12:40                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     424                                            ║
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
ThemisDB Hello World - Tkinter GUI Application
Demonstriert grundlegende CRUD-Operationen mit ThemisDB
"""

import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext
import json
from typing import Optional
from themis_client import ThemisDBClient


# Konfiguration
THEMIS_HOST = "localhost"
THEMIS_PORT = 8080
WINDOW_WIDTH = 600
WINDOW_HEIGHT = 600


class HelloWorldApp:
    """
    Hauptanwendung für ThemisDB Hello World Beispiel.
    """
    
    def __init__(self, root: tk.Tk):
        """
        Initialisiert die Anwendung.
        
        Args:
            root: Tkinter Root-Widget
        """
        self.root = root
        self.root.title("ThemisDB - Hello World")
        self.root.geometry(f"{WINDOW_WIDTH}x{WINDOW_HEIGHT}")
        
        # ThemisDB Client
        self.client = ThemisDBClient(host=THEMIS_HOST, port=THEMIS_PORT)
        
        # UI erstellen
        self._create_ui()
        
        # Verbindung prüfen
        self._check_connection()
        
        # Tastenkombinationen
        self._setup_keybindings()
    
    def _create_ui(self):
        """Erstellt die Benutzeroberfläche."""
        
        # Header mit Connection Status
        header_frame = tk.Frame(self.root, bg="#2c3e50", height=50)
        header_frame.pack(fill=tk.X)
        header_frame.pack_propagate(False)
        
        title_label = tk.Label(
            header_frame,
            text="ThemisDB - Hello World",
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
        
        # Main Content Frame
        content_frame = tk.Frame(self.root, padx=20, pady=20)
        content_frame.pack(fill=tk.BOTH, expand=True)
        
        # Eingabefelder
        self._create_input_fields(content_frame)
        
        # Buttons
        self._create_buttons(content_frame)
        
        # Status Bar
        self._create_status_bar(content_frame)
        
        # Output Bereich
        self._create_output_area(content_frame)
    
    def _create_input_fields(self, parent: tk.Frame):
        """Erstellt die Eingabefelder."""
        
        input_frame = tk.LabelFrame(
            parent,
            text="User Data",
            font=("Arial", 10, "bold"),
            padx=10,
            pady=10
        )
        input_frame.pack(fill=tk.X, pady=(0, 10))
        
        # User ID
        tk.Label(input_frame, text="User ID:", width=10, anchor="w").grid(
            row=0, column=0, sticky="w", pady=5
        )
        self.user_id_entry = tk.Entry(input_frame, width=40)
        self.user_id_entry.grid(row=0, column=1, pady=5, sticky="ew")
        
        # Name
        tk.Label(input_frame, text="Name:", width=10, anchor="w").grid(
            row=1, column=0, sticky="w", pady=5
        )
        self.name_entry = tk.Entry(input_frame, width=40)
        self.name_entry.grid(row=1, column=1, pady=5, sticky="ew")
        
        # Email
        tk.Label(input_frame, text="Email:", width=10, anchor="w").grid(
            row=2, column=0, sticky="w", pady=5
        )
        self.email_entry = tk.Entry(input_frame, width=40)
        self.email_entry.grid(row=2, column=1, pady=5, sticky="ew")
        
        input_frame.columnconfigure(1, weight=1)
    
    def _create_buttons(self, parent: tk.Frame):
        """Erstellt die Action-Buttons."""
        
        button_frame = tk.Frame(parent)
        button_frame.pack(fill=tk.X, pady=(0, 10))
        
        # Button Style
        btn_config = {
            "width": 12,
            "font": ("Arial", 10)
        }
        
        self.create_btn = tk.Button(
            button_frame,
            text="Create",
            command=self._create_user,
            bg="#27ae60",
            fg="white",
            **btn_config
        )
        self.create_btn.pack(side=tk.LEFT, padx=(0, 5))
        
        self.get_btn = tk.Button(
            button_frame,
            text="Get",
            command=self._get_user,
            bg="#3498db",
            fg="white",
            **btn_config
        )
        self.get_btn.pack(side=tk.LEFT, padx=5)
        
        self.update_btn = tk.Button(
            button_frame,
            text="Update",
            command=self._update_user,
            bg="#f39c12",
            fg="white",
            **btn_config
        )
        self.update_btn.pack(side=tk.LEFT, padx=5)
        
        self.delete_btn = tk.Button(
            button_frame,
            text="Delete",
            command=self._delete_user,
            bg="#e74c3c",
            fg="white",
            **btn_config
        )
        self.delete_btn.pack(side=tk.LEFT, padx=5)
        
        # Clear Button
        tk.Button(
            button_frame,
            text="Clear",
            command=self._clear_fields,
            **btn_config
        ).pack(side=tk.RIGHT)
    
    def _create_status_bar(self, parent: tk.Frame):
        """Erstellt die Status-Leiste."""
        
        self.status_label = tk.Label(
            parent,
            text="Ready",
            relief=tk.SUNKEN,
            anchor="w",
            bg="#ecf0f1",
            padx=5,
            pady=5
        )
        self.status_label.pack(fill=tk.X, pady=(0, 10))
    
    def _create_output_area(self, parent: tk.Frame):
        """Erstellt den Output-Bereich."""
        
        output_frame = tk.LabelFrame(
            parent,
            text="Output",
            font=("Arial", 10, "bold"),
            padx=10,
            pady=10
        )
        output_frame.pack(fill=tk.BOTH, expand=True)
        
        self.output_text = scrolledtext.ScrolledText(
            output_frame,
            height=10,
            font=("Courier", 9),
            bg="#f9f9f9"
        )
        self.output_text.pack(fill=tk.BOTH, expand=True)
    
    def _setup_keybindings(self):
        """Richtet Tastenkombinationen ein."""
        self.root.bind("<Control-n>", lambda e: self._create_user())
        self.root.bind("<Control-g>", lambda e: self._get_user())
        self.root.bind("<Control-s>", lambda e: self._update_user())
        self.root.bind("<Control-d>", lambda e: self._delete_user())
        self.root.bind("<Control-q>", lambda e: self.root.quit())
    
    def _check_connection(self):
        """Prüft die Verbindung zu ThemisDB."""
        if self.client.health_check():
            self.connection_status.config(text="● Connected", fg="#27ae60")
            self._set_status("Connected to ThemisDB", "success")
        else:
            self.connection_status.config(text="● Disconnected", fg="#e74c3c")
            self._set_status("Cannot connect to ThemisDB", "error")
    
    def _set_status(self, message: str, status_type: str = "info"):
        """
        Setzt die Status-Nachricht.
        
        Args:
            message: Status-Nachricht
            status_type: Typ (info, success, error)
        """
        colors = {
            "info": "#3498db",
            "success": "#27ae60",
            "error": "#e74c3c"
        }
        self.status_label.config(
            text=message,
            bg=colors.get(status_type, "#ecf0f1")
        )
    
    def _append_output(self, text: str):
        """Fügt Text zum Output-Bereich hinzu."""
        self.output_text.insert(tk.END, text + "\n")
        self.output_text.see(tk.END)
    
    def _clear_output(self):
        """Löscht den Output-Bereich."""
        self.output_text.delete(1.0, tk.END)
    
    def _clear_fields(self):
        """Löscht alle Eingabefelder."""
        self.user_id_entry.delete(0, tk.END)
        self.name_entry.delete(0, tk.END)
        self.email_entry.delete(0, tk.END)
        self._clear_output()
        self._set_status("Fields cleared", "info")
    
    def _validate_inputs(self) -> bool:
        """
        Validiert die Eingaben.
        
        Returns:
            True wenn alle Eingaben gültig sind
        """
        user_id = self.user_id_entry.get().strip()
        name = self.name_entry.get().strip()
        email = self.email_entry.get().strip()
        
        if not user_id:
            messagebox.showerror("Error", "User ID is required")
            return False
        
        if not name:
            messagebox.showerror("Error", "Name is required")
            return False
        
        if not email:
            messagebox.showerror("Error", "Email is required")
            return False
        
        if "@" not in email or "." not in email:
            messagebox.showerror("Error", "Invalid email format")
            return False
        
        return True
    
    def _create_user(self):
        """Erstellt einen neuen Benutzer."""
        if not self._validate_inputs():
            return
        
        user_id = self.user_id_entry.get().strip()
        name = self.name_entry.get().strip()
        email = self.email_entry.get().strip()
        
        try:
            result = self.client.create_user(user_id, name, email)
            self._clear_output()
            self._append_output("✓ User created successfully\n")
            self._append_output(json.dumps(result, indent=2))
            self._set_status(f"User '{user_id}' created", "success")
        except Exception as e:
            self._set_status(f"Error: {str(e)}", "error")
            messagebox.showerror("Error", str(e))
    
    def _get_user(self):
        """Ruft einen Benutzer ab."""
        user_id = self.user_id_entry.get().strip()
        
        if not user_id:
            messagebox.showerror("Error", "User ID is required")
            return
        
        try:
            result = self.client.get_user(user_id)
            
            if result is None:
                self._set_status(f"User '{user_id}' not found", "error")
                messagebox.showwarning("Not Found", f"User '{user_id}' does not exist")
                return
            
            # Felder füllen
            self.name_entry.delete(0, tk.END)
            self.name_entry.insert(0, result.get("name", ""))
            
            self.email_entry.delete(0, tk.END)
            self.email_entry.insert(0, result.get("email", ""))
            
            self._clear_output()
            self._append_output("✓ User retrieved successfully\n")
            self._append_output(json.dumps(result, indent=2))
            self._set_status(f"User '{user_id}' retrieved", "success")
        except Exception as e:
            self._set_status(f"Error: {str(e)}", "error")
            messagebox.showerror("Error", str(e))
    
    def _update_user(self):
        """Aktualisiert einen Benutzer."""
        if not self._validate_inputs():
            return
        
        user_id = self.user_id_entry.get().strip()
        name = self.name_entry.get().strip()
        email = self.email_entry.get().strip()
        
        try:
            result = self.client.update_user(user_id, name, email)
            self._clear_output()
            self._append_output("✓ User updated successfully\n")
            self._append_output(json.dumps(result, indent=2))
            self._set_status(f"User '{user_id}' updated", "success")
        except Exception as e:
            self._set_status(f"Error: {str(e)}", "error")
            messagebox.showerror("Error", str(e))
    
    def _delete_user(self):
        """Löscht einen Benutzer."""
        user_id = self.user_id_entry.get().strip()
        
        if not user_id:
            messagebox.showerror("Error", "User ID is required")
            return
        
        # Bestätigung
        if not messagebox.askyesno("Confirm Delete", f"Delete user '{user_id}'?"):
            return
        
        try:
            self.client.delete_user(user_id)
            self._clear_output()
            self._append_output(f"✓ User '{user_id}' deleted successfully")
            self._set_status(f"User '{user_id}' deleted", "success")
            self._clear_fields()
        except Exception as e:
            self._set_status(f"Error: {str(e)}", "error")
            messagebox.showerror("Error", str(e))


def main():
    """Hauptfunktion."""
    root = tk.Tk()
    app = HelloWorldApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
