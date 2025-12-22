"""
E-Commerce Catalog - Main Application
Produktkatalog mit ThemisDB
"""

import tkinter as tk
from tkinter import ttk

class EcommerceApp:
    """Hauptanwendung für E-Commerce Katalog"""
    
    def __init__(self, root: tk.Tk):
        self.root = root
        self.root.title("E-Commerce Catalog - ThemisDB Example")
        self.root.geometry("1200x800")
        self.setup_ui()
        
    def setup_ui(self):
        ttk.Label(self.root, text="E-Commerce Produktkatalog", 
                  font=('Arial', 20, 'bold')).pack(pady=20)
        ttk.Label(self.root, text="Implementation coming soon...",
                  font=('Arial', 12)).pack(pady=10)

def main():
    root = tk.Tk()
    app = EcommerceApp(root)
    root.mainloop()

if __name__ == "__main__":
    main()
