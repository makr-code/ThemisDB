"""
Kanban Board - Main Application
Projektmanagement mit ThemisDB
"""

import tkinter as tk
from tkinter import ttk

class KanbanBoardApp:
    """Hauptanwendung für Kanban Board"""
    
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("Kanban Board - ThemisDB Example")
        self.root.geometry("1400x800")
        self.setup_ui()
        
    def setup_ui(self):
        ttk.Label(self.root, text="Kanban Board / Project Management", 
                  font=('Arial', 20, 'bold')).pack(pady=20)
        ttk.Label(self.root, text="Implementation coming soon...",
                  font=('Arial', 12)).pack(pady=10)

def main():
    root = tk.Tk()
    app = KanbanBoardApp(root)
    root.mainloop()

if __name__ == "__main__":
    main()
