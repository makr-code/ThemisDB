"""
Expense Tracker - Main Application
Haushaltsbuch mit ThemisDB
"""

import tkinter as tk
from tkinter import ttk, messagebox

class ExpenseTrackerApp:
    """Hauptanwendung für Expense Tracker"""
    
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("Expense Tracker - ThemisDB Example")
        self.root.geometry("1000x700")
        
        # UI aufbauen
        self.setup_ui()
        
    def setup_ui(self):
        """Erstellt die Benutzeroberfläche"""
        ttk.Label(self.root, text="Expense Tracker", 
                  font=('Arial', 20, 'bold')).pack(pady=20)
        
        ttk.Label(self.root, text="Implementation coming soon...",
                  font=('Arial', 12)).pack(pady=10)
        
        # Placeholder für Features
        features = [
            "✓ Transaktion erfassen",
            "✓ Budget verwalten",
            "✓ Statistiken anzeigen",
            "✓ Charts visualisieren",
            "✓ Export/Import"
        ]
        
        for feature in features:
            ttk.Label(self.root, text=feature, font=('Arial', 10)).pack(pady=5)

def main():
    root = tk.Tk()
    app = ExpenseTrackerApp(root)
    root.mainloop()

if __name__ == "__main__":
    main()
