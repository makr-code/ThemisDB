"""
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            main_old.py                                        ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:12:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     576                                            ║
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
Moral Philosophy Debates - Main Application

Tkinter GUI for conducting moral philosophy debates on current news topics.
"""

import tkinter as tk
from tkinter import ttk, messagebox, scrolledtext
from typing import Optional, List
from datetime import datetime

from models import (
    NewsArticle, DebateSession, PhilosophySchool,
    PhilosophicalArgument, PHILOSOPHY_PROFILES
)
from news_researcher import NewsResearcher
from moral_engine import MoralDebateEngine, SimpleLLMBackend
from themis_client import MoralDebateClient


# Configuration
THEMIS_HOST = "localhost"
THEMIS_PORT = 8080
WINDOW_WIDTH = 1200
WINDOW_HEIGHT = 800


class MoralDebateApp:
    """
    Main application for moral philosophy debates.
    """
    
    def __init__(self, root: tk.Tk):
        """
        Initialize the application.
        
        Args:
            root: Tkinter root widget
        """
        self.root = root
        self.root.title("ThemisDB - Moral Philosophy Debates")
        self.root.geometry(f"{WINDOW_WIDTH}x{WINDOW_HEIGHT}")
        
        # Initialize components
        self.client = MoralDebateClient(host=THEMIS_HOST, port=THEMIS_PORT)
        self.news_researcher = NewsResearcher()
        self.llm_backend = SimpleLLMBackend()
        self.debate_engine = MoralDebateEngine(llm_backend=self.llm_backend)
        
        # State
        self.current_articles: List[NewsArticle] = []
        self.selected_article: Optional[NewsArticle] = None
        self.current_debate: Optional[DebateSession] = None
        
        # Create UI
        self._create_ui()
        
        # Check connection
        self._check_connection()
        
        # Load initial news
        self._load_news()
    
    def _create_ui(self):
        """Creates the user interface."""
        
        # Header
        self._create_header()
        
        # Main content area (notebook with tabs)
        self.notebook = ttk.Notebook(self.root)
        self.notebook.pack(fill=tk.BOTH, expand=True, padx=10, pady=10)
        
        # Tab 1: News Selection
        self._create_news_tab()
        
        # Tab 2: Debate View
        self._create_debate_tab()
        
        # Tab 3: History
        self._create_history_tab()
        
        # Status bar
        self._create_status_bar()
    
    def _create_header(self):
        """Creates the header with branding."""
        header_frame = tk.Frame(self.root, bg="#2c3e50", height=70)
        header_frame.pack(fill=tk.X)
        header_frame.pack_propagate(False)
        
        # Title
        title_label = tk.Label(
            header_frame,
            text="⚖️ Moral Philosophy Debates",
            bg="#2c3e50",
            fg="white",
            font=("Arial", 18, "bold")
        )
        title_label.pack(side=tk.LEFT, padx=20, pady=15)
        
        # Subtitle
        subtitle_label = tk.Label(
            header_frame,
            text="Ethische Perspektiven auf aktuelle Weltnachrichten",
            bg="#2c3e50",
            fg="#ecf0f1",
            font=("Arial", 10)
        )
        subtitle_label.pack(side=tk.LEFT, padx=(0, 20))
        
        # Connection status
        self.connection_status = tk.Label(
            header_frame,
            text="● Connecting...",
            bg="#2c3e50",
            fg="#f39c12",
            font=("Arial", 10, "bold")
        )
        self.connection_status.pack(side=tk.RIGHT, padx=20)
    
    def _create_news_tab(self):
        """Creates the news selection tab."""
        news_frame = tk.Frame(self.notebook, bg="white")
        self.notebook.add(news_frame, text="📰 Nachrichten")
        
        # Top controls
        control_frame = tk.Frame(news_frame, bg="white")
        control_frame.pack(fill=tk.X, padx=10, pady=10)
        
        tk.Label(
            control_frame,
            text="Kategorie:",
            bg="white",
            font=("Arial", 10)
        ).pack(side=tk.LEFT, padx=(0, 5))
        
        self.category_combo = ttk.Combobox(
            control_frame,
            values=["Alle", "Technology", "Politics", "Health", "Environment", "Society"],
            state="readonly",
            width=15
        )
        self.category_combo.set("Alle")
        self.category_combo.pack(side=tk.LEFT, padx=5)
        
        tk.Button(
            control_frame,
            text="🔄 Aktualisieren",
            command=self._load_news,
            bg="#3498db",
            fg="white",
            font=("Arial", 10),
            padx=15
        ).pack(side=tk.LEFT, padx=10)
        
        tk.Button(
            control_frame,
            text="▶️ Debatte starten",
            command=self._start_debate,
            bg="#27ae60",
            fg="white",
            font=("Arial", 10, "bold"),
            padx=20
        ).pack(side=tk.RIGHT, padx=10)
        
        # News list
        list_frame = tk.Frame(news_frame)
        list_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=(0, 10))
        
        # Scrollbar
        scrollbar = tk.Scrollbar(list_frame)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        # Listbox for news
        self.news_listbox = tk.Listbox(
            list_frame,
            yscrollcommand=scrollbar.set,
            font=("Arial", 10),
            selectmode=tk.SINGLE
        )
        self.news_listbox.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.config(command=self.news_listbox.yview)
        
        self.news_listbox.bind("<<ListboxSelect>>", self._on_news_select)
        
        # Article details
        details_frame = tk.LabelFrame(
            news_frame,
            text="Artikel-Details",
            font=("Arial", 10, "bold"),
            padx=10,
            pady=10
        )
        details_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=(0, 10))
        
        self.article_text = scrolledtext.ScrolledText(
            details_frame,
            height=8,
            font=("Arial", 10),
            wrap=tk.WORD,
            state=tk.DISABLED
        )
        self.article_text.pack(fill=tk.BOTH, expand=True)
    
    def _create_debate_tab(self):
        """Creates the debate view tab."""
        debate_frame = tk.Frame(self.notebook, bg="white")
        self.notebook.add(debate_frame, text="💬 Debatte")
        
        # Debate info
        info_frame = tk.LabelFrame(
            debate_frame,
            text="Debattenthema",
            font=("Arial", 10, "bold"),
            bg="white",
            padx=10,
            pady=10
        )
        info_frame.pack(fill=tk.X, padx=10, pady=10)
        
        self.debate_topic_label = tk.Label(
            info_frame,
            text="Keine aktive Debatte",
            bg="white",
            font=("Arial", 11, "bold"),
            wraplength=WINDOW_WIDTH - 60,
            justify=tk.LEFT
        )
        self.debate_topic_label.pack(fill=tk.X)
        
        self.ethical_question_label = tk.Label(
            info_frame,
            text="",
            bg="white",
            font=("Arial", 10, "italic"),
            wraplength=WINDOW_WIDTH - 60,
            justify=tk.LEFT,
            fg="#7f8c8d"
        )
        self.ethical_question_label.pack(fill=tk.X, pady=(5, 0))
        
        # Philosophy perspectives (scrollable frame)
        perspectives_frame = tk.LabelFrame(
            debate_frame,
            text="Philosophische Perspektiven",
            font=("Arial", 10, "bold"),
            padx=10,
            pady=10
        )
        perspectives_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=(0, 10))
        
        # Canvas for scrolling
        canvas = tk.Canvas(perspectives_frame, bg="white")
        scrollbar = tk.Scrollbar(perspectives_frame, orient=tk.VERTICAL, command=canvas.yview)
        self.perspectives_inner_frame = tk.Frame(canvas, bg="white")
        
        self.perspectives_inner_frame.bind(
            "<Configure>",
            lambda e: canvas.configure(scrollregion=canvas.bbox("all"))
        )
        
        canvas.create_window((0, 0), window=self.perspectives_inner_frame, anchor="nw")
        canvas.configure(yscrollcommand=scrollbar.set)
        
        canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        
        # Consensus section
        consensus_frame = tk.LabelFrame(
            debate_frame,
            text="Konsens-Analyse",
            font=("Arial", 10, "bold"),
            bg="white",
            padx=10,
            pady=10
        )
        consensus_frame.pack(fill=tk.X, padx=10, pady=(0, 10))
        
        self.consensus_label = tk.Label(
            consensus_frame,
            text="Keine Analyse verfügbar",
            bg="white",
            font=("Arial", 10),
            wraplength=WINDOW_WIDTH - 60,
            justify=tk.LEFT
        )
        self.consensus_label.pack(fill=tk.X)
    
    def _create_history_tab(self):
        """Creates the history tab."""
        history_frame = tk.Frame(self.notebook, bg="white")
        self.notebook.add(history_frame, text="📚 Geschichte")
        
        tk.Label(
            history_frame,
            text="Debattenhistorie (Coming Soon)",
            bg="white",
            font=("Arial", 12, "bold"),
            pady=20
        ).pack()
        
        tk.Label(
            history_frame,
            text="Hier werden vergangene Debatten angezeigt.",
            bg="white",
            font=("Arial", 10),
            fg="#7f8c8d"
        ).pack()
    
    def _create_status_bar(self):
        """Creates the status bar."""
        self.status_label = tk.Label(
            self.root,
            text="Bereit",
            relief=tk.SUNKEN,
            anchor="w",
            bg="#ecf0f1",
            padx=5,
            pady=3
        )
        self.status_label.pack(side=tk.BOTTOM, fill=tk.X)
    
    def _check_connection(self):
        """Checks connection to ThemisDB."""
        if self.client.health_check():
            self.connection_status.config(text="● Verbunden", fg="#27ae60")
            self._set_status("Verbunden mit ThemisDB", "success")
        else:
            self.connection_status.config(text="● Getrennt", fg="#e74c3c")
            self._set_status("Keine Verbindung zu ThemisDB (Demo-Modus)", "warning")
    
    def _set_status(self, message: str, status_type: str = "info"):
        """Sets the status message."""
        colors = {
            "info": "#3498db",
            "success": "#27ae60",
            "error": "#e74c3c",
            "warning": "#f39c12"
        }
        self.status_label.config(
            text=message,
            bg=colors.get(status_type, "#ecf0f1")
        )
    
    def _load_news(self):
        """Loads news articles."""
        self._set_status("Lade Nachrichten...", "info")
        
        category = self.category_combo.get()
        if category == "Alle":
            category = "general"
        else:
            category = category.lower()
        
        try:
            self.current_articles = self.news_researcher.fetch_recent_news(
                category=category,
                limit=20
            )
            
            self.news_listbox.delete(0, tk.END)
            for article in self.current_articles:
                display_text = f"{article.title} ({article.source})"
                self.news_listbox.insert(tk.END, display_text)
            
            self._set_status(f"{len(self.current_articles)} Artikel geladen", "success")
        
        except Exception as e:
            self._set_status(f"Fehler beim Laden: {str(e)}", "error")
            messagebox.showerror("Fehler", f"Konnte Nachrichten nicht laden:\n{str(e)}")
    
    def _on_news_select(self, event):
        """Handles news article selection."""
        selection = self.news_listbox.curselection()
        if not selection:
            return
        
        index = selection[0]
        if index < len(self.current_articles):
            self.selected_article = self.current_articles[index]
            self._display_article(self.selected_article)
    
    def _display_article(self, article: NewsArticle):
        """Displays article details."""
        self.article_text.config(state=tk.NORMAL)
        self.article_text.delete(1.0, tk.END)
        
        text = f"Titel: {article.title}\n\n"
        text += f"Quelle: {article.source}\n"
        if article.published_date:
            text += f"Datum: {article.published_date.strftime('%Y-%m-%d')}\n"
        text += f"\n{article.content}\n\n"
        
        if article.ethical_topics:
            text += f"Ethische Themen: {', '.join(article.ethical_topics)}\n"
        
        self.article_text.insert(1.0, text)
        self.article_text.config(state=tk.DISABLED)
    
    def _start_debate(self):
        """Starts a new debate on the selected article."""
        if not self.selected_article:
            messagebox.showwarning(
                "Keine Auswahl",
                "Bitte wählen Sie zuerst einen Nachrichtenartikel aus."
            )
            return
        
        self._set_status("Starte Debatte...", "info")
        
        try:
            # Create debate session
            session = self.debate_engine.create_debate_session(self.selected_article)
            
            # Conduct the debate
            session = self.debate_engine.conduct_debate(session)
            
            self.current_debate = session
            
            # Store in ThemisDB
            self.client.store_debate_session(session)
            
            # Switch to debate tab
            self.notebook.select(1)
            
            # Display debate
            self._display_debate(session)
            
            self._set_status("Debatte erfolgreich durchgeführt", "success")
        
        except Exception as e:
            self._set_status(f"Fehler: {str(e)}", "error")
            messagebox.showerror("Fehler", f"Konnte Debatte nicht starten:\n{str(e)}")
    
    def _display_debate(self, session: DebateSession):
        """Displays the debate results."""
        # Update topic
        self.debate_topic_label.config(text=session.debate_topic)
        self.ethical_question_label.config(text=f"Ethische Frage: {session.ethical_question}")
        
        # Clear previous perspectives
        for widget in self.perspectives_inner_frame.winfo_children():
            widget.destroy()
        
        # Display each argument
        for i, argument in enumerate(session.arguments):
            self._create_argument_widget(argument, i)
        
        # Update consensus
        if session.consensus_reached:
            text = f"✅ Konsens erreicht: {session.consensus_summary}"
            fg = "#27ae60"
        else:
            text = f"⚠️ {session.consensus_summary}"
            fg = "#e67e22"
        
        self.consensus_label.config(text=text, fg=fg)
    
    def _create_argument_widget(self, argument: PhilosophicalArgument, index: int):
        """Creates a widget to display a philosophical argument."""
        profile = PHILOSOPHY_PROFILES[argument.philosophy_school]
        
        # Color scheme based on philosophy
        colors = {
            PhilosophySchool.KANT: "#3498db",
            PhilosophySchool.UTILITARIANISM: "#27ae60",
            PhilosophySchool.DEONTOLOGY: "#9b59b6",
            PhilosophySchool.CONTRACTUALISM: "#e67e22",
            PhilosophySchool.VIRTUE_ETHICS: "#e74c3c"
        }
        color = colors.get(argument.philosophy_school, "#34495e")
        
        # Main frame
        arg_frame = tk.Frame(
            self.perspectives_inner_frame,
            bg="white",
            relief=tk.RAISED,
            borderwidth=1
        )
        arg_frame.pack(fill=tk.X, padx=5, pady=5)
        
        # Header
        header_frame = tk.Frame(arg_frame, bg=color)
        header_frame.pack(fill=tk.X)
        
        tk.Label(
            header_frame,
            text=profile.name,
            bg=color,
            fg="white",
            font=("Arial", 11, "bold"),
            padx=10,
            pady=5
        ).pack(side=tk.LEFT)
        
        # Content
        content_frame = tk.Frame(arg_frame, bg="white", padx=10, pady=10)
        content_frame.pack(fill=tk.BOTH, expand=True)
        
        # Position
        tk.Label(
            content_frame,
            text="Position:",
            bg="white",
            font=("Arial", 9, "bold"),
            anchor="w"
        ).pack(fill=tk.X)
        
        tk.Label(
            content_frame,
            text=argument.position,
            bg="white",
            font=("Arial", 9),
            wraplength=WINDOW_WIDTH - 80,
            justify=tk.LEFT,
            anchor="w"
        ).pack(fill=tk.X, pady=(0, 10))
        
        # Reasoning
        tk.Label(
            content_frame,
            text="Begründung:",
            bg="white",
            font=("Arial", 9, "bold"),
            anchor="w"
        ).pack(fill=tk.X)
        
        reasoning_text = scrolledtext.ScrolledText(
            content_frame,
            height=6,
            font=("Arial", 9),
            wrap=tk.WORD,
            bg="#f9f9f9"
        )
        reasoning_text.insert(1.0, argument.reasoning)
        reasoning_text.config(state=tk.DISABLED)
        reasoning_text.pack(fill=tk.BOTH, expand=True)
    
    def run(self):
        """Runs the application."""
        self.root.mainloop()


def main():
    """Main function."""
    root = tk.Tk()
    app = MoralDebateApp(root)
    app.run()


if __name__ == "__main__":
    main()
