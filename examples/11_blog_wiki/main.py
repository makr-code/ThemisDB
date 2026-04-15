"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            main.py                                            ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:05:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     271                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
"""

"""
Blog/Wiki-System - Main Application
Tkinter GUI für Content-Management mit ThemisDB
"""

import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext
from typing import List, Optional
import uuid
from datetime import datetime

from models import Article, Comment, Category, ArticleVersion, ArticleStatus
from themis_client import ThemisDBClient


class BlogWikiApp:
    """Hauptanwendung für Blog/Wiki-System"""
    
    def __init__(self, root: tk.Tk):
        """
        Initialisiert die Anwendung
        
        Args:
            root: Tkinter Root-Fenster
        """
        self.root = root
        self.root.title("Blog/Wiki-System - ThemisDB Example")
        self.root.geometry("1200x800")
        
        # ThemisDB Client
        self.client = ThemisDBClient()
        
        # Aktueller Benutzer (in echter App würde dies über Login kommen)
        self.current_user = "demo_user"
        
        # Aktuell ausgewählter Artikel
        self.current_article: Optional[Article] = None
        
        # UI aufbauen
        self.setup_ui()
        
        # Initiale Daten laden
        self.load_articles()
        
    def setup_ui(self):
        """Erstellt die Benutzeroberfläche"""
        # Menüleiste
        menubar = tk.Menu(self.root)
        self.root.config(menu=menubar)
        
        file_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="File", menu=file_menu)
        file_menu.add_command(label="New Article", command=self.new_article)
        file_menu.add_command(label="Export...", command=self.export_articles)
        file_menu.add_command(label="Import...", command=self.import_articles)
        file_menu.add_separator()
        file_menu.add_command(label="Exit", command=self.root.quit)
        
        # Toolbar
        toolbar = ttk.Frame(self.root)
        toolbar.pack(side=tk.TOP, fill=tk.X, padx=5, pady=5)
        
        ttk.Button(toolbar, text="New Article", command=self.new_article).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="Edit", command=self.edit_article).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="Delete", command=self.delete_article).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="History", command=self.show_history).pack(side=tk.LEFT, padx=2)
        
        ttk.Separator(toolbar, orient=tk.VERTICAL).pack(side=tk.LEFT, fill=tk.Y, padx=5)
        
        # Suchfeld
        ttk.Label(toolbar, text="Search:").pack(side=tk.LEFT, padx=2)
        self.search_var = tk.StringVar()
        self.search_var.trace('w', lambda *args: self.search_articles())
        search_entry = ttk.Entry(toolbar, textvariable=self.search_var, width=30)
        search_entry.pack(side=tk.LEFT, padx=2)
        
        # Hauptcontainer
        main_paned = ttk.PanedWindow(self.root, orient=tk.HORIZONTAL)
        main_paned.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        
        # Linke Seite: Artikel-Liste
        left_frame = ttk.Frame(main_paned)
        main_paned.add(left_frame, weight=1)
        
        ttk.Label(left_frame, text="Articles", font=('Arial', 12, 'bold')).pack(pady=5)
        
        # Artikel-Liste
        self.article_listbox = tk.Listbox(left_frame, height=25)
        self.article_listbox.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        self.article_listbox.bind('<<ListboxSelect>>', self.on_article_select)
        
        # Scrollbar für Liste
        scrollbar = ttk.Scrollbar(left_frame, orient=tk.VERTICAL, command=self.article_listbox.yview)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.article_listbox.config(yscrollcommand=scrollbar.set)
        
        # Rechte Seite: Artikel-Details
        right_frame = ttk.Frame(main_paned)
        main_paned.add(right_frame, weight=3)
        
        # Artikel-Titel
        self.title_label = ttk.Label(right_frame, text="No article selected", 
                                      font=('Arial', 16, 'bold'), wraplength=700)
        self.title_label.pack(pady=10)
        
        # Artikel-Metadaten
        meta_frame = ttk.Frame(right_frame)
        meta_frame.pack(fill=tk.X, padx=10, pady=5)
        self.meta_label = ttk.Label(meta_frame, text="", font=('Arial', 9))
        self.meta_label.pack(side=tk.LEFT)
        
        # Artikel-Inhalt
        content_frame = ttk.Frame(right_frame)
        content_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        self.content_text = scrolledtext.ScrolledText(content_frame, wrap=tk.WORD, 
                                                       font=('Arial', 11), state=tk.DISABLED)
        self.content_text.pack(fill=tk.BOTH, expand=True)
        
        # Statusleiste
        self.status_bar = ttk.Label(self.root, text="Ready", relief=tk.SUNKEN)
        self.status_bar.pack(side=tk.BOTTOM, fill=tk.X)
        
    def load_articles(self):
        """Lädt alle Artikel aus ThemisDB"""
        try:
            # In echter Implementation würde hier ein API-Call zu ThemisDB erfolgen
            # Für jetzt: Demo-Daten
            self.articles: List[Article] = []
            
            # Demo-Artikel erstellen
            demo_article = Article(
                id=str(uuid.uuid4()),
                title="Willkommen im Blog/Wiki-System",
                content="# Willkommen!\n\nDies ist ein Demo-Artikel.",
                author=self.current_user,
                category="Tutorial",
                tags=["demo", "welcome"],
                status=ArticleStatus.PUBLISHED
            )
            self.articles.append(demo_article)
            
            self.refresh_article_list()
            self.status_bar.config(text=f"Loaded {len(self.articles)} articles")
            
        except Exception as e:
            messagebox.showerror("Error", f"Failed to load articles: {str(e)}")
            
    def refresh_article_list(self):
        """Aktualisiert die Artikel-Liste"""
        self.article_listbox.delete(0, tk.END)
        for article in self.articles:
            status_icon = "📝" if article.status == ArticleStatus.DRAFT else "✓"
            self.article_listbox.insert(tk.END, f"{status_icon} {article.title}")
            
    def on_article_select(self, event):
        """Wird aufgerufen wenn ein Artikel ausgewählt wird"""
        selection = self.article_listbox.curselection()
        if not selection:
            return
            
        index = selection[0]
        self.current_article = self.articles[index]
        self.display_article(self.current_article)
        
    def display_article(self, article: Article):
        """Zeigt einen Artikel an"""
        # Titel
        self.title_label.config(text=article.title)
        
        # Metadaten
        meta_text = f"By {article.author} | Category: {article.category} | "
        meta_text += f"Tags: {', '.join(article.tags)} | Version: {article.version}"
        self.meta_label.config(text=meta_text)
        
        # Inhalt
        self.content_text.config(state=tk.NORMAL)
        self.content_text.delete(1.0, tk.END)
        self.content_text.insert(1.0, article.content)
        self.content_text.config(state=tk.DISABLED)
        
    def new_article(self):
        """Erstellt einen neuen Artikel"""
        # In vollständiger Implementation: Dialog für neuen Artikel
        messagebox.showinfo("Info", "New Article dialog would open here")
        
    def edit_article(self):
        """Bearbeitet den aktuellen Artikel"""
        if not self.current_article:
            messagebox.showwarning("Warning", "Please select an article first")
            return
        # In vollständiger Implementation: Editor-Dialog
        messagebox.showinfo("Info", "Edit dialog would open here")
        
    def delete_article(self):
        """Löscht den aktuellen Artikel"""
        if not self.current_article:
            messagebox.showwarning("Warning", "Please select an article first")
            return
            
        if messagebox.askyesno("Confirm", "Delete this article?"):
            # In vollständiger Implementation: API-Call zu ThemisDB
            self.articles.remove(self.current_article)
            self.current_article = None
            self.refresh_article_list()
            self.status_bar.config(text="Article deleted")
            
    def show_history(self):
        """Zeigt Versionshistorie des aktuellen Artikels"""
        if not self.current_article:
            messagebox.showwarning("Warning", "Please select an article first")
            return
        # In vollständiger Implementation: History-Dialog
        messagebox.showinfo("Info", "History dialog would open here")
        
    def search_articles(self):
        """Sucht Artikel basierend auf Suchbegriff"""
        search_term = self.search_var.get().lower()
        if not search_term:
            self.refresh_article_list()
            return
            
        # Filtern nach Titel und Inhalt
        self.article_listbox.delete(0, tk.END)
        for article in self.articles:
            if (search_term in article.title.lower() or 
                search_term in article.content.lower() or
                any(search_term in tag.lower() for tag in article.tags)):
                status_icon = "📝" if article.status == ArticleStatus.DRAFT else "✓"
                self.article_listbox.insert(tk.END, f"{status_icon} {article.title}")
                
    def export_articles(self):
        """Exportiert Artikel"""
        messagebox.showinfo("Info", "Export functionality to be implemented")
        
    def import_articles(self):
        """Importiert Artikel"""
        messagebox.showinfo("Info", "Import functionality to be implemented")


def main():
    """Haupteinstiegspunkt"""
    root = tk.Tk()
    app = BlogWikiApp(root)
    root.mainloop()


if __name__ == "__main__":
    main()
